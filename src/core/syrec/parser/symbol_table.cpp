#include "core/syrec/parser/symbol_table.hpp"

#include <functional>

using namespace parser;
bool SymbolTable::contains(const std::string_view& literalIdent) const {
    return this->locals.find(literalIdent) != this->locals.end() || (nullptr != outer && outer->contains(literalIdent));
}

bool SymbolTable::contains(const syrec::Module::ptr& module) const {
    if (nullptr == module) {
        return false;
    }

    bool       foundModuleMatchingSignature    = false;
    const auto module_search_result = this->modules.find(module->name);
    if (module_search_result != this->modules.end()) {
        const syrec::Module::vec modulesWithMatchingName = module_search_result->second;

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
        found_entry.emplace(this->locals.find(literalIdent)->second);
    }
    else if (nullptr != outer) {
        return outer->getVariable(literalIdent);
    }
    return found_entry;
}

std::optional<syrec::Module::vec> SymbolTable::getMatchingModulesForName(const std::string_view& moduleName) const {
    std::optional<syrec::Module::vec> matchingModules;
    if (this->modules.find(moduleName) != this->modules.end()) {
        matchingModules.emplace(this->modules.find(moduleName)->second);
    } else if (nullptr != outer) {
        matchingModules = outer->getMatchingModulesForName(moduleName);
    }
    return matchingModules;
}

bool SymbolTable::addEntry(const syrec::Variable::ptr& variable) {
    if (nullptr == variable || variable->name.empty() || contains(variable->name)) {
        return false;
    }
    
    const std::variant<syrec::Variable::ptr, syrec::Number::ptr> symbolTableEntryValue (variable);
    const std::pair symbolTableEntry(variable->name, symbolTableEntryValue);
    locals.insert(symbolTableEntry);
    return true;
}

bool SymbolTable::addEntry(const syrec::Number::ptr& number) {
    if (nullptr == number || !number->isLoopVariable() || number->variableName().empty() || contains(number->variableName())) {
        return false;
    }

    const std::variant<syrec::Variable::ptr, syrec::Number::ptr> symbolTableEntryValue(number);
    const std::pair                                symbolTableEntry(number->variableName(), symbolTableEntryValue);
    locals.insert(symbolTableEntry);
    return true;
}

bool SymbolTable::addEntry(const syrec::Module::ptr& module) {
    if (nullptr == module) {
        return false;
    }

    if (!contains(module)) {
        modules.insert(std::pair<std::string, std::vector<std::shared_ptr<syrec::Module>>>(module->name, syrec::Module::vec()));
    }
    
    std::vector<syrec::Module::ptr> &modulesWithMatchingName = modules.find(module->name)->second;
    modulesWithMatchingName.emplace_back(module);
    return true;
}

void SymbolTable::openScope(SymbolTable::ptr& currentScope) {
    // TODO: Implement me
    const SymbolTable::ptr newScope(new SymbolTable());
    newScope->outer = currentScope;
    currentScope    = newScope;
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

bool SymbolTable::isAssignableTo(const syrec::Variable::ptr& formalParameter, const syrec::Variable::ptr& actualParameter, bool mustParameterTypesMatch) {
    return (mustParameterTypesMatch ? formalParameter->type == actualParameter->type : true) 
        && formalParameter->dimensions == actualParameter->dimensions
        && formalParameter->bitwidth == actualParameter->bitwidth;
}
