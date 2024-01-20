#include "core/syrec/parser/symbol_table.hpp"

#include <functional>
#include <set>

using namespace parser;

syrec::Variable::vec SymbolTable::DeclaredModuleSignature::determineOptimizedCallSignature(std::unordered_set<std::size_t>* indicesOfRemainingParameters) const {
    if (indicesOfOptimizedAwayParameters.size() == declaredParameters.size()) {
        return {};
    }

    const auto           lookupForOptimizedAwayParameterIndizes = std::set(indicesOfOptimizedAwayParameters.cbegin(), indicesOfOptimizedAwayParameters.cend());
    syrec::Variable::vec optimizedCallSignatureArguments;
    optimizedCallSignatureArguments.reserve(declaredParameters.size() - indicesOfOptimizedAwayParameters.size());
    for (std::size_t i = 0; i < declaredParameters.size(); ++i) {
        if (!lookupForOptimizedAwayParameterIndizes.count(i)) {
            optimizedCallSignatureArguments.emplace_back(declaredParameters.at(i));
            if (indicesOfRemainingParameters) {
                indicesOfRemainingParameters->insert(i);
            }
        }
    }
    return optimizedCallSignatureArguments;
}

bool SymbolTable::contains(const std::string_view& literalIdent) const {
    return this->locals.find(literalIdent) != this->locals.end()
        || this->modules.find(literalIdent) != this->modules.end()
        || (outer && outer->contains(literalIdent));
}

bool SymbolTable::contains(const syrec::Module::ptr& module) const {
    if (!module) {
        return false;
    }

    const auto module_search_result = this->modules.find(module->name);
    if (module_search_result != this->modules.end()) {
        for (const auto& [_, symbolTableDataOfModulesMatchingName] : module_search_result->second->internalDataLookup) {
            if (doModuleSignaturesMatch(symbolTableDataOfModulesMatchingName.declaredModule, module)) {
                return true;
            }
        }
    }

    return outer && outer->contains(module);
}

std::optional<std::variant<syrec::Variable::ptr, syrec::Number::ptr>> SymbolTable::getVariable(const std::string_view& literalIdent) const {
    std::optional<std::variant<syrec::Variable::ptr, syrec::Number::ptr>> found_entry;

    if (this->locals.find(literalIdent) != this->locals.end()) {
        found_entry.emplace(this->locals.find(literalIdent)->second->variableInformation);
    }
    else if (outer) {
        return outer->getVariable(literalIdent);
    }
    return found_entry;
}

std::vector<unsigned> SymbolTable::fetchBitwidthsPerSharedBitwidthSignalGroupsWithAssignableSignals(unsigned requiredMinimumBitwidth) const {
    std::unordered_set<unsigned int> bitwidthGroups;
    for (const auto& localsKvPair: locals) {
        const std::shared_ptr<VariableSymbolTableEntry>& signalInformation = localsKvPair.second;
        if (std::holds_alternative<syrec::Variable::ptr>(signalInformation->variableInformation) && canSignalBeAssignedTo(localsKvPair.first)) {
            if (const syrec::Variable::ptr rawSignalInformation = std::get<syrec::Variable::ptr>(signalInformation->variableInformation); rawSignalInformation->bitwidth >= requiredMinimumBitwidth && !bitwidthGroups.count(rawSignalInformation->bitwidth)) {
                bitwidthGroups.emplace(rawSignalInformation->bitwidth);
            }
        }
    }

    if (bitwidthGroups.empty()) {
        return {};
    }

    std::vector<unsigned int> sortedBitwidthGroups;
    sortedBitwidthGroups.reserve(bitwidthGroups.size());
    for (const unsigned int bitwidth : bitwidthGroups) {
        sortedBitwidthGroups.emplace_back(bitwidth);
    }
    std::sort(sortedBitwidthGroups.begin(), sortedBitwidthGroups.end());
    return sortedBitwidthGroups;
}

syrec::Variable::vec SymbolTable::fetchedDeclaredAssignableSignalsHavingMatchingBitwidth(unsigned requiredBitwidth, const std::optional<std::unordered_set<std::string>>& identsOfSignalsToExclude) const {
    syrec::Variable::vec matchingSignalsForBitwidth;
    for (const auto& localsKvPair : locals) {
        if (identsOfSignalsToExclude.has_value() && identsOfSignalsToExclude->count(localsKvPair.first)) {
            continue;
        }
        const std::shared_ptr<VariableSymbolTableEntry>& signalInformation = localsKvPair.second;
        if (std::holds_alternative<syrec::Variable::ptr>(signalInformation->variableInformation) && canSignalBeAssignedTo(localsKvPair.first)) {
            if (const syrec::Variable::ptr rawSignalInformation = std::get<syrec::Variable::ptr>(signalInformation->variableInformation); rawSignalInformation->bitwidth == requiredBitwidth) {
                matchingSignalsForBitwidth.emplace_back(rawSignalInformation);    
            }
        }
    }
    return matchingSignalsForBitwidth;
}

std::vector<std::string> SymbolTable::fetchIdentsOfSignalsStartingWith(const std::string_view& signalIdentPrefixRequiredForMatch) const {
    std::vector<std::string> matchingSignalIdents;
    for (const auto& localsKvPair : locals) {
        if (!localsKvPair.first.rfind(signalIdentPrefixRequiredForMatch)) {
            matchingSignalIdents.emplace_back(localsKvPair.first);
        }
    }
    return matchingSignalIdents;
}

std::optional<SymbolTable::DeclaredModuleSignature> SymbolTable::getMatchingModuleForIdentifier(const ModuleIdentifier& moduleIdentifier) const {
    if (const auto& matchingModules = getEntryForModulesWithMatchingName(moduleIdentifier.moduleIdent); matchingModules && !matchingModules->internalDataLookup.empty() && matchingModules->internalDataLookup.count(moduleIdentifier.internalModuleId)) {
        const auto& internalDataLookupEntry = matchingModules->internalDataLookup.at(moduleIdentifier.internalModuleId);
        return DeclaredModuleSignature({internalDataLookupEntry.internalId, internalDataLookupEntry.declaredModule->parameters, internalDataLookupEntry.indicesOfDroppedParametersInOptimizedVersion});
    }
    return std::nullopt;
}

