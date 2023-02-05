#include "core/syrec/variable.hpp"
#include "core/syrec/parser/signal_access_restriction.hpp"

#include "gtest/gtest.h"

using namespace parser;

class SignalAccessRestrictionTest: public ::testing::Test {
protected:
    const std::vector<unsigned int> referenceVariableDimensions = {2, 4, 3};
    const static std::size_t              expectedBitWidth            = 8;

    const std::size_t restrictedBitPosition = 2;

    syrec::Variable::ptr referenceVariable;
    std::shared_ptr<SignalAccessRestriction> restriction;

    void SetUp() override {
        referenceVariable = std::make_shared<syrec::Variable>(syrec::Variable(0, "variable", referenceVariableDimensions, expectedBitWidth));
        restriction       = std::make_shared<SignalAccessRestriction>(referenceVariable);
    }

    [[nodiscard]] SignalAccessRestriction::SignalAccess createBitRangeBeforeRestriction() const {
        return SignalAccessRestriction::SignalAccess(0, restrictedBitPosition - 1);
    }

    [[nodiscard]] SignalAccessRestriction::SignalAccess createBitRangeAfterRestriction() const {
        return SignalAccessRestriction::SignalAccess(restrictedBitPosition + 1, expectedBitWidth);
    }

    [[nodiscard]] static SignalAccessRestriction::SignalAccess createBitRangeContainingRestriction() {
        return SignalAccessRestriction::SignalAccess(0, expectedBitWidth - 1);
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
        const auto& actualRestrictionStatus = restriction->isAccessRestrictedToBitRange(dimension, bitRange);
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
            std::string stringifiedPosition = (dimension.has_value() ? " Dimension: " + std::to_string(dimension.value()) : "")
            + (valueOfDimension.has_value() ? " Value for dimension: " + std::to_string(valueOfDimension.value()) : "");

            if (bitRangeStart.has_value() && bitRangeEnd.has_value()) {
                stringifiedPosition += " Bit range (from/to): " + std::to_string(bitRangeStart.value()) + "/" + std::to_string(bitRangeEnd.value()); 
            } else if (bitRangeStart.has_value()) {
                stringifiedPosition += " Bit position: " + std::to_string(bitRangeStart.value()); 
            }

            return stringifiedPosition;
        }
    };

    void validateRestrictedBitPosition() const {
        EXPECT_GT(restrictedBitPosition, 0);
        EXPECT_LT(restrictedBitPosition, expectedBitWidth);    
    }

    static std::vector<std::size_t> createIndexSequence(const std::size_t start, const std::size_t stop) {
        std::vector<std::size_t> container(stop - start);
        std::size_t              value = start;
        for (auto& it: container) {
            it = value++;
        }
        return container;
    }

    template<class F>
    void assertForEachInSequence(const std::size_t start, const std::size_t stop, F&& f) {
        const std::vector<std::size_t>& indexSequence = createIndexSequence(start, stop);
        std::for_each(indexSequence.cbegin(), indexSequence.cend(), f);
    }

    static constexpr bool isBitWithinRange(const std::size_t bitToCheck, const std::size_t bitRangeStart, const std::size_t bitRangeEnd) {
        return bitToCheck >= bitRangeStart && bitToCheck <= bitRangeEnd;
    }

    template<class F>
    void checkRestrictionsStatusForAllBitsGlobally(F&& restrictionStatusDeductor) {
        assertForEachInSequence(0, expectedBitWidth, [&](const std::size_t& bit) {
            assertThatRestrictionStatusMatchesForBit(bit, restrictionStatusDeductor(bit));   
        });
    }

    template<class F>
    void checkRestrictionsStatusForAllBitsOfAllDimensions(F&& restrictionStatusDeductor) {
        assertForEachInSequence(0, referenceVariableDimensions.size(), [&](const std::size_t& dimension) {
            assertForEachInSequence(0, referenceVariableDimensions.at(dimension), [&](const std::size_t& valueOfDimension) {
                assertForEachInSequence(0, expectedBitWidth, [&](const std::size_t& bit) {
                    assertThatRestrictionStatusMatchesForBitOfValueOfDimension(dimension, valueOfDimension, bit, restrictionStatusDeductor(bit, dimension, valueOfDimension));
                });
            });
        });
    }

    template<class F>
    void checkRestrictionsStatusForAllBitRangesGlobally(F&& restrictionStatusDeductor) {
        assertForEachInSequence(0, expectedBitWidth, [&](const std::size_t& bitRangeStart) {
            assertForEachInSequence(bitRangeStart, expectedBitWidth, [&](const std::size_t& bitRangeEnd) {
                const auto bitRange = SignalAccessRestriction::SignalAccess(bitRangeStart, bitRangeEnd);
                assertThatRestrictionStatusMatchesForBitRange(bitRange, restrictionStatusDeductor(bitRange));
            });
        });
    }
    
    template<class F>
    void checkRestrictionsStatusForAllBitRangesPerDimension(F&& restrictionStatusDeductor) {
        assertForEachInSequence(0, referenceVariableDimensions.size(), [&](const std::size_t& dimension) {
            assertForEachInSequence(0, referenceVariableDimensions.at(dimension), [&](const std::size_t& valueOfDimension) {
                assertForEachInSequence(0, expectedBitWidth, [&](const std::size_t& bitRangeStart) {
                    assertForEachInSequence(bitRangeStart, expectedBitWidth, [&](const std::size_t& bitRangeEnd) {
                        const auto bitRange = SignalAccessRestriction::SignalAccess(bitRangeStart, bitRangeEnd);
                        assertThatRestrictionStatusMatchesForBitRangeOfValueOfDimension(dimension, valueOfDimension, bitRange, restrictionStatusDeductor(bitRange, dimension, valueOfDimension));
                    });
                });
            });
        });
    }


    template<class F>
    void checkRestrictionsStatusForDimensions(F&& restrictionStatusDeductor) {
        assertForEachInSequence(0, referenceVariableDimensions.size(), [&](const std::size_t& dimension) {
            assertThatRestrictionStatusMatchesForDimension(dimension, restrictionStatusDeductor(dimension));
        });
    }

    template<class F>
    void checkRestrictionsStatusForAllValuesPerDimensions(F&& restrictionStatusDeductor) {
        assertForEachInSequence(0, referenceVariableDimensions.size(), [&](const std::size_t& dimension) {
            assertForEachInSequence(0, referenceVariableDimensions.at(dimension), [&](const std::size_t& valueOfDimension) {
                assertThatRestrictionStatusMatchesForValueOfDimension(dimension, valueOfDimension, restrictionStatusDeductor(dimension, valueOfDimension));
            });
        });
    }
};

