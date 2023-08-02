#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/assignment_with_none_reversible_operations_and_unique_signal_occurrences_simplifier.hpp"
#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/inorder_operation_node_traversal_helper.hpp"

using namespace noAdditionalLineSynthesis;

bool AssignmentWithNonReversibleOperationsAndUniqueSignalOccurrencesSimplifier::simplificationPrecondition(const syrec::AssignStatement::ptr& assignmentStmt) {
    if (const auto& assignmentStmtCasted = std::dynamic_pointer_cast<syrec::AssignStatement>(assignmentStmt); assignmentStmtCasted != nullptr) {
        const auto couldDetermineWhetherEverySignalAccessOfLhsIsDefineOnceOnEveryLevelOfAST = isEveryLhsOperandOfAnyBinaryExprDefinedOnceOnEveryLevelOfTheAST(assignmentStmtCasted->rhs);
        if (!couldDetermineWhetherEverySignalAccessOfLhsIsDefineOnceOnEveryLevelOfAST.has_value() || !*couldDetermineWhetherEverySignalAccessOfLhsIsDefineOnceOnEveryLevelOfAST) {
            return false;
        }
        return isMinusAndXorOperationOnlyDefinedForLeaveNodesInAST(assignmentStmtCasted->rhs)
            && checkNoConsecutiveNonReversibleOperationsDefinedInExpressionAST(assignmentStmtCasted->rhs, std::nullopt);
    }
    return false;
}

syrec::Statement::vec AssignmentWithNonReversibleOperationsAndUniqueSignalOccurrencesSimplifier::simplifyWithoutPreconditionCheck(const syrec::AssignStatement::ptr& assignmentStmt) {
    const auto& assignmentStmtCasted         = std::dynamic_pointer_cast<syrec::AssignStatement>(assignmentStmt);
    const auto& operationNodeTraversalHelper = std::make_shared<InorderOperationNodeTraversalHelper>(assignmentStmtCasted->rhs, symbolTable);

    const auto& simplificationResultOfAssignmentRhs = simplifyExpressionSubtree(operationNodeTraversalHelper);
    if (globalCreatedAssignmentContainer.empty() || simplificationResultOfAssignmentRhs->expressionsRequiringFixup.empty()) {
        return {};
    }

    syrec::expression::ptr generatedAssignmentRhs;
    if (!simplificationResultOfAssignmentRhs->expressionsRequiringFixup.empty()) {
        generatedAssignmentRhs = simplificationResultOfAssignmentRhs->expressionsRequiringFixup.front();
    }
    else {
        generatedAssignmentRhs = std::make_shared<syrec::VariableExpression>(std::dynamic_pointer_cast<syrec::AssignStatement>(simplificationResultOfAssignmentRhs->generatedAssignments.front())->lhs);
    }

    syrec::Statement::vec generatedAssignments = globalCreatedAssignmentContainer;
    generatedAssignments.emplace_back(std::make_shared<syrec::AssignStatement>(assignmentStmtCasted->lhs, assignmentStmtCasted->op, generatedAssignmentRhs));
    invertAssignments(generatedAssignments, true);
    return generatedAssignments;
}

