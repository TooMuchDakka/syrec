#ifndef TEST_SIGNAL_VALUE_LOOKUP_HPP
#define TEST_SIGNAL_VALUE_LOOKUP_HPP
#pragma once

#include "test_base_signal_value_lookup.hpp"

namespace valueLookup {
    class SignalValueLookupTest:
        public BaseSignalValueLookupTest,
        public testing::Test,
        public testing::WithParamInterface<
                std::tuple<std::string,
                           InitialRestrictionDefinition,
                           ExpectedRestrictionStatusLookup,
                           ExpectedValueLookup>> {
    public:
        const static inline std::vector<unsigned int>                               defaultSignalDimensions             = {2, 4, 2};
        constexpr static unsigned int                                               defaultSignalBitwidth               = 8;
        constexpr static ::optimizations::BitRangeAccessRestriction::BitRangeAccess defaultValueRequiredStorageBitRange = ::optimizations::BitRangeAccessRestriction::BitRangeAccess(0, 7);
        constexpr static unsigned int                                               defaultValue                        = 230; // 1110 0110
        constexpr static unsigned int                                               stripedDefaultValue                 = 170; // 1010 1010

        constexpr static unsigned int lockedValueOfFirstDimension       = 1;
        constexpr static unsigned int lockedOtherValueOfFirstDimension  = 0;
        constexpr static unsigned int lockedValueOfSecondDimension      = 3;
        constexpr static unsigned int lockedOtherValueOfSecondDimension = 1;
        constexpr static unsigned int lockedValueOfThirdDimension       = 1;
        constexpr static unsigned int lockedOtherValueOfThirdDimension  = 0;

        constexpr static unsigned int                                               firstBlockedBitOfDefaultBitRange                         = 2;
        constexpr static unsigned int                                               lastBlockedBitOfDefaultBitRange                          = defaultSignalBitwidth - firstBlockedBitOfDefaultBitRange;
        constexpr static ::optimizations::BitRangeAccessRestriction::BitRangeAccess defaultBlockedBitRange                                   = ::optimizations::BitRangeAccessRestriction::BitRangeAccess(firstBlockedBitOfDefaultBitRange, lastBlockedBitOfDefaultBitRange);
        constexpr static ::optimizations::BitRangeAccessRestriction::BitRangeAccess defaultNonOverlappingBitRange                            = ::optimizations::BitRangeAccessRestriction::BitRangeAccess(0, firstBlockedBitOfDefaultBitRange - 1);
        constexpr static ::optimizations::BitRangeAccessRestriction::BitRangeAccess defaultOverlappingBitRange                               = ::optimizations::BitRangeAccessRestriction::BitRangeAccess(firstBlockedBitOfDefaultBitRange, firstBlockedBitOfDefaultBitRange + 2);
        constexpr static ::optimizations::BitRangeAccessRestriction::BitRangeAccess defaultRemainingBlockedBitRangeAfterOverlappingWasLifted = ::optimizations::BitRangeAccessRestriction::BitRangeAccess(firstBlockedBitOfDefaultBitRange + 3, lastBlockedBitOfDefaultBitRange);

        constexpr static ::optimizations::BitRangeAccessRestriction::BitRangeAccess bitRangeToCopyFrom                     = ::optimizations::BitRangeAccessRestriction::BitRangeAccess(0, 3);
        constexpr static ::optimizations::BitRangeAccessRestriction::BitRangeAccess bitRangeToCopyTo                       = ::optimizations::BitRangeAccessRestriction::BitRangeAccess(4, 7);
        constexpr static unsigned int                                               stripedDefaultValueForBitRangeToCopyTo = 160; // 1010 0000

        constexpr static ::optimizations::BitRangeAccessRestriction::BitRangeAccess defaultRemainingBlockedBitRangeAfterSignalWasUnblocked = ::optimizations::BitRangeAccessRestriction::BitRangeAccess(defaultBlockedBitRange.first - 1, defaultBlockedBitRange.second + 1);

        [[nodiscard]] static SignalValueLookup::ptr createSignalValueLookupInitializedWithDefaultValue() {
            return createSignalValueLookup(defaultSignalBitwidth, defaultSignalDimensions, defaultValue);
        }

        [[nodiscard]] static SignalValueLookup::ptr createDefaultZeroInitializedSignalValueLookup() {
            return createSignalValueLookup(defaultSignalBitwidth, defaultSignalDimensions, 0);
        }

        void SetUp() override {
            signalValueLookup = createSignalValueLookupInitializedWithDefaultValue();
        }
    };
}; // namespace valueLookup
#endif