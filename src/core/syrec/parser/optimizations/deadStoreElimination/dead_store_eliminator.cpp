#include "core/syrec/parser/optimizations/deadStoreElimination/statement_iteration_helper.hpp"
#include "core/syrec/parser/optimizations/deadStoreElimination/dead_store_eliminator.hpp"

#include "core/syrec/parser/operation.hpp"
#include "core/syrec/parser/utils/loop_range_utils.hpp"
#include "core/syrec/parser/utils/signal_access_utils.hpp"

using namespace deadStoreElimination;

void DeadStoreEliminator::removeDeadStoresFrom(syrec::Statement::vec& statementList) {
    const auto& foundDeadStores = findDeadStores(statementList);
    if (foundDeadStores.empty()) {
        return;
    }

    std::size_t deadStoreIndex = 0;
    removeDeadStoresFrom(statementList, foundDeadStores, deadStoreIndex, 0);
}

  [[nodiscard]] std::optional<DeadStoreEliminator::InternalAssignmentData::ptr> DeadStoreEliminator::getOrCreateEntryInInternalLookupForAssignment(const std::variant<std::shared_ptr<syrec::AssignStatement>, std::shared_ptr<syrec::UnaryStatement>>& referencedAssignment, const AssignmentStatementIndexInControlFlowGraph& indexOfAssignmentInControlFlowGraph) {
    const InternalAssignmentData::ptr entry = std::make_shared<InternalAssignmentData>(referencedAssignment, indexOfAssignmentInControlFlowGraph);
    if (!entry) {
        return std::nullopt;
    }

    insertNewEntryIntoInternalLookup(*entry->getAssignedToSignalPartsIdent(), entry);
    insertEntryIntoGraveyard(*entry->getAssignedToSignalPartsIdent(), entry);
    return entry;
}

[[nodiscard]] std::optional<DeadStoreEliminator::InternalAssignmentData::ptr> DeadStoreEliminator::getOrCreateEntryInInternalLookupForSwapStatement(const InternalAssignmentData::SwapOperands& swapOperands, const AssignmentStatementIndexInControlFlowGraph& indexOfSwapStatementInControlFlowGraph) {
    const InternalAssignmentData::ptr entry = std::make_shared<InternalAssignmentData>(swapOperands, indexOfSwapStatementInControlFlowGraph);
    if (!entry) {
        return std::nullopt;
    }

    insertNewEntryIntoInternalLookup(swapOperands.lhsOperand->var->name, entry);
    insertNewEntryIntoInternalLookup(swapOperands.rhsOperand->var->name, entry);

    insertEntryIntoGraveyard(swapOperands.lhsOperand->var->name, entry);
    insertEntryIntoGraveyard(swapOperands.rhsOperand->var->name, entry);
    return entry;
}

std::optional<DeadStoreEliminator::InternalAssignmentData::ptr> DeadStoreEliminator::getEntryByIndexInControlFlowGraph(const std::string_view& assignedToSignalPartsIdent, const AssignmentStatementIndexInControlFlowGraph& indexInControlFlowGraph) const {
    if (!internalAssignmentData.count(assignedToSignalPartsIdent)) {
        return std::nullopt;
    }

    if (const auto& matchingEntriesForAssignedToSignalPartsIdent = internalAssignmentData.find(assignedToSignalPartsIdent); matchingEntriesForAssignedToSignalPartsIdent != internalAssignmentData.cend()) {
        if (const auto& foundMatchingEntryForIndex = std::find_if(
                    matchingEntriesForAssignedToSignalPartsIdent->second.cbegin(),
                    matchingEntriesForAssignedToSignalPartsIdent->second.cend(),
                    [&indexInControlFlowGraph](const InternalAssignmentData::ptr& existingEntry) {
                        return existingEntry->indexInControlFlowGraph == indexInControlFlowGraph;
                    });
            foundMatchingEntryForIndex != matchingEntriesForAssignedToSignalPartsIdent->second.cend()) {
            return std::make_optional(*foundMatchingEntryForIndex);
        }
    }
    return std::nullopt;
}


void DeadStoreEliminator::createGraveyardEntryForSkipStatement(const AssignmentStatementIndexInControlFlowGraph& indexOfSkipStatementInControlFlowGraph) {
    const InternalAssignmentData::ptr entry = std::make_shared<InternalAssignmentData>(indexOfSkipStatementInControlFlowGraph);
    if (!entry) {
        return;
    }
    insertEntryIntoGraveyard(InternalAssignmentData::NONE_ASSIGNMENT_STATEMENT_IDENT_PLACEHOLDER, entry);
}

void DeadStoreEliminator::insertNewEntryIntoInternalLookup(const std::string_view& assignedToSignalIdent, const InternalAssignmentData::ptr& entry) {
    if (!internalAssignmentData.count(assignedToSignalIdent)) {
        std::unordered_set<InternalAssignmentData::ptr> containerForAssignmentsToSignalWithSameIdent;
        containerForAssignmentsToSignalWithSameIdent.insert(entry);
        internalAssignmentData.insert(std::make_pair(assignedToSignalIdent, containerForAssignmentsToSignalWithSameIdent));
    } else {
        internalAssignmentData.find(assignedToSignalIdent)->second.insert(entry);
    }
}

void DeadStoreEliminator::insertEntryIntoGraveyard(const std::string_view& assignedToSignalIdent, const InternalAssignmentData::ptr& entry) {
    if (!graveyard.count(assignedToSignalIdent)) {
        std::unordered_set<InternalAssignmentData::ptr> containerForAssignmentsToSignalWithSameIdent;
        containerForAssignmentsToSignalWithSameIdent.insert(entry);
        graveyard.insert(std::make_pair(assignedToSignalIdent, containerForAssignmentsToSignalWithSameIdent));
    } else {
        graveyard.find(assignedToSignalIdent)->second.insert(entry);
    }
}

void DeadStoreEliminator::removeEntryFromGraveyard(const std::string_view& assignedToSignalIdent, const InternalAssignmentData::ptr& matchingAssignmentEntry) {
    if (!graveyard.count(assignedToSignalIdent)) {
        return;
    }
    graveyard.find(assignedToSignalIdent)->second.erase(matchingAssignmentEntry);
}

