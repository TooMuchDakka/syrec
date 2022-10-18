#include "core/syrec/program.hpp"

#include <fmt/core.h>
#include "gtest/gtest.h"
#include <googletest/googletest/include/gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>
#include <string>

using json = nlohmann::json;

class SyrecParserErrorCasesFixture: public ::testing::TestWithParam<std::pair<std::string, std::string>> {
private:
    const std::string expectedErrorMessageFormat   = "-- line %d col %d: %ls\n";
    const std::string relativePathToTestCaseFile   = "./unittests/syrecParserComponentsTests/testdata/parsing/error/";
    const std::string cJsonErrorLineJsonKey        = "line";
    const std::string cJsonErrorColumnJsonKey      = "column";
    const std::string cJsonErrorExpectedMessageKey = "message";

    const std::string cCircuitDefinitionJsonKey = "circuit";
    const std::string cExpectedErrorsJsonKey    = "errors";

protected:
    struct ExpectedError {
        std::size_t line;
        std::size_t column;
        std::string message;

        explicit ExpectedError(const std::size_t line, const std::size_t column, std::string message):
            line(line), column(column), message(std::move(message)) {}
    };

    std::string                circuitDefinition;
    std::vector<std::string> expectedErrors;
    syrec::program             parserPublicInterface;

    void SetUp() override {
        const std::pair<std::string, std::string> testCaseData = GetParam();
        const std::string                         testCaseJsonKey = testCaseData.second;
        const std::string                         testCaseFileName = testCaseData.first;

        const std::string relativeTestFilePath     = relativePathToTestCaseFile + testCaseFileName + ".json";
        std::ifstream     configFileStream(relativeTestFilePath, std::ios_base::in);
        ASSERT_TRUE(configFileStream.good()) << "Could not open test data json file @ "
                                             << relativeTestFilePath;

        const json parsedJson = json::parse(configFileStream);
        ASSERT_TRUE(parsedJson.contains(testCaseJsonKey)) << "Required entry for given test case with key '" << testCaseJsonKey << "' was not found";

        const json testcaseJsonData = parsedJson[testCaseJsonKey];
        ASSERT_TRUE(testcaseJsonData.is_object()) << "Expected test case data for test with key '" << testCaseJsonKey << "' to be in the form of a json object";

        ASSERT_TRUE(testcaseJsonData.contains(cCircuitDefinitionJsonKey)) << "Required entry with key '" << cCircuitDefinitionJsonKey << "' was not found";
        ASSERT_TRUE(testcaseJsonData.at(cCircuitDefinitionJsonKey).is_string()) << "Expected entry with key '" << cCircuitDefinitionJsonKey << "' to by a string";
        ASSERT_TRUE(testcaseJsonData.contains(cExpectedErrorsJsonKey)) << "Required entry with key '" << cExpectedErrorsJsonKey << "' was not found";
        ASSERT_TRUE(testcaseJsonData.at(cExpectedErrorsJsonKey).is_array()) << "Expected entry with key '" << cExpectedErrorsJsonKey << "' to by an array";

        circuitDefinition = testcaseJsonData.at(cCircuitDefinitionJsonKey).get<std::string>();
        loadExpectedErrors(testcaseJsonData.at(cExpectedErrorsJsonKey));
    }