AssignmentWithNonReversibleOperationsAndUniqueSignalOccurrencesSimplifier::SimplificationScopeReference AssignmentWithNonReversibleOperationsAndUniqueSignalOccurrencesSimplifier::simplifyExpressionSubtree(const std::shared_ptr<InorderOperationNodeTraversalHelper>& operationNodeTraversalHelper) {
    const auto& lastOperationNodeInCurrentSubexpression = *operationNodeTraversalHelper->getNodeIdOfLastOperationIdOfCurrentSubexpression();
    const auto& nextOperationNode                       = operationNodeTraversalHelper->getNextOperationNode();
    while (nextOperationNode.has_value() && nextOperationNode.value()->nodeId <= lastOperationNodeInCurrentSubexpression) {
        const bool doesOperationNodeHaveTwoLeaveNodes = operationNodeTraversalHelper->getLeafNodesOfOperationNode(*nextOperationNode).has_value();
        const bool doesOperationNodeHaveOneLeafNode   = !doesOperationNodeHaveTwoLeaveNodes && operationNodeTraversalHelper->getLeafNodeOfOperationNode(*nextOperationNode).has_value();

        if (doesOperationNodeHaveTwoLeaveNodes) {
            return handleOperationNodeWithTwoLeafNodes(*nextOperationNode, operationNodeTraversalHelper);
        }

        if (doesOperationNodeHaveOneLeafNode) {
            return handleOperationNodeWithOneLeafNode(*nextOperationNode, operationNodeTraversalHelper);
        }
        return handleOperationNodeWithTwoNonLeafNodes(*nextOperationNode, operationNodeTraversalHelper);
    }
    return std::make_unique<SimplificationScope>();
}

