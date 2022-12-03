
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
      "number", "program", "module", "parameterList", "parameter", "parameterType", 
      "signalList", "signalType", "signalDeclaration", "statementList", 
      "statement", "callStatement", "forStatement", "ifStatement", "unaryStatement", 
      "assignStatement", "swapStatement", "skipStatement", "signal", "expression", 
      "binaryExpression", "unaryExpression", "shiftExpression"
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
  	4,1,54,237,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,2,5,7,5,2,6,7,6,2,
  	7,7,7,2,8,7,8,2,9,7,9,2,10,7,10,2,11,7,11,2,12,7,12,2,13,7,13,2,14,7,
  	14,2,15,7,15,2,16,7,16,2,17,7,17,2,18,7,18,2,19,7,19,2,20,7,20,2,21,7,
  	21,2,22,7,22,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,3,0,58,8,0,1,
  	1,4,1,61,8,1,11,1,12,1,62,1,2,1,2,1,2,1,2,1,2,1,2,5,2,71,8,2,10,2,12,
  	2,74,9,2,1,2,1,2,1,3,1,3,1,3,5,3,81,8,3,10,3,12,3,84,9,3,1,4,1,4,1,4,
  	1,5,1,5,1,5,3,5,92,8,5,1,6,1,6,1,6,1,6,5,6,98,8,6,10,6,12,6,101,9,6,1,
  	7,1,7,3,7,105,8,7,1,8,1,8,1,8,1,8,5,8,111,8,8,10,8,12,8,114,9,8,1,8,1,
  	8,1,8,3,8,119,8,8,1,9,1,9,1,9,5,9,124,8,9,10,9,12,9,127,9,9,1,10,1,10,
  	1,10,1,10,1,10,1,10,1,10,3,10,136,8,10,1,11,1,11,1,11,1,11,1,11,1,11,
  	5,11,144,8,11,10,11,12,11,147,9,11,1,11,1,11,1,12,1,12,1,12,1,12,3,12,
  	155,8,12,1,12,1,12,1,12,3,12,160,8,12,1,12,1,12,1,12,3,12,165,8,12,1,
  	12,3,12,168,8,12,1,12,1,12,1,12,1,13,1,13,1,13,1,13,1,13,1,13,1,13,1,
  	13,1,13,1,14,1,14,1,14,1,14,1,15,1,15,1,15,1,15,1,15,1,16,1,16,1,16,1,
  	16,1,17,1,17,1,18,1,18,1,18,1,18,1,18,5,18,202,8,18,10,18,12,18,205,9,
  	18,1,18,1,18,1,18,1,18,3,18,211,8,18,3,18,213,8,18,1,19,1,19,1,19,1,19,
  	1,19,3,19,220,8,19,1,20,1,20,1,20,1,20,1,20,1,20,1,21,1,21,1,21,1,22,
  	1,22,1,22,1,22,1,22,1,22,1,22,0,0,23,0,2,4,6,8,10,12,14,16,18,20,22,24,
  	26,28,30,32,34,36,38,40,42,44,0,7,1,0,4,7,1,0,19,20,1,0,30,32,2,0,4,5,
  	33,33,4,0,4,7,22,22,33,33,38,48,2,0,30,30,49,49,1,0,50,51,244,0,57,1,
  	0,0,0,2,60,1,0,0,0,4,64,1,0,0,0,6,77,1,0,0,0,8,85,1,0,0,0,10,91,1,0,0,
  	0,12,93,1,0,0,0,14,104,1,0,0,0,16,106,1,0,0,0,18,120,1,0,0,0,20,135,1,
  	0,0,0,22,137,1,0,0,0,24,150,1,0,0,0,26,172,1,0,0,0,28,181,1,0,0,0,30,
  	185,1,0,0,0,32,190,1,0,0,0,34,194,1,0,0,0,36,196,1,0,0,0,38,219,1,0,0,
  	0,40,221,1,0,0,0,42,227,1,0,0,0,44,230,1,0,0,0,46,58,5,53,0,0,47,48,5,
  	1,0,0,48,58,5,52,0,0,49,50,5,2,0,0,50,58,5,52,0,0,51,52,5,3,0,0,52,53,
  	3,0,0,0,53,54,7,0,0,0,54,55,3,0,0,0,55,56,5,8,0,0,56,58,1,0,0,0,57,46,
  	1,0,0,0,57,47,1,0,0,0,57,49,1,0,0,0,57,51,1,0,0,0,58,1,1,0,0,0,59,61,
  	3,4,2,0,60,59,1,0,0,0,61,62,1,0,0,0,62,60,1,0,0,0,62,63,1,0,0,0,63,3,
  	1,0,0,0,64,65,5,9,0,0,65,66,5,52,0,0,66,67,5,3,0,0,67,68,3,6,3,0,68,72,
  	5,8,0,0,69,71,3,12,6,0,70,69,1,0,0,0,71,74,1,0,0,0,72,70,1,0,0,0,72,73,
  	1,0,0,0,73,75,1,0,0,0,74,72,1,0,0,0,75,76,3,18,9,0,76,5,1,0,0,0,77,82,
  	3,8,4,0,78,79,5,10,0,0,79,81,3,8,4,0,80,78,1,0,0,0,81,84,1,0,0,0,82,80,
  	1,0,0,0,82,83,1,0,0,0,83,7,1,0,0,0,84,82,1,0,0,0,85,86,3,10,5,0,86,87,
  	3,16,8,0,87,9,1,0,0,0,88,92,5,11,0,0,89,92,5,12,0,0,90,92,5,13,0,0,91,
  	88,1,0,0,0,91,89,1,0,0,0,91,90,1,0,0,0,92,11,1,0,0,0,93,94,3,14,7,0,94,
  	99,3,16,8,0,95,96,5,10,0,0,96,98,3,16,8,0,97,95,1,0,0,0,98,101,1,0,0,
  	0,99,97,1,0,0,0,99,100,1,0,0,0,100,13,1,0,0,0,101,99,1,0,0,0,102,105,
  	5,14,0,0,103,105,5,15,0,0,104,102,1,0,0,0,104,103,1,0,0,0,105,15,1,0,
  	0,0,106,112,5,52,0,0,107,108,5,16,0,0,108,109,5,53,0,0,109,111,5,17,0,
  	0,110,107,1,0,0,0,111,114,1,0,0,0,112,110,1,0,0,0,112,113,1,0,0,0,113,
  	118,1,0,0,0,114,112,1,0,0,0,115,116,5,3,0,0,116,117,5,53,0,0,117,119,
  	5,8,0,0,118,115,1,0,0,0,118,119,1,0,0,0,119,17,1,0,0,0,120,125,3,20,10,
  	0,121,122,5,18,0,0,122,124,3,20,10,0,123,121,1,0,0,0,124,127,1,0,0,0,
  	125,123,1,0,0,0,125,126,1,0,0,0,126,19,1,0,0,0,127,125,1,0,0,0,128,136,
  	3,22,11,0,129,136,3,24,12,0,130,136,3,26,13,0,131,136,3,28,14,0,132,136,
  	3,30,15,0,133,136,3,32,16,0,134,136,3,34,17,0,135,128,1,0,0,0,135,129,
  	1,0,0,0,135,130,1,0,0,0,135,131,1,0,0,0,135,132,1,0,0,0,135,133,1,0,0,
  	0,135,134,1,0,0,0,136,21,1,0,0,0,137,138,7,1,0,0,138,139,5,52,0,0,139,
  	140,5,3,0,0,140,145,5,52,0,0,141,142,5,10,0,0,142,144,5,52,0,0,143,141,
  	1,0,0,0,144,147,1,0,0,0,145,143,1,0,0,0,145,146,1,0,0,0,146,148,1,0,0,
  	0,147,145,1,0,0,0,148,149,5,8,0,0,149,23,1,0,0,0,150,159,5,21,0,0,151,
  	152,5,2,0,0,152,153,5,52,0,0,153,155,5,22,0,0,154,151,1,0,0,0,154,155,
  	1,0,0,0,155,156,1,0,0,0,156,157,3,0,0,0,157,158,5,23,0,0,158,160,1,0,
  	0,0,159,154,1,0,0,0,159,160,1,0,0,0,160,161,1,0,0,0,161,167,3,0,0,0,162,
  	164,5,24,0,0,163,165,5,5,0,0,164,163,1,0,0,0,164,165,1,0,0,0,165,166,
  	1,0,0,0,166,168,3,0,0,0,167,162,1,0,0,0,167,168,1,0,0,0,168,169,1,0,0,
  	0,169,170,3,18,9,0,170,171,5,25,0,0,171,25,1,0,0,0,172,173,5,26,0,0,173,
  	174,3,38,19,0,174,175,5,27,0,0,175,176,3,18,9,0,176,177,5,28,0,0,177,
  	178,3,18,9,0,178,179,5,29,0,0,179,180,3,38,19,0,180,27,1,0,0,0,181,182,
  	7,2,0,0,182,183,5,22,0,0,183,184,3,36,18,0,184,29,1,0,0,0,185,186,3,36,
  	18,0,186,187,7,3,0,0,187,188,5,22,0,0,188,189,3,38,19,0,189,31,1,0,0,
  	0,190,191,3,36,18,0,191,192,5,34,0,0,192,193,3,36,18,0,193,33,1,0,0,0,
  	194,195,5,35,0,0,195,35,1,0,0,0,196,203,5,52,0,0,197,198,5,16,0,0,198,
  	199,3,38,19,0,199,200,5,17,0,0,200,202,1,0,0,0,201,197,1,0,0,0,202,205,
  	1,0,0,0,203,201,1,0,0,0,203,204,1,0,0,0,204,212,1,0,0,0,205,203,1,0,0,
  	0,206,207,5,36,0,0,207,210,3,0,0,0,208,209,5,37,0,0,209,211,3,0,0,0,210,
  	208,1,0,0,0,210,211,1,0,0,0,211,213,1,0,0,0,212,206,1,0,0,0,212,213,1,
  	0,0,0,213,37,1,0,0,0,214,220,3,0,0,0,215,220,3,36,18,0,216,220,3,40,20,
  	0,217,220,3,42,21,0,218,220,3,44,22,0,219,214,1,0,0,0,219,215,1,0,0,0,
  	219,216,1,0,0,0,219,217,1,0,0,0,219,218,1,0,0,0,220,39,1,0,0,0,221,222,
  	5,3,0,0,222,223,3,38,19,0,223,224,7,4,0,0,224,225,3,38,19,0,225,226,5,
  	8,0,0,226,41,1,0,0,0,227,228,7,5,0,0,228,229,3,38,19,0,229,43,1,0,0,0,
  	230,231,5,3,0,0,231,232,3,38,19,0,232,233,7,6,0,0,233,234,3,0,0,0,234,
  	235,5,8,0,0,235,45,1,0,0,0,20,57,62,72,82,91,99,104,112,118,125,135,145,
  	154,159,164,167,203,210,212,219
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
    setState(57);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case SyReCParser::INT: {
        setState(46);
        match(SyReCParser::INT);
        break;
      }

      case SyReCParser::T__0: {
        setState(47);
        match(SyReCParser::T__0);
        setState(48);
        match(SyReCParser::IDENT);
        break;
      }

      case SyReCParser::T__1: {
        setState(49);
        match(SyReCParser::T__1);
        setState(50);
        match(SyReCParser::IDENT);
        break;
      }

      case SyReCParser::T__2: {
        setState(51);
        match(SyReCParser::T__2);
        setState(52);
        number();
        setState(53);
        _la = _input->LA(1);
        if (!(((_la & ~ 0x3fULL) == 0) &&
          ((1ULL << _la) & 240) != 0)) {
        _errHandler->recoverInline(this);
        }
        else {
          _errHandler->reportMatch(this);
          consume();
        }
        setState(54);
        number();
        setState(55);
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
    setState(64);
    match(SyReCParser::T__8);
    setState(65);
    match(SyReCParser::IDENT);
    setState(66);
    match(SyReCParser::T__2);
    setState(67);
    parameterList();
    setState(68);
    match(SyReCParser::T__7);
    setState(72);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SyReCParser::T__13

    || _la == SyReCParser::T__14) {
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
    while (_la == SyReCParser::T__9) {
      setState(78);
      match(SyReCParser::T__9);
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

SyReCParser::ParameterTypeContext* SyReCParser::ParameterContext::parameterType() {
  return getRuleContext<SyReCParser::ParameterTypeContext>(0);
}

SyReCParser::SignalDeclarationContext* SyReCParser::ParameterContext::signalDeclaration() {
  return getRuleContext<SyReCParser::SignalDeclarationContext>(0);
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
    parameterType();
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

//----------------- ParameterTypeContext ------------------------------------------------------------------

SyReCParser::ParameterTypeContext::ParameterTypeContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t SyReCParser::ParameterTypeContext::getRuleIndex() const {
  return SyReCParser::RuleParameterType;
}

void SyReCParser::ParameterTypeContext::copyFrom(ParameterTypeContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- InSignalTypeContext ------------------------------------------------------------------

SyReCParser::InSignalTypeContext::InSignalTypeContext(ParameterTypeContext *ctx) { copyFrom(ctx); }


std::any SyReCParser::InSignalTypeContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SyReCVisitor*>(visitor))
    return parserVisitor->visitInSignalType(this);
  else
    return visitor->visitChildren(this);
}
//----------------- OutSignalTypeContext ------------------------------------------------------------------

SyReCParser::OutSignalTypeContext::OutSignalTypeContext(ParameterTypeContext *ctx) { copyFrom(ctx); }


std::any SyReCParser::OutSignalTypeContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SyReCVisitor*>(visitor))
    return parserVisitor->visitOutSignalType(this);
  else
    return visitor->visitChildren(this);
}
//----------------- InoutSignalTypeContext ------------------------------------------------------------------

