#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/assignment_without_additional_lines_simplifier.hpp"

#include "core/syrec/parser/utils/signal_access_utils.hpp"

/*
 * TODO: Is we are parsing an operation node with one leaf node, we could "carry" over the leaf node as a potential assignment candidate to the processing of the non-leaf-node
 * TODO: Switch operands for expr (a - (b - c)) to (a + (c - b))
 */
noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::SimplificationResultReference noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::simplify(const syrec::AssignStatement::ptr& assignmentStatement, const syrec::AssignStatement::vec& assignmentsDefiningSignalPartsBlockedFromEvaluation) {
    resetInternals();
    const auto& givenAssignmentStmtCasted = std::dynamic_pointer_cast<syrec::AssignStatement>(assignmentStatement);
    if (!givenAssignmentStmtCasted || !doesExpressionDefineNestedSplitableExpr(*givenAssignmentStmtCasted->rhs)) {
        SimplificationResultReference simplificationResult = std::make_unique<SimplificationResult>(SimplificationResult{generatedAssignmentsContainer->getAssignments()});
        return simplificationResult;
    }

    signalPartsBlockedFromEvaluationLookup = buildLookupOfSignalsPartsBlockedFromEvaluation(assignmentsDefiningSignalPartsBlockedFromEvaluation, *symbolTableReference);
    expressionTraversalHelper->buildTraversalQueue(givenAssignmentStmtCasted->rhs, *symbolTableReference);
    std::optional<ExpressionTraversalHelper::OperationNodeReference> topMostOperationNode     = expressionTraversalHelper->getNextOperationNode();
    while (topMostOperationNode.has_value()) {
        // TODO: If an expression is created for the rhs we can still try to split it if a binary operation with an assignment counterpart exists
        auto simplificationResultOfTopmostOperationNode = handleOperationNode(*topMostOperationNode);
        if (simplificationResultOfTopmostOperationNode.has_value()) {
            const auto& generatedExprForTopmostOperationRhsExpr = simplificationResultOfTopmostOperationNode->get()->getGeneratedExpr();
            const auto& generatedLastAssignedToSignalOfRhsExpr  = simplificationResultOfTopmostOperationNode->get()->getAssignedToSignalOfAssignment();

            syrec::VariableAccess::ptr generatedAssignmentAssignedToSignal = givenAssignmentStmtCasted->lhs;
            syrec::expression::ptr generatedAssignmentRhsExpr;
            if (generatedLastAssignedToSignalOfRhsExpr.has_value()) {
                generatedAssignmentRhsExpr = std::make_shared<syrec::VariableExpression>(*generatedLastAssignedToSignalOfRhsExpr);
            }
            else {
                generatedAssignmentRhsExpr = *generatedExprForTopmostOperationRhsExpr;
            }
            const syrec::AssignStatement::ptr generatedAssignment = std::make_shared<syrec::AssignStatement>(
                generatedAssignmentAssignedToSignal,
                givenAssignmentStmtCasted->op,
                generatedAssignmentRhsExpr
            );
            generatedAssignmentsContainer->storeActiveAssignment(generatedAssignment);
            // TODO: Is this call necessary ?
            generatedAssignmentsContainer->invertAllAssignmentsUpToLastCutoff(true);
        }
        topMostOperationNode = expressionTraversalHelper->getNextOperationNode();
    }

    syrec::AssignStatement::vec generatedAssignments;
    if (!generatedAssignmentsContainer->getNumberOfAssignments()) {
        generatedAssignments.emplace_back(assignmentStatement);
    } else {
        generatedAssignments = generatedAssignmentsContainer->getAssignments();
    }
    return std::make_unique<SimplificationResult>(SimplificationResult{generatedAssignments});


    //const auto& initialAssignmentCasted = std::dynamic_pointer_cast<syrec::AssignStatement>(assignmentStatement);
    //if (!initialAssignmentCasted) {
    //    auto finalResult = std::make_unique<SimplificationResult>(syrec::AssignStatement::vec());
    //    return finalResult;
    //}

    //syrec::AssignStatement::vec generatedAssignments;
    //if (const std::optional<ExpressionTraversalHelper::OperationNodeReference> currentOperationNode = expressionTraversalHelper->getNextOperationNode(); currentOperationNode.has_value()) {
    //    auto simplificationResultOfTopMostOperationNode = handleOperationNode(*currentOperationNode);
    //    if (!simplificationResultOfTopMostOperationNode.has_value()) {
    //        if (auto decisionForTopMostOperationNode = tryGetDecisionForOperationNode(currentOperationNode->get()->id); decisionForTopMostOperationNode.has_value()) {
    //            if (decisionForTopMostOperationNode->get()->choosenOperand != Decision::ChoosenOperand::None) {
    //                redecide(*currentOperationNode, *decisionForTopMostOperationNode);
    //                simplificationResultOfTopMostOperationNode = handleOperationNode(*currentOperationNode);
    //            }
    //        }

    //        if (!simplificationResultOfTopMostOperationNode.has_value()) {
    //            return nullptr;
    //        }
    //    }

    //    syrec::AssignStatement::ptr closureAssignment;
    //    generatedAssignments = generatedAssignmentsContainer->getAssignments();
    //    if (const auto& generatedAssignmentForInitialAssignmentRhs = simplificationResultOfTopMostOperationNode->get()->getAssignedToSignalOfAssignment(); generatedAssignmentForInitialAssignmentRhs.has_value()) {
    //        const auto& generatedSignalAccessExprForRhs = std::make_shared<syrec::VariableExpression>(*generatedAssignmentForInitialAssignmentRhs);
    //        if (const auto& initialAssignmentClosureAssignment = tryCreateAssignmentForOperationNode(initialAssignmentCasted->lhs, currentOperationNode->get()->operation, generatedSignalAccessExprForRhs); initialAssignmentClosureAssignment.has_value()) {
    //            closureAssignment = *initialAssignmentClosureAssignment;
    //        }
    //    }
    //    if (const auto& generatedExprForInitialAssignmentRhs = simplificationResultOfTopMostOperationNode->get()->getGeneratedExpr(); generatedExprForInitialAssignmentRhs.has_value()) {
    //        if (const auto& initialAssignmentClosureAssignment = tryCreateAssignmentForOperationNode(initialAssignmentCasted->lhs, currentOperationNode->get()->operation, *generatedExprForInitialAssignmentRhs); initialAssignmentClosureAssignment.has_value()) {
    //            closureAssignment = *initialAssignmentClosureAssignment;
    //        }
    //    }

    //    if (closureAssignment) {
    //        generatedAssignments.emplace_back(closureAssignment);
    //    } else {
    //        generatedAssignments.clear();
    //    }
    //}
    //
    //auto finalResult = std::make_unique<SimplificationResult>(generatedAssignments);
    //return finalResult;
}

// START OF NON-PUBLIC FUNCTION IMPLEMENTATIONS
std::optional<noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::OwningOperationOperandSimplificationResultReference> noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::handleOperationNode(const ExpressionTraversalHelper::OperationNodeReference& operationNode) {
    if (!operationNode->hasAnyLeafOperandNodes()) {
        return handleOperationNodeWithNoLeafNodes(operationNode);
    }
    if (operationNode->areBothOperandsLeafNodes()) {
        return handleOperationNodeWithOnlyLeafNodes(operationNode);
    }
    return handleOperationNodeWithOneLeafNode(operationNode);
}

