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

bool QmddToIdentityTransformer::synthesize(dd::mEdge src, QmddTransformationStatistics* optionalCollectedStatisticsContainer, const std::optional<QmddDumpConfig>& optionalQmddDumpConfig) {
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

    const std::optional<QmddDumpConfig> intermediateQmddTransformationsDumpConfig = optionalQmddDumpConfig.has_value() ? std::make_optional(QmddDumpConfig(optionalQmddDumpConfig->pathToFileToDumpQmddTo, false)) : std::nullopt;
    // TODO: Only for debugging purposes
    exportQmddToFile(getEdgeToRootNode(this->qmddPackage), optionalQmddDumpConfig);

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

        bool                      skipQmddDump          = false;
        QmddPathsStartingFromNode pathsStartingFromNode = {.associatedQmddNode = nodeToProcess, .nEdgePaths = {}, .pPrimeEdgePaths = {}, .nPrimeEdgePaths = {}, .pEdgePaths = {}};
        if (!terminate(nodeToProcess)) {
            pathsStartingFromNode.nEdgePaths      = getAllPathsStartingFromNode(&nodeToProcess, QmddEdgeIndex::N_Path);
            pathsStartingFromNode.pPrimeEdgePaths = getAllPathsStartingFromNode(&nodeToProcess, QmddEdgeIndex::P_Prime_Path);
        } else {
            skipQmddDump = true;
        }

        // P1 algorithm.
        bool resetQueue = skipQmddDump || swapPaths(pathsStartingFromNode);
        if (!resetQueue) {
            pathsStartingFromNode.nPrimeEdgePaths = getAllPathsStartingFromNode(&nodeToProcess, QmddEdgeIndex::N_Prime_Path);
            pathsStartingFromNode.pEdgePaths      = getAllPathsStartingFromNode(&nodeToProcess, QmddEdgeIndex::P_Path);
        }
        // P2 algorithm.
        // Note: The E1 |= E2 assignment operator is equal to E1 = E1 | E2 with the operator | not short circuiting does our P algorithms steps would still be evaluated in case the E1 is true thus explaining our usage of the E1 = E1 || E2 assignment.
        resetQueue = resetQueue || shiftUniquePaths(nodeToProcess, pathsStartingFromNode);

        // P3 and P4 algorithm
        resetQueue = resetQueue || (!terminate(nodeToProcess) ? makeSharedPathOfQmddNodeUnique(nodeToProcess, pathsStartingFromNode) : false);
        if (!skipQmddDump && resetQueue) {
            exportQmddToFile(getEdgeToRootNode(this->qmddPackage), intermediateQmddTransformationsDumpConfig);

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

dd::mEdge QmddToIdentityTransformer::constructQmddFromQuantumComputationStartingFromIdentityQmdd(const qc::QuantumComputation& quantumComputation, dd::Package& qmddPackage, const std::optional<QmddDumpConfig>& optionalQmddDumpConfig) {
    // TODO: Implementation taken from dd::FunctionalityConstruction::buildFunctionality(...) which does not apply the inverse of each operation.
    auto permutation    = quantumComputation.initialLayout;
    auto edgeToRootNode = qmddPackage.createInitialMatrix(quantumComputation.getAncillary());

    for (auto op = quantumComputation.crbegin(); op != quantumComputation.crend(); ++op) {
        auto copyOfOperation = op->get()->clone();
        copyOfOperation->invert();
        // TODO: What is the difference between applying the unitary operation from the left/right?
        edgeToRootNode = dd::applyUnitaryOperation(*copyOfOperation, edgeToRootNode, qmddPackage, permutation, false);
        exportQmddToFile(&edgeToRootNode, optionalQmddDumpConfig);
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
            assert(!pathFromRootToCurrentNode.empty());
            const qc::Controls controlQubitsForPathFromRootToCurrentNode = getControlQubitsForQmddPathFromRootToNode(pathFromRootToCurrentNode);
            applyOperationToQmdd(targetQubit, controlQubitsForPathFromRootToCurrentNode, *edgeToRootNode);
            // TODO: Application of QMDD operation can change structure of QMDD thus previously determined paths may no longer exist
            break;
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
                assert(!pathFromRootToCurrentNode.empty());
                qc::Controls controlQubitsToTargetPathFromRootToCurrentNode = getControlQubitsForQmddPathFromRootToNode(pathFromRootToCurrentNode);
                controlQubitsToTargetPathFromRootToCurrentNode.insert(controlQubitsForUniquePathStartingFromNode.cbegin(), controlQubitsForUniquePathStartingFromNode.cend());
                applyOperationToQmdd(targetQubit, controlQubitsToTargetPathFromRootToCurrentNode, *edgeToRootNode);
                // TODO: Application of QMDD operation can change structure of QMDD thus previously determined paths may no longer exist
                break;
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

    const QmddPath& sharedQmddPathBetweenSubtrees = qmddNodePathSignatures.pPrimeEdgePaths[*idxOfPathInPPrimeEdgeSubtreeSharedWithNEdgeSubstree];
    assert(sharedQmddPathBetweenSubtrees.size() > 1);

    const dd::mEdge* edgeToRootNode = getEdgeToRootNode(qmddPackage);
    assert(edgeToRootNode != nullptr);
    assert(edgeToRootNode->p != nullptr);
    const dd::mNode& rootNode = *edgeToRootNode->p;

    const std::optional<std::pair<qc::Controls, qc::Qubit>> controlAndTargetQubitsToMakeSharedQmddPathUnique = determineControlAndTargetQubitsToMakeSharedQmddPathUnique(sharedQmddPathBetweenSubtrees, qmddNodePathSignatures.nEdgePaths);
    assert(controlAndTargetQubitsToMakeSharedQmddPathUnique.has_value());
    const auto [controlQubitsForPathFromQmddNodeUpToButExcludingTargetQubit, targetQubitToMakeSharedQmddPathUnique] = *controlAndTargetQubitsToMakeSharedQmddPathUnique;

    for (const auto& pathFromRootToCurrentNode: getAllPathsFromRootToNode(rootNode, qmddNodePathSignatures.associatedQmddNode)) {
        assert(!pathFromRootToCurrentNode.empty());
        // All control qubits from the root to the current node as well as for the p' edge of the latter are now set.
        qc::Controls controlQubitsFromRootUpToTargetQubit = getControlQubitsForQmddPathFromRootToNode(pathFromRootToCurrentNode);
        // Add the subpath from the current vertex up to but excluding the target qubit
        controlQubitsFromRootUpToTargetQubit.insert(controlQubitsForPathFromQmddNodeUpToButExcludingTargetQubit.cbegin(), controlQubitsForPathFromQmddNodeUpToButExcludingTargetQubit.cend());
        // Modify the portion of the shared path
        applyOperationToQmdd(targetQubitToMakeSharedQmddPathUnique, controlQubitsFromRootUpToTargetQubit, *edgeToRootNode);
        // TODO: Application of QMDD operation can change structure of QMDD thus previously determined paths may no longer exist
        break;
    }

    // if (node.v == rootNode.v) {
    //     applyOperationToQmdd(targetQubitToMakeSharedQmddPathUnique, controlQubitsForPathFromQmddNodeUpToButExcludingTargetQubit, *edgeToRootNode);
    // } else {
    //     for (const auto& pathFromRootToCurrentNode: getAllPathsFromRootToNode(rootNode, qmddNodePathSignatures.associatedQmddNode)) {
    //         assert(!pathFromRootToCurrentNode.empty());
    //         // All control qubits from the root to the current node as well as for the p' edge of the latter are now set.
    //         qc::Controls controlQubitsFromRootUpToTargetQubit = getControlQubitsForQmddPathFromRootToNode(pathFromRootToCurrentNode);
    //         // Add the subpath from the current vertex up to but excluding the target qubit
    //         controlQubitsFromRootUpToTargetQubit.insert(controlQubitsForPathFromQmddNodeUpToButExcludingTargetQubit.cbegin(), controlQubitsForPathFromQmddNodeUpToButExcludingTargetQubit.cend());
    //         // Modify the portion of the shared path
    //         applyOperationToQmdd(targetQubitToMakeSharedQmddPathUnique, controlQubitsFromRootUpToTargetQubit, *edgeToRootNode);
    //         // TODO: Application of QMDD operation can change structure of QMDD thus previously determined paths may no longer exist
    //         break;
    //     }
    // }
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

// TODO: Pass input edge as parameter?, we are assuming that qmdd paths start in same qmdd node (more like targeting the same qubit since the single path and path collection stem from the subtree of two different edges)
std::optional<std::pair<qc::Controls, qc::Qubit>> QmddToIdentityTransformer::determineControlAndTargetQubitsToMakeSharedQmddPathUnique(const QmddPath& sharedQmddPath, const std::vector<QmddPath>& qmddPaths) {
    if (qmddPaths.empty() || sharedQmddPath.size() < 2) {
        return std::nullopt;
    }

    std::vector<std::span<const QmddPathComponent>> nonEmptyQmddPathsAfterCurrentNode;
    nonEmptyQmddPathsAfterCurrentNode.reserve(qmddPaths.size());

    // To match the qubit ordering of the vertices in the QMDD (most significant bit has highest qubit index) we need to use the std::greater key compare function instead of the default std::less.
    std::map<qc::Qubit, QmddEdgeIndex, std::greater<>> aggregateOfAllPaths;
    for (const auto& qmddPath: qmddPaths) {
        if (qmddPath.size() < 2) {
            continue;
        }

        for (std::size_t i = 1; i < qmddPath.size(); ++i) {
            const QmddPathComponent& qmddPathComponent = qmddPath[i];
            if (auto existingRecordedEntryForQubit = aggregateOfAllPaths.find(qmddPathComponent.qubitAssociatedWithQmddNodeThatIsOriginOfEdge); existingRecordedEntryForQubit != aggregateOfAllPaths.cend()) {
                existingRecordedEntryForQubit->second |= qmddPathComponent.outgoingEdgeIndex;
            } else {
                aggregateOfAllPaths[qmddPathComponent.qubitAssociatedWithQmddNodeThatIsOriginOfEdge] = qmddPathComponent.outgoingEdgeIndex;
            }
        }
        nonEmptyQmddPathsAfterCurrentNode.emplace_back(qmddPath | std::views::drop(1));
    }
    std::ranges::sort(nonEmptyQmddPathsAfterCurrentNode, [](const std::span<const QmddPathComponent>& viewToLQmddPath, const std::span<const QmddPathComponent>& viewToRQmddPath) { return viewToLQmddPath.size() < viewToRQmddPath.size(); });

    // TODO:
    // Find shared qmdd path component at the 'lowest' level in the qmdd for the currently processed shared path. Choosing the lowest level instead of the highest is a currently chosen heuristic.
    std::optional<std::size_t> indexToSharedPathComponentAtLowestLevelInQmdd;
    // TODO: Is it sufficient to only check on of the paths
    const std::span<const QmddPathComponent> nonUniquePath = nonEmptyQmddPathsAfterCurrentNode.front();
    for (auto potentiallySharedPathComponent = nonUniquePath.rbegin(); potentiallySharedPathComponent != nonUniquePath.rend() && !indexToSharedPathComponentAtLowestLevelInQmdd.has_value(); ++potentiallySharedPathComponent) {
        auto matchingPathComponentInAggregatePath     = aggregateOfAllPaths.find(potentiallySharedPathComponent->qubitAssociatedWithQmddNodeThatIsOriginOfEdge);
        indexToSharedPathComponentAtLowestLevelInQmdd = matchingPathComponentInAggregatePath == aggregateOfAllPaths.cend() || getBooleanSignatureComponentForQmddEdge(potentiallySharedPathComponent->outgoingEdgeIndex) == getBooleanSignatureComponentForQmddEdge(matchingPathComponentInAggregatePath->second) ? indexToSharedPathComponentAtLowestLevelInQmdd : (nonUniquePath.size() - (1U + static_cast<std::size_t>(std::distance(nonUniquePath.rbegin(), potentiallySharedPathComponent))));
    }
    assert(indexToSharedPathComponentAtLowestLevelInQmdd.has_value());

    qc::Controls controlsForSharedPathComponents;
    for (std::size_t i = 0; i < *indexToSharedPathComponentAtLowestLevelInQmdd; ++i) {
        const QmddPathComponent& sharedPathComponent = sharedQmddPath[i];
        controlsForSharedPathComponents.emplace(qc::Control(sharedPathComponent.qubitAssociatedWithQmddNodeThatIsOriginOfEdge, getControlQubitPolarityForQmddEdge(sharedPathComponent.outgoingEdgeIndex)));
    }

    const qc::Qubit targetQubit = sharedQmddPath[1U + *indexToSharedPathComponentAtLowestLevelInQmdd].qubitAssociatedWithQmddNodeThatIsOriginOfEdge;
    // TODO: Do we need this second portion of the shared path after the target qubit?
    // for (std::size_t i = *indexToSharedPathComponentAtLowestLevelInQmdd + 1U; i < sharedQmddPath.size(); ++i) {
    //     const QmddPathComponent& sharedPathComponent = sharedQmddPath[i];
    //     controlsForSharedPathComponents.emplace(qc::Control(*sharedPathComponent.qubitAssociatedWithQmddNodeThatHasIncomingEdge, getControlQubitPolarityForQmddEdge(sharedPathComponent.incomingEdgeFromParentQmddNode)));
    // }
    return std::make_pair(controlsForSharedPathComponents, targetQubit);
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
        return std::vector(1, QmddPath({QmddPathComponent({.qubitAssociatedWithQmddNodeThatIsOriginOfEdge = node->v, .outgoingEdgeIndex = qmddPathToTake})}));
    }

    std::vector<QmddPath>               collectedPaths;
    std::stack<QmddEdgeTraversalHelper> toBeVisitedQmddNodes({QmddEdgeTraversalHelper({.refToQmddNode = firstEdgeToProcessedFromNode.p, .lastProcessedEdge = QmddEdgeIndex::P_Path, .visitedAllEdgesOfNodeFlag = false})});
    QmddPath                            pathToVisitedNode({QmddPathComponent({.qubitAssociatedWithQmddNodeThatIsOriginOfEdge = node->v, .outgoingEdgeIndex = qmddPathToTake})});

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
            pathToVisitedNode.emplace_back(QmddPathComponent({.qubitAssociatedWithQmddNodeThatIsOriginOfEdge = visitedQmddNode.refToQmddNode->v, .outgoingEdgeIndex = visitedQmddNode.lastProcessedEdge}));
            collectedPaths.emplace_back(pathToVisitedNode);
            pathToVisitedNode.pop_back();
        } else if (!edgeToToBeVisitedChildQmddNode.isZeroTerminal()) {
            const dd::mNode* toBeVisitedNode = edgeToToBeVisitedChildQmddNode.p;
            assert(toBeVisitedNode->e.size() == 4);
            toBeVisitedQmddNodes.emplace(QmddEdgeTraversalHelper({.refToQmddNode = toBeVisitedNode, .lastProcessedEdge = QmddEdgeIndex::P_Path, .visitedAllEdgesOfNodeFlag = false}));
            pathToVisitedNode.emplace_back(QmddPathComponent({.qubitAssociatedWithQmddNodeThatIsOriginOfEdge = visitedQmddNode.refToQmddNode->v, .outgoingEdgeIndex = visitedQmddNode.lastProcessedEdge}));
        }
    }
    return collectedPaths;
}

[[nodiscard]] qc::Controls QmddToIdentityTransformer::getControlQubitsForQmddPathFromRootToNode(const QmddPath& qmddPath) {
    qc::Controls controlQubits;
    std::ranges::transform(qmddPath,
                           std::inserter(controlQubits, controlQubits.end()),
                           [](const QmddPathComponent& qmddPathComponent) {
                               return qc::Control(qmddPathComponent.qubitAssociatedWithQmddNodeThatIsOriginOfEdge, getControlQubitPolarityForQmddEdge(qmddPathComponent.outgoingEdgeIndex));
                           });
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
                return qc::Control(qmddPathComponent.qubitAssociatedWithQmddNodeThatIsOriginOfEdge, getControlQubitPolarityForQmddEdge(qmddPathComponent.outgoingEdgeIndex));
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

        increment(visitedQmddNode.lastProcessedEdge);
        visitedQmddNode.visitedAllEdgesOfNodeFlag = visitedQmddNode.lastProcessedEdge == QmddEdgeIndex::P_Path;

        const dd::mEdge& edgeToToBeVisitedChildQmddNode = visitedQmddNode.refToQmddNode->e[static_cast<std::size_t>(visitedQmddNode.lastProcessedEdge)];
        if (!edgeToToBeVisitedChildQmddNode.isTerminal() && edgeToToBeVisitedChildQmddNode.p != nullptr && edgeToToBeVisitedChildQmddNode.p->v >= node.v) {
            if (edgeToToBeVisitedChildQmddNode.p->v == node.v) {
                if (edgeToToBeVisitedChildQmddNode.p == &node) {
                    pathToVisitedNode.emplace_back(QmddPathComponent({.qubitAssociatedWithQmddNodeThatIsOriginOfEdge = visitedQmddNode.refToQmddNode->v, .outgoingEdgeIndex = visitedQmddNode.lastProcessedEdge}));
                    collectedPaths.emplace_back(pathToVisitedNode);
                }
            } else {
                const dd::mNode* toBeVisitedNode = edgeToToBeVisitedChildQmddNode.p;
                assert(toBeVisitedNode->e.size() == 4);

                toBeVisitedQmddNodes.emplace(QmddEdgeTraversalHelper({.refToQmddNode = toBeVisitedNode, .lastProcessedEdge = QmddEdgeIndex::P_Path, .visitedAllEdgesOfNodeFlag = false}));
                pathToVisitedNode.emplace_back(QmddPathComponent({.qubitAssociatedWithQmddNodeThatIsOriginOfEdge = visitedQmddNode.refToQmddNode->v, .outgoingEdgeIndex = visitedQmddNode.lastProcessedEdge}));
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

// TODO: Combine with getRootNode
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
                                                                              return lPathComponent.qubitAssociatedWithQmddNodeThatIsOriginOfEdge == rPathComponent.qubitAssociatedWithQmddNodeThatIsOriginOfEdge &&
                                                                                     getBooleanSignatureComponentForQmddEdge(lPathComponent.outgoingEdgeIndex) == getBooleanSignatureComponentForQmddEdge(rPathComponent.outgoingEdgeIndex);
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

void QmddToIdentityTransformer::exportQmddToFile(const dd::mEdge* edgeToRootOfQmdd, const std::optional<QmddDumpConfig>& optionalQmddDumpConfig) {
    if (edgeToRootOfQmdd == nullptr || !optionalQmddDumpConfig.has_value()) {
        return;
    }

    // TODO: How should I/O errors be handled when exceptions should not be thrown?
    const int     outputStreamFlags = std::ofstream::out | (optionalQmddDumpConfig->clearContentsOfFileBeforeExport ? std::ofstream::trunc : std::ofstream::app);
    std::ofstream ofs;
    ofs.open(optionalQmddDumpConfig->pathToFileToDumpQmddTo, outputStreamFlags);
    if (!ofs.good()) {
        return;
    }
    dd::serialize(*edgeToRootOfQmdd, ofs);
}
