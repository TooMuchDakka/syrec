#ifndef SYREC_PARSER_INTERFACE_HPP
#define SYREC_PARSER_INTERFACE_HPP
#pragma once

#include <vector>
#include <string>

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
            bool                     wasParsingSuccessful;
            std::vector<messageUtils::Message> errors;
            std::vector<messageUtils::Message> warnings;
            syrec::Module::vec       foundModules;

            explicit ParsingResult(std::vector<messageUtils::Message> errors, std::vector<messageUtils::Message> warnings):
                wasParsingSuccessful(false), errors(std::move(errors)), warnings(std::move(warnings)), foundModules({}) {}

            explicit ParsingResult(syrec::Module::vec foundModules):
                wasParsingSuccessful(true), errors({}), warnings({}), foundModules(std::move(foundModules)) {}
        };

        [[nodiscard]] static ParsingResult parseProgram(const char* programContent, int programContentSizeInBytes, ParserConfig parserConfig) {
            antlr4::ANTLRInputStream  input(programContent, programContentSizeInBytes);
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
                return ParsingResult(customVisitor->getFoundModules());
            }
            return ParsingResult(customVisitor->getErrors(), customVisitor->getWarnings());
        }
    };
}
#endif