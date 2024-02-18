#include "core/syrec/parser/module_call_guess.hpp"
#include "core/syrec/parser/operation.hpp"
#include "core/syrec/parser/optimizations/deadStoreElimination/dead_store_eliminator.hpp"
#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/main_additional_line_for_assignment_simplifier.hpp"
#include "core/syrec/parser/optimizations/reassociate_expression.hpp"
#include "core/syrec/parser/optimizations/optimizer.hpp"

#include "core/syrec/parser/expression_comparer.hpp"
#include "core/syrec/parser/range_check.hpp"
#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/assignment_without_additional_lines_simplifier.hpp"
#include "core/syrec/parser/utils/binary_expression_simplifier.hpp"
#include "core/syrec/parser/utils/bit_helpers.hpp"
#include "core/syrec/parser/utils/copy_utils.hpp"
#include "core/syrec/parser/utils/dangerous_component_check_utils.hpp"
#include "core/syrec/parser/utils/loop_range_utils.hpp"

#include <functional>
#include <unordered_set>

// TODO: Refactoring if case switches with visitor pattern as its available in std::visit call
optimizations::Optimizer::OptimizationResult<syrec::Module> optimizations::Optimizer::optimizeProgram(const std::vector<std::reference_wrapper<const syrec::Module>>& modules, const std::string_view& defaultMainModuleIdent) {
    std::vector<std::unique_ptr<syrec::Module>> optimizedModules;
    bool                                        optimizedAnyModule = false;

    const auto& identifiedMainModuleSignature = determineActualMainModuleName(modules, defaultMainModuleIdent);
    if (!identifiedMainModuleSignature.has_value()) {
        return OptimizationResult<syrec::Module>::asUnchangedOriginal();
    }

    std::vector<std::size_t> indicesOfUnoptimizedModuleCounterPart;
    std::size_t              moduleIdx = 0;
    for (const auto& module: modules) {
        resetInternalNestingLevelsForIfAndLoopStatements();
        auto&&     optimizedModule          = handleModule(module, *identifiedMainModuleSignature);
        const auto optimizationResultStatus = optimizedModule.getStatusOfResult();
        optimizedAnyModule                  |= optimizationResultStatus != OptimizationResultFlag::IsUnchanged;

        if (optimizationResultStatus == OptimizationResultFlag::WasOptimized) {
            if (auto optimizedModuleData = optimizedModule.tryTakeOwnershipOfOptimizationResult(); optimizedModuleData.has_value()) {
                optimizedModules.emplace_back(std::move(optimizedModuleData->front()));
                indicesOfUnoptimizedModuleCounterPart.emplace_back(moduleIdx);
            }
        } else if (optimizedModule.getStatusOfResult() == OptimizationResultFlag::IsUnchanged) {
            optimizedModules.emplace_back(copyUtils::createCopyOfModule(module));
        }
        moduleIdx++;
    }

    if (parserConfig.deadCodeEliminationEnabled && (optimizedAnyModule ? !optimizedModules.empty() : true)) {
        if (optimizedAnyModule) {
            std::vector<std::reference_wrapper<const syrec::Module>> unoptimizedModuleReferences;
            unoptimizedModuleReferences.reserve(indicesOfUnoptimizedModuleCounterPart.size());

            for (const auto& idx : indicesOfUnoptimizedModuleCounterPart) {
                unoptimizedModuleReferences.emplace_back(modules.at(idx));
            }

            OptimizationResult<syrec::Module> resultContainerOfNotRemovedModules      = removeUnusedModulesFrom(createCallSignaturesFrom(unoptimizedModuleReferences), optimizedModules, *identifiedMainModuleSignature);
            if (resultContainerOfNotRemovedModules.getStatusOfResult() == OptimizationResultFlag::WasOptimized) {
                if (auto resultOfRemovedUnusedModules = resultContainerOfNotRemovedModules.tryTakeOwnershipOfOptimizationResult(); resultOfRemovedUnusedModules.has_value()) {
                    return OptimizationResult<syrec::Module>::fromOptimizedContainer(std::move(*resultOfRemovedUnusedModules));
                }
                // This case should not happen and the result is a fallback value
                return OptimizationResult<syrec::Module>::asUnchangedOriginal();
            }
            // This case should not happen since their must exist at least one module to be a valid SyRec program and thus the result is a fallback value
            if (resultContainerOfNotRemovedModules.getStatusOfResult() == OptimizationResultFlag::RemovedByOptimization) {
                return OptimizationResult<syrec::Module>::asUnchangedOriginal();
            }
        }
        else {
            OptimizationResult<syrec::Module> resultContainerOfNotRemovedModules = removeUnusedModulesFrom(modules, *identifiedMainModuleSignature);
            if (resultContainerOfNotRemovedModules.getStatusOfResult() == OptimizationResultFlag::WasOptimized) {
                if (auto resultOfRemovedUnusedModules = resultContainerOfNotRemovedModules.tryTakeOwnershipOfOptimizationResult(); resultOfRemovedUnusedModules.has_value()) {
                    return OptimizationResult<syrec::Module>::fromOptimizedContainer(std::move(*resultOfRemovedUnusedModules));
                }
            }
        }
    }
    return optimizedAnyModule ? OptimizationResult<syrec::Module>::fromOptimizedContainer(std::move(optimizedModules)) : OptimizationResult<syrec::Module>::asUnchangedOriginal();
}

optimizations::Optimizer::OptimizationResult<syrec::Module> optimizations::Optimizer::handleModule(const syrec::Module& module, const optimizations::Optimizer::ExpectedMainModuleCallSignature& expectedMainModuleSignature) {
    auto copyOfModule = copyUtils::createCopyOfModule(module);

    std::size_t generatedInternalIdForModule = 0;
    activeSymbolTableScope->addEntry(*copyOfModule, &generatedInternalIdForModule);
    internalIdentifierOfCurrentlyProcessedModule.internalModuleId = generatedInternalIdForModule;
    internalIdentifierOfCurrentlyProcessedModule.moduleIdent      = module.name;

    openNewSymbolTableScope();
    const auto& activeSymbolTableScopeAsChildOfGlobalOne = *getActiveSymbolTableScope();
    createSymbolTableEntriesForModuleParametersAndLocalVariables(*activeSymbolTableScopeAsChildOfGlobalOne, module);

    /*
     * Since the optimizer for an assignment without additional lines needs the symbol table information of the module in which the assignment is defined, we need to redefine
     * it once after the symbol table information for the current module was generated
     */
    if (assignmentWithoutAdditionalLineSynthesisOptimizer) {
        assignmentWithoutAdditionalLineSynthesisOptimizer->defineSymbolTable(activeSymbolTableScopeAsChildOfGlobalOne);
        // Since the generated signal names for any replacements generated by this optimizer can be reused per module, we will reload/reset by calling the corresponding function in the optimizer
        assignmentWithoutAdditionalLineSynthesisOptimizer->reloadGenerateableReplacementSignalName();    
    }

    bool wasAnyStatementInModuleBodyOptimized = false;
    if (auto optimizationResultOfModuleStatements = handleStatements(transformCollectionOfSharedPointersToReferences(module.statements)); optimizationResultOfModuleStatements.getStatusOfResult() != OptimizationResultFlag::IsUnchanged) {
        if (optimizationResultOfModuleStatements.getStatusOfResult() == OptimizationResultFlag::WasOptimized) {
           if (auto&& resultOfModuleBodyOptimization = optimizationResultOfModuleStatements.tryTakeOwnershipOfOptimizationResult(); resultOfModuleBodyOptimization.has_value()) {
               copyOfModule->statements = createStatementListFrom({}, std::move(*resultOfModuleBodyOptimization));
           }   
        }
        else {
            copyOfModule->statements.clear();
        }
        wasAnyStatementInModuleBodyOptimized = true;
    }
    
    /*
     * Since the optional optimization 'noAdditionalLineSynthesis' could lead to new local signals being added to the current module (and also being removed again by other optimizations), we need to reload the latter if said optimization is enabled
     */
    if (parserConfig.noAdditionalLineOptimizationConfig.has_value() && parserConfig.noAdditionalLineOptimizationConfig->optionalNewReplacementSignalIdentsPrefix.has_value()) {
        if (const std::optional<std::vector<syrec::Variable::ptr>> reloadedLocalSignalsOfCurrentModule = activeSymbolTableScopeAsChildOfGlobalOne->getLocalSignalsOfModule(internalIdentifierOfCurrentlyProcessedModule); reloadedLocalSignalsOfCurrentModule.has_value()) {
            copyOfModule->variables = *reloadedLocalSignalsOfCurrentModule;
        }
        else {
            closeActiveSymbolTableScope();
            return OptimizationResult<syrec::Module>::asUnchangedOriginal();
        }
    }

    const auto& numberOfDeclaredModuleStatements = copyOfModule->statements.size();
    if (parserConfig.deadStoreEliminationEnabled && !copyOfModule->statements.empty()) {
        const auto deadStoreEliminator = std::make_unique<deadStoreElimination::DeadStoreEliminator>(activeSymbolTableScopeAsChildOfGlobalOne);
        deadStoreEliminator->removeDeadStoresFrom(copyOfModule->statements);
    }
    wasAnyStatementInModuleBodyOptimized |= numberOfDeclaredModuleStatements != copyOfModule->statements.size();

    const auto& callSignatureOfModule     = createCallSignaturesFrom({*copyOfModule});
    const auto& isCurrentModuleMainModule = doesModuleMatchMainModuleSignature(callSignatureOfModule.front(), expectedMainModuleSignature);

    bool wereAnyParametersOrLocalSignalsRemoved = false;
    if (parserConfig.deadCodeEliminationEnabled) {
        const auto& indicesOfRemovedLocalVariablesOfModule = activeSymbolTableScopeAsChildOfGlobalOne->fetchUnusedLocalModuleVariableIndicesAndRemoveFromSymbolTable(parser::SymbolTable::ModuleIdentifier({module.name, generatedInternalIdForModule}));
        removeElementsAtIndices(copyOfModule->variables, indicesOfRemovedLocalVariablesOfModule);

        const auto& indicesOfUnusedParameters = activeSymbolTableScopeAsChildOfGlobalOne->updateOptimizedModuleSignatureByMarkingAndReturningIndicesOfUnusedParameters(parser::SymbolTable::ModuleIdentifier({module.name, generatedInternalIdForModule}));
        removeElementsAtIndices(copyOfModule->parameters, indicesOfUnusedParameters);

        wereAnyParametersOrLocalSignalsRemoved = !indicesOfRemovedLocalVariablesOfModule.empty() || !indicesOfUnusedParameters.empty();
    }
    /*
     * TODO: Check this assumption.
     * Dead code elimination should have already removed calls/uncalls to modules with read-only parameters or without assignments to local signals and modifiable parameters,
     * so no update of the reference counts for said calls/uncalls should be required here.
     *
     * Additionally, we can leave the symbol table entry of the module unchanged as the declared module signature should not change while the internal optimized module signature is already fixed
     * beforehand while determining the unused parameters.
     */ 
    closeActiveSymbolTableScope();
    
     /*
     * If we can determine that the main module only contains read-only parameters or does not perform assignments to either local signals and modifiable parameters,
     * the former can be removed but since the program must consist of some module, the main module will be stripped of all parameters, local signals as well as any
     * declared statements and its module body will only consist of a signal skip statement (which is required so that the module matches the SyReC grammar).
     */
    const auto& globalSymbolTableScope = *getActiveSymbolTableScope();
    
    /*
     * If we can determine that no assignments to modifiable parameters or any potentially 'dangerous' statement was defined in the optimized module body
     * we can not only clear the current module body (in case the current module is the 'main' module) otherwise, the module can be optimized away completely.
     * Additionally, we can mark all variables as optimized away and and clear all local variables of the module.
     *
     * Dead code elimination should already take care to remove any dead code from the module body (e.g. calls to modules with no changes to modifiable parameters, etc.)
     * and since no global variables exist, no reference count updates of any signals used in any statement of the module body need to be applied
     */
    const auto canModuleBodyBeCleared = parserConfig.deadCodeEliminationEnabled && !(dangerousComponentCheckUtils::doesOptimizedModuleBodyContainAssignmentToModifiableParameter(*copyOfModule, *globalSymbolTableScope) || dangerousComponentCheckUtils::doesModuleContainPotentiallyDangerousComponent(*copyOfModule, *globalSymbolTableScope));
    if (canModuleBodyBeCleared) {
        const auto& idOfCurrentModule = parser::SymbolTable::ModuleIdentifier({module.name, generatedInternalIdForModule});
        globalSymbolTableScope->changeStatementsOfModule(idOfCurrentModule, {});
        if (!copyOfModule->variables.empty() || !copyOfModule->parameters.empty()) {
            for (const auto& nowUnusedLocalVariable : copyOfModule->variables) {
                globalSymbolTableScope->markModuleVariableAsUnused(idOfCurrentModule, nowUnusedLocalVariable->name, false);
            }

            for (const auto& nowUnusedModuleParameter: copyOfModule->parameters) {
                globalSymbolTableScope->markModuleVariableAsUnused(idOfCurrentModule, nowUnusedModuleParameter->name, true);
            }

            copyOfModule->variables.clear();
            copyOfModule->parameters.clear();
            wereAnyParametersOrLocalSignalsRemoved = true;
        }

        if (!isCurrentModuleMainModule) {
            return OptimizationResult<syrec::Module>::asOptimizedAwayContainer();    
        }
        copyOfModule->statements.clear();
        wasAnyStatementInModuleBodyOptimized = true;
    }

    /*
     * As defined in the SyReC grammar, a module body cannot be empty (if the dead code elimination is not enabled) and will get a single SKIP statement added. Since the main module cannot be
     * removed by the dead code elimination, a valid SyReC module needs at least one module defined, so this case will also be checked here.
     */
    if (copyOfModule->statements.empty() && (!parserConfig.deadCodeEliminationEnabled || isCurrentModuleMainModule)) {
        const auto& generatedSkipStmtForMainModuleWithEmptyBody = std::make_shared<syrec::SkipStatement>();
        copyOfModule->statements.emplace_back(generatedSkipStmtForMainModuleWithEmptyBody);
    }

    if (!wasAnyStatementInModuleBodyOptimized && !wereAnyParametersOrLocalSignalsRemoved) {
        return OptimizationResult<syrec::Module>::asUnchangedOriginal();
    }
    
    globalSymbolTableScope->changeStatementsOfModule(parser::SymbolTable::ModuleIdentifier({module.name, generatedInternalIdForModule}), copyOfModule->statements);
    return OptimizationResult<syrec::Module>::fromOptimizedContainer(std::move(copyOfModule));
}

optimizations::Optimizer::OptimizationResult<syrec::Statement> optimizations::Optimizer::handleStatements(const std::vector<std::reference_wrapper<const syrec::Statement>>& statements) {
    std::vector<std::unique_ptr<syrec::Statement>> optimizedStmts;
    bool optimizedAnyStmt = false;
    for (const auto& stmt: statements) {
        if (auto simplificationResultOfStatement = handleStatement(stmt); simplificationResultOfStatement.getStatusOfResult() != OptimizationResultFlag::IsUnchanged) {
            optimizedAnyStmt = true;
            if (auto generatedSubStatements = simplificationResultOfStatement.tryTakeOwnershipOfOptimizationResult(); generatedSubStatements.has_value()) {
                for (auto&& optimizedStmt : *generatedSubStatements) {
                    optimizedStmts.emplace_back(std::move(optimizedStmt));
                }
            }
        }
        else {
            optimizedStmts.emplace_back(copyUtils::createCopyOfStmt(stmt));
        }
    }

    if (optimizedAnyStmt) {
        return optimizedStmts.empty()
            ? OptimizationResult<syrec::Statement>::asOptimizedAwayContainer()
            : OptimizationResult<syrec::Statement>::fromOptimizedContainer(std::move(optimizedStmts));
    }
    return OptimizationResult<syrec::Statement>::asUnchangedOriginal();
}

optimizations::Optimizer::OptimizationResult<syrec::Statement> optimizations::Optimizer::handleStatement(const syrec::Statement& stmt) {
    std::optional<OptimizationResult<syrec::Statement>> optimizationResultContainerOfStmt;
    if (typeid(stmt) == typeid(syrec::SkipStatement)) {
        return handleSkipStmt();
    }
    if (const auto& statementAsAssignmentStmt = dynamic_cast<const syrec::AssignStatement*>(&stmt); statementAsAssignmentStmt != nullptr) {
        return handleAssignmentStmt(*statementAsAssignmentStmt);
    }
    if (const auto& statementAsUnaryAssignmentStmt = dynamic_cast<const syrec::UnaryStatement*>(&stmt); statementAsUnaryAssignmentStmt != nullptr) {
        return handleUnaryStmt(*statementAsUnaryAssignmentStmt);
    }
    if (const auto& statementAsIfStatement = dynamic_cast<const syrec::IfStatement*>(&stmt); statementAsIfStatement != nullptr) {
        return handleIfStmt(*statementAsIfStatement);
    }
    if (const auto& statementAsLoopStatement = dynamic_cast<const syrec::ForStatement*>(&stmt); statementAsLoopStatement != nullptr) {
        return handleLoopStmt(*statementAsLoopStatement);
    }
    if (const auto& statementAsSwapStatement = dynamic_cast<const syrec::SwapStatement*>(&stmt); statementAsSwapStatement != nullptr) {
        return handleSwapStmt(*statementAsSwapStatement);
    }
    if (const auto& statementAsCallStatement = dynamic_cast<const syrec::CallStatement*>(&stmt); statementAsCallStatement != nullptr) {
        return handleCallStmt(*statementAsCallStatement);
    }
    if (const auto& statementAsUncallStatement = dynamic_cast<const syrec::UncallStatement*>(&stmt); statementAsUncallStatement != nullptr) {
        return handleUncallStmt(*statementAsUncallStatement);
    }
    return OptimizationResult<syrec::Statement>::asUnchangedOriginal();
}

optimizations::Optimizer::OptimizationResult<syrec::Statement> optimizations::Optimizer::handleAssignmentStmt(const syrec::AssignStatement& assignmentStmt) {
    syrec::VariableAccess::ptr lhsOperand = assignmentStmt.lhs;
    if (auto simplificationResultOfAssignedToSignal = handleSignalAccess(*assignmentStmt.lhs, false, nullptr); simplificationResultOfAssignedToSignal.getStatusOfResult() != OptimizationResultFlag::IsUnchanged) {
        auto lhsSignalAccessAfterOptimization = std::move(simplificationResultOfAssignedToSignal.tryTakeOwnershipOfOptimizationResult()->front());
        lhsOperand = std::make_shared<syrec::VariableAccess>(*lhsSignalAccessAfterOptimization);    
    }
    
    syrec::Expression::ptr rhsOperand = assignmentStmt.rhs;
    if (auto simplificationResultOfRhsExpr = handleExpr(*assignmentStmt.rhs); simplificationResultOfRhsExpr.getStatusOfResult() != OptimizationResultFlag::IsUnchanged) {
        rhsOperand = std::move(simplificationResultOfRhsExpr.tryTakeOwnershipOfOptimizationResult()->front());
    }
    
    const auto& rhsOperandEvaluatedAsConstant = tryEvaluateExpressionToConstant(*rhsOperand, getActiveSymbolTableForEvaluation(), parserConfig.constantPropagationEnabled, evaluableLoopVariableLookup, nullptr);
    const auto& mappedToAssignmentOperationFromFlag = syrec_operation::tryMapAssignmentOperationFlagToEnum(assignmentStmt.op);
    const auto  doesAssignmentNotModifyAssignedToSignalValue = mappedToAssignmentOperationFromFlag.has_value() && rhsOperandEvaluatedAsConstant.has_value() && syrec_operation::isOperandUseAsRhsInOperationIdentityElement(*mappedToAssignmentOperationFromFlag, *rhsOperandEvaluatedAsConstant);

    bool skipCheckForSplitOfAssignmentInSubAssignments = false;
    if (doesAssignmentNotModifyAssignedToSignalValue && parserConfig.deadCodeEliminationEnabled) {
        /*
         * If the rhs operand of the assignment evaluates to a constant that will not result in a change of the value of the assigned to signal can only be removed if
         * the assigned to signal access does not contain any non-compile time constant expressions in any of the signal access components.
         */
        if (dangerousComponentCheckUtils::doesSignalAccessContainPotentiallyDangerousComponent(*lhsOperand)) {
            skipCheckForSplitOfAssignmentInSubAssignments = true;
        }
        else {
            updateReferenceCountOf(lhsOperand->var->name, parser::SymbolTable::ReferenceCountUpdate::Decrement);
            return OptimizationResult<syrec::Statement>::asOptimizedAwayContainer();   
        }
    }

    if (!skipCheckForSplitOfAssignmentInSubAssignments) {
        skipCheckForSplitOfAssignmentInSubAssignments |= !parserConfig.noAdditionalLineOptimizationConfig.has_value() || rhsOperandEvaluatedAsConstant.has_value();
    }

    std::optional<unsigned int>                manuallySetMappedToAssignmentOperationFlag;
    const std::optional<EvaluatedSignalAccess> evaluatedSignalAccessOfAssignedToSignal = !doesAssignmentNotModifyAssignedToSignalValue ? tryEvaluateDefinedSignalAccess(*lhsOperand) : std::nullopt;
    if (!evaluatedSignalAccessOfAssignedToSignal.has_value()) {
        /*
         * If the optimized rhs expr defined the identity element of the given assignment operation while the unoptimized rhs expr did not evaluate to a constant
         * and the dead code elimination optimization is not being enabled, we will create an assignment with the optimized rhs expr. Otherwise, the assignment is left unchanged.
         */
        if (const std::shared_ptr<syrec::NumericExpression>& rhsOperandAsNumericExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(assignmentStmt.rhs); (!rhsOperandAsNumericExpr 
            || !rhsOperandAsNumericExpr->value->isConstant()) && rhsOperandEvaluatedAsConstant.has_value()) {
            if (std::unique_ptr<syrec::AssignStatement> simplifiedAssignment = std::make_unique<syrec::AssignStatement>(lhsOperand, assignmentStmt.op, rhsOperand); simplifiedAssignment) {
                return OptimizationResult<syrec::Statement>::fromOptimizedContainer(std::move(simplifiedAssignment));
            }
        }

        // TODO: This premature exit could lead to problems but since the error handling in the optimizer is work in progress it is ok for now.
        return OptimizationResult<syrec::Statement>::asUnchangedOriginal();
    }

    if (auto activeSymbolTableScope = getActiveSymbolTableScope(); activeSymbolTableScope.has_value()) {
        if (!mappedToAssignmentOperationFromFlag.has_value()) {
            invalidateValueOfAccessedSignalParts(*evaluatedSignalAccessOfAssignedToSignal);
        } else {
            // Try to replace an assignment of the form a += ... to a ^= ... of the symbol table entry for 'a' has the value 0 if the operation strength reduction is enabled.
            if (const std::optional<unsigned int> fetchedValueOfAssignedToSignal = tryFetchValueFromEvaluatedSignalAccess(*evaluatedSignalAccessOfAssignedToSignal); fetchedValueOfAssignedToSignal.has_value() && !*fetchedValueOfAssignedToSignal 
                    && *mappedToAssignmentOperationFromFlag == syrec_operation::operation::AddAssign && parserConfig.operationStrengthReductionEnabled) {
                manuallySetMappedToAssignmentOperationFlag = syrec_operation::tryMapAssignmentOperationEnumToFlag(syrec_operation::operation::XorAssign);
            }

            /*
            * If the value of the rhs expr does not modify the value of the assigned to signal but no dead code elimination is enabled, we can skip the application of the assignment
            */
            if (!doesAssignmentNotModifyAssignedToSignalValue) {
                if (!skipCheckForSplitOfAssignmentInSubAssignments && assignmentWithoutAdditionalLineSynthesisOptimizer) {
                    std::unique_ptr<syrec::AssignStatement> assignmentStmtToSimplify;
                    if (lhsOperand != assignmentStmt.lhs || rhsOperand != assignmentStmt.rhs) {
                        assignmentStmtToSimplify = std::make_unique<syrec::AssignStatement>(lhsOperand, manuallySetMappedToAssignmentOperationFlag.value_or(assignmentStmt.op), rhsOperand);
                    } else {
                        assignmentStmtToSimplify = std::make_unique<syrec::AssignStatement>(assignmentStmt.lhs, manuallySetMappedToAssignmentOperationFlag.value_or(assignmentStmt.op), assignmentStmt.rhs);
                    }

                    noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::SignalValueLookupCallback signalValueLookupCallback = [this](const syrec::VariableAccess& accessedSignalParts) { return tryFetchValueForAccessedSignal(accessedSignalParts); };
                    if (noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier::SimplificationResultReference simplificationResult = assignmentWithoutAdditionalLineSynthesisOptimizer->simplify(*assignmentStmtToSimplify, signalValueLookupCallback); simplificationResult) {
                        if (!simplificationResult->generatedAssignments.empty()) {
                            // TODO: Update reference count of updated assignments, check correct replacement generation in assignment simplifier
                            std::vector<std::unique_ptr<syrec::Statement>> remainingSimplifiedAssignments;
                            remainingSimplifiedAssignments.reserve(simplificationResult->requiredValueResetsForReplacementsTargetingExistingSignals.size() + simplificationResult->generatedAssignments.size() + simplificationResult->requiredInversionsOfValuesResetsForReplacementsTargetingExistingSignals.size());
                            moveOwningCopiesOfStatementsBetweenContainers(remainingSimplifiedAssignments, std::move(simplificationResult->requiredValueResetsForReplacementsTargetingExistingSignals));
                            moveOwningCopiesOfStatementsBetweenContainers(remainingSimplifiedAssignments, std::move(simplificationResult->generatedAssignments));
                            moveOwningCopiesOfStatementsBetweenContainers(remainingSimplifiedAssignments, std::move(simplificationResult->requiredInversionsOfValuesResetsForReplacementsTargetingExistingSignals));
                            if (makeNewlyGeneratedSignalsAvailableInSymbolTableScope(**activeSymbolTableScope, internalIdentifierOfCurrentlyProcessedModule, simplificationResult->newlyGeneratedReplacementSignalDefinitions)) {
                                const ReferenceCountLookupForStatement referenceCountsOfOriginalStatement = ReferenceCountLookupForStatement::createFromStatement(*assignmentStmtToSimplify);
                                ReferenceCountLookupForStatement       referenceCountsOfUpdatedStatements = ReferenceCountLookupForStatement();
                                for (const std::unique_ptr<syrec::Statement>& simplifiedAssignment : remainingSimplifiedAssignments) {
                                    referenceCountsOfUpdatedStatements = referenceCountsOfUpdatedStatements.mergeWith(ReferenceCountLookupForStatement::createFromStatement(*simplifiedAssignment));
                                }
                                updateReferenceCountsOfOptimizedAssignments(**activeSymbolTableScope, referenceCountsOfOriginalStatement.getDifferencesBetweenThisAndOther(referenceCountsOfUpdatedStatements));
                                return OptimizationResult<syrec::Statement>::fromOptimizedContainer(std::move(remainingSimplifiedAssignments));
                            }
                        }
                    }
                }
            }
        }
    }
    performAssignment(*evaluatedSignalAccessOfAssignedToSignal, *mappedToAssignmentOperationFromFlag, rhsOperandEvaluatedAsConstant);
    
    if (lhsOperand != assignmentStmt.lhs || rhsOperand != assignmentStmt.rhs || manuallySetMappedToAssignmentOperationFlag.has_value()) {
        return OptimizationResult<syrec::Statement>::fromOptimizedContainer(std::make_unique<syrec::AssignStatement>(lhsOperand, manuallySetMappedToAssignmentOperationFlag.value_or(assignmentStmt.op), rhsOperand));
    }
    return OptimizationResult<syrec::Statement>::asUnchangedOriginal();
}

