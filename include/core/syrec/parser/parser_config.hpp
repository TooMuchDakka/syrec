#ifndef PARSER_CONFIG_H
#define PARSER_CONFIG_H

#include <string>

namespace parser {
    struct ParserConfig {
        unsigned int cDefaultSignalWidth;
        bool         supportBroadcastingExpressionOperands;
        bool         supportBroadcastingAssignmentOperands;
        bool         reassociateExpressionsEnabled;
        bool         isRemovalOfUnusedVariablesAndModulesEnabled;
        std::string  expectedMainModuleName;

        explicit ParserConfig(
            const unsigned int defaultSignalWidth = 32U, 
            const bool supportBroadcastingExpressionOperands = false,
            const bool supportBroadcastingAssignmentOperands = false,
            const bool reassociateExpressionEnabled = false,
            const bool isRemovalOfUnusedVariablesAndModulesEnabled = false,
            std::string  expectedMainModuleName                      = "main"):
                cDefaultSignalWidth(defaultSignalWidth),
                supportBroadcastingExpressionOperands(supportBroadcastingExpressionOperands),
                supportBroadcastingAssignmentOperands(supportBroadcastingAssignmentOperands),
                reassociateExpressionsEnabled(reassociateExpressionEnabled),
                isRemovalOfUnusedVariablesAndModulesEnabled(isRemovalOfUnusedVariablesAndModulesEnabled),
                expectedMainModuleName(std::move(expectedMainModuleName))
                {}

        
        ParserConfig& operator=(const ParserConfig& copy) {
            cDefaultSignalWidth = copy.cDefaultSignalWidth;
            supportBroadcastingExpressionOperands = copy.supportBroadcastingExpressionOperands;
            supportBroadcastingAssignmentOperands = copy.supportBroadcastingAssignmentOperands;
            reassociateExpressionsEnabled         = copy.reassociateExpressionsEnabled;
            isRemovalOfUnusedVariablesAndModulesEnabled = copy.isRemovalOfUnusedVariablesAndModulesEnabled;
            return *this;
        }
    };
}

#endif