#pragma once

#include "core/syrec/expression.hpp"
#include "core/syrec/grammar.hpp"
#include "core/syrec/module.hpp"
#include "core/syrec/number.hpp"
#include "core/syrec/statement.hpp"
#include "core/syrec/variable.hpp"

#include "core/syrec/parser/parser_config.hpp"

#include <fstream>
#include <vector>

namespace syrec {

    struct ReadProgramSettings {
        explicit ReadProgramSettings(
            unsigned bitwidth = 32U,
            bool reassociateExpressionEnabled = false,
            bool removalOfUnusedVariablesAndModulesEnabled = false,
            bool performConstantPropagation = false,
            std::string expectedMainModuleName = "main"):
            defaultBitwidth(bitwidth),
            reassociateExpressionEnabled(reassociateExpressionEnabled),
            removalOfUnusedVariablesAndModulesEnabled(removalOfUnusedVariablesAndModulesEnabled),
            performConstantPropagation(performConstantPropagation),
            expectedMainModuleName(expectedMainModuleName)
        {};
        unsigned defaultBitwidth;
        bool     reassociateExpressionEnabled;
        bool     removalOfUnusedVariablesAndModulesEnabled;
        bool        performConstantPropagation;
        std::string expectedMainModuleName;
    };

    class program {
    public:
        program() = default;

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
        bool readFile(const std::string& filename, ReadProgramSettings settings, std::string* error = nullptr);
        std::string    parseBufferContent(const unsigned char* buffer, int bufferSizeInBytes, const ReadProgramSettings& config);
        unsigned char* readAndBufferFileContent(const std::string& filename, std::size_t* fileContentLength);
    };

} // namespace syrec
