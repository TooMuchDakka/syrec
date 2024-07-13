#ifndef SYREC_CUSTOM_BASE_VISITOR_SHARED_DATA_HPP
#define SYREC_CUSTOM_BASE_VISITOR_SHARED_DATA_HPP
#pragma once

#include "core/syrec/parser/parser_config.hpp"
#include "core/syrec/parser/symbol_table.hpp"
#include "core/syrec/parser/utils/message_utils.hpp"

#include <optional>

namespace parser {
    class SharedVisitorData {
    public:
        using ptr = std::shared_ptr<SharedVisitorData>;

        static constexpr std::size_t fallbackErrorLinePosition = 0;
        static constexpr std::size_t fallbackErrorColumnPosition = 0;

        std::vector<messageUtils::Message>                                                errors;
        std::vector<messageUtils::Message>                                                warnings;
        std::vector<std::size_t>                                                          syntaxErrorPositionIndices;
        const std::unique_ptr<ParserConfig>                                               parserConfig;
        parser::SymbolTable::ptr                                                          currentSymbolTableScope;
        std::size_t                                                                       currentModuleCallNestingLevel;
        std::optional<unsigned int>                                                       optionalExpectedExpressionSignalWidth;
        std::optional<std::string>                                                        lastDeclaredLoopVariable;
        messageUtils::MessageFactory                                                      messageFactory;
        

        explicit SharedVisitorData(const ParserConfig& parserConfig):
            parserConfig(std::make_unique<ParserConfig>(parserConfig)),
            currentSymbolTableScope(std::make_shared<SymbolTable>()),
            currentModuleCallNestingLevel(0),
            messageFactory(fallbackErrorLinePosition, fallbackErrorColumnPosition)
        {}

        void resetSignalAccessRestriction() {
            signalAccessRestriction = nullptr;
        }

        void createSignalAccessRestriction(const syrec::VariableAccess::ptr& assignedToSignalPart) {
            signalAccessRestriction = assignedToSignalPart;
        }

        [[nodiscard]] std::optional<syrec::VariableAccess::ptr> getSignalAccessRestriction() const {
            return signalAccessRestriction ? std::make_optional(signalAccessRestriction) : std::nullopt;
        }
        
    protected:
        syrec::VariableAccess::ptr signalAccessRestriction;
    };
}; // namespace parser

#endif