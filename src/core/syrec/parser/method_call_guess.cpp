#include "core/syrec/parser/method_call_guess.hpp"

using namespace parser;

void MethodCallGuess::refineWithNextParameter(const syrec::Variable::ptr& actualParameter) noexcept {
    this->discardGuessesWithLessOrEqualNumberOfParameters(this->formalParameterIdx + 1, false);
    this->discardNotMatchingAlternatives(actualParameter);

    if (this->hasSomeMatches()) {
        this->formalParameterIdx++;
    }
}

bool MethodCallGuess::hasSomeMatches() const noexcept {
    return !this->methodsMatchingSignature.empty();
}

std::optional<syrec::Module::vec> MethodCallGuess::getMatchesForGuess() const noexcept {
    return std::make_optional(this->methodsMatchingSignature);
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