#include "test_signal_value_lookup.hpp"

using namespace valueLookup;

using UserDefinedDimensionAccess = std::vector<std::optional<unsigned int>>;
using OptionalBitRangeAccess     = std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess>;

/*
 * a[x][y][z]
*
* 1. a[0][$i]
* 2. a[0][0]
* 3. a[0][$i].s:e
* 4. a[0][1].s:e
* 5. a[0][$i][0]
* * 5. a[0][$i][$j]
* 6. a[0][0][1]
* * 5. a[0][0][$i]
* 7. a[0][$i][0].s:e
+ 8. a[0][0][1].s:e
* 9. a[0]
* 10. a[$i]
* 11. a[0].s:e <-
* 12. a[$i].s:e
* 13. a.s:e
* 14. a
 */

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