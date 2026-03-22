/*
 * Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
 * Copyright (c) 2025 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

#include "algorithms/synthesis/qmddTransformer.hpp"

#include "../../../out/build/x64-Debug/_deps/spdlog-src/include/spdlog/fmt/bundled/compile.h"
#include "dd/Export.hpp"
#include "dd/Operations.hpp"

#include <cmath>
#include <queue>

using namespace syrec;

namespace {

} // namespace

bool QmddTransformer::synthesizeQmdd(dd::mEdge edgeToQmddRoot, QmddTransformationStatistic* optionalTransformationStatistics, const std::optional<QmddDumpConfig>& optionalQmddDumpConfig) {
    if (edgeToQmddRoot.isTerminal() || qc.get().getNqubitsWithoutAncillae() == 0) {
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
    qmddPkg.get().incRef(edgeToQmddRoot);

    // const std::optional<QmddDumpConfig> intermediateQmddTransformationsDumpConfig = optionalQmddDumpConfig.has_value() ? std::make_optional(QmddDumpConfig(optionalQmddDumpConfig->pathToFileToDumpQmddTo, false)) : std::nullopt;
    // TODO: Only for debugging purposes
    exportQmddToFile(tryGetEdgeToQmddRootNode(qmddPkg), optionalQmddDumpConfig, QmddExportOutputStreamOperation::OverwriteExisting);

    // queue for the nodes to be processed in a breadth-first manner.
    std::queue<dd::mEdge> queue{};
    queue.emplace(edgeToQmddRoot);

    // TODO: We should not need such a set since the processing should only move downwards in the QMDD tree?
    // set of nodes that have already been processed.
    // std::unordered_set<dd::mEdge> visited{};

    //const auto transformationStartTime = std::chrono::steady_clock::now();

    // TODO: Compare with reference algorithm from dd_synthesis
    // TODO: Add handling for garbage/ancillary qubits

    // TODO: Is the queue really necessary when we are often jumping back to the root in case that an operation was performed?
    // TODO: Due to jumping to the root one could use the visited set to skip already processed subtrees?
    while (!queue.empty()) {
        const dd::mEdge current = queue.front();
        queue.pop();

        assert(current.p != nullptr);
        const dd::mNode& nodeToProcess = *current.p;
        assert(current.p->e.size() == 4);

        // if (terminate(nodeToProcess)) {
        //     break;
        // }

        // TODO: In test_dd_synthesis_1 some of the found paths contain duplicate entries that are associated with the same qubit but a different edge.
        QmddNodeAndPathsPerEdge qmddPathsStartingFromNode = {.associatedQmddNode = nodeToProcess, .nEdgePaths = {}, .pPrimeEdgePaths = {}, .nPrimeEdgePaths = {}, .pEdgePaths = {}};
        getPathsToOneTerminalThroughEdgeOfQmddNode(nodeToProcess, QmddNodeEdge::N | QmddNodeEdge::P_Prime, qmddPathsStartingFromNode);
        // P1 algorithm
        bool resetQueue = trySwapPathsOfEdgesOfQmddNode(qmddPathsStartingFromNode);
        if (!resetQueue) {
            getPathsToOneTerminalThroughEdgeOfQmddNode(nodeToProcess, QmddNodeEdge::N_Prime | QmddNodeEdge::P, qmddPathsStartingFromNode);
        }
        // P2 algorithm.
        // Note: The E1 |= E2 assignment operator is equal to E1 = E1 | E2 with the operator | not short circuiting does our P algorithms steps would still be evaluated in case the E1 is true thus explaining our usage of the E1 = E1 || E2 assignment.
        resetQueue = resetQueue || tryShiftUniquePathsOfQmddNode(qmddPathsStartingFromNode);

        // TODO: In the original paper the algorithm should continue with step P2 after P4 was performed but this might not take into account that the structure of the QMDD has changed after the associated operation was executed.
        // P3 and P4 algorithm
        resetQueue = resetQueue || (!terminate(nodeToProcess) ? tryMakeSharedPathOfQmddNodeUnique(qmddPathsStartingFromNode) : false);
        if (resetQueue) {
            // The resetQueue variable should be set to true if any of the steps P1, P2, P3 or P4 applied an operation thus the qmdd export should dump the qmdd after said operation was applied thus allowing a "single-step" debugging with the dump file contents if necessary.
            exportQmddToFile(tryGetEdgeToQmddRootNode(qmddPkg), optionalQmddDumpConfig);

            queue = {};
            if (const dd::mEdge* edgeToRootNodeAfterSwap = tryGetEdgeToQmddRootNode(qmddPkg); edgeToRootNodeAfterSwap != nullptr) {
                queue.emplace(*edgeToRootNodeAfterSwap);
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

    // if (optionalCollectedStatisticsContainer != nullptr) {
    //     const auto transformationFinishedTime                       = std::chrono::steady_clock::now();
    //     optionalCollectedStatisticsContainer->runtimeInMilliseconds = (transformationFinishedTime - transformationStartTime).count();
    // }
    return true;
}

dd::mEdge QmddTransformer::constructQmddFromGatesOfQuantumComputation(const qc::QuantumComputation& quantumComputation, dd::Package& qmddPackage, const std::optional<QmddDumpConfig>& optionalQmddDumpConfig) {
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

void QmddTransformer::exportQmddToFile(const dd::mEdge* edgeToRootNodeOfQmdd, const std::optional<QmddDumpConfig>& optionalQmddDumpConfig, const QmddExportOutputStreamOperation qmddExportOutputStreamOperation) {
    if (edgeToRootNodeOfQmdd == nullptr || !optionalQmddDumpConfig.has_value()) {
        return;
    }

    // TODO: How should I/O errors be handled when exceptions should not be thrown?
    const int     outputStreamFlags = std::ofstream::out | (qmddExportOutputStreamOperation == QmddExportOutputStreamOperation::OverwriteExisting ? std::ofstream::trunc : std::ofstream::app);
    std::ofstream ofs;
    ofs.open(optionalQmddDumpConfig->pathToDumpFile, outputStreamFlags);
    if (!ofs.good()) {
        return;
    }
    dd::serialize(*edgeToRootNodeOfQmdd, ofs);
}

// TODO: Make static?
dd::mEdge QmddTransformer::applyMCXGateToQmdd(const dd::mEdge& edgeToRootNodeOfQmdd, const qc::Qubit targetQubit, const qc::Controls& controlQubits) const {
    qc.get().mcx(controlQubits, targetQubit);
    const qc::Operation& generatedQuantumOperationForMCXGate = *qc.get().back();
    // TODO:
    //++numGates;
    return dd::applyUnitaryOperation(generatedQuantumOperationForMCXGate, edgeToRootNodeOfQmdd, qmddPkg, {}, false);
}

// This algorithm swaps the paths present in the p' edge to the n edge and vice versa.
// TODO: In the reimplementation this check is not implemented: "If n' and p paths exists, we move on to P2 algorithm"
// Refer to the P1 algorithm of http://www.informatik.uni-bremen.de/agra/doc/konf/12aspdac_qmdd_synth_rev.pdf
bool QmddTransformer::trySwapPathsOfEdgesOfQmddNode(QmddNodeAndPathsPerEdge& qmddNodeAndEdgePaths) const {
    if (getNumberOfPathsToOneTerminalForQmddPaths(qmddNodeAndEdgePaths.pPrimeEdgePaths) <= getNumberOfPathsToOneTerminalForQmddPaths(qmddNodeAndEdgePaths.nEdgePaths)) {
        return false;
    }

    const dd::mEdge* edgeToRootNode = tryGetEdgeToQmddRootNode(this->qmddPkg);
    assert(edgeToRootNode != nullptr);
    assert(edgeToRootNode->p != nullptr);
    const dd::mNode& rootNode = *edgeToRootNode->p;

    const dd::Qubit targetQubit = qmddNodeAndEdgePaths.associatedQmddNode.get().v;
    if (targetQubit == rootNode.v) {
        const qc::Controls controlQubitsForPathFromRootToCurrentNode;
        applyMCXGateToQmdd(*edgeToRootNode, targetQubit, controlQubitsForPathFromRootToCurrentNode);
    } else {
        // TODO: Iterate all paths from the root to the current node and record the controls for each path P as c(P) then add a toffoli gate TOFF(controls: c(P), target: current)
        for (const UnoptimizedQmddPath& pathFromRootToCurrentNode: getAllPathsFromRootToNode(rootNode, qmddNodeAndEdgePaths.associatedQmddNode)) {
            assert(!pathFromRootToCurrentNode.empty());
            const qc::Controls controlQubitsForPathFromRootToCurrentNode = getControlQubitsFromSignatureOfQmddPathComponents(pathFromRootToCurrentNode);
            // TODO: Root can change?
            applyMCXGateToQmdd(*edgeToRootNode, targetQubit, controlQubitsForPathFromRootToCurrentNode);
            // TODO: Application of QMDD operation can change structure of QMDD thus previously determined paths may no longer exist.
            // One could remove no longer existing paths to trim parts of the path that were simplified but this would require use to iterate over all paths of an edge or potentially all paths
            // starting from the node. Could it also be that no path exists in the QMDD since it now points to the one-terminal?
            break;
        }
    }
    return true;
}

// This algorithm moves the unique paths present in the p' edge to the n edge.
// TODO: In the reimplementation this step is not implemented: 'If there are no unique paths in p' edge, the unique paths present in the n' edge are moved to the p edge if required.'
// Refer to the P2 algorithm of http://www.informatik.uni-bremen.de/agra/doc/konf/12aspdac_qmdd_synth_rev.pdf
bool QmddTransformer::tryShiftUniquePathsOfQmddNode(QmddNodeAndPathsPerEdge& qmddNodeAndEdgePaths) const {
    // TODO: Currently SHE exception with code 0xc0000005 for multiple paths since QMDD could be changed after an operation is applied.
    // TODO: We currently restrict ourselves to the first found unique path while in the reference paper all unique paths are shifted.
    const std::optional<UnoptimizedQmddPath> uniquePathThatCanBeShiftedInPPrimeEdgeSubtree = getFirstQmddPathWithUniqueSignature(qmddNodeAndEdgePaths.pPrimeEdgePaths, qmddNodeAndEdgePaths.nEdgePaths);
    const std::optional<UnoptimizedQmddPath> uniquePathThatCanBeShiftedInNPrimeEdgeSubtree = !uniquePathThatCanBeShiftedInPPrimeEdgeSubtree.has_value() ? getFirstQmddPathWithUniqueSignature(qmddNodeAndEdgePaths.nPrimeEdgePaths, qmddNodeAndEdgePaths.pEdgePaths) : std::nullopt;

    if (!uniquePathThatCanBeShiftedInPPrimeEdgeSubtree.has_value() && !uniquePathThatCanBeShiftedInNPrimeEdgeSubtree.has_value()) {
        return false;
    }

    const dd::mEdge* edgeToRootNode = tryGetEdgeToQmddRootNode(qmddPkg);
    assert(edgeToRootNode != nullptr);
    assert(edgeToRootNode->p != nullptr);
    const dd::mNode& rootNode = *edgeToRootNode->p;

    const qc::Qubit targetQubit = qmddNodeAndEdgePaths.associatedQmddNode.get().v;
    qc::Controls    controlQubitsForQmddPathStartingFromNodeToOneTerminal;
    if (uniquePathThatCanBeShiftedInPPrimeEdgeSubtree.has_value()) {
        assert(!uniquePathThatCanBeShiftedInPPrimeEdgeSubtree->empty());
        // TODO: Temporary solution
        for (const QmddPathComponent& qmddPathComponent: uniquePathThatCanBeShiftedInPPrimeEdgeSubtree.value() | std::views::drop(1)) {
            controlQubitsForQmddPathStartingFromNodeToOneTerminal.emplace(getControlQubitFromSignatureOfQmddPathComponent(qmddPathComponent));
        }
        //controlQubitsForQmddPathStartingFromNodeToOneTerminal = getControlQubitsFromSignatureOfQmddPathComponents(*uniquePathThatCanBeShiftedInPPrimeEdgeSubtree);
    } else {
        assert(!uniquePathThatCanBeShiftedInNPrimeEdgeSubtree->empty());
        // TODO: Temporary solution, one could add an optional parameter to getControlQubitsFromSignatureOfQmddPathComponents(...) function to skip the first entry of the qmdd path or accept a view instead
        for (const QmddPathComponent& qmddPathComponent: uniquePathThatCanBeShiftedInNPrimeEdgeSubtree.value() | std::views::drop(1)) {
            controlQubitsForQmddPathStartingFromNodeToOneTerminal.emplace(getControlQubitFromSignatureOfQmddPathComponent(qmddPathComponent));
        }
        //controlQubitsForQmddPathStartingFromNodeToOneTerminal = getControlQubitsFromSignatureOfQmddPathComponents(*uniquePathThatCanBeShiftedInNPrimeEdgeSubtree);
    }
    // The control qubits of the operation to shift a unique path P includes the control qubits from the root up to but excluding the current qmdd node N as well as the control qubits for the subpath from the first child
    // node of N to the 1-terminal. Since we performed the transformation of P to its associated control qubits for each component of the path we also need to remove the generated control qubit for the current qmdd node N on P
    // since the target qubit of the to be generated operation is defined as the associated qubit of N.
    controlQubitsForQmddPathStartingFromNodeToOneTerminal.erase(targetQubit);

    const std::vector<UnoptimizedQmddPath> pathsFromRootToCurrentNode = getAllPathsFromRootToNode(rootNode, qmddNodeAndEdgePaths.associatedQmddNode);
    if (pathsFromRootToCurrentNode.empty()) {
        // TODO: Maybe use assert(rootNode.v == qmddNodeAndEdgePaths.associatedQmddNode.get().v); instead?
        assert(rootNode.v == targetQubit);
        applyMCXGateToQmdd(*edgeToRootNode, targetQubit, controlQubitsForQmddPathStartingFromNodeToOneTerminal);
    } else {
        for (const UnoptimizedQmddPath& pathFromRootToCurrentNode: pathsFromRootToCurrentNode) {
            assert(!pathFromRootToCurrentNode.empty());
            qc::Controls controlQubitsToTargetPathFromRootToCurrentNode = getControlQubitsFromSignatureOfQmddPathComponents(pathFromRootToCurrentNode);
            controlQubitsToTargetPathFromRootToCurrentNode.insert(controlQubitsForQmddPathStartingFromNodeToOneTerminal.cbegin(), controlQubitsForQmddPathStartingFromNodeToOneTerminal.cend());
            // TODO: Root could change?
            applyMCXGateToQmdd(*edgeToRootNode, targetQubit, controlQubitsToTargetPathFromRootToCurrentNode);
            // TODO: Application of QMDD operation can change structure of QMDD thus previously determined paths may no longer exist
            // TODO: One could check whether parts of the signature from the root to the node still exist and were not targeted by a previous gate?
            break;
        }
    }
    return true;
}

bool QmddTransformer::tryMakeSharedPathOfQmddNodeUnique(QmddNodeAndPathsPerEdge& qmddNodeAndEdgePaths) const {
    const std::optional<TransformationToUniqueQmddPathData> transformationDataForQmddPathOfPPrimeSubtree = getTransformationDataToMakeAnyQmddPathUniqueViaSingleSignatureBitFlip(qmddNodeAndEdgePaths.pPrimeEdgePaths, qmddNodeAndEdgePaths.nEdgePaths);
    const std::optional<TransformationToUniqueQmddPathData> transformationDataForQmddPathOfNPrimeSubtree = !transformationDataForQmddPathOfPPrimeSubtree.has_value() ? getTransformationDataToMakeAnyQmddPathUniqueViaSingleSignatureBitFlip(qmddNodeAndEdgePaths.nPrimeEdgePaths, qmddNodeAndEdgePaths.pEdgePaths) : std::nullopt;

    if (!transformationDataForQmddPathOfPPrimeSubtree.has_value() && !transformationDataForQmddPathOfNPrimeSubtree.has_value()) {
        return false;
    }

    const dd::mEdge* edgeToRootNode = tryGetEdgeToQmddRootNode(qmddPkg);
    assert(edgeToRootNode != nullptr);
    assert(edgeToRootNode->p != nullptr);
    const dd::mNode& rootNode = *edgeToRootNode->p;

    const TransformationToUniqueQmddPathData transformationDataToTurnQmddPathUniqueStartingFromNodeToOneTerminal = transformationDataForQmddPathOfPPrimeSubtree.has_value() ? *transformationDataForQmddPathOfPPrimeSubtree : *transformationDataForQmddPathOfNPrimeSubtree;

    const qc::Qubit                        targetQubit                = transformationDataToTurnQmddPathUniqueStartingFromNodeToOneTerminal.targetQubit;
    const std::vector<UnoptimizedQmddPath> pathsFromRootToCurrentNode = getAllPathsFromRootToNode(rootNode, qmddNodeAndEdgePaths.associatedQmddNode);
    if (pathsFromRootToCurrentNode.empty()) {
        assert(rootNode.v == qmddNodeAndEdgePaths.associatedQmddNode.get().v);
        applyMCXGateToQmdd(*edgeToRootNode, targetQubit, transformationDataToTurnQmddPathUniqueStartingFromNodeToOneTerminal.controlQubitsFromFirstNodeInPathToTargetQubit);
    } else {
        for (const UnoptimizedQmddPath& pathFromRootToCurrentNode: pathsFromRootToCurrentNode) {
            assert(!pathFromRootToCurrentNode.empty());
            qc::Controls controlQubitsToTargetPathFromRootToCurrentNode = getControlQubitsFromSignatureOfQmddPathComponents(pathFromRootToCurrentNode);
            controlQubitsToTargetPathFromRootToCurrentNode.insert(
                    transformationDataToTurnQmddPathUniqueStartingFromNodeToOneTerminal.controlQubitsFromFirstNodeInPathToTargetQubit.cbegin(),
                    transformationDataToTurnQmddPathUniqueStartingFromNodeToOneTerminal.controlQubitsFromFirstNodeInPathToTargetQubit.cend());
            // TODO: Root could change?
            applyMCXGateToQmdd(*edgeToRootNode, targetQubit, controlQubitsToTargetPathFromRootToCurrentNode);
            // TODO: Application of QMDD operation can change structure of QMDD thus previously determined paths may no longer exist
            // TODO: One could check whether parts of the signature from the root to the node still exist and were not targeted by a previous gate?
            break;
        }
    }
    return true;
}

// TODO: Can the transformation of the QMDD result in a QMDD that only consists of the one/zero terminal?
const dd::mEdge* QmddTransformer::tryGetEdgeToQmddRootNode(dd::Package& qmddPkgToGetRootFrom) {
    const auto& setOfRootNodes = qmddPkgToGetRootFrom.getRootSet<dd::mNode>();
    return !setOfRootNodes.empty() ? &setOfRootNodes.begin()->first : nullptr;
}

bool QmddTransformer::terminate(const dd::mNode& nodeToCheck) {
    const auto& edgesOfQmddNode = nodeToCheck.e;
    assert(edgesOfQmddNode.size() == 4);
    // Original implementation also checked that N' edge points to zero terminal but according to the reference paper it should be sufficient to only check the P' edge since
    // the transformations applied by the algorithm should result in a QMDD node which represents the identity by the P' edge pointing to the zero terminal which in turn would also mean that the N' edge points to the zero terminal.
    // TODO: Should we keep the assert?
    //assert( edgesOfQmddNode[static_cast<std::size_t>(QmddNodeEdge::P_Prime)].isZeroTerminal() && edgesOfQmddNode[static_cast<std::size_t>(QmddNodeEdge::N_Prime)].isZeroTerminal());
    return edgesOfQmddNode[static_cast<std::size_t>(QmddNodeEdge::P_Prime)].isZeroTerminal();
}

void QmddTransformer::getPathsToOneTerminalThroughEdgeOfQmddNode(const dd::mNode& qmddNodeToStartPathsFrom, const QmddNodeEdge edgesToGeneratePathsFor, QmddNodeAndPathsPerEdge& containerStoringFoundPaths) {
    assert(qmddNodeToStartPathsFrom.e.size() == 4);
    for (const QmddNodeEdge availableQmddNodeEdge: {QmddNodeEdge::N, QmddNodeEdge::P_Prime, QmddNodeEdge::N_Prime, QmddNodeEdge::P}) {
        if (!(availableQmddNodeEdge & edgesToGeneratePathsFor)) {
            continue;
        }

        const auto& firstEdgeInQmddNodePath = qmddNodeToStartPathsFrom.e[convertQmddNodeEdgeEnumValueToArrayIdx(availableQmddNodeEdge)];
        if (firstEdgeInQmddNodePath.isZeroTerminal()) {
            continue;
        }

        if (firstEdgeInQmddNodePath.isOneTerminal()) {
            auto qmddPathToOneTerminal = OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = qmddNodeToStartPathsFrom.v, .qmddEdgeToChildNode = availableQmddNodeEdge})});
            if (qmddNodeToStartPathsFrom.v > 0U) {
                const dd::Qubit firstQubitInOptimizedQmddPathGap = qmddNodeToStartPathsFrom.v - 1U;
                qmddPathToOneTerminal.emplace_back(QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = firstQubitInOptimizedQmddPathGap, .nConsecutiveQubitInGap = 1 + (firstQubitInOptimizedQmddPathGap > 0 ? firstQubitInOptimizedQmddPathGap - 1U : 0U)}));
            }
            // TODO: Refactor into helper function of anonymous namespace?
            switch (availableQmddNodeEdge) {
                case QmddNodeEdge::N:
                    containerStoringFoundPaths.nEdgePaths.emplace_back(qmddPathToOneTerminal);
                    break;
                case QmddNodeEdge::P_Prime:
                    containerStoringFoundPaths.pPrimeEdgePaths.emplace_back(qmddPathToOneTerminal);
                    break;
                case QmddNodeEdge::N_Prime:
                    containerStoringFoundPaths.nPrimeEdgePaths.emplace_back(qmddPathToOneTerminal);
                    break;
                case QmddNodeEdge::P:
                    containerStoringFoundPaths.pEdgePaths.emplace_back(qmddPathToOneTerminal);
                    break;
                default:
                    // TODO: Throw an exception in exception-free code?
                    break;
            }
            continue;
        }

        std::vector<VisitedQmddNodeEdgesAggregation> toBeVisitedQmddNodesStack;
        toBeVisitedQmddNodesStack.emplace_back(firstEdgeInQmddNodePath.p);
        while (!toBeVisitedQmddNodesStack.empty()) {
            auto& visitedQmddNodeStackEntry = toBeVisitedQmddNodesStack.back();
            if (visitedQmddNodeStackEntry.visitedAllEdges()) {
                toBeVisitedQmddNodesStack.pop_back();
                continue;
            }

            const QmddNodeEdge nextQmddNodeEdgeToVisit = visitedQmddNodeStackEntry.advanceToNextEdge();
            const auto&        edgeToChildQmddNode     = visitedQmddNodeStackEntry.associatedQmddNode->e[convertQmddNodeEdgeEnumValueToArrayIdx(nextQmddNodeEdgeToVisit)];
            if (edgeToChildQmddNode.isOneTerminal()) {
                OptimizedQmddPath qmddPathToOneTerminal;
                // Note that is reserve operation does not account for gaps in qmdd path
                qmddPathToOneTerminal.reserve(toBeVisitedQmddNodesStack.size() + 1U);
                qmddPathToOneTerminal.emplace_back(QmddPathComponent({.qubitAssociatedWithQmddNode = qmddNodeToStartPathsFrom.v, .qmddEdgeToChildNode = availableQmddNodeEdge}));

                dd::Qubit qubitAssociatedWithLastProcessedQmddNodeStackEntry = qmddNodeToStartPathsFrom.v;
                for (const auto& toBeVisitedQmddNodesStackEntry: toBeVisitedQmddNodesStack) {
                    const dd::Qubit qubitAssociatedWithCurrentlyProcessedQmddNodeStackEntry = toBeVisitedQmddNodesStackEntry.associatedQmddNode->v;
                    const dd::Qubit expectedQubitForNextComponentInQmddPath                 = qubitAssociatedWithLastProcessedQmddNodeStackEntry - 1U;

                    if (expectedQubitForNextComponentInQmddPath != qubitAssociatedWithCurrentlyProcessedQmddNodeStackEntry) {
                        // Gap in qmdd path detected due to some node and edges being optimized away by the underlying qmdd data structure due to this "sub-qmdd" representing the identity function for the optimized away qubits.
                        // Since the qmdd transformation algorithm is assumed to require all paths in the qmdd requires us to also record these gaps to correctly calculate the number of qmdd paths through an edge in the qmdd.
                        const std::size_t qmddPathGapSize = expectedQubitForNextComponentInQmddPath - qubitAssociatedWithCurrentlyProcessedQmddNodeStackEntry;
                        qmddPathToOneTerminal.emplace_back(QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = expectedQubitForNextComponentInQmddPath, .nConsecutiveQubitInGap = qmddPathGapSize}));
                        qmddPathToOneTerminal.emplace_back(QmddPathComponent({.qubitAssociatedWithQmddNode = qubitAssociatedWithCurrentlyProcessedQmddNodeStackEntry, .qmddEdgeToChildNode = nextQmddNodeEdgeToVisit}));
                    } else {
                        qmddPathToOneTerminal.emplace_back(QmddPathComponent({.qubitAssociatedWithQmddNode = qubitAssociatedWithCurrentlyProcessedQmddNodeStackEntry, .qmddEdgeToChildNode = *toBeVisitedQmddNodesStackEntry.currVisitedEdge}));
                    }
                    qubitAssociatedWithLastProcessedQmddNodeStackEntry = qubitAssociatedWithCurrentlyProcessedQmddNodeStackEntry;
                }

                switch (availableQmddNodeEdge) {
                    case QmddNodeEdge::N:
                        containerStoringFoundPaths.nEdgePaths.emplace_back(qmddPathToOneTerminal);
                        break;
                    case QmddNodeEdge::P_Prime:
                        containerStoringFoundPaths.pPrimeEdgePaths.emplace_back(qmddPathToOneTerminal);
                        break;
                    case QmddNodeEdge::N_Prime:
                        containerStoringFoundPaths.nPrimeEdgePaths.emplace_back(qmddPathToOneTerminal);
                        break;
                    case QmddNodeEdge::P:
                        containerStoringFoundPaths.pEdgePaths.emplace_back(qmddPathToOneTerminal);
                        break;
                    default:
                        // TODO: Throw an exception in exception-free code?
                        break;
                }
                // Some of the qmdd transformation algorithm rules need to check the number of paths through an edge with a path assuming to end in a one-terminal.
                // However, the data structure used to store the qmdd may optimize paths (i.e. sub-qmdds) that already represent the identity function by simply pointing the
                // edge to the subtree to the one terminal. However, the calculation for the number of paths to the one-terminal (the paths from the root of the qmdd to the last qubit in the qubit ordering of the associated reversible function)
                // should not accumulate the "optimized" path to the one-terminal representing the sub-qmdd by assuming a single path but should instead count all paths in the sub-qmdd.
                //qmddPathToOneTerminal.nonTruncatedPathComponents.pop_back();
                //toBeVisitedQmddNodesStack.pop_back();
            } else if (!edgeToChildQmddNode.isZeroTerminal()) {
                // TODO: Refactor into helper function of anonymous namespace?
                const dd::mNode* toBeVisitedNode = edgeToChildQmddNode.p;
                assert(toBeVisitedNode->e.size() == 4);
                toBeVisitedQmddNodesStack.emplace_back(toBeVisitedNode);
            }
        }
    }
}

std::vector<QmddTransformer::UnoptimizedQmddPath> QmddTransformer::getAllPathsFromRootToNode(const dd::mNode& qmddRootNode, const dd::mNode& qmddNodeToReach) {
    if (qmddRootNode.v == qmddNodeToReach.v) {
        return {};
    }

    assert(qmddRootNode.e.size() == 4);
    std::vector<UnoptimizedQmddPath> containerForFoundQmddpaths;
    UnoptimizedQmddPath              currQmddPathToOneTerminal;

    std::stack<VisitedQmddNodeEdgesAggregation> toBeVisitedQmddNodesStack;
    toBeVisitedQmddNodesStack.emplace(&qmddRootNode);
    while (!toBeVisitedQmddNodesStack.empty()) {
        auto& visitedQmddNode = toBeVisitedQmddNodesStack.top();
        if (visitedQmddNode.visitedAllEdges()) {
            toBeVisitedQmddNodesStack.pop();
            if (!currQmddPathToOneTerminal.empty()) {
                currQmddPathToOneTerminal.pop_back();
            }
            continue;
        }

        // TODO: The searched for node should be reachable by only traversing the N or P edges.
        // TODO: At the moment all possible edges of a node are checked.
        const std::optional<QmddNodeEdge> nextQmddNodeEdgeToVisit = visitedQmddNode.advanceToNextEdgeInPathFromRootToNode();
        if (!nextQmddNodeEdgeToVisit.has_value()) {
            return {};
        }
        const dd::mEdge& edgeToChildQmddNode = visitedQmddNode.associatedQmddNode->e[convertQmddNodeEdgeEnumValueToArrayIdx(*nextQmddNodeEdgeToVisit)];
        const dd::mNode* toBeVisitedNode     = edgeToChildQmddNode.p;
        if (edgeToChildQmddNode.isTerminal() || toBeVisitedNode == nullptr || toBeVisitedNode->v < qmddNodeToReach.v) {
            if (!edgeToChildQmddNode.isTerminal()) {
                visitedQmddNode.markAllEdgesAsVisited();
            }
            continue;
        }

        if (toBeVisitedNode->v == qmddNodeToReach.v) {
            // TODO:
            // We need to check whether the qmdd node associated with the same qubit is actually the node that we are looking for. Otherwise, the search did not take the "correct"
            // edge of the root qmdd node and we need to continue our search in the parent qmdd node.
            if (edgeToChildQmddNode.p == &qmddNodeToReach) {
                currQmddPathToOneTerminal.emplace_back(QmddPathComponent({.qubitAssociatedWithQmddNode = visitedQmddNode.associatedQmddNode->v, .qmddEdgeToChildNode = *nextQmddNodeEdgeToVisit}));
                containerForFoundQmddpaths.emplace_back(currQmddPathToOneTerminal);
            }
        } else {
            assert(toBeVisitedNode->e.size() == 4);
            // We are still processing a parent qmdd node thus we advance one level down in the qmdd
            toBeVisitedQmddNodesStack.emplace(toBeVisitedNode);
            currQmddPathToOneTerminal.emplace_back(QmddPathComponent({.qubitAssociatedWithQmddNode = visitedQmddNode.associatedQmddNode->v, .qmddEdgeToChildNode = *nextQmddNodeEdgeToVisit}));
        }
    }
    return containerForFoundQmddpaths;
}

qc::Controls QmddTransformer::getControlQubitsFromSignatureOfQmddPathComponents(const std::vector<QmddPathComponent>& qmddPathComponents) noexcept {
    qc::Controls controlQubits;
    for (const QmddPathComponent& qmddPathComponent: qmddPathComponents) {
        controlQubits.emplace(getControlQubitFromSignatureOfQmddPathComponent(qmddPathComponent));
    }
    return controlQubits;
}

// TODO: Introduct distinction between untrimmed and trimmed path with the former being returned by this function and potentially used as input for the next qmdd transformation algorithm steps while the latter is returned
// by the initial built of all qmdd paths of the currently processed qmdd node.
// TODO: Can paths that are considered as potentially unique or search through be skipped if they are empty or have a size smaller than one?
std::optional<QmddTransformer::UnoptimizedQmddPath> QmddTransformer::getFirstQmddPathWithUniqueSignature(const std::vector<OptimizedQmddPath>& qmddPathsToSearchForUniqueOne, const std::vector<OptimizedQmddPath>& qmddPathsDefiningComparedToSignatures) {
    for (const OptimizedQmddPath& untrimmedQmddPathWithPotentiallyUniqueSignature: qmddPathsToSearchForUniqueOne) {
        QmddPathGenerator fullUntrimmedQmddPathWithPotentiallyUniqueSignatureGenerator(untrimmedQmddPathWithPotentiallyUniqueSignature);
        while (fullUntrimmedQmddPathWithPotentiallyUniqueSignatureGenerator.generateNextCombination()) {
            // We need to drop the first component of both QMDD paths due to the uniqueness check starting in the first child qmdd node of each path.
            const auto trimmedQmddPathSignatureOfPotentiallyUniquePath = fullUntrimmedQmddPathWithPotentiallyUniqueSignatureGenerator.lastGeneratedCombination | std::views::drop(1) | std::views::transform(getControlQubitFromSignatureOfQmddPathComponent);

            // TODO: What if qmdd paths have different lengths, this should not happen but could
            // TODO: Refactor into own function
            const bool existsQmddPathThatMatchesPotentiallyUniqueOneInComparedToQmddPathCollection = std::ranges::any_of(qmddPathsDefiningComparedToSignatures,
                                                                                                                         [&trimmedQmddPathSignatureOfPotentiallyUniquePath](const OptimizedQmddPath& untrimmedQmddPathNotAllowedToOverlapPotentiallyUniqueOne) {
                                                                                                                             bool existsQmddPathThatMatchesPotentiallyUniqueOneInComparedToQmddPathCombinations = false;

                                                                                                                             QmddPathGenerator fullUntrimmedQmddPathNotAllowedToOverlapPotentiallyUniqueOneGenerator(untrimmedQmddPathNotAllowedToOverlapPotentiallyUniqueOne);
                                                                                                                             while (fullUntrimmedQmddPathNotAllowedToOverlapPotentiallyUniqueOneGenerator.generateNextCombination() && !existsQmddPathThatMatchesPotentiallyUniqueOneInComparedToQmddPathCombinations) {
                                                                                                                                 const auto trimmedQmddPathNotAllowedToOverlapPotentiallyUniqueOne = fullUntrimmedQmddPathNotAllowedToOverlapPotentiallyUniqueOneGenerator.lastGeneratedCombination | std::views::drop(1) | std::views::transform(getControlQubitFromSignatureOfQmddPathComponent);

                                                                                                                                 existsQmddPathThatMatchesPotentiallyUniqueOneInComparedToQmddPathCombinations = std::ranges::equal(trimmedQmddPathSignatureOfPotentiallyUniquePath, trimmedQmddPathNotAllowedToOverlapPotentiallyUniqueOne);
                                                                                                                             }
                                                                                                                             return existsQmddPathThatMatchesPotentiallyUniqueOneInComparedToQmddPathCombinations;
                                                                                                                         });

            if (!existsQmddPathThatMatchesPotentiallyUniqueOneInComparedToQmddPathCollection) {
                return UnoptimizedQmddPath(fullUntrimmedQmddPathWithPotentiallyUniqueSignatureGenerator.lastGeneratedCombination);
            }
        }
    }
    return std::nullopt;
}

std::optional<QmddTransformer::TransformationToUniqueQmddPathData> QmddTransformer::getTransformationDataToMakeAnyQmddPathUniqueViaSingleSignatureBitFlip(const std::vector<OptimizedQmddPath>& qmddPathsContainingPotentiallyTransformableOne, const std::vector<OptimizedQmddPath>& comparedToQmddPaths) {
    for (const OptimizedQmddPath& potentiallyTransformableQmddPath: qmddPathsContainingPotentiallyTransformableOne) {
        if (const std::optional<TransformationToUniqueQmddPathData>& fetchedQmddPathTransformationData = getTransformationDataToMakeQmddPathUniqueViaSingleSignatureBitFlip(potentiallyTransformableQmddPath, comparedToQmddPaths); fetchedQmddPathTransformationData.has_value()) {
            return fetchedQmddPathTransformationData;
        }
    }
    return std::nullopt;
}

std::optional<QmddTransformer::TransformationToUniqueQmddPathData> QmddTransformer::getTransformationDataToMakeQmddPathUniqueViaSingleSignatureBitFlip(const OptimizedQmddPath& qmddPathToTurnUnique, const std::vector<OptimizedQmddPath>& comparedToQmddPaths) {
    // TODO: Handle empty compare to qmdd paths collection
    QmddPathGenerator qmddPathToTurnUniqueGenerator(qmddPathToTurnUnique);
    while (qmddPathToTurnUniqueGenerator.generateNextCombination()) {
        const std::vector<QmddPathComponent>& generatedQmddPathToTurnUnique = qmddPathToTurnUniqueGenerator.lastGeneratedCombination;
        if (generatedQmddPathToTurnUnique.size() < 2) {
            return std::nullopt;
        }

        auto trimmedViewOfSignatureBitsOfQmddPathToTurnUnique     = generatedQmddPathToTurnUnique | std::views::drop(1) | std::views::transform(getControlQubitFromSignatureOfQmddPathComponent);
        auto homogeneousViewOfSignatureBitsOfQmddPathToTurnUnique = std::views::common(trimmedViewOfSignatureBitsOfQmddPathToTurnUnique);

        qc::Controls signatureBitsContainerOfQmddPathToBeTurnedUnique(homogeneousViewOfSignatureBitsOfQmddPathToTurnUnique.begin(), homogeneousViewOfSignatureBitsOfQmddPathToTurnUnique.end());
        for (std::size_t signatureBitIdx = 0U; signatureBitIdx < signatureBitsContainerOfQmddPathToBeTurnedUnique.size(); ++signatureBitIdx) {
            // TODO: Do we have to skip the first entry in the qmdd path?
            // We need to ignore the first entry in the checked qmdd path since our goal is to turn the path unique so that it can be shifted from the current edge of the parent node (i.e. the qmdd node of the first component of the qmdd path) to another edge.
            const qc::Control originalSignatureBitValue = getControlQubitFromSignatureOfQmddPathComponent(generatedQmddPathToTurnUnique.at(1U + signatureBitIdx));
            const auto        flippedSignatureBitValue  = qc::Control(originalSignatureBitValue.qubit, originalSignatureBitValue.type == qc::Control::Type::Pos ? qc::Control::Type::Neg : qc::Control::Type::Pos);

            // Flip signature bit "polarity" and check whether generated signature is unique
            auto extractSignatureBitNode         = signatureBitsContainerOfQmddPathToBeTurnedUnique.extract(originalSignatureBitValue);
            extractSignatureBitNode.value().type = flippedSignatureBitValue.type;
            signatureBitsContainerOfQmddPathToBeTurnedUnique.insert(std::move(extractSignatureBitNode));

            // TODO: The generated signature could match an already existing one that we either already checked or will check in the future, can we cache these results?
            // TODO: Perform check whether generated signature is unique.

            bool didGeneratedSignatureMatchExistingOne = false;
            for (const auto& comparedToQmddPath: comparedToQmddPaths) {
                QmddPathGenerator comparedToQmddPathGenerator(comparedToQmddPath);
                while (comparedToQmddPathGenerator.generateNextCombination() && !didGeneratedSignatureMatchExistingOne) {
                    // TODO: Check correct length (i.e. must be longer than one)
                    // TODO: Check that first qubits of path are equal
                    auto trimmedViewOfComparedToSignatureBits = comparedToQmddPathGenerator.lastGeneratedCombination | std::views::drop(1) | std::views::transform(getControlQubitFromSignatureOfQmddPathComponent);

                    didGeneratedSignatureMatchExistingOne = std::ranges::all_of(trimmedViewOfComparedToSignatureBits, [&signatureBitsContainerOfQmddPathToBeTurnedUnique](const qc::Control comparedToSignatureBit) {
                        return signatureBitsContainerOfQmddPathToBeTurnedUnique.contains(comparedToSignatureBit);
                    });
                }
            }

            if (!didGeneratedSignatureMatchExistingOne) {
                const qc::Qubit targetQubitDefiningFlippedSignatureBit = flippedSignatureBitValue.qubit;
                qc::Controls    controlQubitsToReachTargetQubitFromStartOfQmddPath;
                for (std::size_t signatureIdxToReachTargetQubit = 0U; signatureIdxToReachTargetQubit < 1U + signatureBitIdx; ++signatureIdxToReachTargetQubit) {
                    controlQubitsToReachTargetQubitFromStartOfQmddPath.emplace(getControlQubitFromSignatureOfQmddPathComponent(generatedQmddPathToTurnUnique.at(signatureIdxToReachTargetQubit)));
                }
                return TransformationToUniqueQmddPathData(controlQubitsToReachTargetQubitFromStartOfQmddPath, targetQubitDefiningFlippedSignatureBit);
            }

            // If the current signature formed by the contents of the signature bit lookup is not unique then flip the modified signature bit back to its original value.
            extractSignatureBitNode              = signatureBitsContainerOfQmddPathToBeTurnedUnique.extract(flippedSignatureBitValue);
            extractSignatureBitNode.value().type = originalSignatureBitValue.type;
            signatureBitsContainerOfQmddPathToBeTurnedUnique.insert(std::move(extractSignatureBitNode));
        }
    }
    return std::nullopt;
}

std::size_t QmddTransformer::getNumberOfPathsToOneTerminalForQmddPath(const OptimizedQmddPath& qmddPath) noexcept {
    auto nPaths = static_cast<std::size_t>(!qmddPath.empty());
    for (auto qmddPathIterator = qmddPath.rbegin(); qmddPathIterator != qmddPath.rend(); ++qmddPathIterator) {
        if (const QmddPathGap* qmddPathGap = std::get_if<QmddPathGap>(&*qmddPathIterator); qmddPathGap != nullptr) {
            // TODO: Replace std::pow call
            nPaths *= static_cast<std::size_t>(std::pow(2, qmddPathGap->nConsecutiveQubitInGap));
        }
    }
    return nPaths;
}

std::size_t QmddTransformer::getNumberOfPathsToOneTerminalForQmddPaths(const std::vector<OptimizedQmddPath>& qmddPaths) noexcept {
    std::size_t nPaths = 0U;
    for (const auto& qmddPath: qmddPaths) {
        nPaths += getNumberOfPathsToOneTerminalForQmddPath(qmddPath);
    }
    return nPaths;
}
