#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/expression_to_subassignment_splitter.hpp"

using namespace noAdditionalLineSynthesis;

syrec::AssignStatement::vec ExpressionToSubAssignmentSplitter::createSubAssignmentsBySplitOfExpr(const syrec::VariableAccess::ptr& assignedToSignal, syrec_operation::operation initialAssignmentOperation, const syrec::expression::ptr& expr) {
    resetInternals();
    if (const bool isGivenOperationAssignmentOperation = syrec_operation::isOperationAssignmentOperation(initialAssignmentOperation); isGivenOperationAssignmentOperation) {
        init(assignedToSignal, initialAssignmentOperation);
        if (!handleExpr(transformExprBeforeProcessing(expr))) {
            storeAssignment(createAssignmentFrom(fixedAssignmentLhs, initialAssignmentOperation, expr));
        }
    }
    return generatedAssignments;
}

void ExpressionToSubAssignmentSplitter::resetInternals() {
    operationInversionFlag = false;
    generatedAssignments.clear();
    fixedSubAssignmentOperationsQueue.clear();
    fixedAssignmentLhs = nullptr;
}

void ExpressionToSubAssignmentSplitter::init(const syrec::VariableAccess::ptr& fixedAssignedToSignalParts, syrec_operation::operation initialAssignmentOperation) {
    fixedAssignmentLhs = fixedAssignedToSignalParts;
    operationInversionFlag = initialAssignmentOperation == syrec_operation::operation::MinusAssign;
    fixedSubAssignmentOperationsQueue.emplace_back(initialAssignmentOperation);
}

void ExpressionToSubAssignmentSplitter::updateOperationInversionFlag(syrec_operation::operation operationDeterminingInversionFlag) {
    operationInversionFlag = operationDeterminingInversionFlag == syrec_operation::operation::Subtraction || operationDeterminingInversionFlag == syrec_operation::operation::MinusAssign;
}

bool ExpressionToSubAssignmentSplitter::handleExpr(const syrec::expression::ptr& expr) {
    if (const std::optional<std::pair<bool, bool>> optionalLeafNodeStatusPerOperandOfExpr = determineLeafNodeStatusForOperandsOfExpr(*expr); optionalLeafNodeStatusPerOperandOfExpr.has_value()) {
        if (optionalLeafNodeStatusPerOperandOfExpr->first && optionalLeafNodeStatusPerOperandOfExpr->second) {
            return handleExprWithTwoLeafNodes(expr);
        }
        if (optionalLeafNodeStatusPerOperandOfExpr->first || optionalLeafNodeStatusPerOperandOfExpr->second) {
            return handleExprWithOneLeafNode(expr);            
        }
        return handleExprWithNoLeafNodes(expr);
    }
    return false;
}

bool ExpressionToSubAssignmentSplitter::handleExprWithNoLeafNodes(const syrec::expression::ptr& expr) {
    const std::shared_ptr<syrec::BinaryExpression> exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr);
    if (!exprAsBinaryExpr) {
        return false;
    }

    const syrec_operation::operation definedBinaryOperation = *syrec_operation::tryMapBinaryOperationFlagToEnum(exprAsBinaryExpr->op);
    const std::optional<syrec_operation::operation> assignmentOperationOfRhsOperand = determineAssignmentOperationToUse(definedBinaryOperation);
    if (!assignmentOperationOfRhsOperand.has_value()) {
        return false;   
    }

    bool       currentInversionFlagStatus = createBackupOfInversionFlagStatus();
    const bool couldLhsOperandBeHandled   = handleExpr(transformExprBeforeProcessing(exprAsBinaryExpr->lhs));
    restorePreviousInversionStatusFlag(currentInversionFlagStatus);
    if (!couldLhsOperandBeHandled) {
        return false;
    }

    fixNextSubAssignmentOperation(*assignmentOperationOfRhsOperand);
    updateOperationInversionFlag(definedBinaryOperation);
    currentInversionFlagStatus = createBackupOfInversionFlagStatus();
    const bool couldRhsOperandBeHandled   = handleExpr(transformExprBeforeProcessing(exprAsBinaryExpr->rhs));
    restorePreviousInversionStatusFlag(currentInversionFlagStatus);
    if (!couldRhsOperandBeHandled) {
        return false;
    }
    return true;
}

