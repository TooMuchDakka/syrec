
// Generated from ..\syrec_antlr\src\core\syrec\parser\antlr\SyReC.g4 by ANTLR 4.11.1

#pragma once


#include "antlr4-runtime.h"
#include "SyReCVisitor.h"


namespace parser {

/**
 * This class provides an empty implementation of SyReCVisitor, which can be
 * extended to create a visitor which only needs to handle a subset of the available methods.
 */
class  SyReCBaseVisitor : public SyReCVisitor {
public:

  virtual std::any visitNumberFromConstant(SyReCParser::NumberFromConstantContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNumberFromSignalwidth(SyReCParser::NumberFromSignalwidthContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNumberFromLoopVariable(SyReCParser::NumberFromLoopVariableContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNumberFromExpression(SyReCParser::NumberFromExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitProgram(SyReCParser::ProgramContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitModule(SyReCParser::ModuleContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitParameterList(SyReCParser::ParameterListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitParameter(SyReCParser::ParameterContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSignalList(SyReCParser::SignalListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSignalDeclaration(SyReCParser::SignalDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStatementList(SyReCParser::StatementListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStatement(SyReCParser::StatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCallStatement(SyReCParser::CallStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitForStatement(SyReCParser::ForStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIfStatement(SyReCParser::IfStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUnaryStatement(SyReCParser::UnaryStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAssignStatement(SyReCParser::AssignStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSwapStatement(SyReCParser::SwapStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSkipStatement(SyReCParser::SkipStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSignal(SyReCParser::SignalContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExpressionFromNumber(SyReCParser::ExpressionFromNumberContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExpressionFromSignal(SyReCParser::ExpressionFromSignalContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExpressionFromBinaryExpression(SyReCParser::ExpressionFromBinaryExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExpressionFromUnaryExpression(SyReCParser::ExpressionFromUnaryExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExpressionFromShiftExpression(SyReCParser::ExpressionFromShiftExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBinaryExpression(SyReCParser::BinaryExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUnaryExpression(SyReCParser::UnaryExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitShiftExpression(SyReCParser::ShiftExpressionContext *ctx) override {
    return visitChildren(ctx);
  }


};

}  // namespace parser