AssignmentWithNonReversibleOperationsAndUniqueSignalOccurrencesSimplifier::SimplificationScopeReference AssignmentWithNonReversibleOperationsAndUniqueSignalOccurrencesSimplifier::handleOperationNodeWithTwoNonLeafNodes(const InorderOperationNodeTraversalHelper::OperationNodeReference& operationNode, const std::shared_ptr<InorderOperationNodeTraversalHelper>& operationNodeTraversalHelper) {
    const auto  hasOperationAssignmentCounterpart           = syrec_operation::getMatchingAssignmentOperationForOperation(operationNode->operation).has_value();

    auto generatedSimplificationScope = std::make_unique<SimplificationScope>();
    const auto& firstOperationNodeOfLhs                = *operationNodeTraversalHelper->peekNextOperationNode();
    const auto& simplificationResultOfLhsSubexpression = simplifyExpressionSubtree(operationNodeTraversalHelper);
    const auto& firstOperationNodeOfRhs                = *operationNodeTraversalHelper->peekNextOperationNode();
    const auto& simplificationResultOfRhsSubexpression = simplifyExpressionSubtree(operationNodeTraversalHelper);

    fuseGeneratedExpressionsOf(simplificationResultOfLhsSubexpression, firstOperationNodeOfLhs->operation);
    fuseGeneratedExpressionsOf(simplificationResultOfRhsSubexpression, firstOperationNodeOfRhs->operation);

    copyGeneratedAssignmentsAndExpressionsTo(generatedSimplificationScope, simplificationResultOfLhsSubexpression);
    copyGeneratedAssignmentsAndExpressionsTo(generatedSimplificationScope, simplificationResultOfRhsSubexpression);
    
    if (generatedSimplificationScope->generatedAssignments.size() == 2) {
        const auto& generatedAssignmentOfLhs = generatedSimplificationScope->generatedAssignments.front();
        const auto& generatedAssignmentOfRhs = generatedSimplificationScope->generatedAssignments.back();
        generatedSimplificationScope->generatedAssignments.clear();
        
        if (hasOperationAssignmentCounterpart) {
            const auto& generatedAssignmentStmt = std::make_shared<syrec::AssignStatement>(
                std::dynamic_pointer_cast<syrec::AssignStatement>(generatedAssignmentOfLhs)->lhs,
                *syrec_operation::tryMapAssignmentOperationEnumToFlag(*syrec_operation::getMatchingAssignmentOperationForOperation(operationNode->operation)),
                std::make_shared<syrec::VariableExpression>(std::dynamic_pointer_cast<syrec::AssignStatement>(generatedAssignmentOfRhs)->lhs)
            );
            generatedSimplificationScope->generatedAssignments.emplace_back(generatedAssignmentStmt);
            globalCreatedAssignmentContainer.emplace_back(generatedAssignmentStmt);
        } else {
            const auto& generatedExpression = createExpressionFor(
            std::make_shared<syrec::VariableExpression>(std::dynamic_pointer_cast<syrec::AssignStatement>(generatedAssignmentOfLhs)->lhs),
            operationNode->operation,
            std::make_shared<syrec::VariableExpression>(std::dynamic_pointer_cast<syrec::AssignStatement>(generatedAssignmentOfRhs)->lhs)
            );
            generatedSimplificationScope->expressionsRequiringFixup.emplace_back(generatedExpression);
        }
    } else if (generatedSimplificationScope->expressionsRequiringFixup.size() == 2) {
        fuseGeneratedExpressionsOf(generatedSimplificationScope, operationNode->operation);
    } else {
        if (hasOperationAssignmentCounterpart) {
            std::optional<syrec::VariableAccess::ptr> potentialAssignmentCandidateLhs;
            std::optional<syrec::expression::ptr>     potentialAssignmentCandidateRhs;
            if (!simplificationResultOfLhsSubexpression->generatedAssignments.empty()) {
                potentialAssignmentCandidateLhs = std::dynamic_pointer_cast<syrec::AssignStatement>(simplificationResultOfLhsSubexpression->generatedAssignments.front())->lhs;
                potentialAssignmentCandidateRhs = simplificationResultOfRhsSubexpression->expressionsRequiringFixup.front();
            } else if (!simplificationResultOfRhsSubexpression->generatedAssignments.empty() && syrec_operation::isCommutative(operationNode->operation)) {
                potentialAssignmentCandidateLhs = std::dynamic_pointer_cast<syrec::AssignStatement>(simplificationResultOfRhsSubexpression->generatedAssignments.front())->lhs;
                potentialAssignmentCandidateRhs = simplificationResultOfLhsSubexpression->expressionsRequiringFixup.front();
            }

            if (potentialAssignmentCandidateLhs.has_value()) {
                const auto& generatedAssignmentStmt = std::make_shared<syrec::AssignStatement>(
                        *potentialAssignmentCandidateLhs,
                        *syrec_operation::tryMapAssignmentOperationEnumToFlag(*syrec_operation::getMatchingAssignmentOperationForOperation(operationNode->operation)),
                        *potentialAssignmentCandidateRhs);
                generatedSimplificationScope->generatedAssignments.emplace_back(generatedAssignmentStmt);
                generatedSimplificationScope->expressionsRequiringFixup.clear();
                globalCreatedAssignmentContainer.emplace_back(generatedAssignmentStmt);
            }
            else {
                syrec::expression::ptr generatedExprLhsOperand;
                if (!simplificationResultOfLhsSubexpression->expressionsRequiringFixup.empty()) {
                    generatedExprLhsOperand = simplificationResultOfLhsSubexpression->expressionsRequiringFixup.front();
                } else {
                    generatedExprLhsOperand = std::make_shared<syrec::VariableExpression>(std::dynamic_pointer_cast<syrec::AssignStatement>(simplificationResultOfLhsSubexpression->generatedAssignments.front())->lhs);
                }

                syrec::expression::ptr generatedExprRhsOperand;
                if (!simplificationResultOfRhsSubexpression->expressionsRequiringFixup.empty()) {
                    generatedExprRhsOperand = simplificationResultOfRhsSubexpression->expressionsRequiringFixup.front();
                } else {
                    generatedExprRhsOperand = std::make_shared<syrec::VariableExpression>(std::dynamic_pointer_cast<syrec::AssignStatement>(simplificationResultOfRhsSubexpression->generatedAssignments.front())->lhs);
                }
                generatedSimplificationScope->expressionsRequiringFixup.clear();
                generatedSimplificationScope->expressionsRequiringFixup.emplace_back(createExpressionFor(generatedExprLhsOperand, operationNode->operation, generatedExprRhsOperand));
            }
        } else {
            syrec::expression::ptr generatedExprLhsOperand;
            if (!simplificationResultOfLhsSubexpression->expressionsRequiringFixup.empty()) {
                generatedExprLhsOperand = simplificationResultOfLhsSubexpression->expressionsRequiringFixup.front();
            }
            else {
                generatedExprLhsOperand = std::make_shared<syrec::VariableExpression>(std::dynamic_pointer_cast<syrec::AssignStatement>(simplificationResultOfLhsSubexpression->generatedAssignments.front())->lhs);
            }

            syrec::expression::ptr generatedExprRhsOperand;
            if (!simplificationResultOfRhsSubexpression->expressionsRequiringFixup.empty()) {
                generatedExprRhsOperand = simplificationResultOfRhsSubexpression->expressionsRequiringFixup.front();
            } else {
                generatedExprRhsOperand = std::make_shared<syrec::VariableExpression>(std::dynamic_pointer_cast<syrec::AssignStatement>(simplificationResultOfRhsSubexpression->generatedAssignments.front())->lhs);
            }
            generatedSimplificationScope->expressionsRequiringFixup.clear();
            generatedSimplificationScope->expressionsRequiringFixup.emplace_back(createExpressionFor(generatedExprLhsOperand, operationNode->operation, generatedExprRhsOperand));
        }
    }

    return generatedSimplificationScope;
}

