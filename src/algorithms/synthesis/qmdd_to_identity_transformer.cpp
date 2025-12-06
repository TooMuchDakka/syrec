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

bool QmddToIdentityTransformer::synthesize(dd::mEdge src, QmddTransformationStatistics* optionalCollectedStatisticsContainer, const std::string* optionalPathToFileWhichWillContainQmddExport) {
    if (src.isTerminal() || qc.get().getNqubitsWithoutAncillae() == 0) {
        return false;
    }

    // TODO: Is this really necessary?
    // const auto numQubitsWithoutAncillae = static_cast<qc::Qubit>(qc.get().getNqubitsWithoutAncillae());
    // const auto mostSignificantQubitInQuantumComputation = numQubitsWithoutAncillae - 1U;
    // for (qc::Qubit deviceQubit = 0; deviceQubit < numQubitsWithoutAncillae; ++deviceQubit) {
    //     const qc::Qubit circuitQubit = mostSignificantQubitInQuantumComputation - deviceQubit;
    //     qc.get().initialLayout.at(deviceQubit) = circuitQubit;
    //     qc.get().outputPermutation.at(circuitQubit) = deviceQubit;
    //     // Create mapping for initial layout (i.e. from device to circuit qubits).
    //     //qc.get().initialLayout.emplace(std::make_pair(deviceQubit, circuitQubit));
    //     // Create output permutation to map circuit qubits back to device qubits (do we need to insert SWAP gates to perform this permutation)?
    //     //qc.get().outputPermutation.emplace(std::make_pair(circuitQubit, deviceQubit));
    // }

    // This following ensures that the `src` node resembles an identity structure.
    // Refer to algorithm Q of http://www.informatik.uni-bremen.de/agra/doc/konf/12aspdac_qmdd_synth_rev.pdf.
    this->qmddPackage.get().incRef(src);

    // TODO: Only for debugging purposes
    exportQmddToFile(getEdgeToRootNode(this->qmddPackage), optionalPathToFileWhichWillContainQmddExport, true);

    // queue for the nodes to be processed in a breadth-first manner.
    std::queue<dd::mEdge> queue{};
    queue.emplace(src);

    // TODO: We should not need such a set since the processing should only move downwards in the QMDD tree?
    // set of nodes that have already been processed.
    std::unordered_set<dd::mEdge> visited{};

    const auto transformationStartTime = std::chrono::steady_clock::now();

    // TODO: Compare with reference algorithm from dd_synthesis
    // TODO: Add handling for garbage/ancillary qubits

    // TODO: Is the queue really necessary when we are often jumping back to the root in case that an operation was performed?
    // TODO: Due to jumping to the root one could use the visited set to skip already processed subtrees?
    while (!queue.empty()) {
        const dd::mEdge current = queue.front();
        queue.pop();

        assert(current.p != nullptr);
        const auto& nodeToProcess = *current.p;
        assert(current.p->e.size() == 4);

        QmddPathsStartingFromNode pathsStartingFromNode = {
                .associatedQmddNode = nodeToProcess,
                .nEdgePaths         = getAllPathsStartingFromNode(&nodeToProcess, QmddEdgeIndex::N_Path),
                .pPrimeEdgePaths    = getAllPathsStartingFromNode(&nodeToProcess, QmddEdgeIndex::P_Prime_Path),
                .nPrimeEdgePaths    = {},
                .pEdgePaths         = {}};

        // P1 algorithm.
        bool resetQueue = swapPaths(pathsStartingFromNode);
        if (!resetQueue) {
            pathsStartingFromNode.nPrimeEdgePaths = getAllPathsStartingFromNode(&nodeToProcess, QmddEdgeIndex::N_Prime_Path);
            pathsStartingFromNode.pEdgePaths      = getAllPathsStartingFromNode(&nodeToProcess, QmddEdgeIndex::P_Path);
        }
        // P2 algorithm.
        // Note: The E1 |= E2 assignment operator is equal to E1 = E1 | E2 with the operator | not short circuiting does our P algorithms steps would still be evaluated in case the E1 is true thus explaining our usage of the E1 = E1 || E2 assignment.
        resetQueue = resetQueue || shiftUniquePaths(nodeToProcess, pathsStartingFromNode);

        // P3 and P4 algorithm
        resetQueue = resetQueue || (!terminate(nodeToProcess) ? makeSharedPathOfQmddNodeUnique(nodeToProcess, pathsStartingFromNode) : false);
        if (resetQueue) {
            exportQmddToFile(getEdgeToRootNode(this->qmddPackage), optionalPathToFileWhichWillContainQmddExport);

            queue = {};
            if (const dd::mEdge* edgeToRootNodeAfterSwap = getEdgeToRootNode(this->qmddPackage); edgeToRootNodeAfterSwap != nullptr) {
                queue.push(*edgeToRootNodeAfterSwap);
            }
        } else {
            for (const dd::mEdge& edgesOfCurrentNode: nodeToProcess.e) {
                if (edgesOfCurrentNode.isTerminal()) {
                    continue;
                }
                queue.emplace(edgesOfCurrentNode);
            }
        }
    }

    if (optionalCollectedStatisticsContainer != nullptr) {
        const auto transformationFinishedTime                       = std::chrono::steady_clock::now();
        optionalCollectedStatisticsContainer->runtimeInMilliseconds = (transformationFinishedTime - transformationStartTime).count();
    }
    return true;
}

