#ifndef PARSER_CONFIG_H
#define PARSER_CONFIG_H

namespace parser {
    struct ParserConfig {
        unsigned int cDefaultSignalWidth;
        bool         supportBroadcastingExpressionOperands;
        bool         supportBroadcastingAssignmentOperands;

        explicit ParserConfig(
            const unsigned int defaultSignalWidth = 32U, 
            const bool supportBroadcastingExpressionOperands = false,
            const bool supportBroadcastingAssignmentOperands = false):
            cDefaultSignalWidth(defaultSignalWidth), supportBroadcastingExpressionOperands(supportBroadcastingExpressionOperands), supportBroadcastingAssignmentOperands(supportBroadcastingAssignmentOperands) {};

        
        ParserConfig& operator=(const ParserConfig& copy) {
            cDefaultSignalWidth = copy.cDefaultSignalWidth;
            supportBroadcastingExpressionOperands = copy.supportBroadcastingExpressionOperands;
            supportBroadcastingAssignmentOperands = copy.supportBroadcastingAssignmentOperands;
            return *this;
        }
    };
}

#endif