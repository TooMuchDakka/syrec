#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/expression_to_subassignment_splitter.hpp"

using namespace noAdditionalLineSynthesis;

// TODO: Tests for split of subassignments when topmost one is subtraction (i.e. if nested expression contains operations without assignment equivalent)
// TODO: Implementation of splits for XOR operations does not work correctly [see simplificationWithOnlyReversibleOpsAndUniqueSignalAccessHandlesCreatesAssignmentForOperationNodeWithLhsOperandBeingSignalAccessAndNonLeafCreatingAssignment]
//  should we again perform a swap of the operands for an expression of the form (a ^ <subExpr>) to (<subExpr> ^ a)

// Handling or split of subexpressions should also work under for this example currently does not
syrec::AssignStatement::vec ExpressionToSubAssignmentSplitter::createSubAssignmentsBySplitOfExpr(const syrec::AssignStatement::ptr& assignment, const std::optional<unsigned int>& currentValueOfAssignedToSignal) {
    resetInternals();
    const auto& assignmentCasted = std::dynamic_pointer_cast<syrec::AssignStatement>(assignment);
    bool        wasOriginalXorAssignOperationTransformedToAddAssignOp = false;
    if (assignmentCasted) {
        transformTwoSubsequentMinusOperations(*assignmentCasted);
        // We have to perform this sort of revert of a previous optimization so that our split precondition holds.
        wasOriginalXorAssignOperationTransformedToAddAssignOp = transformXorOperationToAddAssignOperationIfTopmostOpOfRhsExprIsNotBitwiseXor(*assignmentCasted, currentValueOfAssignedToSignal);
        transformAddAssignOperationToXorAssignOperationIfAssignedToSignalValueIsZeroAndTopmostOpOfRhsExprIsBitwiseXor(*assignmentCasted, currentValueOfAssignedToSignal);
    }
    const std::optional<syrec_operation::operation> mappedToAssignmentOperationFromFlag = assignmentCasted ? syrec_operation::tryMapAssignmentOperationFlagToEnum(assignmentCasted->op) : std::nullopt;
    if (mappedToAssignmentOperationFromFlag.has_value() && isPreconditionSatisfied(*mappedToAssignmentOperationFromFlag, *assignmentCasted->rhs, currentValueOfAssignedToSignal)) {
        /*
         * Since a prior transformation of the XOR_ASSIGN to an ADD_ASSIGN operation could have taken place to satisfy the precondition, we use a flag
         * to revert this transformation to fix the assignment operation for the first sub operand of the rhs expression of the original assignment that would otherwise
         * be an ADD_ASSIGN operation instead.
         */
        init(assignmentCasted->lhs, wasOriginalXorAssignOperationTransformedToAddAssignOp ? syrec_operation::operation::XorAssign : *mappedToAssignmentOperationFromFlag, currentValueOfAssignedToSignal);
        if (!handleExpr(assignmentCasted->rhs)) {
            storeAssignment(createAssignmentFrom(fixedAssignmentLhs, *mappedToAssignmentOperationFromFlag, assignmentCasted->rhs));
        }
    } else {
        storeAssignment(assignment);
    }
    return generatedAssignments;
}

bool ExpressionToSubAssignmentSplitter::isPreconditionSatisfied(syrec_operation::operation operation, const syrec::expression& topmostAssignmentRhsExpr, const std::optional<unsigned int>& currentValueOfAssignedToSignal) const {
    if (!syrec_operation::isOperationAssignmentOperation(operation)) {
        return false;
    }
    const auto& topmostExprOfRhsAsBinaryExpr = dynamic_cast<const syrec::BinaryExpression*>(&topmostAssignmentRhsExpr);
    if (!topmostExprOfRhsAsBinaryExpr) {
        return true;
    }

    syrec::expression::ptr                          expressionToCheck      = std::make_shared<syrec::BinaryExpression>(topmostExprOfRhsAsBinaryExpr->lhs, topmostExprOfRhsAsBinaryExpr->op, topmostExprOfRhsAsBinaryExpr->rhs);
    const std::optional<syrec_operation::operation> operationOfTopmostExpr = syrec_operation::tryMapBinaryOperationFlagToEnum(topmostExprOfRhsAsBinaryExpr->op);
    if (!operationOfTopmostExpr.has_value()) {
        return false;
    }

    /*
     * We perform an internal transformation of the given assignment operation to satisfy the precondition in the following case:
     * I. If the assignment operation '+=' is used while the rhs expr defines a binary expression with the '^' operation, will transform the former to '^=' if the assigned to signal value is 0.
     * II. if the assignment operation '^=' is used while the rhs expr defines a binary expression with an operation not equal to '^=', will transform the former to '+=' if the assigned to signal value is 0.
     */
    if ((currentValueOfAssignedToSignal.has_value() && !*currentValueOfAssignedToSignal && operationOfTopmostExpr.has_value())
        && ((operation == syrec_operation::operation::XorAssign && *operationOfTopmostExpr != syrec_operation::operation::BitwiseXor) 
            || (operation == syrec_operation::operation::AddAssign && *operationOfTopmostExpr == syrec_operation::operation::BitwiseXor)
            || (operation == syrec_operation::operation::XorAssign && *operationOfTopmostExpr == syrec_operation::operation::BitwiseXor))) {
            return true;
    }
    return operation != syrec_operation::operation::XorAssign && *operationOfTopmostExpr != syrec_operation::operation::BitwiseXor;
}

