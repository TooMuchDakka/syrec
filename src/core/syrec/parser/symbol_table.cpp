#include "core/syrec/parser/symbol_table.hpp"

#include <functional>

using namespace parser;
bool SymbolTable::contains(const std::string_view& literalIdent) const {
    return this->locals.find(literalIdent) != this->locals.end()
        || this->modules.find(literalIdent) != this->modules.end()
        || (nullptr != outer && outer->contains(literalIdent));
}

bool SymbolTable::contains(const syrec::Module::ptr& module) const {
    if (nullptr == module) {
        return false;
    }

    bool       foundModuleMatchingSignature    = false;
    const auto module_search_result = this->modules.find(module->name);
    if (module_search_result != this->modules.end()) {
        const syrec::Module::vec modulesWithMatchingName = module_search_result->second->matchingModules;

        foundModuleMatchingSignature = std::find_if(
            cbegin(modulesWithMatchingName),
            cend(modulesWithMatchingName),
            [&module](const syrec::Module::ptr& moduleWithMatchingName) { return doModuleSignaturesMatch(moduleWithMatchingName, module); })
        != cend(modulesWithMatchingName);
    }

    return foundModuleMatchingSignature || (nullptr != outer && outer->contains(module));
}

std::optional<std::variant<syrec::Variable::ptr, syrec::Number::ptr>> SymbolTable::getVariable(const std::string_view& literalIdent) const {
    std::optional<std::variant<syrec::Variable::ptr, syrec::Number::ptr>> found_entry;

    if (this->locals.find(literalIdent) != this->locals.end()) {
        found_entry.emplace(this->locals.find(literalIdent)->second->variableInformation);
    }
    else if (nullptr != outer) {
        return outer->getVariable(literalIdent);
    }
    return found_entry;
}

std::optional<syrec::Module::vec> SymbolTable::getMatchingModulesForName(const std::string_view& moduleName) const {
    std::optional<syrec::Module::vec> matchingModules;
    if (this->modules.find(moduleName) != this->modules.end()) {
        matchingModules.emplace(this->modules.find(moduleName)->second->matchingModules);
    } else if (nullptr != outer) {
        matchingModules = outer->getMatchingModulesForName(moduleName);
    }
    return matchingModules;
}

bool SymbolTable::addEntry(const syrec::Variable::ptr& variable) {
    if (nullptr == variable || variable->name.empty() || contains(variable->name)) {
        return false;
    }
    
    locals.insert(std::make_pair(variable->name, std::make_shared<VariableSymbolTableEntry>(VariableSymbolTableEntry(variable))));
    return true;
}

bool SymbolTable::addEntry(const syrec::Number::ptr& number, const unsigned int bitsRequiredToStoreMaximumValue, const std::optional<unsigned int>& defaultLoopVariableValue) {
    if (nullptr == number || !number->isLoopVariable() || number->variableName().empty() || contains(number->variableName())) {
        return false;
    }

    locals.insert(std::make_pair(number->variableName(), std::make_shared<VariableSymbolTableEntry>(VariableSymbolTableEntry(number, bitsRequiredToStoreMaximumValue, defaultLoopVariableValue))));
    return true;
}

bool SymbolTable::addEntry(const syrec::Module::ptr& module, const std::vector<bool>& isUnusedStatusPerModuleParameter) {
    if (nullptr == module) {
        return false;
    }

    auto symbolTableEntryForModuleName = getEntryForModulesWithMatchingName(module->name);
    if (symbolTableEntryForModuleName == nullptr) {
        const auto& newSymbolTableEntryForModuleName = std::make_shared<ModuleSymbolTableEntry>(ModuleSymbolTableEntry());
        modules.insert(std::pair(module->name, newSymbolTableEntryForModuleName));
        symbolTableEntryForModuleName = newSymbolTableEntryForModuleName.get();
    }
    symbolTableEntryForModuleName->addModule(module, isUnusedStatusPerModuleParameter);
    return true;
}

void SymbolTable::removeVariable(const std::string& literalIdent) {
    if (!contains(literalIdent)) {
        return;
    }

    if (locals.count(literalIdent) == 0 && outer != nullptr) {
        outer->removeVariable(literalIdent);
    } else if (locals.count(literalIdent) != 0) {
        locals.erase(literalIdent);
    }
}


