
// Generated from ..\syrec_antlr\src\core\syrec\parser\antlr\SyReC.g4 by ANTLR 4.11.1

#pragma once


#include "antlr4-runtime.h"


namespace parser {


class  SyReCLexer : public antlr4::Lexer {
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
