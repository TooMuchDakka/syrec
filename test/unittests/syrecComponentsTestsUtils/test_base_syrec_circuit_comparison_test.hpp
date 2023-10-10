#ifndef TEST_BASE_SYREC_TEST_HPP
#define TEST_BASE_SYREC_TEST_HPP
#pragma once

#include "core/syrec/program.hpp"
#include "core/syrec/parser/utils/message_utils.hpp"
#include "core/syrec/parser/utils/syrec_ast_dump_utils.hpp"

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <fstream>
#include <cstdlib>
#include <cerrno>
#include <nlohmann/json.hpp>
#include <fmt/core.h>

using json = nlohmann::json;

class BaseSyrecCircuitComparisonTestFixture: public testing::Test {
protected:
    static constexpr unsigned int defaultSignalBitwidth               = 16;
    static constexpr auto         cJsonKeyCircuit                     = "circuit";
    static constexpr auto         cJsonKeyExpectedCircuitOutput       = "expectedCircuit";
    static constexpr auto         cJsonKeyExpectedCircuitOutputs      = "expectedCircuits";
    static constexpr auto         cJsonKeyEnabledOptimizations        = "optimizations";
    static constexpr auto         cJsonKeyExpectedErrors              = "errors";
    static constexpr auto         expectedErrorMessageFormat          = "-- line {0:d} col {1:d}: {2:s}";
    static constexpr auto         cJsonErrorLineJsonKey               = "line";
    static constexpr auto         cJsonErrorColumnJsonKey             = "column";
    static constexpr auto         cJsonErrorExpectedMessageKey        = "message";

    static constexpr auto cJsonKeySupportingBroadcastingExpressionOperands = "exprOperandsBroadcastingON";
    static constexpr auto cJsonKeySupportingBroadCastingAssignmentOperands = "assignmentOperandsBroadcastingON";
    static constexpr auto cJsonKeyDeadCodeEliminationFlag                  = "deadCodeElimON";
    static constexpr auto cJsonKeyPerformConstantPropagationFlag           = "constantPropON";
    static constexpr auto cJsonKeyNoAdditionalLineSynthesisFlag            = "noAddLineSynON";
    static constexpr auto cJsonKeyOperationStrengthReductionEnabled        = "opStrengthReductionON";
    static constexpr auto cJsonKeyDeadStoreEliminationEnabled              = "deadStoreElimON";
    static constexpr auto cJsonKeyCombineRedundantInstructionsEnabled      = "combineInstructionsON";
    static constexpr auto cJsonKeyReassociateExpressionFlag                = "reassociateExprON";
    static constexpr auto cJsonKeyMultiplicationSimplificationMethod       = "multiplySimplifyON";
    static constexpr auto cJsonKeyLoopUnrollMaxNestingLevel                = "loopUnrollMaxNestingLvl";
    static constexpr auto cJsonKeyLoopUnrollMaxUnrollCountPerLoop          = "loopUnrollMaxUnrollCnt";
    static constexpr auto cJsonKeyLoopUnrollMaxAllowedTotalSize            = "loopUnrollMaxTotalSize";
    static constexpr auto cJsonKeyLoopUnrollForceUnrollFlag                = "loopUnrollForceUnrollON";
    static constexpr auto cJsonKeyLoopUnrollAllowRemainderFlag             = "loopUnrollAllowRemainderON";

    static constexpr auto cJsonKeyMultiplicationSimplificationViaBinaryMethod = "binaryMethod";
    static constexpr auto cJsonKeyMultiplicationSimplificationViaBinaryFactoringMethod = "binaryFactoringMethod";

    syrecAstDumpUtils::SyrecASTDumper astDumper;
    syrec::ReadProgramSettings        config;
    syrec::program                    parserPublicInterface;

    std::string circuitToOptimize;
    std::vector<std::string> expectedOptimizedCircuits;

    struct ExpectedError {
        std::size_t line;
        std::size_t column;
        std::string message;
    };
    std::vector<std::string> expectedErrors;

    BaseSyrecCircuitComparisonTestFixture():
        astDumper(true) {}