bool ExpressionToSubAssignmentSplitter::handleExprWithOneLeafNode(const syrec::expression::ptr& expr) {
    const std::shared_ptr<syrec::BinaryExpression> exprAsBinaryExpr                       = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr);
    if (!exprAsBinaryExpr) {
        return false;
    }
    const std::optional<std::pair<bool, bool>>      optionalLeafNodeStatusPerOperandOfExpr = determineLeafNodeStatusForOperandsOfExpr(*expr);
    const std::optional<syrec_operation::operation> assignmentOperationOfLhsExpr           = getOperationOfNextSubAssignment();
    if (!assignmentOperationOfLhsExpr) {
        return false;
    }

    const syrec_operation::operation                definedBinaryOperation                = *syrec_operation::tryMapBinaryOperationFlagToEnum(exprAsBinaryExpr->op);
    const std::optional<syrec_operation::operation> assignmentOperationForBinaryOperation = syrec_operation::getMatchingAssignmentOperationForOperation(definedBinaryOperation);

    if (assignmentOperationForBinaryOperation.has_value()) {
        if (optionalLeafNodeStatusPerOperandOfExpr.has_value() && optionalLeafNodeStatusPerOperandOfExpr->first) {
            if (const std::optional<syrec_operation::operation> assignmentOperationOfRhsOperand = determineAssignmentOperationToUse(definedBinaryOperation); assignmentOperationOfRhsOperand.has_value()) {
                storeAssignment(createAssignmentFrom(fixedAssignmentLhs, *assignmentOperationOfLhsExpr, exprAsBinaryExpr->lhs));
                fixNextSubAssignmentOperation(*assignmentOperationOfRhsOperand);
                updateOperationInversionFlag(*assignmentOperationOfRhsOperand);
                if (!handleExpr(transformExprBeforeProcessing(exprAsBinaryExpr->rhs))) {
                    storeAssignment(createAssignmentFrom(fixedAssignmentLhs, *assignmentOperationOfRhsOperand, exprAsBinaryExpr->rhs));
                }
                return true;
            }
        }
        if (optionalLeafNodeStatusPerOperandOfExpr.has_value() && optionalLeafNodeStatusPerOperandOfExpr->second) {
            if (const std::optional<syrec_operation::operation> assignmentOperationOfRhsOperand = determineAssignmentOperationToUse(definedBinaryOperation); assignmentOperationOfRhsOperand.has_value()) {
                const bool currentInversionFlagStatus = createBackupOfInversionFlagStatus();
                if (!handleExpr(transformExprBeforeProcessing(exprAsBinaryExpr->lhs))) {
                    storeAssignment(createAssignmentFrom(fixedAssignmentLhs, *assignmentOperationOfLhsExpr, exprAsBinaryExpr->lhs));
                }
                restorePreviousInversionStatusFlag(currentInversionFlagStatus);

                storeAssignment(createAssignmentFrom(fixedAssignmentLhs, *assignmentOperationOfRhsOperand, exprAsBinaryExpr->rhs));
                return true;
            }
        }
    }
    else {
        storeAssignment(createAssignmentFrom(fixedAssignmentLhs, *assignmentOperationOfLhsExpr, expr));
        return true;
    }
    return false;
}

