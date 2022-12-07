
// Generated from ..\syrec_antlr\src\core\syrec\parser\antlr\SyReC.g4 by ANTLR 4.11.1


#include "SyReCVisitor.h"

#include "SyReCParser.h"


using namespace antlrcpp;
using namespace parser;

using namespace antlr4;

namespace {

struct SyReCParserStaticData final {
  SyReCParserStaticData(std::vector<std::string> ruleNames,
                        std::vector<std::string> literalNames,
                        std::vector<std::string> symbolicNames)
      : ruleNames(std::move(ruleNames)), literalNames(std::move(literalNames)),
        symbolicNames(std::move(symbolicNames)),
        vocabulary(this->literalNames, this->symbolicNames) {}

  SyReCParserStaticData(const SyReCParserStaticData&) = delete;
  SyReCParserStaticData(SyReCParserStaticData&&) = delete;
  SyReCParserStaticData& operator=(const SyReCParserStaticData&) = delete;
  SyReCParserStaticData& operator=(SyReCParserStaticData&&) = delete;

  std::vector<antlr4::dfa::DFA> decisionToDFA;
  antlr4::atn::PredictionContextCache sharedContextCache;
  const std::vector<std::string> ruleNames;
  const std::vector<std::string> literalNames;
  const std::vector<std::string> symbolicNames;
  const antlr4::dfa::Vocabulary vocabulary;
  antlr4::atn::SerializedATNView serializedATN;
  std::unique_ptr<antlr4::atn::ATN> atn;
};

::antlr4::internal::OnceFlag syrecParserOnceFlag;
SyReCParserStaticData *syrecParserStaticData = nullptr;

void syrecParserInitialize() {
  assert(syrecParserStaticData == nullptr);
  auto staticData = std::make_unique<SyReCParserStaticData>(
    std::vector<std::string>{
      "number", "program", "module", "parameterList", "parameter", "signalList", 
      "signalDeclaration", "statementList", "statement", "callStatement", 
      "forStatement", "ifStatement", "unaryStatement", "assignStatement", 
      "swapStatement", "skipStatement", "signal", "expression", "binaryExpression", 
      "unaryExpression", "shiftExpression"
    },
    std::vector<std::string>{
      "", "'('", "')'", "'module'", "'['", "']'", "'call'", "'uncall'", 
      "'for'", "'to'", "'step'", "'do'", "'rof'", "'if'", "'then'", "'else'", 
      "'fi'", "'<=>'", "'skip'", "'.'", "':'", "'+'", "'-'", "'*'", "'^'", 
      "'>*'", "'/'", "'%'", "'>'", "'>='", "'<'", "'<='", "'='", "'!='", 
      "'&'", "'|'", "'~'", "'&&'", "'||'", "'!'", "'>>'", "'<<'", "'++'", 
      "'--'", "'in'", "'out'", "'inout'", "'wire'", "'state'", "'$'", "'#'", 
      "';'", "','"
    },
    std::vector<std::string>{
      "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", 
      "", "", "", "", "OP_PLUS", "OP_MINUS", "OP_MULTIPLY", "OP_XOR", "OP_UPPER_BIT_MULTIPLY", 
      "OP_DIVISION", "OP_MODULO", "OP_GREATER_THAN", "OP_GREATER_OR_EQUAL", 
      "OP_LESS_THAN", "OP_LESS_OR_EQUAL", "OP_EQUAL", "OP_NOT_EQUAL", "OP_BITWISE_AND", 
      "OP_BITWISE_OR", "OP_BITWISE_NEGATION", "OP_LOGICAL_AND", "OP_LOGICAL_OR", 
      "OP_LOGICAL_NEGATION", "OP_LEFT_SHIFT", "OP_RIGHT_SHIFT", "OP_INCREMENT", 
      "OP_DECREMENT", "VAR_TYPE_IN", "VAR_TYPE_OUT", "VAR_TYPE_INOUT", "VAR_TYPE_WIRE", 
      "VAR_TYPE_STATE", "LOOP_VARIABLE_PREFIX", "SIGNAL_WIDTH_PREFIX", "STATEMENT_DELIMITER", 
      "PARAMETER_DELIMITER", "LINE_COMMENT", "IDENT", "INT", "SKIPABLEWHITSPACES"
    }
  );
  static const int32_t serializedATNSegment[] = {
  	4,1,56,229,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,2,5,7,5,2,6,7,6,2,
  	7,7,7,2,8,7,8,2,9,7,9,2,10,7,10,2,11,7,11,2,12,7,12,2,13,7,13,2,14,7,
  	14,2,15,7,15,2,16,7,16,2,17,7,17,2,18,7,18,2,19,7,19,2,20,7,20,1,0,1,
  	0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,3,0,54,8,0,1,1,4,1,57,8,1,11,1,
  	12,1,58,1,1,1,1,1,2,1,2,1,2,1,2,3,2,67,8,2,1,2,1,2,5,2,71,8,2,10,2,12,
  	2,74,9,2,1,2,1,2,1,3,1,3,1,3,5,3,81,8,3,10,3,12,3,84,9,3,1,4,1,4,1,4,
  	1,5,1,5,1,5,1,5,5,5,93,8,5,10,5,12,5,96,9,5,1,6,1,6,1,6,1,6,5,6,102,8,
  	6,10,6,12,6,105,9,6,1,6,1,6,1,6,3,6,110,8,6,1,7,1,7,1,7,5,7,115,8,7,10,
  	7,12,7,118,9,7,1,8,1,8,1,8,1,8,1,8,1,8,1,8,3,8,127,8,8,1,9,1,9,1,9,1,
  	9,1,9,1,9,5,9,135,8,9,10,9,12,9,138,9,9,1,9,1,9,1,10,1,10,1,10,1,10,3,
  	10,146,8,10,1,10,1,10,1,10,3,10,151,8,10,1,10,1,10,1,10,3,10,156,8,10,
  	1,10,3,10,159,8,10,1,10,1,10,1,10,1,10,1,11,1,11,1,11,1,11,1,11,1,11,
  	1,11,1,11,1,11,1,12,1,12,1,12,1,12,1,13,1,13,1,13,1,13,1,13,1,14,1,14,
  	1,14,1,14,1,15,1,15,1,16,1,16,1,16,1,16,1,16,5,16,194,8,16,10,16,12,16,
  	197,9,16,1,16,1,16,1,16,1,16,3,16,203,8,16,3,16,205,8,16,1,17,1,17,1,
  	17,1,17,1,17,3,17,212,8,17,1,18,1,18,1,18,1,18,1,18,1,18,1,19,1,19,1,
  	19,1,20,1,20,1,20,1,20,1,20,1,20,1,20,0,0,21,0,2,4,6,8,10,12,14,16,18,
  	20,22,24,26,28,30,32,34,36,38,40,0,9,2,0,21,23,26,26,1,0,44,46,1,0,47,
  	48,1,0,6,7,2,0,36,36,42,43,2,0,21,22,24,24,2,0,21,35,37,38,2,0,36,36,
  	39,39,1,0,40,41,236,0,53,1,0,0,0,2,56,1,0,0,0,4,62,1,0,0,0,6,77,1,0,0,
  	0,8,85,1,0,0,0,10,88,1,0,0,0,12,97,1,0,0,0,14,111,1,0,0,0,16,126,1,0,
  	0,0,18,128,1,0,0,0,20,141,1,0,0,0,22,164,1,0,0,0,24,173,1,0,0,0,26,177,
  	1,0,0,0,28,182,1,0,0,0,30,186,1,0,0,0,32,188,1,0,0,0,34,211,1,0,0,0,36,
  	213,1,0,0,0,38,219,1,0,0,0,40,222,1,0,0,0,42,54,5,55,0,0,43,44,5,50,0,
  	0,44,54,5,54,0,0,45,46,5,49,0,0,46,54,5,54,0,0,47,48,5,1,0,0,48,49,3,
  	0,0,0,49,50,7,0,0,0,50,51,3,0,0,0,51,52,5,2,0,0,52,54,1,0,0,0,53,42,1,
  	0,0,0,53,43,1,0,0,0,53,45,1,0,0,0,53,47,1,0,0,0,54,1,1,0,0,0,55,57,3,
  	4,2,0,56,55,1,0,0,0,57,58,1,0,0,0,58,56,1,0,0,0,58,59,1,0,0,0,59,60,1,
  	0,0,0,60,61,5,0,0,1,61,3,1,0,0,0,62,63,5,3,0,0,63,64,5,54,0,0,64,66,5,
  	1,0,0,65,67,3,6,3,0,66,65,1,0,0,0,66,67,1,0,0,0,67,68,1,0,0,0,68,72,5,
  	2,0,0,69,71,3,10,5,0,70,69,1,0,0,0,71,74,1,0,0,0,72,70,1,0,0,0,72,73,
  	1,0,0,0,73,75,1,0,0,0,74,72,1,0,0,0,75,76,3,14,7,0,76,5,1,0,0,0,77,82,
  	3,8,4,0,78,79,5,52,0,0,79,81,3,8,4,0,80,78,1,0,0,0,81,84,1,0,0,0,82,80,
  	1,0,0,0,82,83,1,0,0,0,83,7,1,0,0,0,84,82,1,0,0,0,85,86,7,1,0,0,86,87,
  	3,12,6,0,87,9,1,0,0,0,88,89,7,2,0,0,89,94,3,12,6,0,90,91,5,52,0,0,91,
  	93,3,12,6,0,92,90,1,0,0,0,93,96,1,0,0,0,94,92,1,0,0,0,94,95,1,0,0,0,95,
  	11,1,0,0,0,96,94,1,0,0,0,97,103,5,54,0,0,98,99,5,4,0,0,99,100,5,55,0,
  	0,100,102,5,5,0,0,101,98,1,0,0,0,102,105,1,0,0,0,103,101,1,0,0,0,103,
  	104,1,0,0,0,104,109,1,0,0,0,105,103,1,0,0,0,106,107,5,1,0,0,107,108,5,
  	55,0,0,108,110,5,2,0,0,109,106,1,0,0,0,109,110,1,0,0,0,110,13,1,0,0,0,
  	111,116,3,16,8,0,112,113,5,51,0,0,113,115,3,16,8,0,114,112,1,0,0,0,115,
  	118,1,0,0,0,116,114,1,0,0,0,116,117,1,0,0,0,117,15,1,0,0,0,118,116,1,
  	0,0,0,119,127,3,18,9,0,120,127,3,20,10,0,121,127,3,22,11,0,122,127,3,
  	24,12,0,123,127,3,26,13,0,124,127,3,28,14,0,125,127,3,30,15,0,126,119,
  	1,0,0,0,126,120,1,0,0,0,126,121,1,0,0,0,126,122,1,0,0,0,126,123,1,0,0,
  	0,126,124,1,0,0,0,126,125,1,0,0,0,127,17,1,0,0,0,128,129,7,3,0,0,129,
  	130,5,54,0,0,130,131,5,1,0,0,131,136,5,54,0,0,132,133,5,52,0,0,133,135,
  	5,54,0,0,134,132,1,0,0,0,135,138,1,0,0,0,136,134,1,0,0,0,136,137,1,0,
  	0,0,137,139,1,0,0,0,138,136,1,0,0,0,139,140,5,2,0,0,140,19,1,0,0,0,141,
  	150,5,8,0,0,142,143,5,49,0,0,143,144,5,54,0,0,144,146,5,32,0,0,145,142,
  	1,0,0,0,145,146,1,0,0,0,146,147,1,0,0,0,147,148,3,0,0,0,148,149,5,9,0,
  	0,149,151,1,0,0,0,150,145,1,0,0,0,150,151,1,0,0,0,151,152,1,0,0,0,152,
  	158,3,0,0,0,153,155,5,10,0,0,154,156,5,22,0,0,155,154,1,0,0,0,155,156,
  	1,0,0,0,156,157,1,0,0,0,157,159,3,0,0,0,158,153,1,0,0,0,158,159,1,0,0,
  	0,159,160,1,0,0,0,160,161,5,11,0,0,161,162,3,14,7,0,162,163,5,12,0,0,
  	163,21,1,0,0,0,164,165,5,13,0,0,165,166,3,34,17,0,166,167,5,14,0,0,167,
  	168,3,14,7,0,168,169,5,15,0,0,169,170,3,14,7,0,170,171,5,16,0,0,171,172,
  	3,34,17,0,172,23,1,0,0,0,173,174,7,4,0,0,174,175,5,32,0,0,175,176,3,32,
  	16,0,176,25,1,0,0,0,177,178,3,32,16,0,178,179,7,5,0,0,179,180,5,32,0,
  	0,180,181,3,34,17,0,181,27,1,0,0,0,182,183,3,32,16,0,183,184,5,17,0,0,
  	184,185,3,32,16,0,185,29,1,0,0,0,186,187,5,18,0,0,187,31,1,0,0,0,188,
  	195,5,54,0,0,189,190,5,4,0,0,190,191,3,34,17,0,191,192,5,5,0,0,192,194,
  	1,0,0,0,193,189,1,0,0,0,194,197,1,0,0,0,195,193,1,0,0,0,195,196,1,0,0,
  	0,196,204,1,0,0,0,197,195,1,0,0,0,198,199,5,19,0,0,199,202,3,0,0,0,200,
  	201,5,20,0,0,201,203,3,0,0,0,202,200,1,0,0,0,202,203,1,0,0,0,203,205,
  	1,0,0,0,204,198,1,0,0,0,204,205,1,0,0,0,205,33,1,0,0,0,206,212,3,0,0,
  	0,207,212,3,32,16,0,208,212,3,36,18,0,209,212,3,38,19,0,210,212,3,40,
  	20,0,211,206,1,0,0,0,211,207,1,0,0,0,211,208,1,0,0,0,211,209,1,0,0,0,
  	211,210,1,0,0,0,212,35,1,0,0,0,213,214,5,1,0,0,214,215,3,34,17,0,215,
  	216,7,6,0,0,216,217,3,34,17,0,217,218,5,2,0,0,218,37,1,0,0,0,219,220,
  	7,7,0,0,220,221,3,34,17,0,221,39,1,0,0,0,222,223,5,1,0,0,223,224,3,34,
  	17,0,224,225,7,8,0,0,225,226,3,0,0,0,226,227,5,2,0,0,227,41,1,0,0,0,19,
  	53,58,66,72,82,94,103,109,116,126,136,145,150,155,158,195,202,204,211
  };
  staticData->serializedATN = antlr4::atn::SerializedATNView(serializedATNSegment, sizeof(serializedATNSegment) / sizeof(serializedATNSegment[0]));

  antlr4::atn::ATNDeserializer deserializer;
  staticData->atn = deserializer.deserialize(staticData->serializedATN);

  const size_t count = staticData->atn->getNumberOfDecisions();
  staticData->decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    staticData->decisionToDFA.emplace_back(staticData->atn->getDecisionState(i), i);
  }
  syrecParserStaticData = staticData.release();
}

}

