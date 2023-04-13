#include "test_signal_value_lookup.hpp"
#include <googletest/googletest/include/gtest/gtest.h>

using namespace valueLookup;

using UserDefinedDimensionAccess = std::vector<std::optional<unsigned int>>;
using OptionalBitRangeAccess = std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess>;


TEST_P(SignalValueLookupTest, CheckRestrictionAndFetchedValueOfSignalValueLookupTests) {
    const auto& testParamInfo               = GetParam();
    const auto& initialRestrictionGenerator = std::get<1>(testParamInfo);
    initialRestrictionGenerator(signalValueLookup);

    //createCombinations({2, 4});

    const auto& expectedRestrictionStatusLookup = std::get<2>(testParamInfo);
    const auto& expectedValueLookup             = std::get<3>(testParamInfo);

    ASSERT_NO_FATAL_FAILURE(checkThatNoValueCanBeFetchedForGlobalBitRanges());
    ASSERT_NO_FATAL_FAILURE(checkThatNoValueCanBeFetchedForValuesOrBitRangesOfIntermediateDimensions());

    ASSERT_NO_FATAL_FAILURE(
            createAndCheckConditionForAllDimensionAndBitrangeCombinations(
                    SignalValueLookupTest::defaultSignalDimensions,
                    expectedRestrictionStatusLookup,
                    expectedValueLookup));

    //ASSERT_NO_FATAL_FAILURE(checkThatValueMatchesIfItCanBeFetched(expectedRestrictionStatusLookup, expectedValueLookup));
}

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
            })),
    [](const testing::TestParamInfo<SignalValueLookupTest::ParamType>& info) {
        auto testNameToTransform = std::get<0>(info.param);
        std::replace(testNameToTransform.begin(), testNameToTransform.end(), '-', '_');
        return testNameToTransform;
    });