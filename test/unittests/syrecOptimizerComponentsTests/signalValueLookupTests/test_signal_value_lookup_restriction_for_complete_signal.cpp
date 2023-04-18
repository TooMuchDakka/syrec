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
                        "NoRestrictions",
                        [](const SignalValueLookup::ptr&) {
                            return;
                        },
                        [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess&) {
                            return false;
                        },
                        [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                            if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions))
                                return std::nullopt;

                            if (!accessedBitRange.has_value()) {
                                return std::make_optional(SignalValueLookupTest::defaultValue);
                            }

                            return std::make_optional(SignalValueLookupTest::determineExpectedValueForBitRange(*accessedBitRange));
                        })),
        [](const testing::TestParamInfo<SignalValueLookupTest::ParamType>& info) {
            auto testNameToTransform = std::get<0>(info.param);
            std::replace(testNameToTransform.begin(), testNameToTransform.end(), '-', '_');
            return testNameToTransform;
        });