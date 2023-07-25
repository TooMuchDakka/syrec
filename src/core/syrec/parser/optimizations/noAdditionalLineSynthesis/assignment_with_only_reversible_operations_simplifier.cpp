#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/assignment_with_only_reversible_operations_simplifier.hpp"

using namespace noAdditionalLineSynthesis;

syrec::Statement::vec AssignmentWithOnlyReversibleOperationsSimplifier::simplifyWithoutPreconditionCheck(const syrec::AssignStatement::ptr& assignmentStmt) {
    const auto& assignStmtCasted = std::dynamic_pointer_cast<syrec::AssignStatement>(assignmentStmt);
    if (auto simplifiedRhsExpr = simplifyWithoutPreconditionCheck(assignStmtCasted->rhs); !simplifiedRhsExpr.empty()) {
        const auto& lastAssignedToSignalInRhs = std::dynamic_pointer_cast<syrec::AssignStatement>(simplifiedRhsExpr.back())->lhs;

        const auto topMostAssignment = std::make_shared<syrec::AssignStatement>(
                assignStmtCasted->lhs,
                assignStmtCasted->op,
                std::make_shared<syrec::VariableExpression>(lastAssignedToSignalInRhs));
        simplifiedRhsExpr.emplace_back(topMostAssignment);
        return simplifiedRhsExpr;
    }
    return {};
}

syrec::Statement::vec AssignmentWithOnlyReversibleOperationsSimplifier::simplifyWithoutPreconditionCheck(const syrec::BinaryExpression::ptr& expr) {
    const auto& exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr);
    if (exprAsBinaryExpr == nullptr) {
        return {};
    }

    syrec::Statement::vec generatedAssignments;
    auto&                 lhsOperandOfExpression = exprAsBinaryExpr->lhs;
    auto&                 rhsOperandOfExpression = exprAsBinaryExpr->rhs;

    if (doesExprDefineNestedExpr(exprAsBinaryExpr->lhs)) {
        generatedAssignments       = simplifyWithoutPreconditionCheck(exprAsBinaryExpr->lhs);
        bool simplificationOfLhsOK = false;
        if (!generatedAssignments.empty()) {
            if (const auto& finalAssignmentForLhsExpr = std::dynamic_pointer_cast<syrec::AssignStatement>(generatedAssignments.back()); finalAssignmentForLhsExpr != nullptr) {
                lhsOperandOfExpression = std::make_shared<syrec::VariableExpression>(finalAssignmentForLhsExpr->lhs);
                simplificationOfLhsOK  = true;
            }
        }

        if (!simplificationOfLhsOK) {
            return {};
        }
    }

    if (doesExprDefineNestedExpr(exprAsBinaryExpr->rhs)) {
        const auto& generatedAssignmentsForRhsExpr = simplifyWithoutPreconditionCheck(exprAsBinaryExpr->rhs);
        bool        simplificationOfRhsOK          = false;
        if (!generatedAssignmentsForRhsExpr.empty()) {
            generatedAssignments.insert(generatedAssignments.end(), generatedAssignmentsForRhsExpr.begin(), generatedAssignmentsForRhsExpr.end());
            if (const auto& finalAssignmentForRhsExpr = std::dynamic_pointer_cast<syrec::AssignStatement>(generatedAssignmentsForRhsExpr.back()); finalAssignmentForRhsExpr != nullptr) {
                rhsOperandOfExpression = std::make_shared<syrec::VariableExpression>(finalAssignmentForRhsExpr->lhs);
                simplificationOfRhsOK  = true;
            }
        }

        if (!simplificationOfRhsOK) {
            return {};
        }
    }

    if (isExprConstantNumber(lhsOperandOfExpression) && doesExprDefineVariableAccess(rhsOperandOfExpression)) {
        const auto backupOfRhsOperand = rhsOperandOfExpression;
        rhsOperandOfExpression        = lhsOperandOfExpression;
        lhsOperandOfExpression        = backupOfRhsOperand;
    } else if (isExprConstantNumber(lhsOperandOfExpression) && isExprConstantNumber(rhsOperandOfExpression)) {
        return {};
    }

    const auto generatedAssignmentStatement = std::make_shared<syrec::AssignStatement>(
            std::dynamic_pointer_cast<syrec::VariableExpression>(lhsOperandOfExpression)->var,
            exprAsBinaryExpr->op,
            rhsOperandOfExpression);
    generatedAssignments.emplace_back(generatedAssignmentStatement);
    return generatedAssignments;
}