SyReCParser::InoutSignalTypeContext::InoutSignalTypeContext(ParameterTypeContext *ctx) { copyFrom(ctx); }


std::any SyReCParser::InoutSignalTypeContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SyReCVisitor*>(visitor))
    return parserVisitor->visitInoutSignalType(this);
  else
    return visitor->visitChildren(this);
}
SyReCParser::ParameterTypeContext* SyReCParser::parameterType() {
  ParameterTypeContext *_localctx = _tracker.createInstance<ParameterTypeContext>(_ctx, getState());
  enterRule(_localctx, 10, SyReCParser::RuleParameterType);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(91);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case SyReCParser::T__10: {
        _localctx = _tracker.createInstance<SyReCParser::InSignalTypeContext>(_localctx);
        enterOuterAlt(_localctx, 1);
        setState(88);
        match(SyReCParser::T__10);
        break;
      }

      case SyReCParser::T__11: {
        _localctx = _tracker.createInstance<SyReCParser::OutSignalTypeContext>(_localctx);
        enterOuterAlt(_localctx, 2);
        setState(89);
        match(SyReCParser::T__11);
        break;
      }

      case SyReCParser::T__12: {
        _localctx = _tracker.createInstance<SyReCParser::InoutSignalTypeContext>(_localctx);
        enterOuterAlt(_localctx, 3);
        setState(90);
        match(SyReCParser::T__12);
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

//----------------- SignalListContext ------------------------------------------------------------------

SyReCParser::SignalListContext::SignalListContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

SyReCParser::SignalTypeContext* SyReCParser::SignalListContext::signalType() {
  return getRuleContext<SyReCParser::SignalTypeContext>(0);
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


std::any SyReCParser::SignalListContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SyReCVisitor*>(visitor))
    return parserVisitor->visitSignalList(this);
  else
    return visitor->visitChildren(this);
}

SyReCParser::SignalListContext* SyReCParser::signalList() {
  SignalListContext *_localctx = _tracker.createInstance<SignalListContext>(_ctx, getState());
  enterRule(_localctx, 12, SyReCParser::RuleSignalList);
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
    signalType();
    setState(94);
    signalDeclaration();
    setState(99);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SyReCParser::T__9) {
      setState(95);
      match(SyReCParser::T__9);
      setState(96);
      signalDeclaration();
      setState(101);
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

//----------------- SignalTypeContext ------------------------------------------------------------------

SyReCParser::SignalTypeContext::SignalTypeContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t SyReCParser::SignalTypeContext::getRuleIndex() const {
  return SyReCParser::RuleSignalType;
}

void SyReCParser::SignalTypeContext::copyFrom(SignalTypeContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- StateSignalTypeContext ------------------------------------------------------------------

SyReCParser::StateSignalTypeContext::StateSignalTypeContext(SignalTypeContext *ctx) { copyFrom(ctx); }


std::any SyReCParser::StateSignalTypeContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SyReCVisitor*>(visitor))
    return parserVisitor->visitStateSignalType(this);
  else
    return visitor->visitChildren(this);
}
//----------------- WireSignalTypeContext ------------------------------------------------------------------

SyReCParser::WireSignalTypeContext::WireSignalTypeContext(SignalTypeContext *ctx) { copyFrom(ctx); }


std::any SyReCParser::WireSignalTypeContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SyReCVisitor*>(visitor))
    return parserVisitor->visitWireSignalType(this);
  else
    return visitor->visitChildren(this);
}
SyReCParser::SignalTypeContext* SyReCParser::signalType() {
  SignalTypeContext *_localctx = _tracker.createInstance<SignalTypeContext>(_ctx, getState());
  enterRule(_localctx, 14, SyReCParser::RuleSignalType);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(104);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case SyReCParser::T__13: {
        _localctx = _tracker.createInstance<SyReCParser::WireSignalTypeContext>(_localctx);
        enterOuterAlt(_localctx, 1);
        setState(102);
        match(SyReCParser::T__13);
        break;
      }

      case SyReCParser::T__14: {
        _localctx = _tracker.createInstance<SyReCParser::StateSignalTypeContext>(_localctx);
        enterOuterAlt(_localctx, 2);
        setState(103);
        match(SyReCParser::T__14);
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
  enterRule(_localctx, 16, SyReCParser::RuleSignalDeclaration);
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
    setState(106);
    match(SyReCParser::IDENT);
    setState(112);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SyReCParser::T__15) {
      setState(107);
      match(SyReCParser::T__15);
      setState(108);
      antlrcpp::downCast<SignalDeclarationContext *>(_localctx)->intToken = match(SyReCParser::INT);
      antlrcpp::downCast<SignalDeclarationContext *>(_localctx)->dimensionTokens.push_back(antlrcpp::downCast<SignalDeclarationContext *>(_localctx)->intToken);
      setState(109);
      match(SyReCParser::T__16);
      setState(114);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(118);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == SyReCParser::T__2) {
      setState(115);
      match(SyReCParser::T__2);
      setState(116);
      antlrcpp::downCast<SignalDeclarationContext *>(_localctx)->signalWidthToken = match(SyReCParser::INT);
      setState(117);
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


std::any SyReCParser::StatementListContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SyReCVisitor*>(visitor))
    return parserVisitor->visitStatementList(this);
  else
    return visitor->visitChildren(this);
}

