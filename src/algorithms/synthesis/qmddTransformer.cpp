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

        QmddNodeAndPathsPerEdge qmddPathsStartingFromNode = {.associatedQmddNode = nodeToProcess, .nEdgePaths = {}, .pPrimeEdgePaths = {}, .nPrimeEdgePaths = {}, .pEdgePaths = {}};
        getPathsThroughEdgeStartingFromQmddNode(nodeToProcess, QmddNodeEdge::N | QmddNodeEdge::P_Prime, qmddPathsStartingFromNode);
        // P1 algorithm
        bool resetQueue = trySwapPathsOfEdgesOfQmddNode(qmddPathsStartingFromNode);
        if (!resetQueue) {
            getPathsThroughEdgeStartingFromQmddNode(nodeToProcess, QmddNodeEdge::N_Prime | QmddNodeEdge::P, qmddPathsStartingFromNode);
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
    if (qmddNodeAndEdgePaths.pPrimeEdgePaths.size() <= qmddNodeAndEdgePaths.nEdgePaths.size()) {
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
        for (const QmddPath& pathFromRootToCurrentNode: getAllPathsFromRootToNode(rootNode, qmddNodeAndEdgePaths.associatedQmddNode)) {
            assert(!pathFromRootToCurrentNode.empty());
            const qc::Controls controlQubitsForPathFromRootToCurrentNode = getControlQubitsFromSignatureOfQmddPath(pathFromRootToCurrentNode);
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
    const std::optional<QmddPath> uniquePathThatCanBeShiftedInPPrimeEdgeSubtree = getFirstQmddPathWithUniqueSignature(qmddNodeAndEdgePaths.pPrimeEdgePaths, qmddNodeAndEdgePaths.nEdgePaths);
    const std::optional<QmddPath> uniquePathThatCanBeShiftedInNPrimeEdgeSubtree = !uniquePathThatCanBeShiftedInPPrimeEdgeSubtree.has_value() ? getFirstQmddPathWithUniqueSignature(qmddNodeAndEdgePaths.nPrimeEdgePaths, qmddNodeAndEdgePaths.pEdgePaths) : std::nullopt;

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
        controlQubitsForQmddPathStartingFromNodeToOneTerminal = getControlQubitsFromSignatureOfQmddPath(*uniquePathThatCanBeShiftedInPPrimeEdgeSubtree);
    } else {
        controlQubitsForQmddPathStartingFromNodeToOneTerminal = getControlQubitsFromSignatureOfQmddPath(*uniquePathThatCanBeShiftedInNPrimeEdgeSubtree);
    }
    // The control qubits of the operation to shift a unique path P includes the control qubits from the root up to but excluding the current qmdd node N as well as the control qubits for the subpath from the first child
    // node of N to the 1-terminal. Since we performed the transformation of P to its associated control qubits for each component of the path we also need to remove the generated control qubit for the current qmdd node N on P
    // since the target qubit of the to be generated operation is defined as the associated qubit of N.
    controlQubitsForQmddPathStartingFromNodeToOneTerminal.erase(targetQubit);

    const std::vector<QmddPath> pathsFromRootToCurrentNode = getAllPathsFromRootToNode(rootNode, qmddNodeAndEdgePaths.associatedQmddNode);
    if (pathsFromRootToCurrentNode.empty()) {
        // TODO: Maybe use assert(rootNode.v == qmddNodeAndEdgePaths.associatedQmddNode.get().v); instead?
        assert(rootNode.v == targetQubit);
        applyMCXGateToQmdd(*edgeToRootNode, targetQubit, controlQubitsForQmddPathStartingFromNodeToOneTerminal);
    } else {
        for (const QmddPath& pathFromRootToCurrentNode: pathsFromRootToCurrentNode) {
            assert(!pathFromRootToCurrentNode.empty());
            qc::Controls controlQubitsToTargetPathFromRootToCurrentNode = getControlQubitsFromSignatureOfQmddPath(pathFromRootToCurrentNode);
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

    const qc::Qubit             targetQubit                = transformationDataToTurnQmddPathUniqueStartingFromNodeToOneTerminal.targetQubit;
    const std::vector<QmddPath> pathsFromRootToCurrentNode = getAllPathsFromRootToNode(rootNode, qmddNodeAndEdgePaths.associatedQmddNode);
    if (pathsFromRootToCurrentNode.empty()) {
        assert(rootNode.v == qmddNodeAndEdgePaths.associatedQmddNode.get().v);
        applyMCXGateToQmdd(*edgeToRootNode, targetQubit, transformationDataToTurnQmddPathUniqueStartingFromNodeToOneTerminal.controlQubitsFromFirstNodeInPathToTargetQubit);
    } else {
        for (const QmddPath& pathFromRootToCurrentNode: pathsFromRootToCurrentNode) {
            assert(!pathFromRootToCurrentNode.empty());
            qc::Controls controlQubitsToTargetPathFromRootToCurrentNode = getControlQubitsFromSignatureOfQmddPath(pathFromRootToCurrentNode);
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

void QmddTransformer::getPathsThroughEdgeStartingFromQmddNode(const dd::mNode& qmddNodeToStartPathsFrom, const QmddNodeEdge edgesToGeneratePathsFor, QmddNodeAndPathsPerEdge& containerStoringFoundPaths) {
    assert(qmddNodeToStartPathsFrom.e.size() == 4);
    for (const QmddNodeEdge availableQmddNodeEdge: {QmddNodeEdge::N, QmddNodeEdge::P_Prime, QmddNodeEdge::N_Prime, QmddNodeEdge::P}) {
        if (!(availableQmddNodeEdge & edgesToGeneratePathsFor)) {
            continue;
        }

        const auto& firstEdgeInQmddNodePath = qmddNodeToStartPathsFrom.e[convertQmddNodeEdgeEnumValueToArrayIdx(availableQmddNodeEdge)];
        if (firstEdgeInQmddNodePath.isZeroTerminal()) {
            continue;
        }

        auto qmddPathToOneTerminal = QmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = qmddNodeToStartPathsFrom.v, .qmddEdgeToChildNode = availableQmddNodeEdge})});
        if (firstEdgeInQmddNodePath.isOneTerminal()) {
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

        std::stack<VisitedQmddNodeEdgesAggregation> toBeVisitedQmddNodesStack;
        toBeVisitedQmddNodesStack.emplace(firstEdgeInQmddNodePath.p);
        while (!toBeVisitedQmddNodesStack.empty()) {
            auto& visitedQmddNodeStackEntry = toBeVisitedQmddNodesStack.top();
            if (visitedQmddNodeStackEntry.visitedAllEdges()) {
                toBeVisitedQmddNodesStack.pop();
                if (!qmddPathToOneTerminal.empty()) {
                    qmddPathToOneTerminal.pop_back();
                }
                continue;
            }

            const QmddNodeEdge nextQmddNodeEdgeToVisit = visitedQmddNodeStackEntry.advanceToNextEdge();
            const auto&        edgeToChildQmddNode     = visitedQmddNodeStackEntry.associatedQmddNode->e[convertQmddNodeEdgeEnumValueToArrayIdx(nextQmddNodeEdgeToVisit)];
            if (edgeToChildQmddNode.isOneTerminal()) {
                qmddPathToOneTerminal.emplace_back(QmddPathComponent({.qubitAssociatedWithQmddNode = visitedQmddNodeStackEntry.associatedQmddNode->v, .qmddEdgeToChildNode = nextQmddNodeEdgeToVisit}));
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
                qmddPathToOneTerminal.pop_back();
                toBeVisitedQmddNodesStack.pop();
            } else if (!edgeToChildQmddNode.isZeroTerminal()) {
                // TODO: Refactor into helper function of anonymous namespace?
                const dd::mNode* toBeVisitedNode = edgeToChildQmddNode.p;
                assert(toBeVisitedNode->e.size() == 4);
                toBeVisitedQmddNodesStack.emplace(toBeVisitedNode);
                qmddPathToOneTerminal.emplace_back(QmddPathComponent({.qubitAssociatedWithQmddNode = visitedQmddNodeStackEntry.associatedQmddNode->v, .qmddEdgeToChildNode = nextQmddNodeEdgeToVisit}));
            }
        }
    }
}

std::vector<QmddTransformer::QmddPath> QmddTransformer::getAllPathsFromRootToNode(const dd::mNode& qmddRootNode, const dd::mNode& qmddNodeToReach) {
    if (qmddRootNode.v == qmddNodeToReach.v) {
        return {};
    }

    assert(qmddRootNode.e.size() == 4);
    std::vector<QmddPath> containerForFoundQmddpaths;
    QmddPath              currQmddPathToOneTerminal;

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

        const QmddNodeEdge nextQmddNodeEdgeToVisit = visitedQmddNode.advanceToNextEdge();
        const dd::mEdge&   edgeToChildQmddNode     = visitedQmddNode.associatedQmddNode->e[convertQmddNodeEdgeEnumValueToArrayIdx(nextQmddNodeEdgeToVisit)];
        const dd::mNode*   toBeVisitedNode         = edgeToChildQmddNode.p;
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
                currQmddPathToOneTerminal.emplace_back(QmddPathComponent({.qubitAssociatedWithQmddNode = visitedQmddNode.associatedQmddNode->v, .qmddEdgeToChildNode = nextQmddNodeEdgeToVisit}));
                containerForFoundQmddpaths.emplace_back(currQmddPathToOneTerminal);
            } else {
                visitedQmddNode.markAllEdgesAsVisited();
            }
        } else {
            assert(toBeVisitedNode->e.size() == 4);
            // We are still processing a parent qmdd node thus we advance one level down in the qmdd
            toBeVisitedQmddNodesStack.emplace(toBeVisitedNode);
            currQmddPathToOneTerminal.emplace_back(QmddPathComponent({.qubitAssociatedWithQmddNode = visitedQmddNode.associatedQmddNode->v, .qmddEdgeToChildNode = nextQmddNodeEdgeToVisit}));
        }
    }
    return containerForFoundQmddpaths;
}