// TODO: If the simplification of any non-leaf node did result in a conflict check whether another decision at a previous position is possible, see other functions handling operation node with one leaf node
std::optional<noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::OwningOperationOperandSimplificationResultReference> noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::handleOperationNodeWithNoLeafNodes(const ExpressionTraversalHelper::OperationNodeReference& operationNode) {
    const std::size_t numExistingAssignmentsPriorToAnyOperandHandled = generatedAssignmentsContainer->getNumberOfAssignments();
    generatedAssignmentsContainer->markCutoffForInvertibleAssignments();
    expressionTraversalHelper->markOperationNodeAsPotentialBacktrackOption(operationNode->id);

    const auto&                                                        firstOperandOperationNode        = expressionTraversalHelper->getNextOperationNode();
    std::optional<OwningOperationOperandSimplificationResultReference> firstOperandSimplificationResult = handleOperationNode(*firstOperandOperationNode);

    if (!firstOperandSimplificationResult.has_value()) {
        if (couldAnotherChoiceBeMadeAtPreviousDecision()) {
            generatedAssignmentsContainer->rollbackAssignmentsMadeSinceLastCutoffAndPopCutoff();
            expressionTraversalHelper->removeOperationNodeAsPotentialBacktrackOperation(operationNode->id);
            return std::nullopt;   
        }
        const std::size_t            operationNodeIdOfFirstOperand = *expressionTraversalHelper->getOperandNodeIdOfNestedOperation(*firstOperandOperationNode->get()->parentNodeId, firstOperandOperationNode->get()->id);
        const syrec::expression::ptr firstOperandDataAsExpr        = *expressionTraversalHelper->getOperandAsExpr(operationNodeIdOfFirstOperand);
        firstOperandSimplificationResult                           = std::make_unique<OperationOperandSimplificationResult>(0, firstOperandDataAsExpr);
    }

    const auto&                                                        secondOperandOperationNode        = expressionTraversalHelper->getNextOperationNode();
    std::optional<OwningOperationOperandSimplificationResultReference> secondOperandSimplificationResult = handleOperationNode(*secondOperandOperationNode);

    if (!secondOperandSimplificationResult.has_value()) {
        if (couldAnotherChoiceBeMadeAtPreviousDecision()) {
            generatedAssignmentsContainer->rollbackAssignmentsMadeSinceLastCutoffAndPopCutoff();
            expressionTraversalHelper->removeOperationNodeAsPotentialBacktrackOperation(operationNode->id);
            return std::nullopt;   
        }
        const std::size_t            operationNodeIdOfSecondOperand = *expressionTraversalHelper->getOperandNodeIdOfNestedOperation(*secondOperandOperationNode->get()->parentNodeId, secondOperandOperationNode->get()->id);
        const syrec::expression::ptr secondOperandDataAsExpr        = *expressionTraversalHelper->getOperandAsExpr(operationNodeIdOfSecondOperand);
        secondOperandSimplificationResult                           = std::make_unique<OperationOperandSimplificationResult>(0, secondOperandDataAsExpr);
    }

    bool                    didConflictPreventChoice = false;
    const DecisionReference madeDecision             = makeDecision(operationNode, std::make_pair(**firstOperandSimplificationResult, **secondOperandSimplificationResult), didConflictPreventChoice);

    std::variant<syrec::VariableAccess::ptr, syrec::expression::ptr> simplificationResultData;
    bool                                                             shouldBacktrackDueToConflict = false;

    if (!didConflictPreventChoice && madeDecision->choosenOperand != Decision::ChoosenOperand::None) {
        const std::optional<syrec::AssignStatement::ptr> generatedAssignment = tryCreateAssignmentFromOperands(madeDecision->choosenOperand, **firstOperandSimplificationResult, operationNode->operation, **secondOperandSimplificationResult);
        generatedAssignmentsContainer->storeActiveAssignment(*generatedAssignment);
        generatedAssignmentsContainer->invertAllAssignmentsUpToLastCutoff(true);
        simplificationResultData = std::dynamic_pointer_cast<syrec::AssignStatement>(*generatedAssignment)->lhs;
    }
    else {
        shouldBacktrackDueToConflict = !couldAnotherChoiceBeMadeAtPreviousDecision();
        if (!shouldBacktrackDueToConflict) {
            const std::optional<syrec::expression::ptr> generatedExpr = tryCreateExpressionFromOperationNodeOperandSimplifications(**firstOperandSimplificationResult, operationNode->operation, **secondOperandSimplificationResult);
            simplificationResultData = *generatedExpr;
        }
    }

    removeDecisionFor(operationNode->id);
    expressionTraversalHelper->removeOperationNodeAsPotentialBacktrackOperation(operationNode->id);
    generatedAssignmentsContainer->popLastCutoffForInvertibleAssignments();
    if (shouldBacktrackDueToConflict) {
        expressionTraversalHelper->backtrack();
        return std::nullopt;
    }

    const std::size_t numExistingAssignmentsAfterOperationNodeWasHandled = generatedAssignmentsContainer->getNumberOfAssignments();
    const std::size_t numGeneratedAssignmentsByHandlingOfOperationNode                     = numExistingAssignmentsAfterOperationNodeWasHandled - numExistingAssignmentsPriorToAnyOperandHandled;
    return std::make_unique<OperationOperandSimplificationResult>(numGeneratedAssignmentsByHandlingOfOperationNode, simplificationResultData);
}


//std::optional<noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::OwningOperationOperandSimplificationResultReference> noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::handleOperationNodeWithNoLeafNodes(const ExpressionTraversalHelper::OperationNodeReference& operationNode) {
//    const std::size_t numberOfExistingAssignmentsBeforeOperationNodeWasHandled           = generatedAssignmentsContainer->getNumberOfAssignments();
//    generatedAssignmentsContainer->markCutoffForInvertibleAssignments();
//
//    const auto& lhsOperationNode = expressionTraversalHelper->getNextOperationNode();
//    auto        simplificationResultOfLhsOperand = handleOperationNode(*lhsOperationNode);
//    if (!simplificationResultOfLhsOperand.has_value()) {
//        generatedAssignmentsContainer->rollbackAssignmentsMadeSinceLastCutoffAndPopCutoff();
//        return std::nullopt;
//    }
//
//    const auto& rhsOperationNode = expressionTraversalHelper->getNextOperationNode();
//    auto        simplificationResultOfRhsOperand = handleOperationNode(*rhsOperationNode);
//    if (!simplificationResultOfRhsOperand.has_value()) {
//        generatedAssignmentsContainer->rollbackAssignmentsMadeSinceLastCutoffAndPopCutoff();
//        return std::nullopt;
//    }
//
//    const auto& lastAssignedToSignalOfLhsOperand = simplificationResultOfLhsOperand->get()->getAssignedToSignalOfAssignment();
//    const auto& generatedExprOfLhsOperand        = simplificationResultOfLhsOperand->get()->getGeneratedExpr();
//    const auto& lastAssignedToSignalOfRhsOperand = simplificationResultOfRhsOperand->get()->getAssignedToSignalOfAssignment();
//    const auto& generatedExprOfRhsOperand        = simplificationResultOfRhsOperand->get()->getGeneratedExpr();
//
//    const auto& matchingAssignmentOperationForOperationNode = syrec_operation::getMatchingAssignmentOperationForOperation(operationNode->operation);
//    if (matchingAssignmentOperationForOperationNode.has_value() && (lastAssignedToSignalOfLhsOperand.has_value() || lastAssignedToSignalOfRhsOperand.has_value())) {
//        std::shared_ptr<syrec::VariableAccess> generatedAssignmentLhsOperand = lastAssignedToSignalOfLhsOperand.value_or(*lastAssignedToSignalOfRhsOperand);
//        bool                                   wasLhsOperandChosenAsAssignedToOperand = lastAssignedToSignalOfLhsOperand.has_value();
//        if (doesAssignmentToSignalLeadToConflict(*generatedAssignmentLhsOperand)) {
//            if (syrec_operation::isCommutative(operationNode->operation) && (wasLhsOperandChosenAsAssignedToOperand ? lastAssignedToSignalOfRhsOperand.has_value() : lastAssignedToSignalOfLhsOperand.has_value())) {
//                generatedAssignmentLhsOperand = wasLhsOperandChosenAsAssignedToOperand ? *lastAssignedToSignalOfRhsOperand : *lastAssignedToSignalOfLhsOperand;
//                wasLhsOperandChosenAsAssignedToOperand = !wasLhsOperandChosenAsAssignedToOperand;
//                if (doesAssignmentToSignalLeadToConflict(*generatedAssignmentLhsOperand)) {
//                    generatedAssignmentLhsOperand = nullptr;
//                }
//            } else {
//                generatedAssignmentLhsOperand = nullptr;
//            }
//        }
//
//        if (generatedAssignmentLhsOperand) {
//            syrec::expression::ptr generatedAssignmentRhsOperand = wasLhsOperandChosenAsAssignedToOperand ? generatedExprOfRhsOperand.value_or(nullptr) : generatedExprOfLhsOperand.value_or(nullptr);
//            if (!generatedAssignmentRhsOperand) {
//                const auto& temporaryContainerForLastGeneratedAssignment = wasLhsOperandChosenAsAssignedToOperand ? *lastAssignedToSignalOfRhsOperand : lastAssignedToSignalOfLhsOperand;
//                generatedAssignmentRhsOperand = std::make_shared<syrec::VariableExpression>(*temporaryContainerForLastGeneratedAssignment);
//            }
//
//            if (const auto& generatedAssignment = tryCreateAssignmentForOperationNode(generatedAssignmentLhsOperand, operationNode->operation, generatedAssignmentRhsOperand); generatedAssignment) {
//                generatedAssignmentsContainer->storeActiveAssignment(*generatedAssignment);
//                // Exclude last generated assignment
//                generatedAssignmentsContainer->invertAllAssignmentsUpToLastCutoff(true);
//                generatedAssignmentsContainer->popLastCutoffForInvertibleAssignments();
//
//                const std::size_t currentNumberOfExistingAssignments = generatedAssignmentsContainer->getNumberOfAssignments();
//                const std::size_t numberOfGeneratedAssignmentsForOperationNode = currentNumberOfExistingAssignments - numberOfExistingAssignmentsBeforeOperationNodeWasHandled;
//                auto              simplificationResult                         = std::make_unique<OperationOperandSimplificationResult>(numberOfGeneratedAssignmentsForOperationNode, generatedAssignmentLhsOperand);
//                return simplificationResult;
//            }
//            return std::nullopt;
//        }
//
//        const auto& predecessorDecision                          = tryGetLastDecision();
//        const bool  wereDecisionOptionsOfPredecessorNotExhausted = !predecessorDecision.has_value() || predecessorDecision->get()->choosenOperand != Decision::ChoosenOperand::None;
//        if (!wereDecisionOptionsOfPredecessorNotExhausted) {
//            generatedAssignmentsContainer->rollbackAssignmentsMadeSinceLastCutoffAndPopCutoff();
//            backtrackToLastDecision();
//            return std::nullopt;
//        }
//    }
//
//    syrec::expression::ptr generatedExprLhs;
//    if (lastAssignedToSignalOfLhsOperand.has_value()) {
//        generatedExprLhs = std::make_shared<syrec::VariableExpression>(*lastAssignedToSignalOfLhsOperand);
//    } else {
//        generatedExprLhs = *generatedExprOfLhsOperand;
//    }
//
//    syrec::expression::ptr generatedExprRhs;
//    if (lastAssignedToSignalOfRhsOperand.has_value()) {
//        generatedExprRhs = std::make_shared<syrec::VariableExpression>(*lastAssignedToSignalOfRhsOperand);
//    } else {
//        generatedExprRhs = *generatedExprOfRhsOperand;
//    }
//
//    if (const auto& generatedExpr = createExpressionFrom(generatedExprLhs, operationNode->operation, generatedExprRhs); generatedExpr) {
//        generatedAssignmentsContainer->popLastCutoffForInvertibleAssignments();
//
//        const std::size_t currentNumberOfExistingAssignments          = generatedAssignmentsContainer->getNumberOfAssignments();
//        const std::size_t numberOfGeneratedAssignmentsForOperationNode = currentNumberOfExistingAssignments - numberOfExistingAssignmentsBeforeOperationNodeWasHandled;
//        auto              simplificationResult                         = std::make_unique<OperationOperandSimplificationResult>(numberOfGeneratedAssignmentsForOperationNode, generatedExpr);
//        return simplificationResult;
//    }
//
//    generatedAssignmentsContainer->rollbackAssignmentsMadeSinceLastCutoffAndPopCutoff();
//    return std::nullopt;
//}