    void determineExpectedErrorsFromJson(const nlohmann::json& testCaseJsonData) {
        if (!testCaseJsonData.contains(cJsonKeyExpectedErrors)) {
            return;
        }

        const auto& errorJsonArrayObject = testCaseJsonData.at(cJsonKeyExpectedErrors);
        ASSERT_TRUE(errorJsonArrayObject.is_array()) << "Expected entry with key '" << cJsonKeyExpectedErrors << "' to by an array";

        for (auto& errorJsonObject: errorJsonArrayObject) {
            ASSERT_TRUE(errorJsonObject.is_object()) << "Expected test case data to a json object";
            ASSERT_TRUE(errorJsonObject.contains(cJsonErrorLineJsonKey)) << "Required entry with key '" << cJsonErrorLineJsonKey << "' was not found";
            ASSERT_TRUE(errorJsonObject.contains(cJsonErrorColumnJsonKey)) << "Required entry with key '" << cJsonErrorColumnJsonKey << "' was not found";
            ASSERT_TRUE(errorJsonObject.contains(cJsonErrorExpectedMessageKey)) << "Required entry with key '" << cJsonErrorExpectedMessageKey << "' was not found";

            ASSERT_TRUE(errorJsonObject.at(cJsonErrorLineJsonKey).is_number_unsigned()) << "Expected entry with key '" << cJsonErrorLineJsonKey << "' to be an unsigned integer";
            ASSERT_TRUE(errorJsonObject.at(cJsonErrorColumnJsonKey).is_number_unsigned()) << "Expected entry with key '" << cJsonErrorColumnJsonKey << "' to be an unsigned integer";
            ASSERT_TRUE(errorJsonObject.at(cJsonErrorExpectedMessageKey).is_string()) << "Expected entry with key '" << cJsonErrorExpectedMessageKey << "' to be a string";

            const auto& expectedError = ExpectedError({
                    errorJsonObject.at(cJsonErrorLineJsonKey).get<unsigned int>(),
                    errorJsonObject.at(cJsonErrorColumnJsonKey).get<unsigned int>(),
                    errorJsonObject.at(cJsonErrorExpectedMessageKey).get<std::string>()
            });
            ASSERT_NO_THROW(expectedErrors.emplace_back(fmt::format(expectedErrorMessageFormat, expectedError.line, expectedError.column, expectedError.message)));
        }
    }

    void determineExpectedCircuitsFromJson(const nlohmann::json& testCaseJsonData, std::vector<std::string>& expectedOptimizedCircuits) {
        if (testCaseJsonData.contains(cJsonKeyExpectedCircuitOutput)) {
            ASSERT_TRUE(testCaseJsonData.at(cJsonKeyExpectedCircuitOutput).is_string()) << "Expected entry with key '" << cJsonKeyExpectedCircuitOutput << "' to by a string";
            expectedOptimizedCircuits.emplace_back(testCaseJsonData.at(cJsonKeyExpectedCircuitOutput).get<std::string>());
        } else if (testCaseJsonData.contains(cJsonKeyExpectedCircuitOutputs)) {
            ASSERT_TRUE(testCaseJsonData.at(cJsonKeyExpectedCircuitOutputs).is_array()) << "Expected entry with key '" << cJsonKeyExpectedCircuitOutputs << "' to be an array";
            const auto& expectedCircuitOutputsJsonData        = testCaseJsonData.at(cJsonKeyExpectedCircuitOutputs);
            const auto wereAllDefinedExpectedCircuitsStrings = std::all_of(
                expectedCircuitOutputsJsonData.cbegin(),
                expectedCircuitOutputsJsonData.cend(),
                [](const nlohmann::json& jsonArrayElementJsonData) {
                    return jsonArrayElementJsonData.is_string();
            });
            ASSERT_TRUE(wereAllDefinedExpectedCircuitsStrings) << "Expected all entries of json array element '" << cJsonKeyExpectedCircuitOutputs << "' to be json strings";
            const auto& foundExpectedOptimizedCircuits = expectedCircuitOutputsJsonData.get<std::vector<std::string>>();
            expectedOptimizedCircuits.insert(
                expectedOptimizedCircuits.cend(),
                foundExpectedOptimizedCircuits.begin(),
                foundExpectedOptimizedCircuits.end()
            );
        } else {
            expectedOptimizedCircuits.emplace_back(circuitToOptimize);
        }
    }