AssignmentWithNonReversibleOperationsAndUniqueSignalOccurrencesSimplifier::SimplificationScopeReference AssignmentWithNonReversibleOperationsAndUniqueSignalOccurrencesSimplifier::handleOperationNodeWithTwoLeafNodes(const InorderOperationNodeTraversalHelper::OperationNodeReference& operationNode, const std::shared_ptr<InorderOperationNodeTraversalHelper>& operationNodeTraversalHelper) {
    auto generatedSimplificationScope = std::make_unique<SimplificationScope>();
    if (const auto& leafNodes = operationNodeTraversalHelper->getLeafNodesOfOperationNode(operationNode); leafNodes.has_value()) {
        if (const auto& equivalentAssignmentOperationForBinaryOp = syrec_operation::getMatchingAssignmentOperationForOperation(operationNode->operation); equivalentAssignmentOperationForBinaryOp.has_value()) {
            const auto& lhsLeafAsSignalAccess = leafNodes->first->getAsSignalAccess().value()->var;
            const auto& assignmentRhs         = leafNodes->second->getData();

            const auto& generatedAssignmentStmt = std::make_shared<syrec::AssignStatement>(
                    lhsLeafAsSignalAccess,
                    *syrec_operation::tryMapAssignmentOperationEnumToFlag(*equivalentAssignmentOperationForBinaryOp),
                    assignmentRhs);
            generatedSimplificationScope->generatedAssignments.emplace_back(generatedAssignmentStmt);
            globalCreatedAssignmentContainer.emplace_back(generatedAssignmentStmt);
        } else {
            generatedSimplificationScope->expressionsRequiringFixup.emplace_back(createExpressionFor(leafNodes->first->getData(), operationNode->operation, leafNodes->second->getData()));
        }
    }
    return generatedSimplificationScope;
}

