#include "core/syrec/program.hpp"
#include "core/syrec/parser/infix_iterator.hpp"
#include "core/syrec/parser/parser_utilities.hpp"
#include "core/syrec/parser/utils/message_utils.hpp"

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
    const std::string expectedErrorMessageFormat   = "-- line {0:d} col {1:d}: {2:s}";
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

    static void compareExpectedAndActualErrors(const std::vector<std::string>& expectedErrors, const std::vector<std::string>& actualErrorsInUnifiedFormat) {
        if (expectedErrors.size() != actualErrorsInUnifiedFormat.size()) {
            std::ostringstream expectedErrorsBuffer;
            std::ostringstream actualErrorsBuffer;

            std::copy(expectedErrors.cbegin(), expectedErrors.cend(), InfixIterator<std::string>(expectedErrorsBuffer, "\n"));
            std::copy(actualErrorsInUnifiedFormat.cbegin(), actualErrorsInUnifiedFormat.cend(), InfixIterator<std::string>(actualErrorsBuffer, "\n"));

            FAIL() << "Expected " << expectedErrors.size() << " errors but only " << actualErrorsInUnifiedFormat.size() << " were found!\nExpected: " << expectedErrorsBuffer.str() << "\nActual: " << actualErrorsBuffer.str();
        }

        //ASSERT_EQ(expectedErrors.size(), actualErrorsInUnifiedFormat.size()) << "Expected " << expectedErrors.size() << " errors but only " << actualErrorsInUnifiedFormat.size() << " were found";
        for (size_t errorIdx = 0; errorIdx < expectedErrors.size(); ++errorIdx) {
            ASSERT_EQ(expectedErrors.at(errorIdx), actualErrorsInUnifiedFormat.at(errorIdx)) << "Expected error: " << expectedErrors.at(errorIdx) << "| Actual Error: " << actualErrorsInUnifiedFormat.at(errorIdx);
        }
    }
};

/*
 * TODO:
 * - Create generic "parser" of all tests defined in a .json file instead of having to manually specify them
 * - Add option 'optional' to possible json test data that will add the required IGNORE/IGNORED name prefix so gtest will ignore this test but it will still show up in test test explorer as skipped
 *
 */

