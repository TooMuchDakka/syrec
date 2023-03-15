#ifndef TEST_CASE_CREATION_UTILS_HPP
#define TEST_CASE_CREATION_UTILS_HPP
#pragma once

#include <fstream>
#include <vector>
#include <set>
#include <string>

#include <nlohmann/json.hpp>

namespace syrecTestUtils {
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
        for (const auto& testCaseKVPair: parsedJson.items()) {
            const auto& testCaseName = testCaseKVPair.key();
            if (setOfTestCasesToIgnore.count(testCaseName) != 0) {
                foundTestCases.emplace_back("DISABLED_" + testCaseName);
            } else {
                foundTestCases.emplace_back(testCaseName);
            }
        }
        return foundTestCases;
    }
};

#endif