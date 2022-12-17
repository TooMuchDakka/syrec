#include "SyReCErrorListener.h"

#include "core/syrec/parser/parser_utilities.hpp"

void parser::SyReCErrorListener::syntaxError(antlr4::Recognizer* /*recognizer*/, antlr4::Token* /*offendingSymbol*/, size_t line,
                                                  size_t charPositionInLine, const std::string& msg, std::exception_ptr /*e*/)
{
    errorsContainer.emplace_back(ParserUtilities::createError(line, charPositionInLine, msg));
}
