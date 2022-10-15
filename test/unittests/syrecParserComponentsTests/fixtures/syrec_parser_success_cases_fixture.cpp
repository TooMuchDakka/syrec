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

                            std::make_pair("production_module", "nParametersAndNLocals"),

                            std::make_pair("production_module", "singleStatementBody"),
                            std::make_pair("production_module", "multipleStatementBody"),
                            std::make_pair("production_module", "singleModule"),
                            std::make_pair("production_module", "multipleModules"),

                            /* Call and uncall production */
                            std::make_pair("production_callStatement", "callWithNoParameters"),
                            std::make_pair("production_callStatement", "callWithOneParameter"),
                            std::make_pair("production_callStatement", "callWithNParameters"),

                            /* IfStatement production */
                            std::make_pair("production_ifStatement", "numberAsCondition"),
                            std::make_pair("production_ifStatement", "signalAsCondition"),
                            std::make_pair("production_ifStatement", "binaryExpressionAsCondition"),
                            std::make_pair("production_ifStatement", "unaryExpressionAsCondition"),
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

                            /* SwapStatement */
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

                            /* SkipStatement */
                             std::make_pair("production_skipStatement", "simpleTest"),

                             /* ForStatement */
                             std::make_pair("production_forStatement", "rangeAsOnlyInitialValue"),
                             std::make_pair("production_forStatement", "rangeAsInitialValueWithPositiveStepSize"),
                             std::make_pair("production_forStatement", "rangeAsInitialValueWithNegativeStepSize"),
                             std::make_pair("production_forStatement", "fullRangeWithoutStepsize"),
                             std::make_pair("production_forStatement", "fullRangeWithPositiveStepsize"),
                             std::make_pair("production_forStatement", "fullRangeWithNegativeStepsize"),
                             std::make_pair("production_forStatement", "loopVariableWithOnlyInitialValue"),
                             std::make_pair("production_forStatement", "loopVariableWithInitialValueAndPositiveStepsize"),
                             std::make_pair("production_forStatement", "loopVariableWithInitialValueAndNegativeStepsize"),
                             std::make_pair("production_forStatement", "loopVariableWithFullRangeDefinedAndPositiveStepsize"),
                             std::make_pair("production_forStatement", "loopVariableWithFullRangeDefinedAndNegativeStepsize"),
                             std::make_pair("production_forStatement", "singleStatementInBody"),
                             std::make_pair("production_forStatement", "multipleStatementsInBody"),

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