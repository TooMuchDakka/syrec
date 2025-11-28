/*
 * Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
 * Copyright (c) 2025 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

#include "algorithms/synthesis/qmdd_to_identity_transformer.hpp"

#include "dd/Operations.hpp"

using namespace syrec;

bool QmddToIdentityTransformer::synthesize(dd::mEdge src) {
    if (src.isTerminal()) {
        return false;
    }

    // this->qc = std::make_unique<qc::QuantumComputation>(src.p->v + 1U);

    // This following ensures that the `src` node resembles an identity structure.
    // Refer to algorithm Q of http://www.informatik.uni-bremen.de/agra/doc/konf/12aspdac_qmdd_synth_rev.pdf.

    // TODO: to preserve the `src` DD throughout the synthesis, its reference count has to be at least 2. (Why?)
    const dd::mNode* rootSet = getRootNode(this->qmddPackage);
    if (!rootSet) {
        // Is this double invocation necessary?
        this->qmddPackage.get().incRef(src);
        this->qmddPackage.get().incRef(src);
    } else {
        this->qmddPackage.get().incRef(src);
    }

    // TODO:
    // if (!rootSet->contains(src)) {
    //     this->qmddPackage.incRef(src);
    //     this->qmddPackage.incRef(src);
    // } else if (rootSet->at(src) == 1U) {
    //     this->qmddPackage.incRef(src);
    // }

    // queue for the nodes to be processed in a breadth-first manner.
    std::queue<dd::mEdge> queue{};
    queue.emplace(src);

    // set of nodes that have already been processed.
    std::unordered_set<dd::mEdge> visited{};

    const auto transformationStartTime = std::chrono::steady_clock::now();

    // TODO: Compare with reference algorithm from dd_synthesis
    // TODO: Add handling for garbage/ancillary qubits

    // while there are nodes left to process.
    while (!queue.empty()) {
        const auto current = queue.front();
        queue.pop();

        // shift the paths of the current node.

        // first, increment the reference count of the `src` node,
        // to prevent it from being garbage collected in the `shiftingPaths` call.
        this->qmddPackage.get().incRef(src);

        // perform the shifting paths algorithm.
        const auto srcShifted   = shiftingPaths(src, current);
        const auto pathsShifted = (srcShifted != src);

        // decrement reference count of `src` node again and trigger garbage collection.
        this->qmddPackage.get().decRef(src);
        this->qmddPackage.get().garbageCollect();

        if (pathsShifted) {
            // stopping criterion
            if (srcShifted.isIdentity()) {
                break;
            }

            // if paths were shifted, synthesis starts again from the new `src` node.
            src = srcShifted;
            visited.clear();
            queue = {};
            queue.emplace(src);
            continue;
        }

        // if no paths have been shifted, the children of the current node need to be processed.
        for (const auto& e: current.p->e) {
            if (!e.isTerminal() && !visited.contains(e)) {
                queue.emplace(e);
                visited.emplace(e);
            }
        }
    }
    const auto transformationEndTime = std::chrono::steady_clock::now();
    // TODO: Update signature to return statistics?
    const auto transformationRuntimeInMs = (transformationEndTime - transformationStartTime).count();
    return true;
}

// This function performs the multi-control (if any) X operation.
void QmddToIdentityTransformer::applyOperation(const qc::Qubit targetBit, dd::mEdge& to, const qc::Controls& ctrl) const {
    // create operation and corresponding decision diagram
    qc.get().mcx(ctrl, targetBit);
    const auto opDD = dd::getDD(*qc.get().back(), qmddPackage);
    const auto tmp  = this->qmddPackage.get().multiply(to, opDD);
    this->qmddPackage.get().incRef(tmp);
    this->qmddPackage.get().decRef(to);
    to = tmp;
    this->qmddPackage.get().garbageCollect();
    // TODO:
    //++numGates;
}

// This algorithm swaps the paths present in the p' edge to the n edge and vice versa.
// TODO: In the reimplementation this check is not implemented: "If n' and p paths exists, we move on to P2 algorithm"
// Refer to the P1 algorithm of http://www.informatik.uni-bremen.de/agra/doc/konf/12aspdac_qmdd_synth_rev.pdf
dd::mEdge QmddToIdentityTransformer::swapPaths(dd::mEdge src, const dd::mEdge& current, const QmddPathsStartingFromNode& qmddNodePathSignatures) {
    assert(!src.isTerminal());
    assert(!current.isTerminal());

    if (qmddNodePathSignatures.pPrimeEdgePaths.size() <= qmddNodePathSignatures.nEdgePaths.size()) {
        return src;
    }

    const dd::mNode* rootNode = getRootNode(qmddPackage);
    assert(rootNode != nullptr);

    // TODO: Iterate all paths from the root to the current node and record the controls for each path P as c(P) then add a toffoli gate TOFF(controls: c(P), target: current)
    for (const auto& pathFromRootToCurrentNode: getAllPathsFromRootToNode(*rootNode, *current.p)) {
        const dd::Qubit    targetQubit                               = current.p->v;
        const qc::Controls controlQubitsForPathFromRootToCurrentNode = getControlsQubitsFromQmddPath(pathFromRootToCurrentNode);
        applyOperation(targetQubit, src, controlQubitsForPathFromRootToCurrentNode);
    }
    return src;
}

// This algorithm moves the unique paths present in the p' edge to the n edge.
// TODO: In the reimplementation this step is not implemented: 'If there are no unique paths in p' edge, the unique paths present in the n' edge are moved to the p edge if required.'
// Refer to the P2 algorithm of http://www.informatik.uni-bremen.de/agra/doc/konf/12aspdac_qmdd_synth_rev.pdf
dd::mEdge QmddToIdentityTransformer::shiftUniquePaths(dd::mEdge src, const dd::mEdge& current, const QmddPathsStartingFromNode& qmddNodePathSignatures) {
    assert(!src.isTerminal());
    assert(!current.isTerminal());

    if (qmddNodePathSignatures.pPrimeEdgePaths.empty()) {
        return src;
    }

    // Collect all unique paths from the p' edge of the current node by excluding all shared paths from the n edge.
    const std::vector<QmddPath> uniquePathsFromPPrimeEdge = getUniquePathsForQmddNodeEdge(qmddNodePathSignatures.pPrimeEdgePaths, qmddNodePathSignatures.nEdgePaths);
    if (uniquePathsFromPPrimeEdge.empty()) {
        return src;
    }

    const dd::mNode* rootNode = getRootNode(qmddPackage);
    assert(rootNode != nullptr);
    const std::vector<QmddPath> pathsFromRootToCurrentNode = getAllPathsFromRootToNode(*rootNode, *current.p);

    for (const auto& uniquePathSignatureStartingFromCurrentNode: uniquePathsFromPPrimeEdge) {
        const qc::Controls controlQubitsForUniquePathStartingFromNode = getControlsQubitsFromQmddPath(uniquePathSignatureStartingFromCurrentNode);
        const qc::Qubit    targetQubit                                = current.p->v;
        for (const QmddPath& pathFromRootToCurrentNode: pathsFromRootToCurrentNode) {
            qc::Controls controlQubitsToTargetPathFromRootToCurrentNode = getControlsQubitsFromQmddPath(pathFromRootToCurrentNode);
            controlQubitsToTargetPathFromRootToCurrentNode.insert(controlQubitsForUniquePathStartingFromNode.cbegin(), controlQubitsForUniquePathStartingFromNode.cend());
            applyOperation(targetQubit, src, controlQubitsToTargetPathFromRootToCurrentNode);
        }
    }
    return src;
}

// This algorithm checks whether the p' edge is pointing to zero terminal node.
// TODO: In the reimplementation this step is not implemented: 'This algorithm also checks if n paths == n' paths and p' paths == p paths.'
// Refer to P3 algorithm of http://www.informatik.uni-bremen.de/agra/doc/konf/12aspdac_qmdd_synth_rev.pdf
bool QmddToIdentityTransformer::terminate(const dd::mNode& nodeToCheck) {
    const auto& edgesOfCurrentNode = nodeToCheck.e;
    assert(edgesOfCurrentNode.size() == 4);
    return edgesOfCurrentNode[static_cast<std::size_t>(QmddEdgeIndex::P_Prime_Path)].isZeroTerminal() && edgesOfCurrentNode[static_cast<std::size_t>(QmddEdgeIndex::N_Prime_Path)].isZeroTerminal();
}

// This algorithm ensures that the `current` node has the identity structure.
// Refer to algorithm P of http://www.informatik.uni-bremen.de/agra/doc/konf/12aspdac_qmdd_synth_rev.pdf)
dd::mEdge QmddToIdentityTransformer::shiftingPaths(const dd::mEdge& src, const dd::mEdge& current) {
    assert(!src.isTerminal());
    assert(!current.isTerminal());

    const dd::mNode* processedNode = current.p;
    assert(processedNode != nullptr);
    assert(processedNode->e.size() == 4);

    const QmddPathsStartingFromNode uniquePathsStartingFromNode = {
            .nEdgePaths      = getAllPathsStartingFromNode(QmddEdgeIndex::N_Path, *processedNode->e[static_cast<std::size_t>(QmddEdgeIndex::N_Path)].p),
            .pPrimeEdgePaths = getAllPathsStartingFromNode(QmddEdgeIndex::P_Prime_Path, *processedNode->e[static_cast<std::size_t>(QmddEdgeIndex::P_Prime_Path)].p),
            .nPrimeEdgePaths = getAllPathsStartingFromNode(QmddEdgeIndex::N_Prime_Path, *processedNode->e[static_cast<std::size_t>(QmddEdgeIndex::N_Prime_Path)].p),
            .pEdgePaths      = getAllPathsStartingFromNode(QmddEdgeIndex::P_Path, *processedNode->e[static_cast<std::size_t>(QmddEdgeIndex::P_Path)].p)};

    // P1 algorithm.
    // TODO: Aggregate paths into helper structure?
    if (const auto srcSwapped = swapPaths(src, current, uniquePathsStartingFromNode); srcSwapped != src) {
        return srcSwapped;
    }

    // P2 algorithm.
    // If there are no unique paths in p' edge, the unique paths present in the n' edge are moved to the p edge if required. changePaths flag is set accordingly.
    if (const auto srcUnique = shiftUniquePaths(src, current, uniquePathsStartingFromNode); srcUnique != src) {
        return srcUnique;
    }

    // P3 algorithm.
    if (terminate(*processedNode)) {
        return src;
    }

    // P4 algorithm.
    // if changePaths flag is set, p and n' edges are considered instead of n and p' edges.
    // TODO: Implement me
    // if (changePaths) {
    //     return unifyPath(src, current, uniquePathsStartingFromNode.pEdgePaths, uniquePathsStartingFromNode.nPrimeEdgePaths, changePaths, this->qmddPackage);
    // }
    // return unifyPath(src, current, uniquePathsStartingFromNode.nEdgePaths, uniquePathsStartingFromNode.pPrimeEdgePaths, changePaths, this->qmddPackage);
    return src;
}

constexpr QmddToIdentityTransformer::QmddEdgeIndex QmddToIdentityTransformer::increment(QmddEdgeIndex& qmddEdgeIndex) noexcept {
    switch (qmddEdgeIndex) {
        case QmddEdgeIndex::N_Path:
            qmddEdgeIndex = QmddEdgeIndex::P_Prime_Path;
            break;
        case QmddEdgeIndex::P_Prime_Path:
            qmddEdgeIndex = QmddEdgeIndex::N_Prime_Path;
            break;
        case QmddEdgeIndex::N_Prime_Path:
            qmddEdgeIndex = QmddEdgeIndex::P_Path;
            break;
        default:
            qmddEdgeIndex = QmddEdgeIndex::N_Path;
            break;
    }
    return qmddEdgeIndex;
}

[[nodiscard]] constexpr bool QmddToIdentityTransformer::getBooleanSignatureComponentForQmddEdge(const QmddEdgeIndex qmddEdgeIndex) noexcept {
    return qmddEdgeIndex == QmddEdgeIndex::P_Path || qmddEdgeIndex == QmddEdgeIndex::P_Prime_Path;
}

// TODO: How should garbage qubits be handled? Can their path components be skipped?
// TODO: One should be able to pass a whole existing path signature as a parameter to define the path from the root to the current node
// TODO: Memoize intermediate results?
[[nodiscard]] TruthTable::Cube::Set QmddToIdentityTransformer::getAllPathSignaturesStartingFromNode(const QmddEdgeIndex qmddPathTakenToReachNode, const dd::mNode& node) {
    assert(node.e.size() == 4);

    TruthTable::Cube::Set collectedPathSignatures;
    TruthTable::Cube      pathSignatureToVisitedNode;
    pathSignatureToVisitedNode.emplace_back(getBooleanSignatureComponentForQmddEdge(qmddPathTakenToReachNode));

    std::stack<QmddEdgeTraversalHelper> toBeVisitedQmddNodes;
    toBeVisitedQmddNodes.emplace(QmddEdgeTraversalHelper({.refToQmddNode = &node, .lastProcessedEdge = QmddEdgeIndex::P_Path, .visitedAllEdgesOfNodeFlag = false}));
    pathSignatureToVisitedNode.emplace_back(getBooleanSignatureComponentForQmddEdge(QmddEdgeIndex::N_Path));

    while (!toBeVisitedQmddNodes.empty()) {
        QmddEdgeTraversalHelper& visitedQmddNode = toBeVisitedQmddNodes.top();
        // If traversal of all edges of node was already performed, move the traversal back to the parent node.
        if (visitedQmddNode.lastProcessedEdge == QmddEdgeIndex::P_Path && visitedQmddNode.visitedAllEdgesOfNodeFlag) {
            toBeVisitedQmddNodes.pop();
            pathSignatureToVisitedNode.pop_back();
            continue;
        }

        pathSignatureToVisitedNode[pathSignatureToVisitedNode.size() - 1] = getBooleanSignatureComponentForQmddEdge(increment(visitedQmddNode.lastProcessedEdge));
        visitedQmddNode.visitedAllEdgesOfNodeFlag                         = visitedQmddNode.lastProcessedEdge == QmddEdgeIndex::P_Path;

        const dd::mEdge& edgeToToBeVisitedChildQmddNode = visitedQmddNode.refToQmddNode->e[static_cast<std::size_t>(visitedQmddNode.lastProcessedEdge)];
        if (edgeToToBeVisitedChildQmddNode.isOneTerminal()) {
            collectedPathSignatures.emplace(pathSignatureToVisitedNode);
        } else if (!edgeToToBeVisitedChildQmddNode.isZeroTerminal()) {
            const dd::mNode* toBeVisitedNode = edgeToToBeVisitedChildQmddNode.p;
            assert(toBeVisitedNode->e.size() == 4);
            toBeVisitedQmddNodes.emplace(QmddEdgeTraversalHelper({.refToQmddNode = toBeVisitedNode, .lastProcessedEdge = QmddEdgeIndex::P_Path, .visitedAllEdgesOfNodeFlag = false}));
            pathSignatureToVisitedNode.emplace_back(getBooleanSignatureComponentForQmddEdge(QmddEdgeIndex::N_Path));
        }
    }
    return collectedPathSignatures;
}

[[nodiscard]] std::vector<QmddToIdentityTransformer::QmddPath> QmddToIdentityTransformer::getAllPathsStartingFromNode(const QmddEdgeIndex qmddPathTakenToReachNode, const dd::mNode& node) {
    assert(node.e.size() == 4);

    std::vector<QmddPath> collectedPaths;
    QmddPath              pathToVisitedNode;
    pathToVisitedNode.emplace_back(getBooleanSignatureComponentForQmddEdge(qmddPathTakenToReachNode));

    std::stack<QmddEdgeTraversalHelper> toBeVisitedQmddNodes;
    toBeVisitedQmddNodes.emplace(QmddEdgeTraversalHelper({.refToQmddNode = &node, .lastProcessedEdge = QmddEdgeIndex::P_Path, .visitedAllEdgesOfNodeFlag = false}));
    pathToVisitedNode.emplace_back(QmddPathComponent({.qubitAssociatedWithNodeDefiningOriginOfEdge = node.v, .edgeIndex = QmddEdgeIndex::P_Path}));

    while (!toBeVisitedQmddNodes.empty()) {
        QmddEdgeTraversalHelper& visitedQmddNode = toBeVisitedQmddNodes.top();
        // If traversal of all edges of node was already performed, move the traversal back to the parent node.
        if (visitedQmddNode.lastProcessedEdge == QmddEdgeIndex::P_Path && visitedQmddNode.visitedAllEdgesOfNodeFlag) {
            toBeVisitedQmddNodes.pop();
            pathToVisitedNode.pop_back();
            continue;
        }

        pathToVisitedNode.back().edgeIndex        = increment(visitedQmddNode.lastProcessedEdge);
        visitedQmddNode.visitedAllEdgesOfNodeFlag = visitedQmddNode.lastProcessedEdge == QmddEdgeIndex::P_Path;

        const dd::mEdge& edgeToToBeVisitedChildQmddNode = visitedQmddNode.refToQmddNode->e[static_cast<std::size_t>(visitedQmddNode.lastProcessedEdge)];
        if (edgeToToBeVisitedChildQmddNode.isOneTerminal()) {
            collectedPaths.emplace_back(pathToVisitedNode);
        } else if (!edgeToToBeVisitedChildQmddNode.isZeroTerminal()) {
            const dd::mNode* toBeVisitedNode = edgeToToBeVisitedChildQmddNode.p;
            assert(toBeVisitedNode->e.size() == 4);
            toBeVisitedQmddNodes.emplace(QmddEdgeTraversalHelper({.refToQmddNode = toBeVisitedNode, .lastProcessedEdge = QmddEdgeIndex::P_Path, .visitedAllEdgesOfNodeFlag = false}));
            pathToVisitedNode.emplace_back(QmddPathComponent({.qubitAssociatedWithNodeDefiningOriginOfEdge = node.v, .edgeIndex = QmddEdgeIndex::P_Path}));
        }
    }
    return collectedPaths;
}

[[nodiscard]] qc::Controls QmddToIdentityTransformer::getControlsQubitsFromQmddPath(const QmddPath& qmddPath) {
    qc::Controls controlQubits;
    std::ranges::transform(qmddPath,
                           std::inserter(controlQubits, controlQubits.end()),
                           [](const QmddPathComponent& qmddPathComponent) {
                               return qc::Control(qmddPathComponent.qubitAssociatedWithNodeDefiningOriginOfEdge, getBooleanSignatureComponentForQmddEdge(qmddPathComponent.edgeIndex) ? qc::Control::Type::Pos : qc::Control::Type::Neg);
                           });
    return controlQubits;
}

[[nodiscard]] std::vector<QmddToIdentityTransformer::QmddPath> QmddToIdentityTransformer::getAllPathsFromRootToNode(const dd::mNode& root, const dd::mNode& node) {
    assert(node.e.size() == 4);

    std::vector<QmddPath> collectedPaths;
    QmddPath              pathToVisitedNode;

    std::stack<QmddEdgeTraversalHelper> toBeVisitedQmddNodes;
    toBeVisitedQmddNodes.emplace(QmddEdgeTraversalHelper({.refToQmddNode = &root, .lastProcessedEdge = QmddEdgeIndex::P_Path, .visitedAllEdgesOfNodeFlag = false}));
    pathToVisitedNode.emplace_back(QmddPathComponent({.qubitAssociatedWithNodeDefiningOriginOfEdge = root.v, .edgeIndex = QmddEdgeIndex::P_Path}));

    while (!toBeVisitedQmddNodes.empty()) {
        QmddEdgeTraversalHelper& visitedQmddNode = toBeVisitedQmddNodes.top();
        // If traversal of all edges of node was already performed, move the traversal back to the parent node.
        if (visitedQmddNode.lastProcessedEdge == QmddEdgeIndex::P_Path && visitedQmddNode.visitedAllEdgesOfNodeFlag) {
            toBeVisitedQmddNodes.pop();
            pathToVisitedNode.pop_back();
            continue;
        }

        pathToVisitedNode.back().edgeIndex        = increment(visitedQmddNode.lastProcessedEdge);
        visitedQmddNode.visitedAllEdgesOfNodeFlag = visitedQmddNode.lastProcessedEdge == QmddEdgeIndex::P_Path;

        const dd::mEdge& edgeToToBeVisitedChildQmddNode = visitedQmddNode.refToQmddNode->e[static_cast<std::size_t>(visitedQmddNode.lastProcessedEdge)];
        if (!edgeToToBeVisitedChildQmddNode.isTerminal() && edgeToToBeVisitedChildQmddNode.p != nullptr) {
            if (edgeToToBeVisitedChildQmddNode.p->v == node.v) {
                const dd::mNode* toBeVisitedNode = edgeToToBeVisitedChildQmddNode.p;
                assert(toBeVisitedNode->e.size() == 4);
                toBeVisitedQmddNodes.emplace(QmddEdgeTraversalHelper({.refToQmddNode = toBeVisitedNode, .lastProcessedEdge = QmddEdgeIndex::P_Path, .visitedAllEdgesOfNodeFlag = false}));
                pathToVisitedNode.emplace_back(QmddPathComponent({.qubitAssociatedWithNodeDefiningOriginOfEdge = toBeVisitedNode->v, .edgeIndex = QmddEdgeIndex::N_Path}));
                pathToVisitedNode.emplace_back(getBooleanSignatureComponentForQmddEdge(QmddEdgeIndex::N_Path));
            } else {
                collectedPaths.emplace_back(pathToVisitedNode);
            }
        }
    }
    return collectedPaths;
}

// TODO: Implement me
std::vector<QmddToIdentityTransformer::QmddPath> QmddToIdentityTransformer::getUniquePathsForQmddNodeEdge(const std::vector<QmddPath>& collectionOfPathsToExtractUniqueOnesFrom, const std::vector<QmddPath>& collectionOfPathsUsedToIdentifyDuplicates) {
    return {};
}

const dd::mNode* QmddToIdentityTransformer::getRootNode(dd::Package& qmddPackage) {
    const auto&      setOfRootNodes = qmddPackage.getRootSet<dd::mNode>();
    const dd::mNode* rootNode       = setOfRootNodes.size() == 1 ? setOfRootNodes.cbegin()->first.p : nullptr;
    return rootNode;
}
