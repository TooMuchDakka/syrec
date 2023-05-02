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
* 
* TODO: Add test cases for lifting restrictions when more than one restriction is set
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
                if (!BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)) {
                    return std::nullopt;
                }

                if (accessedBitRange.has_value()) {
                    return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),
        std::make_tuple(
            "LiftRestrictionFromSignalWithWholeIntermediateDimensionBlockedForValueOfSameDimension",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, std::nullopt});
                signalValueLookupWithoutRestriction->liftRestrictionsOfDimensions({SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension}, std::nullopt);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess&) {
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
                if (!BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)) {
                    return std::nullopt;
                }

                if ((*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension) {
                    if ((*accessedDimensions.at(1)) == SignalValueLookupTest::lockedValueOfSecondDimension) {
                        if (accessedBitRange.has_value()) {
                            return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                        }
                        return std::make_optional(SignalValueLookupTest::defaultValue);
                    }
                    return std::nullopt;
                }

                if (accessedBitRange.has_value()) {
                    return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),
        std::make_tuple(
            "LiftRestrictionFromSignalWithWholeIntermediateDimensionBlockedForBitRangeOfWholeOfSameDimension",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, std::nullopt});
                signalValueLookupWithoutRestriction->liftRestrictionsOfDimensions({SignalValueLookupTest::lockedValueOfFirstDimension, std::nullopt}, SignalValueLookupTest::defaultBlockedBitRange);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                if (accessedDimensions.size() >= 2) {
                    if (!accessedDimensions.at(0).has_value()) {
                        return true;
                    } else if ((*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension) {
                        if (!accessedBitRange.has_value()) {
                            return true;
                        }
                        return !BaseSignalValueLookupTest::doesBitrangeLieWithin(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange);
                    }
                    return false;
                }
                return false;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (!BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)) {
                    return std::nullopt;
                }

                if ((*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension) {
                    if (accessedBitRange.has_value()) {
                        if (BaseSignalValueLookupTest::doesBitrangeLieWithin(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange)) {
                            return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                        }
                    }
                    return std::nullopt;
                }
                if (accessedBitRange.has_value()) {
                    return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
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
                            return !BaseSignalValueLookupTest::doesBitrangeLieWithin(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange);
                        }
                    }
                    return false;
                }
                return false;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (!BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)) {
                    return std::nullopt;
                }

                if ((*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension) {
                    if ((*accessedDimensions.at(1)) == SignalValueLookupTest::lockedValueOfSecondDimension) {
                        if (accessedBitRange.has_value() && BaseSignalValueLookupTest::doesBitrangeLieWithin(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange)) {
                            if (BaseSignalValueLookupTest::doesBitrangeLieWithin(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange)) {
                                return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                            }
                        }
                    }
                    return std::nullopt;
                }
                if (accessedBitRange.has_value()) {
                    return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),
        std::make_tuple(
            "LiftRestrictionFromSignalWithWholeIntermediateDimensionBlockedByUnblockingValueOfBlockedAndValueOfSubsequentDimension",
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
                if (!BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)) {
                    return std::nullopt;
                }

                if ((*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension) {
                    if ((*accessedDimensions.at(1)) == SignalValueLookupTest::lockedValueOfSecondDimension && (*accessedDimensions.at(2)) == SignalValueLookupTest::lockedValueOfThirdDimension) {
                        if (accessedBitRange.has_value()) {
                            return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                        }
                        return std::make_optional(SignalValueLookupTest::defaultValue);
                    }
                    return std::nullopt;
                }
                if (accessedBitRange.has_value()) {
                    return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
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
                if (accessedDimensions.empty()) {
                    return true;
                }

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
                if (!BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)) {
                    return std::nullopt;
                }

                if ((*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension) {
                    if ((*accessedDimensions.at(1)) == SignalValueLookupTest::lockedValueOfSecondDimension) {
                        if (accessedBitRange.has_value()) {
                            return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                        }
                        return std::make_optional(SignalValueLookupTest::defaultValue);
                    }
                    return std::nullopt;
                }
                if (accessedBitRange.has_value()) {
                    return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),
        std::make_tuple(
            "LiftRestrictionFromSignalWithWholeIntermediateDimensionBlockedForBitRangeOfValueOfSubsequentDimension",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, std::nullopt});
                signalValueLookupWithoutRestriction->liftRestrictionsOfDimensions({SignalValueLookupTest::lockedValueOfFirstDimension, std::nullopt, SignalValueLookupTest::lockedValueOfThirdDimension}, SignalValueLookupTest::defaultNonOverlappingBitRange);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                if (accessedDimensions.empty() || !accessedDimensions.at(0).has_value()) {
                    return true;
                }

                if ((*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension) {
                    if (!BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions) || !accessedBitRange.has_value()) {
                        return true;
                    }

                    if ((*accessedDimensions.at(2)) != SignalValueLookupTest::lockedValueOfThirdDimension) {
                        return true;
                    }
                    return !BaseSignalValueLookupTest::doesBitrangeLieWithin(*accessedBitRange, SignalValueLookupTest::defaultNonOverlappingBitRange);
                }

                return false;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (!BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)) {
                    return std::nullopt;
                }

                if ((*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension) {
                    if (!accessedBitRange.has_value()) {
                        return std::nullopt;
                    }

                    if ((*accessedDimensions.at(2)) != SignalValueLookupTest::lockedValueOfThirdDimension) {
                        return std::nullopt;
                    }

                    if (BaseSignalValueLookupTest::doesBitrangeLieWithin(*accessedBitRange, SignalValueLookupTest::defaultNonOverlappingBitRange)) {
                        return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                    }
                }

                if (accessedBitRange.has_value()) {
                    return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),
        std::make_tuple(
            "LiftRestrictionFromSignalWithWholeIntermediateDimensionBlockedForBitRangeOfWholeOfSubsequentDimension",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, std::nullopt});
                signalValueLookupWithoutRestriction->liftRestrictionsOfDimensions({SignalValueLookupTest::lockedValueOfFirstDimension, std::nullopt, std::nullopt}, SignalValueLookupTest::defaultNonOverlappingBitRange);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                if (accessedDimensions.empty() || !accessedDimensions.at(0).has_value()) {
                    return true;
                }

                if ((*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension) {
                    if (!BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions) || !accessedBitRange.has_value()) {
                        return true;
                    }
                    return !BaseSignalValueLookupTest::doesBitrangeLieWithin(*accessedBitRange, SignalValueLookupTest::defaultNonOverlappingBitRange);
                }

                return false;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (!BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)) {
                    return std::nullopt;
                }

                if ((*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension) {
                    if (!accessedBitRange.has_value()) {
                        return std::nullopt;
                    }

                    if (BaseSignalValueLookupTest::doesBitrangeLieWithin(*accessedBitRange, SignalValueLookupTest::defaultNonOverlappingBitRange)) {
                        return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                    }
                }

                if (accessedBitRange.has_value()) {
                    return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),
        std::make_tuple(
            "LiftRestrictionFromSignalWithWholeIntermediateDimensionBlockedForWholeParentDimension",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, std::nullopt});
                signalValueLookupWithoutRestriction->liftRestrictionsOfDimensions({std::nullopt}, std::nullopt);
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) {
                return false;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (!BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)) {
                    return std::nullopt;
                }

                if (accessedBitRange.has_value()) {
                    return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),
        std::make_tuple(
            "LiftRestrictionFromSignalWithWholeIntermediateDimensionBlockedForValueOfParentDimension",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, std::nullopt});
                signalValueLookupWithoutRestriction->liftRestrictionsOfDimensions({SignalValueLookupTest::lockedValueOfFirstDimension},std::nullopt);
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) {
                return false;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (!BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)) {
                    return std::nullopt;
                }

                if (accessedBitRange.has_value()) {
                    return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),
        std::make_tuple(
            "LiftRestrictionFromSignalWithWholeIntermediateDimensionBlockedForBitRangeOfWholeParentDimension",
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
                    return !BaseSignalValueLookupTest::doesBitrangeLieWithin(*accessedBitRange, SignalValueLookupTest::defaultNonOverlappingBitRange);
                }
                return false;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (!BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)) {
                    return std::nullopt;
                }

                if ((*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension) {
                    if (accessedBitRange.has_value()) {
                        if (BaseSignalValueLookupTest::doesBitrangeLieWithin(*accessedBitRange, SignalValueLookupTest::defaultNonOverlappingBitRange)) {
                            return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                        }
                    }
                    return std::nullopt;
                }

                if (accessedBitRange.has_value()) {
                    return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),
        std::make_tuple(
            "LiftRestrictionFromSignalWithWholeIntermediateDimensionBlockedForBitRangeOfValueOfParentDimension",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, std::nullopt});
                signalValueLookupWithoutRestriction->liftRestrictionsOfDimensions({SignalValueLookupTest::lockedValueOfFirstDimension}, SignalValueLookupTest::defaultNonOverlappingBitRange);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                if (accessedDimensions.empty() || !accessedDimensions.at(0).has_value())
                    return true;

                if ((*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension) {
                    if (!accessedBitRange.has_value()) {
                        return true;
                    }
                    return !BaseSignalValueLookupTest::doesBitrangeLieWithin(*accessedBitRange, SignalValueLookupTest::defaultNonOverlappingBitRange);
                }
                return false;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (!BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)) {
                    return std::nullopt;
                }

                if ((*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension) {
                    if (accessedBitRange.has_value()) {
                        if (BaseSignalValueLookupTest::doesBitrangeLieWithin(*accessedBitRange, SignalValueLookupTest::defaultNonOverlappingBitRange)) {
                            return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                        }
                    }
                    return std::nullopt;
                }

                if (accessedBitRange.has_value()) {
                    return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),
        std::make_tuple(
            "LiftRestrictionFromSignalWithWholeIntermediateDimensionBlockedForGlobalBitRange",
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
                    return !BaseSignalValueLookupTest::doesBitrangeLieWithin(*accessedBitRange, SignalValueLookupTest::defaultNonOverlappingBitRange);
                }
                return false;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (!BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)) {
                    return std::nullopt;
                }

                if ((*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension) {
                    if (accessedBitRange.has_value()) {
                        if (BaseSignalValueLookupTest::doesBitrangeLieWithin(*accessedBitRange, SignalValueLookupTest::defaultNonOverlappingBitRange)) {
                            return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                        }
                    }
                    return std::nullopt;
                }

                if (accessedBitRange.has_value()) {
                    return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
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
                if (!BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)) {
                    return std::nullopt;
                }

                if (accessedBitRange.has_value()) {
                    return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),
        std::make_tuple(
            "LiftUnrelatedGlobalBitrangeRestrictionFromSignalWithBitRangeOfWholeIntermediateDimensionBlocked",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueForBitrange({SignalValueLookupTest::lockedValueOfFirstDimension, std::nullopt}, SignalValueLookupTest::defaultBlockedBitRange);
                signalValueLookupWithoutRestriction->liftRestrictionsOfDimensions({std::nullopt}, SignalValueLookupTest::defaultNonOverlappingBitRange);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                if (accessedDimensions.empty()) {
                    return true;
                }

                if (accessedDimensions.at(0).has_value()) {
                    if ((*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension) {
                        if (!accessedBitRange.has_value()) {
                            return true;
                        }
                        return BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange);
                    }
                    else {
                        return false;
                    }
                }
                else {
                    return true;
                }
                return false;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (!BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)) {
                    return std::nullopt;
                }

                if ((*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension) {
                    if ((accessedBitRange.has_value() && BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange))
                        || !accessedBitRange.has_value()) {
                        return std::nullopt;
                    }

                    if (accessedBitRange.has_value()) {
                        return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                    }
                    return std::make_optional(SignalValueLookupTest::defaultValue);
                }

                if (accessedBitRange.has_value()) {
                    return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),
        std::make_tuple(
            "LiftUnrelatedValueOfDimensionRestrictionFromSignalWithWholeIntermediateDimensionBlocked",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfFirstDimension});
                signalValueLookupWithoutRestriction->liftRestrictionsOfDimensions({ SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension}, std::nullopt);
                signalValueLookupWithoutRestriction->liftRestrictionsOfDimensions({SignalValueLookupTest::lockedValueOfFirstDimension + 1, SignalValueLookupTest::lockedValueOfSecondDimension}, std::nullopt);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess&) {
                if (accessedDimensions.empty()) {
                    return true;
                }

                if (!accessedDimensions.at(0).has_value()) {
                    return true;
                }

                if ((*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension) {
                    if (accessedDimensions.size() >= 2) {
                        if (!accessedDimensions.at(1).has_value() || (*accessedDimensions.at(1)) == SignalValueLookupTest::lockedValueOfFirstDimension) {
                            return true;
                        }
                    }   
                }
                return false;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (!BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)) {
                    return std::nullopt;
                }

                if ((*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension) {
                    if ((*accessedDimensions.at(1)) == SignalValueLookupTest::lockedValueOfFirstDimension) {
                        return std::nullopt;
                    }
                }

                if (accessedBitRange.has_value()) {
                    return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),
        std::make_tuple(
            "LiftUnrelatedBitrangeRestrictionOfValueOfDimensionFromSignalWithWholeIntermediateDimensionBlocked",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfFirstDimension});
                signalValueLookupWithoutRestriction->liftRestrictionsOfDimensions({SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension, std::nullopt}, std::nullopt);
                signalValueLookupWithoutRestriction->liftRestrictionsOfDimensions({SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension}, std::nullopt);
                signalValueLookupWithoutRestriction->liftRestrictionsOfDimensions({SignalValueLookupTest::lockedValueOfFirstDimension + 1}, std::nullopt);
                signalValueLookupWithoutRestriction->liftRestrictionsOfDimensions({SignalValueLookupTest::lockedValueOfFirstDimension + 1, SignalValueLookupTest::lockedValueOfSecondDimension, std::nullopt}, std::nullopt);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess&) {
                if (accessedDimensions.empty()) {
                    return true;
                }

                if (!accessedDimensions.at(0).has_value()) {
                    return true;
                }

                if ((*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension) {
                    if (accessedDimensions.size() >= 2) {
                        if (!accessedDimensions.at(1).has_value() || (*accessedDimensions.at(1)) == SignalValueLookupTest::lockedValueOfFirstDimension) {
                            return true;
                        }
                    }
                }
                return false;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (!BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)) {
                    return std::nullopt;
                }

                if ((*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension) {
                    if ((*accessedDimensions.at(1)) == SignalValueLookupTest::lockedValueOfFirstDimension) {
                        return std::nullopt;
                    }
                }

                if (accessedBitRange.has_value()) {
                    return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),
        std::make_tuple(
            "LiftUnrelatedBitrangeRestrictionOfSameValueOfDimensionFromSignalWithValueOfIntermediateDimensionBlocked",
            [](const SignalValueLookup::ptr& signalValueLookupWithoutRestriction) {
                signalValueLookupWithoutRestriction->invalidateStoredValueForBitrange({SignalValueLookupTest::lockedValueOfFirstDimension, std::nullopt}, SignalValueLookupTest::defaultBlockedBitRange);
                signalValueLookupWithoutRestriction->liftRestrictionsOfDimensions({SignalValueLookupTest::lockedValueOfFirstDimension, std::nullopt}, SignalValueLookupTest::defaultNonOverlappingBitRange);
                signalValueLookupWithoutRestriction->liftRestrictionsOfDimensions({SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension, SignalValueLookupTest::lockedValueOfThirdDimension}, SignalValueLookupTest::defaultNonOverlappingBitRange);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                if (accessedDimensions.empty()) {
                    return true;
                }

                if (accessedDimensions.at(0).has_value()) {
                    if ((*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension) {
                        if (!accessedBitRange.has_value()) {
                            return true;
                        }
                        return BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange);
                    } else {
                        return false;
                    }
                } else {
                    return true;
                }
                return false;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (!BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)) {
                    return std::nullopt;
                }

                if ((*accessedDimensions.at(0)) == SignalValueLookupTest::lockedValueOfFirstDimension) {
                    if ((accessedBitRange.has_value() && BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange)) || !accessedBitRange.has_value()) {
                        return std::nullopt;
                    }

                    if (accessedBitRange.has_value()) {
                        return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                        //return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                    }
                    return std::make_optional(SignalValueLookupTest::defaultValue);
                }

                if (accessedBitRange.has_value()) {
                    return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            })),
         [](const testing::TestParamInfo<SignalValueLookupTest::ParamType>& info) {
            auto testNameToTransform = std::get<0>(info.param);
            std::replace(testNameToTransform.begin(), testNameToTransform.end(), '-', '_');
            return testNameToTransform;
        });