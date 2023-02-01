#include "core/syrec/variable.hpp"
#include "core/syrec/parser/signal_access_restriction.hpp"

#include "gtest/gtest.h"

using namespace parser;

class SignalAccessRestrictionTest: public ::testing::Test {
protected:
    const std::vector<unsigned int> referenceVariableDimensions = {2, 4, 5};
    const static std::size_t              expectedBitWidth            = 32;

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

    [[nodiscard]] SignalAccessRestriction::SignalAccess createBitRangeContainingRestriction() const {
        return SignalAccessRestriction::SignalAccess(0, expectedBitWidth - 1);
    }

    void assertThatRestrictionStatusMatch(std::size_t bitPosition, bool shouldAccessBeRestricted) const {
        const auto& actualRestrictionStatus = restriction->isAccessRestrictedToBit(bitPosition);
        ASSERT_EQ(shouldAccessBeRestricted, actualRestrictionStatus)
            << "Restriction status for bit " << bitPosition
            << " | Expected: " << shouldAccessBeRestricted << " Actual: " << actualRestrictionStatus;
    }

    void assertThatRestrictionStatusMatch(std::size_t dimension, std::size_t bitPosition, bool shouldAccessBeRestricted) const {
        const auto& actualRestrictionStatus = restriction->isAccessRestrictedToBit(dimension, bitPosition);
        ASSERT_EQ(shouldAccessBeRestricted, actualRestrictionStatus)
            << "Restriction status for bit " << bitPosition << " of dimension " << dimension
            << "| Expected: " << shouldAccessBeRestricted << " Actual: " << actualRestrictionStatus;
    }

    void assertThatRestrictionStatusMatch(std::size_t dimension, std::size_t valueForDimension, std::size_t bitPosition, bool shouldAccessBeRestricted) const {
        const auto& actualRestrictionStatus = restriction->isAccessRestrictedToBit(dimension, bitPosition);
        ASSERT_EQ(shouldAccessBeRestricted, actualRestrictionStatus)
                << "Restriction status for bit " << bitPosition << " of value " << valueForDimension << " of dimension " << dimension
                << "| Expected: " << shouldAccessBeRestricted << " Actual: " << actualRestrictionStatus;
    }

    void assertThatRestrictionStatusMatch(const SignalAccessRestriction::SignalAccess& bitRange, bool shouldAccessBeRestricted) const {
        const auto& actualRestrictionStatus = restriction->isAccessRestrictedToBitRange(bitRange);
        ASSERT_EQ(shouldAccessBeRestricted, actualRestrictionStatus)
                << "Restriction status for bit range " << bitRange.start << " to " << bitRange.stop
                << " | Expected: " << shouldAccessBeRestricted << " Actual: " << actualRestrictionStatus;
    }

    void assertThatRestrictionStatusMatch(std::size_t dimension, const SignalAccessRestriction::SignalAccess& bitRange, bool shouldAccessBeRestricted) const {
        const auto& actualRestrictionStatus = restriction->isAccessRestrictedToBitRange(bitRange);
        ASSERT_EQ(shouldAccessBeRestricted, actualRestrictionStatus)
                << "Restriction status for bit range " << bitRange.start << " to " << bitRange.stop << " of dimension " << dimension
                << " | Expected: " << shouldAccessBeRestricted << " Actual: " << actualRestrictionStatus;
    }

    void assertThatRestrictionStatusMatch(std::size_t dimension, std::size_t valueForDimension, const SignalAccessRestriction::SignalAccess& bitRange, bool shouldAccessBeRestricted) const {
        const auto& actualRestrictionStatus = restriction->isAccessRestrictedToBitRange(bitRange);
        ASSERT_EQ(shouldAccessBeRestricted, actualRestrictionStatus)
                << "Restriction status for bit range " << bitRange.start << " to " << bitRange.stop << " of value " << valueForDimension << " of dimension " << dimension
                << " | Expected: " << shouldAccessBeRestricted << " Actual: " << actualRestrictionStatus;
    }

    void validateRestrictedBitPosition() const {
        EXPECT_GT(restrictedBitPosition, 0);
        EXPECT_LT(restrictedBitPosition, expectedBitWidth);    
    }

    template<typename T, std::size_t N, std::size_t... I>
    constexpr auto create_array_impl(std::index_sequence<I...>) {
        return std::array<T, N>{{I...}};
    }

    template<typename T, std::size_t N>
    constexpr auto create_array() {
        return create_array_impl<T, N>(std::make_index_sequence<N>{});
    }
};

/*
 * Tests for correct signal access restriction
 *
 */

/*
 * Scenario 1:  a.0 += ...
 */
// Success cases for scenario 1
TEST_F(SignalAccessRestrictionTest, restrictAccessOnBitOfSignalGloballyAccessOnSameLevelWithOtherBitsOK) {
    restriction->restrictAccessToBit(restrictedBitPosition);
    //const auto& indizesToCheck = std::make_index_sequence<expectedBitWidth>();

    for (std::size_t bit = 0; bit < expectedBitWidth; ++bit) {
        const bool shouldBitAccessBeRestricted = bit == restrictedBitPosition;
        assertThatRestrictionStatusMatch(bit, shouldBitAccessBeRestricted);
    }
    
}

