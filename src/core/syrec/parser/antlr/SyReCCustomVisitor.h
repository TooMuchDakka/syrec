#ifndef SYREC_CUSTOM_VISITOR_H
#define SYREC_CUSTOM_VISITOR_H

#include "SyReCBaseVisitor.h"    

#include <stack>
#include <string>
#include <vector>

#include "core/syrec/module.hpp"
#include "core/syrec/parser/module_call_stack.hpp"
#include "core/syrec/parser/expression_evaluation_result.hpp"
#include "core/syrec/parser/operation.hpp"
#include "core/syrec/parser/parser_config.hpp"
#include "core/syrec/parser/signal_evaluation_result.hpp"
#include "core/syrec/parser/symbol_table.hpp"

namespace parser {
class SyReCCustomVisitor: public SyReCBaseVisitor {
private:
    SymbolTable::ptr                                             currentSymbolTableScope;
    std::unique_ptr<ModuleCallStack>                             moduleCallStack;
    size_t                                                       moduleCallNestingLevel;

    syrec::Number::loop_variable_mapping                         loopVariableMappingLookup;
    std::stack<syrec::Statement::vec>               statementListContainerStack;
    const std::shared_ptr<ParserConfig>     config;

    std::optional<unsigned int> optionalExpectedExpressionSignalWidth;

    void createErrorAtTokenPosition(const antlr4::Token* token, const std::string& errorMessage);
    void createError(std::size_t line, std::size_t column, const std::string& errorMessage);
    void createWarning(const std::string& warningMessage);

    /**
     * \brief Try to convert the return value of the given production to an instance of std::optional<T>.
     * \tparam T The expected type of the value wrapped inside of a std::optional returned by the production. IMPORTANT: This must be the exact type of the class, currently polymorphism does not work with std::any because of type erasure by std::any (see https://stackoverflow.com/questions/70313749/combining-static-cast-and-stdany-cast)
     * \param productionReturnType The expected type of the value wrapped inside of a std::optional returned by the production
     * \return A std::optional<T> object containing the casted return value of the production
     */
    template<typename T>
    [[nodiscard]] std::optional<T>            tryConvertProductionReturnValue(std::any productionReturnType) const {
        if (!productionReturnType.has_value()) {
            return std::nullopt;
        }

        try {
            return std::any_cast<std::optional<T>>(productionReturnType);
        }
        catch (std::bad_any_cast& ex) {
            // TODO: Better error handling, i.e. logging or returning C-style error code to check whether cast or something else failed
            return std::nullopt;
        }
    }

    /**
     * \brief If the context is not null, try to call the corresponding visitor and try to convert the return value of the given production to an instance of std::optional<T>.
     * \tparam T The expected type of the value wrapped inside of a std::optional returned by the production. IMPORTANT: This must be the exact type of the class, currently polymorphism does not work with std::any because of type erasure by std::any (see https://stackoverflow.com/questions/70313749/combining-static-cast-and-stdany-cast)
     * \param production The expected type of the value wrapped inside of a std::optional returned by the production
     * \return A std::optional<T> object containing the casted return value of the production
     */
    template<typename T>
    [[nodiscard]] std::optional<T> tryVisitAndConvertProductionReturnValue(antlr4::tree::ParseTree* production) {
        if (production == nullptr) {
            return std::nullopt;
        }
        return tryConvertProductionReturnValue<T>(visit(production));
    }

    std::optional<syrec::Variable::Types> getParameterType(const antlr4::Token* token);
    std::optional<syrec::Variable::Types> getSignalType(const antlr4::Token* token);

    [[nodiscard]] static bool isValidBinaryOperation(syrec_operation::operation userDefinedOperation);
    [[nodiscard]] static bool areExpressionsEqual(const ExpressionEvaluationResult::ptr& firstExpr, const ExpressionEvaluationResult::ptr& otherExpr);

