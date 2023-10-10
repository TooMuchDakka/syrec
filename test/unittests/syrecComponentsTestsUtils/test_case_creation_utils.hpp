#ifndef TEST_CASE_CREATION_UTILS_HPP
#define TEST_CASE_CREATION_UTILS_HPP
#pragma once

#include <fstream>
#include <vector>
#include <string>
#include <unordered_set>

#include <nlohmann/json.hpp>

namespace syrecTestUtils {
    constexpr auto notAllowedTestNameCharacter              = '-';
    constexpr auto testNameDelimiterSymbol                  = '_';
    constexpr auto gTestIgnorePrefix                        = "DISABLED";
    constexpr auto testCaseDataParsingFailedNotAnJsonObject = "DISABLED_FAILED_TO_LOAD_TEST_DATA_NOT_JSON_OBJECT";
    constexpr auto testCaseDataParsingFailed                = "DISABLED_FAILED_TO_PARSE_TEST_DATA_JSON";
    constexpr auto testCaseDataLoadFromFileFailed           = "DISABLED_FAILED_TO_LOAD_TEST_DATA_FROM_FILE";

    /*
     * Only non-empty ASCII alphanumeric characters without underscores are supported as tests names
     * see https://google.github.io/googletest/advanced.html#specifying-names-for-value-parameterized-test-parameters
     */ 
    inline bool doesStringContainOnlyAsciiCharactersOrUnderscores(const std::string_view& stringToCheck) {
        return std::all_of(
                stringToCheck.cbegin(),
                stringToCheck.cend(),
                [](const char characterToCheck) {
                    return std::isalnum(characterToCheck) || characterToCheck == testNameDelimiterSymbol;
                });
    }

    inline std::vector<std::string> loadTestCasesNamesFromFile(const std::string& testDataJsonFilename, const std::unordered_set<std::string>& testCasesToIgnore) {
        std::ifstream testDataFileStream(testDataJsonFilename, std::ios_base::in);
        if (!testDataFileStream.good()) {
            return {testCaseDataLoadFromFileFailed};
        }

        const nlohmann::json parsedJson = nlohmann::json::parse(testDataFileStream, nullptr, false);
        if (parsedJson.is_discarded()) {
            return {testCaseDataParsingFailed };
        }
        if (!parsedJson.is_object()) {
            return {testCaseDataParsingFailedNotAnJsonObject };
        }
        
        std::vector<std::string>    foundTestCases;
        std::size_t                 testCaseIdx = 0;
        for (const auto& testCaseKVPair: parsedJson.items()) {
            const auto& testCaseName = testCaseKVPair.key();
            if (testCaseName.empty()) {
                continue;
            }

            if (doesStringContainOnlyAsciiCharactersOrUnderscores(testCaseName)) {
                if (testCasesToIgnore.count(testCaseName)) {
                    foundTestCases.emplace_back(std::string(gTestIgnorePrefix) + testNameDelimiterSymbol + testCaseName);
                }
                else {
                    foundTestCases.emplace_back(testCaseName);   
                }
            } else {
                foundTestCases.emplace_back(std::string(gTestIgnorePrefix) + testNameDelimiterSymbol + "Test" + std::to_string(testCaseIdx) + "ContainedNonAsciiOrUnderscoreCharacter");
            }
            testCaseIdx++;
        }
        return foundTestCases;
    }

    inline std::string extractFileNameWithoutExtensionAndPath(const std::string_view& filePath) {
        const auto& indexOfLastSlash = filePath.find_last_of('/');
        if (indexOfLastSlash == std::string_view::npos) {
            return "";
        }
        const auto& indexOfExtensionDot = filePath.find_first_of('.', indexOfLastSlash + 1);
        if (indexOfExtensionDot == std::string_view::npos) {
            return "";
        }
        const auto& fileNameLength = (indexOfExtensionDot - indexOfLastSlash) - 1;
        return std::string(filePath.substr(indexOfLastSlash + 1, fileNameLength));
    }

    inline std::vector<std::string> loadTestCaseNamesFromFiles(const std::vector<std::string>& testDataJsonFileNames, const std::unordered_map<std::string, std::unordered_set<std::string>>& testCasesToIgnoreLookupPerFile) {
        std::vector<std::string> loadedTestCases;
        for (std::size_t i = 0; i < testDataJsonFileNames.size(); ++i) {
            const auto& testDataFileName                = testDataJsonFileNames.at(i);
            const auto& fileNameWithoutPathAndExtension = extractFileNameWithoutExtensionAndPath(testDataFileName);
            const auto& testCasesToIgnore               = testCasesToIgnoreLookupPerFile.count(fileNameWithoutPathAndExtension)
                ? testCasesToIgnoreLookupPerFile.at(fileNameWithoutPathAndExtension)
                : std::unordered_set<std::string>();
            
            const auto& loadedTestCasesFromFile   = loadTestCasesNamesFromFile(testDataFileName, testCasesToIgnore);

            std::transform(
                    loadedTestCasesFromFile.cbegin(),
                    loadedTestCasesFromFile.cend(),
                    std::back_inserter(loadedTestCases),
                    [&fileNameWithoutPathAndExtension](const std::string& testCaseName) {
                        std::string prefix;
                        std::string testCaseNamePostfix = testNameDelimiterSymbol + testCaseName;
                        if (!testCaseName.find_first_of(gTestIgnorePrefix)) {
                            const auto& ignorePrefixDelimiterPosition            = testCaseName.find_first_of(testNameDelimiterSymbol);
                            const auto& testCaseNameWithoutIgnoreDelimiterLength = testCaseName.size() - ignorePrefixDelimiterPosition;
                            testCaseNamePostfix                                  = testCaseName.substr(ignorePrefixDelimiterPosition, testCaseNameWithoutIgnoreDelimiterLength);
                            prefix                                               = testCaseName.substr(0, ignorePrefixDelimiterPosition + 1);
                        }
                        return prefix + fileNameWithoutPathAndExtension + testCaseNamePostfix;
                    });
        }
        return loadedTestCases;
    }
};

#endif