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
    std::size_t getNumberOfPathsToOneTerminalForQmddPath(const OptimizedQmddPath& qmddPath) {
        return std::transform_reduce(qmddPath.cbegin(), qmddPath.cend(), 1U, std::multiplies{}, [](const std::variant<QmddPathComponent, QmddPathGap>& qmddPathComponent) {
            if (const QmddPathGap* qmddPathGap = std::get_if<QmddPathGap>(&qmddPathComponent); qmddPathGap != nullptr) {
                return static_cast<std::size_t>(std::pow(2, qmddPathGap->nConsecutiveQubitInGap));
            }
            return static_cast<std::size_t>(1U);
        });
    }

    std::size_t getNumberOfPathsToOneTerminalForQmddPaths(const std::vector<OptimizedQmddPath>& qmddPaths) {
        return std::transform_reduce(qmddPaths.cbegin(), qmddPaths.cend(), 0U, std::plus{}, getNumberOfPathsToOneTerminalForQmddPath);
    }

    bool doQmddPathSignaturesMatch(const UnoptimizedQmddPath& lQmddPath, const UnoptimizedQmddPath& rQmddPath, const bool skipFirstQmddPathEntry) {
        bool doQmddPathSignaturesMatch = lQmddPath.size() == rQmddPath.size();
        for (std::size_t i = skipFirstQmddPathEntry ? 1U : 0U; i < lQmddPath.size() && doQmddPathSignaturesMatch; ++i) {
            doQmddPathSignaturesMatch = getControlQubitFromSignatureOfQmddPathComponent(lQmddPath.at(i)) == getControlQubitFromSignatureOfQmddPathComponent(rQmddPath.at(i));
        }
        return doQmddPathSignaturesMatch;
    }

    bool existsQmddPathWithSameSignature(const UnoptimizedQmddPath& referenceQmddPath, const OptimizedQmddPath& comparedToQmddPath, const bool skipFirstQmddPathEntry) {
        bool existsQmddPathWithSameSignature = false;

        QmddPathGenerator comparedToQmddPathsGenerator(comparedToQmddPath);
        for (const UnoptimizedQmddPath* generatedComparedToQmddPath = comparedToQmddPathsGenerator.tryGenerateNextPath();
             generatedComparedToQmddPath != nullptr && generatedComparedToQmddPath->size() > 1 && !existsQmddPathWithSameSignature;
             generatedComparedToQmddPath = comparedToQmddPathsGenerator.tryGenerateNextPath()) {
            existsQmddPathWithSameSignature |= doQmddPathSignaturesMatch(referenceQmddPath, *generatedComparedToQmddPath, skipFirstQmddPathEntry);
        }
        return existsQmddPathWithSameSignature;
    }

    std::optional<UnoptimizedQmddPath> findFirstQmddPathWithUniqueSignature(const OptimizedQmddPath& potentiallyUniqueQmddPath, const std::vector<OptimizedQmddPath>& comparedToQmddPaths, const bool skipFirstQmddPathEntry) {
        QmddPathGenerator potentiallyUniqueQmddPathGenerator(potentiallyUniqueQmddPath);

        for (const UnoptimizedQmddPath* generatedPotentiallyUniqueQmddPath = potentiallyUniqueQmddPathGenerator.tryGenerateNextPath();
             generatedPotentiallyUniqueQmddPath != nullptr && generatedPotentiallyUniqueQmddPath->size() > 1;
             generatedPotentiallyUniqueQmddPath = potentiallyUniqueQmddPathGenerator.tryGenerateNextPath()) {
            if (std::ranges::none_of(comparedToQmddPaths, [generatedPotentiallyUniqueQmddPath, skipFirstQmddPathEntry](const OptimizedQmddPath& comparedToQmddPath) {
                    return existsQmddPathWithSameSignature(*generatedPotentiallyUniqueQmddPath, comparedToQmddPath, skipFirstQmddPathEntry);
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

    std::optional<ToUniqueQmddPathSignatureOperands> getOperandsToMakeQmddPathSignatureUnique(const OptimizedQmddPath& qmddPathToTurnUnique, const std::vector<OptimizedQmddPath>& comparedToQmddPaths, const bool skipFirstQmddPathEntry) {
        QmddPathGenerator qmddPathToTurnUniqueGenerator(qmddPathToTurnUnique);
        for (const UnoptimizedQmddPath* generatedQmddPathToTurnUnique = qmddPathToTurnUniqueGenerator.tryGenerateNextPath();
             generatedQmddPathToTurnUnique != nullptr && generatedQmddPathToTurnUnique->size() > 1;
             generatedQmddPathToTurnUnique = qmddPathToTurnUniqueGenerator.tryGenerateNextPath()) {
            UnoptimizedQmddPath modifiableGeneratedQmddPath = *generatedQmddPathToTurnUnique;
            for (std::size_t qmddPathComponentIdx = 1U; qmddPathComponentIdx < modifiableGeneratedQmddPath.size(); ++qmddPathComponentIdx) {
                QmddPathComponent& modifiedPathComponent  = modifiableGeneratedQmddPath.at(qmddPathComponentIdx);
                const QmddNodeEdge originalQmddEdge       = modifiedPathComponent.qmddEdgeToChildNode;
                modifiedPathComponent.qmddEdgeToChildNode = (modifiedPathComponent.qmddEdgeToChildNode == QmddNodeEdge::N || modifiedPathComponent.qmddEdgeToChildNode == QmddNodeEdge::NPrime) ? QmddNodeEdge::P : QmddNodeEdge::N;

                if (std::ranges::none_of(comparedToQmddPaths, [&modifiableGeneratedQmddPath, skipFirstQmddPathEntry](const OptimizedQmddPath& comparedToQmddPath) {
                        return existsQmddPathWithSameSignature(modifiableGeneratedQmddPath, comparedToQmddPath, skipFirstQmddPathEntry);
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

    std::optional<ToUniqueQmddPathSignatureOperands> getOperandsToMakeOneOfQmddPathSignaturesUnique(const std::vector<OptimizedQmddPath>& qmddPathsContainingPotentiallyTransformableOne, const std::vector<OptimizedQmddPath>& comparedToQmddPaths, const bool skipFirstQmddPathEntry) {
        std::optional<ToUniqueQmddPathSignatureOperands> operandsToMakeQmddPathUnique;
        for (std::size_t i = 0; i < qmddPathsContainingPotentiallyTransformableOne.size() && !operandsToMakeQmddPathUnique.has_value(); ++i) {
            operandsToMakeQmddPathUnique = getOperandsToMakeQmddPathSignatureUnique(qmddPathsContainingPotentiallyTransformableOne.at(i), comparedToQmddPaths, skipFirstQmddPathEntry);
        }
        return operandsToMakeQmddPathUnique;
    }
} // namespace syrec