// TODO: Symbol table entry for module should also contain whether the module modifies any of its modifiable parameters in its body
// TODO: Should the semantic checks that we assume to be already done in the parser be repeated again, i.e. does a matching module exist, etc.
// TODO: Since we have optimized the call statement, we also need to optimize the corresponding uncall statement
// TODO: Calls to readonly module or modules with no parameters can also be optimized away with the dead code elimination
optimizations::Optimizer::OptimizationResult<syrec::Statement> optimizations::Optimizer::handleCallStmt(const syrec::CallStatement& callStatement) {
    const auto& symbolTableScope = getActiveSymbolTableScope();
    if (!symbolTableScope.has_value()) {
        return OptimizationResult<syrec::Statement>::asUnchangedOriginal();
    }

    if (const auto moduleCallGuessResolver = parser::ModuleCallGuess::tryInitializeWithModulesMatchingName(*symbolTableScope, callStatement.target->name); moduleCallGuessResolver.has_value()) {
        const auto moduleIdsOfInitialGuess = moduleCallGuessResolver->get()->getInternalIdsOfModulesMatchingGuess();

        bool callerArgumentsOk = true;
        for (std::size_t i = 0; i < callStatement.parameters.size() && callerArgumentsOk; ++i) {
            if (const auto& symbolTableEntryForCallerArgument = getSymbolTableEntryForVariable(callStatement.parameters.at(i)); symbolTableEntryForCallerArgument.has_value()) {
                moduleCallGuessResolver->get()->refineGuessWithNextParameter(**symbolTableEntryForCallerArgument);
                callerArgumentsOk &= true;
            }
        }

        if (callerArgumentsOk) {
            /*
             * If the called module is not resolved already by the provided call statement, we also need to take special care when more than one module matches the given signature.
             * TODO: Invalidation of provided modifiable caller parameters given the user provided callee arguments
             */
            const auto modulesMatchingCallSignature = moduleCallGuessResolver->get()->getMatchesForGuess();
            if (modulesMatchingCallSignature.empty() || modulesMatchingCallSignature.size() > 1) {
                /*
                 * Correct overload resolution should already take place in the parser so we should think about whether this branch is even relevant.
                 * If the latter is not the case, we should only invalidate the values of the caller arguments that are modifiable and assigned to in
                 * any of the matching overloads. Again, the check for the assignability of a caller argument to the formal one should already take place
                 * in the parser.
                 */
                for (const auto& callerArgumentSignalIdent : callStatement.parameters) {
                    invalidateValueOfWholeSignal(callerArgumentSignalIdent);
                }

                const auto& internalIdsOfModulesMatchingGuess = moduleCallGuessResolver->get()->getInternalIdsOfModulesMatchingGuess();
                std::vector<parser::SymbolTable::ModuleIdentifier> moduleIdentifiers;
                moduleIdentifiers.reserve(internalIdsOfModulesMatchingGuess.size());

                const std::string_view moduleIdent = callStatement.target->name;
                std::transform(
                        internalIdsOfModulesMatchingGuess.cbegin(),
                        internalIdsOfModulesMatchingGuess.cend(),
                        std::back_inserter(moduleIdentifiers), [&moduleIdent](const std::size_t& internalModuleId) {
                            return parser::SymbolTable::ModuleIdentifier({moduleIdent, internalModuleId});
                        });
                symbolTableScope->get()->updateReferenceCountOfModulesMatchingSignature(moduleIdentifiers, parser::SymbolTable::ReferenceCountUpdate::Increment);
            } else {
                const auto& signatureOfCalledModule = modulesMatchingCallSignature.front();

                std::unordered_set<std::size_t> indicesOfRemainingParameters;
                const auto&                     optimizedModuleSignature = signatureOfCalledModule.determineOptimizedCallSignature(&indicesOfRemainingParameters);
               
                if (canModuleCallBeRemovedWhenIgnoringReferenceCount(parser::SymbolTable::ModuleIdentifier({callStatement.target->name, signatureOfCalledModule.internalModuleId}), parser::SymbolTable::ModuleCallSignature({callStatement.target->name, optimizedModuleSignature}))) {
                    return OptimizationResult<syrec::Statement>::asOptimizedAwayContainer();
                }

                const bool shouldValuesOfModifiableParametersBeInvalidated = dangerousComponentCheckUtils::doesOptimizedModuleBodyContainAssignmentToModifiableParameter(*callStatement.target, **symbolTableScope);
                std::vector<std::string>        callerArgumentsForOptimizedModuleCall;
                callerArgumentsForOptimizedModuleCall.reserve(optimizedModuleSignature.size());

                for (const auto& indexOfRemainingFormalParameter: indicesOfRemainingParameters) {
                    const auto& userProvidedParameterIdent = callStatement.parameters.at(indexOfRemainingFormalParameter);
                    callerArgumentsForOptimizedModuleCall.emplace_back(userProvidedParameterIdent);
                    updateReferenceCountOf(userProvidedParameterIdent, parser::SymbolTable::ReferenceCountUpdate::Increment);
                    /*
                     * TODO:
                     * Since we do not perform module inlining, we only need to invalidate the value of assignable parameters in case the called
                     * module actually performs assignments to the given parameters (dead stores and dead code should already be removed if the corresponding
                     * optimizations are enabled), otherwise perform these checks again ?
                     */
                    if (!isVariableReadOnly(*optimizedModuleSignature.at(indexOfRemainingFormalParameter)) && shouldValuesOfModifiableParametersBeInvalidated) {
                        invalidateValueOfWholeSignal(userProvidedParameterIdent);
                    }
                }

                const std::vector moduleIdentifiersToBeUpdated(1, parser::SymbolTable::ModuleIdentifier({callStatement.target->name, signatureOfCalledModule.internalModuleId}));
                symbolTableScope->get()->updateReferenceCountOfModulesMatchingSignature(moduleIdentifiersToBeUpdated, parser::SymbolTable::ReferenceCountUpdate::Increment);
                if (indicesOfRemainingParameters.size() == signatureOfCalledModule.declaredParameters.size()) {
                    return OptimizationResult<syrec::Statement>::asUnchangedOriginal();
                }
                return OptimizationResult<syrec::Statement>::fromOptimizedContainer(std::make_unique<syrec::CallStatement>(callStatement.target, callerArgumentsForOptimizedModuleCall));
            }
        } else {
            /*
             * We are assuming that the all defined callee arguments refer to previously declared signals, thus this branch should not be relevant but could be kept as a fail-safe in case our assumed precondition does not hold.
             * Additionally, on which guess basis should our update start from? The only choice would be the initial one since an invalid callee argument is not supported by the module call guess resolver (but could be supported in the future)
             * and thus our guesses are not further refined (based only on the data of the valid callee arguments and the number of actually defined parameters).
             * TODO: Check whether this branch is required
             */
            const auto&                                              internalIdsOfModulesMatchingGuess = moduleCallGuessResolver->get()->getInternalIdsOfModulesMatchingGuess();
            std::vector<parser::SymbolTable::ModuleIdentifier> moduleIdentifiers;
            moduleIdentifiers.reserve(internalIdsOfModulesMatchingGuess.size());

            const std::string_view moduleIdent = callStatement.target->name;
            std::transform(
                    internalIdsOfModulesMatchingGuess.cbegin(),
                    internalIdsOfModulesMatchingGuess.cend(),
                    std::back_inserter(moduleIdentifiers), [&moduleIdent](const std::size_t& internalModuleId) {
                        return parser::SymbolTable::ModuleIdentifier({moduleIdent, internalModuleId});
                    });
            symbolTableScope->get()->updateReferenceCountOfModulesMatchingSignature(moduleIdentifiers, parser::SymbolTable::ReferenceCountUpdate::Increment);
        }
    }
    return OptimizationResult<syrec::Statement>::asUnchangedOriginal();
}

optimizations::Optimizer::OptimizationResult<syrec::Statement> optimizations::Optimizer::handleUncallStmt(const syrec::UncallStatement& uncallStatement) const {
    const auto& symbolTableScope = getActiveSymbolTableScope();
    if (!symbolTableScope.has_value()) {
        return OptimizationResult<syrec::Statement>::asUnchangedOriginal();
    }

    /*
     * Similarly to the handling of the guard expression in the if-statement, we do not update the reference counts of neither the caller arguments nor the called module.
     * This also holds if the assumed precondition, all ambiguities regarding the call are resolved (i.e. the given arguments are all defined and refer to exactly one module), does not hold.
     */
    if (const auto moduleCallGuessResolver = parser::ModuleCallGuess::tryInitializeWithModulesMatchingName(*symbolTableScope, uncallStatement.target->name); moduleCallGuessResolver.has_value()) {
        const auto moduleIdsOfInitialGuess = moduleCallGuessResolver->get()->getInternalIdsOfModulesMatchingGuess();

        bool                              callerArgumentsOk = true;
        for (std::size_t i = 0; i < uncallStatement.parameters.size() && callerArgumentsOk; ++i) {
            if (const auto& symbolTableEntryForCallerArgument = getSymbolTableEntryForVariable(uncallStatement.parameters.at(i)); symbolTableEntryForCallerArgument.has_value()) {
                moduleCallGuessResolver->get()->refineGuessWithNextParameter(**symbolTableEntryForCallerArgument);
                callerArgumentsOk &= true;
            }
        }

        if (callerArgumentsOk) {
            const auto modulesMatchingCallSignature = moduleCallGuessResolver->get()->getMatchesForGuess();
            if (modulesMatchingCallSignature.size() == 1) {
                const auto& signatureOfUncalledModule = modulesMatchingCallSignature.front();

                std::unordered_set<std::size_t> indicesOfRemainingParameters;
                const auto& optimizedModuleSignature  = signatureOfUncalledModule.determineOptimizedCallSignature(&indicesOfRemainingParameters);
                if (canModuleCallBeRemovedWhenIgnoringReferenceCount(parser::SymbolTable::ModuleIdentifier({uncallStatement.target->name, signatureOfUncalledModule.internalModuleId}), parser::SymbolTable::ModuleCallSignature({uncallStatement.target->name, optimizedModuleSignature}))) {
                    return OptimizationResult<syrec::Statement>::asOptimizedAwayContainer();
                }

                if (indicesOfRemainingParameters.size() != signatureOfUncalledModule.declaredParameters.size()) {
                    std::vector<std::string> callerArgumentsForOptimizedModuleUncall;
                    callerArgumentsForOptimizedModuleUncall.reserve(optimizedModuleSignature.size());

                    for (const auto& indexOfRemainingFormalParameter: indicesOfRemainingParameters) {
                        const auto& userProvidedParameterIdent = uncallStatement.parameters.at(indexOfRemainingFormalParameter);
                        callerArgumentsForOptimizedModuleUncall.emplace_back(userProvidedParameterIdent);
                    }
                    return OptimizationResult<syrec::Statement>::fromOptimizedContainer(std::make_unique<syrec::UncallStatement>(uncallStatement.target, callerArgumentsForOptimizedModuleUncall));
                }
            }
        }
    }
    return OptimizationResult<syrec::Statement>::asUnchangedOriginal();
}

/*
 * In the current implementation, if a branch can be skipped due to the value of the guard condition, the whole omitted branch will not be optimized
 * and any potential errors that would only be detected during optimizations (i.e. during constant propagation) will not be reported. Since error reporting
 * is not implemented for the optimizer, the latter might be not an issues but since the optimizer does also no panic in case of an error, invalid programs could be
 * detected as valid.
 */
optimizations::Optimizer::OptimizationResult<syrec::Statement> optimizations::Optimizer::handleIfStmt(const syrec::IfStatement& ifStatement) {
    auto                   wasGuardExpressionSimplified = false;
    syrec::Expression::ptr simplifiedGuardExpr = ifStatement.condition;
    if (auto simplificationResultOfGuardExpr = handleExpr(*ifStatement.condition); simplificationResultOfGuardExpr.getStatusOfResult() == OptimizationResultFlag::WasOptimized) {
        simplifiedGuardExpr = std::move(simplificationResultOfGuardExpr.tryTakeOwnershipOfOptimizationResult()->front());
        wasGuardExpressionSimplified = true;
    }
    
    auto                                 canTrueBranchBeOmitted  = false;
    auto                                 canFalseBranchBeOmitted = false;
    auto                                 wereAnyStmtsInTrueBranchModified = false;
    auto                                 wereAnyStmtsInFalseBranchModified = false;
    auto                                 canChangesMadeInFalseBranchBeIgnored = false;
    auto                                 canChangesMadeInTrueBranchBeIgnored  = false;

    if (const auto& constantValueOfGuardCondition = tryEvaluateExpressionToConstant(simplifiedGuardExpr ? *simplifiedGuardExpr : *ifStatement.condition, getActiveSymbolTableForEvaluation(), parserConfig.constantPropagationEnabled, evaluableLoopVariableLookup, nullptr); constantValueOfGuardCondition.has_value()) {
        if (!*constantValueOfGuardCondition) {
            canTrueBranchBeOmitted = parserConfig.deadCodeEliminationEnabled;
            canChangesMadeInTrueBranchBeIgnored = true;
        }
        else {
            canFalseBranchBeOmitted = parserConfig.deadCodeEliminationEnabled;
            canChangesMadeInFalseBranchBeIgnored = true;
        }
    }

    if (const auto& guardConditionAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(simplifiedGuardExpr ? simplifiedGuardExpr : ifStatement.condition); guardConditionAsBinaryExpr) {
        if (const auto& equivalenceResultOfBinaryExprOperands = determineEquivalenceOfOperandsOfBinaryExpr(*guardConditionAsBinaryExpr); equivalenceResultOfBinaryExprOperands.has_value()) {
            canChangesMadeInTrueBranchBeIgnored = !*equivalenceResultOfBinaryExprOperands;
            canChangesMadeInFalseBranchBeIgnored = *equivalenceResultOfBinaryExprOperands;
        }
    }

    std::optional<std::vector<std::unique_ptr<syrec::Statement>>> simplifiedTrueBranchStmtsContainer;
    auto                                                          isTrueBranchEmptyAfterOptimization = false;
    const auto&                                                   internalNestingLevelOfIfStatementBranchSymbolTableBackupScope = incrementInternalIfStatementNestingLevelCounter();
    if (!canTrueBranchBeOmitted) {
        openNewIfStatementBranchSymbolTableBackupScope(internalNestingLevelOfIfStatementBranchSymbolTableBackupScope);
        if (auto simplificationResultOfTrueBranchStmt = handleStatements(transformCollectionOfSharedPointersToReferences(ifStatement.thenStatements)); simplificationResultOfTrueBranchStmt.getStatusOfResult() != OptimizationResultFlag::IsUnchanged) {
            simplifiedTrueBranchStmtsContainer = simplificationResultOfTrueBranchStmt.tryTakeOwnershipOfOptimizationResult();
            wereAnyStmtsInTrueBranchModified   = true;
        }

        const auto& wereAllTrueBranchStatementsOptimizedAway = wereAnyStmtsInTrueBranchModified && ((simplifiedTrueBranchStmtsContainer.has_value() && simplifiedTrueBranchStmtsContainer->empty()) || !simplifiedTrueBranchStmtsContainer.has_value());
        isTrueBranchEmptyAfterOptimization                   = wereAllTrueBranchStatementsOptimizedAway || ifStatement.thenStatements.empty();

        if (!canChangesMadeInFalseBranchBeIgnored) {
            updateBackupOfValuesChangedInScopeAndOptionallyResetMadeChanges(true);
        }
        if (peekPredecessorOfCurrentSymbolTableBackupScope().has_value()) {
            transferBackupOfValuesChangedInCurrentScopeToParentScope();
        }
        if (canChangesMadeInFalseBranchBeIgnored) {
            destroySymbolTableBackupScope();
        }
    }
    else {
        isTrueBranchEmptyAfterOptimization = true;
        wereAnyStmtsInTrueBranchModified  = true;
    }
    
    std::optional<std::vector<std::unique_ptr<syrec::Statement>>> simplifiedFalseBranchStmtsContainer;
    auto                                                          isFalseBranchEmptyAfterOptimization = false;
    if (!canFalseBranchBeOmitted) {
        openNewIfStatementBranchSymbolTableBackupScope(internalNestingLevelOfIfStatementBranchSymbolTableBackupScope);
        if (auto simplificationResultOfFalseBranchStmt = handleStatements(transformCollectionOfSharedPointersToReferences(ifStatement.elseStatements)); simplificationResultOfFalseBranchStmt.getStatusOfResult() != OptimizationResultFlag::IsUnchanged) {
            simplifiedFalseBranchStmtsContainer = simplificationResultOfFalseBranchStmt.tryTakeOwnershipOfOptimizationResult();
            wereAnyStmtsInFalseBranchModified   = true;
        }

        const auto& wereAllFalseBranchStatementsOptimizedAway = wereAnyStmtsInFalseBranchModified && ((simplifiedFalseBranchStmtsContainer.has_value() && simplifiedFalseBranchStmtsContainer->empty()) || !simplifiedFalseBranchStmtsContainer.has_value());
        isFalseBranchEmptyAfterOptimization                   = wereAllFalseBranchStatementsOptimizedAway || ifStatement.elseStatements.empty();

        if (!canChangesMadeInTrueBranchBeIgnored) {
            updateBackupOfValuesChangedInScopeAndOptionallyResetMadeChanges(true);
        }
        if (peekPredecessorOfCurrentSymbolTableBackupScope().has_value()) {
            transferBackupOfValuesChangedInCurrentScopeToParentScope();   
        }

        if (canChangesMadeInTrueBranchBeIgnored) {
            destroySymbolTableBackupScope();
        }
    }
    else {
        isFalseBranchEmptyAfterOptimization = true;
        wereAnyStmtsInFalseBranchModified   = true;
    }

    decrementInternalIfStatementNestingLevelCounter();
    if (!canTrueBranchBeOmitted && !canFalseBranchBeOmitted) {
        const auto backupScopeOfFalseBranch = popCurrentSymbolTableBackupScope();
        const auto backupScopeOfTrueBranch  = popCurrentSymbolTableBackupScope();

        /*
         * At this point the values in the currently active symbol table scope match the ones at the end of the second branch (i.e. the false branch).
         * If the changes made in the false branch can be ignored, we revert the changes made in said branch and update the values in the symbol table
         * with the changes made in the true branch
         */
        if (canChangesMadeInFalseBranchBeIgnored && backupScopeOfTrueBranch.has_value()) {
            makeLocalChangesGlobal(*backupScopeOfTrueBranch->get()->backupScope, *backupScopeOfFalseBranch->get()->backupScope);
        }
        /*
         * If no branch can be omitted, we need to merge the changes made in both branches
         */
        if (!canChangesMadeInTrueBranchBeIgnored) {
            if (backupScopeOfFalseBranch.has_value() && backupScopeOfTrueBranch.has_value()) {
                mergeAndMakeLocalChangesGlobal(*backupScopeOfTrueBranch->get()->backupScope, *backupScopeOfFalseBranch->get()->backupScope);
            }
        }
    }

    if (!wereAnyStmtsInTrueBranchModified && !wereAnyStmtsInFalseBranchModified && !wasGuardExpressionSimplified) {
        return OptimizationResult<syrec::Statement>::asUnchangedOriginal();           
    }
    
    if (parserConfig.deadCodeEliminationEnabled) {
        if (((canTrueBranchBeOmitted && isFalseBranchEmptyAfterOptimization)
            || (canFalseBranchBeOmitted && isTrueBranchEmptyAfterOptimization)
            || (isTrueBranchEmptyAfterOptimization && isFalseBranchEmptyAfterOptimization)) 
            && !dangerousComponentCheckUtils::doesExprContainPotentiallyDangerousComponent(*simplifiedGuardExpr)) {
            return OptimizationResult<syrec::Statement>::asOptimizedAwayContainer();
        }
        
        if (canTrueBranchBeOmitted) {
            auto remainingFalseBranchStatements = simplifiedFalseBranchStmtsContainer.has_value()
                ? std::move(*simplifiedFalseBranchStmtsContainer)
                : copyUtils::createCopyOfStatements(ifStatement.elseStatements);
            
            return OptimizationResult<syrec::Statement>::fromOptimizedContainer(std::move(remainingFalseBranchStatements));
        }
        if (canFalseBranchBeOmitted) {
            auto remainingTrueBranchStatements = simplifiedTrueBranchStmtsContainer.has_value()
                ? std::move(*simplifiedTrueBranchStmtsContainer)
                : copyUtils::createCopyOfStatements(ifStatement.thenStatements);

            return OptimizationResult<syrec::Statement>::fromOptimizedContainer(std::move(remainingTrueBranchStatements));
        }
    }

    auto simplifiedIfStatement     = std::make_unique<syrec::IfStatement>();
    simplifiedIfStatement->condition      = simplifiedGuardExpr;
    simplifiedIfStatement->fiCondition    = simplifiedGuardExpr;

    if (wereAnyStmtsInTrueBranchModified) {
        emplaceSingleSkipStatementIfContainerIsEmpty(simplifiedTrueBranchStmtsContainer);
    }
    if (wereAnyStmtsInFalseBranchModified) {
        emplaceSingleSkipStatementIfContainerIsEmpty(simplifiedFalseBranchStmtsContainer);
    }

    simplifiedIfStatement->thenStatements = !simplifiedTrueBranchStmtsContainer.has_value()
        ? createStatementListFrom(ifStatement.thenStatements, {})
        : createStatementListFrom({}, std::move(*simplifiedTrueBranchStmtsContainer));
        
    simplifiedIfStatement->elseStatements = !simplifiedFalseBranchStmtsContainer.has_value()
        ? createStatementListFrom(ifStatement.elseStatements, {})
        : createStatementListFrom({}, std::move(*simplifiedFalseBranchStmtsContainer));
    
    return OptimizationResult<syrec::Statement>::fromOptimizedContainer(std::move(simplifiedIfStatement));
}

