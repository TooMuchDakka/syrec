#include "core/syrec/parser/optimizations/deadStoreElimination/statement_iteration_helper.hpp"
#include "core/syrec/parser/optimizations/deadStoreElimination/dead_store_eliminator.hpp"
#include "core/syrec/parser/utils/loop_range_utils.hpp"

using namespace deadStoreElimination;

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
            markAccessedVariablePartsAsLive(nextStatementAsAssignmentStatement->lhs, indexOfCurrentStmtInControlFlowGraph);
            markAccessedSignalsAsLiveInExpression(nextStatementAsAssignmentStatement->rhs, indexOfCurrentStmtInControlFlowGraph);
            if (!doesAssignmentContainPotentiallyUnsafeOperation(nextStatementAsAssignmentStatement) && !isAssignmentDefinedInLoopPerformingMoreThanOneIteration()) {
                insertPotentiallyDeadAssignmentStatement(nextStatementAsAssignmentStatement->lhs, nextStatement->relativeIndexInControlFlowGraph);   
            }
            //insertPotentiallyDeadAssignmentStatement(nextStatementAsAssignmentStatement->lhs, nextStatement->relativeIndexInControlFlowGraph);   
        } else if (const auto& nextStatementAsUnaryAssignmentStatement = std::dynamic_pointer_cast<syrec::UnaryStatement>(nextStatement->statement); nextStatementAsUnaryAssignmentStatement != nullptr) {
            markAccessedVariablePartsAsLive(nextStatementAsUnaryAssignmentStatement->var, indexOfCurrentStmtInControlFlowGraph);
            if (!doesAssignmentContainPotentiallyUnsafeOperation(nextStatementAsUnaryAssignmentStatement) && !isAssignmentDefinedInLoopPerformingMoreThanOneIteration()) {
                insertPotentiallyDeadAssignmentStatement(nextStatementAsUnaryAssignmentStatement->var, nextStatement->relativeIndexInControlFlowGraph);   
            }
            //insertPotentiallyDeadAssignmentStatement(nextStatementAsUnaryAssignmentStatement->var, nextStatement->relativeIndexInControlFlowGraph);   
        } else if (const auto& nextStatementAsIfStatement = std::dynamic_pointer_cast<syrec::IfStatement>(nextStatement->statement); nextStatementAsIfStatement != nullptr) {
            markAccessedSignalsAsLiveInExpression(nextStatementAsIfStatement->condition, indexOfCurrentStmtInControlFlowGraph);
        } else if (const auto& nextStatementAsCallStatement = std::dynamic_pointer_cast<syrec::CallStatement>(nextStatement->statement); nextStatementAsCallStatement != nullptr) {
            markAccessedSignalsAsLiveInCallStatement(nextStatementAsCallStatement, indexOfCurrentStmtInControlFlowGraph);
        } else if (const auto& nextStatementAsSwapStatement = std::dynamic_pointer_cast<syrec::SwapStatement>(nextStatement->statement); nextStatementAsSwapStatement != nullptr) {
            markAccessedVariablePartsAsLive(nextStatementAsSwapStatement->lhs, indexOfCurrentStmtInControlFlowGraph);
            markAccessedVariablePartsAsLive(nextStatementAsSwapStatement->rhs, indexOfCurrentStmtInControlFlowGraph);
        }
        
        markStatementAsProcessedInLoopBody(nextStatement->statement, nextStatement->relativeIndexInControlFlowGraph.size());
        indexOfLastProcessedStatementInControlFlowGraph.emplace(nextStatement->relativeIndexInControlFlowGraph);

        nextStatement.reset();
        if (const auto& nextFetchedStatement = statementIterationHelper->getNextStatement(); nextFetchedStatement.has_value()) {
            nextStatement.emplace(*nextFetchedStatement);
        }
    }
    return combineAndSortDeadRemainingDeadStores();
}

