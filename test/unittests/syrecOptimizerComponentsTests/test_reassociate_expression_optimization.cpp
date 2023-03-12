#include "syrec_ast_dump_utils.hpp"

#include "gtest/gtest.h"
#include "core/syrec/program.hpp"

#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>

using namespace syrec;

class ReassociateExpressionTest : public testing::TestWithParam<std::string> {
private:
    const std::string relativePathToTestCaseFile    = "./unittests/syrecOptimizerComponentsTests/testdata/reassociate_expression_optimization.json";
    const std::string cJsonKeyCircuit               = "circuit";
    const std::string cJsonKeyExpectedCircuitOutput = "expectedCircuit";

protected:
    syrecAstDumpUtils::SyrecASTDumper astDumper;
    syrec::program parserPublicInterface;

    std::string circuitToOptimize;
    std::string expectedOptimizedCircuit;

    explicit ReassociateExpressionTest():
        astDumper(syrecAstDumpUtils::SyrecASTDumper(true)) {}

    void SetUp() override {
        const std::string                         testCaseJsonKey = GetParam();
        
        std::ifstream     configFileStream(relativePathToTestCaseFile, std::ios_base::in);
        ASSERT_TRUE(configFileStream.good()) << "Could not open test data json file @ "
                                             << relativePathToTestCaseFile;

        const nlohmann::json parsedJson = nlohmann::json::parse(configFileStream);
        ASSERT_TRUE(parsedJson.contains(testCaseJsonKey)) << "Required entry for given test case with key '" << testCaseJsonKey << "' was not found";

        const nlohmann::json testcaseJsonData = parsedJson[testCaseJsonKey];
        ASSERT_TRUE(testcaseJsonData.is_object()) << "Expected test case data for test with key '" << testCaseJsonKey << "' to be in the form of a json object";

        ASSERT_TRUE(testcaseJsonData.contains(cJsonKeyCircuit)) << "Required entry with key '" << cJsonKeyCircuit << "' was not found";
        ASSERT_TRUE(testcaseJsonData.at(cJsonKeyCircuit).is_string()) << "Expected entry with key '" << cJsonKeyCircuit << "' to by a string";

        if (testcaseJsonData.contains(cJsonKeyExpectedCircuitOutput)) {
            ASSERT_TRUE(testcaseJsonData.at(cJsonKeyExpectedCircuitOutput).is_string()) << "Expected entry with key '" << cJsonKeyExpectedCircuitOutput << "' to by an array";
            expectedOptimizedCircuit = testcaseJsonData.at(cJsonKeyCircuit).get<std::string>();
        }

        circuitToOptimize = testcaseJsonData.at(cJsonKeyCircuit).get<std::string>();
    }
};

INSTANTIATE_TEST_SUITE_P(SyReCOptimizations, ReassociateExpressionTest,
                         testing::Values(
                                 "alu_2"),
                         [](const testing::TestParamInfo<ReassociateExpressionTest::ParamType>& info) {
                             auto s = info.param;
                             std::replace( s.begin(), s.end(), '-', '_');
                             return s; });

TEST_P(ReassociateExpressionTest, GenericReassociateExpressionOptimization) {
    std::string errorsFromParsedCircuit;
    ASSERT_NO_THROW(errorsFromParsedCircuit = parserPublicInterface.readFromString(circuitToOptimize));
    ASSERT_TRUE(errorsFromParsedCircuit.empty()) << "Expected to be able to parse given circuit without errors";

    std::string stringifiedProgram;
    ASSERT_NO_THROW(stringifiedProgram = astDumper.stringifyModules(parserPublicInterface.modules())) << "Failed to stringify parsed modules";
    ASSERT_EQ(expectedOptimizedCircuit, stringifiedProgram);
}