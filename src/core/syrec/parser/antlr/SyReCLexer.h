
// Generated from ..\syrec_antlr\src\core\syrec\parser\antlr\SyReC.g4 by ANTLR 4.11.1

#pragma once


#include "antlr4-runtime.h"


namespace parser {


class  SyReCLexer : public antlr4::Lexer {
public:
  enum {
    T__0 = 1, T__1 = 2, T__2 = 3, T__3 = 4, T__4 = 5, T__5 = 6, T__6 = 7, 
    T__7 = 8, T__8 = 9, T__9 = 10, T__10 = 11, T__11 = 12, T__12 = 13, T__13 = 14, 
    T__14 = 15, T__15 = 16, T__16 = 17, T__17 = 18, T__18 = 19, IDENT = 20, 
    INT = 21, SKIPABLEWHITSPACES = 22, OP_PLUS = 23, OP_MINUS = 24, OP_MULTIPLY = 25, 
    OP_XOR = 26, OP_UPPER_BIT_MULTIPLY = 27, OP_DIVISION = 28, OP_MODULO = 29, 
    OP_GREATER_THAN = 30, OP_GREATER_OR_EQUAL = 31, OP_LESS_THAN = 32, OP_LESS_OR_EQUAL = 33, 
    OP_EQUAL = 34, OP_NOT_EQUAL = 35, OP_BITWISE_AND = 36, OP_BITWISE_OR = 37, 
    OP_BITWISE_NEGATION = 38, OP_LOGICAL_AND = 39, OP_LOGICAL_OR = 40, OP_LOGICAL_NEGATION = 41, 
    OP_LEFT_SHIFT = 42, OP_RIGHT_SHIFT = 43, OP_INCREMENT = 44, OP_DECREMENT = 45, 
    VAR_TYPE_IN = 46, VAR_TYPE_OUT = 47, VAR_TYPE_INOUT = 48, VAR_TYPE_WIRE = 49, 
    VAR_TYPE_STATE = 50, LOOP_VARIABLE_PREFIX = 51, SIGNAL_WIDTH_PREFIX = 52, 
    STATEMENT_DELIMITER = 53, PARAMETER_DELIMITER = 54
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