void DeadStoreEliminator::removeDeadStoresFrom(syrec::Statement::vec& statementList, const std::vector<AssignmentStatementIndexInControlFlowGraph>& foundDeadStores, std::size_t& currDeadStoreIndex, std::size_t nestingLevelOfCurrentBlock) const {
    auto        stopProcessing         = currDeadStoreIndex >= foundDeadStores.size();
    std::size_t numRemovedStmtsInBlock = 0;
    while (!stopProcessing) {
        const auto& deadStoreIndex          = foundDeadStores.at(currDeadStoreIndex);
        // Since the first index in the relative index chain is always the position of the statement in the module, we can omit this one nesting level
        const auto& nestingLevelOfDeadStore = deadStoreIndex.relativeStatementIndexPerControlBlock.size() - 1;

        /*
         * Number of removed statements is not persisted and if the sorting is not done correctly leads to an error
         * i.e.
         *
         * if ...
         *  S1
         * else
         *  S2
         *  S3
         *
         *  and the ordering would be S2, S1, S3 => the information for the fixup required for the removal of S3 due to S2 being removed is lost
         *  due to the branch switch to remove S1
         */
        const auto& relativeStatementIndexInCurrentBlockOfDeadStore = deadStoreIndex.relativeStatementIndexPerControlBlock.at(nestingLevelOfCurrentBlock).relativeIndexInBlock - numRemovedStmtsInBlock;

        const auto& referencedStatement = statementList.at(relativeStatementIndexInCurrentBlockOfDeadStore);
        const auto  copyOfCurrentDeadStoreIndex = currDeadStoreIndex;
        if (nestingLevelOfDeadStore > nestingLevelOfCurrentBlock) {
            /*const auto typeOfNestedBlock = nestingLevelOfCurrentBlock == 0
                ? deadStoreIndex.relativeStatementIndexPerControlBlock.at(nestingLevelOfCurrentBlock + 1).blockType
                : deadStoreIndex.relativeStatementIndexPerControlBlock.at(nestingLevelOfCurrentBlock).blockType;*/

            const auto typeOfNestedBlock = deadStoreIndex.relativeStatementIndexPerControlBlock.at(nestingLevelOfCurrentBlock + 1).blockType;

            /*const auto typeOfNestedBlock = nestingLevelOfCurrentBlock + 1 <= nestingLevelOfDeadStore
                ? deadStoreIndex.relativeStatementIndexPerControlBlock.at(nestingLevelOfCurrentBlock + 1).blockType
                : StatementIterationHelper::BlockType::Module;
            */
            switch (typeOfNestedBlock) {
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
                        } else {
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
                        if (referenceStatementAsLoopStatement->statements.empty()) {
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

void DeadStoreEliminator::removeDeadStoresFrom(syrec::Statement::vec& statementList, const std::vector<AssignmentStatementIndexInControlFlowGraph>& foundDeadStores) const {
    std::size_t deadStoreIndex = 0;
    removeDeadStoresFrom(statementList, foundDeadStores, deadStoreIndex, 0);
}

bool DeadStoreEliminator::doesAssignmentContainPotentiallyUnsafeOperation(const syrec::Statement::ptr& stmt) const {
    if (const auto& stmtAsUnaryAssignmentStmt = std::dynamic_pointer_cast<syrec::UnaryStatement>(stmt); stmtAsUnaryAssignmentStmt != nullptr) {
        return !wasSignalDeclaredAndAreAllIndizesOfSignalConstantsAndWithinRange(stmtAsUnaryAssignmentStmt->var)
            || isAssignedToSignalAModifiableParameter(stmtAsUnaryAssignmentStmt->var->var->name);
    }
    if (const auto& stmtAsAssignmentStmt = std::dynamic_pointer_cast<syrec::AssignStatement>(stmt); stmtAsAssignmentStmt != nullptr) {
        return !wasSignalDeclaredAndAreAllIndizesOfSignalConstantsAndWithinRange(stmtAsAssignmentStmt->lhs)
            || isAssignedToSignalAModifiableParameter(stmtAsAssignmentStmt->lhs->var->name)
            || doesExpressionContainPotentiallyUnsafeOperation(stmtAsAssignmentStmt->rhs);
    }
    return false;
}

bool DeadStoreEliminator::doesExpressionContainPotentiallyUnsafeOperation(const syrec::expression::ptr& expr) const {
    if (const auto& exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr); exprAsBinaryExpr != nullptr) {
        auto doesContainPotentiallyUnsafeOperation = doesExpressionContainPotentiallyUnsafeOperation(exprAsBinaryExpr->lhs) || doesExpressionContainPotentiallyUnsafeOperation(exprAsBinaryExpr->rhs);
        // Should we consider a division with unknown divisor as an unsafe operation ?
        if (!doesContainPotentiallyUnsafeOperation && exprAsBinaryExpr->op == syrec::BinaryExpression::Divide) {
            if (const auto& rhsOperandAsNumber = std::dynamic_pointer_cast<syrec::NumericExpression>(exprAsBinaryExpr->rhs); rhsOperandAsNumber != nullptr) {
                doesContainPotentiallyUnsafeOperation = !(tryEvaluateNumber(rhsOperandAsNumber->value).has_value() && tryEvaluateNumber(rhsOperandAsNumber->value) != 0);
            }
        }
        return doesContainPotentiallyUnsafeOperation;
    }
    if (const auto& exprAsShiftExpr = std::dynamic_pointer_cast<syrec::ShiftExpression>(expr); exprAsShiftExpr != nullptr) {
        return doesExpressionContainPotentiallyUnsafeOperation(exprAsShiftExpr->lhs);
    }
    if (const auto& exprAsVariableExpr = std::dynamic_pointer_cast<syrec::VariableExpression>(expr); exprAsVariableExpr != nullptr) {
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
            const auto accessedValueOfDimensionEvaluated = tryEvaluateNumber(accessedValueOfDimensionAsNumericExpr->value);
            dimensionAccessOK                            = accessedValueOfDimensionEvaluated.has_value() && *accessedValueOfDimensionEvaluated < maximumValidValueOfDimension;    
        }
        else {
            dimensionAccessOK = false;
        }
    }

    bool bitRangeOK = dimensionAccessOK;
    if (signalAccess->range.has_value() && bitRangeOK) {
        const auto maximumValidBitRangeIndex = symbolTableEntryForVariable->bitwidth - 1;
        const auto bitRangeStartEvaluated    = tryEvaluateNumber(signalAccess->range->first);
        const auto bitRangeEndEvaluated      = tryEvaluateNumber(signalAccess->range->second);

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

std::optional<unsigned int> DeadStoreEliminator::tryEvaluateNumber(const syrec::Number::ptr& number) const {
    return number->isConstant() ? std::make_optional(number->evaluate({})) : std::nullopt;
}

void DeadStoreEliminator::markAccessedVariablePartsAsLive(const syrec::VariableAccess::ptr& signalAccess, const AssignmentStatementIndexInControlFlowGraph& indexOfStatementContainingSignalAccess) {
    const auto& signalTableEntryForAccessedSignal = symbolTable->getVariable(signalAccess->var->name);
    if (!signalTableEntryForAccessedSignal.has_value() || !std::holds_alternative<syrec::Variable::ptr>(*signalTableEntryForAccessedSignal)) {
        return;
    }

    const auto& currentLivenessStatusLookup = getLivenessStatusLookupForCurrentScope();
    if (currentLivenessStatusLookup.value()->livenessStatusLookup.count(signalAccess->var->name) == 0) {
        if (!tryCreateCopyOfLivenessStatusForSignalInCurrentScope(signalAccess->var->name)) {
            const auto& referencedSignalInVariableAccess = std::get<syrec::Variable::ptr>(*signalTableEntryForAccessedSignal);
            auto        livenessStatusLookupEntry        = std::make_shared<DeadStoreStatusLookup>(
                    referencedSignalInVariableAccess->bitwidth,
                    referencedSignalInVariableAccess->dimensions,
                    std::make_optional(false));
            currentLivenessStatusLookup.value()->livenessStatusLookup.insert(std::make_pair(signalAccess->var->name, livenessStatusLookupEntry));    
        }
    }

    auto&       livenessStatusForAccessedVariable     = currentLivenessStatusLookup.value()->livenessStatusLookup.at(signalAccess->var->name);
    const auto& backingVariableEntryForAccessedSignal = std::get<syrec::Variable::ptr>(*signalTableEntryForAccessedSignal);
    const auto& transformedDimensionAccess = transformUserDefinedDimensionAccess(backingVariableEntryForAccessedSignal->dimensions.size(), signalAccess->indexes);
    const auto& transformedBitRangeAccess  = transformUserDefinedBitRangeAccess(backingVariableEntryForAccessedSignal->bitwidth, signalAccess->range);
    if (transformedBitRangeAccess.has_value()) {
        livenessStatusForAccessedVariable->invalidateStoredValueForBitrange(transformedDimensionAccess, *transformedBitRangeAccess);
    } else {
        livenessStatusForAccessedVariable->invalidateStoredValueFor(transformedDimensionAccess);
    }

    removeNoLongerDeadStores(signalAccess->var->name, indexOfStatementContainingSignalAccess);
    /*
     * After we have updated the liveness status (by removing previously dead assignments made live by the current signal access) for the accessed signal ident,
     * we can consider assignments to the currently accessed signal ident as dead again
     */
    markAccessedSignalPartsAsDead(signalAccess);
}

[[nodiscard]] bool DeadStoreEliminator::isAccessedVariablePartLive(const syrec::VariableAccess::ptr& signalAccess) const {
    const auto& currentLivenessStatusLookup = getLivenessStatusLookupForCurrentScope();
    if (!currentLivenessStatusLookup.has_value() || currentLivenessStatusLookup.value()->livenessStatusLookup.count(signalAccess->var->name) == 0 || !symbolTable->contains(signalAccess->var->name)) {
        return false;
    }

    const auto& symbolTableEntryForAccessedSignal = *symbolTable->getVariable(signalAccess->var->name);
    // TODO:
    if (!std::holds_alternative<syrec::Variable::ptr>(symbolTableEntryForAccessedSignal)) {
        return false;
    }
    
    const auto& backingVariableEntryForAccessedSignal = std::get<syrec::Variable::ptr>(*symbolTable->getVariable(signalAccess->var->name));
    const auto& livenessStatusOfVariable = currentLivenessStatusLookup.value()->livenessStatusLookup.at(signalAccess->var->name);
    return !livenessStatusOfVariable->tryFetchValueFor(
        transformUserDefinedDimensionAccess(backingVariableEntryForAccessedSignal->dimensions.size(), signalAccess->indexes),
        transformUserDefinedBitRangeAccess(backingVariableEntryForAccessedSignal->bitwidth, signalAccess->range))
    .has_value();
}

[[nodiscard]] std::vector<std::optional<unsigned int>> DeadStoreEliminator::transformUserDefinedDimensionAccess(std::size_t numDimensionsOfAccessedSignal, const std::vector<syrec::expression::ptr>& dimensionAccess) const {
    std::vector<std::optional<unsigned int>> transformedAccessOnDimension;
    std::transform(
            dimensionAccess.cbegin(),
            dimensionAccess.cend(),
            std::back_inserter(transformedAccessOnDimension),
            [&](const auto& accessedValueOfDimension) -> std::optional<unsigned int> {
                if (const auto& accessedValueOfDimensionAsNumericExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(accessedValueOfDimension); accessedValueOfDimensionAsNumericExpr != nullptr) {
                    return tryEvaluateNumber(accessedValueOfDimensionAsNumericExpr->value);   
                }
                return std::nullopt;
            });

    if (transformedAccessOnDimension.size() < numDimensionsOfAccessedSignal) {
        const auto numElementsToAppendToDefineFullDimensionAccess = numDimensionsOfAccessedSignal - transformedAccessOnDimension.size();
        for (auto i = 0; i < numElementsToAppendToDefineFullDimensionAccess; ++i) {
            transformedAccessOnDimension.emplace_back(std::nullopt);
        }
    }

    return transformedAccessOnDimension;
}

[[nodiscard]] std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess> DeadStoreEliminator::transformUserDefinedBitRangeAccess(unsigned int accessedSignalBitRange, const std::optional<std::pair<syrec::Number::ptr, syrec::Number::ptr>> & bitRangeAccess) const {
    if (!bitRangeAccess.has_value()) {
        return std::make_optional<optimizations::BitRangeAccessRestriction::BitRangeAccess>(0, accessedSignalBitRange - 1);
    }

    auto bitRangeStartEvaluated = tryEvaluateNumber(bitRangeAccess->first);
    auto bitRangeEndEvaluated   = tryEvaluateNumber(bitRangeAccess->second);
    if (!bitRangeStartEvaluated.has_value() || !bitRangeEndEvaluated.has_value()) {
        return std::make_optional(optimizations::BitRangeAccessRestriction::BitRangeAccess(0, accessedSignalBitRange - 1));
    }

    if (*bitRangeStartEvaluated > accessedSignalBitRange - 1) {
        bitRangeStartEvaluated = accessedSignalBitRange - 1;
    }
    if (*bitRangeEndEvaluated > accessedSignalBitRange - 1) {
        bitRangeEndEvaluated = accessedSignalBitRange - 1;
    }
    return std::make_optional(optimizations::BitRangeAccessRestriction::BitRangeAccess(*bitRangeStartEvaluated, *bitRangeEndEvaluated));
}

std::vector<DeadStoreEliminator::AssignmentStatementIndexInControlFlowGraph> DeadStoreEliminator::combineAndSortDeadRemainingDeadStores() {
    if (assignmentStmtIndizesPerSignal.empty()) {
        return {};   
    }

    std::vector<AssignmentStatementIndexInControlFlowGraph> deadStoreStatementIndizesInFlowGraph;
    for (const auto& [_, deadStores] : assignmentStmtIndizesPerSignal) {
        std::vector<AssignmentStatementIndexInControlFlowGraph> statementIndexPerDeadStore;
        std::transform(
        deadStores.cbegin(),
        deadStores.cend(),
        std::back_inserter(statementIndexPerDeadStore),
        [](const PotentiallyDeadAssignmentStatement& deadStore) {
            return deadStore.indexInControlFlowGraph;
        });
        deadStoreStatementIndizesInFlowGraph.insert(deadStoreStatementIndizesInFlowGraph.end(), statementIndexPerDeadStore.begin(), statementIndexPerDeadStore.end());
    }

    // Performs sort in ascending order of remaining dead stores based on the relative statement index in the control flow 
    std::sort(
    deadStoreStatementIndizesInFlowGraph.begin(),
    deadStoreStatementIndizesInFlowGraph.end(),
    [](const AssignmentStatementIndexInControlFlowGraph& thisElem, const AssignmentStatementIndexInControlFlowGraph& thatElem) {
        const auto& numElementsToCompare = std::min(thisElem.relativeStatementIndexPerControlBlock.size(), thatElem.relativeStatementIndexPerControlBlock.size());

        const auto& lastElementOfThisElem = std::next(thisElem.relativeStatementIndexPerControlBlock.cbegin(), numElementsToCompare);
        const auto& lastElementOfThatElem = std::next(thatElem.relativeStatementIndexPerControlBlock.cbegin(), numElementsToCompare);

        const auto& pairOfMismatchedElements = std::mismatch(
        thisElem.relativeStatementIndexPerControlBlock.cbegin(),
        lastElementOfThisElem,
        thatElem.relativeStatementIndexPerControlBlock.cbegin(),
        lastElementOfThatElem,
        [](const StatementIterationHelper::StatementIndexInBlock& operandOne, const StatementIterationHelper::StatementIndexInBlock& operandTwo) {
            const bool isCurrentBlockOfFirstOperandIfBranch = operandOne.blockType == StatementIterationHelper::BlockType::IfConditionTrueBranch || operandOne.blockType == StatementIterationHelper::BlockType::IfConditionFalseBranch;
            const bool isCurrentBlockOfSecondOperandIBranch = operandTwo.blockType == StatementIterationHelper::BlockType::IfConditionTrueBranch || operandTwo.blockType == StatementIterationHelper::BlockType::IfConditionFalseBranch;

            if (isCurrentBlockOfFirstOperandIfBranch && isCurrentBlockOfSecondOperandIBranch) {
                if (getBlockTypePrecedence(operandOne.blockType) > getBlockTypePrecedence(operandTwo.blockType)) {
                    return false;
                }
            }
            return operandOne.relativeIndexInBlock <= operandTwo.relativeIndexInBlock;
        });

        if (pairOfMismatchedElements.first == lastElementOfThisElem) {
            // Statements with smaller overall index in control flow graph are placed before statements with longer relative index in the control flow graph
            return thisElem.relativeStatementIndexPerControlBlock.size() <= thatElem.relativeStatementIndexPerControlBlock.size();
        }
        return false;

        //const auto& lastElementOfThisElem     = std::next(thisElem.relativeStatementIndexPerControlBlock.cbegin(), numElementsToCompare);
        //const auto& lastElementOfThatElem     = std::next(thatElem.relativeStatementIndexPerControlBlock.cbegin(), numElementsToCompare);

        //const auto& pairOfMismatchedElements = std::mismatch(
        //        thisElem.relativeStatementIndexPerControlBlock.cbegin(),
        //        lastElementOfThisElem,
        //        thatElem.relativeStatementIndexPerControlBlock.cbegin(),
        //        lastElementOfThatElem,
        //        [](const StatementIterationHelper::StatementIndexInBlock& operandOne, const StatementIterationHelper::StatementIndexInBlock& operandTwo) {
        //            return operandOne.relativeIndexInBlock == operandTwo.relativeIndexInBlock;
        //        });

        // If relative statement index per block matches, compare according to block type (which is necessary to guarantee correct ordering of if branch statements
        if (pairOfMismatchedElements.first == lastElementOfThisElem) {
            // Statements with smaller overall index in control flow graph are placed before statements with longer relative index in the control flow graph
            if (thisElem.relativeStatementIndexPerControlBlock.size() < thatElem.relativeStatementIndexPerControlBlock.size()) {
                return true;
            }

            const auto& pairOfMismatchedBlockTypes = std::mismatch(
            thisElem.relativeStatementIndexPerControlBlock.cbegin(),
            lastElementOfThisElem,
            thatElem.relativeStatementIndexPerControlBlock.cbegin(),
            lastElementOfThatElem,
            [](const StatementIterationHelper::StatementIndexInBlock& operandOne, const StatementIterationHelper::StatementIndexInBlock& operandTwo) {
                return operandOne.blockType == operandTwo.blockType;
            });

            /*
             * If every relative index per block matches between the two statements to compare, sort according to the block type precedence of the first mismatched block type 
             */
            if (pairOfMismatchedBlockTypes.first != lastElementOfThisElem) {
                return getBlockTypePrecedence(pairOfMismatchedBlockTypes.first->blockType) < getBlockTypePrecedence(pairOfMismatchedBlockTypes.second->blockType);
            }
        }
        return pairOfMismatchedElements.first->relativeIndexInBlock < pairOfMismatchedElements.second->relativeIndexInBlock;
    });

    return deadStoreStatementIndizesInFlowGraph;
}

void DeadStoreEliminator::markAccessedSignalsAsLiveInExpression(const syrec::expression::ptr& expr, const AssignmentStatementIndexInControlFlowGraph& indexOfStatementContainingExpression) {
    /* Numeric expression can be ignored since they either only define constants (accessing the bitwidth of a variable does also define a constant and its value is independent from the changes made during the execution of the program and thus does not update the liveness status of the accessed signal) or use loop variables
    */
    if (const auto& exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr); exprAsBinaryExpr != nullptr) {
        markAccessedSignalsAsLiveInExpression(exprAsBinaryExpr->lhs, indexOfStatementContainingExpression);
        markAccessedSignalsAsLiveInExpression(exprAsBinaryExpr->rhs, indexOfStatementContainingExpression);
    } else if (const auto& exprAsShiftExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr); exprAsShiftExpr != nullptr) {
        markAccessedSignalsAsLiveInExpression(exprAsShiftExpr->lhs, indexOfStatementContainingExpression);
    } else if (const auto& exprAsVariableExpr = std::dynamic_pointer_cast<syrec::VariableExpression>(expr); exprAsVariableExpr != nullptr) {
        markAccessedVariablePartsAsLive(exprAsVariableExpr->var, indexOfStatementContainingExpression);
    }
    // TODO: Another branch is needed if unary expression are supported by syrec
}

void DeadStoreEliminator::markAccessedSignalsAsLiveInCallStatement(const std::shared_ptr<syrec::CallStatement>& callStmt, const AssignmentStatementIndexInControlFlowGraph& indexOfCallStmt) {
    for (const auto& calleeArgument: callStmt->parameters) {
        const auto& signalTableEntryForAccessedSignal = symbolTable->getVariable(calleeArgument);
        if (!signalTableEntryForAccessedSignal.has_value() || !std::holds_alternative<syrec::Variable::ptr>(*signalTableEntryForAccessedSignal)) {
            continue;
        }
        auto variableAccessForCalleeArgument = std::make_shared<syrec::VariableAccess>();
        variableAccessForCalleeArgument->setVar(std::get<syrec::Variable::ptr>(signalTableEntryForAccessedSignal.value()));
        markAccessedVariablePartsAsLive(variableAccessForCalleeArgument, indexOfCallStmt);
    }
}

void DeadStoreEliminator::insertPotentiallyDeadAssignmentStatement(const syrec::VariableAccess::ptr& assignedToSignalParts, const std::vector<StatementIterationHelper::StatementIndexInBlock>& relativeIndexOfStatementInControlFlowGraph) {
    const auto& accessedSignalIdent = assignedToSignalParts->var->name;
    if (assignmentStmtIndizesPerSignal.count(accessedSignalIdent) == 0) {
        assignmentStmtIndizesPerSignal.insert(std::make_pair<std::string_view, std::vector<PotentiallyDeadAssignmentStatement>>(accessedSignalIdent, {}));
    }
    assignmentStmtIndizesPerSignal.at(accessedSignalIdent).emplace_back(PotentiallyDeadAssignmentStatement(assignedToSignalParts, relativeIndexOfStatementInControlFlowGraph));
}

void DeadStoreEliminator::removeNoLongerDeadStores(const std::string& accessedSignalIdent, const AssignmentStatementIndexInControlFlowGraph& indexOfStatementContainingSignalAccess) {
    if (assignmentStmtIndizesPerSignal.count(accessedSignalIdent) == 0 || !symbolTable->contains(accessedSignalIdent)) {
        return;
    }

    auto& potentiallyDeadStores = assignmentStmtIndizesPerSignal.at(accessedSignalIdent);
    potentiallyDeadStores.erase(
std::remove_if(
        potentiallyDeadStores.begin(),
        potentiallyDeadStores.end(),
        [&](const PotentiallyDeadAssignmentStatement& assignmentStmtInformation) {
            const auto& assignedToSignalParts = assignmentStmtInformation.assignedToSignalParts;
            const auto& currentLivenessStatusLookup = getLivenessStatusLookupForCurrentScope();
            if (!currentLivenessStatusLookup.has_value() || currentLivenessStatusLookup.value()->livenessStatusLookup.count(assignedToSignalParts->var->name) == 0) {
                return false;
            }
            
            const auto& signalTableEntryForAccessedSignal = symbolTable->getVariable(assignedToSignalParts->var->name);
            if (!signalTableEntryForAccessedSignal.has_value() || !std::holds_alternative<syrec::Variable::ptr>(*signalTableEntryForAccessedSignal)) {
                return false;
            }

            if (!isReachableInReverseControlFlowGraph(assignmentStmtInformation.indexInControlFlowGraph, indexOfStatementContainingSignalAccess)) {
                return false;
            }

            const auto& backingVariableEntryForAccessedSignal = std::get<syrec::Variable::ptr>(*symbolTable->getVariable(assignedToSignalParts->var->name));
            const auto& livenessStatusLookupForAssignedToSignal = std::static_pointer_cast<DeadStoreStatusLookup>(currentLivenessStatusLookup.value()->livenessStatusLookup.at(assignedToSignalParts->var->name));

            // TODO: Could we adapt the typedef for the pointer made in the base value lookup to be automatically cast to derived class (aka a polymorphic typedef)
            // This could then also be used syrec polymorphism (expression, etc).
            return !livenessStatusLookupForAssignedToSignal->areAccessedSignalPartsDead(
                    transformUserDefinedDimensionAccess(backingVariableEntryForAccessedSignal->dimensions.size(), assignedToSignalParts->indexes),
                    transformUserDefinedBitRangeAccess(backingVariableEntryForAccessedSignal->bitwidth, assignedToSignalParts->range)
            );
        }),
potentiallyDeadStores.end());

    if (potentiallyDeadStores.empty()) {
        assignmentStmtIndizesPerSignal.erase(accessedSignalIdent);
    }
}

void DeadStoreEliminator::markAccessedSignalPartsAsDead(const syrec::VariableAccess::ptr& signalAccess) const {
    const auto& currentLivenessStatusLookup = getLivenessStatusLookupForCurrentScope();
    if (!currentLivenessStatusLookup.has_value() || currentLivenessStatusLookup.value()->livenessStatusLookup.count(signalAccess->var->name) == 0) {
        return;
    }

    const auto& signalTableEntryForAccessedSignal = symbolTable->getVariable(signalAccess->var->name);
    if (!signalTableEntryForAccessedSignal.has_value() || !std::holds_alternative<syrec::Variable::ptr>(*signalTableEntryForAccessedSignal)) {
        return;
    }

    auto&       livenessStatusForAccessedVariable     = currentLivenessStatusLookup.value()->livenessStatusLookup.at(signalAccess->var->name);
    const auto& backingVariableEntryForAccessedSignal = std::get<syrec::Variable::ptr>(*symbolTable->getVariable(signalAccess->var->name));
    const auto& transformedDimensionAccess        = transformUserDefinedDimensionAccess(backingVariableEntryForAccessedSignal->dimensions.size(), signalAccess->indexes);
    const auto& transformedBitRangeAccess         = transformUserDefinedBitRangeAccess(backingVariableEntryForAccessedSignal->bitwidth, signalAccess->range);

    livenessStatusForAccessedVariable->liftRestrictionsOfDimensions(transformedDimensionAccess, transformedBitRangeAccess);
}

void DeadStoreEliminator::resetInternalData() {
    //livenessStatusLookup.clear();
    assignmentStmtIndizesPerSignal.clear();

    indexOfLastProcessedStatementInControlFlowGraph.reset();
    livenessStatusScopes.clear();
}

void DeadStoreEliminator::decrementReferenceCountOfUsedSignalsInStatement(const syrec::Statement::ptr& statement) const {
    /*
     * We do not have to decrement the reference counts of signals used in a loop statement since we are assuming that its body is already empty and
     * only constants or signal widths, which are already transformed to constants, can be used for which no reference counting is required
     */
    if (const auto& statementAsIfStatement = std::dynamic_pointer_cast<syrec::IfStatement>(statement); statementAsIfStatement != nullptr) {
        decrementReferenceCountsOfUsedSignalsInExpression(statementAsIfStatement->condition);
        decrementReferenceCountsOfUsedSignalsInExpression(statementAsIfStatement->fiCondition);
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

void DeadStoreEliminator::decrementReferenceCountsOfUsedSignalsInExpression(const syrec::expression::ptr& expr) const {
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
    symbolTable->decrementLiteralReferenceCount(accessedSignal->var->name);
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
        symbolTable->decrementLiteralReferenceCount(number->variableName());
    }
}

bool DeadStoreEliminator::isAssignmentDefinedInLoopPerformingMoreThanOneIteration() const {
    return !remainingNonControlFlowStatementsPerLoopBody.empty()
        && std::any_of(
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
    const auto& startValue = tryEvaluateNumber(loopStmt->range.first);
    const auto& endValue = tryEvaluateNumber(loopStmt->range.second);
    const auto& stepSize   = tryEvaluateNumber(loopStmt->step);

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

void DeadStoreEliminator::mergeLivenessStatusOfCurrentScopeWithParent() {
    const auto& livenessStatusLookup = getLivenessStatusLookupForCurrentScope();
    if (!livenessStatusLookup.has_value() || livenessStatusLookup.value()->livenessStatusLookup.size() <= 1) {
        return;
    }

    auto&       currentLivenessStatusScope = livenessStatusScopes.back();
    auto& parentScopeOfCurrentScope  = livenessStatusScopes.end()[-2];

    for (const auto& [signalIdent, livenessStatus] : currentLivenessStatusScope->livenessStatusLookup) {
        if (parentScopeOfCurrentScope->livenessStatusLookup.count(signalIdent) == 0) {
            parentScopeOfCurrentScope->livenessStatusLookup.insert(std::make_pair(signalIdent, livenessStatus));
        }
        else {
            auto& livenessStatusOfSignalInParentScope = parentScopeOfCurrentScope->livenessStatusLookup.at(signalIdent);
            const auto& livenessStatusOfSignalInCurrentScope = *currentLivenessStatusScope->livenessStatusLookup.at(signalIdent);
            livenessStatusOfSignalInParentScope->copyRestrictionsAndUnrestrictedValuesFrom(
                {},
                std::nullopt,
                {},
                std::nullopt,
                livenessStatusOfSignalInCurrentScope
            );
        }
    }
}

void DeadStoreEliminator::destroyCurrentLivenessStatusScope() {
    if (livenessStatusScopes.size() <= 1) {
        return;
    }
    mergeLivenessStatusOfCurrentScopeWithParent();
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

// TODO: Refactor by removing optional return type since we are always creating a new scope
std::optional<std::shared_ptr<DeadStoreEliminator::LivenessStatusLookupScope>> DeadStoreEliminator::getLivenessStatusLookupForCurrentScope() const {
    return livenessStatusScopes.empty() ? std::nullopt : std::make_optional(livenessStatusScopes.back());
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
