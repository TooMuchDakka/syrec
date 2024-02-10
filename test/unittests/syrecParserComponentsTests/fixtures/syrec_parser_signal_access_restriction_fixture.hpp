#ifndef SYREC_PARSER_SIGNAL_ACCESS_RESTRICTION_FIXTURE_HPP
#define SYREC_PARSER_SIGNAL_ACCESS_RESTRICTION_FIXTURE_HPP
#pragma once

#include "core/syrec/variable.hpp"
#include "core/syrec/parser/signal_access_restriction.hpp"

#include "gtest/gtest.h"

namespace parser {
    using RestrictionPtr                                         = std::shared_ptr<SignalAccessRestriction>;
    using RestrictionDefinition                                  = std::function<void(RestrictionPtr&)>;
    using ExpectedBitRestrictionStatusLookup                     = std::function<bool(const std::size_t& bit)>;
    using ExpectedBitOfDimensionRestrictionStatusLookup          = std::function<bool(const std::size_t& bit, const std::size_t& dimension, const std::size_t& valueForDimension)>;
    using ExpectedDimensionRestrictionStatusLookup               = std::function<bool(const std::size_t& dimension)>;
    using ExpectedValueForDimensionRestrictionStatusLookup       = std::function<bool(const std::size_t& dimension, const std::size_t& valueForDimension)>;
    using ExpectedGlobalBitRangesOfSignalRestrictionStatusLookup = std::function<bool(const SignalAccessRestriction::SignalAccess& bitRange)>;
    using ExpectedBitRangeOfSignalRestrictionStatusLookup        = std::function<bool(const SignalAccessRestriction::SignalAccess& bitRange, const std::size_t& dimension, const std::size_t& valueForDimension)>;