std::optional<noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::OwningOperationOperandSimplificationResultReference> noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::handleOperationNodeWithOneLeafNode(const ExpressionTraversalHelper::OperationNodeReference& operationNode) {
    expressionTraversalHelper->markOperationNodeAsPotentialBacktrackOption(operationNode->id);
    generatedAssignmentsContainer->markCutoffForInvertibleAssignments();
    const std::size_t numExistingAssignmentsPriorToHandlingOfOperationNode = generatedAssignmentsContainer->getNumberOfAssignments();

    const auto&                                                              dataOfOperationNodeOperand                 = *expressionTraversalHelper->getNextOperationNode();
    std::optional<OwningOperationOperandSimplificationResultReference>       simplificationResultOfOperationNodeOperand = handleOperationNode(dataOfOperationNodeOperand);
    const bool                                                               wasLhsOperandLeafNode                      = operationNode->getLeafNodeOperandId().value() == operationNode->lhsOperand.id;
    const std::optional<syrec::VariableExpression::ptr>                      dataOfLeafNodeAsVariableExpr               = expressionTraversalHelper->getOperandAsVariableExpr(wasLhsOperandLeafNode ? operationNode->lhsOperand.id : operationNode->rhsOperand.id);
    
    std::variant<syrec::VariableAccess::ptr, syrec::expression::ptr> simplificationResultData;
    bool                                                             didConflictPreventChoice = false;
    DecisionReference                                                madeDecision             = nullptr;

    std::variant<syrec::VariableAccess::ptr, syrec::expression::ptr> simplificationResultDataOfLeafNode;
    if (dataOfLeafNodeAsVariableExpr.has_value()) {
        simplificationResultDataOfLeafNode = std::dynamic_pointer_cast<syrec::VariableExpression>(*dataOfLeafNodeAsVariableExpr)->var;
    } else {
        simplificationResultDataOfLeafNode = *expressionTraversalHelper->getOperandAsExpr(wasLhsOperandLeafNode ? operationNode->lhsOperand.id : operationNode->rhsOperand.id);
    }
    const auto simplificationResultOfLeafNode = OperationOperandSimplificationResult(0, simplificationResultDataOfLeafNode);

    if (simplificationResultOfOperationNodeOperand.has_value()) {
        if (wasLhsOperandLeafNode) {
            madeDecision = makeDecision(operationNode, std::make_pair(simplificationResultOfLeafNode, **simplificationResultOfOperationNodeOperand), didConflictPreventChoice);
        } else {
            madeDecision = makeDecision(operationNode, std::make_pair(**simplificationResultOfOperationNodeOperand, simplificationResultOfLeafNode), didConflictPreventChoice);
        }
    } else {
        if (!couldAnotherChoiceBeMadeAtPreviousDecision()) {
            const std::size_t operandIdOfNonLeafNode   = wasLhsOperandLeafNode ? operationNode->rhsOperand.id : operationNode->lhsOperand.id;
            simplificationResultOfOperationNodeOperand = std::make_unique<OperationOperandSimplificationResult>(0, *expressionTraversalHelper->getOperandAsExpr(operandIdOfNonLeafNode));   
        }
    }
    
    expressionTraversalHelper->removeOperationNodeAsPotentialBacktrackOperation(operationNode->id);
    if (!didConflictPreventChoice && madeDecision->choosenOperand != Decision::ChoosenOperand::None) {
        const std::optional<syrec::AssignStatement::ptr> generatedAssignment = wasLhsOperandLeafNode
            ? tryCreateAssignmentFromOperands(madeDecision->choosenOperand, simplificationResultOfLeafNode, operationNode->operation, **simplificationResultOfOperationNodeOperand)
            : tryCreateAssignmentFromOperands(madeDecision->choosenOperand, **simplificationResultOfOperationNodeOperand, operationNode->operation, simplificationResultOfLeafNode);

        generatedAssignmentsContainer->storeActiveAssignment(*generatedAssignment);
        generatedAssignmentsContainer->invertAllAssignmentsUpToLastCutoff(true);
        simplificationResultData = std::dynamic_pointer_cast<syrec::AssignStatement>(*generatedAssignment)->lhs;
    } else {
        if (couldAnotherChoiceBeMadeAtPreviousDecision()) {
            generatedAssignmentsContainer->rollbackAssignmentsMadeSinceLastCutoffAndPopCutoff();
            expressionTraversalHelper->backtrack();
            return std::nullopt;
        }

        const std::optional<syrec::expression::ptr> generatedExpr = wasLhsOperandLeafNode
            ? tryCreateExpressionFromOperationNodeOperandSimplifications(simplificationResultOfLeafNode, operationNode->operation, **simplificationResultOfOperationNodeOperand)
            : tryCreateExpressionFromOperationNodeOperandSimplifications(**simplificationResultOfOperationNodeOperand, operationNode->operation, simplificationResultOfLeafNode);

        simplificationResultData = *generatedExpr;
        generatedAssignmentsContainer->invertAllAssignmentsUpToLastCutoff(false);   
    }
    generatedAssignmentsContainer->popLastCutoffForInvertibleAssignments();

    const std::size_t numExistingAssignmentsAfterHandlingOfOperationNode = generatedAssignmentsContainer->getNumberOfAssignments();
    const std::size_t numGeneratedAssignmentsByHandlingOfOperationNode   = numExistingAssignmentsAfterHandlingOfOperationNode - numExistingAssignmentsPriorToHandlingOfOperationNode;
    auto simplificationResult                                            = std::make_unique<OperationOperandSimplificationResult>(numGeneratedAssignmentsByHandlingOfOperationNode, simplificationResultData);
    return simplificationResult;
}


//std::optional<noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::OwningOperationOperandSimplificationResultReference> noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::handleOperationNodeWithOneLeafNode(const ExpressionTraversalHelper::OperationNodeReference& operationNode) {
//    const auto& operationNodeOfNonLeafNode        = expressionTraversalHelper->getNextOperationNode();
//    const std::size_t numberOfExistingAssignmentPriorToHandlingOfNonLeafNode = generatedAssignmentsContainer->getNumberOfAssignments();
//
//    expressionTraversalHelper->markOperationNodeAsPotentialBacktrackOption(operationNode->id);
//    generatedAssignmentsContainer->markCutoffForInvertibleAssignments();
//
//    std::optional<OwningOperationOperandSimplificationResultReference> simplificationResultOfNonLeafNode;
//    if (!tryGetDecisionForOperationNode(operationNode->id).has_value()) {
//        simplificationResultOfNonLeafNode = handleOperationNode(*operationNodeOfNonLeafNode);
//        makeDecision(operationNode);
//    }
//    
//    DecisionReference madeDecisionForOperationNode = *tryGetDecisionForOperationNode(operationNode->id);
//    if (!simplificationResultOfNonLeafNode.has_value()) {
//        const auto& predecessorDecision = tryGetLastDecision();
//        const auto& wereAllOptionsOfPredecessorDecisionExhausted = !predecessorDecision.has_value() || predecessorDecision->get()->choosenOperand == Decision::ChoosenOperand::None;
//        
//        expressionTraversalHelper->removeOperationNodeAsPotentialBacktrackOperation(operationNode->id);
//        removeDecisionFor(operationNode->id);
//        if (!wereAllOptionsOfPredecessorDecisionExhausted) {
//            expressionTraversalHelper->backtrack();
//            generatedAssignmentsContainer->rollbackAssignmentsMadeSinceLastCutoffAndPopCutoff();
//            return std::nullopt;
//        }
//
//        generatedAssignmentsContainer->popLastCutoffForInvertibleAssignments();
//        const auto& generatedExpr = createExpressionFromOperationNode(expressionTraversalHelper, operationNode);
//        auto        simplificationResult = std::make_unique<OperationOperandSimplificationResult>(0, generatedExpr);
//        return simplificationResult;
//    }
//
//    const bool isLhsOperandOfOperationNodeLeafNode = operationNode->lhsOperand.isLeafNode;
//    syrec::VariableExpression::ptr generatedAssignmentLhsOperand;
//    syrec::expression::ptr         generatedAssignmentRhsOperand;
//
//    if (madeDecisionForOperationNode->choosenOperand == Decision::ChoosenOperand::Left) {
//        if (isLhsOperandOfOperationNodeLeafNode) {
//            generatedAssignmentLhsOperand = *expressionTraversalHelper->getOperandAsVariableExpr(operationNode->lhsOperand.id);
//        } else if (const auto& lastAssignedToSignalOfNonLeafNode = simplificationResultOfNonLeafNode->get()->getAssignedToSignalOfAssignment(); lastAssignedToSignalOfNonLeafNode.has_value()) {
//            generatedAssignmentLhsOperand = std::make_shared<syrec::VariableExpression>(*lastAssignedToSignalOfNonLeafNode);
//        }
//    } else if (madeDecisionForOperationNode->choosenOperand == Decision::ChoosenOperand::Right) {
//        if (!isLhsOperandOfOperationNodeLeafNode) {
//            generatedAssignmentLhsOperand = *expressionTraversalHelper->getOperandAsVariableExpr(operationNode->rhsOperand.id);
//        } else if (const auto& lastAssignedToSignalOfNonLeafNode = simplificationResultOfNonLeafNode->get()->getAssignedToSignalOfAssignment(); lastAssignedToSignalOfNonLeafNode.has_value()) {
//            generatedAssignmentLhsOperand = std::make_shared<syrec::VariableExpression>(*lastAssignedToSignalOfNonLeafNode);
//        }
//    }
//
//    if (generatedAssignmentLhsOperand && generatedAssignmentRhsOperand) {
//        if (generatedAssignmentRhsOperand) {
//            const auto        generatedAssignmentLhsOperandCasted                       = std::dynamic_pointer_cast<syrec::VariableExpression>(generatedAssignmentLhsOperand);
//            const std::size_t numberOfAssignmentsAfterHandlingOfNonLeafNode             = generatedAssignmentsContainer->getNumberOfAssignments();
//            const std::size_t numberOfAssignmentsGeneratedDuringHandlingOfOperationNode = numberOfAssignmentsAfterHandlingOfNonLeafNode - numberOfExistingAssignmentPriorToHandlingOfNonLeafNode;
//            const auto&       generatedAssignment                                       = tryCreateAssignmentForOperationNode(generatedAssignmentLhsOperandCasted->var, operationNode->operation, generatedAssignmentRhsOperand);
//            generatedAssignmentsContainer->storeActiveAssignment(*generatedAssignment);
//            generatedAssignmentsContainer->invertAllAssignmentsUpToLastCutoff(true);
//            generatedAssignmentsContainer->popLastCutoffForInvertibleAssignments();
//            expressionTraversalHelper->removeOperationNodeAsPotentialBacktrackOperation(operationNode->id);
//
//            auto simplificationResult = std::make_unique<OperationOperandSimplificationResult>(numberOfAssignmentsGeneratedDuringHandlingOfOperationNode, generatedAssignmentLhsOperandCasted->var);
//            return simplificationResult;
//        }
//    }
//
//    const auto& generatedExpr        = createExpressionFromOperationNode(expressionTraversalHelper, operationNode);
//    auto        simplificationResult = std::make_unique<OperationOperandSimplificationResult>(0, generatedExpr);
//    return simplificationResult;
//}