// TODO: Test cases for removal of equal operands of swap ? Check if optimization of swap throws error in such cases, since this should not be possible
optimizations::Optimizer::OptimizationResult<syrec::Statement> optimizations::Optimizer::handleLoopStmt(const syrec::ForStatement& forStatement) {
    syrec::Number::ptr iterationRangeStartSimplified;
    syrec::Number::ptr iterationRangeEndSimplified;
    syrec::Number::ptr iterationRangeStepSizeSimplified;

    std::optional<unsigned int> constantValueOfIterationRangeStart;
    std::optional<unsigned int> constantValueOfIterationRangeEnd;
    std::optional<unsigned int> constantValueOfIterationRangeStepsize;

    const auto& symbolTableScopePointerUsableForEvaluation = getActiveSymbolTableForEvaluation();
    if (!symbolTableScopePointerUsableForEvaluation) {
        return OptimizationResult<syrec::Statement>::asUnchangedOriginal();
    }

    if (auto iterationRangeStartSimplificationResult = handleNumber(*forStatement.range.first); iterationRangeStartSimplificationResult.getStatusOfResult() != OptimizationResultFlag::IsUnchanged) {
        iterationRangeStartSimplified = std::move(iterationRangeStartSimplificationResult.tryTakeOwnershipOfOptimizationResult()->front());
        constantValueOfIterationRangeStart = tryEvaluateNumberAsConstant(*iterationRangeStartSimplified, symbolTableScopePointerUsableForEvaluation, parserConfig.constantPropagationEnabled, evaluableLoopVariableLookup, nullptr);
    } else {
        constantValueOfIterationRangeStart = tryEvaluateNumberAsConstant(*forStatement.range.first, symbolTableScopePointerUsableForEvaluation, parserConfig.constantPropagationEnabled, evaluableLoopVariableLookup, nullptr);
    }

    if (!forStatement.loopVariable.empty()) {
        /*
        * We are assuming that no negative step size are allowed and thus the maximum value assigned to the current loop variable is bounded by the defined
        * iteration range end value. Since we need to know the maximum value for the defined loop variable to create its corresponding symbol table entry
        * but can also use the current loop variable in the expr defining the maximum value, we have a kind of chicken-and-egg problem. We solve this
        * by performing a prior evaluation of the expr defining the maximum value with the current value of the loop variable set to its initial value and only
        * then create the symbol table entry for the loop variable.
        */
        const auto& fetchedMaximumValueOfLoopVariable = tryDetermineMaximumValueOfLoopVariable(forStatement.loopVariable, constantValueOfIterationRangeStart, *forStatement.range.second);
        addSymbolTableEntryForLoopVariable(forStatement.loopVariable, fetchedMaximumValueOfLoopVariable);
        updateValueOfLoopVariable(forStatement.loopVariable, constantValueOfIterationRangeStart);
    }

    const auto doIterationRangeStartAndEndContainerMatch = forStatement.range.first == forStatement.range.second;
    if (doIterationRangeStartAndEndContainerMatch) {
        constantValueOfIterationRangeStart = constantValueOfIterationRangeEnd;
    }
    else {
        if (auto iterationRangeEndSimplificationResult = handleNumber(*forStatement.range.second); iterationRangeEndSimplificationResult.getStatusOfResult() != OptimizationResultFlag::IsUnchanged) {
            iterationRangeEndSimplified      = std::move(iterationRangeEndSimplificationResult.tryTakeOwnershipOfOptimizationResult()->front());
            constantValueOfIterationRangeEnd = tryEvaluateNumberAsConstant(*iterationRangeEndSimplified, symbolTableScopePointerUsableForEvaluation, parserConfig.constantPropagationEnabled, evaluableLoopVariableLookup, nullptr);
        } else {
            constantValueOfIterationRangeEnd = tryEvaluateNumberAsConstant(*forStatement.range.second, symbolTableScopePointerUsableForEvaluation, parserConfig.constantPropagationEnabled, evaluableLoopVariableLookup, nullptr);
        }
    }

    if (auto iterationRangeStepSizeSimplificationResult = handleNumber(*forStatement.step); iterationRangeStepSizeSimplificationResult.getStatusOfResult() != OptimizationResultFlag::IsUnchanged) {
        iterationRangeStepSizeSimplified = std::move(iterationRangeStepSizeSimplificationResult.tryTakeOwnershipOfOptimizationResult()->front());
        constantValueOfIterationRangeStepsize = tryEvaluateNumberAsConstant(*iterationRangeStepSizeSimplified, symbolTableScopePointerUsableForEvaluation, parserConfig.constantPropagationEnabled, evaluableLoopVariableLookup, nullptr);
    } else {
        constantValueOfIterationRangeStepsize = tryEvaluateNumberAsConstant(*forStatement.step, symbolTableScopePointerUsableForEvaluation, parserConfig.constantPropagationEnabled, evaluableLoopVariableLookup, nullptr);
    }

    auto                        shouldValueOfLoopVariableBeAvailableInLoopBody = false;
    std::optional<unsigned int> numLoopIterations;
    if (constantValueOfIterationRangeStart.has_value() && constantValueOfIterationRangeEnd.has_value() && constantValueOfIterationRangeStepsize.has_value()) {
        numLoopIterations                              = utils::determineNumberOfLoopIterations(*constantValueOfIterationRangeStart, *constantValueOfIterationRangeEnd, *constantValueOfIterationRangeStepsize);
    }
    
    const auto  internalLoopNestingLevel        = incrementInternalLoopNestingLevel();
    syrec::Number::ptr       iterationRangeStartContainer    = !iterationRangeStartSimplified ? forStatement.range.first : iterationRangeStartSimplified;
    syrec::Number::ptr       iterationRangeEndContainer      = !iterationRangeEndSimplified ? (doIterationRangeStartAndEndContainerMatch && iterationRangeStartSimplified ? iterationRangeStartSimplified : forStatement.range.second) : iterationRangeEndSimplified;
    syrec::Number::ptr       iterationRangeStepSizeContainer = !iterationRangeStepSizeSimplified ? forStatement.step : iterationRangeStepSizeSimplified;

    // TODO: Handling of loops with zero iterations when dead code elimination is disabled
    if (numLoopIterations.has_value() && !*numLoopIterations) {
        removeLoopVariableFromSymbolTable(forStatement.loopVariable);
        decrementInternalLoopNestingLevel();

        if (parserConfig.deadCodeEliminationEnabled) {
            decrementReferenceCountsOfLoopHeaderComponents(*iterationRangeStartContainer, doIterationRangeStartAndEndContainerMatch ? std::nullopt : std::make_optional(*iterationRangeEndContainer), *iterationRangeStepSizeContainer);
            return OptimizationResult<syrec::Statement>::asOptimizedAwayContainer();
        }
        return OptimizationResult<syrec::Statement>::asUnchangedOriginal();
    }

    syrec::Statement::vec loopBodyToBeOptimized = forStatement.statements;
    syrec::Statement::vec optimizedStatementsExtractedFromLoopBody;
    bool                  wasReferenceLoopCompletelyUnrolled = false;

    bool didSimplificationOfAdditionalStatementsFail = false;
    bool didSimplificationOfLoopBodyStmtsFail        = false;
    bool wasAnyStatementInLoopBodyOptimized = false;
    bool wasInternalLoopNestingLevelAlreadyDecremented = false;
    if (parserConfig.deadCodeEliminationEnabled && parserConfig.optionalLoopUnrollConfig.has_value()) {
        syrec::ForStatement loopStatementWithOptimizedHeader = syrec::ForStatement();
        loopStatementWithOptimizedHeader.loopVariable        = forStatement.loopVariable;
        loopStatementWithOptimizedHeader.range               = std::make_pair(iterationRangeStartContainer, iterationRangeEndContainer);
        loopStatementWithOptimizedHeader.step                = iterationRangeStepSizeContainer;
        loopStatementWithOptimizedHeader.statements          = forStatement.statements;

        // TODO: Make value of loop variable available to optimization of peeled statements
        // TODO: Value of loop variable is not available?
        auto loopUnrollResult = loopUnrollerInstance->tryUnrollLoop(internalLoopNestingLevel, loopStatementWithOptimizedHeader, evaluableLoopVariableLookup, *parserConfig.optionalLoopUnrollConfig);
        if  (auto fullyUnrolledLoopResult = dynamic_cast<LoopUnroller::FullyUnrolledLoopInformation*>(loopUnrollResult.get()); fullyUnrolledLoopResult) {
            /*
             * Since the current for statement was completely unrolled, we need to perform the optimization of the peeled statements as well as of the unrolled loop body statements
             * at one loop nesting level higher
             */
            decrementInternalLoopNestingLevel();
            wasInternalLoopNestingLevelAlreadyDecremented = true;

            didSimplificationOfAdditionalStatementsFail =
                !(optimizePeeledLoopBodyStatements(optimizedStatementsExtractedFromLoopBody, fullyUnrolledLoopResult->peeledStatements)
                && optimizeUnrolledLoopBodyStatements(optimizedStatementsExtractedFromLoopBody, fullyUnrolledLoopResult->numUnrolledIterations, fullyUnrolledLoopResult->unrolledIterations));
            
            numLoopIterations.reset();
            loopBodyToBeOptimized.clear();
            wasReferenceLoopCompletelyUnrolled = true;
            wasAnyStatementInLoopBodyOptimized = true;
        }
        else if (auto partiallyUnrolledLoopResult = dynamic_cast<LoopUnroller::PartiallyUnrolledLoopInformation*>(loopUnrollResult.get()); partiallyUnrolledLoopResult) {
            /*
             * Since the current for statement was completely unrolled, we need to perform the optimization of the peeled statements as well as of the unrolled loop body statements
             * at one loop nesting level higher
             */
            decrementInternalLoopNestingLevel();
            wasInternalLoopNestingLevelAlreadyDecremented = true;

            didSimplificationOfAdditionalStatementsFail =
                !(optimizePeeledLoopBodyStatements(optimizedStatementsExtractedFromLoopBody, partiallyUnrolledLoopResult->peeledStatements) 
                && optimizeUnrolledLoopBodyStatements(optimizedStatementsExtractedFromLoopBody, partiallyUnrolledLoopResult->numUnrolledIterations, partiallyUnrolledLoopResult->unrolledIterations));
            
            // TODO: Checks for return value of std::make_unique
            iterationRangeStartSimplified = std::make_shared<syrec::Number>(partiallyUnrolledLoopResult->remainderLoopHeaderInformation.initialLoopVariableValue);
            constantValueOfIterationRangeStart = partiallyUnrolledLoopResult->remainderLoopHeaderInformation.initialLoopVariableValue;
            iterationRangeStartContainer  = iterationRangeStartSimplified;

            iterationRangeEndSimplified = std::make_shared<syrec::Number>(partiallyUnrolledLoopResult->remainderLoopHeaderInformation.finalLoopVariableValue);
            constantValueOfIterationRangeEnd = partiallyUnrolledLoopResult->remainderLoopHeaderInformation.finalLoopVariableValue;
            iterationRangeEndContainer = iterationRangeEndSimplified;

            iterationRangeStepSizeSimplified = std::make_shared<syrec::Number>(partiallyUnrolledLoopResult->remainderLoopHeaderInformation.stepSize);
            constantValueOfIterationRangeStepsize = partiallyUnrolledLoopResult->remainderLoopHeaderInformation.stepSize;
            iterationRangeStepSizeContainer  = iterationRangeStepSizeSimplified;

            numLoopIterations                              = utils::determineNumberOfLoopIterations(*constantValueOfIterationRangeStart, *constantValueOfIterationRangeEnd, *constantValueOfIterationRangeStepsize);
            /*shouldValueOfLoopVariableBeAvailableInLoopBody = numLoopIterations.has_value() && *numLoopIterations <= 1;
            if (!shouldValueOfLoopVariableBeAvailableInLoopBody && partiallyUnrolledLoopResult->remainderLoopHeaderInformation.loopVariableIdent.has_value()) {
                updateValueOfLoopVariable(*partiallyUnrolledLoopResult->remainderLoopHeaderInformation.loopVariableIdent, std::nullopt);
            }*/

            loopBodyToBeOptimized.clear();
            insertStatementCopiesInto(loopBodyToBeOptimized, std::move(partiallyUnrolledLoopResult->remainderLoopBodyStatements));
            wasAnyStatementInLoopBodyOptimized = true;
            /*
             * Since we have manually decremented the internal loop nesting level for the optimization of both the peeled as well as for the unrolled iteration statements,
             * we need to reset it to the original nesting level (i.e. increment it once), for the correct optimization of the remainder loop body.
             */
            incrementInternalLoopNestingLevel();
        }
    }
    else {
        shouldValueOfLoopVariableBeAvailableInLoopBody = numLoopIterations.has_value() && *numLoopIterations <= 1;   
    }

    auto wereAnyAssignmentsPerformedInLoopBody = false;
    const auto shouldNewValuePropagationBlockerScopeOpened = !wasReferenceLoopCompletelyUnrolled && numLoopIterations.value_or(2) > 1;

    /*
     * TODO: Data flow analysis operates on static loop body information that could change when considering data that is loop variant (i.e. in guard conditions, etc.)
     */
    if (shouldNewValuePropagationBlockerScopeOpened) {
        /*
         * Are assignments that do not change result removed / not considered
         */
        activeDataFlowValuePropagationRestrictions->openNewScopeAndAppendDataFlowAnalysisResult(transformCollectionOfSharedPointersToReferences(loopBodyToBeOptimized), &wereAnyAssignmentsPerformedInLoopBody);
    }

    if (!shouldValueOfLoopVariableBeAvailableInLoopBody && !forStatement.loopVariable.empty()) {
        updateValueOfLoopVariable(forStatement.loopVariable, std::nullopt);
    }
    // TODO: Correct setting of internal flags
    // TODO: General optimization of loop body
    std::optional<std::vector<std::unique_ptr<syrec::Statement>>> loopBodyStmtsSimplified;
    if (auto simplificationResultOfLoopBodyStmts = handleStatements(transformCollectionOfSharedPointersToReferences(loopBodyToBeOptimized)); simplificationResultOfLoopBodyStmts.getStatusOfResult() != OptimizationResultFlag::IsUnchanged) {
        loopBodyStmtsSimplified = simplificationResultOfLoopBodyStmts.tryTakeOwnershipOfOptimizationResult();
        didSimplificationOfLoopBodyStmtsFail = !loopBodyStmtsSimplified.has_value();
        wasAnyStatementInLoopBodyOptimized   = true;
    }

    if (!wereAnyAssignmentsPerformedInLoopBody) {
        std::vector<std::reference_wrapper<syrec::Statement>> statementsToCheck;
        if (loopBodyStmtsSimplified.has_value()) {
            statementsToCheck.reserve(loopBodyStmtsSimplified->size());
            for (auto&& stmt : *loopBodyStmtsSimplified) {
                wereAnyAssignmentsPerformedInLoopBody |= doesStatementDefineAssignmentThatChangesAssignedToSignal(*stmt, true);
                if (wereAnyAssignmentsPerformedInLoopBody) {
                    break;
                }
            }
        }
        else {
            statementsToCheck.reserve(loopBodyToBeOptimized.size());
            for (const auto& stmt: loopBodyToBeOptimized) {
                wereAnyAssignmentsPerformedInLoopBody |= doesStatementDefineAssignmentThatChangesAssignedToSignal(*stmt, false);
                if (wereAnyAssignmentsPerformedInLoopBody) {
                    break;
                }
            }
        }
    }

    if (!wasInternalLoopNestingLevelAlreadyDecremented) {
        decrementInternalLoopNestingLevel();   
    }

    if (shouldNewValuePropagationBlockerScopeOpened) {
        activeDataFlowValuePropagationRestrictions->closeScopeAndDiscardDataFlowAnalysisResult();
    }

    if (didSimplificationOfLoopBodyStmtsFail || didSimplificationOfAdditionalStatementsFail) {
        removeLoopVariableFromSymbolTable(forStatement.loopVariable);
        return OptimizationResult<syrec::Statement>::asUnchangedOriginal();
    }

    const auto isLoopBodyAfterOptimizationsEmpty = (loopBodyStmtsSimplified.has_value() && loopBodyStmtsSimplified->empty()) || (loopBodyStmtsSimplified.has_value() && loopBodyToBeOptimized.empty());
    // TODO: Check whether we can provide our evaluable loop variable mappings to the check for potentially dangerous statements check
    const auto wereAnyPotentiallyDangerousStatementsDefinedInLoopBody = !isLoopBodyAfterOptimizationsEmpty && (wereAnyAssignmentsPerformedInLoopBody || doesLoopBodyDefineAnyPotentiallyDangerousStmt(loopBodyToBeOptimized, loopBodyStmtsSimplified, *symbolTableScopePointerUsableForEvaluation));
    if (((!wereAnyAssignmentsPerformedInLoopBody && !wereAnyPotentiallyDangerousStatementsDefinedInLoopBody) || isLoopBodyAfterOptimizationsEmpty) && optimizedStatementsExtractedFromLoopBody.empty()) {
        if (parserConfig.deadCodeEliminationEnabled) {
            removeLoopVariableFromSymbolTable(forStatement.loopVariable);
            decrementReferenceCountsOfLoopHeaderComponents(*iterationRangeStartContainer, doIterationRangeStartAndEndContainerMatch ? std::nullopt : std::make_optional(*iterationRangeEndContainer), *iterationRangeStepSizeContainer);
            return OptimizationResult<syrec::Statement>::asOptimizedAwayContainer();
        }
        if (isLoopBodyAfterOptimizationsEmpty) {
            std::vector<std::unique_ptr<syrec::SkipStatement>> containerForSingleSkipStmt;
            containerForSingleSkipStmt.reserve(1);
            containerForSingleSkipStmt.emplace_back(std::make_unique<syrec::SkipStatement>());
            loopBodyStmtsSimplified = std::move(containerForSingleSkipStmt);
        }
    }

    if (numLoopIterations.has_value() && *numLoopIterations == 1 && parserConfig.deadCodeEliminationEnabled) {
        /*
         * The value of loop variable should already be propagated through the statements of the loop body and thus the latter should not require second processing pass.
         * TODO: What is constant propagation is turned off? is the value of the loop variable still propagated if only dead code elimination is activated?
         */
        removeLoopVariableFromSymbolTable(forStatement.loopVariable);
        decrementReferenceCountsOfLoopHeaderComponents(*iterationRangeStartContainer, doIterationRangeStartAndEndContainerMatch ? std::nullopt : std::make_optional(*iterationRangeEndContainer), *iterationRangeStepSizeContainer);

        std::vector<std::unique_ptr<syrec::Statement>> copyOfStatementsOfOriginalLoopBody;
        const auto&                                    numberOfStatementsInLoopBody = loopBodyStmtsSimplified.has_value() ? loopBodyStmtsSimplified->size() : loopBodyToBeOptimized.size();
        copyOfStatementsOfOriginalLoopBody.reserve(numberOfStatementsInLoopBody + optimizedStatementsExtractedFromLoopBody.size());

        for (const auto& optimizedLoopBodyStmt: optimizedStatementsExtractedFromLoopBody) {
            auto owningContainerOfOptimizedLoopBodyStmt = copyUtils::createCopyOfStmt(*optimizedLoopBodyStmt);
            if (!owningContainerOfOptimizedLoopBodyStmt) {
                return OptimizationResult<syrec::Statement>::asUnchangedOriginal();
            }
            copyOfStatementsOfOriginalLoopBody.emplace_back(std::move(owningContainerOfOptimizedLoopBodyStmt));
        }

        std::vector<std::unique_ptr<syrec::Statement>> optimizedLoopBodyStmts = loopBodyStmtsSimplified.has_value() ? std::move(*loopBodyStmtsSimplified) : copyUtils::createCopyOfStatements(loopBodyToBeOptimized);

        for (auto&& simplifiedLoopBodyStmt: optimizedLoopBodyStmts) {
            copyOfStatementsOfOriginalLoopBody.emplace_back(std::move(simplifiedLoopBodyStmt));
        }
        return OptimizationResult<syrec::Statement>::fromOptimizedContainer(std::move(copyOfStatementsOfOriginalLoopBody));
    }

    const auto wasAnyComponentOfLoopModified = iterationRangeStartSimplified
        || iterationRangeEndSimplified
        || iterationRangeStepSizeSimplified
        || wasAnyStatementInLoopBodyOptimized;

    if (!wasAnyComponentOfLoopModified || (didSimplificationOfLoopBodyStmtsFail || didSimplificationOfAdditionalStatementsFail)) {
        if (!(didSimplificationOfLoopBodyStmtsFail || didSimplificationOfAdditionalStatementsFail) 
            && parserConfig.deadCodeEliminationEnabled && !forStatement.loopVariable.empty() && !symbolTableScopePointerUsableForEvaluation->isVariableUsedAnywhereBasedOnReferenceCount(forStatement.loopVariable)) {
            removeLoopVariableFromSymbolTable(forStatement.loopVariable);
            auto simplifiedLoopStmt = std::make_unique<syrec::ForStatement>(forStatement);
            simplifiedLoopStmt->loopVariable = "";
            return OptimizationResult<syrec::Statement>::fromOptimizedContainer(std::move(simplifiedLoopStmt));
        }

        removeLoopVariableFromSymbolTable(forStatement.loopVariable);
        return OptimizationResult<syrec::Statement>::asUnchangedOriginal();
    }

    std::vector<std::unique_ptr<syrec::Statement>> simplificationResultContainer;
    simplificationResultContainer.reserve(optimizedStatementsExtractedFromLoopBody.size() + (wasReferenceLoopCompletelyUnrolled ? 0 : 1));

    for (const auto& optimizedStmtExtractedFromLoopBody : optimizedStatementsExtractedFromLoopBody) {
        auto owningContainerOfOptimizedLoopBodyStmt = copyUtils::createCopyOfStmt(*optimizedStmtExtractedFromLoopBody);
        if (!owningContainerOfOptimizedLoopBodyStmt) {
            removeLoopVariableFromSymbolTable(forStatement.loopVariable);
            return OptimizationResult<syrec::Statement>::asUnchangedOriginal();
        }
        simplificationResultContainer.emplace_back(std::move(owningContainerOfOptimizedLoopBodyStmt));       
    }

    if (!wasReferenceLoopCompletelyUnrolled) {
        auto simplifiedLoopStmt = std::make_unique<syrec::ForStatement>();
        /*
         * If loop variable is unused in loop body and if dead code elimination is enabled, the former can be removed
         */
        simplifiedLoopStmt->loopVariable = parserConfig.deadCodeEliminationEnabled && !symbolTableScopePointerUsableForEvaluation->isVariableUsedAnywhereBasedOnReferenceCount(forStatement.loopVariable) ? "" : forStatement.loopVariable;
        simplifiedLoopStmt->step         = iterationRangeStepSizeSimplified ? iterationRangeStepSizeSimplified : forStatement.step;
        simplifiedLoopStmt->range        = std::make_pair(
                iterationRangeStartSimplified ? iterationRangeStartSimplified : forStatement.range.first,
                iterationRangeEndSimplified ? iterationRangeEndSimplified : forStatement.range.second);

        simplifiedLoopStmt->statements = !loopBodyStmtsSimplified.has_value() ? createStatementListFrom(loopBodyToBeOptimized, {}) : createStatementListFrom({}, std::move(*loopBodyStmtsSimplified));
        simplificationResultContainer.emplace_back(std::move(simplifiedLoopStmt));
    }
    else {
        decrementReferenceCountsOfLoopHeaderComponents(*iterationRangeStartContainer, doIterationRangeStartAndEndContainerMatch ? std::nullopt : std::make_optional(*iterationRangeEndContainer), *iterationRangeStepSizeContainer);
    }

    removeLoopVariableFromSymbolTable(forStatement.loopVariable);
    return OptimizationResult<syrec::Statement>::fromOptimizedContainer(std::move(simplificationResultContainer));
}

optimizations::Optimizer::OptimizationResult<syrec::Statement> optimizations::Optimizer::handleSkipStmt() {
    return OptimizationResult<syrec::Statement>::asUnchangedOriginal();
}

/*
 * Removal of swaps with identical operands should not be optimized to NO_OP operations since such swaps are not
 * syntactically correct but the validate the reversibility requirement of assignment statements
 */
optimizations::Optimizer::OptimizationResult<syrec::Statement> optimizations::Optimizer::handleSwapStmt(const syrec::SwapStatement& swapStatement) {
    auto simplificationResultOfLhsOperand = handleSignalAccess(*swapStatement.lhs, false, nullptr);
    auto simplificationResultOfRhsOperand = handleSignalAccess(*swapStatement.rhs, false, nullptr);

    const auto& lhsOperandAfterSimplification = simplificationResultOfLhsOperand.getStatusOfResult() == OptimizationResultFlag::WasOptimized
        ? std::make_shared<syrec::VariableAccess>(*simplificationResultOfLhsOperand.tryTakeOwnershipOfOptimizationResult()->front())
        : swapStatement.lhs;

    const auto& rhsOperandAfterSimplification = simplificationResultOfRhsOperand.getStatusOfResult() == OptimizationResultFlag::WasOptimized
        ? std::make_shared<syrec::VariableAccess>(*simplificationResultOfRhsOperand.tryTakeOwnershipOfOptimizationResult()->front())
        : swapStatement.rhs;
    
    auto evaluatedSwapOperationLhsSignalAccess = tryEvaluateDefinedSignalAccess(*lhsOperandAfterSimplification);
    auto evaluatedSwapOperationRhsSignalAccess = tryEvaluateDefinedSignalAccess(*rhsOperandAfterSimplification);

    if (evaluatedSwapOperationLhsSignalAccess.has_value() && evaluatedSwapOperationRhsSignalAccess.has_value()) {
        const auto& signalAccessEquivalenceResult = areEvaluatedSignalAccessEqualAtCompileTime(*evaluatedSwapOperationLhsSignalAccess, *evaluatedSwapOperationRhsSignalAccess);
        if (signalAccessEquivalenceResult.equality == SignalAccessUtils::SignalAccessEquivalenceResult::Equal && signalAccessEquivalenceResult.isResultCertain) {
            if (parserConfig.deadCodeEliminationEnabled) {
                return OptimizationResult<syrec::Statement>::asOptimizedAwayContainer();
            }
        }
        else {
            performSwap(*evaluatedSwapOperationLhsSignalAccess, *evaluatedSwapOperationRhsSignalAccess);
        }
    } else {
        if (evaluatedSwapOperationLhsSignalAccess.has_value()) {
            invalidateValueOfAccessedSignalParts(*evaluatedSwapOperationLhsSignalAccess);
        }
        if (evaluatedSwapOperationRhsSignalAccess.has_value()) {
            invalidateValueOfAccessedSignalParts(*evaluatedSwapOperationRhsSignalAccess);
        }        
    }

    if (lhsOperandAfterSimplification != swapStatement.lhs || rhsOperandAfterSimplification != swapStatement.rhs) {
        return OptimizationResult<syrec::Statement>::fromOptimizedContainer(std::make_unique<syrec::SwapStatement>(lhsOperandAfterSimplification, rhsOperandAfterSimplification));
    }
    return OptimizationResult<syrec::Statement>::asUnchangedOriginal();
}

optimizations::Optimizer::OptimizationResult<syrec::Statement> optimizations::Optimizer::handleUnaryStmt(const syrec::UnaryStatement& unaryStmt) {
    syrec::VariableAccess::ptr signalAccessOperand = unaryStmt.var;
    if (auto simplificationResultOfUnaryOperandSignalAccess = handleSignalAccess(*unaryStmt.var, false, nullptr); simplificationResultOfUnaryOperandSignalAccess.getStatusOfResult() == OptimizationResultFlag::WasOptimized) {
        signalAccessOperand = std::move(simplificationResultOfUnaryOperandSignalAccess.tryTakeOwnershipOfOptimizationResult()->front());
    }

    if (auto evaluatedUnaryOperandSignalAccess = tryEvaluateDefinedSignalAccess(*signalAccessOperand); evaluatedUnaryOperandSignalAccess.has_value()) {
        const auto& fetchedValueForAssignedToSignal = tryFetchValueFromEvaluatedSignalAccess(*evaluatedUnaryOperandSignalAccess);
        const auto& mappedToUnaryOperationFromFlag  = syrec_operation::tryMapUnaryAssignmentOperationFlagToEnum(unaryStmt.op);
        
        if (fetchedValueForAssignedToSignal.has_value() && mappedToUnaryOperationFromFlag.has_value()) {
            performAssignment(*evaluatedUnaryOperandSignalAccess, *mappedToUnaryOperationFromFlag, std::make_optional<unsigned int>(1));
        } else {
            invalidateValueOfAccessedSignalParts(*evaluatedUnaryOperandSignalAccess);
        }
    }

    if (signalAccessOperand != unaryStmt.var) {
        return OptimizationResult<syrec::Statement>::fromOptimizedContainer(std::make_unique<syrec::UnaryStatement>(unaryStmt.op, signalAccessOperand));   
    }
    return OptimizationResult<syrec::Statement>::asUnchangedOriginal();
}

optimizations::Optimizer::OptimizationResult<syrec::Expression> optimizations::Optimizer::handleExpr(const syrec::Expression& expression) const {
    if (const auto& exprAsBinaryExpr = dynamic_cast<const syrec::BinaryExpression*>(&expression); exprAsBinaryExpr != nullptr) {
        return handleBinaryExpr(*exprAsBinaryExpr);
    }
    if (const auto& exprAsShiftExpr = dynamic_cast<const syrec::ShiftExpression*>(&expression); exprAsShiftExpr != nullptr) {
        return handleShiftExpr(*exprAsShiftExpr);
    }
    if (const auto& exprAsNumericExpr = dynamic_cast<const syrec::NumericExpression*>(&expression); exprAsNumericExpr != nullptr) {
        return handleNumericExpr(*exprAsNumericExpr);
    }
    if (const auto& exprAsVariableExpr = dynamic_cast<const syrec::VariableExpression*>(&expression); exprAsVariableExpr != nullptr) {
        return handleVariableExpr(*exprAsVariableExpr);
    }
    return OptimizationResult<syrec::Expression>::asUnchangedOriginal();
}