/*
 * Tests for correct signal access restriction
 *
 */

/*
 * Scenario 0: a += ...
 */
TEST_F(SignalAccessRestrictionTest, restrictAccessOnWholeSignalAccessOnSignalNotOk) {
    restriction->blockAccessOnSignalCompletely();
    assertThatRestrictionStatusMatchesForWholeSignal(true);
}

TEST_F(SignalAccessRestrictionTest, restrictAccessOnWholeSignalCheckAccessOnBits) {
    restriction->blockAccessOnSignalCompletely();
    checkRestrictionsStatusForAllBitsGlobally([](const std::size_t&) { return true; });
}

TEST_F(SignalAccessRestrictionTest, restrictAccessOnWholeSignalCheckAccessOnBitRanges) {
    restriction->blockAccessOnSignalCompletely();

    checkRestrictionsStatusForAllBitRangesGlobally([](const auto&) { return true; });
}

TEST_F(SignalAccessRestrictionTest, restrictAccessOnWholeSignalCheckAccessOnDimensions) {
    restriction->blockAccessOnSignalCompletely();
    checkRestrictionsStatusForDimensions([](const auto&) { return true; });
}

TEST_F(SignalAccessRestrictionTest, restrictAccessOnWholeSignalCheckAccessOnValuesOfDimensions) {
    restriction->blockAccessOnSignalCompletely();

    checkRestrictionsStatusForAllValuesPerDimensions([](const auto&, const auto&) { return true; });
}