std::vector<SymbolTable::DeclaredModuleSignature> SymbolTable::getMatchingModuleSignaturesForName(const std::string_view& moduleName) const {
    if (const auto& matchingModules = getEntryForModulesWithMatchingName(moduleName); matchingModules && !matchingModules->internalDataLookup.empty()) {
        std::vector<SymbolTable::DeclaredModuleSignature> containerForMatchingSignatures;
        containerForMatchingSignatures.reserve(matchingModules->internalDataLookup.size());
        
        std::transform(
        matchingModules->internalDataLookup.cbegin(),
        matchingModules->internalDataLookup.cend(),
        std::back_inserter(containerForMatchingSignatures),
        [](const std::pair<std::size_t, SymbolTable::ModuleSymbolTableEntry::InternalModuleHelperData>& internalDataLookupEntry) {
            return SymbolTable::DeclaredModuleSignature({internalDataLookupEntry.second.internalId, internalDataLookupEntry.second.declaredModule->parameters, internalDataLookupEntry.second.indicesOfDroppedParametersInOptimizedVersion});
        });
        return containerForMatchingSignatures;
    }
    return {};
}

std::optional<SymbolTable::DeclaredModuleSignature> SymbolTable::tryGetOptimizedSignatureForModuleCall(const std::string_view& moduleName, const std::vector<std::string>& calleeArguments, bool areCalleeArgumentsBasedOnOptimizedSignature) const {
    std::vector<std::reference_wrapper<const syrec::Variable>> symbolTableEntriesForCallerArguments;
    symbolTableEntriesForCallerArguments.reserve(calleeArguments.size());

    for (const auto& callerArgumentSignalIdent : calleeArguments) {
        if (const auto& symbolTableEntry = getEntryForVariable(callerArgumentSignalIdent); symbolTableEntry) {
            const auto& signalInformation = std::get<syrec::Variable::ptr>(symbolTableEntry->variableInformation).get();
            symbolTableEntriesForCallerArguments.emplace_back(*signalInformation);
        }
        else {
            return std::nullopt;   
        }
    }
    
    auto modulesMatchingIdent = getMatchingModuleSignaturesForName(moduleName);
    if (!areCalleeArgumentsBasedOnOptimizedSignature) {
        modulesMatchingIdent.erase(
                std::remove_if(
                        modulesMatchingIdent.begin(),
                        modulesMatchingIdent.end(),
                        [&](const DeclaredModuleSignature& moduleSignature) {
                            bool wasAnyParameterNotAssignable = moduleSignature.declaredParameters.size() != symbolTableEntriesForCallerArguments.size();
                            for (std::size_t i = 0; i < moduleSignature.declaredParameters.size() && !wasAnyParameterNotAssignable; ++i) {
                                const auto& parameterOfModuleSymbolTableEntry = moduleSignature.declaredParameters.at(i);
                                const auto& userProvidedModuleParameter       = symbolTableEntriesForCallerArguments.at(i);
                                wasAnyParameterNotAssignable                  = !isAssignableTo(*parameterOfModuleSymbolTableEntry, userProvidedModuleParameter.get(), true);
                            }
                            return wasAnyParameterNotAssignable;
                        }),
                modulesMatchingIdent.end());
    } else {
        modulesMatchingIdent.erase(
                std::remove_if(
                        modulesMatchingIdent.begin(),
                        modulesMatchingIdent.end(),
                        [&](const DeclaredModuleSignature& moduleSignature) {
                            const auto& signatureOfOptimizedModule   = moduleSignature.determineOptimizedCallSignature(nullptr);
                            bool        wasAnyParameterNotAssignable = signatureOfOptimizedModule.size() != symbolTableEntriesForCallerArguments.size();
                            for (std::size_t i = 0; i < signatureOfOptimizedModule.size() && !wasAnyParameterNotAssignable; ++i) {
                                const auto& parameterOfModuleSymbolTableEntry = signatureOfOptimizedModule.at(i);
                                const auto& userProvidedModuleParameter       = symbolTableEntriesForCallerArguments.at(i);
                                wasAnyParameterNotAssignable                  = !isAssignableTo(*parameterOfModuleSymbolTableEntry, userProvidedModuleParameter.get(), true);
                            }
                            return wasAnyParameterNotAssignable;
                        }),
                modulesMatchingIdent.end());
    }

    
    if (modulesMatchingIdent.empty() || modulesMatchingIdent.size() > 1) {
        return std::nullopt;
    }
    return modulesMatchingIdent.front();
}

std::optional<SymbolTable::DeclaredModuleSignature> SymbolTable::tryGetOptimizedSignatureForModuleCall(const syrec::Module& callTarget) const {
    if (!modules.count(callTarget.name)) {
        return std::nullopt;
    }

    const auto& modulesMatchingName = modules.at(callTarget.name)->internalDataLookup;
    for (const auto& internalModuleLookupEntry : modulesMatchingName) {
        const auto& parametersOfSymbolTableEntry = internalModuleLookupEntry.second.declaredModule->parameters;
        bool doSignaturesMatch = parametersOfSymbolTableEntry.size() == callTarget.parameters.size();
        for (std::size_t i = 0; i < parametersOfSymbolTableEntry.size() && doSignaturesMatch; ++i) {
            const auto& formalParameter = parametersOfSymbolTableEntry.at(i);
            const auto& actualParameter = callTarget.parameters.at(i);
            doSignaturesMatch &= formalParameter->name == actualParameter->name && isAssignableTo(*formalParameter, *actualParameter, true);
        }

        if (doSignaturesMatch) {
            return DeclaredModuleSignature({internalModuleLookupEntry.first, parametersOfSymbolTableEntry, internalModuleLookupEntry.second.indicesOfDroppedParametersInOptimizedVersion});
        }
    }
    return std::nullopt;
}


