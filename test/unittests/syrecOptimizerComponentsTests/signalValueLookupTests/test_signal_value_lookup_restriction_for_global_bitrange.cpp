#include "test_signal_value_lookup.hpp"

using namespace valueLookup;

using UserDefinedDimensionAccess = std::vector<std::optional<unsigned int>>;
using OptionalBitRangeAccess     = std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess>;

/*
* Lift restrictions for global bit range 
* 
* 1. Lift restriction from non-overlapping global bit range
* 2. Lift restriction from non-overlapping bit range of value of intermediate dimension
* 3. Lift restriction from non-overlapping bit range of whole intermediate dimension
* 4. Lift restriction from overlapping global bit range
* 5. Lift restriction from overlapping bit range of whole intermediate dimension
* 6. Lift restriction from overlapping bit range of value of intermediate dimension
* 7. Lift restriction from value of intermediate dimension
* 8. Lift restriction from whole intermediate dimension
* 9. Lift restriction from whole last dimension
* 10. Lift restriction from value of last dimension
* 11. Lift restriction from whole signal
*/

// TODO: Test cases for lifting unrelated restrictions
INSTANTIATE_TEST_SUITE_P(
    SignalValueLookupTests,
    SignalValueLookupTest,
    testing::Values(
         std::make_tuple(
            "LiftRestrictionFromSignalWithGlobalBitRangeRestrictionForNonOverlappingGlobalBitRange",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueForBitrange({std::nullopt}, SignalValueLookupTest::defaultBlockedBitRange);
                signalValueLookupWithoutRestriction->liftRestrictionsOfDimensions({std::nullopt}, SignalValueLookupTest::defaultNonOverlappingBitRange);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                return !SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || !accessedBitRange.has_value()
                    || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange);
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                return std::make_optional(SignalValueLookupTest::determineExpectedValueForBitRange(*accessedBitRange));
            }),
        std::make_tuple(
            "LiftRestrictionFromSignalWithGlobalBitRangeRestrictionForNonOverlappingBitRangeOfValueOfIntermediateDimension",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueForBitrange({std::nullopt}, SignalValueLookupTest::defaultBlockedBitRange);
                signalValueLookupWithoutRestriction->liftRestrictionsOfDimensions({std::nullopt, SignalValueLookupTest::lockedValueOfSecondDimension}, SignalValueLookupTest::defaultNonOverlappingBitRange);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                return !SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || !accessedBitRange.has_value()
                    || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange)
                    || (*accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension && SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultNonOverlappingBitRange));
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                return std::make_optional(SignalValueLookupTest::determineExpectedValueForBitRange(*accessedBitRange));
            }),
        std::make_tuple(
            "LiftRestrictionFromSignalWithGlobalBitRangeRestrictionForNonOverlappingBitRangeOfWholeIntermediateDimension",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueForBitrange({std::nullopt}, SignalValueLookupTest::defaultBlockedBitRange);
                signalValueLookupWithoutRestriction->liftRestrictionsOfDimensions({std::nullopt, std::nullopt}, SignalValueLookupTest::defaultNonOverlappingBitRange);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                return !SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || !accessedBitRange.has_value()
                    || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange);
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                return std::make_optional(SignalValueLookupTest::determineExpectedValueForBitRange(*accessedBitRange));
            }),
        std::make_tuple(
            "LiftRestrictionFromSignalWithGlobalBitRangeRestrictionForOverlappingGlobalBitRange",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueForBitrange({std::nullopt}, SignalValueLookupTest::defaultBlockedBitRange);
                signalValueLookupWithoutRestriction->liftRestrictionsOfDimensions({std::nullopt}, SignalValueLookupTest::defaultOverlappingBitRange);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                return !SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || !accessedBitRange.has_value()
                    || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultRemainingBlockedBitRangeAfterOverlappingWasLifted);
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                return std::make_optional(SignalValueLookupTest::determineExpectedValueForBitRange(*accessedBitRange));
            }),
        std::make_tuple(
            "LiftRestrictionFromSignalWithGlobalBitRangeRestrictionForOverlappingBitRangeOfValueOfIntermediateDimension",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueForBitrange({std::nullopt}, SignalValueLookupTest::defaultBlockedBitRange);
                signalValueLookupWithoutRestriction->liftRestrictionsOfDimensions({SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension}, SignalValueLookupTest::defaultOverlappingBitRange);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                return !SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || !accessedBitRange.has_value()
                    || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange)
                    || (*accessedDimensions.at(0) == SignalValueLookupTest::lockedValueOfFirstDimension 
                        && *accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension 
                        && SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultRemainingBlockedBitRangeAfterOverlappingWasLifted));
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                return std::make_optional(SignalValueLookupTest::determineExpectedValueForBitRange(*accessedBitRange));
            }),
        std::make_tuple(
            "LiftRestrictionFromSignalWithGlobalBitRangeRestrictionForOverlappingBitRangeOfWholeIntermediateDimension",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueForBitrange({std::nullopt}, SignalValueLookupTest::defaultBlockedBitRange);
                signalValueLookupWithoutRestriction->liftRestrictionsOfDimensions({SignalValueLookupTest::lockedValueOfFirstDimension, std::nullopt}, SignalValueLookupTest::defaultOverlappingBitRange);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                return !SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || !accessedBitRange.has_value()
                    || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange)
                    || (*accessedDimensions.at(0) == SignalValueLookupTest::lockedValueOfFirstDimension 
                        && SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultRemainingBlockedBitRangeAfterOverlappingWasLifted));
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                return std::make_optional(SignalValueLookupTest::determineExpectedValueForBitRange(*accessedBitRange));
            }),
        std::make_tuple(
            "LiftRestrictionFromSignalWithGlobalBitRangeRestrictionForValueOfIntermediateDimension",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueForBitrange({std::nullopt}, SignalValueLookupTest::defaultBlockedBitRange);
                signalValueLookupWithoutRestriction->liftRestrictionsOfDimensions({SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension}, std::nullopt);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)){
                    return true;
                }

                if (*accessedDimensions.at(0) == SignalValueLookupTest::lockedValueOfFirstDimension && *accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension){
                    return false;
                }

                return !accessedBitRange.has_value() || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange);
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (accessedBitRange.has_value()) {
                    return std::make_optional(SignalValueLookupTest::determineExpectedValueForBitRange(*accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),
        std::make_tuple(
            "LiftRestrictionFromSignalWithGlobalBitRangeRestrictionForWholeIntermediateDimension",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueForBitrange({std::nullopt}, SignalValueLookupTest::defaultBlockedBitRange);
                signalValueLookupWithoutRestriction->liftRestrictionsOfDimensions({SignalValueLookupTest::lockedValueOfFirstDimension, std::nullopt}, std::nullopt);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                return !SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || (*accessedDimensions.at(0) != SignalValueLookupTest::lockedValueOfFirstDimension
                        && (!accessedBitRange.has_value()
                            || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange)));
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (accessedBitRange.has_value()) {
                    return std::make_optional(SignalValueLookupTest::determineExpectedValueForBitRange(*accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),
        std::make_tuple(
            "LiftRestrictionFromSignalWithGlobalBitRangeRestrictionForWholeLastDimension",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueForBitrange({std::nullopt}, SignalValueLookupTest::defaultBlockedBitRange);
                signalValueLookupWithoutRestriction->liftRestrictionsOfDimensions({SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension, std::nullopt}, std::nullopt);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)){
                    return true;
                }

                if (*accessedDimensions.at(0) == SignalValueLookupTest::lockedValueOfFirstDimension && *accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension){
                    return false;
                }

                return !accessedBitRange.has_value() || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange);
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (accessedBitRange.has_value()) {
                    return std::make_optional(SignalValueLookupTest::determineExpectedValueForBitRange(*accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),
        std::make_tuple(
            "LiftRestrictionFromSignalWithGlobalBitRangeRestrictionForValueOfLastDimension",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueForBitrange({std::nullopt}, SignalValueLookupTest::defaultBlockedBitRange);
                signalValueLookupWithoutRestriction->liftRestrictionsOfDimensions({SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension, SignalValueLookupTest::lockedValueOfThirdDimension}, std::nullopt);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)){
                    return true;
                }

                if (*accessedDimensions.at(0) == SignalValueLookupTest::lockedValueOfFirstDimension
                        && *accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension
                        && *accessedDimensions.at(2) == SignalValueLookupTest::lockedValueOfThirdDimension){
                    return false;
                }

                return !accessedBitRange.has_value() || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange);
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (accessedBitRange.has_value()) {
                    return std::make_optional(SignalValueLookupTest::determineExpectedValueForBitRange(*accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),
        std::make_tuple(
            "LiftRestrictionFromSignalWithGlobalBitRangeRestrictionByLiftingAllRestrictions",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueForBitrange({std::nullopt}, SignalValueLookupTest::defaultBlockedBitRange);
                signalValueLookupWithoutRestriction->liftRestrictionsFromWholeSignal();
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess&) {
                 return !SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (accessedBitRange.has_value()) {
                    return std::make_optional(SignalValueLookupTest::determineExpectedValueForBitRange(*accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            })),
         [](const testing::TestParamInfo<SignalValueLookupTest::ParamType>& info) {
            auto testNameToTransform = std::get<0>(info.param);
            std::replace(testNameToTransform.begin(), testNameToTransform.end(), '-', '_');
            return testNameToTransform;
        });