dd::mEdge QmddToIdentityTransformer::constructQmddFromQuantumComputationStartingFromIdentityQmdd(const qc::QuantumComputation& quantumComputation, dd::Package& qmddPackage) {
    // TODO: Implementation taken from dd::FunctionalityConstruction::buildFunctionality(...) which does not apply the inverse of each operation.
    auto permutation    = quantumComputation.initialLayout;
    auto edgeToRootNode = qmddPackage.createInitialMatrix(quantumComputation.getAncillary());

    for (auto op = quantumComputation.crbegin(); op != quantumComputation.crend(); ++op) {
        auto copyOfOperation = op->get()->clone();
        copyOfOperation->invert();
        edgeToRootNode = dd::applyUnitaryOperation(*copyOfOperation, edgeToRootNode, qmddPackage, permutation);
    }

    // correct permutation if necessary
    changePermutation(edgeToRootNode, permutation, quantumComputation.outputPermutation, qmddPackage);
    edgeToRootNode = qmddPackage.reduceAncillae(edgeToRootNode, quantumComputation.getAncillary());
    return qmddPackage.reduceGarbage(edgeToRootNode, quantumComputation.getGarbage());
}

dd::mEdge QmddToIdentityTransformer::applyOperationToQmdd(const qc::Qubit targetQubit, const qc::Controls& controlQubits, const dd::mEdge& currentEdgeToRootNode) const {
    qc.get().mcx(controlQubits, targetQubit);
    const qc::Operation& generatedQuantumOperationForMCXGate = *qc.get().back();
    // TODO:
    //++numGates;
    return dd::applyUnitaryOperation(generatedQuantumOperationForMCXGate, currentEdgeToRootNode, this->qmddPackage, {}, false);
}

// This algorithm swaps the paths present in the p' edge to the n edge and vice versa.
// TODO: In the reimplementation this check is not implemented: "If n' and p paths exists, we move on to P2 algorithm"
// Refer to the P1 algorithm of http://www.informatik.uni-bremen.de/agra/doc/konf/12aspdac_qmdd_synth_rev.pdf
bool QmddToIdentityTransformer::swapPaths(const QmddPathsStartingFromNode& qmddNodePathSignatures) const {
    if (qmddNodePathSignatures.pPrimeEdgePaths.size() <= qmddNodePathSignatures.nEdgePaths.size()) {
        return false;
    }

    const dd::mEdge* edgeToRootNode = getEdgeToRootNode(qmddPackage);
    assert(edgeToRootNode != nullptr);
    assert(edgeToRootNode->p != nullptr);
    const dd::mNode& rootNode = *edgeToRootNode->p;

    const dd::Qubit targetQubit = qmddNodePathSignatures.associatedQmddNode.get().v;
    if (targetQubit == rootNode.v) {
        const qc::Controls controlQubitsForPathFromRootToCurrentNode;
        applyOperationToQmdd(targetQubit, controlQubitsForPathFromRootToCurrentNode, *edgeToRootNode);
    } else {
        // TODO: Iterate all paths from the root to the current node and record the controls for each path P as c(P) then add a toffoli gate TOFF(controls: c(P), target: current)
        for (const QmddPath& pathFromRootToCurrentNode: getAllPathsFromRootToNode(rootNode, qmddNodePathSignatures.associatedQmddNode)) {
            const qc::Controls controlQubitsForPathFromRootToCurrentNode = getControlQubitsForQmddPathFromRootToNode(rootNode.v, pathFromRootToCurrentNode);
            applyOperationToQmdd(targetQubit, controlQubitsForPathFromRootToCurrentNode, *edgeToRootNode);
        }
    }
    return true;
}

