#ifndef TEST_BASE_SYREC_TEST_HPP
#define TEST_BASE_SYREC_TEST_HPP
#pragma once

#include "core/syrec/program.hpp"
#include "core/syrec/parser/utils/syrec_ast_dump_utils.hpp"

#include "gtest/gtest.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class BaseSyrecCircuitComparisonTestFixture: public testing::Test {
protected:
    const unsigned int defaultSignalBitwidth         = 16;
    const std::string cJsonKeyCircuit               = "circuit";
    const std::string cJsonKeyExpectedCircuitOutput = "expectedCircuit";
    const std::string cJsonKeyEnabledOptimizations  = "enabledOptimizations";

    const std::string cJsonKeySupportingBroadcastingExpressionOperands = "exprOperandsBroadcastingON";
    const std::string cJsonKeySupportingBroadCastingAssignmentOperands = "assignmentOperandsBroadcastingON";
    const std::string cJsonKeyDeadCodeEliminationFlag                  = "deadCodeElimON";
    const std::string cJsonKeyPerformConstantPropagationFlag           = "constantPropON";
    const std::string cJsonKeyNoAdditionalLineSynthesisFlag            = "noAddLineSynON";
    const std::string cJsonKeyOperationStrengthReductionEnabled        = "opStrengthReductionON";
    const std::string cJsonKeyReassociateExpressionFlag                = "reassociateExprON";
    const std::string cJsonKeyMultiplicationSimplificationMethod       = "multiplySimplifyON";
    const std::string cJsonKeyLoopUnrollMaxNestingLevel                = "loopUnrollMaxNestingLvl";
    const std::string cJsonKeyLoopUnrollMaxUnrollCountPerLoop          = "loopUnrollMaxUnrollCnt";
    const std::string cJsonKeyLoopUnrollMaxAllowedTotalSize            = "loopUnrollMaxTotalSize";
    const std::string cJsonKeyLoopUnrollForceUnrollFlag                = "loopUnrollForceUnrollON";
    const std::string cJsonKeyLoopUnrollAllowRemainderFlag             = "loopUnrollAllowRemainderON";

    const std::string cJsonKeyMultiplicationSimplificationViaBinaryMethod = "binaryMethod";
    const std::string cJsonKeyMultiplicationSimplificationViaBinaryFactoringMethod = "binaryFactoringMethod";

    syrecAstDumpUtils::SyrecASTDumper astDumper;
    syrec::ReadProgramSettings        config;
    syrec::program                    parserPublicInterface;

    std::string circuitToOptimize;
    std::string expectedOptimizedCircuit;

    BaseSyrecCircuitComparisonTestFixture():
        astDumper(true) {}

    void SetUp() override {
        const std::string testCaseJsonKey = getTestCaseJsonKey();
        std::ifstream configFileStream(getTestDataFilePath(), std::ios_base::in);
        ASSERT_TRUE(configFileStream.good()) << "Could not open test data json file @ "
                                             << getTestDataFilePath();

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

        std::map<OptimizerOption, std::string> userDefinedOptions;
        if (testcaseJsonData.contains(cJsonKeyEnabledOptimizations)) {
            ASSERT_TRUE(testcaseJsonData.at(cJsonKeyEnabledOptimizations).is_object()) << "Expected entry with key '" << cJsonKeyEnabledOptimizations << "' to be an object";
            // TODO: Refactor checking preconditions, i.e. values do match required type (i.e. json string can be converted to bool or number
            for (const auto& [_, value]: testcaseJsonData.at(cJsonKeyEnabledOptimizations).items()) {
                ASSERT_TRUE(value.is_string()) << "Expected values of all user defined optimization options to be defined as json strings";
            }
            userDefinedOptions = loadDefinedOptimizationOptions(testcaseJsonData.at(cJsonKeyEnabledOptimizations));
        }
        const auto& loadedUserOptimizationOptions = createParserConfigFromOptions(userDefinedOptions);
        config                                    = mergeDefaultAndUserDefinedParserConfigOptions(getDefaultParserConfig(), loadedUserOptimizationOptions, userDefinedOptions);
    }

    [[nodiscard]] virtual std::string                getTestCaseJsonKey()  = 0;
    [[nodiscard]] virtual std::string                getTestDataFilePath() = 0;
    [[nodiscard]] virtual syrec::ReadProgramSettings getDefaultParserConfig() {
        return syrec::ReadProgramSettings(defaultSignalBitwidth);
    }

    enum OptimizerOption {
        SupportingBroadCastingExpressionOperandsFlag,
        SupportingBroadCastingAssignmentOperands,
        DeadCodeEliminationFlag,
        PerformConstantPropagationFlag,
        NoAdditionalLineSynthesisFlag,
        OperationStrengthReductionEnabled,
        ReassociateExpressionFlag,
        MultiplicationSimplificationMethod,
        LoopUnrollMaxNestingLevel,
        LoopUnrollMaxUnrollCountPerLoop,
        LoopUnrollMaxAllowedTotalSize,
        LoopUnrollForceUnrollFlag,
        LoopUnrollAllowRemainderFlag
    };
    [[nodiscard]] std::map<OptimizerOption, std::string> loadDefinedOptimizationOptions(const nlohmann::json& testCaseDataJsonObject) {
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

    [[nodiscard]] syrec::ReadProgramSettings             createParserConfigFromOptions(const std::map<OptimizerOption, std::string>& loadedOptimizationOptions) {
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
                case DeadCodeEliminationFlag:
                    generatedConfig.deadCodeEliminationEnabled = true;
                    break;
                case PerformConstantPropagationFlag:
                    generatedConfig.performConstantPropagation = true;
                    break;
                case NoAdditionalLineSynthesisFlag:
                    generatedConfig.noAdditionalLineOptimizationEnabled = true;
                    break;
                case OperationStrengthReductionEnabled:
                    generatedConfig.operationStrengthReductionEnabled = true;
                    break;
                case ReassociateExpressionFlag:
                    generatedConfig.reassociateExpressionEnabled = true;
                    break;
                case MultiplicationSimplificationMethod:
                    generatedConfig.multiplicationSimplificationMethod = tryMapToMultiplicationSimplificationMethod(value).value_or(optimizations::MultiplicationSimplificationMethod::None);
                    break;
                case LoopUnrollMaxNestingLevel:
                    wereLoopUnrollConfigOptionsDefined = true;
                    maxAllowedNestingLevelOfInnerLoops = 0;
                    break;
                case LoopUnrollMaxUnrollCountPerLoop:
                    wereLoopUnrollConfigOptionsDefined = true;
                    maxUnrollCountPerLoop = 0;
                    break;
                case LoopUnrollMaxAllowedTotalSize:
                    wereLoopUnrollConfigOptionsDefined = true;
                    maxAllowedTotalLoopSize = 0;
                    break;
                case LoopUnrollForceUnrollFlag:
                    wereLoopUnrollConfigOptionsDefined = true;
                    forceUnrollAll = true;
                    break;
                case LoopUnrollAllowRemainderFlag:
                    wereLoopUnrollConfigOptionsDefined = true;
                    allowRemainderLoop = true;
                    break;
            }
        }

        if (wereLoopUnrollConfigOptionsDefined) {
            generatedConfig.optionalLoopUnrollConfig.emplace(optimizations::LoopOptimizationConfig(maxUnrollCountPerLoop, maxAllowedNestingLevelOfInnerLoops, maxAllowedTotalLoopSize, allowRemainderLoop, forceUnrollAll));
        }
        return generatedConfig;
    }
    [[nodiscard]] syrec::ReadProgramSettings mergeDefaultAndUserDefinedParserConfigOptions(
        const syrec::ReadProgramSettings& defaultParserConfig, 
        const syrec::ReadProgramSettings& userDefinedOptimizations, 
        const std::map<OptimizerOption, std::string>& loadedOptimizationOptions) {
        return getDefaultParserConfig();
    }


    void performParsingAndCompareExpectedAndActualCircuit() {
        std::string errorsFromParsedCircuit;
        ASSERT_NO_THROW(errorsFromParsedCircuit = parserPublicInterface.readFromString(circuitToOptimize, config));
        ASSERT_TRUE(errorsFromParsedCircuit.empty()) << "Expected to be able to parse given circuit without errors";

        std::string stringifiedProgram;
        ASSERT_NO_THROW(stringifiedProgram = astDumper.stringifyModules(parserPublicInterface.modules())) << "Failed to stringify parsed modules";
        ASSERT_EQ(expectedOptimizedCircuit, stringifiedProgram);
    }

    [[nodiscard]] std::optional<OptimizerOption> tryMapOptimizationJsonKeyToOptimizerOption(const std::string& jsonKeyOfOptimizationOption) const {
        if (jsonKeyOfOptimizationOption == cJsonKeySupportingBroadcastingExpressionOperands) return std::make_optional(OptimizerOption::SupportingBroadCastingExpressionOperandsFlag);
        if (jsonKeyOfOptimizationOption == cJsonKeySupportingBroadCastingAssignmentOperands) return std::make_optional(OptimizerOption::SupportingBroadCastingAssignmentOperands);
        if (jsonKeyOfOptimizationOption == cJsonKeyDeadCodeEliminationFlag) return std::make_optional(OptimizerOption::DeadCodeEliminationFlag);
        if (jsonKeyOfOptimizationOption == cJsonKeyPerformConstantPropagationFlag) return std::make_optional(OptimizerOption::PerformConstantPropagationFlag);
        if (jsonKeyOfOptimizationOption == cJsonKeyNoAdditionalLineSynthesisFlag) return std::make_optional(OptimizerOption::NoAdditionalLineSynthesisFlag);
        if (jsonKeyOfOptimizationOption == cJsonKeyOperationStrengthReductionEnabled) return std::make_optional(OptimizerOption::OperationStrengthReductionEnabled);
        if (jsonKeyOfOptimizationOption == cJsonKeyReassociateExpressionFlag) return std::make_optional(OptimizerOption::ReassociateExpressionFlag);
        if (jsonKeyOfOptimizationOption == cJsonKeyMultiplicationSimplificationMethod) return std::make_optional(OptimizerOption::MultiplicationSimplificationMethod);
        if (jsonKeyOfOptimizationOption == cJsonKeyLoopUnrollMaxNestingLevel) return std::make_optional(OptimizerOption::LoopUnrollMaxNestingLevel);
        if (jsonKeyOfOptimizationOption == cJsonKeyLoopUnrollMaxUnrollCountPerLoop) return std::make_optional(OptimizerOption::LoopUnrollMaxUnrollCountPerLoop);
        if (jsonKeyOfOptimizationOption == cJsonKeyLoopUnrollMaxAllowedTotalSize) return std::make_optional(OptimizerOption::LoopUnrollMaxAllowedTotalSize);
        if (jsonKeyOfOptimizationOption == cJsonKeyLoopUnrollForceUnrollFlag) return std::make_optional(OptimizerOption::LoopUnrollForceUnrollFlag);
        if (jsonKeyOfOptimizationOption == cJsonKeyLoopUnrollAllowRemainderFlag) return std::make_optional(OptimizerOption::LoopUnrollAllowRemainderFlag);
        return std::nullopt;
    }

    // TODO:
    [[nodiscard]] std::optional<optimizations::MultiplicationSimplificationMethod> tryMapToMultiplicationSimplificationMethod(const std::string_view& stringValue) const {
        if (stringValue == cJsonKeyMultiplicationSimplificationViaBinaryMethod) return std::make_optional(optimizations::MultiplicationSimplificationMethod::BinarySimplification);
        if (stringValue == cJsonKeyMultiplicationSimplificationViaBinaryFactoringMethod) return std::make_optional(optimizations::MultiplicationSimplificationMethod::BinarySimplificationWithFactoring);
        return std::nullopt;
    }

private:
    // TODO: Create read program settings from config and merge with custom implementation of class
};

#endif