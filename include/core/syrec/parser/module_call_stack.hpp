#ifndef MODULE_CALL_STACK_HPP
#define MODULE_CALL_STACK_HPP

#include <memory>
#include <optional>
#include <stack>
#include <string>
#include <vector>

namespace parser {
class ModuleCallStack {
public:
    struct CallStackEntry {
        using ptr = std::shared_ptr<CallStackEntry>;

        std::string              moduleIdent;
        std::vector<std::string> calleeArguments;
        size_t                   nestingLevel;
        std::pair<size_t, size_t> moduleIdentPosition;

        CallStackEntry(const std::string& moduleIdent, const std::vector<std::string>& calleeArguments, const size_t nestingLevel, const std::pair<size_t, size_t> moduleIdentPosition):
            moduleIdent(moduleIdent), calleeArguments(calleeArguments), nestingLevel(nestingLevel), moduleIdentPosition(moduleIdentPosition) {
        }
    };

    [[nodiscard]] bool                               containsAnyUncalledModulesForNestingLevel(size_t currentNestingLevel) const;
    [[nodiscard]] std::optional<CallStackEntry::ptr> getLastCalledModuleForNestingLevel(size_t currentNestingLevel) const;

    [[nodiscard]] std::vector<CallStackEntry::ptr>               popAllForCurrentNestingLevel(size_t currentNestingLevel) const;
    void                      addNewEntry(const CallStackEntry::ptr& newCallStackEntry) const;
    void                      removeLastCalledModule() const;

    ModuleCallStack() {
        callStack = std::make_unique<std::stack<CallStackEntry::ptr>>();
    }

private:
    std::unique_ptr<std::stack<CallStackEntry::ptr>> callStack;
};
} // namespace parser
#endif