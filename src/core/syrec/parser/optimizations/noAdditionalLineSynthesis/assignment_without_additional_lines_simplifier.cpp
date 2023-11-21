#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/assignment_without_additional_lines_simplifier.hpp"

#include "core/syrec/parser/utils/signal_access_utils.hpp"

/*
 * TODO: Is we are parsing an operation node with one leaf node, we could "carry" over the leaf node as a potential assignment candidate to the processing of the non-leaf-node
 * TODO: Switch operands for expr (a - (b - c)) to (a + (c - b))
 *
 * TODO: Infinite loop in simplifyWithOnlyReversibleOpsButNonUniqueSignalAccessWithAssignOperationBeingAdditionWithDegeneratedLhsSubASTOfLhsOfAssignmentExpr
 */
noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::SimplificationResultReference noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::simplify(const syrec::AssignStatement& assignmentStatement, const syrec::AssignStatement::vec& assignmentsDefiningSignalPartsBlockedFromEvaluation) {
    resetInternals();
    if (!doesExpressionDefineNestedSplitableExpr(*assignmentStatement.rhs)) {
        SimplificationResultReference simplificationResult = std::make_unique<SimplificationResult>(SimplificationResult{generatedAssignmentsContainer->getAssignments()});
        return simplificationResult;
    }

    // TODO: Only for debugging uncomment line below in final version
    auto transformedAssignmentStmt = std::make_unique<syrec::AssignStatement>(assignmentStatement);
    //auto transformedAssignmentStmt = transformAssignmentPriorToSimplification(assignmentStatement);
    if (!transformedAssignmentStmt) {
        SimplificationResultReference simplificationResult = std::make_unique<SimplificationResult>(SimplificationResult{generatedAssignmentsContainer->getAssignments()});
        return simplificationResult;
    }

    signalPartsBlockedFromEvaluationLookup = buildLookupOfSignalsPartsBlockedFromEvaluation(assignmentsDefiningSignalPartsBlockedFromEvaluation, *symbolTableReference);
    expressionTraversalHelper->buildTraversalQueue(transformedAssignmentStmt->rhs, *symbolTableReference);

    std::optional<ExpressionTraversalHelper::OperationNodeReference> topMostOperationNode     = expressionTraversalHelper->getNextOperationNode();
    while (topMostOperationNode.has_value()) {
        // TODO: If an expression is created for the rhs we can still try to split it if a binary operation with an assignment counterpart exists
        auto simplificationResultOfTopmostOperationNode = handleOperationNode(*topMostOperationNode);
        if (simplificationResultOfTopmostOperationNode.has_value()) {
            const auto& generatedExprForTopmostOperationRhsExpr = simplificationResultOfTopmostOperationNode->get()->getGeneratedExpr();
            const auto& generatedLastAssignedToSignalOfRhsExpr  = simplificationResultOfTopmostOperationNode->get()->getAssignedToSignalOfAssignment();

            syrec::VariableAccess::ptr generatedAssignmentAssignedToSignal = transformedAssignmentStmt->lhs;
            syrec::expression::ptr generatedAssignmentRhsExpr;
            if (generatedLastAssignedToSignalOfRhsExpr.has_value()) {
                generatedAssignmentRhsExpr = std::make_shared<syrec::VariableExpression>(*generatedLastAssignedToSignalOfRhsExpr);
            }
            else {
                generatedAssignmentRhsExpr = *generatedExprForTopmostOperationRhsExpr;
            }

            if (const auto& assignmentsGeneratedBySplitOfTopmostExpr = trySplitAssignmentRhs(generatedAssignmentAssignedToSignal, *syrec_operation::tryMapAssignmentOperationFlagToEnum(transformedAssignmentStmt->op), generatedAssignmentRhsExpr); assignmentsGeneratedBySplitOfTopmostExpr.has_value()) {
                generatedAssignmentsContainer->storeActiveAssignment(assignmentsGeneratedBySplitOfTopmostExpr->first);
                generatedAssignmentsContainer->storeActiveAssignment(assignmentsGeneratedBySplitOfTopmostExpr->second);
                generatedAssignmentsContainer->invertAllAssignmentsUpToLastCutoff(2);   
            }
            else {
                const syrec::AssignStatement::ptr generatedAssignment = std::make_shared<syrec::AssignStatement>(
                        generatedAssignmentAssignedToSignal,
                        transformedAssignmentStmt->op,
                        generatedAssignmentRhsExpr);
                generatedAssignmentsContainer->storeActiveAssignment(generatedAssignment);
                // TODO: Is this call necessary ?
                generatedAssignmentsContainer->invertAllAssignmentsUpToLastCutoff(1);                
            }
        }
        topMostOperationNode = expressionTraversalHelper->getNextOperationNode();
    }

    syrec::AssignStatement::vec generatedAssignments;
    if (!generatedAssignmentsContainer->getNumberOfAssignments()) {
        if (const std::shared_ptr<syrec::AssignStatement>& copyOfUserProvidedAssignment = std::move(transformedAssignmentStmt); copyOfUserProvidedAssignment) {
            generatedAssignments.emplace_back(copyOfUserProvidedAssignment);   
        } else if (const std::shared_ptr<syrec::AssignStatement>& copyOfUserProvidedAssignment = std::make_shared<syrec::AssignStatement>(assignmentStatement); copyOfUserProvidedAssignment) {
            generatedAssignments.emplace_back(copyOfUserProvidedAssignment);   
        }
    } else {
        generatedAssignments = generatedAssignmentsContainer->getAssignments();
    }
    return std::make_unique<SimplificationResult>(SimplificationResult{generatedAssignments});
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
        if (couldAnotherChoiceBeMadeAtPreviousDecision(operationNode->id)) {
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
        if (couldAnotherChoiceBeMadeAtPreviousDecision(operationNode->id)) {
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
        generatedAssignmentsContainer->invertAllAssignmentsUpToLastCutoff(1);
        simplificationResultData = std::dynamic_pointer_cast<syrec::AssignStatement>(*generatedAssignment)->lhs;
    }
    else {
        shouldBacktrackDueToConflict = !couldAnotherChoiceBeMadeAtPreviousDecision(operationNode->id);
        if (!shouldBacktrackDueToConflict) {
            const std::optional<syrec::expression::ptr> generatedExpr = tryCreateExpressionFromOperationNodeOperandSimplifications(**firstOperandSimplificationResult, operationNode->operation, **secondOperandSimplificationResult);
            simplificationResultData = *generatedExpr;
        }
    }

    // We should probably not remove any past decisions since this would clash with the check whether a different choice could be made at a previous decision
    //removeDecisionFor(operationNode->id);
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

std::optional<noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::OwningOperationOperandSimplificationResultReference> noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::handleOperationNodeWithOneLeafNode(const ExpressionTraversalHelper::OperationNodeReference& operationNode) {
    expressionTraversalHelper->markOperationNodeAsPotentialBacktrackOption(operationNode->id);
    generatedAssignmentsContainer->markCutoffForInvertibleAssignments();
    const std::size_t numExistingAssignmentsPriorToHandlingOfOperationNode = generatedAssignmentsContainer->getNumberOfAssignments();

    const auto&                                                              dataOfOperationNodeOperand                 = *expressionTraversalHelper->getNextOperationNode();
    if (dataOfOperationNodeOperand->id == 5) {
        int x = 0;        
    }
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
        if (!couldAnotherChoiceBeMadeAtPreviousDecision(operationNode->id)) {
            const std::size_t operandIdOfNonLeafNode   = wasLhsOperandLeafNode ? operationNode->rhsOperand.id : operationNode->lhsOperand.id;
            simplificationResultOfOperationNodeOperand = std::make_unique<OperationOperandSimplificationResult>(0, *expressionTraversalHelper->getOperandAsExpr(operandIdOfNonLeafNode));   
        }
    }
    
    expressionTraversalHelper->removeOperationNodeAsPotentialBacktrackOperation(operationNode->id);
    if (!didConflictPreventChoice && madeDecision && madeDecision->choosenOperand != Decision::ChoosenOperand::None) {
        const std::optional<syrec::AssignStatement::ptr> generatedAssignment = wasLhsOperandLeafNode
            ? tryCreateAssignmentFromOperands(madeDecision->choosenOperand, simplificationResultOfLeafNode, operationNode->operation, **simplificationResultOfOperationNodeOperand)
            : tryCreateAssignmentFromOperands(madeDecision->choosenOperand, **simplificationResultOfOperationNodeOperand, operationNode->operation, simplificationResultOfLeafNode);

        generatedAssignmentsContainer->storeActiveAssignment(*generatedAssignment);
        generatedAssignmentsContainer->invertAllAssignmentsUpToLastCutoff(1);
        simplificationResultData = std::dynamic_pointer_cast<syrec::AssignStatement>(*generatedAssignment)->lhs;
    } else {
        if (couldAnotherChoiceBeMadeAtPreviousDecision(operationNode->id)) {
            generatedAssignmentsContainer->rollbackAssignmentsMadeSinceLastCutoffAndPopCutoff();
            expressionTraversalHelper->backtrack();
            return std::nullopt;
        }

        const std::optional<syrec::expression::ptr> generatedExpr = wasLhsOperandLeafNode
            ? tryCreateExpressionFromOperationNodeOperandSimplifications(simplificationResultOfLeafNode, operationNode->operation, **simplificationResultOfOperationNodeOperand)
            : tryCreateExpressionFromOperationNodeOperandSimplifications(**simplificationResultOfOperationNodeOperand, operationNode->operation, simplificationResultOfLeafNode);

        simplificationResultData = *generatedExpr;
        generatedAssignmentsContainer->invertAllAssignmentsUpToLastCutoff(0);   
    }
    generatedAssignmentsContainer->popLastCutoffForInvertibleAssignments();

    const std::size_t numExistingAssignmentsAfterHandlingOfOperationNode = generatedAssignmentsContainer->getNumberOfAssignments();
    const std::size_t numGeneratedAssignmentsByHandlingOfOperationNode   = numExistingAssignmentsAfterHandlingOfOperationNode - numExistingAssignmentsPriorToHandlingOfOperationNode;
    auto simplificationResult                                            = std::make_unique<OperationOperandSimplificationResult>(numGeneratedAssignmentsByHandlingOfOperationNode, simplificationResultData);
    return simplificationResult;
}

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
        shouldBacktrackDueToConflict = !couldAnotherChoiceBeMadeAtPreviousDecision(operationNode->id);
        if (!shouldBacktrackDueToConflict) {
            const std::optional<syrec::expression::ptr> generatedExpr = tryCreateExpressionFromOperationNodeOperandSimplifications(simplificationResultOfFirstOperand, operationNode->operation, simplificationResultOfSecondOperand);
            simplificationResultData                                  = *generatedExpr;
        }
    }

    // We should probably not remove any past decisions since this would clash with the check whether a different choice could be made at a previous decision
    //removeDecisionFor(operationNode->id);
    if (shouldBacktrackDueToConflict) {
        expressionTraversalHelper->backtrack();
        return std::nullopt;
    }
    return std::make_unique<OperationOperandSimplificationResult>(numGeneratedAssignmentsByHandlingOfOperationNode, simplificationResultData);
}

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

    if (!matchingAssignmentOperationForOperationNode.has_value() || !isAnyChoiceBetweenOperandsPossible) {
        didConflictPreventChoice = false;
        madeDecision->choosenOperand = Decision::ChoosenOperand::None;
        return madeDecision;
    }

    /*
     * Our decision follows the pattern:
     * I.   Try to select one of the operands as a potential choice with a higher preference for the left operand as a first choice.
     * II.  Check whether the decision can actually be used on the left-hand side of an assignment and does not lead to a conflict
     * III. If our first choice failed, try to repeat the steps but choose the right operand if possible (i.e. the operation is commutative).
     * IV.  If no operand could be selected as a candidate, the whole expression will be selected as our final decision.
     */
    if (firstOperandAsVariableAccess.has_value()) {
        const auto&                firstChoiceCasted     = std::dynamic_pointer_cast<syrec::VariableAccess>(*firstOperandAsVariableAccess);
        if (firstChoiceCasted && expressionTraversalHelper->canSignalBeUsedOnAssignmentLhs(firstChoiceCasted->var->name)) {
            if (!doesAssignmentToSignalLeadToConflict(*firstChoiceCasted)) {
                madeDecision->choosenOperand = Decision::ChoosenOperand::Left;   
            } else {
                didConflictPreventChoice = true;
            }
        }
    }
    if (madeDecision->choosenOperand == Decision::ChoosenOperand::None && syrec_operation::isCommutative(operationNode->operation) && secondOperandAsVariableAccess.has_value()) {
        // Check whether the "second" potential choice does lead to a conflict, if the first choice was either a nested expression or not a signal access
        const auto& secondChoiceCasted = std::dynamic_pointer_cast<syrec::VariableAccess>(*secondOperandAsVariableAccess);
        if (secondChoiceCasted && expressionTraversalHelper->canSignalBeUsedOnAssignmentLhs(secondChoiceCasted->var->name)) {
            if (!doesAssignmentToSignalLeadToConflict(*secondChoiceCasted)) {
                madeDecision->choosenOperand = Decision::ChoosenOperand::Right;
            } else {
                didConflictPreventChoice = true;
            }
        }
    }
    return madeDecision;
}

