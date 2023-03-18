#include "core/syrec/parser/optimizations/reassociate_expression.hpp"
#include "core/syrec/parser/parser_utilities.hpp"

#include <tuple>

using namespace optimization;

inline std::optional<std::shared_ptr<syrec::BinaryExpression>> tryConvertExpressionToBinaryOne(const syrec::expression::ptr& expr) {
    if (const auto& exprAsBinaryExpression = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr); exprAsBinaryExpression != nullptr) {
        return std::make_optional(exprAsBinaryExpression);
    }
    return std::nullopt;
}

inline syrec::expression::ptr createBinaryExpression(const syrec::expression::ptr& lhsOperand, const unsigned int binaryOperation, const syrec::expression::ptr& rhsOperand) {
    return std::make_shared<syrec::BinaryExpression>(lhsOperand, binaryOperation, rhsOperand);
}

inline syrec::expression::ptr simplifyBinaryExpressionIfSimplificationOfOperandsResultsInNewOperands(const std::shared_ptr<syrec::BinaryExpression>& binaryExpr) {
    const auto simplificationResultOfLhsOperand = simplifyBinaryExpression(binaryExpr->lhs);
    const auto simplificationResultOfRhsOperand = simplifyBinaryExpression(binaryExpr->rhs);
    if (simplificationResultOfLhsOperand == binaryExpr->lhs && simplificationResultOfRhsOperand == binaryExpr->rhs) {
        return binaryExpr;
    }
    return simplifyBinaryExpression(createBinaryExpression(simplificationResultOfLhsOperand, binaryExpr->op, simplificationResultOfRhsOperand));
}

inline bool isArithmeticOperation(syrec_operation::operation operation) {
    switch (operation) {
        case syrec_operation::operation::addition:
        case syrec_operation::operation::subtraction:
        case syrec_operation::operation::division:
        case syrec_operation::operation::multiplication:
            return true;
        default:
            return false;
    }
}

bool isOperationCombinedWithMultiplicationDistributive(syrec_operation::operation operation) {
    switch (operation) {
        case syrec_operation::operation::addition:
        case syrec_operation::operation::subtraction:
        case syrec_operation::operation::multiplication:
            return true;
        default:
            return false;
    }
}

syrec::expression::ptr createExpressionForNumber(const unsigned int number) {
    return std::make_shared<syrec::NumericExpression>(std::make_shared<syrec::Number>(number), 0);
}

std::optional<unsigned int> tryGetConstantValueOfExpression(const syrec::expression::ptr& expr) {
    std::optional<unsigned int> valueOfExpression;
    if (const auto exprAsNumericExpression = std::dynamic_pointer_cast<syrec::NumericExpression>(expr); exprAsNumericExpression != nullptr) {
        if (exprAsNumericExpression->value->isConstant()) {
            valueOfExpression.emplace(exprAsNumericExpression->value->evaluate({}));
        }
    }
    return valueOfExpression;
}

inline bool isExpressionConstantOrSignalAccess(const syrec::expression::ptr& expression) {
    return std::dynamic_pointer_cast<syrec::NumericExpression>(expression) != nullptr || std::dynamic_pointer_cast<syrec::VariableExpression>(expression) != nullptr;
}

std::optional<std::tuple<unsigned int, syrec::expression::ptr>> trySplitExpressionIntoConstantAndNonExprOperand(const std::shared_ptr<syrec::BinaryExpression>& binaryExpr) {
    if (!isExpressionConstantOrSignalAccess(binaryExpr->lhs) && !isExpressionConstantOrSignalAccess(binaryExpr->rhs)) {
        return std::nullopt;
    }

    std::optional<unsigned int> constOperandOfLhsExpr    = tryGetConstantValueOfExpression(binaryExpr->lhs);
    syrec::expression::ptr      nonConstOperandOfLhsExpr = binaryExpr->rhs;
    if (!constOperandOfLhsExpr.has_value()) {
        constOperandOfLhsExpr    = tryGetConstantValueOfExpression(binaryExpr->rhs);
        nonConstOperandOfLhsExpr = binaryExpr->lhs;
    }

    if (constOperandOfLhsExpr.has_value()) {
        return std::make_optional(std::make_tuple(*constOperandOfLhsExpr, nonConstOperandOfLhsExpr));   
    }
    return std::nullopt;
}

