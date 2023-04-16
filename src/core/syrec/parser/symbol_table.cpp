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
        modulesWithMatchingName.isUnusedFlagPerModule.emplace_back(true);
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

void SymbolTable::markLiteralAsUsed(const std::string_view& literalIdent) const {
    if (const auto& optionalSymbolTableEntryForLiteral = getEntryForVariable(literalIdent); optionalSymbolTableEntryForLiteral != nullptr) {
        optionalSymbolTableEntryForLiteral->isUnused = false;
    }
}

void SymbolTable::markModulesMatchingSignatureAsUsed(const syrec::Module::ptr& module) const {
    const auto matchingModulesForName = getEntryForModulesWithMatchingName(module->name);
    if (matchingModulesForName == nullptr) {
        return;
    }

    const auto              numMatchingModulesForName = matchingModulesForName->matchingModules.size();
    const auto& currentlyStoredModules    = matchingModulesForName->matchingModules;
    auto&                   unusedStatusPerModule     = matchingModulesForName->isUnusedFlagPerModule;

    for (std::size_t moduleIdx = 0; moduleIdx < numMatchingModulesForName; ++moduleIdx) {
        if (!unusedStatusPerModule.at(moduleIdx)) {
            continue;
        }

        if (doModuleSignaturesMatch(module, currentlyStoredModules.at(moduleIdx))) {
            unusedStatusPerModule.at(moduleIdx) = false;
        }
    }
}

[[nodiscard]] std::set<std::string> SymbolTable::getUnusedLiterals() const {
    std::set<std::string> unusedLiterals;
    for (const auto& localKVPair : locals) {
        const auto& localSymbolTableEntry = localKVPair.second;
        if (localSymbolTableEntry->isUnused) {
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
                const auto&       isUsedFlagPerModule               = foundModulesMatchingName->isUnusedFlagPerModule;
                return !isUsedFlagPerModule.at(offsetForModuleInSymbolTableEntry);
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