    class SignalAccessRestrictionTest:
        public ::testing::Test,
        public ::testing::WithParamInterface<
                std::tuple<std::string,
                           RestrictionDefinition,
                           RestrictionDefinition,
                           ExpectedGlobalBitRangesOfSignalRestrictionStatusLookup,
                           ExpectedDimensionRestrictionStatusLookup,
                           ExpectedValueForDimensionRestrictionStatusLookup,
                           ExpectedBitRangeOfSignalRestrictionStatusLookup>> {
    public:
        constexpr static std::size_t                  bitWidthOfReferenceSignal = 8;
        const static inline std::vector<unsigned int> dimensionsOfReferenceSignal{2, 4, 3};

        constexpr static std::size_t restrictedBit                 = 2;
        constexpr static std::size_t blockedDimension              = 1;
        constexpr static std::size_t blockedValueForDimension      = 3;
        constexpr static std::size_t otherBlockedValueForDimension = 0;

        constexpr static std::size_t otherBlockedDimension         = 0;
        constexpr static std::size_t blockedValueForOtherDimension = 1;

        const static inline SignalAccessRestriction::SignalAccess bitRangeBeforeRestrictedBit = SignalAccessRestriction::SignalAccess(0, restrictedBit > 1 ? restrictedBit - 1 : 0);
        const static inline SignalAccessRestriction::SignalAccess restrictedBitRange          = SignalAccessRestriction::SignalAccess(restrictedBit, restrictedBit + 2);
        const static inline SignalAccessRestriction::SignalAccess bitRangeAfterGap            = SignalAccessRestriction::SignalAccess(restrictedBit + 3, bitWidthOfReferenceSignal - 1);

        static constexpr bool isBitWithinRange(const std::size_t bitToCheck, const SignalAccessRestriction::SignalAccess& bitRange) {
            return bitToCheck >= bitRange.start && bitToCheck <= bitRange.stop;
        }

        static constexpr bool doesAccessIntersectRegion(const SignalAccessRestriction::SignalAccess& referenceRegion, const SignalAccessRestriction::SignalAccess& signalAccess) {
            return !((signalAccess.start < referenceRegion.start && signalAccess.stop < referenceRegion.start) || signalAccess.start > referenceRegion.stop);
        }

    protected:
        RestrictionPtr restriction;

        void SetUp() override {
            const auto& referenceVariable = std::make_shared<syrec::Variable>(syrec::Variable(0, "variable", dimensionsOfReferenceSignal, bitWidthOfReferenceSignal));
            restriction                   = std::make_shared<SignalAccessRestriction>(referenceVariable);
        }

        void assertThatRestrictionStatusMatchesForWholeSignal(const bool shouldAccessBeCompletelyRestricted) const {
            const auto& actualRestrictionStatus = restriction->isAccessRestrictedOnWholeSignal();
            ASSERT_EQ(shouldAccessBeCompletelyRestricted, actualRestrictionStatus)
                    << printExpectedAndActualRestrictionStatus(shouldAccessBeCompletelyRestricted, actualRestrictionStatus);
        }

        void assertThatRestrictionStatusMatchesForBit(const std::size_t bitPosition, const bool shouldAccessBeRestricted) const {
            const auto& actualRestrictionStatus = restriction->isAccessRestrictedToBit(bitPosition);
            compareRestrictionStatus(shouldAccessBeRestricted, actualRestrictionStatus, printBitSignalPosition(bitPosition));
        }

        void assertThatRestrictionStatusMatchesForBitOfDimension(const std::size_t dimension, const std::size_t bitPosition, const bool shouldAccessBeRestricted) const {
            const auto& actualRestrictionStatus = restriction->isAccessRestrictedToBit(dimension, bitPosition);
            compareRestrictionStatus(shouldAccessBeRestricted, actualRestrictionStatus, printBitOfDimensionSignalPosition(dimension, bitPosition));
        }

        void assertThatRestrictionStatusMatchesForValueOfDimension(const std::size_t dimension, const std::size_t valueForDimension, const bool shouldAccessBeRestricted) const {
            const auto& actualRestrictionStatus = restriction->isAccessRestrictedToValueOfDimension(dimension, valueForDimension);
            compareRestrictionStatus(shouldAccessBeRestricted, actualRestrictionStatus, printValueOfDimensionSignalPosition(dimension, valueForDimension));
        }

        void assertThatRestrictionStatusMatchesForBitOfValueOfDimension(const std::size_t dimension, const std::size_t valueForDimension, const std::size_t bitPosition, const bool shouldAccessBeRestricted) const {
            const auto& actualRestrictionStatus = restriction->isAccessRestrictedToBit(dimension, valueForDimension, bitPosition);
            compareRestrictionStatus(shouldAccessBeRestricted, actualRestrictionStatus, printBitOfValueOfDimensionSignalPosition(dimension, valueForDimension, bitPosition));
        }

        void assertThatRestrictionStatusMatchesForBitRange(const SignalAccessRestriction::SignalAccess& bitRange, const bool shouldAccessBeRestricted) const {
            const auto& actualRestrictionStatus = restriction->isAccessRestrictedToBitRange(bitRange);
            compareRestrictionStatus(shouldAccessBeRestricted, actualRestrictionStatus, printBitRangeSignalPosition(bitRange));
        }

        void assertThatRestrictionStatusMatchesForBitRangeOfDimension(const std::size_t dimension, const SignalAccessRestriction::SignalAccess& bitRange, const bool shouldAccessBeRestricted) const {
            const auto& actualRestrictionStatus = restriction->isAccessRestrictedToBitRange(dimension, bitRange);
            compareRestrictionStatus(shouldAccessBeRestricted, actualRestrictionStatus, printBitRangeOfDimensionSignalPosition(dimension, bitRange));
        }

        void assertThatRestrictionStatusMatchesForBitRangeOfValueOfDimension(const std::size_t dimension, const std::size_t valueForDimension, const SignalAccessRestriction::SignalAccess& bitRange, const bool shouldAccessBeRestricted) const {
            const auto& actualRestrictionStatus = restriction->isAccessRestrictedToBitRange(dimension, valueForDimension, bitRange);
            compareRestrictionStatus(shouldAccessBeRestricted, actualRestrictionStatus, printBitRangeOfValueOfDimensionSignalPosition(dimension, valueForDimension, bitRange));
        }

        void assertThatRestrictionStatusMatchesForDimension(const std::size_t dimension, const bool shouldAccessBeRestricted) const {
            const auto& actualRestrictionStatus = restriction->isAccessRestrictedToDimension(dimension);
            compareRestrictionStatus(shouldAccessBeRestricted, actualRestrictionStatus, printDimensionSignalPosition(dimension));
        }

        static void compareRestrictionStatus(const bool expectedRestrictionStatus, const bool actualRestrictionStatus, const std::string_view& positionStringified) {
            ASSERT_EQ(expectedRestrictionStatus, actualRestrictionStatus)
                    << positionStringified << "\n"
                    << printExpectedAndActualRestrictionStatus(expectedRestrictionStatus, actualRestrictionStatus);
        }

        static std::string printExpectedAndActualRestrictionStatus(const bool expectedRestrictionStatus, const bool actualRestrictionStatus) {
            return "Restriction status (expected|actual):" + stringifyBoolean(expectedRestrictionStatus) + "|" + stringifyBoolean(actualRestrictionStatus);
        }

        static std::string stringifyBoolean(const bool booleanValue) {
            return booleanValue ? "true" : "false";
        }

        static std::string printBitSignalPosition(const std::size_t bitPosition) {
            auto position = SignalPosition();
            position.bitRangeStart.emplace(bitPosition);
            return position.stringify();
        }

        static std::string printBitOfDimensionSignalPosition(const std::size_t dimension, const std::size_t bitPosition) {
            auto position = SignalPosition();
            position.dimension.emplace(dimension);
            position.bitRangeStart.emplace(bitPosition);
            return position.stringify();
        }

        static std::string printBitOfValueOfDimensionSignalPosition(const std::size_t dimension, const std::size_t valueOfDimension, const std::size_t bitPosition) {
            auto position = SignalPosition();
            position.dimension.emplace(dimension);
            position.valueOfDimension.emplace(valueOfDimension);
            position.bitRangeStart.emplace(bitPosition);
            return position.stringify();
        }

        static std::string printBitRangeSignalPosition(const SignalAccessRestriction::SignalAccess& bitRange) {
            auto position = SignalPosition();
            position.bitRangeStart.emplace(bitRange.start);
            position.bitRangeEnd.emplace(bitRange.stop);
            return position.stringify();
        }

        static std::string printBitRangeOfDimensionSignalPosition(const std::size_t dimension, const SignalAccessRestriction::SignalAccess& bitRange) {
            auto position = SignalPosition();
            position.dimension.emplace(dimension);
            position.bitRangeStart.emplace(bitRange.start);
            position.bitRangeEnd.emplace(bitRange.stop);
            return position.stringify();
        }

        static std::string printBitRangeOfValueOfDimensionSignalPosition(const std::size_t dimension, const std::size_t valueOfDimension, const SignalAccessRestriction::SignalAccess& bitRange) {
            auto position = SignalPosition();
            position.dimension.emplace(dimension);
            position.valueOfDimension.emplace(valueOfDimension);
            position.bitRangeStart.emplace(bitRange.start);
            position.bitRangeEnd.emplace(bitRange.stop);
            return position.stringify();
        }

        static std::string printDimensionSignalPosition(std::size_t dimension) {
            auto position = SignalPosition();
            position.dimension.emplace(dimension);
            return position.stringify();
        }

        static std::string printValueOfDimensionSignalPosition(std::size_t dimension, std::size_t valueOfDimension) {
            auto position = SignalPosition();
            position.dimension.emplace(dimension);
            position.valueOfDimension.emplace(valueOfDimension);
            return position.stringify();
        }

        struct SignalPosition {
            std::optional<std::size_t> bitRangeStart;
            std::optional<std::size_t> bitRangeEnd;
            std::optional<std::size_t> dimension;
            std::optional<std::size_t> valueOfDimension;

            [[nodiscard]] std::string stringify() const {
                std::string stringifiedPosition = (dimension.has_value() ? " Dimension: " + std::to_string(dimension.value()) : "") + (valueOfDimension.has_value() ? " Value for dimension: " + std::to_string(valueOfDimension.value()) : "");

                if (bitRangeStart.has_value() && bitRangeEnd.has_value()) {
                    stringifiedPosition += " Bit range (from/to): " + std::to_string(bitRangeStart.value()) + "/" + std::to_string(bitRangeEnd.value());
                } else if (bitRangeStart.has_value()) {
                    stringifiedPosition += " Bit position: " + std::to_string(bitRangeStart.value());
                }

                return stringifiedPosition;
            }
        };

        static void validateRestrictedBitPosition() {
            EXPECT_GT(restrictedBit, 0);
            EXPECT_LT(restrictedBit, bitWidthOfReferenceSignal);
        }

        static std::vector<std::size_t> createIndexSequenceExcludingEnd(const std::size_t start, const std::size_t stop) {
            if (start >= stop)
                return {};

            std::vector<std::size_t> container(((stop - 1) - start) + 1);
            std::size_t              value = start;
            for (auto& it: container) {
                it = value++;
            }
            return container;
        }

        template<class F>
        void assertForEachInSequence(const std::size_t start, const std::size_t stop, F&& f) {
            const std::vector<std::size_t>& indexSequence = createIndexSequenceExcludingEnd(start, stop);
            std::for_each(indexSequence.cbegin(), indexSequence.cend(), f);
        }

        void checkRestrictionsStatusForAllBitsGlobally(const ExpectedBitRestrictionStatusLookup& restrictionStatusDeductor) {
            assertForEachInSequence(0, bitWidthOfReferenceSignal, [&](const std::size_t& bit) {
                assertThatRestrictionStatusMatchesForBit(bit, restrictionStatusDeductor(bit));
            });
        }

        void checkRestrictionsStatusForAllBitsOfAllDimensions(const ExpectedBitOfDimensionRestrictionStatusLookup& restrictionStatusDeductor) {
            assertForEachInSequence(0, dimensionsOfReferenceSignal.size(), [&](const std::size_t& dimension) {
                assertForEachInSequence(0, dimensionsOfReferenceSignal.at(dimension), [&](const std::size_t& valueOfDimension) {
                    assertForEachInSequence(0, bitWidthOfReferenceSignal, [&](const std::size_t& bit) {
                        assertThatRestrictionStatusMatchesForBitOfValueOfDimension(dimension, valueOfDimension, bit, restrictionStatusDeductor(bit, dimension, valueOfDimension));
                    });
                });
            });
        }

        void checkRestrictionsStatusForAllBitRangesGlobally(const ExpectedGlobalBitRangesOfSignalRestrictionStatusLookup& restrictionStatusDeductor) {
            assertForEachInSequence(0, bitWidthOfReferenceSignal, [&](const std::size_t& bitRangeStart) {
                assertForEachInSequence(bitRangeStart, bitWidthOfReferenceSignal, [&](const std::size_t& bitRangeEnd) {
                    const auto bitRange = SignalAccessRestriction::SignalAccess(bitRangeStart, bitRangeEnd);
                    assertThatRestrictionStatusMatchesForBitRange(bitRange, restrictionStatusDeductor(bitRange));
                });
            });
        }

        void checkRestrictionsStatusForAllBitRangesPerDimension(const ExpectedBitRangeOfSignalRestrictionStatusLookup& restrictionStatusDeductor) {
            assertForEachInSequence(0, dimensionsOfReferenceSignal.size(), [&](const std::size_t& dimension) {
                assertForEachInSequence(0, dimensionsOfReferenceSignal.at(dimension), [&](const std::size_t& valueOfDimension) {
                    assertForEachInSequence(0, bitWidthOfReferenceSignal, [&](const std::size_t& bitRangeStart) {
                        assertForEachInSequence(bitRangeStart, bitWidthOfReferenceSignal, [&](const std::size_t& bitRangeEnd) {
                            const auto bitRange = SignalAccessRestriction::SignalAccess(bitRangeStart, bitRangeEnd);
                            assertThatRestrictionStatusMatchesForBitRangeOfValueOfDimension(dimension, valueOfDimension, bitRange, restrictionStatusDeductor(bitRange, dimension, valueOfDimension));
                        });
                    });
                });
            });
        }

        void checkRestrictionsStatusForDimensions(const ExpectedDimensionRestrictionStatusLookup& restrictionStatusDeductor) {
            assertForEachInSequence(0, dimensionsOfReferenceSignal.size(), [&](const std::size_t& dimension) {
                assertThatRestrictionStatusMatchesForDimension(dimension, restrictionStatusDeductor(dimension));
            });
        }

        void checkRestrictionsStatusForAllValuesPerDimensions(const ExpectedValueForDimensionRestrictionStatusLookup& restrictionStatusDeductor) {
            assertForEachInSequence(0, dimensionsOfReferenceSignal.size(), [&](const std::size_t& dimension) {
                assertForEachInSequence(0, dimensionsOfReferenceSignal.at(dimension), [&](const std::size_t& valueOfDimension) {
                    assertThatRestrictionStatusMatchesForValueOfDimension(dimension, valueOfDimension, restrictionStatusDeductor(dimension, valueOfDimension));
                });
            });
        }
    };