std::optional<noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::OwningOperationOperandSimplificationResultReference> noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::handleOperationNodeWithOnlyLeafNodes(const ExpressionTraversalHelper::OperationNodeReference& operationNode) {
    std::optional<std::shared_ptr<syrec::VariableAccess>> accessedSignalPartsOfFirstOperand;
    if (const auto& accessedSignalPartsExprOfFirstOperand = expressionTraversalHelper->getOperandAsVariableExpr(operationNode->lhsOperand.id); accessedSignalPartsExprOfFirstOperand.has_value()) {
        const auto accessedSignalPartsOfFirstOperandCasted = std::dynamic_pointer_cast<syrec::VariableExpression>(*accessedSignalPartsExprOfFirstOperand);
        accessedSignalPartsOfFirstOperand                  = accessedSignalPartsOfFirstOperandCasted->var;
    }

    std::optional<std::shared_ptr<syrec::VariableAccess>> accessedSignalPartsOfSecondOperand;
    if (const auto& accessedSignalPartsExprOfSecondOperand = expressionTraversalHelper->getOperandAsVariableExpr(operationNode->rhsOperand.id); accessedSignalPartsExprOfSecondOperand.has_value()) {
        const auto accessedSignalPartsOfSecondOperandCasted = std::dynamic_pointer_cast<syrec::VariableExpression>(*accessedSignalPartsExprOfSecondOperand);
        accessedSignalPartsOfSecondOperand                  = accessedSignalPartsOfSecondOperandCasted->var;
    }
    
    std::variant<syrec::VariableAccess::ptr, syrec::expression::ptr> simplificationResultDataOfFirstOperand;
    if (accessedSignalPartsOfFirstOperand.has_value()) {
        simplificationResultDataOfFirstOperand = *accessedSignalPartsOfFirstOperand;
    } else {
        simplificationResultDataOfFirstOperand = *expressionTraversalHelper->getOperandAsExpr(operationNode->lhsOperand.id);
    }
    const OperationOperandSimplificationResult simplificationResultOfFirstOperand(0, simplificationResultDataOfFirstOperand);
    
    std::variant<syrec::VariableAccess::ptr, syrec::expression::ptr> simplificationResultDataOfSecondOperand;
    if (accessedSignalPartsOfSecondOperand.has_value()) {
        simplificationResultDataOfSecondOperand = *accessedSignalPartsOfSecondOperand;
    } else {
        simplificationResultDataOfSecondOperand = *expressionTraversalHelper->getOperandAsExpr(operationNode->rhsOperand.id);
    }
    const OperationOperandSimplificationResult simplificationResultOfSecondOperand(0, simplificationResultDataOfSecondOperand);

    bool                    didConflictPreventChoice = false;
    const DecisionReference madeDecision             = makeDecision(operationNode, std::make_pair(simplificationResultOfFirstOperand, simplificationResultOfSecondOperand), didConflictPreventChoice);

    std::size_t                                                      numGeneratedAssignmentsByHandlingOfOperationNode = 0;
    std::variant<syrec::VariableAccess::ptr, syrec::expression::ptr> simplificationResultData;
    bool                                                             shouldBacktrackDueToConflict = false;

    if (!didConflictPreventChoice && madeDecision->choosenOperand != Decision::ChoosenOperand::None) {
        const std::optional<syrec::AssignStatement::ptr> generatedAssignment = tryCreateAssignmentFromOperands(madeDecision->choosenOperand, simplificationResultOfFirstOperand, operationNode->operation, simplificationResultOfSecondOperand);
        generatedAssignmentsContainer->storeActiveAssignment(*generatedAssignment);
        simplificationResultData = std::dynamic_pointer_cast<syrec::AssignStatement>(*generatedAssignment)->lhs;
        ++numGeneratedAssignmentsByHandlingOfOperationNode;
        
    } else {
        shouldBacktrackDueToConflict = !couldAnotherChoiceBeMadeAtPreviousDecision();
        if (!shouldBacktrackDueToConflict) {
            const std::optional<syrec::expression::ptr> generatedExpr = tryCreateExpressionFromOperationNodeOperandSimplifications(simplificationResultOfFirstOperand, operationNode->operation, simplificationResultOfSecondOperand);
            simplificationResultData                                  = *generatedExpr;
        }
    }

    removeDecisionFor(operationNode->id);
    if (shouldBacktrackDueToConflict) {
        expressionTraversalHelper->backtrack();
        return std::nullopt;
    }
    return std::make_unique<OperationOperandSimplificationResult>(numGeneratedAssignmentsByHandlingOfOperationNode, simplificationResultData);
}


