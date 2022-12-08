
// Generated from ..\syrec_antlr\src\core\syrec\parser\antlr\SyReC.g4 by ANTLR 4.11.1

#pragma once


#include "antlr4-runtime.h"


namespace parser {


class  SyReCLexer : public antlr4::Lexer {
public:
  enum {
    T__0 = 1, T__1 = 2, T__2 = 3, T__3 = 4, T__4 = 5, T__5 = 6, T__6 = 7, 
    T__7 = 8, T__8 = 9, T__9 = 10, T__10 = 11, T__11 = 12, T__12 = 13, T__13 = 14, 
    T__14 = 15, T__15 = 16, T__16 = 17, T__17 = 18, T__18 = 19, T__19 = 20, 
    OP_PLUS = 21, OP_MINUS = 22, OP_MULTIPLY = 23, OP_UPPER_BIT_MULTIPLY = 24, 
    OP_DIVISION = 25, OP_MODULO = 26, OP_GREATER_THAN = 27, OP_GREATER_OR_EQUAL = 28, 
    OP_LESS_THAN = 29, OP_LESS_OR_EQUAL = 30, OP_EQUAL = 31, OP_NOT_EQUAL = 32, 
    OP_BITWISE_AND = 33, OP_BITWISE_NEGATION = 34, OP_BITWISE_OR = 35, OP_BITWISE_XOR = 36, 
    OP_LOGICAL_AND = 37, OP_LOGICAL_OR = 38, OP_LOGICAL_NEGATION = 39, OP_LEFT_SHIFT = 40, 
    OP_RIGHT_SHIFT = 41, OP_INCREMENT_ASSIGN = 42, OP_DECREMENT_ASSIGN = 43, 
    OP_INVERT_ASSIGN = 44, OP_ADD_ASSIGN = 45, OP_SUB_ASSIGN = 46, OP_XOR_ASSIGN = 47, 
    VAR_TYPE_IN = 48, VAR_TYPE_OUT = 49, VAR_TYPE_INOUT = 50, VAR_TYPE_WIRE = 51, 
    VAR_TYPE_STATE = 52, LOOP_VARIABLE_PREFIX = 53, SIGNAL_WIDTH_PREFIX = 54, 
    STATEMENT_DELIMITER = 55, PARAMETER_DELIMITER = 56, LINE_COMMENT = 57, 
    IDENT = 58, INT = 59, SKIPABLEWHITSPACES = 60
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