bool noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::couldAnotherChoiceBeMadeAtPreviousDecision(const std::optional<std::size_t>& pastDecisionForOperationNodeWithIdToExclude) const {
    if (pastDecisions.empty()) {
        return false;
    }

    std::size_t lastDecisionOffset = 0;
    if (pastDecisionForOperationNodeWithIdToExclude.has_value() && *pastDecisionForOperationNodeWithIdToExclude == pastDecisions.back()->operationNodeId) {
        ++lastDecisionOffset;
    }

    if (lastDecisionOffset >= pastDecisions.size()) {
        return false;
    }

    return std::any_of(
        std::next(pastDecisions.crbegin(), lastDecisionOffset + 1),
        pastDecisions.crend(),
        [&](const DecisionReference& pastDecision) {
                if (const auto& matchingAssignmentOperationForOperation = syrec_operation::getMatchingAssignmentOperationForOperation(*expressionTraversalHelper->getOperationOfOperationNode(pastDecision->operationNodeId)); matchingAssignmentOperationForOperation.has_value()) {
                    return pastDecision->choosenOperand == Decision::ChoosenOperand::Left && syrec_operation::isCommutative(*expressionTraversalHelper->getOperationOfOperationNode(pastDecision->operationNodeId));
                }
                return false;
            });
}

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