//
//std::optional<noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::OwningOperationOperandSimplificationResultReference> noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::handleOperationNodeWithOnlyLeafNodes(const ExpressionTraversalHelper::OperationNodeReference& operationNode) {
//    makeDecision(operationNode);
//    const auto& matchingAssignmentOperationForOperationNodeOperation = syrec_operation::getMatchingAssignmentOperationForOperation(operationNode->operation);
//    if (!matchingAssignmentOperationForOperationNodeOperation.has_value()) {
//        const auto& generatedExpressionForOperation = createExpressionFromOperationNode(expressionTraversalHelper, operationNode);
//        auto        simplificationResult            = std::make_unique<OperationOperandSimplificationResult>(0, generatedExpressionForOperation);
//        return simplificationResult;
//    }
//    
//    makeDecision(operationNode);
//    DecisionReference madeDecisionForOperationNode = *tryGetDecisionForOperationNode(operationNode->id);
//    const auto&       predecessorDecision                          = tryGetSecondToLastDecision();
//    const bool        wereAllOptionsOfPredecessorDecisionExhausted = !predecessorDecision.has_value() || predecessorDecision.value()->choosenOperand == Decision::ChoosenOperand::None;
//
//    if (madeDecisionForOperationNode->choosenOperand == Decision::ChoosenOperand::None) {
//        removeDecisionFor(operationNode->id);   
//        /*
//         * If either no predecessor decision exists or if all potential options of the predecessor decision were exhausted,
//         * create an expression from the given operation node. Otherwise, backtrack to the last decision.
//         */
//        if (wereAllOptionsOfPredecessorDecisionExhausted) {
//            const auto& generatedExpressionForOperation = createExpressionFromOperationNode(expressionTraversalHelper, operationNode);
//            auto        simplificationResult            = std::make_unique<OperationOperandSimplificationResult>(0, generatedExpressionForOperation);
//            return simplificationResult;
//        }
//
//        backtrackToLastDecision();
//        return std::nullopt;
//    }
//
//    /*
//     * If the current decision chose one the the operands, check whether this decision would lead to a conflict.
//     * If that is the case as well as the operation of the current operation node being commutative, perform a redecision and repeat the check of the first step if the other operand was selected.
//     * Finally, when all operand options for the current operation node were exhausted and thus an expression should be created perform the following checks,
//     * if not all options of the predecessor decision were exhausted report a conflict at the current operation node and backtrack. Otherwise, create the expressio and return.
//     *
//     */
//    bool wasLeftOperandChosen = madeDecisionForOperationNode->choosenOperand == Decision::ChoosenOperand::Left;
//    std::shared_ptr<syrec::VariableExpression> exprContainingChosenOperandAsVariableAccess;
//    if (const std::optional<syrec::VariableExpression::ptr> dataOfChosenOperandAsVariableAccess = expressionTraversalHelper->getOperandAsVariableExpr(wasLeftOperandChosen ? operationNode->lhsOperand.id : operationNode->rhsOperand.id); dataOfChosenOperandAsVariableAccess.has_value()) {
//        exprContainingChosenOperandAsVariableAccess = std::dynamic_pointer_cast<syrec::VariableExpression>(*dataOfChosenOperandAsVariableAccess);
//        if (doesAssignmentToSignalLeadToConflict(*exprContainingChosenOperandAsVariableAccess->var)) {
//            exprContainingChosenOperandAsVariableAccess            = nullptr;
//            const Decision::ChoosenOperand previouslyChosenOperand = madeDecisionForOperationNode->choosenOperand;
//            redecide(operationNode, madeDecisionForOperationNode);
//
//            const bool wasOtherOperandChosenAfterRedecide = previouslyChosenOperand != madeDecisionForOperationNode->choosenOperand && madeDecisionForOperationNode->choosenOperand != Decision::ChoosenOperand::None;
//            if (wasOtherOperandChosenAfterRedecide) {
//                wasLeftOperandChosen = madeDecisionForOperationNode->choosenOperand == Decision::ChoosenOperand::Left;
//                const std::optional<syrec::VariableExpression::ptr> dataOfChosenOperandAfterRedecideAsVariableAccess = expressionTraversalHelper->getOperandAsVariableExpr(wasLeftOperandChosen ? operationNode->lhsOperand.id : operationNode->rhsOperand.id);
//                exprContainingChosenOperandAsVariableAccess                                                          = std::dynamic_pointer_cast<syrec::VariableExpression>(*dataOfChosenOperandAfterRedecideAsVariableAccess);
//                if (doesAssignmentToSignalLeadToConflict(*exprContainingChosenOperandAsVariableAccess->var)) {
//                    exprContainingChosenOperandAsVariableAccess = nullptr;
//                }
//            }
//        }
//    }
//
//    if (!exprContainingChosenOperandAsVariableAccess) {
//        removeDecisionFor(operationNode->id);
//        /*
//         * If either no predecessor decision exists or if all potential options of the predecessor decision were exhausted,
//         * create an expression from the given operation node. Otherwise, backtrack to the last decision.
//         */
//        if (wereAllOptionsOfPredecessorDecisionExhausted) {
//            const auto& generatedExpressionForOperation = createExpressionFromOperationNode(expressionTraversalHelper, operationNode);
//            auto        simplificationResult            = std::make_unique<OperationOperandSimplificationResult>(0, generatedExpressionForOperation);
//            return simplificationResult;
//        }
//
//        backtrackToLastDecision();
//        return std::nullopt;
//    }
//
//    const syrec::expression::ptr generatedAssignmentRhsExpr = *expressionTraversalHelper->getOperandAsExpr(wasLeftOperandChosen ? operationNode->lhsOperand.id : operationNode->rhsOperand.id);
//    const auto&                  generatedAssignment        = *tryCreateAssignmentForOperationNode(exprContainingChosenOperandAsVariableAccess->var, operationNode->operation, generatedAssignmentRhsExpr);
//    generatedAssignmentsContainer->storeActiveAssignment(generatedAssignment);
//    auto simplificationResult = std::make_unique<OperationOperandSimplificationResult>(1, exprContainingChosenOperandAsVariableAccess->var);
//    return simplificationResult;
//}

inline bool noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::doesAssignmentToSignalLeadToConflict(const syrec::VariableAccess& assignedToSignal) const {
    return generatedAssignmentsContainer->existsOverlappingAssignmentFor(assignedToSignal, *symbolTableReference);
}

std::optional<unsigned> noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::tryFetchValueOfSignal(const syrec::VariableAccess& assignedToSignal) const {
    if (!isValueEvaluationOfAccessedSignalPartsBlocked(assignedToSignal)) {
        if (const auto& containerForSignalAccessLookup = std::make_shared<syrec::VariableAccess>(assignedToSignal); containerForSignalAccessLookup) {
            return symbolTableReference->tryFetchValueForLiteral(containerForSignalAccessLookup);    
        }
    }
    return std::nullopt;
}

std::optional<noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::DecisionReference> noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::tryGetDecisionForOperationNode(const std::size_t& operationNodeId) const {
    const auto& matchingDecisionForOperationNode = std::find_if(
            pastDecisions.cbegin(),
            pastDecisions.cend(),
            [&operationNodeId](const DecisionReference& pastDecision) {
                return pastDecision->operationNodeId == operationNodeId;
            });
    if (matchingDecisionForOperationNode != pastDecisions.cend()) {
        return *matchingDecisionForOperationNode;
    }
    return std::nullopt;
}

bool noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::isValueEvaluationOfAccessedSignalPartsBlocked(const syrec::VariableAccess& accessedSignalParts) const {
    if (!signalPartsBlockedFromEvaluationLookup->count(accessedSignalParts.var->name)) {
        return false;
    }

    const auto& existingRestrictionsBlockingValueEvaluation = signalPartsBlockedFromEvaluationLookup->at(accessedSignalParts.var->name);
    const bool  isValueLookupBlocked                        = !std::all_of(
        existingRestrictionsBlockingValueEvaluation.cbegin(),
        existingRestrictionsBlockingValueEvaluation.cend(),
        [&](const syrec::VariableAccess::ptr& existingLookupEntry) {
                const SignalAccessUtils::SignalAccessEquivalenceResult signalAccessEquivalenceResult = SignalAccessUtils::areSignalAccessesEqual(
                    *existingLookupEntry, accessedSignalParts,
                    SignalAccessUtils::SignalAccessComponentEquivalenceCriteria::DimensionAccess::Overlapping,
                    SignalAccessUtils::SignalAccessComponentEquivalenceCriteria::BitRange::Overlapping,
                    *symbolTableReference);
                return signalAccessEquivalenceResult.isResultCertain && signalAccessEquivalenceResult.equality == SignalAccessUtils::SignalAccessEquivalenceResult::NotEqual;
            });

    return isValueLookupBlocked || generatedAssignmentsContainer->existsOverlappingAssignmentFor(accessedSignalParts, *symbolTableReference);
}


//noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::Decision::ChoosenOperand noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::chooseBetweenPotentialOperands(const ExpressionTraversalHelper::OperationNodeReference& operationNode, const std::optional<Decision::ChoosenOperand>& previouslyChosenOperand) const {
//    if (previouslyChosenOperand.has_value() && *previouslyChosenOperand == Decision::ChoosenOperand::None) {
//        return *previouslyChosenOperand;
//    }
//
//    const std::optional<syrec_operation::operation> matchingAssignmentOperationForOperationNode = syrec_operation::getMatchingAssignmentOperationForOperation(operationNode->operation);
//    if (!operationNode->hasAnyLeafOperandNodes() || !matchingAssignmentOperationForOperationNode.has_value()) {
//        return Decision::ChoosenOperand::None;
//    }
//
//    if (!previouslyChosenOperand.has_value()) {
//        const auto isLhsOperandLeafNode = operationNode->getLeafNodeOperandId().value() == operationNode->lhsOperand.id;
//        return isLhsOperandLeafNode ? Decision::ChoosenOperand::Left : Decision::ChoosenOperand::Right;
//    }
//    if (previouslyChosenOperand.has_value() && syrec_operation::isCommutative(operationNode->operation) && operationNode->areBothOperandsLeafNodes()) {
//        return *previouslyChosenOperand == Decision::ChoosenOperand::Left ? Decision::ChoosenOperand::Right : Decision::ChoosenOperand::Left;
//    }
//    return Decision::ChoosenOperand::None;
//}

//noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::DecisionReference noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::makeDecision(const ExpressionTraversalHelper::OperationNodeReference& operationNode) {
//    if (auto previousDecisionForOperationNode = tryGetDecisionForOperationNode(operationNode->id); previousDecisionForOperationNode.has_value()) {
//        redecide(operationNode, *previousDecisionForOperationNode);
//        return;
//    }
//
//    const std::size_t existingAssignmentPriorToDecision = generatedAssignmentsContainer->getNumberOfAssignments();
//    const Decision::ChoosenOperand choosenOperand                    = chooseBetweenPotentialOperands(operationNode, std::nullopt);
//    const auto                     madeDecision                      = std::make_shared<Decision>(operationNode->id, choosenOperand, existingAssignmentPriorToDecision, nullptr);
//    pastDecisions.emplace_back(madeDecision);
//}

noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::DecisionReference noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::makeDecision(const ExpressionTraversalHelper::OperationNodeReference& operationNode, const std::pair<std::reference_wrapper<const OperationOperandSimplificationResult>, std::reference_wrapper<const OperationOperandSimplificationResult>>& potentialChoices, bool& didConflictPreventChoice) {
    const std::optional<DecisionReference>          previousDecisionForOperationNode = tryGetDecisionForOperationNode(operationNode->id);
    const std::optional<syrec::VariableAccess::ptr> firstOperandAsVariableAccess = potentialChoices.first.get().getAssignedToSignalOfAssignment();
    const std::optional<syrec::expression::ptr>     firstOperandAsExpr           = potentialChoices.first.get().getGeneratedExpr();
    const std::optional<syrec::VariableAccess::ptr> secondOperandAsVariableAccess = potentialChoices.second.get().getAssignedToSignalOfAssignment();
    const std::optional<syrec::expression::ptr>     secondOperandAsExpr           = potentialChoices.second.get().getGeneratedExpr();

    const std::optional<syrec_operation::operation> matchingAssignmentOperationForOperationNode = syrec_operation::getMatchingAssignmentOperationForOperation(operationNode->operation);
    const bool                                      isAnyChoiceBetweenOperandsPossible          = matchingAssignmentOperationForOperationNode.has_value() && (firstOperandAsVariableAccess.has_value() || secondOperandAsVariableAccess.has_value());

    DecisionReference madeDecision;
    if (previousDecisionForOperationNode.has_value()) {
        madeDecision = *previousDecisionForOperationNode;
    }
    else {
        madeDecision = std::make_shared<Decision>(Decision{operationNode->id, Decision::ChoosenOperand::None, generatedAssignmentsContainer->getNumberOfAssignments(), nullptr});
        pastDecisions.emplace_back(madeDecision);
    }

    if (!matchingAssignmentOperationForOperationNode.has_value() || !isAnyChoiceBetweenOperandsPossible || (!firstOperandAsVariableAccess.has_value() && !syrec_operation::isCommutative(operationNode->operation))) {
        didConflictPreventChoice = false;
        return madeDecision;
    }

    /*
     * After our choice for an operand is made we need to check whether the chosen operand can actually be used on the left-hand side of an assignment and whether the choice would lead to a conflict
     */
    if (firstOperandAsVariableAccess.has_value()) {
        const bool                 wasFirstOperandChosen = firstOperandAsVariableAccess.has_value();
        syrec::VariableAccess::ptr chosenOperand         = wasFirstOperandChosen ? *firstOperandAsVariableAccess : *secondOperandAsVariableAccess;
        const auto&                firstChoiceCasted     = std::dynamic_pointer_cast<syrec::VariableAccess>(chosenOperand);
        if (firstChoiceCasted && expressionTraversalHelper->canSignalBeUsedOnAssignmentLhs(firstChoiceCasted->var->name) && !doesAssignmentToSignalLeadToConflict(*firstChoiceCasted)) {
            madeDecision->choosenOperand = wasFirstOperandChosen ? Decision::ChoosenOperand::Left : Decision::ChoosenOperand::Right;
        } else if (syrec_operation::isCommutative(operationNode->operation) && (wasFirstOperandChosen ? secondOperandAsVariableAccess.has_value() : firstOperandAsVariableAccess.has_value())) {
            chosenOperand                  = wasFirstOperandChosen ? *secondOperandAsVariableAccess : *firstOperandAsVariableAccess;
            const auto& secondChoiceCasted = std::dynamic_pointer_cast<syrec::VariableAccess>(chosenOperand);
            if (secondChoiceCasted && expressionTraversalHelper->canSignalBeUsedOnAssignmentLhs(secondChoiceCasted->var->name) && !doesAssignmentToSignalLeadToConflict(*secondChoiceCasted)) {
                madeDecision->choosenOperand = wasFirstOperandChosen ? Decision::ChoosenOperand::Right : Decision::ChoosenOperand::Left;
            }
        }
    } else if (syrec_operation::isCommutative(operationNode->operation) && secondOperandAsVariableAccess.has_value()) {
        // Check whether the "second" potential choice does lead to a conflict, if the first choice was either a nested expression or not a signal access
        const auto& secondChoiceCasted = std::dynamic_pointer_cast<syrec::VariableAccess>(*secondOperandAsVariableAccess);
        if (secondChoiceCasted && expressionTraversalHelper->canSignalBeUsedOnAssignmentLhs(secondChoiceCasted->var->name) && !doesAssignmentToSignalLeadToConflict(*secondChoiceCasted)) {
            madeDecision->choosenOperand = Decision::ChoosenOperand::Right;
        }
    }
    
    didConflictPreventChoice = madeDecision->choosenOperand == Decision::ChoosenOperand::None;
    return madeDecision;
}

bool noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::couldAnotherChoiceBeMadeAtPreviousDecision() const {
    return pastDecisions.size() < 2 || std::any_of(
    std::next(pastDecisions.crbegin(), 2),
    pastDecisions.crend(),
    [&](const DecisionReference& pastDecision) {
        if (const auto& matchingAssignmentOperationForOperation = syrec_operation::getMatchingAssignmentOperationForOperation(*expressionTraversalHelper->getOperationOfOperationNode(pastDecision->operationNodeId)); matchingAssignmentOperationForOperation.has_value()) {
            return pastDecision->choosenOperand == Decision::ChoosenOperand::Left && syrec_operation::isCommutative(*expressionTraversalHelper->getOperationOfOperationNode(pastDecision->operationNodeId));
        }
        return false;
    });
}


//bool noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::redecide(const ExpressionTraversalHelper::OperationNodeReference& operationNode, const DecisionReference& previousDecisionForOperationNode) const {
//    const Decision::ChoosenOperand previouslyChosenOperand   = previousDecisionForOperationNode->choosenOperand;
//    const Decision::ChoosenOperand chosenOperandAfterRedecision = chooseBetweenPotentialOperands(operationNode, previousDecisionForOperationNode->choosenOperand);
//    previousDecisionForOperationNode->choosenOperand            = chosenOperandAfterRedecision;
//    previousDecisionForOperationNode->backupOfExpressionRequiringFixup = nullptr;
//    return chosenOperandAfterRedecision != previouslyChosenOperand;
//}

void noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::removeDecisionFor(std::size_t operationNodeId) {
    const auto& matchingDecisionForOperationNode = std::find_if(
        pastDecisions.cbegin(),
        pastDecisions.cend(),
        [&operationNodeId](const DecisionReference& decisionReference) {
            return decisionReference->operationNodeId == operationNodeId;
        });

    if (matchingDecisionForOperationNode != pastDecisions.cend()) {
        pastDecisions.erase(matchingDecisionForOperationNode);
    }
}

inline std::optional<noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::DecisionReference> noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::tryGetLastDecision() const {
    if (pastDecisions.empty()) {
        return std::nullopt;
    }
    return pastDecisions.back();
}

inline std::optional<noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::DecisionReference> noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::tryGetSecondToLastDecision() const {
    if (pastDecisions.size() < 2) {
        return std::nullopt;
    }
    return pastDecisions.at(pastDecisions.size() - 2);
}

void noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::backtrackToLastDecision() {
    if (pastDecisions.empty()) {
        return;
    }

    const auto numberOfAssignmentsPriorToDecision = pastDecisions.back()->numExistingAssignmentsPriorToDecision;
    const auto currentNumberOfAssignments         = generatedAssignmentsContainer->getNumberOfAssignments();
    generatedAssignmentsContainer->rollbackLastXAssignments(currentNumberOfAssignments - numberOfAssignmentsPriorToDecision);
    pastDecisions.pop_back();
    expressionTraversalHelper->backtrack();

}

void noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::resetInternals() {
    pastDecisions.clear();
    generatedAssignmentsContainer->resetInternals();
    expressionTraversalHelper->resetInternals();
    signalPartsBlockedFromEvaluationLookup->clear();
}

syrec::expression::ptr noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::fuseExpressions(const syrec::expression::ptr& lhsOperand, syrec_operation::operation op, const syrec::expression::ptr& rhsOperand) {
    if (syrec_operation::isOperationShiftOperation(op)) {
        const auto& rhsOperandAsNumericExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(rhsOperand);
        const auto  generatedShiftExpr      = std::make_shared<syrec::ShiftExpression>(
                lhsOperand,
                *syrec_operation::tryMapShiftOperationEnumToFlag(op),
                rhsOperandAsNumericExpr->value);
        return generatedShiftExpr;
    }
    const auto generatedBinaryExpr = std::make_shared<syrec::BinaryExpression>(lhsOperand, *syrec_operation::tryMapBinaryOperationEnumToFlag(op), rhsOperand);
    return generatedBinaryExpr;
}

std::optional<syrec::AssignStatement::ptr> noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::tryCreateAssignmentForOperationNode(const syrec::VariableAccess::ptr& assignmentLhs, syrec_operation::operation op, const syrec::expression::ptr& assignmentRhs) {
    if (const auto matchingAssignmentOperationForOperation = syrec_operation::getMatchingAssignmentOperationForOperation(op); matchingAssignmentOperationForOperation.has_value()) {
        const auto& mappedToAssignmentOperationFlag = syrec_operation::tryMapAssignmentOperationEnumToFlag(*matchingAssignmentOperationForOperation);
        const auto& generatedAssignment = std::make_shared<syrec::AssignStatement>(assignmentLhs, *mappedToAssignmentOperationFlag, assignmentRhs);
        return generatedAssignment;
    }
    return std::nullopt;
}

syrec::expression::ptr noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::createExpressionFrom(const syrec::expression::ptr& lhsOperand, syrec_operation::operation op, const syrec::expression::ptr& rhsOperand) {
    if (syrec_operation::isOperationShiftOperation(op)) {
        const auto& rhsOperandAsNumericExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(rhsOperand);
        const auto  generatedShiftExpr      = std::make_shared<syrec::ShiftExpression>(
                lhsOperand,
                *syrec_operation::tryMapShiftOperationEnumToFlag(op),
                rhsOperandAsNumericExpr->value);
        return generatedShiftExpr;
    }
    const auto generatedBinaryExpr = std::make_shared<syrec::BinaryExpression>(lhsOperand, *syrec_operation::tryMapBinaryOperationEnumToFlag(op), rhsOperand);
    return generatedBinaryExpr;
}