SyReCParser::StatementListContext* SyReCParser::statementList() {
  StatementListContext *_localctx = _tracker.createInstance<StatementListContext>(_ctx, getState());
  enterRule(_localctx, 18, SyReCParser::RuleStatementList);
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
    setState(120);
    statement();
    setState(125);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SyReCParser::T__17) {
      setState(121);
      match(SyReCParser::T__17);
      setState(122);
      statement();
      setState(127);
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
  enterRule(_localctx, 20, SyReCParser::RuleStatement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(135);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 10, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(128);
      callStatement();
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(129);
      forStatement();
      break;
    }

    case 3: {
      enterOuterAlt(_localctx, 3);
      setState(130);
      ifStatement();
      break;
    }

    case 4: {
      enterOuterAlt(_localctx, 4);
      setState(131);
      unaryStatement();
      break;
    }

    case 5: {
      enterOuterAlt(_localctx, 5);
      setState(132);
      assignStatement();
      break;
    }

    case 6: {
      enterOuterAlt(_localctx, 6);
      setState(133);
      swapStatement();
      break;
    }

    case 7: {
      enterOuterAlt(_localctx, 7);
      setState(134);
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


std::any SyReCParser::CallStatementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SyReCVisitor*>(visitor))
    return parserVisitor->visitCallStatement(this);
  else
    return visitor->visitChildren(this);
}

