#include "core/syrec/parser/range_check.hpp"

using namespace parser;
std::optional<IndexAccessRangeConstraint> parser::getConstraintsForValidDimensionAccess(const syrec::Variable::ptr& variable, const std::size_t accessedDimensionIdx, const bool usingZeroBasedIndex) {
    std::optional<IndexAccessRangeConstraint> constraintsForAccessOnDimension;

    const std::size_t numDimensionsForVariable = variable->dimensions.empty() ? 1 : variable->dimensions.size();
    if (accessedDimensionIdx > (usingZeroBasedIndex ? numDimensionsForVariable - 1 : numDimensionsForVariable)) {
        return constraintsForAccessOnDimension;
    }

    std::size_t minimumValidIndexForDimension  = 1;
    std::size_t maxmimumValidIndexForDimension = !variable->dimensions.empty() ? variable->dimensions.at(accessedDimensionIdx) : 1;
    if (usingZeroBasedIndex) {
        minimumValidIndexForDimension--;
        maxmimumValidIndexForDimension--;
    }
    constraintsForAccessOnDimension.emplace(IndexAccessRangeConstraint(minimumValidIndexForDimension, maxmimumValidIndexForDimension));
    return constraintsForAccessOnDimension;
}

IndexAccessRangeConstraint parser::getConstraintsForValidBitRangeAccess(const syrec::Variable::ptr& variable, const bool usingZeroBasedIndex) {
    return parser::getConstraintsForValidBitRangeAccess(variable, usingZeroBasedIndex);
}

IndexAccessRangeConstraint parser::getConstraintsForValidBitAccess(const syrec::Variable::ptr& variable, const bool usingZeroBasedIndex) {
    std::size_t minimumValidBitAccessIndex  = 1;
    std::size_t maxmimumValidBitAccessIndex = variable->bitwidth;

    if (usingZeroBasedIndex) {
        minimumValidBitAccessIndex--;
        maxmimumValidBitAccessIndex--;
    }
    return IndexAccessRangeConstraint(minimumValidBitAccessIndex, maxmimumValidBitAccessIndex);
}

bool parser::isValidDimensionAccess(const syrec::Variable::ptr& variable, const std::size_t accessedDimensionIdx, const std::size_t valueForAccessedDimension, const bool usingZeroBasedIndex) {
    const std::optional<IndexAccessRangeConstraint> possibleConstraintForAccessedDimension = parser::getConstraintsForValidDimensionAccess(variable, accessedDimensionIdx, usingZeroBasedIndex);
    if (possibleConstraintForAccessedDimension.has_value()) {
        return valueForAccessedDimension >= possibleConstraintForAccessedDimension.value().minimumValidValue && valueForAccessedDimension <= possibleConstraintForAccessedDimension.value().maximumValidValue;
    }
    return false;
}

bool parser::isValidBitAccess(const syrec::Variable::ptr& variable, const std::size_t accessedBit, const bool usingZeroBasedIndex) {
    const IndexAccessRangeConstraint constraintsForBitAccess = parser::getConstraintsForValidBitAccess(variable, usingZeroBasedIndex);
    return accessedBit >= constraintsForBitAccess.minimumValidValue && accessedBit <= constraintsForBitAccess.maximumValidValue;
}

bool parser::isValidBitRangeAccess(const syrec::Variable::ptr& variable, const std::pair<std::size_t, std::size_t>& accessedBitRange, const bool usingZeroBasedIndex) {
    return parser::isValidBitAccess(variable, accessedBitRange.first, usingZeroBasedIndex) && parser::isValidBitAccess(variable, accessedBitRange.second, usingZeroBasedIndex);
}