/*
 * TODO: More tests with subexpressions being not optimizable or simply testing the other reordering rules currently not covered
 * TODO: Handling of cases (a + b) * c would be expanded to (a * c) + (b * c) which could introduce additional lines into our circuit,
 *          which in turn is a contradiction (for reversivel circuits)
 * TODO: There is room for further optimizations where one would 'accumulate' signals (i.e. a + (2 + a) => (2 * a) + 2)
 */
syrec::expression::ptr optimization::simplifyBinaryExpression(const syrec::expression::ptr& expr) {
    const auto binaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr);
    if (binaryExpr == nullptr) {
        return expr;
    }

    const auto lOperandConstValue = tryGetConstantValueOfExpression(binaryExpr->lhs);
    const auto rOperandConstValue = tryGetConstantValueOfExpression(binaryExpr->rhs);

    std::optional<unsigned int> constOperandOfParentExpr;
    if (lOperandConstValue.has_value()) {
        constOperandOfParentExpr = lOperandConstValue;
    } else if (rOperandConstValue.has_value()) {
        constOperandOfParentExpr = rOperandConstValue;
    }

    const auto mappedFlagToEnum = parser::ParserUtilities::mapInternalBinaryOperationFlagToEnum(binaryExpr->op);
    if (!mappedFlagToEnum.has_value()) {
        return binaryExpr;
    }

    const auto binaryOperationOfExpr = mappedFlagToEnum.value();
    if (lOperandConstValue.has_value() && rOperandConstValue.has_value()) {
        if (const auto resultOfExprWithConstOperands = apply(binaryOperationOfExpr, *lOperandConstValue, *rOperandConstValue);
            resultOfExprWithConstOperands.has_value()) {
            return createExpressionForNumber(*resultOfExprWithConstOperands);
        }
        return expr;
    }

    // TODO: These simplification rules should be extracted into an utility function
    // TODO: Can we apply boolean operation simplifications on arbitrary signals (i.e. x OR 1 = 1) ?
    switch (binaryOperationOfExpr) {
        case syrec_operation::operation::addition: {
            if (constOperandOfParentExpr.has_value() && *constOperandOfParentExpr == 0) {
                return simplifyBinaryExpression(lOperandConstValue.has_value() ? binaryExpr->rhs : binaryExpr->lhs);
            }
            break;
        }
        case syrec_operation::operation::subtraction: {
            if (rOperandConstValue.has_value()) {
                return simplifyBinaryExpression(createBinaryExpression(createExpressionForNumber(-*rOperandConstValue), syrec::BinaryExpression::Add, binaryExpr->lhs));
            }
            // Since we cannot simply negate any expression, we will leave this case unoptimized for now
            /*if (lOperandConstValue.has_value()) {
                return simplifyBinaryExpression(std::make_shared<syrec::BinaryExpression>(createExpressionForNumber(-lOperandConstValue.value()), syrec::BinaryExpression::Add, binaryExpr->rhs));
            }*/
            return simplifyBinaryExpressionIfSimplificationOfOperandsResultsInNewOperands(binaryExpr);    
        }
        case syrec_operation::operation::division: {
            if (lOperandConstValue.has_value() && *lOperandConstValue == 0) {
                return createExpressionForNumber(0);
            }
            break;
        }
        case syrec_operation::operation::multiplication: {
            if (constOperandOfParentExpr.has_value()) {
                // x * 0 = 0 or 0 * x = 0
                if (*constOperandOfParentExpr == 0) {
                    return createExpressionForNumber(0);
                }
                // x * 1 = x or 1 * x = x
                if (*constOperandOfParentExpr == 1) {
                    return simplifyBinaryExpression(lOperandConstValue.has_value() ? binaryExpr->rhs : binaryExpr->lhs);
                }
            }
            break;
        }
        default:
            break;
    }

    if ((binaryOperationOfExpr != syrec_operation::operation::multiplication && binaryOperationOfExpr != syrec_operation::operation::addition) 
        || (!isExpressionConstantOrSignalAccess(binaryExpr->lhs) && !isExpressionConstantOrSignalAccess(binaryExpr->rhs))) {
        if (binaryOperationOfExpr == syrec_operation::operation::addition) {
            const auto lhsExprAsBinaryExpr = tryConvertExpressionToBinaryOne(binaryExpr->lhs);
            const auto rhsExprAsBinaryExpr = tryConvertExpressionToBinaryOne(binaryExpr->rhs);

            if (lhsExprAsBinaryExpr.has_value() && rhsExprAsBinaryExpr.has_value()) {
                if ((*lhsExprAsBinaryExpr)->op == syrec::BinaryExpression::Add && (*rhsExprAsBinaryExpr)->op == syrec::BinaryExpression::Add) {
                    const auto lhsExprDividedIntoConstantAndNonExprOperand = trySplitExpressionIntoConstantAndNonExprOperand(*lhsExprAsBinaryExpr);
                    const auto rhsExprDividedIntoConstantAndNonExprOperand = trySplitExpressionIntoConstantAndNonExprOperand(*rhsExprAsBinaryExpr);

                    /*
                     * We can simplify the expression (c1 + x) + (c2 + y) into (c1 + c2) + (x + y)
                     */
                    if (lhsExprDividedIntoConstantAndNonExprOperand.has_value() && rhsExprDividedIntoConstantAndNonExprOperand.has_value()) {
                        return createBinaryExpression(
                                createExpressionForNumber(
                                        *apply(
                                                syrec_operation::operation::addition,
                                                std::get<unsigned int>(*lhsExprDividedIntoConstantAndNonExprOperand),
                                                std::get<unsigned int>(*rhsExprDividedIntoConstantAndNonExprOperand))),
                                syrec::BinaryExpression::Add,
                                createBinaryExpression(
                                        std::get<syrec::expression::ptr>(*lhsExprDividedIntoConstantAndNonExprOperand),
                                        syrec::BinaryExpression::Add,
                                        std::get<syrec::expression::ptr>(*rhsExprDividedIntoConstantAndNonExprOperand)));
                    }
                }
            }
        }
        return simplifyBinaryExpressionIfSimplificationOfOperandsResultsInNewOperands(binaryExpr);
    }

    std::optional<syrec::expression::ptr> nonNestedExpressionOperandOfParentExpr;
    syrec::expression::ptr                exprToCheckForDistributivity;
    if (isExpressionConstantOrSignalAccess(binaryExpr->lhs)) {
        nonNestedExpressionOperandOfParentExpr = binaryExpr->lhs;
        exprToCheckForDistributivity           = binaryExpr->rhs;
    } else {
        nonNestedExpressionOperandOfParentExpr = binaryExpr->rhs;
        exprToCheckForDistributivity           = binaryExpr->lhs;
    }

    // Both operands of expression are expressions themselves so we recursively apply the simplification operation on both operands
    if (!nonNestedExpressionOperandOfParentExpr.has_value()) {
        return createBinaryExpression(simplifyBinaryExpression(binaryExpr->lhs), binaryExpr->op, simplifyBinaryExpression(binaryExpr->rhs));
    }

    std::optional<std::shared_ptr<syrec::BinaryExpression>> childBinaryExpression;
    if (const auto childOperandAsBinaryExpression = std::dynamic_pointer_cast<syrec::BinaryExpression>(exprToCheckForDistributivity); childOperandAsBinaryExpression != nullptr) {
        childBinaryExpression.emplace(childOperandAsBinaryExpression);
    }

    const std::optional<syrec_operation::operation> childBinaryExprOperation = childBinaryExpression.has_value()
        ? std::make_optional(*parser::ParserUtilities::mapInternalBinaryOperationFlagToEnum((*childBinaryExpression)->op))
        : std::nullopt;

    if (!childBinaryExpression.has_value() || !childBinaryExprOperation.has_value() || !isArithmeticOperation(*childBinaryExprOperation)) {
        /*
         * In case our expression is of the form t op c where op is a commutative operation we rewrite our expression as
         * c op t and retry our simplification
         */ 
        if (rOperandConstValue.has_value()) {
            return simplifyBinaryExpression(createBinaryExpression(binaryExpr->rhs, binaryExpr->op, binaryExpr->lhs));
        }
        
        return simplifyBinaryExpressionIfSimplificationOfOperandsResultsInNewOperands(binaryExpr);
    }

    const auto splitOfChildBinaryExpr        = trySplitExpressionIntoConstantAndNonExprOperand(*childBinaryExpression);
    const auto constOperandOfChildBinaryExpr = splitOfChildBinaryExpr.has_value() ? std::make_optional(std::get<unsigned int>(*splitOfChildBinaryExpr)) : std::nullopt;
    
    if (binaryOperationOfExpr == syrec_operation::operation::multiplication && isOperationCombinedWithMultiplicationDistributive(*childBinaryExprOperation)) {
        if (constOperandOfParentExpr.has_value() && constOperandOfChildBinaryExpr.has_value()) {
            /*
            * Cases:
            * I.   (c1 * (c1 * y))
            * II.  (c1 * (x * c2))
            * III. ((c2 * y) * c1)
            * IV.  ((x * c2) * c1)
            * all simplify to: ((c1 * c2) * x)
            *
            */
            const auto productOfParentAndChildConstOperand = apply(
                    syrec_operation::operation::multiplication,
                    *constOperandOfParentExpr,
                    *constOperandOfChildBinaryExpr);

            if (*childBinaryExprOperation == syrec_operation::operation::multiplication) {
                return createBinaryExpression(
                        createExpressionForNumber(*productOfParentAndChildConstOperand),
                        (*childBinaryExpression)->op,
                        std::get<syrec::expression::ptr>(*splitOfChildBinaryExpr));
            }

            /*
            * Cases:
            * I.   (c1 * (c1 op y))
            * II.  (c1 * (x op c2))
            * III. ((c2 op y) * c1)
            * IV.  ((x op c2) * c1)
            * all simplify to: ((c1 op c2) * (c1 op x))
            *
            */
            return createBinaryExpression(
                    createExpressionForNumber(*productOfParentAndChildConstOperand),
                    (*childBinaryExpression)->op,
                    simplifyBinaryExpression(
                            createBinaryExpression(
                                    *nonNestedExpressionOperandOfParentExpr,
                                    syrec::BinaryExpression::Multiply,
                                    std::get<syrec::expression::ptr>(*splitOfChildBinaryExpr))));
        }

        if (*childBinaryExprOperation == syrec_operation::operation::multiplication) {
            /*
            * If no other simplification rule can be applied we rewrite our expression (t * (t1 * t2)) as ((t * t1) * t2)
            */
            if (*childBinaryExpression == binaryExpr->rhs) {
                return createBinaryExpression(
                        simplifyBinaryExpression(
                                createBinaryExpression(
                                        binaryExpr->lhs,
                                        syrec::BinaryExpression::Multiply,
                                        (*childBinaryExpression)->lhs)),
                        syrec::BinaryExpression::Multiply,
                        (*childBinaryExpression)->rhs);
            }
        }

        /*
        * Otherwise apply distributivity rule:
        * I.  (x op y) * a
        * II. a * (x op y)
        * all 'simplify' to: ((x * a) op (y * a))
        */
        if (constOperandOfParentExpr.has_value()) {
            return createBinaryExpression(
                    simplifyBinaryExpression(
                            createBinaryExpression(
                                    *nonNestedExpressionOperandOfParentExpr,
                                    syrec::BinaryExpression::Multiply,
                                    (*childBinaryExpression)->lhs)),
                    (*childBinaryExpression)->op,
                    simplifyBinaryExpression(
                            createBinaryExpression(
                                    *nonNestedExpressionOperandOfParentExpr,
                                    syrec::BinaryExpression::Multiply,
                                    (*childBinaryExpression)->rhs)));
        }
    }

    if (binaryOperationOfExpr == syrec_operation::operation::addition && childBinaryExprOperation == syrec_operation::operation::addition) {
        if (constOperandOfChildBinaryExpr.has_value() && constOperandOfParentExpr.has_value()) {
            /*
            * The cases:
            * I.   c1 + (c2 + y)
            * II.  c1 + (x + c2)
            * III. (x + c2) + c1
            * IV.  (c2 + x) + c1
            * all simplify to:
            * ((c1 + c2) + x)
            */
            const auto sumOfParentAndChildConstOperand = apply(
                    syrec_operation::operation::addition,
                    *constOperandOfParentExpr,
                    *constOperandOfChildBinaryExpr);

            return simplifyBinaryExpression(
                    createBinaryExpression(
                            createExpressionForNumber(*sumOfParentAndChildConstOperand),
                            syrec::BinaryExpression::Add,
                            std::get<syrec::expression::ptr>(*splitOfChildBinaryExpr)));
        }

        /*
         * If no other simplification rule can be applied we rewrite our expression (t + (t1 + t2)) as ((t + t1) + t2)
         */
        if (*childBinaryExpression == binaryExpr->rhs) {
            return createBinaryExpression(
                    simplifyBinaryExpression(createBinaryExpression(binaryExpr->lhs, syrec::BinaryExpression::Add, (*childBinaryExpression)->lhs)),
                    syrec::BinaryExpression::Add,
                    (*childBinaryExpression)->rhs);    
        }
    }

    /*
     * In case our expression is of the form t op c where op is a commutative operation we rewrite our expression as
     * c op t and retry our simplification
     */
    if (rOperandConstValue.has_value()) {
        return simplifyBinaryExpression(createBinaryExpression(binaryExpr->rhs, binaryExpr->op, binaryExpr->lhs));
    }

    return binaryExpr;
}