std::optional<std::unique_ptr<syrec::Module>> SymbolTable::getFullDeclaredModuleInformation(const ModuleIdentifier& moduleIdentifier) const {
    if (const auto& matchingModules = getEntryForModulesWithMatchingName(moduleIdentifier.moduleIdent); matchingModules && !matchingModules->internalDataLookup.empty()) {
        const auto moduleMatchingId = matchingModules->internalDataLookup.find(moduleIdentifier.internalModuleId);
        if (moduleMatchingId != matchingModules->internalDataLookup.cend()) {
            return std::make_unique<syrec::Module>(*moduleMatchingId->second.declaredModule);    
        }
    }
    return std::nullopt;
}

bool SymbolTable::addEntry(const syrec::Variable& variable) {
    if (variable.name.empty() || contains(variable.name)) {
        return false;
    }
    
    locals.insert(std::make_pair(variable.name, std::make_shared<VariableSymbolTableEntry>(VariableSymbolTableEntry(variable))));
    return true;
}

bool SymbolTable::addEntry(const syrec::Number& number, const unsigned int bitsRequiredToStoreMaximumValue, const std::optional<unsigned int>& defaultLoopVariableValue) {
    if (!number.isLoopVariable() || number.variableName().empty() || contains(number.variableName())) {
        return false;
    }

    const auto& symbolTableEntryForNumber = std::make_shared<VariableSymbolTableEntry>(number, bitsRequiredToStoreMaximumValue, defaultLoopVariableValue);
    locals.insert(std::make_pair(number.variableName(), symbolTableEntryForNumber));
    return true;
}

bool SymbolTable::addEntry(const syrec::Module& module, std::size_t* internalModuleId) {
    const auto symbolTableEntryForModulesMatchingName = getEntryForModulesWithMatchingName(module.name);
    std::size_t generatedIdForModule;
    if (!symbolTableEntryForModulesMatchingName) {
        const auto& createdSymbolTableEntryForModulesMatchingName = std::make_shared<ModuleSymbolTableEntry>();
        generatedIdForModule = createdSymbolTableEntryForModulesMatchingName->addModule(module);
        modules.insert(std::make_pair(module.name, createdSymbolTableEntryForModulesMatchingName));
    } else {
        generatedIdForModule = symbolTableEntryForModulesMatchingName->addModule(module);
    }

    if (internalModuleId) {
        *internalModuleId = generatedIdForModule;
    }
    return true;
}

void SymbolTable::removeVariable(const std::string& literalIdent) {
    if (!contains(literalIdent)) {
        return;
    }

    if (!locals.count(literalIdent) && outer) {
        outer->removeVariable(literalIdent);
    } else if (locals.count(literalIdent)) {
        locals.erase(literalIdent);
    }
}

void SymbolTable::markModuleVariableAsUnused(const ModuleIdentifier& moduleIdentifier, const std::string_view& literalIdent, bool doesIdentReferenceModuleParameter) const {
    if (const auto& matchingSymbolTableEntryForModuleMatchingGivenModuleName = getEntryForModulesWithMatchingName(moduleIdentifier.moduleIdent); matchingSymbolTableEntryForModuleMatchingGivenModuleName) {
        if (!matchingSymbolTableEntryForModuleMatchingGivenModuleName->internalDataLookup.count(moduleIdentifier.internalModuleId)) {
            return;
        }

        auto& matchingSymbolTableEntryForModule = matchingSymbolTableEntryForModuleMatchingGivenModuleName->internalDataLookup.at(moduleIdentifier.internalModuleId);
        if (const auto& foundSymbolTableEntryForLiteral = getEntryForVariable(literalIdent); foundSymbolTableEntryForLiteral) {
            std::optional<std::size_t> foundIndexOfParameterOrLocalVariable;
            if (doesIdentReferenceModuleParameter) {
                const auto& foundMatchingParameterEntry = std::find_if(
                    matchingSymbolTableEntryForModule.declaredModule->parameters.cbegin(), 
                    matchingSymbolTableEntryForModule.declaredModule->parameters.cend(), 
                    [&literalIdent](const syrec::Variable::ptr& parameter) {
                            return parameter->name == literalIdent;
                });
                if (foundMatchingParameterEntry != matchingSymbolTableEntryForModule.declaredModule->parameters.cend()) {
                    foundIndexOfParameterOrLocalVariable = std::distance(matchingSymbolTableEntryForModule.declaredModule->parameters.cbegin(), foundMatchingParameterEntry);
                }
            }
            else {
                const auto& foundMatchingLocalVariableEntry = std::find_if(
                        matchingSymbolTableEntryForModule.declaredModule->variables.cbegin(),
                        matchingSymbolTableEntryForModule.declaredModule->variables.cend(),
                        [&literalIdent](const syrec::Variable::ptr& localVariable) {
                            return localVariable->name == literalIdent;
                        });
                if (foundMatchingLocalVariableEntry != matchingSymbolTableEntryForModule.declaredModule->parameters.cend()) {
                    foundIndexOfParameterOrLocalVariable = std::distance(matchingSymbolTableEntryForModule.declaredModule->variables.cbegin(), foundMatchingLocalVariableEntry);
                }
            }

            if (!foundIndexOfParameterOrLocalVariable.has_value()) {
                return;
            }
            if (std::find_if(
                matchingSymbolTableEntryForModule.indicesOfDroppedParametersInOptimizedVersion.cbegin(), 
                matchingSymbolTableEntryForModule.indicesOfDroppedParametersInOptimizedVersion.cend(), 
                [&foundIndexOfParameterOrLocalVariable](const std::size_t index) {
                    return index == *foundIndexOfParameterOrLocalVariable;
            }) != matchingSymbolTableEntryForModule.indicesOfDroppedParametersInOptimizedVersion.cend()) {
                matchingSymbolTableEntryForModule.indicesOfDroppedParametersInOptimizedVersion.emplace_back(*foundIndexOfParameterOrLocalVariable);
            }
            foundSymbolTableEntryForLiteral->referenceCount = 0;
        }
    }
}

