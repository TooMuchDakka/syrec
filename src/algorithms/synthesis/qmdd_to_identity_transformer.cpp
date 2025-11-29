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

#include "dd/Export.hpp"
#include "dd/Operations.hpp"

using namespace syrec;

namespace {
    // TODO: Only for debugging purposes
    void dumpQmdd(const dd::mEdge& edgeToRootOfQmdd, const bool clearPreviousDumpFileContents) {
        std::ofstream ofs;
        if (clearPreviousDumpFileContents) {
            ofs.open("C:\\School\\MThesis\\test.txt", std::ofstream::out | std::ofstream::trunc);
        } else {
            ofs.open("C:\\School\\MThesis\\test.txt", std::ofstream::out | std::ofstream::app);
        }
        dd::serialize(edgeToRootOfQmdd, ofs);
        ofs.flush();
    }
} // namespace

bool QmddToIdentityTransformer::synthesize(dd::mEdge src) {
    if (src.isTerminal()) {
        return false;
    }

    // TODO: Only for debugging purposes
    dumpQmdd(src, true);
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

    // TODO: We should not need such a set since the processing should only move downwards in the QMDD tree?
    // set of nodes that have already been processed.
    std::unordered_set<dd::mEdge> visited{};

    const auto transformationStartTime = std::chrono::steady_clock::now();

    // TODO: Compare with reference algorithm from dd_synthesis
    // TODO: Add handling for garbage/ancillary qubits

    // while there are nodes left to process.
    while (!queue.empty()) {
        const dd::mEdge& current = queue.front();
        queue.pop();

        // shift the paths of the current node.

        // first, increment the reference count of the `src` node,
        // to prevent it from being garbage collected in the `shiftingPaths` call.
        this->qmddPackage.get().incRef(src);

        // perform the shifting paths algorithm.
        shiftPathsFromPPrimeEdgeToNPrimeEdge(src);
        dumpQmdd(src, false);
        // TODO: How can we assert this in release mode.
        assert(src.isIdentity());

        for (const dd::mEdge& edgesOfCurrentNode: src.p->e) {
            if (edgesOfCurrentNode.isTerminal()) {
                continue;
            }
            queue.emplace(edgesOfCurrentNode);
        }
        //
        // const auto srcShifted = shiftPathsFromPPrimeEdgeToNPrimeEdge(src, current);
        // // TODO: Only for debugging purposes
        // dumpQmdd(srcShifted, false);
        // return true;
        //
        // const auto pathsShifted = (srcShifted != src);
        //
        // // TODO: If a new edge is created by a multiplication operation decrementing its reference count will cause it to be potentially deleted
        // //          the subsequent garbage collection operation?
        // // decrement reference count of `src` node again and trigger garbage collection.
        // // this->qmddPackage.get().decRef(src);
        // // this->qmddPackage.get().garbageCollect();
        //
        // if (pathsShifted) {
        //     // stopping criterion
        //     if (srcShifted.isIdentity()) {
        //         break;
        //     }
        //
        //     // if paths were shifted, synthesis starts again from the new `src` node.
        //     src = srcShifted;
        //     visited.clear();
        //     queue = {};
        //     queue.emplace(src);
        //     continue;
        // }
        //
        // // if no paths have been shifted, the children of the current node need to be processed.
        // for (const auto& e: current.p->e) {
        //     if (!e.isTerminal() && !visited.contains(e)) {
        //         queue.emplace(e);
        //         visited.emplace(e);
        //     }
        // }
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
bool QmddToIdentityTransformer::swapPaths(dd::mEdge& src, const QmddPathsStartingFromNode& qmddNodePathSignatures) const {
    assert(!src.isTerminal());
    if (qmddNodePathSignatures.pPrimeEdgePaths.size() <= qmddNodePathSignatures.nEdgePaths.size()) {
        return false;
    }

    const dd::mNode* rootNode = getRootNode(qmddPackage);
    assert(rootNode != nullptr);

    const dd::Qubit targetQubit = qmddNodePathSignatures.associatedQmddNode.get().v;
    if (targetQubit == rootNode->v) {
        const qc::Controls controlQubitsForPathFromRootToCurrentNode;
        applyOperation(targetQubit, src, controlQubitsForPathFromRootToCurrentNode);
    } else {
        // TODO: Iterate all paths from the root to the current node and record the controls for each path P as c(P) then add a toffoli gate TOFF(controls: c(P), target: current)
        for (const QmddPath& pathFromRootToCurrentNode: getAllPathsFromRootToNode(*rootNode, qmddNodePathSignatures.associatedQmddNode)) {
            const qc::Controls controlQubitsForPathFromRootToCurrentNode = getControlsQubitsFromQmddPath(pathFromRootToCurrentNode);
            applyOperation(targetQubit, src, controlQubitsForPathFromRootToCurrentNode);
        }
    }
    return true;
}

// This algorithm moves the unique paths present in the p' edge to the n edge.
// TODO: In the reimplementation this step is not implemented: 'If there are no unique paths in p' edge, the unique paths present in the n' edge are moved to the p edge if required.'
// Refer to the P2 algorithm of http://www.informatik.uni-bremen.de/agra/doc/konf/12aspdac_qmdd_synth_rev.pdf
void QmddToIdentityTransformer::shiftUniquePaths(dd::mEdge& src, QmddPathsStartingFromNode& qmddNodePathSignatures) const {
    assert(!src.isTerminal());
    // Collect all unique paths from the p' edge of the current node by excluding all shared paths from the n edge.
    const std::vector<std::size_t> uniquePathIndicesFromPPrimeEdge = getIndicesOfUniquePathsForQmddNodeEdge(qmddNodePathSignatures.pPrimeEdgePaths, qmddNodePathSignatures.nEdgePaths);
    if (uniquePathIndicesFromPPrimeEdge.empty()) {
        return;
    }

    const dd::mNode* rootNode = getRootNode(qmddPackage);
    assert(rootNode != nullptr);
    const std::vector<QmddPath> pathsFromRootToCurrentNode = getAllPathsFromRootToNode(*rootNode, qmddNodePathSignatures.associatedQmddNode);

    for (const auto& pathIndexInPPrimeEdgeSubtree: uniquePathIndicesFromPPrimeEdge) {
        const QmddPath& uniquePathInPPrimeEdgeSubstree = qmddNodePathSignatures.pPrimeEdgePaths[pathIndexInPPrimeEdgeSubtree];

        // Perform modification of QMDD tree
        const qc::Controls controlQubitsForUniquePathStartingFromNode = getControlsQubitsFromQmddPath(uniquePathInPPrimeEdgeSubstree);
        const qc::Qubit    targetQubit                                = qmddNodePathSignatures.associatedQmddNode.get().v;
        for (const QmddPath& pathFromRootToCurrentNode: pathsFromRootToCurrentNode) {
            qc::Controls controlQubitsToTargetPathFromRootToCurrentNode = getControlsQubitsFromQmddPath(pathFromRootToCurrentNode);
            controlQubitsToTargetPathFromRootToCurrentNode.insert(controlQubitsForUniquePathStartingFromNode.cbegin(), controlQubitsForUniquePathStartingFromNode.cend());
            applyOperation(targetQubit, src, controlQubitsToTargetPathFromRootToCurrentNode);
        }

        // Update internal path signature data structure by adding the unique path of the p' edge to the n edge.
        qmddNodePathSignatures.pPrimeEdgePaths.emplace_back(uniquePathInPPrimeEdgeSubstree);
    }

    // Update internal path signature data structure by removing all unique paths from the p' edge using the erase-remove idiom.
    const std::unordered_set uniquePathsOfPPrimeEdgeLookup(uniquePathIndicesFromPPrimeEdge.cbegin(), uniquePathIndicesFromPPrimeEdge.cend());

    auto       iteratorInPPrimeEdgePaths                   = qmddNodePathSignatures.pPrimeEdgePaths.begin();
    const auto predicateIdentifyingUniquePathsInPPrimeEdge = [&](const QmddPath&) {
        if (iteratorInPPrimeEdgePaths == qmddNodePathSignatures.pPrimeEdgePaths.end()) {
            return false;
        }
        const auto indexOfElementInPPrimePathsContainer = static_cast<std::size_t>(std::distance(qmddNodePathSignatures.pPrimeEdgePaths.begin(), iteratorInPPrimeEdgePaths));
        ++iteratorInPPrimeEdgePaths;
        return uniquePathsOfPPrimeEdgeLookup.contains(indexOfElementInPPrimePathsContainer);
    };
    std::erase_if(qmddNodePathSignatures.pPrimeEdgePaths, predicateIdentifyingUniquePathsInPPrimeEdge);
}

// This algorithm checks whether the p' edge is pointing to zero terminal node.
// TODO: In the reimplementation this step is not implemented: 'This algorithm also checks if n paths == n' paths and p' paths == p paths.'
// Refer to P3 algorithm of http://www.informatik.uni-bremen.de/agra/doc/konf/12aspdac_qmdd_synth_rev.pdf
bool QmddToIdentityTransformer::terminate(const dd::mNode& nodeToCheck) {
    const auto& edgesOfCurrentNode = nodeToCheck.e;
    assert(edgesOfCurrentNode.size() == 4);
    return edgesOfCurrentNode[static_cast<std::size_t>(QmddEdgeIndex::P_Prime_Path)].isZeroTerminal() && edgesOfCurrentNode[static_cast<std::size_t>(QmddEdgeIndex::N_Prime_Path)].isZeroTerminal();
}

void QmddToIdentityTransformer::makePathsOfEdgeOfQmddNodeUnique(dd::mEdge& src, const QmddEdgeIndex processedEdge, QmddPathsStartingFromNode& qmddNodePathSignatures) const {
    assert(processedEdge == QmddEdgeIndex::P_Prime_Path);

    const std::optional<std::size_t> idxOfPathInPPrimeEdgeSubtreeSharedWithNEdgeSubstree = getIndexOfFirstSharedPathBetweenQmddNodeEdgeSubtrees(qmddNodePathSignatures.pPrimeEdgePaths, qmddNodePathSignatures.nEdgePaths);
    assert(idxOfPathInPPrimeEdgeSubtreeSharedWithNEdgeSubstree.has_value());

    // TODO: Determine subsequent vertex that can be used as target qubit t to make the shared path unique. Additionally, all qubits q < t need to be used as control qubits?
    const std::optional<QmddPath> optionalNotOverlappingPathFromPPrimeEdgeSubstree = determineUniquePathFromCollection(qmddNodePathSignatures.pPrimeEdgePaths);
    assert(optionalNotOverlappingPathFromPPrimeEdgeSubstree.has_value());

    const QmddPath& notOverlappingPathFromPPrimeEdgeSubstree = *optionalNotOverlappingPathFromPPrimeEdgeSubstree;
    const QmddPath& sharedPathBetweenNAndPPrimeEdgeSubstree  = qmddNodePathSignatures.pPrimeEdgePaths[*idxOfPathInPPrimeEdgeSubtreeSharedWithNEdgeSubstree];
    assert(sharedPathBetweenNAndPPrimeEdgeSubstree.size() == notOverlappingPathFromPPrimeEdgeSubstree.size());

    std::size_t       startIndexInPathForSearchOfEdgeMismatch = 0;
    const std::size_t pathLengthFromCurrentNodeToTerminal     = notOverlappingPathFromPPrimeEdgeSubstree.size();

    const dd::mNode* rootNode = getRootNode(qmddPackage);
    assert(rootNode != nullptr);
    const std::vector<QmddPath> pathsFromRootToCurrentNode = getAllPathsFromRootToNode(*rootNode, qmddNodePathSignatures.associatedQmddNode);

    qc::Controls controlQubitsForPathFromFirstVertexInPPrimeSubstreeToTargetQubit;
    // Add control qubit for p' edge of current vertex
    controlQubitsForPathFromFirstVertexInPPrimeSubstreeToTargetQubit.emplace(qc::Control(src.p->v, qc::Control::Type::Pos));

    for (std::size_t i = startIndexInPathForSearchOfEdgeMismatch; i < pathLengthFromCurrentNodeToTerminal; ++i) {
        const auto& firstMismatchedPathComponent = std::find_first_of(
                std::next(notOverlappingPathFromPPrimeEdgeSubstree.cbegin(), static_cast<std::ptrdiff_t>(startIndexInPathForSearchOfEdgeMismatch)),
                notOverlappingPathFromPPrimeEdgeSubstree.cend(),
                std::next(sharedPathBetweenNAndPPrimeEdgeSubstree.cbegin(), static_cast<std::ptrdiff_t>(startIndexInPathForSearchOfEdgeMismatch + 1U)),
                sharedPathBetweenNAndPPrimeEdgeSubstree.cend(),
                [](const QmddPathComponent& lPathComponent, const QmddPathComponent& rPathComponent) {
                    return lPathComponent.edgeIndex != rPathComponent.edgeIndex;
                });

        if (firstMismatchedPathComponent == notOverlappingPathFromPPrimeEdgeSubstree.cend()) {
            break;
        }

        const auto idxOfFirstMismatchedPathComponent = std::distance(notOverlappingPathFromPPrimeEdgeSubstree.cbegin(), firstMismatchedPathComponent);
        // Add control qubits for shared path up to first mismatched qubit
        for (auto sharedPathComponentsIterator = std::next(sharedPathBetweenNAndPPrimeEdgeSubstree.begin(), static_cast<std::ptrdiff_t>(startIndexInPathForSearchOfEdgeMismatch));
             sharedPathComponentsIterator != std::next(sharedPathBetweenNAndPPrimeEdgeSubstree.begin(), idxOfFirstMismatchedPathComponent + 1U);
             ++sharedPathComponentsIterator) {
            controlQubitsForPathFromFirstVertexInPPrimeSubstreeToTargetQubit.emplace(
                    sharedPathComponentsIterator->qubitAssociatedWithNodeDefiningOriginOfEdge, getBooleanSignatureComponentForQmddEdge(sharedPathComponentsIterator->edgeIndex) ? qc::Control::Type::Pos : qc::Control::Type::Neg);
        }

        const qc::Qubit targetQubit = firstMismatchedPathComponent->qubitAssociatedWithNodeDefiningOriginOfEdge;
        for (const auto& pathFromRootToCurrentNode: pathsFromRootToCurrentNode) {
            // All control qubits from the root to the current node as well as for the p' edge of the latter are now set.
            qc::Controls controlQubitsForPathFromRootToCurrentNode = getControlsQubitsFromQmddPath(pathFromRootToCurrentNode);
            // Add the subpath from the current vertex up to but excluding the target qubit
            controlQubitsForPathFromRootToCurrentNode.insert(controlQubitsForPathFromFirstVertexInPPrimeSubstreeToTargetQubit.cbegin(), controlQubitsForPathFromFirstVertexInPPrimeSubstreeToTargetQubit.cend());
            // Modify the portion of the shared path
            applyOperation(targetQubit, src, controlQubitsForPathFromRootToCurrentNode);
        }
        // Target qubit is now part of control qubits
        controlQubitsForPathFromFirstVertexInPPrimeSubstreeToTargetQubit.emplace(qc::Control(targetQubit, getBooleanSignatureComponentForQmddEdge(firstMismatchedPathComponent->edgeIndex) ? qc::Control::Type::Pos : qc::Control::Type::Neg));
        // And search continues for next mismatch path component starting at qubit targetQubit + 1
        startIndexInPathForSearchOfEdgeMismatch += static_cast<std::size_t>(idxOfFirstMismatchedPathComponent) + 1U;
    }
    // TODO: We now should be able to remove the shared path from the QmddPathsStartingFromNode container for the p' edge paths and insert the unique path?
}

// This algorithm ensures that the `current` node has the identity structure.
// Refer to algorithm P of http://www.informatik.uni-bremen.de/agra/doc/konf/12aspdac_qmdd_synth_rev.pdf)
void QmddToIdentityTransformer::shiftPathsFromPPrimeEdgeToNPrimeEdge(dd::mEdge& src) const {
    assert(!src.isTerminal());

    const dd::mNode* processedNode = src.p;
    assert(processedNode != nullptr);
    assert(processedNode->e.size() == 4);

    // For the first step of the P algorithm we only need to know the paths from the "n" and "p'" edge of the currently processed node.
    // After the first step is done, the paths for the "n'" and "p" edge can be determined which will save us one swap operation if said first step of the
    // algorithms performs a swap of the paths of the edges of the currently processed node.
    QmddPathsStartingFromNode uniquePathsStartingFromNode = {
            .associatedQmddNode = *src.p,
            .nEdgePaths         = getAllPathsStartingFromNode(QmddEdgeIndex::N_Path, *processedNode->e[static_cast<std::size_t>(QmddEdgeIndex::N_Path)].p),
            .pPrimeEdgePaths    = getAllPathsStartingFromNode(QmddEdgeIndex::P_Prime_Path, *processedNode->e[static_cast<std::size_t>(QmddEdgeIndex::P_Prime_Path)].p),
            .nPrimeEdgePaths    = {},
            .pEdgePaths         = {}};

    // P1 algorithm.
    if (swapPaths(src, uniquePathsStartingFromNode)) {
        // TODO: Swap swapped paths in helper struct
        // At this point in the qmdd the respective swap operation was already performed but we still need to update out helper struct by performing said swap of the paths originated from the currently processed qmdd node.
        // TODO: Instead of a costly swap operation one could also only store references to the path objects in the helper struct.
        std::swap(uniquePathsStartingFromNode.nEdgePaths, uniquePathsStartingFromNode.pPrimeEdgePaths);
        uniquePathsStartingFromNode.nPrimeEdgePaths = getAllPathsStartingFromNode(QmddEdgeIndex::P_Path, *src.p->e[static_cast<std::size_t>(QmddEdgeIndex::P_Path)].p);
        uniquePathsStartingFromNode.pEdgePaths      = getAllPathsStartingFromNode(QmddEdgeIndex::N_Prime_Path, *src.p->e[static_cast<std::size_t>(QmddEdgeIndex::N_Prime_Path)].p);

        std::swap(uniquePathsStartingFromNode.pEdgePaths, uniquePathsStartingFromNode.nPrimeEdgePaths);
    } else {
        uniquePathsStartingFromNode.nPrimeEdgePaths = getAllPathsStartingFromNode(QmddEdgeIndex::N_Prime_Path, *processedNode->e[static_cast<std::size_t>(QmddEdgeIndex::N_Prime_Path)].p);
        uniquePathsStartingFromNode.pEdgePaths      = getAllPathsStartingFromNode(QmddEdgeIndex::P_Path, *processedNode->e[static_cast<std::size_t>(QmddEdgeIndex::P_Path)].p);
    }

    for (;;) {
        // P2 algorithm.
        shiftUniquePaths(src, uniquePathsStartingFromNode);

        // P3 algorithm.
        if (terminate(*src.p)) {
            break;
        }
        // P4 algorithm.
        makePathsOfEdgeOfQmddNodeUnique(src, QmddEdgeIndex::P_Prime_Path, uniquePathsStartingFromNode);

        // if changePaths flag is set, p and n' edges are considered instead of n and p' edges.
        // TODO: Implement me
        // if (changePaths) {
        //     return unifyPath(src, current, uniquePathsStartingFromNode.pEdgePaths, uniquePathsStartingFromNode.nPrimeEdgePaths, changePaths, this->qmddPackage);
        // }
        // return unifyPath(src, current, uniquePathsStartingFromNode.nEdgePaths, uniquePathsStartingFromNode.pPrimeEdgePaths, changePaths, this->qmddPackage);
    }
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

constexpr QmddToIdentityTransformer::QmddEdgeIndex syrec::operator&(const QmddToIdentityTransformer::QmddEdgeIndex lOperand, const QmddToIdentityTransformer::QmddEdgeIndex rOperand) noexcept {
    return static_cast<QmddToIdentityTransformer::QmddEdgeIndex>(static_cast<std::uint8_t>(lOperand) & static_cast<std::uint8_t>(rOperand));
}

constexpr QmddToIdentityTransformer::QmddEdgeIndex syrec::operator|(const QmddToIdentityTransformer::QmddEdgeIndex lOperand, const QmddToIdentityTransformer::QmddEdgeIndex rOperand) noexcept {
    return static_cast<QmddToIdentityTransformer::QmddEdgeIndex>(static_cast<std::uint8_t>(lOperand) | static_cast<std::uint8_t>(rOperand));
}

constexpr void syrec::operator|=(QmddToIdentityTransformer::QmddEdgeIndex& assignedToOperand, const QmddToIdentityTransformer::QmddEdgeIndex rOperand) noexcept {
    assignedToOperand = assignedToOperand | rOperand;
}

std::optional<QmddToIdentityTransformer::QmddPath> QmddToIdentityTransformer::determineUniquePathFromCollection(const std::vector<QmddPath>& qmddPaths) {
    if (qmddPaths.empty()) {
        return std::nullopt;
    }

    std::vector aggregateOfAllPaths(qmddPaths.front().size(), QmddPathComponent({.qubitAssociatedWithNodeDefiningOriginOfEdge = 0U, .edgeIndex = QmddEdgeIndex::N_Path}));
    // TODO: If all paths have the same length (and under the assumption that the variable ordering is from qubit 0 to N - 1) then the variables associated with each path component could be implemented as an
    //  collection with a stepsize of one between its elements starting at qubit i. The path objects then would also not have to store its associated component?
    for (std::size_t i = 0; i < aggregateOfAllPaths.size(); ++i) {
        aggregateOfAllPaths[i].qubitAssociatedWithNodeDefiningOriginOfEdge = qmddPaths.front()[i].qubitAssociatedWithNodeDefiningOriginOfEdge;
    }

    for (const auto& qmddPath: qmddPaths) {
        const std::size_t minElementsToAddFromPath = std::min(qmddPath.size(), aggregateOfAllPaths.size());
        // TODO: All paths in qmdd should have same length but what if identities during creation of qmdd are already removed?
        for (std::size_t i = 0; i < minElementsToAddFromPath; ++i) {
            aggregateOfAllPaths[i].edgeIndex |= qmddPath[i].edgeIndex;
        }
    }

    std::vector uniquePath(aggregateOfAllPaths.size(), QmddPathComponent({.qubitAssociatedWithNodeDefiningOriginOfEdge = 0U, .edgeIndex = QmddEdgeIndex::N_Path}));
    for (std::size_t i = 0; i < aggregateOfAllPaths.size(); ++i) {
        uniquePath[i].qubitAssociatedWithNodeDefiningOriginOfEdge = aggregateOfAllPaths[i].qubitAssociatedWithNodeDefiningOriginOfEdge;
        // Since the aggregate could only consist some of the available edges we need to use the bitwise and operation instead of a bitwise or to get a singular edge instead of another aggregate state that
        // could stem from an aggregation of the subresults via a bitwise or operation.
        uniquePath[i].edgeIndex = aggregateOfAllPaths[i].edgeIndex & QmddEdgeIndex::N_Path;
        uniquePath[i].edgeIndex = aggregateOfAllPaths[i].edgeIndex & QmddEdgeIndex::N_Prime_Path;
        uniquePath[i].edgeIndex = aggregateOfAllPaths[i].edgeIndex & QmddEdgeIndex::P_Prime_Path;
        uniquePath[i].edgeIndex = aggregateOfAllPaths[i].edgeIndex & QmddEdgeIndex::P_Path;
    }
    return uniquePath;
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
std::vector<std::size_t> QmddToIdentityTransformer::getIndicesOfUniquePathsForQmddNodeEdge(const std::vector<QmddPath>& collectionOfPathsToExtractUniqueOnesFrom, const std::vector<QmddPath>& collectionOfPathsUsedToIdentifyDuplicates) {
    std::vector<std::size_t> uniquePathIndices;
    for (std::size_t i = 0; i < collectionOfPathsToExtractUniqueOnesFrom.size(); ++i) {
        if (std::ranges::none_of(collectionOfPathsUsedToIdentifyDuplicates,
                                 [&collectionOfPathsToExtractUniqueOnesFrom, i](const QmddPath& pathForWhichNoOverlapShouldExist) {
                                     return doQmddPathsOverlap(collectionOfPathsToExtractUniqueOnesFrom[i], pathForWhichNoOverlapShouldExist);
                                 })) {
            uniquePathIndices.emplace_back(i);
        }
    }
    return uniquePathIndices;
}

std::optional<std::size_t> QmddToIdentityTransformer::getIndexOfFirstSharedPathBetweenQmddNodeEdgeSubtrees(const std::vector<QmddPath>& collectionOfPathsToFindSharedOneFrom, const std::vector<QmddPath>& collectionUsedToDetermineWhetherDuplicatePathExists) {
    std::optional<std::size_t> indexOfFirstSharedPath;
    for (std::size_t i = 0; i < collectionOfPathsToFindSharedOneFrom.size() && !indexOfFirstSharedPath.has_value(); ++i) {
        indexOfFirstSharedPath = std::ranges::any_of(collectionUsedToDetermineWhetherDuplicatePathExists,
                                                     [&collectionOfPathsToFindSharedOneFrom, i](const QmddPath& pathForWhichOverlapCouldExist) {
                                                         return doQmddPathsOverlap(collectionOfPathsToFindSharedOneFrom[i], pathForWhichOverlapCouldExist);
                                                     }) ?
                                         std::make_optional(i) :
                                         std::nullopt;
    }
    return indexOfFirstSharedPath;
}

const dd::mNode* QmddToIdentityTransformer::getRootNode(dd::Package& qmddPackage) {
    const auto& setOfRootNodes = qmddPackage.getRootSet<dd::mNode>();
    // TODO: The root set often contains more than one entry with the current implementation of the applyOperation function? Is the latter correctly implemented or do
    // we need another mechanism to identify the root?
    const dd::mNode* rootNode = setOfRootNodes.size() == 1 ? setOfRootNodes.cbegin()->first.p : nullptr;
    return rootNode;
}

bool QmddToIdentityTransformer::doQmddPathsOverlap(const QmddPath& lQmddPath, const QmddPath& rQmddPath) {
    const std::size_t smallestPathLength = std::min(lQmddPath.size(), rQmddPath.size());
    if (lQmddPath.empty() || rQmddPath.empty() || lQmddPath.size() > rQmddPath.size()) {
        return false;
    }
    auto truncatedLQmddPath = lQmddPath | std::views::take(smallestPathLength);
    auto truncatedRQmddPath = rQmddPath | std::views::take(smallestPathLength);
    return std::ranges::equal(truncatedLQmddPath, truncatedRQmddPath,
                              [](const QmddPathComponent& lPathComponent, const QmddPathComponent& rPathComponent) {
                                  return lPathComponent.qubitAssociatedWithNodeDefiningOriginOfEdge == rPathComponent.qubitAssociatedWithNodeDefiningOriginOfEdge && lPathComponent.edgeIndex == rPathComponent.edgeIndex;
                              });
}
