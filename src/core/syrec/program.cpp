#include "core/syrec/program.hpp"

#include "core/syrec/parser/parser_utilities.hpp"
#include "core/syrec/parser/antlr/parserComponents/syrec_parser_interface.hpp"
#include "core/syrec/parser/optimizations/optimizer.hpp"

using namespace syrec;

std::string program::read(const std::string& filename, const ReadProgramSettings settings) {
    std::size_t fileContentLength = 0;
    auto const*     fileContentBuffer  = readAndBufferFileContent(filename, &fileContentLength);
    if (nullptr == fileContentBuffer) {
        return "Cannot open given circuit file @ " + filename;
    }

    return parseBufferContent(fileContentBuffer, fileContentLength, settings);
}

// TODO: Replace ReadProgramSettings with ParserConfig
std::string program::readFromString(const std::string& circuitStringified, const ReadProgramSettings settings) {
    return parseBufferContent(reinterpret_cast<const unsigned char*>(circuitStringified.c_str()), circuitStringified.size(), settings);
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
std::string program::parseBufferContent(const unsigned char* buffer, const int bufferSizeInBytes, const ReadProgramSettings& config) {
    if (nullptr == buffer) {
        return "Cannot parse invalid buffer";
    }

    const auto& parserConfigToUse = ::parser::ParserConfig(config.defaultBitwidth,
                                                           false,
                                                           false,
                                                           config.reassociateExpressionEnabled,
                                                           config.deadCodeEliminationEnabled,
                                                           config.performConstantPropagation,
                                                           config.noAdditionalLineOptimizationEnabled,
                                                           config.operationStrengthReductionEnabled,
                                                           config.deadStoreEliminationEnabled,
                                                           config.combineRedundantInstructions,
                                                           config.multiplicationSimplificationMethod,
                                                           config.optionalLoopUnrollConfig,
                                                           config.expectedMainModuleName);
    const char*                  bufferCasted = (char *)(buffer);
    const auto  parsingResult     = ::parser::SyrecParserInterface::parseProgram(
                 bufferCasted,
                 bufferSizeInBytes,
                 parserConfigToUse);
    if (parsingResult.wasParsingSuccessful) {
        const auto& optimizer = std::make_unique<optimizations::Optimizer>(parserConfigToUse, nullptr);
        if (auto optimizedProgram = optimizer->optimizeProgram(prepareParsingResultForOptimizations(parsingResult.foundModules)); !optimizedProgram.empty()) {
            this->modulesVec = transformProgramOptimizationResult(std::move(optimizedProgram));   
        } else {
            this->modulesVec = parsingResult.foundModules;    
        }
        return "";
    }

    if (const auto& stringifiedMessages = messageUtils::tryStringifyMessages(parsingResult.errors); stringifiedMessages.has_value()) {
        return *stringifiedMessages;
    }
    // TODO: Syntax errors will be inserted before semantic errors (i.e. the errors are not sorted according to their position)
    return "Failed to combine the found error messages";
}

std::vector<std::reference_wrapper<syrec::Module>> program::prepareParsingResultForOptimizations(const syrec::Module::vec& foundModules) {
    std::vector<std::reference_wrapper<syrec::Module>> transformedModuleReferences;
    for (const auto& foundModule: foundModules) {
        transformedModuleReferences.emplace_back(*foundModule.get());
    }
    return transformedModuleReferences;
}

syrec::Module::vec program::transformProgramOptimizationResult(std::vector<std::unique_ptr<syrec::Module>>&& optimizedProgram) {
    syrec::Module::vec resultContainer(optimizedProgram.size(), nullptr);
    for (std::size_t i = 0; i < optimizedProgram.size(); ++i) {
        resultContainer.at(i) = std::move(optimizedProgram.at(i));
    }
    return resultContainer;
}