optimizations::Optimizer::OptimizationResult<syrec::Expression> optimizations::Optimizer::handleBinaryExpr(const syrec::BinaryExpression& expression) const {
    std::unique_ptr<syrec::Expression> simplifiedLhsExpr;
    std::unique_ptr<syrec::Expression> simplifiedRhsExpr;
    auto                               wasOriginalExprModified = false;

    if (auto simplificationResultOfLhsExpr = handleExpr(*expression.lhs); simplificationResultOfLhsExpr.getStatusOfResult() != OptimizationResultFlag::IsUnchanged) {
        wasOriginalExprModified = true;
        simplifiedLhsExpr       = std::move(simplificationResultOfLhsExpr.tryTakeOwnershipOfOptimizationResult()->front());
    }

    if (auto simplificationResultOfRhsExpr = handleExpr(*expression.rhs); simplificationResultOfRhsExpr.getStatusOfResult() != OptimizationResultFlag::IsUnchanged) {
        wasOriginalExprModified = true;
        simplifiedRhsExpr       = std::move(simplificationResultOfRhsExpr.tryTakeOwnershipOfOptimizationResult()->front());
    }

    std::unique_ptr<syrec::Expression> simplifiedBinaryExpr;
    const auto constantValueOfLhsExpr = tryEvaluateExpressionToConstant(simplifiedLhsExpr ? *simplifiedLhsExpr : *expression.lhs, getActiveSymbolTableForEvaluation(), parserConfig.constantPropagationEnabled, evaluableLoopVariableLookup, nullptr);
    const auto constantValueOfRhsExpr = tryEvaluateExpressionToConstant(simplifiedRhsExpr ? *simplifiedRhsExpr : *expression.rhs, getActiveSymbolTableForEvaluation(), parserConfig.constantPropagationEnabled, evaluableLoopVariableLookup, nullptr);
    const auto mappedToBinaryOperation = syrec_operation::tryMapBinaryOperationFlagToEnum(expression.op);

    if (mappedToBinaryOperation.has_value()) {
        if (constantValueOfLhsExpr.has_value() && syrec_operation::isOperandUsedAsLhsInOperationIdentityElement(*mappedToBinaryOperation, *constantValueOfLhsExpr)) {
            simplifiedBinaryExpr = std::move(simplifiedRhsExpr);
        }
        if (constantValueOfRhsExpr.has_value() && syrec_operation::isOperandUseAsRhsInOperationIdentityElement(*mappedToBinaryOperation, *constantValueOfRhsExpr)) {
            simplifiedBinaryExpr = std::move(simplifiedLhsExpr);
        }
        if (constantValueOfLhsExpr.has_value() && constantValueOfRhsExpr.has_value()) {
            if (const auto& evaluationResultOfBinaryOperation = syrec_operation::apply(*mappedToBinaryOperation, *constantValueOfLhsExpr, *constantValueOfRhsExpr); evaluationResultOfBinaryOperation.has_value()) {
                simplifiedBinaryExpr = std::make_unique<syrec::NumericExpression>(std::make_unique<syrec::Number>(*evaluationResultOfBinaryOperation), expression.bitwidth());
            }
        }
        wasOriginalExprModified |= simplifiedBinaryExpr != nullptr;
    }
    if ((simplifiedLhsExpr || simplifiedRhsExpr) && !simplifiedBinaryExpr) {
        syrec::Expression::ptr lhsOperandOfSimplifiedExpr = simplifiedLhsExpr ? std::move(simplifiedLhsExpr) : expression.lhs;
        syrec::Expression::ptr rhsOperandOfSimplifiedExpr = simplifiedRhsExpr ? std::move(simplifiedRhsExpr) : expression.rhs;
        simplifiedBinaryExpr                              = std::make_unique<syrec::BinaryExpression>(
                lhsOperandOfSimplifiedExpr,
                expression.op,
                rhsOperandOfSimplifiedExpr);
        wasOriginalExprModified = true;
    } else if (!simplifiedBinaryExpr) {
        simplifiedBinaryExpr = copyUtils::createCopyOfExpression(expression);
    }

    std::vector<syrec::VariableAccess::ptr> optimizedAwaySignalAccesses;
    if (const auto& activeSymbolTableScope = getActiveSymbolTableScope(); activeSymbolTableScope.has_value()) {
        if (auto simplificationResultOfExpr = trySimplifyExpr(*simplifiedBinaryExpr, **activeSymbolTableScope, parserConfig.reassociateExpressionsEnabled, parserConfig.operationStrengthReductionEnabled, &optimizedAwaySignalAccesses); simplificationResultOfExpr != nullptr) {
            simplifiedBinaryExpr    = std::move(simplificationResultOfExpr);
            wasOriginalExprModified = true;
        }   
    }

    for (const auto& optimizedAwaySignalAccess : optimizedAwaySignalAccesses) {
        updateReferenceCountOf(optimizedAwaySignalAccess->var->name, parser::SymbolTable::ReferenceCountUpdate::Decrement);
    }

    /*
     * Check whether the both operands of the binary expression are signal accesses that are equal at compile time if the dead code elimination optimization is enabled.
     * Furthermore, if the operation is either a relational or equivalence operation (=, !=), the binary expression can be replaced with the result of the operation given the two equal operands
     */
    if (const auto& finalExprAsBinaryExpr = wasOriginalExprModified ? dynamic_cast<const syrec::BinaryExpression*>(&*simplifiedBinaryExpr) : &expression; finalExprAsBinaryExpr != nullptr && parserConfig.deadCodeEliminationEnabled) {
        if (const auto& equivalenceCheckResultOfOperands = determineEquivalenceOfOperandsOfBinaryExpr(*finalExprAsBinaryExpr); equivalenceCheckResultOfOperands.has_value()) {
            const auto& containerForSimplifiedResult = std::make_shared<syrec::Number>(*equivalenceCheckResultOfOperands);
            simplifiedBinaryExpr                     = std::make_unique<syrec::NumericExpression>(containerForSimplifiedResult, 1);
            wasOriginalExprModified                  = true;

            updateReferenceCountsOfSignalIdentsUsedIn(*finalExprAsBinaryExpr->lhs, parser::SymbolTable::ReferenceCountUpdate::Decrement);
            updateReferenceCountsOfSignalIdentsUsedIn(*finalExprAsBinaryExpr->rhs, parser::SymbolTable::ReferenceCountUpdate::Decrement);
        }
    }

    return wasOriginalExprModified
        ? OptimizationResult<syrec::Expression>::fromOptimizedContainer(std::move(simplifiedBinaryExpr))
        : OptimizationResult<syrec::Expression>::asUnchangedOriginal();
}

optimizations::Optimizer::OptimizationResult<syrec::Expression> optimizations::Optimizer::handleNumericExpr(const syrec::NumericExpression& numericExpr) const {
    if (auto optimizationResultOfUnderlyingNumber = handleNumber(*numericExpr.value); optimizationResultOfUnderlyingNumber.getStatusOfResult() == OptimizationResultFlag::WasOptimized) {
        return OptimizationResult<syrec::Expression>::fromOptimizedContainer(std::make_unique<syrec::NumericExpression>(std::move(optimizationResultOfUnderlyingNumber.tryTakeOwnershipOfOptimizationResult()->front()), numericExpr.bitwidth()));
    }
    return OptimizationResult<syrec::Expression>::asUnchangedOriginal();
}

optimizations::Optimizer::OptimizationResult<syrec::Expression> optimizations::Optimizer::handleVariableExpr(const syrec::VariableExpression& expression) const {
    std::optional<unsigned int> fetchedValueOfSignal;
    if (auto simplificationResultOfUserDefinedSignalAccess = handleSignalAccess(*expression.var, parserConfig.constantPropagationEnabled, &fetchedValueOfSignal); simplificationResultOfUserDefinedSignalAccess.getStatusOfResult() != OptimizationResultFlag::IsUnchanged) {
        if (fetchedValueOfSignal.has_value()) {
            updateReferenceCountOf(expression.var->var->name, parser::SymbolTable::ReferenceCountUpdate::Decrement);
            return OptimizationResult<syrec::Expression>::fromOptimizedContainer(std::make_unique<syrec::NumericExpression>(std::make_unique<syrec::Number>(*fetchedValueOfSignal), expression.bitwidth()));
        }
        if (simplificationResultOfUserDefinedSignalAccess.getStatusOfResult() == OptimizationResultFlag::WasOptimized) {
            return OptimizationResult<syrec::Expression>::fromOptimizedContainer(std::make_unique<syrec::VariableExpression>(std::move(simplificationResultOfUserDefinedSignalAccess.tryTakeOwnershipOfOptimizationResult()->front())));
        }
    }
    return OptimizationResult<syrec::Expression>::asUnchangedOriginal();
}

optimizations::Optimizer::OptimizationResult<syrec::Expression> optimizations::Optimizer::handleShiftExpr(const syrec::ShiftExpression& expression) const {
    std::unique_ptr<syrec::Expression> simplifiedToBeShiftedExpr;
    std::unique_ptr<syrec::Number>     simplifiedShiftAmount;

    if (auto simplificationResultOfToBeShiftedExpr = handleExpr(*expression.lhs); simplificationResultOfToBeShiftedExpr.getStatusOfResult() != OptimizationResultFlag::IsUnchanged) {
        simplifiedToBeShiftedExpr = std::move(simplificationResultOfToBeShiftedExpr.tryTakeOwnershipOfOptimizationResult()->front());
    }

    if (auto simplificationResultOfShiftAmount = handleNumber(*expression.rhs); simplificationResultOfShiftAmount.getStatusOfResult() != OptimizationResultFlag::IsUnchanged) {
        simplifiedShiftAmount = std::move(simplificationResultOfShiftAmount.tryTakeOwnershipOfOptimizationResult()->front());
    }

    if (const auto& mappedToShiftOperation = syrec_operation::tryMapShiftOperationFlagToEnum(expression.op); mappedToShiftOperation.has_value()) {
        std::unique_ptr<syrec::Expression> simplificationResultOfExpr;
        const auto& constantValueOfToBeShiftedExpr = tryEvaluateExpressionToConstant(simplifiedToBeShiftedExpr ? *simplifiedToBeShiftedExpr : *expression.lhs, getActiveSymbolTableForEvaluation(), parserConfig.constantPropagationEnabled, evaluableLoopVariableLookup, nullptr);
        const auto& constantValueOfShiftAmount     = tryEvaluateNumberAsConstant(simplifiedShiftAmount ? *simplifiedShiftAmount : *expression.rhs, getActiveSymbolTableForEvaluation(), parserConfig.constantPropagationEnabled, evaluableLoopVariableLookup, nullptr);

        if (constantValueOfToBeShiftedExpr.has_value() && syrec_operation::isOperandUsedAsLhsInOperationIdentityElement(*mappedToShiftOperation, *constantValueOfToBeShiftedExpr)) {
            if (!simplifiedShiftAmount) {
                simplificationResultOfExpr = std::make_unique<syrec::NumericExpression>(expression.rhs, expression.bitwidth());
            }
            else {
                simplificationResultOfExpr = std::make_unique<syrec::NumericExpression>(std::move(simplifiedShiftAmount), expression.bitwidth());
            }
        } else if (constantValueOfShiftAmount.has_value() && syrec_operation::isOperandUseAsRhsInOperationIdentityElement(*mappedToShiftOperation, *constantValueOfShiftAmount)) {
            if (!simplifiedToBeShiftedExpr) {
                simplificationResultOfExpr = copyUtils::createCopyOfExpression(*expression.lhs);
            } else {
                simplificationResultOfExpr = std::move(simplifiedToBeShiftedExpr);
            }
        }

        if (constantValueOfToBeShiftedExpr.has_value() && constantValueOfShiftAmount.has_value()) {
            if (const auto& evaluationResultOfShiftExprAtCompileTime = syrec_operation::apply(*mappedToShiftOperation, *constantValueOfToBeShiftedExpr, *constantValueOfShiftAmount); evaluationResultOfShiftExprAtCompileTime.has_value()) {
                const auto& containerForResult = std::make_shared<syrec::Number>(*evaluationResultOfShiftExprAtCompileTime);
                simplificationResultOfExpr     = std::make_unique<syrec::NumericExpression>(containerForResult, expression.bitwidth());
            }
        }

        if(simplificationResultOfExpr) {
            return OptimizationResult<syrec::Expression>::fromOptimizedContainer(std::move(simplificationResultOfExpr));   
        }
    }

    if (!simplifiedToBeShiftedExpr && !simplifiedShiftAmount) {
        return OptimizationResult<syrec::Expression>::asUnchangedOriginal();
    }

    syrec::Expression::ptr simplifiedLhsOperand = simplifiedToBeShiftedExpr ? std::move(simplifiedToBeShiftedExpr) : expression.lhs;
    syrec::Number::ptr     simplifiedRhsOperand = simplifiedShiftAmount ? std::move(simplifiedShiftAmount) : expression.rhs;
    auto                   simplifiedShiftExpr  = std::make_unique<syrec::ShiftExpression>(
            simplifiedToBeShiftedExpr ? std::move(simplifiedToBeShiftedExpr) : expression.lhs,
            expression.op,
            simplifiedShiftAmount ? std::move(simplifiedShiftAmount) : expression.rhs);
    return OptimizationResult<syrec::Expression>::fromOptimizedContainer(std::move(simplifiedShiftExpr));
}

optimizations::Optimizer::OptimizationResult<syrec::Number> optimizations::Optimizer::handleNumber(const syrec::Number& number) const {
    if (number.isConstant()) {
        return OptimizationResult<syrec::Number>::asUnchangedOriginal();
    }
    if (number.isLoopVariable()) {
        if (const auto& activeSymbolTableScope = getActiveSymbolTableScope(); activeSymbolTableScope.has_value()) {
            if (const auto& fetchedValueOfLoopVariable = tryFetchValueOfLoopVariable(number.variableName(), **activeSymbolTableScope, parserConfig.constantPropagationEnabled, evaluableLoopVariableLookup); fetchedValueOfLoopVariable.has_value()) {
                updateReferenceCountOf(number.variableName(), parser::SymbolTable::ReferenceCountUpdate::Decrement);
                return OptimizationResult<syrec::Number>::fromOptimizedContainer(std::make_unique<syrec::Number>(*fetchedValueOfLoopVariable));   
            }
        }

        updateReferenceCountOf(number.variableName(), parser::SymbolTable::ReferenceCountUpdate::Increment);
        return OptimizationResult<syrec::Number>::asUnchangedOriginal();
    }
    if (number.isCompileTimeConstantExpression()) {
        if (const auto& activeSymbolTableScope = getActiveSymbolTableScope(); activeSymbolTableScope.has_value()) {
            if (auto simplificationResult = trySimplifyNumber(number); simplificationResult.has_value()) {
                if (std::holds_alternative<unsigned int>(*simplificationResult)) {
                    return OptimizationResult<syrec::Number>::fromOptimizedContainer(std::make_unique<syrec::Number>(std::get<unsigned int>(*simplificationResult)));
                }
                if (std::holds_alternative<std::unique_ptr<syrec::Number>>(*simplificationResult)) {
                    auto containerForSimplifiedNumber = std::move(std::get<std::unique_ptr<syrec::Number>>(*simplificationResult));
                    updateReferenceCountsOfSignalIdentsUsedIn(*containerForSimplifiedNumber, parser::SymbolTable::ReferenceCountUpdate::Increment);
                    return OptimizationResult<syrec::Number>::fromOptimizedContainer(std::move(containerForSimplifiedNumber));
                }
            }    
        }
        
    }
    updateReferenceCountsOfSignalIdentsUsedIn(number, parser::SymbolTable::ReferenceCountUpdate::Increment);
    return OptimizationResult<syrec::Number>::asUnchangedOriginal();
}

std::unique_ptr<syrec::Expression> optimizations::Optimizer::trySimplifyExpr(const syrec::Expression& expr, const parser::SymbolTable& symbolTable, bool simplifyExprByReassociation, bool performOperationStrengthReduction, std::vector<syrec::VariableAccess::ptr>* optimizedAwaySignalAccesses) {
    std::unique_ptr<syrec::Expression> simplificationResult;
    const std::shared_ptr<syrec::Expression> toBeSimplifiedExpr = copyUtils::createCopyOfExpression(expr);
    if (simplifyExprByReassociation) {
        // TODO: Rework return value to either return null if no simplification took place or return std::optional or return additional boolean flag
        if (const auto& reassociateExpressionResult = optimizations::simplifyBinaryExpression(toBeSimplifiedExpr, performOperationStrengthReduction, std::nullopt, symbolTable, optimizedAwaySignalAccesses); reassociateExpressionResult != toBeSimplifiedExpr) {
            simplificationResult = copyUtils::createCopyOfExpression(*reassociateExpressionResult);
        }
    } else {
        if (const auto& generalBinaryExpressionSimplificationResult = optimizations::trySimplify(toBeSimplifiedExpr, performOperationStrengthReduction, symbolTable, optimizedAwaySignalAccesses); generalBinaryExpressionSimplificationResult.couldSimplify) {
            simplificationResult = copyUtils::createCopyOfExpression(*generalBinaryExpressionSimplificationResult.simplifiedExpression);
        }
    }
    return simplificationResult;
}

std::unique_ptr<syrec::Expression> optimizations::Optimizer::transformCompileTimeConstantExpressionToNumber(const syrec::Number::CompileTimeConstantExpression& compileTimeConstantExpr) {
    if (const auto& mappedToExprOperation = tryMapCompileTimeConstantOperation(compileTimeConstantExpr.operation); mappedToExprOperation.has_value()) {
        auto lhsOperandAsExpr = tryMapCompileTimeConstantOperandToExpr(*compileTimeConstantExpr.lhsOperand);
        auto rhsOperandAsExpr = tryMapCompileTimeConstantOperandToExpr(*compileTimeConstantExpr.rhsOperand);
        if (lhsOperandAsExpr && rhsOperandAsExpr) {
            return std::make_unique<syrec::BinaryExpression>(std::move(lhsOperandAsExpr), *syrec_operation::tryMapBinaryOperationEnumToFlag(*mappedToExprOperation), std::move(rhsOperandAsExpr));
        }
    }
    return nullptr;
}

std::unique_ptr<syrec::Expression> optimizations::Optimizer::tryMapCompileTimeConstantOperandToExpr(const syrec::Number& number) {
    if (number.isConstant()) {
        return std::make_unique<syrec::NumericExpression>(std::make_unique<syrec::Number>(number.evaluate({})), BitHelpers::getRequiredBitsToStoreValue(number.evaluate({})));
    }
    if (number.isLoopVariable()) {
        /*
         * TODO: Can we determine the bitwidth of the given loop variable or in other words, does the symbol table entry for the given loop variable
         * contain the maximum value for the given loop variable
         */
        return std::make_unique<syrec::NumericExpression>(std::make_unique<syrec::Number>(number.variableName()), 1);
    }
    if (number.isCompileTimeConstantExpression()) {
        return transformCompileTimeConstantExpressionToNumber(number.getExpression());
    }
    return nullptr;
}

std::unique_ptr<syrec::Number> optimizations::Optimizer::tryMapExprToCompileTimeConstantExpr(const syrec::Expression& expr) {
    if (const auto& exprAsBinaryExpr = dynamic_cast<const syrec::BinaryExpression*>(&expr); exprAsBinaryExpr) {
        if (const auto& mappedToBinaryOp = syrec_operation::tryMapBinaryOperationFlagToEnum(exprAsBinaryExpr->op); mappedToBinaryOp.has_value() && tryMapOperationToCompileTimeConstantOperation(*mappedToBinaryOp).has_value()) {
            auto lhsExprAsOperand = tryMapExprToCompileTimeConstantExpr(*exprAsBinaryExpr->lhs);
            auto rhsExprAsOperand = tryMapExprToCompileTimeConstantExpr(*exprAsBinaryExpr->rhs);
            if (lhsExprAsOperand && rhsExprAsOperand) {
                return std::make_unique<syrec::Number>(syrec::Number::CompileTimeConstantExpression(std::move(lhsExprAsOperand), *tryMapOperationToCompileTimeConstantOperation(*mappedToBinaryOp), std::move(rhsExprAsOperand)));       
            }
        }
    }
    if (const auto& exprAsNumericExpr = dynamic_cast<const syrec::NumericExpression*>(&expr); exprAsNumericExpr) {
        return copyUtils::createCopyOfNumber(*exprAsNumericExpr->value);   
    }
    return nullptr;
}

std::optional<std::variant<unsigned, std::unique_ptr<syrec::Number>>> optimizations::Optimizer::trySimplifyNumber(const syrec::Number& number) const {
    if (number.isConstant()) {
        return std::make_optional(number.evaluate({}));
    }
    if (!number.isCompileTimeConstantExpression()) {
        return std::nullopt;
    }

    if (const auto& compileTimeExprAsBinaryExpr = createBinaryExprFromCompileTimeConstantExpr(number.getExpression()); compileTimeExprAsBinaryExpr) {
        if (auto simplificationResultOfExpr = handleExpr(*compileTimeExprAsBinaryExpr); simplificationResultOfExpr.getStatusOfResult() == OptimizationResultFlag::WasOptimized) {
            if (auto owningContainerOfSimplificationResult = simplificationResultOfExpr.tryTakeOwnershipOfOptimizationResult(); owningContainerOfSimplificationResult.has_value()) {
                if (auto simplifiedExprAsCompileTimeConstantExpr = tryMapExprToCompileTimeConstantExpr(*owningContainerOfSimplificationResult->front()); simplifiedExprAsCompileTimeConstantExpr) {
                    return simplifiedExprAsCompileTimeConstantExpr;
                }   
            }
        }
    }
    return std::nullopt;
}


std::optional<syrec_operation::operation> optimizations::Optimizer::tryMapCompileTimeConstantOperation(syrec::Number::CompileTimeConstantExpression::Operation compileTimeConstantExprOperation) {
    switch (compileTimeConstantExprOperation) {
        case syrec::Number::CompileTimeConstantExpression::Operation::Addition:
            return std::make_optional(syrec_operation::operation::Addition);
        case syrec::Number::CompileTimeConstantExpression::Operation::Subtraction:
            return std::make_optional(syrec_operation::operation::Subtraction);
        case syrec::Number::CompileTimeConstantExpression::Operation::Multiplication:
            return std::make_optional(syrec_operation::operation::Multiplication);
        case syrec::Number::CompileTimeConstantExpression::Operation::Division:
            return std::make_optional(syrec_operation::operation::Division);
        default:
            return std::nullopt;
    }
}

std::optional<syrec::Number::CompileTimeConstantExpression::Operation> optimizations::Optimizer::tryMapOperationToCompileTimeConstantOperation(syrec_operation::operation operation) {
    switch (operation) {
        case syrec_operation::operation::Addition:
            return std::make_optional(syrec::Number::CompileTimeConstantExpression::Addition);
        case syrec_operation::operation::Subtraction:
            return std::make_optional(syrec::Number::CompileTimeConstantExpression::Addition);
        case syrec_operation::operation::Multiplication:
            return std::make_optional(syrec::Number::CompileTimeConstantExpression::Multiplication);
        case syrec_operation::operation::Division:
            return std::make_optional(syrec::Number::CompileTimeConstantExpression::Division);
        default:
            return std::nullopt;
    }
}

void optimizations::Optimizer::updateReferenceCountOf(const std::string_view& signalIdent, parser::SymbolTable::ReferenceCountUpdate typeOfUpdate) const {
    if (const auto& activeSymbolTableScope = getActiveSymbolTableScope(); activeSymbolTableScope.has_value() && internalReferenceCountModificationEnabledFlag.value_or(true)) {
        activeSymbolTableScope->get()->updateReferenceCountOfLiteral(signalIdent, typeOfUpdate);
    }
}

std::optional<unsigned> optimizations::Optimizer::tryFetchValueForAccessedSignal(const syrec::VariableAccess& accessedSignal) const {
    if (!parserConfig.constantPropagationEnabled || isValueLookupBlockedByDataFlowAnalysisRestriction(accessedSignal)) {
        return std::nullopt;
    }

    if (auto evaluatedSignalAccess = tryEvaluateDefinedSignalAccess(accessedSignal); evaluatedSignalAccess.has_value()) {
        return tryFetchValueFromEvaluatedSignalAccess(*evaluatedSignalAccess);
    }
    return std::nullopt;
}

optimizations::Optimizer::DimensionAccessEvaluationResult optimizations::Optimizer::evaluateUserDefinedDimensionAccess(const std::string_view& accessedSignalIdent, const std::vector<std::reference_wrapper<const syrec::Expression>>& accessedValuePerDimension) const {
    if (accessedValuePerDimension.empty()) {
        return DimensionAccessEvaluationResult({false, std::vector<SignalAccessIndexEvaluationResult<syrec::Expression>>(), false});
    }

    const auto& signalData = *getSymbolTableEntryForVariable(accessedSignalIdent);
    const auto wasUserDefinedAccessTrimmed = accessedValuePerDimension.size() > signalData->dimensions.size();
    const auto numEntriesToTransform       = wasUserDefinedAccessTrimmed ? signalData->dimensions.size() : accessedValuePerDimension.size();
    
    std::vector<SignalAccessIndexEvaluationResult<syrec::Expression>> evaluatedValuePerDimension;
    evaluatedValuePerDimension.reserve(numEntriesToTransform);

    bool        wasAnyUserDefinedDimensionAccessSimplified = false;
    const auto& symbolTableScopeUsableForEvaluation = getActiveSymbolTableForEvaluation();
    for (std::size_t i = 0; i < numEntriesToTransform; ++i) {
        const auto& userDefinedValueOfDimension = accessedValuePerDimension.at(i);
        bool        wasUserDefinedValueForDimensionSimplified = false;
        bool        didUserDefinedValueOfDimensionEvaluateToConstant = false;

        // TODO: Should we handle the case that the optimization did remove the user defined value of the dimension which in a correct implementation should not happen
        if (auto optimizationResultOfValueOfDimension = handleExpr(userDefinedValueOfDimension); optimizationResultOfValueOfDimension.getStatusOfResult() != OptimizationResultFlag::RemovedByOptimization) {
            std::unique_ptr<syrec::Expression> owningContainerOfOptimizationResult;
            if (optimizationResultOfValueOfDimension.getStatusOfResult() == OptimizationResultFlag::IsUnchanged) {
                owningContainerOfOptimizationResult = copyUtils::createCopyOfExpression(userDefinedValueOfDimension);
            }
            else if (auto temporaryOwningContainerOfOptimizationResult = optimizationResultOfValueOfDimension.tryTakeOwnershipOfOptimizationResult(); temporaryOwningContainerOfOptimizationResult.has_value()) {
                owningContainerOfOptimizationResult = std::move(temporaryOwningContainerOfOptimizationResult->front());
                wasUserDefinedValueForDimensionSimplified = true;
            }
            if (const auto& constantValueOfAccessedValueOfDimension = tryEvaluateExpressionToConstant(*owningContainerOfOptimizationResult, symbolTableScopeUsableForEvaluation, parserConfig.constantPropagationEnabled, evaluableLoopVariableLookup, &didUserDefinedValueOfDimensionEvaluateToConstant); constantValueOfAccessedValueOfDimension.has_value()) {
                const auto isAccessedValueOfDimensionWithinRange = parser::isValidDimensionAccess(signalData, i, *constantValueOfAccessedValueOfDimension);
                evaluatedValuePerDimension.emplace_back(SignalAccessIndexEvaluationResult<syrec::Expression>::createFromConstantValue(isAccessedValueOfDimensionWithinRange ? IndexValidityStatus::Valid : IndexValidityStatus::OutOfRange, wasUserDefinedValueForDimensionSimplified, *constantValueOfAccessedValueOfDimension));
                wasUserDefinedValueForDimensionSimplified |= didUserDefinedValueOfDimensionEvaluateToConstant;
            }
            else {
                evaluatedValuePerDimension.emplace_back(SignalAccessIndexEvaluationResult<syrec::Expression>::createFromSimplifiedNonConstantValue(IndexValidityStatus::Unknown, false, owningContainerOfOptimizationResult ? std::move(owningContainerOfOptimizationResult) : copyUtils::createCopyOfExpression(userDefinedValueOfDimension)));
            }
        }
        wasAnyUserDefinedDimensionAccessSimplified |= wasUserDefinedValueForDimensionSimplified;
    }
    return DimensionAccessEvaluationResult({.wasTrimmed = wasUserDefinedAccessTrimmed, .valuePerDimension = std::move(evaluatedValuePerDimension), .wasAnyExprSimplified = wasAnyUserDefinedDimensionAccessSimplified});
}

