#pragma once

#include "core/syrec/expression.hpp"
#include "core/syrec/grammar.hpp"
#include "core/syrec/module.hpp"
#include "parser/parser_config.hpp"
#include "parser/optimizations/loop_optimizer.hpp"
#include "parser/optimizations/operationSimplification/base_multiplication_simplifier.hpp"

#include <vector>

namespace syrec {
    struct ReadProgramSettings {
        explicit ReadProgramSettings(
                unsigned                                                 bitwidth                                = 32U,
                bool                                                     reassociateExpressionEnabled            = false,
                bool                                                     deadCodeEliminationEnabled              = false,
                bool                                                     performConstantPropagation              = false,
                bool                                                     operationStrengthReductionEnabled       = false,
                bool                                                     deadStoreEliminationEnabled             = false,
                bool                                                     combineRedundantInstructions            = false,
                optimizations::MultiplicationSimplificationMethod        multiplicationSimplificationMethod      = optimizations::MultiplicationSimplificationMethod::None,
                std::optional<optimizations::LoopOptimizationConfig>     optionalLoopUnrollConfig                = std::nullopt,
                std::optional<::parser::NoAdditionalLineSynthesisConfig> optionalNoAdditionalLineSynthesisConfig = std::nullopt,
                std::string                                              expectedMainModuleName                  = "main"):
            defaultBitwidth(bitwidth),
            reassociateExpressionEnabled(reassociateExpressionEnabled),
            deadCodeEliminationEnabled(deadCodeEliminationEnabled),
            performConstantPropagation(performConstantPropagation),
            operationStrengthReductionEnabled(operationStrengthReductionEnabled),
            deadStoreEliminationEnabled(deadStoreEliminationEnabled),
            combineRedundantInstructions(combineRedundantInstructions),
            multiplicationSimplificationMethod(multiplicationSimplificationMethod),
            optionalLoopUnrollConfig(optionalLoopUnrollConfig),
            optionalNoAdditionalLineSynthesisConfig(optionalNoAdditionalLineSynthesisConfig),
            expectedMainModuleName(std::move(expectedMainModuleName)) {}

        unsigned                                                 defaultBitwidth;
        bool                                                     reassociateExpressionEnabled;
        bool                                                     deadCodeEliminationEnabled;
        bool                                                     performConstantPropagation;
        bool                                                     operationStrengthReductionEnabled;
        bool                                                     deadStoreEliminationEnabled;
        bool                                                     combineRedundantInstructions;
        optimizations::MultiplicationSimplificationMethod        multiplicationSimplificationMethod;
        std::optional<optimizations::LoopOptimizationConfig>     optionalLoopUnrollConfig;
        std::optional<::parser::NoAdditionalLineSynthesisConfig> optionalNoAdditionalLineSynthesisConfig;
        std::string                                              expectedMainModuleName;

        ReadProgramSettings& operator=(ReadProgramSettings other) noexcept {
            using std::swap;
            swap(defaultBitwidth, other.defaultBitwidth);
            swap(reassociateExpressionEnabled, other.reassociateExpressionEnabled);
            swap(deadCodeEliminationEnabled, other.deadCodeEliminationEnabled);
            swap(performConstantPropagation, other.performConstantPropagation);
            swap(operationStrengthReductionEnabled, other.operationStrengthReductionEnabled);
            swap(deadStoreEliminationEnabled, other.deadStoreEliminationEnabled);
            swap(combineRedundantInstructions, other.combineRedundantInstructions),
                    swap(multiplicationSimplificationMethod, other.multiplicationSimplificationMethod);
            if (other.optionalLoopUnrollConfig.has_value()) {
                optionalLoopUnrollConfig.emplace(*other.optionalLoopUnrollConfig);
            } else {
                optionalLoopUnrollConfig.reset();
            }

            if (other.optionalNoAdditionalLineSynthesisConfig.has_value()) {
                optionalNoAdditionalLineSynthesisConfig.emplace(*other.optionalNoAdditionalLineSynthesisConfig);
            } else {
                optionalNoAdditionalLineSynthesisConfig.reset();
            }
            swap(expectedMainModuleName, other.expectedMainModuleName);
            return *this;
        }
    };

    class Program {
    public:
        Program() = default;

        void addModule(const Module::ptr& module) {
            modulesVec.emplace_back(module);
        }

        [[nodiscard]] const Module::vec& modules() const {
            return modulesVec;
        }

        [[nodiscard]] Module::ptr findModule(const std::string& name) const {
            for (const Module::ptr& p: modulesVec) {
                if (p->name == name) {
                    return p;
                }
            }

            return {};
        }

        // TODO: Replace ReadProgramSettings with ParserConfig
        std::string read(const std::string& filename, ReadProgramSettings settings = ReadProgramSettings{});
        // TODO: Replace ReadProgramSettings with ParserConfig
        std::string readFromString(const std::string& circuitStringified, ReadProgramSettings settings = ReadProgramSettings{});

    private:
        Module::vec modulesVec;

        /**
        * @brief Parser for a SyReC program
        *
        * This function call performs both the lexical parsing
        * as well as the semantic analysis of the program which
        * creates the corresponding C++ constructs for the
        * program.
        *
        * @param filename File-name to parse from
        * @param settings Settings
        * @param error Error Message, in case the function returns false
        *
        * @return true if parsing was successful, otherwise false
        */
        bool                                                                          readFile(const std::string& filename, ReadProgramSettings settings, std::string* error = nullptr);
        [[nodiscard]] static std::optional<std::string>                               tryReadFileContent(const std::string_view& fileName, std::string* foundFileHandlingErrors);
        [[maybe_unused]] bool                                                         parseFileContent(std::string_view programToBeParsed, const ReadProgramSettings& config, std::string* foundErrors);
        [[nodiscard]] static std::vector<std::reference_wrapper<const syrec::Module>> prepareParsingResultForOptimizations(const syrec::Module::vec& foundModules);
        [[nodiscard]] static syrec::Module::vec                                       transformProgramOptimizationResult(std::vector<std::unique_ptr<syrec::Module>>&& optimizedProgram);
    };

} // namespace syrec
