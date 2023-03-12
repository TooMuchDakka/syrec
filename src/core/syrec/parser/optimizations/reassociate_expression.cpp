#include "core/syrec/parser/optimizations/reassociate_expression.hpp"
#include "core/syrec/parser/parser_utilities.hpp"

using namespace optimization;

bool isArithmeticOperation(syrec_operation::operation operation) {
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

syrec::expression::ptr simplifyBinaryExpression(const std::shared_ptr<syrec::BinaryExpression>& binaryExpr) {
    const auto lOperandConstValue = tryGetConstantValueOfExpression(binaryExpr->lhs);
    const auto rOperandConstValue = tryGetConstantValueOfExpression(binaryExpr->rhs);

    const auto                 mappedFlagToEnum    = parser::ParserUtilities::mapInternalBinaryOperationFlagToEnum(binaryExpr->op);
    if (!mappedFlagToEnum.has_value()) {
        return binaryExpr;
    }

    const syrec_operation::operation binaryExprOperation = mappedFlagToEnum.value();
    if (lOperandConstValue.has_value() && rOperandConstValue.has_value()) {
        const auto simplificationResult = trySimplifyConstantExpression(lOperandConstValue.value(), binaryExprOperation, rOperandConstValue.value());
        if (simplificationResult.has_value()) {
            // TODO: Bitwidth
            return createExpressionForNumber(simplificationResult.value());
        }
    }

    if (binaryExprOperation == syrec_operation::operation::subtraction) {
        if (rOperandConstValue.has_value()) {
            return simplifyBinaryExpression(std::make_shared<syrec::BinaryExpression>(createExpressionForNumber(-rOperandConstValue.value()), syrec::BinaryExpression::Add, binaryExpr->lhs));    
        }
        if (lOperandConstValue.has_value()) {
            return simplifyBinaryExpression(std::make_shared<syrec::BinaryExpression>(createExpressionForNumber(-lOperandConstValue.value()), syrec::BinaryExpression::Add, binaryExpr->rhs));
        }

        return simplifyBinaryExpression(std::make_shared<syrec::BinaryExpression>(simplifyBinaryExpression(binaryExpr->lhs), syrec::BinaryExpression::Subtract, simplifyBinaryExpression(binaryExpr->rhs)));
    }
    
    if ((binaryExprOperation != syrec_operation::operation::multiplication && binaryExprOperation != syrec_operation::operation::addition) 
        || (!lOperandConstValue.has_value() && !rOperandConstValue.has_value())) {
        return simplifyBinaryExpression(std::make_shared<syrec::BinaryExpression>(simplifyBinaryExpression(binaryExpr->lhs), binaryExpr->op, simplifyBinaryExpression(binaryExpr->rhs)));
    }

    unsigned int parentExprConstOperand;
    syrec::expression::ptr exprToCheckForDistributivity;

    if (lOperandConstValue.has_value()) {
        parentExprConstOperand = rOperandConstValue.value();
        exprToCheckForDistributivity = binaryExpr->rhs;
    }
    else {
        parentExprConstOperand = lOperandConstValue.value();
        exprToCheckForDistributivity = binaryExpr->lhs;
    }

    std::optional<std::shared_ptr<syrec::BinaryExpression>> childBinaryExpression;
    if (const auto childOperandAsBinaryExpression = std::dynamic_pointer_cast<syrec::BinaryExpression>(exprToCheckForDistributivity); childOperandAsBinaryExpression != nullptr) {
        childBinaryExpression.emplace(childOperandAsBinaryExpression);
    }

    const auto                 childBinaryExprOperationMapped = parser::ParserUtilities::mapInternalBinaryOperationFlagToEnum((*childBinaryExpression)->op);
    if (!childBinaryExprOperationMapped.has_value()) {
        return simplifyBinaryExpression(std::make_shared<syrec::BinaryExpression>(simplifyBinaryExpression(binaryExpr->lhs), binaryExpr->op, simplifyBinaryExpression(binaryExpr->rhs)));
    }

    const syrec_operation::operation childBinaryExprOperation = childBinaryExprOperationMapped.value();
    if (!childBinaryExpression.has_value() || !isArithmeticOperation(childBinaryExprOperation)) {
        return simplifyBinaryExpression(std::make_shared<syrec::BinaryExpression>(simplifyBinaryExpression(binaryExpr->lhs), binaryExpr->op, simplifyBinaryExpression(binaryExpr->rhs)));
    }

    const auto childBinaryExprLOperandConstValue = tryGetConstantValueOfExpression(childBinaryExpression.value()->lhs);
    const auto childBinaryExprROperandConstValue = childBinaryExprLOperandConstValue.has_value() ? std::nullopt : tryGetConstantValueOfExpression(childBinaryExpression.value()->rhs);
    const bool hasChildBinaryExprAnyConstantOperand = childBinaryExprLOperandConstValue.has_value() || childBinaryExprROperandConstValue.has_value();

    // IS a * (b / c) commutative ?
    if (binaryExprOperation == syrec_operation::operation::multiplication && isOperationCombinedWithMultiplicationDistributive(*childBinaryExprOperationMapped)) {
        if (hasChildBinaryExprAnyConstantOperand) {
            const auto productOfParentAndChildConstOperand = trySimplifyConstantExpression(parentExprConstOperand, syrec_operation::operation::multiplication, childBinaryExprLOperandConstValue.has_value() ? childBinaryExprLOperandConstValue.value() : childBinaryExprROperandConstValue.value());

            return simplifyBinaryExpression(
                std::make_shared<syrec::BinaryExpression>(
                    createExpressionForNumber(productOfParentAndChildConstOperand.value()), 
                    (*childBinaryExpression)->op,
                    childBinaryExprLOperandConstValue.has_value() ? (*childBinaryExpression)->rhs : (*childBinaryExpression)->lhs)
            );
        } 
        return simplifyBinaryExpression(
            std::make_shared<syrec::BinaryExpression>(
                std::make_shared<syrec::BinaryExpression>(createExpressionForNumber(parentExprConstOperand), syrec::BinaryExpression::Multiply, (*childBinaryExpression)->lhs),
                (*childBinaryExpression)->op,
                std::make_shared<syrec::BinaryExpression>(createExpressionForNumber(parentExprConstOperand), syrec::BinaryExpression::Multiply, (*childBinaryExpression)->rhs))
        );   
    }

    if (binaryExprOperation == syrec_operation::operation::addition && childBinaryExprOperation == syrec_operation::operation::addition && hasChildBinaryExprAnyConstantOperand) {
        const auto sumOfParentAndChildConstOperand = trySimplifyConstantExpression(parentExprConstOperand, syrec_operation::operation::addition, childBinaryExprLOperandConstValue.has_value() ? childBinaryExprLOperandConstValue.value() : childBinaryExprROperandConstValue.value());
        return simplifyBinaryExpression(
            std::make_shared<syrec::BinaryExpression>(
                createExpressionForNumber(sumOfParentAndChildConstOperand.value()),
                syrec::BinaryExpression::Add,
                std::make_shared<syrec::BinaryExpression>(
                    createExpressionForNumber(parentExprConstOperand),
                    syrec::BinaryExpression::Add,
                    childBinaryExprLOperandConstValue.has_value() ? (*childBinaryExpression)->rhs : (*childBinaryExpression)->lhs
                    )
            )
        );
    }

    return binaryExpr;
}