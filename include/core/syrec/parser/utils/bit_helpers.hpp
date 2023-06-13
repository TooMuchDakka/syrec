#ifndef BIT_HELPERS_HPP
#define BIT_HELPERS_HPP
#pragma once

#include "core/syrec/parser/optimizations/constantPropagation/bit_range_access_restriction.hpp"

namespace BitHelpers {
    [[nodiscard]] bool isValueStorableInBitRange(const unsigned int value, const ::optimizations::BitRangeAccessRestriction::BitRangeAccess& accessedBitRange);
    [[nodiscard]] unsigned int extractBitsFromValue(const unsigned int value, const ::optimizations::BitRangeAccessRestriction::BitRangeAccess& accessedBitRange);
    [[nodiscard]] unsigned int getMaximumStorableValueInBitRange(const unsigned int numAccessedBits);
    [[nodiscard]] unsigned int mergeValues(const unsigned int currentValue, const unsigned int newValue, const ::optimizations::BitRangeAccessRestriction::BitRangeAccess& partsToUpdate);
    [[nodiscard]] std::size_t  getRequiredBitsToStoreValue(unsigned int value);
}
#endif