syrec::expression::ptr noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::createExpressionFromOperationNode(const ExpressionTraversalHelper::ptr& expressionTraversalHelper, const ExpressionTraversalHelper::OperationNodeReference& operationNode) {
    const auto& lhsOperandData = expressionTraversalHelper->getOperandAsExpr(operationNode->lhsOperand.id);
    const auto& rhsOperandData = expressionTraversalHelper->getOperandAsExpr(operationNode->rhsOperand.id);
    return createExpressionFrom(*lhsOperandData, operationNode->operation, *rhsOperandData);
}

inline syrec::expression::ptr noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::createExpressionFromOperandSimplificationResult(const OperationOperandSimplificationResult& operandSimplificationResult) {
    if (const auto& lastAssignedToSignalInOperandSimplificationResult = operandSimplificationResult.getAssignedToSignalOfAssignment(); lastAssignedToSignalInOperandSimplificationResult.has_value()) {
        return std::make_shared<syrec::VariableExpression>(*lastAssignedToSignalInOperandSimplificationResult);
    }
    return *operandSimplificationResult.getGeneratedExpr();
}

bool noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::doesExpressionDefineNestedSplitableExpr(const syrec::expression& expr) {
    // We are assuming here that compile time constant expressions were already converted to binary expressions
    return dynamic_cast<const syrec::BinaryExpression*>(&expr) != nullptr || dynamic_cast<const syrec::ShiftExpression*>(&expr) != nullptr;
}

noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::SignalPartsBlockedFromEvaluationLookupReference noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::buildLookupOfSignalsPartsBlockedFromEvaluation(const syrec::AssignStatement::vec& assignmentsDefiningSignalPartsBlockedFromEvaluation, const parser::SymbolTable& symbolTableReference) {
    SignalPartsBlockedFromEvaluationLookupReference lookupReference = std::make_unique<SignalPartsBlockedFromEvaluationLookup>();

    for (const auto& assignment : assignmentsDefiningSignalPartsBlockedFromEvaluation) {
        const auto& assignmentCasted = std::dynamic_pointer_cast<syrec::AssignStatement>(assignment);
        if (!assignmentCasted) {
            continue;
        }

        const auto& assignedToSignalIdent = assignmentCasted->lhs->var->name;
        if (!lookupReference->count(assignedToSignalIdent)) {
            std::vector<syrec::VariableAccess::ptr> assignmentsForSignalIdent(1, assignmentCasted->lhs);
            lookupReference->emplace(std::make_pair(assignedToSignalIdent, assignmentsForSignalIdent));
        }
        else {
            auto& existingLookupEntries = lookupReference->at(assignedToSignalIdent);
            const auto& existsOverlappingEntry    = !std::all_of(
                       existingLookupEntries.cbegin(),
                       existingLookupEntries.cend(),
                       [&](const syrec::VariableAccess::ptr& existingLookupEntry) {
                        const SignalAccessUtils::SignalAccessEquivalenceResult signalAccessEquivalenceResult = SignalAccessUtils::areSignalAccessesEqual(
                            *existingLookupEntry, *assignmentCasted->lhs,
                            SignalAccessUtils::SignalAccessComponentEquivalenceCriteria::DimensionAccess::Overlapping,
                            SignalAccessUtils::SignalAccessComponentEquivalenceCriteria::BitRange::Overlapping,
                            symbolTableReference
                        );
                        return signalAccessEquivalenceResult.isResultCertain && signalAccessEquivalenceResult.equality == SignalAccessUtils::SignalAccessEquivalenceResult::NotEqual;
                    });
            if (!existsOverlappingEntry) {
                existingLookupEntries.emplace_back(assignmentCasted->lhs);
            }
        }
    }
    return lookupReference;
}

std::optional<syrec::AssignStatement::ptr> noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::tryCreateAssignmentFromOperands(Decision::ChoosenOperand chosenOperandAsAssignedToSignal, const OperationOperandSimplificationResult& simplificationResultOfFirstOperand, syrec_operation::operation operationNodeOperation, const OperationOperandSimplificationResult& simplificationResultOfSecondOperand) {
    if (chosenOperandAsAssignedToSignal == Decision::ChoosenOperand::None) {
        return std::nullopt;   
    }

    const std::optional<syrec::VariableAccess::ptr> lastAssignedToSignalOfFirstOperand = simplificationResultOfFirstOperand.getAssignedToSignalOfAssignment();
    const std::optional<syrec::VariableAccess::ptr> lastAssignedToSignalOfSecondOperand = simplificationResultOfSecondOperand.getAssignedToSignalOfAssignment();

    if ((chosenOperandAsAssignedToSignal == Decision::ChoosenOperand::Left && !lastAssignedToSignalOfFirstOperand.has_value()) || (chosenOperandAsAssignedToSignal == Decision::ChoosenOperand::Right && !lastAssignedToSignalOfSecondOperand.has_value())) {
        return std::nullopt;
    }

    syrec::VariableAccess::ptr assignedToSignalOfGeneratedAssignment;
    syrec::expression::ptr     generatedAssignmentRhsExpr;
    if (chosenOperandAsAssignedToSignal == Decision::ChoosenOperand::Left) {
        assignedToSignalOfGeneratedAssignment = *lastAssignedToSignalOfFirstOperand;
        if (lastAssignedToSignalOfSecondOperand.has_value()) {
            generatedAssignmentRhsExpr = std::make_shared<syrec::VariableExpression>(*lastAssignedToSignalOfSecondOperand);
        } else {
            generatedAssignmentRhsExpr = *simplificationResultOfSecondOperand.getGeneratedExpr();
        }
    } else {
        assignedToSignalOfGeneratedAssignment = *lastAssignedToSignalOfSecondOperand;
        if (lastAssignedToSignalOfFirstOperand.has_value()) {
            generatedAssignmentRhsExpr = std::make_shared<syrec::VariableExpression>(*lastAssignedToSignalOfFirstOperand);
        } else {
            generatedAssignmentRhsExpr = *simplificationResultOfFirstOperand.getGeneratedExpr();
        }
    }

    const auto& generatedAssignment = tryCreateAssignmentForOperationNode(assignedToSignalOfGeneratedAssignment, operationNodeOperation, generatedAssignmentRhsExpr);
    return generatedAssignment;
}

std::optional<syrec::expression::ptr> noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::tryCreateExpressionFromOperationNodeOperandSimplifications(const OperationOperandSimplificationResult& simplificationResultOfFirstOperand, syrec_operation::operation operationNodeOperation, const OperationOperandSimplificationResult& simplificationResultOfSecondOperand) {
    syrec::expression::ptr generatedExprLhsOperand;
    syrec::expression::ptr generatedExprRhsOperand;
    
    if (const std::optional<syrec::VariableAccess::ptr> lastAssignedToSignalOfFirstOperand = simplificationResultOfFirstOperand.getAssignedToSignalOfAssignment(); lastAssignedToSignalOfFirstOperand.has_value()) {
        generatedExprLhsOperand = std::make_shared<syrec::VariableExpression>(*lastAssignedToSignalOfFirstOperand);
    }
    else {
        generatedExprLhsOperand = *simplificationResultOfFirstOperand.getGeneratedExpr();
    }

    if (const std::optional<syrec::VariableAccess::ptr> lastAssignedToSignalOfSecondOperand = simplificationResultOfSecondOperand.getAssignedToSignalOfAssignment(); lastAssignedToSignalOfSecondOperand.has_value()) {
        generatedExprRhsOperand = std::make_shared<syrec::VariableExpression>(*lastAssignedToSignalOfSecondOperand);
    }
    else {
        generatedExprRhsOperand = *simplificationResultOfSecondOperand.getGeneratedExpr();
    }
    return fuseExpressions(generatedExprLhsOperand, operationNodeOperation, generatedExprRhsOperand);
}