void ExpressionToSubAssignmentSplitter::resetInternals() {
    operationInversionFlag = false;
    generatedAssignments.clear();
    fixedSubAssignmentOperationsQueue.clear();
    fixedAssignmentLhs = nullptr;
    currentAssignedToSignalValue.reset();
}

void ExpressionToSubAssignmentSplitter::init(const syrec::VariableAccess::ptr& fixedAssignedToSignalParts, syrec_operation::operation initialAssignmentOperation, const std::optional<unsigned int>& initialValueOfAssignedToSignalOfOriginalAssignment) {
    fixedAssignmentLhs = fixedAssignedToSignalParts;
    operationInversionFlag = initialAssignmentOperation == syrec_operation::operation::MinusAssign;
    fixedSubAssignmentOperationsQueue.clear();
    fixedSubAssignmentOperationsQueueIdx = 0;
    fixedSubAssignmentOperationsQueue.emplace_back(initialAssignmentOperation);
    currentAssignedToSignalValue = initialValueOfAssignedToSignalOfOriginalAssignment;
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
    if (!assignmentOperationOfRhsOperand.has_value() || (*assignmentOperationOfRhsOperand == syrec_operation::operation::XorAssign && !isSplitOfXorOperationIntoSubAssignmentsAllowedForExpr(*expr))) {
        return false;   
    }

    bool       currentInversionFlagStatus = createBackupOfInversionFlagStatus();
    const bool couldLhsOperandBeHandled   = handleExpr(exprAsBinaryExpr->lhs);
    restorePreviousInversionStatusFlag(currentInversionFlagStatus);
    if (!couldLhsOperandBeHandled) {
        return false;
    }

    // If we skip this check an operation node with two leaf nodes will still be split even if precondition for such a split is not satisfied after the handling of the lhs expr
    // because the split precondition check is not performed when handling an operation node with two leaf nodes
    if (*assignmentOperationOfRhsOperand == syrec_operation::operation::XorAssign && !isSplitOfXorOperationIntoSubAssignmentsAllowedForExpr(*exprAsBinaryExpr->rhs)) {
        storeAssignment(createAssignmentFrom(fixedAssignmentLhs, *assignmentOperationOfRhsOperand, exprAsBinaryExpr->rhs));
        return true;
    }

    fixNextSubAssignmentOperation(*assignmentOperationOfRhsOperand);
    updateOperationInversionFlag(*assignmentOperationOfRhsOperand);
    currentInversionFlagStatus = createBackupOfInversionFlagStatus();
    const bool couldRhsOperandBeHandled   = handleExpr(exprAsBinaryExpr->rhs);
    restorePreviousInversionStatusFlag(currentInversionFlagStatus);
    if (!couldRhsOperandBeHandled) {
        return false;
    }
    return true;
}

