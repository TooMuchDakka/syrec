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
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess& accessedBitRange) {
                if (!accessedBitRange.has_value()) {
                    return true;
                }
                return SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions) || !accessedBitRange.has_value()) {
                    return std::nullopt;
                }

                return std::make_optional(SignalValueLookupTest::determineExpectedValueForBitRange(*accessedBitRange));
            }),
        std::make_tuple(
            "LiftRestrictionFromSignalWithGlobalBitRangeRestrictionForNonOverlappingBitRangeOfValueOfIntermediateDimension",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueForBitrange({std::nullopt}, SignalValueLookupTest::defaultBlockedBitRange);
                signalValueLookupWithoutRestriction->liftRestrictionsOfDimensions({std::nullopt, SignalValueLookupTest::lockedValueOfSecondDimension}, SignalValueLookupTest::defaultNonOverlappingBitRange);
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess& accessedBitRange) {
                if (!accessedBitRange.has_value() || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange)) {
                    return true;
                }

                return false;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions) || !accessedBitRange.has_value()) {
                    return std::nullopt;
                }

                if (!SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange)) {
                    return std::make_optional(SignalValueLookupTest::determineExpectedValueForBitRange(*accessedBitRange));
                }
                return std::nullopt;
            }),
        std::make_tuple(
            "LiftRestrictionFromSignalWithGlobalBitRangeRestrictionForNonOverlappingBitRangeOfWholeIntermediateDimension",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueForBitrange({std::nullopt}, SignalValueLookupTest::defaultBlockedBitRange);
                signalValueLookupWithoutRestriction->liftRestrictionsOfDimensions({std::nullopt, std::nullopt}, SignalValueLookupTest::defaultNonOverlappingBitRange);
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess& accessedBitRange) {
                if (!accessedBitRange.has_value()) {
                    return true;
                }

                if (SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange)) {
                    return true;
                }

                return false;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions) || !accessedBitRange.has_value()) {
                    return std::nullopt;
                }

                if (SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange)) {
                    return std::nullopt;
                }
                return std::make_optional(SignalValueLookupTest::determineExpectedValueForBitRange(*accessedBitRange));
            }),
        std::make_tuple(
            "LiftRestrictionFromSignalWithGlobalBitRangeRestrictionForOverlappingGlobalBitRange",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueForBitrange({std::nullopt}, SignalValueLookupTest::defaultBlockedBitRange);
                signalValueLookupWithoutRestriction->liftRestrictionsOfDimensions({std::nullopt}, SignalValueLookupTest::defaultOverlappingBitRange);
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess& accessedBitRange) {
                if (!accessedBitRange.has_value() || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultRemainingBlockedBitRangeAfterOverlappingWasLifted)) {
                    return true;
                }

                return false;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions) || !accessedBitRange.has_value()) {
                    return std::nullopt;
                }

                if (SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultRemainingBlockedBitRangeAfterOverlappingWasLifted)) {
                    return std::nullopt;
                }
                return std::make_optional(SignalValueLookupTest::determineExpectedValueForBitRange(*accessedBitRange));
            }),
        std::make_tuple(
            "LiftRestrictionFromSignalWithGlobalBitRangeRestrictionForOverlappingBitRangeOfValueOfIntermediateDimension",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueForBitrange({std::nullopt}, SignalValueLookupTest::defaultBlockedBitRange);
                signalValueLookupWithoutRestriction->liftRestrictionsOfDimensions({SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension}, SignalValueLookupTest::defaultOverlappingBitRange);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                if (!accessedBitRange.has_value()) {
                    return true;
                }

                if (accessedDimensions.size() >= 2 && accessedDimensions.at(0).has_value() && (*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension && accessedDimensions.at(1).has_value() && (*accessedDimensions.at(1)) == SignalValueLookupTest::lockedValueOfSecondDimension) {
                    return SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultRemainingBlockedBitRangeAfterOverlappingWasLifted);
                }
                return SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions) || !accessedBitRange.has_value() || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultRemainingBlockedBitRangeAfterOverlappingWasLifted)) {
                    return std::nullopt;
                }

                if ((*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension && (*accessedDimensions.at(1)) == SignalValueLookupTest::lockedValueOfSecondDimension) {
                    return std::make_optional(SignalValueLookupTest::determineExpectedValueForBitRange(*accessedBitRange));
                }

                if (SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange)) {
                    return std::nullopt;
                }
                return std::make_optional(SignalValueLookupTest::determineExpectedValueForBitRange(*accessedBitRange));
            }),
        std::make_tuple(
            "LiftRestrictionFromSignalWithGlobalBitRangeRestrictionForOverlappingBitRangeOfWholeIntermediateDimension",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueForBitrange({std::nullopt}, SignalValueLookupTest::defaultBlockedBitRange);
                signalValueLookupWithoutRestriction->liftRestrictionsOfDimensions({SignalValueLookupTest::lockedValueOfFirstDimension, std::nullopt}, SignalValueLookupTest::defaultOverlappingBitRange);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                if (!accessedBitRange.has_value()) {
                    return true;
                }

                if (accessedDimensions.size() > 1 && accessedDimensions.at(1).has_value() && (*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension) {
                    return SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultRemainingBlockedBitRangeAfterOverlappingWasLifted);
                }
                return SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions) || !accessedBitRange.has_value() || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultRemainingBlockedBitRangeAfterOverlappingWasLifted)) {
                    return std::nullopt;
                }

                if ((*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension) {
                    return std::make_optional(SignalValueLookupTest::determineExpectedValueForBitRange(*accessedBitRange));
                }

                if (SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange)) {
                    return std::nullopt;
                }
                return std::make_optional(SignalValueLookupTest::determineExpectedValueForBitRange(*accessedBitRange));
            }),
        std::make_tuple(
            "LiftRestrictionFromSignalWithGlobalBitRangeRestrictionForValueOfIntermediateDimension",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueForBitrange({std::nullopt}, SignalValueLookupTest::defaultBlockedBitRange);
                signalValueLookupWithoutRestriction->liftRestrictionsOfDimensions({SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension}, std::nullopt);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                if (accessedDimensions.size() >= 2 && accessedDimensions.at(0).has_value() && (*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension && accessedDimensions.at(1).has_value() && (*accessedDimensions.at(1)) == SignalValueLookupTest::lockedValueOfSecondDimension) {
                    return false;
                }

                if (!accessedBitRange.has_value()) {
                    return true;
                }
                return SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)) {
                    return std::nullopt;
                }

                if ((*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension && (*accessedDimensions.at(1)) == SignalValueLookupTest::lockedValueOfSecondDimension) {
                    if (accessedBitRange.has_value()) {
                        return std::make_optional(SignalValueLookupTest::determineExpectedValueForBitRange(*accessedBitRange));
                    }
                    return std::make_optional(SignalValueLookupTest::defaultValue);
                }

                if (accessedBitRange.has_value()) {
                    if (SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange)) {
                        return std::nullopt;
                    }
                    return std::make_optional(SignalValueLookupTest::determineExpectedValueForBitRange(*accessedBitRange));
                }
                return std::nullopt;
            }),
        std::make_tuple(
            "LiftRestrictionFromSignalWithGlobalBitRangeRestrictionForWholeIntermediateDimension",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueForBitrange({std::nullopt}, SignalValueLookupTest::defaultBlockedBitRange);
                signalValueLookupWithoutRestriction->liftRestrictionsOfDimensions({SignalValueLookupTest::lockedValueOfFirstDimension, std::nullopt}, std::nullopt);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                if (accessedDimensions.size() >= 1 && accessedDimensions.at(0).has_value() && (*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension) {
                    return false;
                }

                if (!accessedBitRange.has_value()) {
                    return true;
                }
                return SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)) {
                    return std::nullopt;
                }

                if ((*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension) {
                    if (accessedBitRange.has_value()) {
                        return std::make_optional(SignalValueLookupTest::determineExpectedValueForBitRange(*accessedBitRange));
                    }
                    return std::make_optional(SignalValueLookupTest::defaultValue);
                }

                if (accessedBitRange.has_value()) {
                    if (SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange)) {
                        return std::nullopt;
                    }
                    return std::make_optional(SignalValueLookupTest::determineExpectedValueForBitRange(*accessedBitRange));
                }
                return std::nullopt;
            }),
        std::make_tuple(
            "LiftRestrictionFromSignalWithGlobalBitRangeRestrictionForWholeLastDimension",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueForBitrange({std::nullopt}, SignalValueLookupTest::defaultBlockedBitRange);
                signalValueLookupWithoutRestriction->liftRestrictionsOfDimensions({SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension, std::nullopt}, std::nullopt);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                if (accessedDimensions.size() >= 2 && (*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension && (*accessedDimensions.at(1)) == SignalValueLookupTest::lockedValueOfSecondDimension) {
                    return false;
                }

                if (accessedBitRange.has_value()) {
                    return SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange);
                }
                return true;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)) {
                    return std::nullopt;
                }

                if ((*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension && (*accessedDimensions.at(1)) == SignalValueLookupTest::lockedValueOfSecondDimension) {
                    if (accessedBitRange.has_value()) {
                        return std::make_optional(SignalValueLookupTest::determineExpectedValueForBitRange(*accessedBitRange));
                    }
                    return std::make_optional(SignalValueLookupTest::defaultValue);
                }

                if (accessedBitRange.has_value()) {
                    if (SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange)) {
                        return std::nullopt;
                    }
                    return std::make_optional(SignalValueLookupTest::determineExpectedValueForBitRange(*accessedBitRange));
                }
                return std::nullopt;
            }),
        std::make_tuple(
            "LiftRestrictionFromSignalWithGlobalBitRangeRestrictionForValueOfLastDimension",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueForBitrange({std::nullopt}, SignalValueLookupTest::defaultBlockedBitRange);
                signalValueLookupWithoutRestriction->liftRestrictionsOfDimensions({SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension, SignalValueLookupTest::lockedValueOfThirdDimension}, std::nullopt);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)) {
                    return true;
                }

                if ((*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension && (*accessedDimensions.at(1)) == SignalValueLookupTest::lockedValueOfSecondDimension && (*accessedDimensions.at(2)) == SignalValueLookupTest::lockedValueOfThirdDimension) {
                    return false;
                }

                if (accessedBitRange.has_value()) {
                    return SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange);
                }
                return true;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)) {
                    return std::nullopt;
                }

                if ((*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension && (*accessedDimensions.at(1)) == SignalValueLookupTest::lockedValueOfSecondDimension && (*accessedDimensions.at(2)) == SignalValueLookupTest::lockedValueOfThirdDimension) {
                    if (accessedBitRange.has_value()) {
                        return std::make_optional(SignalValueLookupTest::determineExpectedValueForBitRange(*accessedBitRange));
                    }
                    return std::make_optional(SignalValueLookupTest::defaultValue);
                }

                if (accessedBitRange.has_value()) {
                    if (SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange)) {
                        return std::nullopt;
                    }
                    return std::make_optional(SignalValueLookupTest::determineExpectedValueForBitRange(*accessedBitRange));
                }
                return std::nullopt;
            }),
        std::make_tuple(
            "LiftRestrictionFromSignalWithGlobalBitRangeRestrictionByLiftingAllRestrictions",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueForBitrange({std::nullopt}, SignalValueLookupTest::defaultBlockedBitRange);
                signalValueLookupWithoutRestriction->liftRestrictionsFromWholeSignal();
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) {
                 return false;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)) {
                    return std::nullopt;
                }

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