//std::vector<syrec::AssignStatement::ptr> noAdditionalLineSynthesis::AssignmentWithoutAdditionalLinesSimplifier::simplify(const syrec::AssignStatement& assignmentStmtToSimplify, const DataflowAnalysisResultLookupReference& blockedAssignmentsLookup) {
//    return {};
//}
//
//// START OF IMPLEMENTATION OF PRIVATE OR PROTECTED FUNCTIONS
//void noAdditionalLineSynthesis::AssignmentWithoutAdditionalLinesSimplifier::ExpressionSimplificationResult::fuseGeneratedExpressions(syrec_operation::operation operationOfFusedExpr) {
//    if (expressionsRequiringFixup.size() != 2) {
//        return;
//    }
//
//    const auto& lhsOperand = expressionsRequiringFixup.front();
//    const auto& rhsOperand = expressionsRequiringFixup.back();
//    expressionsRequiringFixup.clear();
//    const auto& generatedBinaryExpr = std::make_shared<syrec::BinaryExpression>(
//        lhsOperand,
//        *syrec_operation::tryMapBinaryOperationEnumToFlag(operationOfFusedExpr),
//        rhsOperand
//    );
//    expressionsRequiringFixup.emplace_back(generatedBinaryExpr);
//}
//
//void noAdditionalLineSynthesis::AssignmentWithoutAdditionalLinesSimplifier::ExpressionSimplificationResult::copySimplificationResult(const ExpressionSimplificationResult& other) {
//    expressionsRequiringFixup.insert(expressionsRequiringFixup.end(), expressionsRequiringFixup.cbegin(), expressionsRequiringFixup.cend());
//}
//
//bool noAdditionalLineSynthesis::AssignmentWithoutAdditionalLinesSimplifier::checkPrecondition(const syrec::AssignStatement& assignmentStatement) const {
//    return isOriginallyAssignedToSignalAccessNotUsedInRhs(*assignmentStatement.lhs, *assignmentStatement.rhs, *symbolTable) && existsMappingForOperationOfExpr(*assignmentStatement.rhs);
//}
//
//std::optional<bool> noAdditionalLineSynthesis::AssignmentWithoutAdditionalLinesSimplifier::chooseBetweenPotentialOperands(const ExpressionTraversalHelper::OperationNodeReference& operationNode) {
//    const auto& existingDecisionForOperationNode   = std::find_if(madeDecisions.begin(), madeDecisions.end(), [&operationNode](const Decision& decision) { return decision.operationNodeId == operationNode->id; });
//    const bool  existsPastDecisionForOperationNode = existingDecisionForOperationNode != madeDecisions.end();
//    
//    if (!syrec_operation::getMatchingAssignmentOperationForOperation(operationNode->operation)) {
//        if (existsPastDecisionForOperationNode) {
//            return std::nullopt;
//        }        
//    }
//    
//    std::optional<bool> choosenOperand;
//    if (existsPastDecisionForOperationNode) {
//        if (existingDecisionForOperationNode->chooseLhsOperand.has_value()) {
//            const bool canChooseBetweenAnyOperand = syrec_operation::isCommutative(operationNode->operation);
//            if (!canChooseBetweenAnyOperand || !*choosenOperand) {
//                return std::nullopt;
//            }
//            choosenOperand = !*choosenOperand;
//        }
//    } else {
//        choosenOperand.emplace(true);
//    }
//    return choosenOperand;
//}
//
//void noAdditionalLineSynthesis::AssignmentWithoutAdditionalLinesSimplifier::makeDecision(const ExpressionTraversalHelper::OperationNodeReference& operationNode, syrec::expression::vec& currentExpressionsRequiringFixup) {
//    const auto& existingDecisionForOperationNode   = std::find_if(madeDecisions.begin(), madeDecisions.end(), [&operationNode](const Decision& decision) { return decision.operationNodeId == operationNode->id; });
//    const bool  existsPastDecisionForOperationNode = existingDecisionForOperationNode != madeDecisions.end();
//
//    std::optional<bool> choosenOperand = chooseBetweenPotentialOperands(operationNode);
//}
//
//
//void noAdditionalLineSynthesis::AssignmentWithoutAdditionalLinesSimplifier::rollbackToLastDecision() {
//    if (madeDecisions.empty()) {
//        return;
//    }
//
//    const auto& lastMadeDecision = madeDecisions.back();
//    const auto  existingAssignmentsAtLastMadeDecision = lastMadeDecision.numberOfExistingAssignments;
//    generatedAssignments.erase(std::next(generatedAssignments.begin(), existingAssignmentsAtLastMadeDecision), generatedAssignments.end());
//    expressionTraversalHelper->tryBacktrackToLastCheckpoint();
//}
//
//
//std::optional<unsigned int> noAdditionalLineSynthesis::AssignmentWithoutAdditionalLinesSimplifier::determineValueOfAssignedToSignal(const syrec::VariableAccess& assignedToSignal) const {
//    bool isValueLookupBlocked = false;
//    if (valueLookupsBlockedByDataflowAnalysis->count(assignedToSignal.var->name)) {
//        const auto& assignmentsDeterminedByDataflowAnalysis = valueLookupsBlockedByDataflowAnalysis->at(assignedToSignal.var->name);
//        isValueLookupBlocked                                = std::any_of(
//                                               assignmentsDeterminedByDataflowAnalysis.cbegin(),
//                                               assignmentsDeterminedByDataflowAnalysis.cend(),
//                                               [&, assignedToSignal](const syrec::VariableAccess::ptr& valueLookupBlockedByDataflowAnalysis) {
//                                                         const auto& signalAccessEquivalenceResult = SignalAccessUtils::areSignalAccessesEqual(
//                                                           assignedToSignal,
//                                                           *valueLookupBlockedByDataflowAnalysis,
//                                                           SignalAccessUtils::SignalAccessComponentEquivalenceCriteria::DimensionAccess::Overlapping,
//                                                           SignalAccessUtils::SignalAccessComponentEquivalenceCriteria::BitRange::Overlapping,
//                                                           *symbolTable);
//                    return !(signalAccessEquivalenceResult.isResultCertain && signalAccessEquivalenceResult.equality == SignalAccessUtils::SignalAccessEquivalenceResult::NotEqual);
//                });
//    }
//
//    if (!isValueLookupBlocked && locallyBlockedSignals->count(assignedToSignal.var->name)) {
//        const auto& locallyBlockedSignalsMatchingAssignedToSignalIdent = locallyBlockedSignals->at(assignedToSignal.var->name);
//        isValueLookupBlocked = std::any_of(
//                locallyBlockedSignalsMatchingAssignedToSignalIdent.cbegin(),
//                locallyBlockedSignalsMatchingAssignedToSignalIdent.cend(),
//                [&, assignedToSignal](const syrec::VariableAccess::ptr& locallyBlockedAssignedToSignal) {
//                    const auto& signalAccessEquivalenceResult = SignalAccessUtils::areSignalAccessesEqual(
//                        assignedToSignal,
//                        *locallyBlockedAssignedToSignal,
//                        SignalAccessUtils::SignalAccessComponentEquivalenceCriteria::DimensionAccess::Overlapping,
//                        SignalAccessUtils::SignalAccessComponentEquivalenceCriteria::BitRange::Overlapping, 
//                        *symbolTable);
//                    return !(signalAccessEquivalenceResult.isResultCertain && signalAccessEquivalenceResult.equality == SignalAccessUtils::SignalAccessEquivalenceResult::NotEqual);
//                });
//    }
//
//    if (!isValueLookupBlocked) {
//        const auto& temporaryContainerForSignalAccess = std::make_shared<syrec::VariableAccess>(assignedToSignal);
//        return symbolTable->tryFetchValueForLiteral(temporaryContainerForSignalAccess);
//    }
//
//    return std::nullopt;
//}
//
//// TODO: If support for unary expression is added in the SyReC framework, also add support for it here
//bool noAdditionalLineSynthesis::AssignmentWithoutAdditionalLinesSimplifier::isOriginallyAssignedToSignalAccessNotUsedInRhs(const syrec::VariableAccess& originalAssignedToSignal, const syrec::expression& originalAssignmentRhs, const parser::SymbolTable& symbolTable) {
//    if (const auto& exprAsShiftExpr = dynamic_cast<const syrec::ShiftExpression*>(&originalAssignmentRhs); exprAsShiftExpr) {
//        return isOriginallyAssignedToSignalAccessNotUsedInRhs(originalAssignedToSignal, *exprAsShiftExpr->lhs, symbolTable);
//    }
//    if (const auto& exprAsBinaryExpr = dynamic_cast<const syrec::BinaryExpression*>(&originalAssignmentRhs); exprAsBinaryExpr) {
//        return isOriginallyAssignedToSignalAccessNotUsedInRhs(originalAssignedToSignal, *exprAsBinaryExpr->lhs, symbolTable) && isOriginallyAssignedToSignalAccessNotUsedInRhs(originalAssignedToSignal, *exprAsBinaryExpr->rhs, symbolTable);
//    }
//    if (const auto& exprAsVariableExpr = dynamic_cast<const syrec::VariableExpression*>(&originalAssignmentRhs); exprAsVariableExpr) {
//        const auto& signalAccessEquivalenceResult = SignalAccessUtils::areSignalAccessesEqual(originalAssignedToSignal, *exprAsVariableExpr->var, SignalAccessUtils::SignalAccessComponentEquivalenceCriteria::DimensionAccess::Overlapping,
//                                                         SignalAccessUtils::SignalAccessComponentEquivalenceCriteria::BitRange::Overlapping, symbolTable);
//        return signalAccessEquivalenceResult.isResultCertain && signalAccessEquivalenceResult.equality == SignalAccessUtils::SignalAccessEquivalenceResult::NotEqual;
//    }
//    if (const auto& exprAsNumericExpr = dynamic_cast<const syrec::NumericExpression*>(&originalAssignmentRhs); exprAsNumericExpr) {
//        return true;
//    }
//    return false;
//}
//
//syrec::expression::ptr noAdditionalLineSynthesis::AssignmentWithoutAdditionalLinesSimplifier::createExpressionFor(const syrec::expression::ptr& lhsOperand, syrec_operation::operation operation, const syrec::expression::ptr& rhsOperand) {
//    const auto& generatedExpression = std::make_shared<syrec::BinaryExpression>(lhsOperand, *syrec_operation::tryMapBinaryOperationEnumToFlag(operation), rhsOperand);
//    return generatedExpression;
//}
//
//bool noAdditionalLineSynthesis::AssignmentWithoutAdditionalLinesSimplifier::existsMappingForOperationOfExpr(const syrec::expression& expr) {
//    if (const auto& exprAsShiftExpr = dynamic_cast<const syrec::ShiftExpression*>(&expr); exprAsShiftExpr) {
//        return syrec_operation::tryMapShiftOperationFlagToEnum(exprAsShiftExpr->op).has_value();
//    }
//    if (const auto& exprAsBinaryExpr = dynamic_cast<const syrec::BinaryExpression*>(&expr); exprAsBinaryExpr) {
//        return syrec_operation::tryMapBinaryOperationFlagToEnum(exprAsBinaryExpr->op).has_value();
//    }
//    return true;
//}
//
//void noAdditionalLineSynthesis::AssignmentWithoutAdditionalLinesSimplifier::resetInternals() {
//    valueLookupsBlockedByDataflowAnalysis->clear();
//    locallyBlockedSignals->clear();
//    expressionTraversalHelper.reset();
//    madeDecisions.clear();
//    generatedAssignments.clear();
//}