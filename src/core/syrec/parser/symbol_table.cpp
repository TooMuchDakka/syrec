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

bool SymbolTable::addEntry(const syrec::Module::ptr& module) {
    if (nullptr == module) {
        return false;
    }

    if (!contains(module)) {
        modules.insert(std::pair(module->name, std::make_shared<ModuleSymbolTableEntry>(ModuleSymbolTableEntry(module))));
    } else {
        ModuleSymbolTableEntry& modulesWithMatchingName = *getEntryForModulesWithMatchingName(module->name);
        modulesWithMatchingName.matchingModules.emplace_back(module);
        modulesWithMatchingName.referenceCountsPerModule.emplace_back(0);
    }
    return true;
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
        foundSymbolTableEntryForLiteral->referenceCount = std::min(static_cast<std::size_t>(0), foundSymbolTableEntryForLiteral->referenceCount - 1);
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
                currentReferenceCountOfModule = std::min(static_cast<std::size_t>(0), referenceCountPerModule.at(moduleIdx) - 1);   
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
void SymbolTable::updateStoredValueFor(const syrec::VariableAccess::ptr& assignedToSignalParts, unsigned int newValue) const {
    const auto& symbolTableEntryForVariable = getEntryForVariable(assignedToSignalParts->var->name);
    if (symbolTableEntryForVariable == nullptr || !symbolTableEntryForVariable->optionalValueLookup.has_value()) {
        return;
    }

    const auto isAssignedToVariableLoopVariable = std::holds_alternative<syrec::Number::ptr>(symbolTableEntryForVariable->variableInformation);
    const auto& transformedDimensionAccess = tryTransformAccessedDimensions(assignedToSignalParts, isAssignedToVariableLoopVariable);
    const auto& transformedBitRangeAccess        = tryTransformAccessedBitRange(assignedToSignalParts);
    const auto& signalValueLookup                = *symbolTableEntryForVariable->optionalValueLookup;

    signalValueLookup->updateStoredValueFor(transformedDimensionAccess, transformedBitRangeAccess, newValue);
}
