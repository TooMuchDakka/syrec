#include "core/syrec/program.hpp"

#include "core/syrec/parser/antlr/parserComponents/syrec_parser_interface.hpp"
#include "core/syrec/parser/optimizations/optimizer.hpp"

using namespace syrec;

std::string program::read(const std::string& filename, const ReadProgramSettings settings) {
    std::string foundErrorBuffer;
    const auto& readFileContent = tryReadFileContent(filename, &foundErrorBuffer);
    if (!foundErrorBuffer.empty() || !readFileContent.has_value()) {
        return foundErrorBuffer;
    }
    return readFromString(*readFileContent, settings);
}

// TODO: Replace ReadProgramSettings with ParserConfig
std::string program::readFromString(const std::string& circuitStringified, const ReadProgramSettings settings) {
    std::string foundErrorBuffer;
    parseFileContent(circuitStringified, settings, &foundErrorBuffer);
    return foundErrorBuffer;
}

std::optional<std::string> program::tryReadFileContent(const std::string_view& fileName, std::string* foundFileHandlingErrors) {
    if (std::ifstream inputFileStream(fileName.data(), std::ifstream::in | std::ifstream::binary); inputFileStream.is_open()) {
        inputFileStream.ignore(std::numeric_limits<std::streamsize>::max());
        const auto fileContentLength = inputFileStream.gcount();
        inputFileStream.clear(); //  Since ignore will have set eof.
        inputFileStream.seekg(0, std::ios_base::beg);

        std::string fileContentBuffer(fileContentLength, ' ');
        inputFileStream.read(fileContentBuffer.data(), fileContentLength);
        if (inputFileStream.bad()) {
            if (foundFileHandlingErrors) {
                *foundFileHandlingErrors = "Error while reading content from file @ " + std::string(fileName);
            }
            return std::nullopt;
        }
        return std::make_optional(fileContentBuffer);
    }

    if (foundFileHandlingErrors) {
        *foundFileHandlingErrors = "Cannot open given circuit file @ " + std::string(fileName);
    }
    return std::nullopt;
}


bool program::parseFileContent(std::string_view programToBeParsed, const ReadProgramSettings& config, std::string* foundErrors) {
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
    const auto  parsingResult     = ::parser::SyrecParserInterface::parseProgram(programToBeParsed, parserConfigToUse);
    if (parsingResult.wasParsingSuccessful) {
        const auto& optionalUserDefinedMainModuleName = config.expectedMainModuleName.empty() ? std::nullopt : std::make_optional(config.expectedMainModuleName);
        const auto& optimizer                         = std::make_unique<optimizations::Optimizer>(parserConfigToUse, nullptr);
        auto        optimizationResultOfProgram       = optimizer->optimizeProgram(prepareParsingResultForOptimizations(parsingResult.foundModules), optionalUserDefinedMainModuleName);
        if (optimizationResultOfProgram.getStatusOfResult() == optimizations::Optimizer::IsUnchanged) {
            this->modulesVec = parsingResult.foundModules;
        } else if (optimizationResultOfProgram.getStatusOfResult() == optimizations::Optimizer::WasOptimized) {
            auto&& optimizedModules = optimizationResultOfProgram.tryTakeOwnershipOfOptimizationResult();
            if (!optimizedModules.has_value()) {
                // TODO: This should not happen
                return "";
            }

            this->modulesVec.reserve(optimizedModules->size());
            std::move(optimizedModules->begin(), optimizedModules->end(), std::back_inserter(this->modulesVec));
        } else {
            // TODO: What should happen in this case
            this->modulesVec = parsingResult.foundModules;
        }
        return true;
    }

    const auto& concatinatedFoundErrors = messageUtils::tryStringifyMessages(parsingResult.errors);
    if (foundErrors && concatinatedFoundErrors.has_value()) {
        *foundErrors = *concatinatedFoundErrors;
    }
    return false;
}


bool program::readFile(const std::string& filename, const ReadProgramSettings settings, std::string* error) {
    *error = read(filename, settings);
    return error->empty();
}

std::vector<std::reference_wrapper<const syrec::Module>> program::prepareParsingResultForOptimizations(const syrec::Module::vec& foundModules) {
    std::vector<std::reference_wrapper<const syrec::Module>> transformedModuleReferences;
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
