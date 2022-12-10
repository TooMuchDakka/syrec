
// Generated from ..\syrec_antlr\src\core\syrec\parser\antlr\SyReC.g4 by ANTLR 4.11.1

#pragma once


#include "antlr4-runtime.h"


namespace parser {


class  SyReCParser : public antlr4::Parser {
public:
  enum {
    T__0 = 1, T__1 = 2, T__2 = 3, T__3 = 4, T__4 = 5, T__5 = 6, T__6 = 7, 
    T__7 = 8, T__8 = 9, T__9 = 10, T__10 = 11, T__11 = 12, T__12 = 13, T__13 = 14, 
    T__14 = 15, T__15 = 16, T__16 = 17, T__17 = 18, OP_PLUS = 19, OP_MINUS = 20, 
    OP_MULTIPLY = 21, OP_UPPER_BIT_MULTIPLY = 22, OP_DIVISION = 23, OP_MODULO = 24, 
    OP_GREATER_THAN = 25, OP_GREATER_OR_EQUAL = 26, OP_LESS_THAN = 27, OP_LESS_OR_EQUAL = 28, 
    OP_EQUAL = 29, OP_NOT_EQUAL = 30, OP_BITWISE_AND = 31, OP_BITWISE_NEGATION = 32, 
    OP_BITWISE_OR = 33, OP_BITWISE_XOR = 34, OP_LOGICAL_AND = 35, OP_LOGICAL_OR = 36, 
    OP_LOGICAL_NEGATION = 37, OP_LEFT_SHIFT = 38, OP_RIGHT_SHIFT = 39, OP_INCREMENT_ASSIGN = 40, 
    OP_DECREMENT_ASSIGN = 41, OP_INVERT_ASSIGN = 42, OP_ADD_ASSIGN = 43, 
    OP_SUB_ASSIGN = 44, OP_XOR_ASSIGN = 45, OP_CALL = 46, OP_UNCALL = 47, 
    VAR_TYPE_IN = 48, VAR_TYPE_OUT = 49, VAR_TYPE_INOUT = 50, VAR_TYPE_WIRE = 51, 
    VAR_TYPE_STATE = 52, LOOP_VARIABLE_PREFIX = 53, SIGNAL_WIDTH_PREFIX = 54, 
    STATEMENT_DELIMITER = 55, PARAMETER_DELIMITER = 56, IDENT = 57, INT = 58, 
    SKIPABLEWHITSPACES = 59
  };

  enum {
    RuleNumber = 0, RuleProgram = 1, RuleModule = 2, RuleParameterList = 3, 
    RuleParameter = 4, RuleSignalList = 5, RuleSignalDeclaration = 6, RuleStatementList = 7, 
    RuleStatement = 8, RuleCallStatement = 9, RuleLoopVariableDefinition = 10, 
    RuleLoopStepsizeDefinition = 11, RuleForStatement = 12, RuleIfStatement = 13, 
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
  class SignalListContext;
  class SignalDeclarationContext;
  class StatementListContext;
  class StatementContext;
  class CallStatementContext;
  class LoopVariableDefinitionContext;
  class LoopStepsizeDefinitionContext;
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
   
