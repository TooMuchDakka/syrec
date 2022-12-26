#include "core/syrec/parser/module_call_stack.hpp"

using namespace parser;

void ModuleCallStack::addNewEntry(const CallStackEntry::ptr& newCallStackEntry) const {
    callStack->push(newCallStackEntry);
}

bool ModuleCallStack::containsAnyUncalledModulesForNestingLevel(size_t currentNestingLevel) const {
    return !callStack->empty() && callStack->top()->nestingLevel == currentNestingLevel;
}

std::optional<ModuleCallStack::CallStackEntry::ptr> ModuleCallStack::getLastCalledModuleForNestingLevel(size_t currentNestingLevel) const {
    if (!containsAnyUncalledModulesForNestingLevel(currentNestingLevel)) {
        return std::nullopt;
    }

    return std::make_optional(callStack->top());
}

std::vector<ModuleCallStack::CallStackEntry::ptr> ModuleCallStack::popAllForCurrentNestingLevel(size_t currentNestingLevel) const {
    std::vector<CallStackEntry::ptr> poppedModules;

    while (containsAnyUncalledModulesForNestingLevel(currentNestingLevel)) {
        poppedModules.emplace_back(callStack->top());
        removeLastCalledModule();
    }
    std::reverse(poppedModules.begin(), poppedModules.end());
    return poppedModules;
}

void ModuleCallStack::removeLastCalledModule() const {
    if (!callStack->empty()) {
        callStack->pop();
    }
}



