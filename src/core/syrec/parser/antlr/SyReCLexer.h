
// Generated from ..\syrec_antlr\src\core\syrec\parser\antlr\SyReC.g4 by ANTLR 4.11.1

#pragma once


#include "antlr4-runtime.h"


namespace parser {


class  SyReCLexer : public antlr4::Lexer {
public:
  enum {
    T__0 = 1, T__1 = 2, T__2 = 3, T__3 = 4, T__4 = 5, T__5 = 6, T__6 = 7, 
    T__7 = 8, T__8 = 9, T__9 = 10, T__10 = 11, T__11 = 12, T__12 = 13, T__13 = 14, 
    T__14 = 15, T__15 = 16, T__16 = 17, T__17 = 18, OP_INCREMENT_ASSIGN = 19, 
    OP_DECREMENT_ASSIGN = 20, OP_INVERT_ASSIGN = 21, OP_ADD_ASSIGN = 22, 
    OP_SUB_ASSIGN = 23, OP_XOR_ASSIGN = 24, OP_PLUS = 25, OP_MINUS = 26, 
    OP_MULTIPLY = 27, OP_UPPER_BIT_MULTIPLY = 28, OP_DIVISION = 29, OP_MODULO = 30, 
    OP_LEFT_SHIFT = 31, OP_RIGHT_SHIFT = 32, OP_GREATER_OR_EQUAL = 33, OP_LESS_OR_EQUAL = 34, 
    OP_GREATER_THAN = 35, OP_LESS_THAN = 36, OP_EQUAL = 37, OP_NOT_EQUAL = 38, 
    OP_LOGICAL_AND = 39, OP_LOGICAL_OR = 40, OP_LOGICAL_NEGATION = 41, OP_BITWISE_AND = 42, 
    OP_BITWISE_NEGATION = 43, OP_BITWISE_OR = 44, OP_BITWISE_XOR = 45, OP_CALL = 46, 
    OP_UNCALL = 47, VAR_TYPE_IN = 48, VAR_TYPE_OUT = 49, VAR_TYPE_INOUT = 50, 
    VAR_TYPE_WIRE = 51, VAR_TYPE_STATE = 52, LOOP_VARIABLE_PREFIX = 53, 
    SIGNAL_WIDTH_PREFIX = 54, STATEMENT_DELIMITER = 55, PARAMETER_DELIMITER = 56, 
    SKIPABLEWHITSPACES = 57, LINE_COMMENT = 58, MULTI_LINE_COMMENT = 59, 
    IDENT = 60, INT = 61
  };

  explicit SyReCLexer(antlr4::CharStream *input);

  ~SyReCLexer() override;


  std::string getGrammarFileName() const override;

  const std::vector<std::string>& getRuleNames() const override;

  const std::vector<std::string>& getChannelNames() const override;

  const std::vector<std::string>& getModeNames() const override;

  const antlr4::dfa::Vocabulary& getVocabulary() const override;

  antlr4::atn::SerializedATNView getSerializedATN() const override;

  const antlr4::atn::ATN& getATN() const override;

  // By default the static state used to implement the lexer is lazily initialized during the first
  // call to the constructor. You can call this function if you wish to initialize the static state
  // ahead of time.
  static void initialize();

private:

  // Individual action functions triggered by action() above.

  // Individual semantic predicate functions triggered by sempred() above.

};

}  // namespace parser