std::optional<unsigned> optimizations::Optimizer::tryEvaluateNumberAsConstant(const syrec::Number& number, const parser::SymbolTable* symbolTableUsedForEvaluation, bool shouldPerformConstantPropagation, const syrec::Number::loop_variable_mapping& evaluableLoopVariablesIfConstantPropagationIsDisabled, bool *wasOriginalNumberSimplified) {
    if (number.isConstant()) {
        return std::make_optional(number.evaluate({}));
    }

    if (number.isLoopVariable() && symbolTableUsedForEvaluation) {
        if (const auto& fetchedValueOfLoopVariable = tryFetchValueOfLoopVariable(number.variableName(), *symbolTableUsedForEvaluation, shouldPerformConstantPropagation, evaluableLoopVariablesIfConstantPropagationIsDisabled); fetchedValueOfLoopVariable.has_value()) {
            if (wasOriginalNumberSimplified) {
                *wasOriginalNumberSimplified = true;
            }
            return fetchedValueOfLoopVariable;
        }
        return std::nullopt;
    }
    // TODO: handling and simplification of compile time constant expressions
    if (number.isCompileTimeConstantExpression()) {
        return std::nullopt;
    }
    return std::nullopt;
}

std::optional<optimizations::Optimizer::BitRangeEvaluationResult> optimizations::Optimizer::evaluateUserDefinedBitRangeAccess(const std::string_view& accessedSignalIdent, const std::optional<std::pair<const syrec::Number*, const syrec::Number*>>& accessedBitRange) const {
    const auto& signalData = getSymbolTableEntryForVariable(accessedSignalIdent);
    if (!signalData.has_value() || !accessedBitRange.has_value()) {
        return std::nullopt;
    }

    /*
     * TODO: Assuming that the original bit range start/end component is assumed to be unchanged in case that either the optimization failed or said component was removed
     * could lead to erroneous result that ultimately lead to invalid SyRec programs. We could resolve this problem, by introducing a failed result for the SignalAccessIndexEvaluationResult
     */
    bool wasOriginalBitRangeStartSimplified = false;
    auto bitRangeStartEvaluationStatus      = IndexValidityStatus::Unknown;
    std::unique_ptr<syrec::Number> evaluationResultOfBitRangeStart;
    if (auto optimizationResultOfBitRangeStart = handleNumber(*accessedBitRange->first); optimizationResultOfBitRangeStart.getStatusOfResult() != OptimizationResultFlag::RemovedByOptimization) {
        if (auto temporaryContainerOwningOptimizationResult = optimizationResultOfBitRangeStart.tryTakeOwnershipOfOptimizationResult(); temporaryContainerOwningOptimizationResult.has_value()) {
            evaluationResultOfBitRangeStart = std::move(temporaryContainerOwningOptimizationResult->front());
        }
        wasOriginalBitRangeStartSimplified = optimizationResultOfBitRangeStart.getStatusOfResult() == OptimizationResultFlag::WasOptimized;
    }

    if (!evaluationResultOfBitRangeStart) {
        evaluationResultOfBitRangeStart = copyUtils::createCopyOfNumber(*accessedBitRange->first);
    }

    bool wasOriginalBitRangeEndSimplified = false;
    auto bitRangeEndEvaluationStatus      = IndexValidityStatus::Unknown;
    std::unique_ptr<syrec::Number> evaluationResultOfBitRangeEnd;

    if (accessedBitRange->first != accessedBitRange->second) {
        if (auto optimizationResultOfBitRangeEnd = handleNumber(*accessedBitRange->second); optimizationResultOfBitRangeEnd.getStatusOfResult() != OptimizationResultFlag::WasOptimized) {
            if (auto temporaryContainerOwningOptimizationResult = optimizationResultOfBitRangeEnd.tryTakeOwnershipOfOptimizationResult(); temporaryContainerOwningOptimizationResult.has_value()) {
                evaluationResultOfBitRangeEnd = std::move(temporaryContainerOwningOptimizationResult->front());
            }
            wasOriginalBitRangeEndSimplified = optimizationResultOfBitRangeEnd.getStatusOfResult() == OptimizationResultFlag::WasOptimized;
        }

        if (!evaluationResultOfBitRangeEnd) {
            evaluationResultOfBitRangeEnd = copyUtils::createCopyOfNumber(*accessedBitRange->second);
        }   
    }
    else {
        evaluationResultOfBitRangeEnd = copyUtils::createCopyOfNumber(*evaluationResultOfBitRangeStart);
        wasOriginalBitRangeEndSimplified = wasOriginalBitRangeStartSimplified;
    }

    const auto& bitRangeStartEvaluated = tryEvaluateNumberAsConstant(*evaluationResultOfBitRangeStart, getActiveSymbolTableForEvaluation(), parserConfig.constantPropagationEnabled, evaluableLoopVariableLookup, &wasOriginalBitRangeStartSimplified);
    const auto& bitRangeEndEvaluated   = tryEvaluateNumberAsConstant(*evaluationResultOfBitRangeEnd, getActiveSymbolTableForEvaluation(), parserConfig.constantPropagationEnabled, evaluableLoopVariableLookup, &wasOriginalBitRangeEndSimplified);

    if (bitRangeStartEvaluated.has_value()) {
        bitRangeStartEvaluationStatus = parser::isValidBitAccess(*signalData, *bitRangeStartEvaluated) ? IndexValidityStatus::Valid : IndexValidityStatus::OutOfRange;
    }

    if (bitRangeEndEvaluated.has_value()) {
        bitRangeEndEvaluationStatus = parser::isValidBitAccess(*signalData, *bitRangeEndEvaluated) ? IndexValidityStatus::Valid : IndexValidityStatus::OutOfRange;
    }

    auto rangeStartEvaluationResult = bitRangeStartEvaluationStatus == IndexValidityStatus::Unknown
        ? SignalAccessIndexEvaluationResult<syrec::Number>::createFromSimplifiedNonConstantValue(bitRangeStartEvaluationStatus, false, std::move(evaluationResultOfBitRangeStart))
        : SignalAccessIndexEvaluationResult<syrec::Number>::createFromConstantValue(bitRangeStartEvaluationStatus, wasOriginalBitRangeStartSimplified , *bitRangeStartEvaluated);

    auto rangeEndEvaluationResult = bitRangeEndEvaluationStatus == IndexValidityStatus::Unknown
        ? SignalAccessIndexEvaluationResult<syrec::Number>::createFromSimplifiedNonConstantValue(bitRangeEndEvaluationStatus, false, std::move(evaluationResultOfBitRangeEnd))
        : SignalAccessIndexEvaluationResult<syrec::Number>::createFromConstantValue(bitRangeEndEvaluationStatus, wasOriginalBitRangeEndSimplified, *bitRangeEndEvaluated);

    return std::make_optional(BitRangeEvaluationResult({.rangeStartEvaluationResult = std::move(rangeStartEvaluationResult), .rangeEndEvaluationResult = std::move(rangeEndEvaluationResult)}));
}

std::optional<unsigned> optimizations::Optimizer::tryEvaluateExpressionToConstant(const syrec::Expression& expr, const parser::SymbolTable* symbolTableUsedForEvaluation, bool shouldPerformConstantPropagation, const syrec::Number::loop_variable_mapping& evaluableLoopVariablesIfConstantPropagationIsDisabled, bool* wasOriginalExprSimplified) {
    if (const auto& exprAsNumericExpr = dynamic_cast<const syrec::NumericExpression*>(&expr); exprAsNumericExpr) {
        return tryEvaluateNumberAsConstant(*exprAsNumericExpr->value, symbolTableUsedForEvaluation, shouldPerformConstantPropagation, evaluableLoopVariablesIfConstantPropagationIsDisabled, wasOriginalExprSimplified);
    }
    return std::nullopt;
}

void optimizations::Optimizer::updateReferenceCountsOfSignalIdentsUsedIn(const syrec::Number& number, parser::SymbolTable::ReferenceCountUpdate typeOfUpdate) const {
    if (number.isLoopVariable()) {
        updateReferenceCountOf(number.variableName(), typeOfUpdate);
    } else if (number.isCompileTimeConstantExpression()) {
        updateReferenceCountsOfSignalIdentsUsedIn(*number.getExpression().lhsOperand, typeOfUpdate);
        updateReferenceCountsOfSignalIdentsUsedIn(*number.getExpression().rhsOperand, typeOfUpdate);
    }
}

void optimizations::Optimizer::updateReferenceCountsOfSignalIdentsUsedIn(const syrec::Expression& expr, parser::SymbolTable::ReferenceCountUpdate typeOfUpdate) const {
    SignalIdentCountLookup signalIdentCountLookup;
    determineUsedSignalIdentsIn(expr, signalIdentCountLookup);

    for (const auto& [signalIdent, occurrenceCount] : signalIdentCountLookup.lookup) {
        for (std::size_t i = 0; i < occurrenceCount; ++i) {
            updateReferenceCountOf(signalIdent, typeOfUpdate);    
        }
    }
}

void optimizations::Optimizer::createSymbolTableEntryForVariable(parser::SymbolTable& symbolTable, const syrec::Variable& variable) {
    if(!symbolTable.contains(variable.name)) {
        symbolTable.addEntry(variable);
    }
}

void optimizations::Optimizer::createSymbolTableEntriesForModuleParametersAndLocalVariables(parser::SymbolTable& symbolTable, const syrec::Module& module) {
    for (const auto& parameter : module.parameters) {
        createSymbolTableEntryForVariable(symbolTable, *parameter);
    }

    for (const auto& localVariable : module.variables) {
        createSymbolTableEntryForVariable(symbolTable, *localVariable);
    }
}


std::optional<syrec::Variable::ptr> optimizations::Optimizer::getSymbolTableEntryForVariable(const std::string_view& signalLiteralIdent) const {
    if (const auto& activeSymbolTableScope = getActiveSymbolTableScope(); activeSymbolTableScope.has_value()) {
        if (const auto& matchingSymbolTableEntryForIdent = activeSymbolTableScope->get()->getVariable(signalLiteralIdent); matchingSymbolTableEntryForIdent.has_value() && std::holds_alternative<syrec::Variable::ptr>(*matchingSymbolTableEntryForIdent)) {
            return std::get<syrec::Variable::ptr>(*matchingSymbolTableEntryForIdent);
        }
    }
    return std::nullopt;
}

std::optional<syrec::Number::ptr> optimizations::Optimizer::getSymbolTableEntryForLoopVariable(const std::string_view& loopVariableIdent) const {
    if (const auto& activeSymbolTableScope = getActiveSymbolTableScope(); activeSymbolTableScope.has_value()) {
        if (const auto& matchingSymbolTableEntryForIdent = activeSymbolTableScope->get()->getVariable(loopVariableIdent); matchingSymbolTableEntryForIdent.has_value() && std::holds_alternative<syrec::Number::ptr>(*matchingSymbolTableEntryForIdent)) {
            return std::get<syrec::Number::ptr>(*matchingSymbolTableEntryForIdent);
        }
    }
    return std::nullopt;
}

std::optional<parser::SymbolTable::ptr> optimizations::Optimizer::getActiveSymbolTableScope() const {
    return activeSymbolTableScope ? std::make_optional(activeSymbolTableScope) : std::nullopt;
}

void optimizations::Optimizer::openNewSymbolTableScope() {
    parser::SymbolTable::openScope(activeSymbolTableScope);
}

void optimizations::Optimizer::closeActiveSymbolTableScope() {
    parser::SymbolTable::closeScope(activeSymbolTableScope);
}

/*
 * We are only using this function to convert the given compile time constant expression to a binary expression to reuse the optimization functionality of the latter for the former
 * and are performing the reverse mapping after the optimization, the bitwidths of the expressions representing the operands of the compile time constant expressions need not to be known.
 */
std::unique_ptr<syrec::BinaryExpression> optimizations::Optimizer::createBinaryExprFromCompileTimeConstantExpr(const syrec::Number::CompileTimeConstantExpression& compileTimeConstantExpression) {
    if (const auto& mappedToBinaryOperation = tryMapCompileTimeConstantOperation(compileTimeConstantExpression.operation); mappedToBinaryOperation.has_value()) {
        syrec::Expression::ptr exprForLhsOperand;
        if (compileTimeConstantExpression.lhsOperand->isCompileTimeConstantExpression()) {
            exprForLhsOperand = createBinaryExprFromCompileTimeConstantExpr(compileTimeConstantExpression.lhsOperand->getExpression());
        }
        else {
            exprForLhsOperand = std::make_unique<syrec::NumericExpression>(compileTimeConstantExpression.lhsOperand, 0);
        }

        syrec::Expression::ptr exprForRhsOperand;
        if (compileTimeConstantExpression.rhsOperand->isCompileTimeConstantExpression()) {
            exprForRhsOperand = createBinaryExprFromCompileTimeConstantExpr(compileTimeConstantExpression.rhsOperand->getExpression());
        } else {
            exprForRhsOperand = std::make_unique<syrec::NumericExpression>(compileTimeConstantExpression.rhsOperand, 0);
        }

        if (exprForLhsOperand && exprForRhsOperand && syrec_operation::tryMapBinaryOperationEnumToFlag(*mappedToBinaryOperation)) {
            return std::make_unique<syrec::BinaryExpression>(exprForLhsOperand, *syrec_operation::tryMapBinaryOperationEnumToFlag(*mappedToBinaryOperation), exprForRhsOperand);
        }
    }
    return nullptr;
}

void optimizations::Optimizer::determineUsedSignalIdentsIn(const syrec::Expression& expr, SignalIdentCountLookup& lookupContainer) {
    if (const auto& exprAsBinaryExpr = dynamic_cast<const syrec::BinaryExpression*>(&expr); exprAsBinaryExpr != nullptr) {
        determineUsedSignalIdentsIn(*exprAsBinaryExpr->lhs, lookupContainer);
        determineUsedSignalIdentsIn(*exprAsBinaryExpr->rhs, lookupContainer);
    } else if (const auto& exprAsShiftExpr = dynamic_cast<const syrec::ShiftExpression*>(&expr); exprAsShiftExpr != nullptr) {
        determineUsedSignalIdentsIn(*exprAsShiftExpr->lhs, lookupContainer);
        determineUsedSignalIdentsIn(*exprAsShiftExpr->rhs, lookupContainer);
    } else if (const auto& exprAsVariableExpr = dynamic_cast<const syrec::VariableExpression*>(&expr); exprAsVariableExpr != nullptr) {
        addSignalIdentToLookup(exprAsVariableExpr->var->var->name, lookupContainer);
    } else if (const auto& exprAsNumericExpr = dynamic_cast<const syrec::NumericExpression*>(&expr); exprAsNumericExpr != nullptr) {
        determineUsedSignalIdentsIn(*exprAsNumericExpr->value, lookupContainer);
    }
}

void optimizations::Optimizer::determineUsedSignalIdentsIn(const syrec::Number& number, SignalIdentCountLookup& lookupContainer) {
    if (number.isLoopVariable()) {
        addSignalIdentToLookup(number.variableName(), lookupContainer);
    }
    if (number.isCompileTimeConstantExpression()) {
        determineUsedSignalIdentsIn(*number.getExpression().lhsOperand, lookupContainer);
        determineUsedSignalIdentsIn(*number.getExpression().rhsOperand, lookupContainer);
    }
}

void optimizations::Optimizer::addSignalIdentToLookup(const std::string_view& signalIdent, SignalIdentCountLookup& lookupContainer) {
    if (lookupContainer.lookup.find(signalIdent) != lookupContainer.lookup.end()) {
        lookupContainer.lookup.at(signalIdent)++;
    }else {
        lookupContainer.lookup.insert(std::make_pair(signalIdent, 1));
    }
}

// TODO: Conversion from smart pointers to reference wrappers for dimension as well as bit range access
std::optional<optimizations::Optimizer::EvaluatedSignalAccess> optimizations::Optimizer::tryEvaluateDefinedSignalAccess(const syrec::VariableAccess& accessedSignalParts) const {
    if (const auto& symbolTableEntryForAccessedSignal = getSymbolTableEntryForVariable(accessedSignalParts.var->name); symbolTableEntryForAccessedSignal.has_value()) {
        const std::string_view& accessedSignalIdent = accessedSignalParts.var->name;

        std::vector<std::reference_wrapper<const syrec::Expression>> accessedValueOfDimensionWithoutOwnership;
        accessedValueOfDimensionWithoutOwnership.reserve(accessedSignalParts.indexes.size());

        for (const auto& indexe: accessedSignalParts.indexes) {
            accessedValueOfDimensionWithoutOwnership.emplace_back(*indexe);
        }

        std::optional<std::pair<const syrec::Number*, const syrec::Number*>> accessedBitRangeWithoutOwnership;
        if (accessedSignalParts.range.has_value()) {
            accessedBitRangeWithoutOwnership.emplace(std::make_pair(accessedSignalParts.range->first.get(), accessedSignalParts.range->second.get()));
        }

        auto evaluatedUserDefinedDimensionAccess = evaluateUserDefinedDimensionAccess(accessedSignalIdent, accessedValueOfDimensionWithoutOwnership);
        auto evaluatedUserDefinedBitRangeAccess  = evaluateUserDefinedBitRangeAccess(accessedSignalIdent, accessedBitRangeWithoutOwnership);
        const bool  wasAnyIndexComponentOfUserDefinedSignalAccessOptimized = evaluatedUserDefinedDimensionAccess.wasAnyExprSimplified
            || (evaluatedUserDefinedBitRangeAccess.has_value() && (evaluatedUserDefinedBitRangeAccess->rangeStartEvaluationResult.wasToBeEvaluatedIndexSimplified || evaluatedUserDefinedBitRangeAccess->rangeEndEvaluationResult.wasToBeEvaluatedIndexSimplified));

        return std::make_optional(EvaluatedSignalAccess({accessedSignalParts.var->name,
                                                         std::move(evaluatedUserDefinedDimensionAccess),
                                                         std::move(evaluatedUserDefinedBitRangeAccess), wasAnyIndexComponentOfUserDefinedSignalAccessOptimized}));
    }
    return std::nullopt;
}

std::unique_ptr<syrec::VariableAccess> optimizations::Optimizer::transformEvaluatedSignalAccess(const EvaluatedSignalAccess& evaluatedSignalAccess, std::optional<bool>* wasAnyDefinedIndexOutOfRange, bool* didAllDefinedIndicesEvaluateToConstants) const {
    if (const auto& matchingSymbolTableEntryForIdent = getSymbolTableEntryForVariable(evaluatedSignalAccess.accessedSignalIdent); matchingSymbolTableEntryForIdent.has_value()) {
        if (didAllDefinedIndicesEvaluateToConstants) {
            *didAllDefinedIndicesEvaluateToConstants = true;
        }

        std::optional<std::pair<syrec::Number::ptr, syrec::Number::ptr>> definedBitRangeAccess;
        std::vector<syrec::Expression::ptr>                              definedDimensionAccess;
        definedDimensionAccess.reserve(evaluatedSignalAccess.accessedValuePerDimension.valuePerDimension.size());

        if (evaluatedSignalAccess.accessedBitRange.has_value()) {
            syrec::Number::ptr bitRangeStart;
            syrec::Number::ptr bitRangeEnd;

            if (const auto& constantValueOfBitRangeStart = tryFetchConstantValueOfIndex(evaluatedSignalAccess.accessedBitRange->rangeStartEvaluationResult); constantValueOfBitRangeStart.has_value()) {
                bitRangeStart = std::make_shared<syrec::Number>(*constantValueOfBitRangeStart);
                if (didAllDefinedIndicesEvaluateToConstants) {
                    *didAllDefinedIndicesEvaluateToConstants = true;
                }
            } else {
                bitRangeStart = copyUtils::createCopyOfNumber(tryFetchNonOwningReferenceOfNonConstantValueOfIndex(evaluatedSignalAccess.accessedBitRange->rangeStartEvaluationResult)->get());
                if (didAllDefinedIndicesEvaluateToConstants) {
                    *didAllDefinedIndicesEvaluateToConstants = false;
                }
            }

            if (wasAnyDefinedIndexOutOfRange) {
                *wasAnyDefinedIndexOutOfRange = evaluatedSignalAccess.accessedBitRange->rangeStartEvaluationResult.validityStatus == IndexValidityStatus::OutOfRange;
            }

            if (const auto& constantValueOfBitRangeEnd = tryFetchConstantValueOfIndex(evaluatedSignalAccess.accessedBitRange->rangeEndEvaluationResult); constantValueOfBitRangeEnd.has_value()) {
                bitRangeEnd = std::make_shared<syrec::Number>(*constantValueOfBitRangeEnd);
                if (didAllDefinedIndicesEvaluateToConstants) {
                    *didAllDefinedIndicesEvaluateToConstants &= true;
                }
            } else {
                bitRangeEnd = copyUtils::createCopyOfNumber(tryFetchNonOwningReferenceOfNonConstantValueOfIndex(evaluatedSignalAccess.accessedBitRange->rangeEndEvaluationResult)->get());
                if (didAllDefinedIndicesEvaluateToConstants) {
                    *didAllDefinedIndicesEvaluateToConstants = false;
                }
            }

            if (wasAnyDefinedIndexOutOfRange) {
                const auto& wasBitRangeEndOutOfRange = evaluatedSignalAccess.accessedBitRange->rangeEndEvaluationResult.validityStatus == IndexValidityStatus::OutOfRange;
                *wasAnyDefinedIndexOutOfRange        = wasAnyDefinedIndexOutOfRange->value_or(false) | wasBitRangeEndOutOfRange;
            }

            definedBitRangeAccess = std::make_pair(bitRangeStart, bitRangeEnd);
        }
        
        for (auto& evaluatedValueOfDimension : evaluatedSignalAccess.accessedValuePerDimension.valuePerDimension) {
            if (const auto& constantValueOfValueOfDimension = tryFetchConstantValueOfIndex(evaluatedValueOfDimension); constantValueOfValueOfDimension.has_value()) {
                definedDimensionAccess.emplace_back(std::make_shared<syrec::NumericExpression>(std::make_shared<syrec::Number>(*constantValueOfValueOfDimension), BitHelpers::getRequiredBitsToStoreValue(*constantValueOfValueOfDimension)));
                if (didAllDefinedIndicesEvaluateToConstants) {
                    *didAllDefinedIndicesEvaluateToConstants &= true;
                }
            } else {
                auto copyOfEvaluatedValueOfDimension = copyUtils::createCopyOfExpression(tryFetchNonOwningReferenceOfNonConstantValueOfIndex(evaluatedValueOfDimension)->get());
                definedDimensionAccess.emplace_back(std::move(copyOfEvaluatedValueOfDimension));
                if (didAllDefinedIndicesEvaluateToConstants) {
                    *didAllDefinedIndicesEvaluateToConstants = false;
                }
            }

            const auto& wasDefinedValueOfDimensionOutOfRange = evaluatedValueOfDimension.validityStatus == IndexValidityStatus::OutOfRange;
            if (wasAnyDefinedIndexOutOfRange) {
                *wasAnyDefinedIndexOutOfRange = wasAnyDefinedIndexOutOfRange->value_or(false) | wasDefinedValueOfDimensionOutOfRange;
            }
        }

        auto transformedToSignalAccess     = std::make_unique<syrec::VariableAccess>();
        transformedToSignalAccess->var     = *matchingSymbolTableEntryForIdent;
        transformedToSignalAccess->indexes = definedDimensionAccess;
        transformedToSignalAccess->range   = definedBitRangeAccess;
        return transformedToSignalAccess;
    }
    return nullptr;
}

/*
 * TODO: Removed additional pointer conversion when symbol table can be called with references instead of smart pointers
 */
void optimizations::Optimizer::invalidateValueOfAccessedSignalParts(const EvaluatedSignalAccess& accessedSignalParts) {
    if (auto transformedSignalAccess = transformEvaluatedSignalAccess(accessedSignalParts, nullptr, nullptr); transformedSignalAccess != nullptr) {
        const std::shared_ptr<syrec::VariableAccess> sharedTransformedSignalAccess = std::move(transformedSignalAccess);
        invalidateStoredValueFor(sharedTransformedSignalAccess);
    }
}

void optimizations::Optimizer::invalidateValueOfWholeSignal(const std::string_view& signalIdent) {
    if (const auto& matchingSymbolTableEntry = getSymbolTableEntryForVariable(signalIdent); matchingSymbolTableEntry.has_value()) {
        const auto& sharedTransformedSignalAccess = std::make_shared<syrec::VariableAccess>();
        sharedTransformedSignalAccess->var        = *matchingSymbolTableEntry;
        invalidateStoredValueFor(sharedTransformedSignalAccess);
    }
}

/*
 * TODO: Removed additional pointer conversion when symbol table can be called with references instead of smart pointers
 */
