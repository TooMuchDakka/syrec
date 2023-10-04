#ifndef VALUE_BROADCASTER_HPP
#define VALUE_BROADCASTER_HPP
#pragma once

#include <optional>
#include <vector>

namespace valueBroadcastCheck
{
    struct AccessedValuesOfDimensionMissmatch {
        std::size_t dimensionIdx;
        unsigned int expectedNumberOfValues;
        unsigned int actualNumberOfValues;
    };

    struct DimensionAccessMissmatchResult {
        enum MissmatchReason {
            SizeOfResult,
            AccessedValueOfAnyDimension
        };

        MissmatchReason                                 missmatchReason;
        std::pair<std::size_t, std::size_t>             numNotExplicitlyAccessedDimensionPerOperand;
        std::vector<AccessedValuesOfDimensionMissmatch> valuePerNotExplicitlyAccessedDimensionMissmatch;
    };

    [[nodiscard]] std::optional<const DimensionAccessMissmatchResult> determineMissmatchesInAccessedValuesPerDimension(std::size_t numDeclaredDimensionsOfLhsOperand, std::size_t indexOfFirstNotExplicitlyAccessedDimensionOfLhsOperand, const std::vector<unsigned int>& numAccessedValuesPerDimensionOfLhsOperand,
                                                                                                                       std::size_t numDeclaredDimensionsOfRhsOperand, std::size_t indexOfFirstNotExplicitlyAccessedDimensionOfRhsOperand, const std::vector<unsigned int>& numAccessedValuesPerDimensionOfRhsOperand);
}

#endif