SyReCParser::SyReCParser(TokenStream *input) : SyReCParser(input, antlr4::atn::ParserATNSimulatorOptions()) {}

SyReCParser::SyReCParser(TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options) : Parser(input) {
  SyReCParser::initialize();
  _interpreter = new atn::ParserATNSimulator(this, *syrecParserStaticData->atn, syrecParserStaticData->decisionToDFA, syrecParserStaticData->sharedContextCache, options);
}

SyReCParser::~SyReCParser() {
  delete _interpreter;
}

const atn::ATN& SyReCParser::getATN() const {
  return *syrecParserStaticData->atn;
}

std::string SyReCParser::getGrammarFileName() const {
  return "SyReC.g4";
}

const std::vector<std::string>& SyReCParser::getRuleNames() const {
  return syrecParserStaticData->ruleNames;
}

const dfa::Vocabulary& SyReCParser::getVocabulary() const {
  return syrecParserStaticData->vocabulary;
}

antlr4::atn::SerializedATNView SyReCParser::getSerializedATN() const {
  return syrecParserStaticData->serializedATN;
}


//----------------- NumberContext ------------------------------------------------------------------

SyReCParser::NumberContext::NumberContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t SyReCParser::NumberContext::getRuleIndex() const {
  return SyReCParser::RuleNumber;
}

