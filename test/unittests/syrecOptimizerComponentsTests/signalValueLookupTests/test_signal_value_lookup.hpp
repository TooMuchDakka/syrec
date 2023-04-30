#ifndef TEST_SIGNAL_VALUE_LOOKUP_HPP
#define TEST_SIGNAL_VALUE_LOOKUP_HPP
#pragma once

#include <gtest/gtest.h>
#include "core/syrec/parser/optimizations/constantPropagation/valueLookup/signal_value_lookup.hpp"
#include "core/syrec/parser/utils/bit_helpers.hpp"

#include <vector>

namespace valueLookup {
    using InitialRestrictionDefinition = std::function<void(const SignalValueLookup::ptr& signalValueLookup)>;
    using ExpectedRestrictionStatusLookup = std::function<bool(const std::vector<std::optional<unsigned int>>& accessedDimensions, const std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess>& accessedBitRange)>;
    using ExpectedValueLookup = std::function<std::optional<unsigned int>(const std::vector<std::optional<unsigned int>>& accessedDimensions, const std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess>& optionallyAccessedBitRange)>;

    class SignalValueLookupTest:
        public ::testing::Test,
        public ::testing::WithParamInterface<
                std::tuple<std::string,
                           InitialRestrictionDefinition,
                           ExpectedRestrictionStatusLookup,
                           ExpectedValueLookup>> {
    public:
        const static inline std::vector<unsigned int> defaultSignalDimensions = {2, 4, 2};
        constexpr static unsigned int                 defaultSignalBitwidth   = 8;
        constexpr static ::optimizations::BitRangeAccessRestriction::BitRangeAccess defaultValueRequiredStorageBitRange  = ::optimizations::BitRangeAccessRestriction::BitRangeAccess(0, 7);
        constexpr static unsigned int                 defaultValue            = 230; // 1110 0110

        constexpr static unsigned int                                               lockedValueOfFirstDimension                              = 1;
        constexpr static unsigned int                                               lockedOtherValueOfFirstDimension                         = 2;
        constexpr static unsigned int                                               lockedValueOfSecondDimension                             = 3;
        constexpr static unsigned int                                               lockedValueOfThirdDimension                              = 1;
        constexpr static unsigned int                                               firstBlockedBitOfDefaultBitRange = 2;
        constexpr static unsigned int                                               lastBlockedBitOfDefaultBitRange  = defaultSignalBitwidth - firstBlockedBitOfDefaultBitRange;
        constexpr static ::optimizations::BitRangeAccessRestriction::BitRangeAccess defaultBlockedBitRange           = ::optimizations::BitRangeAccessRestriction::BitRangeAccess(firstBlockedBitOfDefaultBitRange, lastBlockedBitOfDefaultBitRange);
        constexpr static ::optimizations::BitRangeAccessRestriction::BitRangeAccess defaultNonOverlappingBitRange    = ::optimizations::BitRangeAccessRestriction::BitRangeAccess(0, firstBlockedBitOfDefaultBitRange - 1);
        constexpr static ::optimizations::BitRangeAccessRestriction::BitRangeAccess defaultOverlappingBitRange       = ::optimizations::BitRangeAccessRestriction::BitRangeAccess(firstBlockedBitOfDefaultBitRange, firstBlockedBitOfDefaultBitRange + 2);
        constexpr static ::optimizations::BitRangeAccessRestriction::BitRangeAccess defaultRemainingBlockedBitRangeAfterOverlappingWasLifted = ::optimizations::BitRangeAccessRestriction::BitRangeAccess(firstBlockedBitOfDefaultBitRange + 3, lastBlockedBitOfDefaultBitRange);

        constexpr static ::optimizations::BitRangeAccessRestriction::BitRangeAccess defaultRemainingBlockedBitRangeAfterSignalWasUnblocked = ::optimizations::BitRangeAccessRestriction::BitRangeAccess(defaultBlockedBitRange.first - 1, defaultBlockedBitRange.second + 1);
        [[nodiscard]] static bool isFullySpecifiedDimensionAccess(const std::vector<std::optional<unsigned int>>& accessedDimensions) {
            return std::all_of(
                    accessedDimensions.cbegin(),
                    accessedDimensions.cend(),
                    [](const std::optional<unsigned int>& accessedValueOfDimension) {
                        return accessedValueOfDimension.has_value();
                    });
        }

        [[nodiscard]] static bool doesBitrangeOverlapOther(const ::optimizations::BitRangeAccessRestriction::BitRangeAccess& bitRangeToCheck,
                                                     const ::optimizations::BitRangeAccessRestriction::BitRangeAccess& other) {
            return !(bitRangeToCheck.second < other.first || bitRangeToCheck.first > other.second);
        }

        [[nodiscard]] static bool doesBitrangeLieWithin(const ::optimizations::BitRangeAccessRestriction::BitRangeAccess& bitRangeToCheck,
            const ::optimizations::BitRangeAccessRestriction::BitRangeAccess& other) {
            return bitRangeToCheck.first >= other.first && bitRangeToCheck.second <= other.second;
        }

        [[nodiscard]] static unsigned int determineValueForBitRangeFromValue(const unsigned int value, const ::optimizations::BitRangeAccessRestriction::BitRangeAccess& bitRange) {
            return BitHelpers::extractBitsFromValue(value, bitRange);
        }

        [[nodiscard]] static unsigned int determineExpectedValueForBitRange(const ::optimizations::BitRangeAccessRestriction::BitRangeAccess& bitRange) {
            return determineValueForBitRangeFromValue(defaultValue, bitRange);
        }

        [[nodiscard]] static SignalValueLookup::ptr createZeroedSignalValueLookup() {
            return std::make_shared<SignalValueLookup>(defaultSignalBitwidth, defaultSignalDimensions, defaultValue);
        }

        static void resetAllValuesTo(const SignalValueLookup::ptr& signalValueLookup, const unsigned int newDefaultValue) {
            std::vector<std::optional<unsigned int>> dimensionAccessToReset(defaultSignalDimensions.size(), 0);
            std::size_t                              dimensionIdx        = defaultSignalDimensions.size() - 1;
            const std::size_t                        lastDefinedSignalDimensionIdx = defaultSignalDimensions.size() - 1;

            bool continueResetting = true;
            while (continueResetting) {
                const unsigned int numValuesForPosition = defaultSignalDimensions.at(dimensionIdx);
                for (unsigned int value = 0; value < numValuesForPosition; ++value) {
                    dimensionAccessToReset[dimensionIdx] = value;
                    signalValueLookup->updateStoredValueFor(dimensionAccessToReset, std::nullopt, newDefaultValue);
                }

                dimensionAccessToReset.at(dimensionIdx) = 0;
                bool       backtracking                 = dimensionIdx > 0;
                const bool hadToBacktrack               = backtracking;
                bool       didBacktrackMoreThanOneLevel = false;
                while (backtracking) {
                    dimensionIdx--;
                    const unsigned valuesForPrevPosition      = defaultSignalDimensions.at(dimensionIdx);
                    const auto&    currentValueAtPrevPosition = *dimensionAccessToReset.at(dimensionIdx);
                    if (currentValueAtPrevPosition + 1 == valuesForPrevPosition) {
                        dimensionAccessToReset.at(dimensionIdx) = 0;
                        backtracking                            = dimensionIdx > 0;
                        didBacktrackMoreThanOneLevel            = true;
                    } else {
                        dimensionAccessToReset.at(dimensionIdx) = currentValueAtPrevPosition + 1;
                        // If we did not backtrack we process the current dimension further by simply undo or decrement of the dimension idx
                        if (!didBacktrackMoreThanOneLevel) {
                            dimensionIdx++;   
                        } else {
                            // If we backtracked at least one level back, we start processing again at the last possible signal dimension
                            dimensionIdx = lastDefinedSignalDimensionIdx;
                        }
                        backtracking = false;
                    }
                }

                continueResetting = !(hadToBacktrack && dimensionIdx == 0 && dimensionAccessToReset.front() == 0);
            }
        }

    protected:
        SignalValueLookup::ptr signalValueLookup;

        void SetUp() override {
            signalValueLookup = std::make_shared<SignalValueLookup>(defaultSignalBitwidth, defaultSignalDimensions, defaultValue);
        }

        void checkThatNoValueCanBeFetchedForGlobalBitRanges() const {
            std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess> globalBitRange = std::make_optional(optimizations::BitRangeAccessRestriction::BitRangeAccess(0, 0));

            for (unsigned int bitRangeStart = 0; bitRangeStart < defaultSignalBitwidth; ++bitRangeStart) {
                (*globalBitRange).first = bitRangeStart;
                (*globalBitRange).second = bitRangeStart;
                ASSERT_NO_FATAL_FAILURE(assertThatNoValueCanBeFetched({}, globalBitRange));

                for (unsigned int bitRangeEnd = bitRangeStart; bitRangeEnd < defaultSignalBitwidth; ++bitRangeEnd) {
                    (*globalBitRange).second = bitRangeEnd;
                    ASSERT_NO_FATAL_FAILURE(assertThatNoValueCanBeFetched({}, globalBitRange));
                }
            }
        }

        void checkThatNoValueCanBeFetchedForValuesOrBitRangesOfIntermediateDimensions() const {
            std::vector<std::optional<unsigned int>> accessedDimensions;
            ASSERT_NO_FATAL_FAILURE(assertThatNoValueCanBeFetched(accessedDimensions, std::nullopt));

            for (unsigned int dimension = 0; dimension < defaultSignalDimensions.size() - 1; ++dimension) {
                accessedDimensions.emplace_back(std::nullopt);
                ASSERT_NO_FATAL_FAILURE(assertThatNoValueCanBeFetched(accessedDimensions, std::nullopt));
                
                const auto valuesOfDimension = defaultSignalDimensions.at(dimension);
                for (unsigned int valueOfDimension = 0; valueOfDimension < valuesOfDimension; ++valueOfDimension) {
                    accessedDimensions.at(dimension) = valueOfDimension;
                    ASSERT_NO_FATAL_FAILURE(assertThatNoValueCanBeFetched(accessedDimensions, std::nullopt));

                    std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess> accessedBitRange = std::make_optional(optimizations::BitRangeAccessRestriction::BitRangeAccess(0, 0));
                    for (unsigned int bitRangeStart = 0; bitRangeStart < defaultSignalBitwidth; ++bitRangeStart) {
                        (*accessedBitRange).first = bitRangeStart;
                        (*accessedBitRange).second = bitRangeStart;
                        ASSERT_NO_FATAL_FAILURE(assertThatNoValueCanBeFetched(accessedDimensions, accessedBitRange));

                        for (unsigned int bitRangeEnd = bitRangeStart; bitRangeEnd < defaultSignalBitwidth; ++bitRangeEnd) {
                            (*accessedBitRange).second = bitRangeEnd;
                            ASSERT_NO_FATAL_FAILURE(assertThatNoValueCanBeFetched(accessedDimensions, accessedBitRange));
                        }
                    }
                }
            }
        }

        void checkThatValueMatchesIfItCanBeFetched(const ExpectedRestrictionStatusLookup& expectedRestrictionStatusLookup, const ExpectedValueLookup& expectedValueLookup) const {
            std::vector<std::optional<unsigned int>>                                accessedDimensions;
            std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess> accessedBitRange = std::make_optional(optimizations::BitRangeAccessRestriction::BitRangeAccess(0, 0));

            for (std::size_t dimension = 0; dimension < defaultSignalDimensions.size(); ++dimension) {
                if (accessedDimensions.size() < dimension + 1) {
                    accessedDimensions.emplace_back(std::nullopt);
                }
                const auto numValuesOfDimension = defaultSignalDimensions.at(dimension);
                for (unsigned int valueOfDimension = 0; valueOfDimension < numValuesOfDimension; ++valueOfDimension) {
                    accessedDimensions.at(dimension) = valueOfDimension;

                    ASSERT_NO_FATAL_FAILURE(assertThatValuesMatch(
                            accessedDimensions,
                            std::nullopt,
                            expectedRestrictionStatusLookup,
                            expectedValueLookup));

                    for (unsigned int bitRangeStart = 0; bitRangeStart < defaultSignalBitwidth; ++bitRangeStart) {
                        (*accessedBitRange).first  = bitRangeStart;
                        (*accessedBitRange).second = bitRangeStart;

                        ASSERT_NO_FATAL_FAILURE(assertThatValuesMatch(
                                accessedDimensions,
                                accessedBitRange,
                                expectedRestrictionStatusLookup,
                                expectedValueLookup));

                        for (unsigned int bitRangeEnd = bitRangeStart; bitRangeEnd < defaultSignalBitwidth; ++bitRangeEnd) {
                            ASSERT_NO_FATAL_FAILURE(assertThatValuesMatch(
                                    accessedDimensions,
                                    accessedBitRange,
                                    expectedRestrictionStatusLookup,
                                    expectedValueLookup));
                        }
                    }
                }
            }
        }

        void createAndCheckConditionForAllDimensionAndBitrangeCombinations(
                const std::vector<unsigned int>&       dimensions,
                const ExpectedRestrictionStatusLookup& expectedRestrictionStatusLookup,
                const ExpectedValueLookup&             expectedValueLookup) const {
            std::vector<std::optional<unsigned int>> combination(dimensions.size(), 0);
            std::size_t                              idx = dimensions.size() - 1;

            bool continueChecks = true;

            while (continueChecks) {
                const unsigned int numValuesForPosition = dimensions.at(idx);
                for (unsigned int value = 0; value <= numValuesForPosition; ++value) {
                    combination.at(idx) = value == numValuesForPosition ? std::nullopt : std::make_optional(value);
                    
                    ASSERT_NO_FATAL_FAILURE(assertThatValuesMatch(
                            combination,
                            std::nullopt,
                            expectedRestrictionStatusLookup,
                            expectedValueLookup));

                    std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess> accessedBitRange = std::make_optional(optimizations::BitRangeAccessRestriction::BitRangeAccess(0, 0));
                    for (unsigned int bitRangeStart = 0; bitRangeStart < defaultSignalBitwidth; ++bitRangeStart) {
                        (*accessedBitRange).first  = bitRangeStart;
                        (*accessedBitRange).second = bitRangeStart;

                        ASSERT_NO_FATAL_FAILURE(assertThatValuesMatch(
                                combination,
                                accessedBitRange,
                                expectedRestrictionStatusLookup,
                                expectedValueLookup));

                        for (unsigned int bitRangeEnd = bitRangeStart; bitRangeEnd < defaultSignalBitwidth; ++bitRangeEnd) {
                            (*accessedBitRange).second = bitRangeEnd;
                            ASSERT_NO_FATAL_FAILURE(assertThatValuesMatch(
                                    combination,
                                    accessedBitRange,
                                    expectedRestrictionStatusLookup,
                                    expectedValueLookup));
                        }
                    }
                }

                combination.at(idx)       = 0;
                bool       backtracking   = idx > 0;
                const bool hadToBacktrack = backtracking;
                while (backtracking) {
                    idx--;
                    const unsigned valuesForPrevPosition      = dimensions.at(idx);
                    const auto&    currentValueAtPrevPosition = *combination.at(idx);
                    if (currentValueAtPrevPosition + 1 > valuesForPrevPosition) {
                        combination.at(idx) = 0;
                        backtracking        = idx > 0;
                    } else {
                        combination.at(idx) = currentValueAtPrevPosition + 1;
                        idx++;
                        backtracking = false;
                    }
                }
                continueChecks = hadToBacktrack && idx == 0 && combination.at(0) == 0;
            }
        }

        void assertThatValuesMatch(
            const std::vector<std::optional<unsigned int>>& accessedDimensions, 
            const std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess>& accessedBitRange,
            const ExpectedRestrictionStatusLookup& expectedRestrictionStatusLookup,
            const ExpectedValueLookup& expectedValueLookup) const {

            const auto actuallyFetchedValue = signalValueLookup->tryFetchValueFor(accessedDimensions, accessedBitRange);
            if (!expectedRestrictionStatusLookup(accessedDimensions, accessedBitRange)) {
                const auto expectedValue        = expectedValueLookup(accessedDimensions, accessedBitRange);
                ASSERT_EQ(expectedValue, actuallyFetchedValue)
                    << "Access on: " + stringifyAccess(accessedDimensions, accessedBitRange)
                        + "\n Expected value: " + stringifyValue(expectedValue)
                        + "\n Actual value: " + stringifyValue(actuallyFetchedValue);

            } else if (actuallyFetchedValue.has_value()) {
                ASSERT_TRUE(false) << 
                    "Access on: " + stringifyAccess(accessedDimensions, accessedBitRange)
                    + ", expected not being able to fetch a value but was actually " + stringifyValue(actuallyFetchedValue);
            }
        }

        void assertThatNoValueCanBeFetched(
            const std::vector<std::optional<unsigned int>>&                                accessedDimensions,
            const std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess>& accessedBitRange) const
        {
            const auto& actuallyFetchedValue = signalValueLookup->tryFetchValueFor(accessedDimensions, accessedBitRange);
            ASSERT_FALSE(actuallyFetchedValue.has_value()) << 
                "Access @ " + stringifyAccess(accessedDimensions, accessedBitRange)
                + "\nExpected not being able to fetch a value but was actually " + stringifyValue(actuallyFetchedValue);    
        }

        [[nodiscard]] std::string stringifyAccess(const std::vector<std::optional<unsigned int>> & accessedDimensions, const std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess>& accessedBitRange) const {
            std::string              transformedSignalAccess;
            for (const auto& accessedValueOfDimension : accessedDimensions) {
                transformedSignalAccess += "[" + (accessedValueOfDimension.has_value() ? std::to_string(*accessedValueOfDimension) : "$i") + "]";
            }
            if (accessedBitRange.has_value()) {
                if ((*accessedBitRange).first == (*accessedBitRange).second) {
                    transformedSignalAccess += "." + std::to_string((*accessedBitRange).first);
                } else {
                    transformedSignalAccess += "." + std::to_string((*accessedBitRange).first) + ":" + std::to_string((*accessedBitRange).second);
                }
            }
            return transformedSignalAccess;
        }

        [[nodiscard]] std::string stringifyValue(const std::optional<unsigned int>& optionalValue) const {
            return optionalValue.has_value() ? std::to_string(*optionalValue) : "<empty>";
        }
    };

