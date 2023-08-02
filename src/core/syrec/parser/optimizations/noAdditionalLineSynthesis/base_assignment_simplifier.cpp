#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/base_assignment_simplifier.hpp"

#include "core/syrec/parser/operation.hpp"

using namespace noAdditionalLineSynthesis;

BaseAssignmentSimplifier::~BaseAssignmentSimplifier() = default;

syrec::Statement::vec BaseAssignmentSimplifier::simplify(const syrec::AssignStatement::ptr& assignmentStmt) {
    const auto& assignStmtCasted = std::dynamic_pointer_cast<syrec::AssignStatement>(assignmentStmt);
    if (assignStmtCasted == nullptr || !simplificationPrecondition(assignmentStmt)) {
        return {};
    }
    return simplifyWithoutPreconditionCheck(assignmentStmt);
}

std::optional<bool> BaseAssignmentSimplifier::isEveryLhsOperandOfAnyBinaryExprDefinedOnceOnEveryLevelOfTheAST(const syrec::BinaryExpression::ptr& rootExpr) const {
    RestrictionMap definedSignalAccesses = RestrictionMap();
    return isEveryLhsOperandOfAnyBinaryExprDefinedOnceOnEveryLevelOfTheAST(rootExpr, true, definedSignalAccesses);
}

std::optional<bool> BaseAssignmentSimplifier::isEveryLhsOperandOfAnyBinaryExprDefinedOnceOnEveryLevelOfTheAST(const syrec::BinaryExpression::ptr& expr, bool isRootExpr, RestrictionMap& notUsableSignals) const {
    if (isRootExpr && !doesExprDefineNestedExpr(expr)) {
        return std::make_optional(false);
    }

    if (const auto& exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr); exprAsBinaryExpr != nullptr) {
        if (!doesExprDefineNestedExpr(exprAsBinaryExpr->lhs) && !doesExprDefineNestedExpr(exprAsBinaryExpr->rhs) && isExprConstantNumber(exprAsBinaryExpr->lhs) && isExprConstantNumber(exprAsBinaryExpr->rhs)) {
            return std::nullopt;
        }

        const auto& variableAccessOfLhs = doesExprDefineVariableAccess(exprAsBinaryExpr->lhs) ? std::make_optional(std::dynamic_pointer_cast<syrec::VariableExpression>(exprAsBinaryExpr->lhs)->var) : std::nullopt;
        const auto& variableAccessOfRhs = doesExprDefineVariableAccess(exprAsBinaryExpr->rhs) ? std::make_optional(std::dynamic_pointer_cast<syrec::VariableExpression>(exprAsBinaryExpr->rhs)->var) : std::nullopt;
        if ((variableAccessOfLhs.has_value() && wasAccessOnSignalAlreadyDefined(*variableAccessOfLhs, notUsableSignals)) || (variableAccessOfRhs.has_value() && wasAccessOnSignalAlreadyDefined(*variableAccessOfRhs, notUsableSignals)) || (variableAccessOfLhs.has_value() && variableAccessOfRhs.has_value() && doAccessedSignalPartsOverlap(*variableAccessOfLhs, *variableAccessOfRhs))) {
            return std::make_optional(false);
        }

        if (variableAccessOfLhs.has_value()) {
            markSignalAccessAsNotUsableInExpr(*variableAccessOfLhs, notUsableSignals);
        }

        if (variableAccessOfRhs.has_value()) {
            markSignalAccessAsNotUsableInExpr(*variableAccessOfRhs, notUsableSignals);
        }

        std::optional<bool> nestedExprOk = std::make_optional(true);
        if (doesExprDefineNestedExpr(exprAsBinaryExpr->lhs)) {
            nestedExprOk = isEveryLhsOperandOfAnyBinaryExprDefinedOnceOnEveryLevelOfTheAST(exprAsBinaryExpr->lhs, false, notUsableSignals);
        }
        if (nestedExprOk.has_value() && *nestedExprOk && doesExprDefineNestedExpr(exprAsBinaryExpr->rhs)) {
            nestedExprOk = isEveryLhsOperandOfAnyBinaryExprDefinedOnceOnEveryLevelOfTheAST(exprAsBinaryExpr->rhs, false, notUsableSignals);
        }
        return nestedExprOk;
    }
    return std::nullopt;
}

bool BaseAssignmentSimplifier::isExprConstantNumber(const syrec::expression::ptr& expr) {
    if (const auto& exprAsNumericExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(expr); exprAsNumericExpr != nullptr) {
        return exprAsNumericExpr->value->isConstant();
    }
    return false;
}

bool BaseAssignmentSimplifier::doesExprDefineVariableAccess(const syrec::expression::ptr& expr) {
    return std::dynamic_pointer_cast<syrec::VariableExpression>(expr) != nullptr;
}

bool BaseAssignmentSimplifier::doesExprDefineNestedExpr(const syrec::expression::ptr& expr) {
    return std::dynamic_pointer_cast<syrec::BinaryExpression>(expr) != nullptr;
}