void SyReCParser::NumberContext::copyFrom(NumberContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- NumberFromSignalwidthContext ------------------------------------------------------------------

tree::TerminalNode* SyReCParser::NumberFromSignalwidthContext::SIGNAL_WIDTH_PREFIX() {
  return getToken(SyReCParser::SIGNAL_WIDTH_PREFIX, 0);
}

tree::TerminalNode* SyReCParser::NumberFromSignalwidthContext::IDENT() {
  return getToken(SyReCParser::IDENT, 0);
}

SyReCParser::NumberFromSignalwidthContext::NumberFromSignalwidthContext(NumberContext *ctx) { copyFrom(ctx); }


std::any SyReCParser::NumberFromSignalwidthContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SyReCVisitor*>(visitor))
    return parserVisitor->visitNumberFromSignalwidth(this);
  else
    return visitor->visitChildren(this);
}
//----------------- NumberFromLoopVariableContext ------------------------------------------------------------------

tree::TerminalNode* SyReCParser::NumberFromLoopVariableContext::LOOP_VARIABLE_PREFIX() {
  return getToken(SyReCParser::LOOP_VARIABLE_PREFIX, 0);
}

tree::TerminalNode* SyReCParser::NumberFromLoopVariableContext::IDENT() {
  return getToken(SyReCParser::IDENT, 0);
}

SyReCParser::NumberFromLoopVariableContext::NumberFromLoopVariableContext(NumberContext *ctx) { copyFrom(ctx); }


std::any SyReCParser::NumberFromLoopVariableContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SyReCVisitor*>(visitor))
    return parserVisitor->visitNumberFromLoopVariable(this);
  else
    return visitor->visitChildren(this);
}
//----------------- NumberFromConstantContext ------------------------------------------------------------------

tree::TerminalNode* SyReCParser::NumberFromConstantContext::INT() {
  return getToken(SyReCParser::INT, 0);
}

SyReCParser::NumberFromConstantContext::NumberFromConstantContext(NumberContext *ctx) { copyFrom(ctx); }


std::any SyReCParser::NumberFromConstantContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SyReCVisitor*>(visitor))
    return parserVisitor->visitNumberFromConstant(this);
  else
    return visitor->visitChildren(this);
}
//----------------- NumberFromExpressionContext ------------------------------------------------------------------

std::vector<SyReCParser::NumberContext *> SyReCParser::NumberFromExpressionContext::number() {
  return getRuleContexts<SyReCParser::NumberContext>();
}

SyReCParser::NumberContext* SyReCParser::NumberFromExpressionContext::number(size_t i) {
  return getRuleContext<SyReCParser::NumberContext>(i);
}

tree::TerminalNode* SyReCParser::NumberFromExpressionContext::OP_PLUS() {
  return getToken(SyReCParser::OP_PLUS, 0);
}

tree::TerminalNode* SyReCParser::NumberFromExpressionContext::OP_MINUS() {
  return getToken(SyReCParser::OP_MINUS, 0);
}

tree::TerminalNode* SyReCParser::NumberFromExpressionContext::OP_MULTIPLY() {
  return getToken(SyReCParser::OP_MULTIPLY, 0);
}

tree::TerminalNode* SyReCParser::NumberFromExpressionContext::OP_DIVISION() {
  return getToken(SyReCParser::OP_DIVISION, 0);
}

SyReCParser::NumberFromExpressionContext::NumberFromExpressionContext(NumberContext *ctx) { copyFrom(ctx); }


std::any SyReCParser::NumberFromExpressionContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SyReCVisitor*>(visitor))
    return parserVisitor->visitNumberFromExpression(this);
  else
    return visitor->visitChildren(this);
}
SyReCParser::NumberContext* SyReCParser::number() {
  NumberContext *_localctx = _tracker.createInstance<NumberContext>(_ctx, getState());
  enterRule(_localctx, 0, SyReCParser::RuleNumber);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(53);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case SyReCParser::INT: {
        _localctx = _tracker.createInstance<SyReCParser::NumberFromConstantContext>(_localctx);
        enterOuterAlt(_localctx, 1);
        setState(42);
        match(SyReCParser::INT);
        break;
      }

      case SyReCParser::SIGNAL_WIDTH_PREFIX: {
        _localctx = _tracker.createInstance<SyReCParser::NumberFromSignalwidthContext>(_localctx);
        enterOuterAlt(_localctx, 2);
        setState(43);
        match(SyReCParser::SIGNAL_WIDTH_PREFIX);
        setState(44);
        match(SyReCParser::IDENT);
        break;
      }

      case SyReCParser::LOOP_VARIABLE_PREFIX: {
        _localctx = _tracker.createInstance<SyReCParser::NumberFromLoopVariableContext>(_localctx);
        enterOuterAlt(_localctx, 3);
        setState(45);
        match(SyReCParser::LOOP_VARIABLE_PREFIX);
        setState(46);
        match(SyReCParser::IDENT);
        break;
      }

      case SyReCParser::T__0: {
        _localctx = _tracker.createInstance<SyReCParser::NumberFromExpressionContext>(_localctx);
        enterOuterAlt(_localctx, 4);
        setState(47);
        match(SyReCParser::T__0);
        setState(48);
        antlrcpp::downCast<NumberFromExpressionContext *>(_localctx)->lhsOperand = number();
        setState(49);
        antlrcpp::downCast<NumberFromExpressionContext *>(_localctx)->op = _input->LT(1);
        _la = _input->LA(1);
        if (!(((_la & ~ 0x3fULL) == 0) &&
          ((1ULL << _la) & 81788928) != 0)) {
          antlrcpp::downCast<NumberFromExpressionContext *>(_localctx)->op = _errHandler->recoverInline(this);
        }
        else {
          _errHandler->reportMatch(this);
          consume();
        }
        setState(50);
        antlrcpp::downCast<NumberFromExpressionContext *>(_localctx)->rhsOperand = number();
        setState(51);
        match(SyReCParser::T__1);
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ProgramContext ------------------------------------------------------------------

SyReCParser::ProgramContext::ProgramContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* SyReCParser::ProgramContext::EOF() {
  return getToken(SyReCParser::EOF, 0);
}

std::vector<SyReCParser::ModuleContext *> SyReCParser::ProgramContext::module() {
  return getRuleContexts<SyReCParser::ModuleContext>();
}

SyReCParser::ModuleContext* SyReCParser::ProgramContext::module(size_t i) {
  return getRuleContext<SyReCParser::ModuleContext>(i);
}


size_t SyReCParser::ProgramContext::getRuleIndex() const {
  return SyReCParser::RuleProgram;
}


std::any SyReCParser::ProgramContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SyReCVisitor*>(visitor))
    return parserVisitor->visitProgram(this);
  else
    return visitor->visitChildren(this);
}

SyReCParser::ProgramContext* SyReCParser::program() {
  ProgramContext *_localctx = _tracker.createInstance<ProgramContext>(_ctx, getState());
  enterRule(_localctx, 2, SyReCParser::RuleProgram);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(56); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(55);
      module();
      setState(58); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while (_la == SyReCParser::T__2);
    setState(60);
    match(SyReCParser::EOF);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ModuleContext ------------------------------------------------------------------

SyReCParser::ModuleContext::ModuleContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* SyReCParser::ModuleContext::IDENT() {
  return getToken(SyReCParser::IDENT, 0);
}

SyReCParser::StatementListContext* SyReCParser::ModuleContext::statementList() {
  return getRuleContext<SyReCParser::StatementListContext>(0);
}

SyReCParser::ParameterListContext* SyReCParser::ModuleContext::parameterList() {
  return getRuleContext<SyReCParser::ParameterListContext>(0);
}

std::vector<SyReCParser::SignalListContext *> SyReCParser::ModuleContext::signalList() {
  return getRuleContexts<SyReCParser::SignalListContext>();
}

SyReCParser::SignalListContext* SyReCParser::ModuleContext::signalList(size_t i) {
  return getRuleContext<SyReCParser::SignalListContext>(i);
}


size_t SyReCParser::ModuleContext::getRuleIndex() const {
  return SyReCParser::RuleModule;
}


std::any SyReCParser::ModuleContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SyReCVisitor*>(visitor))
    return parserVisitor->visitModule(this);
  else
    return visitor->visitChildren(this);
}

