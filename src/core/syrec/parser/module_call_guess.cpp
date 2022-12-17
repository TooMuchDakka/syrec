#include "core/syrec/parser/module_call_guess.hpp"

using namespace parser;

std::optional<ModuleCallGuess::ptr> ModuleCallGuess::fetchPotentialMatchesForMethodIdent(const SymbolTable::ptr& activeSymbolTableScope, const std::string_view& moduleIdent) {
    const std::optional<syrec::Module::vec>& modulesMatchingName = activeSymbolTableScope->getMatchingModulesForName(moduleIdent);
    if (modulesMatchingName.has_value()) {
        return std::make_optional(std::shared_ptr<ModuleCallGuess>(new ModuleCallGuess(*modulesMatchingName)));
    }
    return std::nullopt;
}

bool ModuleCallGuess::hasSomeMatches() const {
    return !this->modulesMatchingSignature.empty();
}

syrec::Module::vec ModuleCallGuess::getMatchesForGuess() const {
    return this->modulesMatchingSignature;
}


void ModuleCallGuess::refineGuessWithNextParameter(const syrec::Variable::ptr& nextActualParameter) {
    this->discardGuessesWithLessThanActualNumberOfParameters(this->formalParameterIdx + 1);
    this->discardGuessesWhereActualParameterIsNotAssignableToFormalOne(nextActualParameter);
    this->formalParameterIdx++;
}


void ModuleCallGuess::discardGuessesWithLessThanActualNumberOfParameters(const std::size_t& numActualParameters) {
    return discardGuessBasedOnNumberOfParameters(numActualParameters, true);
}

void ModuleCallGuess::discardGuessesWithMoreThanNParameters(const std::size_t& maxAllowedNumFormalParametersPerGuess) {
    return discardGuessBasedOnNumberOfParameters(maxAllowedNumFormalParametersPerGuess, false);
}

void ModuleCallGuess::discardGuessBasedOnNumberOfParameters(const std::size_t& numParameterThreshold, bool filterWithLess) {
    if (this->modulesMatchingSignature.empty()) {
        return;
    }

    modulesMatchingSignature.erase(
            std::remove_if(
                    this->modulesMatchingSignature.begin(),
                    this->modulesMatchingSignature.end(),
                    [numParameterThreshold, filterWithLess](const syrec::Module::ptr& module) {
                        if (filterWithLess) {
                            return module->parameters.size() < numParameterThreshold;
                        }
                        return module->parameters.size() > numParameterThreshold;
                    }),
            this->modulesMatchingSignature.end());
}

void ModuleCallGuess::discardGuessesWhereActualParameterIsNotAssignableToFormalOne(const syrec::Variable::ptr& actualParameter) {
    if (this->modulesMatchingSignature.empty()) {
        return;
    }

    const std::size_t tmpFormalParameterIdx = this->formalParameterIdx;
    modulesMatchingSignature.erase(
            std::remove_if(
                    this->modulesMatchingSignature.begin(),
                    this->modulesMatchingSignature.end(),
                    [tmpFormalParameterIdx, actualParameter](const syrec::Module::ptr& module) {
                        const syrec::Variable::ptr& formalParameter = module->parameters.at(tmpFormalParameterIdx);
                        return !isActualParameterAssignableToFormalOne(actualParameter, formalParameter);
                    }),
            this->modulesMatchingSignature.end());
}

// TODO: Is flag in symbol table required
bool ModuleCallGuess::isActualParameterAssignableToFormalOne(const syrec::Variable::ptr& actualParameter, const syrec::Variable::ptr& formalParameter) {
    return SymbolTable::isAssignableTo(formalParameter, actualParameter, false);
}