#include "core/syrec/variable.hpp"
#include "core/syrec/parser/range_check.hpp"

#include "gtest/gtest.h"

using namespace parser;

class RangeCheckTest : public::testing::Test {
protected:
    const std::vector<unsigned int> referenceVariableDimensions = {2, 4, 5};
    const unsigned int              expectedBitWidth            = 32;

    syrec::Variable::ptr referenceVariable;

	void SetUp() override {
        referenceVariable = std::make_shared<syrec::Variable>(syrec::Variable(0, "variable", referenceVariableDimensions, expectedBitWidth));
	}
};


/*
 * Tests for correct return of constraints
 *
 */
TEST_F(RangeCheckTest, noConstraintsForOutOfRangeDimension) {
    EXPECT_FALSE(parser::getConstraintsForValidDimensionAccess(referenceVariable, referenceVariableDimensions.size()).has_value());
}

TEST_F(RangeCheckTest, constraintsForDimension) {
    const size_t                              accessedDimension               = referenceVariableDimensions.size() - 1;
    const size_t                              expectedMaximumValue            = referenceVariableDimensions[accessedDimension] - 1;
    const std::optional<IndexAccessRangeConstraint> expectedConstraintsForDimension = std::make_optional(IndexAccessRangeConstraint(0, expectedMaximumValue));

    const std::optional<IndexAccessRangeConstraint> actualConstraintsForDimension = getConstraintsForValidDimensionAccess(referenceVariable, accessedDimension);
    EXPECT_TRUE(actualConstraintsForDimension.has_value());
    EXPECT_EQ(expectedConstraintsForDimension->minimumValidValue, actualConstraintsForDimension->minimumValidValue);
    EXPECT_EQ(expectedConstraintsForDimension->maximumValidValue, actualConstraintsForDimension->maximumValidValue);
}

TEST_F(RangeCheckTest, constraintsForBitRangeAccess) {
    const std::optional<IndexAccessRangeConstraint> expectedConstraintsForBitRange = std::make_optional(IndexAccessRangeConstraint(0, this->expectedBitWidth - 1));
    const std::optional<IndexAccessRangeConstraint> actualConstraintsForBitRange = getConstraintsForValidBitRangeAccess(referenceVariable);

    EXPECT_TRUE(actualConstraintsForBitRange.has_value());
    EXPECT_EQ(expectedConstraintsForBitRange->minimumValidValue, actualConstraintsForBitRange->minimumValidValue);
    EXPECT_EQ(expectedConstraintsForBitRange->maximumValidValue, actualConstraintsForBitRange->maximumValidValue);
}

TEST_F(RangeCheckTest, constraintsForBitAccess) {
    const std::optional<IndexAccessRangeConstraint> expectedBitAccessConstraints = std::make_optional(IndexAccessRangeConstraint(0, this->expectedBitWidth - 1));
    const std::optional<IndexAccessRangeConstraint> actualBitAccessConstraints    = getConstraintsForValidBitAccess(referenceVariable);

    EXPECT_TRUE(actualBitAccessConstraints.has_value());
    EXPECT_EQ(expectedBitAccessConstraints->minimumValidValue, actualBitAccessConstraints->minimumValidValue);
    EXPECT_EQ(expectedBitAccessConstraints->maximumValidValue, actualBitAccessConstraints->maximumValidValue);
}

/*
 * Tests whether access is ok
 *
 */

TEST_F(RangeCheckTest, validBitAccessIsOk) {
    EXPECT_TRUE(parser::isValidBitAccess(referenceVariable, 0));
    EXPECT_TRUE(parser::isValidBitAccess(referenceVariable, 2));
    EXPECT_TRUE(parser::isValidBitAccess(referenceVariable, expectedBitWidth - 1));
}

TEST_F(RangeCheckTest, outOfRangeBitAccessIsNotOk) {
    EXPECT_FALSE(parser::isValidBitAccess(referenceVariable, expectedBitWidth));
}

TEST_F(RangeCheckTest, validBitRangeAccessIsOk) {
    EXPECT_TRUE(parser::isValidBitRangeAccess(referenceVariable, std::make_pair(0, expectedBitWidth - 1)));
    EXPECT_TRUE(parser::isValidBitRangeAccess(referenceVariable, std::make_pair(expectedBitWidth / 2, expectedBitWidth - 1)));
}

TEST_F(RangeCheckTest, bitRangeWithStartAndEndMatchingIsOk) {
    EXPECT_TRUE(parser::isValidBitRangeAccess(referenceVariable, std::make_pair(expectedBitWidth - 1, expectedBitWidth - 1)));
}

TEST_F(RangeCheckTest, bitRangeWithStartLargerThanEndIsNotOk) {
    EXPECT_FALSE(parser::isValidBitRangeAccess(referenceVariable, std::make_pair(expectedBitWidth, expectedBitWidth - 1)));
}

TEST_F(RangeCheckTest, bitRangeInvalidStartOutOfRangeIsNotOk) {
    EXPECT_FALSE(parser::isValidBitRangeAccess(referenceVariable, std::make_pair(expectedBitWidth, expectedBitWidth - 1)));
}

TEST_F(RangeCheckTest, bitRangeInvalidEndOutOfRangeIsNotOk) {
    EXPECT_FALSE(parser::isValidBitRangeAccess(referenceVariable, std::make_pair(0, expectedBitWidth + 10)));
}

TEST_F(RangeCheckTest, dimensionAccessOutOfRangeIsNotOk) {
    EXPECT_FALSE(parser::isValidDimensionAccess(referenceVariable, referenceVariableDimensions.size(), 0));
}

TEST_F(RangeCheckTest, valueForDimensionOutOfRangeIsNotOk) {
    const size_t accessedDimension = referenceVariableDimensions.size() - 1;
    const size_t valueForAccessedDimension = referenceVariableDimensions[accessedDimension] + 2;

    EXPECT_FALSE(parser::isValidDimensionAccess(referenceVariable, accessedDimension, valueForAccessedDimension));
}

TEST_F(RangeCheckTest, validDimensionAccessIsOk) {
    const size_t accessedDimension         = referenceVariableDimensions.size() - 1;
    const size_t valueForAccessedDimension = referenceVariableDimensions[accessedDimension] - 1;

    EXPECT_TRUE(parser::isValidDimensionAccess(referenceVariable, accessedDimension, valueForAccessedDimension));
}