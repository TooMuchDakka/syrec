#include "test_signal_value_lookup.hpp"

using namespace valueLookup;

using UserDefinedDimensionAccess = std::vector<std::optional<unsigned int>>;
using OptionalBitRangeAccess     = std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess>;

// TODO: Test cases for lifting unrelated restrictions
INSTANTIATE_TEST_SUITE_P(
    SignalValueLookupTests,
    SignalValueLookupTest,
    testing::Values(
        std::make_tuple(
            "UpdateByAssignmentToCompletelyBlockedSignal",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                signalValueLookup->invalidateAllStoredValuesForSignal();
                const auto& other = SignalValueLookupTest::createZeroedSignalValueLookup();
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                        {0, 3, 1},
                        std::make_optional(::optimizations::BitRangeAccessRestriction::BitRangeAccess(0, 4)),
                        {1, 2, 0},
                        std::make_optional(::optimizations::BitRangeAccessRestriction::BitRangeAccess(4, 8)),
                        *other);
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { return std::nullopt; })),
    [](const testing::TestParamInfo<SignalValueLookupTest::ParamType>& info) {
        auto testNameToTransform = std::get<0>(info.param);
        std::replace(testNameToTransform.begin(), testNameToTransform.end(), '-', '_');
        return testNameToTransform;
    });
