#include <googletest/googletest/include/gtest/gtest.h>
#include "gtest/gtest.h"
#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>

#include "syrec_ast_dump_utils.hpp"
#include "core/syrec/program.hpp"

using json = nlohmann::json;

class SyrecParserSuccessCasesFixture: public ::testing::TestWithParam<std::pair<std::string, std::string>> {
private:
    const std::string cJsonKeyCircuit                  = "circuit";
    const std::string cJsonKeyExpectedCircuitOutput    = "expectedCircuit";
    const std::string cJsonKeyTestCaseName             = "testCaseName";
    const std::string testCaseFolderRelativePath = "./unittests/syrecParserComponentsTests/testdata/parsing/success/";

protected:
    std::string                        circuit;
    std::string                        expectedCircuit;
    syrec::program                  parserPublicInterface;
    syrecAstDumpUtils::SyrecASTDumper  astDumper;

    explicit SyrecParserSuccessCasesFixture() : astDumper(syrecAstDumpUtils::SyrecASTDumper(true)) {}

    void SetUp() override {
        const std::pair<std::string, std::string> testCaseData = GetParam();
        const std::string                    testCaseDataFileName = testCaseData.first;
        const std::string                    testCaseJsonKey  = testCaseData.second;
        
        const std::string finalTestDataFileRelativePath = testCaseFolderRelativePath + testCaseDataFileName + ".json";
        std::ifstream     configFileStream(finalTestDataFileRelativePath, std::ios_base::in);
        ASSERT_TRUE(configFileStream.good()) << "Could not open test data json file @ " << finalTestDataFileRelativePath;

        const json parsedJson = json::parse(configFileStream);
        ASSERT_TRUE(parsedJson.is_object()) << "Expected test data to be in the form of a json array";
        ASSERT_TRUE(parsedJson.contains(testCaseJsonKey)) << "Expected an entry for the test '" << testCaseJsonKey << "'";

        const json testCaseJsonData = parsedJson[testCaseJsonKey];
        ASSERT_TRUE(testCaseJsonData.is_object()) << "Expected test case data for test '" << testCaseJsonKey << "' to be in the form of a json object";
        ASSERT_TRUE(testCaseJsonData.contains(cJsonKeyCircuit) && testCaseJsonData[cJsonKeyCircuit].is_string()) << "Test case did not contain an entry with key '" << cJsonKeyCircuit << "' and a string as value";
        circuit = testCaseJsonData.at(cJsonKeyCircuit);

        if (testCaseJsonData.contains(cJsonKeyExpectedCircuitOutput)) {
            ASSERT_TRUE(testCaseJsonData[cJsonKeyExpectedCircuitOutput].is_string()) << "Expected optional circuit output data to be in the form of a json string";
            expectedCircuit = testCaseJsonData[cJsonKeyExpectedCircuitOutput].get<std::string>();
        }
        else {
            expectedCircuit = circuit;
        }
    }
};

// TODO: Success cases for broadcasting logic for AssignStatement, Binary- and ShiftExpression
// TODO: Success cases for broadcasting of Call- and UncallStatements

