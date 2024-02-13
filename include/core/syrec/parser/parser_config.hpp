#ifndef PARSER_CONFIG_HPP
#define PARSER_CONFIG_HPP

#include "optimizations/loop_optimizer.hpp"
#include "optimizations/noAdditionalLineSynthesis/no_additional_line_synthesis_config.hpp"
#include "optimizations/operationSimplification/base_multiplication_simplifier.hpp"

#include <string>

namespace parser {
   

    struct ParserConfig {
        static const inline std::string defaultExpectedMainModuleName = "main";

        unsigned int                                                              cDefaultSignalWidth;
        bool                                                                      supportBroadcastingExpressionOperands;
        bool                                                                      supportBroadcastingAssignmentOperands;
        bool                                                                      reassociateExpressionsEnabled;
        bool                                                                      deadCodeEliminationEnabled;
        bool                                                                      constantPropagationEnabled;
        bool                                                                      operationStrengthReductionEnabled;
        bool                                                                      deadStoreEliminationEnabled;
        bool                                                                      combineRedundantInstructions;
        optimizations::MultiplicationSimplificationMethod                         multiplicationSimplificationMethod;
        std::optional<optimizations::LoopOptimizationConfig>                      optionalLoopUnrollConfig;
        std::optional<noAdditionalLineSynthesis::NoAdditionalLineSynthesisConfig> noAdditionalLineOptimizationConfig;
        std::string                                                               expectedMainModuleName;

        explicit ParserConfig(
                const unsigned int                                                              defaultSignalWidth                          = 32U,
                const bool                                                                      supportBroadcastingExpressionOperands       = false,
                const bool                                                                      supportBroadcastingAssignmentOperands       = false,
                const bool                                                                      reassociateExpressionEnabled                = false,
                const bool                                                                      isRemovalOfUnusedVariablesAndModulesEnabled = false,
                const bool                                                                      constantPropagationEnabled                  = false,
                const bool                                                                      operationStrengthReductionEnabled           = false,
                const bool                                                                      deadStoreEliminationEnabled                 = false,
                const bool                                                                      combineRedundantInstructions                = false,
                const optimizations::MultiplicationSimplificationMethod                         multiplicationSimplificationMethod          = optimizations::MultiplicationSimplificationMethod::None,
                const std::optional<optimizations::LoopOptimizationConfig>                      optionalLoopUnrollConfig                    = std::nullopt,
                const std::optional<noAdditionalLineSynthesis::NoAdditionalLineSynthesisConfig> noAdditionalLineOptimizationConfig          = std::nullopt,
                std::string                                                                     expectedMainModuleName                      = defaultExpectedMainModuleName):
            cDefaultSignalWidth(defaultSignalWidth),
            supportBroadcastingExpressionOperands(supportBroadcastingExpressionOperands),
            supportBroadcastingAssignmentOperands(supportBroadcastingAssignmentOperands),
            reassociateExpressionsEnabled(reassociateExpressionEnabled),
            deadCodeEliminationEnabled(isRemovalOfUnusedVariablesAndModulesEnabled),
            constantPropagationEnabled(constantPropagationEnabled),
            operationStrengthReductionEnabled(operationStrengthReductionEnabled),
            deadStoreEliminationEnabled(deadStoreEliminationEnabled),
            combineRedundantInstructions(combineRedundantInstructions),
            multiplicationSimplificationMethod(multiplicationSimplificationMethod),
            optionalLoopUnrollConfig(optionalLoopUnrollConfig),
            noAdditionalLineOptimizationConfig(noAdditionalLineOptimizationConfig),
            expectedMainModuleName(std::move(expectedMainModuleName)) {}

        ParserConfig& operator=(const ParserConfig& copy) {
            cDefaultSignalWidth                   = copy.cDefaultSignalWidth;
            supportBroadcastingExpressionOperands = copy.supportBroadcastingExpressionOperands;
            supportBroadcastingAssignmentOperands = copy.supportBroadcastingAssignmentOperands;
            reassociateExpressionsEnabled         = copy.reassociateExpressionsEnabled;
            deadCodeEliminationEnabled            = copy.deadCodeEliminationEnabled;
            constantPropagationEnabled            = copy.constantPropagationEnabled;
            operationStrengthReductionEnabled     = copy.operationStrengthReductionEnabled;
            deadStoreEliminationEnabled           = copy.deadStoreEliminationEnabled;
            combineRedundantInstructions          = copy.combineRedundantInstructions;
            multiplicationSimplificationMethod    = copy.multiplicationSimplificationMethod;
            if (copy.optionalLoopUnrollConfig.has_value()) {
                optionalLoopUnrollConfig.emplace(*copy.optionalLoopUnrollConfig);
            } else {
                optionalLoopUnrollConfig.reset();
            }

            if (noAdditionalLineOptimizationConfig.has_value()) {
                noAdditionalLineOptimizationConfig.emplace(*copy.noAdditionalLineOptimizationConfig);
            } else {
                noAdditionalLineOptimizationConfig.reset();
            }

            expectedMainModuleName = copy.expectedMainModuleName;
            return *this;
        }
    };
}

#endif