/*
 *  TODO:
 *  - Error cases for binary expression where the dimension of the operands do not match, similarily to the shift expression error cases
 *  - What is the behaviour of a shift where the operand to be shifted is a signal with multiple dimensions (should that be possible or is that an error)
 * - // TODO: Success cases for broadcasting of Call- and UncallStatements
 */
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
                             std::make_pair("production_module", "localWithDimensionNotBeingAConstant"),
                             std::make_pair("production_module", "localWithSignalwidthNotBeingANumber"),
                             std::make_pair("production_module", "localWithSignalwidthNotBeingAConstant"),
                             std::make_pair("production_module", "localWithSignalwidthMissingOpeningBracket"),
                             std::make_pair("production_module", "localWithSignalwidthMissingClosingBracket"),
                             
                             std::make_pair("production_module", "missingLocalDelimiter"),
                             std::make_pair("production_module", "missingLocalAfterDelimiter"),
                             std::make_pair("production_module", "duplicateLocalInSameLine"),
                             std::make_pair("production_module", "duplicateLocalInDifferentDeclaration"),
                             std::make_pair("production_module", "duplicateLocalOfDifferentType"),
                             std::make_pair("production_module", "invalidStatementInvalidatesModule"),
                             std::make_pair("production_module", "emptyStatementBody"),
                             std::make_pair("production_module", "duplicateModules"),

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
                             std::make_pair("production_callStatement", "missingUncallAfterCall"),
                             std::make_pair("production_callStatement", "uncallWithDifferentParameterThanCall"),
                             std::make_pair("production_callStatement", "uncallWithLessParameterThanCall"),
                             std::make_pair("production_callStatement", "uncallWithMoreParametersThanCall"),

                             /* ForStatement production */
                             std::make_pair("production_forStatement", "missingLoopVariablePrefix"),
                             std::make_pair("production_forStatement", "invalidForIdent"),
                             std::make_pair("production_forStatement", "invalidLoopVariablePrefix"),
                             std::make_pair("production_forStatement", "invalidLoopVariableInitialValueAssignmentOperator"),
                             std::make_pair("production_forStatement", "missingLoopVariableInitialValueAssignmentOperator"),
                             std::make_pair("production_forStatement", "invalidRangeStartAndEndDelimiterIdent"),
                             std::make_pair("production_forStatement", "missingRangeStartAndEndDelimiterIdent"),
                             std::make_pair("production_forStatement", "invalidStepsizeIdent"),
                             std::make_pair("production_forStatement", "missingStepsizeIdent"),
                             std::make_pair("production_forStatement", "invalidNegativeStepsizeIdent"),
                             std::make_pair("production_forStatement", "startValueLargerThanEndWithPositiveStepsize"),
                             std::make_pair("production_forStatement", "startValueLargerThanEndWithPositiveStepsizeNoEndValueDefined"),
                             std::make_pair("production_forStatement", "startValueLargerThanEndWithPositiveStepsizeThatWasNotDefined"),
                             std::make_pair("production_forStatement", "startValueSmallerThanEndWithNegativeStepsize"),
                             std::make_pair("production_forStatement", "missingLoopHeaderPostfixIdent"),
                             std::make_pair("production_forStatement", "invalidLoopHeaderPostfixIdent"),
                             std::make_pair("production_forStatement", "invalidRofEndDelimiter"),
                             std::make_pair("production_forStatement", "missingRofEndDelimiter"),
                             std::make_pair("production_forStatement", "duplicateLoopVariableNameInNestedLoop"),
                             std::make_pair("production_forStatement", "invalidSelfReferenceToLoopVariableInStartValueDefinition"),
                             std::make_pair("production_forStatement", "stepSizeOfZeroCreatesError"),
                             std::make_pair("production_forStatement", "stepSizeOfZeroCreatesErrorIfStartValueIsOmittedAndOnlyEndValueIsCompileTimeConstantGreaterThanZero"),

                             /* IfStatement production */
                             std::make_pair("production_ifStatement", "invalidIfIdent"),
                             std::make_pair("production_ifStatement", "missingIfIdent"),
                             std::make_pair("production_ifStatement", "invalidThenIdent"),
                             std::make_pair("production_ifStatement", "missingThenIdent"),
                             std::make_pair("production_ifStatement", "invalidElseIdent"),
                             std::make_pair("production_ifStatement", "invalidFiIdent"),
                             std::make_pair("production_ifStatement", "invalidIfConditionExpression"),
                             std::make_pair("production_ifStatement", "invalidFiConditionExpression"),
                             std::make_pair("production_ifStatement", "emptyIfBody"),
                             std::make_pair("production_ifStatement", "emptyElseBody"),
                             std::make_pair("production_ifStatement", "missmatchBetweenIfAndFiCondition"),

                             /* UnaryStatement production */
                             std::make_pair("production_unaryStatement", "invalidUnaryOperator"),
                             std::make_pair("production_unaryStatement", "missingEqualityOperator"),
                             std::make_pair("production_unaryStatement", "invalidEqualityOperator"),

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

                             std::make_pair("production_assignStatement", "missingAssignmentOperatorPostfix"),
                             std::make_pair("production_assignStatement", "invalidAssignmentOperatorPostfix"),
                             std::make_pair("production_assignStatement", "invalidAssignmentVariableIsUsedInRhs"),

                             /* Do all of these cases need to be checked ? */
                             std::make_pair("production_assignStatement", "invalidAssignmentToInParameterBit"),
                             std::make_pair("production_assignStatement", "invalidAssignmentToInParameterBitRange"),
                             std::make_pair("production_assignStatement", "invalidAssignmentToInParameterDimension"),
                             std::make_pair("production_assignStatement", "invalidAssignmentToInParameterDimensionBit"),
                             std::make_pair("production_assignStatement", "invalidAssignmentToInParameterDimensionBitRange"),
                             std::make_pair("production_assignStatement", "invalidAssignmentToInParameterNestedDimension"),
                             std::make_pair("production_assignStatement", "invalidAssignmentToInParameterNestedDimensionBit"),
                             std::make_pair("production_assignStatement", "invalidAssignmentToInParameterNestedDimensionBitRange"),
                             std::make_pair("production_assignStatement", "invalidAssignmentOperandStructureMissmatchWithLhsSignalHavingMoreDimensions"),
                             std::make_pair("production_assignStatement", "invalidAssignmentOperandStructureMissmatchWithLhsSignalHavingLessDimensions"),
                             std::make_pair("production_assignStatement", "invalidAssignmentOperandStructureMissmatchForSingleDimension"),
                             std::make_pair("production_assignStatement", "invalidAssignmentOperandStructureMissmatchForMultipleDimensions"),
                             std::make_pair("production_assignStatement", "invalidAssignmentOperandStructureMissmatchLhsIsBitAccessOnDimension"),
                             std::make_pair("production_assignStatement", "invalidAssignmentOperandStructureMissmatchLhsIsBitAccess"),
                             std::make_pair("production_assignStatement", "invalidAssignmentOperandStructureMissmatchLhsIsBitRangeAccessOnDimension"),
                             std::make_pair("production_assignStatement", "invalidAssignmentOperandStructureMissmatchLhsIsBitRangeAccess"),

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
                                 
                             std::make_pair("production_swapStatement", "invalidSwapOfLhsSignalWithRhsSignalWithValueForDimensionTooSmall"),
                             std::make_pair("production_swapStatement", "invalidSwapOfLhsSignalWithRhsSignalWithValueForDimensionTooLarge"),
                             std::make_pair("production_swapStatement", "invalidSwapOfLhsSignalWithRhsSignalWithValueForOneNestedDimensionTooSmall"),
                             std::make_pair("production_swapStatement", "invalidSwapOfLhsSignalWithRhsSignalWithValueForOneNestedDimensionTooLarge"),

                             std::make_pair("production_swapStatement", "invalidSwapOfLhsSignalBitDefinedAsCompileTimeExpressionWithRhsSignal"),
                             std::make_pair("production_swapStatement", "invalidSwapOfLhsSignalWithRhsSignalBitDefinedAsCompileTimeExpression"),

                             /* SkipStatement production */
                             std::make_pair("production_skipStatement", "missingSkipIdent"),

                             /* Signal production */
                             std::make_pair("production_signal", "undeclaredIdent"),
                             std::make_pair("production_signal", "missingDimensionAccessOpeningBracket"),
                             std::make_pair("production_signal", "missingDimensionAccessOpeningBracketInNestedDimensionAccess"),
                             std::make_pair("production_signal", "missingDimensionAccessClosingBracket"),
                             std::make_pair("production_signal", "missingDimensionAccessClosingBracketInNestedDimensionAccess"),
                             std::make_pair("production_signal", "invalidBitAccessPrefix"),
                             std::make_pair("production_signal", "missingBitAccessIdent"),
                             std::make_pair("production_signal", "missingBitRangeAccessEndValuePrefixIdent"),
                             std::make_pair("production_signal", "invalidBitRangeAccessEndValuePrefixIdent"),
                             std::make_pair("production_signal", "dimensionOutOfRange"),
                             std::make_pair("production_signal", "valueForDimensionOutOfRange"),
                             std::make_pair("production_signal", "bitAccessOutOfRange"),
                             std::make_pair("production_signal", "bitRangeStartOutOfRange"),
                             std::make_pair("production_signal", "bitRangeEndOutOfRange"),
                             std::make_pair("production_signal", "bitRangeStartValueLargerThanEnd"),

                             /* number */
                             std::make_pair("number", "notAnInteger"),
                             std::make_pair("number", "accessingBitwidthOfUndeclaredSignal"),
                             std::make_pair("number", "accessingUndeclaredLoopVariable"),
                             std::make_pair("number", "missingOpeningBracketInExpression"),
                             std::make_pair("number", "missingOpeningBracketInNestedExpression"),
                             std::make_pair("number", "missingClosingBracketInExpression"),
                             std::make_pair("number", "missingClosingBracketInNestedExpression"),
                             std::make_pair("number", "invalidOperationInExpression"),
                             std::make_pair("number", "invalidOperationInNestedExpression"),

                             /* BinaryExpression production */
                             std::make_pair("production_binaryExpression", "lhsOperandIsInvalidExpression"),
                             std::make_pair("production_binaryExpression", "rhsOperandIsInvalidExpression"),
                             std::make_pair("production_binaryExpression", "invalidBinaryOperation"),
                             std::make_pair("production_binaryExpression", "missingOpeningBracket"),
                             std::make_pair("production_binaryExpression", "invalidOpeningBracket"),
                             std::make_pair("production_binaryExpression", "missingClosingBracket"),
                             std::make_pair("production_binaryExpression", "invalidClosingBracket"),
                             std::make_pair("production_binaryExpression", "operandsStructureMissmatchValueForSingleDimensionMissmatch"),
                             std::make_pair("production_binaryExpression", "operandsStructureMissmatchValueForMultipleDimensionsMissmatch"),
                             std::make_pair("production_binaryExpression", "operandsStructureMissmatchValueForMultipleDimensionsInNestedDimensionAccessMissmatch"),
                             std::make_pair("production_binaryExpression", "operandsStructureMissmatchTooManyDimensions"),
                             std::make_pair("production_binaryExpression", "operandsStructureMissmatchTooFewDimensions"),
                             std::make_pair("production_binaryExpression", "operandsStructureMissmatchOneOperandIsBitAccessAndOtherIsFullSignal"),
                             std::make_pair("production_binaryExpression", "operandsStructureMissmatchOneOperandIsBitAccessOnDimensionAndOtherIsFullSignal"),
                             std::make_pair("production_binaryExpression", "operandsStructureMissmatchOneOperandIsBitRangeAccessAndOtherIsFullSignal"),
                             std::make_pair("production_binaryExpression", "operandsStructureMissmatchOneOperandIsBitRangeAccessOnDimensionAndOtherIsFullSignal"),
                             std::make_pair("production_binaryExpression", "operandsStructureMissmatchOneOperandIsSubDimensionAccessAndOtherIsFullSignal"),
                             std::make_pair("production_binaryExpression", "operandsStructureMissmatchOneOperandIsFullSignalAndOtherIsNumber"),

                             /* UnaryExpression production */
                             std::make_pair("production_unaryExpression", "invalidUnaryOperation"),

                             /* ShiftExpression production */
                             std::make_pair("production_shiftExpression", "shiftAmountNotANumber"),
                             std::make_pair("production_shiftExpression", "invalidNegativeShiftAmount"),
                             std::make_pair("production_shiftExpression", "missingOpeningBracket"),
                             std::make_pair("production_shiftExpression", "invalidOpeningBracket"),
                             std::make_pair("production_shiftExpression", "missingClosingBracket"),
                             std::make_pair("production_shiftExpression", "invalidClosingBracket"),

                             std::make_pair("production_shiftExpression", "broadcastingOf1DSignalWithMoreThanOneValueRequiredButNotSupported"),
                             std::make_pair("production_shiftExpression", "broadcastingOfNDSignalRequiredButNotSupported"),
                             std::make_pair("production_shiftExpression", "broadcastingOfNDSignalBitAccessRequiredButNotSupported"),
                             std::make_pair("production_shiftExpression", "broadcastingOfNDSignalBitRangeAccessRequiredButNotSupported")
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
    
    ASSERT_NO_THROW(compareExpectedAndActualErrors(this->expectedErrors, messageUtils::tryDeserializeStringifiedMessagesFromString(actualErrorsConcatinated))) << "Missmatch between expected and actual errors";
}