bool BaseAssignmentSimplifier::wasAccessOnSignalAlreadyDefined(const syrec::VariableAccess::ptr& accessedSignalParts, const RestrictionMap& notUsableSignals) const {
    if (notUsableSignals.count(accessedSignalParts->var->name) == 0) {
        return false;
    }

    const auto& alreadyDefinedAccessesForSignal = notUsableSignals.at(accessedSignalParts->var->name);
    return std::none_of(
            alreadyDefinedAccessesForSignal.cbegin(),
            alreadyDefinedAccessesForSignal.cend(),
            [&](const syrec::VariableAccess::ptr& alreadyDefinedSignalAccess) {
                return doAccessedSignalPartsOverlap(accessedSignalParts, alreadyDefinedSignalAccess);
            });
}

bool BaseAssignmentSimplifier::doAccessedSignalPartsOverlap(const syrec::VariableAccess::ptr& accessedSignalPartsOfLhs, const syrec::VariableAccess::ptr& accessedSignalPartsOfRhs) const {
    if (accessedSignalPartsOfLhs->var->name != accessedSignalPartsOfRhs->var->name) {
        return false;
    }

    const auto& numDimensionsToCheck = std::min(
            accessedSignalPartsOfLhs->indexes.empty() ? 0 : accessedSignalPartsOfLhs->indexes.size(),
            accessedSignalPartsOfRhs->indexes.empty() ? 0 : accessedSignalPartsOfRhs->indexes.size());

    const bool didDimensionAccessesMatch = std::find_first_of(
                                                   accessedSignalPartsOfLhs->indexes.cbegin(),
                                                   std::prev(accessedSignalPartsOfLhs->indexes.cbegin(), numDimensionsToCheck),
                                                   accessedSignalPartsOfRhs->indexes.cbegin(),
                                                   std::prev(accessedSignalPartsOfRhs->indexes.cbegin(), numDimensionsToCheck),
                                                   [](const syrec::expression::ptr& accessedValueOfDimensionOfLhs, const syrec::expression::ptr& accessedValueOfDimensionOfRhs) {
                                                       const auto& evaluatedValueOfDimensionOfLhs = tryFetchValueOfExpr(accessedValueOfDimensionOfLhs);
                                                       const auto& evaluatedValueOfDimensionOfRhs = tryFetchValueOfExpr(accessedValueOfDimensionOfRhs);
                                                       if (evaluatedValueOfDimensionOfLhs.has_value() && evaluatedValueOfDimensionOfRhs.has_value()) {
                                                           return *evaluatedValueOfDimensionOfLhs != *evaluatedValueOfDimensionOfRhs;
                                                       }
                                                       return true;
                                                   }) == accessedSignalPartsOfLhs->indexes.cend();
    return didDimensionAccessesMatch && doAccessedBitRangesOverlap(accessedSignalPartsOfLhs, accessedSignalPartsOfRhs, true);
}

bool BaseAssignmentSimplifier::doAccessedBitRangesOverlap(const syrec::VariableAccess::ptr& accessedSignalParts, const syrec::VariableAccess::ptr& potentiallyEnclosingSignalAccess, bool shouldAccessedBitRangeBeFullyEnclosed) const {
    const auto& accessedBitRange  = transformUserDefinedBitRangeAccess(accessedSignalParts);
    const auto& referenceBitRange = transformUserDefinedBitRangeAccess(potentiallyEnclosingSignalAccess);
    if (!accessedBitRange.has_value() || !referenceBitRange.has_value()) {
        return false;
    }

    if (shouldAccessedBitRangeBeFullyEnclosed) {
        return accessedBitRange->first >= referenceBitRange->first && accessedBitRange->second <= referenceBitRange->second;
    }

    const bool doesAssignedToBitRangePrecedeAccessedBitRange = accessedBitRange->first < referenceBitRange->first;
    const bool doesAssignedToBitRangeExceedAccessedBitRange  = accessedBitRange->second > referenceBitRange->second;
    /*
     * Check cases:
     * Assigned TO: |---|
     * Accessed BY:      |---|
     *
     * and
     * Assigned TO:      |---|
     * Accessed BY: |---|
     *
     */
    if ((doesAssignedToBitRangePrecedeAccessedBitRange && accessedBitRange->second < referenceBitRange->first) || (doesAssignedToBitRangeExceedAccessedBitRange && accessedBitRange->first > referenceBitRange->second)) {
        return false;
    }
    return true;
}

