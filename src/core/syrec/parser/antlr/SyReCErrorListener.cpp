#include "SyReCErrorListener.h"

#include "core/syrec/parser/utils/message_utils.hpp"

void parser::SyReCErrorListener::syntaxError(antlr4::Recognizer* /*recognizer*/, antlr4::Token* /*offendingSymbol*/, size_t line,
                                             size_t charPositionInLine, const std::string& msg, std::exception_ptr /*e*/)
{
    errorsContainer.emplace_back(messageUtils::Message({messageUtils::Message::ErrorCategory::Syntax, messageUtils::Message::Position(line, charPositionInLine), messageUtils::Message::Severity::Error, msg}));
    
}
