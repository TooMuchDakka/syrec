#include "core/syrec/parser/optimizations/deadStoreElimination/statement_iteration_helper.hpp"
#include "core/syrec/parser/optimizations/deadStoreElimination/dead_store_eliminator.hpp"

using namespace deadStoreElimination;

std::vector<DeadStoreEliminator::AssignmentStatementIndexInControlFlowGraph> DeadStoreEliminator::findDeadStores(const syrec::Statement::vec& statementList) {
    resetInternalData();
    const std::unique_ptr<StatementIterationHelper> statementIterationHelper = std::make_unique<StatementIterationHelper>(statementList);

    std::optional<StatementIterationHelper::StatementAndRelativeIndexPair> nextStatement = statementIterationHelper->getNextNonControlFlowStatement();
    while (nextStatement.has_value()) {
        if (const auto& nextStatementAsAssignmentStatement = std::dynamic_pointer_cast<syrec::AssignStatement>(nextStatement->statement); nextStatementAsAssignmentStatement != nullptr) {
            markAccessedVariablePartsAsLive(nextStatementAsAssignmentStatement->lhs);
            markAccessedSignalsAsLiveInExpression(nextStatementAsAssignmentStatement->rhs);
            if (!doesAssignmentContainPotentiallyUnsafeOperation(nextStatementAsAssignmentStatement)) {
                insertPotentiallyDeadAssignmentStatement(nextStatementAsAssignmentStatement->lhs, nextStatement->relativeIndexInControlFlowGraph);   
            }
        } else if (const auto& nextStatementAsUnaryAssignmentStatement = std::dynamic_pointer_cast<syrec::UnaryStatement>(nextStatement->statement); nextStatementAsUnaryAssignmentStatement != nullptr) {
            markAccessedVariablePartsAsLive(nextStatementAsUnaryAssignmentStatement->var);
            if (!doesAssignmentContainPotentiallyUnsafeOperation(nextStatementAsUnaryAssignmentStatement)) {
                insertPotentiallyDeadAssignmentStatement(nextStatementAsUnaryAssignmentStatement->var, nextStatement->relativeIndexInControlFlowGraph);   
            }
        } else if (const auto& nextStatementAsIfStatement = std::dynamic_pointer_cast<syrec::IfStatement>(nextStatement->statement); nextStatementAsIfStatement != nullptr) {
            markAccessedSignalsAsLiveInExpression(nextStatementAsIfStatement->condition);
        } else if (const auto& nextStatementAsCallStatement = std::dynamic_pointer_cast<syrec::CallStatement>(nextStatement->statement); nextStatementAsCallStatement != nullptr) {
            markAccessedSignalsAsLiveInCallStatement(nextStatementAsCallStatement);
        } else if (const auto& nextStatementAsSwapStatement = std::dynamic_pointer_cast<syrec::SwapStatement>(nextStatement->statement); nextStatementAsSwapStatement != nullptr) {
            markAccessedVariablePartsAsLive(nextStatementAsSwapStatement->lhs);
            markAccessedVariablePartsAsLive(nextStatementAsSwapStatement->rhs);
        }

        nextStatement.reset();
        if (const auto& nextFetchedStatement = statementIterationHelper->getNextNonControlFlowStatement(); nextFetchedStatement.has_value()) {
            nextStatement.emplace(*nextFetchedStatement);
        }
    }
    return combineAndSortDeadRemainingDeadStores();
}

// TODO: Implement me
void DeadStoreEliminator::removeDeadStoresFrom(syrec::Statement::vec& statementList, const std::vector<AssignmentStatementIndexInControlFlowGraph>& foundDeadStores) {
    return;
}


bool DeadStoreEliminator::doesAssignmentContainPotentiallyUnsafeOperation(const syrec::Statement::ptr& stmt) const {
    if (const auto& stmtAsUnaryAssignmentStmt = std::dynamic_pointer_cast<syrec::UnaryStatement>(stmt); stmtAsUnaryAssignmentStmt != nullptr) {
        return isAssignedToSignalAModifiableParameter(stmtAsUnaryAssignmentStmt->var->var->name)
            || wasSignalDeclaredAndAreAllIndizesOfSignalConstantsAndWithinRange(stmtAsUnaryAssignmentStmt->var);
    }
    if (const auto& stmtAsAssignmentStmt = std::dynamic_pointer_cast<syrec::AssignStatement>(stmt); stmtAsAssignmentStmt != nullptr) {
        return isAssignedToSignalAModifiableParameter(stmtAsAssignmentStmt->lhs->var->name)
            || wasSignalDeclaredAndAreAllIndizesOfSignalConstantsAndWithinRange(stmtAsAssignmentStmt->lhs)
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
        return doesExpressionContainPotentiallyUnsafeOperation(expr);
    }
    return false;
}

