#ifndef SYREC_PARSER_ERROR_LISTENER_H
#define SYREC_PARSER_ERROR_LISTENER_H
#include "BaseErrorListener.h"

namespace parser {
class SyReCParserErrorListener : public antlr4::BaseErrorListener {
public:
    void syntaxError(antlr4::Recognizer* /*recognizer*/, antlr4::Token* /*offendingSymbol*/, size_t /*line*/,
                             size_t /*charPositionInLine*/, const std::string& /*msg*/, std::exception_ptr /*e*/) override;
};
}

#endif
