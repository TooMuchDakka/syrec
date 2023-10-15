#ifndef SYREC_PARSER_INTERFACE_HPP
#define SYREC_PARSER_INTERFACE_HPP
#pragma once

#include <vector>

#include "CommonTokenStream.h"
#include "SyReCLexer.h"
#include "SyReCParser.h"
#include "SyReCErrorListener.h"

#include "core/syrec/parser/antlr/parserComponents/syrec_module_visitor.hpp"
#include "core/syrec/module.hpp"
#include <core/syrec/parser/utils/message_utils.hpp>

namespace parser {
    class SyrecParserInterface {
    public:
        struct ParsingResult {
            const bool                               wasParsingSuccessful;
            const std::vector<messageUtils::Message> errors;
            const std::vector<messageUtils::Message> warnings;
            const syrec::Module::vec                 foundModules;

            [[nodiscard]] static ParsingResult asNotSuccessful(std::vector<messageUtils::Message> errors, std::vector<messageUtils::Message> warnings) {
                return ParsingResult({.wasParsingSuccessful = false, .errors = errors, .warnings = warnings, .foundModules = {}});
            }

            [[nodiscard]] static ParsingResult asSuccessWithResult(syrec::Module::vec foundModules) {
                return ParsingResult({.wasParsingSuccessful = true, .errors = {}, .warnings = {}, .foundModules = foundModules});
            }
        };

        [[nodiscard]] static ParsingResult parseProgram(std::string_view programContent, ParserConfig parserConfig) {
            antlr4::ANTLRInputStream input(programContent);
            SyReCLexer      lexer(&input);
            antlr4::CommonTokenStream tokens(&lexer);
            SyReCParser     antlrParser(&tokens);

            const std::shared_ptr<SharedVisitorData> sharedVisitorData        = std::make_shared<SharedVisitorData>(parserConfig);
            const auto customVisitor            = std::make_unique<SyReCModuleVisitor>(SyReCModuleVisitor(sharedVisitorData));
            const auto antlrParserErrorListener = std::make_unique<SyReCErrorListener>(SyReCErrorListener(sharedVisitorData->errors));
            lexer.addErrorListener(antlrParserErrorListener.get());
            antlrParser.addErrorListener(antlrParserErrorListener.get());

            customVisitor->visitProgram(antlrParser.program());

            lexer.removeErrorListener(antlrParserErrorListener.get());
            antlrParser.removeErrorListener(antlrParserErrorListener.get());

            if (customVisitor->getErrors().empty()) {
                return ParsingResult::asSuccessWithResult(customVisitor->getFoundModules());
            }
            return ParsingResult::asNotSuccessful(customVisitor->getErrors(), customVisitor->getWarnings());
        }
    };
}
#endif