// This algorithm moves the unique paths present in the p' edge to the n edge.
// TODO: In the reimplementation this step is not implemented: 'If there are no unique paths in p' edge, the unique paths present in the n' edge are moved to the p edge if required.'
// Refer to the P2 algorithm of http://www.informatik.uni-bremen.de/agra/doc/konf/12aspdac_qmdd_synth_rev.pdf
bool QmddToIdentityTransformer::shiftUniquePaths(const dd::mNode& node, const QmddPathsStartingFromNode& qmddNodePathSignatures) const {
    // Collect all unique paths from the p' edge of the current node by excluding all shared paths from the n edge.
    const std::vector<QmddPath>* qmddNodePathSignaturesSearchedForUniquePath  = &qmddNodePathSignatures.pPrimeEdgePaths;
    std::vector<std::size_t>     uniquePathIndicesInSearchedForPathSignatures = getIndicesOfUniquePathsForQmddNodeEdge(qmddNodePathSignatures.pPrimeEdgePaths, qmddNodePathSignatures.nEdgePaths);
    if (uniquePathIndicesInSearchedForPathSignatures.empty()) {
        // Collect all unique paths from the n' edge of the current node by excluding all shared paths from the p edge.
        uniquePathIndicesInSearchedForPathSignatures = getIndicesOfUniquePathsForQmddNodeEdge(qmddNodePathSignatures.nPrimeEdgePaths, qmddNodePathSignatures.pEdgePaths);
        if (uniquePathIndicesInSearchedForPathSignatures.empty()) {
            return false;
        }
        qmddNodePathSignaturesSearchedForUniquePath = &qmddNodePathSignatures.nPrimeEdgePaths;
    }

    const dd::mEdge* edgeToRootNode = getEdgeToRootNode(qmddPackage);
    assert(edgeToRootNode != nullptr);
    assert(edgeToRootNode->p != nullptr);
    const dd::mNode& rootNode = *edgeToRootNode->p;

    const std::vector<QmddPath> pathsFromRootToCurrentNode = getAllPathsFromRootToNode(rootNode, qmddNodePathSignatures.associatedQmddNode);
    for (const auto& uniquePathIndexInSearchedThroughPathSignatures: uniquePathIndicesInSearchedForPathSignatures) {
        const QmddPath& uniquePathInSearchThroughPathSignatures = qmddNodePathSignaturesSearchedForUniquePath->operator[](uniquePathIndexInSearchedThroughPathSignatures);

        // Perform modification of QMDD tree
        const qc::Controls controlQubitsForUniquePathStartingFromNode = getControlQubitsForQmddPathStartingFromQmddNode(uniquePathInSearchThroughPathSignatures, true);
        const qc::Qubit    targetQubit                                = qmddNodePathSignatures.associatedQmddNode.get().v;

        if (pathsFromRootToCurrentNode.empty()) {
            assert(rootNode.v == node.v);
            applyOperationToQmdd(targetQubit, controlQubitsForUniquePathStartingFromNode, *edgeToRootNode);
        } else {
            for (const QmddPath& pathFromRootToCurrentNode: pathsFromRootToCurrentNode) {
                qc::Controls controlQubitsToTargetPathFromRootToCurrentNode = getControlQubitsForQmddPathFromRootToNode(rootNode.v, pathFromRootToCurrentNode);
                controlQubitsToTargetPathFromRootToCurrentNode.insert(controlQubitsForUniquePathStartingFromNode.cbegin(), controlQubitsForUniquePathStartingFromNode.cend());
                applyOperationToQmdd(targetQubit, controlQubitsToTargetPathFromRootToCurrentNode, *edgeToRootNode);
            }
        }
        // TODO: Currently SHE exception with code 0xc0000005 for multiple paths since QMDD could be changed after an operation is applied.
        break;
    }
    return true;
}