// We are assuming zero based indexing and are also checking further conditions required for a valid bit range access
// are the latter really necessary ?
bool DeadStoreEliminator::wasSignalDeclaredAndAreAllIndizesOfSignalConstantsAndWithinRange(const syrec::VariableAccess::ptr& signalAccess) const {
    const auto fetchedSymbolTableEntry = symbolTable->getVariable(signalAccess->var->name);
    if (!fetchedSymbolTableEntry.has_value()) {
        return false;
    } else if (std::holds_alternative<syrec::Number::ptr>(*fetchedSymbolTableEntry)) {
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

        bitRangeOK = bitRangeStartEvaluated.has_value() && bitRangeEndEvaluated.has_value() && *bitRangeStartEvaluated < maximumValidBitRangeIndex && *bitRangeEndEvaluated < maximumValidBitRangeIndex && *bitRangeStartEvaluated < *bitRangeEndEvaluated;
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

void DeadStoreEliminator::markAccessedVariablePartsAsLive(const syrec::VariableAccess::ptr& signalAccess) {
    if (livenessStatusLookup.count(signalAccess->var->name) == 0) {
        const auto& signalTableEntryForAccessedSignal = symbolTable->getVariable(signalAccess->var->name);
        if (!signalTableEntryForAccessedSignal.has_value() || !std::holds_alternative<syrec::Variable::ptr>(*signalTableEntryForAccessedSignal)) {
            return;
        }
        const auto& referencedSignalInVariableAccess = std::get<syrec::Variable::ptr>(*signalTableEntryForAccessedSignal);
        auto        livenessStatusLookupEntry        = std::make_shared<DeadStoreStatusLookup>(
                referencedSignalInVariableAccess->bitwidth,
                referencedSignalInVariableAccess->dimensions,
                std::make_optional(false));
        livenessStatusLookupEntry->invalidateAllStoredValuesForSignal();
        livenessStatusLookup.insert(std::make_pair(signalAccess->var->name, livenessStatusLookupEntry));
    }

    auto&       livenessStatusForAccessedVariable = livenessStatusLookup.at(signalAccess->var->name);
    const auto& backingVariableEntryForAccessedSignal = std::get<syrec::Variable::ptr>(*symbolTable->getVariable(signalAccess->var->name));
    livenessStatusForAccessedVariable->liftRestrictionsOfDimensions(
        transformUserDefinedDimensionAccess(backingVariableEntryForAccessedSignal->dimensions.size(), signalAccess->indexes),
        transformUserDefinedBitRangeAccess(backingVariableEntryForAccessedSignal->bitwidth, signalAccess->range)
    );
    removeNoLongerDeadStores(signalAccess->var->name);
    /*
     * After we have updated the liveness status (by removing previously dead assignments made live by the current signal access) for the accessed signal ident,
     * we can consider assignments to the currently accessed signal ident as dead again
     */
    markAccessedSignalPartsAsDead(signalAccess);
}

[[nodiscard]] bool DeadStoreEliminator::isAccessedVariablePartLive(const syrec::VariableAccess::ptr& signalAccess) {
    if (livenessStatusLookup.count(signalAccess->var->name) == 0 || !symbolTable->contains(signalAccess->var->name)) {
        return false;
    }

    const auto& symbolTableEntryForAccessedSignal = *symbolTable->getVariable(signalAccess->var->name);
    // TODO:
    if (!std::holds_alternative<syrec::Variable::ptr>(symbolTableEntryForAccessedSignal)) {
        return false;
    }
    
    const auto& backingVariableEntryForAccessedSignal = std::get<syrec::Variable::ptr>(*symbolTable->getVariable(signalAccess->var->name));
    const auto& livenessStatusOfVariable = livenessStatusLookup.at(signalAccess->var->name);
    return livenessStatusOfVariable->tryFetchValueFor(
        transformUserDefinedDimensionAccess(backingVariableEntryForAccessedSignal->dimensions.size(), signalAccess->indexes), 
        transformUserDefinedBitRangeAccess(backingVariableEntryForAccessedSignal->bitwidth, signalAccess->range)
    ).has_value();
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
        if (thisElem.relativeStatementIndexPerControlBlock.size() != thatElem.relativeStatementIndexPerControlBlock.size()) {
            return thisElem.relativeStatementIndexPerControlBlock.size() < thatElem.relativeStatementIndexPerControlBlock.size();
        }

        const auto& firstMissmatchedRelativeIndexForAnyBlock = std::mismatch(
                thisElem.relativeStatementIndexPerControlBlock.cbegin(),
                thisElem.relativeStatementIndexPerControlBlock.cend(),
                thatElem.relativeStatementIndexPerControlBlock.cbegin(),
                thatElem.relativeStatementIndexPerControlBlock.cend()
        );

        if (firstMissmatchedRelativeIndexForAnyBlock.first == thisElem.relativeStatementIndexPerControlBlock.cend()) {
            return true;
        }
        return *firstMissmatchedRelativeIndexForAnyBlock.first < *firstMissmatchedRelativeIndexForAnyBlock.second;
    });

    return deadStoreStatementIndizesInFlowGraph;
}