     static void compareExpectedAndActualErrors(const std::vector<std::string>& expectedErrors, const std::vector<std::string>& actualErrorsInUnifiedFormat) {
        if (expectedErrors.size() != actualErrorsInUnifiedFormat.size()) {
            std::ostringstream expectedErrorsBuffer;
            std::ostringstream actualErrorsBuffer;

            std::copy(expectedErrors.cbegin(), expectedErrors.cend(), InfixIterator<std::string>(expectedErrorsBuffer, "\n"));
            std::copy(actualErrorsInUnifiedFormat.cbegin(), actualErrorsInUnifiedFormat.cend(), InfixIterator<std::string>(actualErrorsBuffer, "\n"));

            FAIL() << "Expected " << expectedErrors.size() << " errors but only " << actualErrorsInUnifiedFormat.size() << " were found!\nExpected: " << expectedErrorsBuffer.str() << "\nActual: " << actualErrorsBuffer.str();
        }
        
        for (size_t errorIdx = 0; errorIdx < expectedErrors.size(); ++errorIdx) {
            ASSERT_EQ(expectedErrors.at(errorIdx), actualErrorsInUnifiedFormat.at(errorIdx)) << "Expected error: " << expectedErrors.at(errorIdx) << "| Actual Error: " << actualErrorsInUnifiedFormat.at(errorIdx);
        }
    }

    void SetUp() override {
        const std::string testCaseJsonKey  = extractTestCaseNameFromParameter(getTestCaseJsonKey());
        const auto&       testDataFilePath = lookupTestCaseFilePathFromTestCaseName(getTestCaseJsonKey());
        std::ifstream configFileStream(testDataFilePath, std::ios_base::in);
        ASSERT_TRUE(configFileStream.good()) << "Could not open test data json file @ "
                                             << testDataFilePath;

        const nlohmann::json parsedJson = nlohmann::json::parse(configFileStream);
        ASSERT_TRUE(parsedJson.contains(testCaseJsonKey)) << "Required entry for given test case with key '" << testCaseJsonKey << "' was not found";

        const nlohmann::json testcaseJsonData = parsedJson[testCaseJsonKey];
        ASSERT_TRUE(testcaseJsonData.is_object()) << "Expected test case data for test with key '" << testCaseJsonKey << "' to be in the form of a json object";

        ASSERT_TRUE(testcaseJsonData.contains(cJsonKeyCircuit)) << "Required entry with key '" << cJsonKeyCircuit << "' was not found";
        ASSERT_TRUE(testcaseJsonData.at(cJsonKeyCircuit).is_string()) << "Expected entry with key '" << cJsonKeyCircuit << "' to by a string";
        circuitToOptimize = testcaseJsonData.at(cJsonKeyCircuit).get<std::string>();
        ASSERT_NO_FATAL_FAILURE(determineExpectedCircuitsFromJson(testcaseJsonData, expectedOptimizedCircuits));
        ASSERT_NO_FATAL_FAILURE(determineExpectedErrorsFromJson(testcaseJsonData));

        std::map<OptimizerOption, std::string> userDefinedOptions;
        if (testcaseJsonData.contains(cJsonKeyEnabledOptimizations)) {
            ASSERT_TRUE(testcaseJsonData.at(cJsonKeyEnabledOptimizations).is_object()) << "Expected entry with key '" << cJsonKeyEnabledOptimizations << "' to be an object";
            // TODO: Refactor checking preconditions, i.e. values do match required type (i.e. json string can be converted to bool or number
            for (const auto& [_, value]: testcaseJsonData.at(cJsonKeyEnabledOptimizations).items()) {
                ASSERT_TRUE(value.is_string()) << "Expected values of all user defined optimization options to be defined as json strings";
            }
            userDefinedOptions = loadDefinedOptimizationOptions(testcaseJsonData.at(cJsonKeyEnabledOptimizations));
        }
        std::optional<syrec::ReadProgramSettings> loadedUserOptimizationOptions;
        ASSERT_NO_FATAL_FAILURE(createParserConfigFromOptions(userDefinedOptions, loadedUserOptimizationOptions)) << "Expected to be able to parse user supplied optimization options";
        if (!loadedUserOptimizationOptions.has_value()) {
            FAIL();
        }
        config                                    = mergeDefaultAndUserDefinedParserConfigOptions(getDefaultParserConfig(), *loadedUserOptimizationOptions, userDefinedOptions);
    }

