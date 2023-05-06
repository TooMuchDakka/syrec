#include "core/syrec/parser/assignment_signal_access_restriction.hpp"

using namespace parser;

bool AssignmentSignalAccessRestriction::isAccessRestrictedTo(const syrec::VariableAccess::ptr& accessedPartsOfSignal) const {
    return false;
}


std::pair<unsigned, unsigned> AssignmentSignalAccessRestriction::initializeRestrictedBitRange(const syrec::VariableAccess::ptr& assignedToPartsOfSignal) {
    unsigned int accessedBitRangeStart = 0;
    unsigned int accessedBitRangeEnd   = assignedToPartsOfSignal->var->bitwidth - 1;
    if (assignedToPartsOfSignal->range->first->isConstant() && assignedToPartsOfSignal->range->second->isConstant()) {
        accessedBitRangeStart = assignedToPartsOfSignal->range->first->evaluate({});
        accessedBitRangeEnd   = assignedToPartsOfSignal->range->second->evaluate({});
    }
    return std::make_pair(accessedBitRangeStart, accessedBitRangeEnd);
}

std::vector<std::optional<unsigned>> AssignmentSignalAccessRestriction::initializedRestrictedValuesPerDimensions(const syrec::VariableAccess::ptr& assignedToPartsOfSignal) {
    std::vector<std::optional<unsigned int>> lockedValuesPerDimension(assignedToPartsOfSignal->var->dimensions.size(), std::nullopt);

    std::size_t dimensionIdx = 0;
    for (const auto& accessedValueOfDimension: assignedToPartsOfSignal->indexes) {
        if (const auto& numericExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(accessedValueOfDimension); numericExpr != nullptr && numericExpr->value->isConstant()) {
            lockedValuesPerDimension[dimensionIdx] = numericExpr->value->evaluate({});
        }
        dimensionIdx++;
    }
    return lockedValuesPerDimension;
}