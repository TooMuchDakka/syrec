#ifndef ASSIGNMENT_SIGNAL_ACCESS_RESTRICTION_HPP
#define ASSIGNMENT_SIGNAL_ACCESS_RESTRICTION_HPP

#include "core/syrec/expression.hpp"
#include "core/syrec/variable.hpp"

#include <optional>
#include <vector>

namespace parser {
    class AssignmentSignalAccessRestriction {
    public:
        explicit AssignmentSignalAccessRestriction(const syrec::VariableAccess::ptr& assignedToPartsOfSignal):
            assignedToSignal(assignedToPartsOfSignal->var->name),
            restrictedBitRange(initializeRestrictedBitRange(assignedToPartsOfSignal)),
            lockedValuePerDimension(initializedRestrictedValuesPerDimensions(assignedToPartsOfSignal)) {
        }

        [[nodiscard]] bool isAccessRestrictedTo(const syrec::VariableAccess::ptr& accessedPartsOfSignal) const;

    private:
        const std::string                                          assignedToSignal;
        const std::pair<unsigned int, unsigned int> restrictedBitRange;
        const std::vector<std::optional<unsigned int>>             lockedValuePerDimension;

        static std::pair<unsigned int, unsigned int> initializeRestrictedBitRange(const syrec::VariableAccess::ptr& assignedToPartsOfSignal);
        static std::vector<std::optional<unsigned int>> initializedRestrictedValuesPerDimensions(const syrec::VariableAccess::ptr& assignedToPartsOfSignal);
    };
}
#endif