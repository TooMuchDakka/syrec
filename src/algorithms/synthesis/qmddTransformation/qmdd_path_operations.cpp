/*
 * Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
 * Copyright (c) 2025 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

#pragma once

#include "algorithms/synthesis/qmddTransformation/qmdd_path_operations.hpp"

#include "algorithms/synthesis/qmddTransformation/qmdd_path_definitions.hpp"
#include "algorithms/synthesis/qmddTransformation/qmdd_path_generator.hpp"
#include "ir/Definitions.hpp"
#include "ir/operations/Control.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <numeric>
#include <optional>
#include <variant>
#include <vector>

namespace syrec {
    std::size_t getUnrolledLengthOfOptimizedQmddPath(const OptimizedQmddPath& optimizedQmddPath) {
        return std::transform_reduce(optimizedQmddPath.cbegin(), optimizedQmddPath.cend(), 0U, std::plus{},
                                     [](const std::variant<QmddPathComponent, QmddPathGap>& qmddPathComponent) {
                                         const QmddPathGap* qmddPathGap = std::get_if<QmddPathGap>(&qmddPathComponent);
                                         return qmddPathGap ? qmddPathGap->nConsecutiveQubitInGap : 1U;
                                     });
    }

    std::size_t getNumberOfPathsToOneTerminalForQmddPath(const OptimizedQmddPath& qmddPath) {
        return qmddPath.empty() ? 0U : std::transform_reduce(qmddPath.cbegin(), qmddPath.cend(), 1U, std::multiplies{}, [](const std::variant<QmddPathComponent, QmddPathGap>& qmddPathComponent) {
            const QmddPathGap* qmddPathGap = std::get_if<QmddPathGap>(&qmddPathComponent);
            return qmddPathGap ? static_cast<std::size_t>(std::pow(2, qmddPathGap->nConsecutiveQubitInGap)) : 1U;
        });
    }

    std::optional<bool> doQmddPathSignaturesMatch(const UnoptimizedQmddPath& lQmddPath, const UnoptimizedQmddPath& rQmddPath, const bool skipFirstQmddPathEntry) {
        if (lQmddPath.size() != rQmddPath.size() || (!lQmddPath.empty() && lQmddPath.front().qubitAssociatedWithQmddNode != rQmddPath.front().qubitAssociatedWithQmddNode)) {
            return std::nullopt;
        }

        bool doQmddPathSignaturesMatch = true;
        for (std::size_t i = skipFirstQmddPathEntry ? 1U : 0U; i < lQmddPath.size() && doQmddPathSignaturesMatch; ++i) {
            doQmddPathSignaturesMatch = lQmddPath.at(i).qubitAssociatedWithQmddNode == rQmddPath.at(i).qubitAssociatedWithQmddNode && getControlQubitFromSignatureOfQmddPathComponent(lQmddPath.at(i)) == getControlQubitFromSignatureOfQmddPathComponent(rQmddPath.at(i));
        }
        return doQmddPathSignaturesMatch;
    }

    std::optional<bool> existsQmddPathWithSameSignature(const UnoptimizedQmddPath& referenceQmddPath, const OptimizedQmddPath& comparedToQmddPath, const bool skipFirstQmddPathEntry) {
        if (getUnrolledLengthOfOptimizedQmddPath(comparedToQmddPath) != referenceQmddPath.size()) {
            return std::nullopt;
        }
        bool              existsQmddPathWithSameSignature = false;
        QmddPathGenerator comparedToQmddPathsGenerator(comparedToQmddPath);
        if (!comparedToQmddPathsGenerator.canGenerateCombinations()) {
            return std::nullopt;
        }

        for (const UnoptimizedQmddPath* generatedComparedToQmddPath = comparedToQmddPathsGenerator.tryGenerateNextPath();
             generatedComparedToQmddPath != nullptr && generatedComparedToQmddPath->size() > 1 && !existsQmddPathWithSameSignature;
             generatedComparedToQmddPath = comparedToQmddPathsGenerator.tryGenerateNextPath()) {
            const std::optional<bool> comparisonResult = doQmddPathSignaturesMatch(referenceQmddPath, *generatedComparedToQmddPath, skipFirstQmddPathEntry);
            if (!comparisonResult.has_value()) {
                return std::nullopt;
            }
            existsQmddPathWithSameSignature |= *comparisonResult;
        }
        return existsQmddPathWithSameSignature;
    }

    std::optional<UnoptimizedQmddPath> findFirstQmddPathWithUniqueSignature(const OptimizedQmddPath& potentiallyUniqueQmddPath, const std::vector<OptimizedQmddPath>& comparedToQmddPaths, const bool skipFirstQmddPathEntry) {
        if (std::ranges::any_of(comparedToQmddPaths, [&potentiallyUniqueQmddPath](const OptimizedQmddPath& comparedToQmddPath) {
                return getUnrolledLengthOfOptimizedQmddPath(potentiallyUniqueQmddPath) != getUnrolledLengthOfOptimizedQmddPath(comparedToQmddPath);
            })) {
            return std::nullopt;
        }

        QmddPathGenerator potentiallyUniqueQmddPathGenerator(potentiallyUniqueQmddPath);
        if (!potentiallyUniqueQmddPathGenerator.canGenerateCombinations()) {
            return std::nullopt;
        }

        for (const UnoptimizedQmddPath* generatedPotentiallyUniqueQmddPath = potentiallyUniqueQmddPathGenerator.tryGenerateNextPath();
             generatedPotentiallyUniqueQmddPath != nullptr && generatedPotentiallyUniqueQmddPath->size() > 1;
             generatedPotentiallyUniqueQmddPath = potentiallyUniqueQmddPathGenerator.tryGenerateNextPath()) {
            if (std::ranges::none_of(comparedToQmddPaths, [generatedPotentiallyUniqueQmddPath, skipFirstQmddPathEntry](const OptimizedQmddPath& comparedToQmddPath) {
                    return existsQmddPathWithSameSignature(*generatedPotentiallyUniqueQmddPath, comparedToQmddPath, skipFirstQmddPathEntry).value_or(true);
                })) {
                return UnoptimizedQmddPath(*generatedPotentiallyUniqueQmddPath);
            }
        }
        return std::nullopt;
    }

    std::optional<UnoptimizedQmddPath> findFirstQmddPathWithUniqueSignature(const std::vector<OptimizedQmddPath>& potentiallyUniqueQmddPaths, const std::vector<OptimizedQmddPath>& comparedToQmddPaths, const bool skipFirstQmddPathEntry) {
        std::optional<UnoptimizedQmddPath> qmddPathWithUniqueSignature;
        for (std::size_t i = 0; i < potentiallyUniqueQmddPaths.size() && !qmddPathWithUniqueSignature.has_value(); ++i) {
            qmddPathWithUniqueSignature = findFirstQmddPathWithUniqueSignature(potentiallyUniqueQmddPaths.at(i), comparedToQmddPaths, skipFirstQmddPathEntry);
        }
        return qmddPathWithUniqueSignature;
    }

    bool operator==(const ToUniqueQmddPathSignatureOperands& lOperands, const ToUniqueQmddPathSignatureOperands& rOperands) noexcept {
        return lOperands.targetQubit == rOperands.targetQubit && lOperands.controlQubitsFromFirstNodeInPathToTargetQubit.size() == rOperands.controlQubitsFromFirstNodeInPathToTargetQubit.size() && std::ranges::all_of(lOperands.controlQubitsFromFirstNodeInPathToTargetQubit, [&rOperands](const qc::Control& lOperandControl) {
                   return rOperands.controlQubitsFromFirstNodeInPathToTargetQubit.contains(lOperandControl);
               });
    }

    std::ostream& operator<<(std::ostream& ostream, const ToUniqueQmddPathSignatureOperands& operandsOfQmddOperationToTurnQmddPathUnique) {
        ostream << "Target qubit: " << operandsOfQmddOperationToTurnQmddPathUnique.targetQubit << " | Control qubits: (";
        for (const qc::Control& control: operandsOfQmddOperationToTurnQmddPathUnique.controlQubitsFromFirstNodeInPathToTargetQubit) {
            ostream << control.toString();
        }
        ostream << ")";
        return ostream;
    }

    std::optional<ToUniqueQmddPathSignatureOperands> getOperandsToMakeQmddPathSignatureUnique(const OptimizedQmddPath& qmddPathToTurnUnique, const std::vector<OptimizedQmddPath>& comparedToQmddPaths) {
        if (std::ranges::any_of(comparedToQmddPaths, [&qmddPathToTurnUnique](const OptimizedQmddPath& comparedToQmddPath) {
                return getUnrolledLengthOfOptimizedQmddPath(qmddPathToTurnUnique) != getUnrolledLengthOfOptimizedQmddPath(comparedToQmddPath);
            }) ||
            comparedToQmddPaths.empty()) {
            return std::nullopt;
        }

        QmddPathGenerator qmddPathToTurnUniqueGenerator(qmddPathToTurnUnique);
        if (!qmddPathToTurnUniqueGenerator.canGenerateCombinations()) {
            return std::nullopt;
        }

        for (const UnoptimizedQmddPath* generatedQmddPathToTurnUnique = qmddPathToTurnUniqueGenerator.tryGenerateNextPath();
             generatedQmddPathToTurnUnique != nullptr && generatedQmddPathToTurnUnique->size() > 1;
             generatedQmddPathToTurnUnique = qmddPathToTurnUniqueGenerator.tryGenerateNextPath()) {
            UnoptimizedQmddPath modifiableGeneratedQmddPath = *generatedQmddPathToTurnUnique;

            // An overlap between the currently processed and any of the compared to qmdd paths is assumed to exist.
            // If that is not the case then simply fail the operation. This check is valid for qmdd paths with or without gaps
            // since unique qmdd paths should have already been swapped between the edges of the currently processed qmdd node
            // by a previous operation.
            if (std::ranges::none_of(comparedToQmddPaths, [&modifiableGeneratedQmddPath](const OptimizedQmddPath& comparedToQmddPath) {
                    return existsQmddPathWithSameSignature(modifiableGeneratedQmddPath, comparedToQmddPath, true).value_or(true);
                })) {
                return std::nullopt;
            }

            for (std::size_t qmddPathComponentIdx = 1U; qmddPathComponentIdx < modifiableGeneratedQmddPath.size(); ++qmddPathComponentIdx) {
                QmddPathComponent& modifiedPathComponent  = modifiableGeneratedQmddPath.at(qmddPathComponentIdx);
                const QmddNodeEdge originalQmddEdge       = modifiedPathComponent.qmddEdgeToChildNode;
                modifiedPathComponent.qmddEdgeToChildNode = (modifiedPathComponent.qmddEdgeToChildNode == QmddNodeEdge::N || modifiedPathComponent.qmddEdgeToChildNode == QmddNodeEdge::NPrime) ? QmddNodeEdge::P : QmddNodeEdge::N;

                if (std::ranges::none_of(comparedToQmddPaths, [&modifiableGeneratedQmddPath](const OptimizedQmddPath& comparedToQmddPath) {
                        return existsQmddPathWithSameSignature(modifiableGeneratedQmddPath, comparedToQmddPath, true).value_or(true);
                    })) {
                    const qc::Qubit targetQubit = modifiedPathComponent.qubitAssociatedWithQmddNode;
                    qc::Controls    controlQubitsToReachTargetQubitFromStartOfQmddPath;
                    for (std::size_t i = 0U; i < qmddPathComponentIdx; ++i) {
                        controlQubitsToReachTargetQubitFromStartOfQmddPath.emplace(getControlQubitFromSignatureOfQmddPathComponent(generatedQmddPathToTurnUnique->at(i)));
                    }
                    return ToUniqueQmddPathSignatureOperands(controlQubitsToReachTargetQubitFromStartOfQmddPath, targetQubit);
                }
                modifiedPathComponent.qmddEdgeToChildNode = originalQmddEdge;
            }
        }
        return std::nullopt;
    }

    std::optional<ToUniqueQmddPathSignatureOperands> getOperandsToMakeOneOfQmddPathSignaturesUnique(const std::vector<OptimizedQmddPath>& qmddPathsContainingPotentiallyTransformableOne, const std::vector<OptimizedQmddPath>& comparedToQmddPaths) {
        std::optional<ToUniqueQmddPathSignatureOperands> operandsToMakeQmddPathUnique;
        if (!comparedToQmddPaths.empty()) {
            for (std::size_t i = 0; i < qmddPathsContainingPotentiallyTransformableOne.size() && !operandsToMakeQmddPathUnique.has_value(); ++i) {
                operandsToMakeQmddPathUnique = getOperandsToMakeQmddPathSignatureUnique(qmddPathsContainingPotentiallyTransformableOne.at(i), comparedToQmddPaths);
            }
        }
        return operandsToMakeQmddPathUnique;
    }
} // namespace syrec