qc::Controls QmddTransformer::getControlQubitsFromSignatureOfQmddPath(const QmddPath& qmddPath) noexcept {
    qc::Controls controlQubits;
    for (const QmddPathComponent& qmddPathComponent: qmddPath) {
        controlQubits.emplace(getControlQubitForQmddPathComponent(qmddPathComponent));
    }
    return controlQubits;
}

qc::Control QmddTransformer::getControlQubitForQmddPathComponent(const QmddPathComponent qmddPathComponent) {
    return qc::Control(qmddPathComponent.qubitAssociatedWithQmddNode, getControlQubitTypeForQmddNodeEdge(qmddPathComponent.qmddEdgeToChildNode));
}

std::optional<QmddTransformer::QmddPath> QmddTransformer::getFirstQmddPathWithUniqueSignature(const std::vector<QmddPath>& qmddPathsToSearchForUniqueOne, const std::vector<QmddPath>& qmddPathsDefiningComparedToSignatures) {
    for (const QmddPath& qmddPathWithPotentiallyUniqueSignature: qmddPathsToSearchForUniqueOne) {
        // We need to drop the first component of both QMDD paths due to the uniqueness check starting in the first child qmdd node of each path.
        const auto trimmedQmddPathWithPotentiallyUniqueSignature = qmddPathWithPotentiallyUniqueSignature | std::views::drop(1);
        const auto viewOfPotentiallyUniquePathSignature          = trimmedQmddPathWithPotentiallyUniqueSignature | std::views::transform([](const QmddPathComponent& qmddPathComponent) { return qc::Control(qmddPathComponent.qubitAssociatedWithQmddNode, getControlQubitTypeForQmddNodeEdge(qmddPathComponent.qmddEdgeToChildNode)); });

        for (const QmddPath& qmddPathThatShouldNotShareSignature: qmddPathsDefiningComparedToSignatures) {
            // We need to drop the first component of both QMDD paths due to the uniqueness check starting in the first child qmdd node of each path.
            const auto& trimmedQmddPathThatShouldNotSharedSignature                 = qmddPathThatShouldNotShareSignature | std::views::drop(1);
            const auto  viewOfPathSignatureThatShouldNotOverlapPotentiallyUniqueOne = trimmedQmddPathThatShouldNotSharedSignature | std::views::transform([](const QmddPathComponent& qmddPathComponent) { return qc::Control(qmddPathComponent.qubitAssociatedWithQmddNode, getControlQubitTypeForQmddNodeEdge(qmddPathComponent.qmddEdgeToChildNode)); });

            // TODO: Can there be gaps in the paths due to the QMDD optimizing QMDD nodes that represent the identity? At minimum the compare paths can have different lengths due to different optimizations already
            // being applied to one or both paths.
            const bool doesPotentiallyUniqueAndComparedToSignatureOverlap = std::ranges::find_first_of(
                                                                                    viewOfPotentiallyUniquePathSignature.begin(),
                                                                                    viewOfPotentiallyUniquePathSignature.end(),
                                                                                    viewOfPathSignatureThatShouldNotOverlapPotentiallyUniqueOne.begin(),
                                                                                    viewOfPathSignatureThatShouldNotOverlapPotentiallyUniqueOne.end()) != viewOfPotentiallyUniquePathSignature.end();

            if (!doesPotentiallyUniqueAndComparedToSignatureOverlap) {
                return qmddPathWithPotentiallyUniqueSignature;
            }
        }
    }
    return std::nullopt;
}