bool BaseAssignmentSimplifier::wasAnyAccessedSignalPartPreviouslyNotAccessed(const syrec::VariableAccess::ptr& accessedSignalParts, const RestrictionMap& previouslyAccessedSignalPartsLookup) const {
    const auto& accessedSignalIdent = accessedSignalParts->var->name;
    if (previouslyAccessedSignalPartsLookup.count(accessedSignalIdent) == 0) {
        return true;
    }

    const auto& previouslyAccessedSignalPartsInLookup = previouslyAccessedSignalPartsLookup.at(accessedSignalIdent);
    return std::all_of(
            previouslyAccessedSignalPartsInLookup.cbegin(),
            previouslyAccessedSignalPartsInLookup.cend(),
            [&](const syrec::VariableAccess::ptr& previouslyAccessedSignalParts) {
                return doAccessedSignalPartsOverlap(accessedSignalParts, previouslyAccessedSignalParts) ? !doAccessedBitRangesOverlap(accessedSignalParts, previouslyAccessedSignalParts, false) : true;
            });
}

void BaseAssignmentSimplifier::markSignalAccessAsNotUsableInExpr(const syrec::VariableAccess::ptr& accessedSignalParts, RestrictionMap& notUsableSignals) const {
    if (!wasAnyAccessedSignalPartPreviouslyNotAccessed(accessedSignalParts, notUsableSignals)) {
        return;
    }

    const auto& accessedSignalIdent = accessedSignalParts->var->name;
    if (notUsableSignals.count(accessedSignalIdent) == 0) {
        notUsableSignals.insert(std::make_pair(accessedSignalIdent, std::vector<syrec::VariableAccess::ptr>(1, accessedSignalParts)));
    } else {
        notUsableSignals.at(accessedSignalIdent).emplace_back(accessedSignalParts);
    }
}

void BaseAssignmentSimplifier::invertAssignments(syrec::Statement::vec& assignmentsToInvert, bool excludeLastAssignment) {
    syrec::Statement::vec invertedAssignments;
    const auto&           firstAssignmentToInvert = assignmentsToInvert.cbegin();
    const auto&           lastAssignmentToInvert  = excludeLastAssignment ? std::prev(assignmentsToInvert.cend()) : assignmentsToInvert.cend();

    std::transform(
    firstAssignmentToInvert,
    lastAssignmentToInvert,
    std::back_inserter(assignmentsToInvert),
    [](const syrec::AssignStatement::ptr& assignmentStmt) {
        const auto assignmentCasted = std::dynamic_pointer_cast<syrec::AssignStatement>(assignmentStmt);
        return std::make_shared<syrec::AssignStatement>(
                assignmentCasted->lhs,
                *syrec_operation::tryMapAssignmentOperationEnumToFlag(*syrec_operation::invert(*syrec_operation::tryMapAssignmentOperationFlagToEnum(assignmentCasted->op))),
                assignmentCasted->rhs);
    });
}

std::optional<unsigned int> BaseAssignmentSimplifier::tryFetchValueOfNumber(const syrec::Number::ptr& number) {
    return number->isConstant() ? std::make_optional(number->evaluate({})) : std::nullopt;
}

std::optional<unsigned> BaseAssignmentSimplifier::tryFetchValueOfExpr(const syrec::expression::ptr& expr) {
    if (const auto& exprAsNumericExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(expr); exprAsNumericExpr != nullptr) {
        return tryFetchValueOfNumber(exprAsNumericExpr->value);
    }
    return std::nullopt;
}

std::optional<unsigned int> BaseAssignmentSimplifier::fetchBitWidthOfSignal(const std::string_view& signalIdent) const {
    if (const auto& symbolTableEntryForSignal = fetchSymbolTableEntryForSignalAccess(signalIdent); symbolTableEntryForSignal.has_value()) {
        return std::make_optional(symbolTableEntryForSignal.value()->bitwidth);
    }
    return std::nullopt;
}

std::optional<syrec::Variable::ptr> BaseAssignmentSimplifier::fetchSymbolTableEntryForSignalAccess(const std::string_view& signalIdent) const {
    if (const auto& symbolTableEntryForSignal = symbolTable->getVariable(signalIdent); symbolTableEntryForSignal.has_value() && std::holds_alternative<syrec::Variable::ptr>(*symbolTableEntryForSignal)) {
        return std::make_optional(std::get<syrec::Variable::ptr>(*symbolTableEntryForSignal));
    }
    return std::nullopt;
}

std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess> BaseAssignmentSimplifier::transformUserDefinedBitRangeAccess(const syrec::VariableAccess::ptr& accessedSignalParts) const {
    if (!accessedSignalParts->range.has_value() || !tryFetchValueOfNumber(accessedSignalParts->range->first).has_value() || !tryFetchValueOfNumber(accessedSignalParts->range->second).has_value()) {
        const auto& bitWidthOfAccessedSignal = fetchBitWidthOfSignal(accessedSignalParts->var->name);
        if (bitWidthOfAccessedSignal.has_value()) {
            return std::make_optional(optimizations::BitRangeAccessRestriction::BitRangeAccess(0, *bitWidthOfAccessedSignal - 1));
        }
    }
    return std::make_optional(optimizations::BitRangeAccessRestriction::BitRangeAccess(*tryFetchValueOfNumber(accessedSignalParts->range->first), *tryFetchValueOfNumber(accessedSignalParts->range->second) - 1));
}