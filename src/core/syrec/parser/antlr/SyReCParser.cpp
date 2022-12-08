
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
      "loopVariableDefinition", "loopStepsizeDefinition", "forStatement", 
      "ifStatement", "unaryStatement", "assignStatement", "swapStatement", 
      "skipStatement", "signal", "expression", "binaryExpression", "unaryExpression", 
      "shiftExpression"
    },
    std::vector<std::string>{
      "", "'('", "')'", "'module'", "'['", "']'", "'call'", "'uncall'", 
      "'step'", "'for'", "'to'", "'do'", "'rof'", "'if'", "'then'", "'else'", 
      "'fi'", "'<=>'", "'skip'", "'.'", "':'", "'+'", "'-'", "'*'", "'>*'", 
      "'/'", "'%'", "'>'", "'>='", "'<'", "'<='", "'='", "'!='", "'&'", 
      "'~'", "'|'", "'^'", "'&&'", "'||'", "'!'", "'>>'", "'<<'", "'++='", 
      "'--='", "'~='", "'+='", "'-='", "'^='", "'in'", "'out'", "'inout'", 
      "'wire'", "'state'", "'$'", "'#'", "';'", "','"
    },
    std::vector<std::string>{
      "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", 
      "", "", "", "", "OP_PLUS", "OP_MINUS", "OP_MULTIPLY", "OP_UPPER_BIT_MULTIPLY", 
      "OP_DIVISION", "OP_MODULO", "OP_GREATER_THAN", "OP_GREATER_OR_EQUAL", 
      "OP_LESS_THAN", "OP_LESS_OR_EQUAL", "OP_EQUAL", "OP_NOT_EQUAL", "OP_BITWISE_AND", 
      "OP_BITWISE_NEGATION", "OP_BITWISE_OR", "OP_BITWISE_XOR", "OP_LOGICAL_AND", 
      "OP_LOGICAL_OR", "OP_LOGICAL_NEGATION", "OP_LEFT_SHIFT", "OP_RIGHT_SHIFT", 
      "OP_INCREMENT_ASSIGN", "OP_DECREMENT_ASSIGN", "OP_INVERT_ASSIGN", 
      "OP_ADD_ASSIGN", "OP_SUB_ASSIGN", "OP_XOR_ASSIGN", "VAR_TYPE_IN", 
      "VAR_TYPE_OUT", "VAR_TYPE_INOUT", "VAR_TYPE_WIRE", "VAR_TYPE_STATE", 
      "LOOP_VARIABLE_PREFIX", "SIGNAL_WIDTH_PREFIX", "STATEMENT_DELIMITER", 
      "PARAMETER_DELIMITER", "LINE_COMMENT", "IDENT", "INT", "SKIPABLEWHITSPACES"
    }
  );
  static const int32_t serializedATNSegment[] = {
  	4,1,60,235,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,2,5,7,5,2,6,7,6,2,
  	7,7,7,2,8,7,8,2,9,7,9,2,10,7,10,2,11,7,11,2,12,7,12,2,13,7,13,2,14,7,
  	14,2,15,7,15,2,16,7,16,2,17,7,17,2,18,7,18,2,19,7,19,2,20,7,20,2,21,7,
  	21,2,22,7,22,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,3,0,58,8,0,1,
  	1,4,1,61,8,1,11,1,12,1,62,1,1,1,1,1,2,1,2,1,2,1,2,3,2,71,8,2,1,2,1,2,
  	5,2,75,8,2,10,2,12,2,78,9,2,1,2,1,2,1,3,1,3,1,3,5,3,85,8,3,10,3,12,3,
  	88,9,3,1,4,1,4,1,4,1,5,1,5,1,5,1,5,5,5,97,8,5,10,5,12,5,100,9,5,1,6,1,
  	6,1,6,1,6,5,6,106,8,6,10,6,12,6,109,9,6,1,6,1,6,1,6,3,6,114,8,6,1,7,1,
  	7,1,7,5,7,119,8,7,10,7,12,7,122,9,7,1,8,1,8,1,8,1,8,1,8,1,8,1,8,3,8,131,
  	8,8,1,9,1,9,1,9,1,9,1,9,1,9,5,9,139,8,9,10,9,12,9,142,9,9,1,9,1,9,1,10,
  	1,10,1,10,1,10,1,11,1,11,3,11,152,8,11,1,11,1,11,1,12,1,12,3,12,158,8,
  	12,1,12,1,12,1,12,3,12,163,8,12,1,12,1,12,3,12,167,8,12,1,12,1,12,1,12,
  	1,12,1,13,1,13,1,13,1,13,1,13,1,13,1,13,1,13,1,13,1,14,1,14,1,14,1,15,
  	1,15,1,15,1,15,1,16,1,16,1,16,1,16,1,17,1,17,1,18,1,18,1,18,1,18,1,18,
  	5,18,200,8,18,10,18,12,18,203,9,18,1,18,1,18,1,18,1,18,3,18,209,8,18,
  	3,18,211,8,18,1,19,1,19,1,19,1,19,1,19,3,19,218,8,19,1,20,1,20,1,20,1,
  	20,1,20,1,20,1,21,1,21,1,21,1,22,1,22,1,22,1,22,1,22,1,22,1,22,0,0,23,
  	0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,0,9,2,
  	0,21,23,25,25,1,0,48,50,1,0,51,52,1,0,6,7,1,0,42,44,1,0,45,47,2,0,21,
  	33,35,38,2,0,34,34,39,39,1,0,40,41,240,0,57,1,0,0,0,2,60,1,0,0,0,4,66,
  	1,0,0,0,6,81,1,0,0,0,8,89,1,0,0,0,10,92,1,0,0,0,12,101,1,0,0,0,14,115,
  	1,0,0,0,16,130,1,0,0,0,18,132,1,0,0,0,20,145,1,0,0,0,22,149,1,0,0,0,24,
  	155,1,0,0,0,26,172,1,0,0,0,28,181,1,0,0,0,30,184,1,0,0,0,32,188,1,0,0,
  	0,34,192,1,0,0,0,36,194,1,0,0,0,38,217,1,0,0,0,40,219,1,0,0,0,42,225,
  	1,0,0,0,44,228,1,0,0,0,46,58,5,59,0,0,47,48,5,54,0,0,48,58,5,58,0,0,49,
  	50,5,53,0,0,50,58,5,58,0,0,51,52,5,1,0,0,52,53,3,0,0,0,53,54,7,0,0,0,
  	54,55,3,0,0,0,55,56,5,2,0,0,56,58,1,0,0,0,57,46,1,0,0,0,57,47,1,0,0,0,
  	57,49,1,0,0,0,57,51,1,0,0,0,58,1,1,0,0,0,59,61,3,4,2,0,60,59,1,0,0,0,
  	61,62,1,0,0,0,62,60,1,0,0,0,62,63,1,0,0,0,63,64,1,0,0,0,64,65,5,0,0,1,
  	65,3,1,0,0,0,66,67,5,3,0,0,67,68,5,58,0,0,68,70,5,1,0,0,69,71,3,6,3,0,
  	70,69,1,0,0,0,70,71,1,0,0,0,71,72,1,0,0,0,72,76,5,2,0,0,73,75,3,10,5,
  	0,74,73,1,0,0,0,75,78,1,0,0,0,76,74,1,0,0,0,76,77,1,0,0,0,77,79,1,0,0,
  	0,78,76,1,0,0,0,79,80,3,14,7,0,80,5,1,0,0,0,81,86,3,8,4,0,82,83,5,56,
  	0,0,83,85,3,8,4,0,84,82,1,0,0,0,85,88,1,0,0,0,86,84,1,0,0,0,86,87,1,0,
  	0,0,87,7,1,0,0,0,88,86,1,0,0,0,89,90,7,1,0,0,90,91,3,12,6,0,91,9,1,0,
  	0,0,92,93,7,2,0,0,93,98,3,12,6,0,94,95,5,56,0,0,95,97,3,12,6,0,96,94,
  	1,0,0,0,97,100,1,0,0,0,98,96,1,0,0,0,98,99,1,0,0,0,99,11,1,0,0,0,100,
  	98,1,0,0,0,101,107,5,58,0,0,102,103,5,4,0,0,103,104,5,59,0,0,104,106,
  	5,5,0,0,105,102,1,0,0,0,106,109,1,0,0,0,107,105,1,0,0,0,107,108,1,0,0,
  	0,108,113,1,0,0,0,109,107,1,0,0,0,110,111,5,1,0,0,111,112,5,59,0,0,112,
  	114,5,2,0,0,113,110,1,0,0,0,113,114,1,0,0,0,114,13,1,0,0,0,115,120,3,
  	16,8,0,116,117,5,55,0,0,117,119,3,16,8,0,118,116,1,0,0,0,119,122,1,0,
  	0,0,120,118,1,0,0,0,120,121,1,0,0,0,121,15,1,0,0,0,122,120,1,0,0,0,123,
  	131,3,18,9,0,124,131,3,24,12,0,125,131,3,26,13,0,126,131,3,28,14,0,127,
  	131,3,30,15,0,128,131,3,32,16,0,129,131,3,34,17,0,130,123,1,0,0,0,130,
  	124,1,0,0,0,130,125,1,0,0,0,130,126,1,0,0,0,130,127,1,0,0,0,130,128,1,
  	0,0,0,130,129,1,0,0,0,131,17,1,0,0,0,132,133,7,3,0,0,133,134,5,58,0,0,
  	134,135,5,1,0,0,135,140,5,58,0,0,136,137,5,56,0,0,137,139,5,58,0,0,138,
  	136,1,0,0,0,139,142,1,0,0,0,140,138,1,0,0,0,140,141,1,0,0,0,141,143,1,
  	0,0,0,142,140,1,0,0,0,143,144,5,2,0,0,144,19,1,0,0,0,145,146,5,53,0,0,
  	146,147,5,58,0,0,147,148,5,31,0,0,148,21,1,0,0,0,149,151,5,8,0,0,150,
  	152,5,22,0,0,151,150,1,0,0,0,151,152,1,0,0,0,152,153,1,0,0,0,153,154,
  	3,0,0,0,154,23,1,0,0,0,155,162,5,9,0,0,156,158,3,20,10,0,157,156,1,0,
  	0,0,157,158,1,0,0,0,158,159,1,0,0,0,159,160,3,0,0,0,160,161,5,10,0,0,
  	161,163,1,0,0,0,162,157,1,0,0,0,162,163,1,0,0,0,163,164,1,0,0,0,164,166,
  	3,0,0,0,165,167,3,22,11,0,166,165,1,0,0,0,166,167,1,0,0,0,167,168,1,0,
  	0,0,168,169,5,11,0,0,169,170,3,14,7,0,170,171,5,12,0,0,171,25,1,0,0,0,
  	172,173,5,13,0,0,173,174,3,38,19,0,174,175,5,14,0,0,175,176,3,14,7,0,
  	176,177,5,15,0,0,177,178,3,14,7,0,178,179,5,16,0,0,179,180,3,38,19,0,
  	180,27,1,0,0,0,181,182,7,4,0,0,182,183,3,36,18,0,183,29,1,0,0,0,184,185,
  	3,36,18,0,185,186,7,5,0,0,186,187,3,38,19,0,187,31,1,0,0,0,188,189,3,
  	36,18,0,189,190,5,17,0,0,190,191,3,36,18,0,191,33,1,0,0,0,192,193,5,18,
  	0,0,193,35,1,0,0,0,194,201,5,58,0,0,195,196,5,4,0,0,196,197,3,38,19,0,
  	197,198,5,5,0,0,198,200,1,0,0,0,199,195,1,0,0,0,200,203,1,0,0,0,201,199,
  	1,0,0,0,201,202,1,0,0,0,202,210,1,0,0,0,203,201,1,0,0,0,204,205,5,19,
  	0,0,205,208,3,0,0,0,206,207,5,20,0,0,207,209,3,0,0,0,208,206,1,0,0,0,
  	208,209,1,0,0,0,209,211,1,0,0,0,210,204,1,0,0,0,210,211,1,0,0,0,211,37,
  	1,0,0,0,212,218,3,0,0,0,213,218,3,36,18,0,214,218,3,40,20,0,215,218,3,
  	42,21,0,216,218,3,44,22,0,217,212,1,0,0,0,217,213,1,0,0,0,217,214,1,0,
  	0,0,217,215,1,0,0,0,217,216,1,0,0,0,218,39,1,0,0,0,219,220,5,1,0,0,220,
  	221,3,38,19,0,221,222,7,6,0,0,222,223,3,38,19,0,223,224,5,2,0,0,224,41,
  	1,0,0,0,225,226,7,7,0,0,226,227,3,38,19,0,227,43,1,0,0,0,228,229,5,1,
  	0,0,229,230,3,38,19,0,230,231,7,8,0,0,231,232,3,0,0,0,232,233,5,2,0,0,
  	233,45,1,0,0,0,19,57,62,70,76,86,98,107,113,120,130,140,151,157,162,166,
  	201,208,210,217
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
    setState(57);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case SyReCParser::INT: {
        _localctx = _tracker.createInstance<SyReCParser::NumberFromConstantContext>(_localctx);
        enterOuterAlt(_localctx, 1);
        setState(46);
        match(SyReCParser::INT);
        break;
      }

      case SyReCParser::SIGNAL_WIDTH_PREFIX: {
        _localctx = _tracker.createInstance<SyReCParser::NumberFromSignalwidthContext>(_localctx);
        enterOuterAlt(_localctx, 2);
        setState(47);
        match(SyReCParser::SIGNAL_WIDTH_PREFIX);
        setState(48);
        match(SyReCParser::IDENT);
        break;
      }

      case SyReCParser::LOOP_VARIABLE_PREFIX: {
        _localctx = _tracker.createInstance<SyReCParser::NumberFromLoopVariableContext>(_localctx);
        enterOuterAlt(_localctx, 3);
        setState(49);
        match(SyReCParser::LOOP_VARIABLE_PREFIX);
        setState(50);
        match(SyReCParser::IDENT);
        break;
      }

      case SyReCParser::T__0: {
        _localctx = _tracker.createInstance<SyReCParser::NumberFromExpressionContext>(_localctx);
        enterOuterAlt(_localctx, 4);
        setState(51);
        match(SyReCParser::T__0);
        setState(52);
        antlrcpp::downCast<NumberFromExpressionContext *>(_localctx)->lhsOperand = number();
        setState(53);
        antlrcpp::downCast<NumberFromExpressionContext *>(_localctx)->op = _input->LT(1);
        _la = _input->LA(1);
        if (!(((_la & ~ 0x3fULL) == 0) &&
          ((1ULL << _la) & 48234496) != 0)) {
          antlrcpp::downCast<NumberFromExpressionContext *>(_localctx)->op = _errHandler->recoverInline(this);
        }
        else {
          _errHandler->reportMatch(this);
          consume();
        }
        setState(54);
        antlrcpp::downCast<NumberFromExpressionContext *>(_localctx)->rhsOperand = number();
        setState(55);
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
    setState(60); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(59);
      module();
      setState(62); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while (_la == SyReCParser::T__2);
    setState(64);
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
    setState(66);
    match(SyReCParser::T__2);
    setState(67);
    match(SyReCParser::IDENT);
    setState(68);
    match(SyReCParser::T__0);
    setState(70);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 1970324836974592) != 0) {
      setState(69);
      parameterList();
    }
    setState(72);
    match(SyReCParser::T__1);
    setState(76);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SyReCParser::VAR_TYPE_WIRE

    || _la == SyReCParser::VAR_TYPE_STATE) {
      setState(73);
      signalList();
      setState(78);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(79);
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
    setState(81);
    parameter();
    setState(86);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SyReCParser::PARAMETER_DELIMITER) {
      setState(82);
      match(SyReCParser::PARAMETER_DELIMITER);
      setState(83);
      parameter();
      setState(88);
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
    setState(89);
    _la = _input->LA(1);
    if (!(((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 1970324836974592) != 0)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(90);
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
    setState(92);
    _la = _input->LA(1);
    if (!(_la == SyReCParser::VAR_TYPE_WIRE

    || _la == SyReCParser::VAR_TYPE_STATE)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(93);
    signalDeclaration();
    setState(98);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SyReCParser::PARAMETER_DELIMITER) {
      setState(94);
      match(SyReCParser::PARAMETER_DELIMITER);
      setState(95);
      signalDeclaration();
      setState(100);
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
    setState(101);
    match(SyReCParser::IDENT);
    setState(107);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SyReCParser::T__3) {
      setState(102);
      match(SyReCParser::T__3);
      setState(103);
      antlrcpp::downCast<SignalDeclarationContext *>(_localctx)->intToken = match(SyReCParser::INT);
      antlrcpp::downCast<SignalDeclarationContext *>(_localctx)->dimensionTokens.push_back(antlrcpp::downCast<SignalDeclarationContext *>(_localctx)->intToken);
      setState(104);
      match(SyReCParser::T__4);
      setState(109);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(113);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == SyReCParser::T__0) {
      setState(110);
      match(SyReCParser::T__0);
      setState(111);
      antlrcpp::downCast<SignalDeclarationContext *>(_localctx)->signalWidthToken = match(SyReCParser::INT);
      setState(112);
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
    setState(115);
    antlrcpp::downCast<StatementListContext *>(_localctx)->statementContext = statement();
    antlrcpp::downCast<StatementListContext *>(_localctx)->stmts.push_back(antlrcpp::downCast<StatementListContext *>(_localctx)->statementContext);
    setState(120);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SyReCParser::STATEMENT_DELIMITER) {
      setState(116);
      match(SyReCParser::STATEMENT_DELIMITER);
      setState(117);
      antlrcpp::downCast<StatementListContext *>(_localctx)->statementContext = statement();
      antlrcpp::downCast<StatementListContext *>(_localctx)->stmts.push_back(antlrcpp::downCast<StatementListContext *>(_localctx)->statementContext);
      setState(122);
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
    setState(130);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 9, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(123);
      callStatement();
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(124);
      forStatement();
      break;
    }

    case 3: {
      enterOuterAlt(_localctx, 3);
      setState(125);
      ifStatement();
      break;
    }

    case 4: {
      enterOuterAlt(_localctx, 4);
      setState(126);
      unaryStatement();
      break;
    }

    case 5: {
      enterOuterAlt(_localctx, 5);
      setState(127);
      assignStatement();
      break;
    }

    case 6: {
      enterOuterAlt(_localctx, 6);
      setState(128);
      swapStatement();
      break;
    }

    case 7: {
      enterOuterAlt(_localctx, 7);
      setState(129);
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
    setState(132);
    antlrcpp::downCast<CallStatementContext *>(_localctx)->callOp = _input->LT(1);
    _la = _input->LA(1);
    if (!(_la == SyReCParser::T__5

    || _la == SyReCParser::T__6)) {
      antlrcpp::downCast<CallStatementContext *>(_localctx)->callOp = _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(133);
    antlrcpp::downCast<CallStatementContext *>(_localctx)->moduleIdent = match(SyReCParser::IDENT);
    setState(134);
    match(SyReCParser::T__0);
    setState(135);
    antlrcpp::downCast<CallStatementContext *>(_localctx)->identToken = match(SyReCParser::IDENT);
    antlrcpp::downCast<CallStatementContext *>(_localctx)->calleArguments.push_back(antlrcpp::downCast<CallStatementContext *>(_localctx)->identToken);
    setState(140);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SyReCParser::PARAMETER_DELIMITER) {
      setState(136);
      match(SyReCParser::PARAMETER_DELIMITER);
      setState(137);
      antlrcpp::downCast<CallStatementContext *>(_localctx)->identToken = match(SyReCParser::IDENT);
      antlrcpp::downCast<CallStatementContext *>(_localctx)->calleeArguments.push_back(antlrcpp::downCast<CallStatementContext *>(_localctx)->identToken);
      setState(142);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(143);
    match(SyReCParser::T__1);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- LoopVariableDefinitionContext ------------------------------------------------------------------

SyReCParser::LoopVariableDefinitionContext::LoopVariableDefinitionContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* SyReCParser::LoopVariableDefinitionContext::LOOP_VARIABLE_PREFIX() {
  return getToken(SyReCParser::LOOP_VARIABLE_PREFIX, 0);
}

tree::TerminalNode* SyReCParser::LoopVariableDefinitionContext::OP_EQUAL() {
  return getToken(SyReCParser::OP_EQUAL, 0);
}

tree::TerminalNode* SyReCParser::LoopVariableDefinitionContext::IDENT() {
  return getToken(SyReCParser::IDENT, 0);
}


size_t SyReCParser::LoopVariableDefinitionContext::getRuleIndex() const {
  return SyReCParser::RuleLoopVariableDefinition;
}


std::any SyReCParser::LoopVariableDefinitionContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SyReCVisitor*>(visitor))
    return parserVisitor->visitLoopVariableDefinition(this);
  else
    return visitor->visitChildren(this);
}

SyReCParser::LoopVariableDefinitionContext* SyReCParser::loopVariableDefinition() {
  LoopVariableDefinitionContext *_localctx = _tracker.createInstance<LoopVariableDefinitionContext>(_ctx, getState());
  enterRule(_localctx, 20, SyReCParser::RuleLoopVariableDefinition);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(145);
    match(SyReCParser::LOOP_VARIABLE_PREFIX);
    setState(146);
    antlrcpp::downCast<LoopVariableDefinitionContext *>(_localctx)->variableIdent = match(SyReCParser::IDENT);
    setState(147);
    match(SyReCParser::OP_EQUAL);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- LoopStepsizeDefinitionContext ------------------------------------------------------------------

SyReCParser::LoopStepsizeDefinitionContext::LoopStepsizeDefinitionContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

SyReCParser::NumberContext* SyReCParser::LoopStepsizeDefinitionContext::number() {
  return getRuleContext<SyReCParser::NumberContext>(0);
}

tree::TerminalNode* SyReCParser::LoopStepsizeDefinitionContext::OP_MINUS() {
  return getToken(SyReCParser::OP_MINUS, 0);
}


size_t SyReCParser::LoopStepsizeDefinitionContext::getRuleIndex() const {
  return SyReCParser::RuleLoopStepsizeDefinition;
}


std::any SyReCParser::LoopStepsizeDefinitionContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SyReCVisitor*>(visitor))
    return parserVisitor->visitLoopStepsizeDefinition(this);
  else
    return visitor->visitChildren(this);
}

SyReCParser::LoopStepsizeDefinitionContext* SyReCParser::loopStepsizeDefinition() {
  LoopStepsizeDefinitionContext *_localctx = _tracker.createInstance<LoopStepsizeDefinitionContext>(_ctx, getState());
  enterRule(_localctx, 22, SyReCParser::RuleLoopStepsizeDefinition);
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
    setState(149);
    match(SyReCParser::T__7);
    setState(151);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == SyReCParser::OP_MINUS) {
      setState(150);
      match(SyReCParser::OP_MINUS);
    }
    setState(153);
    number();
   
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

SyReCParser::LoopStepsizeDefinitionContext* SyReCParser::ForStatementContext::loopStepsizeDefinition() {
  return getRuleContext<SyReCParser::LoopStepsizeDefinitionContext>(0);
}

SyReCParser::LoopVariableDefinitionContext* SyReCParser::ForStatementContext::loopVariableDefinition() {
  return getRuleContext<SyReCParser::LoopVariableDefinitionContext>(0);
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
  enterRule(_localctx, 24, SyReCParser::RuleForStatement);
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
    setState(155);
    match(SyReCParser::T__8);
    setState(162);
    _errHandler->sync(this);

    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 13, _ctx)) {
    case 1: {
      setState(157);
      _errHandler->sync(this);

      switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 12, _ctx)) {
      case 1: {
        setState(156);
        loopVariableDefinition();
        break;
      }

      default:
        break;
      }
      setState(159);
      antlrcpp::downCast<ForStatementContext *>(_localctx)->startValue = number();
      setState(160);
      match(SyReCParser::T__9);
      break;
    }

    default:
      break;
    }
    setState(164);
    antlrcpp::downCast<ForStatementContext *>(_localctx)->endValue = number();
    setState(166);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == SyReCParser::T__7) {
      setState(165);
      loopStepsizeDefinition();
    }
    setState(168);
    match(SyReCParser::T__10);
    setState(169);
    statementList();
    setState(170);
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
  enterRule(_localctx, 26, SyReCParser::RuleIfStatement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(172);
    match(SyReCParser::T__12);
    setState(173);
    antlrcpp::downCast<IfStatementContext *>(_localctx)->guardCondition = expression();
    setState(174);
    match(SyReCParser::T__13);
    setState(175);
    antlrcpp::downCast<IfStatementContext *>(_localctx)->trueBranchStmts = statementList();
    setState(176);
    match(SyReCParser::T__14);
    setState(177);
    antlrcpp::downCast<IfStatementContext *>(_localctx)->falseBranchStmts = statementList();
    setState(178);
    match(SyReCParser::T__15);
    setState(179);
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

SyReCParser::SignalContext* SyReCParser::UnaryStatementContext::signal() {
  return getRuleContext<SyReCParser::SignalContext>(0);
}

tree::TerminalNode* SyReCParser::UnaryStatementContext::OP_INVERT_ASSIGN() {
  return getToken(SyReCParser::OP_INVERT_ASSIGN, 0);
}

tree::TerminalNode* SyReCParser::UnaryStatementContext::OP_INCREMENT_ASSIGN() {
  return getToken(SyReCParser::OP_INCREMENT_ASSIGN, 0);
}

tree::TerminalNode* SyReCParser::UnaryStatementContext::OP_DECREMENT_ASSIGN() {
  return getToken(SyReCParser::OP_DECREMENT_ASSIGN, 0);
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
  enterRule(_localctx, 28, SyReCParser::RuleUnaryStatement);
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
    setState(181);
    antlrcpp::downCast<UnaryStatementContext *>(_localctx)->unaryOp = _input->LT(1);
    _la = _input->LA(1);
    if (!(((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 30786325577728) != 0)) {
      antlrcpp::downCast<UnaryStatementContext *>(_localctx)->unaryOp = _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(182);
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

SyReCParser::ExpressionContext* SyReCParser::AssignStatementContext::expression() {
  return getRuleContext<SyReCParser::ExpressionContext>(0);
}

tree::TerminalNode* SyReCParser::AssignStatementContext::OP_ADD_ASSIGN() {
  return getToken(SyReCParser::OP_ADD_ASSIGN, 0);
}

tree::TerminalNode* SyReCParser::AssignStatementContext::OP_SUB_ASSIGN() {
  return getToken(SyReCParser::OP_SUB_ASSIGN, 0);
}

tree::TerminalNode* SyReCParser::AssignStatementContext::OP_XOR_ASSIGN() {
  return getToken(SyReCParser::OP_XOR_ASSIGN, 0);
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
  enterRule(_localctx, 30, SyReCParser::RuleAssignStatement);
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
    setState(184);
    signal();
    setState(185);
    antlrcpp::downCast<AssignStatementContext *>(_localctx)->assignmentOp = _input->LT(1);
    _la = _input->LA(1);
    if (!(((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 246290604621824) != 0)) {
      antlrcpp::downCast<AssignStatementContext *>(_localctx)->assignmentOp = _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(186);
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
  enterRule(_localctx, 32, SyReCParser::RuleSwapStatement);

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
    antlrcpp::downCast<SwapStatementContext *>(_localctx)->lhsOperand = signal();
    setState(189);
    match(SyReCParser::T__16);
    setState(190);
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
  enterRule(_localctx, 34, SyReCParser::RuleSkipStatement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(192);
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
  enterRule(_localctx, 36, SyReCParser::RuleSignal);
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
    setState(194);
    match(SyReCParser::IDENT);
    setState(201);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SyReCParser::T__3) {
      setState(195);
      match(SyReCParser::T__3);
      setState(196);
      antlrcpp::downCast<SignalContext *>(_localctx)->expressionContext = expression();
      antlrcpp::downCast<SignalContext *>(_localctx)->accessedDimensions.push_back(antlrcpp::downCast<SignalContext *>(_localctx)->expressionContext);
      setState(197);
      match(SyReCParser::T__4);
      setState(203);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(210);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == SyReCParser::T__18) {
      setState(204);
      match(SyReCParser::T__18);
      setState(205);
      antlrcpp::downCast<SignalContext *>(_localctx)->bitStart = number();
      setState(208);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == SyReCParser::T__19) {
        setState(206);
        match(SyReCParser::T__19);
        setState(207);
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
  enterRule(_localctx, 38, SyReCParser::RuleExpression);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(217);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 18, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<SyReCParser::ExpressionFromNumberContext>(_localctx);
      enterOuterAlt(_localctx, 1);
      setState(212);
      number();
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<SyReCParser::ExpressionFromSignalContext>(_localctx);
      enterOuterAlt(_localctx, 2);
      setState(213);
      signal();
      break;
    }

    case 3: {
      _localctx = _tracker.createInstance<SyReCParser::ExpressionFromBinaryExpressionContext>(_localctx);
      enterOuterAlt(_localctx, 3);
      setState(214);
      binaryExpression();
      break;
    }

    case 4: {
      _localctx = _tracker.createInstance<SyReCParser::ExpressionFromUnaryExpressionContext>(_localctx);
      enterOuterAlt(_localctx, 4);
      setState(215);
      unaryExpression();
      break;
    }

    case 5: {
      _localctx = _tracker.createInstance<SyReCParser::ExpressionFromShiftExpressionContext>(_localctx);
      enterOuterAlt(_localctx, 5);
      setState(216);
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

tree::TerminalNode* SyReCParser::BinaryExpressionContext::OP_BITWISE_XOR() {
  return getToken(SyReCParser::OP_BITWISE_XOR, 0);
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
  enterRule(_localctx, 40, SyReCParser::RuleBinaryExpression);
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
    match(SyReCParser::T__0);
    setState(220);
    antlrcpp::downCast<BinaryExpressionContext *>(_localctx)->lhsOperand = expression();
    setState(221);
    antlrcpp::downCast<BinaryExpressionContext *>(_localctx)->binaryOperation = _input->LT(1);
    _la = _input->LA(1);
    if (!(((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 532573847552) != 0)) {
      antlrcpp::downCast<BinaryExpressionContext *>(_localctx)->binaryOperation = _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(222);
    antlrcpp::downCast<BinaryExpressionContext *>(_localctx)->rhsOperand = expression();
    setState(223);
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
  enterRule(_localctx, 42, SyReCParser::RuleUnaryExpression);
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
    setState(225);
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
    setState(226);
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
  enterRule(_localctx, 44, SyReCParser::RuleShiftExpression);
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
    setState(228);
    match(SyReCParser::T__0);
    setState(229);
    expression();
    setState(230);
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
    setState(231);
    number();
    setState(232);
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
