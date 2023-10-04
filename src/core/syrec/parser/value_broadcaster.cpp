#include "core/syrec/parser/value_broadcaster.hpp"

using namespace valueBroadcastCheck;

[[nodiscard]] inline bool isUserDefinedDimensionAccessOk(std::size_t numDeclaredDimensions, std::size_t numAccessedDimensions) noexcept {
    return numAccessedDimensions <= numDeclaredDimensions;
}

[[nodiscard]] inline std::optional<std::size_t> determineIndexOfFirstNotExplicitlyAccessedDimensions(std::size_t numDeclaredDimensions, std::size_t numAccessedDimensions) noexcept {
    if (isUserDefinedDimensionAccessOk(numDeclaredDimensions, numAccessedDimensions)) {
        return numAccessedDimensions + (numDeclaredDimensions - numAccessedDimensions);
    }
    return std::nullopt;
}

std::optional<const DimensionAccessMissmatchResult> valueBroadcastCheck::determineMissmatchesInAccessedValuesPerDimension(std::size_t numDeclaredDimensionsOfLhsOperand, std::size_t indexOfFirstNotExplicitlyAccessedDimensionOfLhsOperand, const std::vector<unsigned int>& accessedValuesPerDimensionOfLhsOperand,
                                                                                                                          std::size_t numDeclaredDimensionsOfRhsOperand, std::size_t indexOfFirstNotExplicitlyAccessedDimensionOfRhsOperand, const std::vector<unsigned int>& accessedValuesPerDimensionOfRhsOperand) {
    if (indexOfFirstNotExplicitlyAccessedDimensionOfLhsOperand > numDeclaredDimensionsOfLhsOperand
        || indexOfFirstNotExplicitlyAccessedDimensionOfRhsOperand > numDeclaredDimensionsOfRhsOperand) {
        return std::nullopt;
    }

    constexpr std::size_t minOperandSizeAfterAccessWasApplied = 1;
    const auto            lhsOperandSizeAfterAccessWasApplied = std::max(!indexOfFirstNotExplicitlyAccessedDimensionOfLhsOperand ? numDeclaredDimensionsOfLhsOperand : numDeclaredDimensionsOfLhsOperand - indexOfFirstNotExplicitlyAccessedDimensionOfLhsOperand, minOperandSizeAfterAccessWasApplied);
    const auto            rhsOperandSizeAfterAccessWasApplied = std::max(!indexOfFirstNotExplicitlyAccessedDimensionOfRhsOperand ? numDeclaredDimensionsOfRhsOperand : numDeclaredDimensionsOfRhsOperand - indexOfFirstNotExplicitlyAccessedDimensionOfRhsOperand, minOperandSizeAfterAccessWasApplied);
    if (lhsOperandSizeAfterAccessWasApplied != rhsOperandSizeAfterAccessWasApplied) {
        return std::make_optional(DimensionAccessMissmatchResult{DimensionAccessMissmatchResult::MissmatchReason::SizeOfResult, std::make_pair(lhsOperandSizeAfterAccessWasApplied, rhsOperandSizeAfterAccessWasApplied), {}});
    }

    std::vector<AccessedValuesOfDimensionMissmatch> missmatchContainer;
    for (std::size_t i = 0; i < lhsOperandSizeAfterAccessWasApplied - 1; ++i) {
        const auto& numImplicitlyAccessedValuesOfDimensionOfLhsOperand = accessedValuesPerDimensionOfLhsOperand.at(indexOfFirstNotExplicitlyAccessedDimensionOfLhsOperand++);
        const auto& numImplicitlyAccessedValuesOfDimensionOfRhsOperand = accessedValuesPerDimensionOfRhsOperand.at(indexOfFirstNotExplicitlyAccessedDimensionOfRhsOperand++);
        if (numImplicitlyAccessedValuesOfDimensionOfLhsOperand != numImplicitlyAccessedValuesOfDimensionOfRhsOperand) {
            missmatchContainer.emplace_back(AccessedValuesOfDimensionMissmatch{i, numImplicitlyAccessedValuesOfDimensionOfLhsOperand, numImplicitlyAccessedValuesOfDimensionOfRhsOperand});
        }
    }
    const auto& relativeIdxOfLastImplicitlyAccessedDimension = lhsOperandSizeAfterAccessWasApplied - 1;
    const auto numImplicitlyAccessedValuesOfLastDimensionOfLhsOperand = ((numDeclaredDimensionsOfLhsOperand == 1 && indexOfFirstNotExplicitlyAccessedDimensionOfLhsOperand == 1) || (indexOfFirstNotExplicitlyAccessedDimensionOfLhsOperand == numDeclaredDimensionsOfLhsOperand)) ? 1 : accessedValuesPerDimensionOfLhsOperand.at(indexOfFirstNotExplicitlyAccessedDimensionOfLhsOperand);
    const auto numImplicitlyAccessedValuesOfLastDimensionOfRhsOperand = ((numDeclaredDimensionsOfRhsOperand == 1 && indexOfFirstNotExplicitlyAccessedDimensionOfRhsOperand == 1) || (indexOfFirstNotExplicitlyAccessedDimensionOfRhsOperand == numDeclaredDimensionsOfRhsOperand)) ? 1 : accessedValuesPerDimensionOfRhsOperand.at(indexOfFirstNotExplicitlyAccessedDimensionOfRhsOperand);
    if (numImplicitlyAccessedValuesOfLastDimensionOfLhsOperand != numImplicitlyAccessedValuesOfLastDimensionOfRhsOperand) {
        missmatchContainer.emplace_back(AccessedValuesOfDimensionMissmatch{relativeIdxOfLastImplicitlyAccessedDimension, numImplicitlyAccessedValuesOfLastDimensionOfLhsOperand, numImplicitlyAccessedValuesOfLastDimensionOfRhsOperand});
    }   
     if (!missmatchContainer.empty()) {
        return std::make_optional(DimensionAccessMissmatchResult{DimensionAccessMissmatchResult::MissmatchReason::AccessedValueOfAnyDimension, std::make_pair(0, 0), missmatchContainer});
    }
    return std::nullopt;
}