void SymbolTable::updateReferenceCountOfLiteral(const std::string_view& literalIdent, ReferenceCountUpdate typeOfUpdate) const {
    if (const auto& foundSymbolTableEntryForLiteral = getEntryForVariable(literalIdent); foundSymbolTableEntryForLiteral) {
        performReferenceCountUpdate(foundSymbolTableEntryForLiteral->referenceCount, typeOfUpdate);
    }
}

void SymbolTable::updateReferenceCountOfModulesMatchingSignature(const std::vector<SymbolTable::ModuleIdentifier>& moduleIdentifiers, SymbolTable::ReferenceCountUpdate typeOfUpdate) const {
    if (const auto& symbolTableEntryForModulesMatchingIdent = getEntryForModulesWithMatchingName(moduleIdentifiers.front().moduleIdent); symbolTableEntryForModulesMatchingIdent) {
        for (const auto& moduleIdentifier : moduleIdentifiers) {
            if (const auto& internalDataOfModuleMatchingId = symbolTableEntryForModulesMatchingIdent->internalDataLookup.find(moduleIdentifier.internalModuleId); internalDataOfModuleMatchingId != symbolTableEntryForModulesMatchingIdent->internalDataLookup.end()) {
                performReferenceCountUpdate(internalDataOfModuleMatchingId->second.referenceCount, typeOfUpdate);
            }
        }
    }
}

bool SymbolTable::changeStatementsOfModule(const ModuleIdentifier& moduleIdentifier, const syrec::Statement::vec& updatedModuleBodyStatements) const {
    if (const auto& symbolTableEntryForModulesMatchingIdent = getEntryForModulesWithMatchingName(moduleIdentifier.moduleIdent); symbolTableEntryForModulesMatchingIdent != nullptr) {
        if (const auto& internalDataOfModuleMatchingId = symbolTableEntryForModulesMatchingIdent->internalDataLookup.find(moduleIdentifier.internalModuleId); internalDataOfModuleMatchingId != symbolTableEntryForModulesMatchingIdent->internalDataLookup.end()) {
            const syrec::Module::ptr& referenceModuleData = internalDataOfModuleMatchingId->second.declaredModule;
            referenceModuleData->statements               = updatedModuleBodyStatements;
            return true;
        }
    }
    return false;
}

bool SymbolTable::isVariableUsedAnywhereBasedOnReferenceCount(const std::string_view& literalIdent) const {
    if (const auto& matchingEntryForVariable = getEntryForVariable(literalIdent); matchingEntryForVariable) {
        return matchingEntryForVariable->referenceCount;
    }
    return false;
}

void SymbolTable::openScope(SymbolTable::ptr& currentScope) {
    if (currentScope) {
        const auto newScope = std::make_shared<SymbolTable>();
        newScope->outer     = currentScope;
        currentScope        = newScope;   
    } else {
        currentScope = std::make_shared<SymbolTable>();
    }
}

void SymbolTable::closeScope(SymbolTable::ptr& currentScope) {
    if (currentScope) {
        currentScope = currentScope->outer;
    } 
}

bool SymbolTable::doModuleSignaturesMatch(const syrec::Module::ptr& module, const syrec::Module::ptr& otherModule) {
    auto doSignaturesMatch = module->parameters.size() == otherModule->parameters.size();
    for (std::size_t i = 0; i < module->parameters.size() && doSignaturesMatch; ++i) {
        const auto& thisModuleParameter = module->parameters.at(i);
        const auto& thatModuleParameter = otherModule->parameters.at(i);
        doSignaturesMatch &= thisModuleParameter->name == thatModuleParameter->name && isAssignableTo(*thisModuleParameter, *thatModuleParameter, true);
    }
    return doSignaturesMatch;
}

// TODO: Can this condition be relaxed (i.e. is a variable of bitwidth 32 assignable to one with a bitwidth of 16 by truncation ?)
bool SymbolTable::isAssignableTo(const syrec::Variable& formalParameter, const syrec::Variable& actualParameter, bool mustParameterTypesMatch) {
    return (mustParameterTypesMatch ? formalParameter.type == actualParameter.type : true) 
        && formalParameter.dimensions == actualParameter.dimensions
        && formalParameter.bitwidth == actualParameter.bitwidth;
}

std::optional<unsigned int> SymbolTable::tryFetchValueForLiteral(const syrec::VariableAccess::ptr& assignedToSignalParts) const {
    const auto& symbolTableEntryForVariable = getEntryForVariable(assignedToSignalParts->var->name);
    if (symbolTableEntryForVariable == nullptr || !symbolTableEntryForVariable->optionalValueLookup.has_value() || !doesVariableAccessAllowValueLookup(symbolTableEntryForVariable, assignedToSignalParts)) {
        return std::nullopt;
    }
    
    const auto  isAssignedToVariableLoopVariable = std::holds_alternative<syrec::Number::ptr>(symbolTableEntryForVariable->variableInformation);
    const auto& transformedBitRange = tryTransformAccessedBitRange(assignedToSignalParts, false);
    const auto& transformedDimensionAccess = tryTransformAccessedDimensions(assignedToSignalParts, isAssignedToVariableLoopVariable);

    const auto& signalValueLookup = *symbolTableEntryForVariable->optionalValueLookup;
    return signalValueLookup->tryFetchValueFor(transformedDimensionAccess, transformedBitRange);
}

std::optional<unsigned> SymbolTable::tryFetchValueOfLoopVariable(const std::string_view& loopVariableIdent) const {
    const auto& symbolTableEntryForVariable = getEntryForVariable(loopVariableIdent);
    if (!symbolTableEntryForVariable || !symbolTableEntryForVariable->optionalValueLookup.has_value() || !std::holds_alternative<syrec::Number::ptr>(symbolTableEntryForVariable->variableInformation)) {
        return std::nullopt;
    }
    const auto& signalValueLookup = *symbolTableEntryForVariable->optionalValueLookup;
    return signalValueLookup->tryFetchValueFor({std::make_optional(0u)}, std::nullopt);
}