std::optional<std::set<std::size_t>> SymbolTable::getPositionOfUnusedParametersForModule(const syrec::Module::ptr& module) const {
    const auto& symbolTableEntryForModuleName = getEntryForModulesWithMatchingName(module->name);
    if (symbolTableEntryForModuleName == nullptr) {
        return std::nullopt;
    }
    
    const auto& foundPositionOfModuleMatchingSignature =
            std::find_if(symbolTableEntryForModuleName->matchingModules.cbegin(),
                         symbolTableEntryForModuleName->matchingModules.cend(),
                         [&module](const syrec::Module::ptr& moduleStoredInSymbolTable) {
                             return doModuleSignaturesMatch(module, moduleStoredInSymbolTable);
                         });
    if (foundPositionOfModuleMatchingSignature == symbolTableEntryForModuleName->matchingModules.cend()) {
        return std::nullopt;
    }

    const std::size_t positionOfMatch = std::distance(symbolTableEntryForModuleName->matchingModules.cbegin(), foundPositionOfModuleMatchingSignature);
    const auto&              isUnusedStatusPerParameter = symbolTableEntryForModuleName->optimizedAwayStatusPerParameterOfModulePerModule.at(positionOfMatch);
    std::set<std::size_t> positionOfUnusedParameters;
    for (std::size_t i = 0; i < isUnusedStatusPerParameter.size(); ++i) {
        if (isUnusedStatusPerParameter.at(i)) {
            positionOfUnusedParameters.emplace(i);
        }
    }
    return std::make_optional(positionOfUnusedParameters);
}


void SymbolTable::openScope(SymbolTable::ptr& currentScope) {
    // TODO: Implement me
    SymbolTable::ptr       newScope = std::make_shared<SymbolTable>(SymbolTable());
    newScope->outer           = currentScope;
    currentScope                    = newScope;
}

void SymbolTable::closeScope(SymbolTable::ptr& currentScope) {
    if (nullptr != currentScope) {
        currentScope = currentScope->outer;
    } 
}

bool SymbolTable::doModuleSignaturesMatch(const syrec::Module::ptr& module, const syrec::Module::ptr& otherModule) {
    bool signatures_match = module->parameters.size() == otherModule->parameters.size();
    for (size_t param_idx = 0; param_idx < module->parameters.size() && signatures_match; param_idx++) {
        const syrec::Variable::ptr & parameter_first_module = module->parameters.at(param_idx);
        const syrec::Variable::ptr&  parameter_other_module = otherModule->parameters.at(param_idx);
        signatures_match                                    = parameter_first_module->name == parameter_other_module->name && isAssignableTo(parameter_first_module, parameter_other_module, true);
    }
    return signatures_match;
}

// TODO: Can this condition be relaxed (i.e. is a variable of bitwidth 32 assignable to one with a bitwidth of 16 by truncation ?)
bool SymbolTable::isAssignableTo(const syrec::Variable::ptr& formalParameter, const syrec::Variable::ptr& actualParameter, bool mustParameterTypesMatch) {
    return (mustParameterTypesMatch ? formalParameter->type == actualParameter->type : true) 
        && formalParameter->dimensions == actualParameter->dimensions
        && formalParameter->bitwidth == actualParameter->bitwidth;
}

void SymbolTable::incrementLiteralReferenceCount(const std::string_view& literalIdent) const {
    if (const auto& foundSymbolTableEntryForLiteral = getEntryForVariable(literalIdent); foundSymbolTableEntryForLiteral != nullptr) {
        if (foundSymbolTableEntryForLiteral->referenceCount == SIZE_MAX) {
            return;
        }
        foundSymbolTableEntryForLiteral->referenceCount++;
    }
}

void SymbolTable::decrementLiteralReferenceCount(const std::string_view& literalIdent) const {
    if (const auto& foundSymbolTableEntryForLiteral = getEntryForVariable(literalIdent); foundSymbolTableEntryForLiteral != nullptr) {
        foundSymbolTableEntryForLiteral->referenceCount = foundSymbolTableEntryForLiteral->referenceCount == 0 ? 0 : foundSymbolTableEntryForLiteral->referenceCount - 1;
    }
}

