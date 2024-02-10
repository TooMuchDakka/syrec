#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/temporary_expressions_container.hpp"
#include "core/syrec/parser/utils/signal_access_utils.hpp"

#include <algorithm>

using namespace noAdditionalLineSynthesis;

void TemporaryExpressionsContainer::resetInternals() {
    activeExpressions.clear();
}

void TemporaryExpressionsContainer::defineSymbolTable(const parser::SymbolTable::ptr& symbolTable) {
    symbolTableReference = symbolTable;
}

void TemporaryExpressionsContainer::activateExpression(const syrec::Expression::ptr& expr) {
    activeExpressions.emplace(expr);
}

void TemporaryExpressionsContainer::deactivateExpression(const syrec::Expression::ptr& expr) {
    activeExpressions.erase(expr);
}

std::optional<bool> TemporaryExpressionsContainer::existsAnyExpressionDefiningOverlappingSignalAccess(const syrec::VariableAccess& accessedSignalPartsToCheckForOverlaps) const {
    if (!symbolTableReference) {
        return std::nullopt;
    }

    return std::any_of(
    activeExpressions.cbegin(),
    activeExpressions.cend(),
    [&accessedSignalPartsToCheckForOverlaps, this](const syrec::Expression::ptr& exprFromInternalLookup) {
        return doesExpressionDefineOverlappingSignalAccess(*exprFromInternalLookup, accessedSignalPartsToCheckForOverlaps, *symbolTableReference);
    });
}

// BEGIN: NON-PUBLIC Functions
bool TemporaryExpressionsContainer::doesExpressionDefineOverlappingSignalAccess(const syrec::Expression& expr, const syrec::VariableAccess& accessedSignalPartsToCheckForOverlaps, const parser::SymbolTable& symbolTableReference) {
    if (const auto& exprAsVariableExpr = dynamic_cast<const syrec::VariableExpression*>(&expr); exprAsVariableExpr) {
        const SignalAccessUtils::SignalAccessEquivalenceResult& equivalenceCheckResult = SignalAccessUtils::areSignalAccessesEqual(
            accessedSignalPartsToCheckForOverlaps, *exprAsVariableExpr->var, 
            SignalAccessUtils::SignalAccessComponentEquivalenceCriteria::DimensionAccess::Overlapping,
            SignalAccessUtils::SignalAccessComponentEquivalenceCriteria::BitRange::Overlapping, symbolTableReference);
        return !(equivalenceCheckResult.isResultCertain && equivalenceCheckResult.equality == SignalAccessUtils::SignalAccessEquivalenceResult::NotEqual);
    }
    if (const auto& exprAsBinaryExpr = dynamic_cast<const syrec::BinaryExpression*>(&expr); exprAsBinaryExpr) {
        return doesExpressionDefineOverlappingSignalAccess(*exprAsBinaryExpr->lhs, accessedSignalPartsToCheckForOverlaps, symbolTableReference) || doesExpressionDefineOverlappingSignalAccess(*exprAsBinaryExpr->rhs, accessedSignalPartsToCheckForOverlaps, symbolTableReference);
    }
    if (const auto& exprAsShiftExpr = dynamic_cast<const syrec::ShiftExpression*>(&expr); exprAsShiftExpr) {
        return doesExpressionDefineOverlappingSignalAccess(*exprAsShiftExpr->lhs, accessedSignalPartsToCheckForOverlaps, symbolTableReference);
    }
    return false;
}