std::vector<DeadStoreEliminator::AssignmentStatementIndexInControlFlowGraph> DeadStoreEliminator::findDeadStores(const syrec::Statement::vec& statementList) {
    resetInternalData();
    const std::unique_ptr<StatementIterationHelper> statementIterationHelper = std::make_unique<StatementIterationHelper>(statementList);

    std::optional<StatementIterationHelper::StatementAndRelativeIndexPair> nextStatement = statementIterationHelper->getNextStatement();
    while (nextStatement.has_value()) {
        const auto& indexOfCurrentStmtInControlFlowGraph = AssignmentStatementIndexInControlFlowGraph(nextStatement->relativeIndexInControlFlowGraph);
        updateLivenessStatusScopeAccordingToNestingLevelOfStatement(indexOfLastProcessedStatementInControlFlowGraph, indexOfCurrentStmtInControlFlowGraph);

       if (const auto& nextStatementAsLoopStatement = std::dynamic_pointer_cast<syrec::ForStatement>(nextStatement->statement); nextStatementAsLoopStatement != nullptr) {
            addInformationAboutLoopWithMoreThanOneStatement(nextStatementAsLoopStatement, nextStatement->relativeIndexInControlFlowGraph.size());
        } else if (const auto& nextStatementAsAssignmentStatement = std::dynamic_pointer_cast<syrec::AssignStatement>(nextStatement->statement); nextStatementAsAssignmentStatement != nullptr) {
            const std::optional<syrec_operation::operation> mappedToAssignmentOperation = syrec_operation::tryMapAssignmentOperationFlagToEnum(nextStatementAsAssignmentStatement->op);
            const std::optional<unsigned int>               assignmentRhsExprAsConstant = tryEvaluateExprToConstant(*nextStatementAsAssignmentStatement->rhs);

            const std::optional<InternalAssignmentData::ptr> optionalMatchingInternalEntryForAssignment = getOrCreateEntryInInternalLookupForAssignment(nextStatementAsAssignmentStatement, indexOfCurrentStmtInControlFlowGraph);
            if (!optionalMatchingInternalEntryForAssignment.has_value()) {
                return {};
            }
            const InternalAssignmentData::ptr& matchingInternalEntryForAssignment = *optionalMatchingInternalEntryForAssignment;
            const bool doesAssignmentRhsDefinedIdentityElement = mappedToAssignmentOperation.has_value() && assignmentRhsExprAsConstant.has_value() && syrec_operation::isOperandUseAsRhsInOperationIdentityElement(*mappedToAssignmentOperation, *assignmentRhsExprAsConstant);
            if (!doesAssignmentRhsDefinedIdentityElement) {
                if (doesAssignmentContainPotentiallyUnsafeOperation(nextStatementAsAssignmentStatement) || (isAssignedToSignalAModifiableParameter(*matchingInternalEntryForAssignment->getAssignedToSignalPartsIdent()) && isAssignmentDefinedInLoopPerformingMoreThanOneIteration())) {
                    removeEntryFromGraveyard(*matchingInternalEntryForAssignment->getAssignedToSignalPartsIdent(), matchingInternalEntryForAssignment);
                    removeDataDependenciesOfAssignmentFromGraveyard(matchingInternalEntryForAssignment);
                }
            }
        } else if (const auto& nextStatementAsUnaryAssignmentStatement = std::dynamic_pointer_cast<syrec::UnaryStatement>(nextStatement->statement); nextStatementAsUnaryAssignmentStatement != nullptr) {
            const std::optional<InternalAssignmentData::ptr> optionalMatchingInternalEntryForAssignment = getOrCreateEntryInInternalLookupForAssignment(nextStatementAsUnaryAssignmentStatement, indexOfCurrentStmtInControlFlowGraph);
            if (!optionalMatchingInternalEntryForAssignment.has_value()) {
                return {};
            }
            const InternalAssignmentData::ptr& matchingInternalEntryForAssignment = *optionalMatchingInternalEntryForAssignment;
            if (doesAssignmentContainPotentiallyUnsafeOperation(nextStatementAsAssignmentStatement) || (isAssignedToSignalAModifiableParameter(*matchingInternalEntryForAssignment->getAssignedToSignalPartsIdent()) && isAssignmentDefinedInLoopPerformingMoreThanOneIteration())) {
                removeEntryFromGraveyard(*matchingInternalEntryForAssignment->getAssignedToSignalPartsIdent(), matchingInternalEntryForAssignment);
                removeDataDependenciesOfAssignmentFromGraveyard(matchingInternalEntryForAssignment);
            }
        } else if (const auto& nextStatementAsIfStatement = std::dynamic_pointer_cast<syrec::IfStatement>(nextStatement->statement); nextStatementAsIfStatement != nullptr) {
            if (isAssignmentDefinedInLoopPerformingMoreThanOneIteration() || doesExpressionContainPotentiallyUnsafeOperation(nextStatementAsIfStatement->condition)) {
                removeOverlappingAssignmentsForSignalAccessesInExpressionFromGraveyard(nextStatementAsIfStatement->condition, indexOfCurrentStmtInControlFlowGraph);    
            }
        } else if (const auto& nextStatementAsCallStatement = std::dynamic_pointer_cast<syrec::CallStatement>(nextStatement->statement); nextStatementAsCallStatement != nullptr) {
            if (isAssignmentDefinedInLoopPerformingMoreThanOneIteration()) {
                for (const auto& callerArgument: nextStatementAsCallStatement->parameters) {
                    if (internalAssignmentData.count(callerArgument)) {
                        for (const auto& definedAssignmentWithAssignedToSignalPartsIdentMatchingCallerArgument: internalAssignmentData.at(callerArgument)) {
                            removeDataDependenciesOfAssignmentFromGraveyard(definedAssignmentWithAssignedToSignalPartsIdentMatchingCallerArgument);
                        }
                    }
                }   
            }
        } else if (const auto& nextStatementAsSwapStatement = std::dynamic_pointer_cast<syrec::SwapStatement>(nextStatement->statement); nextStatementAsSwapStatement != nullptr) {
            const InternalAssignmentData::SwapOperands       operands(nextStatementAsSwapStatement->lhs, nextStatementAsSwapStatement->rhs);
            const std::optional<InternalAssignmentData::ptr> optionalMatchingInternalEntryForSwapLhsOperandAssignment = getOrCreateEntryInInternalLookupForSwapStatement(operands, indexOfCurrentStmtInControlFlowGraph);
            if (!optionalMatchingInternalEntryForSwapLhsOperandAssignment.has_value()) {
                return {};
            }

            const std::string_view&            swapOperandLhsSignalIdent             = operands.lhsOperand.get()->var->name;
            const std::string_view& swapOperandRhsSignalIdent = operands.rhsOperand.get()->var->name;

            const InternalAssignmentData::ptr& matchingInternalEntryForSwapStatement = *optionalMatchingInternalEntryForSwapLhsOperandAssignment;
            if ((isAssignedToSignalAModifiableParameter(swapOperandLhsSignalIdent) || isAssignedToSignalAModifiableParameter(swapOperandRhsSignalIdent)) && isAssignmentDefinedInLoopPerformingMoreThanOneIteration()) {
                removeEntryFromGraveyard(swapOperandLhsSignalIdent, matchingInternalEntryForSwapStatement);
                removeEntryFromGraveyard(swapOperandRhsSignalIdent, matchingInternalEntryForSwapStatement);
            }
        } else {
            const syrec::Statement* internalPointerOfStatement = nextStatement->statement.get();
            if (typeid(*internalPointerOfStatement) == typeid(syrec::SkipStatement)) {
                createGraveyardEntryForSkipStatement(indexOfCurrentStmtInControlFlowGraph);    
            }
        }
        markStatementAsProcessedInLoopBody(nextStatement->statement, nextStatement->relativeIndexInControlFlowGraph.size());
        indexOfLastProcessedStatementInControlFlowGraph.emplace(nextStatement->relativeIndexInControlFlowGraph);

        nextStatement.reset();
        if (const auto& nextFetchedStatement = statementIterationHelper->getNextStatement(); nextFetchedStatement.has_value()) {
            nextStatement.emplace(*nextFetchedStatement);
        }
    }
    return determineDeadStores();
}


