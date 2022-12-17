#include "core/syrec/program.hpp"

#include "CommonTokenStream.h"
#include "core/syrec/parser/infix_iterator.hpp"
#include "parser/antlr/SyReCCustomVisitor.h"
#include "parser/antlr/SyReCLexer.h"
#include "parser/antlr/SyReCParser.h"
#include "parser/antlr/SyReCErrorListener.h"

using namespace syrec;

std::string program::read(const std::string& filename, const ReadProgramSettings settings) {
    std::size_t fileContentLength = 0;
    auto const*     fileContentBuffer  = readAndBufferFileContent(filename, &fileContentLength);
    if (nullptr == fileContentBuffer) {
        return "Cannot open given circuit file @ " + filename;
    }

    return parseBufferContent(fileContentBuffer, fileContentLength);
}

// TODO: Replace ReadProgramSettings with ParserConfig
std::string program::readFromString(const std::string& circuitStringified, const ReadProgramSettings settings) {
    return parseBufferContent(reinterpret_cast<const unsigned char*>(circuitStringified.c_str()), circuitStringified.size());
}

bool program::readFile(const std::string& filename, const ReadProgramSettings settings, std::string* error) {
    *error = read(filename, settings);
    return error->empty();
}

unsigned char* program::readAndBufferFileContent(const std::string& filename, std::size_t* fileContentLength) {
    unsigned char*                     fileContentBuffer = nullptr;

    std::basic_ifstream<unsigned char> is(filename.c_str(), std::ifstream::in | std::ifstream::binary);
    if (is.is_open()) {
        is.seekg(0, is.end);
        *fileContentLength = is.tellg();
        is.seekg(0, is.beg);
        
        fileContentBuffer = new unsigned char[*fileContentLength];
        is.read(fileContentBuffer, *fileContentLength);
    }
    return fileContentBuffer;
}

// TODO: Added erros from parser to return value
std::string program::parseBufferContent(const unsigned char* buffer, const int bufferSizeInBytes) {
    if (nullptr == buffer) {
        return "Cannot parse invalid buffer";
    }

    const char*               tmp = (char *)(buffer);
    antlr4::ANTLRInputStream  input(tmp, bufferSizeInBytes);
    ::parser::SyReCLexer      lexer(&input);
    antlr4::CommonTokenStream tokens(&lexer);
    ::parser::SyReCParser     antlrParser(&tokens);

    const auto  parserConfig             = std::make_shared<::parser::ParserConfig>();
    const auto& customVisitor            = std::make_unique<::parser::SyReCCustomVisitor>(parserConfig);

    const auto antlrParserErrorListener = std::make_shared<::parser::SyReCErrorListener>(::parser::SyReCErrorListener(customVisitor->errors));
    lexer.addErrorListener(antlrParserErrorListener.get());
    antlrParser.addErrorListener(antlrParserErrorListener.get());

    //const auto x = lexer.getAllTokens();
    if (std::any_cast<bool>(customVisitor->visit(antlrParser.program()))) {
        this->modulesVec = customVisitor->modules;
        return "";
    }

    std::ostringstream errorsConcatinatedBuffer;
    std::copy(customVisitor->errors.cbegin(), customVisitor->errors.cend(), infix_ostream_iterator<std::string>(errorsConcatinatedBuffer, "\n"));
    return errorsConcatinatedBuffer.str();
}

