#ifndef METHOD_CALL_GUESS_HPP
#define METHOD_CALL_GUESS_HPP

#include <optional>
#include <string>
#include <vector>

#include "core/syrec/module.hpp"
#include "core/syrec/parser/symbol_table.hpp"


namespace syrec {
    class MethodCallGuess {
    public:
        typedef std::shared_ptr<MethodCallGuess> ptr;

        explicit MethodCallGuess(const SymbolTable::ptr& activeSymbolTableScope, const std::string_view& methodName)
            : methodsMatchingSignature({}), formalParameterIdx(0) {
            const std::optional<Module::vec>& modulesMatchingName = activeSymbolTableScope->getMatchingModulesForName(methodName);
            if (modulesMatchingName.has_value()) {
                methodsMatchingSignature = modulesMatchingName.value();
            }
        }

        void refineWithNextParameter(const Variable::ptr& actualParameter) noexcept;
        [[nodiscard]] bool hasSomeMatches() const noexcept;
        [[nodiscard]] std::optional<Module::vec> getMatchesForGuess() const noexcept;
        void                                     discardGuessesWithMoreThanNParameters(const std::size_t& numFormalParameters) noexcept;

    private:
        Module::vec methodsMatchingSignature;
        std::size_t              formalParameterIdx;

        void        discardGuessesWithLessOrEqualNumberOfParameters(const std::size_t& numFormalParameters, bool mustNumParamsMatch) noexcept;
        void discardNotMatchingAlternatives(const Variable::ptr& actualParameter) noexcept;
        static bool hasModuleAtLeastNFormalParameters(const std::size_t& numFormalParametersOfGuess, const Module::ptr& moduleToCheck, bool mustNumParamsMatch) noexcept;
        static bool isActualParameterAssignableToFormalOne(const Variable::ptr& actualParameter, const Variable::ptr& formalParameter) noexcept;
    };
}

#endif