std::optional<QmddTransformer::TransformationToUniqueQmddPathData> QmddTransformer::getTransformationDataToMakeAnyQmddPathUniqueViaSingleSignatureBitFlip(const std::vector<QmddPath>& qmddPathsContainingPotentiallyTransformableOne, const std::vector<QmddPath>& comparedToQmddPaths) {
    for (const QmddPath& potentiallyTransformableQmddPath: qmddPathsContainingPotentiallyTransformableOne) {
        if (const std::optional<TransformationToUniqueQmddPathData>& fetchedQmddPathTransformationData = getTransformationDataToMakeQmddPathUniqueViaSingleSignatureBitFlip(potentiallyTransformableQmddPath, comparedToQmddPaths); fetchedQmddPathTransformationData.has_value()) {
            return fetchedQmddPathTransformationData;
        }
    }
    return std::nullopt;
}

std::optional<QmddTransformer::TransformationToUniqueQmddPathData> QmddTransformer::getTransformationDataToMakeQmddPathUniqueViaSingleSignatureBitFlip(const QmddPath& qmddPathToTurnUnique, const std::vector<QmddPath>& comparedToQmddPaths) {
    if (qmddPathToTurnUnique.size() < 2 || std::ranges::any_of(
                                                   comparedToQmddPaths,
                                                   [&qmddPathToTurnUnique](const QmddPath& comparedToQmddPath) {
                                                       // Can we exclude paths with a length smaller than 2 since the optimized away path components are technically still available in the tree but the last node of the path points to the 1-terminal.
                                                       return comparedToQmddPath.size() < 2 || comparedToQmddPath.front().qubitAssociatedWithQmddNode != qmddPathToTurnUnique.front().qubitAssociatedWithQmddNode;
                                                   })) {
        return std::nullopt;
    }

    auto viewOfSignatureBitsOfQmddPathToBeTurnedUnique                    = qmddPathToTurnUnique | std::views::drop(1) | std::views::transform([](const QmddPathComponent& qmddPathComponent) {
                                                             return qc::Control(qmddPathComponent.qubitAssociatedWithQmddNode, getControlQubitTypeForQmddNodeEdge(qmddPathComponent.qmddEdgeToChildNode));
                                                         });
    auto homogeneousIteratorViewOfSignatureBitsOfQmddPathToBeTurnedUnique = std::views::common(viewOfSignatureBitsOfQmddPathToBeTurnedUnique);

    // TODO: Simplify this logic
    qc::Controls signatureBitsContainerOfQmddPathToBeTurnedUnique(homogeneousIteratorViewOfSignatureBitsOfQmddPathToBeTurnedUnique.begin(), homogeneousIteratorViewOfSignatureBitsOfQmddPathToBeTurnedUnique.end());
    for (std::size_t flippedSignatureBitPos = 0; flippedSignatureBitPos < signatureBitsContainerOfQmddPathToBeTurnedUnique.size(); ++flippedSignatureBitPos) {
        qc::Control flippedSignatureBit = getControlQubitForQmddPathComponent(qmddPathToTurnUnique.at(1U + flippedSignatureBitPos));
        flippedSignatureBit.type        = flippedSignatureBit.type == qc::Control::Type::Pos ? qc::Control::Type::Neg : qc::Control::Type::Pos;

        auto viewOfSignatureBitsPerComparedToQmddPath = comparedToQmddPaths | std::views::transform([](const QmddPath& comparedToQmddPath) {
                                                            return comparedToQmddPath | std::views::drop(1) | std::views::transform([](const QmddPathComponent& qmddPathComponent) {
                                                                       return qc::Control(qmddPathComponent.qubitAssociatedWithQmddNode, getControlQubitTypeForQmddNodeEdge(qmddPathComponent.qmddEdgeToChildNode));
                                                                   });
                                                        });

        bool doesSignatureOfQmddPathWithFlippedSignatureBitMatchExistingQmddPath = false;
        for (const auto& signatureBitsOfComparedToPath: viewOfSignatureBitsPerComparedToQmddPath) {
            if (std::ranges::all_of(
                        signatureBitsOfComparedToPath, [&signatureBitsContainerOfQmddPathToBeTurnedUnique, flippedSignatureBit](const qc::Control signatureBitOfComparedToPath) {
                            if (const auto matchingSignatureBitBasedOnQubitInCheckQmddPath = signatureBitsContainerOfQmddPathToBeTurnedUnique.find(signatureBitOfComparedToPath.qubit); matchingSignatureBitBasedOnQubitInCheckQmddPath != signatureBitsContainerOfQmddPathToBeTurnedUnique.cend()) {
                                const qc::Control::Type potentiallyFlippedSignatureBitOfQmddPathToBeTurnedUnique = matchingSignatureBitBasedOnQubitInCheckQmddPath->qubit == flippedSignatureBit.qubit ? flippedSignatureBit.type : matchingSignatureBitBasedOnQubitInCheckQmddPath->type;
                                return potentiallyFlippedSignatureBitOfQmddPathToBeTurnedUnique == signatureBitOfComparedToPath.type;
                            }
                            return false;
                        })) {
                doesSignatureOfQmddPathWithFlippedSignatureBitMatchExistingQmddPath = true;
                break;
            }
        }

        // TODO: The edge to the target qubit needs to be omitted?
        if (!doesSignatureOfQmddPathWithFlippedSignatureBitMatchExistingQmddPath) {
            // We assume that the to be made unique qmdd path P starting in some qmdd node N is located in the p' or n' edge of the associated qmdd node N and starts with N.
            // The goal of this function is to make P unique in the associated subtree (unique in the set of either the p' or n' paths of the node N) thus to perform the qubit signature flip for P
            // we also need to include the respective control qubit to follow the p' or n' edge of N into the set of control qubits which we otherwise skipped in our check for a unique path.
            auto viewOfSignatureBitsInQmddPathToBeTurnedUniqueToReachTargetQubit            = qmddPathToTurnUnique | std::views::take(1U + flippedSignatureBitPos) | std::views::transform([](const QmddPathComponent& qmddPathComponent) {
                                                                                       return qc::Control(qmddPathComponent.qubitAssociatedWithQmddNode, getControlQubitTypeForQmddNodeEdge(qmddPathComponent.qmddEdgeToChildNode));
                                                                                   });
            auto homogeneousViewOfSignatureBitsInQmddPathToBeTurnedUniqueToReachTargetQubit = std::views::common(viewOfSignatureBitsInQmddPathToBeTurnedUniqueToReachTargetQubit);
            return TransformationToUniqueQmddPathData({.controlQubitsFromFirstNodeInPathToTargetQubit = qc::Controls(homogeneousViewOfSignatureBitsInQmddPathToBeTurnedUniqueToReachTargetQubit.begin(), homogeneousViewOfSignatureBitsInQmddPathToBeTurnedUniqueToReachTargetQubit.end()),
                                                       .targetQubit                                   = qmddPathToTurnUnique.at(1U + flippedSignatureBitPos).qubitAssociatedWithQmddNode});
        }
    }
    //
    // std::vector<qc::Control> signatureBitsContainerOfQmddPathToBeTurnedUnique;
    // signatureBitsContainerOfQmddPathToBeTurnedUnique.reserve(qmddPathToTurnUnique.size() - 1U);
    // for (auto qmddPathComponentIterator = std::next(qmddPathToTurnUnique.cbegin()); qmddPathComponentIterator != qmddPathToTurnUnique.cend(); ++qmddPathComponentIterator) {
    //     signatureBitsContainerOfQmddPathToBeTurnedUnique[static_cast<std::size_t>(std::distance(qmddPathComponentIterator, qmddPathToTurnUnique.end()))] = qc::Control(qmddPathComponentIterator->qubitAssociatedWithQmddNode, getControlQubitTypeForQmddNodeEdge(qmddPathComponentIterator->qmddEdgeToChildNode));
    // }
    // std::vector<std::optional<qc::Qubit>> firstQubitWithMatchingSignaturePerComparedToPath(comparedToQmddPaths.size() - 1U);
    // for (const QmddPath& comparedQmddPath : comparedToQmddPaths | std::views::filter([&qmddPathToTurnUnique](const QmddPath& qmddPath) { return !qmddPath.empty() && qmddPath.front().qubitAssociatedWithQmddNode == qmddPathToTurnUnique.front().qubitAssociatedWithQmddNode})) {
    //     if (comparedQmddPath.empty() || comparedQmddPath.front().qubitAssociatedWithQmddNode != qmddPathToTurnUnique.front().qubitAssociatedWithQmddNode) {
    //         continue;
    //     }
    //
    //     auto qmddPathToTurnUniqueIterator = std::next(qmddPathToTurnUnique.cbegin());
    //     auto endIteratorOfQmddPathToTurnUnique = qmddPathToTurnUnique.cend();
    //
    //
    //
    //     // TODO: One could optimize whether the first flipped signature bit should be higher or lower in the QMDD tree.
    //     std::optional<qc::Qubit> firstQubitWithMatchingSignatureBetweenPaths;
    //     for (auto comparedToQmddPathIterator = std::next(comparedQmddPath.cbegin()); comparedToQmddPathIterator != comparedQmddPath.cend(); ++comparedToQmddPathIterator) {
    //         // TODO: Invert signature bit before comparison
    //         const auto [qubitOfQmddNodeInCheckedPath, signatureBitOfQmddNodeInCheckedPath] = *qmddPathToTurnUniqueIterator;
    //         const auto [qubitOfQmddNodeInComparedToPath, signatureBitOfQmddNodeInComparedToPath] = *comparedToQmddPathIterator;
    //
    //         // Can this case happen that a qubit in one of the paths is omitted since it was transformed to the identity?
    //         if (qubitOfQmddNodeInCheckedPath > qubitOfQmddNodeInComparedToPath) {
    //             ++qmddPathToTurnUniqueIterator;
    //             continue;
    //         }
    //         if (qubitOfQmddNodeInComparedToPath > qubitOfQmddNodeInCheckedPath) {
    //             continue;
    //         }
    //
    //         if (qubitOfQmddNodeInCheckedPath == qubitOfQmddNodeInComparedToPath && signatureBitOfQmddNodeInCheckedPath == signatureBitOfQmddNodeInComparedToPath) {
    //             // Path can only be made unique if at most one signature bit matches.
    //             if (firstQubitWithMatchingSignatureBetweenPaths.has_value()) {
    //                 return std::nullopt;
    //             }
    //             firstQubitWithMatchingSignatureBetweenPaths = qubitOfQmddNodeInCheckedPath;
    //             break;
    //         }
    //     }
    //     // auto signatureOfComparedToQmddPath = comparedQmddPath
    //     //     | std::views::drop(1)
    //     //     | std::views::transform([](const QmddPathComponent& qmddPathComponent) { return qc::Control(qmddPathComponent.qubitAssociatedWithQmddNode, getControlQubitTypeForQmddNodeEdge(qmddPathComponent.qmddEdgeToChildNode)); });
    //}
    return std::nullopt;
}