    NumberContext() = default;
    void copyFrom(NumberContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  NumberFromSignalwidthContext : public NumberContext {
  public:
    NumberFromSignalwidthContext(NumberContext *ctx);

    antlr4::tree::TerminalNode *SIGNAL_WIDTH_PREFIX();
    antlr4::tree::TerminalNode *IDENT();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  NumberFromLoopVariableContext : public NumberContext {
  public:
    NumberFromLoopVariableContext(NumberContext *ctx);

    antlr4::tree::TerminalNode *LOOP_VARIABLE_PREFIX();
    antlr4::tree::TerminalNode *IDENT();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  NumberFromConstantContext : public NumberContext {
  public:
    NumberFromConstantContext(NumberContext *ctx);

    antlr4::tree::TerminalNode *INT();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  NumberFromExpressionContext : public NumberContext {
  public:
    NumberFromExpressionContext(NumberContext *ctx);

    SyReCParser::NumberContext *lhsOperand = nullptr;
    antlr4::Token *op = nullptr;
    SyReCParser::NumberContext *rhsOperand = nullptr;
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
    antlr4::tree::TerminalNode *EOF();
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
    StatementListContext *statementList();
    ParameterListContext *parameterList();
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
    std::vector<antlr4::Token *> calleeArguments;
    CallStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *OP_CALL();
    antlr4::tree::TerminalNode *OP_UNCALL();
    std::vector<antlr4::tree::TerminalNode *> IDENT();
    antlr4::tree::TerminalNode* IDENT(size_t i);
    std::vector<antlr4::tree::TerminalNode *> PARAMETER_DELIMITER();
    antlr4::tree::TerminalNode* PARAMETER_DELIMITER(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  CallStatementContext* callStatement();

  class  LoopVariableDefinitionContext : public antlr4::ParserRuleContext {
  public:
    antlr4::Token *variableIdent = nullptr;
    LoopVariableDefinitionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *LOOP_VARIABLE_PREFIX();
    antlr4::tree::TerminalNode *OP_EQUAL();
    antlr4::tree::TerminalNode *IDENT();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  LoopVariableDefinitionContext* loopVariableDefinition();

  class  LoopStepsizeDefinitionContext : public antlr4::ParserRuleContext {
  public:
    LoopStepsizeDefinitionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    NumberContext *number();
    antlr4::tree::TerminalNode *OP_MINUS();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  LoopStepsizeDefinitionContext* loopStepsizeDefinition();

  class  ForStatementContext : public antlr4::ParserRuleContext {
  public:
    SyReCParser::NumberContext *startValue = nullptr;
    SyReCParser::NumberContext *endValue = nullptr;
    ForStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    StatementListContext *statementList();
    std::vector<NumberContext *> number();
    NumberContext* number(size_t i);
    LoopStepsizeDefinitionContext *loopStepsizeDefinition();
    LoopVariableDefinitionContext *loopVariableDefinition();


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
    antlr4::Token *unaryOp = nullptr;
    UnaryStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    SignalContext *signal();
    antlr4::tree::TerminalNode *OP_INVERT_ASSIGN();
    antlr4::tree::TerminalNode *OP_INCREMENT_ASSIGN();
    antlr4::tree::TerminalNode *OP_DECREMENT_ASSIGN();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  UnaryStatementContext* unaryStatement();

  class  AssignStatementContext : public antlr4::ParserRuleContext {
  public:
    antlr4::Token *assignmentOp = nullptr;
    AssignStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    SignalContext *signal();
    ExpressionContext *expression();
    antlr4::tree::TerminalNode *OP_ADD_ASSIGN();
    antlr4::tree::TerminalNode *OP_SUB_ASSIGN();
    antlr4::tree::TerminalNode *OP_XOR_ASSIGN();


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
    SyReCParser::ExpressionContext *expressionContext = nullptr;
    std::vector<ExpressionContext *> accessedDimensions;
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
   
    ExpressionContext() = default;
    void copyFrom(ExpressionContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  ExpressionFromSignalContext : public ExpressionContext {
  public:
    ExpressionFromSignalContext(ExpressionContext *ctx);

    SignalContext *signal();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  ExpressionFromBinaryExpressionContext : public ExpressionContext {
  public:
    ExpressionFromBinaryExpressionContext(ExpressionContext *ctx);

    BinaryExpressionContext *binaryExpression();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  ExpressionFromNumberContext : public ExpressionContext {
  public:
    ExpressionFromNumberContext(ExpressionContext *ctx);

    NumberContext *number();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  ExpressionFromUnaryExpressionContext : public ExpressionContext {
  public:
    ExpressionFromUnaryExpressionContext(ExpressionContext *ctx);

    UnaryExpressionContext *unaryExpression();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  ExpressionFromShiftExpressionContext : public ExpressionContext {
  public:
    ExpressionFromShiftExpressionContext(ExpressionContext *ctx);

    ShiftExpressionContext *shiftExpression();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  ExpressionContext* expression();

  class  BinaryExpressionContext : public antlr4::ParserRuleContext {
  public:
    SyReCParser::ExpressionContext *lhsOperand = nullptr;
    antlr4::Token *binaryOperation = nullptr;
    SyReCParser::ExpressionContext *rhsOperand = nullptr;
    BinaryExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<ExpressionContext *> expression();
    ExpressionContext* expression(size_t i);
    antlr4::tree::TerminalNode *OP_PLUS();
    antlr4::tree::TerminalNode *OP_MINUS();
    antlr4::tree::TerminalNode *OP_MULTIPLY();
    antlr4::tree::TerminalNode *OP_DIVISION();
    antlr4::tree::TerminalNode *OP_MODULO();
    antlr4::tree::TerminalNode *OP_UPPER_BIT_MULTIPLY();
    antlr4::tree::TerminalNode *OP_LOGICAL_AND();
    antlr4::tree::TerminalNode *OP_LOGICAL_OR();
    antlr4::tree::TerminalNode *OP_BITWISE_AND();
    antlr4::tree::TerminalNode *OP_BITWISE_OR();
    antlr4::tree::TerminalNode *OP_BITWISE_XOR();
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
    antlr4::Token *unaryOperation = nullptr;
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
    antlr4::Token *shiftOperation = nullptr;
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
