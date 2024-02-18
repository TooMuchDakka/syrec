#include "core/syrec/parser/optimizations/operation_strength_reduction.hpp"
#include "core/syrec/parser/operation.hpp"

using namespace operationStrengthReduction;

bool operationStrengthReduction::tryPerformOperationStrengthReduction(syrec::Expression::ptr& expr) {
    if (const auto& exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr); exprAsBinaryExpr != nullptr) {
        const auto mappedToBinaryExpr = syrec_operation::tryMapBinaryOperationFlagToEnum(exprAsBinaryExpr->op);
        if (!mappedToBinaryExpr.has_value() || *mappedToBinaryExpr != syrec_operation::operation::Multiplication || *mappedToBinaryExpr != syrec_operation::operation::Division) {
            return false;
        }

        if (const auto& binaryExprRhsAsNumericExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(exprAsBinaryExpr->rhs); binaryExprRhsAsNumericExpr != nullptr) {
            if (!binaryExprRhsAsNumericExpr->value->isConstant()) {
                return false;
            }

            const auto constantValueOfBinaryExprRhs = binaryExprRhsAsNumericExpr->value->evaluate({});
            if (constantValueOfBinaryExprRhs % 2 == 0) {
                const auto shiftAmount = (constantValueOfBinaryExprRhs / 2) + 1;
                const auto shiftOperation = *mappedToBinaryExpr == syrec_operation::operation::Multiplication ? syrec_operation::operation::ShiftLeft : syrec_operation::operation::ShiftRight;
                exprAsBinaryExpr->op  = *syrec_operation::tryMapShiftOperationEnumToFlag(shiftOperation);
                exprAsBinaryExpr->rhs = std::make_shared<syrec::NumericExpression>(std::make_shared<syrec::Number>(shiftAmount), exprAsBinaryExpr->rhs->bitwidth());
                expr                  = exprAsBinaryExpr;
                return true;
            }
        }
    }
    return false;
}

std::optional<std::unique_ptr<syrec::AssignStatement>> AssignmentOperationStrengthReductionResult::getResultAsBinaryAssignment() {
    if (std::holds_alternative<std::unique_ptr<syrec::AssignStatement>>(result)) {
        return std::move(std::get<std::unique_ptr<syrec::AssignStatement>>(result));
    }
    return std::nullopt;
}

std::optional<std::unique_ptr<syrec::UnaryStatement>> AssignmentOperationStrengthReductionResult::getResultAsUnaryAssignment() {
    if (std::holds_alternative<std::unique_ptr<syrec::UnaryStatement>>(result)) {
        return std::move(std::get<std::unique_ptr<syrec::UnaryStatement>>(result));
    }
    return std::nullopt;
}

std::optional<operationStrengthReduction::AssignmentOperationStrengthReductionResult> operationStrengthReduction::performOperationStrengthReduction(const syrec::AssignStatement& assignment, const std::optional<unsigned>& currentValueOfAssignedToSignalPriorToAssignment) {
    if (!currentValueOfAssignedToSignalPriorToAssignment.has_value()) {
        return std::nullopt;
    }

    const std::optional<syrec_operation::operation> mappedToAssignmentOperationEnumValueFromFlag = syrec_operation::tryMapAssignmentOperationFlagToEnum(assignment.op);
    if (!mappedToAssignmentOperationEnumValueFromFlag.has_value()) {
        return std::nullopt;
    }

    const std::shared_ptr<syrec::NumericExpression> assignmentRhsExprAsNumericExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(assignment.rhs);
    if (assignmentRhsExprAsNumericExpr && assignmentRhsExprAsNumericExpr->value->isConstant() && assignmentRhsExprAsNumericExpr->value->evaluate({}) == 1) {
        if (const std::optional<unsigned int> mappedToSimplifiedAddAssignmentOperationFlag = syrec_operation::tryMapAssignmentOperationEnumToFlag(syrec_operation::operation::IncrementAssign); mappedToSimplifiedAddAssignmentOperationFlag.has_value() && *mappedToAssignmentOperationEnumValueFromFlag == syrec_operation::operation::AddAssign) {
            if (std::unique_ptr<syrec::UnaryStatement> simplifiedAssignment = std::make_unique<syrec::UnaryStatement>(*mappedToSimplifiedAddAssignmentOperationFlag, assignment.lhs); simplifiedAssignment) {
                return AssignmentOperationStrengthReductionResult(std::move(simplifiedAssignment));
            }
        } else if (const std::optional<unsigned int> mappedToSimplifiedMinusAssignmentOperationFlag = syrec_operation::tryMapAssignmentOperationEnumToFlag(syrec_operation::operation::DecrementAssign); mappedToSimplifiedMinusAssignmentOperationFlag.has_value() && *mappedToAssignmentOperationEnumValueFromFlag == syrec_operation::operation::MinusAssign) {
            if (std::unique_ptr<syrec::UnaryStatement> simplifiedAssignment = std::make_unique<syrec::UnaryStatement>(*mappedToSimplifiedMinusAssignmentOperationFlag, assignment.lhs); simplifiedAssignment) {
                return AssignmentOperationStrengthReductionResult(std::move(simplifiedAssignment));
            }
        }
    }
    
    if (*mappedToAssignmentOperationEnumValueFromFlag == syrec_operation::operation::AddAssign && currentValueOfAssignedToSignalPriorToAssignment.has_value() && !*currentValueOfAssignedToSignalPriorToAssignment) {
        if (const std::optional<unsigned int> mappedToFlagValueForXorAssignmentOperation = syrec_operation::tryMapAssignmentOperationEnumToFlag(syrec_operation::operation::XorAssign); mappedToFlagValueForXorAssignmentOperation.has_value()) {
            if (std::unique_ptr<syrec::AssignStatement> modifiedAssignment = std::make_unique<syrec::AssignStatement>(assignment.lhs, *mappedToFlagValueForXorAssignmentOperation, assignment.rhs); modifiedAssignment) {
                return AssignmentOperationStrengthReductionResult(std::move(modifiedAssignment));
            }
        }
        return std::nullopt;
    }
    return std::nullopt;
}