    void loadExpectedErrors(const json& json) {
        for (auto& errorJsonObject: json) {
            ASSERT_TRUE(errorJsonObject.is_object()) << "Expected test case data to a json object";
            ASSERT_TRUE(errorJsonObject.contains(cJsonErrorLineJsonKey)) << "Required entry with key '" << cJsonErrorLineJsonKey << "' was not found";
            ASSERT_TRUE(errorJsonObject.contains(cJsonErrorColumnJsonKey)) << "Required entry with key '" << cJsonErrorColumnJsonKey << "' was not found";
            ASSERT_TRUE(errorJsonObject.contains(cJsonErrorExpectedMessageKey)) << "Required entry with key '" << cJsonErrorExpectedMessageKey << "' was not found";

            ASSERT_TRUE(errorJsonObject.at(cJsonErrorLineJsonKey).is_number_unsigned()) << "Expected entry with key '" << cJsonErrorLineJsonKey << "' to be an unsigned integer";
            ASSERT_TRUE(errorJsonObject.at(cJsonErrorColumnJsonKey).is_number_unsigned()) << "Expected entry with key '" << cJsonErrorColumnJsonKey << "' to be an unsigned integer";
            ASSERT_TRUE(errorJsonObject.at(cJsonErrorExpectedMessageKey).is_string()) << "Expected entry with key '" << cJsonErrorExpectedMessageKey << "' to be a string";

            const ExpectedError expectedError = ExpectedError(
                    errorJsonObject.at(cJsonErrorLineJsonKey).get<unsigned int>(),
                    errorJsonObject.at(cJsonErrorColumnJsonKey).get<unsigned int>(),
                    errorJsonObject.at(cJsonErrorExpectedMessageKey).get<std::string>()
            );
            ASSERT_NO_THROW(expectedErrors.emplace_back(fmt::format(expectedErrorMessageFormat, expectedError.line, expectedError.column, expectedError.message)));
        }
    }

    static std::vector<std::string> transformActualErrors(const std::string& rawActualErrors) {
        std::vector<std::string> actualErrors{};
        size_t                   previousErrorEndPosition = 0;
        size_t                   currErrorEndPosition     = rawActualErrors.find_first_of('\n', previousErrorEndPosition);

        while (std::string::npos != previousErrorEndPosition) {
            // TODO: Index check
            actualErrors.emplace_back(rawActualErrors.substr(previousErrorEndPosition + 1, (currErrorEndPosition - previousErrorEndPosition - 1)));
            previousErrorEndPosition = currErrorEndPosition;
        }
        return actualErrors;
    }
    

    static void compareExpectedAndActualErrors(const std::vector<std::string>& expectedErrors, const std::vector<std::string>& actualErrorsInUnifiedFormat) {
        ASSERT_EQ(expectedErrors.size(), actualErrorsInUnifiedFormat.size()) << "Expected " << expectedErrors.size() << " errors but only " << actualErrorsInUnifiedFormat.size() << " were found";
        for (size_t errorIdx = 0; errorIdx < expectedErrors.size(); ++errorIdx) {
            ASSERT_EQ(expectedErrors.at(errorIdx), actualErrorsInUnifiedFormat.at(errorIdx)) << "Expected error: " << expectedErrors.at(errorIdx) << "| Actual Error: " << actualErrorsInUnifiedFormat.at(errorIdx);
        }
    }
};

