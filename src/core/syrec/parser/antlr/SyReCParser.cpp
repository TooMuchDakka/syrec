
#include <vector>
#include <string>


// Generated from ..\syrec_antlr\src\core\syrec\parser\antlr\SyReC.g4 by ANTLR 4.11.1



#include "SyReCParser.h"


using namespace antlrcpp;
using namespace syrec;

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
      "", "'#'", "'$'", "'('", "'+'", "'-'", "'*'", "'/'", "')'", "'module'", 
      "','", "'in'", "'out'", "'inout'", "'wire'", "'state'", "'['", "']'", 
      "';'", "'call'", "'uncall'", "'for'", "'='", "'to'", "'step'", "'rof'", 
      "'if'", "'then'", "'else'", "'fi'", "'~'", "'++'", "'--'", "'^'", 
      "'<=>'", "'skip'", "'.'", "':'", "'%'", "'*>'", "'&&'", "'||'", "'&'", 
      "'|'", "'<'", "'>'", "'!='", "'<='", "'>='", "'!'", "'<<'", "'>>'"
    },
    std::vector<std::string>{
      "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", 
      "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", 
      "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", 
      "", "IDENT", "INT", "SKIPABLEWHITSPACES"
    }
  );
  static const int32_t serializedATNSegment[] = {
  	4,1,54,224,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,2,5,7,5,2,6,7,6,2,
  	7,7,7,2,8,7,8,2,9,7,9,2,10,7,10,2,11,7,11,2,12,7,12,2,13,7,13,2,14,7,
  	14,2,15,7,15,2,16,7,16,2,17,7,17,2,18,7,18,2,19,7,19,2,20,7,20,1,0,1,
  	0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,3,0,54,8,0,1,1,4,1,57,8,1,11,1,
  	12,1,58,1,2,1,2,1,2,1,2,1,2,1,2,5,2,67,8,2,10,2,12,2,70,9,2,1,2,1,2,1,
  	3,1,3,1,3,5,3,77,8,3,10,3,12,3,80,9,3,1,4,1,4,1,4,1,5,1,5,1,5,1,5,5,5,
  	89,8,5,10,5,12,5,92,9,5,1,6,1,6,1,6,1,6,5,6,98,8,6,10,6,12,6,101,9,6,
  	1,6,1,6,1,6,3,6,106,8,6,1,7,1,7,1,7,5,7,111,8,7,10,7,12,7,114,9,7,1,8,
  	1,8,1,8,1,8,1,8,1,8,1,8,3,8,123,8,8,1,9,1,9,1,9,1,9,1,9,1,9,5,9,131,8,
  	9,10,9,12,9,134,9,9,1,9,1,9,1,10,1,10,1,10,1,10,3,10,142,8,10,1,10,1,
  	10,1,10,3,10,147,8,10,1,10,1,10,1,10,3,10,152,8,10,1,10,3,10,155,8,10,
  	1,10,1,10,1,10,1,11,1,11,1,11,1,11,1,11,1,11,1,11,1,11,1,11,1,12,1,12,
  	1,12,1,12,1,13,1,13,1,13,1,13,1,13,1,14,1,14,1,14,1,14,1,15,1,15,1,16,
  	1,16,1,16,1,16,1,16,5,16,189,8,16,10,16,12,16,192,9,16,1,16,1,16,1,16,
  	1,16,3,16,198,8,16,3,16,200,8,16,1,17,1,17,1,17,1,17,1,17,3,17,207,8,
  	17,1,18,1,18,1,18,1,18,1,18,1,18,1,19,1,19,1,19,1,20,1,20,1,20,1,20,1,
  	20,1,20,1,20,0,0,21,0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,
  	36,38,40,0,9,1,0,4,7,1,0,11,13,1,0,14,15,1,0,19,20,1,0,30,32,2,0,4,5,
  	33,33,4,0,4,7,22,22,33,33,38,48,2,0,30,30,49,49,1,0,50,51,230,0,53,1,
  	0,0,0,2,56,1,0,0,0,4,60,1,0,0,0,6,73,1,0,0,0,8,81,1,0,0,0,10,84,1,0,0,
  	0,12,93,1,0,0,0,14,107,1,0,0,0,16,122,1,0,0,0,18,124,1,0,0,0,20,137,1,
  	0,0,0,22,159,1,0,0,0,24,168,1,0,0,0,26,172,1,0,0,0,28,177,1,0,0,0,30,
  	181,1,0,0,0,32,183,1,0,0,0,34,206,1,0,0,0,36,208,1,0,0,0,38,214,1,0,0,
  	0,40,217,1,0,0,0,42,54,5,53,0,0,43,44,5,1,0,0,44,54,5,52,0,0,45,46,5,
  	2,0,0,46,54,5,52,0,0,47,48,5,3,0,0,48,49,3,0,0,0,49,50,7,0,0,0,50,51,
  	3,0,0,0,51,52,5,8,0,0,52,54,1,0,0,0,53,42,1,0,0,0,53,43,1,0,0,0,53,45,
  	1,0,0,0,53,47,1,0,0,0,54,1,1,0,0,0,55,57,3,4,2,0,56,55,1,0,0,0,57,58,
  	1,0,0,0,58,56,1,0,0,0,58,59,1,0,0,0,59,3,1,0,0,0,60,61,5,9,0,0,61,62,
  	5,52,0,0,62,63,5,3,0,0,63,64,3,6,3,0,64,68,5,8,0,0,65,67,3,10,5,0,66,
  	65,1,0,0,0,67,70,1,0,0,0,68,66,1,0,0,0,68,69,1,0,0,0,69,71,1,0,0,0,70,
  	68,1,0,0,0,71,72,3,14,7,0,72,5,1,0,0,0,73,78,3,8,4,0,74,75,5,10,0,0,75,
  	77,3,8,4,0,76,74,1,0,0,0,77,80,1,0,0,0,78,76,1,0,0,0,78,79,1,0,0,0,79,
  	7,1,0,0,0,80,78,1,0,0,0,81,82,7,1,0,0,82,83,3,12,6,0,83,9,1,0,0,0,84,
  	85,7,2,0,0,85,90,3,12,6,0,86,87,5,10,0,0,87,89,3,12,6,0,88,86,1,0,0,0,
  	89,92,1,0,0,0,90,88,1,0,0,0,90,91,1,0,0,0,91,11,1,0,0,0,92,90,1,0,0,0,
  	93,99,5,52,0,0,94,95,5,16,0,0,95,96,5,53,0,0,96,98,5,17,0,0,97,94,1,0,
  	0,0,98,101,1,0,0,0,99,97,1,0,0,0,99,100,1,0,0,0,100,105,1,0,0,0,101,99,
  	1,0,0,0,102,103,5,3,0,0,103,104,5,53,0,0,104,106,5,8,0,0,105,102,1,0,
  	0,0,105,106,1,0,0,0,106,13,1,0,0,0,107,112,3,16,8,0,108,109,5,18,0,0,
  	109,111,3,16,8,0,110,108,1,0,0,0,111,114,1,0,0,0,112,110,1,0,0,0,112,
  	113,1,0,0,0,113,15,1,0,0,0,114,112,1,0,0,0,115,123,3,18,9,0,116,123,3,
  	20,10,0,117,123,3,22,11,0,118,123,3,24,12,0,119,123,3,26,13,0,120,123,
  	3,28,14,0,121,123,3,30,15,0,122,115,1,0,0,0,122,116,1,0,0,0,122,117,1,
  	0,0,0,122,118,1,0,0,0,122,119,1,0,0,0,122,120,1,0,0,0,122,121,1,0,0,0,
  	123,17,1,0,0,0,124,125,7,3,0,0,125,126,5,52,0,0,126,127,5,3,0,0,127,132,
  	5,52,0,0,128,129,5,10,0,0,129,131,5,52,0,0,130,128,1,0,0,0,131,134,1,
  	0,0,0,132,130,1,0,0,0,132,133,1,0,0,0,133,135,1,0,0,0,134,132,1,0,0,0,
  	135,136,5,8,0,0,136,19,1,0,0,0,137,146,5,21,0,0,138,139,5,2,0,0,139,140,
  	5,52,0,0,140,142,5,22,0,0,141,138,1,0,0,0,141,142,1,0,0,0,142,143,1,0,
  	0,0,143,144,3,0,0,0,144,145,5,23,0,0,145,147,1,0,0,0,146,141,1,0,0,0,
  	146,147,1,0,0,0,147,148,1,0,0,0,148,154,3,0,0,0,149,151,5,24,0,0,150,
  	152,5,5,0,0,151,150,1,0,0,0,151,152,1,0,0,0,152,153,1,0,0,0,153,155,3,
  	0,0,0,154,149,1,0,0,0,154,155,1,0,0,0,155,156,1,0,0,0,156,157,3,14,7,
  	0,157,158,5,25,0,0,158,21,1,0,0,0,159,160,5,26,0,0,160,161,3,34,17,0,
  	161,162,5,27,0,0,162,163,3,14,7,0,163,164,5,28,0,0,164,165,3,14,7,0,165,
  	166,5,29,0,0,166,167,3,34,17,0,167,23,1,0,0,0,168,169,7,4,0,0,169,170,
  	5,22,0,0,170,171,3,32,16,0,171,25,1,0,0,0,172,173,3,32,16,0,173,174,7,
  	5,0,0,174,175,5,22,0,0,175,176,3,34,17,0,176,27,1,0,0,0,177,178,3,32,
  	16,0,178,179,5,34,0,0,179,180,3,32,16,0,180,29,1,0,0,0,181,182,5,35,0,
  	0,182,31,1,0,0,0,183,190,5,52,0,0,184,185,5,16,0,0,185,186,3,34,17,0,
  	186,187,5,17,0,0,187,189,1,0,0,0,188,184,1,0,0,0,189,192,1,0,0,0,190,
  	188,1,0,0,0,190,191,1,0,0,0,191,199,1,0,0,0,192,190,1,0,0,0,193,194,5,
  	36,0,0,194,197,3,0,0,0,195,196,5,37,0,0,196,198,3,0,0,0,197,195,1,0,0,
  	0,197,198,1,0,0,0,198,200,1,0,0,0,199,193,1,0,0,0,199,200,1,0,0,0,200,
  	33,1,0,0,0,201,207,3,0,0,0,202,207,3,32,16,0,203,207,3,36,18,0,204,207,
  	3,38,19,0,205,207,3,40,20,0,206,201,1,0,0,0,206,202,1,0,0,0,206,203,1,
  	0,0,0,206,204,1,0,0,0,206,205,1,0,0,0,207,35,1,0,0,0,208,209,5,3,0,0,
  	209,210,3,34,17,0,210,211,7,6,0,0,211,212,3,34,17,0,212,213,5,8,0,0,213,
  	37,1,0,0,0,214,215,7,7,0,0,215,216,3,34,17,0,216,39,1,0,0,0,217,218,5,
  	3,0,0,218,219,3,34,17,0,219,220,7,8,0,0,220,221,3,0,0,0,221,222,5,8,0,
  	0,222,41,1,0,0,0,18,53,58,68,78,90,99,105,112,122,132,141,146,151,154,
  	190,197,199,206
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

tree::TerminalNode* SyReCParser::NumberContext::IDENT() {
  return getToken(SyReCParser::IDENT, 0);
}

std::vector<SyReCParser::NumberContext *> SyReCParser::NumberContext::number() {
  return getRuleContexts<SyReCParser::NumberContext>();
}

SyReCParser::NumberContext* SyReCParser::NumberContext::number(size_t i) {
  return getRuleContext<SyReCParser::NumberContext>(i);
}


size_t SyReCParser::NumberContext::getRuleIndex() const {
  return SyReCParser::RuleNumber;
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

      case SyReCParser::T__0: {
        setState(43);
        match(SyReCParser::T__0);
        setState(44);
        match(SyReCParser::IDENT);
        break;
      }

      case SyReCParser::T__1: {
        setState(45);
        match(SyReCParser::T__1);
        setState(46);
        match(SyReCParser::IDENT);
        break;
      }

      case SyReCParser::T__2: {
        setState(47);
        match(SyReCParser::T__2);
        setState(48);
        number();
        setState(49);
        _la = _input->LA(1);
        if (!(((_la & ~ 0x3fULL) == 0) &&
          ((1ULL << _la) & 240) != 0)) {
        _errHandler->recoverInline(this);
        }
        else {
          _errHandler->reportMatch(this);
          consume();
        }
        setState(50);
        number();
        setState(51);
        match(SyReCParser::T__7);
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
    } while (_la == SyReCParser::T__8);
   
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
    match(SyReCParser::T__8);
    setState(61);
    match(SyReCParser::IDENT);
    setState(62);
    match(SyReCParser::T__2);
    setState(63);
    parameterList();
    setState(64);
    match(SyReCParser::T__7);
    setState(68);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SyReCParser::T__13

    || _la == SyReCParser::T__14) {
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


size_t SyReCParser::ParameterListContext::getRuleIndex() const {
  return SyReCParser::RuleParameterList;
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
    while (_la == SyReCParser::T__9) {
      setState(74);
      match(SyReCParser::T__9);
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


size_t SyReCParser::ParameterContext::getRuleIndex() const {
  return SyReCParser::RuleParameter;
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
      ((1ULL << _la) & 14336) != 0)) {
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


size_t SyReCParser::SignalListContext::getRuleIndex() const {
  return SyReCParser::RuleSignalList;
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
    _la = _input->LA(1);
    if (!(_la == SyReCParser::T__13

    || _la == SyReCParser::T__14)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(85);
    signalDeclaration();
    setState(90);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SyReCParser::T__9) {
      setState(86);
      match(SyReCParser::T__9);
      setState(87);
      signalDeclaration();
      setState(92);
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
    setState(93);
    match(SyReCParser::IDENT);
    setState(99);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SyReCParser::T__15) {
      setState(94);
      match(SyReCParser::T__15);
      setState(95);
      match(SyReCParser::INT);
      setState(96);
      match(SyReCParser::T__16);
      setState(101);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(105);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == SyReCParser::T__2) {
      setState(102);
      match(SyReCParser::T__2);
      setState(103);
      match(SyReCParser::INT);
      setState(104);
      match(SyReCParser::T__7);
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


size_t SyReCParser::StatementListContext::getRuleIndex() const {
  return SyReCParser::RuleStatementList;
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
    setState(107);
    statement();
    setState(112);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SyReCParser::T__17) {
      setState(108);
      match(SyReCParser::T__17);
      setState(109);
      statement();
      setState(114);
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
    setState(122);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 8, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(115);
      callStatement();
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(116);
      forStatement();
      break;
    }

    case 3: {
      enterOuterAlt(_localctx, 3);
      setState(117);
      ifStatement();
      break;
    }

    case 4: {
      enterOuterAlt(_localctx, 4);
      setState(118);
      unaryStatement();
      break;
    }

    case 5: {
      enterOuterAlt(_localctx, 5);
      setState(119);
      assignStatement();
      break;
    }

    case 6: {
      enterOuterAlt(_localctx, 6);
      setState(120);
      swapStatement();
      break;
    }

    case 7: {
      enterOuterAlt(_localctx, 7);
      setState(121);
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


size_t SyReCParser::CallStatementContext::getRuleIndex() const {
  return SyReCParser::RuleCallStatement;
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
    setState(124);
    _la = _input->LA(1);
    if (!(_la == SyReCParser::T__18

    || _la == SyReCParser::T__19)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(125);
    match(SyReCParser::IDENT);
    setState(126);
    match(SyReCParser::T__2);
    setState(127);
    match(SyReCParser::IDENT);
    setState(132);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SyReCParser::T__9) {
      setState(128);
      match(SyReCParser::T__9);
      setState(129);
      match(SyReCParser::IDENT);
      setState(134);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(135);
    match(SyReCParser::T__7);
   
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

std::vector<SyReCParser::NumberContext *> SyReCParser::ForStatementContext::number() {
  return getRuleContexts<SyReCParser::NumberContext>();
}

SyReCParser::NumberContext* SyReCParser::ForStatementContext::number(size_t i) {
  return getRuleContext<SyReCParser::NumberContext>(i);
}

SyReCParser::StatementListContext* SyReCParser::ForStatementContext::statementList() {
  return getRuleContext<SyReCParser::StatementListContext>(0);
}

tree::TerminalNode* SyReCParser::ForStatementContext::IDENT() {
  return getToken(SyReCParser::IDENT, 0);
}


size_t SyReCParser::ForStatementContext::getRuleIndex() const {
  return SyReCParser::RuleForStatement;
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
    setState(137);
    match(SyReCParser::T__20);
    setState(146);
    _errHandler->sync(this);

    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 11, _ctx)) {
    case 1: {
      setState(141);
      _errHandler->sync(this);

      switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 10, _ctx)) {
      case 1: {
        setState(138);
        match(SyReCParser::T__1);
        setState(139);
        match(SyReCParser::IDENT);
        setState(140);
        match(SyReCParser::T__21);
        break;
      }

      default:
        break;
      }
      setState(143);
      number();
      setState(144);
      match(SyReCParser::T__22);
      break;
    }

    default:
      break;
    }
    setState(148);
    number();
    setState(154);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == SyReCParser::T__23) {
      setState(149);
      match(SyReCParser::T__23);
      setState(151);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == SyReCParser::T__4) {
        setState(150);
        match(SyReCParser::T__4);
      }
      setState(153);
      number();
    }
    setState(156);
    statementList();
    setState(157);
    match(SyReCParser::T__24);
   
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
    setState(159);
    match(SyReCParser::T__25);
    setState(160);
    expression();
    setState(161);
    match(SyReCParser::T__26);
    setState(162);
    statementList();
    setState(163);
    match(SyReCParser::T__27);
    setState(164);
    statementList();
    setState(165);
    match(SyReCParser::T__28);
    setState(166);
    expression();
   
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


size_t SyReCParser::UnaryStatementContext::getRuleIndex() const {
  return SyReCParser::RuleUnaryStatement;
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
    setState(168);
    _la = _input->LA(1);
    if (!(((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 7516192768) != 0)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(169);
    match(SyReCParser::T__21);
    setState(170);
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


size_t SyReCParser::AssignStatementContext::getRuleIndex() const {
  return SyReCParser::RuleAssignStatement;
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
    setState(172);
    signal();
    setState(173);
    _la = _input->LA(1);
    if (!(((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 8589934640) != 0)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(174);
    match(SyReCParser::T__21);
    setState(175);
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
    setState(177);
    signal();
    setState(178);
    match(SyReCParser::T__33);
    setState(179);
    signal();
   
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
    setState(181);
    match(SyReCParser::T__34);
   
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
    setState(183);
    match(SyReCParser::IDENT);
    setState(190);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SyReCParser::T__15) {
      setState(184);
      match(SyReCParser::T__15);
      setState(185);
      expression();
      setState(186);
      match(SyReCParser::T__16);
      setState(192);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(199);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == SyReCParser::T__35) {
      setState(193);
      match(SyReCParser::T__35);
      setState(194);
      number();
      setState(197);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == SyReCParser::T__36) {
        setState(195);
        match(SyReCParser::T__36);
        setState(196);
        number();
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
    setState(206);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 17, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(201);
      number();
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(202);
      signal();
      break;
    }

    case 3: {
      enterOuterAlt(_localctx, 3);
      setState(203);
      binaryExpression();
      break;
    }

    case 4: {
      enterOuterAlt(_localctx, 4);
      setState(204);
      unaryExpression();
      break;
    }

    case 5: {
      enterOuterAlt(_localctx, 5);
      setState(205);
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


size_t SyReCParser::BinaryExpressionContext::getRuleIndex() const {
  return SyReCParser::RuleBinaryExpression;
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
    setState(208);
    match(SyReCParser::T__2);
    setState(209);
    expression();
    setState(210);
    _la = _input->LA(1);
    if (!(((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 562683669643504) != 0)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(211);
    expression();
    setState(212);
    match(SyReCParser::T__7);
   
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


size_t SyReCParser::UnaryExpressionContext::getRuleIndex() const {
  return SyReCParser::RuleUnaryExpression;
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
    setState(214);
    _la = _input->LA(1);
    if (!(_la == SyReCParser::T__29

    || _la == SyReCParser::T__48)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(215);
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


size_t SyReCParser::ShiftExpressionContext::getRuleIndex() const {
  return SyReCParser::RuleShiftExpression;
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
    setState(217);
    match(SyReCParser::T__2);
    setState(218);
    expression();
    setState(219);
    _la = _input->LA(1);
    if (!(_la == SyReCParser::T__49

    || _la == SyReCParser::T__50)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(220);
    number();
    setState(221);
    match(SyReCParser::T__7);
   
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
