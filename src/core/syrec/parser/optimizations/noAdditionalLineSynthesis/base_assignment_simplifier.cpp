#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/base_assignment_simplifier.hpp"

#include "core/syrec/parser/operation.hpp"
#include "core/syrec/parser/utils/signal_access_utils.hpp"

using namespace noAdditionalLineSynthesis;

BaseAssignmentSimplifier::~BaseAssignmentSimplifier() = default;

syrec::Statement::vec BaseAssignmentSimplifier::simplify(const syrec::AssignStatement::ptr& assignmentStmt, bool isValueOfAssignedToSignalBlockedByDataFlowAnalysis) {
    resetInternals();
    const auto& assignStmtCasted = std::dynamic_pointer_cast<syrec::AssignStatement>(assignmentStmt);
    const auto& assignStmtOperation = assignStmtCasted != nullptr ? syrec_operation::tryMapAssignmentOperationFlagToEnum(assignStmtCasted->op) : std::nullopt;
    if (!assignStmtOperation.has_value()) {
        return {};
    }

    bool canHandleDefinedAssignmentOperation = false;
    switch (*assignStmtOperation) {
        case syrec_operation::operation::AddAssign:
        case syrec_operation::operation::MinusAssign:
        case syrec_operation::operation::XorAssign:
            canHandleDefinedAssignmentOperation = true;
            break;
        default:
            break;
    }

    if (!canHandleDefinedAssignmentOperation || !simplificationPrecondition(assignmentStmt)) {
        return {};
    }
    return simplifyWithoutPreconditionCheck(assignmentStmt, isValueOfAssignedToSignalBlockedByDataFlowAnalysis);
}

void BaseAssignmentSimplifier::resetInternals() {
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
        if (const auto& variableAccessOnLhs = std::dynamic_pointer_cast<syrec::VariableExpression>(exprAsBinaryExpr->lhs); doesExprDefineVariableAccess(exprAsBinaryExpr->lhs) && variableAccessOnLhs != nullptr) {
            if (wasAccessOnSignalAlreadyDefined(variableAccessOnLhs->var, notUsableSignals)) {
                return std::make_optional(false);
            }
            markSignalAccessAsNotUsableInExpr(variableAccessOnLhs->var, notUsableSignals);
        }
        if (const auto& variableAccessOnRhs = std::dynamic_pointer_cast<syrec::VariableExpression>(exprAsBinaryExpr->rhs); doesExprDefineVariableAccess(exprAsBinaryExpr->rhs) && variableAccessOnRhs != nullptr) {
            if (wasAccessOnSignalAlreadyDefined(variableAccessOnRhs->var, notUsableSignals)) {
                return std::make_optional(false);
            }
            markSignalAccessAsNotUsableInExpr(variableAccessOnRhs->var, notUsableSignals);
        }

        std::optional<bool> nestedExprOk = std::make_optional(true);
        if (doesExprDefineNestedExpr(exprAsBinaryExpr->lhs)) {
            nestedExprOk = isEveryLhsOperandOfAnyBinaryExprDefinedOnceOnEveryLevelOfTheAST(exprAsBinaryExpr->lhs, false, notUsableSignals);
        }
        if (nestedExprOk.value_or(false) && *nestedExprOk && doesExprDefineNestedExpr(exprAsBinaryExpr->rhs)) {
            nestedExprOk = isEveryLhsOperandOfAnyBinaryExprDefinedOnceOnEveryLevelOfTheAST(exprAsBinaryExpr->rhs, false, notUsableSignals);
        }
        return nestedExprOk;
    }
    return std::nullopt;
}

std::optional<bool> BaseAssignmentSimplifier::doesExprOnlyContainUniqueSignalAccesses(const syrec::expression::ptr& exprToCheck, RestrictionMap& alreadyDefinedSignalAccesses) {
    if (const auto& exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(exprToCheck); exprAsBinaryExpr != nullptr) {
        return doesExprOnlyContainUniqueSignalAccesses(exprAsBinaryExpr->lhs, alreadyDefinedSignalAccesses) && doesExprOnlyContainUniqueSignalAccesses(exprAsBinaryExpr->rhs, alreadyDefinedSignalAccesses);
    } if (const auto& exprAsShiftExpr = std::dynamic_pointer_cast<syrec::ShiftExpression>(exprToCheck); exprAsShiftExpr != nullptr) {
        return doesExprOnlyContainUniqueSignalAccesses(exprAsShiftExpr->lhs, alreadyDefinedSignalAccesses);
    } if (const auto& exprAsVariableExpr = std::dynamic_pointer_cast<syrec::VariableExpression>(exprToCheck); exprAsVariableExpr != nullptr) {
        if (!wasAnyAccessedSignalPartPreviouslyNotAccessed(exprAsVariableExpr->var, alreadyDefinedSignalAccesses)) {
            markSignalAccessAsNotUsableInExpr(exprAsVariableExpr->var, alreadyDefinedSignalAccesses);
            return true;
        }
        return false;
    }
    return true;
}