    [[nodiscard]] virtual std::string                getTestCaseJsonKey() const = 0;
    [[nodiscard]] virtual std::string                getTestDataFilePath() const = 0;
    [[nodiscard]] virtual std::vector<std::string>   getTestDataFilePaths() const { return { getTestDataFilePath()}; }
    [[nodiscard]] virtual syrec::ReadProgramSettings getDefaultParserConfig() const {
        return syrec::ReadProgramSettings(defaultSignalBitwidth);
    }

    enum OptimizerOption {
        SupportingBroadCastingExpressionOperandsFlag,
        SupportingBroadCastingAssignmentOperands,
        DeadCodeEliminationFlag,
        PerformConstantPropagationFlag,
        NoAdditionalLineSynthesisFlag,
        OperationStrengthReductionEnabled,
        DeadStoreEliminationEnabled,
        CombineRedundantInstructionsEnabled,
        ReassociateExpressionFlag,
        MultiplicationSimplificationMethod,
        LoopUnrollMaxNestingLevel,
        LoopUnrollMaxUnrollCountPerLoop,
        LoopUnrollMaxAllowedTotalSize,
        LoopForceUnrollFlag,
        LoopUnrollAllowRemainderFlag
    };
    [[nodiscard]] std::map<OptimizerOption, std::string> loadDefinedOptimizationOptions(const nlohmann::json& testCaseDataJsonObject) const {
        std::map<OptimizerOption, std::string> userDefinedOptions;
        for (const auto& [key, value]: testCaseDataJsonObject.items()) {
            const auto& mappedToOptimizerOption = tryMapOptimizationJsonKeyToOptimizerOption(key);
            if (!mappedToOptimizerOption.has_value()) {
                continue;
            }
            userDefinedOptions.insert_or_assign(*mappedToOptimizerOption, value.get<std::string>());
        }
        return userDefinedOptions;
    }