void SymbolTable::invalidateStoredValuesFor(const syrec::VariableAccess::ptr& assignedToSignalParts) const {
    const auto& symbolTableEntryForVariable = getEntryForVariable(assignedToSignalParts->var->name);
    if (!symbolTableEntryForVariable || !symbolTableEntryForVariable->optionalValueLookup.has_value()) {
        return;
    }

    const auto  isAssignedToVariableLoopVariable = std::holds_alternative<syrec::Number::ptr>(symbolTableEntryForVariable->variableInformation);
    const auto& transformedBitRange              = tryTransformAccessedBitRange(assignedToSignalParts);
    const auto& transformedDimensionAccess       = tryTransformAccessedDimensions(assignedToSignalParts, isAssignedToVariableLoopVariable);
    
    const auto& signalValueLookup = *symbolTableEntryForVariable->optionalValueLookup;
    if (!transformedBitRange.has_value()) {
        if (transformedDimensionAccess.empty()) {
            signalValueLookup->invalidateAllStoredValuesForSignal();
        } else {
            signalValueLookup->invalidateStoredValueFor(transformedDimensionAccess);
        }
    }
    else {
        if (isAssignedToVariableLoopVariable) {
            signalValueLookup->invalidateStoredValueFor(transformedDimensionAccess);
        } else {
            signalValueLookup->invalidateStoredValueForBitrange(transformedDimensionAccess, *transformedBitRange);   
        }
    }
}

void SymbolTable::invalidateStoredValueForLoopVariable(const std::string_view& loopVariableIdent) const {
    const auto& symbolTableEntryForVariable = getEntryForVariable(loopVariableIdent);
    if (!symbolTableEntryForVariable || !symbolTableEntryForVariable->optionalValueLookup.has_value()) {
        return;
    }

    const auto& signalValueLookup = *symbolTableEntryForVariable->optionalValueLookup;
    signalValueLookup->invalidateAllStoredValuesForSignal();
}


[[nodiscard]] std::unordered_set<std::string> SymbolTable::getUnusedLiterals() const {
    std::unordered_set<std::string> unusedLiterals;
    for (const auto& localKVPair : locals) {
        const auto& localSymbolTableEntry = localKVPair.second;
        if (localSymbolTableEntry->referenceCount == 0) {
            unusedLiterals.emplace(localKVPair.first);
        }
    }
    return unusedLiterals;
}

std::unordered_set<std::size_t> SymbolTable::updateOptimizedModuleSignatureByMarkingAndReturningIndicesOfUnusedParameters(const ModuleIdentifier& moduleIdentifier) const {
    std::unordered_set<std::size_t> optimizedAwayParameterIndices;
    if (const auto& matchingSymbolTableEntryForModuleMatchingGivenModuleName = getEntryForModulesWithMatchingName(moduleIdentifier.moduleIdent); matchingSymbolTableEntryForModuleMatchingGivenModuleName) {
        if (!matchingSymbolTableEntryForModuleMatchingGivenModuleName->internalDataLookup.count(moduleIdentifier.internalModuleId)) {
            return optimizedAwayParameterIndices;
        }

        auto&       matchingSymbolTableEntryForModule = matchingSymbolTableEntryForModuleMatchingGivenModuleName->internalDataLookup.at(moduleIdentifier.internalModuleId);
        const auto& moduleParameters                  = matchingSymbolTableEntryForModule.declaredModule->parameters;
        for (std::size_t i = 0; i < moduleParameters.size(); i++) {
            if (const auto& symbolTableEntryForParameter = getEntryForVariable(moduleParameters.at(i)->name); symbolTableEntryForParameter && !symbolTableEntryForParameter->referenceCount) {
                optimizedAwayParameterIndices.emplace(i);
                addIndexOfDroppedParameterToOptimizedModule(matchingSymbolTableEntryForModule, i);
            }
        }
    }
    return optimizedAwayParameterIndices;
}

std::unordered_set<std::size_t> SymbolTable::fetchUnusedLocalModuleVariableIndicesAndRemoveFromSymbolTable(const ModuleIdentifier& moduleIdentifier) const {
    std::unordered_set<std::size_t> optimizedAwayLocalVariables;
    if (const auto& matchingSymbolTableEntryForModuleMatchingGivenModuleName = getEntryForModulesWithMatchingName(moduleIdentifier.moduleIdent); matchingSymbolTableEntryForModuleMatchingGivenModuleName) {
        if (!matchingSymbolTableEntryForModuleMatchingGivenModuleName->internalDataLookup.count(moduleIdentifier.internalModuleId)) {
            return optimizedAwayLocalVariables;
        }

        const auto& matchingSymbolTableEntryForModule = matchingSymbolTableEntryForModuleMatchingGivenModuleName->internalDataLookup.at(moduleIdentifier.internalModuleId);
        auto& localVariablesOfModule            = matchingSymbolTableEntryForModule.declaredModule->variables;

        std::size_t localVariableIndex = 0;
        for (auto localVariableOfModuleIterator = localVariablesOfModule.begin(); localVariableOfModuleIterator != localVariablesOfModule.end();) {
            if (const auto& symbolTableEntryForLocalVariable = getEntryForVariable(localVariableOfModuleIterator->get()->name); symbolTableEntryForLocalVariable && !symbolTableEntryForLocalVariable->referenceCount) {
                optimizedAwayLocalVariables.emplace(localVariableIndex);
                localVariableOfModuleIterator = localVariablesOfModule.erase(localVariableOfModuleIterator);
            }
            else {
                ++localVariableOfModuleIterator;
            }
            localVariableIndex++;
        }

    }
    return optimizedAwayLocalVariables;
}