void DeadStoreEliminator::removeDataDependenciesOfAssignmentFromGraveyard(const InternalAssignmentData::ptr& internalContainerForAssignment) {
    const std::optional<syrec::Expression::ptr> optionalAssignmentRhsExpr = internalContainerForAssignment->getAssignmentRhsExpr();
    if (internalContainerForAssignment->optionalSwapOperandsData.has_value()) {
        return;
    }

    const syrec::VariableAccess::ptr assignedToSignalParts = internalContainerForAssignment->getAssignedToSignalParts().value();
    removeEntryFromGraveyard(assignedToSignalParts->var->name, internalContainerForAssignment);

    const std::vector<InternalAssignmentData::ptr> overlappingSignalAccessesForAssignedToSignalParts = determineOverlappingAssignmentsForGivenSignalAccess(*assignedToSignalParts, internalContainerForAssignment->indexInControlFlowGraph);
    for (const InternalAssignmentData::ptr& overlappingAssignmentsForAssignedToSignalParts: overlappingSignalAccessesForAssignedToSignalParts) {
        removeDataDependenciesOfAssignmentFromGraveyard(overlappingAssignmentsForAssignedToSignalParts);
    }

    if (!optionalAssignmentRhsExpr.has_value()) {
        return;
    }

    const syrec::Expression::ptr&                  assignmentRhsExpr                        = *optionalAssignmentRhsExpr;
    const std::vector<syrec::VariableAccess::ptr>& definedSignalAccessedInAssignmentRhsExpr = getAccessedLocalSignalsFromExpression(assignmentRhsExpr);
    for (const syrec::VariableAccess::ptr& definedSignalAccessInAssignmentRhsExpr: definedSignalAccessedInAssignmentRhsExpr) {
        const std::vector<InternalAssignmentData::ptr> overlappingAssignmentsForAccessedSignalParts = determineOverlappingAssignmentsForGivenSignalAccess(*definedSignalAccessInAssignmentRhsExpr, internalContainerForAssignment->indexInControlFlowGraph);
        for (const InternalAssignmentData::ptr& overlappingAssignmentsForAccessedToSignalParts: overlappingAssignmentsForAccessedSignalParts) {
            removeDataDependenciesOfAssignmentFromGraveyard(overlappingAssignmentsForAccessedToSignalParts);
        }
    }
}

void DeadStoreEliminator::removeOverlappingAssignmentsFromGraveyard(const syrec::VariableAccess& assignedToSignalParts, const AssignmentStatementIndexInControlFlowGraph& indexOfAssignmentWhereSignalAccessWasDefined) {
    const std::vector<InternalAssignmentData::ptr> overlappingAssignmentsForAccessedSignalParts = determineOverlappingAssignmentsForGivenSignalAccess(assignedToSignalParts, indexOfAssignmentWhereSignalAccessWasDefined);
    for (const InternalAssignmentData::ptr& overlappingAssignmentsForAccessedToSignalParts: overlappingAssignmentsForAccessedSignalParts) {
        removeEntryFromGraveyard(assignedToSignalParts.var->name, overlappingAssignmentsForAccessedToSignalParts);
    }
}

void DeadStoreEliminator::removeOverlappingAssignmentsForSignalAccessesInExpressionFromGraveyard(const syrec::Expression::ptr& expression, const AssignmentStatementIndexInControlFlowGraph& indexOfAssignmentWhereSignalAccessWasDefined) {
    const std::vector<syrec::VariableAccess::ptr>& definedSignalAccessedInAssignmentRhsExpr = getAccessedLocalSignalsFromExpression(expression);
    for (const syrec::VariableAccess::ptr& definedSignalAccessInAssignmentRhsExpr: definedSignalAccessedInAssignmentRhsExpr) {
        const std::vector<InternalAssignmentData::ptr> overlappingAssignmentsForAccessedSignalParts = determineOverlappingAssignmentsForGivenSignalAccess(*definedSignalAccessInAssignmentRhsExpr, indexOfAssignmentWhereSignalAccessWasDefined);
        for (const InternalAssignmentData::ptr& overlappingAssignmentsForAccessedToSignalParts: overlappingAssignmentsForAccessedSignalParts) {
            removeEntryFromGraveyard(definedSignalAccessInAssignmentRhsExpr->var->name, overlappingAssignmentsForAccessedToSignalParts);
        }
    }
}

std::vector<DeadStoreEliminator::InternalAssignmentData::ptr> DeadStoreEliminator::determineOverlappingAssignmentsForGivenSignalAccess(const syrec::VariableAccess& signalAccess, const AssignmentStatementIndexInControlFlowGraph& indexOfAssignmentWhereSignalAccessWasDefined) const {
    if (!graveyard.count(signalAccess.var->name)) {
        return {};
    }

    std::vector<DeadStoreEliminator::InternalAssignmentData::ptr> overlappingAssignments;
    for (const InternalAssignmentData::ptr& deadAssignment: graveyard.at(signalAccess.var->name)) {
        if (!isReachableInReverseControlFlowGraph(deadAssignment->indexInControlFlowGraph, indexOfAssignmentWhereSignalAccessWasDefined)) {
            continue;   
        }

        if (const std::optional<syrec::VariableAccess::ptr> assignedToSignalPartsOfDeadAssignment = deadAssignment->getAssignedToSignalParts(); assignedToSignalPartsOfDeadAssignment.has_value()) {
            const SignalAccessUtils::SignalAccessEquivalenceResult equivalenceCheckResult = SignalAccessUtils::areSignalAccessesEqual(
                    **assignedToSignalPartsOfDeadAssignment,
                    signalAccess,
                    SignalAccessUtils::SignalAccessComponentEquivalenceCriteria::DimensionAccess::Overlapping,
                    SignalAccessUtils::SignalAccessComponentEquivalenceCriteria::BitRange::Overlapping,
                    *symbolTable);
            if ((!(equivalenceCheckResult.isResultCertain && equivalenceCheckResult.equality == SignalAccessUtils::SignalAccessEquivalenceResult::Equality::NotEqual))) {
                overlappingAssignments.emplace_back(deadAssignment);
            }
        } else if (const std::optional<InternalAssignmentData::SwapOperands>& swapOperands = deadAssignment->getSwapOperands(); swapOperands.has_value()) {
            std::optional<SignalAccessUtils::SignalAccessEquivalenceResult> equivalenceCheckResultOfLhsOperand;
            std::optional<SignalAccessUtils::SignalAccessEquivalenceResult> equivalenceCheckResultOfRhsOperand;

            if (swapOperands->lhsOperand->var->name == signalAccess.var->name) {
                equivalenceCheckResultOfLhsOperand = SignalAccessUtils::areSignalAccessesEqual(
                        *swapOperands->lhsOperand,
                        signalAccess,
                        SignalAccessUtils::SignalAccessComponentEquivalenceCriteria::DimensionAccess::Overlapping,
                        SignalAccessUtils::SignalAccessComponentEquivalenceCriteria::BitRange::Overlapping,
                        *symbolTable);
            }
            if (swapOperands->rhsOperand->var->name == signalAccess.var->name) {
                equivalenceCheckResultOfRhsOperand = SignalAccessUtils::areSignalAccessesEqual(
                        *swapOperands->rhsOperand,
                        signalAccess,
                        SignalAccessUtils::SignalAccessComponentEquivalenceCriteria::DimensionAccess::Overlapping,
                        SignalAccessUtils::SignalAccessComponentEquivalenceCriteria::BitRange::Overlapping,
                        *symbolTable);
            }

             if ((equivalenceCheckResultOfLhsOperand.has_value() && !(equivalenceCheckResultOfLhsOperand->isResultCertain && equivalenceCheckResultOfLhsOperand->equality == SignalAccessUtils::SignalAccessEquivalenceResult::Equality::NotEqual)) 
                 || (equivalenceCheckResultOfRhsOperand.has_value() && !(equivalenceCheckResultOfRhsOperand->isResultCertain && equivalenceCheckResultOfRhsOperand->equality == SignalAccessUtils::SignalAccessEquivalenceResult::Equality::NotEqual))) {
                overlappingAssignments.emplace_back(deadAssignment);
            }
        }
    }
    return overlappingAssignments;
}


