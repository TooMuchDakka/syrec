
// Generated from ..\syrec_antlr\src\core\syrec\parser\antlr\SyReC.g4 by ANTLR 4.11.1


#include "SyReCLexer.h"


using namespace antlr4;

using namespace parser;


using namespace antlr4;

namespace {

struct SyReCLexerStaticData final {
  SyReCLexerStaticData(std::vector<std::string> ruleNames,
                          std::vector<std::string> channelNames,
                          std::vector<std::string> modeNames,
                          std::vector<std::string> literalNames,
                          std::vector<std::string> symbolicNames)
      : ruleNames(std::move(ruleNames)), channelNames(std::move(channelNames)),
        modeNames(std::move(modeNames)), literalNames(std::move(literalNames)),
        symbolicNames(std::move(symbolicNames)),
        vocabulary(this->literalNames, this->symbolicNames) {}

  SyReCLexerStaticData(const SyReCLexerStaticData&) = delete;
  SyReCLexerStaticData(SyReCLexerStaticData&&) = delete;
  SyReCLexerStaticData& operator=(const SyReCLexerStaticData&) = delete;
  SyReCLexerStaticData& operator=(SyReCLexerStaticData&&) = delete;

  std::vector<antlr4::dfa::DFA> decisionToDFA;
  antlr4::atn::PredictionContextCache sharedContextCache;
  const std::vector<std::string> ruleNames;
  const std::vector<std::string> channelNames;
  const std::vector<std::string> modeNames;
  const std::vector<std::string> literalNames;
  const std::vector<std::string> symbolicNames;
  const antlr4::dfa::Vocabulary vocabulary;
  antlr4::atn::SerializedATNView serializedATN;
  std::unique_ptr<antlr4::atn::ATN> atn;
};

::antlr4::internal::OnceFlag syreclexerLexerOnceFlag;
SyReCLexerStaticData *syreclexerLexerStaticData = nullptr;

void syreclexerLexerInitialize() {
  assert(syreclexerLexerStaticData == nullptr);
  auto staticData = std::make_unique<SyReCLexerStaticData>(
    std::vector<std::string>{
      "T__0", "T__1", "T__2", "T__3", "T__4", "T__5", "T__6", "T__7", "T__8", 
      "T__9", "T__10", "T__11", "T__12", "T__13", "T__14", "T__15", "T__16", 
      "T__17", "OP_PLUS", "OP_MINUS", "OP_MULTIPLY", "OP_UPPER_BIT_MULTIPLY", 
      "OP_DIVISION", "OP_MODULO", "OP_GREATER_THAN", "OP_GREATER_OR_EQUAL", 
      "OP_LESS_THAN", "OP_LESS_OR_EQUAL", "OP_EQUAL", "OP_NOT_EQUAL", "OP_BITWISE_AND", 
      "OP_BITWISE_NEGATION", "OP_BITWISE_OR", "OP_BITWISE_XOR", "OP_LOGICAL_AND", 
      "OP_LOGICAL_OR", "OP_LOGICAL_NEGATION", "OP_LEFT_SHIFT", "OP_RIGHT_SHIFT", 
      "OP_INCREMENT_ASSIGN", "OP_DECREMENT_ASSIGN", "OP_INVERT_ASSIGN", 
      "OP_ADD_ASSIGN", "OP_SUB_ASSIGN", "OP_XOR_ASSIGN", "OP_CALL", "OP_UNCALL", 
      "VAR_TYPE_IN", "VAR_TYPE_OUT", "VAR_TYPE_INOUT", "VAR_TYPE_WIRE", 
      "VAR_TYPE_STATE", "LOOP_VARIABLE_PREFIX", "SIGNAL_WIDTH_PREFIX", "STATEMENT_DELIMITER", 
      "PARAMETER_DELIMITER", "LETTER", "DIGIT", "IDENT", "INT", "SKIPABLEWHITSPACES"
    },
    std::vector<std::string>{
      "DEFAULT_TOKEN_CHANNEL", "HIDDEN"
    },
    std::vector<std::string>{
      "DEFAULT_MODE"
    },
    std::vector<std::string>{
      "", "'('", "')'", "'module'", "'['", "']'", "'step'", "'for'", "'to'", 
      "'do'", "'rof'", "'if'", "'then'", "'else'", "'fi'", "'<=>'", "'skip'", 
      "'.'", "':'", "'+'", "'-'", "'*'", "'*>'", "'/'", "'%'", "'>'", "'>='", 
      "'<'", "'<='", "'='", "'!='", "'&'", "'~'", "'|'", "'^'", "'&&'", 
      "'||'", "'!'", "'<<'", "'>>'", "'++='", "'--='", "'~='", "'+='", "'-='", 
      "'^='", "'call'", "'uncall'", "'in'", "'out'", "'inout'", "'wire'", 
      "'state'", "'$'", "'#'", "';'", "','"
    },
    std::vector<std::string>{
      "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", 
      "", "", "OP_PLUS", "OP_MINUS", "OP_MULTIPLY", "OP_UPPER_BIT_MULTIPLY", 
      "OP_DIVISION", "OP_MODULO", "OP_GREATER_THAN", "OP_GREATER_OR_EQUAL", 
      "OP_LESS_THAN", "OP_LESS_OR_EQUAL", "OP_EQUAL", "OP_NOT_EQUAL", "OP_BITWISE_AND", 
      "OP_BITWISE_NEGATION", "OP_BITWISE_OR", "OP_BITWISE_XOR", "OP_LOGICAL_AND", 
      "OP_LOGICAL_OR", "OP_LOGICAL_NEGATION", "OP_LEFT_SHIFT", "OP_RIGHT_SHIFT", 
      "OP_INCREMENT_ASSIGN", "OP_DECREMENT_ASSIGN", "OP_INVERT_ASSIGN", 
      "OP_ADD_ASSIGN", "OP_SUB_ASSIGN", "OP_XOR_ASSIGN", "OP_CALL", "OP_UNCALL", 
      "VAR_TYPE_IN", "VAR_TYPE_OUT", "VAR_TYPE_INOUT", "VAR_TYPE_WIRE", 
      "VAR_TYPE_STATE", "LOOP_VARIABLE_PREFIX", "SIGNAL_WIDTH_PREFIX", "STATEMENT_DELIMITER", 
      "PARAMETER_DELIMITER", "IDENT", "INT", "SKIPABLEWHITSPACES"
    }
  );
  static const int32_t serializedATNSegment[] = {
  	4,0,59,328,6,-1,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,2,5,7,5,2,6,7,
  	6,2,7,7,7,2,8,7,8,2,9,7,9,2,10,7,10,2,11,7,11,2,12,7,12,2,13,7,13,2,14,
  	7,14,2,15,7,15,2,16,7,16,2,17,7,17,2,18,7,18,2,19,7,19,2,20,7,20,2,21,
  	7,21,2,22,7,22,2,23,7,23,2,24,7,24,2,25,7,25,2,26,7,26,2,27,7,27,2,28,
  	7,28,2,29,7,29,2,30,7,30,2,31,7,31,2,32,7,32,2,33,7,33,2,34,7,34,2,35,
  	7,35,2,36,7,36,2,37,7,37,2,38,7,38,2,39,7,39,2,40,7,40,2,41,7,41,2,42,
  	7,42,2,43,7,43,2,44,7,44,2,45,7,45,2,46,7,46,2,47,7,47,2,48,7,48,2,49,
  	7,49,2,50,7,50,2,51,7,51,2,52,7,52,2,53,7,53,2,54,7,54,2,55,7,55,2,56,
  	7,56,2,57,7,57,2,58,7,58,2,59,7,59,2,60,7,60,1,0,1,0,1,1,1,1,1,2,1,2,
  	1,2,1,2,1,2,1,2,1,2,1,3,1,3,1,4,1,4,1,5,1,5,1,5,1,5,1,5,1,6,1,6,1,6,1,
  	6,1,7,1,7,1,7,1,8,1,8,1,8,1,9,1,9,1,9,1,9,1,10,1,10,1,10,1,11,1,11,1,
  	11,1,11,1,11,1,12,1,12,1,12,1,12,1,12,1,13,1,13,1,13,1,14,1,14,1,14,1,
  	14,1,15,1,15,1,15,1,15,1,15,1,16,1,16,1,17,1,17,1,18,1,18,1,19,1,19,1,
  	20,1,20,1,21,1,21,1,21,1,22,1,22,1,23,1,23,1,24,1,24,1,25,1,25,1,25,1,
  	26,1,26,1,27,1,27,1,27,1,28,1,28,1,29,1,29,1,29,1,30,1,30,1,31,1,31,1,
  	32,1,32,1,33,1,33,1,34,1,34,1,34,1,35,1,35,1,35,1,36,1,36,1,37,1,37,1,
  	37,1,38,1,38,1,38,1,39,1,39,1,39,1,39,1,40,1,40,1,40,1,40,1,41,1,41,1,
  	41,1,42,1,42,1,42,1,43,1,43,1,43,1,44,1,44,1,44,1,45,1,45,1,45,1,45,1,
  	45,1,46,1,46,1,46,1,46,1,46,1,46,1,46,1,47,1,47,1,47,1,48,1,48,1,48,1,
  	48,1,49,1,49,1,49,1,49,1,49,1,49,1,50,1,50,1,50,1,50,1,50,1,51,1,51,1,
  	51,1,51,1,51,1,51,1,52,1,52,1,53,1,53,1,54,1,54,1,55,1,55,1,56,1,56,1,
  	57,1,57,1,58,1,58,3,58,307,8,58,1,58,1,58,1,58,5,58,312,8,58,10,58,12,
  	58,315,9,58,1,59,4,59,318,8,59,11,59,12,59,319,1,60,4,60,323,8,60,11,
  	60,12,60,324,1,60,1,60,0,0,61,1,1,3,2,5,3,7,4,9,5,11,6,13,7,15,8,17,9,
  	19,10,21,11,23,12,25,13,27,14,29,15,31,16,33,17,35,18,37,19,39,20,41,
  	21,43,22,45,23,47,24,49,25,51,26,53,27,55,28,57,29,59,30,61,31,63,32,
  	65,33,67,34,69,35,71,36,73,37,75,38,77,39,79,40,81,41,83,42,85,43,87,
  	44,89,45,91,46,93,47,95,48,97,49,99,50,101,51,103,52,105,53,107,54,109,
  	55,111,56,113,0,115,0,117,57,119,58,121,59,1,0,2,2,0,65,90,97,122,3,0,
  	9,10,13,13,32,32,331,0,1,1,0,0,0,0,3,1,0,0,0,0,5,1,0,0,0,0,7,1,0,0,0,
  	0,9,1,0,0,0,0,11,1,0,0,0,0,13,1,0,0,0,0,15,1,0,0,0,0,17,1,0,0,0,0,19,
  	1,0,0,0,0,21,1,0,0,0,0,23,1,0,0,0,0,25,1,0,0,0,0,27,1,0,0,0,0,29,1,0,
  	0,0,0,31,1,0,0,0,0,33,1,0,0,0,0,35,1,0,0,0,0,37,1,0,0,0,0,39,1,0,0,0,
  	0,41,1,0,0,0,0,43,1,0,0,0,0,45,1,0,0,0,0,47,1,0,0,0,0,49,1,0,0,0,0,51,
  	1,0,0,0,0,53,1,0,0,0,0,55,1,0,0,0,0,57,1,0,0,0,0,59,1,0,0,0,0,61,1,0,
  	0,0,0,63,1,0,0,0,0,65,1,0,0,0,0,67,1,0,0,0,0,69,1,0,0,0,0,71,1,0,0,0,
  	0,73,1,0,0,0,0,75,1,0,0,0,0,77,1,0,0,0,0,79,1,0,0,0,0,81,1,0,0,0,0,83,
  	1,0,0,0,0,85,1,0,0,0,0,87,1,0,0,0,0,89,1,0,0,0,0,91,1,0,0,0,0,93,1,0,
  	0,0,0,95,1,0,0,0,0,97,1,0,0,0,0,99,1,0,0,0,0,101,1,0,0,0,0,103,1,0,0,
  	0,0,105,1,0,0,0,0,107,1,0,0,0,0,109,1,0,0,0,0,111,1,0,0,0,0,117,1,0,0,
  	0,0,119,1,0,0,0,0,121,1,0,0,0,1,123,1,0,0,0,3,125,1,0,0,0,5,127,1,0,0,
  	0,7,134,1,0,0,0,9,136,1,0,0,0,11,138,1,0,0,0,13,143,1,0,0,0,15,147,1,
  	0,0,0,17,150,1,0,0,0,19,153,1,0,0,0,21,157,1,0,0,0,23,160,1,0,0,0,25,
  	165,1,0,0,0,27,170,1,0,0,0,29,173,1,0,0,0,31,177,1,0,0,0,33,182,1,0,0,
  	0,35,184,1,0,0,0,37,186,1,0,0,0,39,188,1,0,0,0,41,190,1,0,0,0,43,192,
  	1,0,0,0,45,195,1,0,0,0,47,197,1,0,0,0,49,199,1,0,0,0,51,201,1,0,0,0,53,
  	204,1,0,0,0,55,206,1,0,0,0,57,209,1,0,0,0,59,211,1,0,0,0,61,214,1,0,0,
  	0,63,216,1,0,0,0,65,218,1,0,0,0,67,220,1,0,0,0,69,222,1,0,0,0,71,225,
  	1,0,0,0,73,228,1,0,0,0,75,230,1,0,0,0,77,233,1,0,0,0,79,236,1,0,0,0,81,
  	240,1,0,0,0,83,244,1,0,0,0,85,247,1,0,0,0,87,250,1,0,0,0,89,253,1,0,0,
  	0,91,256,1,0,0,0,93,261,1,0,0,0,95,268,1,0,0,0,97,271,1,0,0,0,99,275,
  	1,0,0,0,101,281,1,0,0,0,103,286,1,0,0,0,105,292,1,0,0,0,107,294,1,0,0,
  	0,109,296,1,0,0,0,111,298,1,0,0,0,113,300,1,0,0,0,115,302,1,0,0,0,117,
  	306,1,0,0,0,119,317,1,0,0,0,121,322,1,0,0,0,123,124,5,40,0,0,124,2,1,
  	0,0,0,125,126,5,41,0,0,126,4,1,0,0,0,127,128,5,109,0,0,128,129,5,111,
  	0,0,129,130,5,100,0,0,130,131,5,117,0,0,131,132,5,108,0,0,132,133,5,101,
  	0,0,133,6,1,0,0,0,134,135,5,91,0,0,135,8,1,0,0,0,136,137,5,93,0,0,137,
  	10,1,0,0,0,138,139,5,115,0,0,139,140,5,116,0,0,140,141,5,101,0,0,141,
  	142,5,112,0,0,142,12,1,0,0,0,143,144,5,102,0,0,144,145,5,111,0,0,145,
  	146,5,114,0,0,146,14,1,0,0,0,147,148,5,116,0,0,148,149,5,111,0,0,149,
  	16,1,0,0,0,150,151,5,100,0,0,151,152,5,111,0,0,152,18,1,0,0,0,153,154,
  	5,114,0,0,154,155,5,111,0,0,155,156,5,102,0,0,156,20,1,0,0,0,157,158,
  	5,105,0,0,158,159,5,102,0,0,159,22,1,0,0,0,160,161,5,116,0,0,161,162,
  	5,104,0,0,162,163,5,101,0,0,163,164,5,110,0,0,164,24,1,0,0,0,165,166,
  	5,101,0,0,166,167,5,108,0,0,167,168,5,115,0,0,168,169,5,101,0,0,169,26,
  	1,0,0,0,170,171,5,102,0,0,171,172,5,105,0,0,172,28,1,0,0,0,173,174,5,
  	60,0,0,174,175,5,61,0,0,175,176,5,62,0,0,176,30,1,0,0,0,177,178,5,115,
  	0,0,178,179,5,107,0,0,179,180,5,105,0,0,180,181,5,112,0,0,181,32,1,0,
  	0,0,182,183,5,46,0,0,183,34,1,0,0,0,184,185,5,58,0,0,185,36,1,0,0,0,186,
  	187,5,43,0,0,187,38,1,0,0,0,188,189,5,45,0,0,189,40,1,0,0,0,190,191,5,
  	42,0,0,191,42,1,0,0,0,192,193,5,42,0,0,193,194,5,62,0,0,194,44,1,0,0,
  	0,195,196,5,47,0,0,196,46,1,0,0,0,197,198,5,37,0,0,198,48,1,0,0,0,199,
  	200,5,62,0,0,200,50,1,0,0,0,201,202,5,62,0,0,202,203,5,61,0,0,203,52,
  	1,0,0,0,204,205,5,60,0,0,205,54,1,0,0,0,206,207,5,60,0,0,207,208,5,61,
  	0,0,208,56,1,0,0,0,209,210,5,61,0,0,210,58,1,0,0,0,211,212,5,33,0,0,212,
  	213,5,61,0,0,213,60,1,0,0,0,214,215,5,38,0,0,215,62,1,0,0,0,216,217,5,
  	126,0,0,217,64,1,0,0,0,218,219,5,124,0,0,219,66,1,0,0,0,220,221,5,94,
  	0,0,221,68,1,0,0,0,222,223,5,38,0,0,223,224,5,38,0,0,224,70,1,0,0,0,225,
  	226,5,124,0,0,226,227,5,124,0,0,227,72,1,0,0,0,228,229,5,33,0,0,229,74,
  	1,0,0,0,230,231,5,60,0,0,231,232,5,60,0,0,232,76,1,0,0,0,233,234,5,62,
  	0,0,234,235,5,62,0,0,235,78,1,0,0,0,236,237,5,43,0,0,237,238,5,43,0,0,
  	238,239,5,61,0,0,239,80,1,0,0,0,240,241,5,45,0,0,241,242,5,45,0,0,242,
  	243,5,61,0,0,243,82,1,0,0,0,244,245,5,126,0,0,245,246,5,61,0,0,246,84,
  	1,0,0,0,247,248,5,43,0,0,248,249,5,61,0,0,249,86,1,0,0,0,250,251,5,45,
  	0,0,251,252,5,61,0,0,252,88,1,0,0,0,253,254,5,94,0,0,254,255,5,61,0,0,
  	255,90,1,0,0,0,256,257,5,99,0,0,257,258,5,97,0,0,258,259,5,108,0,0,259,
  	260,5,108,0,0,260,92,1,0,0,0,261,262,5,117,0,0,262,263,5,110,0,0,263,
  	264,5,99,0,0,264,265,5,97,0,0,265,266,5,108,0,0,266,267,5,108,0,0,267,
  	94,1,0,0,0,268,269,5,105,0,0,269,270,5,110,0,0,270,96,1,0,0,0,271,272,
  	5,111,0,0,272,273,5,117,0,0,273,274,5,116,0,0,274,98,1,0,0,0,275,276,
  	5,105,0,0,276,277,5,110,0,0,277,278,5,111,0,0,278,279,5,117,0,0,279,280,
  	5,116,0,0,280,100,1,0,0,0,281,282,5,119,0,0,282,283,5,105,0,0,283,284,
  	5,114,0,0,284,285,5,101,0,0,285,102,1,0,0,0,286,287,5,115,0,0,287,288,
  	5,116,0,0,288,289,5,97,0,0,289,290,5,116,0,0,290,291,5,101,0,0,291,104,
  	1,0,0,0,292,293,5,36,0,0,293,106,1,0,0,0,294,295,5,35,0,0,295,108,1,0,
  	0,0,296,297,5,59,0,0,297,110,1,0,0,0,298,299,5,44,0,0,299,112,1,0,0,0,
  	300,301,7,0,0,0,301,114,1,0,0,0,302,303,2,48,57,0,303,116,1,0,0,0,304,
  	307,5,95,0,0,305,307,3,113,56,0,306,304,1,0,0,0,306,305,1,0,0,0,307,313,
  	1,0,0,0,308,312,5,95,0,0,309,312,3,113,56,0,310,312,3,115,57,0,311,308,
  	1,0,0,0,311,309,1,0,0,0,311,310,1,0,0,0,312,315,1,0,0,0,313,311,1,0,0,
  	0,313,314,1,0,0,0,314,118,1,0,0,0,315,313,1,0,0,0,316,318,3,115,57,0,
  	317,316,1,0,0,0,318,319,1,0,0,0,319,317,1,0,0,0,319,320,1,0,0,0,320,120,
  	1,0,0,0,321,323,7,1,0,0,322,321,1,0,0,0,323,324,1,0,0,0,324,322,1,0,0,
  	0,324,325,1,0,0,0,325,326,1,0,0,0,326,327,6,60,0,0,327,122,1,0,0,0,6,
  	0,306,311,313,319,324,1,6,0,0
  };
  staticData->serializedATN = antlr4::atn::SerializedATNView(serializedATNSegment, sizeof(serializedATNSegment) / sizeof(serializedATNSegment[0]));

  antlr4::atn::ATNDeserializer deserializer;
  staticData->atn = deserializer.deserialize(staticData->serializedATN);

  const size_t count = staticData->atn->getNumberOfDecisions();
  staticData->decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    staticData->decisionToDFA.emplace_back(staticData->atn->getDecisionState(i), i);
  }
  syreclexerLexerStaticData = staticData.release();
}

}