std::optional<bool> SymbolTable::canSignalBeAssignedTo(const std::string_view& literalIdent) const {
    if (const auto& matchingSymbolTableEntryForSignal = getEntryForVariable(literalIdent); matchingSymbolTableEntryForSignal) {
        if (std::holds_alternative<syrec::Variable::ptr>(matchingSymbolTableEntryForSignal->variableInformation)) {
            const auto& variableInformation = std::get<syrec::Variable::ptr>(matchingSymbolTableEntryForSignal->variableInformation);
            const auto& mappedToVariableTypeEnumFromFlag = convertVariableTypeFlagToEnum(variableInformation->type);
            return mappedToVariableTypeEnumFromFlag.has_value() && doesVariableTypeAllowAssignment(*mappedToVariableTypeEnumFromFlag);
        }
    }
    return false;
}


// For correct overload resolution of the user supplied module, the unoptimized module call signature shall be provided as the calling argument to this function
std::optional<bool> SymbolTable::determineIfModuleWasUnused(const ModuleCallSignature& moduleSignatureToCheck) const {
    // We should only use this function in the global symbol table scope
    if (outer) {
        return std::nullopt;
    }

    // There should be an entry in the symbol table for the module even if it was unused, the latter will only be determine by the reference count
    if (!modules.count(moduleSignatureToCheck.moduleIdent)) {
        return std::nullopt;
    }

    const auto& symbolTableEntriesMatchingCalledModuleIdent = modules.at(moduleSignatureToCheck.moduleIdent)->internalDataLookup;
    for (const auto& symbolTableEntryKvPairForModule: symbolTableEntriesMatchingCalledModuleIdent) {
        const auto& symbolTableEntryForModule               = symbolTableEntryKvPairForModule.second;
        if (symbolTableEntryForModule.declaredModule->parameters.size() != moduleSignatureToCheck.parameters.size()) {
            continue;
        }

        const syrec::Variable::vec& referenceModuleParameters = symbolTableEntryForModule.declaredModule->parameters;
        bool                 doModuleParametersMatch  = true;
        for (std::size_t i = 0; i < referenceModuleParameters.size() && doModuleParametersMatch; ++i) {
            const auto& formalParameterName = referenceModuleParameters.at(i)->name;
            const auto& userProvidedParameterName = moduleSignatureToCheck.parameters.at(i)->name;
            doModuleParametersMatch &= formalParameterName == userProvidedParameterName;
        }
        return doModuleParametersMatch && !symbolTableEntryForModule.referenceCount;
    }
    return std::nullopt;
}

std::optional<valueLookup::SignalValueLookup::ptr> SymbolTable::createBackupOfValueOfSignal(const std::string_view& literalIdent) const {
    if (const auto& symbolTableEntryForLiteral = getEntryForVariable(literalIdent); symbolTableEntryForLiteral) {
        if (symbolTableEntryForLiteral->optionalValueLookup.has_value()) {
            return std::make_optional((*symbolTableEntryForLiteral->optionalValueLookup)->clone());
        }
    }
    return std::nullopt;
}

void SymbolTable::restoreValuesFromBackup(const std::string_view& literalIdent, const valueLookup::SignalValueLookup::ptr& newValues) const {
    if (const auto& symbolTableEntryForLiteral = getEntryForVariable(literalIdent); symbolTableEntryForLiteral) {
        if (symbolTableEntryForLiteral->optionalValueLookup.has_value()) {
            const auto& currentValueOfLiteral = *symbolTableEntryForLiteral->optionalValueLookup;
            currentValueOfLiteral->copyRestrictionsAndUnrestrictedValuesFrom(
                {},
                std::nullopt,
                {},
                std::nullopt,
                *newValues
            );
        }
    }
}


SymbolTable::ModuleSymbolTableEntry* SymbolTable::getEntryForModulesWithMatchingName(const std::string_view& moduleIdent) const {
    const auto& symbolTableEntryForModule = modules.find(moduleIdent);
    
    if (symbolTableEntryForModule != modules.end()) {
        return &(*symbolTableEntryForModule->second);
    }

    if (nullptr != outer) {
        return outer->getEntryForModulesWithMatchingName(moduleIdent);
    }

    return nullptr;
}

SymbolTable::VariableSymbolTableEntry* SymbolTable::getEntryForVariable(const std::string_view& literalIdent) const {
    const auto& symbolTableEntryForIdent = locals.find(literalIdent);

    if (symbolTableEntryForIdent != locals.end()) {
        return &(*symbolTableEntryForIdent->second);
    }

    if (nullptr != outer) {
        return outer->getEntryForVariable(literalIdent);
    }

    return nullptr;
}

bool               SymbolTable::doesVariableAccessAllowValueLookup(const VariableSymbolTableEntry* symbolTableEntryForVariable, const syrec::VariableAccess::ptr& variableAccess) const {
    if (std::holds_alternative<syrec::Number::ptr>(symbolTableEntryForVariable->variableInformation)) {
        return !variableAccess->range.has_value() && (variableAccess->indexes.empty() || variableAccess->indexes.size() == 1);
    }

    const auto& declaredDimensionsOfSignal    = std::get<syrec::Variable::ptr>(symbolTableEntryForVariable->variableInformation)->dimensions;
    if (!variableAccess->indexes.empty()) {
        return declaredDimensionsOfSignal.size() == variableAccess->indexes.size()
            && std::none_of(
                variableAccess->indexes.cbegin(),
                variableAccess->indexes.cend(),
                [](const syrec::expression::ptr& accessedValueOfDimensionExpr) {
                    if (const auto& numericExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(accessedValueOfDimensionExpr); numericExpr) {
                        return !numericExpr->value->isConstant();
                    }
                    return true;
        });
    }
    return declaredDimensionsOfSignal.size() == 1 && declaredDimensionsOfSignal.front() == 1;
}

