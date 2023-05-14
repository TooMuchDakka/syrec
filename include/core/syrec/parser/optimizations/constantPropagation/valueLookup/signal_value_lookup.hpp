#ifndef SIGNAL_VALUE_LOOKUP_HPP
#define SIGNAL_VALUE_LOOKUP_HPP
#pragma once

#include "core/syrec/parser/optimizations/constantPropagation/valueLookup/base_value_lookup.hpp"

namespace valueLookup {
    class SignalValueLookup : public BaseValueLookup<unsigned int> {
    public:
        explicit SignalValueLookup(const unsigned signalBitwidth, const std::vector<unsigned int>& signalDimensions, const unsigned int& defaultValue)
            : BaseValueLookup<unsigned>(signalBitwidth, signalDimensions, std::make_optional(defaultValue)) {}

        ~SignalValueLookup() override = default;

        [[nodiscard]] SignalValueLookup::ptr clone() override;
    protected:
        [[nodiscard]] bool     canStoreValue(const std::any& value, const optimizations::BitRangeAccessRestriction::BitRangeAccess& availableStorageSpace) const override;
        [[nodiscard]] std::any extractPartsOfValue(const std::any& value, const optimizations::BitRangeAccessRestriction::BitRangeAccess& availableStorageSpace) const override;
        [[nodiscard]] std::any transformExistingValueByMergingWithNewOne(const std::any& currentValue, const std::any& newValue, const optimizations::BitRangeAccessRestriction::BitRangeAccess& partsToUpdate) const override;
        [[nodiscard]] std::any wrapValueOnOverflow(const std::any& value, unsigned int numBitsOfStorage) const override; 
    };
}; // namespace valueLookup

#endif