SyReCLexer::SyReCLexer(CharStream *input) : Lexer(input) {
  SyReCLexer::initialize();
  _interpreter = new atn::LexerATNSimulator(this, *syreclexerLexerStaticData->atn, syreclexerLexerStaticData->decisionToDFA, syreclexerLexerStaticData->sharedContextCache);
}

SyReCLexer::~SyReCLexer() {
  delete _interpreter;
}

std::string SyReCLexer::getGrammarFileName() const {
  return "SyReC.g4";
}

const std::vector<std::string>& SyReCLexer::getRuleNames() const {
  return syreclexerLexerStaticData->ruleNames;
}

const std::vector<std::string>& SyReCLexer::getChannelNames() const {
  return syreclexerLexerStaticData->channelNames;
}

const std::vector<std::string>& SyReCLexer::getModeNames() const {
  return syreclexerLexerStaticData->modeNames;
}

const dfa::Vocabulary& SyReCLexer::getVocabulary() const {
  return syreclexerLexerStaticData->vocabulary;
}

antlr4::atn::SerializedATNView SyReCLexer::getSerializedATN() const {
  return syreclexerLexerStaticData->serializedATN;
}

const atn::ATN& SyReCLexer::getATN() const {
  return *syreclexerLexerStaticData->atn;
}




void SyReCLexer::initialize() {
  ::antlr4::internal::call_once(syreclexerLexerOnceFlag, syreclexerLexerInitialize);
}