std::optional<SymbolTable::DeclaredModuleSignature> SymbolTable::tryGetOptimizedSignatureForModuleCall(const std::string_view& moduleName, std::size_t& internalModuleId) const {
    if (!modules.count(moduleName)) {
        return std::nullopt;
    }

    const auto& signaturesOfModulesMatchingName = getMatchingModuleSignaturesForName(moduleName);
    const auto& moduleMatchingNameAndInternalId = std::find_if(signaturesOfModulesMatchingName.cbegin(),
                 signaturesOfModulesMatchingName.cend(),
                 [&internalModuleId](const DeclaredModuleSignature& moduleSignature) {
                     return moduleSignature.internalModuleId == internalModuleId;
                 });
    return moduleMatchingNameAndInternalId == signaturesOfModulesMatchingName.cend() ? std::nullopt : std::make_optional(*moduleMatchingNameAndInternalId);
}

std::optional<::optimizations::BitRangeAccessRestriction::BitRangeAccess> SymbolTable::tryTransformAccessedBitRange(const syrec::VariableAccess::ptr& accessedSignalParts, bool considerUnknownBitRangeStartAndEndAsWholeSignalAccess) {
    const auto&                                                               accessedBitRange = accessedSignalParts->range;
    std::optional<::optimizations::BitRangeAccessRestriction::BitRangeAccess> transformedBitRangeAccess;
    if (accessedBitRange.has_value()) {
        unsigned int bitRangeStart = 0;
        unsigned int bitRangeEnd   = accessedSignalParts->var->bitwidth - 1;
        if (accessedBitRange->first->isConstant() && accessedBitRange->second->isConstant()) {
            bitRangeStart = accessedBitRange->first->evaluate({});
            bitRangeEnd   = accessedBitRange->second->evaluate({});
        } else if (!considerUnknownBitRangeStartAndEndAsWholeSignalAccess) {
            return std::nullopt;
        }
        transformedBitRangeAccess.emplace(std::pair(bitRangeStart, bitRangeEnd));
    }
    return transformedBitRangeAccess;
}

std::vector<std::optional<unsigned>> SymbolTable::tryTransformAccessedDimensions(const syrec::VariableAccess::ptr& accessedSignalParts, bool isAccessedSignalLoopVariable) {
    if (accessedSignalParts->indexes.empty() && accessedSignalParts->var->dimensions.size() == 1 && accessedSignalParts->var->dimensions.front() == 1) {
        return { std::make_optional(0) };
    }

    // TODO: Do we need to insert an entry into this vector when working with loop variables or 1D signals (for the latter it seems natural)
    std::vector<std::optional<unsigned int>> transformedDimensionAccess;
    if (!accessedSignalParts->indexes.empty() && !isAccessedSignalLoopVariable) {
        std::transform(
                accessedSignalParts->indexes.cbegin(),
                accessedSignalParts->indexes.cend(),
                std::back_inserter(transformedDimensionAccess),
                [](const syrec::expression::ptr& accessedValueOfDimensionExpr) -> std::optional<unsigned int> {
                    if (const auto& numericExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(accessedValueOfDimensionExpr); numericExpr) {
                        if (numericExpr->value->isConstant()) {
                            return std::make_optional(numericExpr->value->evaluate({}));
                        }
                    }
                    return std::nullopt;
                });
    }
    return transformedDimensionAccess;
}

// TODO: CONSTANT_PROPAGATION: What should happen if the value cannot be stored inside the accessed signal part
// TODO: CONSTANT_PROPAGATION: Should it be the responsibility of the caller to validate the assigned to signal part prior to this call ?
/*
 * TODO: CONSTANT_PROPAGATION: What should happen on value overflow (more of a problem for the parser since it provides the new value as a parameter)
 * Think about the following problem: Assume a.0 += 1; a.0 += 1; What should happen now ? (Use overflow semantics and assume 1 or 0) ?
 *
 * Offer configuration to either overflow or invalidate value on overflow
 */ 
void SymbolTable::updateStoredValueFor(const syrec::VariableAccess::ptr& assignedToSignalParts, unsigned int newValue) const {
    const auto& symbolTableEntryForVariable = getEntryForVariable(assignedToSignalParts->var->name);
    if (symbolTableEntryForVariable == nullptr || !symbolTableEntryForVariable->optionalValueLookup.has_value()) {
        return;
    }

    const auto isAssignedToVariableLoopVariable = std::holds_alternative<syrec::Number::ptr>(symbolTableEntryForVariable->variableInformation);
    const auto& transformedDimensionAccess = tryTransformAccessedDimensions(assignedToSignalParts, isAssignedToVariableLoopVariable);
    const auto& transformedBitRangeAccess        = tryTransformAccessedBitRange(assignedToSignalParts);
    const auto& signalValueLookup                = *symbolTableEntryForVariable->optionalValueLookup;

    // TODO: SIGNAL_VALUE_LOOKUP: Lifting existing restrictions could also be done in the update call and "should" not be the task of the user ?
    signalValueLookup->liftRestrictionsOfDimensions(transformedDimensionAccess, transformedBitRangeAccess);
    signalValueLookup->updateStoredValueFor(transformedDimensionAccess, transformedBitRangeAccess, newValue);
}

void SymbolTable::updateStoredValueForLoopVariable(const std::string_view& loopVariableIdent, unsigned newValue) const {
    const auto& symbolTableEntryForVariable = getEntryForVariable(loopVariableIdent);
    if (symbolTableEntryForVariable == nullptr || !std::holds_alternative<syrec::Number::ptr>(symbolTableEntryForVariable->variableInformation) || !symbolTableEntryForVariable->optionalValueLookup.has_value()) {
        return;
    }

    const auto& signalValueLookup = *symbolTableEntryForVariable->optionalValueLookup;
    const auto& requiredDimensionAccessToUpdateValue = std::vector({std::make_optional<unsigned int>(0u)});

    /*
     * We need to lift all restrictions that would prevent an update of the signal value.
     * Since loop variables are readonly variables for the user, we control its value and thus restrictions can only stem from the initial readonly parsing of the loop
     */ 
    signalValueLookup->liftRestrictionsFromWholeSignal();
    signalValueLookup->updateStoredValueFor(requiredDimensionAccessToUpdateValue, std::nullopt, newValue);
}


