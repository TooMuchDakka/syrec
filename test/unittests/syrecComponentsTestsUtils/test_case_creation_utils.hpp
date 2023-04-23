#ifndef TEST_CASE_CREATION_UTILS_HPP
#define TEST_CASE_CREATION_UTILS_HPP
#pragma once

#include <fstream>
#include <vector>
#include <set>
#include <string>

#include <nlohmann/json.hpp>

namespace syrecTestUtils {
    /*
     * Only non-empty ASCII alphanumeric characters without underscores are supported as tests names
     * see https://google.github.io/googletest/advanced.html#specifying-names-for-value-parameterized-test-parameters
     */ 
    inline bool doesStringContainOnlyAsciiCharactersAndNoUnderscores(const std::string_view& stringToCheck) {
        return std::all_of(
                stringToCheck.cbegin(),
                stringToCheck.cend(),
                [](const char characterToCheck) {
                    return std::isalnum(characterToCheck);
                });
    }

    inline std::vector<std::string> loadTestCasesNamesFromFile(const std::string& testDataJsonFilename, const std::vector<std::string>& testCasesToIgnore) {
        std::ifstream testDataFileStream(testDataJsonFilename, std::ios_base::in);
        if (!testDataFileStream.good()) {
            return {"DISABLED_FAILED_TO_LOAD_TEST_DATA_FROM_FILE"};
        }

        const nlohmann::json parsedJson = nlohmann::json::parse(testDataFileStream, nullptr, false);
        if (parsedJson.is_discarded()) {
            return {"DISABLED_FAILED_TO_PARSE_TEST_DATA_JSON"};
        }
        if (!parsedJson.is_object()) {
            return {"DISABLED_FAILED_TO_LOAD_TEST_DATA_NOT_JSON_OBJECT"};
        }

        const std::set<std::string> setOfTestCasesToIgnore(testCasesToIgnore.cbegin(), testCasesToIgnore.cend());
        std::vector<std::string>    foundTestCases;
        std::size_t                 testCaseIdx = 0;
        for (const auto& testCaseKVPair: parsedJson.items()) {
            const auto& testCaseName = testCaseKVPair.key();
            if (testCaseName.empty()) {
                continue;
            }

            if (doesStringContainOnlyAsciiCharactersAndNoUnderscores(testCaseName)) {
                if (setOfTestCasesToIgnore.count(testCaseName) != 0) {
                    foundTestCases.emplace_back("DISABLED_" + testCaseName);
                } else {
                    foundTestCases.emplace_back(testCaseName);
                }   
            } else {
                foundTestCases.emplace_back("DISABLED_Test" + std::to_string(testCaseIdx) + "ContainedNonAsciiOrUnderscoreCharacter");
            }
            testCaseIdx++;
        }
        return foundTestCases;
    }
};

#endif