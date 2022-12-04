
// Generated from ..\syrec_antlr\src\core\syrec\parser\antlr\SyReC.g4 by ANTLR 4.11.1

#pragma once


#include "antlr4-runtime.h"
#include "SyReCParser.h"


namespace parser {

/**
 * This class defines an abstract visitor for a parse tree
 * produced by SyReCParser.
 */
class  SyReCVisitor : public antlr4::tree::AbstractParseTreeVisitor {
public:

  /**
   * Visit parse trees produced by SyReCParser.
   */
    virtual std::any visitNumber(SyReCParser::NumberContext *context) = 0;

    virtual std::any visitProgram(SyReCParser::ProgramContext *context) = 0;

    virtual std::any visitModule(SyReCParser::ModuleContext *context) = 0;

    virtual std::any visitParameterList(SyReCParser::ParameterListContext *context) = 0;

    virtual std::any visitParameter(SyReCParser::ParameterContext *context) = 0;

    virtual std::any visitSignalList(SyReCParser::SignalListContext *context) = 0;

    virtual std::any visitSignalDeclaration(SyReCParser::SignalDeclarationContext *context) = 0;

    virtual std::any visitStatementList(SyReCParser::StatementListContext *context) = 0;

    virtual std::any visitStatement(SyReCParser::StatementContext *context) = 0;

    virtual std::any visitCallStatement(SyReCParser::CallStatementContext *context) = 0;

    virtual std::any visitForStatement(SyReCParser::ForStatementContext *context) = 0;

    virtual std::any visitIfStatement(SyReCParser::IfStatementContext *context) = 0;

    virtual std::any visitUnaryStatement(SyReCParser::UnaryStatementContext *context) = 0;

    virtual std::any visitAssignStatement(SyReCParser::AssignStatementContext *context) = 0;

    virtual std::any visitSwapStatement(SyReCParser::SwapStatementContext *context) = 0;

    virtual std::any visitSkipStatement(SyReCParser::SkipStatementContext *context) = 0;

    virtual std::any visitSignal(SyReCParser::SignalContext *context) = 0;

    virtual std::any visitExpression(SyReCParser::ExpressionContext *context) = 0;

    virtual std::any visitBinaryExpression(SyReCParser::BinaryExpressionContext *context) = 0;

    virtual std::any visitUnaryExpression(SyReCParser::UnaryExpressionContext *context) = 0;

    virtual std::any visitShiftExpression(SyReCParser::ShiftExpressionContext *context) = 0;


};

}  // namespace parser
