#include <googletest/googletest/include/gtest/gtest.h>
#include "gtest/gtest.h"
#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>

#include "syrec_ast_dump_utils.hpp"
#include "core/syrec/program.hpp"

using json = nlohmann::json;

class SyReCParserSuccessCasesFixture: public ::testing::TestWithParam<std::string> {
private:
    const std::string cJsonKeyCircuit                  = "circuit";
    const std::string cJsonKeyExpectedCircuitOutput    = "expectedCircuit";
    const std::string cJsonKeyTestCaseName             = "testCaseName";
    const std::string testCaseFileLocationRelativePath = "./unittests/syrecParserComponentsTests/testdata/";

protected:
    std::string                        circuit;
    std::string                        expectedCircuit;
    syrec::program                  parserPublicInterface;
    syrecAstDumpUtils::SyrecASTDumper  astDumper;

    SyReCParserSuccessCasesFixture() : astDumper(syrecAstDumpUtils::SyrecASTDumper(true)) {}

    void SetUp() override {
        const std::string testCaseKey                  = GetParam();
        const std::string testCaseDataFileRelativePath = testCaseFileLocationRelativePath + testCaseKey + ".json";
        std::ifstream     configFileStream(testCaseDataFileRelativePath, std::ios_base::in);
        ASSERT_TRUE(configFileStream.good()) << "Could not open test data json file @ " << testCaseDataFileRelativePath;

        const json parsedJson = json::parse(configFileStream);
        ASSERT_TRUE(parsedJson.is_object()) << "Expected test data to be in the form of a json array";
        ASSERT_TRUE(parsedJson.contains(GetParam())) << "Expected an entry for the test '" << testCaseKey << "'";

        const json testCaseJsonData = parsedJson[testCaseKey];
        ASSERT_TRUE(testCaseJsonData.is_object()) << "Expected test case data for test '" << testCaseKey << "' to be in the form of a json object";
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

INSTANTIATE_TEST_SUITE_P(Another,
                         SyReCParserSuccessCasesFixture,
                         testing::Values("parser_success_cases"),
                        [](const testing::TestParamInfo<SyReCParserSuccessCasesFixture::ParamType>& info) {
                            auto s = info.param;
                            std::replace(s.begin(), s.end(), '-', '_');
                            return s;
                        });

TEST_P(SyReCParserSuccessCasesFixture, GenericSyrecParserSuccessTest) {
    std::string errorsFromParsedCircuit;
    ASSERT_NO_THROW(errorsFromParsedCircuit = parserPublicInterface.readFromString(this->circuit));
    ASSERT_TRUE(errorsFromParsedCircuit.empty()) << "Expected to be able to parse given circuit without errors";

    int z = 0;

    std::string stringifiedProgram;
    ASSERT_NO_THROW(stringifiedProgram = astDumper.stringifyModules(parserPublicInterface.modules())) << "Failed to stringify parsed modules";
    ASSERT_EQ(this->expectedCircuit, stringifiedProgram);
}