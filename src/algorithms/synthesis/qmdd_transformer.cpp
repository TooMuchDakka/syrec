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

#include "algorithms/synthesis/qmddTransformation/qmdd_path_definitions.hpp"
#include "algorithms/synthesis/qmddTransformation/qmdd_transformation_operations.hpp"
#include "algorithms/synthesis/qmddTransformation/qmdd_traversal.hpp"
#include "dd/Export.hpp"
#include "dd/Operations.hpp"

#include <queue>

using namespace syrec;

namespace {

} // namespace

bool QmddTransformer::synthesizeQmdd(dd::mEdge edgeToQmddRoot, QmddTransformationStatistic* optionalTransformationStatistics, const QmddDumpConfig* optionalQmddDumpConfig) const {
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

    if (optionalTransformationStatistics != nullptr) {
        const auto transformationFinishedTime                                 = std::chrono::steady_clock::now();
        optionalTransformationStatistics->transformationRuntimeInMilliseconds = static_cast<std::uint64_t>((transformationFinishedTime - transformationStartTime).count());
    }
    return !forceCancellationOfTransformation;
}

dd::mEdge QmddTransformer::constructQmddFromGatesOfQuantumComputation(const qc::QuantumComputation& quantumComputation, dd::Package& qmddPackage, const QmddDumpConfig* optionalQmddDumpConfig) {
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

void QmddTransformer::exportQmddToFile(const dd::mEdge* edgeToRootNodeOfQmdd, const QmddDumpConfig* optionalQmddDumpConfig, const QmddExportOutputStreamOperation qmddExportOutputStreamOperation) {
    if (edgeToRootNodeOfQmdd == nullptr || optionalQmddDumpConfig == nullptr) {
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

// TODO: Introduct distinction between untrimmed and trimmed path with the former being returned by this function and potentially used as input for the next qmdd transformation algorithm steps while the latter is returned
// by the initial built of all qmdd paths of the currently processed qmdd node.
// TODO: Can paths that are considered as potentially unique or search through be skipped if they are empty or have a size smaller than one?
// std::optional<UnoptimizedQmddPath> QmddTransformer::findQmddPathWithUniqueSignature(const std::vector<OptimizedQmddPath>& qmddPathsToSearchForUniqueOne, const std::vector<OptimizedQmddPath>& qmddPathsDefiningComparedToSignatures) {
//     for (const OptimizedQmddPath& untrimmedQmddPathWithPotentiallyUniqueSignature: qmddPathsToSearchForUniqueOne) {
//         QmddPathGenerator fullUntrimmedQmddPathWithPotentiallyUniqueSignatureGenerator(untrimmedQmddPathWithPotentiallyUniqueSignature);
//
//         for (const UnoptimizedQmddPath* generatedFullUntrimmedQmddPathWithPotentiallyUniqueSignature = fullUntrimmedQmddPathWithPotentiallyUniqueSignatureGenerator.tryGenerateNextPath();
//              generatedFullUntrimmedQmddPathWithPotentiallyUniqueSignature != nullptr && generatedFullUntrimmedQmddPathWithPotentiallyUniqueSignature->size() > 1;
//              generatedFullUntrimmedQmddPathWithPotentiallyUniqueSignature = fullUntrimmedQmddPathWithPotentiallyUniqueSignatureGenerator.tryGenerateNextPath()) {
//             // We need to drop the first component of both QMDD paths due to the uniqueness check starting in the first child qmdd node of each path.
//             const auto trimmedQmddPathSignatureOfPotentiallyUniquePath = *generatedFullUntrimmedQmddPathWithPotentiallyUniqueSignature | std::views::drop(1) | std::views::transform(getControlQubitFromSignatureOfQmddPathComponent);
//
//             // TODO: What if qmdd paths have different lengths, this should not happen but could
//             // TODO: Refactor into own function
//             const bool existsQmddPathThatMatchesPotentiallyUniqueOneInComparedToQmddPathCollection = std::ranges::any_of(qmddPathsDefiningComparedToSignatures,
//                                                                                                                          [&trimmedQmddPathSignatureOfPotentiallyUniquePath](const OptimizedQmddPath& untrimmedQmddPathNotAllowedToOverlapPotentiallyUniqueOne) {
//                                                                                                                              bool existsQmddPathThatMatchesPotentiallyUniqueOneInComparedToQmddPathCombinations = false;
//
//                                                                                                                              QmddPathGenerator fullUntrimmedQmddPathNotAllowedToOverlapPotentiallyUniqueOneGenerator(untrimmedQmddPathNotAllowedToOverlapPotentiallyUniqueOne);
//
//                                                                                                                              for (const UnoptimizedQmddPath* generatedFullUntrimmedQmddPathNotAllowedToOverlapPotentiallyUniqueOne = fullUntrimmedQmddPathNotAllowedToOverlapPotentiallyUniqueOneGenerator.tryGenerateNextPath();
//                                                                                                                                   generatedFullUntrimmedQmddPathNotAllowedToOverlapPotentiallyUniqueOne != nullptr && generatedFullUntrimmedQmddPathNotAllowedToOverlapPotentiallyUniqueOne->size() > 1;
//                                                                                                                                   generatedFullUntrimmedQmddPathNotAllowedToOverlapPotentiallyUniqueOne = fullUntrimmedQmddPathNotAllowedToOverlapPotentiallyUniqueOneGenerator.tryGenerateNextPath()) {
//                                                                                                                                  const auto trimmedQmddPathNotAllowedToOverlapPotentiallyUniqueOne             = *generatedFullUntrimmedQmddPathNotAllowedToOverlapPotentiallyUniqueOne | std::views::drop(1) | std::views::transform(getControlQubitFromSignatureOfQmddPathComponent);
//                                                                                                                                  existsQmddPathThatMatchesPotentiallyUniqueOneInComparedToQmddPathCombinations = std::ranges::equal(trimmedQmddPathSignatureOfPotentiallyUniquePath, trimmedQmddPathNotAllowedToOverlapPotentiallyUniqueOne);
//                                                                                                                              }
//                                                                                                                              return existsQmddPathThatMatchesPotentiallyUniqueOneInComparedToQmddPathCombinations;
//                                                                                                                          });
//
//             if (!existsQmddPathThatMatchesPotentiallyUniqueOneInComparedToQmddPathCollection) {
//                 return UnoptimizedQmddPath(*generatedFullUntrimmedQmddPathWithPotentiallyUniqueSignature);
//             }
//         }
//     }
//     return std::nullopt;
// }

// std::optional<QmddTransformer::UniqueSignatureTransformationOperands> QmddTransformer::getOperandsToMakeOneOfQmddPathSignaturesUnique(const std::vector<OptimizedQmddPath>& qmddPathsContainingPotentiallyTransformableOne, const std::vector<OptimizedQmddPath>& comparedToQmddPaths) {
//     for (const OptimizedQmddPath& potentiallyTransformableQmddPath: qmddPathsContainingPotentiallyTransformableOne) {
//         if (const std::optional<UniqueSignatureTransformationOperands>& fetchedQmddPathTransformationData = getOperandsToMakeQmddPathSignatureUnique(potentiallyTransformableQmddPath, comparedToQmddPaths); fetchedQmddPathTransformationData.has_value()) {
//             return fetchedQmddPathTransformationData;
//         }
//     }
//     return std::nullopt;
// }

// std::optional<QmddTransformer::UniqueSignatureTransformationOperands> QmddTransformer::getOperandsToMakeQmddPathSignatureUnique(const OptimizedQmddPath& qmddPathToTurnUnique, const std::vector<OptimizedQmddPath>& comparedToQmddPaths) {
// TODO: Handle empty compare to qmdd paths collection
// QmddPathGenerator qmddPathToTurnUniqueGenerator(qmddPathToTurnUnique);
// for (const UnoptimizedQmddPath* generatedQmddPathToTurnUnique = qmddPathToTurnUniqueGenerator.tryGenerateNextPath();
//      generatedQmddPathToTurnUnique != nullptr && generatedQmddPathToTurnUnique->size() > 1;
//      generatedQmddPathToTurnUnique = qmddPathToTurnUniqueGenerator.tryGenerateNextPath()) {
//     auto trimmedViewOfSignatureBitsOfQmddPathToTurnUnique     = *generatedQmddPathToTurnUnique | std::views::drop(1) | std::views::transform(getControlQubitFromSignatureOfQmddPathComponent);
//     auto homogeneousViewOfSignatureBitsOfQmddPathToTurnUnique = std::views::common(trimmedViewOfSignatureBitsOfQmddPathToTurnUnique);
//
//     qc::Controls signatureBitsContainerOfQmddPathToBeTurnedUnique(homogeneousViewOfSignatureBitsOfQmddPathToTurnUnique.begin(), homogeneousViewOfSignatureBitsOfQmddPathToTurnUnique.end());
//     for (std::size_t signatureBitIdx = 0U; signatureBitIdx < signatureBitsContainerOfQmddPathToBeTurnedUnique.size(); ++signatureBitIdx) {
//         // TODO: Do we have to skip the first entry in the qmdd path?
//         // We need to ignore the first entry in the checked qmdd path since our goal is to turn the path unique so that it can be shifted from the current edge of the parent node (i.e. the qmdd node of the first component of the qmdd path) to another edge.
//         const qc::Control originalSignatureBitValue = getControlQubitFromSignatureOfQmddPathComponent(generatedQmddPathToTurnUnique->at(1U + signatureBitIdx));
//         const auto        flippedSignatureBitValue  = qc::Control(originalSignatureBitValue.qubit, originalSignatureBitValue.type == qc::Control::Type::Pos ? qc::Control::Type::Neg : qc::Control::Type::Pos);
//
//         // Flip signature bit "polarity" and check whether generated signature is unique
//         auto extractSignatureBitNode         = signatureBitsContainerOfQmddPathToBeTurnedUnique.extract(originalSignatureBitValue);
//         extractSignatureBitNode.value().type = flippedSignatureBitValue.type;
//         signatureBitsContainerOfQmddPathToBeTurnedUnique.insert(std::move(extractSignatureBitNode));
//
//         // TODO: The generated signature could match an already existing one that we either already checked or will check in the future, can we cache these results?
//         // TODO: Perform check whether generated signature is unique.
//
//         bool didGeneratedSignatureMatchExistingOne = false;
//         for (const auto& comparedToQmddPath: comparedToQmddPaths) {
//             QmddPathGenerator comparedToQmddPathGenerator(comparedToQmddPath);
//             for (const UnoptimizedQmddPath* generatedComparedToQmddPath = comparedToQmddPathGenerator.tryGenerateNextPath();
//                  generatedComparedToQmddPath != nullptr && generatedComparedToQmddPath->size() > 1;
//                  generatedComparedToQmddPath = comparedToQmddPathGenerator.tryGenerateNextPath()) {
//                 // TODO: Check correct length (i.e. must be longer than one)
//                 // TODO: Check that first qubits of path are equal
//                 auto trimmedViewOfComparedToSignatureBits = *generatedComparedToQmddPath | std::views::drop(1) | std::views::transform(getControlQubitFromSignatureOfQmddPathComponent);
//
//                 didGeneratedSignatureMatchExistingOne = std::ranges::all_of(trimmedViewOfComparedToSignatureBits, [&signatureBitsContainerOfQmddPathToBeTurnedUnique](const qc::Control comparedToSignatureBit) {
//                     return signatureBitsContainerOfQmddPathToBeTurnedUnique.contains(comparedToSignatureBit);
//                 });
//             }
//         }
//
//         if (!didGeneratedSignatureMatchExistingOne) {
//             const qc::Qubit targetQubitDefiningFlippedSignatureBit = flippedSignatureBitValue.qubit;
//             qc::Controls    controlQubitsToReachTargetQubitFromStartOfQmddPath;
//             for (std::size_t signatureIdxToReachTargetQubit = 0U; signatureIdxToReachTargetQubit < 1U + signatureBitIdx; ++signatureIdxToReachTargetQubit) {
//                 controlQubitsToReachTargetQubitFromStartOfQmddPath.emplace(getControlQubitFromSignatureOfQmddPathComponent(generatedQmddPathToTurnUnique->at(signatureIdxToReachTargetQubit)));
//             }
//             return UniqueSignatureTransformationOperands(controlQubitsToReachTargetQubitFromStartOfQmddPath, targetQubitDefiningFlippedSignatureBit);
//         }
//
//         // If the current signature formed by the contents of the signature bit lookup is not unique then flip the modified signature bit back to its original value.
//         extractSignatureBitNode              = signatureBitsContainerOfQmddPathToBeTurnedUnique.extract(flippedSignatureBitValue);
//         extractSignatureBitNode.value().type = originalSignatureBitValue.type;
//         signatureBitsContainerOfQmddPathToBeTurnedUnique.insert(std::move(extractSignatureBitNode));
//     }
// }
// return std::nullopt;
//}

// std::size_t QmddTransformer::getNumberOfPathsToOneTerminalForQmddPath(const OptimizedQmddPath& qmddPath) noexcept {
//     auto nPaths = static_cast<std::size_t>(!qmddPath.empty());
//     for (auto qmddPathIterator = qmddPath.rbegin(); qmddPathIterator != qmddPath.rend(); ++qmddPathIterator) {
//         if (const QmddPathGap* qmddPathGap = std::get_if<QmddPathGap>(&*qmddPathIterator); qmddPathGap != nullptr) {
//             // TODO: Replace std::pow call
//             nPaths *= static_cast<std::size_t>(std::pow(2, qmddPathGap->nConsecutiveQubitInGap));
//         }
//     }
//     return nPaths;
// }
//
// std::size_t QmddTransformer::getNumberOfPathsToOneTerminalForQmddPaths(const std::vector<OptimizedQmddPath>& qmddPaths) noexcept {
//     std::size_t nPaths = 0U;
//     for (const auto& qmddPath: qmddPaths) {
//         nPaths += getNumberOfPathsToOneTerminalForQmddPath(qmddPath);
//     }
//     return nPaths;
// }
