#ifndef SYREC_CUSTOM_BASE_VISITOR_SHARED_DATA_HPP
#define SYREC_CUSTOM_BASE_VISITOR_SHARED_DATA_HPP
#pragma once

#include "core/syrec/parser/parser_config.hpp"
#include "core/syrec/parser/signal_access_restriction.hpp"
#include "core/syrec/parser/symbol_table.hpp"

#include <optional>

namespace parser {
    class SharedVisitorData {
    public:
        std::vector<std::string>                                errors;
        std::vector<std::string>                                warnings;
        const std::unique_ptr<ParserConfig>                      parserConfig;
        std::shared_ptr<SymbolTable>                             currentSymbolTableScope;
        std::size_t                                              currentModuleCallNestingLevel;
        syrec::Number::loop_variable_mapping                     loopVariableMappingLookup;
        bool                                                     shouldSkipSignalAccessRestrictionCheck;
        std::optional<unsigned int>                              optionalExpectedExpressionSignalWidth;
        std::set<std::string>                                    evaluableLoopVariables;
        std::unordered_map<std::string, SignalAccessRestriction> signalAccessRestrictions;
        std::optional<std::string>                               lastDeclaredLoopVariable;

        explicit SharedVisitorData(const ParserConfig& parserConfig) :
            parserConfig(std::make_unique<ParserConfig>(parserConfig)),
            currentSymbolTableScope(std::make_shared<SymbolTable>()),
            currentModuleCallNestingLevel(0),
            shouldSkipSignalAccessRestrictionCheck(true)
        {}
    };
}; // namespace parser

#endif