bool QmddToIdentityTransformer::makeSharedPathOfQmddNodeUnique(const dd::mNode& node, const QmddPathsStartingFromNode& qmddNodePathSignatures) const {
    const std::optional<std::size_t> idxOfPathInPPrimeEdgeSubtreeSharedWithNEdgeSubstree = getIndexOfFirstSharedPathBetweenQmddNodeEdgeSubtrees(qmddNodePathSignatures.pPrimeEdgePaths, qmddNodePathSignatures.nEdgePaths);
    if (!idxOfPathInPPrimeEdgeSubtreeSharedWithNEdgeSubstree.has_value()) {
        return false;
    }

    const std::optional<QmddPath> optionalNotOverlappingPathFromPPrimeEdgeSubstree = determineUniquePathFromCollection(qmddNodePathSignatures.pPrimeEdgePaths);
    assert(optionalNotOverlappingPathFromPPrimeEdgeSubstree.has_value());

    const QmddPath&   uniquePathNotExistingInPPrimedgeSubtreeToOneTerminal = *optionalNotOverlappingPathFromPPrimeEdgeSubstree;
    const QmddPath&   sharedPathBetweenNAndPPrimeEdgeSubtreeToOneTerminal  = qmddNodePathSignatures.pPrimeEdgePaths[*idxOfPathInPPrimeEdgeSubtreeSharedWithNEdgeSubstree];
    const std::size_t minimumSharedPathLength                              = std::min(uniquePathNotExistingInPPrimedgeSubtreeToOneTerminal.size(), sharedPathBetweenNAndPPrimeEdgeSubtreeToOneTerminal.size());
    assert(minimumSharedPathLength > 0);

    std::optional<qc::Qubit> targetQubitToMakeSharedPathUnique;
    std::optional<qc::Qubit> controlQubitForEdgeOnSharedPath = node.v;
    qc::Controls             sharedControlQubitsBetweenSharedAndUniquePathStartingFromNode;
    for (std::size_t i = 0; i < minimumSharedPathLength && !targetQubitToMakeSharedPathUnique.has_value(); ++i) {
        const bool doEdgesOnComparedPathsMatch = uniquePathNotExistingInPPrimedgeSubtreeToOneTerminal[i].qubitAssociatedWithQmddNodeThatHasIncomingEdge == sharedPathBetweenNAndPPrimeEdgeSubtreeToOneTerminal[i].qubitAssociatedWithQmddNodeThatHasIncomingEdge && uniquePathNotExistingInPPrimedgeSubtreeToOneTerminal[i].incomingEdgeFromParentQmddNode == sharedPathBetweenNAndPPrimeEdgeSubtreeToOneTerminal[i].incomingEdgeFromParentQmddNode;

        if (doEdgesOnComparedPathsMatch) {
            sharedControlQubitsBetweenSharedAndUniquePathStartingFromNode.emplace(qc::Control(*controlQubitForEdgeOnSharedPath, getControlQubitPolarityForQmddEdge(uniquePathNotExistingInPPrimedgeSubtreeToOneTerminal[i].incomingEdgeFromParentQmddNode)));
            controlQubitForEdgeOnSharedPath = uniquePathNotExistingInPPrimedgeSubtreeToOneTerminal[i].qubitAssociatedWithQmddNodeThatHasIncomingEdge;
        } else {
            targetQubitToMakeSharedPathUnique = controlQubitForEdgeOnSharedPath;
        }
    }
    assert(targetQubitToMakeSharedPathUnique.has_value());
    assert(!sharedControlQubitsBetweenSharedAndUniquePathStartingFromNode.empty());

    const dd::mEdge* edgeToRootNode = getEdgeToRootNode(qmddPackage);
    assert(edgeToRootNode != nullptr);
    assert(edgeToRootNode->p != nullptr);
    const dd::mNode& rootNode = *edgeToRootNode->p;

    for (const auto& pathFromRootToCurrentNode: getAllPathsFromRootToNode(rootNode, qmddNodePathSignatures.associatedQmddNode)) {
        // All control qubits from the root to the current node as well as for the p' edge of the latter are now set.
        qc::Controls controlQubitsFromRootUpToTargetQubit = getControlQubitsForQmddPathFromRootToNode(rootNode.v, pathFromRootToCurrentNode);
        // Add the subpath from the current vertex up to but excluding the target qubit
        controlQubitsFromRootUpToTargetQubit.insert(sharedControlQubitsBetweenSharedAndUniquePathStartingFromNode.cbegin(), sharedControlQubitsBetweenSharedAndUniquePathStartingFromNode.cend());
        // Modify the portion of the shared path
        applyOperationToQmdd(*targetQubitToMakeSharedPathUnique, controlQubitsFromRootUpToTargetQubit, *edgeToRootNode);
    }
    return true;
}