void optimizations::Optimizer::performAssignment(const EvaluatedSignalAccess& assignedToSignalParts, syrec_operation::operation assignmentOperation, const std::optional<unsigned int>& assignmentRhsValue) {
    std::optional<bool> areAnyComponentsOfAssignedToSignalAccessOutOfRange;
    bool                didAllDefinedIndicesEvaluateToConstants = false;
    if (auto transformedSignalAccess = transformEvaluatedSignalAccess(assignedToSignalParts, &areAnyComponentsOfAssignedToSignalAccessOutOfRange, &didAllDefinedIndicesEvaluateToConstants); transformedSignalAccess != nullptr) {
        const std::shared_ptr<syrec::VariableAccess> sharedTransformedSignalAccess = std::move(transformedSignalAccess);

        if (!areAnyComponentsOfAssignedToSignalAccessOutOfRange.value_or(false) && didAllDefinedIndicesEvaluateToConstants && assignmentRhsValue.has_value()) {
            if (const auto& currentValueOfAssignedToSignal = tryFetchValueFromEvaluatedSignalAccess(assignedToSignalParts); currentValueOfAssignedToSignal.has_value()) {
                if (isOperationUnaryAssignmentOperation(assignmentOperation) && currentValueOfAssignedToSignal.has_value()) {
                    if (const auto evaluationResultOfOperation = syrec_operation::apply(assignmentOperation, *currentValueOfAssignedToSignal); evaluationResultOfOperation.has_value()) {
                        updateStoredValueOf(sharedTransformedSignalAccess, *evaluationResultOfOperation);
                        return;
                    }
                } else if (const auto evaluationResultOfOperation = syrec_operation::apply(assignmentOperation, currentValueOfAssignedToSignal, assignmentRhsValue); evaluationResultOfOperation.has_value()) {
                    updateStoredValueOf(sharedTransformedSignalAccess, *evaluationResultOfOperation);
                    return;
                }
            }   
        }
        invalidateStoredValueFor(sharedTransformedSignalAccess);
    }
}

/*
 * TODO: Removed additional pointer conversion when symbol table can be called with references instead of smart pointers
 */
void optimizations::Optimizer::performAssignment(const EvaluatedSignalAccess& assignmentLhsOperand, syrec_operation::operation assignmentOperation, const EvaluatedSignalAccess& assignmentRhsOperand) {
    if (const auto fetchedValueForAssignmentRhsOperand = tryFetchValueFromEvaluatedSignalAccess(assignmentRhsOperand); fetchedValueForAssignmentRhsOperand.has_value()) {
        performAssignment(assignmentLhsOperand, assignmentOperation, fetchedValueForAssignmentRhsOperand);
        updateReferenceCountOf(assignmentRhsOperand.accessedSignalIdent, parser::SymbolTable::ReferenceCountUpdate::Decrement);
        return;
    }
    auto                                         transformedLhsOperandSignalAccess       = transformEvaluatedSignalAccess(assignmentLhsOperand, nullptr, nullptr);
    const std::shared_ptr<syrec::VariableAccess> sharedTransformedLhsOperandSignalAccess = std::move(transformedLhsOperandSignalAccess);
    invalidateStoredValueFor(sharedTransformedLhsOperandSignalAccess);
}

void optimizations::Optimizer::performSwap(const EvaluatedSignalAccess& swapOperationLhsOperand, const EvaluatedSignalAccess& swapOperationRhsOperand) {
    std::optional<bool> areAnyComponentsOfSignalAccessOfSwapLhsOperandOutOfRange;
    std::optional<bool> areAnyComponentsOfSignalAccessOfSwapRhsOperandOutOfRange;
    auto                didAllDefinedIndicesOfLhsSwapOperandEvaluateToConstants = false;
    auto                didAllDefinedIndicesOfRhsSwapOperandEvaluateToConstants = false;

    auto transformedSignalAccessForLhsOperand = transformEvaluatedSignalAccess(swapOperationLhsOperand, &areAnyComponentsOfSignalAccessOfSwapLhsOperandOutOfRange, &didAllDefinedIndicesOfLhsSwapOperandEvaluateToConstants);
    auto transformedSignalAccessForRhsOperand = transformEvaluatedSignalAccess(swapOperationRhsOperand, &areAnyComponentsOfSignalAccessOfSwapRhsOperandOutOfRange, &didAllDefinedIndicesOfRhsSwapOperandEvaluateToConstants);

    const std::shared_ptr<syrec::VariableAccess> sharedTransformedSwapLhsOperand = std::move(transformedSignalAccessForLhsOperand);
    const std::shared_ptr<syrec::VariableAccess> sharedTransformedSwapRhsOperand = std::move(transformedSignalAccessForRhsOperand);

    if (areAnyComponentsOfSignalAccessOfSwapLhsOperandOutOfRange.has_value() && !*areAnyComponentsOfSignalAccessOfSwapLhsOperandOutOfRange && didAllDefinedIndicesOfLhsSwapOperandEvaluateToConstants
        && areAnyComponentsOfSignalAccessOfSwapRhsOperandOutOfRange.has_value() && !*areAnyComponentsOfSignalAccessOfSwapRhsOperandOutOfRange && didAllDefinedIndicesOfRhsSwapOperandEvaluateToConstants) {
        performSwapAndCreateBackupOfOperands(sharedTransformedSwapLhsOperand, sharedTransformedSwapRhsOperand);
        return;
    }

    if (sharedTransformedSwapLhsOperand) {
        invalidateStoredValueFor(sharedTransformedSwapLhsOperand);
    }
    if (sharedTransformedSwapRhsOperand) {
        invalidateStoredValueFor(sharedTransformedSwapRhsOperand);
    }
}

void optimizations::Optimizer::updateStoredValueOf(const syrec::VariableAccess::ptr& assignedToSignal, unsigned int newValueOfAssignedToSignal) {
    if (const auto& activeSymbolTableScope = getActiveSymbolTableScope(); activeSymbolTableScope.has_value()) {
        createBackupOfAssignedToSignal(*assignedToSignal);
        activeSymbolTableScope->get()->updateStoredValueFor(assignedToSignal, newValueOfAssignedToSignal);   
    }
}

void optimizations::Optimizer::invalidateStoredValueFor(const syrec::VariableAccess::ptr& assignedToSignal) {
    if (const auto& activeSymbolTableScope = getActiveSymbolTableScope(); activeSymbolTableScope.has_value()) {
        createBackupOfAssignedToSignal(*assignedToSignal);
        activeSymbolTableScope->get()->invalidateStoredValuesFor(assignedToSignal);
    }
}

void optimizations::Optimizer::performSwapAndCreateBackupOfOperands(const syrec::VariableAccess::ptr& swapLhsOperand, const syrec::VariableAccess::ptr& swapRhsOperand) {
    if (const auto& activeSymbolTableScope = getActiveSymbolTableScope(); activeSymbolTableScope.has_value()) {
        createBackupOfAssignedToSignal(*swapLhsOperand);
        createBackupOfAssignedToSignal(*swapRhsOperand);
        activeSymbolTableScope->get()->swap(swapLhsOperand, swapRhsOperand);
    }
}

std::optional<unsigned> optimizations::Optimizer::tryFetchValueFromEvaluatedSignalAccess(const EvaluatedSignalAccess& accessedSignalParts) const {
    if (!parserConfig.constantPropagationEnabled) {
        return std::nullopt;
    }

    if (const auto& activeSymbolTableScope = getActiveSymbolTableScope(); activeSymbolTableScope.has_value()) {
        std::optional<bool> areAnyComponentsOfAssignedToSignalAccessOutOfRange;
        bool                didAllDefinedIndicesEvaluateToConstants;
        if (auto transformedSignalAccess = transformEvaluatedSignalAccess(accessedSignalParts, &areAnyComponentsOfAssignedToSignalAccessOutOfRange, &didAllDefinedIndicesEvaluateToConstants); transformedSignalAccess != nullptr) {
            if (!areAnyComponentsOfAssignedToSignalAccessOutOfRange.value_or(false) && didAllDefinedIndicesEvaluateToConstants && !isValueLookupBlockedByDataFlowAnalysisRestriction(*transformedSignalAccess)) {
                const std::shared_ptr<syrec::VariableAccess> sharedTransformedSignalAccess = std::move(transformedSignalAccess);
                return !isValueLookupBlockedByDataFlowAnalysisRestriction(*sharedTransformedSignalAccess) ? activeSymbolTableScope->get()->tryFetchValueForLiteral(sharedTransformedSignalAccess) : std::nullopt;
            }
        }        
    }
    return std::nullopt;
}

bool optimizations::Optimizer::isValueLookupBlockedByDataFlowAnalysisRestriction(const syrec::VariableAccess& accessedSignalParts) const {
    return activeDataFlowValuePropagationRestrictions->isAccessBlockedFor(accessedSignalParts);
}

SignalAccessUtils::SignalAccessEquivalenceResult optimizations::Optimizer::areEvaluatedSignalAccessEqualAtCompileTime(const EvaluatedSignalAccess& thisAccessedSignalParts, const EvaluatedSignalAccess& thatAccessedSignalParts) const {
    if (thisAccessedSignalParts.accessedSignalIdent != thatAccessedSignalParts.accessedSignalIdent) {
        return SignalAccessUtils::SignalAccessEquivalenceResult(SignalAccessUtils::SignalAccessEquivalenceResult::NotEqual, true);
    }

    std::optional<bool> wasAnyIndexOfThisSignalAccessOutOfRange;
    bool                wereAllIndicesOfThisSignalAccessConstant;

    std::optional<bool> wasAnyIndexOfThatSignalAccessOutOfRange;
    bool                wereAllIndicesOfThatSignalAccessConstant;

    const auto& transformedThisSignalAccess = transformEvaluatedSignalAccess(thisAccessedSignalParts, &wasAnyIndexOfThisSignalAccessOutOfRange, &wereAllIndicesOfThisSignalAccessConstant);
    const auto& transformedThatSignalAccess = transformEvaluatedSignalAccess(thatAccessedSignalParts, &wasAnyIndexOfThatSignalAccessOutOfRange, &wereAllIndicesOfThatSignalAccessConstant);

    if (!wereAllIndicesOfThisSignalAccessConstant || !wereAllIndicesOfThatSignalAccessConstant) {
        return SignalAccessUtils::SignalAccessEquivalenceResult(SignalAccessUtils::SignalAccessEquivalenceResult::Maybe, true);
    }

    if (const auto& activeSymbolTableScope = getActiveSymbolTableScope(); activeSymbolTableScope.has_value()) {
        return SignalAccessUtils::areSignalAccessesEqual(*transformedThisSignalAccess, *transformedThatSignalAccess, SignalAccessUtils::SignalAccessComponentEquivalenceCriteria::DimensionAccess::Equal, SignalAccessUtils::SignalAccessComponentEquivalenceCriteria::BitRange::Equal, **activeSymbolTableScope);    
    }
    return SignalAccessUtils::SignalAccessEquivalenceResult(SignalAccessUtils::SignalAccessEquivalenceResult::Maybe, true);
}

bool optimizations::Optimizer::checkIfOperandsOfBinaryExpressionAreSignalAccessesAndEqual(const syrec::Expression& lhsOperand, const syrec::Expression& rhsOperand) const {
    const auto& lhsExprAsVariableExpr = dynamic_cast<const syrec::VariableExpression*>(&lhsOperand);
    const auto& rhsExprAsVariableExpr = dynamic_cast<const syrec::VariableExpression*>(&rhsOperand);

    if (!lhsExprAsVariableExpr || !rhsExprAsVariableExpr || lhsExprAsVariableExpr->var->var->name != rhsExprAsVariableExpr->var->var->name) {
        return false;
    }

    const auto& lhsOperandAsSignalAccess = *lhsExprAsVariableExpr->var;
    const auto& rhsOperandAsSignalAccess = *rhsExprAsVariableExpr->var;

    if (const auto& activeSymbolTableScope = getActiveSymbolTableScope(); activeSymbolTableScope.has_value()) {
        if (const auto& signalAccessEquivalenceResult = SignalAccessUtils::areSignalAccessesEqual(lhsOperandAsSignalAccess, rhsOperandAsSignalAccess, SignalAccessUtils::SignalAccessComponentEquivalenceCriteria::DimensionAccess::Equal, SignalAccessUtils::SignalAccessComponentEquivalenceCriteria::BitRange::Equal, **activeSymbolTableScope);
                signalAccessEquivalenceResult.equality == SignalAccessUtils::SignalAccessEquivalenceResult::Equal && signalAccessEquivalenceResult.isResultCertain) {
            return true;
        }
    }
    return false;
}

// TODO: Signal value is fetched before bit range is evaluated and applied
/*
 * Reference counts of signals used in expressions of dimensio access are not updated correctly
 */
optimizations::Optimizer::OptimizationResult<syrec::VariableAccess> optimizations::Optimizer::handleSignalAccess(const syrec::VariableAccess& signalAccess, bool performConstantPropagationForAccessedSignal, std::optional<unsigned int>* fetchedValue) const {
    auto evaluatedSignalAccess = tryEvaluateDefinedSignalAccess(signalAccess);
    if (!evaluatedSignalAccess.has_value()) {
        updateReferenceCountOf(signalAccess.var->name, parser::SymbolTable::ReferenceCountUpdate::Increment);
        return OptimizationResult<syrec::VariableAccess>::asUnchangedOriginal();
    }
    updateReferenceCountOf(evaluatedSignalAccess->accessedSignalIdent, parser::SymbolTable::ReferenceCountUpdate::Increment);

    const auto& wasDefinedBitRangeAccessOk      = evaluatedSignalAccess->accessedBitRange.has_value() ? evaluatedSignalAccess->accessedBitRange->rangeStartEvaluationResult.validityStatus != IndexValidityStatus::OutOfRange : true;
    const auto& wasDefinedBitRangeStartAccessOk = evaluatedSignalAccess->accessedBitRange.has_value() ? evaluatedSignalAccess->accessedBitRange->rangeEndEvaluationResult.validityStatus != IndexValidityStatus::OutOfRange : true;
    const auto& wasDefinedDimensionAccessOk     = !evaluatedSignalAccess->accessedValuePerDimension.wasTrimmed && std::none_of(
                                                                                                                          evaluatedSignalAccess->accessedValuePerDimension.valuePerDimension.cbegin(),
                                                                                                                          evaluatedSignalAccess->accessedValuePerDimension.valuePerDimension.cend(),
                                                                                                                          [](const SignalAccessIndexEvaluationResult<syrec::Expression>& valueOfDimensionEvaluationResult) {
                                                                                                                          return valueOfDimensionEvaluationResult.validityStatus == IndexValidityStatus::OutOfRange;
                                                                                                                      });

    if (!(wasDefinedBitRangeAccessOk && wasDefinedBitRangeStartAccessOk && wasDefinedDimensionAccessOk)) {
        return OptimizationResult<syrec::VariableAccess>::asUnchangedOriginal();
    }

    auto signalAccessUsedForValueLookup = std::make_unique<syrec::VariableAccess>(signalAccess);
    bool                       wasAnyOriginalIndexModified    = false;
    if (evaluatedSignalAccess->wereAnyIndicesSimplified) {
        if (evaluatedSignalAccess->accessedBitRange.has_value()) {
            auto definedBitRangeStartEvaluationResult = std::move(evaluatedSignalAccess->accessedBitRange->rangeStartEvaluationResult);
            auto definedBitRangeEndEvaluationResult   = std::move(evaluatedSignalAccess->accessedBitRange->rangeEndEvaluationResult);

            auto simplifiedBitRangeStart = tryConvertSimplifiedIndexValueToUsableSignalAccessIndex(definedBitRangeStartEvaluationResult);
            auto simplifiedBitRangeEnd   = tryConvertSimplifiedIndexValueToUsableSignalAccessIndex(definedBitRangeEndEvaluationResult);
            
            if (signalAccess.range->first == signalAccess.range->second) {
                syrec::Number::ptr bitRangeStartAndEndContainer = simplifiedBitRangeStart.has_value() ? std::move(*simplifiedBitRangeStart) : signalAccess.range->first;
                signalAccessUsedForValueLookup->range = std::make_pair(bitRangeStartAndEndContainer, bitRangeStartAndEndContainer);
            }
            else {
                signalAccessUsedForValueLookup->range = std::make_pair(
                        simplifiedBitRangeStart.has_value() ? std::move(*simplifiedBitRangeStart) : signalAccess.range->first,
                        simplifiedBitRangeEnd.has_value() ? std::move(*simplifiedBitRangeEnd) : signalAccess.range->second);                
            }
            wasAnyOriginalIndexModified = simplifiedBitRangeStart.has_value() || simplifiedBitRangeEnd.has_value();
        }

        std::size_t dimensionIdx = 0;
        for (auto& evaluatedValueOfDimension: evaluatedSignalAccess->accessedValuePerDimension.valuePerDimension) {
            if (auto simplifiedValueOfDimension = tryConvertSimplifiedIndexValueToUsableSignalAccessIndex(evaluatedValueOfDimension); simplifiedValueOfDimension.has_value()) {
                signalAccessUsedForValueLookup->indexes[dimensionIdx] = std::move(*simplifiedValueOfDimension);
                wasAnyOriginalIndexModified                           = true;
            }
            ++dimensionIdx;
        }
    } else if (!performConstantPropagationForAccessedSignal) {
        return OptimizationResult<syrec::VariableAccess>::asUnchangedOriginal();
    }

    if (performConstantPropagationForAccessedSignal) {
        if (const auto& fetchedValueForSimplifiedSignalAccess = tryFetchValueForAccessedSignal(*signalAccessUsedForValueLookup); fetchedValueForSimplifiedSignalAccess.has_value()) {
            if (fetchedValue) {
                *fetchedValue = *fetchedValueForSimplifiedSignalAccess;
                return OptimizationResult<syrec::VariableAccess>::fromOptimizedContainer(std::move(signalAccessUsedForValueLookup));
            }
        }
    }
    return !wasAnyOriginalIndexModified
        ? OptimizationResult<syrec::VariableAccess>::asUnchangedOriginal()
        : OptimizationResult<syrec::VariableAccess>::fromOptimizedContainer(std::move(signalAccessUsedForValueLookup));     
}

void optimizations::Optimizer::filterAssignmentsThatDoNotChangeAssignedToSignal(syrec::Statement::vec& assignmentsToCheck) {
    assignmentsToCheck.erase(
        std::remove_if(assignmentsToCheck.begin(), assignmentsToCheck.end(), [](const syrec::Statement::ptr& stmt) {
            if (const auto& stmtAsAssignmentStmt = std::dynamic_pointer_cast<syrec::AssignStatement>(stmt); stmtAsAssignmentStmt != nullptr) {
                if (const auto& mappedToAssignmentOperation = syrec_operation::tryMapAssignmentOperationFlagToEnum(stmtAsAssignmentStmt->op); mappedToAssignmentOperation.has_value()) {
                    if (const auto& assignmentStmtRhsEvaluated = std::dynamic_pointer_cast<syrec::NumericExpression>(stmtAsAssignmentStmt->rhs); assignmentStmtRhsEvaluated != nullptr && assignmentStmtRhsEvaluated->value->isConstant()) {
                        return syrec_operation::isOperandUseAsRhsInOperationIdentityElement(*mappedToAssignmentOperation, assignmentStmtRhsEvaluated->value->evaluate({}));
                    }
                }
                return false;
            }
            return true;
    }),
    assignmentsToCheck.end());
}

std::optional<std::unique_ptr<syrec::Number>> optimizations::Optimizer::tryConvertSimplifiedIndexValueToUsableSignalAccessIndex(SignalAccessIndexEvaluationResult<syrec::Number>& index) {
    if (const auto& fetchedConstantValueForIndex = tryFetchConstantValueOfIndex(index); fetchedConstantValueForIndex.has_value()) {
        return std::make_unique<syrec::Number>(*fetchedConstantValueForIndex);
    }
    return tryFetchAndTakeOwnershipOfNonConstantValueOfIndex(index);
}

std::optional<std::unique_ptr<syrec::Expression>> optimizations::Optimizer::tryConvertSimplifiedIndexValueToUsableSignalAccessIndex(SignalAccessIndexEvaluationResult<syrec::Expression>& index) {
    if (const auto& fetchedConstantValueForIndex = tryFetchConstantValueOfIndex(index); fetchedConstantValueForIndex.has_value()) {
        return std::make_unique<syrec::NumericExpression>(std::make_unique<syrec::Number>(*fetchedConstantValueForIndex), BitHelpers::getRequiredBitsToStoreValue(*fetchedConstantValueForIndex));
    }
    return tryFetchAndTakeOwnershipOfNonConstantValueOfIndex(index);
}

void optimizations::Optimizer::openNewIfStatementBranchSymbolTableBackupScope(std::size_t internalNestingLevel) {
    if (const auto& createdSymbolTableBackupScope = std::make_shared<parser::SymbolTableBackupHelper>(); createdSymbolTableBackupScope) {
        symbolTableBackupScopeContainers.emplace_back(IfStatementBranchSymbolTableBackupScope({internalNestingLevel, createdSymbolTableBackupScope}));   
    }
}

void optimizations::Optimizer::updateBackupOfValuesChangedInScopeAndOptionallyResetMadeChanges(bool resetLocalChanges) const {
    const auto& peekedActiveSymbolTableBackupScope = peekCurrentSymbolTableBackupScope();
    if (!peekedActiveSymbolTableBackupScope.has_value() || peekedActiveSymbolTableBackupScope->get().backupScope->getIdentsOfChangedSignals().empty()) {
        return;
    }

    if (const auto& activeSymbolTableScope = getActiveSymbolTableScope(); activeSymbolTableScope.has_value()) {
        for (auto& [signalIdent, valueAndRestrictionBackup]: peekedActiveSymbolTableBackupScope->get().backupScope->getBackedUpEntries()) {
            if (const auto& backupOfCurrentSignalValue = activeSymbolTableScope->get()->createBackupOfValueOfSignal(signalIdent); backupOfCurrentSignalValue.has_value()) {
                if (resetLocalChanges) {
                    activeSymbolTableScope->get()->restoreValuesFromBackup(signalIdent, valueAndRestrictionBackup);   
                }
                valueAndRestrictionBackup->copyRestrictionsAndUnrestrictedValuesFrom(
                        {}, std::nullopt,
                        {}, std::nullopt,
                        **backupOfCurrentSignalValue);    
            }
        }
    }
}

void optimizations::Optimizer::transferBackupOfValuesChangedInCurrentScopeToParentScope() {
    const auto& peekedActiveSymbolTableBackupScope = peekCurrentSymbolTableBackupScope();
    if (!peekedActiveSymbolTableBackupScope.has_value() || peekedActiveSymbolTableBackupScope->get().backupScope->getIdentsOfChangedSignals().empty()) {
        return;
    }

    const auto& peekedAssumedParentSymbolTableBackupScope = peekPredecessorOfCurrentSymbolTableBackupScope();
    if (!peekedAssumedParentSymbolTableBackupScope.has_value()) {
        return;
    }

    for (auto& [signalIdent, valueAndRestrictionBackup]: peekedActiveSymbolTableBackupScope->get().backupScope->getBackedUpEntries()) {
        if (!peekedAssumedParentSymbolTableBackupScope->get().backupScope->hasEntryFor(signalIdent)) {
            peekedAssumedParentSymbolTableBackupScope->get().backupScope->storeBackupOf(signalIdent, valueAndRestrictionBackup);   
        }
    }
}


std::optional<std::reference_wrapper<const optimizations::Optimizer::IfStatementBranchSymbolTableBackupScope>> optimizations::Optimizer::peekCurrentSymbolTableBackupScope() const {
    if (symbolTableBackupScopeContainers.empty()) {
        return std::nullopt;
    }
    return symbolTableBackupScopeContainers.back();
}

std::optional<const std::reference_wrapper<optimizations::Optimizer::IfStatementBranchSymbolTableBackupScope>> optimizations::Optimizer::peekModifiableCurrentSymbolTableBackupScope() {
    if (symbolTableBackupScopeContainers.empty()) {
        return std::nullopt;
    }
    return symbolTableBackupScopeContainers.back();
}

std::optional<const std::reference_wrapper<optimizations::Optimizer::IfStatementBranchSymbolTableBackupScope>> optimizations::Optimizer::peekPredecessorOfCurrentSymbolTableBackupScope() {
    if (symbolTableBackupScopeContainers.size() < 2) {
        return std::nullopt;
    }

    const auto& nestingLevelOfCurrentSymbolTableBackupScope = symbolTableBackupScopeContainers.back().ifStatementNestingLevel;
    const auto& foundPredecessorOfCurrentSymbolTableScope = std::find_if(
            std::next(symbolTableBackupScopeContainers.rbegin(), 1),
            symbolTableBackupScopeContainers.rend(),
            [&nestingLevelOfCurrentSymbolTableBackupScope](const optimizations::Optimizer::IfStatementBranchSymbolTableBackupScope& ifStatementBranchSymbolTableBackupScope) {
                return ifStatementBranchSymbolTableBackupScope.ifStatementNestingLevel < nestingLevelOfCurrentSymbolTableBackupScope;
            });
    if (foundPredecessorOfCurrentSymbolTableScope != symbolTableBackupScopeContainers.rend()) {
        return *foundPredecessorOfCurrentSymbolTableScope;
    }
    return std::nullopt;

    //return symbolTableBackupScopeContainers.at(symbolTableBackupScopeContainers.size() - 2);
}


