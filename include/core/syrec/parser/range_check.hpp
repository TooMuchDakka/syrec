#ifndef RANGE_CHECK_HPP
#define RANGE_CHECK_HPP

#include "core/syrec/variable.hpp"

namespace parser {
    struct IndexAccessRangeConstraint {
        const std::size_t minimumValidValue;
        const std::size_t maximumValidValue;

        explicit IndexAccessRangeConstraint(const std::size_t minimumValidValue, const std::size_t maximumValidValue):
            minimumValidValue(minimumValidValue), maximumValidValue(maximumValidValue)
        {
        }
    };
    [[nodiscard]] std::optional<IndexAccessRangeConstraint> getConstraintsForValidDimensionAccess(const syrec::Variable::ptr& variable, std::size_t accessedDimensionIdx, bool usingZeroBasedIndex = true);
    [[nodiscard]] IndexAccessRangeConstraint                getConstraintsForValidBitRangeAccess(const syrec::Variable::ptr& variable, bool usingZeroBasedIndex = true);
    [[nodiscard]] IndexAccessRangeConstraint                getConstraintsForValidBitAccess(const syrec::Variable::ptr& variable, bool usingZeroBasedIndex = true);

    [[nodiscard]] bool isValidDimensionAccess(const syrec::Variable::ptr &variable, std::size_t accessedDimensionIdx, std::size_t valueForAccessedDimension, bool usingZeroBasedIndex = true);
    [[nodiscard]] bool isValidBitAccess(const syrec::Variable::ptr &variable, std::size_t accessedBit, bool usingZeroBasedIndex = true);
    [[nodiscard]] bool isValidBitRangeAccess(const syrec::Variable::ptr &variable, const std::pair<std::size_t, std::size_t> &accessedBitRange, bool usingZeroBasedIndex = true);


}

#endif