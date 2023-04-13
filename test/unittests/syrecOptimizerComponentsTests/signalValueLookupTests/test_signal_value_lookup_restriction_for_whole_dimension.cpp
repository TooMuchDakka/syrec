#include "test_signal_value_lookup.hpp"

using namespace valueLookup;

using UserDefinedDimensionAccess = std::vector<std::optional<unsigned int>>;
using OptionalBitRangeAccess     = std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess>;

/*
* a[0][$i][x]
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
            "LiftRestrictionFromSignalWithWholeIntermediateDimensionBlockedForWholeOfSameDimension",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, std::nullopt});
                signalValueLookupWithoutRestriction->liftRestrictionsOfDimensions({SignalValueLookupTest::lockedValueOfFirstDimension, std::nullopt}, std::nullopt);
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
        std::make_tuple(
            "LiftRestrictionFromSignalWithWholeIntermediateDimensionBlockedForValueOfSameDimension",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, std::nullopt});
                signalValueLookupWithoutRestriction->liftRestrictionsOfDimensions({SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension}, std::nullopt);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                if (accessedDimensions.empty()) {
                    return true;
                }
                if (accessedDimensions.size() >= 1) {
                    if (!accessedDimensions.at(0).has_value()) {
                        return true;
                    } else if ((*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension) {
                        if (accessedDimensions.size() >= 2) {
                            if (!accessedDimensions.at(1).has_value()) {
                                return true;
                            }
                            return !(accessedDimensions.at(1).has_value() && (*accessedDimensions.at(1)) == SignalValueLookupTest::lockedValueOfSecondDimension);
                        } else {
                            return true;
                        }
                    }
                }
                return false;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)) {
                    return std::nullopt;
                }

                if ((*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension) {
                    if ((*accessedDimensions.at(1)) == SignalValueLookupTest::lockedValueOfSecondDimension) {
                        if (accessedBitRange.has_value()) {
                            return std::make_optional(SignalValueLookupTest::determineExpectedValueForBitRange(*accessedBitRange));
                        }
                        return std::make_optional(SignalValueLookupTest::defaultValue);
                    }
                    return std::nullopt;
                }

                if (accessedBitRange.has_value()) {
                    return std::make_optional(SignalValueLookupTest::determineExpectedValueForBitRange(*accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),
        std::make_tuple(
            "LiftRestrictionFromSignalWithWholeIntermediateDimensionBlockedForBitRangeOfWholeOfSameDimension",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, std::nullopt});
                signalValueLookupWithoutRestriction->liftRestrictionsOfDimensions({(SignalValueLookupTest::lockedValueOfFirstDimension, std::nullopt)}, SignalValueLookupTest::defaultBlockedBitRange);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                if (accessedDimensions.size() >= 2) {
                    if (!accessedDimensions.at(0).has_value()) {
                        return true;
                    } else if ((*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension) {
                        if (!accessedBitRange.has_value()) {
                            return true;
                        }
                        return !SignalValueLookupTest::doesBitrangeLieWithin(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange);
                    }
                    return false;
                }
                return false;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)) {
                    return std::nullopt;
                }

                if ((*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension) {
                    if (accessedBitRange.has_value()) {
                        if (SignalValueLookupTest::doesBitrangeLieWithin(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange)) {
                            return std::make_optional(SignalValueLookupTest::determineExpectedValueForBitRange(*accessedBitRange));
                        }
                    }
                    return std::nullopt;
                }
                if (accessedBitRange.has_value()) {
                    return std::make_optional(SignalValueLookupTest::determineExpectedValueForBitRange(*accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),
        std::make_tuple(
            "LiftRestrictionFromSignalWithWholeIntermediateDimensionBlockedForBitRangeOfValueOfSameDimension",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, std::nullopt});
                signalValueLookupWithoutRestriction->liftRestrictionsOfDimensions({SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension}, SignalValueLookupTest::defaultBlockedBitRange);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                if (accessedDimensions.size() >= 2) {
                    if (!accessedDimensions.at(0).has_value()) {
                        return true;
                    } else if ((*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension) {
                        if (!accessedDimensions.at(1).has_value()) {
                            return true;
                        }
                        if ((*accessedDimensions.at(1)) == SignalValueLookupTest::lockedValueOfSecondDimension) {
                            if (!accessedBitRange.has_value()) {
                                return true;
                            }
                            return !SignalValueLookupTest::doesBitrangeLieWithin(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange);
                        }
                    }
                    return false;
                }
                return false;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)) {
                    return std::nullopt;
                }

                if ((*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension) {
                    if ((*accessedDimensions.at(1)) == SignalValueLookupTest::lockedValueOfSecondDimension) {
                        if (accessedBitRange.has_value() && SignalValueLookupTest::doesBitrangeLieWithin(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange)) {
                            if (SignalValueLookupTest::doesBitrangeLieWithin(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange)) {
                                return std::make_optional(SignalValueLookupTest::determineExpectedValueForBitRange(*accessedBitRange));
                            }
                        }
                    }
                    return std::nullopt;
                }
                if (accessedBitRange.has_value()) {
                    return std::make_optional(SignalValueLookupTest::determineExpectedValueForBitRange(*accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),
        std::make_tuple(
            "LiftRestrictionFromSignalWithWholeIntermediateDimensionBlockedByUnblockingValueOfBlockedAndValueSubsequentDimension",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, std::nullopt});
                signalValueLookupWithoutRestriction->liftRestrictionsOfDimensions({SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension, SignalValueLookupTest::lockedValueOfThirdDimension}, std::nullopt);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess&) {
                switch (accessedDimensions.size()) {
                    case 1:
                        if (!accessedDimensions.at(0).has_value())
                            return true;
                        break;
                    case 2:
                        if (!accessedDimensions.at(0).has_value())
                            return true;

                        if ((*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension)
                            return true;

                        if (!accessedDimensions.at(1).has_value())
                            return true;
                        if ((*accessedDimensions.at(1)) != SignalValueLookupTest::lockedValueOfSecondDimension)
                            return true;

                        return false;
                        break;
                    case 3:
                        if (!accessedDimensions.at(0).has_value())
                            return true;
                        if (!accessedDimensions.at(1).has_value())
                            return true;
                        if (!accessedDimensions.at(2).has_value())
                            return true;

                        if ((*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension && (*accessedDimensions.at(1)) != SignalValueLookupTest::lockedValueOfSecondDimension && (*accessedDimensions.at(2)) != SignalValueLookupTest::lockedValueOfThirdDimension)
                            return true;

                        return false;
                        break;
                    default:
                        return true;
                }
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)) {
                    return std::nullopt;
                }

                if ((*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension) {
                    if ((*accessedDimensions.at(1)) == SignalValueLookupTest::lockedValueOfSecondDimension && (*accessedDimensions.at(2)) == SignalValueLookupTest::lockedValueOfThirdDimension) {
                        if (accessedBitRange.has_value()) {
                            return std::make_optional(SignalValueLookupTest::determineExpectedValueForBitRange(*accessedBitRange));
                        }
                        return std::make_optional(SignalValueLookupTest::defaultValue);
                    }
                    return std::nullopt;
                }
                if (accessedBitRange.has_value()) {
                    return std::make_optional(SignalValueLookupTest::determineExpectedValueForBitRange(*accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),
        std::make_tuple(
            "LiftRestrictionFromSignalWithWholeIntermediateDimensionBlockedByUnblockingValueOfBlockedAndWholeSubsequentDimension",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, std::nullopt});
                signalValueLookupWithoutRestriction->liftRestrictionsOfDimensions({SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension, std::nullopt}, std::nullopt);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess&) {
                switch (accessedDimensions.size()) {
                    case 1:
                        if (!accessedDimensions.at(0).has_value())
                            return true;
                        break;
                    case 2:
                    case 3:
                        if (!accessedDimensions.at(0).has_value())
                            return true;

                        if ((*accessedDimensions.at(0)) != SignalValueLookupTest::lockedValueOfFirstDimension)
                            return false;

                        if (!accessedDimensions.at(1).has_value())
                            return true;
                        if ((*accessedDimensions.at(1)) != SignalValueLookupTest::lockedValueOfSecondDimension)
                            return true;

                        return false;
                        break;
                    default:
                        return true;
                }
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)) {
                    return std::nullopt;
                }

                if ((*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension) {
                    if ((*accessedDimensions.at(1)) == SignalValueLookupTest::lockedValueOfSecondDimension) {
                        if (accessedBitRange.has_value()) {
                            return std::make_optional(SignalValueLookupTest::determineExpectedValueForBitRange(*accessedBitRange));
                        }
                        return std::make_optional(SignalValueLookupTest::defaultValue);
                    }
                    return std::nullopt;
                }
                if (accessedBitRange.has_value()) {
                    return std::make_optional(SignalValueLookupTest::determineExpectedValueForBitRange(*accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),
        std::make_tuple(
            "DISABLED_LiftRestrictionFromSignalWithWholeIntermediateDimensionBlockedForBitRangeOfValueOfSubsequentDimension",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, std::nullopt});
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                return false;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                return std::nullopt;
            }),
        std::make_tuple(
            "DISABLED_LiftRestrictionFromSignalWithWholeIntermediateDimensionBlockedForBitRangeOfWholeOfSubsequentDimension",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, std::nullopt});
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                return false;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                return std::nullopt;
            }),
        std::make_tuple(
            "DISABLED_LiftRestrictionFromSignalWithWholeIntermediateDimensionBlockedForWholeParentDimension",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, std::nullopt});
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                return false;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                return std::nullopt;
            }),
        std::make_tuple(
            "DISABLED_LiftRestrictionFromSignalWithWholeIntermediateDimensionBlockedForValueOfParentDimension",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, std::nullopt});
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                return false;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                return std::nullopt;
            }),
        std::make_tuple(
            "DISABLED_LiftRestrictionFromSignalWithWholeIntermediateDimensionBlockedForBitRangeWholeParentDimension",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, std::nullopt});
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                return false;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                return std::nullopt;
            }),
        std::make_tuple(
            "DISABLED_LiftRestrictionFromSignalWithWholeIntermediateDimensionBlockedForBitRangeValueOfParentDimension",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, std::nullopt});
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                return false;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                return std::nullopt;
            }),
        std::make_tuple(
            "DISABLED_LiftRestrictionFromSignalWithWholeIntermediateDimensionBlockedForGlobalBitRange",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, std::nullopt});
                signalValueLookupWithoutRestriction->liftRestrictionsOfDimensions({std::nullopt}, SignalValueLookupTest::defaultNonOverlappingBitRange);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                if (accessedDimensions.empty() || !accessedDimensions.at(0).has_value())
                    return true;

                if ((*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension) {
                    if (!accessedBitRange.has_value()) {
                        return true;
                    }
                    return SignalValueLookupTest::doesBitrangeLieWithin(*accessedBitRange, SignalValueLookupTest::defaultNonOverlappingBitRange);
                }
                return false;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)) {
                }
                return std::nullopt;
            }),
        std::make_tuple(
            "LiftRestrictionFromSignalWithWholeIntermediateDimensionBlockedForWholeSignal",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueForBitrange({std::nullopt, SignalValueLookupTest::lockedValueOfSecondDimension, std::nullopt}, SignalValueLookupTest::defaultBlockedBitRange);
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