bool ExpressionToSubAssignmentSplitter::handleExprWithTwoLeafNodes(const syrec::expression::ptr& expr) {
    const std::optional<syrec_operation::operation> assignmentOperationOfLhsExpr = getOperationOfNextSubAssignment();
    if (const auto& exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr); exprAsBinaryExpr && assignmentOperationOfLhsExpr.has_value()) {
        const std::optional<syrec_operation::operation> definedBinaryOperation = syrec_operation::tryMapBinaryOperationFlagToEnum(exprAsBinaryExpr->op);
        const std::optional<syrec_operation::operation> matchingAssignmentOperationForBinaryOperation = definedBinaryOperation.has_value()
            ? syrec_operation::getMatchingAssignmentOperationForOperation(*definedBinaryOperation)
            : std::nullopt;

        if (matchingAssignmentOperationForBinaryOperation.has_value()) {
            if (const std::optional<syrec_operation::operation> assignmentOperationOfRhsExpr = determineAssignmentOperationToUse(*definedBinaryOperation); assignmentOperationOfRhsExpr.has_value()) {
                syrec::expression::ptr transformedLhsExpr = transformExprBeforeProcessing(exprAsBinaryExpr->lhs);
                bool                   handlingOfOperandsOk = true;
                if (transformedLhsExpr != exprAsBinaryExpr->lhs) {
                    const bool currentInversionFlagStatus = createBackupOfInversionFlagStatus();
                    updateOperationInversionFlag(*definedBinaryOperation);
                    if (!handleExpr(transformedLhsExpr)) {
                        transformedLhsExpr = exprAsBinaryExpr->lhs;
                        handlingOfOperandsOk = false;
                    }
                    restorePreviousInversionStatusFlag(currentInversionFlagStatus);
                }

                syrec::expression::ptr transformedRhsExpr = transformExprBeforeProcessing(exprAsBinaryExpr->rhs);
                if (transformedRhsExpr != exprAsBinaryExpr->rhs && handlingOfOperandsOk) {
                    const bool currentInversionFlagStatus = createBackupOfInversionFlagStatus();
                    updateOperationInversionFlag(*definedBinaryOperation);
                    if (!handleExpr(transformedRhsExpr)) {
                        transformedRhsExpr = exprAsBinaryExpr->rhs;
                        handlingOfOperandsOk = false;
                    }
                    restorePreviousInversionStatusFlag(currentInversionFlagStatus);
                }

                if (!handlingOfOperandsOk) {
                    storeAssignment(createAssignmentFrom(fixedAssignmentLhs, *assignmentOperationOfLhsExpr, transformExprBeforeProcessing(exprAsBinaryExpr->rhs)));
                    storeAssignment(createAssignmentFrom(fixedAssignmentLhs, *assignmentOperationOfRhsExpr, transformExprBeforeProcessing(exprAsBinaryExpr->rhs)));   
                }
            }
        }
        storeAssignment(createAssignmentFrom(fixedAssignmentLhs, *assignmentOperationOfLhsExpr, expr));
        return true;   
    }
    return false;
}

void ExpressionToSubAssignmentSplitter::storeAssignment(const syrec::AssignStatement::ptr& assignment) {
    generatedAssignments.emplace_back(assignment);
}

std::optional<syrec_operation::operation> ExpressionToSubAssignmentSplitter::getOperationOfNextSubAssignment() {
    if (fixedSubAssignmentOperationsQueueIdx >= fixedSubAssignmentOperationsQueue.size()) {
        return std::nullopt;
    }

    const syrec_operation::operation operationInQueue = fixedSubAssignmentOperationsQueue.at(fixedSubAssignmentOperationsQueueIdx++);
    if (syrec_operation::isOperationAssignmentOperation(operationInQueue)) {
        return operationInQueue;
    }
    return syrec_operation::getMatchingAssignmentOperationForOperation(operationInQueue);
}


bool ExpressionToSubAssignmentSplitter::createBackupOfInversionFlagStatus() const {
    return operationInversionFlag;
}

void ExpressionToSubAssignmentSplitter::restorePreviousInversionStatusFlag(bool previousInversionStatus) {
    operationInversionFlag = previousInversionStatus;
}

void ExpressionToSubAssignmentSplitter::fixNextSubAssignmentOperation(syrec_operation::operation operation) {
    fixedSubAssignmentOperationsQueue.emplace_back(operation);
}

syrec::AssignStatement::ptr ExpressionToSubAssignmentSplitter::createAssignmentFrom(const syrec::VariableAccess::ptr& assignedToSignalParts, syrec_operation::operation operation, const syrec::expression::ptr& assignmentRhsExpr) {
    if (!syrec_operation::isOperationAssignmentOperation(operation)) {
        operation = *syrec_operation::getMatchingAssignmentOperationForOperation(operation);
    }
    syrec::AssignStatement::ptr generatedAssignment = std::make_shared<syrec::AssignStatement>(assignedToSignalParts, *syrec_operation::tryMapAssignmentOperationEnumToFlag(operation), assignmentRhsExpr);
    return generatedAssignment;
}

bool ExpressionToSubAssignmentSplitter::doesExprDefineSubExpression(const syrec::expression& expr) {
    return dynamic_cast<const syrec::BinaryExpression*>(&expr) != nullptr;
}

std::optional<syrec_operation::operation> ExpressionToSubAssignmentSplitter::determineAssignmentOperationToUse(syrec_operation::operation operation) const {
    std::optional<syrec_operation::operation> matchingAssignmentOperation = syrec_operation::getMatchingAssignmentOperationForOperation(operation);
    if (matchingAssignmentOperation.has_value() && operationInversionFlag) {
        matchingAssignmentOperation = syrec_operation::invert(*matchingAssignmentOperation);
    }
    return matchingAssignmentOperation;
}

