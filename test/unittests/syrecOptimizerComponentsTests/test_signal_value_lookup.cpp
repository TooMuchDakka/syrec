#include "test_signal_value_lookup.hpp"

using namespace valueLookup;

using UserDefinedDimensionAccess = std::vector<std::optional<unsigned int>>;
using OptionalBitRangeAccess = std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess>;

// TODO: Test cases for lifting unrelated restrictions
INSTANTIATE_TEST_SUITE_P(
    MultiDimensionSignalValueLookupTests,
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
            }),
        std::make_tuple(
            "DISABLED_GlobalBitRangeRestrictionNotSpanningWholeSignalWidth",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestrictions) {
                signalValueLookupWithoutRestrictions->invalidateStoredValueForBitrange({}, SignalValueLookupTest::defaultBlockedBitRange);                
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess& accessedBitRange) {
                return accessedBitRange.has_value() ? SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange) : true;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || (accessedBitRange.has_value() && SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange))
                    || !accessedBitRange.has_value())
                    return std::nullopt;

                return std::make_optional(SignalValueLookupTest::determineExpectedValueForBitRange(*accessedBitRange));
            }),
        std::make_tuple(
            "BlockSignalCompletely",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateAllStoredValuesForSignal();
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) {
                return true;
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) -> std::optional<unsigned int> {
                return std::nullopt;
            }),
        std::make_tuple(
            "BlockWholeIntermediateDimensionButOmittingDimensionsAfter",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, std::nullopt});
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess&) {
                return accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions) 
                    || !accessedDimensions.front().has_value() 
                    || (*accessedDimensions.front()) == SignalValueLookupTest::lockedValueOfFirstDimension) {
                    return std::nullopt;
                }

                if (!accessedBitRange.has_value()) {
                    return std::make_optional(SignalValueLookupTest::defaultValue);
                }
                return std::make_optional(SignalValueLookupTest::determineExpectedValueForBitRange(*accessedBitRange));
            }),
        std::make_tuple(
            "BlockWholeIntermediateDimensionButOmittingDimensionsAfterAndBitrangeAccess",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueForBitrange({SignalValueLookupTest::lockedValueOfFirstDimension, std::nullopt}, SignalValueLookupTest::defaultBlockedBitRange);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                return (!accessedDimensions.front().has_value() 
                    || (*accessedDimensions.front()) == SignalValueLookupTest::lockedValueOfFirstDimension)
                        && (!accessedBitRange.has_value() || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange));
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions) 
                    || (!accessedDimensions.front().has_value() 
                        || (*accessedDimensions.front()) == SignalValueLookupTest::lockedValueOfFirstDimension) && (!accessedBitRange.has_value() || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange))) {
                    return std::nullopt;
                }

                if (!accessedBitRange.has_value()) {
                    return std::make_optional(SignalValueLookupTest::defaultValue);
                }
                return std::make_optional(SignalValueLookupTest::determineExpectedValueForBitRange(*accessedBitRange));
            }),
        std::make_tuple(
            "BlockValueOfIntermediateDimensionButOmittingDimensionsAfter",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension});
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess&) {
                const bool accessesBlockedValueForFirstDimension = !accessedDimensions.front().has_value() || (accessedDimensions.front().has_value() && (*accessedDimensions.front()) == SignalValueLookupTest::lockedValueOfFirstDimension);
                const bool accessesBlockedValueForSecondDimension = accessesBlockedValueForFirstDimension && accessedDimensions.size() > 1 && (!accessedDimensions.at(1).has_value() || (*accessedDimensions.at(1)) == SignalValueLookupTest::lockedValueOfSecondDimension);
                return accessesBlockedValueForSecondDimension;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)) {
                    return std::nullopt;
                }

                const bool accessesBlockedValueForFirstDimension  = !accessedDimensions.front().has_value() || (accessedDimensions.front().has_value() && (*accessedDimensions.front()) == SignalValueLookupTest::lockedValueOfFirstDimension);
                const bool accessesBlockedValueForSecondDimension = accessesBlockedValueForFirstDimension && accessedDimensions.size() > 1 && (!accessedDimensions.at(1).has_value() || (*accessedDimensions.at(1)) == SignalValueLookupTest::lockedValueOfSecondDimension);

                if (accessesBlockedValueForSecondDimension) {
                    return std::nullopt;
                }

                if (!accessedBitRange.has_value()) {
                    return std::make_optional(SignalValueLookupTest::defaultValue);
                }
                return std::make_optional(SignalValueLookupTest::determineExpectedValueForBitRange(*accessedBitRange));
            }),
        std::make_tuple(
            "BlockValueOfIntermediateDimensionButOmittingDimensionsAfterAndBitrangeAccess",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueForBitrange({SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension}, SignalValueLookupTest::defaultBlockedBitRange);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                const bool accessesBlockedValueForFirstDimension  = !accessedDimensions.front().has_value() || (accessedDimensions.front().has_value() && (*accessedDimensions.front()) == SignalValueLookupTest::lockedValueOfFirstDimension);
                const bool accessesBlockedValueForSecondDimension = accessesBlockedValueForFirstDimension && accessedDimensions.size() > 1 && (!accessedDimensions.at(1).has_value() || (*accessedDimensions.at(1)) == SignalValueLookupTest::lockedValueOfSecondDimension);
                return accessesBlockedValueForSecondDimension && (accessedBitRange.has_value() ? SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange) : true);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)) {
                    return std::nullopt;
                }

                const bool accessesBlockedValueForFirstDimension  = !accessedDimensions.front().has_value() || (accessedDimensions.front().has_value() && (*accessedDimensions.front()) == SignalValueLookupTest::lockedValueOfFirstDimension);
                const bool accessesBlockedValueForSecondDimension = accessesBlockedValueForFirstDimension && accessedDimensions.size() > 1 && (!accessedDimensions.at(1).has_value() || (*accessedDimensions.at(1)) == SignalValueLookupTest::lockedValueOfSecondDimension);
                if (accessesBlockedValueForSecondDimension && (accessedBitRange.has_value() ? SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange) : true)) {
                    return std::nullopt;
                }

                if (!accessedBitRange.has_value()) {
                    return std::make_optional(SignalValueLookupTest::defaultValue);
                }
                return std::make_optional(SignalValueLookupTest::determineExpectedValueForBitRange(*accessedBitRange));
            }),
        std::make_tuple(
            "FullySpecififiedDimensionAccessWithMixedAccess",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueFor({std::nullopt, SignalValueLookupTest::lockedValueOfSecondDimension, std::nullopt});
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess&) {
                if (accessedDimensions.size() > 1) {
                    return accessedDimensions.at(1).has_value() ? (*accessedDimensions.at(1)) == SignalValueLookupTest::lockedValueOfSecondDimension : true;    
                }
                return true;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)) {
                    return std::nullopt;
                }

                if (accessedDimensions.size() > 1 && accessedDimensions.at(1).has_value() && (*accessedDimensions.at(1)) == SignalValueLookupTest::lockedValueOfSecondDimension) {
                    return std::nullopt;
                }

                if (!accessedBitRange.has_value()) {
                    return std::make_optional(SignalValueLookupTest::defaultValue);
                }
                return std::make_optional(SignalValueLookupTest::determineExpectedValueForBitRange(*accessedBitRange));
            }),
        std::make_tuple(
            "FullySpecififiedDimensionAccessWithMixedAccessAndBitrangeAccess",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueForBitrange({std::nullopt, SignalValueLookupTest::lockedValueOfSecondDimension, std::nullopt}, SignalValueLookupTest::defaultBlockedBitRange);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                if (accessedDimensions.size() > 1) {
                    if (accessedDimensions.at(1).has_value()) {
                        return (*accessedDimensions.at(1)) == SignalValueLookupTest::lockedValueOfSecondDimension && (accessedBitRange.has_value() ? SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange) : true);
                    }
                }
                return true;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)) {
                    return std::nullopt;
                }
                if (!accessedBitRange.has_value()) {
                    return std::make_optional(SignalValueLookupTest::defaultValue);
                }
                return std::make_optional(SignalValueLookupTest::determineExpectedValueForBitRange(*accessedBitRange));
            }),
        /*
         * EXTENDING RESTRICTION TESTS
         */
        std::make_tuple(
            "DISABLED_extendRestrictionForTODO",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueForBitrange({std::nullopt, SignalValueLookupTest::lockedValueOfSecondDimension, std::nullopt}, SignalValueLookupTest::defaultBlockedBitRange);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                return false;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                return std::nullopt;
            }),

        /*
         * LIFTING RESTRICTIONS TESTS
         */
        std::make_tuple(
            "DISABLED_LiftRestrictionForWholeSignalFromCompletelyBlockedSignal",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueForBitrange({std::nullopt, SignalValueLookupTest::lockedValueOfSecondDimension, std::nullopt}, SignalValueLookupTest::defaultBlockedBitRange);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                return false;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                return std::nullopt;
            }),
        std::make_tuple(
            "DISABLED_LiftRestrictionForGlobalBitRangeFromCompletelyBlockedSignal",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueForBitrange({std::nullopt, SignalValueLookupTest::lockedValueOfSecondDimension, std::nullopt}, SignalValueLookupTest::defaultBlockedBitRange);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                return false;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                return std::nullopt;
            }),
        std::make_tuple(
            "DISABLED_LiftRestrictionForBitRangeOfWholeIntermediateDimensionFromCompletelyBlockedSignal",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueForBitrange({std::nullopt, SignalValueLookupTest::lockedValueOfSecondDimension, std::nullopt}, SignalValueLookupTest::defaultBlockedBitRange);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                return false;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                return std::nullopt;
            }),
        std::make_tuple(
            "DISABLED_LiftRestrictionForBitRangeOfValueOfIntermediateDimensionFromCompletelyBlockedSignal",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueForBitrange({std::nullopt, SignalValueLookupTest::lockedValueOfSecondDimension, std::nullopt}, SignalValueLookupTest::defaultBlockedBitRange);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                return false;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                return std::nullopt;
            }),
        std::make_tuple(
            "DISABLED_LiftRestrictionForBitRangeOfValueOfLastDimensionFromCompletelyBlockedSignal",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueForBitrange({std::nullopt, SignalValueLookupTest::lockedValueOfSecondDimension, std::nullopt}, SignalValueLookupTest::defaultBlockedBitRange);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                return false;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                return std::nullopt;
            }),
        std::make_tuple(
            "DISABLED_LiftRestrictionForBitRangeOfWholeLastDimensionFromCompletelyBlockedSignal",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueForBitrange({std::nullopt, SignalValueLookupTest::lockedValueOfSecondDimension, std::nullopt}, SignalValueLookupTest::defaultBlockedBitRange);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                return false;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                return std::nullopt;
            }),
        std::make_tuple(
            "DISABLED_LiftRestrictionForValueOfIntermediateDimensionFromCompletelyBlockedSignal",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueForBitrange({std::nullopt, SignalValueLookupTest::lockedValueOfSecondDimension, std::nullopt}, SignalValueLookupTest::defaultBlockedBitRange);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                return false;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                return std::nullopt;
            }),
        std::make_tuple(
            "DISABLED_LiftRestrictionForWholeIntermediateDimensionFromCompletelyBlockedSignal",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueForBitrange({std::nullopt, SignalValueLookupTest::lockedValueOfSecondDimension, std::nullopt}, SignalValueLookupTest::defaultBlockedBitRange);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                return false;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                return std::nullopt;
            }),

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

                if (accessedDimensions.size() >= 2
                    && accessedDimensions.at(0).has_value() && (*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension
                    && accessedDimensions.at(1).has_value() && (*accessedDimensions.at(1)) == SignalValueLookupTest::lockedValueOfSecondDimension) {
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
                signalValueLookupWithoutRestriction->invalidateStoredValueForBitrange({ std::nullopt }, SignalValueLookupTest::defaultBlockedBitRange);
                signalValueLookupWithoutRestriction->liftRestrictionsOfDimensions({ SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension }, std::nullopt);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                if (accessedDimensions.size() >= 2
                    && accessedDimensions.at(0).has_value() && (*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension 
                    && accessedDimensions.at(1).has_value() && (*accessedDimensions.at(1)) == SignalValueLookupTest::lockedValueOfSecondDimension) {
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

                if ((*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension 
                    && (*accessedDimensions.at(1)) == SignalValueLookupTest::lockedValueOfSecondDimension) {
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

                if ((*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension
                    && (*accessedDimensions.at(1)) == SignalValueLookupTest::lockedValueOfSecondDimension
                    && (*accessedDimensions.at(2)) == SignalValueLookupTest::lockedValueOfThirdDimension) {
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

                if ((*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension 
                    && (*accessedDimensions.at(1)) == SignalValueLookupTest::lockedValueOfSecondDimension 
                    && (*accessedDimensions.at(2)) == SignalValueLookupTest::lockedValueOfThirdDimension) {
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
            }),
        /*
         * a[0][$i][x]
         *
         * 1. a[0][$i]
         * 2. a[0][$i][0]
         * 3. a[0][1]
         * 4. a[0][1][0]
         * 5. a[0][$i].s:e
         * 6. a[0][1].s:e
         * 7. a.s:e
         * 8.
         * 9.
         */
        std::make_tuple(
            "DISABLED_LiftRestrictionForWholeDimensionFromCompletelyBlockedDimensionForSameDimensoin",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueForBitrange({std::nullopt, SignalValueLookupTest::lockedValueOfSecondDimension, std::nullopt}, SignalValueLookupTest::defaultBlockedBitRange);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                return false;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                return std::nullopt;
            }),
        std::make_tuple(
            "DISABLED_LiftRestrictionForValueOfDimensionFromCompletelyBlockedDimensionForSameDimension",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueForBitrange({std::nullopt, SignalValueLookupTest::lockedValueOfSecondDimension, std::nullopt}, SignalValueLookupTest::defaultBlockedBitRange);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                return false;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                return std::nullopt;
            }),
        std::make_tuple(
            "DISABLED_LiftRestrictionForBitRangeOfWholeDimensionFromCompletelyBlockedDimension",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueForBitrange({std::nullopt, SignalValueLookupTest::lockedValueOfSecondDimension, std::nullopt}, SignalValueLookupTest::defaultBlockedBitRange);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                return false;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                return std::nullopt;
            }),
        std::make_tuple(
            "DISABLED_LiftRestrictionForBitRangeOfValueOfDimensionFromCompletelyBlockedDimension",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueForBitrange({std::nullopt, SignalValueLookupTest::lockedValueOfSecondDimension, std::nullopt}, SignalValueLookupTest::defaultBlockedBitRange);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                return false;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                return std::nullopt;
            }),
        std::make_tuple(
            "DISABLED_LiftRestrictionForGlobalBitRangeWithCompletelyBlockedDimension",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueForBitrange({std::nullopt, SignalValueLookupTest::lockedValueOfSecondDimension, std::nullopt}, SignalValueLookupTest::defaultBlockedBitRange);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                return false;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                return std::nullopt;
            })
        ),
    [](const testing::TestParamInfo<SignalValueLookupTest::ParamType>& info) {
        auto testNameToTransform = std::get<0>(info.param);
        std::replace(testNameToTransform.begin(), testNameToTransform.end(), '-', '_');
        return testNameToTransform;
    });