    /*
     * Return type is null because otherwise GTEST assertions cannot be used, see: https://google.github.io/googletest/advanced.html#assertion-placement
     */
    void createParserConfigFromOptions(const std::map<OptimizerOption, std::string>& loadedOptimizationOptions, std::optional<syrec::ReadProgramSettings>& parsedProgramSettings) const {
        syrec::ReadProgramSettings generatedConfig;

        std::size_t                     maxUnrollCountPerLoop = 0;
        std::size_t                     maxAllowedNestingLevelOfInnerLoops = 0;
        std::size_t                     maxAllowedTotalLoopSize = 0;
        bool                            allowRemainderLoop = false;
        bool                            forceUnrollAll = false;
        bool                            wereLoopUnrollConfigOptionsDefined = false;

        for (const auto& [key, value] : loadedOptimizationOptions) {
            switch (key) {
                case SupportingBroadCastingExpressionOperandsFlag:
                case SupportingBroadCastingAssignmentOperands:
                    break;
                case DeadCodeEliminationFlag: {
                    const auto parsedDeadCodeEliminationFlagValue = tryParseStringToNumber(value);
                    ASSERT_TRUE(parsedDeadCodeEliminationFlagValue.has_value()) << "Failed to map " << value << " to a valid dead code elimination value";
                    generatedConfig.deadCodeEliminationEnabled = *parsedDeadCodeEliminationFlagValue > 0;
                    break;   
                }
                case CombineRedundantInstructionsEnabled: {
                    const auto parsedCombineRedundantInstructionsFlagValue = tryParseStringToNumber(value);
                    ASSERT_TRUE(parsedCombineRedundantInstructionsFlagValue.has_value()) << "Failed to map " << value << " to a valid combine redundant instructions value";
                    generatedConfig.combineRedundantInstructions = *parsedCombineRedundantInstructionsFlagValue > 0;
                    break;
                }
                case PerformConstantPropagationFlag: {
                    const auto parsedConstantPropagationFlagValue = tryParseStringToNumber(value);
                    ASSERT_TRUE(parsedConstantPropagationFlagValue.has_value()) << "Failed to map " << value << " to a valid constant propagation value";
                    generatedConfig.performConstantPropagation = *parsedConstantPropagationFlagValue > 0;
                    break;
                }
                case NoAdditionalLineSynthesisFlag: {
                    const auto parsedNoAdditionalLineSynthesisFlagValue = tryParseStringToNumber(value);
                    ASSERT_TRUE(parsedNoAdditionalLineSynthesisFlagValue.has_value()) << "Failed to map " << value << " to a valid no additional line synthesis value";
                    generatedConfig.noAdditionalLineOptimizationEnabled = *parsedNoAdditionalLineSynthesisFlagValue > 0;
                    break;
                }
                case OperationStrengthReductionEnabled: {
                    const auto parsedOperationStrengthReductionFlagValue = tryParseStringToNumber(value);
                    ASSERT_TRUE(parsedOperationStrengthReductionFlagValue.has_value()) << "Failed to map " << value << " to a valid operation strength reduction value";
                    generatedConfig.operationStrengthReductionEnabled = *parsedOperationStrengthReductionFlagValue > 0;
                    break;
                }
                case DeadStoreEliminationEnabled: {
                    const auto parsedDeadStoreEliminationEnabledFlagValue = tryParseStringToNumber(value);
                    ASSERT_TRUE(parsedDeadStoreEliminationEnabledFlagValue.has_value()) << "Failed to map " << value << " to a valid dead store elimination value";
                    generatedConfig.deadStoreEliminationEnabled = *parsedDeadStoreEliminationEnabledFlagValue > 0;
                    break;
                }
                case ReassociateExpressionFlag: {
                    const auto parsedReassociateExpressionFlagValue = tryParseStringToNumber(value);
                    ASSERT_TRUE(parsedReassociateExpressionFlagValue.has_value()) << "Failed to map " << value << " to a valid reassociate expression value";
                    generatedConfig.reassociateExpressionEnabled = *parsedReassociateExpressionFlagValue > 0;
                    break;
                }
                case MultiplicationSimplificationMethod: {
                    const auto mappedToSimplificationMethod = tryMapToMultiplicationSimplificationMethod(value);
                    ASSERT_TRUE(mappedToSimplificationMethod.has_value()) << "Failed to map " << value << " to a valid multiplication simplification method";
                    generatedConfig.multiplicationSimplificationMethod = *mappedToSimplificationMethod;
                    break;
                }
                case LoopUnrollMaxNestingLevel: {
                    const auto parsedMaxNestingLevelValue = tryParseStringToNumber(value);
                    ASSERT_TRUE(parsedMaxNestingLevelValue.has_value()) << "Failed to map given maximum allowed nesting level of loops to a number: " << value;
                    wereLoopUnrollConfigOptionsDefined = true;
                    maxAllowedNestingLevelOfInnerLoops = *parsedMaxNestingLevelValue;
                    break;
                }
                case LoopUnrollMaxUnrollCountPerLoop: {
                    const auto parsedMaxUnrollCountPerLoopValue = tryParseStringToNumber(value);
                    ASSERT_TRUE(parsedMaxUnrollCountPerLoopValue.has_value()) << "Failed to map given maximum allowed unroll level of loops to a number: " << value;
                    wereLoopUnrollConfigOptionsDefined = true;
                    maxUnrollCountPerLoop              = *parsedMaxUnrollCountPerLoopValue;
                    break;
                }
                case LoopUnrollMaxAllowedTotalSize: {
                    const auto parsedMaxAllowedTotalLoopSize = tryParseStringToNumber(value);
                    ASSERT_TRUE(parsedMaxAllowedTotalLoopSize.has_value()) << "Failed to map given maximum allowed total loop size to a number: " << value;
                    wereLoopUnrollConfigOptionsDefined = true;
                    maxAllowedTotalLoopSize            = *parsedMaxAllowedTotalLoopSize;
                    break;
                }
                case LoopForceUnrollFlag: {
                    wereLoopUnrollConfigOptionsDefined = true;
                    const auto parsedForceUnrollFlag   = tryParseStringToNumber(value);
                    ASSERT_TRUE(parsedForceUnrollFlag.has_value()) << "Failed to map given force unrolling of loops flag value to a number: " << value;
                    forceUnrollAll                     = *parsedForceUnrollFlag > 0;
                    break;   
                }
                case LoopUnrollAllowRemainderFlag: {
                    wereLoopUnrollConfigOptionsDefined = true;
                    const auto parsedAllowLoopRemainderFlag   = tryParseStringToNumber(value);
                    ASSERT_TRUE(parsedAllowLoopRemainderFlag.has_value()) << "Failed to map given allow remainder of loop after unroll flag value to a number: " << value;
                    allowRemainderLoop = *parsedAllowLoopRemainderFlag > 0;
                    break;
                }
            }
        }

        if (wereLoopUnrollConfigOptionsDefined) {
            generatedConfig.optionalLoopUnrollConfig.emplace(optimizations::LoopOptimizationConfig(maxUnrollCountPerLoop, maxAllowedNestingLevelOfInnerLoops, maxAllowedTotalLoopSize, allowRemainderLoop, forceUnrollAll));
        }
        parsedProgramSettings.emplace(generatedConfig);
    }