void optimizations::Optimizer::mergeAndMakeLocalChangesGlobal(const parser::SymbolTableBackupHelper& backupOfSignalsChangedInFirstBranch, const parser::SymbolTableBackupHelper& backupOfSignalsChangedInSecondBranch) const {
    const auto& activeSymbolTableScope = getActiveSymbolTableScope();
    if (!activeSymbolTableScope.has_value()) {
        return;
    }

    const auto& modifiedSignalsInFirstScope  = backupOfSignalsChangedInFirstBranch.getIdentsOfChangedSignals();
    const auto& modifiedSignalsInSecondScope = backupOfSignalsChangedInSecondBranch.getIdentsOfChangedSignals();

    std::unordered_set<std::string> setOfSignalsRequiringMergeOfChanges;
    std::unordered_set<std::string> setOfSignalsChangedOnlyInFirstScope;
    std::unordered_set<std::string> setOfSignalsChangedOnlyInSecondScope;
    for (const auto& modifiedSignalInFirstScope: modifiedSignalsInFirstScope) {
        if (modifiedSignalsInSecondScope.count(modifiedSignalInFirstScope)) {
            setOfSignalsRequiringMergeOfChanges.emplace(modifiedSignalInFirstScope);
        }
        else {
            setOfSignalsChangedOnlyInFirstScope.emplace(modifiedSignalInFirstScope);
        }
    }

    if (modifiedSignalsInFirstScope.size() != modifiedSignalsInSecondScope.size()) {
        for (const auto& modifiedSignalInSecondScope: modifiedSignalsInSecondScope) {
            if (modifiedSignalsInFirstScope.count(modifiedSignalInSecondScope)) {
                setOfSignalsRequiringMergeOfChanges.emplace(modifiedSignalInSecondScope);
            }
            else {
                setOfSignalsChangedOnlyInSecondScope.emplace(modifiedSignalInSecondScope);
            }
        }
    }
    
    for (const auto& signalRequiringMergeOfChanges: setOfSignalsRequiringMergeOfChanges) {
        const auto& currentValueOfSignalAtEndOfSecondScope     = *backupOfSignalsChangedInSecondBranch.getBackedUpEntryFor(signalRequiringMergeOfChanges);
        const auto& currentValueOfSignalAtEndOfFirstScope      = *backupOfSignalsChangedInFirstBranch.getBackedUpEntryFor(signalRequiringMergeOfChanges);
        currentValueOfSignalAtEndOfSecondScope->copyRestrictionsAndInvalidateChangedValuesFrom(currentValueOfSignalAtEndOfFirstScope);
        activeSymbolTableScope->get()->restoreValuesFromBackup(signalRequiringMergeOfChanges, currentValueOfSignalAtEndOfSecondScope);
    }
    
    for (const auto& signalChangedOnlyInFirstScope : setOfSignalsChangedOnlyInFirstScope) {
        /*const auto& initialValueOfChangedSignalPriorToAnyScope = *backupOfSignalsChangedInFirstBranch.getBackedUpEntryFor(signalChangedOnlyInFirstScope);
        const auto& currentValueOfSignalAtEndOfFirstScope = *backupOfSignalsChangedInFirstBranch.getBackedUpEntryFor(signalChangedOnlyInFirstScope);
        currentValueOfSignalAtEndOfFirstScope->copyRestrictionsAndInvalidateChangedValuesFrom(initialValueOfChangedSignalPriorToAnyScope);
        activeSymbolTableScope->get()->restoreValuesFromBackup(signalChangedOnlyInFirstScope, currentValueOfSignalAtEndOfFirstScope);*/

        const auto& valueOfSignalInActiveSymbolTableScope  = activeSymbolTableScope->get()->createBackupOfValueOfSignal(signalChangedOnlyInFirstScope);
        const auto& currentValueOfSignalAtEndOfSecondScope = *backupOfSignalsChangedInFirstBranch.getBackedUpEntryFor(signalChangedOnlyInFirstScope);
        valueOfSignalInActiveSymbolTableScope->get()->copyRestrictionsAndInvalidateChangedValuesFrom(currentValueOfSignalAtEndOfSecondScope);
        activeSymbolTableScope->get()->restoreValuesFromBackup(signalChangedOnlyInFirstScope, *valueOfSignalInActiveSymbolTableScope);
    }

    for (const auto& signalChangedOnlyInSecondScope : setOfSignalsChangedOnlyInSecondScope) {
       /* const auto& initialValueOfChangedSignalPriorToAnyScope = *backupOfSignalsChangedInSecondBranch.getBackedUpEntryFor(signalChangedOnlyInSecondScope);
        const auto& currentValueOfSignalAtEndOfSecondScope = *backupOfSignalsChangedInSecondBranch.getBackedUpEntryFor(signalChangedOnlyInSecondScope);
        currentValueOfSignalAtEndOfSecondScope->copyRestrictionsAndInvalidateChangedValuesFrom(initialValueOfChangedSignalPriorToAnyScope);
        activeSymbolTableScope->get()->restoreValuesFromBackup(signalChangedOnlyInSecondScope, currentValueOfSignalAtEndOfSecondScope);*/
        
        const auto& valueOfSignalInActiveSymbolTableScope = activeSymbolTableScope->get()->createBackupOfValueOfSignal(signalChangedOnlyInSecondScope);
        const auto& currentValueOfSignalAtEndOfSecondScope     = *backupOfSignalsChangedInSecondBranch.getBackedUpEntryFor(signalChangedOnlyInSecondScope);
        valueOfSignalInActiveSymbolTableScope->get()->copyRestrictionsAndInvalidateChangedValuesFrom(currentValueOfSignalAtEndOfSecondScope);
        activeSymbolTableScope->get()->restoreValuesFromBackup(signalChangedOnlyInSecondScope, *valueOfSignalInActiveSymbolTableScope);
    }
}

void optimizations::Optimizer::makeLocalChangesGlobal(const parser::SymbolTableBackupHelper& backupOfCurrentValuesOfSignalsAtEndOfBranch, const parser::SymbolTableBackupHelper& backupCurrentValuesOfSignalsInToBeOmittedBranch) const {
    const auto& activeSymbolTableScope = getActiveSymbolTableScope();
    if (!activeSymbolTableScope.has_value()) {
        return;
    }
    
    const auto& modifiedSignalsInRemainingBranch  = backupOfCurrentValuesOfSignalsAtEndOfBranch.getIdentsOfChangedSignals();
    const auto& modifiedSignalsInOmittedBranch = backupCurrentValuesOfSignalsInToBeOmittedBranch.getIdentsOfChangedSignals();

    for (const auto& modifiedSignalInOmittedBranch : modifiedSignalsInOmittedBranch) {
        if (!modifiedSignalsInRemainingBranch.count(modifiedSignalInOmittedBranch)) {
            activeSymbolTableScope->get()->restoreValuesFromBackup(modifiedSignalInOmittedBranch, *backupCurrentValuesOfSignalsInToBeOmittedBranch.getBackedUpEntryFor(modifiedSignalInOmittedBranch));
        }
    }

    for (const auto& modifiedSignalInRemainingBranch : modifiedSignalsInRemainingBranch) {
        activeSymbolTableScope->get()->restoreValuesFromBackup(modifiedSignalInRemainingBranch, *backupOfCurrentValuesOfSignalsAtEndOfBranch.getBackedUpEntryFor(modifiedSignalInRemainingBranch));
    }
}


void optimizations::Optimizer::destroySymbolTableBackupScope() {
    if (symbolTableBackupScopeContainers.empty()) {
        return;
    }
    symbolTableBackupScopeContainers.pop_back();
}

void optimizations::Optimizer::createBackupOfAssignedToSignal(const syrec::VariableAccess& assignedToVariableAccess) {
    const std::string_view& assignedToSignalIdent = assignedToVariableAccess.var->name;
    const auto&             peekedActiveSymbolTableBackupScope = peekModifiableCurrentSymbolTableBackupScope();
    if (!peekedActiveSymbolTableBackupScope.has_value() || peekedActiveSymbolTableBackupScope->get().backupScope->hasEntryFor(assignedToSignalIdent)) {
        return;
    }

    if (const auto& activeSymbolTableScope = getActiveSymbolTableScope(); activeSymbolTableScope.has_value()) {
        if (const auto& backupOfAssignedToSignal = activeSymbolTableScope->get()->createBackupOfValueOfSignal(assignedToSignalIdent); backupOfAssignedToSignal.has_value()) {
            peekedActiveSymbolTableBackupScope->get().backupScope->storeBackupOf(std::string(assignedToSignalIdent), *backupOfAssignedToSignal);
        }        
    }
}

std::optional<std::unique_ptr<optimizations::Optimizer::IfStatementBranchSymbolTableBackupScope>> optimizations::Optimizer::popCurrentSymbolTableBackupScope() {
    if (symbolTableBackupScopeContainers.empty()) {
        return std::nullopt;
    }

    auto currentSymbolTableBackupScope = std::make_unique<optimizations::Optimizer::IfStatementBranchSymbolTableBackupScope>(symbolTableBackupScopeContainers.back());
    symbolTableBackupScopeContainers.pop_back();
    decrementInternalIfStatementNestingLevelCounter();
    return std::make_optional(std::move(currentSymbolTableBackupScope));
}

std::vector<std::reference_wrapper<const syrec::Statement>> optimizations::Optimizer::transformCollectionOfSharedPointersToReferences(const syrec::Statement::vec& statements) {
    std::vector<std::reference_wrapper<const syrec::Statement>> resultContainer;
    resultContainer.reserve(statements.size());

    for (const auto& stmt : statements) {
        resultContainer.emplace_back(*stmt.get());
    }
    return resultContainer;
}

void optimizations::Optimizer::decrementReferenceCountsOfLoopHeaderComponents(const syrec::Number& iterationRangeStart, const std::optional<std::reference_wrapper<const syrec::Number>>& iterationRangeEnd, const syrec::Number& stepSize) const {
    updateReferenceCountsOfSignalIdentsUsedIn(iterationRangeStart, parser::SymbolTable::ReferenceCountUpdate::Decrement);
    if (iterationRangeEnd.has_value()) {
        updateReferenceCountsOfSignalIdentsUsedIn(*iterationRangeEnd, parser::SymbolTable::ReferenceCountUpdate::Decrement);
    }
    updateReferenceCountsOfSignalIdentsUsedIn(stepSize, parser::SymbolTable::ReferenceCountUpdate::Decrement);
}

syrec::Statement::vec optimizations::Optimizer::createStatementListFrom(const syrec::Statement::vec& originalStmtList, std::vector<std::unique_ptr<syrec::Statement>> simplifiedStmtList) {
    if (originalStmtList.empty() && simplifiedStmtList.empty()) {
        return {};
    }

    if (!originalStmtList.empty()) {
        return originalStmtList;
    }

    syrec::Statement::vec stmtsContainer;
    stmtsContainer.reserve(simplifiedStmtList.size());

    for (auto&& simplifiedStmt : simplifiedStmtList) {
        stmtsContainer.emplace_back(std::move(simplifiedStmt));
    }
    return stmtsContainer;
}

void optimizations::Optimizer::updateValueOfLoopVariable(const std::string& loopVariableIdent, const std::optional<unsigned>& value) {
    if (loopVariableIdent.empty()) {
        return;
    }

    if (const auto& activeSymbolTableScope = getActiveSymbolTableScope(); activeSymbolTableScope.has_value()) {
        if (!value.has_value()) {
            activeSymbolTableScope->get()->invalidateStoredValueForLoopVariable(loopVariableIdent);
            if (evaluableLoopVariableLookup.count(loopVariableIdent)) {
                evaluableLoopVariableLookup.erase(loopVariableIdent);
            }
        } else {
            activeSymbolTableScope->get()->updateStoredValueForLoopVariable(loopVariableIdent, *value);
            if (evaluableLoopVariableLookup.count(loopVariableIdent)) {
                evaluableLoopVariableLookup[loopVariableIdent] = *value;
            } else {
                evaluableLoopVariableLookup.insert_or_assign(loopVariableIdent, *value);
            }
        }
    }
}

void optimizations::Optimizer::addSymbolTableEntryForLoopVariable(const std::string_view& loopVariableIdent, const std::optional<unsigned>& expectedMaximumValue) const {
    if (const auto& activeSymbolTableScope = getActiveSymbolTableScope(); activeSymbolTableScope.has_value()) {
        if (!activeSymbolTableScope->get()->contains(loopVariableIdent)) {
            activeSymbolTableScope->get()->addEntry(syrec::Number({std::string(loopVariableIdent)}), expectedMaximumValue.has_value() ? BitHelpers::getRequiredBitsToStoreValue(*expectedMaximumValue) : UINT_MAX, expectedMaximumValue);
        }
    }
}

void optimizations::Optimizer::removeLoopVariableFromSymbolTable(const std::string& loopVariableIdent) {
    if (loopVariableIdent.empty()) {
        return;
    }

    if (const auto& activeSymbolTableScope = getActiveSymbolTableScope(); activeSymbolTableScope.has_value()) {
        activeSymbolTableScope->get()->removeVariable(std::string(loopVariableIdent));   
    }

    if (evaluableLoopVariableLookup.count(loopVariableIdent)) {
        evaluableLoopVariableLookup.erase(loopVariableIdent);
    }
}

std::optional<unsigned> optimizations::Optimizer::tryDetermineMaximumValueOfLoopVariable(const std::string_view loopVariableIdent, const std::optional<unsigned>& initialValueOfLoopVariable, const syrec::Number& maximumValueContainerToBeEvaluated) {
    std::optional<unsigned int> fetchedMaximumValue;
    if (const auto& activeSymbolTableScope = getActiveSymbolTableScope(); activeSymbolTableScope.has_value()) {
        addSymbolTableEntryForLoopVariable(loopVariableIdent, initialValueOfLoopVariable);
        internalReferenceCountModificationEnabledFlag.emplace(false);
        if (auto evaluationResultOfContainerDefiningMaximumValue = handleNumber(maximumValueContainerToBeEvaluated); evaluationResultOfContainerDefiningMaximumValue.getStatusOfResult() == OptimizationResultFlag::WasOptimized) {
            if (const auto& ownershipOfEvaluationResult = evaluationResultOfContainerDefiningMaximumValue.tryTakeOwnershipOfOptimizationResult(); ownershipOfEvaluationResult.has_value() && ownershipOfEvaluationResult->size() == 1) {
                fetchedMaximumValue = tryEvaluateNumberAsConstant(*ownershipOfEvaluationResult->front(), activeSymbolTableScope->get(), true, evaluableLoopVariableLookup, nullptr);
            }
        }
        else {
            fetchedMaximumValue = tryEvaluateNumberAsConstant(maximumValueContainerToBeEvaluated, activeSymbolTableScope->get(), true, evaluableLoopVariableLookup, nullptr);
        }
        internalReferenceCountModificationEnabledFlag.reset();
        removeLoopVariableFromSymbolTable(std::string(loopVariableIdent));
    }
    else {
        fetchedMaximumValue = tryEvaluateNumberAsConstant(maximumValueContainerToBeEvaluated, activeSymbolTableScope->get(), true, evaluableLoopVariableLookup, nullptr);
    }
    return fetchedMaximumValue;
}


bool optimizations::Optimizer::isAnyModifiableParameterOrLocalModifiedInModuleBody(const syrec::Module& module, bool areCallerArgumentsBasedOnOptimizedSignature) const {
    if (module.statements.empty()) {
        return false;
    }

    std::unordered_set<std::string> lookupOfModifiableParametersAndLocals;
    for (const auto& moduleParameter : module.parameters) {
        if (!isVariableReadOnly(*moduleParameter) && !lookupOfModifiableParametersAndLocals.count(moduleParameter->name)) {
            lookupOfModifiableParametersAndLocals.insert(moduleParameter->name);
        }
    }

    for (const auto& moduleLocalVariable: module.variables) {
        if (!isVariableReadOnly(*moduleLocalVariable) && !lookupOfModifiableParametersAndLocals.count(moduleLocalVariable->name)) {
            lookupOfModifiableParametersAndLocals.insert(moduleLocalVariable->name);
        }
    }

    if (lookupOfModifiableParametersAndLocals.empty()) {
        return false;
    }

    return std::any_of(module.statements.cbegin(), module.statements.cend(), [&](const syrec::Statement::ptr& stmt) {
        return isAnyModifiableParameterOrLocalModifiedInStatement(*stmt, lookupOfModifiableParametersAndLocals, areCallerArgumentsBasedOnOptimizedSignature);
    });
}

// TODO: Swap with lhs and rhs operand being equal does not count as a signal modification
bool optimizations::Optimizer::isAnyModifiableParameterOrLocalModifiedInStatement(const syrec::Statement& statement, const std::unordered_set<std::string>& parameterAndLocalLookup, bool areCallerArgumentsBasedOnOptimizedSignature) const {
    if (const auto& stmtAsLoopStatement = dynamic_cast<const syrec::ForStatement*>(&statement); stmtAsLoopStatement != nullptr) {
        const syrec::Number::loop_variable_mapping evaluableLoopVariablesIfConstantPropagationIsDisabled;
        const auto&                                iterationRangeStartEvaluated = tryEvaluateNumberAsConstant(*stmtAsLoopStatement->range.first, nullptr, false, evaluableLoopVariablesIfConstantPropagationIsDisabled, nullptr);
        const auto&                                iterationRangEndEvaluated    = tryEvaluateNumberAsConstant(*stmtAsLoopStatement->range.second, nullptr, false, evaluableLoopVariablesIfConstantPropagationIsDisabled, nullptr);
        const auto&                                iterationRangeStepSize       = tryEvaluateNumberAsConstant(*stmtAsLoopStatement->step, nullptr, false, evaluableLoopVariablesIfConstantPropagationIsDisabled, nullptr);
        if (iterationRangeStartEvaluated.has_value() && iterationRangEndEvaluated.has_value() && iterationRangeStepSize.has_value()) {
            if (const auto& numberOfIterations = utils::determineNumberOfLoopIterations(*iterationRangeStartEvaluated, *iterationRangEndEvaluated, *iterationRangeStepSize); numberOfIterations.has_value()) {
                return !*numberOfIterations;
            }
        }
        return isAnyModifiableParameterOrLocalModifiedInStatements(transformCollectionOfSharedPointersToReferences(stmtAsLoopStatement->statements), parameterAndLocalLookup, areCallerArgumentsBasedOnOptimizedSignature);
    }
    if (const auto& stmtAsIfStmt = dynamic_cast<const syrec::IfStatement*>(&statement); stmtAsIfStmt != nullptr) {
        return isAnyModifiableParameterOrLocalModifiedInStatements(transformCollectionOfSharedPointersToReferences(stmtAsIfStmt->thenStatements), parameterAndLocalLookup, areCallerArgumentsBasedOnOptimizedSignature)
            || isAnyModifiableParameterOrLocalModifiedInStatements(transformCollectionOfSharedPointersToReferences(stmtAsIfStmt->elseStatements), parameterAndLocalLookup, areCallerArgumentsBasedOnOptimizedSignature);
    }
    if (const auto& stmtAsAssignmentStmt = dynamic_cast<const syrec::AssignStatement*>(&statement); stmtAsAssignmentStmt != nullptr) {
        if (const auto& assignmentRhsOperandAsConstant = tryEvaluateExpressionToConstant(*stmtAsAssignmentStmt->rhs, nullptr, false, {}, nullptr); assignmentRhsOperandAsConstant.has_value() && syrec_operation::tryMapAssignmentOperationFlagToEnum(stmtAsAssignmentStmt->op).has_value()) {
            if (syrec_operation::isOperandUseAsRhsInOperationIdentityElement(*syrec_operation::tryMapAssignmentOperationFlagToEnum(stmtAsAssignmentStmt->op), *assignmentRhsOperandAsConstant)) {
                return false;
            }
        }
        return parameterAndLocalLookup.count(stmtAsAssignmentStmt->lhs->var->name);
    }
    if (const auto& stmtAsUnaryStmt = dynamic_cast<const syrec::UnaryStatement*>(&statement); stmtAsUnaryStmt != nullptr) {
        return parameterAndLocalLookup.count(stmtAsUnaryStmt->var->var->name);
    }
    if (const auto& stmtAsSwapStmt = dynamic_cast<const syrec::SwapStatement*>(&statement); stmtAsSwapStmt != nullptr) {
        return parameterAndLocalLookup.count(stmtAsSwapStmt->lhs->var->name) || parameterAndLocalLookup.count(stmtAsSwapStmt->rhs->var->name);
    }
    if (const auto& stmtAsCallStmt = dynamic_cast<const syrec::CallStatement*>(&statement); stmtAsCallStmt != nullptr) {
        if (const auto& activeSymbolTableScope = getActiveSymbolTableScope(); activeSymbolTableScope.has_value()) {
            if (const auto& matchingSymbolTableEntryForModule = activeSymbolTableScope->get()->tryGetOptimizedSignatureForModuleCall(stmtAsCallStmt->target->name, stmtAsCallStmt->parameters, areCallerArgumentsBasedOnOptimizedSignature); matchingSymbolTableEntryForModule.has_value()) {
                std::unordered_set<std::size_t> indicesOfRemainingParameters;
                matchingSymbolTableEntryForModule->determineOptimizedCallSignature(&indicesOfRemainingParameters);
                return std::any_of(
                        indicesOfRemainingParameters.cbegin(),
                        indicesOfRemainingParameters.cend(),
                        [&parameterAndLocalLookup, &matchingSymbolTableEntryForModule, &stmtAsCallStmt](const std::size_t remainingParameterIndex) {
                            const auto& formalParameter      = matchingSymbolTableEntryForModule->declaredParameters.at(remainingParameterIndex);
                            const auto& actualParameterIdent = stmtAsCallStmt->parameters.at(remainingParameterIndex);
                            return !isVariableReadOnly(*formalParameter) && parameterAndLocalLookup.count(actualParameterIdent);
                        });
            }    
        }
    }
    return false;
}

bool optimizations::Optimizer::isAnyModifiableParameterOrLocalModifiedInStatements(const std::vector<std::reference_wrapper<const syrec::Statement>>& statements, const std::unordered_set<std::string>& parameterAndLocalLookup, bool areCallerArgumentsBasedOnOptimizedSignature) const {
    return std::any_of(statements.cbegin(), statements.cend(), [&](const syrec::Statement& statement) {
        return isAnyModifiableParameterOrLocalModifiedInStatement(statement, parameterAndLocalLookup, areCallerArgumentsBasedOnOptimizedSignature);
    });
}

inline bool optimizations::Optimizer::isVariableReadOnly(const syrec::Variable& variable) {
    return variable.type == syrec::Variable::Types::In;
}

std::optional<bool> optimizations::Optimizer::determineEquivalenceOfOperandsOfBinaryExpr(const syrec::BinaryExpression& binaryExpression) {
    if (const auto& definedBinaryOperation = syrec_operation::tryMapBinaryOperationFlagToEnum(binaryExpression.op); definedBinaryOperation.has_value()) {
        if (parser::areSyntacticallyEquivalent(binaryExpression.lhs, binaryExpression.rhs)) {
            return syrec_operation::determineBooleanResultIfOperandsOfBinaryExprMatchForOperation(*definedBinaryOperation);
        }
    }
    return std::nullopt;
}

bool optimizations::Optimizer::doesStatementDefineAssignmentThatChangesAssignedToSignal(const syrec::Statement& statement, bool areCallerArgumentsBasedOnOptimizedSignature) {
    if (const auto& stmtAsLoopStatement = dynamic_cast<const syrec::ForStatement*>(&statement); stmtAsLoopStatement != nullptr) {
        return std::any_of(stmtAsLoopStatement->statements.cbegin(), stmtAsLoopStatement->statements.cend(), [&](const syrec::Statement::ptr& loopBodyStatement) {
            return doesStatementDefineAssignmentThatChangesAssignedToSignal(*loopBodyStatement, areCallerArgumentsBasedOnOptimizedSignature);
        });
    }
    if (const auto& stmtAsIfStmt = dynamic_cast<const syrec::IfStatement*>(&statement); stmtAsIfStmt != nullptr) {
        return std::any_of(stmtAsIfStmt->thenStatements.cbegin(), stmtAsIfStmt->thenStatements.cend(), [&](const syrec::Statement::ptr& trueBranchStatement) {
            return doesStatementDefineAssignmentThatChangesAssignedToSignal(*trueBranchStatement, areCallerArgumentsBasedOnOptimizedSignature);
        })
        || std::any_of(stmtAsIfStmt->elseStatements.cbegin(), stmtAsIfStmt->elseStatements.cend(), [&](const syrec::Statement::ptr& falseBranchStatement) {
            return doesStatementDefineAssignmentThatChangesAssignedToSignal(*falseBranchStatement, areCallerArgumentsBasedOnOptimizedSignature);
        });
    }
    if (const auto& stmtAsAssignmentStmt = dynamic_cast<const syrec::AssignStatement*>(&statement); stmtAsAssignmentStmt != nullptr) {
        if (const auto& mappedToAssignmentOperationFromFlag = syrec_operation::tryMapAssignmentOperationFlagToEnum(stmtAsAssignmentStmt->op); mappedToAssignmentOperationFromFlag.has_value()) {
            if (const auto& rhsValueEvaluatedAsConstant = tryEvaluateExpressionToConstant(*stmtAsAssignmentStmt->rhs, nullptr, false, {}, nullptr); rhsValueEvaluatedAsConstant.has_value()) {
                return !syrec_operation::isOperandUseAsRhsInOperationIdentityElement(*mappedToAssignmentOperationFromFlag, *rhsValueEvaluatedAsConstant);
            }   
        }
        return true;
    }
    if (const auto& stmtAsUnaryStmt = dynamic_cast<const syrec::UnaryStatement*>(&statement); stmtAsUnaryStmt != nullptr) {
        return true;
    }
    if (const auto& stmtAsSwapStmt = dynamic_cast<const syrec::SwapStatement*>(&statement); stmtAsSwapStmt != nullptr) {
        const auto& lhsOperandEvaluated = tryEvaluateDefinedSignalAccess(*stmtAsSwapStmt->lhs);
        const auto& rhsOperandEvaluated = tryEvaluateDefinedSignalAccess(*stmtAsSwapStmt->rhs);
        if (lhsOperandEvaluated.has_value() && rhsOperandEvaluated.has_value()) {
            const auto& equivalenceCheckResultOfSwapOperands = areEvaluatedSignalAccessEqualAtCompileTime(*lhsOperandEvaluated, *rhsOperandEvaluated);
            return !(equivalenceCheckResultOfSwapOperands.isResultCertain && equivalenceCheckResultOfSwapOperands.equality == SignalAccessUtils::SignalAccessEquivalenceResult::Equal);
        }
        return true;
    }
    if (const auto& stmtAsCallStmt = dynamic_cast<const syrec::CallStatement*>(&statement); stmtAsCallStmt != nullptr) {
        if (const auto& activeSymbolTableScope = getActiveSymbolTableScope(); activeSymbolTableScope.has_value()) {
            if (const auto& matchingSymbolTableEntryForModule = activeSymbolTableScope->get()->tryGetOptimizedSignatureForModuleCall(stmtAsCallStmt->target->name, stmtAsCallStmt->parameters, areCallerArgumentsBasedOnOptimizedSignature); matchingSymbolTableEntryForModule.has_value()) {
                const auto& parametersOfOptimizedModuleSignature = matchingSymbolTableEntryForModule->determineOptimizedCallSignature(nullptr);
                return !std::any_of(parametersOfOptimizedModuleSignature.cbegin(), parametersOfOptimizedModuleSignature.cend(), [](const syrec::Variable::ptr& parameter) {
                    return isVariableReadOnly(*parameter);
                });
            }
        }
    }
    return false;
}

void optimizations::Optimizer::emplaceSingleSkipStatementIfContainerIsEmpty(std::optional<std::vector<std::unique_ptr<syrec::Statement>>>& statementsContainer) {
    if (!statementsContainer.has_value() || statementsContainer->empty()) {
        std::vector<std::unique_ptr<syrec::Statement>> singleSkipStatementContainer;
        singleSkipStatementContainer.emplace_back(std::make_unique<syrec::SkipStatement>());
        statementsContainer.emplace(std::move(singleSkipStatementContainer));
    }
}