AssignmentWithNonReversibleOperationsAndUniqueSignalOccurrencesSimplifier::SimplificationScopeReference AssignmentWithNonReversibleOperationsAndUniqueSignalOccurrencesSimplifier::handleOperationNodeWithOneLeafNode(const InorderOperationNodeTraversalHelper::OperationNodeReference& operationNode, const std::shared_ptr<InorderOperationNodeTraversalHelper>& operationNodeTraversalHelper) {
    const auto& leafNode = operationNodeTraversalHelper->getLeafNodeOfOperationNode(operationNode);
    const auto  isOperationReversible = syrec_operation::getMatchingAssignmentOperationForOperation(operationNode->operation).has_value();
    const auto& simplificationResultOfNonLeafNode = simplifyExpressionSubtree(operationNodeTraversalHelper);
    
    auto generatedScope = std::make_unique<SimplificationScope>();

    syrec::expression::ptr generatedStatementRhs;
    if (!simplificationResultOfNonLeafNode->expressionsRequiringFixup.empty()) {
        generatedStatementRhs = simplificationResultOfNonLeafNode->expressionsRequiringFixup.front();
    } else {
        generatedStatementRhs = std::make_shared<syrec::VariableExpression>(std::dynamic_pointer_cast<syrec::AssignStatement>(simplificationResultOfNonLeafNode->generatedAssignments.back())->lhs);
    }

    /*
     * Currently we have no way to convert expressions of the form (expr - signalAccess) to (-signalAccess += expr) since an assignment with the lhs negated is not allowed
     * by the SyReC grammar and we currently have not determine how we correctly negate an unsigned integer value. Thus, we create an binary expression instead of an assignment for now.
     * TODO: If assignment lhs can be defined as negated and unsigned integer values can be negated, an assignment instead of an binary expression should be created
     */
    const auto shouldExpressionInsteadOfAssignmentBeCreated = operationNode->operation == syrec_operation::operation::Subtraction && !operationNode->rhs.isOperationNode;
    if (isOperationReversible && shouldExpressionInsteadOfAssignmentBeCreated) {
        const auto& generatedAssignmentStatement = std::make_shared<syrec::AssignStatement>(
            leafNode.value()->getAsSignalAccess().value()->var,
            *syrec_operation::tryMapAssignmentOperationEnumToFlag(*syrec_operation::getMatchingAssignmentOperationForOperation(operationNode->operation)),
            generatedStatementRhs);
        generatedScope->generatedAssignments.emplace_back(generatedAssignmentStatement);
        globalCreatedAssignmentContainer.emplace_back(generatedAssignmentStatement);
    } else {
        generatedScope->expressionsRequiringFixup.emplace_back(createExpressionFor(leafNode.value()->getData(), operationNode->operation, generatedStatementRhs));
    }
    return generatedScope;
}

void AssignmentWithNonReversibleOperationsAndUniqueSignalOccurrencesSimplifier::copyGeneratedAssignmentsAndExpressionsTo(const SimplificationScopeReference& copiedToScope, const SimplificationScopeReference& copiedFromScope) {
    if (!copiedFromScope->expressionsRequiringFixup.empty()) {
        copiedToScope->expressionsRequiringFixup.insert(copiedToScope->expressionsRequiringFixup.end(), copiedFromScope->expressionsRequiringFixup.begin(), copiedFromScope->expressionsRequiringFixup.end());   
    }
    if (!copiedFromScope->generatedAssignments.empty()) {
        copiedToScope->generatedAssignments.insert(copiedToScope->generatedAssignments.end(), copiedFromScope->generatedAssignments.begin(), copiedFromScope->generatedAssignments.end());   
    }
}

void AssignmentWithNonReversibleOperationsAndUniqueSignalOccurrencesSimplifier::fuseGeneratedExpressionsOf(const SimplificationScopeReference& scope, syrec_operation::operation operation) {
    if (scope->expressionsRequiringFixup.size() != 2) {
        return;
    }

    const auto& exprLhsOperand = scope->expressionsRequiringFixup.front();
    const auto& exprRhsOperand = scope->expressionsRequiringFixup.back();
    scope->expressionsRequiringFixup.clear();
    scope->expressionsRequiringFixup.emplace_back(createExpressionFor(exprLhsOperand, operation, exprRhsOperand));
}

syrec::expression::ptr AssignmentWithNonReversibleOperationsAndUniqueSignalOccurrencesSimplifier::createExpressionFor(const syrec::expression::ptr& lhsOperand, syrec_operation::operation operation, const syrec::expression::ptr& rhsOperand) {
    if (operation == syrec_operation::operation::ShiftLeft || operation == syrec_operation::operation::ShiftRight) {
        return std::make_shared<syrec::ShiftExpression>(
            lhsOperand,
            *syrec_operation::tryMapShiftOperationEnumToFlag(operation),
            std::dynamic_pointer_cast<syrec::NumericExpression>(rhsOperand)->value
        );
    }
    return std::make_shared<syrec::BinaryExpression>(
        lhsOperand,
        *syrec_operation::tryMapBinaryOperationEnumToFlag(operation),
        rhsOperand
    );
}

