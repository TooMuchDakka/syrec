#ifndef EXPRESSION_COMPARER_HPP
#define EXPRESSION_COMPARER_HPP

#include "core/syrec/expression.hpp"

namespace parser {
    [[nodiscard]] bool areSyntacticallyEquivalent(const syrec::expression::ptr& referenceExpr, const syrec::expression::ptr& exprToCheck);
    [[nodiscard]] bool isNumericExpressionAndEquivalentToReference(const std::shared_ptr<syrec::NumericExpression>& referenceExpr, const syrec::expression::ptr& exprToCheck);
    [[nodiscard]] bool isVariableExpressionAndEquivalentToReference(const std::shared_ptr<syrec::VariableExpression>& referenceExpr, const syrec::expression::ptr& exprToCheck);
    [[nodiscard]] bool isBinaryExpressionAndEquivalentToReference(const std::shared_ptr<syrec::BinaryExpression>& referenceExpr, const syrec::expression::ptr& exprToCheck);
    [[nodiscard]] bool isShiftExpressionAndEquivalentToReference(const std::shared_ptr<syrec::ShiftExpression>& referenceExpr, const syrec::expression::ptr& exprToCheck);
    [[nodiscard]] bool isVariableAccessAndEquivalentToReference(const syrec::VariableAccess::ptr& referenceVariableAccess, const syrec::VariableAccess::ptr& variableAccessToCheck);
    [[nodiscard]] bool isNumberAndEquivalentToReference(const syrec::Number::ptr& referenceNumber, const syrec::Number::ptr& numberToCheck);

    [[nodiscard]] bool isNumericExpression(const syrec::expression::ptr& exprToCheck);
    [[nodiscard]] bool isVariableExpression(const syrec::expression::ptr& exprToCheck);
    [[nodiscard]] bool isBinaryExpression(const syrec::expression::ptr& exprToCheck);
    [[nodiscard]] bool isShiftExpression(const syrec::expression::ptr& exprToCheck);
}
#endif 