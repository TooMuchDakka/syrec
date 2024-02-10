#include "gtest/gtest.h"

#include "syrec_parser_signal_access_restriction_error_fixture.hpp"

using namespace SignalAccessRestrictionParserTests;

TEST_P(SyrecParserSignalAccessRestrictionErrorFixture, CheckThatRestrictionChanged){
    const auto testData = createTestData(assignmentLhsOperand, excludedTestCases);
    for (const auto& test : testData) {
        syrec::Program parserInterface;
        std::string actualErrorsConcatinated;

        const auto& programToParse = test.first;
        const auto& expectedError = test.second;

        ASSERT_NO_THROW(actualErrorsConcatinated = parserInterface.readFromString(programToParse, syrec::ReadProgramSettings{})) << "Error while parsing circuit";
        ASSERT_TRUE(parserInterface.modules().empty()) << "Expected no valid modules but actually found " << parserInterface.modules().size() << " valid modules";
        ASSERT_FALSE(actualErrorsConcatinated.empty()) << "Expected at least one error";

        std::vector<std::string> remainingActualErrorsAfterFiltering;
        ASSERT_NO_THROW(remainingActualErrorsAfterFiltering = filterNotRelevantErrors(actualErrorsConcatinated)) << "Error while trying to filter out not relevant errors";
        ASSERT_EQ(1u, remainingActualErrorsAfterFiltering.size()) << "Expected only one remaining error but " << remainingActualErrorsAfterFiltering.size() << " error remained after filtering out not relevant ones";
        
        const std::string& actualErrorStringified = remainingActualErrorsAfterFiltering.at(0);
        const std::string& expectedErrorStringified = expectedError.stringifiy();

        ASSERT_EQ(expectedErrorStringified, actualErrorStringified) << "Expected error message: " << expectedErrorStringified << " but was actually: " << actualErrorStringified;
    }
};

// TODO: Bit range tests for partial ones (i.e. .0:$i, $i:0, $i:($i +1)) ?