std::size_t optimizations::Optimizer::incrementInternalIfStatementNestingLevelCounter() {
    if (internalIfStatementNestingLevel == UINT_MAX) {
        return internalIfStatementNestingLevel;
    }
    return internalIfStatementNestingLevel++;
}

void optimizations::Optimizer::decrementInternalIfStatementNestingLevelCounter() {
    if (!internalIfStatementNestingLevel) {
        return;
    }
    internalIfStatementNestingLevel--;
}

std::size_t optimizations::Optimizer::incrementInternalLoopNestingLevel() {
    if (internalLoopNestingLevel == UINT_MAX) {
        return internalLoopNestingLevel;
    }
    return internalLoopNestingLevel++;
}

void optimizations::Optimizer::decrementInternalLoopNestingLevel() {
    if (!internalLoopNestingLevel) {
        return;
    }
    internalLoopNestingLevel--;
}

void optimizations::Optimizer::resetInternalNestingLevelsForIfAndLoopStatements() {
    internalLoopNestingLevel = 0;
    internalIfStatementNestingLevel = 0;
}

std::unordered_set<std::size_t> optimizations::Optimizer::determineIndicesOfUnusedModules(const std::vector<parser::SymbolTable::ModuleCallSignature>& unoptimizedModuleCallSignatures) const {
    const auto& activeSymbolTableScope = getActiveSymbolTableScope();
    if (!activeSymbolTableScope) {
        return {};
    }

    std::unordered_set<std::size_t> unusedModuleIndexLookup;
    for (std::size_t i = 0; i < unoptimizedModuleCallSignatures.size(); ++i) {
        if (const auto& notUsageStatusOfModule = activeSymbolTableScope->get()->determineIfModuleWasUnused(unoptimizedModuleCallSignatures.at(i)); notUsageStatusOfModule.has_value() && *notUsageStatusOfModule) {
            unusedModuleIndexLookup.emplace(i);
        }
    }
    return unusedModuleIndexLookup;
}

optimizations::Optimizer::OptimizationResult<syrec::Module> optimizations::Optimizer::removeUnusedModulesFrom(const std::vector<std::reference_wrapper<const syrec::Module>>& unoptimizedModules, const ExpectedMainModuleCallSignature& expectedMainModuleSignature) const {
    const auto& activeSymbolTableScope = getActiveSymbolTableScope();
    if (!activeSymbolTableScope) {
        return OptimizationResult<syrec::Module>::asUnchangedOriginal();
    }
    
    if (const auto& unusedModuleIndices = determineIndicesOfUnusedModules(createCallSignaturesFrom(unoptimizedModules)); !unusedModuleIndices.empty()) {
        std::vector<std::unique_ptr<syrec::Module>> remainingModulesContainer;
        remainingModulesContainer.reserve(std::max(static_cast<std::size_t>(1), unoptimizedModules.size() - unusedModuleIndices.size()));

        for (std::size_t i = 0; i < unoptimizedModules.size(); ++i) {
            /*
             * If a module with ident "main" was declared or if the last declared module should be removed in case the former was not declared
             * should not be removed, since the given module is the assumed "main" module and thus required for a valid SyReC grammar.
             */
            if (!unusedModuleIndices.count(i)) {
                remainingModulesContainer.emplace_back(copyUtils::createCopyOfModule(unoptimizedModules.at(i)));
            }
            else {
                const auto& callSignatureContainerOfUnoptimizedModule = createCallSignaturesFrom({unoptimizedModules.at(i)});
                if (doesModuleMatchMainModuleSignature(callSignatureContainerOfUnoptimizedModule.front(), expectedMainModuleSignature)) {
                    remainingModulesContainer.emplace_back(copyUtils::createCopyOfModule(unoptimizedModules.at(i)));
                }
            }
        }
        return OptimizationResult<syrec::Module>::fromOptimizedContainer(std::move(remainingModulesContainer));
    }
    return OptimizationResult<syrec::Module>::asUnchangedOriginal();
}

optimizations::Optimizer::OptimizationResult<syrec::Module> optimizations::Optimizer::removeUnusedModulesFrom(const std::vector<parser::SymbolTable::ModuleCallSignature>& unoptimizedModuleCallSignatures, std::vector<std::unique_ptr<syrec::Module>>& optimizedModules, const ExpectedMainModuleCallSignature& expectedMainModuleSignature) const {
    const auto& activeSymbolTableScope = getActiveSymbolTableScope();
    if (!activeSymbolTableScope) {
        return OptimizationResult<syrec::Module>::asUnchangedOriginal();
    }

    if (const auto& unusedModuleIndices = determineIndicesOfUnusedModules(unoptimizedModuleCallSignatures); !unusedModuleIndices.empty()) {
        std::vector<std::unique_ptr<syrec::Module>> remainingModulesContainer;
        remainingModulesContainer.reserve(std::max(static_cast<std::size_t>(1), optimizedModules.size() - unusedModuleIndices.size()));
        for (std::size_t i = 0; i < optimizedModules.size(); ++i) {
            /*
             * If a module with ident "main" was declared or if the last declared module should be removed in case the former was not declared
             * should not be removed, since the given module is the assumed "main" module and thus required for a valid SyReC grammar.
             */
            if (!unusedModuleIndices.count(i) || doesModuleMatchMainModuleSignature(unoptimizedModuleCallSignatures.at(i), expectedMainModuleSignature)) {
                remainingModulesContainer.emplace_back(std::move(optimizedModules.at(i)));
            }
        }
        return OptimizationResult<syrec::Module>::fromOptimizedContainer(std::move(remainingModulesContainer));
    }
    return OptimizationResult<syrec::Module>::asUnchangedOriginal();
}

bool optimizations::Optimizer::canModuleCallBeRemovedWhenIgnoringReferenceCount(const parser::SymbolTable::ModuleIdentifier& moduleIdentifier, const parser::SymbolTable::ModuleCallSignature& userProvidedCallSignature) const {
    const auto& activeSymbolTableScope = getActiveSymbolTableScope();
    if (!activeSymbolTableScope.has_value() || !parserConfig.deadCodeEliminationEnabled) {
        return false;
    }

   const auto& fullModuleInformationFromSymbolTable = activeSymbolTableScope->get()->getFullDeclaredModuleInformation(moduleIdentifier);
    if (!fullModuleInformationFromSymbolTable.has_value()) {
       return false;
    }

    const auto& moduleSignatureFromSymbolTable = activeSymbolTableScope->get()->getMatchingModuleForIdentifier(moduleIdentifier);
    if (!moduleSignatureFromSymbolTable.has_value()) {
        return false;
    }
    
    if (dangerousComponentCheckUtils::doesOptimizedModuleSignatureContainNoParametersOrOnlyReadonlyOnes(userProvidedCallSignature)) {
        return !dangerousComponentCheckUtils::doesModuleContainPotentiallyDangerousComponent(**fullModuleInformationFromSymbolTable, **activeSymbolTableScope);
    }
    return !(dangerousComponentCheckUtils::doesOptimizedModuleBodyContainAssignmentToModifiableParameter(**fullModuleInformationFromSymbolTable, **activeSymbolTableScope) || dangerousComponentCheckUtils::doesModuleContainPotentiallyDangerousComponent(**fullModuleInformationFromSymbolTable, **activeSymbolTableScope));

  /*  bool canModuleCallBeRemovedFlag = false;
    if (const auto& moduleInformationFromSymbolTable = activeSymbolTableScope->get()->getFullDeclaredModuleInformation(moduleIdentifier); moduleInformationFromSymbolTable.has_value()) {
        if (const auto& moduleSignatureInformation = activeSymbolTableScope->get()->getMatchingModuleForIdentifier(moduleIdentifier); moduleSignatureInformation.has_value()) {
            const auto&                     optimizedCallSignature = moduleSignatureInformation->determineOptimizedCallSignature(nullptr);
            std::unordered_set<std::string> lookupOfModifiableModuleVariables;
            for (std::size_t i = 0; i < optimizedCallSignature.size(); ++i) {
                if (!isVariableReadOnly(*optimizedCallSignature.at(i))) {
                    lookupOfModifiableModuleVariables.emplace(optimizedCallSignature.at(i)->name);
                }
            }
            
            if (!lookupOfModifiableModuleVariables.empty()) {
                const auto& moduleBodyStatements = moduleInformationFromSymbolTable->get()->statements;
                for (std::size_t i = 0; i < moduleBodyStatements.size() && !canModuleCallBeRemovedFlag; ++i) {
                    canModuleCallBeRemovedFlag = isAnyModifiableParameterOrLocalModifiedInStatement(*moduleBodyStatements.at(i), lookupOfModifiableModuleVariables, true);
                }
            }
            else {
                canModuleCallBeRemovedFlag = true;
            }
        }
    }
    return canModuleCallBeRemovedFlag;*/
}

std::optional<unsigned> optimizations::Optimizer::tryFetchValueOfLoopVariable(const std::string& loopVariableIdent, const parser::SymbolTable& activeSymbolTableScope, bool isConstantPropagationEnabled, const syrec::Number::loop_variable_mapping& evaluableLoopVariablesIfConstantPropagationIsDisabled) {
    return isConstantPropagationEnabled || evaluableLoopVariablesIfConstantPropagationIsDisabled.count(loopVariableIdent) ? activeSymbolTableScope.tryFetchValueOfLoopVariable(loopVariableIdent) : std::nullopt;
}

bool optimizations::Optimizer::doesLoopBodyDefineAnyPotentiallyDangerousStmt(const syrec::Statement::vec& unoptimizedLoopBodyStmts, const std::optional<std::vector<std::unique_ptr<syrec::Statement>>>& optimizedLoopBodyStmts, const parser::SymbolTable& activeSymbolTableScope) {
    if (optimizedLoopBodyStmts.has_value()) {
        return std::any_of(
            optimizedLoopBodyStmts->begin(), 
            optimizedLoopBodyStmts->end(), 
            [&activeSymbolTableScope](const std::unique_ptr<syrec::Statement>& optimizedLoopBodyStmt) {
                    return dangerousComponentCheckUtils::doesStatementContainPotentiallyDangerousComponent(*optimizedLoopBodyStmt, activeSymbolTableScope);
                });
    }
    return std::any_of(
        unoptimizedLoopBodyStmts.cbegin(),
        unoptimizedLoopBodyStmts.cend(),
        [&activeSymbolTableScope](const syrec::Statement::ptr& unoptimizedLoopBodyStmt) {
                return dangerousComponentCheckUtils::doesStatementContainPotentiallyDangerousComponent(*unoptimizedLoopBodyStmt, activeSymbolTableScope);
    });
}

bool optimizations::Optimizer::doesModuleMatchMainModuleSignature(const parser::SymbolTable::ModuleCallSignature& moduleSignature, const ExpectedMainModuleCallSignature& expectedMainModuleSignature) {
    bool doSignaturesMatch = moduleSignature.moduleIdent == expectedMainModuleSignature.moduleIdent && moduleSignature.parameters.size() == expectedMainModuleSignature.parameterIdentsOfUnoptimizedModuleVersion.size();
    for (std::size_t i = 0; i < moduleSignature.parameters.size() && doSignaturesMatch; ++i) {
        doSignaturesMatch &= moduleSignature.parameters.at(i)->name == expectedMainModuleSignature.parameterIdentsOfUnoptimizedModuleVersion.at(i);
    }
    return doSignaturesMatch;
}


std::optional<optimizations::Optimizer::ExpectedMainModuleCallSignature> optimizations::Optimizer::determineActualMainModuleName(const std::vector<std::reference_wrapper<const syrec::Module>>& unoptimizedModules, const std::string_view& defaultMainModuleIdent) {
    if (unoptimizedModules.empty()) {
        return std::nullopt;
    }

    std::string identifiedMainModuleName = std::string(defaultMainModuleIdent);
    syrec::Variable::vec     mainModuleParameters;
    std::vector<std::string> mainModuleParameterNamesContainer;
    if (const auto& moduleMatchingDefaultMainModuleIdent = std::find_if(
            unoptimizedModules.cbegin(),
            unoptimizedModules.cend(),
            [&defaultMainModuleIdent](const syrec::Module& module) {
                return module.name == defaultMainModuleIdent;
            }); moduleMatchingDefaultMainModuleIdent != unoptimizedModules.cend()) {
        mainModuleParameters = moduleMatchingDefaultMainModuleIdent->get().parameters;
    }
    else {
        identifiedMainModuleName = unoptimizedModules.back().get().name;
        mainModuleParameters = unoptimizedModules.back().get().parameters;
    }

    mainModuleParameterNamesContainer.reserve(mainModuleParameters.size());
    std::transform(
            mainModuleParameters.cbegin(),
            mainModuleParameters.cend(),
            std::back_inserter(mainModuleParameterNamesContainer),
            [](const syrec::Variable::ptr& parameter) {
                return parameter->name;
            });
    return ExpectedMainModuleCallSignature({identifiedMainModuleName, mainModuleParameterNamesContainer});
}

std::vector<parser::SymbolTable::ModuleCallSignature> optimizations::Optimizer::createCallSignaturesFrom(const std::vector<std::reference_wrapper<const syrec::Module>>& modules) {
    if (modules.empty()) {
        return {};
    }

    std::vector<parser::SymbolTable::ModuleCallSignature> callSignatures;
    callSignatures.reserve(modules.size());

    std::transform(
            modules.cbegin(),
            modules.cend(),
            std::back_inserter(callSignatures),
            [](const syrec::Module& userProvidedCall) {
                return parser::SymbolTable::ModuleCallSignature({userProvidedCall.name, userProvidedCall.parameters});
            });
    return callSignatures;
}

void optimizations::Optimizer::insertStatementCopiesInto(syrec::Statement::vec& containerForCopies, std::vector<std::unique_ptr<syrec::Statement>> copiesOfStatements) {
    for (auto&& copyOfStmt : copiesOfStatements) {
        syrec::Statement::ptr sharedReferenceToCopyOfStmt = std::move(copyOfStmt);
        containerForCopies.emplace_back(sharedReferenceToCopyOfStmt);
    }
}

bool optimizations::Optimizer::optimizeUnrolledLoopBodyStatements(syrec::Statement::vec& containerForResult, std::size_t numUnrolledIterations, LoopUnroller::UnrolledIterationInformation& unrolledLoopBodyStatementsInformation) {
    if (unrolledLoopBodyStatementsInformation.iterationStatements.empty() || !numUnrolledIterations) {
        return true;
    }

    bool wasOptimizationCancelledPrematurely = false;
    for (std::size_t i = 0; i < numUnrolledIterations && !wasOptimizationCancelledPrematurely; ++i) {
        if (unrolledLoopBodyStatementsInformation.loopVariableValuePerIteration.has_value()) {
            updateValueOfLoopVariable(unrolledLoopBodyStatementsInformation.loopVariableValuePerIteration->loopVariableIdent, unrolledLoopBodyStatementsInformation.loopVariableValuePerIteration->valuePerIteration.at(i));    
        }
        wasOptimizationCancelledPrematurely = !optimizePeeledLoopBodyStatements(containerForResult, unrolledLoopBodyStatementsInformation.iterationStatements);
    }
    return !wasOptimizationCancelledPrematurely;
}

bool optimizations::Optimizer::optimizePeeledLoopBodyStatements(syrec::Statement::vec& containerForResult, const std::vector<std::unique_ptr<syrec::Statement>>& peeledLoopBodyStatements) {
    for (auto&& unrolledStmt: peeledLoopBodyStatements) {
        if (auto optimizationResultContainerOfUnrolledStmt = handleStatement(*unrolledStmt); optimizationResultContainerOfUnrolledStmt.getStatusOfResult() != OptimizationResultFlag::RemovedByOptimization) {
            std::optional<std::vector<std::unique_ptr<syrec::Statement>>> containerOwningOptimizationResult;
            if (optimizationResultContainerOfUnrolledStmt.getStatusOfResult() == OptimizationResultFlag::IsUnchanged) {
                if (auto createdCopyOfUnrolledStmt = copyUtils::createCopyOfStmt(*unrolledStmt); createdCopyOfUnrolledStmt) {
                    std::vector<std::unique_ptr<syrec::Statement>> intermediateContainer;
                    intermediateContainer.reserve(1);
                    intermediateContainer.emplace_back(std::move(createdCopyOfUnrolledStmt));
                    containerOwningOptimizationResult = std::move(intermediateContainer);
                } else {
                    return false;
                }
            } else {
                containerOwningOptimizationResult   = optimizationResultContainerOfUnrolledStmt.tryTakeOwnershipOfOptimizationResult();
                if (!containerOwningOptimizationResult.has_value()) {
                    return false;
                }
            }

            for (auto&& optimizedSubStmt: containerOwningOptimizationResult.value()) {
                containerForResult.emplace_back(std::move(optimizedSubStmt));
            }   
        }
    }
    return true;
}

const parser::SymbolTable* optimizations::Optimizer::getActiveSymbolTableForEvaluation() const {
    if (const auto& activeSymbolTableScope = getActiveSymbolTableScope(); activeSymbolTableScope.has_value()) {
        return activeSymbolTableScope->get();
    }
    return nullptr;
}

bool optimizations::Optimizer::makeNewlyGeneratedSignalsAvailableInSymbolTableScope(parser::SymbolTable& symbolTableScope, const parser::SymbolTable::ModuleIdentifier& identifierOfModuleToWhichSignalsWillBeAddedTo, const syrec::Variable::vec& newlyGeneratedSignalsUsedForReplacements) {
    /*
     * Since we are here only updating the module based on the given module identifier in the global symbol table scope, we need to additionally add the information about the newly added signals for the current module
     * to the currently active symbol table scope for said module
     */
    if (!symbolTableScope.addNewLocalSignalsToModule(identifierOfModuleToWhichSignalsWillBeAddedTo, newlyGeneratedSignalsUsedForReplacements)) {
        return false;
    }
    
    for (const syrec::Variable::ptr& newlyGeneratedSignalUsedForReplacements : newlyGeneratedSignalsUsedForReplacements) {
        createSymbolTableEntryForVariable(symbolTableScope, *newlyGeneratedSignalUsedForReplacements);
        /*
         * We do not update the reference counts of the newly generated replacement signals not referencing existing signals here since the updates of the reference counts
         * is done in a later step when the information of all signals is available in the symbol table. Which is the case after this call.
         */
    }
    return true;
}


void optimizations::Optimizer::moveOwningCopiesOfStatementsBetweenContainers(std::vector<std::unique_ptr<syrec::Statement>>& toBeMovedToContainer, std::vector<InternalOwningAssignmentType>&& toBeMovedFromContainer) {
    for (InternalOwningAssignmentType& i: toBeMovedFromContainer) {
        InternalOwningAssignmentType toBeMovedAssignmentFromContainer = std::move(i);
        if (std::holds_alternative<std::unique_ptr<syrec::AssignStatement>>(toBeMovedAssignmentFromContainer)) {
            toBeMovedToContainer.push_back(std::get<std::unique_ptr<syrec::AssignStatement>>(std::move(toBeMovedAssignmentFromContainer)));
        } else if (std::holds_alternative<std::unique_ptr<syrec::UnaryStatement>>(toBeMovedAssignmentFromContainer)) {
            toBeMovedToContainer.push_back(std::get<std::unique_ptr<syrec::UnaryStatement>>(std::move(toBeMovedAssignmentFromContainer)));
        }
    }
}

optimizations::Optimizer::ReferenceCountLookupForStatement& optimizations::Optimizer::ReferenceCountLookupForStatement::mergeWith(const ReferenceCountLookupForStatement& other) {
    for (const auto& [signalIdent, referenceCounts] : other.signalReferenceCountLookup) {
        if (signalReferenceCountLookup.count(signalIdent)) {
            const std::size_t currentReferenceCountsForSignal = signalReferenceCountLookup.at(signalIdent);
            if ((SIZE_MAX - currentReferenceCountsForSignal) < referenceCounts) {
                signalReferenceCountLookup[signalIdent] = SIZE_MAX;
            }
            else {
                signalReferenceCountLookup[signalIdent] += referenceCounts;
            }
        }
        else {
            signalReferenceCountLookup.insert(std::make_pair(signalIdent, referenceCounts));
        }
    }
    return *this;
}

optimizations::Optimizer::ReferenceCountLookupForStatement optimizations::Optimizer::ReferenceCountLookupForStatement::createFromStatement(const syrec::Statement& statement) {
    if (const auto& stmtAsUnaryAssignmentStmt = dynamic_cast<const syrec::UnaryStatement*>(&statement);stmtAsUnaryAssignmentStmt) {
        return createFromSignalAccess(*stmtAsUnaryAssignmentStmt->var);
    }
    if (const auto& stmtAsBinaryAssignmentStmt = dynamic_cast<const syrec::AssignStatement*>(&statement); stmtAsBinaryAssignmentStmt) {
        return createFromSignalAccess(*stmtAsBinaryAssignmentStmt->lhs).mergeWith(createFromExpression(*stmtAsBinaryAssignmentStmt->rhs));
    }
    return {};
}

optimizations::Optimizer::ReferenceCountLookupForStatement optimizations::Optimizer::ReferenceCountLookupForStatement::createFromExpression(const syrec::Expression& expression) {
    if (const auto& exprAsVariableExpr = dynamic_cast<const syrec::VariableExpression*>(&expression); exprAsVariableExpr) {
        return createFromSignalAccess(*exprAsVariableExpr->var);
    }
    if (const auto& exprAsShiftExpr = dynamic_cast<const syrec::ShiftExpression*>(&expression); exprAsShiftExpr) {
        return createFromExpression(*exprAsShiftExpr->lhs).mergeWith(createFromNumber(*exprAsShiftExpr->rhs));
    }
    if (const auto& exprAsBinaryExpr = dynamic_cast<const syrec::BinaryExpression*>(&expression); exprAsBinaryExpr) {
        return createFromExpression(*exprAsBinaryExpr->lhs).mergeWith(createFromExpression(*exprAsBinaryExpr->rhs));
    }
    if (const auto& exprAsNumericExpr = dynamic_cast<const syrec::NumericExpression*>(&expression); exprAsNumericExpr) {
        return createFromNumber(*exprAsNumericExpr->value);
    }
    return {};
}

optimizations::Optimizer::ReferenceCountLookupForStatement optimizations::Optimizer::ReferenceCountLookupForStatement::createFromNumber(const syrec::Number& number) {
    if (!number.isCompileTimeConstantExpression()) {
        return {};
    }
    const syrec::Number::CompileTimeConstantExpression& numberAsCompileTimeConstantExpression = number.getExpression();
    return createFromNumber(*numberAsCompileTimeConstantExpression.lhsOperand).mergeWith(createFromNumber(*numberAsCompileTimeConstantExpression.rhsOperand));
}

optimizations::Optimizer::ReferenceCountLookupForStatement optimizations::Optimizer::ReferenceCountLookupForStatement::createFromSignalAccess(const syrec::VariableAccess& signalAccess) {
    ReferenceCountLookupForStatement aggregateLookup = ReferenceCountLookupForStatement();
    aggregateLookup.signalReferenceCountLookup.insert(std::make_pair(signalAccess.var->name, 1));
    for (const syrec::Expression::ptr& exprDefiningAccessedValueOfDimension: signalAccess.indexes) {
        aggregateLookup = aggregateLookup.mergeWith(createFromExpression(*exprDefiningAccessedValueOfDimension));
    }
    if (signalAccess.range.has_value()) {
        const syrec::Number::ptr& bitRangeStart = signalAccess.range->first;
        const syrec::Number::ptr& bitRangeEnd = signalAccess.range->second;
        return aggregateLookup.mergeWith(createFromNumber(*bitRangeStart).mergeWith(createFromNumber(*bitRangeEnd)));
    }
    return aggregateLookup;
}

optimizations::Optimizer::ReferenceCountLookupForStatement::ReferenceCountDifferences optimizations::Optimizer::ReferenceCountLookupForStatement::getDifferencesBetweenThisAndOther(const ReferenceCountLookupForStatement& other) const {
    std::unordered_map<std::string, std::size_t> lookupOfSignalsWithIncrementedSignalCounts;
    std::unordered_map<std::string, std::size_t> lookupOfSignalsWithDecrementedSignalCounts;

    for (const auto& [signalIdent, referenceCount] : other.signalReferenceCountLookup) {
        if (!signalReferenceCountLookup.count(signalIdent)) {
            lookupOfSignalsWithIncrementedSignalCounts.insert(std::make_pair(signalIdent, referenceCount));
        }
        else {
            const std::size_t referenceCountsInThisLookup = signalReferenceCountLookup.at(signalIdent);
            if (referenceCount != referenceCountsInThisLookup) {
                if (referenceCount < referenceCountsInThisLookup) {
                    lookupOfSignalsWithDecrementedSignalCounts.insert(std::make_pair(signalIdent, referenceCountsInThisLookup - referenceCount));
                }
                else {
                    lookupOfSignalsWithIncrementedSignalCounts.insert(std::make_pair(signalIdent, referenceCount - referenceCountsInThisLookup));
                }
            }
            else {
                lookupOfSignalsWithIncrementedSignalCounts.insert(std::make_pair(signalIdent, 0));
            }
        }
    }

    for (const auto& [signalIdent, referenceCount] : signalReferenceCountLookup) {
        if(!lookupOfSignalsWithIncrementedSignalCounts.count(signalIdent)) {
            lookupOfSignalsWithDecrementedSignalCounts.insert(std::make_pair(signalIdent, referenceCount));
        }
    }

    return ReferenceCountDifferences({lookupOfSignalsWithIncrementedSignalCounts, lookupOfSignalsWithDecrementedSignalCounts});
}

void optimizations::Optimizer::updateReferenceCountsOfOptimizedAssignments(const parser::SymbolTable& symbolTable, const ReferenceCountLookupForStatement::ReferenceCountDifferences& referenceCountDifferences) {
    for (const auto& [signalIdent, incrementedReferenceCounts] : referenceCountDifferences.lookupOfSignalsWithIncrementedReferenceCounts) {
        symbolTable.updateReferenceCountOfLiteralNTimes(signalIdent, incrementedReferenceCounts, parser::SymbolTable::ReferenceCountUpdate::Increment);
    }
    for (const auto& [signalIdent, decrementedReferenceCounts]: referenceCountDifferences.lookupOfSignalsWithDecrementedReferenceCounts) {
        symbolTable.updateReferenceCountOfLiteralNTimes(signalIdent, decrementedReferenceCounts, parser::SymbolTable::ReferenceCountUpdate::Decrement);
    }
}
