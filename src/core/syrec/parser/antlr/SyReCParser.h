
#include <vector>
#include <string>


// Generated from ..\syrec_antlr\src\core\syrec\parser\antlr\SyReC.g4 by ANTLR 4.11.1

#pragma once


#include "antlr4-runtime.h"


namespace syrec {


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
    RuleParameter = 4, RuleSignalList = 5, RuleSignalDeclaration = 6, RuleStatementList = 7, 
    RuleStatement = 8, RuleCallStatement = 9, RuleForStatement = 10, RuleIfStatement = 11, 
    RuleUnaryStatement = 12, RuleAssignStatement = 13, RuleSwapStatement = 14, 
    RuleSkipStatement = 15, RuleSignal = 16, RuleExpression = 17, RuleBinaryExpression = 18, 
    RuleUnaryExpression = 19, RuleShiftExpression = 20
  };

  explicit SyReCParser(antlr4::TokenStream *input);

  SyReCParser(antlr4::TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options);

  ~SyReCParser() override;

  std::string getGrammarFileName() const override;

  const antlr4::atn::ATN& getATN() const override;

  const std::vector<std::string>& getRuleNames() const override;

  const antlr4::dfa::Vocabulary& getVocabulary() const override;

  antlr4::atn::SerializedATNView getSerializedATN() const override;


  public:
  	void Parse() {}

  private:
  	void Test() {}


  class NumberContext;
  class ProgramContext;
  class ModuleContext;
  class ParameterListContext;
  class ParameterContext;
  class SignalListContext;
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

   
  };

  NumberContext* number();

  class  ProgramContext : public antlr4::ParserRuleContext {
  public:
    ProgramContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<ModuleContext *> module();
    ModuleContext* module(size_t i);

   
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

   
  };

  ModuleContext* module();

  class  ParameterListContext : public antlr4::ParserRuleContext {
  public:
    ParameterListContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<ParameterContext *> parameter();
    ParameterContext* parameter(size_t i);

   
  };

  ParameterListContext* parameterList();

  class  ParameterContext : public antlr4::ParserRuleContext {
  public:
    ParameterContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    SignalDeclarationContext *signalDeclaration();

   
  };

  ParameterContext* parameter();

  class  SignalListContext : public antlr4::ParserRuleContext {
  public:
    SignalListContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<SignalDeclarationContext *> signalDeclaration();
    SignalDeclarationContext* signalDeclaration(size_t i);

   
  };

  SignalListContext* signalList();

  class  SignalDeclarationContext : public antlr4::ParserRuleContext {
  public:
    SignalDeclarationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *IDENT();
    std::vector<antlr4::tree::TerminalNode *> INT();
    antlr4::tree::TerminalNode* INT(size_t i);

   
  };

  SignalDeclarationContext* signalDeclaration();

  class  StatementListContext : public antlr4::ParserRuleContext {
  public:
    StatementListContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<StatementContext *> statement();
    StatementContext* statement(size_t i);

   
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

   
  };

  StatementContext* statement();

  class  CallStatementContext : public antlr4::ParserRuleContext {
  public:
    CallStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<antlr4::tree::TerminalNode *> IDENT();
    antlr4::tree::TerminalNode* IDENT(size_t i);

   
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

   
  };

  IfStatementContext* ifStatement();

  class  UnaryStatementContext : public antlr4::ParserRuleContext {
  public:
    UnaryStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    SignalContext *signal();

   
  };

  UnaryStatementContext* unaryStatement();

  class  AssignStatementContext : public antlr4::ParserRuleContext {
  public:
    AssignStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    SignalContext *signal();
    ExpressionContext *expression();

   
  };

  AssignStatementContext* assignStatement();

  class  SwapStatementContext : public antlr4::ParserRuleContext {
  public:
    SwapStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<SignalContext *> signal();
    SignalContext* signal(size_t i);

   
  };

  SwapStatementContext* swapStatement();

  class  SkipStatementContext : public antlr4::ParserRuleContext {
  public:
    SkipStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;

   
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

   
  };

  ExpressionContext* expression();

  class  BinaryExpressionContext : public antlr4::ParserRuleContext {
  public:
    BinaryExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<ExpressionContext *> expression();
    ExpressionContext* expression(size_t i);

   
  };

  BinaryExpressionContext* binaryExpression();

  class  UnaryExpressionContext : public antlr4::ParserRuleContext {
  public:
    UnaryExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ExpressionContext *expression();

   
  };

  UnaryExpressionContext* unaryExpression();

  class  ShiftExpressionContext : public antlr4::ParserRuleContext {
  public:
    ShiftExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ExpressionContext *expression();
    NumberContext *number();

   
  };

  ShiftExpressionContext* shiftExpression();


  // By default the static state used to implement the parser is lazily initialized during the first
  // call to the constructor. You can call this function if you wish to initialize the static state
  // ahead of time.
  static void initialize();

private:
};

}  // namespace syrec
