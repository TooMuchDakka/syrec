
// Generated from ..\syrec_antlr\src\core\syrec\parser\antlr\SyReC.g4 by ANTLR 4.11.1

#pragma once


#include "antlr4-runtime.h"


namespace parser {


class  SyReCParser : public antlr4::Parser {
public:
  enum {
    T__0 = 1, T__1 = 2, T__2 = 3, T__3 = 4, T__4 = 5, T__5 = 6, T__6 = 7, 
    T__7 = 8, T__8 = 9, T__9 = 10, T__10 = 11, T__11 = 12, T__12 = 13, T__13 = 14, 
    T__14 = 15, T__15 = 16, T__16 = 17, T__17 = 18, T__18 = 19, T__19 = 20, 
    T__20 = 21, T__21 = 22, T__22 = 23, T__23 = 24, T__24 = 25, T__25 = 26, 
    T__26 = 27, T__27 = 28, T__28 = 29, T__29 = 30, T__30 = 31, T__31 = 32, 
    T__32 = 33, T__33 = 34, T__34 = 35, T__35 = 36, T__36 = 37, T__37 = 38, 
    T__38 = 39, T__39 = 40, T__40 = 41, T__41 = 42, T__42 = 43, T__43 = 44, 
    T__44 = 45, T__45 = 46, T__46 = 47, T__47 = 48, T__48 = 49, T__49 = 50, 
    T__50 = 51, IDENT = 52, INT = 53, SKIPABLEWHITSPACES = 54
  };

  enum {
    RuleNumber = 0, RuleProgram = 1, RuleModule = 2, RuleParameterList = 3, 
    RuleParameter = 4, RuleParameterType = 5, RuleSignalList = 6, RuleSignalType = 7, 
    RuleSignalDeclaration = 8, RuleStatementList = 9, RuleStatement = 10, 
    RuleCallStatement = 11, RuleForStatement = 12, RuleIfStatement = 13, 
    RuleUnaryStatement = 14, RuleAssignStatement = 15, RuleSwapStatement = 16, 
    RuleSkipStatement = 17, RuleSignal = 18, RuleExpression = 19, RuleBinaryExpression = 20, 
    RuleUnaryExpression = 21, RuleShiftExpression = 22
  };

  explicit SyReCParser(antlr4::TokenStream *input);

  SyReCParser(antlr4::TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options);

  ~SyReCParser() override;

  std::string getGrammarFileName() const override;

  const antlr4::atn::ATN& getATN() const override;

  const std::vector<std::string>& getRuleNames() const override;

  const antlr4::dfa::Vocabulary& getVocabulary() const override;

  antlr4::atn::SerializedATNView getSerializedATN() const override;


  class NumberContext;
  class ProgramContext;
  class ModuleContext;
  class ParameterListContext;
  class ParameterContext;
  class ParameterTypeContext;
  class SignalListContext;
  class SignalTypeContext;
  class SignalDeclarationContext;
  class StatementListContext;
  class StatementContext;
  class CallStatementContext;
  class ForStatementContext;
  class IfStatementContext;
  class UnaryStatementContext;
  class AssignStatementContext;
  class SwapStatementContext;
  class SkipStatementContext;
  class SignalContext;
  class ExpressionContext;
  class BinaryExpressionContext;
  class UnaryExpressionContext;
  class ShiftExpressionContext; 