bool AssignmentWithOnlyReversibleOperationsSimplifier::simplificationPrecondition(const syrec::AssignStatement::ptr& assignmentStmt) {
    const auto& assignStmtCasted = std::dynamic_pointer_cast<syrec::AssignStatement>(assignmentStmt);
    if (assignStmtCasted == nullptr) {
        return {};
    }

    const auto preconditionStatus = isEveryLhsOperandOfAnyBinaryExprDefinedOnceOnEveryLevelOfTheAST(assignStmtCasted->rhs);
    return preconditionStatus.has_value() && *preconditionStatus;
}

//
//std::optional<bool> AssignmentWithOnlyReversibleOperationsSimplifier::isEveryLhsOperandOfAnyBinaryExprDefinedOnceOnEveryLevelOfTheAST(const syrec::BinaryExpression::ptr& rootExpr) const {
//    RestrictionMap definedSignalAccesses = RestrictionMap();
//    return isEveryLhsOperandOfAnyBinaryExprDefinedOnceOnEveryLevelOfTheAST(rootExpr, true, definedSignalAccesses);
//}
//
//std::optional<bool> AssignmentWithOnlyReversibleOperationsSimplifier::isEveryLhsOperandOfAnyBinaryExprDefinedOnceOnEveryLevelOfTheAST(const syrec::BinaryExpression::ptr& expr, bool isRootExpr, RestrictionMap& notUsableSignals) const {
//    if (isRootExpr && !doesExprDefineNestedExpr(expr)) {
//        return std::make_optional(false);
//    }
//
//    if (const auto& exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr); exprAsBinaryExpr != nullptr) {
//        if (!doesExprDefineNestedExpr(exprAsBinaryExpr->lhs) && !doesExprDefineNestedExpr(exprAsBinaryExpr->rhs) && isExprConstantNumber(exprAsBinaryExpr->lhs) && isExprConstantNumber(exprAsBinaryExpr->rhs)) {
//            return std::nullopt;
//        }
//
//        const auto& variableAccessOfLhs = doesExprDefineVariableAccess(exprAsBinaryExpr->lhs) ? std::make_optional(std::dynamic_pointer_cast<syrec::VariableExpression>(exprAsBinaryExpr->lhs)->var) : std::nullopt;
//        const auto& variableAccessOfRhs = doesExprDefineVariableAccess(exprAsBinaryExpr->rhs) ? std::make_optional(std::dynamic_pointer_cast<syrec::VariableExpression>(exprAsBinaryExpr->rhs)->var) : std::nullopt;
//        if ((variableAccessOfLhs.has_value() && wasAccessOnSignalAlreadyDefined(*variableAccessOfLhs, notUsableSignals)) || (variableAccessOfRhs.has_value() && wasAccessOnSignalAlreadyDefined(*variableAccessOfRhs, notUsableSignals)) || (variableAccessOfLhs.has_value() && variableAccessOfRhs.has_value() && doAccessedSignalPartsOverlap(*variableAccessOfLhs, *variableAccessOfRhs))) {
//            return std::make_optional(false);
//        }
//
//        if (variableAccessOfLhs.has_value()) {
//            markSignalAccessAsNotUsableInExpr(*variableAccessOfLhs, notUsableSignals);
//        }
//
//        if (variableAccessOfRhs.has_value()) {
//            markSignalAccessAsNotUsableInExpr(*variableAccessOfRhs, notUsableSignals);
//        }
//
//        std::optional<bool> nestedExprOk = std::make_optional(true);
//        if (doesExprDefineNestedExpr(exprAsBinaryExpr->lhs)) {
//            nestedExprOk = isEveryLhsOperandOfAnyBinaryExprDefinedOnceOnEveryLevelOfTheAST(exprAsBinaryExpr->lhs, false, notUsableSignals);
//        }
//        if (nestedExprOk.has_value() && *nestedExprOk && doesExprDefineNestedExpr(exprAsBinaryExpr->rhs)) {
//            nestedExprOk = isEveryLhsOperandOfAnyBinaryExprDefinedOnceOnEveryLevelOfTheAST(exprAsBinaryExpr->rhs, false, notUsableSignals);
//        }
//        return nestedExprOk;
//    }
//    return std::nullopt;
//}
//
//bool AssignmentWithOnlyReversibleOperationsSimplifier::isExprConstantNumber(const syrec::expression::ptr& expr) {
//    if (const auto& exprAsNumericExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(expr); exprAsNumericExpr != nullptr) {
//        return exprAsNumericExpr->value->isConstant();
//    }
//    return false;
//}
//
//bool AssignmentWithOnlyReversibleOperationsSimplifier::doesExprDefineVariableAccess(const syrec::expression::ptr& expr) {
//    return std::dynamic_pointer_cast<syrec::VariableExpression>(expr) != nullptr;
//}
//
//bool AssignmentWithOnlyReversibleOperationsSimplifier::doesExprDefineNestedExpr(const syrec::expression::ptr& expr) {
//    return std::dynamic_pointer_cast<syrec::BinaryExpression>(expr) != nullptr;
//}
//
//bool AssignmentWithOnlyReversibleOperationsSimplifier::wasAccessOnSignalAlreadyDefined(const syrec::VariableAccess::ptr& accessedSignalParts, const RestrictionMap& notUsableSignals) const {
//    if (notUsableSignals.count(accessedSignalParts->var->name) == 0) {
//        return false;
//    }
//
//    const auto& alreadyDefinedAccessesForSignal = notUsableSignals.at(accessedSignalParts->var->name);
//    return std::none_of(
//alreadyDefinedAccessesForSignal.cbegin(),
//   alreadyDefinedAccessesForSignal.cend(),
//   [&](const syrec::VariableAccess::ptr& alreadyDefinedSignalAccess) {
//       return doAccessedSignalPartsOverlap(accessedSignalParts, alreadyDefinedSignalAccess);
//   });
//}
//
//bool AssignmentWithOnlyReversibleOperationsSimplifier::doAccessedSignalPartsOverlap(const syrec::VariableAccess::ptr& accessedSignalPartsOfLhs, const syrec::VariableAccess::ptr& accessedSignalPartsOfRhs) const {
//    if (accessedSignalPartsOfLhs->var->name != accessedSignalPartsOfRhs->var->name) {
//        return false;
//    }
//
//    const auto& numDimensionsToCheck = std::min(
//            accessedSignalPartsOfLhs->indexes.empty() ? 0 : accessedSignalPartsOfLhs->indexes.size(),
//            accessedSignalPartsOfRhs->indexes.empty() ? 0 : accessedSignalPartsOfRhs->indexes.size());
//
//    const bool didDimensionAccessesMatch = std::find_first_of(
//   accessedSignalPartsOfLhs->indexes.cbegin(),
//   std::prev(accessedSignalPartsOfLhs->indexes.cbegin(), numDimensionsToCheck),
//   accessedSignalPartsOfRhs->indexes.cbegin(),
//   std::prev(accessedSignalPartsOfRhs->indexes.cbegin(), numDimensionsToCheck),
//   [](const syrec::expression::ptr& accessedValueOfDimensionOfLhs, const syrec::expression::ptr& accessedValueOfDimensionOfRhs) {
//       const auto& evaluatedValueOfDimensionOfLhs = tryFetchValueOfExpr(accessedValueOfDimensionOfLhs);
//       const auto& evaluatedValueOfDimensionOfRhs = tryFetchValueOfExpr(accessedValueOfDimensionOfRhs);
//       if (evaluatedValueOfDimensionOfLhs.has_value() && evaluatedValueOfDimensionOfRhs.has_value()) {
//           return *evaluatedValueOfDimensionOfLhs != *evaluatedValueOfDimensionOfRhs;
//       }
//       return true;
//   }) == accessedSignalPartsOfLhs->indexes.cend();
//    return didDimensionAccessesMatch && doAccessedBitRangesOverlap(accessedSignalPartsOfLhs, accessedSignalPartsOfRhs, true);
//}
//
//bool AssignmentWithOnlyReversibleOperationsSimplifier::doAccessedBitRangesOverlap(const syrec::VariableAccess::ptr& accessedSignalParts, const syrec::VariableAccess::ptr& potentiallyEnclosingSignalAccess, bool shouldAccessedBitRangeBeFullyEnclosed) const {
//    const auto& accessedBitRange  = transformUserDefinedBitRangeAccess(accessedSignalParts);
//    const auto& referenceBitRange = transformUserDefinedBitRangeAccess(potentiallyEnclosingSignalAccess);
//    if (!accessedBitRange.has_value() || !referenceBitRange.has_value()) {
//        return false;
//    }
//
//    if (shouldAccessedBitRangeBeFullyEnclosed) {
//        return accessedBitRange->first >= referenceBitRange->first && accessedBitRange->second <= referenceBitRange->second;   
//    }
//
//    const bool doesAssignedToBitRangePrecedeAccessedBitRange = accessedBitRange->first < referenceBitRange->first;
//    const bool doesAssignedToBitRangeExceedAccessedBitRange  = accessedBitRange->second > referenceBitRange->second;
//    /*
//     * Check cases:
//     * Assigned TO: |---|
//     * Accessed BY:      |---|
//     *
//     * and
//     * Assigned TO:      |---|
//     * Accessed BY: |---|
//     *
//     */
//    if ((doesAssignedToBitRangePrecedeAccessedBitRange && accessedBitRange->second < referenceBitRange->first) || (doesAssignedToBitRangeExceedAccessedBitRange && accessedBitRange->first > referenceBitRange->second)) {
//        return false;
//    }
//    return true;
//}
//
//bool AssignmentWithOnlyReversibleOperationsSimplifier::wasAnyAccessedSignalPartPreviouslyNotAccessed(const syrec::VariableAccess::ptr& accessedSignalParts, const RestrictionMap& previouslyAccessedSignalPartsLookup) const {
//    const auto& accessedSignalIdent = accessedSignalParts->var->name;
//    if (previouslyAccessedSignalPartsLookup.count(accessedSignalIdent) == 0) {
//        return true;
//    }
//
//    const auto& previouslyAccessedSignalPartsInLookup = previouslyAccessedSignalPartsLookup.at(accessedSignalIdent);
//    return std::all_of(
//    previouslyAccessedSignalPartsInLookup.cbegin(),
//    previouslyAccessedSignalPartsInLookup.cend(),
//    [&](const syrec::VariableAccess::ptr& previouslyAccessedSignalParts) {
//        return doAccessedSignalPartsOverlap(accessedSignalParts, previouslyAccessedSignalParts) ? !doAccessedBitRangesOverlap(accessedSignalParts, previouslyAccessedSignalParts, false) : true;
//    });
//}
//
//void AssignmentWithOnlyReversibleOperationsSimplifier::markSignalAccessAsNotUsableInExpr(const syrec::VariableAccess::ptr& accessedSignalParts, RestrictionMap& notUsableSignals) const {
//    if (!wasAnyAccessedSignalPartPreviouslyNotAccessed(accessedSignalParts, notUsableSignals)) {
//        return;
//    }
//
//    const auto& accessedSignalIdent = accessedSignalParts->var->name;
//    if (notUsableSignals.count(accessedSignalIdent) == 0) {
//        notUsableSignals.insert(std::make_pair(accessedSignalIdent, std::vector<syrec::VariableAccess::ptr>(1, accessedSignalParts)));
//    } else {
//        notUsableSignals.at(accessedSignalIdent).emplace_back(accessedSignalParts);
//    }
//}
//
//std::optional<unsigned int> AssignmentWithOnlyReversibleOperationsSimplifier::tryFetchValueOfNumber(const syrec::Number::ptr& number) {
//    return number->isConstant() ? std::make_optional(number->evaluate({})) : std::nullopt;
//}
//
//std::optional<unsigned> AssignmentWithOnlyReversibleOperationsSimplifier::tryFetchValueOfExpr(const syrec::expression::ptr& expr) {
//    if (const auto& exprAsNumericExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(expr); exprAsNumericExpr != nullptr) {
//        return tryFetchValueOfNumber(exprAsNumericExpr->value);
//    }
//    return std::nullopt;
//}
//
//std::optional<unsigned int> AssignmentWithOnlyReversibleOperationsSimplifier::fetchBitWidthOfSignal(const std::string_view& signalIdent) const {
//    if (const auto& symbolTableEntryForSignal = fetchSymbolTableEntryForSignalAccess(signalIdent); symbolTableEntryForSignal.has_value()) {
//        return std::make_optional(symbolTableEntryForSignal.value()->bitwidth);
//    }
//    return std::nullopt;
//}
//
//std::optional<syrec::Variable::ptr> AssignmentWithOnlyReversibleOperationsSimplifier::fetchSymbolTableEntryForSignalAccess(const std::string_view& signalIdent) const {
//    if (const auto& symbolTableEntryForSignal = symbolTable->getVariable(signalIdent); symbolTableEntryForSignal.has_value() && std::holds_alternative<syrec::Variable::ptr>(*symbolTableEntryForSignal)) {
//        return std::make_optional(std::get<syrec::Variable::ptr>(*symbolTableEntryForSignal));
//    }
//    return std::nullopt;
//}
//
//std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess> AssignmentWithOnlyReversibleOperationsSimplifier::transformUserDefinedBitRangeAccess(const syrec::VariableAccess::ptr& accessedSignalParts) const {
//    if (!accessedSignalParts->range.has_value() || !tryFetchValueOfNumber(accessedSignalParts->range->first).has_value() || !tryFetchValueOfNumber(accessedSignalParts->range->second).has_value()) {
//        const auto& bitWidthOfAccessedSignal = fetchBitWidthOfSignal(accessedSignalParts->var->name);
//        if (bitWidthOfAccessedSignal.has_value()) {
//            return std::make_optional(optimizations::BitRangeAccessRestriction::BitRangeAccess(0, *bitWidthOfAccessedSignal - 1));
//        }
//    }
//    return std::make_optional(optimizations::BitRangeAccessRestriction::BitRangeAccess(*tryFetchValueOfNumber(accessedSignalParts->range->first), *tryFetchValueOfNumber(accessedSignalParts->range->second) - 1));
//}
