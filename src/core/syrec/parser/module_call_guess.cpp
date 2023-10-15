#include "core/syrec/parser/module_call_guess.hpp"

using namespace parser;

std::optional<std::unique_ptr<ModuleCallGuess>> ModuleCallGuess::tryInitializeWithModulesMatchingName(const SymbolTable::ptr& symbolTable, const std::string_view& moduleIdent) {
    if (const auto& modulesMatchingName = symbolTable->getMatchingModuleSignaturesForName(moduleIdent); !modulesMatchingName.empty()) {
        return createFrom(modulesMatchingName);
    }
    return std::nullopt;
}

bool ModuleCallGuess::hasSomeMatches() const {
    return !this->modulesMatchingRequestedSignature.empty();
}

std::vector<SymbolTable::DeclaredModuleSignature> ModuleCallGuess::getMatchesForGuess() const {
    return this->modulesMatchingRequestedSignature;
}

std::vector<std::size_t> ModuleCallGuess::getInternalIdsOfModulesMatchingGuess() const {
    std::vector<std::size_t> internalModuleIds;
    internalModuleIds.reserve(this->modulesMatchingRequestedSignature.size());
    std::transform(
            this->modulesMatchingRequestedSignature.cbegin(),
            this->modulesMatchingRequestedSignature.cend(),
            std::back_inserter(internalModuleIds),
            [](const SymbolTable::DeclaredModuleSignature& moduleSignature) {
                return moduleSignature.internalModuleId;
            });
    return internalModuleIds;
}


void ModuleCallGuess::refineGuessWithNextParameter(const syrec::Variable& nextActualParameter) {
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
    if (this->modulesMatchingRequestedSignature.empty()) {
        return;
    }

    modulesMatchingRequestedSignature.erase(
            std::remove_if(
                    this->modulesMatchingRequestedSignature.begin(),
                    this->modulesMatchingRequestedSignature.end(),
                    [numParameterThreshold, filterWithLess](const SymbolTable::DeclaredModuleSignature& moduleSignature) {
                        if (filterWithLess) {
                            return moduleSignature.declaredParameters.size() < numParameterThreshold;
                        }
                        return moduleSignature.declaredParameters.size() > numParameterThreshold;
                    }),
            this->modulesMatchingRequestedSignature.end());
}

void ModuleCallGuess::discardGuessesWhereActualParameterIsNotAssignableToFormalOne(const syrec::Variable& actualParameter) {
    if (this->modulesMatchingRequestedSignature.empty()) {
        return;
    }

    const std::size_t tmpFormalParameterIdx = this->formalParameterIdx;
    modulesMatchingRequestedSignature.erase(
            std::remove_if(
                    this->modulesMatchingRequestedSignature.begin(),
                    this->modulesMatchingRequestedSignature.end(),
                    [tmpFormalParameterIdx, actualParameter](const SymbolTable::DeclaredModuleSignature& moduleSignature) {
                        const syrec::Variable& formalParameter = *moduleSignature.declaredParameters.at(tmpFormalParameterIdx);
                        return !isActualParameterAssignableToFormalOne(actualParameter, formalParameter);
                    }),
            this->modulesMatchingRequestedSignature.end());
}

// TODO: Is flag in symbol table required
bool ModuleCallGuess::isActualParameterAssignableToFormalOne(const syrec::Variable& actualParameter, const syrec::Variable& formalParameter) {
    return SymbolTable::isAssignableTo(formalParameter, actualParameter, false);
}