    [[nodiscard]] static syrec::ReadProgramSettings mergeDefaultAndUserDefinedParserConfigOptions(
        const syrec::ReadProgramSettings& defaultParserConfig, 
        const syrec::ReadProgramSettings& userDefinedOptimizations, 
        const std::map<OptimizerOption, std::string>& loadedOptimizationOptions) {

        syrec::ReadProgramSettings mergedOptions;
        mergedOptions.deadCodeEliminationEnabled = chooseValueForOptionWhereUserSuppliedOptionHasHighestPriority(OptimizerOption::DeadCodeEliminationFlag, loadedOptimizationOptions, defaultParserConfig.deadCodeEliminationEnabled, userDefinedOptimizations.deadCodeEliminationEnabled);
        mergedOptions.performConstantPropagation = chooseValueForOptionWhereUserSuppliedOptionHasHighestPriority(OptimizerOption::PerformConstantPropagationFlag, loadedOptimizationOptions, defaultParserConfig.performConstantPropagation, userDefinedOptimizations.performConstantPropagation);
        mergedOptions.noAdditionalLineOptimizationEnabled = chooseValueForOptionWhereUserSuppliedOptionHasHighestPriority(OptimizerOption::NoAdditionalLineSynthesisFlag, loadedOptimizationOptions, defaultParserConfig.noAdditionalLineOptimizationEnabled, userDefinedOptimizations.noAdditionalLineOptimizationEnabled);
        mergedOptions.operationStrengthReductionEnabled   = chooseValueForOptionWhereUserSuppliedOptionHasHighestPriority(OptimizerOption::OperationStrengthReductionEnabled, loadedOptimizationOptions, defaultParserConfig.operationStrengthReductionEnabled, userDefinedOptimizations.operationStrengthReductionEnabled);
        mergedOptions.deadStoreEliminationEnabled         = chooseValueForOptionWhereUserSuppliedOptionHasHighestPriority(OptimizerOption::DeadStoreEliminationEnabled, loadedOptimizationOptions, defaultParserConfig.deadStoreEliminationEnabled, userDefinedOptimizations.deadStoreEliminationEnabled);
        mergedOptions.combineRedundantInstructions        = chooseValueForOptionWhereUserSuppliedOptionHasHighestPriority(OptimizerOption::CombineRedundantInstructionsEnabled, loadedOptimizationOptions, defaultParserConfig.combineRedundantInstructions, userDefinedOptimizations.combineRedundantInstructions);
        mergedOptions.reassociateExpressionEnabled        = chooseValueForOptionWhereUserSuppliedOptionHasHighestPriority(OptimizerOption::ReassociateExpressionFlag, loadedOptimizationOptions, defaultParserConfig.reassociateExpressionEnabled, userDefinedOptimizations.reassociateExpressionEnabled);
        mergedOptions.multiplicationSimplificationMethod  = chooseValueForOptionWhereUserSuppliedOptionHasHighestPriority(OptimizerOption::MultiplicationSimplificationMethod, loadedOptimizationOptions, defaultParserConfig.multiplicationSimplificationMethod, userDefinedOptimizations.multiplicationSimplificationMethod);
        
        if (userDefinedOptimizations.optionalLoopUnrollConfig.has_value() && !defaultParserConfig.optionalLoopUnrollConfig.has_value()) {
            mergedOptions.optionalLoopUnrollConfig.emplace(*userDefinedOptimizations.optionalLoopUnrollConfig);
        } else if (userDefinedOptimizations.optionalLoopUnrollConfig.has_value() && defaultParserConfig.optionalLoopUnrollConfig.has_value()) {
            const auto maxAllowedTotalLoopSize = chooseValueForOptionWhereUserSuppliedOptionHasHighestPriority(OptimizerOption::LoopUnrollMaxAllowedTotalSize, loadedOptimizationOptions, defaultParserConfig.optionalLoopUnrollConfig->maxAllowedTotalLoopSize, userDefinedOptimizations.optionalLoopUnrollConfig->maxAllowedTotalLoopSize);
            const auto maxAllowedLoopNestingLevel = chooseValueForOptionWhereUserSuppliedOptionHasHighestPriority(OptimizerOption::LoopUnrollMaxNestingLevel, loadedOptimizationOptions, defaultParserConfig.optionalLoopUnrollConfig->maxAllowedNestingLevelOfInnerLoops, userDefinedOptimizations.optionalLoopUnrollConfig->maxAllowedNestingLevelOfInnerLoops);
            const auto maxUnrolledLoopIterations  = chooseValueForOptionWhereUserSuppliedOptionHasHighestPriority(OptimizerOption::LoopUnrollMaxUnrollCountPerLoop, loadedOptimizationOptions, defaultParserConfig.optionalLoopUnrollConfig->maxUnrollCountPerLoop, userDefinedOptimizations.optionalLoopUnrollConfig->maxUnrollCountPerLoop);
            const auto allowLoopRemainder         = chooseValueForOptionWhereUserSuppliedOptionHasHighestPriority(OptimizerOption::LoopUnrollAllowRemainderFlag, loadedOptimizationOptions, defaultParserConfig.optionalLoopUnrollConfig->allowRemainderLoop, userDefinedOptimizations.optionalLoopUnrollConfig->allowRemainderLoop);
            const auto forceUnrollLoops           = chooseValueForOptionWhereUserSuppliedOptionHasHighestPriority(OptimizerOption::LoopForceUnrollFlag, loadedOptimizationOptions, defaultParserConfig.optionalLoopUnrollConfig->forceUnrollAll, userDefinedOptimizations.optionalLoopUnrollConfig->forceUnrollAll);
            mergedOptions.optionalLoopUnrollConfig.emplace(optimizations::LoopOptimizationConfig(maxUnrolledLoopIterations, maxAllowedLoopNestingLevel, maxAllowedTotalLoopSize, allowLoopRemainder, forceUnrollLoops));
        } else if (!userDefinedOptimizations.optionalLoopUnrollConfig.has_value() && defaultParserConfig.optionalLoopUnrollConfig.has_value()) {
            mergedOptions.optionalLoopUnrollConfig.emplace(*defaultParserConfig.optionalLoopUnrollConfig);
        }
        return mergedOptions;
    }