std::vector<DeadStoreEliminator::AssignmentStatementIndexInControlFlowGraph> DeadStoreEliminator::determineDeadStores() const {
    std::vector<DeadStoreEliminator::AssignmentStatementIndexInControlFlowGraph> deadStoreIndicesInControlFlowGraph;
    for (const std::pair<std::string, std::unordered_set<DeadStoreEliminator::InternalAssignmentData::ptr>> graveYardEntryForGroupOfAssignmentsToSignalIdent: graveyard) {
        for (const auto& graveYardEntry: graveYardEntryForGroupOfAssignmentsToSignalIdent.second) {
            deadStoreIndicesInControlFlowGraph.emplace_back(graveYardEntry->indexInControlFlowGraph);
        }
    }
    std::sort(deadStoreIndicesInControlFlowGraph.begin(), deadStoreIndicesInControlFlowGraph.end());
    return deadStoreIndicesInControlFlowGraph;
}

void DeadStoreEliminator::removeDeadStoresFrom(syrec::Statement::vec& statementList, const std::vector<AssignmentStatementIndexInControlFlowGraph>& foundDeadStores, std::size_t& currDeadStoreIndex, std::size_t nestingLevelOfCurrentBlock) const {
    auto        stopProcessing         = currDeadStoreIndex >= foundDeadStores.size();
    std::size_t numRemovedStmtsInBlock = 0;
    while (!stopProcessing) {
        const auto& deadStoreIndex          = foundDeadStores.at(currDeadStoreIndex);
        // Since the first index in the relative index chain is always the position of the statement in the module, we can omit this one nesting level
        const auto& nestingLevelOfDeadStore = deadStoreIndex.relativeStatementIndexPerControlBlock.size() - 1;
        const auto& relativeStatementIndexInCurrentBlockOfDeadStore = deadStoreIndex.relativeStatementIndexPerControlBlock.at(nestingLevelOfCurrentBlock).relativeIndexInBlock - numRemovedStmtsInBlock;

        const auto& referencedStatement = statementList.at(relativeStatementIndexInCurrentBlockOfDeadStore);
        const auto  copyOfCurrentDeadStoreIndex = currDeadStoreIndex;
        if (nestingLevelOfDeadStore > nestingLevelOfCurrentBlock) {
            switch (deadStoreIndex.relativeStatementIndexPerControlBlock.at(nestingLevelOfCurrentBlock + 1).blockType) {
                /*
                 * We could perform a flip of the branches (which would also require a flip of the guard as well as closing guard expression)
                 * in case the true branch is empty while the false branch is not
                 */
                case StatementIterationHelper::IfConditionTrueBranch: {
                    if (auto referenceStatementAsIfStatement = std::dynamic_pointer_cast<syrec::IfStatement>(referencedStatement); referenceStatementAsIfStatement != nullptr) {
                        removeDeadStoresFrom(referenceStatementAsIfStatement->thenStatements, foundDeadStores, currDeadStoreIndex, nestingLevelOfCurrentBlock + 1);
                        if (referenceStatementAsIfStatement->thenStatements.empty()) {
                            const auto anyDeadStoresRemaining = currDeadStoreIndex < foundDeadStores.size();
                            if (!anyDeadStoresRemaining || !isNextDeadStoreInSameBranch(copyOfCurrentDeadStoreIndex, foundDeadStores, nestingLevelOfCurrentBlock + 1)) {
                                referenceStatementAsIfStatement->thenStatements.emplace_back(std::make_shared<syrec::SkipStatement>());
                            } else {
                                const auto& nextDeadStore               = foundDeadStores.at(currDeadStoreIndex);
                                const auto& nestingLevelOfNextDeadStore = nextDeadStore.relativeStatementIndexPerControlBlock.size();
                                /*
                                 * We know that if the next dead store is found at a higher nesting level that there are no remaining dead stores in the else branch
                                 * and thus if the latter was empty, we are now allowed to remove the whole if statement since the true branch is empty after all dead stores were removed
                                 */
                                if (nestingLevelOfNextDeadStore < nestingLevelOfDeadStore && referenceStatementAsIfStatement->elseStatements.empty()) {
                                    decrementReferenceCountOfUsedSignalsInStatement(referenceStatementAsIfStatement);
                                    statementList.erase(std::next(statementList.begin(), relativeStatementIndexInCurrentBlockOfDeadStore));
                                    numRemovedStmtsInBlock++;
                                }
                            }
                        }
                    }
                    break;
                }
                case StatementIterationHelper::IfConditionFalseBranch: {
                    if (auto referenceStatementAsIfStatement = std::dynamic_pointer_cast<syrec::IfStatement>(referencedStatement); referenceStatementAsIfStatement != nullptr) {
                        removeDeadStoresFrom(referenceStatementAsIfStatement->elseStatements, foundDeadStores, currDeadStoreIndex, nestingLevelOfCurrentBlock + 1);
                        if (referenceStatementAsIfStatement->elseStatements.empty()) {
                            if (referenceStatementAsIfStatement->thenStatements.empty()) {
                                decrementReferenceCountOfUsedSignalsInStatement(referenceStatementAsIfStatement);
                                statementList.erase(std::next(statementList.begin(), relativeStatementIndexInCurrentBlockOfDeadStore));
                                numRemovedStmtsInBlock++;
                            }
                            else {
                                /*
                                 * Since empty branches are not allowed by the SyReC grammar, we need to insert a skip statement into the
                                 * else branch so that the if statement conforms to the grammar
                                 */ 
                                referenceStatementAsIfStatement->elseStatements.emplace_back(std::make_shared<syrec::SkipStatement>());
                            }
                        } else if (!doesExpressionContainPotentiallyUnsafeOperation(referenceStatementAsIfStatement->condition) && doesStatementListOnlyContainSingleSkipStatement(referenceStatementAsIfStatement->thenStatements) && doesStatementListOnlyContainSingleSkipStatement(referenceStatementAsIfStatement->elseStatements)) {
                            decrementReferenceCountOfUsedSignalsInStatement(referenceStatementAsIfStatement);
                            statementList.erase(std::next(statementList.begin(), relativeStatementIndexInCurrentBlockOfDeadStore));
                            numRemovedStmtsInBlock++;
                        }
                        else {
                            if (referenceStatementAsIfStatement->thenStatements.empty()) {
                                /*
                                 * Since empty branches are not allowed by the SyReC grammar, we need to insert a skip statement into the
                                 * true branch so that the if statement conforms to the grammar
                                 */ 
                                referenceStatementAsIfStatement->thenStatements.emplace_back(std::make_shared<syrec::SkipStatement>());
                            }
                        }
                    }
                    break;
                }
                case StatementIterationHelper::Loop: {
                    if (auto referenceStatementAsLoopStatement = std::dynamic_pointer_cast<syrec::ForStatement>(referencedStatement); referenceStatementAsLoopStatement != nullptr) {
                        removeDeadStoresFrom(referenceStatementAsLoopStatement->statements, foundDeadStores, currDeadStoreIndex, nestingLevelOfCurrentBlock + 1);
                        // TODO: Only remove loop if body contains single skip statement and loop header does not contain any potentially dangerous operations
                        if (referenceStatementAsLoopStatement->statements.empty() || doesStatementListOnlyContainSingleSkipStatement(referenceStatementAsLoopStatement->statements)) {
                            decrementReferenceCountOfUsedSignalsInStatement(referenceStatementAsLoopStatement);
                            statementList.erase(std::next(statementList.begin(), relativeStatementIndexInCurrentBlockOfDeadStore));
                            numRemovedStmtsInBlock++;
                            currDeadStoreIndex++;
                        }
                    }
                    break;   
                }
                default:
                    break;
            }
        } else {
            decrementReferenceCountOfUsedSignalsInStatement(referencedStatement);
            statementList.erase(std::next(statementList.begin(), relativeStatementIndexInCurrentBlockOfDeadStore));
            numRemovedStmtsInBlock++;
            currDeadStoreIndex++;
        }
        
        stopProcessing = currDeadStoreIndex >= foundDeadStores.size() || statementList.empty() || !isNextDeadStoreInSameBranch(copyOfCurrentDeadStoreIndex, foundDeadStores, nestingLevelOfCurrentBlock + 1);
    }
}

