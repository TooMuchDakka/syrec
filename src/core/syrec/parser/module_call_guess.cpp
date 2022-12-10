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
    return discardGuessBasedOnNumberOfParameters(maxAllowedNumFormalParametersPerGuess + 1, false);
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


/*
void ModuleCallGuess::refineGuessWithNextParameter(const syrec::Variable::ptr& nextActualParameter) {
    this->discardGuessesWithLessOrEqualNumberOfParameters(this->formalParameterIdx + 1, false);
    this->discardNotMatchingAlternatives(nextActualParameter);

    if (this->hasSomeMatches()) {
        this->formalParameterIdx++;
    }
}


void MethodCallGuess::discardGuessesWithMoreThanNParameters(const std::size_t& numFormalParameters) noexcept {
    if (this->methodsMatchingSignature.empty()) {
        return;
    }

    this->discardGuessesWithLessOrEqualNumberOfParameters(numFormalParameters, true);
}


void MethodCallGuess::discardGuessesWithLessOrEqualNumberOfParameters(const std::size_t& numFormalParameters, bool mustNumParamsMatch) noexcept {
    if (this->methodsMatchingSignature.empty()) {
        return;
    }

    this->methodsMatchingSignature.erase(
            std::remove_if(
                    this->methodsMatchingSignature.begin(),
                    this->methodsMatchingSignature.end(),
                    [numFormalParameters, mustNumParamsMatch](const syrec::Module::ptr& module) { return !hasModuleAtLeastNFormalParameters(numFormalParameters, module, mustNumParamsMatch); }),
            this->methodsMatchingSignature.end()
    );
}

void MethodCallGuess::discardNotMatchingAlternatives(const syrec::Variable::ptr& actualParameter) noexcept {
    if (this->methodsMatchingSignature.empty()) {
        return;
    }

    const std::size_t formalParameterIdx = this->formalParameterIdx;
    this->methodsMatchingSignature.erase(
            std::remove_if(
                    this->methodsMatchingSignature.begin(),
                    this->methodsMatchingSignature.end(),
                    [formalParameterIdx, actualParameter](const syrec::Module::ptr& module) { return !isActualParameterAssignableToFormalOne(actualParameter, module->parameters.at(formalParameterIdx)); }),
            this->methodsMatchingSignature.end());
}

bool MethodCallGuess::hasModuleAtLeastNFormalParameters(const std::size_t& numFormalParametersOfGuess, const syrec::Module::ptr& moduleToCheck, const bool mustNumParamsMatch) noexcept {
    if (mustNumParamsMatch) {
        return moduleToCheck->parameters.size() == numFormalParametersOfGuess;   
    }
    else {
        return !(moduleToCheck->parameters.size() < numFormalParametersOfGuess);   
    }
    
}

bool MethodCallGuess::isActualParameterAssignableToFormalOne(const syrec::Variable::ptr& actualParameter, const syrec::Variable::ptr& formalParameter) noexcept {
    return SymbolTable::isAssignableTo(formalParameter, actualParameter, false);
}
*/