    void performParsingAndCompareExpectedAndActualCircuit() {
        std::string errorsFromParsedCircuit;
        ASSERT_NO_THROW(errorsFromParsedCircuit = parserPublicInterface.readFromString(circuitToOptimize, config));
        if (expectedErrors.empty()) {
            ASSERT_TRUE(errorsFromParsedCircuit.empty()) << "Expected to be able to parse given circuit without errors";

            std::string stringifiedProgram;
            ASSERT_NO_THROW(stringifiedProgram = astDumper.stringifyModules(parserPublicInterface.modules())) << "Failed to stringify parsed modules";
            ASSERT_THAT(stringifiedProgram, testing::AnyOfArray(expectedOptimizedCircuits));
        }
        else {
            ASSERT_NO_THROW(compareExpectedAndActualErrors(expectedErrors, messageUtils::tryDeserializeStringifiedMessagesFromString(errorsFromParsedCircuit))) << "Missmatch between expected and actual errors";
        }
    }

    [[nodiscard]] std::optional<OptimizerOption> tryMapOptimizationJsonKeyToOptimizerOption(const std::string& jsonKeyOfOptimizationOption) const {
        if (jsonKeyOfOptimizationOption == cJsonKeySupportingBroadcastingExpressionOperands) return std::make_optional(OptimizerOption::SupportingBroadCastingExpressionOperandsFlag);
        if (jsonKeyOfOptimizationOption == cJsonKeySupportingBroadCastingAssignmentOperands) return std::make_optional(OptimizerOption::SupportingBroadCastingAssignmentOperands);
        if (jsonKeyOfOptimizationOption == cJsonKeyDeadCodeEliminationFlag) return std::make_optional(OptimizerOption::DeadCodeEliminationFlag);
        if (jsonKeyOfOptimizationOption == cJsonKeyPerformConstantPropagationFlag) return std::make_optional(OptimizerOption::PerformConstantPropagationFlag);
        if (jsonKeyOfOptimizationOption == cJsonKeyNoAdditionalLineSynthesisFlag) return std::make_optional(OptimizerOption::NoAdditionalLineSynthesisFlag);
        if (jsonKeyOfOptimizationOption == cJsonKeyOperationStrengthReductionEnabled) return std::make_optional(OptimizerOption::OperationStrengthReductionEnabled);
        if (jsonKeyOfOptimizationOption == cJsonKeyDeadStoreEliminationEnabled) return std::make_optional(OptimizerOption::DeadStoreEliminationEnabled);
        if (jsonKeyOfOptimizationOption == cJsonKeyCombineRedundantInstructionsEnabled) return std::make_optional(OptimizerOption::CombineRedundantInstructionsEnabled);
        if (jsonKeyOfOptimizationOption == cJsonKeyReassociateExpressionFlag) return std::make_optional(OptimizerOption::ReassociateExpressionFlag);
        if (jsonKeyOfOptimizationOption == cJsonKeyMultiplicationSimplificationMethod) return std::make_optional(OptimizerOption::MultiplicationSimplificationMethod);
        if (jsonKeyOfOptimizationOption == cJsonKeyLoopUnrollMaxNestingLevel) return std::make_optional(OptimizerOption::LoopUnrollMaxNestingLevel);
        if (jsonKeyOfOptimizationOption == cJsonKeyLoopUnrollMaxUnrollCountPerLoop) return std::make_optional(OptimizerOption::LoopUnrollMaxUnrollCountPerLoop);
        if (jsonKeyOfOptimizationOption == cJsonKeyLoopUnrollMaxAllowedTotalSize) return std::make_optional(OptimizerOption::LoopUnrollMaxAllowedTotalSize);
        if (jsonKeyOfOptimizationOption == cJsonKeyLoopUnrollForceUnrollFlag) return std::make_optional(OptimizerOption::LoopForceUnrollFlag);
        if (jsonKeyOfOptimizationOption == cJsonKeyLoopUnrollAllowRemainderFlag) return std::make_optional(OptimizerOption::LoopUnrollAllowRemainderFlag);
        return std::nullopt;
    }

