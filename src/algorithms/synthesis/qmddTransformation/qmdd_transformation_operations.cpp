/*
 * Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
 * Copyright (c) 2025 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

#include "algorithms/synthesis/qmddTransformation/qmdd_transformation_operations.hpp"

#include "algorithms/synthesis/qmddTransformation/qmdd_path_operations.hpp"
#include "algorithms/synthesis/qmdd_transformer.hpp"
#include "dd/Operations.hpp"

namespace {
    qc::Controls getControlQubitsFromSignatureOfQmddPathComponents(const std::vector<syrec::QmddPathComponent>& qmddPathComponents) noexcept {
        qc::Controls controlQubits;
        for (const syrec::QmddPathComponent& qmddPathComponent: qmddPathComponents) {
            controlQubits.emplace(getControlQubitFromSignatureOfQmddPathComponent(qmddPathComponent));
        }
        return controlQubits;
    }
} // namespace

namespace syrec {
    void applyMCXGateToQmdd(qc::QuantumComputation& quantumComputation, dd::Package& qmddPkg, const dd::mEdge& edgeToRootNodeOfQmdd, const qc::Qubit targetQubit, const qc::Controls& controlQubits) {
        quantumComputation.mcx(controlQubits, targetQubit);
        const qc::Operation& generatedQuantumOperationForMCXGate = *quantumComputation.back();
        // TODO:
        //++numGates;
        dd::applyUnitaryOperation(generatedQuantumOperationForMCXGate, edgeToRootNodeOfQmdd, qmddPkg, {}, false);
    }

    // This algorithm swaps the paths present in the p' edge to the n edge and vice versa.
    // TODO: In the reimplementation this check is not implemented: "If n' and p paths exists, we move on to P2 algorithm"
    // Refer to the P1 algorithm of http://www.informatik.uni-bremen.de/agra/doc/konf/12aspdac_qmdd_synth_rev.pdf
    bool trySwapPathsOfEdgesOfQmddNode(qc::QuantumComputation& quantumComputation, dd::Package& qmddPkg, const QmddNodeAndPathsPerEdge& qmddNodeAndEdgePaths) {
        if (getNumberOfPathsToOneTerminalForQmddPaths(qmddNodeAndEdgePaths.pPrimeEdgePaths) <= getNumberOfPathsToOneTerminalForQmddPaths(qmddNodeAndEdgePaths.nEdgePaths)) {
            return false;
        }

        const dd::mEdge* edgeToRootNode = tryGetEdgeToQmddRootNode(qmddPkg);
        assert(edgeToRootNode != nullptr);
        assert(edgeToRootNode->p != nullptr);
        const dd::mNode& rootNode = *edgeToRootNode->p;

        const dd::Qubit targetQubit = qmddNodeAndEdgePaths.associatedQmddNode.get().v;
        if (targetQubit == rootNode.v) {
            const qc::Controls controlQubitsForPathFromRootToCurrentNode;
            applyMCXGateToQmdd(quantumComputation, qmddPkg, *edgeToRootNode, targetQubit, controlQubitsForPathFromRootToCurrentNode);
        } else {
            // TODO: Iterate all paths from the root to the current node and record the controls for each path P as c(P) then add a toffoli gate TOFF(controls: c(P), target: current)
            for (const UnoptimizedQmddPath& pathFromRootToCurrentNode: getAllPathsFromRootToNode(rootNode, qmddNodeAndEdgePaths.associatedQmddNode)) {
                assert(!pathFromRootToCurrentNode.empty());
                const qc::Controls controlQubitsForPathFromRootToCurrentNode = getControlQubitsFromSignatureOfQmddPathComponents(pathFromRootToCurrentNode);
                // TODO: Root can change?
                applyMCXGateToQmdd(quantumComputation, qmddPkg, *edgeToRootNode, targetQubit, controlQubitsForPathFromRootToCurrentNode);
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
    bool tryShiftUniquePathsOfQmddNode(qc::QuantumComputation& quantumComputation, dd::Package& qmddPkg, const QmddNodeAndPathsPerEdge& qmddNodeAndEdgePaths) {
        // TODO: Currently SHE exception with code 0xc0000005 for multiple paths since QMDD could be changed after an operation is applied.
        // TODO: We currently restrict ourselves to the first found unique path while in the reference paper all unique paths are shifted.
        const std::optional<UnoptimizedQmddPath> uniquePathThatCanBeShiftedInPPrimeEdgeSubtree = findFirstQmddPathWithUniqueSignature(qmddNodeAndEdgePaths.pPrimeEdgePaths, qmddNodeAndEdgePaths.nEdgePaths, true);
        const std::optional<UnoptimizedQmddPath> uniquePathThatCanBeShiftedInNPrimeEdgeSubtree = !uniquePathThatCanBeShiftedInPPrimeEdgeSubtree.has_value() ? findFirstQmddPathWithUniqueSignature(qmddNodeAndEdgePaths.nPrimeEdgePaths, qmddNodeAndEdgePaths.pEdgePaths, true) : std::nullopt;

        if (!uniquePathThatCanBeShiftedInPPrimeEdgeSubtree.has_value() && !uniquePathThatCanBeShiftedInNPrimeEdgeSubtree.has_value()) {
            return false;
        }

        const dd::mEdge* edgeToRootNode = tryGetEdgeToQmddRootNode(qmddPkg);
        assert(edgeToRootNode != nullptr);
        assert(edgeToRootNode->p != nullptr);
        const dd::mNode& rootNode = *edgeToRootNode->p;

        const qc::Qubit            targetQubit                  = qmddNodeAndEdgePaths.associatedQmddNode.get().v;
        const UnoptimizedQmddPath& shiftableQmddPathFromSubtree = uniquePathThatCanBeShiftedInPPrimeEdgeSubtree.has_value() ? *uniquePathThatCanBeShiftedInPPrimeEdgeSubtree : *uniquePathThatCanBeShiftedInNPrimeEdgeSubtree;
        assert(!shiftableQmddPathFromSubtree.empty());

        qc::Controls controlQubitsForQmddPathStartingFromNodeToOneTerminal;
        // TODO: Update comment
        // The control qubits of the operation to shift a unique path P includes the control qubits from the root up to but excluding the current qmdd node N as well as the control qubits for the subpath from the first child
        // node of N to the 1-terminal. Since we performed the transformation of P to its associated control qubits for each component of the path we also need to remove the generated control qubit for the current qmdd node N on P
        // since the target qubit of the to be generated operation is defined as the associated qubit of N.
        for (const auto& controlQubit: shiftableQmddPathFromSubtree | std::views::drop(1) | std::views::transform(getControlQubitFromSignatureOfQmddPathComponent)) {
            controlQubitsForQmddPathStartingFromNodeToOneTerminal.emplace(controlQubit);
        }

        const std::vector<UnoptimizedQmddPath> pathsFromRootToCurrentNode = getAllPathsFromRootToNode(rootNode, qmddNodeAndEdgePaths.associatedQmddNode);
        if (pathsFromRootToCurrentNode.empty()) {
            // TODO: Maybe use assert(rootNode.v == qmddNodeAndEdgePaths.associatedQmddNode.get().v); instead?
            assert(rootNode.v == targetQubit);
            applyMCXGateToQmdd(quantumComputation, qmddPkg, *edgeToRootNode, targetQubit, controlQubitsForQmddPathStartingFromNodeToOneTerminal);
        } else {
            for (const UnoptimizedQmddPath& pathFromRootToCurrentNode: pathsFromRootToCurrentNode) {
                assert(!pathFromRootToCurrentNode.empty());
                qc::Controls controlQubitsToTargetPathFromRootToCurrentNode = getControlQubitsFromSignatureOfQmddPathComponents(pathFromRootToCurrentNode);
                controlQubitsToTargetPathFromRootToCurrentNode.insert(controlQubitsForQmddPathStartingFromNodeToOneTerminal.cbegin(), controlQubitsForQmddPathStartingFromNodeToOneTerminal.cend());
                // TODO: Root could change?
                applyMCXGateToQmdd(quantumComputation, qmddPkg, *edgeToRootNode, targetQubit, controlQubitsToTargetPathFromRootToCurrentNode);
                // TODO: Application of QMDD operation can change structure of QMDD thus previously determined paths may no longer exist
                // TODO: One could check whether parts of the signature from the root to the node still exist and were not targeted by a previous gate?
                break;
            }
        }
        return true;
    }

    bool tryMakeSharedPathOfQmddNodeUnique(qc::QuantumComputation& quantumComputation, dd::Package& qmddPkg, const QmddNodeAndPathsPerEdge& qmddNodeAndEdgePaths) {
        const std::optional<ToUniqueQmddPathSignatureOperands> transformationDataForQmddPathOfPPrimeSubtree = getOperandsToMakeOneOfQmddPathSignaturesUnique(qmddNodeAndEdgePaths.pPrimeEdgePaths, qmddNodeAndEdgePaths.nEdgePaths);
        const std::optional<ToUniqueQmddPathSignatureOperands> transformationDataForQmddPathOfNPrimeSubtree = !transformationDataForQmddPathOfPPrimeSubtree.has_value() ? getOperandsToMakeOneOfQmddPathSignaturesUnique(qmddNodeAndEdgePaths.nPrimeEdgePaths, qmddNodeAndEdgePaths.pEdgePaths) : std::nullopt;

        if (!transformationDataForQmddPathOfPPrimeSubtree.has_value() && !transformationDataForQmddPathOfNPrimeSubtree.has_value()) {
            return false;
        }

        const dd::mEdge* edgeToRootNode = tryGetEdgeToQmddRootNode(qmddPkg);
        assert(edgeToRootNode != nullptr);
        assert(edgeToRootNode->p != nullptr);
        const dd::mNode& rootNode = *edgeToRootNode->p;

        const ToUniqueQmddPathSignatureOperands& transformationDataToTurnQmddPathUniqueStartingFromNodeToOneTerminal = transformationDataForQmddPathOfPPrimeSubtree.has_value() ? *transformationDataForQmddPathOfPPrimeSubtree : *transformationDataForQmddPathOfNPrimeSubtree;

        const qc::Qubit                        targetQubit                = transformationDataToTurnQmddPathUniqueStartingFromNodeToOneTerminal.targetQubit;
        const std::vector<UnoptimizedQmddPath> pathsFromRootToCurrentNode = getAllPathsFromRootToNode(rootNode, qmddNodeAndEdgePaths.associatedQmddNode);
        if (pathsFromRootToCurrentNode.empty()) {
            assert(rootNode.v == qmddNodeAndEdgePaths.associatedQmddNode.get().v);
            applyMCXGateToQmdd(quantumComputation, qmddPkg, *edgeToRootNode, targetQubit, transformationDataToTurnQmddPathUniqueStartingFromNodeToOneTerminal.controlQubitsFromFirstNodeInPathToTargetQubit);
        } else {
            for (const UnoptimizedQmddPath& pathFromRootToCurrentNode: pathsFromRootToCurrentNode) {
                assert(!pathFromRootToCurrentNode.empty());
                qc::Controls controlQubitsToTargetPathFromRootToCurrentNode = getControlQubitsFromSignatureOfQmddPathComponents(pathFromRootToCurrentNode);
                controlQubitsToTargetPathFromRootToCurrentNode.insert(
                        transformationDataToTurnQmddPathUniqueStartingFromNodeToOneTerminal.controlQubitsFromFirstNodeInPathToTargetQubit.cbegin(),
                        transformationDataToTurnQmddPathUniqueStartingFromNodeToOneTerminal.controlQubitsFromFirstNodeInPathToTargetQubit.cend());
                // TODO: Root could change?
                applyMCXGateToQmdd(quantumComputation, qmddPkg, *edgeToRootNode, targetQubit, controlQubitsToTargetPathFromRootToCurrentNode);
                // TODO: Application of QMDD operation can change structure of QMDD thus previously determined paths may no longer exist
                // TODO: One could check whether parts of the signature from the root to the node still exist and were not targeted by a previous gate?
                break;
            }
        }
        return true;
    }

    // TODO: Can the transformation of the QMDD result in a QMDD that only consists of the one/zero terminal?
    const dd::mEdge* tryGetEdgeToQmddRootNode(dd::Package& qmddPkgToGetRootFrom) {
        const auto& setOfRootNodes = qmddPkgToGetRootFrom.getRootSet<dd::mNode>();
        return !setOfRootNodes.empty() ? &setOfRootNodes.begin()->first : nullptr;
    }

    bool terminate(const dd::mNode& nodeToCheck) {
        const auto& edgesOfQmddNode = nodeToCheck.e;
        assert(edgesOfQmddNode.size() == 4);
        // Original implementation also checked that N' edge points to zero terminal but according to the reference paper it should be sufficient to only check the P' edge since
        // the transformations applied by the algorithm should result in a QMDD node which represents the identity by the P' edge pointing to the zero terminal which in turn would also mean that the N' edge points to the zero terminal.
        // TODO: Should we keep the assert?
        //assert( edgesOfQmddNode[static_cast<std::size_t>(QmddNodeEdge::P_Prime)].isZeroTerminal() && edgesOfQmddNode[static_cast<std::size_t>(QmddNodeEdge::N_Prime)].isZeroTerminal());
        return edgesOfQmddNode[convertQmddNodeEdgeEnumValueToArrayIdx(QmddNodeEdge::PPrime)].isZeroTerminal();
    }
} //namespace syrec