SyReCParser::ModuleContext* SyReCParser::module() {
  ModuleContext *_localctx = _tracker.createInstance<ModuleContext>(_ctx, getState());
  enterRule(_localctx, 4, SyReCParser::RuleModule);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(62);
    match(SyReCParser::T__2);
    setState(63);
    match(SyReCParser::IDENT);
    setState(64);
    match(SyReCParser::T__0);
    setState(66);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 123145302310912) != 0) {
      setState(65);
      parameterList();
    }
    setState(68);
    match(SyReCParser::T__1);
    setState(72);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SyReCParser::VAR_TYPE_WIRE

    || _la == SyReCParser::VAR_TYPE_STATE) {
      setState(69);
      signalList();
      setState(74);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(75);
    statementList();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ParameterListContext ------------------------------------------------------------------

SyReCParser::ParameterListContext::ParameterListContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<SyReCParser::ParameterContext *> SyReCParser::ParameterListContext::parameter() {
  return getRuleContexts<SyReCParser::ParameterContext>();
}

SyReCParser::ParameterContext* SyReCParser::ParameterListContext::parameter(size_t i) {
  return getRuleContext<SyReCParser::ParameterContext>(i);
}

std::vector<tree::TerminalNode *> SyReCParser::ParameterListContext::PARAMETER_DELIMITER() {
  return getTokens(SyReCParser::PARAMETER_DELIMITER);
}

tree::TerminalNode* SyReCParser::ParameterListContext::PARAMETER_DELIMITER(size_t i) {
  return getToken(SyReCParser::PARAMETER_DELIMITER, i);
}


size_t SyReCParser::ParameterListContext::getRuleIndex() const {
  return SyReCParser::RuleParameterList;
}


std::any SyReCParser::ParameterListContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SyReCVisitor*>(visitor))
    return parserVisitor->visitParameterList(this);
  else
    return visitor->visitChildren(this);
}

SyReCParser::ParameterListContext* SyReCParser::parameterList() {
  ParameterListContext *_localctx = _tracker.createInstance<ParameterListContext>(_ctx, getState());
  enterRule(_localctx, 6, SyReCParser::RuleParameterList);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(77);
    parameter();
    setState(82);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SyReCParser::PARAMETER_DELIMITER) {
      setState(78);
      match(SyReCParser::PARAMETER_DELIMITER);
      setState(79);
      parameter();
      setState(84);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ParameterContext ------------------------------------------------------------------

SyReCParser::ParameterContext::ParameterContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

SyReCParser::SignalDeclarationContext* SyReCParser::ParameterContext::signalDeclaration() {
  return getRuleContext<SyReCParser::SignalDeclarationContext>(0);
}

tree::TerminalNode* SyReCParser::ParameterContext::VAR_TYPE_IN() {
  return getToken(SyReCParser::VAR_TYPE_IN, 0);
}

tree::TerminalNode* SyReCParser::ParameterContext::VAR_TYPE_OUT() {
  return getToken(SyReCParser::VAR_TYPE_OUT, 0);
}

tree::TerminalNode* SyReCParser::ParameterContext::VAR_TYPE_INOUT() {
  return getToken(SyReCParser::VAR_TYPE_INOUT, 0);
}


size_t SyReCParser::ParameterContext::getRuleIndex() const {
  return SyReCParser::RuleParameter;
}


std::any SyReCParser::ParameterContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SyReCVisitor*>(visitor))
    return parserVisitor->visitParameter(this);
  else
    return visitor->visitChildren(this);
}

SyReCParser::ParameterContext* SyReCParser::parameter() {
  ParameterContext *_localctx = _tracker.createInstance<ParameterContext>(_ctx, getState());
  enterRule(_localctx, 8, SyReCParser::RuleParameter);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(85);
    _la = _input->LA(1);
    if (!(((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 123145302310912) != 0)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(86);
    signalDeclaration();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- SignalListContext ------------------------------------------------------------------

SyReCParser::SignalListContext::SignalListContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<SyReCParser::SignalDeclarationContext *> SyReCParser::SignalListContext::signalDeclaration() {
  return getRuleContexts<SyReCParser::SignalDeclarationContext>();
}

SyReCParser::SignalDeclarationContext* SyReCParser::SignalListContext::signalDeclaration(size_t i) {
  return getRuleContext<SyReCParser::SignalDeclarationContext>(i);
}

tree::TerminalNode* SyReCParser::SignalListContext::VAR_TYPE_WIRE() {
  return getToken(SyReCParser::VAR_TYPE_WIRE, 0);
}

tree::TerminalNode* SyReCParser::SignalListContext::VAR_TYPE_STATE() {
  return getToken(SyReCParser::VAR_TYPE_STATE, 0);
}

std::vector<tree::TerminalNode *> SyReCParser::SignalListContext::PARAMETER_DELIMITER() {
  return getTokens(SyReCParser::PARAMETER_DELIMITER);
}

tree::TerminalNode* SyReCParser::SignalListContext::PARAMETER_DELIMITER(size_t i) {
  return getToken(SyReCParser::PARAMETER_DELIMITER, i);
}


size_t SyReCParser::SignalListContext::getRuleIndex() const {
  return SyReCParser::RuleSignalList;
}


std::any SyReCParser::SignalListContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SyReCVisitor*>(visitor))
    return parserVisitor->visitSignalList(this);
  else
    return visitor->visitChildren(this);
}

SyReCParser::SignalListContext* SyReCParser::signalList() {
  SignalListContext *_localctx = _tracker.createInstance<SignalListContext>(_ctx, getState());
  enterRule(_localctx, 10, SyReCParser::RuleSignalList);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(88);
    _la = _input->LA(1);
    if (!(_la == SyReCParser::VAR_TYPE_WIRE

    || _la == SyReCParser::VAR_TYPE_STATE)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(89);
    signalDeclaration();
    setState(94);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SyReCParser::PARAMETER_DELIMITER) {
      setState(90);
      match(SyReCParser::PARAMETER_DELIMITER);
      setState(91);
      signalDeclaration();
      setState(96);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- SignalDeclarationContext ------------------------------------------------------------------

SyReCParser::SignalDeclarationContext::SignalDeclarationContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* SyReCParser::SignalDeclarationContext::IDENT() {
  return getToken(SyReCParser::IDENT, 0);
}

std::vector<tree::TerminalNode *> SyReCParser::SignalDeclarationContext::INT() {
  return getTokens(SyReCParser::INT);
}

tree::TerminalNode* SyReCParser::SignalDeclarationContext::INT(size_t i) {
  return getToken(SyReCParser::INT, i);
}


size_t SyReCParser::SignalDeclarationContext::getRuleIndex() const {
  return SyReCParser::RuleSignalDeclaration;
}


std::any SyReCParser::SignalDeclarationContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SyReCVisitor*>(visitor))
    return parserVisitor->visitSignalDeclaration(this);
  else
    return visitor->visitChildren(this);
}

SyReCParser::SignalDeclarationContext* SyReCParser::signalDeclaration() {
  SignalDeclarationContext *_localctx = _tracker.createInstance<SignalDeclarationContext>(_ctx, getState());
  enterRule(_localctx, 12, SyReCParser::RuleSignalDeclaration);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(97);
    match(SyReCParser::IDENT);
    setState(103);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SyReCParser::T__3) {
      setState(98);
      match(SyReCParser::T__3);
      setState(99);
      antlrcpp::downCast<SignalDeclarationContext *>(_localctx)->intToken = match(SyReCParser::INT);
      antlrcpp::downCast<SignalDeclarationContext *>(_localctx)->dimensionTokens.push_back(antlrcpp::downCast<SignalDeclarationContext *>(_localctx)->intToken);
      setState(100);
      match(SyReCParser::T__4);
      setState(105);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(109);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == SyReCParser::T__0) {
      setState(106);
      match(SyReCParser::T__0);
      setState(107);
      antlrcpp::downCast<SignalDeclarationContext *>(_localctx)->signalWidthToken = match(SyReCParser::INT);
      setState(108);
      match(SyReCParser::T__1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- StatementListContext ------------------------------------------------------------------

SyReCParser::StatementListContext::StatementListContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<SyReCParser::StatementContext *> SyReCParser::StatementListContext::statement() {
  return getRuleContexts<SyReCParser::StatementContext>();
}

SyReCParser::StatementContext* SyReCParser::StatementListContext::statement(size_t i) {
  return getRuleContext<SyReCParser::StatementContext>(i);
}

std::vector<tree::TerminalNode *> SyReCParser::StatementListContext::STATEMENT_DELIMITER() {
  return getTokens(SyReCParser::STATEMENT_DELIMITER);
}

tree::TerminalNode* SyReCParser::StatementListContext::STATEMENT_DELIMITER(size_t i) {
  return getToken(SyReCParser::STATEMENT_DELIMITER, i);
}


size_t SyReCParser::StatementListContext::getRuleIndex() const {
  return SyReCParser::RuleStatementList;
}


std::any SyReCParser::StatementListContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SyReCVisitor*>(visitor))
    return parserVisitor->visitStatementList(this);
  else
    return visitor->visitChildren(this);
}

SyReCParser::StatementListContext* SyReCParser::statementList() {
  StatementListContext *_localctx = _tracker.createInstance<StatementListContext>(_ctx, getState());
  enterRule(_localctx, 14, SyReCParser::RuleStatementList);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(111);
    antlrcpp::downCast<StatementListContext *>(_localctx)->statementContext = statement();
    antlrcpp::downCast<StatementListContext *>(_localctx)->stmts.push_back(antlrcpp::downCast<StatementListContext *>(_localctx)->statementContext);
    setState(116);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SyReCParser::STATEMENT_DELIMITER) {
      setState(112);
      match(SyReCParser::STATEMENT_DELIMITER);
      setState(113);
      antlrcpp::downCast<StatementListContext *>(_localctx)->statementContext = statement();
      antlrcpp::downCast<StatementListContext *>(_localctx)->stmts.push_back(antlrcpp::downCast<StatementListContext *>(_localctx)->statementContext);
      setState(118);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- StatementContext ------------------------------------------------------------------

SyReCParser::StatementContext::StatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

SyReCParser::CallStatementContext* SyReCParser::StatementContext::callStatement() {
  return getRuleContext<SyReCParser::CallStatementContext>(0);
}

SyReCParser::ForStatementContext* SyReCParser::StatementContext::forStatement() {
  return getRuleContext<SyReCParser::ForStatementContext>(0);
}

SyReCParser::IfStatementContext* SyReCParser::StatementContext::ifStatement() {
  return getRuleContext<SyReCParser::IfStatementContext>(0);
}

SyReCParser::UnaryStatementContext* SyReCParser::StatementContext::unaryStatement() {
  return getRuleContext<SyReCParser::UnaryStatementContext>(0);
}

SyReCParser::AssignStatementContext* SyReCParser::StatementContext::assignStatement() {
  return getRuleContext<SyReCParser::AssignStatementContext>(0);
}

SyReCParser::SwapStatementContext* SyReCParser::StatementContext::swapStatement() {
  return getRuleContext<SyReCParser::SwapStatementContext>(0);
}

SyReCParser::SkipStatementContext* SyReCParser::StatementContext::skipStatement() {
  return getRuleContext<SyReCParser::SkipStatementContext>(0);
}


size_t SyReCParser::StatementContext::getRuleIndex() const {
  return SyReCParser::RuleStatement;
}


std::any SyReCParser::StatementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SyReCVisitor*>(visitor))
    return parserVisitor->visitStatement(this);
  else
    return visitor->visitChildren(this);
}