    // TODO:
    [[nodiscard]] std::optional<optimizations::MultiplicationSimplificationMethod> tryMapToMultiplicationSimplificationMethod(const std::string_view& stringValue) const {
        if (stringValue == cJsonKeyMultiplicationSimplificationViaBinaryMethod) return std::make_optional(optimizations::MultiplicationSimplificationMethod::BinarySimplification);
        if (stringValue == cJsonKeyMultiplicationSimplificationViaBinaryFactoringMethod) return std::make_optional(optimizations::MultiplicationSimplificationMethod::BinarySimplificationWithFactoring);
        return std::nullopt;
    }

    [[nodiscard]] std::optional<unsigned int> tryParseStringToNumber(const std::string_view& stringToBeParsed) const {
        if (!std::all_of(stringToBeParsed.cbegin(), stringToBeParsed.cend(), [](const char character) { return std::isdigit(static_cast<unsigned int>(character)); })) {
            return std::nullopt;
        }
        char* end = nullptr;
        const auto numberParsedFromString = std::strtoul(stringToBeParsed.data(), &end, 10);
        if (errno == ERANGE) {
            errno = 0;
            return std::nullopt;
        }
        if (numberParsedFromString <= std::numeric_limits<unsigned int>::max()) {
            return std::make_optional(static_cast<unsigned int>(numberParsedFromString));
        }

        return std::nullopt;
    }

    [[nodiscard]] virtual std::string lookupTestCaseFilePathFromTestCaseName(const std::string_view&) const {
        return getTestDataFilePath();
    }

    [[nodiscard]] virtual std::string extractTestCaseNameFromParameter(const std::string& parameter) const {
        return std::string(parameter);
    }

    template<typename T>
    [[nodiscard]] static T chooseValueForOptionWhereUserSuppliedOptionHasHighestPriority(OptimizerOption option, const std::map<OptimizerOption, std::string>& userSuppliedOptions, T& defaultValue, T& userSuppliedValue) {
        if (userSuppliedOptions.count(option) == 0) {
            return defaultValue;
        }
        return userSuppliedValue;
    }
};

#endif