bool DeadStoreEliminator::doesAssignmentContainPotentiallyUnsafeOperation(const syrec::Statement::ptr& stmt) const {
    if (const auto& stmtAsUnaryAssignmentStmt = std::dynamic_pointer_cast<syrec::UnaryStatement>(stmt); stmtAsUnaryAssignmentStmt != nullptr) {
        return !wasSignalDeclaredAndAreAllIndizesOfSignalConstantsAndWithinRange(stmtAsUnaryAssignmentStmt->var);
    }
    if (const auto& stmtAsAssignmentStmt = std::dynamic_pointer_cast<syrec::AssignStatement>(stmt); stmtAsAssignmentStmt != nullptr) {
        return !wasSignalDeclaredAndAreAllIndizesOfSignalConstantsAndWithinRange(stmtAsAssignmentStmt->lhs)
            || doesExpressionContainPotentiallyUnsafeOperation(stmtAsAssignmentStmt->rhs);
    }
    return false;
}

bool DeadStoreEliminator::doesExpressionContainPotentiallyUnsafeOperation(const syrec::Expression::ptr& expr) const {
    if (const auto& exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr); exprAsBinaryExpr) {
        auto doesContainPotentiallyUnsafeOperation = doesExpressionContainPotentiallyUnsafeOperation(exprAsBinaryExpr->lhs) || doesExpressionContainPotentiallyUnsafeOperation(exprAsBinaryExpr->rhs);
        // Should we consider a division with unknown divisor as an unsafe operation ?
        if (!doesContainPotentiallyUnsafeOperation && exprAsBinaryExpr->op == syrec::BinaryExpression::Divide) {
            if (const auto& rhsOperandAsNumber = std::dynamic_pointer_cast<syrec::NumericExpression>(exprAsBinaryExpr->rhs); rhsOperandAsNumber) {
                doesContainPotentiallyUnsafeOperation = !(tryEvaluateNumber(*rhsOperandAsNumber->value).has_value() && tryEvaluateNumber(*rhsOperandAsNumber->value) != 0);
            }
        }
        return doesContainPotentiallyUnsafeOperation;
    }
    if (const auto& exprAsShiftExpr = std::dynamic_pointer_cast<syrec::ShiftExpression>(expr); exprAsShiftExpr) {
        return doesExpressionContainPotentiallyUnsafeOperation(exprAsShiftExpr->lhs);
    }
    if (const auto& exprAsVariableExpr = std::dynamic_pointer_cast<syrec::VariableExpression>(expr); exprAsVariableExpr) {
        return !wasSignalDeclaredAndAreAllIndizesOfSignalConstantsAndWithinRange(exprAsVariableExpr->var);
    }
    return false;
}

// We are assuming zero based indexing and are also checking further conditions required for a valid bit range access
// are the latter really necessary ?
bool DeadStoreEliminator::wasSignalDeclaredAndAreAllIndizesOfSignalConstantsAndWithinRange(const syrec::VariableAccess::ptr& signalAccess) const {
    const auto fetchedSymbolTableEntry = symbolTable->getVariable(signalAccess->var->name);
    if (!fetchedSymbolTableEntry.has_value()) {
        return false;
    }
    if (std::holds_alternative<syrec::Number::ptr>(*fetchedSymbolTableEntry)) {
        return true;
    }

    const auto& symbolTableEntryForVariable = std::get<syrec::Variable::ptr>(*fetchedSymbolTableEntry);
    if (signalAccess->indexes.size() > symbolTableEntryForVariable->dimensions.size()) {
        return false;
    }

    bool dimensionAccessOK = true;
    for (std::size_t i = 0; i < signalAccess->indexes.size() && dimensionAccessOK; ++i) {
        const auto& accessedValueOfDimension          = signalAccess->indexes.at(i);
        const auto& maximumValidValueOfDimension      = symbolTableEntryForVariable->dimensions.at(i) - 1;

        if (const auto& accessedValueOfDimensionAsNumericExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(accessedValueOfDimension); accessedValueOfDimensionAsNumericExpr != nullptr) {
            const auto accessedValueOfDimensionEvaluated = tryEvaluateNumber(*accessedValueOfDimensionAsNumericExpr->value);
            dimensionAccessOK                            = accessedValueOfDimensionEvaluated.has_value() && *accessedValueOfDimensionEvaluated <= maximumValidValueOfDimension;    
        }
        else {
            dimensionAccessOK = false;
        }
    }

    bool bitRangeOK = dimensionAccessOK;
    if (signalAccess->range.has_value() && bitRangeOK) {
        const auto maximumValidBitRangeIndex = symbolTableEntryForVariable->bitwidth - 1;
        const auto bitRangeStartEvaluated    = tryEvaluateNumber(*signalAccess->range->first);
        const auto bitRangeEndEvaluated      = tryEvaluateNumber(*signalAccess->range->second);

        bitRangeOK = bitRangeStartEvaluated.has_value() && bitRangeEndEvaluated.has_value() && *bitRangeStartEvaluated <= maximumValidBitRangeIndex && *bitRangeEndEvaluated <= maximumValidBitRangeIndex && *bitRangeStartEvaluated <= *bitRangeEndEvaluated;
    }

    return dimensionAccessOK && bitRangeOK;
}

