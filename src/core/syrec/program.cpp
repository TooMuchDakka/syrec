#include "core/syrec/program.hpp"

#include "core/syrec/parser/antlr/parserComponents/syrec_parser_interface.hpp"
#include "core/syrec/parser/optimizations/optimizer.hpp"
#include "core/syrec/parser/utils/syrec_ast_dump_utils.hpp"

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

Program::OptimizationResult Program::readAndOptimizeCircuit(const std::string& circuitFilename, ReadProgramSettings settings) {
    std::string foundErrorBuffer;
    if (const std::optional<std::string>& readFileContent = tryReadFileContent(circuitFilename, &foundErrorBuffer); readFileContent.has_value() && foundErrorBuffer.empty()) {
        return readAndOptimizeCircuitFromString(*readFileContent, settings);
    }
    return OptimizationResult({foundErrorBuffer, std::nullopt});
}

Program::OptimizationResult Program::readAndOptimizeCircuitFromString(const std::string& circuitStringified, ReadProgramSettings settings) {
    std::string foundErrorBuffer = readFromString(circuitStringified, settings);
    if (foundErrorBuffer.empty()) {
        auto parserConfigToUse                                  = ::parser::ParserConfig();
        parserConfigToUse.cDefaultSignalWidth                   = settings.defaultBitwidth;
        parserConfigToUse.combineRedundantInstructions          = settings.combineRedundantInstructions;
        parserConfigToUse.constantPropagationEnabled            = settings.constantPropagationEnabled;
        parserConfigToUse.deadCodeEliminationEnabled            = settings.deadCodeEliminationEnabled;
        parserConfigToUse.deadStoreEliminationEnabled           = settings.deadStoreEliminationEnabled;
        parserConfigToUse.expectedMainModuleName                = settings.expectedMainModuleName;
        parserConfigToUse.multiplicationSimplificationMethod    = settings.multiplicationSimplificationMethod;
        parserConfigToUse.noAdditionalLineOptimizationConfig    = settings.optionalNoAdditionalLineSynthesisConfig;
        parserConfigToUse.operationStrengthReductionEnabled     = settings.operationStrengthReductionEnabled;
        parserConfigToUse.optionalLoopUnrollConfig              = settings.optionalLoopUnrollConfig;
        parserConfigToUse.reassociateExpressionsEnabled         = settings.reassociateExpressionEnabled;
        parserConfigToUse.supportBroadcastingAssignmentOperands = false;
        parserConfigToUse.supportBroadcastingExpressionOperands = false;

        if (const std::optional<syrec::Module::vec>& optimizedProgramModules = optimizeProgram(modulesVec, parserConfigToUse, foundErrorBuffer); foundErrorBuffer.empty()) {
            if (optimizedProgramModules.has_value()) {
                modulesVec = *optimizedProgramModules;
            }
            auto syrecAstDumper = syrecAstDumpUtils::SyrecASTDumper::createUsingDefaultASTDumpConfig();
            return OptimizationResult({"", syrecAstDumper.stringifyModules(modulesVec)});
        }
    }
    return OptimizationResult({foundErrorBuffer, std::nullopt});
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
    auto parserConfigToUse = ::parser::ParserConfig();
    parserConfigToUse.cDefaultSignalWidth = config.defaultBitwidth;
    parserConfigToUse.supportBroadcastingAssignmentOperands = false;
    parserConfigToUse.supportBroadcastingExpressionOperands = false;
    parserConfigToUse.expectedMainModuleName                = config.expectedMainModuleName;
    
    const auto  parsingResult     = ::parser::SyrecParserInterface::parseProgram(programToBeParsed, parserConfigToUse);
    if (parsingResult.wasParsingSuccessful) {
        modulesVec = parsingResult.foundModules;
        return true;
    }

    if (const std::optional<std::string> concatinatedFoundErrors = foundErrors ? messageUtils::tryStringifyMessages(parsingResult.errors) : std::nullopt; concatinatedFoundErrors.has_value()) {
        *foundErrors = concatinatedFoundErrors.value();
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

std::optional<syrec::Module::vec> Program::optimizeProgram(const syrec::Module::vec& parsedProgram, const ::parser::ParserConfig& config, std::string& foundErrors) {
    const auto& optimizer                   = std::make_unique<optimizations::Optimizer>(config, nullptr);
    const auto& userDefinedMainModuleName   = config.expectedMainModuleName.empty() ? ::parser::ParserConfig::defaultExpectedMainModuleName : config.expectedMainModuleName;
    auto        optimizationResultOfProgram = optimizer->optimizeProgram(prepareParsingResultForOptimizations(parsedProgram), userDefinedMainModuleName);
    
    std::string reasonForFallbackToUnoptimizedResult;
    if (optimizationResultOfProgram.getStatusOfResult() == optimizations::Optimizer::IsUnchanged) {
        return parsedProgram;
    }

    if (optimizationResultOfProgram.getStatusOfResult() == optimizations::Optimizer::WasOptimized) {
        if (auto&& optimizedModules = optimizationResultOfProgram.tryTakeOwnershipOfOptimizationResult(); optimizedModules.has_value()) {
            syrec::Module::vec containerForOptimizedResult;
            containerForOptimizedResult.reserve(optimizedModules->size());
            std::move(optimizedModules->begin(), optimizedModules->end(), std::back_inserter(containerForOptimizedResult));
            return containerForOptimizedResult;
        }
    }
    // This case should not happen and will default to returning the unoptimized result
    foundErrors = "Expected program to not be completely optimized away, will assume unoptimized result";
    return std::nullopt;
}
