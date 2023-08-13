#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/assignment_with_none_reversible_operations_and_unique_signal_occurrences_simplifier.hpp"
#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/inorder_operation_node_traversal_helper.hpp"

using namespace noAdditionalLineSynthesis;

/*
 * TODO: a ^= ((c - 2) ^ (b / 2)) not correctly synthesized
 * TODO: a ^= ((d - 2) + (b ^ 2)) not correctly synthesized
 * TODO: a -= ((d - 2) + (b - 2)) not correctly synthesized, inverted statements missing
 * TODO: a ^= ((d - 2) + (b ^ (c + d)))
 *
 * TODO: a += (((2 - b) - (2 - c)) ^ d) should use a different synthesis approach than this optimizer by always assigning to the lhs of the original assignment instead of the leaf node
 * TODO: One could implement the optimization that an assignment of the form a ^= (<subExpr1> - <subExpr2>) could be converted to a += <subExpr1>; a -= <subExpr2> is a was 0 prior to the assignment
 * 
 */

bool AssignmentWithNonReversibleOperationsAndUniqueSignalOccurrencesSimplifier::simplificationPrecondition(const syrec::AssignStatement::ptr& assignmentStmt) {
    if (const auto& assignmentStmtCasted = std::dynamic_pointer_cast<syrec::AssignStatement>(assignmentStmt); assignmentStmtCasted != nullptr) {
        const auto couldDetermineWhetherEverySignalAccessOfLhsIsDefineOnceOnEveryLevelOfAST = isEveryLhsOperandOfAnyBinaryExprDefinedOnceOnEveryLevelOfTheAST(assignmentStmtCasted->rhs);
        
        /*
         * TODO: Temporarily relaxed precondition of originally proposed algorithm since this implementation only include some of its concepts but is otherwise a new implementation
         * and thus the following precondition no longer apply (still need to be checked more thoroughly):
         * I. The "^" and "-" operation only operate on leaf nodes only
         * II. There exist no two consecutive operation nodes with operations without an assignment operation counterpart
         */
        return couldDetermineWhetherEverySignalAccessOfLhsIsDefineOnceOnEveryLevelOfAST.value_or(false);
    }
    return false;
}

