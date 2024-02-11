#ifndef PARSER_CONFIG_HPP
#define PARSER_CONFIG_HPP

#include "optimizations/loop_optimizer.hpp"
#include "optimizations/operationSimplification/base_multiplication_simplifier.hpp"

#include <string>

namespace parser {
    struct NoAdditionalLineSynthesisConfig {
        bool useGeneratedAssignmentsByDecisionAsTieBreaker = false;
        /**
         * \brief Prefer the simplification result of the more complex algorithm regardless of the costs in comparision to the simplification result of the "simpler" algorithm. If the former did not produce any assignments, the results from the latter are used instead.
         */
        bool preferAssignmentsGeneratedByChoiceRegardlessOfCost = false;
        /**
         * \brief This additional factor for the cost of an expression can be used to achieve a balance between code-size and the number of additional signals required for the synthesis of an assignment
         */
        double                     expressionNestingPenalty                       = 75;
        double                     expressionNestingPenaltyScalingPerNestingLevel = 3.5;
        std::optional<std::string> optionalNewReplacementSignalIdentsPrefix;
    };

    struct ParserConfig {
        static const inline std::string defaultExpectedMainModuleName = "main";

        unsigned int                                         cDefaultSignalWidth;
        bool                                                 supportBroadcastingExpressionOperands;
        bool                                                 supportBroadcastingAssignmentOperands;
        bool                                                 reassociateExpressionsEnabled;
        bool                                                 deadCodeEliminationEnabled;
        bool                                                 constantPropagationEnabled;
        bool                                                 operationStrengthReductionEnabled;
        bool                                                 deadStoreEliminationEnabled;
        bool                                                 combineRedundantInstructions;
        optimizations::MultiplicationSimplificationMethod    multiplicationSimplificationMethod;
        std::optional<optimizations::LoopOptimizationConfig> optionalLoopUnrollConfig;
        std::optional<NoAdditionalLineSynthesisConfig>       noAdditionalLineOptimizationConfig;
        std::string                                          expectedMainModuleName;

        explicit ParserConfig(
                const unsigned int                                         defaultSignalWidth                          = 32U,
                const bool                                                 supportBroadcastingExpressionOperands       = false,
                const bool                                                 supportBroadcastingAssignmentOperands       = false,
                const bool                                                 reassociateExpressionEnabled                = false,
                const bool                                                 isRemovalOfUnusedVariablesAndModulesEnabled = false,
                const bool                                                 constantPropagationEnabled                  = false,
                const bool                                                 operationStrengthReductionEnabled           = false,
                const bool                                                 deadStoreEliminationEnabled                 = false,
                const bool                                                 combineRedundantInstructions                = false,
                const optimizations::MultiplicationSimplificationMethod    multiplicationSimplificationMethod          = optimizations::MultiplicationSimplificationMethod::None,
                const std::optional<optimizations::LoopOptimizationConfig> optionalLoopUnrollConfig                    = std::nullopt,
                const std::optional<NoAdditionalLineSynthesisConfig>      noAdditionalLineOptimizationConfig          = std::nullopt,
                std::string                                                expectedMainModuleName                      = defaultExpectedMainModuleName):
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