#ifndef SYREC_CUSTOM_VISITOR_H
#define SYREC_CUSTOM_VISITOR_H
    

#include <stack>
#include <string>
#include <vector>

#include "core/syrec/module.hpp"
#include "core/syrec/parser/parser_config.hpp"
#include "core/syrec/parser/signal_evaluation_result.hpp"
#include "core/syrec/parser/symbol_table.hpp"

#include "SyReCBaseVisitor.h"

namespace parser {
class SyReCCustomVisitor: public SyReCBaseVisitor {
private:
    SymbolTable::ptr                                             currentSymbolTableScope;
    std::stack<std::pair<std::string, std::vector<std::string>>> callStack;
    syrec::Number::loop_variable_mapping                         loopVariableMappingLookup;

    ParserConfig     config;
    const std::string messageFormat = "-- line {0:d} col {1:d}: {2:s}";

    void createError(const std::string& errorMessage);
    void createError(size_t line, size_t column, const std::string& errorMessage);

    void createWarning(const std::string& warningMessage);
    void createWarning(size_t line, size_t column, const std::string& warningMessage);

    [[nodiscard]] std::optional<unsigned int> convertToNumber(const antlr4::Token* token) const;

    template<typename T>
    [[nodiscard]] std::optional<T>            tryConvertProductionReturnValue(std::any productionReturnType) const {
        if (!productionReturnType.has_value()) {
            return std::nullopt;
        }

        try {
            return std::any_cast<std::optional<T>>(productionReturnType);
        }
        catch (std::bad_any_cast&) {
            return std::nullopt;
        }
    }

    std::optional<syrec::Variable::Types> getParameterType(const antlr4::Token* token);
    std::optional<syrec::Variable::Types> getSignalType(const antlr4::Token* token);

public:
    syrec::Module::vec modules;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;

    /**
     * @brief Sets the number of line
     *
     * This method sets the number of lines of the circuit.
     *
     * Changing this number will not affect the data in the gates.
     * For example: If there is a gate with a control on line 3,
     * and the number of lines is reduced to 2 in the circuit, then
     * the control is still on line 3 but not visible in this circuit.
     *
     * So, changing the lines after already adding gates can lead
     * to invalid gates.
     *
     * @param context test
     */
    std::any visitProgram(SyReCParser::ProgramContext* context) override;
    std::any visitModule(SyReCParser::ModuleContext* context) override;
    std::any visitParameterList(SyReCParser::ParameterListContext* context) override;
    std::any visitParameter(SyReCParser::ParameterContext* context) override;
    std::any visitSignalList(SyReCParser::SignalListContext* context) override;
    std::any visitSignalDeclaration(SyReCParser::SignalDeclarationContext* context) override;

    std::any visitUnaryStatement(SyReCParser::UnaryStatementContext* context) override;
    
};
    
} // namespace parser
#endif