  class  NumberContext : public antlr4::ParserRuleContext {
  public:
    NumberContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *INT();
    antlr4::tree::TerminalNode *IDENT();
    std::vector<NumberContext *> number();
    NumberContext* number(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  NumberContext* number();

  class  ProgramContext : public antlr4::ParserRuleContext {
  public:
    ProgramContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<ModuleContext *> module();
    ModuleContext* module(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ProgramContext* program();

  class  ModuleContext : public antlr4::ParserRuleContext {
  public:
    ModuleContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *IDENT();
    ParameterListContext *parameterList();
    StatementListContext *statementList();
    std::vector<SignalListContext *> signalList();
    SignalListContext* signalList(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ModuleContext* module();

  class  ParameterListContext : public antlr4::ParserRuleContext {
  public:
    ParameterListContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<ParameterContext *> parameter();
    ParameterContext* parameter(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ParameterListContext* parameterList();

  class  ParameterContext : public antlr4::ParserRuleContext {
  public:
    ParameterContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ParameterTypeContext *parameterType();
    SignalDeclarationContext *signalDeclaration();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ParameterContext* parameter();

  class  ParameterTypeContext : public antlr4::ParserRuleContext {
  public:
    ParameterTypeContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    ParameterTypeContext() = default;
    void copyFrom(ParameterTypeContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  InSignalTypeContext : public ParameterTypeContext {
  public:
    InSignalTypeContext(ParameterTypeContext *ctx);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  OutSignalTypeContext : public ParameterTypeContext {
  public:
    OutSignalTypeContext(ParameterTypeContext *ctx);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  InoutSignalTypeContext : public ParameterTypeContext {
  public:
    InoutSignalTypeContext(ParameterTypeContext *ctx);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  ParameterTypeContext* parameterType();

  class  SignalListContext : public antlr4::ParserRuleContext {
  public:
    SignalListContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    SignalTypeContext *signalType();
    std::vector<SignalDeclarationContext *> signalDeclaration();
    SignalDeclarationContext* signalDeclaration(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  SignalListContext* signalList();

  class  SignalTypeContext : public antlr4::ParserRuleContext {
  public:
    SignalTypeContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    SignalTypeContext() = default;
    void copyFrom(SignalTypeContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  StateSignalTypeContext : public SignalTypeContext {
  public:
    StateSignalTypeContext(SignalTypeContext *ctx);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  WireSignalTypeContext : public SignalTypeContext {
  public:
    WireSignalTypeContext(SignalTypeContext *ctx);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  SignalTypeContext* signalType();

  class  SignalDeclarationContext : public antlr4::ParserRuleContext {
  public:
    antlr4::Token *intToken = nullptr;
    std::vector<antlr4::Token *> dimensionTokens;
    antlr4::Token *signalWidthToken = nullptr;
    SignalDeclarationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *IDENT();
    std::vector<antlr4::tree::TerminalNode *> INT();
    antlr4::tree::TerminalNode* INT(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  SignalDeclarationContext* signalDeclaration();

  class  StatementListContext : public antlr4::ParserRuleContext {
  public:
    StatementListContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<StatementContext *> statement();
    StatementContext* statement(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  StatementListContext* statementList();

  class  StatementContext : public antlr4::ParserRuleContext {
  public:
    StatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    CallStatementContext *callStatement();
    ForStatementContext *forStatement();
    IfStatementContext *ifStatement();
    UnaryStatementContext *unaryStatement();
    AssignStatementContext *assignStatement();
    SwapStatementContext *swapStatement();
    SkipStatementContext *skipStatement();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  StatementContext* statement();

  class  CallStatementContext : public antlr4::ParserRuleContext {
  public:
    CallStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<antlr4::tree::TerminalNode *> IDENT();
    antlr4::tree::TerminalNode* IDENT(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  CallStatementContext* callStatement();

  class  ForStatementContext : public antlr4::ParserRuleContext {
  public:
    ForStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<NumberContext *> number();
    NumberContext* number(size_t i);
    StatementListContext *statementList();
    antlr4::tree::TerminalNode *IDENT();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ForStatementContext* forStatement();

  class  IfStatementContext : public antlr4::ParserRuleContext {
  public:
    IfStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<ExpressionContext *> expression();
    ExpressionContext* expression(size_t i);
    std::vector<StatementListContext *> statementList();
    StatementListContext* statementList(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  IfStatementContext* ifStatement();

  class  UnaryStatementContext : public antlr4::ParserRuleContext {
  public:
    UnaryStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    SignalContext *signal();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  UnaryStatementContext* unaryStatement();

  class  AssignStatementContext : public antlr4::ParserRuleContext {
  public:
    AssignStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    SignalContext *signal();
    ExpressionContext *expression();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  AssignStatementContext* assignStatement();

  class  SwapStatementContext : public antlr4::ParserRuleContext {
  public:
    SwapStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<SignalContext *> signal();
    SignalContext* signal(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  SwapStatementContext* swapStatement();

  class  SkipStatementContext : public antlr4::ParserRuleContext {
  public:
    SkipStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  SkipStatementContext* skipStatement();

  class  SignalContext : public antlr4::ParserRuleContext {
  public:
    SignalContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *IDENT();
    std::vector<ExpressionContext *> expression();
    ExpressionContext* expression(size_t i);
    std::vector<NumberContext *> number();
    NumberContext* number(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  SignalContext* signal();

  class  ExpressionContext : public antlr4::ParserRuleContext {
  public:
    ExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    NumberContext *number();
    SignalContext *signal();
    BinaryExpressionContext *binaryExpression();
    UnaryExpressionContext *unaryExpression();
    ShiftExpressionContext *shiftExpression();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ExpressionContext* expression();

  class  BinaryExpressionContext : public antlr4::ParserRuleContext {
  public:
    BinaryExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<ExpressionContext *> expression();
    ExpressionContext* expression(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  BinaryExpressionContext* binaryExpression();

  class  UnaryExpressionContext : public antlr4::ParserRuleContext {
  public:
    UnaryExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ExpressionContext *expression();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  UnaryExpressionContext* unaryExpression();

  class  ShiftExpressionContext : public antlr4::ParserRuleContext {
  public:
    ShiftExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ExpressionContext *expression();
    NumberContext *number();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ShiftExpressionContext* shiftExpression();


  // By default the static state used to implement the parser is lazily initialized during the first
  // call to the constructor. You can call this function if you wish to initialize the static state
  // ahead of time.
  static void initialize();

private:
};

}  // namespace parser