INSTANTIATE_TEST_SUITE_P(
        SyrecParserErrorCases,
        SyrecParserSignalAccessRestrictionErrorFixture,
        testing::Values(
                std::make_tuple(
                        "testAccessingRestrictedPartOfAssignmentOfWholeSignal",
                        SignalOperandBuilderHelpers::createAccessOnWholeSignal(SyrecParserSignalAccessRestrictionErrorFixture::assignedToSignalName),
                        ExcludedCases::OverlappingBitRanges),
                std::make_tuple(
                        "testAccessingRestrictedPartOfAssignmentToBitOfSignal",
                        SignalOperandBuilderHelpers::createAccessOnWholeSignal(SyrecParserSignalAccessRestrictionErrorFixture::assignedToSignalName)
                            .accessBit(std::make_optional(SyrecParserSignalAccessRestrictionErrorFixture::defaultBitRange.first)),
                        ExcludedCases::OverlappingBitRanges),
                std::make_tuple(
                        "testAccessingRestrictedPartOfAssignmentToAnyBitOfSignal",
                        SignalOperandBuilderHelpers::createAccessOnWholeSignal(SyrecParserSignalAccessRestrictionErrorFixture::assignedToSignalName)
                            .accessBit(std::nullopt),
                        ExcludedCases::OverlappingBitRanges),
                std::make_tuple(
                        "testAccessingRestrictedPartOfAssignmentToBitRangeOfSignal",
                        SignalOperandBuilderHelpers::createAccessOnWholeSignal(SyrecParserSignalAccessRestrictionErrorFixture::assignedToSignalName)
                            .accessBitRange(SyrecParserSignalAccessRestrictionErrorFixture::defaultBitRange),
                        ExcludedCases::NoneExcluded),
                std::make_tuple(
                        "testAccessingRestrictedPartOfAssignmentToBitRangeWithEndBeingOnlyRuntimeConstantOfSignal",
                        SignalOperandBuilderHelpers::createAccessOnWholeSignal(SyrecParserSignalAccessRestrictionErrorFixture::assignedToSignalName)
                            .accessBitRange(std::make_pair(std::make_optional(SyrecParserSignalAccessRestrictionErrorFixture::defaultBitRange.first), std::nullopt)),
                        ExcludedCases::NoneExcluded),
                std::make_tuple(
                        "testAccessingRestrictedPartOfAssignmentToBitRangeWithStartBeingOnlyRuntimeConstantOfSignal",
                        SignalOperandBuilderHelpers::createAccessOnWholeSignal(SyrecParserSignalAccessRestrictionErrorFixture::assignedToSignalName)
                            .accessBitRange(std::make_pair(std::nullopt, std::make_optional(SyrecParserSignalAccessRestrictionErrorFixture::defaultBitRange.second))),
                        ExcludedCases::NoneExcluded),
                std::make_tuple(
                        "testAccessingRestrictedPartOfAssignmentToBitRangeWithBordersBeingRuntimeConstantsOfSignal",
                        SignalOperandBuilderHelpers::createAccessOnWholeSignal(SyrecParserSignalAccessRestrictionErrorFixture::assignedToSignalName)
                            .accessBitRange(std::make_pair(std::nullopt, std::nullopt)),
                        ExcludedCases::NoneExcluded),
                std::make_tuple(
                        "testAccessingRestrictedPartOfAssignmentToConcreteBitOfCompletelyBlockedDimension",
                        SignalOperandBuilderHelpers::createAccessOnWholeSignal(SyrecParserSignalAccessRestrictionErrorFixture::assignedToSignalName)
                            .accessDimension(std::nullopt)
                            .accessBit(std::make_optional(SyrecParserSignalAccessRestrictionErrorFixture::defaultBitRange.first)),
                        ExcludedCases::OverlappingBitRanges),
                std::make_tuple(
                        "testAccessingRestrictedPartOfAssignmentToAnyBitOfCompletelyBlockedDimension",
                        SignalOperandBuilderHelpers::createAccessOnWholeSignal(SyrecParserSignalAccessRestrictionErrorFixture::assignedToSignalName)
                            .accessDimension(std::nullopt)
                            .accessBit(std::nullopt),
                        ExcludedCases::OverlappingBitRanges),
                std::make_tuple(
                        "testAccessingRestrictedPartOfAssignmentToConcreteBitOfValueForDimension",
                        SignalOperandBuilderHelpers::createAccessOnWholeSignal(SyrecParserSignalAccessRestrictionErrorFixture::assignedToSignalName)
                            .accessDimension(SyrecParserSignalAccessRestrictionErrorFixture::defaultValueForAccessedDimension)
                            .accessBit(std::make_optional(SyrecParserSignalAccessRestrictionErrorFixture::defaultBitRange.first)),
                        ExcludedCases::OverlappingBitRanges),
                std::make_tuple(
                        "testAccessingRestrictedPartOfAssignmentToAnyBitOfValueForDimension",
                        SignalOperandBuilderHelpers::createAccessOnWholeSignal(SyrecParserSignalAccessRestrictionErrorFixture::assignedToSignalName)
                            .accessDimension(SyrecParserSignalAccessRestrictionErrorFixture::defaultValueForAccessedDimension)
                            .accessBit(std::nullopt),
                        ExcludedCases::OverlappingBitRanges),
                std::make_tuple(
                        "testAccessingRestrictedPartOfAssignmentToBitRangeOfCompletelyBlockedDimension",
                        SignalOperandBuilderHelpers::createAccessOnWholeSignal(SyrecParserSignalAccessRestrictionErrorFixture::assignedToSignalName)
                            .accessDimension(std::nullopt)
                            .accessBitRange(SyrecParserSignalAccessRestrictionErrorFixture::defaultBitRange),
                        NoneExcluded),
                std::make_tuple(
                        "testAccessingRestrictedPartOfAssignmentToBitRangeWithEndBeingOnlyRuntimeConstantOfCompletelyBlockedDimension",
                        SignalOperandBuilderHelpers::createAccessOnWholeSignal(SyrecParserSignalAccessRestrictionErrorFixture::assignedToSignalName)
                            .accessDimension(std::nullopt)
                            .accessBitRange(std::make_pair(std::make_optional(SyrecParserSignalAccessRestrictionErrorFixture::defaultBitRange.first), std::nullopt)),
                        ExcludedCases::NoneExcluded),
                std::make_tuple(
                        "testAccessingRestrictedPartOfAssignmentToBitRangeWithStartBeingOnlyRuntimeConstantOfCompletelyBlockedDimension",
                        SignalOperandBuilderHelpers::createAccessOnWholeSignal(SyrecParserSignalAccessRestrictionErrorFixture::assignedToSignalName)
                            .accessDimension(std::nullopt)
                            .accessBitRange(std::make_pair(std::nullopt, std::make_optional(SyrecParserSignalAccessRestrictionErrorFixture::defaultBitRange.second))),
                        ExcludedCases::NoneExcluded),
                std::make_tuple(
                        "testAccessingRestrictedPartOfAssignmentToBitRangeWithBordersBeingRuntimeConstantsOfCompletelyBlockedDimension",
                        SignalOperandBuilderHelpers::createAccessOnWholeSignal(SyrecParserSignalAccessRestrictionErrorFixture::assignedToSignalName)
                            .accessDimension(std::nullopt)
                            .accessBitRange(std::make_pair(std::nullopt, std::nullopt)),
                        ExcludedCases::NoneExcluded),
                std::make_tuple(
                        "testAccessingRestrictedPartOfAssignmentToBitRangeOfValueForDimension",
                        SignalOperandBuilderHelpers::createAccessOnWholeSignal(SyrecParserSignalAccessRestrictionErrorFixture::assignedToSignalName)
                            .accessDimension(SyrecParserSignalAccessRestrictionErrorFixture::defaultValueForAccessedDimension)
                            .accessBitRange(SyrecParserSignalAccessRestrictionErrorFixture::defaultBitRange),
                        NoneExcluded),
                std::make_tuple(
                        "testAccessingRestrictedPartOfAssignmentToBitRangeWithEndBeingOnlyRuntimeConstantOfValueForDimension",
                        SignalOperandBuilderHelpers::createAccessOnWholeSignal(SyrecParserSignalAccessRestrictionErrorFixture::assignedToSignalName)
                            .accessDimension(SyrecParserSignalAccessRestrictionErrorFixture::defaultValueForAccessedDimension)
                            .accessBitRange(std::make_pair(std::make_optional(SyrecParserSignalAccessRestrictionErrorFixture::defaultBitRange.first), std::nullopt)),
                        ExcludedCases::NoneExcluded),
                std::make_tuple(
                        "testAccessingRestrictedPartOfAssignmentToBitRangeWithStartBeingOnlyRuntimeConstantOfValueForDimension",
                        SignalOperandBuilderHelpers::createAccessOnWholeSignal(SyrecParserSignalAccessRestrictionErrorFixture::assignedToSignalName)
                            .accessDimension(SyrecParserSignalAccessRestrictionErrorFixture::defaultValueForAccessedDimension)
                            .accessBitRange(std::make_pair(std::nullopt, std::make_optional(SyrecParserSignalAccessRestrictionErrorFixture::defaultBitRange.second))),
                        ExcludedCases::NoneExcluded),
                std::make_tuple(
                        "testAccessingRestrictedPartOfAssignmentToBitRangeWithBordersBeingRuntimeConstantsOfValueForDimension",
                        SignalOperandBuilderHelpers::createAccessOnWholeSignal(SyrecParserSignalAccessRestrictionErrorFixture::assignedToSignalName)
                            .accessDimension(SyrecParserSignalAccessRestrictionErrorFixture::defaultValueForAccessedDimension)
                            .accessBitRange(std::make_pair(std::nullopt, std::nullopt)),
                        ExcludedCases::NoneExcluded)
    ),
    [](const testing::TestParamInfo<SyrecParserSignalAccessRestrictionErrorFixture::ParamType>& info) {
        auto testNameToTransform = std::get<0>(info.param);
        std::replace(testNameToTransform.begin(), testNameToTransform.end(), '-', '_');
        return testNameToTransform;
});