std::unique_ptr<syrec::AssignStatement> noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::transformAssignmentPriorToSimplification(const syrec::AssignStatement& assignmentToSimplify) const {
    if (std::unique_ptr<syrec::AssignStatement> owningCopyOfAssignmentStmt = std::make_unique<syrec::AssignStatement>(assignmentToSimplify); owningCopyOfAssignmentStmt) {
        transformExpressionPriorToSimplification(*owningCopyOfAssignmentStmt->rhs);
        return owningCopyOfAssignmentStmt;
    }
    return nullptr;
}

void noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::transformExpressionPriorToSimplification(syrec::expression& expr) const {
    if (auto* exprAsBinaryExpr = dynamic_cast<syrec::BinaryExpression*>(&expr); exprAsBinaryExpr) {
        transformExpressionPriorToSimplification(*exprAsBinaryExpr->lhs);

        const bool doesLhsOperandDefineNestedEpxr = doesExpressionDefineNestedSplitableExpr(*exprAsBinaryExpr->lhs);
        const bool doesRhsOperandDefineNestedExpr  = doesExpressionDefineNestedSplitableExpr(*exprAsBinaryExpr->rhs);
        const std::optional<syrec_operation::operation> mappedToOperationOfParentExpr  = syrec_operation::tryMapBinaryOperationFlagToEnum(exprAsBinaryExpr->op);
        if (const std::shared_ptr<syrec::BinaryExpression> rhsExprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(exprAsBinaryExpr->rhs); rhsExprAsBinaryExpr) {
            const std::optional<syrec_operation::operation> mappedToOperationOfRhsExpr    = syrec_operation::tryMapBinaryOperationFlagToEnum(rhsExprAsBinaryExpr->op);

            /*
             * Try to transform an expression of the form (a - (b - c)) to (a + (c - b)) only if b is not a signal access. The latter condition is heuristic as the transformation could prevent the creation of an assignment
             * of the form b -= c.
             */
            if (!doesLhsOperandDefineNestedEpxr || doesRhsOperandDefineNestedExpr) {
                if (mappedToOperationOfParentExpr.has_value() && mappedToOperationOfRhsExpr.has_value() && *mappedToOperationOfRhsExpr == syrec_operation::operation::Subtraction && *mappedToOperationOfParentExpr == syrec_operation::operation::Subtraction) {
                    exprAsBinaryExpr->op                     = *syrec_operation::tryMapBinaryOperationEnumToFlag(syrec_operation::operation::Addition);
                    const auto backupOfNestedExprLhsOperand = rhsExprAsBinaryExpr->lhs;
                    rhsExprAsBinaryExpr->lhs                 = rhsExprAsBinaryExpr->rhs;
                    rhsExprAsBinaryExpr->rhs                 = backupOfNestedExprLhsOperand;
                }   
            }
        }
        else if (doesRhsOperandDefineNestedExpr && !doesLhsOperandDefineNestedEpxr && mappedToOperationOfParentExpr.has_value() && syrec_operation::isCommutative(*mappedToOperationOfParentExpr)) {
            const auto backupOfBinaryExprLhsOperand = exprAsBinaryExpr->lhs;
            exprAsBinaryExpr->rhs                    = exprAsBinaryExpr->lhs;
            exprAsBinaryExpr->lhs                    = backupOfBinaryExprLhsOperand;
        }
        transformExpressionPriorToSimplification(*exprAsBinaryExpr->rhs);
    } else if (const auto& exprAsShiftExpr = dynamic_cast<const syrec::ShiftExpression*>(&expr); exprAsShiftExpr) {
        transformExpressionPriorToSimplification(*exprAsShiftExpr->lhs);
        // TODO: Simplification of number
    }
}