std::optional<bool> BaseAssignmentSimplifier::isValueOfAssignedToSignalZero(const syrec::VariableAccess::ptr& assignedToSignal) const {
    if (const auto& fetchedSignalValue = symbolTable->tryFetchValueForLiteral(assignedToSignal); fetchedSignalValue.has_value()) {
        if (!*fetchedSignalValue && locallyDisabledSignalWithValueOfZeroMap->count(assignedToSignal->var->name) != 0) {
            return std::none_of(
            locallyDisabledSignalWithValueOfZeroMap->at(assignedToSignal->var->name).cbegin(),
            locallyDisabledSignalWithValueOfZeroMap->at(assignedToSignal->var->name).cend(),
            [&](const syrec::VariableAccess::ptr& locallyDisabledSignalWithValueOfZero) {
                const auto& signalAccessEquivalenceResult = SignalAccessUtils::areSignalAccessesEqual(assignedToSignal, locallyDisabledSignalWithValueOfZero, SignalAccessUtils::SignalAccessComponentEquivalenceCriteria::DimensionAccess::Overlapping, SignalAccessUtils::SignalAccessComponentEquivalenceCriteria::BitRange::Overlapping, symbolTable);
                return signalAccessEquivalenceResult.equality != SignalAccessUtils::SignalAccessEquivalenceResult::NotEqual;
            });
        }   
    }
    return std::nullopt;
}

