#ifndef PARSER_CONFIG_H
#define PARSER_CONFIG_H

#include "optimizations/loop_optimizer.hpp"
#include "optimizations/operationSimplification/base_multiplication_simplifier.hpp"

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
        bool                                                 deadStoreEliminationEnabled;
        optimizations::MultiplicationSimplificationMethod    multiplicationSimplificationMethod;
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
                const bool deadStoreEliminationEnabled = false,
                const optimizations::MultiplicationSimplificationMethod multiplicationSimplificationMethod = optimizations::MultiplicationSimplificationMethod::None,
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
                deadStoreEliminationEnabled(deadStoreEliminationEnabled),
                multiplicationSimplificationMethod(multiplicationSimplificationMethod),
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
            deadStoreEliminationEnabled                  = copy.deadStoreEliminationEnabled;
            multiplicationSimplificationMethod           = copy.multiplicationSimplificationMethod;
            if (copy.optionalLoopUnrollConfig.has_value()) {
                optionalLoopUnrollConfig.emplace(*copy.optionalLoopUnrollConfig);
            }
            expectedMainModuleName                       = copy.expectedMainModuleName;
            return *this;
        }
    };
}

#endif