    //TEST_P(SignalValueLookupTest, CheckRestrictionAndFetchedValueOfSignalValueLookupTests) {
    //    const auto& testParamInfo               = GetParam();
    //    const auto& initialRestrictionGenerator = std::get<1>(testParamInfo);
    //    initialRestrictionGenerator(signalValueLookup);

    //    //createCombinations({2, 4});

    //    const auto& expectedRestrictionStatusLookup = std::get<2>(testParamInfo);
    //    const auto& expectedValueLookup = std::get<3>(testParamInfo);

    //    ASSERT_NO_FATAL_FAILURE(checkThatNoValueCanBeFetchedForGlobalBitRanges());
    //    ASSERT_NO_FATAL_FAILURE(checkThatNoValueCanBeFetchedForValuesOrBitRangesOfIntermediateDimensions());

    //    ASSERT_NO_FATAL_FAILURE(
    //        createAndCheckConditionForAllDimensionAndBitrangeCombinations(
    //            SignalValueLookupTest::defaultSignalDimensions,
    //            expectedRestrictionStatusLookup,
    //            expectedValueLookup)
    //    );

    //    //ASSERT_NO_FATAL_FAILURE(checkThatValueMatchesIfItCanBeFetched(expectedRestrictionStatusLookup, expectedValueLookup));
    //}
}; // namespace valueLookup
#endif