void SymbolTable::incrementReferenceCountOfModulesMatchingSignature(const syrec::Module::ptr& module) const {
    incrementOrDecrementReferenceCountOfModulesMatchingSignature(module, true);
}

void SymbolTable::decrementReferenceCountOfModulesMatchingSignature(const syrec::Module::ptr& module) const {
    incrementOrDecrementReferenceCountOfModulesMatchingSignature(module, false);
}

std::optional<unsigned int> SymbolTable::tryFetchValueForLiteral(const syrec::VariableAccess::ptr& assignedToSignalParts) const {
    const auto& symbolTableEntryForVariable = getEntryForVariable(assignedToSignalParts->var->name);
    if (symbolTableEntryForVariable == nullptr || !symbolTableEntryForVariable->optionalValueLookup.has_value() || !doesVariableAccessAllowValueLookup(symbolTableEntryForVariable, assignedToSignalParts)) {
        return std::nullopt;
    }
    
    const auto  isAssignedToVariableLoopVariable = std::holds_alternative<syrec::Number::ptr>(symbolTableEntryForVariable->variableInformation);
    const auto& transformedBitRange = tryTransformAccessedBitRange(assignedToSignalParts);
    const auto& transformedDimensionAccess = tryTransformAccessedDimensions(assignedToSignalParts, isAssignedToVariableLoopVariable);

    const auto& signalValueLookup = *symbolTableEntryForVariable->optionalValueLookup;
    return signalValueLookup->tryFetchValueFor(transformedDimensionAccess, transformedBitRange);
}