bool ExpressionToSubAssignmentSplitter::handleExprWithOneLeafNode(const syrec::expression::ptr& expr) {
    const std::shared_ptr<syrec::BinaryExpression> exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr);
    if (!exprAsBinaryExpr) {
        return false;
    }

    const std::optional<std::pair<bool, bool>>      optionalLeafNodeStatusPerOperandOfExpr = determineLeafNodeStatusForOperandsOfExpr(*expr);
    const syrec_operation::operation                definedBinaryOperation                = *syrec_operation::tryMapBinaryOperationFlagToEnum(exprAsBinaryExpr->op);

    if (!optionalLeafNodeStatusPerOperandOfExpr.has_value()) {
        return false;
    }

    const std::optional<syrec_operation::operation> predefinedAssignmentOperationOfSubAssignment = getOperationOfNextSubAssignment(false);
    if (!predefinedAssignmentOperationOfSubAssignment.has_value()) {
        return false;
    }

    const std::optional<syrec_operation::operation> assignmentOperationOfRhsOperand = determineAssignmentOperationToUse(definedBinaryOperation);
    if (!assignmentOperationOfRhsOperand.has_value()) {
        storeAssignment(createAssignmentFrom(fixedAssignmentLhs, *predefinedAssignmentOperationOfSubAssignment, expr));
        return true;
    }

    if (optionalLeafNodeStatusPerOperandOfExpr->first) {
        if (const std::optional<syrec_operation::operation> assignmentOperationForSubAssignment = getOperationOfNextSubAssignment(); assignmentOperationForSubAssignment.has_value()) {
            storeAssignment(createAssignmentFrom(fixedAssignmentLhs, *assignmentOperationForSubAssignment,exprAsBinaryExpr->lhs));
            if (*assignmentOperationOfRhsOperand == syrec_operation::operation::XorAssign && !isSplitOfXorOperationIntoSubAssignmentsAllowedForExpr(*exprAsBinaryExpr->rhs)) {
                storeAssignment(createAssignmentFrom(fixedAssignmentLhs, syrec_operation::operation::XorAssign, exprAsBinaryExpr->rhs));
                return true;
            }

            fixNextSubAssignmentOperation(*assignmentOperationOfRhsOperand);
            updateOperationInversionFlag(*assignmentOperationOfRhsOperand);
            if (!handleExpr(exprAsBinaryExpr->rhs)) {
                storeAssignment(createAssignmentFrom(fixedAssignmentLhs, *assignmentOperationOfRhsOperand, exprAsBinaryExpr->rhs));
            }
            return true;
        }
    } else {
        const bool currentInversionFlagStatus = createBackupOfInversionFlagStatus();
        if (!handleExpr(exprAsBinaryExpr->lhs)) {
            storeAssignment(createAssignmentFrom(fixedAssignmentLhs, *predefinedAssignmentOperationOfSubAssignment, exprAsBinaryExpr->lhs));
        }
        restorePreviousInversionStatusFlag(currentInversionFlagStatus);
        storeAssignment(createAssignmentFrom(fixedAssignmentLhs, *assignmentOperationOfRhsOperand, exprAsBinaryExpr->rhs));
        return true;    
    }
    return false;
}

bool ExpressionToSubAssignmentSplitter::handleExprWithTwoLeafNodes(const syrec::expression::ptr& expr) {
    const std::optional<syrec_operation::operation> assignmentOperationOfLhsExpr = getOperationOfNextSubAssignment();
    const auto&                                     exprAsBinaryExpr             = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr);

    if (!assignmentOperationOfLhsExpr.has_value() || !exprAsBinaryExpr) {
        return false;
    }

    const std::optional<syrec_operation::operation> definedBinaryOperation = syrec_operation::tryMapBinaryOperationFlagToEnum(exprAsBinaryExpr->op);
    const std::optional<syrec_operation::operation> subAssignmentOperationForRhs = definedBinaryOperation.has_value() ? determineAssignmentOperationToUse(*definedBinaryOperation) : std::nullopt;

    if (!subAssignmentOperationForRhs.has_value() || !definedBinaryOperation.has_value()) {
        storeAssignment(createAssignmentFrom(fixedAssignmentLhs, *assignmentOperationOfLhsExpr, expr));
    } else {
        storeAssignment(createAssignmentFrom(fixedAssignmentLhs, *assignmentOperationOfLhsExpr, exprAsBinaryExpr->lhs));
        storeAssignment(createAssignmentFrom(fixedAssignmentLhs, *subAssignmentOperationForRhs, exprAsBinaryExpr->rhs));        
    }
    return true;
}

void ExpressionToSubAssignmentSplitter::storeAssignment(const syrec::AssignStatement::ptr& assignment) {
    if (const auto& assignmentCasted = std::dynamic_pointer_cast<syrec::AssignStatement>(assignment); assignmentCasted) {
        transformTwoSubsequentMinusOperations(*assignmentCasted);
        updateValueOfAssignedToSignalViaAssignment(syrec_operation::tryMapAssignmentOperationFlagToEnum(assignmentCasted->op), assignmentCasted->rhs.get());
    } else {
        updateValueOfAssignedToSignalViaAssignment(std::nullopt, nullptr);
    }

    generatedAssignments.emplace_back(assignment);
}

