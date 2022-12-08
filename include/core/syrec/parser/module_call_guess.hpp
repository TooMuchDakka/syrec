#ifndef METHOD_CALL_GUESS_HPP
#define METHOD_CALL_GUESS_HPP

#include <optional>
#include <vector>

#include "core/syrec/module.hpp"
#include "core/syrec/parser/symbol_table.hpp"


namespace parser {
    class ModuleCallGuess {
    public:
        typedef std::shared_ptr<ModuleCallGuess> ptr;

        [[nodiscard]] static std::optional<ModuleCallGuess::ptr> fetchPotentialMatchesForMethodIdent(const SymbolTable::ptr& activeSymbolTableScope, const std::string_view& moduleIdent);
        [[nodiscard]] bool hasSomeMatches() const;
        [[nodiscard]] syrec::Module::vec getMatchesForGuess() const;

        void refineGuessWithNextParameter(const syrec::Variable::ptr& nextActualParameter);
        void discardGuessesWithMoreThanNParameters(const std::size_t& maxAllowedNumFormalParametersPerGuess);

    private:
        syrec::Module::vec modulesMatchingSignature;
        std::size_t              formalParameterIdx;

        ModuleCallGuess(const syrec::Module::vec& modulesMatchingSignature): modulesMatchingSignature(modulesMatchingSignature), formalParameterIdx(0) {}
        
        void discardGuessesWithLessThanActualNumberOfParameters(const std::size_t& numActualParameters);
        void discardGuessBasedOnNumberOfParameters(const std::size_t& numParameterThreshold, bool filterWithLess);

        void        discardGuessesWhereActualParameterIsNotAssignableToFormalOne(const syrec::Variable::ptr& actualParameter);
        static bool isActualParameterAssignableToFormalOne(const syrec::Variable::ptr& actualParameter, const syrec::Variable::ptr& formalParameter);
    };
}

#endif