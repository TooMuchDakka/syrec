#include "core/syrec/program.hpp"
#include "syrec_ast_dump_utils.hpp"
#include "test_case_creation_utils.hpp"

#include "gtest/gtest.h"
#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>

using namespace syrec;

class NoAdditionalLineSynthesisTest: public testing::TestWithParam<std::string> {
public:
    inline static const std::string pathToTestCaseFile = "./unittests/syrecOptimizerComponentsTests/testdata/no_additional_line_synthesis.json";

private:
    const std::string cJsonKeyCircuit               = "circuit";
    const std::string cJsonKeyExpectedCircuitOutput = "expectedCircuit";

protected:
    syrecAstDumpUtils::SyrecASTDumper astDumper;
    syrec::ReadProgramSettings        config;
    syrec::program                    parserPublicInterface;

    std::string circuitToOptimize;
    std::string expectedOptimizedCircuit;

    explicit NoAdditionalLineSynthesisTest():
        astDumper(syrecAstDumpUtils::SyrecASTDumper(true)), config(16, true, false, true, true) {}

    void SetUp() override {
        const std::string testCaseJsonKey = GetParam();

        std::ifstream configFileStream(NoAdditionalLineSynthesisTest::pathToTestCaseFile, std::ios_base::in);
        ASSERT_TRUE(configFileStream.good()) << "Could not open test data json file @ "
                                             << NoAdditionalLineSynthesisTest::pathToTestCaseFile;

        const nlohmann::json parsedJson = nlohmann::json::parse(configFileStream);
        ASSERT_TRUE(parsedJson.contains(testCaseJsonKey)) << "Required entry for given test case with key '" << testCaseJsonKey << "' was not found";

        const nlohmann::json testcaseJsonData = parsedJson[testCaseJsonKey];
        ASSERT_TRUE(testcaseJsonData.is_object()) << "Expected test case data for test with key '" << testCaseJsonKey << "' to be in the form of a json object";

        ASSERT_TRUE(testcaseJsonData.contains(cJsonKeyCircuit)) << "Required entry with key '" << cJsonKeyCircuit << "' was not found";
        ASSERT_TRUE(testcaseJsonData.at(cJsonKeyCircuit).is_string()) << "Expected entry with key '" << cJsonKeyCircuit << "' to by a string";
        circuitToOptimize = testcaseJsonData.at(cJsonKeyCircuit).get<std::string>();

        if (testcaseJsonData.contains(cJsonKeyExpectedCircuitOutput)) {
            ASSERT_TRUE(testcaseJsonData.at(cJsonKeyExpectedCircuitOutput).is_string()) << "Expected entry with key '" << cJsonKeyExpectedCircuitOutput << "' to by an array";
            expectedOptimizedCircuit = testcaseJsonData.at(cJsonKeyExpectedCircuitOutput).get<std::string>();
        } else {
            expectedOptimizedCircuit = circuitToOptimize;
        }
    }
};

INSTANTIATE_TEST_SUITE_P(SyReCOptimizations, NoAdditionalLineSynthesisTest,
                         testing::ValuesIn(syrecTestUtils::loadTestCasesNamesFromFile(NoAdditionalLineSynthesisTest::pathToTestCaseFile, {})),
                         [](const testing::TestParamInfo<NoAdditionalLineSynthesisTest::ParamType>& info) {
                             auto s = info.param;
                             std::replace( s.begin(), s.end(), '-', '_');
                             return s; });

TEST_P(NoAdditionalLineSynthesisTest, GenericNoAdditionalLineSynthesisOptimization) {
    std::string errorsFromParsedCircuit;
    ASSERT_NO_THROW(errorsFromParsedCircuit = parserPublicInterface.readFromString(circuitToOptimize, config));
    ASSERT_TRUE(errorsFromParsedCircuit.empty()) << "Expected to be able to parse given circuit without errors";

    std::string stringifiedProgram;
    ASSERT_NO_THROW(stringifiedProgram = astDumper.stringifyModules(parserPublicInterface.modules())) << "Failed to stringify parsed modules";
    ASSERT_EQ(expectedOptimizedCircuit, stringifiedProgram);
}