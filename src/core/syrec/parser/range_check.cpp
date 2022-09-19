#include "core/syrec/parser/range_check.hpp"

using namespace range_check;
static std::optional<IndexAccessRangeConstraint> range_check::getConstraintsForValidDimensionAccess(const syrec::Variable::ptr& variable, const std::size_t accessedDimensionIdx, const bool usingZeroBasedIndex) {
    std::optional<IndexAccessRangeConstraint> constraintsForAccessOnDimension;

    const std::size_t numDimensionsForVariable = variable->dimensions.empty() ? 1 : variable->dimensions.size();
    if (accessedDimensionIdx > (usingZeroBasedIndex ? numDimensionsForVariable - 1 : numDimensionsForVariable)) {
        return constraintsForAccessOnDimension;
    }

    std::size_t minimumValidIndexForDimension = 1;
    std::size_t maxmimumValidIndexForDimension = !variable->dimensions.empty() ? variable->dimensions.at(accessedDimensionIdx) : 1;
    if (usingZeroBasedIndex) {
        minimumValidIndexForDimension--;
        maxmimumValidIndexForDimension--;
    }
    constraintsForAccessOnDimension.emplace(IndexAccessRangeConstraint(minimumValidIndexForDimension, maxmimumValidIndexForDimension));
    return constraintsForAccessOnDimension;
}

static IndexAccessRangeConstraint range_check::getConstraintsForValidBitRangeAccess(const syrec::Variable::ptr& variable, const bool usingZeroBasedIndex) {
    return getConstraintsForValidBitRangeAccess(variable, usingZeroBasedIndex);
}

static IndexAccessRangeConstraint range_check::getConstraintsForValidBitAccess(const syrec::Variable::ptr& variable, const bool usingZeroBasedIndex) {
    std::size_t minimumValidBitAccessIndex = 1;
    std::size_t maxmimumValidBitAccessIndex = variable->bitwidth;

    if (usingZeroBasedIndex) {
        minimumValidBitAccessIndex--;
        maxmimumValidBitAccessIndex--;
    }
    return IndexAccessRangeConstraint(minimumValidBitAccessIndex, maxmimumValidBitAccessIndex);
}

static bool range_check::isValidDimensionAccess(const syrec::Variable::ptr& variable, const std::size_t accessedDimensionIdx, const std::size_t valueForAccessedDimension, const bool usingZeroBasedIndex) {
    const std::optional<IndexAccessRangeConstraint> possibleConstraintForAccessedDimension = getConstraintsForValidDimensionAccess(variable, accessedDimensionIdx, usingZeroBasedIndex);
    if (possibleConstraintForAccessedDimension.has_value()) {
        return valueForAccessedDimension >= possibleConstraintForAccessedDimension.value().minimumValidValue && valueForAccessedDimension <= possibleConstraintForAccessedDimension.value().maximumValidValue;
    }
    return false;
}

static bool range_check::isValidBitAccess(const syrec::Variable::ptr& variable, const std::size_t accessedBit, const bool usingZeroBasedIndex) {
    const IndexAccessRangeConstraint constraintsForBitAccess = getConstraintsForValidBitAccess(variable, usingZeroBasedIndex);
    return accessedBit >= constraintsForBitAccess.minimumValidValue && accessedBit <= constraintsForBitAccess.maximumValidValue;
}

static bool range_check::isValidBitRangeAccess(const syrec::Variable::ptr& variable, const std::pair<std::size_t, std::size_t>& accessedBitRange, const bool usingZeroBasedIndex) {
    return isValidBitAccess(variable, accessedBitRange.first, usingZeroBasedIndex) && isValidBitAccess(variable, accessedBitRange.second, usingZeroBasedIndex);
}