    TEST_P(SignalAccessRestrictionTest, CheckThatRestrictionChanged) {
        const auto& testParamInfo                = GetParam();
        const auto& initialRestrictionDefinition = std::get<1>(testParamInfo);
        initialRestrictionDefinition(restriction);

        const auto& additionalRestrictions = std::get<2>(testParamInfo);
        additionalRestrictions(restriction);

        const ExpectedGlobalBitRangesOfSignalRestrictionStatusLookup& expectedSignalBitRangeRestrictionStatusLookup              = std::get<3>(testParamInfo);
        const ExpectedDimensionRestrictionStatusLookup&               expectedDimensionRestrictionStatusLookup                   = std::get<4>(testParamInfo);
        const ExpectedValueForDimensionRestrictionStatusLookup&       expectedValueForDimensionRestrictionStatusLookup           = std::get<5>(testParamInfo);
        const ExpectedBitRangeOfSignalRestrictionStatusLookup&        expectedBitRangeOfValueForDimensionRestrictionStatusLookup = std::get<6>(testParamInfo);

        checkRestrictionsStatusForAllBitRangesGlobally(expectedSignalBitRangeRestrictionStatusLookup);
        checkRestrictionsStatusForDimensions(expectedDimensionRestrictionStatusLookup);
        checkRestrictionsStatusForAllValuesPerDimensions(expectedValueForDimensionRestrictionStatusLookup);
        checkRestrictionsStatusForAllBitRangesPerDimension(expectedBitRangeOfValueForDimensionRestrictionStatusLookup);
    }
}
#endif