// TODO: Handling of state variables
bool DeadStoreEliminator::isAssignedToSignalAModifiableParameter(const std::string_view& assignedToSignalIdent) const {
    if (const auto& symbolTableEntryForAssignedToSignal = symbolTable->getVariable(assignedToSignalIdent); symbolTableEntryForAssignedToSignal.has_value()) {
        if (std::holds_alternative<syrec::Variable::ptr>(*symbolTableEntryForAssignedToSignal)) {
            const auto& assignedToVariable = std::get<syrec::Variable::ptr>(*symbolTableEntryForAssignedToSignal);
            return assignedToVariable->type == syrec::Variable::Types::Inout || assignedToVariable->type == syrec::Variable::Types::Out;
        }
    }
    return false;
}

std::optional<unsigned int> DeadStoreEliminator::tryEvaluateNumber(const syrec::Number& number) {
    return number.isConstant() ? std::make_optional(number.evaluate({})) : std::nullopt;
}

std::optional<unsigned> DeadStoreEliminator::tryEvaluateExprToConstant(const syrec::Expression& expr) {
    if (const auto& exprAsNumericExpr = dynamic_cast<const syrec::NumericExpression*>(&expr); exprAsNumericExpr) {
        return tryEvaluateNumber(*exprAsNumericExpr->value);
    }
    return std::nullopt;
}

void DeadStoreEliminator::resetInternalData() {
    indexOfLastProcessedStatementInControlFlowGraph.reset();
    livenessStatusScopes.clear();
    graveyard.clear();
    internalAssignmentData.clear();
}

void DeadStoreEliminator::decrementReferenceCountOfUsedSignalsInStatement(const syrec::Statement::ptr& statement) const {
    /*
     * We do not have to decrement the reference counts of signals used in a loop statement since we are assuming that its body is already empty and
     * only constants or signal widths, which are already transformed to constants, can be used for which no reference counting is required
     */
    if (const auto& statementAsIfStatement = std::dynamic_pointer_cast<syrec::IfStatement>(statement); statementAsIfStatement != nullptr) {
        /*
         * Since reference counts will only be updated for the defined signal accesses in the guard condition, we also only need to decrement them for the guard expression when removing an if statement
         */
        decrementReferenceCountsOfUsedSignalsInExpression(statementAsIfStatement->condition);
    } else if (const auto& statementAsAssignmentStatement = std::dynamic_pointer_cast<syrec::AssignStatement>(statement); statementAsAssignmentStatement != nullptr) {
        decrementReferenceCountForAccessedSignal(statementAsAssignmentStatement->lhs);
        decrementReferenceCountsOfUsedSignalsInExpression(statementAsAssignmentStatement->rhs);
    } else if (const auto& statementAsUnaryAssignmentStatement = std::dynamic_pointer_cast<syrec::UnaryStatement>(statement); statementAsUnaryAssignmentStatement != nullptr) {
        decrementReferenceCountForAccessedSignal(statementAsUnaryAssignmentStatement->var);
    } else if (const auto& statementAsSwapStatement = std::dynamic_pointer_cast<syrec::SwapStatement>(statement); statementAsSwapStatement != nullptr) {
        decrementReferenceCountForAccessedSignal(statementAsSwapStatement->lhs);
        decrementReferenceCountForAccessedSignal(statementAsSwapStatement->rhs);
    } else if (const auto& statementAsCallStatement = std::dynamic_pointer_cast<syrec::CallStatement>(statement); statementAsCallStatement != nullptr) {
        for (const auto& calleeArgument : statementAsCallStatement->parameters) {
            const auto& fetchedOptionalSymbolTableEntry = symbolTable->getVariable(calleeArgument);
            if (!fetchedOptionalSymbolTableEntry.has_value() || !std::holds_alternative<syrec::Variable::ptr>(*fetchedOptionalSymbolTableEntry)) {
                continue;
            }

            const auto& accessedVariable = std::get<syrec::Variable::ptr>(*fetchedOptionalSymbolTableEntry);
            const auto  signalAccess     = std::make_shared<syrec::VariableAccess>();
            signalAccess->setVar(accessedVariable);
            decrementReferenceCountForAccessedSignal(signalAccess);
        }
    }
}

void DeadStoreEliminator::decrementReferenceCountsOfUsedSignalsInExpression(const syrec::Expression::ptr& expr) const {
    if (const auto& exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr); exprAsBinaryExpr != nullptr) {
        decrementReferenceCountsOfUsedSignalsInExpression(exprAsBinaryExpr->lhs);
        decrementReferenceCountsOfUsedSignalsInExpression(exprAsBinaryExpr->rhs);
    } else if (const auto& exprAsShiftExpr = std::dynamic_pointer_cast<syrec::ShiftExpression>(expr); exprAsShiftExpr != nullptr) {
        decrementReferenceCountsOfUsedSignalsInExpression(exprAsShiftExpr->lhs);
        decrementReferenceCountOfNumber(exprAsShiftExpr->rhs);
    } else if (const auto& exprAsVariableExpr = std::dynamic_pointer_cast<syrec::VariableExpression>(expr); exprAsVariableExpr != nullptr) {
        decrementReferenceCountForAccessedSignal(exprAsVariableExpr->var);
    } else if (const auto& exprAsNumericExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(expr); exprAsNumericExpr != nullptr) {
        decrementReferenceCountOfNumber(exprAsNumericExpr->value);
    }
    // TODO: Support for unary expressions and maybe reference counting of loop variables ?
}

void DeadStoreEliminator::decrementReferenceCountForAccessedSignal(const syrec::VariableAccess::ptr& accessedSignal) const {
    symbolTable->updateReferenceCountOfLiteral(accessedSignal->var->name, parser::SymbolTable::ReferenceCountUpdate::Decrement);
}

void DeadStoreEliminator::decrementReferenceCountOfNumber(const syrec::Number::ptr& number) const {
    if (number->isConstant()) {
        return;
    }

    if (number->isCompileTimeConstantExpression()) {
        decrementReferenceCountOfNumber(number->getExpression().lhsOperand);
        decrementReferenceCountOfNumber(number->getExpression().rhsOperand);
    }
    else if (number->isLoopVariable()){
        symbolTable->updateReferenceCountOfLiteral(number->variableName(), parser::SymbolTable::ReferenceCountUpdate::Decrement);
    }
}

bool DeadStoreEliminator::isAssignmentDefinedInLoopPerformingMoreThanOneIteration() const {
    return remainingNonControlFlowStatementsPerLoopBody.empty()
        || std::any_of(
        remainingNonControlFlowStatementsPerLoopBody.cbegin(), 
        remainingNonControlFlowStatementsPerLoopBody.cend(), 
        [](const LoopStatementInformation& loopInformation) {
            return loopInformation.performsMoreThanOneIteration;
        });
}

