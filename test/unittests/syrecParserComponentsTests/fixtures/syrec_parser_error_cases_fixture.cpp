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
    const std::string cExpectedErrorsJsonKey    = "expected_errors";

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
        for (auto& errorJsonObject: json.at(cExpectedErrorsJsonKey)) {
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
                                 std::make_pair("production_module", "")
                         ),
                         [](const testing::TestParamInfo<SyrecParserErrorCasesFixture::ParamType>& info) {
                             auto s = info.param.first + "_" + info.param.second;
                             std::replace(s.begin(), s.end(), '-', '_');
                             return s;
                        });

TEST_P(SyrecParserErrorCasesFixture, GenericSyrecParserErrorTest) {
    std::string actualErrorsConcatinated;
    ASSERT_NO_THROW(actualErrorsConcatinated = parserPublicInterface.readFromString(this->circuitDefinition, syrec::ReadProgramSettings{})) << "Error while parsing circuit";
    ASSERT_NO_THROW(compareExpectedAndActualErrors(this->expectedErrors, transformActualErrors(actualErrorsConcatinated))) << "Missmatch between expected and actual errors";
}
