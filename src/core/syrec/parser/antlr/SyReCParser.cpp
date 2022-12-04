
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
      "'for'", "'to'", "'step'", "'rof'", "'if'", "'then'", "'else'", "'fi'", 
      "'<=>'", "'skip'", "'.'", "':'", "'+'", "'-'", "'*'", "'^'", "'>*'", 
      "'/'", "'%'", "'>'", "'>='", "'<'", "'<='", "'='", "'!='", "'&'", 
      "'|'", "'~'", "'&&'", "'||'", "'!'", "'>>'", "'<<'", "'++'", "'--'", 
      "'in'", "'out'", "'inout'", "'wire'", "'state'", "'$'", "'#'", "';'", 
      "','"
    },
    std::vector<std::string>{
      "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", 
      "", "", "", "OP_PLUS", "OP_MINUS", "OP_MULTIPLY", "OP_XOR", "OP_UPPER_BIT_MULTIPLY", 
      "OP_DIVISION", "OP_MODULO", "OP_GREATER_THAN", "OP_GREATER_OR_EQUAL", 
      "OP_LESS_THAN", "OP_LESS_OR_EQUAL", "OP_EQUAL", "OP_NOT_EQUAL", "OP_BITWISE_AND", 
      "OP_BITWISE_OR", "OP_BITWISE_NEGATION", "OP_LOGICAL_AND", "OP_LOGICAL_OR", 
      "OP_LOGICAL_NEGATION", "OP_LEFT_SHIFT", "OP_RIGHT_SHIFT", "OP_INCREMENT", 
      "OP_DECREMENT", "VAR_TYPE_IN", "VAR_TYPE_OUT", "VAR_TYPE_INOUT", "VAR_TYPE_WIRE", 
      "VAR_TYPE_STATE", "LOOP_VARIABLE_PREFIX", "SIGNAL_WIDTH_PREFIX", "STATEMENT_DELIMITER", 
      "PARAMETER_DELIMITER", "IDENT", "INT", "SKIPABLEWHITSPACES"
    }
  );
  static const int32_t serializedATNSegment[] = {
  	4,1,54,226,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,2,5,7,5,2,6,7,6,2,
  	7,7,7,2,8,7,8,2,9,7,9,2,10,7,10,2,11,7,11,2,12,7,12,2,13,7,13,2,14,7,
  	14,2,15,7,15,2,16,7,16,2,17,7,17,2,18,7,18,2,19,7,19,2,20,7,20,1,0,1,
  	0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,3,0,54,8,0,1,1,4,1,57,8,1,11,1,
  	12,1,58,1,2,1,2,1,2,1,2,1,2,1,2,5,2,67,8,2,10,2,12,2,70,9,2,1,2,1,2,1,
  	3,1,3,1,3,5,3,77,8,3,10,3,12,3,80,9,3,1,4,1,4,1,4,1,5,1,5,1,5,1,5,1,5,
  	1,5,5,5,91,8,5,10,5,12,5,94,9,5,1,6,1,6,1,6,1,6,5,6,100,8,6,10,6,12,6,
  	103,9,6,1,6,1,6,1,6,3,6,108,8,6,1,7,1,7,1,7,5,7,113,8,7,10,7,12,7,116,
  	9,7,1,8,1,8,1,8,1,8,1,8,1,8,1,8,3,8,125,8,8,1,9,1,9,1,9,1,9,1,9,1,9,5,
  	9,133,8,9,10,9,12,9,136,9,9,1,9,1,9,1,10,1,10,1,10,1,10,3,10,144,8,10,
  	1,10,1,10,1,10,3,10,149,8,10,1,10,1,10,1,10,3,10,154,8,10,1,10,3,10,157,
  	8,10,1,10,1,10,1,10,1,11,1,11,1,11,1,11,1,11,1,11,1,11,1,11,1,11,1,12,
  	1,12,1,12,1,12,1,13,1,13,1,13,1,13,1,13,1,14,1,14,1,14,1,14,1,15,1,15,
  	1,16,1,16,1,16,1,16,1,16,5,16,191,8,16,10,16,12,16,194,9,16,1,16,1,16,
  	1,16,1,16,3,16,200,8,16,3,16,202,8,16,1,17,1,17,1,17,1,17,1,17,3,17,209,
  	8,17,1,18,1,18,1,18,1,18,1,18,1,18,1,19,1,19,1,19,1,20,1,20,1,20,1,20,
  	1,20,1,20,1,20,0,0,21,0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,
  	36,38,40,0,8,2,0,20,22,25,25,1,0,43,45,1,0,6,7,2,0,35,35,41,42,2,0,20,
  	21,23,23,2,0,20,34,36,37,2,0,35,35,38,38,1,0,39,40,232,0,53,1,0,0,0,2,
  	56,1,0,0,0,4,60,1,0,0,0,6,73,1,0,0,0,8,81,1,0,0,0,10,84,1,0,0,0,12,95,
  	1,0,0,0,14,109,1,0,0,0,16,124,1,0,0,0,18,126,1,0,0,0,20,139,1,0,0,0,22,
  	161,1,0,0,0,24,170,1,0,0,0,26,174,1,0,0,0,28,179,1,0,0,0,30,183,1,0,0,
  	0,32,185,1,0,0,0,34,208,1,0,0,0,36,210,1,0,0,0,38,216,1,0,0,0,40,219,
  	1,0,0,0,42,54,5,53,0,0,43,44,5,49,0,0,44,54,5,52,0,0,45,46,5,48,0,0,46,
  	54,5,52,0,0,47,48,5,1,0,0,48,49,3,0,0,0,49,50,7,0,0,0,50,51,3,0,0,0,51,
  	52,5,2,0,0,52,54,1,0,0,0,53,42,1,0,0,0,53,43,1,0,0,0,53,45,1,0,0,0,53,
  	47,1,0,0,0,54,1,1,0,0,0,55,57,3,4,2,0,56,55,1,0,0,0,57,58,1,0,0,0,58,
  	56,1,0,0,0,58,59,1,0,0,0,59,3,1,0,0,0,60,61,5,3,0,0,61,62,5,52,0,0,62,
  	63,5,1,0,0,63,64,3,6,3,0,64,68,5,2,0,0,65,67,3,10,5,0,66,65,1,0,0,0,67,
  	70,1,0,0,0,68,66,1,0,0,0,68,69,1,0,0,0,69,71,1,0,0,0,70,68,1,0,0,0,71,
  	72,3,14,7,0,72,5,1,0,0,0,73,78,3,8,4,0,74,75,5,51,0,0,75,77,3,8,4,0,76,
  	74,1,0,0,0,77,80,1,0,0,0,78,76,1,0,0,0,78,79,1,0,0,0,79,7,1,0,0,0,80,
  	78,1,0,0,0,81,82,7,1,0,0,82,83,3,12,6,0,83,9,1,0,0,0,84,85,5,46,0,0,85,
  	86,5,47,0,0,86,87,1,0,0,0,87,92,3,12,6,0,88,89,5,51,0,0,89,91,3,12,6,
  	0,90,88,1,0,0,0,91,94,1,0,0,0,92,90,1,0,0,0,92,93,1,0,0,0,93,11,1,0,0,
  	0,94,92,1,0,0,0,95,101,5,52,0,0,96,97,5,4,0,0,97,98,5,53,0,0,98,100,5,
  	5,0,0,99,96,1,0,0,0,100,103,1,0,0,0,101,99,1,0,0,0,101,102,1,0,0,0,102,
  	107,1,0,0,0,103,101,1,0,0,0,104,105,5,1,0,0,105,106,5,53,0,0,106,108,
  	5,2,0,0,107,104,1,0,0,0,107,108,1,0,0,0,108,13,1,0,0,0,109,114,3,16,8,
  	0,110,111,5,50,0,0,111,113,3,16,8,0,112,110,1,0,0,0,113,116,1,0,0,0,114,
  	112,1,0,0,0,114,115,1,0,0,0,115,15,1,0,0,0,116,114,1,0,0,0,117,125,3,
  	18,9,0,118,125,3,20,10,0,119,125,3,22,11,0,120,125,3,24,12,0,121,125,
  	3,26,13,0,122,125,3,28,14,0,123,125,3,30,15,0,124,117,1,0,0,0,124,118,
  	1,0,0,0,124,119,1,0,0,0,124,120,1,0,0,0,124,121,1,0,0,0,124,122,1,0,0,
  	0,124,123,1,0,0,0,125,17,1,0,0,0,126,127,7,2,0,0,127,128,5,52,0,0,128,
  	129,5,1,0,0,129,134,5,52,0,0,130,131,5,51,0,0,131,133,5,52,0,0,132,130,
  	1,0,0,0,133,136,1,0,0,0,134,132,1,0,0,0,134,135,1,0,0,0,135,137,1,0,0,
  	0,136,134,1,0,0,0,137,138,5,2,0,0,138,19,1,0,0,0,139,148,5,8,0,0,140,
  	141,5,48,0,0,141,142,5,52,0,0,142,144,5,31,0,0,143,140,1,0,0,0,143,144,
  	1,0,0,0,144,145,1,0,0,0,145,146,3,0,0,0,146,147,5,9,0,0,147,149,1,0,0,
  	0,148,143,1,0,0,0,148,149,1,0,0,0,149,150,1,0,0,0,150,156,3,0,0,0,151,
  	153,5,10,0,0,152,154,5,21,0,0,153,152,1,0,0,0,153,154,1,0,0,0,154,155,
  	1,0,0,0,155,157,3,0,0,0,156,151,1,0,0,0,156,157,1,0,0,0,157,158,1,0,0,
  	0,158,159,3,14,7,0,159,160,5,11,0,0,160,21,1,0,0,0,161,162,5,12,0,0,162,
  	163,3,34,17,0,163,164,5,13,0,0,164,165,3,14,7,0,165,166,5,14,0,0,166,
  	167,3,14,7,0,167,168,5,15,0,0,168,169,3,34,17,0,169,23,1,0,0,0,170,171,
  	7,3,0,0,171,172,5,31,0,0,172,173,3,32,16,0,173,25,1,0,0,0,174,175,3,32,
  	16,0,175,176,7,4,0,0,176,177,5,31,0,0,177,178,3,34,17,0,178,27,1,0,0,
  	0,179,180,3,32,16,0,180,181,5,16,0,0,181,182,3,32,16,0,182,29,1,0,0,0,
  	183,184,5,17,0,0,184,31,1,0,0,0,185,192,5,52,0,0,186,187,5,4,0,0,187,
  	188,3,34,17,0,188,189,5,5,0,0,189,191,1,0,0,0,190,186,1,0,0,0,191,194,
  	1,0,0,0,192,190,1,0,0,0,192,193,1,0,0,0,193,201,1,0,0,0,194,192,1,0,0,
  	0,195,196,5,18,0,0,196,199,3,0,0,0,197,198,5,19,0,0,198,200,3,0,0,0,199,
  	197,1,0,0,0,199,200,1,0,0,0,200,202,1,0,0,0,201,195,1,0,0,0,201,202,1,
  	0,0,0,202,33,1,0,0,0,203,209,3,0,0,0,204,209,3,32,16,0,205,209,3,36,18,
  	0,206,209,3,38,19,0,207,209,3,40,20,0,208,203,1,0,0,0,208,204,1,0,0,0,
  	208,205,1,0,0,0,208,206,1,0,0,0,208,207,1,0,0,0,209,35,1,0,0,0,210,211,
  	5,1,0,0,211,212,3,34,17,0,212,213,7,5,0,0,213,214,3,34,17,0,214,215,5,
  	2,0,0,215,37,1,0,0,0,216,217,7,6,0,0,217,218,3,34,17,0,218,39,1,0,0,0,
  	219,220,5,1,0,0,220,221,3,34,17,0,221,222,7,7,0,0,222,223,3,0,0,0,223,
  	224,5,2,0,0,224,41,1,0,0,0,18,53,58,68,78,92,101,107,114,124,134,143,
  	148,153,156,192,199,201,208
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

tree::TerminalNode* SyReCParser::NumberContext::INT() {
  return getToken(SyReCParser::INT, 0);
}

tree::TerminalNode* SyReCParser::NumberContext::SIGNAL_WIDTH_PREFIX() {
  return getToken(SyReCParser::SIGNAL_WIDTH_PREFIX, 0);
}

tree::TerminalNode* SyReCParser::NumberContext::IDENT() {
  return getToken(SyReCParser::IDENT, 0);
}

tree::TerminalNode* SyReCParser::NumberContext::LOOP_VARIABLE_PREFIX() {
  return getToken(SyReCParser::LOOP_VARIABLE_PREFIX, 0);
}

std::vector<SyReCParser::NumberContext *> SyReCParser::NumberContext::number() {
  return getRuleContexts<SyReCParser::NumberContext>();
}

SyReCParser::NumberContext* SyReCParser::NumberContext::number(size_t i) {
  return getRuleContext<SyReCParser::NumberContext>(i);
}

tree::TerminalNode* SyReCParser::NumberContext::OP_PLUS() {
  return getToken(SyReCParser::OP_PLUS, 0);
}

tree::TerminalNode* SyReCParser::NumberContext::OP_MINUS() {
  return getToken(SyReCParser::OP_MINUS, 0);
}

tree::TerminalNode* SyReCParser::NumberContext::OP_MULTIPLY() {
  return getToken(SyReCParser::OP_MULTIPLY, 0);
}

tree::TerminalNode* SyReCParser::NumberContext::OP_DIVISION() {
  return getToken(SyReCParser::OP_DIVISION, 0);
}


size_t SyReCParser::NumberContext::getRuleIndex() const {
  return SyReCParser::RuleNumber;
}


std::any SyReCParser::NumberContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SyReCVisitor*>(visitor))
    return parserVisitor->visitNumber(this);
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
    enterOuterAlt(_localctx, 1);
    setState(53);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case SyReCParser::INT: {
        setState(42);
        match(SyReCParser::INT);
        break;
      }

      case SyReCParser::SIGNAL_WIDTH_PREFIX: {
        setState(43);
        match(SyReCParser::SIGNAL_WIDTH_PREFIX);
        setState(44);
        match(SyReCParser::IDENT);
        break;
      }

      case SyReCParser::LOOP_VARIABLE_PREFIX: {
        setState(45);
        match(SyReCParser::LOOP_VARIABLE_PREFIX);
        setState(46);
        match(SyReCParser::IDENT);
        break;
      }

      case SyReCParser::T__0: {
        setState(47);
        match(SyReCParser::T__0);
        setState(48);
        antlrcpp::downCast<NumberContext *>(_localctx)->lhsOperand = number();
        setState(49);
        _la = _input->LA(1);
        if (!(((_la & ~ 0x3fULL) == 0) &&
          ((1ULL << _la) & 40894464) != 0)) {
        _errHandler->recoverInline(this);
        }
        else {
          _errHandler->reportMatch(this);
          consume();
        }
        setState(50);
        antlrcpp::downCast<NumberContext *>(_localctx)->rhsOperand = number();
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

SyReCParser::ParameterListContext* SyReCParser::ModuleContext::parameterList() {
  return getRuleContext<SyReCParser::ParameterListContext>(0);
}

SyReCParser::StatementListContext* SyReCParser::ModuleContext::statementList() {
  return getRuleContext<SyReCParser::StatementListContext>(0);
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
    setState(60);
    match(SyReCParser::T__2);
    setState(61);
    match(SyReCParser::IDENT);
    setState(62);
    match(SyReCParser::T__0);
    setState(63);
    parameterList();
    setState(64);
    match(SyReCParser::T__1);
    setState(68);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SyReCParser::VAR_TYPE_WIRE) {
      setState(65);
      signalList();
      setState(70);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(71);
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
    setState(73);
    parameter();
    setState(78);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SyReCParser::PARAMETER_DELIMITER) {
      setState(74);
      match(SyReCParser::PARAMETER_DELIMITER);
      setState(75);
      parameter();
      setState(80);
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
    setState(81);
    _la = _input->LA(1);
    if (!(((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 61572651155456) != 0)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(82);
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
    setState(84);
    match(SyReCParser::VAR_TYPE_WIRE);
    setState(85);
    match(SyReCParser::VAR_TYPE_STATE);
    setState(87);
    signalDeclaration();
    setState(92);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SyReCParser::PARAMETER_DELIMITER) {
      setState(88);
      match(SyReCParser::PARAMETER_DELIMITER);
      setState(89);
      signalDeclaration();
      setState(94);
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
    setState(95);
    match(SyReCParser::IDENT);
    setState(101);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SyReCParser::T__3) {
      setState(96);
      match(SyReCParser::T__3);
      setState(97);
      antlrcpp::downCast<SignalDeclarationContext *>(_localctx)->intToken = match(SyReCParser::INT);
      antlrcpp::downCast<SignalDeclarationContext *>(_localctx)->dimensionTokens.push_back(antlrcpp::downCast<SignalDeclarationContext *>(_localctx)->intToken);
      setState(98);
      match(SyReCParser::T__4);
      setState(103);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(107);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == SyReCParser::T__0) {
      setState(104);
      match(SyReCParser::T__0);
      setState(105);
      antlrcpp::downCast<SignalDeclarationContext *>(_localctx)->signalWidthToken = match(SyReCParser::INT);
      setState(106);
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
    setState(109);
    antlrcpp::downCast<StatementListContext *>(_localctx)->statementContext = statement();
    antlrcpp::downCast<StatementListContext *>(_localctx)->stmts.push_back(antlrcpp::downCast<StatementListContext *>(_localctx)->statementContext);
    setState(114);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SyReCParser::STATEMENT_DELIMITER) {
      setState(110);
      match(SyReCParser::STATEMENT_DELIMITER);
      setState(111);
      antlrcpp::downCast<StatementListContext *>(_localctx)->statementContext = statement();
      antlrcpp::downCast<StatementListContext *>(_localctx)->stmts.push_back(antlrcpp::downCast<StatementListContext *>(_localctx)->statementContext);
      setState(116);
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
    setState(124);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 8, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(117);
      callStatement();
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(118);
      forStatement();
      break;
    }

    case 3: {
      enterOuterAlt(_localctx, 3);
      setState(119);
      ifStatement();
      break;
    }

    case 4: {
      enterOuterAlt(_localctx, 4);
      setState(120);
      unaryStatement();
      break;
    }

    case 5: {
      enterOuterAlt(_localctx, 5);
      setState(121);
      assignStatement();
      break;
    }

    case 6: {
      enterOuterAlt(_localctx, 6);
      setState(122);
      swapStatement();
      break;
    }

    case 7: {
      enterOuterAlt(_localctx, 7);
      setState(123);
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
    setState(126);
    _la = _input->LA(1);
    if (!(_la == SyReCParser::T__5

    || _la == SyReCParser::T__6)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(127);
    antlrcpp::downCast<CallStatementContext *>(_localctx)->moduleIdent = match(SyReCParser::IDENT);
    setState(128);
    match(SyReCParser::T__0);
    setState(129);
    antlrcpp::downCast<CallStatementContext *>(_localctx)->identToken = match(SyReCParser::IDENT);
    antlrcpp::downCast<CallStatementContext *>(_localctx)->calleArguments.push_back(antlrcpp::downCast<CallStatementContext *>(_localctx)->identToken);
    setState(134);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SyReCParser::PARAMETER_DELIMITER) {
      setState(130);
      match(SyReCParser::PARAMETER_DELIMITER);
      setState(131);
      antlrcpp::downCast<CallStatementContext *>(_localctx)->identToken = match(SyReCParser::IDENT);
      antlrcpp::downCast<CallStatementContext *>(_localctx)->calleeArguments.push_back(antlrcpp::downCast<CallStatementContext *>(_localctx)->identToken);
      setState(136);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(137);
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
    setState(139);
    match(SyReCParser::T__7);
    setState(148);
    _errHandler->sync(this);

    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 11, _ctx)) {
    case 1: {
      setState(143);
      _errHandler->sync(this);

      switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 10, _ctx)) {
      case 1: {
        setState(140);
        match(SyReCParser::LOOP_VARIABLE_PREFIX);
        setState(141);
        antlrcpp::downCast<ForStatementContext *>(_localctx)->variableIdent = match(SyReCParser::IDENT);
        setState(142);
        match(SyReCParser::OP_EQUAL);
        break;
      }

      default:
        break;
      }
      setState(145);
      antlrcpp::downCast<ForStatementContext *>(_localctx)->startValue = number();
      setState(146);
      match(SyReCParser::T__8);
      break;
    }

    default:
      break;
    }
    setState(150);
    antlrcpp::downCast<ForStatementContext *>(_localctx)->endValue = number();
    setState(156);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == SyReCParser::T__9) {
      setState(151);
      match(SyReCParser::T__9);
      setState(153);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == SyReCParser::OP_MINUS) {
        setState(152);
        match(SyReCParser::OP_MINUS);
      }
      setState(155);
      antlrcpp::downCast<ForStatementContext *>(_localctx)->stepSize = number();
    }
    setState(158);
    statementList();
    setState(159);
    match(SyReCParser::T__10);
   
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
    setState(161);
    match(SyReCParser::T__11);
    setState(162);
    antlrcpp::downCast<IfStatementContext *>(_localctx)->guardCondition = expression();
    setState(163);
    match(SyReCParser::T__12);
    setState(164);
    antlrcpp::downCast<IfStatementContext *>(_localctx)->trueBranchStmts = statementList();
    setState(165);
    match(SyReCParser::T__13);
    setState(166);
    antlrcpp::downCast<IfStatementContext *>(_localctx)->falseBranchStmts = statementList();
    setState(167);
    match(SyReCParser::T__14);
    setState(168);
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
    setState(170);
    _la = _input->LA(1);
    if (!(((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 6631429505024) != 0)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(171);
    match(SyReCParser::OP_EQUAL);
    setState(172);
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
    setState(174);
    signal();
    setState(175);
    _la = _input->LA(1);
    if (!(((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 11534336) != 0)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(176);
    match(SyReCParser::OP_EQUAL);
    setState(177);
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
    setState(179);
    antlrcpp::downCast<SwapStatementContext *>(_localctx)->lhsOperand = signal();
    setState(180);
    match(SyReCParser::T__15);
    setState(181);
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
    setState(183);
    match(SyReCParser::T__16);
   
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
    setState(185);
    match(SyReCParser::IDENT);
    setState(192);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SyReCParser::T__3) {
      setState(186);
      match(SyReCParser::T__3);
      setState(187);
      expression();
      setState(188);
      match(SyReCParser::T__4);
      setState(194);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(201);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == SyReCParser::T__17) {
      setState(195);
      match(SyReCParser::T__17);
      setState(196);
      antlrcpp::downCast<SignalContext *>(_localctx)->bitStart = number();
      setState(199);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == SyReCParser::T__18) {
        setState(197);
        match(SyReCParser::T__18);
        setState(198);
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

SyReCParser::NumberContext* SyReCParser::ExpressionContext::number() {
  return getRuleContext<SyReCParser::NumberContext>(0);
}

SyReCParser::SignalContext* SyReCParser::ExpressionContext::signal() {
  return getRuleContext<SyReCParser::SignalContext>(0);
}

SyReCParser::BinaryExpressionContext* SyReCParser::ExpressionContext::binaryExpression() {
  return getRuleContext<SyReCParser::BinaryExpressionContext>(0);
}

SyReCParser::UnaryExpressionContext* SyReCParser::ExpressionContext::unaryExpression() {
  return getRuleContext<SyReCParser::UnaryExpressionContext>(0);
}

SyReCParser::ShiftExpressionContext* SyReCParser::ExpressionContext::shiftExpression() {
  return getRuleContext<SyReCParser::ShiftExpressionContext>(0);
}


size_t SyReCParser::ExpressionContext::getRuleIndex() const {
  return SyReCParser::RuleExpression;
}


std::any SyReCParser::ExpressionContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SyReCVisitor*>(visitor))
    return parserVisitor->visitExpression(this);
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
    setState(208);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 17, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(203);
      number();
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(204);
      signal();
      break;
    }

    case 3: {
      enterOuterAlt(_localctx, 3);
      setState(205);
      binaryExpression();
      break;
    }

    case 4: {
      enterOuterAlt(_localctx, 4);
      setState(206);
      unaryExpression();
      break;
    }

    case 5: {
      enterOuterAlt(_localctx, 5);
      setState(207);
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
    setState(210);
    match(SyReCParser::T__0);
    setState(211);
    antlrcpp::downCast<BinaryExpressionContext *>(_localctx)->lhsOperand = expression();
    setState(212);
    _la = _input->LA(1);
    if (!(((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 240517120000) != 0)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(213);
    antlrcpp::downCast<BinaryExpressionContext *>(_localctx)->rhsOperand = expression();
    setState(214);
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
    setState(216);
    _la = _input->LA(1);
    if (!(_la == SyReCParser::OP_BITWISE_NEGATION

    || _la == SyReCParser::OP_LOGICAL_NEGATION)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(217);
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
    setState(219);
    match(SyReCParser::T__0);
    setState(220);
    expression();
    setState(221);
    _la = _input->LA(1);
    if (!(_la == SyReCParser::OP_LEFT_SHIFT

    || _la == SyReCParser::OP_RIGHT_SHIFT)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(222);
    number();
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

void SyReCParser::initialize() {
  ::antlr4::internal::call_once(syrecParserOnceFlag, syrecParserInitialize);
}
