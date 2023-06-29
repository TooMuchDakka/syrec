#ifndef DEAD_STORE_STATUS_LOOKUP_HPP
#define DEAD_STORE_STATUS_LOOKUP_HPP
#pragma once
#include "core/syrec/parser/optimizations/constantPropagation/valueLookup/base_value_lookup.hpp"

namespace deadStoreElimination {
    class DeadStoreStatusLookup : public valueLookup::BaseValueLookup<bool> {
    public:
        explicit DeadStoreStatusLookup(const unsigned signalBitwidth, const std::vector<unsigned int>& signalDimensions, const std::optional<bool>& defaultValue):
            BaseValueLookup<bool>(signalBitwidth, signalDimensions, defaultValue) {}

        [[nodiscard]] std::shared_ptr<BaseValueLookup<bool>> clone() override;
        [[nodiscard]] bool                                   areAccessedSignalPartsDead(const std::vector<std::optional<unsigned int>>& accessedDimensions, const std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess>& accessedBitRange) const;

    protected:
        [[nodiscard]] bool     canStoreValue(const std::any& value, const optimizations::BitRangeAccessRestriction::BitRangeAccess& availableStorageSpace) const override;
        [[nodiscard]] std::any extractPartsOfValue(const std::any& value, const optimizations::BitRangeAccessRestriction::BitRangeAccess& availableStorageSpace) const override;
        [[nodiscard]] std::any transformExistingValueByMergingWithNewOne(const std::any& currentValue, const std::any& newValue, const optimizations::BitRangeAccessRestriction::BitRangeAccess& partsToUpdate) const override;
        [[nodiscard]] std::any wrapValueOnOverflow(const std::any& value, unsigned numBitsOfStorage) const override;
    };
}
#endif