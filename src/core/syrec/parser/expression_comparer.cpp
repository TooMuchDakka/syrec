#include <algorithm>
#include <unordered_map>
#include "core/syrec/parser/expression_comparer.hpp"

using namespace parser;

static std::unordered_map<size_t, size_t> expressionSubTypeCaseOffsetLookup {};

// TODO: https://www.stroustrup.com/OOPSLA-typeswitch-draft.pdf (Memoization device)
// TODO: Refactor into own namespace ?
// TODO: Fix missing annotations for fall through ?
// TODO: Check safety of code
bool parser::areSyntacticallyEquivalent(const syrec::Expression::ptr& referenceExpr, const syrec::Expression::ptr& exprToCheck) {
    const size_t& exprToCheckTypeInfo = typeid(*referenceExpr.get()).hash_code();
    switch (size_t& subTypeOffset = expressionSubTypeCaseOffsetLookup[exprToCheckTypeInfo]) {
        default: {
            if (isNumericExpression(referenceExpr)) {
                subTypeOffset = 1;
                case 1: {
                    return isNumericExpressionAndEquivalentToReference(std::reinterpret_pointer_cast<syrec::NumericExpression>(referenceExpr), exprToCheck);  
                }
            }
            if (isVariableExpression(referenceExpr)) {
                subTypeOffset = 2;
                case 2: 
                    return isVariableExpressionAndEquivalentToReference(std::reinterpret_pointer_cast<syrec::VariableExpression>(referenceExpr), exprToCheck);
            }
            if (isBinaryExpression(referenceExpr)) {
                subTypeOffset = 3;
                case 3: 
                    return isBinaryExpressionAndEquivalentToReference(std::reinterpret_pointer_cast<syrec::BinaryExpression>(referenceExpr), exprToCheck);
            }
            if (isShiftExpression(referenceExpr)) {
                subTypeOffset = 4;
                case 4: 
                    return isShiftExpressionAndEquivalentToReference(std::reinterpret_pointer_cast<syrec::ShiftExpression>(referenceExpr), exprToCheck);
            }
            subTypeOffset = 5;
            //[[fallthrough]];
        }
        case 5: {
            break;   
        }
    }
    return false;
}

bool parser::isNumericExpressionAndEquivalentToReference(const std::shared_ptr<syrec::NumericExpression>& referenceExpr, const syrec::Expression::ptr& exprToCheck) {
    if (!referenceExpr || !exprToCheck) {
        return false;
    }

    const auto exprToCheckCasted = std::dynamic_pointer_cast<syrec::NumericExpression>(exprToCheck);
    return exprToCheckCasted && isNumberAndEquivalentToReference(referenceExpr->value, exprToCheckCasted->value);
}

bool parser::isVariableExpressionAndEquivalentToReference(const std::shared_ptr<syrec::VariableExpression>& referenceExpr, const syrec::Expression::ptr& exprToCheck) {
    if (!referenceExpr || !exprToCheck) {
        return false;
    }

    const auto exprToCheckCasted = std::dynamic_pointer_cast<syrec::VariableExpression>(exprToCheck);
    return exprToCheckCasted && isVariableAccessAndEquivalentToReference(referenceExpr->var, exprToCheckCasted->var);
}

bool parser::isBinaryExpressionAndEquivalentToReference(const std::shared_ptr<syrec::BinaryExpression>& referenceExpr, const syrec::Expression::ptr& exprToCheck) {
    if (!referenceExpr || !exprToCheck) {
        return false;
    }

    const auto exprToCheckCasted = std::dynamic_pointer_cast<syrec::BinaryExpression>(exprToCheck);
    return exprToCheckCasted
        && referenceExpr->op == exprToCheckCasted->op
        && areSyntacticallyEquivalent(referenceExpr->lhs, exprToCheckCasted->lhs)
        && areSyntacticallyEquivalent(referenceExpr->rhs, exprToCheckCasted->rhs);
}

bool parser::isShiftExpressionAndEquivalentToReference(const std::shared_ptr<syrec::ShiftExpression>& referenceExpr, const syrec::Expression::ptr& exprToCheck) {
    if (!referenceExpr || !exprToCheck) {
        return false;
    }

    const auto exprToCheckCasted = std::dynamic_pointer_cast<syrec::ShiftExpression>(exprToCheck);
    return exprToCheckCasted
        && referenceExpr->op == exprToCheckCasted->op
        && areSyntacticallyEquivalent(referenceExpr->lhs, exprToCheckCasted->lhs)
        && isNumberAndEquivalentToReference(referenceExpr->rhs, exprToCheckCasted->rhs);
}

bool parser::isVariableAccessAndEquivalentToReference(const syrec::VariableAccess::ptr& referenceVariableAccess, const syrec::VariableAccess::ptr& variableAccessToCheck) {
    if (!referenceVariableAccess || !variableAccessToCheck
        || !referenceVariableAccess->var || !variableAccessToCheck->var) {
        return false;
    }

    bool areEquivalent = referenceVariableAccess->var->name == variableAccessToCheck->var->name;
    areEquivalent &= std::equal(
            referenceVariableAccess->indexes.cbegin(),
            referenceVariableAccess->indexes.cend(),
            variableAccessToCheck->indexes.cbegin(),
            variableAccessToCheck->indexes.cend(),
            [](const syrec::Expression::ptr& referenceDimensionAccess, const syrec::Expression::ptr& variableToCheckDimensionAccess) {
                return areSyntacticallyEquivalent(referenceDimensionAccess, variableToCheckDimensionAccess);
            });

    areEquivalent &= !(referenceVariableAccess->range.has_value() ^ variableAccessToCheck->range.has_value());
    if (areEquivalent && referenceVariableAccess->range.has_value()) {
        const auto referenceRange = (*referenceVariableAccess->range);
        const auto rangeToCheck   = (*variableAccessToCheck->range);
        areEquivalent &= isNumberAndEquivalentToReference(referenceRange.first, rangeToCheck.first)
            && isNumberAndEquivalentToReference(referenceRange.second, rangeToCheck.second);
    }

    return areEquivalent;
}

bool parser::isNumberAndEquivalentToReference(const syrec::Number::ptr& referenceNumber, const syrec::Number::ptr& numberToCheck) {
    if (!referenceNumber || !numberToCheck) {
        return false;
    }

    if (referenceNumber->isConstant()) {
        return numberToCheck->isConstant() && referenceNumber->evaluate({}) == numberToCheck->evaluate({});
    }
    return numberToCheck->isLoopVariable() && referenceNumber->variableName() == numberToCheck->variableName();
}

bool parser::isBinaryExpression(const syrec::Expression::ptr& exprToCheck) {
    return std::dynamic_pointer_cast<syrec::BinaryExpression>(exprToCheck) != nullptr;
}

bool parser::isNumericExpression(const syrec::Expression::ptr& exprToCheck) {
    return std::dynamic_pointer_cast<syrec::NumericExpression>(exprToCheck) != nullptr;
}

bool parser::isShiftExpression(const syrec::Expression::ptr& exprToCheck) {
    return std::dynamic_pointer_cast<syrec::ShiftExpression>(exprToCheck) != nullptr;
}

bool parser::isVariableExpression(const syrec::Expression::ptr& exprToCheck) {
    return std::dynamic_pointer_cast<syrec::VariableExpression>(exprToCheck) != nullptr;
}