SyReCParser::StatementContext* SyReCParser::statement() {
  StatementContext *_localctx = _tracker.createInstance<StatementContext>(_ctx, getState());
  enterRule(_localctx, 16, SyReCParser::RuleStatement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(126);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 9, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(119);
      callStatement();
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(120);
      forStatement();
      break;
    }

    case 3: {
      enterOuterAlt(_localctx, 3);
      setState(121);
      ifStatement();
      break;
    }

    case 4: {
      enterOuterAlt(_localctx, 4);
      setState(122);
      unaryStatement();
      break;
    }

    case 5: {
      enterOuterAlt(_localctx, 5);
      setState(123);
      assignStatement();
      break;
    }

    case 6: {
      enterOuterAlt(_localctx, 6);
      setState(124);
      swapStatement();
      break;
    }

    case 7: {
      enterOuterAlt(_localctx, 7);
      setState(125);
      skipStatement();
      break;
    }

    default:
      break;
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- CallStatementContext ------------------------------------------------------------------

SyReCParser::CallStatementContext::CallStatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<tree::TerminalNode *> SyReCParser::CallStatementContext::IDENT() {
  return getTokens(SyReCParser::IDENT);
}

tree::TerminalNode* SyReCParser::CallStatementContext::IDENT(size_t i) {
  return getToken(SyReCParser::IDENT, i);
}

std::vector<tree::TerminalNode *> SyReCParser::CallStatementContext::PARAMETER_DELIMITER() {
  return getTokens(SyReCParser::PARAMETER_DELIMITER);
}

tree::TerminalNode* SyReCParser::CallStatementContext::PARAMETER_DELIMITER(size_t i) {
  return getToken(SyReCParser::PARAMETER_DELIMITER, i);
}


size_t SyReCParser::CallStatementContext::getRuleIndex() const {
  return SyReCParser::RuleCallStatement;
}


std::any SyReCParser::CallStatementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SyReCVisitor*>(visitor))
    return parserVisitor->visitCallStatement(this);
  else
    return visitor->visitChildren(this);
}

SyReCParser::CallStatementContext* SyReCParser::callStatement() {
  CallStatementContext *_localctx = _tracker.createInstance<CallStatementContext>(_ctx, getState());
  enterRule(_localctx, 18, SyReCParser::RuleCallStatement);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(128);
    _la = _input->LA(1);
    if (!(_la == SyReCParser::T__5

    || _la == SyReCParser::T__6)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(129);
    antlrcpp::downCast<CallStatementContext *>(_localctx)->moduleIdent = match(SyReCParser::IDENT);
    setState(130);
    match(SyReCParser::T__0);
    setState(131);
    antlrcpp::downCast<CallStatementContext *>(_localctx)->identToken = match(SyReCParser::IDENT);
    antlrcpp::downCast<CallStatementContext *>(_localctx)->calleArguments.push_back(antlrcpp::downCast<CallStatementContext *>(_localctx)->identToken);
    setState(136);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SyReCParser::PARAMETER_DELIMITER) {
      setState(132);
      match(SyReCParser::PARAMETER_DELIMITER);
      setState(133);
      antlrcpp::downCast<CallStatementContext *>(_localctx)->identToken = match(SyReCParser::IDENT);
      antlrcpp::downCast<CallStatementContext *>(_localctx)->calleeArguments.push_back(antlrcpp::downCast<CallStatementContext *>(_localctx)->identToken);
      setState(138);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(139);
    match(SyReCParser::T__1);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ForStatementContext ------------------------------------------------------------------

SyReCParser::ForStatementContext::ForStatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

SyReCParser::StatementListContext* SyReCParser::ForStatementContext::statementList() {
  return getRuleContext<SyReCParser::StatementListContext>(0);
}

std::vector<SyReCParser::NumberContext *> SyReCParser::ForStatementContext::number() {
  return getRuleContexts<SyReCParser::NumberContext>();
}

SyReCParser::NumberContext* SyReCParser::ForStatementContext::number(size_t i) {
  return getRuleContext<SyReCParser::NumberContext>(i);
}

tree::TerminalNode* SyReCParser::ForStatementContext::LOOP_VARIABLE_PREFIX() {
  return getToken(SyReCParser::LOOP_VARIABLE_PREFIX, 0);
}

tree::TerminalNode* SyReCParser::ForStatementContext::OP_EQUAL() {
  return getToken(SyReCParser::OP_EQUAL, 0);
}

tree::TerminalNode* SyReCParser::ForStatementContext::OP_MINUS() {
  return getToken(SyReCParser::OP_MINUS, 0);
}

tree::TerminalNode* SyReCParser::ForStatementContext::IDENT() {
  return getToken(SyReCParser::IDENT, 0);
}


size_t SyReCParser::ForStatementContext::getRuleIndex() const {
  return SyReCParser::RuleForStatement;
}


std::any SyReCParser::ForStatementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SyReCVisitor*>(visitor))
    return parserVisitor->visitForStatement(this);
  else
    return visitor->visitChildren(this);
}