    bool                                                                           checkIfSignalWasDeclaredOrLogError(const antlr4::Token* signalIdentToken, bool isLoopVariable=false);
    [[nodiscard]] bool validateSemanticChecksIfDimensionExpressionIsConstant(const antlr4::Token* dimensionToken, size_t accessedDimensionIdx, const syrec::Variable::ptr& accessedSignal, const std::optional<ExpressionEvaluationResult::ptr>& expressionEvaluationResult);
    [[nodiscard]] std::optional<std::pair<syrec::Number::ptr, syrec::Number::ptr>> isBitOrRangeAccessDefined(SyReCParser::NumberContext* bitRangeStartToken, SyReCParser::NumberContext* bitRangeEndToken);
    [[nodiscard]] std::optional<syrec_operation::operation> getDefinedOperation(const antlr4::Token* definedOperationToken);
    [[nodiscard]] std::optional<unsigned int> tryEvaluateNumber(const syrec::Number::ptr& numberContainer);
    [[nodiscard]] std::optional<unsigned int> applyBinaryOperation(syrec_operation::operation operation, unsigned int leftOperand, unsigned int rightOperand);
    [[nodiscard]] bool isSignalAssignableOtherwiseCreateError(const antlr4::Token* signalIdentToken, const syrec::VariableAccess::ptr& assignedToVariable);
    void addStatementToOpenContainer(const syrec::Statement::ptr& statement);

    bool areSemanticChecksForCallOrUncallDependingOnNameValid(bool isCallOperation, const std::pair<size_t, size_t>& moduleIdentTokenPosition, const std::optional<std::string>& moduleIdent);
    bool doArgumentsBetweenCallAndUncallMatch(const std::pair<size_t, size_t>& positionOfPotentialError, const std::string& uncalledModuleIdent, const std::vector<std::string>& calleeArguments);


public:
    syrec::Module::vec modules;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;

    explicit SyReCCustomVisitor(const std::shared_ptr<ParserConfig>& parserConfig):
        config(parserConfig), moduleCallNestingLevel(0) {
        moduleCallStack = std::make_unique<ModuleCallStack>();
    }

    /*
     * TODO: Visitor can be split into parts with error containers being shared
     * I.Module declaration visitor
     * II. Expression visitor
     * III. Statement visitor
     *
     * Class structure could look like
     *
     * CustomParser
     *  offers parser function and holds internal data structures (symbol table, errors-, warnings container) that
     *  are passed to the visitiors
     *  - offers utility functions to create errors, warnings (basically reusing utility functions)
     *
     * ModuleVisitor(customParser)
     *  -> Statement visitor(customParser)
     *      -> Expression visitor(customParser)
     */
    
    std::any visitProgram(SyReCParser::ProgramContext* context) override;
    std::any visitModule(SyReCParser::ModuleContext* context) override;
    std::any visitParameterList(SyReCParser::ParameterListContext* context) override;
    std::any visitParameter(SyReCParser::ParameterContext* context) override;
    std::any visitSignalList(SyReCParser::SignalListContext* context) override;
    std::any visitSignalDeclaration(SyReCParser::SignalDeclarationContext* context) override;

    std::any visitSignal(SyReCParser::SignalContext* context) override;

    std::any visitNumberFromConstant(SyReCParser::NumberFromConstantContext* context) override;
    std::any visitNumberFromSignalwidth(SyReCParser::NumberFromSignalwidthContext* context) override;
    std::any visitNumberFromExpression(SyReCParser::NumberFromExpressionContext* context) override;
    std::any visitNumberFromLoopVariable(SyReCParser::NumberFromLoopVariableContext* context) override;

    /*
     * Expression production visitors
     */
    std::any visitExpressionFromNumber(SyReCParser::ExpressionFromNumberContext* context) override;
    std::any visitExpressionFromSignal(SyReCParser::ExpressionFromSignalContext* context) override;
    std::any visitBinaryExpression(SyReCParser::BinaryExpressionContext* context) override;
    std::any visitUnaryExpression(SyReCParser::UnaryExpressionContext* context) override;
    std::any visitShiftExpression(SyReCParser::ShiftExpressionContext* context) override;

    /*
     * Statement production visitors
     */
    std::any visitStatementList(SyReCParser::StatementListContext* context) override;
    std::any visitCallStatement(SyReCParser::CallStatementContext* context) override;
    std::any visitForStatement(SyReCParser::ForStatementContext* context) override;
    std::any visitLoopStepsizeDefinition(SyReCParser::LoopStepsizeDefinitionContext* context) override;
    std::any visitLoopVariableDefinition(SyReCParser::LoopVariableDefinitionContext* context) override;

    std::any visitIfStatement(SyReCParser::IfStatementContext* context) override;
    std::any visitUnaryStatement(SyReCParser::UnaryStatementContext* context) override;
    std::any visitAssignStatement(SyReCParser::AssignStatementContext* context) override;
    std::any visitSwapStatement(SyReCParser::SwapStatementContext* context) override;
    std::any visitSkipStatement(SyReCParser::SkipStatementContext* context) override;
};
    
} // namespace parser
#endif