void SymbolTable::updateViaSignalAssignment(const syrec::VariableAccess::ptr& assignmentLhsOperand, const syrec::VariableAccess::ptr& assignmentRhsOperand) const {
    const auto& symbolTableEntryForLhsVariable = getEntryForVariable(assignmentLhsOperand->var->name);
    const auto& symbolTableEntryForRhsVariable = getEntryForVariable(assignmentRhsOperand->var->name);

    // TODO: Are these checks necessary or can we require the caller to validate its arguments and throw an exception otherwise.
    // TODO: One could also invalidate the lhs if the rhs either has not value, is not an inout / out parameter or has not corresponding symbol table entry
    if (!symbolTableEntryForLhsVariable || !symbolTableEntryForLhsVariable->optionalValueLookup.has_value() 
        || !symbolTableEntryForRhsVariable || !symbolTableEntryForRhsVariable->optionalValueLookup.has_value()) {
        return;
    }

    const auto& signalValueLookupOfLhsVariable = *symbolTableEntryForLhsVariable->optionalValueLookup;
    const auto& signalValueLookupOfRhsVariable = *symbolTableEntryForRhsVariable->optionalValueLookup;

    signalValueLookupOfLhsVariable->copyRestrictionsAndUnrestrictedValuesFrom(
        tryTransformAccessedDimensions(assignmentLhsOperand, false), tryTransformAccessedBitRange(assignmentLhsOperand),
        tryTransformAccessedDimensions(assignmentRhsOperand, false), tryTransformAccessedBitRange(assignmentRhsOperand),
        *signalValueLookupOfRhsVariable
    );
}

void SymbolTable::swap(const syrec::VariableAccess::ptr& swapLhsOperand, const syrec::VariableAccess::ptr& swapRhsOperand) const {
    const auto& symbolTableEntryForLhsVariable = getEntryForVariable(swapLhsOperand->var->name);
    const auto& symbolTableEntryForRhsVariable = getEntryForVariable(swapRhsOperand->var->name);
    
    // TODO: Are these checks necessary or can we require the caller to validate its arguments and throw an exception otherwise.
    // TODO: One could also invalidate the lhs if the rhs either has not value, is not an inout / out parameter or has not corresponding symbol table entry
    if (symbolTableEntryForLhsVariable == nullptr || !symbolTableEntryForLhsVariable->optionalValueLookup.has_value() 
        || symbolTableEntryForRhsVariable == nullptr || !symbolTableEntryForRhsVariable->optionalValueLookup.has_value()) {
        return;
    }

    const auto& signalValueLookupOfLhsVariable = *symbolTableEntryForLhsVariable->optionalValueLookup;
    const auto& signalValueLookupOfRhsVariable = *symbolTableEntryForRhsVariable->optionalValueLookup;

    signalValueLookupOfLhsVariable->swapValuesAndRestrictionsBetween(
            tryTransformAccessedDimensions(swapLhsOperand, false), tryTransformAccessedBitRange(swapLhsOperand),
            tryTransformAccessedDimensions(swapRhsOperand, false), tryTransformAccessedBitRange(swapRhsOperand),
        *signalValueLookupOfRhsVariable
    );
}

void SymbolTable::performReferenceCountUpdate(std::size_t& currentReferenceCount, ReferenceCountUpdate typeOfUpdate) {
    if (typeOfUpdate == ReferenceCountUpdate::Increment) {
        if (currentReferenceCount != SIZE_MAX) {
            currentReferenceCount++;
        }
    } else if (typeOfUpdate == ReferenceCountUpdate::Decrement) {
        if (currentReferenceCount) {
            currentReferenceCount--;
        }
    }
}

void SymbolTable::addIndexOfDroppedParameterToOptimizedModule(ModuleSymbolTableEntry::InternalModuleHelperData& optimizedModuleData, std::size_t indexOfOptimizedAwayParameter) {
    if (!optimizedModuleData.declaredModule || indexOfOptimizedAwayParameter > optimizedModuleData.declaredModule->parameters.size()) {
        return;
    }

    if (std::find_if(
            optimizedModuleData.indicesOfDroppedParametersInOptimizedVersion.cbegin(),
            optimizedModuleData.indicesOfDroppedParametersInOptimizedVersion.cend(),
            [&indexOfOptimizedAwayParameter](const std::size_t existingIndexOfOptimizedAwayParameter) {
                return existingIndexOfOptimizedAwayParameter == indexOfOptimizedAwayParameter;
            }) == optimizedModuleData.indicesOfDroppedParametersInOptimizedVersion.cend()) {
        optimizedModuleData.indicesOfDroppedParametersInOptimizedVersion.emplace_back(indexOfOptimizedAwayParameter);
    }
}

// TODO: Handling of state flag
bool SymbolTable::doesVariableTypeAllowAssignment(syrec::Variable::Types variableType) {
    switch (variableType) {
        case syrec::Variable::Inout:
        case syrec::Variable::Out:
        case syrec::Variable::Wire:
        //case syrec::Variable::State:
            return true;
        default:
            return false;
    }
}

std::optional<syrec::Variable::Types> SymbolTable::convertVariableTypeFlagToEnum(unsigned variableTypeFlag) {
    if (variableTypeFlag == syrec::Variable::Types::In) {
        return syrec::Variable::Types::In;
    }
    if (variableTypeFlag == syrec::Variable::Types::Out) {
        return syrec::Variable::Types::Out;
    }
    if (variableTypeFlag == syrec::Variable::Types::Inout) {
        return syrec::Variable::Types::Inout;
    }
    if (variableTypeFlag == syrec::Variable::Types::Wire) {
        return syrec::Variable::Types::Wire;
    }
    if (variableTypeFlag == syrec::Variable::Types::State) {
        return syrec::Variable::Types::State;
    }
    return std::nullopt;
}
