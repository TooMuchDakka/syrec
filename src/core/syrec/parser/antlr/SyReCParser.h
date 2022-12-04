
// Generated from ..\syrec_antlr\src\core\syrec\parser\antlr\SyReC.g4 by ANTLR 4.11.1

#pragma once


#include "antlr4-runtime.h"


namespace parser {


class  SyReCParser : public antlr4::Parser {
public:
  enum {
    T__0 = 1, T__1 = 2, T__2 = 3, T__3 = 4, T__4 = 5, T__5 = 6, T__6 = 7, 
    T__7 = 8, T__8 = 9, T__9 = 10, T__10 = 11, T__11 = 12, T__12 = 13, T__13 = 14, 
    T__14 = 15, T__15 = 16, T__16 = 17, T__17 = 18, T__18 = 19, OP_PLUS = 20, 
    OP_MINUS = 21, OP_MULTIPLY = 22, OP_XOR = 23, OP_UPPER_BIT_MULTIPLY = 24, 
    OP_DIVISION = 25, OP_MODULO = 26, OP_GREATER_THAN = 27, OP_GREATER_OR_EQUAL = 28, 
    OP_LESS_THAN = 29, OP_LESS_OR_EQUAL = 30, OP_EQUAL = 31, OP_NOT_EQUAL = 32, 
    OP_BITWISE_AND = 33, OP_BITWISE_OR = 34, OP_BITWISE_NEGATION = 35, OP_LOGICAL_AND = 36, 
    OP_LOGICAL_OR = 37, OP_LOGICAL_NEGATION = 38, OP_LEFT_SHIFT = 39, OP_RIGHT_SHIFT = 40, 
    OP_INCREMENT = 41, OP_DECREMENT = 42, VAR_TYPE_IN = 43, VAR_TYPE_OUT = 44, 
    VAR_TYPE_INOUT = 45, VAR_TYPE_WIRE = 46, VAR_TYPE_STATE = 47, LOOP_VARIABLE_PREFIX = 48, 
    SIGNAL_WIDTH_PREFIX = 49, STATEMENT_DELIMITER = 50, PARAMETER_DELIMITER = 51, 
    IDENT = 52, INT = 53, SKIPABLEWHITSPACES = 54
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
    SyReCParser::NumberContext *lhsOperand = nullptr;
    SyReCParser::NumberContext *rhsOperand = nullptr;
    NumberContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *INT();
    antlr4::tree::TerminalNode *SIGNAL_WIDTH_PREFIX();
    antlr4::tree::TerminalNode *IDENT();
    antlr4::tree::TerminalNode *LOOP_VARIABLE_PREFIX();
    std::vector<NumberContext *> number();
    NumberContext* number(size_t i);
    antlr4::tree::TerminalNode *OP_PLUS();
    antlr4::tree::TerminalNode *OP_MINUS();
    antlr4::tree::TerminalNode *OP_MULTIPLY();
    antlr4::tree::TerminalNode *OP_DIVISION();


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
    std::vector<antlr4::tree::TerminalNode *> PARAMETER_DELIMITER();
    antlr4::tree::TerminalNode* PARAMETER_DELIMITER(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ParameterListContext* parameterList();

  class  ParameterContext : public antlr4::ParserRuleContext {
  public:
    ParameterContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    SignalDeclarationContext *signalDeclaration();
    antlr4::tree::TerminalNode *VAR_TYPE_IN();
    antlr4::tree::TerminalNode *VAR_TYPE_OUT();
    antlr4::tree::TerminalNode *VAR_TYPE_INOUT();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ParameterContext* parameter();

  class  SignalListContext : public antlr4::ParserRuleContext {
  public:
    SignalListContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<SignalDeclarationContext *> signalDeclaration();
    SignalDeclarationContext* signalDeclaration(size_t i);
    antlr4::tree::TerminalNode *VAR_TYPE_WIRE();
    antlr4::tree::TerminalNode *VAR_TYPE_STATE();
    std::vector<antlr4::tree::TerminalNode *> PARAMETER_DELIMITER();
    antlr4::tree::TerminalNode* PARAMETER_DELIMITER(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  SignalListContext* signalList();

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
    SyReCParser::StatementContext *statementContext = nullptr;
    std::vector<StatementContext *> stmts;
    StatementListContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<StatementContext *> statement();
    StatementContext* statement(size_t i);
    std::vector<antlr4::tree::TerminalNode *> STATEMENT_DELIMITER();
    antlr4::tree::TerminalNode* STATEMENT_DELIMITER(size_t i);


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
    antlr4::Token *moduleIdent = nullptr;
    antlr4::Token *identToken = nullptr;
    std::vector<antlr4::Token *> calleArguments;
    std::vector<antlr4::Token *> calleeArguments;
    CallStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<antlr4::tree::TerminalNode *> IDENT();
    antlr4::tree::TerminalNode* IDENT(size_t i);
    std::vector<antlr4::tree::TerminalNode *> PARAMETER_DELIMITER();
    antlr4::tree::TerminalNode* PARAMETER_DELIMITER(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  CallStatementContext* callStatement();

  class  ForStatementContext : public antlr4::ParserRuleContext {
  public:
    antlr4::Token *variableIdent = nullptr;
    SyReCParser::NumberContext *startValue = nullptr;
    SyReCParser::NumberContext *endValue = nullptr;
    SyReCParser::NumberContext *stepSize = nullptr;
    ForStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    StatementListContext *statementList();
    std::vector<NumberContext *> number();
    NumberContext* number(size_t i);
    antlr4::tree::TerminalNode *LOOP_VARIABLE_PREFIX();
    antlr4::tree::TerminalNode *OP_EQUAL();
    antlr4::tree::TerminalNode *OP_MINUS();
    antlr4::tree::TerminalNode *IDENT();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ForStatementContext* forStatement();

  class  IfStatementContext : public antlr4::ParserRuleContext {
  public:
    SyReCParser::ExpressionContext *guardCondition = nullptr;
    SyReCParser::StatementListContext *trueBranchStmts = nullptr;
    SyReCParser::StatementListContext *falseBranchStmts = nullptr;
    SyReCParser::ExpressionContext *matchingGuardExpression = nullptr;
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
    antlr4::tree::TerminalNode *OP_EQUAL();
    SignalContext *signal();
    antlr4::tree::TerminalNode *OP_BITWISE_NEGATION();
    antlr4::tree::TerminalNode *OP_INCREMENT();
    antlr4::tree::TerminalNode *OP_DECREMENT();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  UnaryStatementContext* unaryStatement();

  class  AssignStatementContext : public antlr4::ParserRuleContext {
  public:
    AssignStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    SignalContext *signal();
    antlr4::tree::TerminalNode *OP_EQUAL();
    ExpressionContext *expression();
    antlr4::tree::TerminalNode *OP_XOR();
    antlr4::tree::TerminalNode *OP_PLUS();
    antlr4::tree::TerminalNode *OP_MINUS();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  AssignStatementContext* assignStatement();

  class  SwapStatementContext : public antlr4::ParserRuleContext {
  public:
    SyReCParser::SignalContext *lhsOperand = nullptr;
    SyReCParser::SignalContext *rhsOperand = nullptr;
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
    SyReCParser::NumberContext *bitStart = nullptr;
    SyReCParser::NumberContext *bitRangeEnd = nullptr;
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
    SyReCParser::ExpressionContext *lhsOperand = nullptr;
    SyReCParser::ExpressionContext *rhsOperand = nullptr;
    BinaryExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<ExpressionContext *> expression();
    ExpressionContext* expression(size_t i);
    antlr4::tree::TerminalNode *OP_PLUS();
    antlr4::tree::TerminalNode *OP_MINUS();
    antlr4::tree::TerminalNode *OP_XOR();
    antlr4::tree::TerminalNode *OP_MULTIPLY();
    antlr4::tree::TerminalNode *OP_DIVISION();
    antlr4::tree::TerminalNode *OP_MODULO();
    antlr4::tree::TerminalNode *OP_UPPER_BIT_MULTIPLY();
    antlr4::tree::TerminalNode *OP_LOGICAL_AND();
    antlr4::tree::TerminalNode *OP_LOGICAL_OR();
    antlr4::tree::TerminalNode *OP_BITWISE_AND();
    antlr4::tree::TerminalNode *OP_BITWISE_OR();
    antlr4::tree::TerminalNode *OP_LESS_THAN();
    antlr4::tree::TerminalNode *OP_GREATER_THAN();
    antlr4::tree::TerminalNode *OP_EQUAL();
    antlr4::tree::TerminalNode *OP_NOT_EQUAL();
    antlr4::tree::TerminalNode *OP_LESS_OR_EQUAL();
    antlr4::tree::TerminalNode *OP_GREATER_OR_EQUAL();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  BinaryExpressionContext* binaryExpression();

  class  UnaryExpressionContext : public antlr4::ParserRuleContext {
  public:
    UnaryExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ExpressionContext *expression();
    antlr4::tree::TerminalNode *OP_LOGICAL_NEGATION();
    antlr4::tree::TerminalNode *OP_BITWISE_NEGATION();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  UnaryExpressionContext* unaryExpression();

  class  ShiftExpressionContext : public antlr4::ParserRuleContext {
  public:
    ShiftExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ExpressionContext *expression();
    NumberContext *number();
    antlr4::tree::TerminalNode *OP_RIGHT_SHIFT();
    antlr4::tree::TerminalNode *OP_LEFT_SHIFT();


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
