#ifndef METHOD_CALL_GUESS_HPP
#define METHOD_CALL_GUESS_HPP

#include <optional>
#include <vector>

#include "core/syrec/module.hpp"
#include "core/syrec/parser/symbol_table.hpp"


namespace parser {
    class ModuleCallGuess {
    public:
        [[nodiscard]] static std::optional<std::unique_ptr<ModuleCallGuess>> tryInitializeWithModulesMatchingName(const SymbolTable::ptr& symbolTable, const std::string_view& moduleIdent);
        [[nodiscard]] bool                                                   hasSomeMatches() const;
        [[nodiscard]] std::vector<SymbolTable::DeclaredModuleSignature>      getMatchesForGuess() const;
        [[nodiscard]] std::vector<std::size_t>                               getInternalIdsOfModulesMatchingGuess() const;

        void refineGuessWithNextParameter(const syrec::Variable& nextActualParameter);
        void discardGuessesWithMoreThanNParameters(const std::size_t& maxAllowedNumFormalParametersPerGuess);

    private:
        template<typename... Arg>
        std::unique_ptr<ModuleCallGuess> static createFrom(Arg&&... arg) {
            struct EnableMakeShared: ModuleCallGuess {
                EnableMakeShared(Arg&&... arg):
                    ModuleCallGuess(std::forward<Arg>(arg)...) {}
            };
            return std::make_unique<EnableMakeShared>(std::forward<Arg>(arg)...);
        }

        std::vector<SymbolTable::DeclaredModuleSignature> modulesMatchingRequestedSignature;
        std::size_t              formalParameterIdx;

        ModuleCallGuess(std::vector<SymbolTable::DeclaredModuleSignature> modulesMatchingName):
            modulesMatchingRequestedSignature(std::move(modulesMatchingName)), formalParameterIdx(0) {}
        
        void discardGuessesWithLessThanActualNumberOfParameters(const std::size_t& numActualParameters);
        void discardGuessBasedOnNumberOfParameters(const std::size_t& numParameterThreshold, bool filterWithLess);

        void                      discardGuessesWhereActualParameterIsNotAssignableToFormalOne(const syrec::Variable& actualParameter);
        [[nodiscard]] static bool isActualParameterAssignableToFormalOne(const syrec::Variable& actualParameter, const syrec::Variable& formalParameter);
    };
}

#endif