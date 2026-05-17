/*
 * Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
 * Copyright (c) 2025 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

#include "algorithms/synthesis/qmdd_transformer.hpp"

#include "algorithms/synthesis/qmddTransformation/qmdd_dumper.hpp"
#include "algorithms/synthesis/qmddTransformation/qmdd_path_definitions.hpp"
#include "algorithms/synthesis/qmddTransformation/qmdd_transformation_operations.hpp"
#include "algorithms/synthesis/qmddTransformation/qmdd_traversal.hpp"
#include "dd/Operations.hpp"

#include <queue>

using namespace syrec;

bool QmddTransformer::synthesizeQmdd(dd::mEdge edgeToQmddRoot, QmddTransformationStatistic* optionalTransformationStatistics, BaseQmddDumper* qmddDumper) const {
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

    if (qmddDumper != nullptr) {
        qmddDumper->dumpQmdd(tryGetEdgeToQmddRootNode(qmddPkg));
    }

    // queue for the nodes to be processed in a breadth-first manner.
    std::queue<dd::mEdge> queue{};
    queue.emplace(edgeToQmddRoot);

    // TODO: We should not need such a set since the processing should only move downwards in the QMDD tree?
    // set of nodes that have already been processed.
    // std::unordered_set<dd::mEdge> visited{};

    const auto transformationStartTime           = std::chrono::steady_clock::now();
    bool       forceCancellationOfTransformation = false;

    // TODO: Compare with reference algorithm from dd_synthesis
    // TODO: Add handling for garbage/ancillary qubits

    // TODO: Is the queue really necessary when we are often jumping back to the root in case that an operation was performed?
    // TODO: Due to jumping to the root one could use the visited set to skip already processed subtrees?
    while (!queue.empty() && !forceCancellationOfTransformation) {
        const dd::mEdge current = queue.front();
        queue.pop();

        assert(current.p != nullptr);
        const dd::mNode& nodeToProcess = *current.p;
        assert(current.p->e.size() == 4);

        if (terminate(nodeToProcess)) {
            for (const dd::mEdge& edgesOfCurrentNode: nodeToProcess.e) {
                if (edgesOfCurrentNode.isTerminal()) {
                    continue;
                }
                queue.emplace(edgesOfCurrentNode);
            }
            continue;
        }

        // TODO: In test_dd_synthesis_1 some of the found paths contain duplicate entries that are associated with the same qubit but a different edge.
        // P1 algorithm
        bool resetQueue = trySwapPathsOfEdgesOfQmddNode(qc, qmddPkg, getNPathsToOneTerminalPerEdgeOfQmddNode(nodeToProcess));
        // P2 algorithm.
        auto qmddPathsStartingFromNode = QmddNodeAndPathsPerEdge(nodeToProcess);
        getPathsToOneTerminalThroughEdgeOfQmddNode(nodeToProcess, QmddNodeEdge::N | QmddNodeEdge::PPrime | QmddNodeEdge::NPrime | QmddNodeEdge::P, qmddPathsStartingFromNode);
        // Note: The E1 |= E2 assignment operator is equal to E1 = E1 | E2 with the operator | not short circuiting does our P algorithms steps would still be evaluated in case the E1 is true thus explaining our usage of the E1 = E1 || E2 assignment.
        resetQueue = resetQueue || tryShiftUniquePathsOfQmddNode(qc, qmddPkg, qmddPathsStartingFromNode);

        // TODO: In the original paper the algorithm should continue with step P2 after P4 was performed but this might not take into account that the structure of the QMDD has changed after the associated operation was executed.
        // P3 and P4 algorithm
        if (!resetQueue) {
            if (!terminate(nodeToProcess)) {
                // TODO: Comment as to why this case can happen and how the reference algorithm is not able to cope with this case causing an infinite loop.
                if (!tryMakeSharedPathOfQmddNodeUnique(qc, qmddPkg, qmddPathsStartingFromNode)) {
                    forceCancellationOfTransformation = true;
                    continue;
                }
                resetQueue = true;
            }
        }

        if (resetQueue) {
            // The resetQueue variable should be set to true if any of the steps P1, P2, P3 or P4 applied an operation thus the qmdd export should dump the qmdd after said operation was applied thus allowing a "single-step" debugging with the dump file contents if necessary.
            //exportQuantumGate(*qc.get().back().get(), appliedOperation, optionalQmddDumpConfig);
            if (qmddDumper != nullptr) {
                qmddDumper->dumpQmdd(tryGetEdgeToQmddRootNode(qmddPkg));
            }

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

    if (optionalTransformationStatistics != nullptr) {
        const auto transformationFinishedTime                                 = std::chrono::steady_clock::now();
        optionalTransformationStatistics->transformationRuntimeInMilliseconds = static_cast<std::uint64_t>((transformationFinishedTime - transformationStartTime).count());
    }
    return !forceCancellationOfTransformation;
}

dd::mEdge QmddTransformer::constructQmddFromGatesOfQuantumComputation(const qc::QuantumComputation& quantumComputation, dd::Package& qmddPackage, BaseQmddDumper* qmddDumper) {
    // TODO: Implementation taken from dd::FunctionalityConstruction::buildFunctionality(...) which does not apply the inverse of each operation.
    auto permutation    = quantumComputation.initialLayout;
    auto edgeToRootNode = qmddPackage.createInitialMatrix(quantumComputation.getAncillary());

    for (auto op = quantumComputation.crbegin(); op != quantumComputation.crend(); ++op) {
        auto copyOfOperation = op->get()->clone();
        copyOfOperation->invert();
        // TODO: What is the difference between applying the unitary operation from the left/right?
        edgeToRootNode = dd::applyUnitaryOperation(*copyOfOperation, edgeToRootNode, qmddPackage, permutation, false);
        if (qmddDumper != nullptr) {
            qmddDumper->dumpQmdd(&edgeToRootNode);
        }
    }

    // correct permutation if necessary
    changePermutation(edgeToRootNode, permutation, quantumComputation.outputPermutation, qmddPackage);
    edgeToRootNode = qmddPackage.reduceAncillae(edgeToRootNode, quantumComputation.getAncillary());
    return qmddPackage.reduceGarbage(edgeToRootNode, quantumComputation.getGarbage());
}