SyReCParser::ForStatementContext* SyReCParser::forStatement() {
  ForStatementContext *_localctx = _tracker.createInstance<ForStatementContext>(_ctx, getState());
  enterRule(_localctx, 20, SyReCParser::RuleForStatement);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(141);
    match(SyReCParser::T__7);
    setState(150);
    _errHandler->sync(this);

    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 12, _ctx)) {
    case 1: {
      setState(145);
      _errHandler->sync(this);

      switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 11, _ctx)) {
      case 1: {
        setState(142);
        match(SyReCParser::LOOP_VARIABLE_PREFIX);
        setState(143);
        antlrcpp::downCast<ForStatementContext *>(_localctx)->variableIdent = match(SyReCParser::IDENT);
        setState(144);
        match(SyReCParser::OP_EQUAL);
        break;
      }

      default:
        break;
      }
      setState(147);
      antlrcpp::downCast<ForStatementContext *>(_localctx)->startValue = number();
      setState(148);
      match(SyReCParser::T__8);
      break;
    }

    default:
      break;
    }
    setState(152);
    antlrcpp::downCast<ForStatementContext *>(_localctx)->endValue = number();
    setState(158);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == SyReCParser::T__9) {
      setState(153);
      match(SyReCParser::T__9);
      setState(155);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == SyReCParser::OP_MINUS) {
        setState(154);
        match(SyReCParser::OP_MINUS);
      }
      setState(157);
      antlrcpp::downCast<ForStatementContext *>(_localctx)->stepSize = number();
    }
    setState(160);
    match(SyReCParser::T__10);
    setState(161);
    statementList();
    setState(162);
    match(SyReCParser::T__11);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- IfStatementContext ------------------------------------------------------------------

SyReCParser::IfStatementContext::IfStatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<SyReCParser::ExpressionContext *> SyReCParser::IfStatementContext::expression() {
  return getRuleContexts<SyReCParser::ExpressionContext>();
}

SyReCParser::ExpressionContext* SyReCParser::IfStatementContext::expression(size_t i) {
  return getRuleContext<SyReCParser::ExpressionContext>(i);
}

std::vector<SyReCParser::StatementListContext *> SyReCParser::IfStatementContext::statementList() {
  return getRuleContexts<SyReCParser::StatementListContext>();
}

SyReCParser::StatementListContext* SyReCParser::IfStatementContext::statementList(size_t i) {
  return getRuleContext<SyReCParser::StatementListContext>(i);
}


size_t SyReCParser::IfStatementContext::getRuleIndex() const {
  return SyReCParser::RuleIfStatement;
}


std::any SyReCParser::IfStatementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SyReCVisitor*>(visitor))
    return parserVisitor->visitIfStatement(this);
  else
    return visitor->visitChildren(this);
}

SyReCParser::IfStatementContext* SyReCParser::ifStatement() {
  IfStatementContext *_localctx = _tracker.createInstance<IfStatementContext>(_ctx, getState());
  enterRule(_localctx, 22, SyReCParser::RuleIfStatement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(164);
    match(SyReCParser::T__12);
    setState(165);
    antlrcpp::downCast<IfStatementContext *>(_localctx)->guardCondition = expression();
    setState(166);
    match(SyReCParser::T__13);
    setState(167);
    antlrcpp::downCast<IfStatementContext *>(_localctx)->trueBranchStmts = statementList();
    setState(168);
    match(SyReCParser::T__14);
    setState(169);
    antlrcpp::downCast<IfStatementContext *>(_localctx)->falseBranchStmts = statementList();
    setState(170);
    match(SyReCParser::T__15);
    setState(171);
    antlrcpp::downCast<IfStatementContext *>(_localctx)->matchingGuardExpression = expression();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- UnaryStatementContext ------------------------------------------------------------------

SyReCParser::UnaryStatementContext::UnaryStatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* SyReCParser::UnaryStatementContext::OP_EQUAL() {
  return getToken(SyReCParser::OP_EQUAL, 0);
}

SyReCParser::SignalContext* SyReCParser::UnaryStatementContext::signal() {
  return getRuleContext<SyReCParser::SignalContext>(0);
}

tree::TerminalNode* SyReCParser::UnaryStatementContext::OP_BITWISE_NEGATION() {
  return getToken(SyReCParser::OP_BITWISE_NEGATION, 0);
}

tree::TerminalNode* SyReCParser::UnaryStatementContext::OP_INCREMENT() {
  return getToken(SyReCParser::OP_INCREMENT, 0);
}

tree::TerminalNode* SyReCParser::UnaryStatementContext::OP_DECREMENT() {
  return getToken(SyReCParser::OP_DECREMENT, 0);
}


size_t SyReCParser::UnaryStatementContext::getRuleIndex() const {
  return SyReCParser::RuleUnaryStatement;
}


std::any SyReCParser::UnaryStatementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SyReCVisitor*>(visitor))
    return parserVisitor->visitUnaryStatement(this);
  else
    return visitor->visitChildren(this);
}