bool AssignmentWithNonReversibleOperationsAndUniqueSignalOccurrencesSimplifier::isMinusAndXorOperationOnlyDefinedForLeaveNodesInAST(const syrec::expression::ptr& exprToCheck) {
    if (const auto& exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(exprToCheck); exprAsBinaryExpr != nullptr) {
        return exprAsBinaryExpr->op == syrec::BinaryExpression::Exor || exprAsBinaryExpr->op == syrec::BinaryExpression::Subtract
            ? ((doesExprDefineNestedExpr(exprAsBinaryExpr->lhs) ? isMinusAndXorOperationOnlyDefinedForLeaveNodesInAST(exprAsBinaryExpr->lhs) : true)
                && (doesExprDefineNestedExpr(exprAsBinaryExpr->rhs) ? isMinusAndXorOperationOnlyDefinedForLeaveNodesInAST(exprAsBinaryExpr->rhs) : true))
            : true;
    }
    if (const auto& exprAsShiftExpr = std::dynamic_pointer_cast<syrec::ShiftExpression>(exprToCheck); exprAsShiftExpr != nullptr) {
        return doesExprDefineNestedExpr(exprAsShiftExpr->lhs) ? isMinusAndXorOperationOnlyDefinedForLeaveNodesInAST(exprAsShiftExpr->lhs) : true;
    }
    return true;
}

bool AssignmentWithNonReversibleOperationsAndUniqueSignalOccurrencesSimplifier::checkNoConsecutiveNonReversibleOperationsDefinedInExpressionAST(const syrec::expression::ptr& exprToCheck, const std::optional<syrec_operation::operation>& parentNodeOperation) {
    std::optional<syrec_operation::operation> currentExprOperation;
    if (const auto& exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(exprToCheck); exprAsBinaryExpr != nullptr) {
        currentExprOperation = syrec_operation::tryMapBinaryOperationFlagToEnum(exprAsBinaryExpr->op);
        if (parentNodeOperation.has_value() && currentExprOperation.has_value()) {
            if (!isOperationAdditionSubtractionOrXor(*currentExprOperation) && !isOperationAdditionSubtractionOrXor(*parentNodeOperation)) {
                return false;
            }
        }
        return checkNoConsecutiveNonReversibleOperationsDefinedInExpressionAST(exprAsBinaryExpr->lhs, currentExprOperation)
            && checkNoConsecutiveNonReversibleOperationsDefinedInExpressionAST(exprAsBinaryExpr->rhs, currentExprOperation);
    }
    if (const auto& exprAsShiftExpr = std::dynamic_pointer_cast<syrec::ShiftExpression>(exprToCheck); exprAsShiftExpr != nullptr) {
        currentExprOperation = syrec_operation::tryMapShiftOperationFlagToEnum(exprAsShiftExpr->op);
        if (parentNodeOperation.has_value() && currentExprOperation.has_value()) {
            if (!isOperationAdditionSubtractionOrXor(*currentExprOperation) && !isOperationAdditionSubtractionOrXor(*parentNodeOperation)) {
                return false;
            }
        }
        return checkNoConsecutiveNonReversibleOperationsDefinedInExpressionAST(exprAsShiftExpr->lhs, currentExprOperation);
    }
    return true;
}

bool AssignmentWithNonReversibleOperationsAndUniqueSignalOccurrencesSimplifier::isOperationAdditionSubtractionOrXor(const syrec_operation::operation operation) {
    switch (operation) {
        case syrec_operation::operation::Addition:
        case syrec_operation::operation::Subtraction:
        case syrec_operation::operation::BitwiseXor:
            return true;
        default:
            return false;
    }
}