TEST_F(SignalAccessRestrictionTest, restrictAccessOnBitOfSignalGloballyAccessOnSameLevelWithNonOverlappingBitRangeOK) {
    restriction->restrictAccessToBit(restrictedBitPosition);
    validateRestrictedBitPosition();

    assertThatRestrictionStatusMatch(createBitRangeBeforeRestriction(), false);
    assertThatRestrictionStatusMatch(createBitRangeAfterRestriction(), false);
}

TEST_F(SignalAccessRestrictionTest, restrictAccessOnBitOfSignalGloballyAccessOnOtherBitsInSubdimensionsOK) {
    restriction->restrictAccessToBit(restrictedBitPosition);
    

    for (std::size_t dimension = 0; dimension < referenceVariableDimensions.size(); ++dimension) {
        for (std::size_t bit = 0; bit < expectedBitWidth; ++bit) {
            const bool shouldBitAccessBeRestricted = bit == restrictedBitPosition;
            assertThatRestrictionStatusMatch(dimension, bit, shouldBitAccessBeRestricted);
            for (std::size_t valueForDimension = 0; valueForDimension < referenceVariable->dimensions.at(dimension); ++valueForDimension) {
                assertThatRestrictionStatusMatch(dimension, valueForDimension, bit, shouldBitAccessBeRestricted);
            }
        }    
    }
}

TEST_F(SignalAccessRestrictionTest, restrictAccessOnBitOfSignalGloballyAccessOnNonOverlappingBitRangeInSubdimensionsOK) {
    restriction->restrictAccessToBit(restrictedBitPosition);
    validateRestrictedBitPosition();

    for (std::size_t dimension = 0; dimension < referenceVariableDimensions.size(); ++dimension) {
        assertThatRestrictionStatusMatch(dimension, createBitRangeBeforeRestriction(), false);
        assertThatRestrictionStatusMatch(dimension, createBitRangeAfterRestriction(), false);
        assertThatRestrictionStatusMatch(dimension, createBitRangeContainingRestriction(), true);

        for (std::size_t valueForDimension = 0; valueForDimension < referenceVariable->dimensions.at(dimension); ++valueForDimension) {
            assertThatRestrictionStatusMatch(dimension, valueForDimension, createBitRangeBeforeRestriction(), false);
            assertThatRestrictionStatusMatch(dimension, valueForDimension, createBitRangeAfterRestriction(), false);
            assertThatRestrictionStatusMatch(dimension, valueForDimension, createBitRangeContainingRestriction(), true);
        }
    }
}


// Error cases for scenario 1
TEST_F(SignalAccessRestrictionTest, restrictAccessOnBitOfSignalGloballyAccessOnWholeSignalNotOK) {
    restriction->restrictAccessToBit(restrictedBitPosition);
}

TEST_F(SignalAccessRestrictionTest, restrictAccessOnBitOfSignalGloballyAccessOnSameLevelOnSameBitNotOK) {
    restriction->restrictAccessToBit(restrictedBitPosition);
    assertThatRestrictionStatusMatch(restrictedBitPosition, true);
}

TEST_F(SignalAccessRestrictionTest, restrictAccessOnBitOfSignalGloballyAccessOnSameLevelWithOverlappingBitRangeNotOK) {
    restriction->restrictAccessToBit(restrictedBitPosition);
    validateRestrictedBitPosition();

    assertThatRestrictionStatusMatch(createBitRangeContainingRestriction(), true);
}

TEST_F(SignalAccessRestrictionTest, restrictAccessOnBitOfSignalGloballyAccessOnWholeValueOfDimensionNotOK) {
    restriction->restrictAccessToBit(restrictedBitPosition);
}

TEST_F(SignalAccessRestrictionTest, restrictAccessOnBitOfSignalGloballyAccessOnSameBitOfSubDimensionsNotOK) {
    restriction->restrictAccessToBit(restrictedBitPosition);
    for (std::size_t dimension = 0; dimension < referenceVariableDimensions.size(); ++dimension) {
        for (std::size_t valueForDimension = 0; valueForDimension < referenceVariableDimensions.at(dimension); ++valueForDimension) {
            assertThatRestrictionStatusMatch(dimension, valueForDimension, restrictedBitPosition, true);
        }
    }
}

TEST_F(SignalAccessRestrictionTest, restrictAccessOnBitOfSignalGloballyAccessOnOverlappingBitRangeOfSubDimensionsNotOK) {
    restriction->restrictAccessToBit(restrictedBitPosition);
    for (std::size_t dimension = 0; dimension < referenceVariableDimensions.size(); ++dimension) {
        for (std::size_t valueForDimension = 0; valueForDimension < referenceVariableDimensions.at(dimension); ++valueForDimension) {
            assertThatRestrictionStatusMatch(dimension, valueForDimension, createBitRangeContainingRestriction(), true);
        }
    }
}