std::optional<std::pair<syrec::AssignStatement::ptr, syrec::AssignStatement::ptr>> noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::trySplitAssignmentRhs(const syrec::VariableAccess::ptr& assignedToSignal, syrec_operation::operation assignmentOperation, const syrec::expression::ptr& assignmentRhsExpr) const {
    const auto& exprAsBinaryExpr              = std::dynamic_pointer_cast<syrec::BinaryExpression>(assignmentRhsExpr);
    const auto& mappedToBinaryOperationOfExpr = exprAsBinaryExpr ? syrec_operation::tryMapBinaryOperationFlagToEnum(exprAsBinaryExpr->op) : std::nullopt;
    if (!exprAsBinaryExpr || !mappedToBinaryOperationOfExpr.has_value() || !syrec_operation::isOperationAssignmentOperation(assignmentOperation)) {
        return std::nullopt;
    }

    std::optional<std::pair<syrec_operation::operation, syrec_operation::operation>>   assignmentOperationPerAssignment;
    std::optional<std::pair<syrec::AssignStatement::ptr, syrec::AssignStatement::ptr>> generatedAssignments;

    constexpr unsigned int test = combineAssignmentAndBinaryOperation(syrec_operation::operation::AddAssign, syrec_operation::operation::Addition);

    switch (AssignmentWithoutAdditionalLineSimplifier::combineAssignmentAndBinaryOperation(assignmentOperation, *mappedToBinaryOperationOfExpr)) {
        //case AssignmentWithoutAdditionalLineSimplifier::combineAssignmentAndBinaryOperation(syrec_operation::operation::AddAssign, syrec_operation::operation::Addition):
        case test:
            assignmentOperationPerAssignment = std::make_pair(syrec_operation::operation::AddAssign, syrec_operation::operation::AddAssign);
            break;
        case AssignmentWithoutAdditionalLineSimplifier::combineAssignmentAndBinaryOperation(syrec_operation::operation::AddAssign, syrec_operation::operation::Subtraction):
            assignmentOperationPerAssignment = std::make_pair(syrec_operation::operation::AddAssign, syrec_operation::operation::MinusAssign);
            break;
        case AssignmentWithoutAdditionalLineSimplifier::combineAssignmentAndBinaryOperation(syrec_operation::operation::AddAssign, syrec_operation::operation::BitwiseXor):
            if (const auto& fetchedValueForAssignedToSignal = tryFetchValueOfSignal(*assignedToSignal); fetchedValueForAssignedToSignal.has_value() && !*fetchedValueForAssignedToSignal) {
                assignmentOperationPerAssignment = std::make_pair(syrec_operation::operation::XorAssign, syrec_operation::operation::XorAssign);
            }
            break;
        case AssignmentWithoutAdditionalLineSimplifier::combineAssignmentAndBinaryOperation(syrec_operation::operation::MinusAssign, syrec_operation::operation::Addition):
            assignmentOperationPerAssignment = std::make_pair(syrec_operation::operation::MinusAssign, syrec_operation::operation::MinusAssign);
            break;
        case AssignmentWithoutAdditionalLineSimplifier::combineAssignmentAndBinaryOperation(syrec_operation::operation::MinusAssign, syrec_operation::operation::Subtraction):
            assignmentOperationPerAssignment = std::make_pair(syrec_operation::operation::MinusAssign, syrec_operation::operation::AddAssign);
            break;
        case AssignmentWithoutAdditionalLineSimplifier::combineAssignmentAndBinaryOperation(syrec_operation::operation::XorAssign, syrec_operation::operation::BitwiseXor):
            assignmentOperationPerAssignment = std::make_pair(syrec_operation::operation::XorAssign, syrec_operation::operation::XorAssign);
            break;
        case AssignmentWithoutAdditionalLineSimplifier::combineAssignmentAndBinaryOperation(syrec_operation::operation::XorAssign, syrec_operation::operation::Addition):
            if (const auto& fetchedValueForAssignedToSignal = tryFetchValueOfSignal(*assignedToSignal); fetchedValueForAssignedToSignal.has_value() && !*fetchedValueForAssignedToSignal) {
                assignmentOperationPerAssignment = std::make_pair(syrec_operation::operation::AddAssign, syrec_operation::operation::AddAssign);
            }
            break;
        default:
            break;
    }

    if (assignmentOperationPerAssignment.has_value()) {
        const auto& firstGeneratedAssignment  = std::make_shared<syrec::AssignStatement>(assignedToSignal, *syrec_operation::tryMapAssignmentOperationEnumToFlag(assignmentOperationPerAssignment->first), exprAsBinaryExpr->lhs);
        const auto& secondGeneratedAssignment = std::make_shared<syrec::AssignStatement>(assignedToSignal, *syrec_operation::tryMapAssignmentOperationEnumToFlag(assignmentOperationPerAssignment->second), exprAsBinaryExpr->rhs);
        if (firstGeneratedAssignment && secondGeneratedAssignment) {
            return std::make_pair(firstGeneratedAssignment, secondGeneratedAssignment);
        }
    }
    return std::nullopt;
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