SyReCParser::UnaryStatementContext* SyReCParser::unaryStatement() {
  UnaryStatementContext *_localctx = _tracker.createInstance<UnaryStatementContext>(_ctx, getState());
  enterRule(_localctx, 24, SyReCParser::RuleUnaryStatement);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(173);
    _la = _input->LA(1);
    if (!(((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 13262859010048) != 0)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(174);
    match(SyReCParser::OP_EQUAL);
    setState(175);
    signal();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- AssignStatementContext ------------------------------------------------------------------

SyReCParser::AssignStatementContext::AssignStatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

SyReCParser::SignalContext* SyReCParser::AssignStatementContext::signal() {
  return getRuleContext<SyReCParser::SignalContext>(0);
}

tree::TerminalNode* SyReCParser::AssignStatementContext::OP_EQUAL() {
  return getToken(SyReCParser::OP_EQUAL, 0);
}

SyReCParser::ExpressionContext* SyReCParser::AssignStatementContext::expression() {
  return getRuleContext<SyReCParser::ExpressionContext>(0);
}

tree::TerminalNode* SyReCParser::AssignStatementContext::OP_XOR() {
  return getToken(SyReCParser::OP_XOR, 0);
}

tree::TerminalNode* SyReCParser::AssignStatementContext::OP_PLUS() {
  return getToken(SyReCParser::OP_PLUS, 0);
}

tree::TerminalNode* SyReCParser::AssignStatementContext::OP_MINUS() {
  return getToken(SyReCParser::OP_MINUS, 0);
}


size_t SyReCParser::AssignStatementContext::getRuleIndex() const {
  return SyReCParser::RuleAssignStatement;
}


std::any SyReCParser::AssignStatementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SyReCVisitor*>(visitor))
    return parserVisitor->visitAssignStatement(this);
  else
    return visitor->visitChildren(this);
}

SyReCParser::AssignStatementContext* SyReCParser::assignStatement() {
  AssignStatementContext *_localctx = _tracker.createInstance<AssignStatementContext>(_ctx, getState());
  enterRule(_localctx, 26, SyReCParser::RuleAssignStatement);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(177);
    signal();
    setState(178);
    _la = _input->LA(1);
    if (!(((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 23068672) != 0)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(179);
    match(SyReCParser::OP_EQUAL);
    setState(180);
    expression();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- SwapStatementContext ------------------------------------------------------------------

SyReCParser::SwapStatementContext::SwapStatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<SyReCParser::SignalContext *> SyReCParser::SwapStatementContext::signal() {
  return getRuleContexts<SyReCParser::SignalContext>();
}

SyReCParser::SignalContext* SyReCParser::SwapStatementContext::signal(size_t i) {
  return getRuleContext<SyReCParser::SignalContext>(i);
}


size_t SyReCParser::SwapStatementContext::getRuleIndex() const {
  return SyReCParser::RuleSwapStatement;
}


std::any SyReCParser::SwapStatementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SyReCVisitor*>(visitor))
    return parserVisitor->visitSwapStatement(this);
  else
    return visitor->visitChildren(this);
}

SyReCParser::SwapStatementContext* SyReCParser::swapStatement() {
  SwapStatementContext *_localctx = _tracker.createInstance<SwapStatementContext>(_ctx, getState());
  enterRule(_localctx, 28, SyReCParser::RuleSwapStatement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(182);
    antlrcpp::downCast<SwapStatementContext *>(_localctx)->lhsOperand = signal();
    setState(183);
    match(SyReCParser::T__16);
    setState(184);
    antlrcpp::downCast<SwapStatementContext *>(_localctx)->rhsOperand = signal();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- SkipStatementContext ------------------------------------------------------------------

SyReCParser::SkipStatementContext::SkipStatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t SyReCParser::SkipStatementContext::getRuleIndex() const {
  return SyReCParser::RuleSkipStatement;
}


std::any SyReCParser::SkipStatementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SyReCVisitor*>(visitor))
    return parserVisitor->visitSkipStatement(this);
  else
    return visitor->visitChildren(this);
}

SyReCParser::SkipStatementContext* SyReCParser::skipStatement() {
  SkipStatementContext *_localctx = _tracker.createInstance<SkipStatementContext>(_ctx, getState());
  enterRule(_localctx, 30, SyReCParser::RuleSkipStatement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(186);
    match(SyReCParser::T__17);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- SignalContext ------------------------------------------------------------------

SyReCParser::SignalContext::SignalContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* SyReCParser::SignalContext::IDENT() {
  return getToken(SyReCParser::IDENT, 0);
}

std::vector<SyReCParser::ExpressionContext *> SyReCParser::SignalContext::expression() {
  return getRuleContexts<SyReCParser::ExpressionContext>();
}

SyReCParser::ExpressionContext* SyReCParser::SignalContext::expression(size_t i) {
  return getRuleContext<SyReCParser::ExpressionContext>(i);
}

std::vector<SyReCParser::NumberContext *> SyReCParser::SignalContext::number() {
  return getRuleContexts<SyReCParser::NumberContext>();
}

SyReCParser::NumberContext* SyReCParser::SignalContext::number(size_t i) {
  return getRuleContext<SyReCParser::NumberContext>(i);
}


size_t SyReCParser::SignalContext::getRuleIndex() const {
  return SyReCParser::RuleSignal;
}


std::any SyReCParser::SignalContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SyReCVisitor*>(visitor))
    return parserVisitor->visitSignal(this);
  else
    return visitor->visitChildren(this);
}

SyReCParser::SignalContext* SyReCParser::signal() {
  SignalContext *_localctx = _tracker.createInstance<SignalContext>(_ctx, getState());
  enterRule(_localctx, 32, SyReCParser::RuleSignal);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(188);
    match(SyReCParser::IDENT);
    setState(195);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SyReCParser::T__3) {
      setState(189);
      match(SyReCParser::T__3);
      setState(190);
      antlrcpp::downCast<SignalContext *>(_localctx)->expressionContext = expression();
      antlrcpp::downCast<SignalContext *>(_localctx)->accessedDimensions.push_back(antlrcpp::downCast<SignalContext *>(_localctx)->expressionContext);
      setState(191);
      match(SyReCParser::T__4);
      setState(197);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(204);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == SyReCParser::T__18) {
      setState(198);
      match(SyReCParser::T__18);
      setState(199);
      antlrcpp::downCast<SignalContext *>(_localctx)->bitStart = number();
      setState(202);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == SyReCParser::T__19) {
        setState(200);
        match(SyReCParser::T__19);
        setState(201);
        antlrcpp::downCast<SignalContext *>(_localctx)->bitRangeEnd = number();
      }
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ExpressionContext ------------------------------------------------------------------

SyReCParser::ExpressionContext::ExpressionContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t SyReCParser::ExpressionContext::getRuleIndex() const {
  return SyReCParser::RuleExpression;
}

void SyReCParser::ExpressionContext::copyFrom(ExpressionContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- ExpressionFromSignalContext ------------------------------------------------------------------

SyReCParser::SignalContext* SyReCParser::ExpressionFromSignalContext::signal() {
  return getRuleContext<SyReCParser::SignalContext>(0);
}

SyReCParser::ExpressionFromSignalContext::ExpressionFromSignalContext(ExpressionContext *ctx) { copyFrom(ctx); }


std::any SyReCParser::ExpressionFromSignalContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SyReCVisitor*>(visitor))
    return parserVisitor->visitExpressionFromSignal(this);
  else
    return visitor->visitChildren(this);
}
//----------------- ExpressionFromBinaryExpressionContext ------------------------------------------------------------------

SyReCParser::BinaryExpressionContext* SyReCParser::ExpressionFromBinaryExpressionContext::binaryExpression() {
  return getRuleContext<SyReCParser::BinaryExpressionContext>(0);
}

SyReCParser::ExpressionFromBinaryExpressionContext::ExpressionFromBinaryExpressionContext(ExpressionContext *ctx) { copyFrom(ctx); }


std::any SyReCParser::ExpressionFromBinaryExpressionContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SyReCVisitor*>(visitor))
    return parserVisitor->visitExpressionFromBinaryExpression(this);
  else
    return visitor->visitChildren(this);
}
//----------------- ExpressionFromNumberContext ------------------------------------------------------------------

SyReCParser::NumberContext* SyReCParser::ExpressionFromNumberContext::number() {
  return getRuleContext<SyReCParser::NumberContext>(0);
}

SyReCParser::ExpressionFromNumberContext::ExpressionFromNumberContext(ExpressionContext *ctx) { copyFrom(ctx); }


std::any SyReCParser::ExpressionFromNumberContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SyReCVisitor*>(visitor))
    return parserVisitor->visitExpressionFromNumber(this);
  else
    return visitor->visitChildren(this);
}
//----------------- ExpressionFromUnaryExpressionContext ------------------------------------------------------------------

SyReCParser::UnaryExpressionContext* SyReCParser::ExpressionFromUnaryExpressionContext::unaryExpression() {
  return getRuleContext<SyReCParser::UnaryExpressionContext>(0);
}

SyReCParser::ExpressionFromUnaryExpressionContext::ExpressionFromUnaryExpressionContext(ExpressionContext *ctx) { copyFrom(ctx); }


std::any SyReCParser::ExpressionFromUnaryExpressionContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SyReCVisitor*>(visitor))
    return parserVisitor->visitExpressionFromUnaryExpression(this);
  else
    return visitor->visitChildren(this);
}
//----------------- ExpressionFromShiftExpressionContext ------------------------------------------------------------------

SyReCParser::ShiftExpressionContext* SyReCParser::ExpressionFromShiftExpressionContext::shiftExpression() {
  return getRuleContext<SyReCParser::ShiftExpressionContext>(0);
}

SyReCParser::ExpressionFromShiftExpressionContext::ExpressionFromShiftExpressionContext(ExpressionContext *ctx) { copyFrom(ctx); }


std::any SyReCParser::ExpressionFromShiftExpressionContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SyReCVisitor*>(visitor))
    return parserVisitor->visitExpressionFromShiftExpression(this);
  else
    return visitor->visitChildren(this);
}
SyReCParser::ExpressionContext* SyReCParser::expression() {
  ExpressionContext *_localctx = _tracker.createInstance<ExpressionContext>(_ctx, getState());
  enterRule(_localctx, 34, SyReCParser::RuleExpression);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(211);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 18, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<SyReCParser::ExpressionFromNumberContext>(_localctx);
      enterOuterAlt(_localctx, 1);
      setState(206);
      number();
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<SyReCParser::ExpressionFromSignalContext>(_localctx);
      enterOuterAlt(_localctx, 2);
      setState(207);
      signal();
      break;
    }

    case 3: {
      _localctx = _tracker.createInstance<SyReCParser::ExpressionFromBinaryExpressionContext>(_localctx);
      enterOuterAlt(_localctx, 3);
      setState(208);
      binaryExpression();
      break;
    }

    case 4: {
      _localctx = _tracker.createInstance<SyReCParser::ExpressionFromUnaryExpressionContext>(_localctx);
      enterOuterAlt(_localctx, 4);
      setState(209);
      unaryExpression();
      break;
    }

    case 5: {
      _localctx = _tracker.createInstance<SyReCParser::ExpressionFromShiftExpressionContext>(_localctx);
      enterOuterAlt(_localctx, 5);
      setState(210);
      shiftExpression();
      break;
    }

    default:
      break;
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- BinaryExpressionContext ------------------------------------------------------------------

SyReCParser::BinaryExpressionContext::BinaryExpressionContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<SyReCParser::ExpressionContext *> SyReCParser::BinaryExpressionContext::expression() {
  return getRuleContexts<SyReCParser::ExpressionContext>();
}

SyReCParser::ExpressionContext* SyReCParser::BinaryExpressionContext::expression(size_t i) {
  return getRuleContext<SyReCParser::ExpressionContext>(i);
}