SyReCParser::CallStatementContext* SyReCParser::callStatement() {
  CallStatementContext *_localctx = _tracker.createInstance<CallStatementContext>(_ctx, getState());
  enterRule(_localctx, 22, SyReCParser::RuleCallStatement);
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
    _la = _input->LA(1);
    if (!(_la == SyReCParser::T__18

    || _la == SyReCParser::T__19)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(138);
    match(SyReCParser::IDENT);
    setState(139);
    match(SyReCParser::T__2);
    setState(140);
    match(SyReCParser::IDENT);
    setState(145);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SyReCParser::T__9) {
      setState(141);
      match(SyReCParser::T__9);
      setState(142);
      match(SyReCParser::IDENT);
      setState(147);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(148);
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
    setState(150);
    match(SyReCParser::T__20);
    setState(159);
    _errHandler->sync(this);

    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 13, _ctx)) {
    case 1: {
      setState(154);
      _errHandler->sync(this);

      switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 12, _ctx)) {
      case 1: {
        setState(151);
        match(SyReCParser::T__1);
        setState(152);
        match(SyReCParser::IDENT);
        setState(153);
        match(SyReCParser::T__21);
        break;
      }

      default:
        break;
      }
      setState(156);
      number();
      setState(157);
      match(SyReCParser::T__22);
      break;
    }

    default:
      break;
    }
    setState(161);
    number();
    setState(167);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == SyReCParser::T__23) {
      setState(162);
      match(SyReCParser::T__23);
      setState(164);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == SyReCParser::T__4) {
        setState(163);
        match(SyReCParser::T__4);
      }
      setState(166);
      number();
    }
    setState(169);
    statementList();
    setState(170);
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
    match(SyReCParser::T__25);
    setState(173);
    expression();
    setState(174);
    match(SyReCParser::T__26);
    setState(175);
    statementList();
    setState(176);
    match(SyReCParser::T__27);
    setState(177);
    statementList();
    setState(178);
    match(SyReCParser::T__28);
    setState(179);
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
    _la = _input->LA(1);
    if (!(((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 7516192768) != 0)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(182);
    match(SyReCParser::T__21);
    setState(183);
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
    setState(185);
    signal();
    setState(186);
    _la = _input->LA(1);
    if (!(((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 8589934640) != 0)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(187);
    match(SyReCParser::T__21);
    setState(188);
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
    setState(190);
    signal();
    setState(191);
    match(SyReCParser::T__33);
    setState(192);
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
    setState(194);
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
    setState(196);
    match(SyReCParser::IDENT);
    setState(203);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SyReCParser::T__15) {
      setState(197);
      match(SyReCParser::T__15);
      setState(198);
      expression();
      setState(199);
      match(SyReCParser::T__16);
      setState(205);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(212);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == SyReCParser::T__35) {
      setState(206);
      match(SyReCParser::T__35);
      setState(207);
      number();
      setState(210);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == SyReCParser::T__36) {
        setState(208);
        match(SyReCParser::T__36);
        setState(209);
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


std::any SyReCParser::ExpressionContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SyReCVisitor*>(visitor))
    return parserVisitor->visitExpression(this);
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
    setState(219);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 19, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(214);
      number();
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(215);
      signal();
      break;
    }

    case 3: {
      enterOuterAlt(_localctx, 3);
      setState(216);
      binaryExpression();
      break;
    }

    case 4: {
      enterOuterAlt(_localctx, 4);
      setState(217);
      unaryExpression();
      break;
    }

    case 5: {
      enterOuterAlt(_localctx, 5);
      setState(218);
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
    setState(221);
    match(SyReCParser::T__2);
    setState(222);
    expression();
    setState(223);
    _la = _input->LA(1);
    if (!(((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 562683669643504) != 0)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(224);
    expression();
    setState(225);
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
    setState(227);
    _la = _input->LA(1);
    if (!(_la == SyReCParser::T__29

    || _la == SyReCParser::T__48)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(228);
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
    setState(230);
    match(SyReCParser::T__2);
    setState(231);
    expression();
    setState(232);
    _la = _input->LA(1);
    if (!(_la == SyReCParser::T__49

    || _la == SyReCParser::T__50)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(233);
    number();
    setState(234);
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