void DeadStoreEliminator::markStatementAsProcessedInLoopBody(const syrec::Statement::ptr& stmt, std::size_t nestingLevelOfStmt) {
    if (std::dynamic_pointer_cast<syrec::ForStatement>(stmt) != nullptr 
        || remainingNonControlFlowStatementsPerLoopBody.empty() 
        || nestingLevelOfStmt > remainingNonControlFlowStatementsPerLoopBody.back().nestingLevelOfLoop ) {
        return;
    }

    auto& numRemainingStmtInLoopBody = remainingNonControlFlowStatementsPerLoopBody.back().numRemainingNonControlFlowStatementsInLoopBody;
    if (numRemainingStmtInLoopBody >= 1) {
        numRemainingStmtInLoopBody--;
    }

    if (numRemainingStmtInLoopBody == 0) {
        remainingNonControlFlowStatementsPerLoopBody.erase(std::prev(remainingNonControlFlowStatementsPerLoopBody.end()));
    }
}

void DeadStoreEliminator::addInformationAboutLoopWithMoreThanOneStatement(const std::shared_ptr<syrec::ForStatement>& loopStmt, std::size_t nestingLevelOfStmt) {
    remainingNonControlFlowStatementsPerLoopBody.emplace_back(LoopStatementInformation(nestingLevelOfStmt, loopStmt->statements.size(), doesLoopPerformMoreThanOneIteration(loopStmt)));
}

bool DeadStoreEliminator::doesLoopPerformMoreThanOneIteration(const std::shared_ptr<syrec::ForStatement>& loopStmt) const {
    const auto& startValue = tryEvaluateNumber(*loopStmt->range.first);
    const auto& endValue = tryEvaluateNumber(*loopStmt->range.second);
    const auto& stepSize   = tryEvaluateNumber(*loopStmt->step);

    if (!startValue.has_value() || !endValue.has_value() || !stepSize.has_value()) {
        return false;
    }
    
    if (const auto numIterations = utils::determineNumberOfLoopIterations(*startValue, *endValue, *stepSize); numIterations.has_value()) {
        return *numIterations > 1;
    }
    return true;
}

void DeadStoreEliminator::createNewLivenessStatusScope() {
    livenessStatusScopes.emplace_back(std::make_shared<LivenessStatusLookupScope>());
}

bool DeadStoreEliminator::tryCreateCopyOfLivenessStatusForSignalInCurrentScope(const std::string& signalIdent) {
    const auto& indexOfParentScopeContainingEntryForSignal = findScopeContainingEntryForSignal(signalIdent);
    if (!indexOfParentScopeContainingEntryForSignal.has_value()) {
        return false;
    }

    const auto& livenessStatusOfSignalInParentScope = livenessStatusScopes.at(*indexOfParentScopeContainingEntryForSignal)->livenessStatusLookup.at(signalIdent);
    auto&       currentLivenessStatusScope          = livenessStatusScopes.back();
    
    auto        livenessStatusCopyForCurrentScope   = livenessStatusOfSignalInParentScope->clone();
    currentLivenessStatusScope->livenessStatusLookup.insert_or_assign(signalIdent, livenessStatusCopyForCurrentScope);
    return true;
}

void DeadStoreEliminator::destroyCurrentLivenessStatusScope() {
    if (livenessStatusScopes.size() <= 1) {
        return;
    }
    //mergeLivenessStatusOfCurrentScopeWithParent();
    livenessStatusScopes.erase(std::prev(livenessStatusScopes.end()));
}

void DeadStoreEliminator::updateLivenessStatusScopeAccordingToNestingLevelOfStatement(const std::optional<AssignmentStatementIndexInControlFlowGraph>& indexOfLastProcessedStatement, const AssignmentStatementIndexInControlFlowGraph& indexOfCurrentStatement) {
    const std::size_t nestingLevelOfLastProcessedStatement      = indexOfLastProcessedStatement.has_value() ? determineNestingLevelMeasuredForIfStatements(*indexOfLastProcessedStatement) : 0;
    const auto nestingLevelOfCurrentStatement       = determineNestingLevelMeasuredForIfStatements(indexOfCurrentStatement);

    const auto numberOfScopesToAdd = nestingLevelOfCurrentStatement > nestingLevelOfLastProcessedStatement ? nestingLevelOfCurrentStatement - nestingLevelOfLastProcessedStatement : 0;
    const auto numberOfScopesToRemove = nestingLevelOfCurrentStatement < nestingLevelOfLastProcessedStatement ? nestingLevelOfLastProcessedStatement - nestingLevelOfCurrentStatement : 0;

    const auto shouldAddNewScopes = numberOfScopesToRemove == 0 && numberOfScopesToAdd > 0;
    const auto shouldRemoveScopes = !shouldAddNewScopes && numberOfScopesToRemove > 0;
    if (!shouldAddNewScopes && !shouldRemoveScopes) {
        if (nestingLevelOfLastProcessedStatement == 0 && livenessStatusScopes.empty()) {
            createNewLivenessStatusScope();
        }
        return;
    }
    
    const std::size_t numUpdatedScopes = shouldAddNewScopes ? numberOfScopesToAdd : numberOfScopesToRemove;
    if (shouldAddNewScopes) {
        for (std::size_t i = 0; i < numUpdatedScopes; ++i) {
            createNewLivenessStatusScope();
        }
    }
    else {
        for (std::size_t i = 0; i < numUpdatedScopes; ++i) {
            destroyCurrentLivenessStatusScope();
        }
    }
}

std::size_t DeadStoreEliminator::determineNestingLevelMeasuredForIfStatements(const AssignmentStatementIndexInControlFlowGraph& statementIndexInControlFlowGraph) const {
    return std::count_if(
    statementIndexInControlFlowGraph.relativeStatementIndexPerControlBlock.cbegin(),
    statementIndexInControlFlowGraph.relativeStatementIndexPerControlBlock.cend(),
    [](const StatementIterationHelper::StatementIndexInBlock& relativeStatementIndexInBlock) {
        return relativeStatementIndexInBlock.blockType == StatementIterationHelper::IfConditionFalseBranch
            || relativeStatementIndexInBlock.blockType == StatementIterationHelper::IfConditionTrueBranch;
    });
}

bool DeadStoreEliminator::isReachableInReverseControlFlowGraph(const AssignmentStatementIndexInControlFlowGraph& assignmentStatement, const AssignmentStatementIndexInControlFlowGraph& usageOfAssignedToSignal) const {
    const std::size_t& numIndizesToCheck = std::min(assignmentStatement.relativeStatementIndexPerControlBlock.size(), usageOfAssignedToSignal.relativeStatementIndexPerControlBlock.size());
    bool        isReachable       = true;

    for (std::size_t i = 0; i < numIndizesToCheck && isReachable; ++i) {
        const auto& currRelativeIndexForAssignmentStmt = assignmentStatement.relativeStatementIndexPerControlBlock.at(i);
        const auto& currRelativeIndexForUsageStmt = usageOfAssignedToSignal.relativeStatementIndexPerControlBlock.at(i);
        
        if (currRelativeIndexForUsageStmt.relativeIndexInBlock < currRelativeIndexForAssignmentStmt.relativeIndexInBlock) {
            isReachable = false;
            continue;
        }

        const bool isCurrentIndexOfAssignmentStmtInIfBranch = currRelativeIndexForAssignmentStmt.blockType == StatementIterationHelper::IfConditionTrueBranch || currRelativeIndexForAssignmentStmt.blockType == StatementIterationHelper::IfConditionFalseBranch;
        const bool isCurrentIndexOfUsageStmtInIfBranch      = currRelativeIndexForUsageStmt.blockType == StatementIterationHelper::IfConditionTrueBranch || currRelativeIndexForUsageStmt.blockType == StatementIterationHelper::IfConditionFalseBranch;

        // If the current relative statement index for both statements is in a branch of an if statement check whether they are in the same branch (otherwise the assignment statement is not reachable from the current one)
        if (isCurrentIndexOfAssignmentStmtInIfBranch && isCurrentIndexOfUsageStmtInIfBranch) {
            isReachable = currRelativeIndexForAssignmentStmt.blockType == currRelativeIndexForUsageStmt.blockType;
        }
        else {
            isReachable = true;
        }
    }
    return isReachable;
}