void DeadStoreEliminator::markAccessedSignalsAsLiveInExpression(const syrec::expression::ptr& expr) {
    /* Numeric expression can be ignored since they either only define constants (accessing the bitwidth of a variable does also define a constant and its value is independent from the changes made during the execution of the program and thus does not update the liveness status of the accessed signal) or use loop variables
    */
    if (const auto& exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr); exprAsBinaryExpr != nullptr) {
        markAccessedSignalsAsLiveInExpression(exprAsBinaryExpr->lhs);
        markAccessedSignalsAsLiveInExpression(exprAsBinaryExpr->rhs);
    } else if (const auto& exprAsShiftExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr); exprAsShiftExpr != nullptr) {
        markAccessedSignalsAsLiveInExpression(exprAsShiftExpr->lhs);
    } else if (const auto& exprAsVariableExpr = std::dynamic_pointer_cast<syrec::VariableExpression>(expr); exprAsVariableExpr != nullptr) {
        markAccessedVariablePartsAsLive(exprAsVariableExpr->var);
    }
    // TODO: Another branch is needed if unary expression are supported by syrec
}

void DeadStoreEliminator::markAccessedSignalsAsLiveInCallStatement(const std::shared_ptr<syrec::CallStatement>& callStmt) {
    for (const auto& calleeArgument: callStmt->parameters) {
        const auto& signalTableEntryForAccessedSignal = symbolTable->getVariable(calleeArgument);
        if (!signalTableEntryForAccessedSignal.has_value() || !std::holds_alternative<syrec::Variable::ptr>(*signalTableEntryForAccessedSignal)) {
            continue;
        }
        auto variableAccessForCalleeArgument = std::make_shared<syrec::VariableAccess>();
        variableAccessForCalleeArgument->setVar(std::get<syrec::Variable::ptr>(signalTableEntryForAccessedSignal.value()));
        markAccessedVariablePartsAsLive(variableAccessForCalleeArgument);
    }
}

void DeadStoreEliminator::insertPotentiallyDeadAssignmentStatement(const syrec::VariableAccess::ptr& assignedToSignalParts, const std::vector<std::size_t>& relativeIndexOfStatementInControlFlowGraph) {
    const auto& accessedSignalIdent = assignedToSignalParts->var->name;
    if (assignmentStmtIndizesPerSignal.count(accessedSignalIdent) == 0) {
        assignmentStmtIndizesPerSignal.insert(std::make_pair<std::string_view, std::vector<PotentiallyDeadAssignmentStatement>>(accessedSignalIdent, {}));
    }
    assignmentStmtIndizesPerSignal.at(accessedSignalIdent).emplace_back(PotentiallyDeadAssignmentStatement(assignedToSignalParts, relativeIndexOfStatementInControlFlowGraph));
}

void DeadStoreEliminator::removeNoLongerDeadStores(const std::string& accessedSignalIdent) {
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
            if (livenessStatusLookup.count(assignedToSignalParts->var->name) == 0) {
                return false;
            }
            
            const auto& signalTableEntryForAccessedSignal = symbolTable->getVariable(assignedToSignalParts->var->name);
            if (!signalTableEntryForAccessedSignal.has_value() || !std::holds_alternative<syrec::Variable::ptr>(*signalTableEntryForAccessedSignal)) {
                return false;
            }

            const auto& backingVariableEntryForAccessedSignal = std::get<syrec::Variable::ptr>(*symbolTable->getVariable(assignedToSignalParts->var->name));
            const auto& livenessStatusLookupForAssignedToSignal                  = std::static_pointer_cast<DeadStoreStatusLookup>(livenessStatusLookup.at(assignedToSignalParts->var->name));

            // TODO: Could we adapt the typedef for the pointer made in the base value lookup to be automatically cast to derived class (aka a polymorphic typedef)
            // This could then also be used syrec polymorphism (expression, etc).
            return livenessStatusLookupForAssignedToSignal->areAccessedSignalPartsDead(
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
    if (livenessStatusLookup.count(signalAccess->var->name) == 0) {
        return;
    }

    const auto& signalTableEntryForAccessedSignal = symbolTable->getVariable(signalAccess->var->name);
    if (!signalTableEntryForAccessedSignal.has_value() || !std::holds_alternative<syrec::Variable::ptr>(*signalTableEntryForAccessedSignal)) {
        return;
    }

    auto&       livenessStatusForAccessedVariable = livenessStatusLookup.at(signalAccess->var->name);
    const auto& backingVariableEntryForAccessedSignal = std::get<syrec::Variable::ptr>(*symbolTable->getVariable(signalAccess->var->name));
    const auto& transformedDimensionAccess        = transformUserDefinedDimensionAccess(backingVariableEntryForAccessedSignal->dimensions.size(), signalAccess->indexes);
    const auto& transformedBitRangeAccess         = transformUserDefinedBitRangeAccess(backingVariableEntryForAccessedSignal->bitwidth, signalAccess->range);

    if (transformedBitRangeAccess.has_value()) {
        livenessStatusForAccessedVariable->invalidateStoredValueForBitrange(transformedDimensionAccess, *transformedBitRangeAccess);    
    } else {
        livenessStatusForAccessedVariable->invalidateStoredValueFor(transformedDimensionAccess);
    }
}

void DeadStoreEliminator::resetInternalData() {
    livenessStatusLookup.clear();
    assignmentStmtIndizesPerSignal.clear();
}