// This algorithm checks whether the p' edge is pointing to zero terminal node.
// TODO: In the reimplementation this step is not implemented: 'This algorithm also checks if n paths == n' paths and p' paths == p paths.'
// Refer to P3 algorithm of http://www.informatik.uni-bremen.de/agra/doc/konf/12aspdac_qmdd_synth_rev.pdf
bool QmddToIdentityTransformer::terminate(const dd::mNode& nodeToCheck) {
    const auto& edgesOfCurrentNode = nodeToCheck.e;
    assert(edgesOfCurrentNode.size() == 4);
    return edgesOfCurrentNode[static_cast<std::size_t>(QmddEdgeIndex::P_Prime_Path)].isZeroTerminal() && edgesOfCurrentNode[static_cast<std::size_t>(QmddEdgeIndex::N_Prime_Path)].isZeroTerminal();
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

constexpr QmddToIdentityTransformer::QmddEdgeIndex QmddToIdentityTransformer::decrement(QmddEdgeIndex& qmddEdgeIndex) noexcept {
    switch (qmddEdgeIndex) {
        case QmddEdgeIndex::N_Path:
            qmddEdgeIndex = QmddEdgeIndex::P_Path;
            break;
        case QmddEdgeIndex::P_Prime_Path:
            qmddEdgeIndex = QmddEdgeIndex::N_Path;
            break;
        case QmddEdgeIndex::N_Prime_Path:
            qmddEdgeIndex = QmddEdgeIndex::P_Prime_Path;
            break;
        default:
            qmddEdgeIndex = QmddEdgeIndex::N_Prime_Path;
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

// TODO: Pass input edge as parameter?
std::optional<QmddToIdentityTransformer::QmddPath> QmddToIdentityTransformer::determineUniquePathFromCollection(const std::vector<QmddPath>& qmddPaths) {
    // TODO: Skip first entries in qmdd paths
    std::vector aggregateOfAllPaths(qmddPaths.front().size(), QmddPathComponent({.qubitAssociatedWithQmddNodeThatHasIncomingEdge = 0U, .incomingEdgeFromParentQmddNode = QmddEdgeIndex::N_Path}));
    // TODO: If all paths have the same length (and under the assumption that the variable ordering is from qubit 0 to N - 1) then the variables associated with each path component could be implemented as an
    //  collection with a stepsize of one between its elements starting at qubit i. The path objects then would also not have to store its associated component?
    for (std::size_t i = 0; i < aggregateOfAllPaths.size(); ++i) {
        aggregateOfAllPaths[i].qubitAssociatedWithQmddNodeThatHasIncomingEdge = qmddPaths.front()[i].qubitAssociatedWithQmddNodeThatHasIncomingEdge;
    }

    for (const auto& qmddPath: qmddPaths) {
        const std::size_t minElementsToAddFromPath = std::min(qmddPath.size(), aggregateOfAllPaths.size());
        // TODO: All paths in qmdd should have same length but what if identities during creation of qmdd are already removed?
        for (std::size_t i = 0; i < minElementsToAddFromPath; ++i) {
            aggregateOfAllPaths[i].incomingEdgeFromParentQmddNode |= qmddPath[i].incomingEdgeFromParentQmddNode;
        }
    }

    std::vector uniquePath(aggregateOfAllPaths.size(), QmddPathComponent({.qubitAssociatedWithQmddNodeThatHasIncomingEdge = 0U, .incomingEdgeFromParentQmddNode = QmddEdgeIndex::N_Path}));
    for (std::size_t i = 0; i < aggregateOfAllPaths.size(); ++i) {
        uniquePath[i].qubitAssociatedWithQmddNodeThatHasIncomingEdge = aggregateOfAllPaths[i].qubitAssociatedWithQmddNodeThatHasIncomingEdge;

        // TODO: We need to use the signature values
        // Since the aggregate could only consist some of the available edges we need to use the bitwise and operation instead of a bitwise or to get a singular edge instead of another aggregate state that
        // could stem from an aggregation of the subresults via a bitwise or operation.
        uniquePath[i].incomingEdgeFromParentQmddNode = aggregateOfAllPaths[i].incomingEdgeFromParentQmddNode & QmddEdgeIndex::N_Path;
        uniquePath[i].incomingEdgeFromParentQmddNode = aggregateOfAllPaths[i].incomingEdgeFromParentQmddNode & QmddEdgeIndex::N_Prime_Path;
        uniquePath[i].incomingEdgeFromParentQmddNode = aggregateOfAllPaths[i].incomingEdgeFromParentQmddNode & QmddEdgeIndex::P_Prime_Path;
        uniquePath[i].incomingEdgeFromParentQmddNode = aggregateOfAllPaths[i].incomingEdgeFromParentQmddNode & QmddEdgeIndex::P_Path;
    }
    return uniquePath;
}

[[nodiscard]] std::vector<QmddToIdentityTransformer::QmddPath> QmddToIdentityTransformer::getAllPathsStartingFromNode(const dd::mNode* node, const QmddEdgeIndex qmddPathToTake) {
    if (node == nullptr) {
        return {};
    }
    assert(node->e.size() == 4);

    const auto& firstEdgeToProcessedFromNode = node->e[static_cast<std::size_t>(qmddPathToTake)];
    if (firstEdgeToProcessedFromNode.isTerminal()) {
        if (firstEdgeToProcessedFromNode.isZeroTerminal()) {
            return {};
        }
        return std::vector(1, QmddPath({QmddPathComponent({.qubitAssociatedWithQmddNodeThatHasIncomingEdge = node->v, .incomingEdgeFromParentQmddNode = qmddPathToTake})}));
    }

    std::vector<QmddPath>               collectedPaths;
    std::stack<QmddEdgeTraversalHelper> toBeVisitedQmddNodes({QmddEdgeTraversalHelper({.refToQmddNode = firstEdgeToProcessedFromNode.p, .lastProcessedEdge = QmddEdgeIndex::P_Path, .visitedAllEdgesOfNodeFlag = false})});
    QmddPath                            pathToVisitedNode({QmddPathComponent({.qubitAssociatedWithQmddNodeThatHasIncomingEdge = node->v, .incomingEdgeFromParentQmddNode = qmddPathToTake})});

    while (!toBeVisitedQmddNodes.empty()) {
        QmddEdgeTraversalHelper& visitedQmddNode = toBeVisitedQmddNodes.top();
        // If traversal of all edges of node was already performed, move the traversal back to the parent node.
        if (visitedQmddNode.lastProcessedEdge == QmddEdgeIndex::P_Path && visitedQmddNode.visitedAllEdgesOfNodeFlag) {
            toBeVisitedQmddNodes.pop();
            if (!pathToVisitedNode.empty()) {
                pathToVisitedNode.pop_back();
            }
            continue;
        }

        //pathToVisitedNode.back().incomingEdgeFromParentQmddNode = increment(visitedQmddNode.lastProcessedEdge);
        increment(visitedQmddNode.lastProcessedEdge);
        visitedQmddNode.visitedAllEdgesOfNodeFlag = visitedQmddNode.lastProcessedEdge == QmddEdgeIndex::P_Path;

        // TODO: What if refToQmddNode is null?
        const dd::mEdge& edgeToToBeVisitedChildQmddNode = visitedQmddNode.refToQmddNode->e[static_cast<std::size_t>(visitedQmddNode.lastProcessedEdge)];
        if (edgeToToBeVisitedChildQmddNode.isOneTerminal()) {
            pathToVisitedNode.emplace_back(QmddPathComponent({.qubitAssociatedWithQmddNodeThatHasIncomingEdge = visitedQmddNode.refToQmddNode->v, .incomingEdgeFromParentQmddNode = visitedQmddNode.lastProcessedEdge}));
            collectedPaths.emplace_back(pathToVisitedNode);
            pathToVisitedNode.pop_back();
        } else if (!edgeToToBeVisitedChildQmddNode.isZeroTerminal()) {
            const dd::mNode* toBeVisitedNode = edgeToToBeVisitedChildQmddNode.p;
            assert(toBeVisitedNode->e.size() == 4);
            toBeVisitedQmddNodes.emplace(QmddEdgeTraversalHelper({.refToQmddNode = toBeVisitedNode, .lastProcessedEdge = QmddEdgeIndex::P_Path, .visitedAllEdgesOfNodeFlag = false}));
            pathToVisitedNode.emplace_back(QmddPathComponent({.qubitAssociatedWithQmddNodeThatHasIncomingEdge = visitedQmddNode.refToQmddNode->v, .incomingEdgeFromParentQmddNode = visitedQmddNode.lastProcessedEdge}));
        }
    }
    return collectedPaths;
}

[[nodiscard]] qc::Controls QmddToIdentityTransformer::getControlQubitsForQmddPathFromRootToNode(const qc::Qubit qubitAssociatedToQmddRootNode, const QmddPath& qmddPath) {
    qc::Controls controlQubits;
    if (qmddPath.size() < 2) {
        return qc::Controls();
    }

    qc::Qubit controlQubit = qubitAssociatedToQmddRootNode;
    for (std::size_t i = 0; i <= qmddPath.size() - 1; ++i) {
        controlQubits.emplace(qc::Control(controlQubit, getControlQubitPolarityForQmddEdge(qmddPath[i].incomingEdgeFromParentQmddNode)));
        controlQubit = qmddPath[i].qubitAssociatedWithQmddNodeThatHasIncomingEdge.value_or(0U);
    }
    return controlQubits;
}

qc::Controls QmddToIdentityTransformer::getControlQubitsForQmddPathStartingFromQmddNode(const QmddPath& qmddPath, const bool skipFirstPathEntry) {
    if (skipFirstPathEntry && qmddPath.size() < 2) {
        return {};
    }

    qc::Controls controlQubits;
    std::transform(
            skipFirstPathEntry ? std::next(qmddPath.cbegin()) : qmddPath.cbegin(),
            qmddPath.cend(),
            std::inserter(controlQubits, controlQubits.end()),
            [](const QmddPathComponent& qmddPathComponent) {
                return qc::Control(*qmddPathComponent.qubitAssociatedWithQmddNodeThatHasIncomingEdge, getControlQubitPolarityForQmddEdge(qmddPathComponent.incomingEdgeFromParentQmddNode));
            });
    return controlQubits;
}

[[nodiscard]] std::vector<QmddToIdentityTransformer::QmddPath> QmddToIdentityTransformer::getAllPathsFromRootToNode(const dd::mNode& root, const dd::mNode& node) {
    if (root.v == node.v) {
        return {};
    }

    assert(node.e.size() == 4);
    std::vector<QmddPath> collectedPaths;
    QmddPath              pathToVisitedNode;

    std::stack<QmddEdgeTraversalHelper> toBeVisitedQmddNodes;
    toBeVisitedQmddNodes.emplace(QmddEdgeTraversalHelper({.refToQmddNode = &root, .lastProcessedEdge = QmddEdgeIndex::P_Path, .visitedAllEdgesOfNodeFlag = false}));
    //pathToVisitedNode.emplace_back(QmddPathComponent({.qubitAssociatedWithQmddNodeThatHasIncomingEdge = getQubitOfQmddNodeReachedByEdge(root, Qmdd) root.e[static_cast<std::size_t>(QmddEdgeIndex::N_Path)].p->v, .incomingEdgeFromParentQmddNode = QmddEdgeIndex::P_Path}));

    while (!toBeVisitedQmddNodes.empty()) {
        QmddEdgeTraversalHelper& visitedQmddNode = toBeVisitedQmddNodes.top();
        // If traversal of all edges of node was already performed, move the traversal back to the parent node.
        if (visitedQmddNode.lastProcessedEdge == QmddEdgeIndex::P_Path && visitedQmddNode.visitedAllEdgesOfNodeFlag) {
            toBeVisitedQmddNodes.pop();
            pathToVisitedNode.pop_back();
            continue;
        }

        increment(visitedQmddNode.lastProcessedEdge);
        visitedQmddNode.visitedAllEdgesOfNodeFlag = visitedQmddNode.lastProcessedEdge == QmddEdgeIndex::P_Path;

        const dd::mEdge& edgeToToBeVisitedChildQmddNode = visitedQmddNode.refToQmddNode->e[static_cast<std::size_t>(visitedQmddNode.lastProcessedEdge)];
        if (!edgeToToBeVisitedChildQmddNode.isTerminal() && edgeToToBeVisitedChildQmddNode.p != nullptr) {
            if (edgeToToBeVisitedChildQmddNode.p->v == node.v) {
                pathToVisitedNode.emplace_back(QmddPathComponent({.qubitAssociatedWithQmddNodeThatHasIncomingEdge = std::nullopt, .incomingEdgeFromParentQmddNode = visitedQmddNode.lastProcessedEdge}));
                collectedPaths.emplace_back(pathToVisitedNode);
            } else {
                const dd::mNode* toBeVisitedNode = edgeToToBeVisitedChildQmddNode.p;

                assert(toBeVisitedNode == nullptr || toBeVisitedNode->e.size() == 4);
                toBeVisitedQmddNodes.emplace(QmddEdgeTraversalHelper({.refToQmddNode = toBeVisitedNode, .lastProcessedEdge = QmddEdgeIndex::P_Path, .visitedAllEdgesOfNodeFlag = false}));
                pathToVisitedNode.emplace_back(QmddPathComponent({.qubitAssociatedWithQmddNodeThatHasIncomingEdge = toBeVisitedNode != nullptr ? std::make_optional(toBeVisitedNode->v) : std::nullopt, .incomingEdgeFromParentQmddNode = visitedQmddNode.lastProcessedEdge}));
            }
        }
    }
    return collectedPaths;
}

// TODO: Implement me
std::vector<std::size_t> QmddToIdentityTransformer::getIndicesOfUniquePathsForQmddNodeEdge(const std::vector<QmddPath>& collectionOfPathsToExtractUniqueOnesFrom, const std::vector<QmddPath>& collectionOfPathsUsedToIdentifyDuplicates) {
    std::vector<std::size_t> uniquePathIndices;
    // TODO: One could use the std::views::iota(0U, collectionOfPathsToExtractUniqueOnesFrom.size() - 1U) | std::views::filter(...) | std::ranges::to<std::vector<std::size_t>>() construct to implement the same functionality but the latter pipe does not compile at the moment.
    for (std::size_t i = 0; i < collectionOfPathsToExtractUniqueOnesFrom.size(); ++i) {
        if (const QmddPath& qmddPathToCheckForUniqueness = collectionOfPathsToExtractUniqueOnesFrom[i]; qmddPathToCheckForUniqueness.size() > 1 && !existsQmddPathWithSameSignatureInCollection(qmddPathToCheckForUniqueness, collectionOfPathsUsedToIdentifyDuplicates)) {
            uniquePathIndices.emplace_back(i);
        }
    }
    return uniquePathIndices;
}

std::optional<std::size_t> QmddToIdentityTransformer::getIndexOfFirstSharedPathBetweenQmddNodeEdgeSubtrees(const std::vector<QmddPath>& collectionOfPathsToFindSharedOneFrom, const std::vector<QmddPath>& collectionUsedToDetermineWhetherDuplicatePathExists) {
    std::optional<std::size_t> indexOfFirstSharedPath;
    for (std::size_t i = 0; i < collectionOfPathsToFindSharedOneFrom.size() && !indexOfFirstSharedPath.has_value(); ++i) {
        const QmddPath& qmddPathToCheckForDuplicates = collectionOfPathsToFindSharedOneFrom[i];
        indexOfFirstSharedPath                       = qmddPathToCheckForDuplicates.size() > 1 && existsQmddPathWithSameSignatureInCollection(qmddPathToCheckForDuplicates, collectionUsedToDetermineWhetherDuplicatePathExists) ? std::make_optional(i) : std::nullopt;
    }
    return indexOfFirstSharedPath;
}

// TODO: root set should always only contain one node?
const dd::mEdge* QmddToIdentityTransformer::getEdgeToRootNode(dd::Package& qmddPackage) {
    const auto& setOfRootNodes = qmddPackage.getRootSet<dd::mNode>();
    return !setOfRootNodes.empty() ? &setOfRootNodes.begin()->first : nullptr;
}

const dd::mNode* QmddToIdentityTransformer::getRootNode(dd::Package& qmddPackage) {
    const dd::mEdge* edgeToRootNode = getEdgeToRootNode(qmddPackage);
    return edgeToRootNode != nullptr ? edgeToRootNode->p : nullptr;
}

bool QmddToIdentityTransformer::existsQmddPathWithSameSignatureInCollection(const QmddPath& potentiallyUniqueQmddPath, const std::vector<QmddPath>& qmddPathCollectionToSearchThrough) {
    if (qmddPathCollectionToSearchThrough.empty() || potentiallyUniqueQmddPath.size() < 2) {
        return false;
    }

    const std::span<const QmddPathComponent> trimmedPotentiallyUniquePath(potentiallyUniqueQmddPath | std::views::drop(1));
    return std::ranges::any_of(
            qmddPathCollectionToSearchThrough,
            [trimmedPotentiallyUniquePath](const QmddPath& qmddPathFromSearchThroughCollection) {
                const std::span<const QmddPathComponent> trimmedQmddPathInSearchThroughCollection(qmddPathFromSearchThroughCollection | std::views::drop(1));
                return doQmddPathsOverlap(trimmedPotentiallyUniquePath, trimmedQmddPathInSearchThroughCollection);
            });
}

bool QmddToIdentityTransformer::doQmddPathsOverlap(const std::span<const QmddPathComponent>& lQmddPath, const std::span<const QmddPathComponent>& rQmddPath) {
    const std::size_t smallestPathLength = std::min(lQmddPath.size(), rQmddPath.size());
    // TODO: Can we simply truncate the paths here or do we need to skip not relevant entries in the longer path?
    auto truncatedLQmddPath = lQmddPath | std::views::take(smallestPathLength);
    auto truncatedRQmddPath = rQmddPath | std::views::take(smallestPathLength);
    return (lQmddPath.empty() && rQmddPath.empty()) || std::ranges::equal(truncatedLQmddPath, truncatedRQmddPath,
                                                                          [](const QmddPathComponent& lPathComponent, const QmddPathComponent& rPathComponent) {
                                                                              return lPathComponent.qubitAssociatedWithQmddNodeThatHasIncomingEdge == rPathComponent.qubitAssociatedWithQmddNodeThatHasIncomingEdge &&
                                                                                     getBooleanSignatureComponentForQmddEdge(lPathComponent.incomingEdgeFromParentQmddNode) == getBooleanSignatureComponentForQmddEdge(rPathComponent.incomingEdgeFromParentQmddNode);
                                                                          });
}

std::optional<qc::Qubit> QmddToIdentityTransformer::getQubitOfQmddNodeReachedByEdge(const dd::mNode* qmddNodeBeingOriginOfEdge, QmddEdgeIndex edgeToTake) {
    if (qmddNodeBeingOriginOfEdge == nullptr) {
        return std::nullopt;
    }
    assert(qmddNodeBeingOriginOfEdge->e.size() == 4);
    const dd::mNode* childNode = qmddNodeBeingOriginOfEdge->e[static_cast<std::size_t>(edgeToTake)].p;
    return childNode != nullptr ? std::make_optional(childNode->v) : std::nullopt;
}

constexpr qc::Control::Type QmddToIdentityTransformer::getControlQubitPolarityForQmddEdge(QmddEdgeIndex qmddEdge) noexcept {
    return getBooleanSignatureComponentForQmddEdge(qmddEdge) ? qc::Control::Type::Pos : qc::Control::Type::Neg;
}

std::vector<QmddToIdentityTransformer::QmddPath> QmddToIdentityTransformer::generatePathsForQmddNodeWithOnlyTerminalNodeChildren(const QmddEdgeIndex qmddEdgeToNode, const dd::mNode& qmddNode) {
    std::vector<QmddPath> qmddPathsStartingFromNode;
    for (const auto edge: {QmddEdgeIndex::N_Path, QmddEdgeIndex::P_Prime_Path, QmddEdgeIndex::N_Prime_Path, QmddEdgeIndex::P_Path}) {
        if (qmddNode.e[static_cast<std::size_t>(edge)].p == nullptr) {
            continue;
        }
        qmddPathsStartingFromNode.emplace_back(QmddPath({QmddPathComponent({.qubitAssociatedWithQmddNodeThatHasIncomingEdge = qmddNode.v, .incomingEdgeFromParentQmddNode = qmddEdgeToNode}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNodeThatHasIncomingEdge = std::nullopt, .incomingEdgeFromParentQmddNode = edge})}));
    }
    return qmddPathsStartingFromNode;
}

void QmddToIdentityTransformer::exportQmddToFile(const dd::mEdge* edgeToRootOfQmdd, const std::string* optionalPathToFileWhichWillContainQmddExport, const bool clearContentsOfFileBeforeExport) {
    if (edgeToRootOfQmdd == nullptr || optionalPathToFileWhichWillContainQmddExport == nullptr) {
        return;
    }

    const int     outputStreamFlags = std::ofstream::out | (clearContentsOfFileBeforeExport ? std::ofstream::trunc : std::ofstream::app);
    std::ofstream ofs;
    ofs.open(*optionalPathToFileWhichWillContainQmddExport, outputStreamFlags);
    if (!ofs.good()) {
        return;
    }
    dd::serialize(*edgeToRootOfQmdd, ofs);
}