syrec::Statement::vec AssignmentWithNonReversibleOperationsAndUniqueSignalOccurrencesSimplifier::simplifyWithoutPreconditionCheck(const syrec::AssignStatement::ptr& assignmentStmt, bool isValueOfAssignedToSignalBlockedByDataFlowAnalysis) {
    const auto& assignmentStmtCasted         = std::dynamic_pointer_cast<syrec::AssignStatement>(assignmentStmt);
    const auto& operationNodeTraversalHelper = std::make_shared<InorderOperationNodeTraversalHelper>(assignmentStmtCasted->rhs, symbolTable);

    const auto& simplificationResultOfAssignmentRhs = simplifyExpressionSubtree(operationNodeTraversalHelper);
    if (globalCreatedAssignmentContainer.empty() && simplificationResultOfAssignmentRhs->expressionsRequiringFixup.empty()) {
        return {};
    }

    syrec::Statement::vec generatedAssignments = globalCreatedAssignmentContainer;
    const auto&           finalAssignmentStatements = createFinalAssignmentFromOptimizedRhsOfInitialAssignment(assignmentStmtCasted->lhs, assignmentStmtCasted->op, simplificationResultOfAssignmentRhs, isValueOfAssignedToSignalBlockedByDataFlowAnalysis);
    generatedAssignments.insert(generatedAssignments.cend(), finalAssignmentStatements.begin(), finalAssignmentStatements.end());
    return invertAssignmentsButIgnoreSome(generatedAssignments, finalAssignmentStatements.size());
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
                generatedSimplificationScope->generatedAssignments.clear();
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
            /*
             * We should only create an assignment if one of the following conditions holds:
             * I. The lhs operand of the expression is a signal access and the operation has a matching assignment operation counterpart, the latter is already checked at this point
             * II. The rhs operand is a signal access while the lhs is not, additionally the given assignment operation is commutative.
             * In all other cases an expression should be generated instead
             */
            if (leafNodes->first->getAsSignalAccess().has_value()) {
                const auto& lhsLeafAsSignalAccess = leafNodes->first->getAsSignalAccess().value()->var;
                const auto& assignmentRhs         = leafNodes->second->getData();

                const auto& generatedAssignmentStmt = std::make_shared<syrec::AssignStatement>(
                        lhsLeafAsSignalAccess,
                        *syrec_operation::tryMapAssignmentOperationEnumToFlag(*equivalentAssignmentOperationForBinaryOp),
                        assignmentRhs);
                generatedSimplificationScope->generatedAssignments.emplace_back(generatedAssignmentStmt);
                globalCreatedAssignmentContainer.emplace_back(generatedAssignmentStmt);   
            } else {
                if (leafNodes->second->getAsSignalAccess().has_value() && equivalentAssignmentOperationForBinaryOp.has_value() && syrec_operation::isCommutative(operationNode->operation)) {
                    const auto assignmentRhs = leafNodes->first->getData();
                    const auto& generatedAssignmentStmt = std::make_shared<syrec::AssignStatement>(
                            leafNodes->second->getAsSignalAccess().value()->var,
                            *syrec_operation::tryMapAssignmentOperationEnumToFlag(*equivalentAssignmentOperationForBinaryOp),
                            assignmentRhs);
                    generatedSimplificationScope->generatedAssignments.emplace_back(generatedAssignmentStmt);
                    globalCreatedAssignmentContainer.emplace_back(generatedAssignmentStmt);   
                } else {
                    generatedSimplificationScope->expressionsRequiringFixup.emplace_back(createExpressionFor(leafNodes->first->getData(), operationNode->operation, leafNodes->second->getData()));                    
                }
            }
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
    bool                   didRhsGeneratedAssignmentStmt = false;
    if (!simplificationResultOfNonLeafNode->expressionsRequiringFixup.empty()) {
        generatedStatementRhs = simplificationResultOfNonLeafNode->expressionsRequiringFixup.front();
    } else {
        generatedStatementRhs = std::make_shared<syrec::VariableExpression>(std::dynamic_pointer_cast<syrec::AssignStatement>(simplificationResultOfNonLeafNode->generatedAssignments.back())->lhs);
        didRhsGeneratedAssignmentStmt = true;
    }

    const bool isLeafNodeLeftOperandOfOperation = !operationNode->lhs.isOperationNode;
    /*
     * Currently we have no way to convert expressions of the form (expr - signalAccess) to (-signalAccess += expr) since an assignment with the lhs negated is not allowed
     * by the SyReC grammar and we currently have not determine how we correctly negate an unsigned integer value. Thus, we create an binary expression instead of an assignment for now.
     *
     * Additionally, if the leaf node did not define a signal access we need to create an expression instead of an assignment if:
     * I. The leaf node did not define a signal access and the non-leaf node did not define an assignment
     * II. The leaf node did not define a signal access while the non-leaf node did define an assignment but the operation of the operation node is not commutative
     *
     * TODO: If assignment lhs can be defined as negated and unsigned integer values can be negated, an assignment instead of an binary expression should be created
     */
    const auto shouldExpressionInsteadOfAssignmentBeCreated =
        (operationNode->operation == syrec_operation::operation::Subtraction && !operationNode->rhs.isOperationNode)
        || (!leafNode.value()->getAsSignalAccess().has_value() && !didRhsGeneratedAssignmentStmt)
        || (!leafNode.value()->getAsSignalAccess() && didRhsGeneratedAssignmentStmt && (isOperationReversible ? !syrec_operation::isCommutative(operationNode->operation) : true));

    if (isOperationReversible && !shouldExpressionInsteadOfAssignmentBeCreated) {
        syrec::VariableAccess::ptr assignedToSignal;
        syrec::expression::ptr     assignmentRhs;

        if (!leafNode.value()->getAsSignalAccess().has_value()) {
            assignedToSignal = std::dynamic_pointer_cast<syrec::AssignStatement>(simplificationResultOfNonLeafNode->generatedAssignments.back())->lhs;
            assignmentRhs    = leafNode.value()->getData();
        } else {
            assignedToSignal = leafNode.value()->getAsSignalAccess().value()->var;
            assignmentRhs    = generatedStatementRhs;
        }

        /*
         * Try to generate sub-assignments by splitting the generated expr of the none leaf node at operations that have an assignment operation counter part, starting from the topmost operation of the expr.
         * Otherwise, simply generated an assignment with the leaf node being the assigned to signal.
         */
        const auto& assignmentOperationToBeUsed = *syrec_operation::getMatchingAssignmentOperationForOperation(operationNode->operation);
        /*
         * TODO: If we were to actually perform updates of the signal value (if the value was zero prior to the assignment) and invalidating the value when generating an assignment
         * we could perform more simplification that are currently lost since we are assuming the value to be not zero
         */
        bool        isAssignedToSignalZeroPriorToAssignment = false;
        if (const auto& generatedSubAssignmentsForExpr = splitAssignmentRhs(assignmentOperationToBeUsed, assignmentRhs, isAssignedToSignalZeroPriorToAssignment); !generatedSubAssignmentsForExpr.empty()) {
            for (const auto& subAssignment : generatedSubAssignmentsForExpr) {
                const auto& generatedAssignmentStatement = std::make_shared<syrec::AssignStatement>(
                    assignedToSignal,
                    *syrec_operation::tryMapAssignmentOperationEnumToFlag(subAssignment->mappedToAssignmentOperation),
                    subAssignment->assignmentRhsOperand
                );
                globalCreatedAssignmentContainer.emplace_back(generatedAssignmentStatement);
            }
            generatedScope->generatedAssignments.emplace_back(globalCreatedAssignmentContainer.back());
        }
        else {
            const auto& generatedAssignmentStatement = std::make_shared<syrec::AssignStatement>(
                    assignedToSignal,
                    *syrec_operation::tryMapAssignmentOperationEnumToFlag(assignmentOperationToBeUsed),
                    assignmentRhs);

            generatedScope->generatedAssignments.emplace_back(generatedAssignmentStatement);
            globalCreatedAssignmentContainer.emplace_back(generatedAssignmentStatement);   
        }
    } else {
        // We should keep the order of the operands in our generated expression, thus the distinction whether the leaf node was the left or right operand is needed
        if (isLeafNodeLeftOperandOfOperation) {
            generatedScope->expressionsRequiringFixup.emplace_back(createExpressionFor(leafNode.value()->getData(), operationNode->operation, generatedStatementRhs));    
        }
        else {
            generatedScope->expressionsRequiringFixup.emplace_back(createExpressionFor(generatedStatementRhs, operationNode->operation, leafNode.value()->getData()));
        }
    }
    return generatedScope;
}

