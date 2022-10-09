#include <googletest/googletest/include/gtest/gtest.h>
#include "gtest/gtest.h"
#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>

#include "syrec_ir_json_utils.hpp"
#include "core/syrec/program.hpp"

using json = nlohmann::json;

class SyReCParserSuccessCasesFixture: public ::testing::TestWithParam<std::string> {
private:
    const std::string testCaseFileLocationRelativePath = "./unittests/syrecParserComponentsTests/testdata/";
    const syrecTestsJsonUtils::SyrecIRJsonParser syrecIRJsonParser;

protected:
    std::string circuit;
    std::vector<syrec::Module::ptr> expectedIR;
    syrec::program                  parserPublicInterface;

    void SetUp() override {
        const std::string testCaseDataFileRelativePath = testCaseFileLocationRelativePath + GetParam() + ".json";
        std::ifstream configFileStream(testCaseDataFileRelativePath, std::ios_base::in);
        ASSERT_TRUE(configFileStream.good()) << "Could not open test data json file @ " << testCaseDataFileRelativePath;

        json parsedJson = json::parse(configFileStream);
        ASSERT_TRUE(parsedJson.is_object()) << "Expected test data in json to an object";
        ASSERT_TRUE(parsedJson.contains(syrecTestsJsonUtils::cJsonKeyTestCaseRawCircuitData))
                << "Expected json entry with key '" << syrecTestsJsonUtils::cJsonKeyTestCaseRawCircuitData << "' was not found";
        ASSERT_TRUE(parsedJson.at(syrecTestsJsonUtils::cJsonKeyTestCaseRawCircuitData).is_string())
                << "Expected json entry with key '" << syrecTestsJsonUtils::cJsonKeyTestCaseRawCircuitData << "' to be a string";

        ASSERT_TRUE(parsedJson.contains(syrecTestsJsonUtils::cJsonKeyTestCaseExpectedIR))
                << "Expected json entry with key '" << syrecTestsJsonUtils::cJsonKeyTestCaseExpectedIR << "' was not found";
        ASSERT_TRUE(parsedJson.at(syrecTestsJsonUtils::cJsonKeyTestCaseExpectedIR).is_array())
                << "Expected json entry with key '" << syrecTestsJsonUtils::cJsonKeyTestCaseExpectedIR << "' to be an array";

        ASSERT_NO_THROW(parseExpectedModuleJsonData(parsedJson.at(syrecTestsJsonUtils::cJsonKeyTestCaseExpectedIR))) << "Failed to parsed test case data";
    }

    void parseExpectedModuleJsonData(const json& jsonData) {
        for (auto& jsonModuleEntry: jsonData) {
            ASSERT_TRUE(jsonModuleEntry.is_object()) << "Expected definition of expected IR module entry in json to be a json object";

            std::optional<syrec::Module::ptr>       parsedModule;
            ASSERT_NO_THROW(syrecIRJsonParser.parseModuleFromJson(jsonModuleEntry, parsedModule)) << "Failed to parse json entry for one of the expected modules";
            // TODO: Failure should not happen here
            ASSERT_TRUE(parsedModule.has_value());
            expectedIR.emplace_back(parsedModule.value());
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

}