tree::TerminalNode* SyReCParser::BinaryExpressionContext::OP_PLUS() {
  return getToken(SyReCParser::OP_PLUS, 0);
}

tree::TerminalNode* SyReCParser::BinaryExpressionContext::OP_MINUS() {
  return getToken(SyReCParser::OP_MINUS, 0);
}

tree::TerminalNode* SyReCParser::BinaryExpressionContext::OP_XOR() {
  return getToken(SyReCParser::OP_XOR, 0);
}

tree::TerminalNode* SyReCParser::BinaryExpressionContext::OP_MULTIPLY() {
  return getToken(SyReCParser::OP_MULTIPLY, 0);
}

tree::TerminalNode* SyReCParser::BinaryExpressionContext::OP_DIVISION() {
  return getToken(SyReCParser::OP_DIVISION, 0);
}

tree::TerminalNode* SyReCParser::BinaryExpressionContext::OP_MODULO() {
  return getToken(SyReCParser::OP_MODULO, 0);
}

tree::TerminalNode* SyReCParser::BinaryExpressionContext::OP_UPPER_BIT_MULTIPLY() {
  return getToken(SyReCParser::OP_UPPER_BIT_MULTIPLY, 0);
}

tree::TerminalNode* SyReCParser::BinaryExpressionContext::OP_LOGICAL_AND() {
  return getToken(SyReCParser::OP_LOGICAL_AND, 0);
}

tree::TerminalNode* SyReCParser::BinaryExpressionContext::OP_LOGICAL_OR() {
  return getToken(SyReCParser::OP_LOGICAL_OR, 0);
}

tree::TerminalNode* SyReCParser::BinaryExpressionContext::OP_BITWISE_AND() {
  return getToken(SyReCParser::OP_BITWISE_AND, 0);
}

tree::TerminalNode* SyReCParser::BinaryExpressionContext::OP_BITWISE_OR() {
  return getToken(SyReCParser::OP_BITWISE_OR, 0);
}

tree::TerminalNode* SyReCParser::BinaryExpressionContext::OP_LESS_THAN() {
  return getToken(SyReCParser::OP_LESS_THAN, 0);
}

tree::TerminalNode* SyReCParser::BinaryExpressionContext::OP_GREATER_THAN() {
  return getToken(SyReCParser::OP_GREATER_THAN, 0);
}

tree::TerminalNode* SyReCParser::BinaryExpressionContext::OP_EQUAL() {
  return getToken(SyReCParser::OP_EQUAL, 0);
}

tree::TerminalNode* SyReCParser::BinaryExpressionContext::OP_NOT_EQUAL() {
  return getToken(SyReCParser::OP_NOT_EQUAL, 0);
}

tree::TerminalNode* SyReCParser::BinaryExpressionContext::OP_LESS_OR_EQUAL() {
  return getToken(SyReCParser::OP_LESS_OR_EQUAL, 0);
}

tree::TerminalNode* SyReCParser::BinaryExpressionContext::OP_GREATER_OR_EQUAL() {
  return getToken(SyReCParser::OP_GREATER_OR_EQUAL, 0);
}


size_t SyReCParser::BinaryExpressionContext::getRuleIndex() const {
  return SyReCParser::RuleBinaryExpression;
}


std::any SyReCParser::BinaryExpressionContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SyReCVisitor*>(visitor))
    return parserVisitor->visitBinaryExpression(this);
  else
    return visitor->visitChildren(this);
}

SyReCParser::BinaryExpressionContext* SyReCParser::binaryExpression() {
  BinaryExpressionContext *_localctx = _tracker.createInstance<BinaryExpressionContext>(_ctx, getState());
  enterRule(_localctx, 36, SyReCParser::RuleBinaryExpression);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(213);
    match(SyReCParser::T__0);
    setState(214);
    antlrcpp::downCast<BinaryExpressionContext *>(_localctx)->lhsOperand = expression();
    setState(215);
    antlrcpp::downCast<BinaryExpressionContext *>(_localctx)->binaryOperation = _input->LT(1);
    _la = _input->LA(1);
    if (!(((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 481034240000) != 0)) {
      antlrcpp::downCast<BinaryExpressionContext *>(_localctx)->binaryOperation = _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(216);
    antlrcpp::downCast<BinaryExpressionContext *>(_localctx)->rhsOperand = expression();
    setState(217);
    match(SyReCParser::T__1);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- UnaryExpressionContext ------------------------------------------------------------------

SyReCParser::UnaryExpressionContext::UnaryExpressionContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

SyReCParser::ExpressionContext* SyReCParser::UnaryExpressionContext::expression() {
  return getRuleContext<SyReCParser::ExpressionContext>(0);
}

tree::TerminalNode* SyReCParser::UnaryExpressionContext::OP_LOGICAL_NEGATION() {
  return getToken(SyReCParser::OP_LOGICAL_NEGATION, 0);
}

tree::TerminalNode* SyReCParser::UnaryExpressionContext::OP_BITWISE_NEGATION() {
  return getToken(SyReCParser::OP_BITWISE_NEGATION, 0);
}


size_t SyReCParser::UnaryExpressionContext::getRuleIndex() const {
  return SyReCParser::RuleUnaryExpression;
}


std::any SyReCParser::UnaryExpressionContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SyReCVisitor*>(visitor))
    return parserVisitor->visitUnaryExpression(this);
  else
    return visitor->visitChildren(this);
}

SyReCParser::UnaryExpressionContext* SyReCParser::unaryExpression() {
  UnaryExpressionContext *_localctx = _tracker.createInstance<UnaryExpressionContext>(_ctx, getState());
  enterRule(_localctx, 38, SyReCParser::RuleUnaryExpression);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(219);
    antlrcpp::downCast<UnaryExpressionContext *>(_localctx)->unaryOperation = _input->LT(1);
    _la = _input->LA(1);
    if (!(_la == SyReCParser::OP_BITWISE_NEGATION

    || _la == SyReCParser::OP_LOGICAL_NEGATION)) {
      antlrcpp::downCast<UnaryExpressionContext *>(_localctx)->unaryOperation = _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(220);
    expression();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ShiftExpressionContext ------------------------------------------------------------------

SyReCParser::ShiftExpressionContext::ShiftExpressionContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

SyReCParser::ExpressionContext* SyReCParser::ShiftExpressionContext::expression() {
  return getRuleContext<SyReCParser::ExpressionContext>(0);
}

SyReCParser::NumberContext* SyReCParser::ShiftExpressionContext::number() {
  return getRuleContext<SyReCParser::NumberContext>(0);
}

tree::TerminalNode* SyReCParser::ShiftExpressionContext::OP_RIGHT_SHIFT() {
  return getToken(SyReCParser::OP_RIGHT_SHIFT, 0);
}

tree::TerminalNode* SyReCParser::ShiftExpressionContext::OP_LEFT_SHIFT() {
  return getToken(SyReCParser::OP_LEFT_SHIFT, 0);
}


size_t SyReCParser::ShiftExpressionContext::getRuleIndex() const {
  return SyReCParser::RuleShiftExpression;
}


std::any SyReCParser::ShiftExpressionContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SyReCVisitor*>(visitor))
    return parserVisitor->visitShiftExpression(this);
  else
    return visitor->visitChildren(this);
}

SyReCParser::ShiftExpressionContext* SyReCParser::shiftExpression() {
  ShiftExpressionContext *_localctx = _tracker.createInstance<ShiftExpressionContext>(_ctx, getState());
  enterRule(_localctx, 40, SyReCParser::RuleShiftExpression);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(222);
    match(SyReCParser::T__0);
    setState(223);
    expression();
    setState(224);
    antlrcpp::downCast<ShiftExpressionContext *>(_localctx)->shiftOperation = _input->LT(1);
    _la = _input->LA(1);
    if (!(_la == SyReCParser::OP_LEFT_SHIFT

    || _la == SyReCParser::OP_RIGHT_SHIFT)) {
      antlrcpp::downCast<ShiftExpressionContext *>(_localctx)->shiftOperation = _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(225);
    number();
    setState(226);
    match(SyReCParser::T__1);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

void SyReCParser::initialize() {
  ::antlr4::internal::call_once(syrecParserOnceFlag, syrecParserInitialize);
}
