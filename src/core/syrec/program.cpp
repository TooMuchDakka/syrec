#include "core/syrec/program.hpp"

#include "core/syrec/parser/antlr/parserComponents/syrec_parser_interface.hpp"
#include "core/syrec/parser/optimizations/optimizer.hpp"

using namespace syrec;

std::string Program::read(const std::string& filename, const ReadProgramSettings settings) {
    std::string foundErrorBuffer;
    const auto& readFileContent = tryReadFileContent(filename, &foundErrorBuffer);
    if (!foundErrorBuffer.empty() || !readFileContent.has_value()) {
        return foundErrorBuffer;
    }
    return readFromString(*readFileContent, settings);
}

// TODO: Replace ReadProgramSettings with ParserConfig
std::string Program::readFromString(const std::string& circuitStringified, const ReadProgramSettings settings) {
    std::string foundErrorBuffer;
    parseFileContent(circuitStringified, settings, &foundErrorBuffer);
    return foundErrorBuffer;
}

std::optional<std::string> Program::tryReadFileContent(const std::string_view& fileName, std::string* foundFileHandlingErrors) {
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


bool Program::parseFileContent(std::string_view programToBeParsed, const ReadProgramSettings& config, std::string* foundErrors) {
    const auto& parserConfigToUse = ::parser::ParserConfig(config.defaultBitwidth,
                                                           false,
                                                           false,
                                                           config.reassociateExpressionEnabled,
                                                           config.deadCodeEliminationEnabled,
                                                           config.performConstantPropagation,
                                                           config.operationStrengthReductionEnabled,
                                                           config.deadStoreEliminationEnabled,
                                                           config.combineRedundantInstructions,
                                                           config.multiplicationSimplificationMethod,
                                                           config.optionalLoopUnrollConfig,
                                                           config.optionalNoAdditionalLineSynthesisConfig,
                                                           config.expectedMainModuleName);
    const auto  parsingResult     = ::parser::SyrecParserInterface::parseProgram(programToBeParsed, parserConfigToUse);
    if (parsingResult.wasParsingSuccessful) {
        const auto& optimizer                         = std::make_unique<optimizations::Optimizer>(parserConfigToUse, nullptr);
        const auto& userDefinedMainModuleName   = config.expectedMainModuleName.empty() ? ::parser::ParserConfig::defaultExpectedMainModuleName : config.expectedMainModuleName;
        auto        optimizationResultOfProgram       = optimizer->optimizeProgram(prepareParsingResultForOptimizations(parsingResult.foundModules), userDefinedMainModuleName);

        std::string reasonForFallbackToUnoptimizedResult;
        if (optimizationResultOfProgram.getStatusOfResult() == optimizations::Optimizer::IsUnchanged) {
            this->modulesVec = parsingResult.foundModules;
        } else if (optimizationResultOfProgram.getStatusOfResult() == optimizations::Optimizer::WasOptimized) {
            auto&& optimizedModules = optimizationResultOfProgram.tryTakeOwnershipOfOptimizationResult();
            // This case should not happen and will default to returning the unoptimized result
            if (!optimizedModules.has_value()) {
                reasonForFallbackToUnoptimizedResult = "Expected program to not be completely optimized away, will assume unoptimized result";
            } else {
                this->modulesVec.reserve(optimizedModules->size());
                std::move(optimizedModules->begin(), optimizedModules->end(), std::back_inserter(this->modulesVec));                
            }
        // This case should not happen and will default to returning the unoptimized result
        } else {
            reasonForFallbackToUnoptimizedResult = "Expected program to not be completely optimized away, will assume unoptimized result";
        }

        if (!reasonForFallbackToUnoptimizedResult.empty()) {
            this->modulesVec = parsingResult.foundModules;
            if (foundErrors) {
                *foundErrors = reasonForFallbackToUnoptimizedResult;
            }
        }
        return true;
    }

    const auto& concatinatedFoundErrors = messageUtils::tryStringifyMessages(parsingResult.errors);
    if (foundErrors && concatinatedFoundErrors.has_value()) {
        *foundErrors = *concatinatedFoundErrors;
    }
    return false;
}

bool Program::readFile(const std::string& filename, const ReadProgramSettings settings, std::string* error) {
    *error = read(filename, settings);
    return error->empty();
}

std::vector<std::reference_wrapper<const syrec::Module>> Program::prepareParsingResultForOptimizations(const syrec::Module::vec& foundModules) {
    std::vector<std::reference_wrapper<const syrec::Module>> transformedModuleReferences;
    for (const auto& foundModule: foundModules) {
        transformedModuleReferences.emplace_back(*foundModule.get());
    }
    return transformedModuleReferences;
}

syrec::Module::vec Program::transformProgramOptimizationResult(std::vector<std::unique_ptr<syrec::Module>>&& optimizedProgram) {
    syrec::Module::vec resultContainer(optimizedProgram.size(), nullptr);
    for (std::size_t i = 0; i < optimizedProgram.size(); ++i) {
        resultContainer.at(i) = std::move(optimizedProgram.at(i));
    }
    return resultContainer;
}