syrec::Statement::vec AssignmentWithNonReversibleOperationsAndUniqueSignalOccurrencesSimplifier::createFinalAssignmentFromOptimizedRhsOfInitialAssignment(const syrec::VariableAccess::ptr& initialAssignmentLhs, unsigned int initialAssignmentOperation, const SimplificationScopeReference& optimizedRhsOfInitialAssignment, bool isValueOfAssignedToSignalBlockedByDataFlowAnalysis) const {
    if (!syrec_operation::tryMapAssignmentOperationFlagToEnum(initialAssignmentOperation).has_value()) {
        const auto& finalAssignmentStmtRhsExpr = optimizedRhsOfInitialAssignment->expressionsRequiringFixup.empty()
            ? std::make_shared<syrec::VariableExpression>(std::dynamic_pointer_cast<syrec::AssignStatement>(optimizedRhsOfInitialAssignment->generatedAssignments.front())->lhs)
            : optimizedRhsOfInitialAssignment->expressionsRequiringFixup.front();

        const auto& finalAssignmentStatement = std::make_shared<syrec::AssignStatement>(
                initialAssignmentLhs,
                initialAssignmentOperation,
                finalAssignmentStmtRhsExpr);
        return {finalAssignmentStatement};
    }

    auto                   finalAssignmentOperation = *syrec_operation::tryMapAssignmentOperationFlagToEnum(initialAssignmentOperation);
    syrec::expression::ptr finalAssignmentRhsExpr;

    
   bool isValueOfAssignedToSignalZeroPriorToAssignment = false;
    if (const auto& preAssignmentValueOfAssignedToSignal = symbolTable->tryFetchValueForLiteral(initialAssignmentLhs);
        preAssignmentValueOfAssignedToSignal.has_value() && *preAssignmentValueOfAssignedToSignal == 0) {
        isValueOfAssignedToSignalZeroPriorToAssignment = true;
    }

    if (!optimizedRhsOfInitialAssignment->expressionsRequiringFixup.empty()) {
        const auto& finalAssignmentRhs = optimizedRhsOfInitialAssignment->expressionsRequiringFixup.front();
        if (const auto& finalAssignmentRhsAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(finalAssignmentRhs); finalAssignmentRhsAsBinaryExpr != nullptr) {
            if (const auto& definedBinaryOperation = syrec_operation::tryMapBinaryOperationFlagToEnum(finalAssignmentRhsAsBinaryExpr->op); definedBinaryOperation.has_value()) {
                std::optional<std::pair<syrec_operation::operation, syrec_operation::operation>> assignmentOperationsForRhsSplit;

                switch (finalAssignmentOperation) {
                    case syrec_operation::operation::AddAssign:
                        if (isValueOfAssignedToSignalZeroPriorToAssignment && !isValueOfAssignedToSignalBlockedByDataFlowAnalysis) {
                            if (*definedBinaryOperation == syrec_operation::operation::Addition || *definedBinaryOperation == syrec_operation::operation::BitwiseXor) {
                                assignmentOperationsForRhsSplit = std::make_optional(std::make_pair(finalAssignmentOperation, syrec_operation::operation::XorAssign));
                            } else if (const auto& mappedToAssignmentOperationOfBinaryExprOperation = syrec_operation::getMatchingAssignmentOperationForOperation(*definedBinaryOperation); mappedToAssignmentOperationOfBinaryExprOperation.has_value()) {
                                assignmentOperationsForRhsSplit = std::make_optional(std::make_pair(finalAssignmentOperation, *mappedToAssignmentOperationOfBinaryExprOperation));
                            }

                            finalAssignmentOperation = syrec_operation::operation::XorAssign;

                            /*
                            * Assignment of the form a += (b ^ c) can be split into a ^= b; a ^= c iff a = 0 prior to the initial assignment
                            */
                            if (*definedBinaryOperation == syrec_operation::operation::BitwiseXor) {
                                assignmentOperationsForRhsSplit = std::make_optional(std::make_pair(finalAssignmentOperation, syrec_operation::operation::XorAssign));
                            } else {
                                if (const auto& mappedToAssignmentOperationOfBinaryExprOperation = syrec_operation::getMatchingAssignmentOperationForOperation(*definedBinaryOperation); mappedToAssignmentOperationOfBinaryExprOperation.has_value()) {
                                    assignmentOperationsForRhsSplit = std::make_optional(std::make_pair(finalAssignmentOperation, *mappedToAssignmentOperationOfBinaryExprOperation));
                                }
                            }
                        }
                        if (*definedBinaryOperation == syrec_operation::operation::Subtraction) {
                            assignmentOperationsForRhsSplit = std::make_optional(std::make_pair(finalAssignmentOperation, syrec_operation::operation::MinusAssign));
                        }
                        break;
                    case syrec_operation::operation::MinusAssign:
                        if (*definedBinaryOperation == syrec_operation::operation::Addition) {
                            assignmentOperationsForRhsSplit = std::make_optional(std::make_pair(syrec_operation::operation::MinusAssign, syrec_operation::operation::MinusAssign));
                        }
                        if (*definedBinaryOperation == syrec_operation::operation::Subtraction) {
                            assignmentOperationsForRhsSplit = std::make_optional(std::make_pair(syrec_operation::operation::MinusAssign, syrec_operation::operation::AddAssign));
                        }
                        break;
                    case syrec_operation::operation::XorAssign:
                        /*
                        * Assignment of the form a ^= (b ^ c) can be split into a ^= b; a ^= c iff a = 0 prior to the initial assignment
                        */
                        if (isValueOfAssignedToSignalZeroPriorToAssignment && !isValueOfAssignedToSignalBlockedByDataFlowAnalysis) {
                            switch (*definedBinaryOperation) {
                                case syrec_operation::operation::BitwiseXor:
                                    assignmentOperationsForRhsSplit = std::make_optional(std::make_pair(syrec_operation::operation::XorAssign, syrec_operation::operation::XorAssign));
                                    break;
                                case syrec_operation::operation::Addition:
                                    assignmentOperationsForRhsSplit = std::make_optional(std::make_pair(syrec_operation::operation::XorAssign, syrec_operation::operation::AddAssign));
                                    break;
                                case syrec_operation::operation::Subtraction:
                                    assignmentOperationsForRhsSplit = std::make_optional(std::make_pair(syrec_operation::operation::AddAssign, syrec_operation::operation::MinusAssign));
                                    break;
                                default:
                                    break;
                            }
                        }
                        break;
                    default:
                        break;
                }
               
                if (doesExprDefineNestedExpr(finalAssignmentRhsAsBinaryExpr->lhs) || doesExprDefineNestedExpr(finalAssignmentRhsAsBinaryExpr->rhs)) {
                    finalAssignmentOperation = assignmentOperationsForRhsSplit.has_value() ? assignmentOperationsForRhsSplit->first : finalAssignmentOperation;
                    isValueOfAssignedToSignalZeroPriorToAssignment &= !isValueOfAssignedToSignalBlockedByDataFlowAnalysis;
                    return createAssignmentsForSplitAssignment(initialAssignmentLhs, splitAssignmentRhs(finalAssignmentOperation, finalAssignmentRhs, isValueOfAssignedToSignalZeroPriorToAssignment));
                }
                
                //if (assignmentOperationsForRhsSplit.has_value()) {
                //    if (doesExprDefineNestedExpr(finalAssignmentRhsAsBinaryExpr->lhs) || doesExprDefineNestedExpr(finalAssignmentRhsAsBinaryExpr->rhs)) {
                //        return createAssignmentsForSplitAssignment(initialAssignmentLhs, splitAssignmentRhs(assignmentOperationsForRhsSplit->first, finalAssignmentRhsExpr, isValueOfAssignedToSignalZeroPriorToAssignment));

                //        //const auto& assignmentForLhsOfRhsExpr = std::make_shared<syrec::AssignStatement>(
                //        //        initialAssignmentLhs,
                //        //        *syrec_operation::tryMapAssignmentOperationEnumToFlag(assignmentOperationsForRhsSplit->first),
                //        //        finalAssignmentRhsAsBinaryExpr->lhs);

                //        //const auto& assignmentForRhsOfRhsExpr = std::make_shared<syrec::AssignStatement>(
                //        //        initialAssignmentLhs,
                //        //        *syrec_operation::tryMapAssignmentOperationEnumToFlag(assignmentOperationsForRhsSplit->second),
                //        //        finalAssignmentRhsAsBinaryExpr->rhs);

                //        //return {assignmentForLhsOfRhsExpr, assignmentForRhsOfRhsExpr};
                //    } else {
                //        finalAssignmentOperation = assignmentOperationsForRhsSplit->first;
                //    }
                //}
            }
        }
        finalAssignmentRhsExpr = optimizedRhsOfInitialAssignment->expressionsRequiringFixup.front();
    } else {
        const auto& finalAssignmentRhs = std::make_shared<syrec::VariableExpression>(std::dynamic_pointer_cast<syrec::AssignStatement>(optimizedRhsOfInitialAssignment->generatedAssignments.front())->lhs);
        // Transform assignment of the form a += X to a ^= X if a = 0 prior to the assignment
        if (isValueOfAssignedToSignalZeroPriorToAssignment && finalAssignmentOperation == syrec_operation::operation::AddAssign
            && !isValueOfAssignedToSignalBlockedByDataFlowAnalysis) {
            finalAssignmentOperation = syrec_operation::operation::XorAssign;
        }
        finalAssignmentRhsExpr = std::make_shared<syrec::VariableExpression>(finalAssignmentRhs->var);
    }

    const auto& finalAssignmentStatement = std::make_shared<syrec::AssignStatement>(
            initialAssignmentLhs,
            *syrec_operation::tryMapAssignmentOperationEnumToFlag(finalAssignmentOperation),
            finalAssignmentRhsExpr);
    return {finalAssignmentStatement};
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

std::vector<AssignmentWithNonReversibleOperationsAndUniqueSignalOccurrencesSimplifier::SplitAssignmentExprPartReference> AssignmentWithNonReversibleOperationsAndUniqueSignalOccurrencesSimplifier::splitAssignmentRhs(syrec_operation::operation originalAssignmentOperation, const syrec::BinaryExpression::ptr& assignmentRhsBinaryExpr, bool& isAssignedToSignalZeroPriorToAssignment) {
    const auto& binaryExprCasted = std::dynamic_pointer_cast<syrec::BinaryExpression>(assignmentRhsBinaryExpr);
    if (binaryExprCasted == nullptr 
        || !syrec_operation::getMatchingAssignmentOperationForOperation(*syrec_operation::tryMapBinaryOperationFlagToEnum(binaryExprCasted->op)).has_value() 
        || (!doesExprDefineNestedExpr(binaryExprCasted->lhs) && !doesExprDefineNestedExpr(binaryExprCasted->rhs))) {
        if (originalAssignmentOperation == syrec_operation::operation::AddAssign && isAssignedToSignalZeroPriorToAssignment) {
            isAssignedToSignalZeroPriorToAssignment = false;
            return {std::make_shared<SplitAssignmentExprPart>(syrec_operation::operation::XorAssign, assignmentRhsBinaryExpr)};
        }
        isAssignedToSignalZeroPriorToAssignment = false;
        return {std::make_shared<SplitAssignmentExprPart>(originalAssignmentOperation, assignmentRhsBinaryExpr)};
    }

 /*   if ((!doesExprDefineNestedExpr(binaryExprCasted->lhs) && !doesExprDefineNestedExpr(binaryExprCasted->rhs))) {
        if (originalAssignmentOperation == syrec_operation::operation::AddAssign && isAssignedToSignalZeroPriorToAssignment) {
            isAssignedToSignalZeroPriorToAssignment = false;
            return {std::make_shared<SplitAssignmentExprPart>(syrec_operation::operation::XorAssign, assignmentRhsBinaryExpr)};
        }
        isAssignedToSignalZeroPriorToAssignment = false;
        return {std::make_shared<SplitAssignmentExprPart>(originalAssignmentOperation, assignmentRhsBinaryExpr)};
    }*/

    auto mappedToBinaryOperation = *syrec_operation::tryMapBinaryOperationFlagToEnum(binaryExprCasted->op);
  /*  if (mappedToBinaryOperation == syrec_operation::operation::BitwiseXor) {
        if (originalAssignmentOperation == syrec_operation::operation::XorAssign && isAssignedToSignalZeroPriorToAssignment) {
            auto        createdSubAssignments = splitAssignmentRhs(originalAssignmentOperation, binaryExprCasted->lhs, isAssignedToSignalZeroPriorToAssignment);
            const auto& splitRhsOperand       = splitAssignmentSubexpression(mappedToBinaryOperation, binaryExprCasted->rhs);
            createdSubAssignments.insert(createdSubAssignments.cend(), splitRhsOperand.begin(), splitRhsOperand.end());
            return createdSubAssignments;
        }
        isAssignedToSignalZeroPriorToAssignment = false;
        return {std::make_shared<SplitAssignmentExprPart>(originalAssignmentOperation, assignmentRhsBinaryExpr)};
    }*/

    switch (originalAssignmentOperation) {
        case syrec_operation::operation::AddAssign:
            if (isAssignedToSignalZeroPriorToAssignment) {
                originalAssignmentOperation = syrec_operation::operation::XorAssign;
            } else if (mappedToBinaryOperation == syrec_operation::operation::BitwiseXor) {
                return {std::make_shared<SplitAssignmentExprPart>(originalAssignmentOperation, assignmentRhsBinaryExpr)};
            }
            break;
        case syrec_operation::operation::MinusAssign:
            switch (mappedToBinaryOperation) {
                case syrec_operation::operation::Addition:
                    mappedToBinaryOperation = syrec_operation::operation::Subtraction;
                    break;
                case syrec_operation::operation::Subtraction:
                    mappedToBinaryOperation = syrec_operation::operation::Addition;
                    break;
                case syrec_operation::operation::BitwiseXor:
                    return {std::make_shared<SplitAssignmentExprPart>(originalAssignmentOperation, assignmentRhsBinaryExpr)};
                default:
                    break;
            }
            break;
        case syrec_operation::operation::XorAssign:
            if (isAssignedToSignalZeroPriorToAssignment && (mappedToBinaryOperation == syrec_operation::operation::Subtraction || mappedToBinaryOperation == syrec_operation::operation::Addition)) {
                originalAssignmentOperation = syrec_operation::operation::AddAssign;
            }
            /*
             * We cannot split an expression of the form a ^= (<subExpr1> op <subExpr2>) if either the value of a is not zero prior to the assignment
             * or if the operation of the rhs is not '+' since only an addition could potentially be optimized to an '^=' assignment 
             */
            else if (!isAssignedToSignalZeroPriorToAssignment || (isAssignedToSignalZeroPriorToAssignment && mappedToBinaryOperation != syrec_operation::operation::Addition && mappedToBinaryOperation != syrec_operation::operation::BitwiseXor)) {
                return {std::make_shared<SplitAssignmentExprPart>(originalAssignmentOperation, assignmentRhsBinaryExpr)};
            }
            break;
        default:
            break;
    }

    //if (originalAssignmentOperation == syrec_operation::operation::MinusAssign) {
    //    if (mappedToBinaryOperation == syrec_operation::operation::Subtraction) {
    //        mappedToBinaryOperation = syrec_operation::operation::Addition;
    //    } else if (mappedToBinaryOperation == syrec_operation::operation::Addition) {
    //        mappedToBinaryOperation = syrec_operation::operation::Subtraction;
    //    }
    //} else if (originalAssignmentOperation == syrec_operation::operation::XorAssign) {
    //    if (mappedToBinaryOperation == syrec_operation::operation::Addition && isAssignedToSignalZeroPriorToAssignment) {
    //        mappedToBinaryOperation = syrec_operation::operation::BitwiseXor;
    //    } else {
    //        return {std::make_shared<SplitAssignmentExprPart>(originalAssignmentOperation, assignmentRhsBinaryExpr)};
    //    }
    //}

    auto        createdSubAssignments = splitAssignmentRhs(originalAssignmentOperation, binaryExprCasted->lhs, isAssignedToSignalZeroPriorToAssignment);
    const auto& splitRhsOperand       = splitAssignmentSubexpression(mappedToBinaryOperation, binaryExprCasted->rhs);
    createdSubAssignments.insert(createdSubAssignments.cend(), splitRhsOperand.begin(), splitRhsOperand.end());

    if (createdSubAssignments.empty()) {
        createdSubAssignments.emplace_back(std::make_shared<SplitAssignmentExprPart>(originalAssignmentOperation, assignmentRhsBinaryExpr));
    }
    return createdSubAssignments;
}

std::vector<AssignmentWithNonReversibleOperationsAndUniqueSignalOccurrencesSimplifier::SplitAssignmentExprPartReference> AssignmentWithNonReversibleOperationsAndUniqueSignalOccurrencesSimplifier::splitAssignmentSubexpression(syrec_operation::operation parentBinaryExprOperation, const syrec::expression::ptr& subExpr) {
    const auto& binaryExprCasted = std::dynamic_pointer_cast<syrec::BinaryExpression>(subExpr);
    const auto& matchingAssignmentOperationForGivenParentOperation = *syrec_operation::getMatchingAssignmentOperationForOperation(parentBinaryExprOperation);

    if (binaryExprCasted == nullptr || matchingAssignmentOperationForGivenParentOperation == syrec_operation::operation::XorAssign
        || (!doesExprDefineNestedExpr(binaryExprCasted->lhs) && !doesExprDefineNestedExpr(binaryExprCasted->rhs))) {
        return {std::make_shared<SplitAssignmentExprPart>(matchingAssignmentOperationForGivenParentOperation, subExpr)};
    }
   
    auto binaryOperationToUse = *syrec_operation::tryMapBinaryOperationFlagToEnum(binaryExprCasted->op);
    if (parentBinaryExprOperation == syrec_operation::operation::Subtraction) {
        if (binaryOperationToUse == syrec_operation::operation::Addition) {
            binaryOperationToUse = syrec_operation::operation::Subtraction;
        } else if (binaryOperationToUse == syrec_operation::operation::Subtraction) {
            binaryOperationToUse = syrec_operation::operation::Addition;
        }
    }
    
    const auto& mappedToAssignmentOperationForCurrentBinaryOperation = syrec_operation::getMatchingAssignmentOperationForOperation(binaryOperationToUse);
    if (!mappedToAssignmentOperationForCurrentBinaryOperation.has_value()) {
        return {std::make_shared<SplitAssignmentExprPart>(matchingAssignmentOperationForGivenParentOperation, subExpr)};
    }

    auto        createdSubAssignments = splitAssignmentSubexpression(parentBinaryExprOperation, binaryExprCasted->lhs);
    const auto& splitRhsOperand       = splitAssignmentSubexpression(binaryOperationToUse, binaryExprCasted->rhs);
    createdSubAssignments.insert(createdSubAssignments.cend(), splitRhsOperand.begin(), splitRhsOperand.end());
    return createdSubAssignments;
}

syrec::AssignStatement::vec AssignmentWithNonReversibleOperationsAndUniqueSignalOccurrencesSimplifier::createAssignmentsForSplitAssignment(const syrec::VariableAccess::ptr& assignedToSignalParts, const std::vector<SplitAssignmentExprPartReference>& splitUpAssignmentRhsParts) {
    syrec::AssignStatement::vec createdAssignments(splitUpAssignmentRhsParts.size(), nullptr);
    for (std::size_t i = 0; i < createdAssignments.size(); ++i) {
        createdAssignments.at(i) = std::make_shared<syrec::AssignStatement>(
            assignedToSignalParts,
            *syrec_operation::tryMapAssignmentOperationEnumToFlag(splitUpAssignmentRhsParts.at(i)->mappedToAssignmentOperation),
            splitUpAssignmentRhsParts.at(i)->assignmentRhsOperand
        );
    }
    return createdAssignments;
}