INSTANTIATE_TEST_SUITE_P(SyrecParserSuccessCases,
                        SyrecParserSuccessCasesFixture,
                        testing::Values(
                        /* AssignStatement production */
                        std::make_pair("production_assignStatement", "xorAssign_lhsNoExplicitDimension"),
                        std::make_pair("production_assignStatement", "xorAssign_lhsNoExplicitDimensionOneBit"),
                        std::make_pair("production_assignStatement", "xorAssign_lhsNoExplicitDimensionBitRange"),
                        std::make_pair("production_assignStatement", "xorAssign_lhsOneExplicitDimension"),
                        std::make_pair("production_assignStatement", "xorAssign_lhsOneExplicitDimensionOneBit"),
                        std::make_pair("production_assignStatement", "xorAssign_lhsOneExplicitDimensionBitRange"),
                        std::make_pair("production_assignStatement", "xorAssign_lhsMultipleDimensionsNested"),
                        std::make_pair("production_assignStatement", "xorAssign_lhsMultipleDimensionsNestedOneBit"),
                        std::make_pair("production_assignStatement", "xorAssign_lhsMultipleDimensionsNestedBitRange"),
                        std::make_pair("production_assignStatement", "addAssign_lhsNoExplicitDimension"),
                        std::make_pair("production_assignStatement", "addAssign_lhsNoExplicitDimensionOneBit"),
                        std::make_pair("production_assignStatement", "addAssign_lhsNoExplicitDimensionBitRange"),
                        std::make_pair("production_assignStatement", "addAssign_lhsOneExplicitDimension"),
                        std::make_pair("production_assignStatement", "addAssign_lhsOneExplicitDimensionOneBit"),
                        std::make_pair("production_assignStatement", "addAssign_lhsOneExplicitDimensionBitRange"),
                        std::make_pair("production_assignStatement", "addAssign_lhsMultipleDimensionsNested"),
                        std::make_pair("production_assignStatement", "addAssign_lhsMultipleDimensionsNestedOneBit"),
                        std::make_pair("production_assignStatement", "addAssign_lhsMultipleDimensionsNestedBitRange"),
                        std::make_pair("production_assignStatement", "subAssign_lhsNoExplicitDimension"),
                        std::make_pair("production_assignStatement", "subAssign_lhsNoExplicitDimensionOneBit"),
                        std::make_pair("production_assignStatement", "subAssign_lhsNoExplicitDimensionBitRange"),
                        std::make_pair("production_assignStatement", "subAssign_lhsOneExplicitDimension"),
                        std::make_pair("production_assignStatement", "subAssign_lhsOneExplicitDimensionOneBit"),
                        std::make_pair("production_assignStatement", "subAssign_lhsOneExplicitDimensionBitRange"),
                        std::make_pair("production_assignStatement", "subAssign_lhsMultipleDimensionsNested"),
                        std::make_pair("production_assignStatement", "subAssign_lhsMultipleDimensionsNestedOneBit"),
                        std::make_pair("production_assignStatement", "subAssign_lhsMultipleDimensionsNestedBitRange"),

                        /* Module production*/
                        std::make_pair("production_module", "noParametersAndLocals"),
                        std::make_pair("production_module", "oneParameterNoLocalsNoCustomBitwidthAndDimensionDefaultsAssumed_inType"),
                        std::make_pair("production_module", "oneParameterNoLocalsWithCustomBitwidthAndNoExplicitDimension_inType"),
                        std::make_pair("production_module", "oneParameterNoLocalsWithCustomBitwidthAndOneExplicitDimension_inType"),
                        std::make_pair("production_module", "oneParameterNoLocalsWithCustomBitwidthAndNExplicitDimensions_inType"),
                        std::make_pair("production_module", "oneParameterNoLocalsWithOnlyOneDimension_inType"),
                        std::make_pair("production_module", "oneParameterNoLocalsWithMultipleDimensions_inType"),
                        std::make_pair("production_module", "nParametersSameTypeNoLocals_inType"),

                        std::make_pair("production_module", "oneParameterNoLocalsNoCustomBitwidthAndDimensionDefaultsAssumed_outType"),
                        std::make_pair("production_module", "oneParameterNoLocalsWithCustomBitwidthAndNoExplicitDimension_outType"),
                        std::make_pair("production_module", "oneParameterNoLocalsWithCustomBitwidthAndOneExplicitDimension_outType"),
                        std::make_pair("production_module", "oneParameterNoLocalsWithCustomBitwidthAndNExplicitDimensions_outType"),
                        std::make_pair("production_module", "oneParameterNoLocalsWithOnlyOneDimension_outType"),
                        std::make_pair("production_module", "oneParameterNoLocalsWithMultipleDimensions_outType"),
                        std::make_pair("production_module", "nParametersSameTypeNoLocals_outType"),
                        
                        std::make_pair("production_module", "oneParameterNoLocalsNoCustomBitwidthAndDimensionDefaultsAssumed_inoutType"),
                        std::make_pair("production_module", "oneParameterNoLocalsWithCustomBitwidthAndNoExplicitDimension_inoutType"),
                        std::make_pair("production_module", "oneParameterNoLocalsWithCustomBitwidthAndOneExplicitDimension_inoutType"),
                        std::make_pair("production_module", "oneParameterNoLocalsWithCustomBitwidthAndNExplicitDimensions_inoutType"),
                        std::make_pair("production_module", "oneParameterNoLocalsWithOnlyOneDimension_inoutType"),
                        std::make_pair("production_module", "oneParameterNoLocalsWithMultipleDimensions_inoutType"),
                        std::make_pair("production_module", "nParametersSameTypeNoLocals_inoutType"),

                        std::make_pair("production_module", "nParametersMixedTypeNoLocals"),

                        std::make_pair("production_module", "noParameterOneLocalNoCustomBitwidthAndDimensionDefaultsAssumed_wireType"),
                        std::make_pair("production_module", "noParameterOneLocalWithCustomBitwidthAndNoExplicitDimension_wireType"),
                        std::make_pair("production_module", "noParameterOneLocalWithCustomBitwidthAndOneExplicitDimension_wireType"),
                        std::make_pair("production_module", "noParametersOneLocalWithCustomBitwidthAndNExplicitDimensions_wireType"),
                        std::make_pair("production_module", "noParametersOneLocalWithOnlyOneDimension_wireType"),
                        std::make_pair("production_module", "noParametersOneLocalWithMultipleDimensions_wireType"),
                        std::make_pair("production_module", "noParametersNLocalsSameTypeAllInOneLine_wireType"),
                        std::make_pair("production_module", "noParametersNLocalsSameTypeInNLines_wireType"),

                        std::make_pair("production_module", "noParameterOneLocalNoCustomBitwidthAndDimensionDefaultsAssumed_stateType"),
                        std::make_pair("production_module", "noParameterOneLocalWithCustomBitwidthAndNoExplicitDimension_stateType"),
                        std::make_pair("production_module", "noParameterOneLocalWithCustomBitwidthAndOneExplicitDimension_stateType"),
                        std::make_pair("production_module", "noParametersOneLocalWithCustomBitwidthAndNExplicitDimensions_stateType"),
                        std::make_pair("production_module", "noParametersOneLocalWithOnlyOneDimension_stateType"),
                        std::make_pair("production_module", "noParametersOneLocalWithMultipleDimensions_stateType"),
                        std::make_pair("production_module", "noParametersNLocalsSameTypeAllInOneLine_stateType"),
                        std::make_pair("production_module", "noParametersNLocalsSameTypeInNLines_stateType"),
                            
                        std::make_pair("production_module", "singleStatementBody"),
                        std::make_pair("production_module", "multipleStatementBody"),
                        std::make_pair("production_module", "multipleModules"),

                        /* StatementList production */
                        std::make_pair("production_statementList", "singleStatement"),
                        std::make_pair("production_statementList", "multipleStatements"),
                            
                        /* Call and uncall production */
                        std::make_pair("production_callStatement", "callWithOneParameter"),
                        std::make_pair("production_callStatement", "callWithNParameters"),

                        /* IfStatement production */
                        std::make_pair("production_ifStatement", "numberAsCondition"),
                        std::make_pair("production_ifStatement", "signalAsCondition"),
                        std::make_pair("production_ifStatement", "binaryExpressionAsCondition"),
                        std::make_pair("DISABLED_production_ifStatement", "unaryExpressionAsCondition"),
                        std::make_pair("production_ifStatement", "shiftExpressionAsCondition"),

                        std::make_pair("production_ifStatement", "singleStatementThenBranch"),
                        std::make_pair("production_ifStatement", "multipleStatementsThenBranch"),
                        std::make_pair("production_ifStatement", "singleStatementElseBranch"),
                        std::make_pair("production_ifStatement", "multipleStatementsElseBranch"),

                        /* UnaryStatement production */
                        std::make_pair("production_unaryStatement", "negateAssign_fullSignal"),
                        std::make_pair("production_unaryStatement", "negateAssign_noExplicitDimensionBitAccess"),
                        std::make_pair("production_unaryStatement", "negateAssign_noExplicitDimensionBitRange"),
                        std::make_pair("production_unaryStatement", "negateAssign_oneExplicitDimension"),
                        std::make_pair("production_unaryStatement", "negateAssign_oneExplicitDimensionBitAccess"),
                        std::make_pair("production_unaryStatement", "negateAssign_oneExplicitDimensionBitRange"),
                        std::make_pair("production_unaryStatement", "negateAssign_nExplicitDimensions"),
                        std::make_pair("production_unaryStatement", "negateAssign_nExplicitDimensionBitAccess"),
                        std::make_pair("production_unaryStatement", "negateAssign_nExplicitDimensionBitRange"),

                        std::make_pair("production_unaryStatement", "incrementAssign_fullSignal"),
                        std::make_pair("production_unaryStatement", "incrementAssign_noExplicitDimensionBitAccess"),
                        std::make_pair("production_unaryStatement", "incrementAssign_noExplicitDimensionBitRange"),
                        std::make_pair("production_unaryStatement", "incrementAssign_oneExplicitDimension"),
                        std::make_pair("production_unaryStatement", "incrementAssign_oneExplicitDimensionBitAccess"),
                        std::make_pair("production_unaryStatement", "incrementAssign_oneExplicitDimensionBitRange"),
                        std::make_pair("production_unaryStatement", "incrementAssign_nExplicitDimensions"),
                        std::make_pair("production_unaryStatement", "incrementAssign_nExplicitDimensionBitAccess"),
                        std::make_pair("production_unaryStatement", "incrementAssign_nExplicitDimensionBitRange"),

                        std::make_pair("production_unaryStatement", "decrementAssign_fullSignal"),
                        std::make_pair("production_unaryStatement", "decrementAssign_noExplicitDimensionBitAccess"),
                        std::make_pair("production_unaryStatement", "decrementAssign_noExplicitDimensionBitRange"),
                        std::make_pair("production_unaryStatement", "decrementAssign_oneExplicitDimension"),
                        std::make_pair("production_unaryStatement", "decrementAssign_oneExplicitDimensionBitAccess"),
                        std::make_pair("production_unaryStatement", "decrementAssign_oneExplicitDimensionBitRange"),
                        std::make_pair("production_unaryStatement", "decrementAssign_nExplicitDimensions"),
                        std::make_pair("production_unaryStatement", "decrementAssign_nExplicitDimensionBitAccess"),
                        std::make_pair("production_unaryStatement", "decrementAssign_nExplicitDimensionBitRange"),

                        /* SwapStatement production */
                        std::make_pair("production_swapStatement", "bothSignalsTotal"),
                        std::make_pair("production_swapStatement", "oneBitOfLhs"),
                        std::make_pair("production_swapStatement", "bitRangeOfLhs"),
                        std::make_pair("production_swapStatement", "oneDimensionOfLhs"),
                        std::make_pair("production_swapStatement", "oneBitOfOneDimensionOfLhs"),
                        std::make_pair("production_swapStatement", "bitRangeOfOneDimensionOfLhs"),
                        std::make_pair("production_swapStatement", "nestedDimensionOfLhs"),
                        std::make_pair("production_swapStatement", "oneBitOfNestedDimensionOfLhs"),
                        std::make_pair("production_swapStatement", "bitRangeOfNestedDimensionOfLhs"),

                        std::make_pair("production_swapStatement", "oneBitOfRhs"),
                        std::make_pair("production_swapStatement", "bitRangeOfRhs"),
                        std::make_pair("production_swapStatement", "oneDimensionOfRhs"),
                        std::make_pair("production_swapStatement", "oneBitOfOneDimensionOfRhs"),
                        std::make_pair("production_swapStatement", "bitRangeOfOneDimensionOfRhs"),
                        std::make_pair("production_swapStatement", "nestedDimensionOfRhs"),
                        std::make_pair("production_swapStatement", "oneBitOfNestedDimensionOfRhs"),
                        std::make_pair("production_swapStatement", "bitRangeOfNestedDimensionOfRhs"),
                        std::make_pair("production_swapStatement", "lhsAndRhsWithSameDimensions"),

                        /* SkipStatement production */
                        std::make_pair("production_skipStatement", "simpleTest"),

                        /* ForStatement production */
                        std::make_pair("production_forStatement", "rangeAsOnlyInitialValue"),
                        std::make_pair("production_forStatement", "rangeAsInitialValueWithPositiveStepSize"),
                        std::make_pair("production_forStatement", "rangeAsInitialValueWithNegativeStepSize"),
                        std::make_pair("production_forStatement", "rangeWithInitialValueAsExpression"),
                        std::make_pair("production_forStatement", "rangeWithEndValueAsExpression"),
                        std::make_pair("production_forStatement", "fullRangeWithoutStepsize"),
                        std::make_pair("production_forStatement", "fullRangeWithPositiveStepsize"),
                        std::make_pair("production_forStatement", "fullRangeWithNegativeStepsize"),
                        std::make_pair("production_forStatement", "loopVariableWithoutStepsize"),
                        std::make_pair("production_forStatement", "loopVariableWithPositiveStepsize"),
                        std::make_pair("production_forStatement", "loopVariableWithNegativeStepsize"),
                        std::make_pair("production_forStatement", "stepSizeAsExpression"),
                        std::make_pair("production_forStatement", "parentLoopVariableUsedInNestedLoop"),
                        std::make_pair("production_forStatement", "singleStatementInBody"),
                        std::make_pair("production_forStatement", "multipleStatementsInBody"),
                        std::make_pair("production_forStatement", "reusingLoopVariableNameInAnotherLoopAfterPreviousOneWasLeft"),
                        std::make_pair("production_forStatement", "usageOfLoopVariableInAnyExpressionExceptInitialValueOk"),

                        /* Expression production */
                        std::make_pair("production_expression", "isConstant"),
                        std::make_pair("production_expression", "isSignalWidth"),
                        std::make_pair("DISABLED_production_expression", "isLoopVariable"),
                        std::make_pair("production_expression", "isExpressionWithoutNesting"),
                        std::make_pair("production_expression", "isExpressionWithLhsBeingNested"),
                        std::make_pair("production_expression", "isExpressionWithRhsBeingNested"),
                        std::make_pair("production_expression", "isFullSignal"),
                        std::make_pair("production_expression", "isBitOfSignal"),
                        std::make_pair("production_expression", "isBitRangeOfSignal"),
                        std::make_pair("production_expression", "isBitOfSignalDimension"),
                        std::make_pair("production_expression", "isBitRangeOfSignalDimension"),
                        std::make_pair("production_expression", "isBitOfNestedSignalDimension"),
                        std::make_pair("production_expression", "isBitRangeOfNestedSignalDimension"),

                        /* BinaryExpression production */
                        std::make_pair("production_binaryExpression", "addOperationResultNoSimplification"),
                        std::make_pair("production_binaryExpression", "subOperationResultNoSimplification"),
                        std::make_pair("production_binaryExpression", "xorOperationResultNoSimplification"),
                        std::make_pair("production_binaryExpression", "multiplyOperationResultNoSimplification"),
                        std::make_pair("production_binaryExpression", "divideOperationResultNoSimplification"),
                        std::make_pair("production_binaryExpression", "moduloOperationResultNoSimplification"),
                        std::make_pair("production_binaryExpression", "upperbitsMultiplicationOperationResultNoSimplification"),
                        std::make_pair("production_binaryExpression", "logicalAndOperationResultNoSimplification"),
                        std::make_pair("production_binaryExpression", "logicalOrOperationResultNoSimplification"),
                        std::make_pair("production_binaryExpression", "bitwiseAndOperationResultNoSimplification"),
                        std::make_pair("production_binaryExpression", "bitwiseOrOperationResultNoSimplification"),
                        std::make_pair("production_binaryExpression", "lessThanOperationResultNoSimplification"),
                        std::make_pair("production_binaryExpression", "greaterThanOperationResultNoSimplification"),
                        std::make_pair("production_binaryExpression", "equalOperationResultNoSimplification"),
                        std::make_pair("production_binaryExpression", "notEqualOperationResultNoSimplification"),
                        std::make_pair("production_binaryExpression", "lessOrEqualThanOperationResultNoSimplification"),
                        std::make_pair("production_binaryExpression", "greaterOrEqualThanOperationResultNoSimplification"),

                        std::make_pair("production_binaryExpression", "addOperationResultWithSimplification"),
                        std::make_pair("production_binaryExpression", "subOperationResultWithSimplification"),
                        std::make_pair("production_binaryExpression", "xorOperationResultWithSimplification"),
                        std::make_pair("production_binaryExpression", "multiplyOperationResultWithSimplification"),
                        std::make_pair("production_binaryExpression", "divideOperationResultWithSimplification"),
                        std::make_pair("production_binaryExpression", "moduloOperationResultWithSimplification"),
                        std::make_pair("production_binaryExpression", "upperbitsMultiplicationOperationResultWithSimplification"),
                        std::make_pair("production_binaryExpression", "logicalAndOperationResultWithSimplification"),
                        std::make_pair("production_binaryExpression", "logicalOrOperationResultWithSimplification"),
                        std::make_pair("production_binaryExpression", "bitwiseAndOperationResultWithSimplification"),
                        std::make_pair("production_binaryExpression", "bitwiseOrOperationResultWithSimplification"),
                        std::make_pair("production_binaryExpression", "lessThanOperationResultWithSimplification"),
                        std::make_pair("production_binaryExpression", "greaterThanOperationResultWithSimplification"),
                        std::make_pair("production_binaryExpression", "equalOperationResultWithSimplification"),
                        std::make_pair("production_binaryExpression", "notEqualOperationResultWithSimplification"),
                        std::make_pair("production_binaryExpression", "lessOrEqualThanOperationResultWithSimplification"),
                        std::make_pair("production_binaryExpression", "greaterOrEqualThanOperationResultWithSimplification"),

                        /* UnaryExpression production */
                        std::make_pair("DISABLED_production_unaryExpression", "logicalNegationOfNumber"),
                        std::make_pair("DISABLED_production_unaryExpression", "logicalNegationOfSignal"),
                        std::make_pair("DISABLED_production_unaryExpression", "logicalNegationOfExpression"),
                        std::make_pair("DISABLED_production_unaryExpression", "logicalNegationSimplified"),

                        std::make_pair("DISABLED_production_unaryExpression", "bitwiseNegationOfNumber"),
                        std::make_pair("DISABLED_production_unaryExpression", "bitwiseNegationOfSignal"),
                        std::make_pair("DISABLED_production_unaryExpression", "bitwiseNegationOfExpression"),
                        std::make_pair("DISABLED_production_unaryExpression", "bitwiseNegationSimplified"),

                        /* ShiftExpression production */
                        std::make_pair("production_shiftExpression", "leftShift"),
                        std::make_pair("production_shiftExpression", "leftShiftSimplified"),
                        std::make_pair("production_shiftExpression", "rightShift"),
                        std::make_pair("production_shiftExpression", "rightShiftSimplified"),

                        /* Signal production */
                        std::make_pair("production_signal", "completeSignalOfParameter"),
                        std::make_pair("production_signal", "accessingBitOfParameter"),
                        std::make_pair("production_signal", "accessingBitOfParameterAsNotSimplifiedExpression"),
                        std::make_pair("production_signal", "accessingBitRangeOfParameter"),
                        std::make_pair("production_signal", "accessingBitRangeOfParameterAsNotSimplifiedExpression"),
                        std::make_pair("production_signal", "accessingSingleDimensionOfParameter"),
                        std::make_pair("production_signal", "accessingSingleDimensionOfParameterAsNotSimplifiedExpression"),
                        std::make_pair("production_signal", "accessingBitOfSingleDimensionOfParameter"),
                        std::make_pair("production_signal", "accessingBitRangeOfSingleDimensionOfParameter"),
                        std::make_pair("production_signal", "accessingNestedDimensionOfParameter"),
                        std::make_pair("production_signal", "accessingNestedDimensionOfParameterAsNotSimplifiedExpression"),
                        std::make_pair("production_signal", "accessingBitOfNestedDimensionOfParameter"),
                        std::make_pair("production_signal", "accessingBitRangeOfNestedDimensionOfParameter"),

                        std::make_pair("production_signal", "completeSignalOfLocal"),
                        std::make_pair("production_signal", "accessingBitOfLocal"),
                        std::make_pair("production_signal", "accessingBitOfLocalAsNotSimplifiedExpression"),
                        std::make_pair("production_signal", "accessingBitRangeOfLocal"),
                        std::make_pair("production_signal", "accessingBitRangeOfLocalAsNotSimplifiedExpression"),
                        std::make_pair("production_signal", "accessingSingleDimensionOfLocal"),
                        std::make_pair("production_signal", "accessingSingleDimensionOfLocalAsNotSimplifiedExpression"),
                        std::make_pair("production_signal", "accessingBitOfSingleDimensionOfLocal"),
                        std::make_pair("production_signal", "accessingBitRangeOfSingleDimensionOfLocal"),
                        std::make_pair("production_signal", "accessingNestedDimensionOfLocal"),
                        std::make_pair("production_signal", "accessingNestedDimensionOfLocalAsNotSimplifiedExpression"),
                        std::make_pair("production_signal", "accessingBitOfNestedDimensionOfLocal"),
                        std::make_pair("production_signal", "accessingBitRangeOfNestedDimensionOfLocal"),

                        /* number */
                        std::make_pair("number", "asConstant"),
                        std::make_pair("number", "asSignalWidth"),
                        std::make_pair("number", "asLoopVariable"),
                        std::make_pair("number", "asSimpleExpression"),
                        std::make_pair("number", "asExpressionWithLhsBeingNested"),
                        std::make_pair("number", "asExpressionWithRhsBeingNested"),

                        // TODO: Should there be tests checking that line comments are correctly skipped

                        /* Complete benchmarks */
                        std::make_pair("complete_circuits", "test_circuit")
                        ),
                        [](const testing::TestParamInfo<SyrecParserSuccessCasesFixture::ParamType>& info) {
                             auto s = info.param.first + "_" + info.param.second;
                            std::replace(s.begin(), s.end(), '-', '_');
                            return s;
                        });

TEST_P(SyrecParserSuccessCasesFixture, GenericSyrecParserSuccessTest) {
    std::string errorsFromParsedCircuit;
    ASSERT_NO_THROW(errorsFromParsedCircuit = parserPublicInterface.readFromString(this->circuit));
    ASSERT_TRUE(errorsFromParsedCircuit.empty()) << "Expected to be able to parse given circuit without errors";
    
    std::string stringifiedProgram;
    ASSERT_NO_THROW(stringifiedProgram = astDumper.stringifyModules(parserPublicInterface.modules())) << "Failed to stringify parsed modules";
    ASSERT_EQ(this->expectedCircuit, stringifiedProgram);
}