TEST_F(SignalAccessRestrictionTest, restrictAccessOnWholeSignalCheckAccessOnBitsOfAnyDimension) {
    restriction->restrictAccessToBit(restrictedBitPosition);

    checkRestrictionsStatusForAllBitsOfAllDimensions([](const auto&, const auto&, const auto&) { return true; });
}

TEST_F(SignalAccessRestrictionTest, restrictAccessOnWholeSignalCheckAccessOnBitRangesOfAnyDimension) {
    restriction->restrictAccessToBit(restrictedBitPosition);
    validateRestrictedBitPosition();

    checkRestrictionsStatusForAllBitRangesPerDimension([](const auto&, const auto&, const auto&) { return true; });
}

/*
 * Scenario 1:  a.0 += ...
 */
TEST_F(SignalAccessRestrictionTest, restrictAccessOnBitOfSignalGloballyCheckAccessOnBits) {
    restriction->restrictAccessToBit(restrictedBitPosition);

    checkRestrictionsStatusForAllBitsGlobally([&](const auto& bit) { return bit == restrictedBitPosition; });
}

TEST_F(SignalAccessRestrictionTest, restrictAccessOnBitOfSignalGloballyCheckAccessOnBitRanges) {
    restriction->restrictAccessToBit(restrictedBitPosition);
    validateRestrictedBitPosition();

    checkRestrictionsStatusForAllBitRangesGlobally([&](const SignalAccessRestriction::SignalAccess& bitRange) { return isBitWithinRange(restrictedBitPosition, bitRange.start, bitRange.stop); });
}

TEST_F(SignalAccessRestrictionTest, restrictAccessOnBitOfSignalGloballyCheckAccessOnDimensions) {
    restriction->restrictAccessToBit(restrictedBitPosition);

    checkRestrictionsStatusForDimensions([](const auto&) { return true; });
}

TEST_F(SignalAccessRestrictionTest, restrictAccessOnBitOfSignalGloballyCheckAccessOnValuesOfDimensions) {
    restriction->restrictAccessToBit(restrictedBitPosition);

    checkRestrictionsStatusForAllValuesPerDimensions([](const auto&, const auto&) { return true; });
}

TEST_F(SignalAccessRestrictionTest, restrictAccessOnBitOfSignalGloballyCheckAccessOnBitsOfAnyDimension) {
    restriction->restrictAccessToBit(restrictedBitPosition);

    checkRestrictionsStatusForAllBitsOfAllDimensions([&](const auto& bit, const auto&, const auto&) { return bit == restrictedBitPosition; });
}

TEST_F(SignalAccessRestrictionTest, restrictAccessOnBitOfSignalGloballyCheckAccessOnBitRangesOfAnyDimension) {
    restriction->restrictAccessToBit(restrictedBitPosition);
    validateRestrictedBitPosition();

    checkRestrictionsStatusForAllBitRangesPerDimension([&](const SignalAccessRestriction::SignalAccess& bitRange, const auto&, const auto&) { return isBitWithinRange(restrictedBitPosition, bitRange.start, bitRange.stop); });
}

/*
 * Scenario 2: a.0:5 += ...
 */

/*
 * Scenario 3: a[0] += ...
 */

/*
 * Scenario 4: a[2].0 += ...
 */

/*
 * Scenario 5: a[2].0:10 += ...
 */

/*
 * Scenario 6: a[0][2] += ...
 */

/*
 * Scenario 7: a[0][2].0 += ...
 */

/*
 * Scenario 8: a[0][2].0:5 += ...
 */