void SymbolTable::invalidateStoredValuesFor(const syrec::VariableAccess::ptr& assignedToSignalParts) const {
    const auto& symbolTableEntryForVariable = getEntryForVariable(assignedToSignalParts->var->name);
    if (symbolTableEntryForVariable == nullptr || !symbolTableEntryForVariable->optionalValueLookup.has_value()) {
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
    if (symbolTableEntryForVariable == nullptr || !symbolTableEntryForVariable->optionalValueLookup.has_value()) {
        return;
    }

    const auto& signalValueLookup = *symbolTableEntryForVariable->optionalValueLookup;
    signalValueLookup->invalidateAllStoredValuesForSignal();
}


[[nodiscard]] std::set<std::string> SymbolTable::getUnusedLiterals() const {
    std::set<std::string> unusedLiterals;
    for (const auto& localKVPair : locals) {
        const auto& localSymbolTableEntry = localKVPair.second;
        if (localSymbolTableEntry->referenceCount == 0) {
            unusedLiterals.emplace(localKVPair.first);
        }
    }
    return unusedLiterals;
}

std::vector<bool> SymbolTable::determineIfModuleWasUsed(const syrec::Module::vec& modules) const {
    std::vector<bool> usageStatusPerModule(modules.size());
    std::transform(
            modules.cbegin(),
            modules.cend(),
            usageStatusPerModule.begin(),
            [&](const syrec::Module::ptr& moduleToCheck) {
                const auto foundModulesMatchingName = getEntryForModulesWithMatchingName(moduleToCheck->name);
                if (foundModulesMatchingName == nullptr) {
                    return false;
                }

                const auto& modulesMatchingName          = foundModulesMatchingName->matchingModules;
                const auto& foundModuleMatchingSignature = std::find_if(
                        modulesMatchingName.cbegin(),
                        modulesMatchingName.cend(),
                        [&moduleToCheck](const syrec::Module::ptr& moduleInSymbolTable) {
                            return doModuleSignaturesMatch(moduleInSymbolTable, moduleToCheck);
                        });

                if (foundModuleMatchingSignature == modulesMatchingName.end()) {
                    return false;
                }

                const std::size_t offsetForModuleInSymbolTableEntry = std::distance(modulesMatchingName.begin(), foundModuleMatchingSignature);
                const auto&       referenceCountsPerModule               = foundModulesMatchingName->referenceCountsPerModule;
                return referenceCountsPerModule.at(offsetForModuleInSymbolTableEntry) > 0;
            });
    return usageStatusPerModule;
}

std::optional<valueLookup::SignalValueLookup::ptr> SymbolTable::createBackupOfValueOfSignal(const std::string_view& literalIdent) const {
    if (const auto& symbolTableEntryForLiteral = getEntryForVariable(literalIdent); symbolTableEntryForLiteral != nullptr) {
        if (symbolTableEntryForLiteral->optionalValueLookup.has_value()) {
            return std::make_optional((*symbolTableEntryForLiteral->optionalValueLookup)->clone());
        }
    }
    return std::nullopt;
}

void SymbolTable::restoreValuesFromBackup(const std::string_view& literalIdent, const valueLookup::SignalValueLookup::ptr& newValues) const {
    if (const auto& symbolTableEntryForLiteral = getEntryForVariable(literalIdent); symbolTableEntryForLiteral != nullptr) {
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

void SymbolTable::incrementOrDecrementReferenceCountOfModulesMatchingSignature(const syrec::Module::ptr& module, bool incrementReferenceCount) const {
    const auto matchingModulesForName = getEntryForModulesWithMatchingName(module->name);
    if (matchingModulesForName == nullptr) {
        return;
    }

    const auto  numMatchingModulesForName = matchingModulesForName->matchingModules.size();
    const auto& currentlyStoredModules    = matchingModulesForName->matchingModules;
    auto&       referenceCountPerModule   = matchingModulesForName->referenceCountsPerModule;

    for (std::size_t moduleIdx = 0; moduleIdx < numMatchingModulesForName; ++moduleIdx) {
        if (referenceCountPerModule.at(moduleIdx) > 0) {
            continue;
        }

        if (doModuleSignaturesMatch(module, currentlyStoredModules.at(moduleIdx))) {
            auto& currentReferenceCountOfModule = referenceCountPerModule.at(moduleIdx);
            if (incrementReferenceCount) {
                if (currentReferenceCountOfModule != SIZE_MAX) {
                    currentReferenceCountOfModule++;
                }
            } else {
                currentReferenceCountOfModule = referenceCountPerModule.at(moduleIdx) == 0 ? 0 : referenceCountPerModule.at(moduleIdx) - 1;   
            }
        }
    }
}

bool SymbolTable::doesVariableAccessAllowValueLookup(const VariableSymbolTableEntry* symbolTableEntryForVariable, const syrec::VariableAccess::ptr& variableAccess) const {
    const auto isAssignedToVariableLoopVariable = std::holds_alternative<syrec::Number::ptr>(symbolTableEntryForVariable->variableInformation);
    if (isAssignedToVariableLoopVariable && (variableAccess->range.has_value() || !variableAccess->indexes.empty())) {
        return false;
    }

    if (!variableAccess->indexes.empty()) {
        if (std::any_of(
                    variableAccess->indexes.cbegin(),
                    variableAccess->indexes.cend(),
                    [](const syrec::expression::ptr& accessedValueOfDimensionExpr) {
                        if (const auto& numericExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(accessedValueOfDimensionExpr); numericExpr != nullptr) {
                            return !numericExpr->value->isConstant();
                        }
                        return true;
                    })) {
            return false;
        }
    } else {
        if (variableAccess->var->dimensions.size() > 1 || variableAccess->var->dimensions.front() > 1) {
            return false;
        }
    }

    return true;
}

std::optional<::optimizations::BitRangeAccessRestriction::BitRangeAccess> SymbolTable::tryTransformAccessedBitRange(const syrec::VariableAccess::ptr& accessedSignalParts) {
    const auto&                                                               accessedBitRange = accessedSignalParts->range;
    std::optional<::optimizations::BitRangeAccessRestriction::BitRangeAccess> transformedBitRangeAccess;
    if (accessedBitRange.has_value()) {
        unsigned int bitRangeStart = 0;
        unsigned int bitRangeEnd   = accessedSignalParts->var->bitwidth - 1;
        if (accessedBitRange->first->isConstant() && accessedBitRange->second->isConstant()) {
            bitRangeStart = accessedBitRange->first->evaluate({});
            bitRangeEnd   = accessedBitRange->second->evaluate({});
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
                    if (const auto& numericExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(accessedValueOfDimensionExpr); numericExpr != nullptr) {
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
    if (symbolTableEntryForLhsVariable == nullptr || !symbolTableEntryForLhsVariable->optionalValueLookup.has_value() 
        || symbolTableEntryForRhsVariable == nullptr || !symbolTableEntryForRhsVariable->optionalValueLookup.has_value()) {
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