std::optional<syrec_operation::operation> ExpressionToSubAssignmentSplitter::getOperationOfNextSubAssignment(bool markAsConsumed) {
    if (fixedSubAssignmentOperationsQueueIdx >= fixedSubAssignmentOperationsQueue.size()) {
        return std::nullopt;
    }

    const syrec_operation::operation operationInQueue = fixedSubAssignmentOperationsQueue.at(fixedSubAssignmentOperationsQueueIdx);
    if (markAsConsumed) {
        ++fixedSubAssignmentOperationsQueueIdx;
    }

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

void ExpressionToSubAssignmentSplitter::updateValueOfAssignedToSignalViaAssignment(const std::optional<syrec_operation::operation>& assignmentOperation, const syrec::expression* assignmentRhsExpr) {
    if (const auto& exprAsNumericExpr = dynamic_cast<const syrec::NumericExpression*>(assignmentRhsExpr); exprAsNumericExpr) {
        if (exprAsNumericExpr->value->isConstant()) {
            currentAssignedToSignalValue = assignmentOperation.has_value()
                ? syrec_operation::apply(*assignmentOperation, currentAssignedToSignalValue, exprAsNumericExpr->value->evaluate({}))
                : std::nullopt;
        }
    }
    else {
        currentAssignedToSignalValue.reset();
    }
}

bool ExpressionToSubAssignmentSplitter::isSplitOfXorOperationIntoSubAssignmentsAllowedForExpr(const syrec::expression& expr) const {
    if (const auto& exprAsNumericExpr = dynamic_cast<const syrec::NumericExpression*>(&expr); exprAsNumericExpr) {
        if (!exprAsNumericExpr->value->isCompileTimeConstantExpression()) {
            return true;
        }
    } else if (const auto& exprAsVariableExpr = dynamic_cast<const syrec::VariableExpression*>(&expr); exprAsVariableExpr) {
        return true;
    }
    return currentAssignedToSignalValue.has_value() && !currentAssignedToSignalValue.value();
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

void ExpressionToSubAssignmentSplitter::transformTwoSubsequentMinusOperations(syrec::AssignStatement& assignment) {
    /*
     * Try perform transformation of expression <assignedToSignal> -= (<subExpr_1> - <subExpr_2>) to <assignedToSignal += (<subExpr_2> - <subExpr_1>) to enable the optimization of replacing
     * an += operation with a ^= operation if the symbol table entry for the assigned to signal value is zero. The latter will not be checked here and only the assignment will be transformed.
     */
    const std::optional<syrec_operation::operation>               usedAssignmentOperation       = syrec_operation::tryMapAssignmentOperationFlagToEnum(assignment.op);
    const std::optional<std::shared_ptr<syrec::BinaryExpression>> assignmentRhsExprAsBinaryExpr = usedAssignmentOperation.has_value() ? std::make_optional(std::dynamic_pointer_cast<syrec::BinaryExpression>(assignment.rhs)) : std::nullopt;

    if (usedAssignmentOperation.has_value() && *usedAssignmentOperation == syrec_operation::operation::MinusAssign && assignmentRhsExprAsBinaryExpr.has_value() && *assignmentRhsExprAsBinaryExpr) {
        if (const std::optional<syrec_operation::operation> binaryOperationOfRhsExpr = syrec_operation::tryMapBinaryOperationFlagToEnum(assignmentRhsExprAsBinaryExpr->get()->op); binaryOperationOfRhsExpr.has_value() && *binaryOperationOfRhsExpr == syrec_operation::operation::Subtraction) {
            if (const std::optional<unsigned int> mappedToOperationFlagFromEnum = syrec_operation::tryMapAssignmentOperationEnumToFlag(syrec_operation::operation::AddAssign); mappedToOperationFlagFromEnum.has_value()) {
                const syrec::expression::ptr backupOfAssignmentRhsExprLhsOperand = assignmentRhsExprAsBinaryExpr->get()->lhs;
                assignmentRhsExprAsBinaryExpr->get()->lhs                        = assignmentRhsExprAsBinaryExpr->get()->rhs;
                assignmentRhsExprAsBinaryExpr->get()->rhs                        = backupOfAssignmentRhsExprLhsOperand;
                assignment.op                                                    = *mappedToOperationFlagFromEnum;
            }
        }
    }
}

bool ExpressionToSubAssignmentSplitter::transformXorOperationToAddAssignOperationIfTopmostOpOfRhsExprIsNotBitwiseXor(syrec::AssignStatement& assignment, const std::optional<unsigned int>& currentValueOfAssignedToSignal) {
    if (!currentValueOfAssignedToSignal.has_value() || *currentValueOfAssignedToSignal) {
        return false;
    }
    const std::optional<syrec_operation::operation>               usedAssignmentOperation       = syrec_operation::tryMapAssignmentOperationFlagToEnum(assignment.op);
    const std::optional<std::shared_ptr<syrec::BinaryExpression>> assignmentRhsExprAsBinaryExpr = usedAssignmentOperation.has_value() ? std::make_optional(std::dynamic_pointer_cast<syrec::BinaryExpression>(assignment.rhs)) : std::nullopt;
    const std::optional<syrec_operation::operation>               usedBinaryOperation           = assignmentRhsExprAsBinaryExpr.has_value() && *assignmentRhsExprAsBinaryExpr ? syrec_operation::tryMapBinaryOperationFlagToEnum(assignmentRhsExprAsBinaryExpr->get()->op) : std::nullopt;

    if (usedBinaryOperation.has_value() && *usedBinaryOperation != syrec_operation::operation::BitwiseXor && *usedAssignmentOperation == syrec_operation::operation::XorAssign) {
        if (const std::optional<unsigned int> flagForAddAssignOperation = syrec_operation::tryMapAssignmentOperationEnumToFlag(syrec_operation::operation::AddAssign); flagForAddAssignOperation.has_value()) {
            assignment.op = *flagForAddAssignOperation;
            return true;
        }
    }
    return false;
}

void ExpressionToSubAssignmentSplitter::transformAddAssignOperationToXorAssignOperationIfAssignedToSignalValueIsZeroAndTopmostOpOfRhsExprIsBitwiseXor(syrec::AssignStatement& assignment, const std::optional<unsigned>& currentValueOfAssignedToSignal) {
    if (!currentValueOfAssignedToSignal.has_value() || *currentValueOfAssignedToSignal) {
        return;
    }
    const std::optional<syrec_operation::operation>               usedAssignmentOperation       = syrec_operation::tryMapAssignmentOperationFlagToEnum(assignment.op);
    const std::optional<std::shared_ptr<syrec::BinaryExpression>> assignmentRhsExprAsBinaryExpr = usedAssignmentOperation.has_value() ? std::make_optional(std::dynamic_pointer_cast<syrec::BinaryExpression>(assignment.rhs)) : std::nullopt;
    const std::optional<syrec_operation::operation>               usedBinaryOperation           = assignmentRhsExprAsBinaryExpr.has_value() && *assignmentRhsExprAsBinaryExpr ? syrec_operation::tryMapBinaryOperationFlagToEnum(assignmentRhsExprAsBinaryExpr->get()->op) : std::nullopt;

    if (usedBinaryOperation.has_value() && *usedBinaryOperation == syrec_operation::operation::BitwiseXor && *usedAssignmentOperation == syrec_operation::operation::AddAssign) {
        if (const std::optional<unsigned int> flagForAddAssignOperation = syrec_operation::tryMapAssignmentOperationEnumToFlag(syrec_operation::operation::XorAssign); flagForAddAssignOperation.has_value()) {
            assignment.op = *flagForAddAssignOperation;
        }
    }
}


bool ExpressionToSubAssignmentSplitter::doesExprNotContainAnyOperationWithoutAssignmentCounterpartWhenBothOperandsAreNestedExpr(const syrec::expression& expr) {
    if (const auto& exprAsBinaryExpr = dynamic_cast<const syrec::BinaryExpression*>(&expr); exprAsBinaryExpr) {
        const std::optional<syrec_operation::operation> mappedToBinaryOperationEnumValueFromFlag = syrec_operation::tryMapBinaryOperationFlagToEnum(exprAsBinaryExpr->op);
        if (!mappedToBinaryOperationEnumValueFromFlag.has_value()) {
            return false;
        }

        if (syrec_operation::getMatchingAssignmentOperationForOperation(*mappedToBinaryOperationEnumValueFromFlag).has_value()) {
            return doesExprNotContainAnyOperationWithoutAssignmentCounterpartWhenBothOperandsAreNestedExpr(*exprAsBinaryExpr->lhs) && doesExprNotContainAnyOperationWithoutAssignmentCounterpartWhenBothOperandsAreNestedExpr(*exprAsBinaryExpr->rhs);
        }
        return !doesExprDefineSubExpression(*exprAsBinaryExpr->lhs) && !doesExprDefineSubExpression(*exprAsBinaryExpr->rhs);
    }
    if (const auto& exprAsSignalAccessExpr = dynamic_cast<const syrec::VariableExpression*>(&expr); exprAsSignalAccessExpr) {
        return true;
    }
    if (const auto& exprAsNumericExpr = dynamic_cast<const syrec::NumericExpression*>(&expr); exprAsNumericExpr) {
        return true;
    }
    return false;
}