INSTANTIATE_TEST_SUITE_P(SyrecParserErrorCases,
                         SyrecParserErrorCasesFixture,
                         testing::Values(
                             /* Module production */
                             std::make_pair("production_module", "missingModulePrefix"),
                             std::make_pair("production_module", "invalidSymbolAtModuleIdentStart"),
                             std::make_pair("production_module", "invalidSymbolInModuleIdent"),
                             std::make_pair("production_module", "missingParameterListOpeningBracket"),
                             std::make_pair("production_module", "missingParameterListClosingBracket"),
                             std::make_pair("production_module", "invalidParameterType"),
                             std::make_pair("production_module", "invalidParameterIdent"),
                             std::make_pair("production_module", "parameterWithDimensionMissingOpeningBracket"),
                             std::make_pair("production_module", "parameterWithDimensionMissingClosingBracket"),
                             std::make_pair("production_module", "parameterWithDimensionNotBeingANumber"),
                             std::make_pair("production_module", "parameterWithSignalwidthMissingOpeningBracket"),

                             std::make_pair("production_module", "parameterWithSignalwidthMissingClosingBracket"),
                             std::make_pair("production_module", "parameterWithSignalwidthNotBeingANumber"),
                             std::make_pair("production_module", "missingParameterDelimiter"),
                             std::make_pair("production_module", "missingParameterAfterDelimiter"),
                             std::make_pair("production_module", "duplicateParameterOfSameType"),
                             std::make_pair("production_module", "duplicateParameterOfDifferentType"),
                             std::make_pair("production_module", "duplicateDeclarationMatchBetweenParameterAndLocal"),
                             std::make_pair("production_module", "invalidLocalType"),
                             std::make_pair("production_module", "invalidLocalIdent"),
                             std::make_pair("production_module", "localWithDimensionMissingOpeningBracket"),
                             std::make_pair("production_module", "localWithDimensionMissingClosingBracket"),
                             std::make_pair("production_module", "localWithDimensionNotBeingANumber"),
                             std::make_pair("production_module", "localWithSignalwidthMissingOpeningBracket"),
                             std::make_pair("production_module", "localWithSignalwidthMissingClosingBracket"),

                             std::make_pair("production_module", "localWithSignalwidthNotBeingANumber"),
                             std::make_pair("production_module", "missingLocalDelimiter"),
                             std::make_pair("production_module", "missingLocalAfterDelimiter"),
                             std::make_pair("production_module", "duplicateLocalInSameLine"),
                             std::make_pair("production_module", "duplicateLocalInDifferentDeclaration"),
                             std::make_pair("production_module", "duplicateLocalOfDifferentType"),
                             std::make_pair("production_module", "invalidStatementInvalidatesModule"),
                             std::make_pair("production_module", "emptyStatementBody"),
                             std::make_pair("production_module", "duplicateModules"),
                             std::make_pair("production_module", "missingMainModule"),

                             /* StatementList production */
                             std::make_pair("production_statementList", "missingStatementDelimiter"),
                             std::make_pair("production_statementList", "oneDelimiterTooManyAfterLastStatementOfBlock"),

                             /* CallStatement production */
                             std::make_pair("production_callStatement", "invalidCallIdent"),
                             std::make_pair("production_callStatement", "invalidUncallIdent"),
                             std::make_pair("production_callStatement", "callWithMissingOpeningBracket"),
                             std::make_pair("production_callStatement", "callWithMissingClosingBracket"),
                             std::make_pair("production_callStatement", "callOfUndefinedModule"),
                             std::make_pair("production_callStatement", "callOfModuleWithNotEnoughParameters"),
                             std::make_pair("production_callStatement", "callOfModuleWithTooManyParameters"),
                             std::make_pair("production_callStatement", "callWithInvalidParameterDelimiter"),
                             std::make_pair("production_callStatement", "uncallWithInvalidParameterDelimiter"),
                             std::make_pair("production_callStatement", "callAfterCallWithoutPriorUncallOfTheFormer"),

                             /* ForStatement production */
                             std::make_pair("production_forStatement", "invalidForIdent"),
                             std::make_pair("production_forStatement", "invalidLoopVariablePrefix"),
                             std::make_pair("production_forStatement", "invalidLoopVariableInitialValueAssignmentOperator"),
                             std::make_pair("production_forStatement", "invalidRangeStartAndEndDelimiterIdent"),
                             std::make_pair("production_forStatement", "invalidStepsizeIdent"),
                             std::make_pair("production_forStatement", "invalidNegativeStepsizeIdent"),
                             std::make_pair("production_forStatement", "invalidLoopHeaderPostfixIdent"),
                             std::make_pair("production_forStatement", "invalidRofEndDelimiter"),
                             std::make_pair("production_forStatement", "assignmentToLoopVariableNotValid"),

                             /* IfStatement production */
                             std::make_pair("production_ifStatement", "invalidIfIdent"),
                             std::make_pair("production_ifStatement", "invalidThenIdent"),
                             std::make_pair("production_ifStatement", "invalidElseIdent"),
                             std::make_pair("production_ifStatement", "invalidFiIdent"),
                             std::make_pair("production_ifStatement", "invalidIfConditionExpression"),
                             std::make_pair("production_ifStatement", "invalidFiConditionExpression"),
                             std::make_pair("production_ifStatement", "emptyIfBody"),
                             std::make_pair("production_ifStatement", "emptyThenBody"),
                             std::make_pair("production_ifStatement", "missmatchBetweenIfAndFiCondition"),

                             /* UnaryStatement production */
                             std::make_pair("production_unaryStatement", "invalidUnaryOperator"),
                             /* Do all of these cases need to be checked ?*/
                             std::make_pair("production_unaryStatement", "invalidAssignmentToInParameter"),
                             std::make_pair("production_unaryStatement", "invalidAssignmentToInParameterBit"),
                             std::make_pair("production_unaryStatement", "invalidAssignmentToInParameterBitRange"),
                             std::make_pair("production_unaryStatement", "invalidAssignmentToInParameterDimension"),
                             std::make_pair("production_unaryStatement", "invalidAssignmentToInParameterDimensionBit"),
                             std::make_pair("production_unaryStatement", "invalidAssignmentToInParameterDimensionBitRange"),
                             std::make_pair("production_unaryStatement", "invalidAssignmentToInParameterNestedDimension"),
                             std::make_pair("production_unaryStatement", "invalidAssignmentToInParameterNestedDimensionBit"),
                             std::make_pair("production_unaryStatement", "invalidAssignmentToInParameterNestedDimensionBitRange"),

                             /* AssignStatement production */
                             std::make_pair("production_assignStatement", "invalidAssignOperator"),
                             std::make_pair("production_assignStatement", "invalidAssignmentToInParameter"),
                             /* Do all of these cases need to be checked ? */
                             std::make_pair("production_assignStatement", "invalidAssignmentToInParameterBit"),
                             std::make_pair("production_assignStatement", "invalidAssignmentToInParameterBitRange"),
                             std::make_pair("production_assignStatement", "invalidAssignmentToInParameterDimension"),
                             std::make_pair("production_assignStatement", "invalidAssignmentToInParameterDimensionBit"),
                             std::make_pair("production_assignStatement", "invalidAssignmentToInParameterDimensionBitRange"),
                             std::make_pair("production_assignStatement", "invalidAssignmentToInParameterNestedDimension"),
                             std::make_pair("production_assignStatement", "invalidAssignmentToInParameterNestedDimensionBit"),
                             std::make_pair("production_assignStatement", "invalidAssignmentToInParameterNestedDimensionBitRange"),

                             /* SwapStatement production */
                             std::make_pair("production_swapStatement", "invalidSwapStatementOperator"),
                             std::make_pair("production_swapStatement", "invalidSwapOfLhsBitWithBitRangeOfRhs"),
                             std::make_pair("production_swapStatement", "invalidSwapOfLhsBitWithDimensionOfRhsWithLongerSignalwidth"),
                             std::make_pair("production_swapStatement", "invalidSwapOfLhsBitWithNestedDimensionOfRhsWithLongerSignalwidth"),
                             std::make_pair("production_swapStatement", "invalidSwapOfLhsBitWithFullSignalOfRhsWithLongerSignalwidth"),
                             std::make_pair("production_swapStatement", "invalidSwapOfLhsBitRangeWithBitOfRhs"),
                             std::make_pair("production_swapStatement", "invalidSwapOfLhsBitRangeWithDimensionOfRhsWithLongerSignalwidth"),
                             std::make_pair("production_swapStatement", "invalidSwapOfLhsBitRangeWithNestedDimensionOfRhsWithLongerSignalwidth"),
                             std::make_pair("production_swapStatement", "invalidSwapOfLhsBitRangeWithFullSignalOfRhsWithLongerSignalwidth"),
                             std::make_pair("production_swapStatement", "invalidSwapOfLhsBitRangeWithDimensionOfRhsWithSmallerSignalwidth"),
                             std::make_pair("production_swapStatement", "invalidSwapOfLhsBitRangeWithNestedDimensionOfRhsWithSmallerSignalwidth"),
                             std::make_pair("production_swapStatement", "invalidSwapOfLhsBitRangeWithFullSignalOfSmallerSignalwidth"),
                             std::make_pair("production_swapStatement", "invalidSwapOfLhsSignalWithTooLargeBitRangeOfRhs"),
                             std::make_pair("production_swapStatement", "invalidSwapOfLhsSignalWithTooSmallBitRangeOfRhs"),
                             std::make_pair("production_swapStatement", "invalidSwapOfLhsSignalWithRhsSignalWithLessDimensions"),
                             std::make_pair("production_swapStatement", "invalidSwapOfLhsSignalWithRhsSignalWithTooManyDimensions"),
                             std::make_pair("production_swapStatement", "invalidSwapOfLhsSignalWithRhsSignalWithSmallerBitwidth"),
                             std::make_pair("production_swapStatement", "invalidSwapOfLhsSignalWithRhsSignalWithLargerBitwidth"),

                             /* SkipStatement production */
                             std::make_pair("production_skipStatement", "missingSkipIdent"),

                             /* Signal production */
                             std::make_pair("production_signal", "undeclaredIdent"),
                             std::make_pair("production_signal", "missingDimensionAccessOpeningBracket"),
                             std::make_pair("production_signal", "missingDimensionAccessOpeningBracketInNestedDimensionAccess"),
                             std::make_pair("production_signal", "missingDimensionAccessClosingBracket"),
                             std::make_pair("production_signal", "missingDimensionAccessClosingBracketInNestedDimensionAccess"),
                             std::make_pair("production_signal", "missingBitAccessIdent"),
                             std::make_pair("production_signal", "missingBitRangeAccessEndPrefixIdent"),
                             std::make_pair("production_signal", "dimensionOutOfRange"),
                             std::make_pair("production_signal", "valueForDimensionOutOfRange"),
                             std::make_pair("production_signal", "bitAccessOutOfRange"),
                             std::make_pair("production_signal", "bitRangeStartOutOfRange"),
                             std::make_pair("production_signal", "bitRangeEndOutOfRange"),

                             /* number */
                             std::make_pair("number", "accessingBitwidthOfUndeclaredSignal"),
                             std::make_pair("number", "accessingUndeclaredLoopVariable"),
                             std::make_pair("number", "missingOpeningBracketInExpression"),
                             std::make_pair("number", "missingOpeningBracketInNestedExpression"),
                             std::make_pair("number", "missingClosingBracketInExpression"),
                             std::make_pair("number", "missingClosingBracketInNestedExpression"),
                             std::make_pair("number", "invalidOperationInExpression"),
                             std::make_pair("number", "invalidOperationInNestedExpression")
                         ),
                         [](const testing::TestParamInfo<SyrecParserErrorCasesFixture::ParamType>& info) {
                             auto s = info.param.first + "_" + info.param.second;
                             std::replace(s.begin(), s.end(), '-', '_');
                             return s;
                        });

TEST_P(SyrecParserErrorCasesFixture, GenericSyrecParserErrorTest) {
    std::string actualErrorsConcatinated;
    ASSERT_NO_THROW(actualErrorsConcatinated = parserPublicInterface.readFromString(this->circuitDefinition, syrec::ReadProgramSettings{})) << "Error while parsing circuit";
    ASSERT_TRUE(this->parserPublicInterface.modules().empty()) << "Expected no valid modules but actually found " << this->parserPublicInterface.modules().size() << " valid modules";
    ASSERT_FALSE(actualErrorsConcatinated.empty()) << "Expected at least one error";
    ASSERT_NO_THROW(compareExpectedAndActualErrors(this->expectedErrors, transformActualErrors(actualErrorsConcatinated))) << "Missmatch between expected and actual errors";
}
