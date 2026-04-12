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

#include <cstddef>
#include <unordered_set>

namespace syrec {
    class QmddPathGenerator {
    public:
        // TODO: Add documentation
        explicit QmddPathGenerator(const OptimizedQmddPath& qmddPath);
        [[nodiscard]] const UnoptimizedQmddPath* tryGenerateNextPath();
        [[nodiscard]] bool                       canGenerateCombinations() const noexcept;

    private:
        UnoptimizedQmddPath             lastGeneratedCombination;
        std::unordered_set<std::size_t> nonGapQmddPathIndices;

        std::size_t nCombinationsToGenerate = 0;
        std::size_t nGeneratedCombinations  = 0;
    };
} // namespace syrec
