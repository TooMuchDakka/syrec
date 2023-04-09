#ifndef SHARED_DIMENSION_BLOCKING_INFORMATION_HPP
#define SHARED_DIMENSION_BLOCKING_INFORMATION_HPP
#pragma once

#include "core/syrec/parser/optimizations/constantPropagation/bit_range_access_restriction.hpp"

#include <optional>
#include <vector>

namespace optimizations {
    struct SignalDimensionInformation {
        const unsigned int               bitWidth;
        const std::vector<unsigned int>& valuesPerDimension;

        explicit SignalDimensionInformation(unsigned int bitWidth, const std::vector<unsigned int>& valuesPerDimension):
            bitWidth(bitWidth), valuesPerDimension(valuesPerDimension) {}
    };

    struct SharedBlockerInformation {
        bool                                           isDimensionCompletelyBlocked;
        const SignalDimensionInformation&              signalInformation;
        std::optional<BitRangeAccessRestriction::ptr>& sharedDimensionBitRangeAccessRestriction;

        explicit SharedBlockerInformation(const SignalDimensionInformation& sharedSignalInformation, std::optional<BitRangeAccessRestriction::ptr>& sharedDimensionBitRangeAccessRestriction):
            isDimensionCompletelyBlocked(false), signalInformation(sharedSignalInformation), sharedDimensionBitRangeAccessRestriction(sharedDimensionBitRangeAccessRestriction) {}
    };
}; // namespace optimizations
#endif