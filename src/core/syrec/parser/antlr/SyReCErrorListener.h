#ifndef SYREC_PARSER_ERROR_LISTENER_H
#define SYREC_PARSER_ERROR_LISTENER_H
#include "BaseErrorListener.h"

namespace parser {
class SyReCErrorListener : public antlr4::BaseErrorListener {
private:
    std::vector<std::string>& errorsContainer;

public:
    SyReCErrorListener(std::vector<std::string>& errorsContainer):
        errorsContainer(errorsContainer) {}

    void syntaxError(antlr4::Recognizer* /*recognizer*/, antlr4::Token* /*offendingSymbol*/, size_t line,
                             size_t charPositionInLine, const std::string& msg, std::exception_ptr /*e*/) override;
};
}

#endif