bool ExpressionToSubAssignmentSplitter::doesOperandDefineLeafNode(const syrec::expression& expr) {
    return !doesExprDefineSubExpression(expr);
}

std::optional<std::pair<bool, bool>> ExpressionToSubAssignmentSplitter::determineLeafNodeStatusForOperandsOfExpr(const syrec::expression& expr) {
    if (!doesExprDefineSubExpression(expr)) {
        return std::nullopt;
    }

    if (const auto& exprAsBinaryExpr = dynamic_cast<const syrec::BinaryExpression*>(&expr); exprAsBinaryExpr) {
        return std::make_pair(!doesExprDefineSubExpression(*exprAsBinaryExpr->lhs), !doesExprDefineSubExpression(*exprAsBinaryExpr->rhs));
    }
    if (const auto& exprAsShiftExpr = dynamic_cast<const syrec::ShiftExpression*>(&expr); exprAsShiftExpr) {
        return std::make_pair(!doesExprDefineSubExpression(*exprAsShiftExpr->lhs), false);
    }
    return std::nullopt;
}

std::optional<syrec::BinaryExpression::ptr> ExpressionToSubAssignmentSplitter::convertNumericExprToBinary(const syrec::NumericExpression& numericExpr) {
    if (!numericExpr.value->isCompileTimeConstantExpression()) {
        return std::nullopt;
    }
    return convertCompileTimeConstantExprToBinaryExpr(numericExpr.value->getExpression(), numericExpr.bwidth);
}

std::optional<syrec_operation::operation> ExpressionToSubAssignmentSplitter::tryMapCompileTimeConstantExprOperationToBinaryOperation(syrec::Number::CompileTimeConstantExpression::Operation operation) {
    switch (operation) {
        case syrec::Number::CompileTimeConstantExpression::Addition:
            return syrec_operation::operation::Addition;
        case syrec::Number::CompileTimeConstantExpression::Subtraction:
            return syrec_operation::operation::Subtraction;
        case syrec::Number::CompileTimeConstantExpression::Multiplication:
            return syrec_operation::operation::Multiplication;
        case syrec::Number::CompileTimeConstantExpression::Division:
        default:
            return std::nullopt;
    }
}

syrec::expression::ptr ExpressionToSubAssignmentSplitter::convertNumberToExpr(const syrec::Number::ptr& number, unsigned int expectedBitwidth) {
    if (number->isCompileTimeConstantExpression()) {
        return nullptr;
    }

    const syrec::NumericExpression::ptr generatedExpr = std::make_shared<syrec::NumericExpression>(number, expectedBitwidth);
    return generatedExpr;
}

std::optional<syrec::BinaryExpression::ptr> ExpressionToSubAssignmentSplitter::convertCompileTimeConstantExprToBinaryExpr(const syrec::Number::CompileTimeConstantExpression& compileTimeConstantExpr, unsigned int expectedBitwidth) {
    const std::optional<syrec_operation::operation>     mappedToOperation                    = tryMapCompileTimeConstantExprOperationToBinaryOperation(compileTimeConstantExpr.operation);
    if (!mappedToOperation.has_value()) {
        return std::nullopt;
    }

    const syrec::expression::ptr& lhsOperandConverted = convertNumberToExpr(compileTimeConstantExpr.lhsOperand, expectedBitwidth);
    const syrec::expression::ptr& rhsOperandConverted = convertNumberToExpr(compileTimeConstantExpr.rhsOperand, expectedBitwidth);
    if (!lhsOperandConverted || !rhsOperandConverted) {
        return std::nullopt;
    }

    const syrec::BinaryExpression::ptr generatedExpr = std::make_shared<syrec::BinaryExpression>(lhsOperandConverted, *syrec_operation::tryMapBinaryOperationEnumToFlag(*mappedToOperation), rhsOperandConverted);
    return generatedExpr;
    
}

syrec::expression::ptr ExpressionToSubAssignmentSplitter::transformExprBeforeProcessing(const syrec::expression::ptr& initialExpr) {
    if (const auto& exprAsNumericExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(initialExpr); exprAsNumericExpr) {
        if (const auto& exprConverted = convertNumericExprToBinary(*exprAsNumericExpr); exprConverted.has_value()) {
            return *exprConverted;
        }
    }
    return initialExpr;
}