std::optional<std::size_t> DeadStoreEliminator::findScopeContainingEntryForSignal(const std::string& signalIdent) const {
    std::size_t numScopesToCheck = !livenessStatusScopes.empty() ? livenessStatusScopes.size() - 1 : 0;
    
    if (numScopesToCheck == 0) {
        return std::nullopt;
    }

    bool foundEntry = false;
    auto scopeIndex = livenessStatusScopes.size();
    for (auto reverseScopeIterator = livenessStatusScopes.rbegin(); reverseScopeIterator != livenessStatusScopes.rend() && !foundEntry && numScopesToCheck > 0; ++reverseScopeIterator) {
        --scopeIndex;
        --numScopesToCheck;
        foundEntry = reverseScopeIterator->get()->livenessStatusLookup.count(signalIdent) != 0;
    }
    return foundEntry ? std::make_optional(scopeIndex) : std::nullopt;
}

std::size_t DeadStoreEliminator::getBlockTypePrecedence(const StatementIterationHelper::BlockType blockType) {
    switch (blockType) {
        case StatementIterationHelper::Module:
            return 0;
        case StatementIterationHelper::Loop:
            return 1;
        case StatementIterationHelper::IfConditionTrueBranch:
            return 2;
        case StatementIterationHelper::IfConditionFalseBranch:
            return 3;
        default:
            return 4;
    }
}

bool DeadStoreEliminator::isNextDeadStoreInSameBranch(std::size_t currentDeadStoreIndex, const std::vector<AssignmentStatementIndexInControlFlowGraph>& foundDeadStores, std::size_t currentNestingLevel) const {
    const auto nextDeadStoreIndex = currentDeadStoreIndex + 1;
    if (foundDeadStores.empty() || nextDeadStoreIndex >= foundDeadStores.size()) {
        return false;
    }

    const auto& nextDeadStore = foundDeadStores.at(nextDeadStoreIndex);
    const auto& currentDeadStore = foundDeadStores.at(currentDeadStoreIndex);
    

    const auto numBlocksToCheck = std::min(currentNestingLevel, std::min(currentDeadStore.relativeStatementIndexPerControlBlock.size(), nextDeadStore.relativeStatementIndexPerControlBlock.size()));

    const auto& firstElementToCheckInCurrentDeadStore = currentDeadStore.relativeStatementIndexPerControlBlock.cbegin();
    const auto& lastElementToCheckInCurrentDeadStore  = std::next(currentDeadStore.relativeStatementIndexPerControlBlock.cbegin(), numBlocksToCheck);

    const auto& firstElementToCheckInNextDeadStore = nextDeadStore.relativeStatementIndexPerControlBlock.cbegin();
    const auto& lastElementToCheckInNextDeadStore  = std::next(nextDeadStore.relativeStatementIndexPerControlBlock.cbegin(), numBlocksToCheck);

    const auto& foundMismatchInIndexOrBlockType = std::mismatch(
    firstElementToCheckInCurrentDeadStore,
    lastElementToCheckInCurrentDeadStore,
    firstElementToCheckInNextDeadStore,
    lastElementToCheckInNextDeadStore,
    [](const StatementIterationHelper::StatementIndexInBlock& relativeIndexInBlockForCurrentDeadStore, const StatementIterationHelper::StatementIndexInBlock& relativeIndexInBlockForNextDeadStore) {
        return relativeIndexInBlockForCurrentDeadStore.relativeIndexInBlock <= relativeIndexInBlockForNextDeadStore.relativeIndexInBlock
            && relativeIndexInBlockForCurrentDeadStore.blockType == relativeIndexInBlockForNextDeadStore.blockType;      
    });

    return foundMismatchInIndexOrBlockType.first == lastElementToCheckInCurrentDeadStore;
}

std::vector<syrec::VariableAccess::ptr> DeadStoreEliminator::getAccessedLocalSignalsFromExpression(const syrec::Expression::ptr& expr) const {
    if (const auto& exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr); exprAsBinaryExpr != nullptr) {
        auto foundLocalsSignalsFromLhsExpr = getAccessedLocalSignalsFromExpression(exprAsBinaryExpr->lhs);
        const auto& foundLocalsSignalsFromRhsExpr = getAccessedLocalSignalsFromExpression(exprAsBinaryExpr->rhs);
        foundLocalsSignalsFromLhsExpr.insert(foundLocalsSignalsFromLhsExpr.end(), foundLocalsSignalsFromRhsExpr.begin(), foundLocalsSignalsFromRhsExpr.end());
        return foundLocalsSignalsFromLhsExpr;
    } if (const auto& exprAsShiftExpr = std::dynamic_pointer_cast<syrec::ShiftExpression>(expr); exprAsShiftExpr != nullptr) {
        return getAccessedLocalSignalsFromExpression(exprAsShiftExpr->lhs);
    } if (const auto& exprAsVariableExpr = std::dynamic_pointer_cast<syrec::VariableExpression>(expr); exprAsVariableExpr != nullptr) {
        if (isAccessedSignalLocalOfModule(exprAsVariableExpr->var) && symbolTable->canSignalBeAssignedTo(exprAsVariableExpr->var->var->name)) {
            return {exprAsVariableExpr->var};
        }
    }
    return {};
}

bool DeadStoreEliminator::isAccessedSignalLocalOfModule(const syrec::VariableAccess::ptr& accessedSignal) const {
    if (const auto& symbolTableEntryForAssignedToSignal = symbolTable->getVariable(accessedSignal->var->name); symbolTableEntryForAssignedToSignal.has_value()) {
        if (std::holds_alternative<syrec::Variable::ptr>(*symbolTableEntryForAssignedToSignal)) {
            const auto& assignedToVariable = std::get<syrec::Variable::ptr>(*symbolTableEntryForAssignedToSignal);
            return assignedToVariable->type == syrec::Variable::Types::Wire || assignedToVariable->type == syrec::Variable::Types::State;
        }
    }
    return false;
}

bool DeadStoreEliminator::doesStatementListOnlyContainSingleSkipStatement(const syrec::Statement::vec& statementsToCheck) {
    if (statementsToCheck.size() != 1) {
        return false;   
    }

    const syrec::Statement* statementOfInterest = statementsToCheck.front().get();
    return typeid(*statementOfInterest) == typeid(syrec::SkipStatement);
}
