#ifndef PARSER_CONFIG_H
#define PARSER_CONFIG_H

#include "optimizations/loop_optimizer.hpp"

#include <string>

namespace parser {
    struct ParserConfig {
        unsigned int                                         cDefaultSignalWidth;
        bool                                                 supportBroadcastingExpressionOperands;
        bool                                                 supportBroadcastingAssignmentOperands;
        bool                                                 reassociateExpressionsEnabled;
        bool                                                 deadCodeEliminationEnabled;
        bool                                                 performConstantPropagation;
        bool                                                 noAdditionalLineOptimizationEnabled;
        bool                                                 operationStrengthReductionEnabled;
        std::optional<optimizations::LoopOptimizationConfig> optionalLoopUnrollConfig;
        std::string  expectedMainModuleName;

        explicit ParserConfig(
                const unsigned int defaultSignalWidth                          = 32U,
                const bool         supportBroadcastingExpressionOperands       = false,
                const bool         supportBroadcastingAssignmentOperands       = false,
                const bool         reassociateExpressionEnabled                = false,
                const bool         isRemovalOfUnusedVariablesAndModulesEnabled = false,
                const bool         performConstantPropagation                  = false,
                const bool         noAdditionalLineOptimizationEnabled         = false,
                const bool         operationStrengthReductionEnabled           = false,
                const std::optional<optimizations::LoopOptimizationConfig> optionalLoopUnrollConfig = std::nullopt,
                std::string  expectedMainModuleName                      = "main"):
                cDefaultSignalWidth(defaultSignalWidth),
                supportBroadcastingExpressionOperands(supportBroadcastingExpressionOperands),
                supportBroadcastingAssignmentOperands(supportBroadcastingAssignmentOperands),
                reassociateExpressionsEnabled(reassociateExpressionEnabled),
                deadCodeEliminationEnabled(isRemovalOfUnusedVariablesAndModulesEnabled),
                performConstantPropagation(performConstantPropagation),
                noAdditionalLineOptimizationEnabled(noAdditionalLineOptimizationEnabled),
                operationStrengthReductionEnabled(operationStrengthReductionEnabled),
                optionalLoopUnrollConfig(optionalLoopUnrollConfig),
                expectedMainModuleName(std::move(expectedMainModuleName))
                {}

        
        ParserConfig& operator=(const ParserConfig& copy) {
            cDefaultSignalWidth = copy.cDefaultSignalWidth;
            supportBroadcastingExpressionOperands        = copy.supportBroadcastingExpressionOperands;
            supportBroadcastingAssignmentOperands        = copy.supportBroadcastingAssignmentOperands;
            reassociateExpressionsEnabled                = copy.reassociateExpressionsEnabled;
            deadCodeEliminationEnabled                   = copy.deadCodeEliminationEnabled;
            performConstantPropagation                   = copy.performConstantPropagation;
            noAdditionalLineOptimizationEnabled          = copy.noAdditionalLineOptimizationEnabled;
            operationStrengthReductionEnabled            = copy.operationStrengthReductionEnabled;
            if (copy.optionalLoopUnrollConfig.has_value()) {
                optionalLoopUnrollConfig.emplace(*copy.optionalLoopUnrollConfig);
            }
            expectedMainModuleName                       = copy.expectedMainModuleName;
            return *this;
        }
    };
}

#endif