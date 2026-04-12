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

#include "algorithms/synthesis/qmddTransformation/qmdd_path_definitions.hpp"
#include "ir/Definitions.hpp"
#include "ir/operations/Control.hpp"

#include <cstddef>
#include <optional>
#include <vector>

namespace syrec {
    // TODO: Add documentation
    std::size_t getUnrolledLengthOfOptimizedQmddPath(const OptimizedQmddPath& optimizedQmddPath);
    std::size_t getNumberOfPathsToOneTerminalForQmddPath(const OptimizedQmddPath& qmddPath);
    std::size_t getNumberOfPathsToOneTerminalForQmddPaths(const std::vector<OptimizedQmddPath>& qmddPaths);

    std::optional<bool> doQmddPathSignaturesMatch(const UnoptimizedQmddPath& lQmddPath, const UnoptimizedQmddPath& rQmddPath, bool skipFirstQmddPathEntry);
    std::optional<bool> existsQmddPathWithSameSignature(const UnoptimizedQmddPath& referenceQmddPath, const OptimizedQmddPath& comparedToQmddPath, bool skipFirstQmddPathEntry);

    std::optional<UnoptimizedQmddPath> findFirstQmddPathWithUniqueSignature(const OptimizedQmddPath& potentiallyUniqueQmddPath, const std::vector<OptimizedQmddPath>& comparedToQmddPaths, bool skipFirstQmddPathEntry);
    std::optional<UnoptimizedQmddPath> findFirstQmddPathWithUniqueSignature(const std::vector<OptimizedQmddPath>& potentiallyUniqueQmddPaths, const std::vector<OptimizedQmddPath>& comparedToQmddPaths, bool skipFirstQmddPathEntry);

    struct ToUniqueQmddPathSignatureOperands {
        qc::Controls controlQubitsFromFirstNodeInPathToTargetQubit;
        qc::Qubit    targetQubit;
    };

    bool          operator==(const ToUniqueQmddPathSignatureOperands& lOperands, const ToUniqueQmddPathSignatureOperands& rOperands) noexcept;
    std::ostream& operator<<(std::ostream& ostream, const ToUniqueQmddPathSignatureOperands& operandsOfQmddOperationToTurnQmddPathUnique);

    std::optional<ToUniqueQmddPathSignatureOperands> getOperandsToMakeQmddPathSignatureUnique(const OptimizedQmddPath& qmddPathToTurnUnique, const std::vector<OptimizedQmddPath>& comparedToQmddPaths, bool skipFirstQmddPathEntry);
    std::optional<ToUniqueQmddPathSignatureOperands> getOperandsToMakeOneOfQmddPathSignaturesUnique(const std::vector<OptimizedQmddPath>& qmddPathsContainingPotentiallyTransformableOne, const std::vector<OptimizedQmddPath>& comparedToQmddPaths, bool skipFirstQmddPathEntry);
} // namespace syrec
