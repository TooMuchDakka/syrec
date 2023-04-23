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
                "UpdateValueOfBit",
                [](const SignalValueLookup::ptr& signalValueLookup) {
                    SignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                    signalValueLookup->updateStoredValueFor(
                            {SignalValueLookupTest::lockedValueOfFirstDimension,
                             SignalValueLookupTest::lockedValueOfSecondDimension,
                             SignalValueLookupTest::lockedValueOfThirdDimension},
                            std::make_optional(::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::defaultBlockedBitRange.first, SignalValueLookupTest::defaultBlockedBitRange.first)),
                            1);
                },
                [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
                [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                    if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions))
                        return std::nullopt;

                    const bool accessedUpdatedDimension =
                        accessedDimensions.at(0) == SignalValueLookupTest::lockedValueOfFirstDimension
                    && accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension
                    && accessedDimensions.at(2) == SignalValueLookupTest::lockedValueOfThirdDimension;

                    if (!accessedUpdatedDimension)
                        return 0;

                    const unsigned int currentDefaultValue = 1 << SignalValueLookupTest::defaultBlockedBitRange.first;
                    if (!accessedBitRange.has_value()) {
                        return std::make_optional(currentDefaultValue);
                    }

                    return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(currentDefaultValue, *accessedBitRange));
                }),
            std::make_tuple(
                "UpdateValueOfBitRange",
                [](const SignalValueLookup::ptr& signalValueLookup) {
                    SignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                    signalValueLookup->updateStoredValueFor(
                        {SignalValueLookupTest::lockedValueOfFirstDimension,
                         SignalValueLookupTest::lockedValueOfSecondDimension,
                         SignalValueLookupTest::lockedValueOfThirdDimension},
                        std::make_optional(::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::defaultBlockedBitRange.first, SignalValueLookupTest::defaultBlockedBitRange.second)),
                        // .2:6 => 1010 1100
                        172);
                },
                [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
                [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                    if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions))
                        return std::nullopt;

                    const bool accessedUpdatedDimension =
                        accessedDimensions.at(0) == SignalValueLookupTest::lockedValueOfFirstDimension
                    && accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension
                    && accessedDimensions.at(2) == SignalValueLookupTest::lockedValueOfThirdDimension;

                    if (!accessedUpdatedDimension)
                        return 0;

                    const unsigned int currentDefaultValue = 172;
                    if (!accessedBitRange.has_value()) {
                        return std::make_optional(currentDefaultValue);
                    }

                    return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(currentDefaultValue, *accessedBitRange));
                }
            ),
            std::make_tuple(
                "UpdateCompleteValue",
                [](const SignalValueLookup::ptr& signalValueLookup) {
                    SignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                    signalValueLookup->updateStoredValueFor(
                            {SignalValueLookupTest::lockedValueOfFirstDimension,
                             SignalValueLookupTest::lockedValueOfSecondDimension,
                             SignalValueLookupTest::lockedValueOfThirdDimension},
                            std::nullopt,
                            SignalValueLookupTest::defaultValue
                            );
                },
                [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
                [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                    if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions))
                        return std::nullopt;

                    const bool accessedUpdatedDimension =
                        accessedDimensions.at(0) == SignalValueLookupTest::lockedValueOfFirstDimension
                    && accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension
                    && accessedDimensions.at(2) == SignalValueLookupTest::lockedValueOfThirdDimension;

                    if (!accessedUpdatedDimension)
                        return 0;

                    const unsigned int currentDefaultValue = SignalValueLookupTest::defaultValue;
                    if (!accessedBitRange.has_value()) {
                        return std::make_optional(currentDefaultValue);
                    }

                    return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(currentDefaultValue, *accessedBitRange));
                }),
            std::make_tuple(
                "UpdateBitOfAlreadySetValue",
                [](const SignalValueLookup::ptr& signalValueLookup) {
                    SignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                    signalValueLookup->updateStoredValueFor(
                        {SignalValueLookupTest::lockedValueOfFirstDimension,
                         SignalValueLookupTest::lockedValueOfSecondDimension,
                         SignalValueLookupTest::lockedValueOfThirdDimension},
                        std::make_optional(::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::defaultBlockedBitRange.first, SignalValueLookupTest::defaultBlockedBitRange.second)),
                        // .2:6 => 1010 1100
                        172);
                    signalValueLookup->updateStoredValueFor(
                        {SignalValueLookupTest::lockedValueOfFirstDimension,
                         SignalValueLookupTest::lockedValueOfSecondDimension,
                         SignalValueLookupTest::lockedValueOfThirdDimension},
                        std::make_optional(::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::defaultBlockedBitRange.first, SignalValueLookupTest::defaultBlockedBitRange.first)),
                        0);
                },
                [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
                [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                    if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions))
                        return std::nullopt;

                    const bool accessedUpdatedDimension = 
                        accessedDimensions.at(0) == SignalValueLookupTest::lockedValueOfFirstDimension
                        && accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension
                        && accessedDimensions.at(2) == SignalValueLookupTest::lockedValueOfThirdDimension;
                    if (!accessedUpdatedDimension)
                        return 0;

                    const unsigned int currentDefaultValue = 168;
                    if (!accessedBitRange.has_value()) {
                        return std::make_optional(currentDefaultValue);
                    }

                    return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(currentDefaultValue, *accessedBitRange));
                }),
            std::make_tuple(
                "UpdateBitRangeOfAlreadySetValue",
                [](const SignalValueLookup::ptr& signalValueLookup) {
                    SignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                    signalValueLookup->updateStoredValueFor(
                            {SignalValueLookupTest::lockedValueOfFirstDimension,
                             SignalValueLookupTest::lockedValueOfSecondDimension,
                             SignalValueLookupTest::lockedValueOfThirdDimension},
                            std::make_optional(::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::defaultBlockedBitRange.first, SignalValueLookupTest::defaultBlockedBitRange.second)),
                            // .2:6 => 1010 1100
                            0);
                },
                [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
                [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                    if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions))
                        return std::nullopt;

                    const bool accessedUpdatedDimension =
                        accessedDimensions.at(0) == SignalValueLookupTest::lockedValueOfFirstDimension
                    && accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension
                    && accessedDimensions.at(2) == SignalValueLookupTest::lockedValueOfThirdDimension;

                    if (!accessedUpdatedDimension)
                        return 0;

                    const unsigned int currentDefaultValue = 0;
                    if (!accessedBitRange.has_value()) {
                        return std::make_optional(currentDefaultValue);
                    }

                    return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(currentDefaultValue, *accessedBitRange));
                })
        ),
        [](const testing::TestParamInfo<SignalValueLookupTest::ParamType>& info) {
            auto testNameToTransform = std::get<0>(info.param);
            std::replace(testNameToTransform.begin(), testNameToTransform.end(), '-', '_');
            return testNameToTransform;
        });