void BaseAssignmentSimplifier::invalidateSignalWithPreviousValueOfZero(const syrec::VariableAccess::ptr& assignedToSignal) const {
    if (const auto& fetchedSignalValue = symbolTable->tryFetchValueForLiteral(assignedToSignal); fetchedSignalValue.has_value()) {
        if (*fetchedSignalValue) {
            return;
        }

        if (locallyDisabledSignalWithValueOfZeroMap->count(assignedToSignal->var->name) != 0) {
            if (std::none_of(
            locallyDisabledSignalWithValueOfZeroMap->at(assignedToSignal->var->name).cbegin(),
            locallyDisabledSignalWithValueOfZeroMap->at(assignedToSignal->var->name).cend(),
            [&](const syrec::VariableAccess::ptr& locallyDisabledSignalWithValueOfZero) {
                    const auto& signalAccessEquivalenceResult = SignalAccessUtils::areSignalAccessesEqual(assignedToSignal, locallyDisabledSignalWithValueOfZero, SignalAccessUtils::SignalAccessComponentEquivalenceCriteria::DimensionAccess::Overlapping, SignalAccessUtils::SignalAccessComponentEquivalenceCriteria::BitRange::Overlapping, symbolTable);
                    return signalAccessEquivalenceResult.equality != SignalAccessUtils::SignalAccessEquivalenceResult::NotEqual;
            })) {
                locallyDisabledSignalWithValueOfZeroMap->at(assignedToSignal->var->name).emplace_back(assignedToSignal);
            }
        } else {
            locallyDisabledSignalWithValueOfZeroMap->insert(std::make_pair(assignedToSignal->var->name, std::vector(1, assignedToSignal)));
        }
    }
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

bool BaseAssignmentSimplifier::doesExprOnlyContainReversibleOperations(const syrec::expression::ptr& expr) {
    if (const auto exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr); exprAsBinaryExpr != nullptr) {
        const auto mappedToOperationFlagOfBinaryExpr = syrec_operation::tryMapBinaryOperationFlagToEnum(exprAsBinaryExpr->op);
        if (!mappedToOperationFlagOfBinaryExpr.has_value() || !syrec_operation::getMatchingAssignmentOperationForOperation(*mappedToOperationFlagOfBinaryExpr).has_value() || !syrec_operation::invert(*syrec_operation::getMatchingAssignmentOperationForOperation(*mappedToOperationFlagOfBinaryExpr)).has_value() || !syrec_operation::invert(*mappedToOperationFlagOfBinaryExpr).has_value()) {
            return false;
        }

        bool doesLhsExprContainOnlyReversibleOperations = false;
        if (const auto& lhsExprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(exprAsBinaryExpr->lhs); lhsExprAsBinaryExpr != nullptr) {
            doesLhsExprContainOnlyReversibleOperations = doesExprOnlyContainReversibleOperations(lhsExprAsBinaryExpr);
        } else if (const auto& lhsExprAsVariableAccess = std::dynamic_pointer_cast<syrec::VariableExpression>(exprAsBinaryExpr->lhs); lhsExprAsVariableAccess != nullptr) {
            doesLhsExprContainOnlyReversibleOperations = true;
        } else if (const auto& lhsExprAsNumericExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(exprAsBinaryExpr->lhs); lhsExprAsNumericExpr != nullptr) {
            doesLhsExprContainOnlyReversibleOperations = true;
        }

        if (doesLhsExprContainOnlyReversibleOperations) {
            if (const auto& rhsExprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(exprAsBinaryExpr->rhs); rhsExprAsBinaryExpr != nullptr) {
                return doesExprOnlyContainReversibleOperations(rhsExprAsBinaryExpr);
            }
            if (const auto& rhsExprAsVariableAccess = std::dynamic_pointer_cast<syrec::VariableExpression>(exprAsBinaryExpr->rhs); rhsExprAsVariableAccess != nullptr) {
                return true;
            }
            if (const auto& rhsExprAsNumericExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(exprAsBinaryExpr->rhs); rhsExprAsNumericExpr != nullptr) {
                return true;
            }
        }
    }
    return false;
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
                const auto& signalAccessEquivalenceResult = SignalAccessUtils::areSignalAccessesEqual(accessedSignalParts, alreadyDefinedSignalAccess, SignalAccessUtils::SignalAccessComponentEquivalenceCriteria::DimensionAccess::Overlapping, SignalAccessUtils::SignalAccessComponentEquivalenceCriteria::BitRange::Overlapping, symbolTable);
                return signalAccessEquivalenceResult.equality != SignalAccessUtils::SignalAccessEquivalenceResult::NotEqual;
            });
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
                const auto& signalAccessEquivalenceResult = SignalAccessUtils::areSignalAccessesEqual(accessedSignalParts, previouslyAccessedSignalParts, SignalAccessUtils::SignalAccessComponentEquivalenceCriteria::DimensionAccess::Overlapping, SignalAccessUtils::SignalAccessComponentEquivalenceCriteria::BitRange::Overlapping, symbolTable);
                return signalAccessEquivalenceResult.equality == SignalAccessUtils::SignalAccessEquivalenceResult::NotEqual && signalAccessEquivalenceResult.isResultCertain;
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

syrec::Statement::vec BaseAssignmentSimplifier::invertAssignmentsButIgnoreSome(const syrec::Statement::vec& assignmentsToInvert, const std::size_t numStatementsToIgnoreStartingFromLastOne) {
    const auto numStatementsToInvert = numStatementsToIgnoreStartingFromLastOne >= assignmentsToInvert.size() ? 0 : assignmentsToInvert.size() - numStatementsToIgnoreStartingFromLastOne;
    if (numStatementsToInvert == 0) {
        return assignmentsToInvert;
    }

    const auto numTotalAssignments = assignmentsToInvert.size() + numStatementsToInvert;
    syrec::Statement::vec invertedAssignments(numTotalAssignments);
    std::copy(assignmentsToInvert.cbegin(), assignmentsToInvert.cend(), invertedAssignments.begin());

    auto invertedStatementIdxInFinalContainer = assignmentsToInvert.size();
    for (auto statementsToInvertIterator = std::next(assignmentsToInvert.rbegin(), numStatementsToIgnoreStartingFromLastOne); statementsToInvertIterator != assignmentsToInvert.rend(); ++statementsToInvertIterator) {
        const auto assignmentCasted = std::dynamic_pointer_cast<syrec::AssignStatement>(*statementsToInvertIterator);
        invertedAssignments.at(invertedStatementIdxInFinalContainer++)   = std::make_shared<syrec::AssignStatement>(
                assignmentCasted->lhs,
                *syrec_operation::tryMapAssignmentOperationEnumToFlag(*syrec_operation::invert(*syrec_operation::tryMapAssignmentOperationFlagToEnum(assignmentCasted->op))),
                assignmentCasted->rhs);
    }
    return invertedAssignments;
}