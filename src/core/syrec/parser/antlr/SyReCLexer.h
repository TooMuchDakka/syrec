
// Generated from ..\syrec_antlr\src\core\syrec\parser\antlr\SyReC.g4 by ANTLR 4.11.1

#pragma once


#include "antlr4-runtime.h"


namespace parser {


class  SyReCLexer : public antlr4::Lexer {
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
