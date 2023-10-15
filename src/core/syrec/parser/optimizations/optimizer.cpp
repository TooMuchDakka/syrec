#include "core/syrec/parser/optimizations/optimizer.hpp"

#include "core/syrec/parser/module_call_guess.hpp"
#include "core/syrec/parser/operation.hpp"
#include "core/syrec/parser/range_check.hpp"
#include "core/syrec/parser/optimizations/reassociate_expression.hpp"
#include "core/syrec/parser/optimizations/deadStoreElimination/dead_store_eliminator.hpp"
#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/main_additional_line_for_assignment_simplifier.hpp"
#include "core/syrec/parser/utils/binary_expression_simplifier.hpp"
#include "core/syrec/parser/utils/bit_helpers.hpp"
#include "core/syrec/parser/utils/loop_range_utils.hpp"

#include <functional>
#include <unordered_set>

// TODO: Refactoring if case switches with visitor pattern as its available in std::visit call
optimizations::Optimizer::OptimizationResult<syrec::Module> optimizations::Optimizer::optimizeProgram(const std::vector<std::reference_wrapper<const syrec::Module>>& modules, const std::optional<std::string>& userDefinedExpectedMainModuleIdent) {
    std::vector<std::unique_ptr<syrec::Module>> optimizedModules;
    bool                                        optimizedAnyModule = false;

    std::optional<std::string> mainModuleIdent = userDefinedExpectedMainModuleIdent;
    if (!userDefinedExpectedMainModuleIdent.has_value() && !modules.empty()) {
        mainModuleIdent = modules.end()->get().name;
    }

    for (const auto& module: modules) {
        auto&&     optimizedModule          = handleModule(module, mainModuleIdent);
        const auto optimizationResultStatus = optimizedModule.getStatusOfResult();
        optimizedAnyModule                  |= optimizationResultStatus != OptimizationResultFlag::IsUnchanged;

        if (optimizationResultStatus == OptimizationResultFlag::WasOptimized) {
            if (auto optimizedModuleData = optimizedModule.tryTakeOwnershipOfOptimizationResult(); optimizedModuleData.has_value()) {
                optimizedModules.emplace_back(std::move(optimizedModuleData->front()));
            }
        } else if (optimizedModule.getStatusOfResult() == OptimizationResultFlag::IsUnchanged) {
            optimizedModules.emplace_back(createCopyOfModule(module));
        }
    }

    if (optimizedAnyModule) {
        return !optimizedModules.empty() ? OptimizationResult<syrec::Module>::fromOptimizedContainer(std::move(optimizedModules)) : OptimizationResult<syrec::Module>::asOptimizedAwayContainer();
    }
    return OptimizationResult<syrec::Module>::asUnchangedOriginal();
}

// TODO:
optimizations::Optimizer::OptimizationResult<syrec::Module> optimizations::Optimizer::handleModule(const syrec::Module& module, const std::optional<std::string>& optionalMainModuleIdent) {
    auto copyOfModule = createCopyOfModule(module);

    std::size_t generatedInternalIdForModule = 0;
    activeSymbolTableScope->addEntry(*copyOfModule, &generatedInternalIdForModule);

    openNewSymbolTableScope();
    createSymbolTableEntriesForModuleParametersAndLocalVariables(module);
    
    if (auto optimizationResultOfModuleStatements = handleStatements(transformCollectionOfSharedPointersToReferences(module.statements)); optimizationResultOfModuleStatements.getStatusOfResult() != OptimizationResultFlag::IsUnchanged) {
        if (optimizationResultOfModuleStatements.getStatusOfResult() == OptimizationResultFlag::WasOptimized) {
           if (auto resultOfModuleBodyOptimization = optimizationResultOfModuleStatements.tryTakeOwnershipOfOptimizationResult(); resultOfModuleBodyOptimization.has_value()) {
               copyOfModule->statements = createStatementListFrom({}, std::move(*resultOfModuleBodyOptimization));
           }   
        }
        else {
            copyOfModule->statements.clear();
        }
    }

    const auto& activeSymbolTableScope           = *getActiveSymbolTableScope();
    const auto& numberOfDeclaredModuleStatements = copyOfModule->statements.size();
    if (parserConfig.deadStoreEliminationEnabled && !copyOfModule->statements.empty()) {
        const auto deadStoreEliminator = std::make_unique<deadStoreElimination::DeadStoreEliminator>(activeSymbolTableScope);
        deadStoreEliminator->removeDeadStoresFrom(copyOfModule->statements);
    }

    bool wereAnyParametersOrLocalSignalsRemoved = false;
    if (parserConfig.deadCodeEliminationEnabled) {
        const auto& indicesOfRemovedLocalVariablesOfModule = activeSymbolTableScope->fetchUnusedLocalModuleVariablesAndRemoveFromSymbolTable(module.name, generatedInternalIdForModule);
        removeElementsAtIndices(copyOfModule->variables, indicesOfRemovedLocalVariablesOfModule);

        const auto& indicesOfUnusedParameters = activeSymbolTableScope->updateOptimizedModuleSignatureByMarkingAndReturningIndicesOfUnusedParameters(module.name, generatedInternalIdForModule);
        removeElementsAtIndices(copyOfModule->parameters, indicesOfUnusedParameters);

        wereAnyParametersOrLocalSignalsRemoved = !indicesOfRemovedLocalVariablesOfModule.empty() || !indicesOfUnusedParameters.empty();
    }
    
    auto doesOptimizedModuleBodyContainAnyStatement = !copyOfModule->statements.empty();
    auto doesOptimizedModuleOnlyHaveReadOnlyParameters = copyOfModule->parameters.empty()
        || std::all_of(copyOfModule->parameters.cbegin(), copyOfModule->parameters.cend(), [](const syrec::Variable::ptr& declaredParameter) {
                return isVariableReadOnly(*declaredParameter);
        });

    auto doesModulePerformAssignmentToAnyModifiableParameterOrLocal = isAnyModifiableParameterOrLocalModifiedInModuleBody(*copyOfModule);
    /*
     * If we can determine that the main module only contains read-only parameters or does not perform assignments to either local signals and modifiable parameters,
     * the former can be removed but since the program must consist of some module, the main module will be stripped of all parameters, local signals as well as any
     * declared statements and its module body will only consist of a signal skip statement (which is required so that the module matches the SyReC grammar).
     */
    if (parserConfig.deadCodeEliminationEnabled && doesCurrentModuleIdentMatchUserDefinedMainModuleIdent(module.name, optionalMainModuleIdent) 
            && ((doesOptimizedModuleOnlyHaveReadOnlyParameters && (copyOfModule->variables.empty() || !doesModulePerformAssignmentToAnyModifiableParameterOrLocal)) 
                || !doesModulePerformAssignmentToAnyModifiableParameterOrLocal)) {
        /*
         * TODO: Check this assumption.
         * Dead code elimination should have already removed calls/uncalls to modules with read-only parameters or without assignments to local signals and modifiable parameters,
         * so no update of the reference counts for said calls/uncalls should be required here.
         *
         * Additionally, we can leave the symbol table entry of the module unchanged as the declared module signature should not change while the internal optimized module signature is already fixed
         * beforehand while determining the unused parameters.
         */
        copyOfModule->statements.clear();
        copyOfModule->parameters.clear();
        copyOfModule->variables.clear();
        doesOptimizedModuleBodyContainAnyStatement = false;
        doesModulePerformAssignmentToAnyModifiableParameterOrLocal = false;
        doesOptimizedModuleOnlyHaveReadOnlyParameters              = false;
    }
    
    /*
     * The declared main module body cannot be empty as defined by the SyReC grammar, thus we append a single skip statement in this case.
     */
    if (!doesOptimizedModuleBodyContainAnyStatement && doesCurrentModuleIdentMatchUserDefinedMainModuleIdent(module.name, optionalMainModuleIdent)) {
        const auto& generatedSkipStmtForMainModuleWithEmptyBody = std::make_shared<syrec::SkipStatement>();
        copyOfModule->statements.emplace_back(generatedSkipStmtForMainModuleWithEmptyBody);
        doesOptimizedModuleBodyContainAnyStatement = true;
    }

    closeActiveSymbolTableScope();
    const auto& globalSymbolTableScope = *getActiveSymbolTableScope();
    if (!doesCurrentModuleIdentMatchUserDefinedMainModuleIdent(module.name, optionalMainModuleIdent) 
        && (!doesOptimizedModuleBodyContainAnyStatement 
            || (doesOptimizedModuleOnlyHaveReadOnlyParameters && copyOfModule->variables.empty())
            || (copyOfModule->parameters.empty() && copyOfModule->variables.empty())
            || !doesModulePerformAssignmentToAnyModifiableParameterOrLocal)) {
        globalSymbolTableScope->changeStatementsOfModule(module.name, generatedInternalIdForModule, {});
        return OptimizationResult<syrec::Module>::asOptimizedAwayContainer();
    }

    if (numberOfDeclaredModuleStatements == copyOfModule->statements.size() && !wereAnyParametersOrLocalSignalsRemoved) {
        return OptimizationResult<syrec::Module>::asUnchangedOriginal();
    }

    globalSymbolTableScope->changeStatementsOfModule(module.name, generatedInternalIdForModule, copyOfModule->statements);
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
            optimizedStmts.emplace_back(createCopyOfStmt(stmt));
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

optimizations::Optimizer::OptimizationResult<syrec::Statement> optimizations::Optimizer::handleAssignmentStmt(const syrec::AssignStatement& assignmentStmt) const {
    syrec::VariableAccess::ptr lhsOperand = assignmentStmt.lhs;
    if (auto simplificationResultOfAssignedToSignal = handleSignalAccess(*assignmentStmt.lhs, false, nullptr); simplificationResultOfAssignedToSignal.getStatusOfResult() != OptimizationResultFlag::IsUnchanged) {
        auto lhsSignalAccessAfterOptimization = std::move(simplificationResultOfAssignedToSignal.tryTakeOwnershipOfOptimizationResult()->front());
        lhsOperand = std::make_shared<syrec::VariableAccess>(*lhsSignalAccessAfterOptimization);    
    }
    
    syrec::expression::ptr rhsOperand = assignmentStmt.rhs;
    if (auto simplificationResultOfRhsExpr = handleExpr(*assignmentStmt.rhs); simplificationResultOfRhsExpr.getStatusOfResult() != OptimizationResultFlag::IsUnchanged) {
        rhsOperand = std::move(simplificationResultOfRhsExpr.tryTakeOwnershipOfOptimizationResult()->front());
    }

    const auto& rhsOperandEvaluatedAsConstant = tryEvaluateExpressionToConstant(*assignmentStmt.rhs);
    const auto& mappedToAssignmentOperationFromFlag = syrec_operation::tryMapAssignmentOperationFlagToEnum(assignmentStmt.op);

    if (mappedToAssignmentOperationFromFlag.has_value() && rhsOperandEvaluatedAsConstant.has_value() && syrec_operation::isOperandUseAsRhsInOperationIdentityElement(*mappedToAssignmentOperationFromFlag, *rhsOperandEvaluatedAsConstant)
        && parserConfig.deadCodeEliminationEnabled) {
        updateReferenceCountOf(lhsOperand->var->name, parser::SymbolTable::ReferenceCountUpdate::Decrement);
        return OptimizationResult<syrec::Statement>::asOptimizedAwayContainer();
    }

    const auto skipCheckForSplitOfAssignmentInSubAssignments = !parserConfig.noAdditionalLineOptimizationEnabled || rhsOperandEvaluatedAsConstant.has_value();
    if (auto evaluatedAssignedToSignal = tryEvaluateDefinedSignalAccess(*lhsOperand); evaluatedAssignedToSignal.has_value()) {
        /*
         * TODO:
         * When an assignment should be performed and only if we know that the size both operands after an dimension access is at most 1, we can use the perform assignment call
         * Otherwise, we either need to perform a broadcast check (which should have been done in the parser already) which in case that no broadcasting is required still
         * needs to handle assignments of N-d signals (for which we need to determine whether the synthesizer supports such statements).
         */
        performAssignment(*evaluatedAssignedToSignal, *mappedToAssignmentOperationFromFlag, rhsOperandEvaluatedAsConstant);
    }

    if (const auto& activeSymbolTableScope = getActiveSymbolTableScope(); !skipCheckForSplitOfAssignmentInSubAssignments && activeSymbolTableScope.has_value()) {
        const auto& noAdditionalLineAssignmentSimplifier = std::make_unique<noAdditionalLineSynthesis::MainAdditionalLineForAssignmentSimplifier>(*activeSymbolTableScope, nullptr, nullptr);
        const auto assignmentStmtToSimplify = lhsOperand != assignmentStmt.lhs || rhsOperand != assignmentStmt.rhs
            ? std::make_unique<syrec::AssignStatement>(lhsOperand, assignmentStmt.op, rhsOperand)
            : std::make_shared<syrec::AssignStatement>(assignmentStmt);
        
        if (auto generatedSimplifierAssignments = noAdditionalLineAssignmentSimplifier->tryReduceRequiredAdditionalLinesFor(assignmentStmtToSimplify, isValueLookupBlockedByDataFlowAnalysisRestriction(*assignmentStmtToSimplify->lhs)); !generatedSimplifierAssignments.empty()) {
            filterAssignmentsThatDoNotChangeAssignedToSignal(generatedSimplifierAssignments);
            if (generatedSimplifierAssignments.empty()) {
                updateReferenceCountOf(lhsOperand->var->name, parser::SymbolTable::ReferenceCountUpdate::Decrement);
                updateReferenceCountsOfSignalIdentsUsedIn(*rhsOperand, parser::SymbolTable::ReferenceCountUpdate::Decrement);
                return OptimizationResult<syrec::Statement>::asOptimizedAwayContainer();        
            }

            std::vector<std::unique_ptr<syrec::Statement>> remainingSimplifiedAssignments;
            remainingSimplifiedAssignments.reserve(generatedSimplifierAssignments.size());
            for (const auto& generatedSubAssignment : generatedSimplifierAssignments) {
                const auto subAssignmentCasted = std::static_pointer_cast<syrec::AssignStatement>(generatedSubAssignment);
                remainingSimplifiedAssignments.emplace_back(std::make_unique<syrec::AssignStatement>(subAssignmentCasted->lhs, subAssignmentCasted->op, subAssignmentCasted->rhs));
            }
            return OptimizationResult<syrec::Statement>::fromOptimizedContainer(std::move(remainingSimplifiedAssignments));
        }
    }

    if (lhsOperand != assignmentStmt.lhs || rhsOperand != assignmentStmt.rhs) {
        return OptimizationResult<syrec::Statement>::fromOptimizedContainer(std::make_unique<syrec::AssignStatement>(lhsOperand, assignmentStmt.op, rhsOperand));
    }
    return OptimizationResult<syrec::Statement>::asUnchangedOriginal();
}

// TODO: Should the semantic checks that we assume to be already done in the parser be repeated again, i.e. does a matching module exist, etc.
// TODO: Since we have optimized the call statement, we also need to optimize the corresponding uncall statement
optimizations::Optimizer::OptimizationResult<syrec::Statement> optimizations::Optimizer::handleCallStmt(const syrec::CallStatement& callStatement) const {
    const auto& symbolTableScope = getActiveSymbolTableScope();
    if (!symbolTableScope.has_value()) {
        return OptimizationResult<syrec::Statement>::asUnchangedOriginal();
    }

    if (const auto moduleCallGuessResolver = parser::ModuleCallGuess::tryInitializeWithModulesMatchingName(*symbolTableScope, callStatement.target->name); moduleCallGuessResolver.has_value()) {
        // TODO:
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
                for (const auto& callerArgumentSignalIdent : callStatement.parameters) {
                    invalidateValueOfWholeSignal(callerArgumentSignalIdent);
                }
                symbolTableScope->get()->updateReferenceCountOfModulesMatchingSignature(callStatement.target->name, moduleCallGuessResolver->get()->getInternalIdsOfModulesMatchingGuess(), parser::SymbolTable::ReferenceCountUpdate::Increment);
            } else {
                const auto&                     signatureOfCalledModule = modulesMatchingCallSignature.front();
                std::vector<std::string>        callerArgumentsForOptimizedModuleCall;
                std::unordered_set<std::size_t> indicesOfRemainingParameters;
                if (const auto& optimizedModuleSignature = signatureOfCalledModule.determineOptimizedCallSignature(&indicesOfRemainingParameters); !optimizedModuleSignature.empty()) {
                    callerArgumentsForOptimizedModuleCall.resize(optimizedModuleSignature.size());

                    for (const auto& i: indicesOfRemainingParameters) {
                        const auto& symbolTableEntryForCallerArgument = signatureOfCalledModule.declaredParameters.at(i);

                        callerArgumentsForOptimizedModuleCall.emplace_back(symbolTableEntryForCallerArgument->name);
                        if (!(symbolTableEntryForCallerArgument->type == syrec::Variable::Types::Out || symbolTableEntryForCallerArgument->type == syrec::Variable::Types::Inout)) {
                            continue;
                        }
                        invalidateValueOfWholeSignal(symbolTableEntryForCallerArgument->name);
                        updateReferenceCountOf(symbolTableEntryForCallerArgument->name, parser::SymbolTable::ReferenceCountUpdate::Increment);
                    }
                }
                if (indicesOfRemainingParameters.empty()) {
                    return OptimizationResult<syrec::Statement>::asOptimizedAwayContainer();
                }
                symbolTableScope->get()->updateReferenceCountOfModulesMatchingSignature(callStatement.target->name, {signatureOfCalledModule.internalModuleId}, parser::SymbolTable::ReferenceCountUpdate::Increment);
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
            symbolTableScope->get()->updateReferenceCountOfModulesMatchingSignature(callStatement.target->name, moduleCallGuessResolver->get()->getInternalIdsOfModulesMatchingGuess(), parser::SymbolTable::ReferenceCountUpdate::Increment);
        }

        /*
         * In case that our precondition does not hold, invalid callee arguments will not produce an increment of the reference count of all declared signals in all reachable scopes of the symbol table.
         * TODO: Is this assumption OK?
         */
        for (const auto& callerArgument: callStatement.parameters) {
            updateReferenceCountOf(callerArgument, parser::SymbolTable::ReferenceCountUpdate::Increment);
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
                const auto& signatureOfCalledModule = modulesMatchingCallSignature.front();
                std::vector<std::string>        callerArgumentsForOptimizedModuleCall;
                std::unordered_set<std::size_t> indicesOfRemainingParameters;
                if (const auto& optimizedModuleSignature = signatureOfCalledModule.determineOptimizedCallSignature(&indicesOfRemainingParameters); !optimizedModuleSignature.empty()) {
                    callerArgumentsForOptimizedModuleCall.resize(optimizedModuleSignature.size());

                    for (const auto& i: indicesOfRemainingParameters) {
                        const auto& symbolTableEntryForCallerArgument = signatureOfCalledModule.declaredParameters.at(i);
                        callerArgumentsForOptimizedModuleCall.emplace_back(symbolTableEntryForCallerArgument->name);
                    }
                }
                if (indicesOfRemainingParameters.empty()) {
                    return OptimizationResult<syrec::Statement>::asOptimizedAwayContainer();
                }
                if (indicesOfRemainingParameters.size() != signatureOfCalledModule.declaredParameters.size()) {
                    return OptimizationResult<syrec::Statement>::fromOptimizedContainer(std::make_unique<syrec::UncallStatement>(uncallStatement.target, callerArgumentsForOptimizedModuleCall));
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
    syrec::expression::ptr simplifiedGuardExpr = ifStatement.condition;
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

    if (parserConfig.deadCodeEliminationEnabled) {
        if (const auto& constantValueOfGuardCondition = tryEvaluateExpressionToConstant(simplifiedGuardExpr ? *simplifiedGuardExpr : *ifStatement.condition); constantValueOfGuardCondition.has_value()) {
            canTrueBranchBeOmitted = *constantValueOfGuardCondition == 0;
            canFalseBranchBeOmitted = *constantValueOfGuardCondition;
            canChangesMadeInFalseBranchBeIgnored = canFalseBranchBeOmitted;
            canChangesMadeInTrueBranchBeIgnored  = canTrueBranchBeOmitted;
        }
    }

    if (const auto& guardConditionAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(simplifiedGuardExpr ? simplifiedGuardExpr : ifStatement.condition); guardConditionAsBinaryExpr != nullptr) {
        if (const auto& equivalenceResultOfBinaryExprOperands = determineEquivalenceOfOperandsOfBinaryExpr(*guardConditionAsBinaryExpr); equivalenceResultOfBinaryExprOperands.has_value()) {
            canChangesMadeInTrueBranchBeIgnored = !*equivalenceResultOfBinaryExprOperands;
            canChangesMadeInFalseBranchBeIgnored = *equivalenceResultOfBinaryExprOperands;
        }
    }

    std::optional<std::vector<std::unique_ptr<syrec::Statement>>> simplifiedTrueBranchStmtsContainer;
    auto                                                          isTrueBranchEmptyAfterOptimization = false;
    if (!canTrueBranchBeOmitted) {
        openNewSymbolTableBackupScope();
        if (auto simplificationResultOfTrueBranchStmt = handleStatements(transformCollectionOfSharedPointersToReferences(ifStatement.thenStatements)); simplificationResultOfTrueBranchStmt.getStatusOfResult() != OptimizationResultFlag::IsUnchanged) {
            simplifiedTrueBranchStmtsContainer = simplificationResultOfTrueBranchStmt.tryTakeOwnershipOfOptimizationResult();
            wereAnyStmtsInTrueBranchModified   = true;
        }

        isTrueBranchEmptyAfterOptimization = (simplifiedTrueBranchStmtsContainer.has_value() && simplifiedTrueBranchStmtsContainer->empty()) || ifStatement.thenStatements.empty();
        if (canFalseBranchBeOmitted) {
            destroySymbolTableBackupScope();
        }
        else {
            updateBackupOfValuesChangedInScopeAndResetMadeChanges();
        }
    }
    else {
        isTrueBranchEmptyAfterOptimization = true;
        wereAnyStmtsInTrueBranchModified  = true;
    }

    std::optional<std::vector<std::unique_ptr<syrec::Statement>>> simplifiedFalseBranchStmtsContainer;
    auto                                                          isFalseBranchEmptyAfterOptimization = false;
    if (!canFalseBranchBeOmitted) {
        openNewSymbolTableBackupScope();
        if (auto simplificationResultOfFalseBranchStmt = handleStatements(transformCollectionOfSharedPointersToReferences(ifStatement.elseStatements)); simplificationResultOfFalseBranchStmt.getStatusOfResult() != OptimizationResultFlag::IsUnchanged) {
            simplifiedFalseBranchStmtsContainer = simplificationResultOfFalseBranchStmt.tryTakeOwnershipOfOptimizationResult();
            wereAnyStmtsInFalseBranchModified   = true;
        }

        isFalseBranchEmptyAfterOptimization = (simplifiedFalseBranchStmtsContainer.has_value() && simplifiedFalseBranchStmtsContainer->empty()) || ifStatement.elseStatements.empty();
        if (canFalseBranchBeOmitted) {
            destroySymbolTableBackupScope();
        }
    }
    else {
        isFalseBranchEmptyAfterOptimization = true;
        wereAnyStmtsInFalseBranchModified   = true;
    }

    if (!canTrueBranchBeOmitted && !canFalseBranchBeOmitted) {
        const auto backupScopeOfFalseBranch = popCurrentSymbolTableBackupScope();
        const auto backupScopeOfTrueBranch  = popCurrentSymbolTableBackupScope();

        /*
         * At this point the values in the currently active symbol table scope match the ones at the end of the second branch (i.e. the false branch).
         * If the changes made in the false branch can be ignored, we revert the changes made in said branch and update the values in the symbol table
         * with the changes made in the true branch
         */
        if (canChangesMadeInFalseBranchBeIgnored && backupScopeOfTrueBranch.has_value()) {
            makeLocalChangesGlobal(**backupScopeOfTrueBranch, **backupScopeOfFalseBranch);
        }
        /*
         * If no branch can be omitted, we need to merge the changes made in both branches
         */
        if (!canChangesMadeInTrueBranchBeIgnored) {
            if (backupScopeOfFalseBranch.has_value() && backupScopeOfTrueBranch.has_value()) {
                mergeAndMakeLocalChangesGlobal(**backupScopeOfTrueBranch, **backupScopeOfFalseBranch);
            }
        }
    }

    if (!wereAnyStmtsInTrueBranchModified && !wereAnyStmtsInFalseBranchModified && !wasGuardExpressionSimplified) {
        return OptimizationResult<syrec::Statement>::asUnchangedOriginal();           
    }
    
    if (parserConfig.deadCodeEliminationEnabled) {
        if ((canTrueBranchBeOmitted && isFalseBranchEmptyAfterOptimization)
            || (canFalseBranchBeOmitted && isTrueBranchEmptyAfterOptimization)
            || (isTrueBranchEmptyAfterOptimization && isFalseBranchEmptyAfterOptimization)) {
            return OptimizationResult<syrec::Statement>::asOptimizedAwayContainer();
        }
        
        if (canTrueBranchBeOmitted) {
            auto remainingFalseBranchStatements = simplifiedFalseBranchStmtsContainer.has_value()
                ? std::move(*simplifiedFalseBranchStmtsContainer)
                : createCopyOfStatements(ifStatement.elseStatements);
            
            return OptimizationResult<syrec::Statement>::fromOptimizedContainer(std::move(remainingFalseBranchStatements));
        }
        if (canFalseBranchBeOmitted) {
            auto remainingTrueBranchStatements = simplifiedTrueBranchStmtsContainer.has_value()
                ? std::move(*simplifiedTrueBranchStmtsContainer)
                : createCopyOfStatements(ifStatement.thenStatements);

            return OptimizationResult<syrec::Statement>::fromOptimizedContainer(std::move(remainingTrueBranchStatements));
        }
    }

    auto simplifiedIfStatement     = std::make_unique<syrec::IfStatement>();
    simplifiedIfStatement->condition      = simplifiedGuardExpr;
    simplifiedIfStatement->fiCondition    = simplifiedGuardExpr;
    simplifiedIfStatement->thenStatements = !simplifiedTrueBranchStmtsContainer.has_value()
        ? createStatementListFrom(ifStatement.thenStatements, {})
        : createStatementListFrom({}, std::move(*simplifiedTrueBranchStmtsContainer));
        
    simplifiedIfStatement->elseStatements = !simplifiedFalseBranchStmtsContainer.has_value()
        ? createStatementListFrom(ifStatement.elseStatements, {})
        : createStatementListFrom({}, std::move(*simplifiedFalseBranchStmtsContainer));
    
    return OptimizationResult<syrec::Statement>::fromOptimizedContainer(std::move(simplifiedIfStatement));
}

optimizations::Optimizer::OptimizationResult<syrec::Statement> optimizations::Optimizer::handleLoopStmt(const syrec::ForStatement& forStatement) {
    std::unique_ptr<syrec::Number> iterationRangeStartSimplified;
    std::unique_ptr<syrec::Number> iterationRangeEndSimplified;
    std::unique_ptr<syrec::Number> iterationRangeStepSizeSimplified;

    std::optional<unsigned int> constantValueOfIterationRangeStart;
    std::optional<unsigned int> constantValueOfIterationRangeEnd;
    std::optional<unsigned int> constantValueOfIterationRangeStepsize;

    if (auto iterationRangeStartSimplificationResult = handleNumber(*forStatement.range.first); iterationRangeStartSimplificationResult.getStatusOfResult() != OptimizationResultFlag::IsUnchanged) {
        iterationRangeStartSimplified = std::move(iterationRangeStartSimplificationResult.tryTakeOwnershipOfOptimizationResult()->front());
        constantValueOfIterationRangeStart = tryEvaluateNumberAsConstant(*iterationRangeStartSimplified);
    } else {
        constantValueOfIterationRangeStart = tryEvaluateNumberAsConstant(*forStatement.range.first);
    }
    updateValueOfLoopVariable(forStatement.loopVariable, constantValueOfIterationRangeStart);

    const auto doIterationRangeStartAndEndContainerMatch = forStatement.range.first == forStatement.range.second;
    if (doIterationRangeStartAndEndContainerMatch) {
        constantValueOfIterationRangeStart = constantValueOfIterationRangeEnd;
    }
    else {
        if (auto iterationRangeEndSimplificationResult = handleNumber(*forStatement.range.second); iterationRangeEndSimplificationResult.getStatusOfResult() != OptimizationResultFlag::IsUnchanged) {
            iterationRangeEndSimplified      = std::move(iterationRangeEndSimplificationResult.tryTakeOwnershipOfOptimizationResult()->front());
            constantValueOfIterationRangeEnd = tryEvaluateNumberAsConstant(*iterationRangeEndSimplified);
        } else {
            constantValueOfIterationRangeEnd = tryEvaluateNumberAsConstant(*forStatement.range.second);
        }
    }

    if (auto iterationRangeStepSizeSimplificationResult = handleNumber(*forStatement.step); iterationRangeStepSizeSimplificationResult.getStatusOfResult() != OptimizationResultFlag::IsUnchanged) {
        iterationRangeStepSizeSimplified = std::move(iterationRangeStepSizeSimplificationResult.tryTakeOwnershipOfOptimizationResult()->front());
        constantValueOfIterationRangeStepsize = tryEvaluateNumberAsConstant(*iterationRangeStepSizeSimplified);
    } else {
        constantValueOfIterationRangeStepsize = tryEvaluateNumberAsConstant(*forStatement.step);
    }

    auto                        shouldValueOfLoopVariableBeAvailableInLoopBody = false;
    std::optional<unsigned int> numLoopIterations;
    if (constantValueOfIterationRangeStart.has_value() && constantValueOfIterationRangeEnd.has_value() && constantValueOfIterationRangeStepsize.has_value()) {
        numLoopIterations = utils::determineNumberOfLoopIterations(*constantValueOfIterationRangeStart, *constantValueOfIterationRangeEnd, *constantValueOfIterationRangeStepsize);
        shouldValueOfLoopVariableBeAvailableInLoopBody = numLoopIterations.has_value() && *numLoopIterations <= 1;
    }

    if (!shouldValueOfLoopVariableBeAvailableInLoopBody) {
        removeLoopVariableFromSymbolTable(forStatement.loopVariable);    
    }

    const auto& iterationRangeStartContainer    = !iterationRangeStartSimplified ? *forStatement.range.first : *iterationRangeStartSimplified;
    const auto& iterationRangeEndContainer      = !iterationRangeEndSimplified ? (iterationRangeStartSimplified ? *iterationRangeStartSimplified : *forStatement.range.second) : *iterationRangeEndSimplified;
    const auto& iterationRangeStepSizeContainer = !iterationRangeStepSizeSimplified ? *forStatement.step : *iterationRangeStepSizeSimplified;

    // TODO: Handling of loops with zero iterations when dead code elimination is disabled
    if (numLoopIterations.has_value() && !*numLoopIterations) {
        removeLoopVariableFromSymbolTable(forStatement.loopVariable);

        if (parserConfig.deadCodeEliminationEnabled) {
            decrementReferenceCountsOfLoopHeaderComponents(iterationRangeStartContainer, doIterationRangeStartAndEndContainerMatch ? std::nullopt : std::make_optional(iterationRangeEndContainer), iterationRangeStepSizeContainer);
            return OptimizationResult<syrec::Statement>::asOptimizedAwayContainer();
        }
        return OptimizationResult<syrec::Statement>::asUnchangedOriginal();
    }

    auto wereAnyAssignmentPerformInLoopBody = false;
    const auto shouldNewValuePropagationBlockerScopeOpened = numLoopIterations.value_or(2) > 1;

    // TODO: Should constant propagation be perform again here
    if (shouldNewValuePropagationBlockerScopeOpened) {
        /*
         * Are assignments that do not change result removed / not considered
         */
        activeDataFlowValuePropagationRestrictions->openNewScopeAndAppendDataDataFlowAnalysisResult(transformCollectionOfSharedPointersToReferences(forStatement.statements), &wereAnyAssignmentPerformInLoopBody);
    }
    
    // TODO: Correct setting of internal flags
    // TODO: General optimization of loop body
    std::optional<std::vector<std::unique_ptr<syrec::Statement>>> loopBodyStmtsSimplified;
    if (auto simplificationResultOfLoopBodyStmts = handleStatements(transformCollectionOfSharedPointersToReferences(forStatement.statements)); simplificationResultOfLoopBodyStmts.getStatusOfResult() != OptimizationResultFlag::IsUnchanged) {
        loopBodyStmtsSimplified = simplificationResultOfLoopBodyStmts.tryTakeOwnershipOfOptimizationResult();    
    }

    if (!wereAnyAssignmentPerformInLoopBody) {
        std::vector<std::reference_wrapper<syrec::Statement>> statementsToCheck;
        if (loopBodyStmtsSimplified.has_value()) {
            statementsToCheck.reserve(loopBodyStmtsSimplified->size());
            for (auto&& stmt : *loopBodyStmtsSimplified) {
                wereAnyAssignmentPerformInLoopBody |= doesStatementDefineAssignmentThatChangesAssignedToSignal(*stmt);
                if (wereAnyAssignmentPerformInLoopBody) {
                    break;
                }
            }
        }
        else {
            statementsToCheck.reserve(forStatement.statements.size());
            for (const auto& stmt : forStatement.statements) {
                wereAnyAssignmentPerformInLoopBody |= doesStatementDefineAssignmentThatChangesAssignedToSignal(*stmt);
                if (wereAnyAssignmentPerformInLoopBody) {
                    break;
                }
            }
        }
    }

    if (shouldValueOfLoopVariableBeAvailableInLoopBody) {
        removeLoopVariableFromSymbolTable(forStatement.loopVariable);        
    }

    if (shouldNewValuePropagationBlockerScopeOpened) {
        activeDataFlowValuePropagationRestrictions->closeScopeAndDiscardDataFlowAnalysisResult();
    }

    const auto& isLoopBodyAfterOptimizationsEmpty = (loopBodyStmtsSimplified.has_value() && loopBodyStmtsSimplified->empty()) || (loopBodyStmtsSimplified.has_value() && forStatement.statements.empty());
    if (!wereAnyAssignmentPerformInLoopBody || isLoopBodyAfterOptimizationsEmpty) {
        if (parserConfig.deadCodeEliminationEnabled) {
            return OptimizationResult<syrec::Statement>::asOptimizedAwayContainer();
        }
        if (isLoopBodyAfterOptimizationsEmpty) {
            std::vector<std::unique_ptr<syrec::SkipStatement>> containerForSingleSkipStmt;
            containerForSingleSkipStmt.reserve(1);
            containerForSingleSkipStmt.emplace_back( std::make_unique<syrec::SkipStatement>());
            loopBodyStmtsSimplified = std::move(containerForSingleSkipStmt);
        }
    }

    if (numLoopIterations.has_value() && *numLoopIterations == 1 && parserConfig.deadCodeEliminationEnabled) {
        decrementReferenceCountsOfLoopHeaderComponents(iterationRangeStartContainer, doIterationRangeStartAndEndContainerMatch ? std::nullopt : std::make_optional(iterationRangeEndContainer), iterationRangeStepSizeContainer);
        
        if (loopBodyStmtsSimplified.has_value()) {
            return OptimizationResult<syrec::Statement>::fromOptimizedContainer(std::move(*loopBodyStmtsSimplified));
        }

        std::vector<std::unique_ptr<syrec::Statement>> copyOfStatementsOfOriginalLoopBody;
        copyOfStatementsOfOriginalLoopBody.reserve(forStatement.statements.size());

        for (const auto& stmt: forStatement.statements) {
            copyOfStatementsOfOriginalLoopBody.emplace_back(createCopyOfStmt(*stmt));
        }
        return OptimizationResult<syrec::Statement>::fromOptimizedContainer(std::move(copyOfStatementsOfOriginalLoopBody));
    }

    const auto wasAnyComponentOfLoopModified = iterationRangeStartSimplified
        || iterationRangeEndSimplified
        || iterationRangeStepSizeSimplified
        || loopBodyStmtsSimplified.has_value();

    if (!wasAnyComponentOfLoopModified) {
        return OptimizationResult<syrec::Statement>::asUnchangedOriginal();
    }
    
    auto simplifiedLoopStmt = std::make_unique<syrec::ForStatement>();
    // TODO: If loop only performs one iteration and the dead code elimination is not enabled, one could replace the declaration of for $i = X to Y step Z with for $X to Y step Z
    simplifiedLoopStmt->loopVariable = forStatement.loopVariable;
    simplifiedLoopStmt->step         = iterationRangeStepSizeSimplified ? std::move(iterationRangeStepSizeSimplified) : createCopyOfNumber(*forStatement.step);
    simplifiedLoopStmt->range        = std::make_pair(
        iterationRangeStartSimplified ? std::move(iterationRangeStartSimplified) : createCopyOfNumber(*forStatement.range.first),
        iterationRangeEndSimplified ? std::move(iterationRangeEndSimplified) : createCopyOfNumber(*forStatement.range.second)
    );

    simplifiedLoopStmt->statements = !loopBodyStmtsSimplified.has_value()
        ? createStatementListFrom(forStatement.statements, {})
        : createStatementListFrom({}, std::move(*loopBodyStmtsSimplified));

    return OptimizationResult<syrec::Statement>::fromOptimizedContainer(std::move(simplifiedLoopStmt));
}

optimizations::Optimizer::OptimizationResult<syrec::Statement> optimizations::Optimizer::handleSkipStmt() {
    return OptimizationResult<syrec::Statement>::asUnchangedOriginal();
}

optimizations::Optimizer::OptimizationResult<syrec::Statement> optimizations::Optimizer::handleSwapStmt(const syrec::SwapStatement& swapStatement) const {
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

optimizations::Optimizer::OptimizationResult<syrec::Statement> optimizations::Optimizer::handleUnaryStmt(const syrec::UnaryStatement& unaryStmt) const {
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

optimizations::Optimizer::OptimizationResult<syrec::expression> optimizations::Optimizer::handleExpr(const syrec::expression& expression) const {
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
    return OptimizationResult<syrec::expression>::asUnchangedOriginal();
}

optimizations::Optimizer::OptimizationResult<syrec::expression> optimizations::Optimizer::handleBinaryExpr(const syrec::BinaryExpression& expression) const {
    std::unique_ptr<syrec::expression> simplifiedLhsExpr;
    std::unique_ptr<syrec::expression> simplifiedRhsExpr;
    auto                               wasOriginalExprModified = false;

    if (auto simplificationResultOfLhsExpr = handleExpr(*expression.lhs); simplificationResultOfLhsExpr.getStatusOfResult() != OptimizationResultFlag::IsUnchanged) {
        wasOriginalExprModified = true;
        simplifiedLhsExpr       = std::move(simplificationResultOfLhsExpr.tryTakeOwnershipOfOptimizationResult()->front());
    }

    if (auto simplificationResultOfRhsExpr = handleExpr(*expression.rhs); simplificationResultOfRhsExpr.getStatusOfResult() != OptimizationResultFlag::IsUnchanged) {
        wasOriginalExprModified = true;
        simplifiedRhsExpr       = std::move(simplificationResultOfRhsExpr.tryTakeOwnershipOfOptimizationResult()->front());
    }

    std::unique_ptr<syrec::expression> simplifiedBinaryExpr;
    const auto constantValueOfLhsExpr = tryEvaluateExpressionToConstant(simplifiedLhsExpr ? *simplifiedLhsExpr : *expression.lhs);
    const auto constantValueOfRhsExpr = tryEvaluateExpressionToConstant(simplifiedRhsExpr ? *simplifiedRhsExpr : *expression.rhs);
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
        syrec::expression::ptr lhsOperandOfSimplifiedExpr = simplifiedLhsExpr ? std::move(simplifiedLhsExpr) : expression.lhs;
        syrec::expression::ptr rhsOperandOfSimplifiedExpr = simplifiedRhsExpr ? std::move(simplifiedRhsExpr) : expression.rhs;
        simplifiedBinaryExpr                              = std::make_unique<syrec::BinaryExpression>(
                lhsOperandOfSimplifiedExpr,
                expression.op,
                rhsOperandOfSimplifiedExpr);
        wasOriginalExprModified = true;
    } else if (!simplifiedBinaryExpr) {
        simplifiedBinaryExpr = createCopyOfExpression(expression);
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
        ? OptimizationResult<syrec::expression>::fromOptimizedContainer(std::move(simplifiedBinaryExpr))
        : OptimizationResult<syrec::expression>::asUnchangedOriginal();
}

optimizations::Optimizer::OptimizationResult<syrec::expression> optimizations::Optimizer::handleNumericExpr(const syrec::NumericExpression& numericExpr) const {
    if (auto optimizationResultOfUnderlyingNumber = handleNumber(*numericExpr.value); optimizationResultOfUnderlyingNumber.getStatusOfResult() == OptimizationResultFlag::WasOptimized) {
        return OptimizationResult<syrec::expression>::fromOptimizedContainer(std::make_unique<syrec::NumericExpression>(std::move(optimizationResultOfUnderlyingNumber.tryTakeOwnershipOfOptimizationResult()->front()), numericExpr.bitwidth()));
    }
    return OptimizationResult<syrec::expression>::asUnchangedOriginal();
}

optimizations::Optimizer::OptimizationResult<syrec::expression> optimizations::Optimizer::handleVariableExpr(const syrec::VariableExpression& expression) const {
    std::optional<unsigned int> fetchedValueOfSignal;
    if (auto simplificationResultOfUserDefinedSignalAccess = handleSignalAccess(*expression.var, parserConfig.performConstantPropagation, &fetchedValueOfSignal); simplificationResultOfUserDefinedSignalAccess.getStatusOfResult() != OptimizationResultFlag::IsUnchanged) {
        if (fetchedValueOfSignal.has_value()) {
            return OptimizationResult<syrec::expression>::fromOptimizedContainer(std::make_unique<syrec::NumericExpression>(std::make_unique<syrec::Number>(*fetchedValueOfSignal), expression.bitwidth()));
        }
        if (simplificationResultOfUserDefinedSignalAccess.getStatusOfResult() == OptimizationResultFlag::WasOptimized) {
            return OptimizationResult<syrec::expression>::fromOptimizedContainer(std::make_unique<syrec::VariableExpression>(std::move(simplificationResultOfUserDefinedSignalAccess.tryTakeOwnershipOfOptimizationResult()->front())));
        }
    }
    return OptimizationResult<syrec::expression>::asUnchangedOriginal();
}

optimizations::Optimizer::OptimizationResult<syrec::expression> optimizations::Optimizer::handleShiftExpr(const syrec::ShiftExpression& expression) const {
    std::unique_ptr<syrec::expression> simplifiedToBeShiftedExpr;
    std::unique_ptr<syrec::Number>     simplifiedShiftAmount;

    if (auto simplificationResultOfToBeShiftedExpr = handleExpr(*expression.lhs); simplificationResultOfToBeShiftedExpr.getStatusOfResult() != OptimizationResultFlag::IsUnchanged) {
        simplifiedToBeShiftedExpr = std::move(simplificationResultOfToBeShiftedExpr.tryTakeOwnershipOfOptimizationResult()->front());
    }

    if (auto simplificationResultOfShiftAmount = handleNumber(*expression.rhs); simplificationResultOfShiftAmount.getStatusOfResult() != OptimizationResultFlag::IsUnchanged) {
        simplifiedShiftAmount = std::move(simplificationResultOfShiftAmount.tryTakeOwnershipOfOptimizationResult()->front());
    }

    if (const auto& mappedToShiftOperation = syrec_operation::tryMapShiftOperationFlagToEnum(expression.op); mappedToShiftOperation.has_value()) {
        std::unique_ptr<syrec::expression> simplificationResultOfExpr;
        const auto& constantValueOfToBeShiftedExpr = tryEvaluateExpressionToConstant(simplifiedToBeShiftedExpr ? *simplifiedToBeShiftedExpr : *expression.lhs);
        const auto& constantValueOfShiftAmount     = tryEvaluateNumberAsConstant(simplifiedShiftAmount ? *simplifiedShiftAmount : *expression.rhs);

        if (constantValueOfToBeShiftedExpr.has_value() && syrec_operation::isOperandUsedAsLhsInOperationIdentityElement(*mappedToShiftOperation, *constantValueOfToBeShiftedExpr)) {
            if (!simplifiedShiftAmount) {
                simplificationResultOfExpr = std::make_unique<syrec::NumericExpression>(expression.rhs, expression.bitwidth());
            }
            else {
                simplificationResultOfExpr = std::make_unique<syrec::NumericExpression>(std::move(simplifiedShiftAmount), expression.bitwidth());
            }
        } else if (constantValueOfShiftAmount.has_value() && syrec_operation::isOperandUseAsRhsInOperationIdentityElement(*mappedToShiftOperation, *constantValueOfShiftAmount)) {
            if (!simplifiedToBeShiftedExpr) {
                simplificationResultOfExpr = createCopyOfExpression(*expression.lhs);
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
            return OptimizationResult<syrec::expression>::fromOptimizedContainer(std::move(simplificationResultOfExpr));   
        }
    }

    if (!simplifiedToBeShiftedExpr && !simplifiedShiftAmount) {
        return OptimizationResult<syrec::expression>::asUnchangedOriginal();
    }

    syrec::expression::ptr simplifiedLhsOperand = simplifiedToBeShiftedExpr ? std::move(simplifiedToBeShiftedExpr) : expression.lhs;
    syrec::Number::ptr     simplifiedRhsOperand = simplifiedShiftAmount ? std::move(simplifiedShiftAmount) : expression.rhs;
    auto                   simplifiedShiftExpr  = std::make_unique<syrec::ShiftExpression>(
            simplifiedToBeShiftedExpr ? std::move(simplifiedToBeShiftedExpr) : expression.lhs,
            expression.op,
            simplifiedShiftAmount ? std::move(simplifiedShiftAmount) : expression.rhs);
    return OptimizationResult<syrec::expression>::fromOptimizedContainer(std::move(simplifiedShiftExpr));
}

optimizations::Optimizer::OptimizationResult<syrec::Number> optimizations::Optimizer::handleNumber(const syrec::Number& number) const {
    if (number.isConstant()) {
        return OptimizationResult<syrec::Number>::asUnchangedOriginal();
    }
    if (number.isLoopVariable()) {
        if (const auto& activeSymbolTableScope = getActiveSymbolTableScope(); activeSymbolTableScope.has_value() && parserConfig.performConstantPropagation) {
            if (const auto& fetchedValueOfLoopVariable = activeSymbolTableScope->get()->tryFetchValueOfLoopVariable(number.variableName()); fetchedValueOfLoopVariable.has_value()) {
                updateReferenceCountOf(number.variableName(), parser::SymbolTable::ReferenceCountUpdate::Decrement);
                return OptimizationResult<syrec::Number>::fromOptimizedContainer(std::make_unique<syrec::Number>(*fetchedValueOfLoopVariable));   
            }
        }

        updateReferenceCountOf(number.variableName(), parser::SymbolTable::ReferenceCountUpdate::Increment);
        return OptimizationResult<syrec::Number>::asUnchangedOriginal();
    }
    if (number.isCompileTimeConstantExpression()) {
        if (const auto& activeSymbolTableScope = getActiveSymbolTableScope(); activeSymbolTableScope.has_value()) {
            if (auto simplificationResult = trySimplifyNumber(number, **activeSymbolTableScope, parserConfig.reassociateExpressionsEnabled, parserConfig.operationStrengthReductionEnabled, nullptr); simplificationResult.has_value()) {
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

std::unique_ptr<syrec::Module> optimizations::Optimizer::createCopyOfModule(const syrec::Module& module) {
    auto copyOfModule        = std::make_unique<syrec::Module>(module.name);
    copyOfModule->parameters = module.parameters;
    copyOfModule->variables  = module.variables;
    copyOfModule->statements = module.statements;
    return copyOfModule;
}

std::vector<std::unique_ptr<syrec::Statement>> optimizations::Optimizer::createCopyOfStatements(const syrec::Statement::vec& statements) {
    std::vector<std::unique_ptr<syrec::Statement>> statementCopyContainer;
    statementCopyContainer.reserve(statements.size());

    for (const auto& stmt : statements) {
        statementCopyContainer.emplace_back(createCopyOfStmt(*stmt));
    }
    return statementCopyContainer;
}


std::unique_ptr<syrec::Statement> optimizations::Optimizer::createCopyOfStmt(const syrec::Statement& stmt) {
    if (typeid(stmt) == typeid(syrec::SkipStatement)) {
        return std::make_unique<syrec::SkipStatement>();
    }
    if (const auto& statementAsAssignmentStmt = dynamic_cast<const syrec::AssignStatement*>(&stmt); statementAsAssignmentStmt != nullptr) {
        return std::make_unique<syrec::AssignStatement>(statementAsAssignmentStmt->lhs, statementAsAssignmentStmt->op, statementAsAssignmentStmt->rhs);
    }
    if (const auto& statementAsUnaryAssignmentStmt = dynamic_cast<const syrec::UnaryStatement*>(&stmt); statementAsUnaryAssignmentStmt != nullptr) {
        return std::make_unique<syrec::UnaryStatement>(statementAsUnaryAssignmentStmt->op, statementAsUnaryAssignmentStmt->var);
    }
    if (const auto& statementAsIfStatement = dynamic_cast<const syrec::IfStatement*>(&stmt); statementAsIfStatement != nullptr) {
        auto copyOfIfStatement = std::make_unique<syrec::IfStatement>();
        copyOfIfStatement->condition = statementAsIfStatement->condition;
        copyOfIfStatement->fiCondition = statementAsIfStatement->fiCondition;
        copyOfIfStatement->thenStatements = statementAsIfStatement->thenStatements;
        copyOfIfStatement->elseStatements = statementAsIfStatement->elseStatements;
        return copyOfIfStatement;
    }
    if (const auto& statementAsLoopStatement = dynamic_cast<const syrec::ForStatement*>(&stmt); statementAsLoopStatement != nullptr) {
        auto copyOfLoopStatement = std::make_unique<syrec::ForStatement>();
        copyOfLoopStatement->loopVariable = statementAsLoopStatement->loopVariable;
        copyOfLoopStatement->range = statementAsLoopStatement->range;
        copyOfLoopStatement->step       = statementAsLoopStatement->step;
        copyOfLoopStatement->statements = statementAsLoopStatement->statements;
        return copyOfLoopStatement;
    }
    if (const auto& statementAsSwapStatement = dynamic_cast<const syrec::SwapStatement*>(&stmt); statementAsSwapStatement != nullptr) {
        return std::make_unique<syrec::SwapStatement>(statementAsSwapStatement->lhs, statementAsSwapStatement->rhs);
    }
    if (const auto& statementAsCallStatement = dynamic_cast<const syrec::CallStatement*>(&stmt); statementAsCallStatement != nullptr) {
        return std::make_unique<syrec::CallStatement>(statementAsCallStatement->target, statementAsCallStatement->parameters);    
    }
    return nullptr;
}

std::unique_ptr<syrec::expression> optimizations::Optimizer::createCopyOfExpression(const syrec::expression& expr) {
    if (const auto& exprAsBinaryExpr = dynamic_cast<const syrec::BinaryExpression*>(&expr); exprAsBinaryExpr != nullptr) {
        return std::make_unique<syrec::BinaryExpression>(
            exprAsBinaryExpr->lhs,
            exprAsBinaryExpr->op,
            exprAsBinaryExpr->rhs
        );
    }
    if (const auto& exprAsShiftExpr = dynamic_cast<const syrec::ShiftExpression*>(&expr); exprAsShiftExpr != nullptr) {
        return std::make_unique<syrec::ShiftExpression>(
            exprAsShiftExpr->lhs,
            exprAsShiftExpr->op,
            exprAsShiftExpr->rhs
        );
    }
    if (const auto& exprAsNumericExpr = dynamic_cast<const syrec::NumericExpression*>(&expr); exprAsNumericExpr != nullptr) {
        return std::make_unique<syrec::NumericExpression>(
                createCopyOfNumber(*exprAsNumericExpr->value),
                exprAsNumericExpr->bwidth
        );
    }
    if (const auto& exprAsVariableExpr = dynamic_cast<const syrec::VariableExpression*>(&expr); exprAsVariableExpr != nullptr) {
        return std::make_unique<syrec::VariableExpression>(exprAsVariableExpr->var);
    }
    return nullptr;
}

std::unique_ptr<syrec::Number> optimizations::Optimizer::createCopyOfNumber(const syrec::Number& number) {
    if (number.isConstant()) {
        return std::make_unique<syrec::Number>(number.evaluate({}));
    }
    if (number.isLoopVariable()) {
        return std::make_unique<syrec::Number>(number.variableName());
    }
    if (number.isCompileTimeConstantExpression()) {
        return std::make_unique<syrec::Number>(number.getExpression());
    }
    return nullptr;
}

std::unique_ptr<syrec::expression> optimizations::Optimizer::trySimplifyExpr(const syrec::expression& expr, const parser::SymbolTable& symbolTable, bool simplifyExprByReassociation, bool performOperationStrengthReduction, std::vector<syrec::VariableAccess::ptr>* optimizedAwaySignalAccesses) {
    std::unique_ptr<syrec::expression> simplificationResult;
    const std::shared_ptr<syrec::expression> toBeSimplifiedExpr = createCopyOfExpression(expr);
    if (simplifyExprByReassociation) {
        // TODO: Rework return value to either return null if no simplification took place or return std::optional or return additional boolean flag
        if (const auto& reassociateExpressionResult = optimizations::simplifyBinaryExpression(toBeSimplifiedExpr, performOperationStrengthReduction, std::nullopt, symbolTable, optimizedAwaySignalAccesses); reassociateExpressionResult != toBeSimplifiedExpr) {
            simplificationResult = createCopyOfExpression(*reassociateExpressionResult);
        }
    } else {
        if (const auto& generalBinaryExpressionSimplificationResult = optimizations::trySimplify(toBeSimplifiedExpr, performOperationStrengthReduction, symbolTable, optimizedAwaySignalAccesses); generalBinaryExpressionSimplificationResult.couldSimplify) {
            simplificationResult = createCopyOfExpression(*generalBinaryExpressionSimplificationResult.simplifiedExpression);
        }
    }
    return simplificationResult;
}

std::unique_ptr<syrec::expression> optimizations::Optimizer::transformCompileTimeConstantExpressionToNumber(const syrec::Number::CompileTimeConstantExpression& compileTimeConstantExpr) {
    if (const auto& mappedToExprOperation = tryMapCompileTimeConstantOperation(compileTimeConstantExpr.operation); mappedToExprOperation.has_value()) {
        auto lhsOperandAsExpr = tryMapCompileTimeConstantOperandToExpr(*compileTimeConstantExpr.lhsOperand);
        auto rhsOperandAsExpr = tryMapCompileTimeConstantOperandToExpr(*compileTimeConstantExpr.rhsOperand);
        if (lhsOperandAsExpr != nullptr && rhsOperandAsExpr != nullptr) {
            return std::make_unique<syrec::BinaryExpression>(std::move(lhsOperandAsExpr), *syrec_operation::tryMapBinaryOperationEnumToFlag(*mappedToExprOperation), std::move(rhsOperandAsExpr));
        }
    }
    return nullptr;
}

// TODO: bitwidth
std::unique_ptr<syrec::expression> optimizations::Optimizer::tryMapCompileTimeConstantOperandToExpr(const syrec::Number& number) {
    if (number.isConstant()) {
        return std::make_unique<syrec::NumericExpression>(std::make_unique<syrec::Number>(number.evaluate({})), BitHelpers::getRequiredBitsToStoreValue(number.evaluate({})));
    }
    if (number.isLoopVariable()) {
        return std::make_unique<syrec::NumericExpression>(std::make_unique<syrec::Number>(number.variableName()), 1);
    }
    if (number.isCompileTimeConstantExpression()) {
        return transformCompileTimeConstantExpressionToNumber(number.getExpression());
    }
    return nullptr;
}

std::unique_ptr<syrec::Number> optimizations::Optimizer::tryMapExprToCompileTimeConstantExpr(const syrec::expression& expr) {
    if (const auto& exprAsBinaryExpr = dynamic_cast<const syrec::BinaryExpression*>(&expr); exprAsBinaryExpr != nullptr) {
        if (const auto& mappedToBinaryOp = syrec_operation::tryMapBinaryOperationFlagToEnum(exprAsBinaryExpr->op); mappedToBinaryOp.has_value() && tryMapOperationToCompileTimeConstantOperation(*mappedToBinaryOp).has_value()) {
            auto lhsExprAsOperand = tryMapExprToCompileTimeConstantExpr(*exprAsBinaryExpr->lhs);
            auto rhsExprAsOperand = tryMapExprToCompileTimeConstantExpr(*exprAsBinaryExpr->rhs);
            if (lhsExprAsOperand != nullptr && rhsExprAsOperand != nullptr) {
                return std::make_unique<syrec::Number>(syrec::Number::CompileTimeConstantExpression(std::move(lhsExprAsOperand), *tryMapOperationToCompileTimeConstantOperation(*mappedToBinaryOp), std::move(rhsExprAsOperand)));       
            }
        }
    }
    if (const auto& exprAsNumericExpr = dynamic_cast<const syrec::NumericExpression*>(&expr); exprAsNumericExpr != nullptr) {
        return createCopyOfNumber(*exprAsNumericExpr->value);   
    }
    return nullptr;
}

std::optional<std::variant<unsigned, std::unique_ptr<syrec::Number>>> optimizations::Optimizer::trySimplifyNumber(const syrec::Number& number, const parser::SymbolTable& symbolTable, bool simplifyExprByReassociation, bool performOperationStrengthReduction, std::vector<syrec::VariableAccess::ptr>* optimizedAwaySignalAccesses) {
    if (number.isConstant()) {
        return std::make_optional(number.evaluate({}));
    }
    if (!number.isCompileTimeConstantExpression()) {
        return std::nullopt;
    }

    if (const auto& compileTimeExprAsBinaryExpr = createBinaryExprFromCompileTimeConstantExpr(number); compileTimeExprAsBinaryExpr != nullptr) {
        if (const auto& simplificationResultOfExpr = trySimplifyExpr(*compileTimeExprAsBinaryExpr, symbolTable, simplifyExprByReassociation, performOperationStrengthReduction, optimizedAwaySignalAccesses); simplificationResultOfExpr != nullptr) {
            if (auto simplifiedExprAsCompileTimeConstantExpr = tryMapExprToCompileTimeConstantExpr(*simplificationResultOfExpr); simplifiedExprAsCompileTimeConstantExpr != nullptr) {
                return simplifiedExprAsCompileTimeConstantExpr;
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
    if (const auto& activeSymbolTableScope = getActiveSymbolTableScope(); activeSymbolTableScope.has_value()) {
        activeSymbolTableScope->get()->updateReferenceCountOfLiteral(signalIdent, typeOfUpdate);
    }
}

std::optional<unsigned> optimizations::Optimizer::tryFetchValueForAccessedSignal(const syrec::VariableAccess& accessedSignal) const {
    if (!parserConfig.performConstantPropagation || isValueLookupBlockedByDataFlowAnalysisRestriction(accessedSignal)) {
        return std::nullopt;
    }

    if (auto evaluatedSignalAccess = tryEvaluateDefinedSignalAccess(accessedSignal); evaluatedSignalAccess.has_value()) {
        return tryFetchValueFromEvaluatedSignalAccess(*evaluatedSignalAccess);
    }
    return std::nullopt;
}

optimizations::Optimizer::DimensionAccessEvaluationResult optimizations::Optimizer::evaluateUserDefinedDimensionAccess(const std::string_view& accessedSignalIdent, const std::vector<std::reference_wrapper<const syrec::expression>>& accessedValuePerDimension) const {
    if (accessedValuePerDimension.empty()) {
        return DimensionAccessEvaluationResult({false, std::vector<SignalAccessIndexEvaluationResult<syrec::expression>>()});
    }

    const auto& signalData = *getSymbolTableEntryForVariable(accessedSignalIdent);
    const auto wasUserDefinedAccessTrimmed = accessedValuePerDimension.size() > signalData->dimensions.size();
    const auto numEntriesToTransform       = wasUserDefinedAccessTrimmed ? signalData->dimensions.size() : accessedValuePerDimension.size();
    
    std::vector<SignalAccessIndexEvaluationResult<syrec::expression>> evaluatedValuePerDimension;
    evaluatedValuePerDimension.reserve(numEntriesToTransform);

    for (std::size_t i = 0; i < numEntriesToTransform; ++i) {
        const auto& userDefinedValueOfDimension = accessedValuePerDimension.at(i);
        if (const auto& valueOfDimensionEvaluated   = tryEvaluateExpressionToConstant(userDefinedValueOfDimension); valueOfDimensionEvaluated.has_value()) {
            const auto isAccessedValueOfDimensionWithinRange = parser::isValidDimensionAccess(signalData, i, *valueOfDimensionEvaluated);
            evaluatedValuePerDimension.emplace_back(SignalAccessIndexEvaluationResult<syrec::expression>::createFromConstantValue(isAccessedValueOfDimensionWithinRange ? IndexValidityStatus::Valid : IndexValidityStatus::OutOfRange, *valueOfDimensionEvaluated));
        } else {
            evaluatedValuePerDimension.emplace_back(SignalAccessIndexEvaluationResult<syrec::expression>::createFromSimplifiedNonConstantValue(IndexValidityStatus::Unknown, createCopyOfExpression(userDefinedValueOfDimension)));
        }
    }
    return DimensionAccessEvaluationResult({wasUserDefinedAccessTrimmed, std::move(evaluatedValuePerDimension)});
}

std::optional<std::pair<unsigned, unsigned>> optimizations::Optimizer::tryEvaluateBitRangeAccessComponents(const std::pair<std::reference_wrapper<const syrec::Number>, std::reference_wrapper<const syrec::Number>>& accessedBitRange) const {
    const auto& bitRangeStartEvaluated = tryEvaluateNumberAsConstant(accessedBitRange.first);
    const auto& bitRangeEndEvaluated   = tryEvaluateNumberAsConstant(accessedBitRange.second);
    if (bitRangeStartEvaluated.has_value() && bitRangeEndEvaluated.has_value()) {
        return std::make_optional(std::make_pair(*bitRangeStartEvaluated, *bitRangeEndEvaluated));
    }
    return std::nullopt;
}

std::optional<unsigned> optimizations::Optimizer::tryEvaluateNumberAsConstant(const syrec::Number& number) const {
    if (number.isConstant()) {
        return std::make_optional(number.evaluate({}));
    }
    return std::nullopt;
}

std::optional<optimizations::Optimizer::BitRangeEvaluationResult> optimizations::Optimizer::evaluateUserDefinedBitRangeAccess(const std::string_view& accessedSignalIdent, const std::optional<std::pair<std::reference_wrapper<const syrec::Number>, std::reference_wrapper<const syrec::Number>>>& accessedBitRange) const {
    const auto& signalData = getSymbolTableEntryForVariable(accessedSignalIdent);
    if (!signalData.has_value() || !accessedBitRange.has_value()) {
        return std::nullopt;
    }
    
    const auto& bitRangeStartEvaluated = tryEvaluateNumberAsConstant(accessedBitRange->first);
    const auto& bitRangeEndEvaluated   = tryEvaluateNumberAsConstant(accessedBitRange->second);

    auto bitRangeStartEvaluationStatus = IndexValidityStatus::Unknown;
    if (bitRangeStartEvaluated.has_value()) {
        bitRangeStartEvaluationStatus = parser::isValidBitAccess(*signalData, *bitRangeStartEvaluated) ? IndexValidityStatus::Valid : IndexValidityStatus::OutOfRange;
    }
    auto bitRangeEndEvaluationStatus = IndexValidityStatus::Unknown;
    if (bitRangeEndEvaluated.has_value()) {
        bitRangeEndEvaluationStatus = parser::isValidBitAccess(*signalData, *bitRangeEndEvaluated) ? IndexValidityStatus::Valid : IndexValidityStatus::OutOfRange;
    }

    auto rangeStartEvaluationResult = bitRangeStartEvaluationStatus == IndexValidityStatus::Unknown
        ? SignalAccessIndexEvaluationResult<syrec::Number>::createFromSimplifiedNonConstantValue(bitRangeStartEvaluationStatus, createCopyOfNumber(accessedBitRange->first))
        : SignalAccessIndexEvaluationResult<syrec::Number>::createFromConstantValue(bitRangeStartEvaluationStatus, *bitRangeStartEvaluated);

    auto rangeEndEvaluationResult = bitRangeEndEvaluationStatus == IndexValidityStatus::Unknown
        ? SignalAccessIndexEvaluationResult<syrec::Number>::createFromSimplifiedNonConstantValue(bitRangeEndEvaluationStatus, createCopyOfNumber(accessedBitRange->second))
        : SignalAccessIndexEvaluationResult<syrec::Number>::createFromConstantValue(bitRangeEndEvaluationStatus, *bitRangeEndEvaluated);

    return std::make_optional(BitRangeEvaluationResult({.rangeStartEvaluationResult = std::move(rangeStartEvaluationResult), .rangeEndEvaluationResult = std::move(rangeEndEvaluationResult)}));
}

std::optional<unsigned> optimizations::Optimizer::tryEvaluateExpressionToConstant(const syrec::expression& expr) const {
    if (const auto& exprAsNumericExpr = dynamic_cast<const syrec::NumericExpression*>(&expr); exprAsNumericExpr != nullptr) {
        return tryEvaluateNumberAsConstant(*exprAsNumericExpr->value);
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

void optimizations::Optimizer::updateReferenceCountsOfSignalIdentsUsedIn(const syrec::expression& expr, parser::SymbolTable::ReferenceCountUpdate typeOfUpdate) const {
    SignalIdentCountLookup signalIdentCountLookup;
    determineUsedSignalIdentsIn(expr, signalIdentCountLookup);

    for (const auto& [signalIdent, occurrenceCount] : signalIdentCountLookup.lookup) {
        for (std::size_t i = 0; i < occurrenceCount; ++i) {
            updateReferenceCountOf(signalIdent, typeOfUpdate);    
        }
    }
}

void optimizations::Optimizer::createSymbolTableEntryForVariable(const syrec::Variable& variable) const {
    if (const auto& activeSymbolTableScope = getActiveSymbolTableScope(); activeSymbolTableScope.has_value() && !activeSymbolTableScope->get()->contains(variable.name)) {
        activeSymbolTableScope->get()->addEntry(variable);
    }   
}

void optimizations::Optimizer::createSymbolTableEntriesForModuleParametersAndLocalVariables(const syrec::Module& module) const {
    for (const auto& parameter : module.parameters) {
        createSymbolTableEntryForVariable(*parameter);
    }

    for (const auto& localVariable : module.variables) {
        createSymbolTableEntryForVariable(*localVariable);
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


// TODO:
std::unique_ptr<syrec::BinaryExpression> optimizations::Optimizer::createBinaryExprFromCompileTimeConstantExpr(const syrec::Number& number) {
    return nullptr;
}

void optimizations::Optimizer::determineUsedSignalIdentsIn(const syrec::expression& expr, SignalIdentCountLookup& lookupContainer) {
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

        std::vector<std::reference_wrapper<const syrec::expression>> accessedValueOfDimensionWithoutOwnership;
        accessedValueOfDimensionWithoutOwnership.reserve(accessedSignalParts.indexes.size());

        for (std::size_t i = 0; i < accessedSignalParts.indexes.size(); ++i) {
            accessedValueOfDimensionWithoutOwnership.emplace_back(*accessedSignalParts.indexes.at(i));
        }

        std::optional<std::pair<std::reference_wrapper<const syrec::Number>, std::reference_wrapper<const syrec::Number>>> accessedBitRangeWithoutOwnership;
        if (accessedSignalParts.range.has_value()) {
            accessedBitRangeWithoutOwnership = std::make_pair(*accessedSignalParts.range->first, *accessedSignalParts.range->second);
        }
        
        return std::make_optional(EvaluatedSignalAccess({accessedSignalParts.var->name,
                                                         evaluateUserDefinedDimensionAccess(accessedSignalIdent, accessedValueOfDimensionWithoutOwnership),
                                                         evaluateUserDefinedBitRangeAccess(accessedSignalIdent, accessedBitRangeWithoutOwnership), false}));
    }
    return std::nullopt;
}

std::unique_ptr<syrec::VariableAccess> optimizations::Optimizer::transformEvaluatedSignalAccess(const EvaluatedSignalAccess& evaluatedSignalAccess, std::optional<bool>* wasAnyDefinedIndexOutOfRange, bool* didAllDefinedIndicesEvaluateToConstants) const {
    if (const auto& matchingSymbolTableEntryForIdent = getSymbolTableEntryForVariable(evaluatedSignalAccess.accessedSignalIdent); matchingSymbolTableEntryForIdent.has_value()) {
        if (didAllDefinedIndicesEvaluateToConstants) {
            *didAllDefinedIndicesEvaluateToConstants = true;
        }

        std::optional<std::pair<syrec::Number::ptr, syrec::Number::ptr>> definedBitRangeAccess;
        std::vector<syrec::expression::ptr>                              definedDimensionAccess;
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
                bitRangeStart = createCopyOfNumber(tryFetchNonOwningReferenceOfNonConstantValueOfIndex(evaluatedSignalAccess.accessedBitRange->rangeStartEvaluationResult)->get());
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
                bitRangeEnd = createCopyOfNumber(tryFetchNonOwningReferenceOfNonConstantValueOfIndex(evaluatedSignalAccess.accessedBitRange->rangeEndEvaluationResult)->get());
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
                auto copyOfEvaluatedValueOfDimension = createCopyOfExpression(tryFetchNonOwningReferenceOfNonConstantValueOfIndex(evaluatedValueOfDimension)->get());
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
void optimizations::Optimizer::invalidateValueOfAccessedSignalParts(const EvaluatedSignalAccess& accessedSignalParts) const {
    if (auto transformedSignalAccess = transformEvaluatedSignalAccess(accessedSignalParts, nullptr, nullptr); transformedSignalAccess != nullptr) {
        const std::shared_ptr<syrec::VariableAccess> sharedTransformedSignalAccess = std::move(transformedSignalAccess);
        invalidateStoredValueFor(sharedTransformedSignalAccess);
    }
}

void optimizations::Optimizer::invalidateValueOfWholeSignal(const std::string_view& signalIdent) const {
    if (const auto& matchingSymbolTableEntry = getSymbolTableEntryForVariable(signalIdent); matchingSymbolTableEntry.has_value()) {
        const auto& sharedTransformedSignalAccess = std::make_shared<syrec::VariableAccess>();
        sharedTransformedSignalAccess->var        = *matchingSymbolTableEntry;
        invalidateStoredValueFor(sharedTransformedSignalAccess);
    }
}

/*
 * TODO: Removed additional pointer conversion when symbol table can be called with references instead of smart pointers
 */
void optimizations::Optimizer::performAssignment(const EvaluatedSignalAccess& assignedToSignalParts, syrec_operation::operation assignmentOperation, const std::optional<unsigned int>& assignmentRhsValue) const {
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
void optimizations::Optimizer::performAssignment(EvaluatedSignalAccess& assignmentLhsOperand, syrec_operation::operation assignmentOperation, EvaluatedSignalAccess& assignmentRhsOperand) const {
    if (const auto fetchedValueForAssignmentRhsOperand = tryFetchValueFromEvaluatedSignalAccess(assignmentRhsOperand); fetchedValueForAssignmentRhsOperand.has_value()) {
        performAssignment(assignmentLhsOperand, assignmentOperation, fetchedValueForAssignmentRhsOperand);
        updateReferenceCountOf(assignmentRhsOperand.accessedSignalIdent, parser::SymbolTable::ReferenceCountUpdate::Decrement);
        return;
    }
    auto                                         transformedLhsOperandSignalAccess       = transformEvaluatedSignalAccess(assignmentLhsOperand, nullptr, nullptr);
    const std::shared_ptr<syrec::VariableAccess> sharedTransformedLhsOperandSignalAccess = std::move(transformedLhsOperandSignalAccess);
    invalidateStoredValueFor(sharedTransformedLhsOperandSignalAccess);
}

void optimizations::Optimizer::performSwap(const EvaluatedSignalAccess& swapOperationLhsOperand, const EvaluatedSignalAccess& swapOperationRhsOperand) const {
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
    }

    if (sharedTransformedSwapLhsOperand) {
        invalidateStoredValueFor(sharedTransformedSwapLhsOperand);
    }
    if (sharedTransformedSwapRhsOperand) {
        invalidateStoredValueFor(sharedTransformedSwapRhsOperand);
    }
}

void optimizations::Optimizer::updateStoredValueOf(const syrec::VariableAccess::ptr& assignedToSignal, unsigned int newValueOfAssignedToSignal) const {
    if (const auto& activeSymbolTableScope = getActiveSymbolTableScope(); activeSymbolTableScope.has_value()) {
        createBackupOfAssignedToSignal(*assignedToSignal);
        activeSymbolTableScope->get()->updateStoredValueFor(assignedToSignal, newValueOfAssignedToSignal);   
    }
}

void optimizations::Optimizer::invalidateStoredValueFor(const syrec::VariableAccess::ptr& assignedToSignal) const {
    if (const auto& activeSymbolTableScope = getActiveSymbolTableScope(); activeSymbolTableScope.has_value()) {
        createBackupOfAssignedToSignal(*assignedToSignal);
        activeSymbolTableScope->get()->invalidateStoredValuesFor(assignedToSignal);
    }
}

void optimizations::Optimizer::performSwapAndCreateBackupOfOperands(const syrec::VariableAccess::ptr& swapLhsOperand, const syrec::VariableAccess::ptr& swapRhsOperand) const {
    if (const auto& activeSymbolTableScope = getActiveSymbolTableScope(); activeSymbolTableScope.has_value()) {
        createBackupOfAssignedToSignal(*swapLhsOperand);
        createBackupOfAssignedToSignal(*swapRhsOperand);
        activeSymbolTableScope->get()->swap(swapLhsOperand, swapRhsOperand);
    }
}

std::optional<unsigned> optimizations::Optimizer::tryFetchValueFromEvaluatedSignalAccess(const EvaluatedSignalAccess& accessedSignalParts) const {
    if (const auto& activeSymbolTableScope = getActiveSymbolTableScope(); activeSymbolTableScope.has_value()) {
        std::optional<bool> areAnyComponentsOfAssignedToSignalAccessOutOfRange;
        bool                didAllDefinedIndicesEvaluateToConstants;
        if (auto transformedSignalAccess = transformEvaluatedSignalAccess(accessedSignalParts, &areAnyComponentsOfAssignedToSignalAccessOutOfRange, &didAllDefinedIndicesEvaluateToConstants); transformedSignalAccess != nullptr) {
            if (!areAnyComponentsOfAssignedToSignalAccessOutOfRange.value_or(false) && didAllDefinedIndicesEvaluateToConstants && !isValueLookupBlockedByDataFlowAnalysisRestriction(*transformedSignalAccess)) {
                const std::shared_ptr<syrec::VariableAccess> sharedTransformedSignalAccess = std::move(transformedSignalAccess);
                return activeSymbolTableScope->get()->tryFetchValueForLiteral(sharedTransformedSignalAccess);
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

bool optimizations::Optimizer::checkIfOperandsOfBinaryExpressionAreSignalAccessesAndEqual(const syrec::expression& lhsOperand, const syrec::expression& rhsOperand) const {
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
                                                                                                                          [](const SignalAccessIndexEvaluationResult<syrec::expression>& valueOfDimensionEvaluationResult) {
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

            signalAccessUsedForValueLookup->range = std::make_pair(
                    simplifiedBitRangeStart.has_value() ? std::move(*simplifiedBitRangeStart) : signalAccess.range->first,
                    simplifiedBitRangeEnd.has_value() ? std::move(*simplifiedBitRangeEnd) : signalAccess.range->second);
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

std::optional<std::unique_ptr<syrec::expression>> optimizations::Optimizer::tryConvertSimplifiedIndexValueToUsableSignalAccessIndex(SignalAccessIndexEvaluationResult<syrec::expression>& index) {
    if (const auto& fetchedConstantValueForIndex = tryFetchConstantValueOfIndex(index); fetchedConstantValueForIndex.has_value()) {
        return std::make_unique<syrec::NumericExpression>(std::make_unique<syrec::Number>(*fetchedConstantValueForIndex), BitHelpers::getRequiredBitsToStoreValue(*fetchedConstantValueForIndex));
    }
    return tryFetchAndTakeOwnershipOfNonConstantValueOfIndex(index);
}

void optimizations::Optimizer::openNewSymbolTableBackupScope() {
    symbolTableBackupScopeStack.push(std::make_unique<parser::SymbolTableBackupHelper>());
}

void optimizations::Optimizer::updateBackupOfValuesChangedInScopeAndResetMadeChanges() const {
    const auto& peekedActiveSymbolTableBackupScope = peekCurrentSymbolTableBackupScope();
    if (!peekedActiveSymbolTableBackupScope.has_value() || peekedActiveSymbolTableBackupScope->get().getIdentsOfChangedSignals().empty()) {
        return;
    }

    if (const auto& activeSymbolTableScope = getActiveSymbolTableScope(); activeSymbolTableScope.has_value()) {
        for (auto& [signalIdent, valueAndRestrictionBackup]: symbolTableBackupScopeStack.top()->getBackedUpEntries()) {
            if (const auto& backupOfCurrentSignalValue = activeSymbolTableScope->get()->createBackupOfValueOfSignal(signalIdent); backupOfCurrentSignalValue.has_value()) {
                activeSymbolTableScope->get()->restoreValuesFromBackup(signalIdent, valueAndRestrictionBackup);
                valueAndRestrictionBackup->copyRestrictionsAndUnrestrictedValuesFrom(
                        {}, std::nullopt,
                        {}, std::nullopt,
                        **backupOfCurrentSignalValue);    
            }
        }
    }
}

std::optional<std::reference_wrapper<const parser::SymbolTableBackupHelper>> optimizations::Optimizer::peekCurrentSymbolTableBackupScope() const {
    if (symbolTableBackupScopeStack.empty()) {
        return std::nullopt;
    }
    return *symbolTableBackupScopeStack.top();
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
        }
    }

    for (const auto& signalRequiringMergeOfChanges: setOfSignalsRequiringMergeOfChanges) {
        const auto& initialValueOfChangedSignalPriorToAnyScope = *backupOfSignalsChangedInSecondBranch.getBackedUpEntryFor(signalRequiringMergeOfChanges);
        const auto& currentValueOfSignalAtEndOfFirstScope      = *backupOfSignalsChangedInFirstBranch.getBackedUpEntryFor(signalRequiringMergeOfChanges);
        const auto& currentValueOfSignalAtEndOfSecondScope     = activeSymbolTableScope->get()->createBackupOfValueOfSignal(signalRequiringMergeOfChanges);

        initialValueOfChangedSignalPriorToAnyScope->copyRestrictionsAndMergeValuesFromAlternatives(currentValueOfSignalAtEndOfFirstScope, *currentValueOfSignalAtEndOfSecondScope);
        activeSymbolTableScope->get()->restoreValuesFromBackup(signalRequiringMergeOfChanges, initialValueOfChangedSignalPriorToAnyScope);
    }

    /*
     * Our assumption is that the entries in the current symbol table scope have already all changes made in the second branch applied to them. Thus, we do not need to handle
     * the assignments made only in the second branch. But, since the assignments defined only in the first branch signals were discarded at the start of the second branch, due
     * to the requirement that the symbol table at the start of the second branch does not contain the changes from the first branch, we need to reapply the assignments local only to the first branch.
     * Overlapping assignments made in both branches were already merged at this point.
     *
     */
    for (const auto& signalChangedOnlyInFirstScope : setOfSignalsChangedOnlyInFirstScope) {
        const auto& currentValueOfSignalAtEndOfFirstScope = *backupOfSignalsChangedInFirstBranch.getBackedUpEntryFor(signalChangedOnlyInFirstScope);
        activeSymbolTableScope->get()->restoreValuesFromBackup(signalChangedOnlyInFirstScope, currentValueOfSignalAtEndOfFirstScope);
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
    if (symbolTableBackupScopeStack.empty()) {
        return;
    }
    symbolTableBackupScopeStack.pop();
}

void optimizations::Optimizer::createBackupOfAssignedToSignal(const syrec::VariableAccess& assignedToVariableAccess) const {
    const std::string_view& assignedToSignalIdent = assignedToVariableAccess.var->name;
    if (symbolTableBackupScopeStack.empty() || symbolTableBackupScopeStack.top()->hasEntryFor(assignedToSignalIdent)) {
        return;
    }

    if (const auto& activeSymbolTableScope = getActiveSymbolTableScope(); activeSymbolTableScope.has_value()) {
        if (const auto& backupOfAssignedToSignal = activeSymbolTableScope->get()->createBackupOfValueOfSignal(assignedToSignalIdent); backupOfAssignedToSignal.has_value()) {
            symbolTableBackupScopeStack.top()->storeBackupOf(std::string(assignedToSignalIdent), *backupOfAssignedToSignal);
        }        
    }
}

std::optional<std::unique_ptr<parser::SymbolTableBackupHelper>> optimizations::Optimizer::popCurrentSymbolTableBackupScope() {
    if (symbolTableBackupScopeStack.empty()) {
        return std::nullopt;
    }

    auto currentSymbolTableBackupScope = std::move(symbolTableBackupScopeStack.top());
    symbolTableBackupScopeStack.pop();
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

void optimizations::Optimizer::updateValueOfLoopVariable(const std::string_view& loopVariableIdent, std::optional<unsigned> value) const {
    if (loopVariableIdent.empty()) {
        return;
    }

    if (const auto& activeSymbolTableScope = getActiveSymbolTableScope(); activeSymbolTableScope.has_value()) {
        if (!activeSymbolTableScope->get()->contains(loopVariableIdent)) {
            activeSymbolTableScope->get()->addEntry(syrec::Number({std::string(loopVariableIdent)}), value.has_value() ? BitHelpers::getRequiredBitsToStoreValue(*value) : UINT_MAX, value);
        }
        else {
            if (!value.has_value()) {
                activeSymbolTableScope->get()->invalidateStoredValueForLoopVariable(loopVariableIdent);
            }
            else {
                activeSymbolTableScope->get()->updateStoredValueForLoopVariable(loopVariableIdent, *value);        
            }
        }
    }
}

void optimizations::Optimizer::removeLoopVariableFromSymbolTable(const std::string_view& loopVariableIdent) const {
    if (loopVariableIdent.empty()) {
        return;
    }

    if (const auto& activeSymbolTableScope = getActiveSymbolTableScope(); activeSymbolTableScope.has_value()) {
        activeSymbolTableScope->get()->removeVariable(std::string(loopVariableIdent));   
    }
}

bool optimizations::Optimizer::isAnyModifiableParameterOrLocalModifiedInModuleBody(const syrec::Module& module) const {
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
        return isAnyModifiableParameterOrLocalModifiedInStatement(*stmt, lookupOfModifiableParametersAndLocals);
    });
}

// TODO: Swap with lhs and rhs operand being equal does not count as a signal modification
bool optimizations::Optimizer::isAnyModifiableParameterOrLocalModifiedInStatement(const syrec::Statement& statement, const std::unordered_set<std::string>& parameterAndLocalLookup) const {
    if (const auto& stmtAsLoopStatement = dynamic_cast<const syrec::ForStatement*>(&statement); stmtAsLoopStatement != nullptr) {
        const auto& iterationRangeStartEvaluated = tryEvaluateNumberAsConstant(*stmtAsLoopStatement->range.first);
        const auto& iterationRangEndEvaluated    = tryEvaluateNumberAsConstant(*stmtAsLoopStatement->range.second);
        const auto& iterationRangeStepSize       = tryEvaluateNumberAsConstant(*stmtAsLoopStatement->step);
        if (iterationRangeStartEvaluated.has_value() && iterationRangEndEvaluated.has_value() && iterationRangeStepSize.has_value()) {
            if (const auto& numberOfIterations = utils::determineNumberOfLoopIterations(*iterationRangeStartEvaluated, *iterationRangEndEvaluated, *iterationRangeStepSize); numberOfIterations.has_value()) {
                return !*numberOfIterations;
            }
        }
        return isAnyModifiableParameterOrLocalModifiedInStatements(transformCollectionOfSharedPointersToReferences(stmtAsLoopStatement->statements), parameterAndLocalLookup);
    }
    if (const auto& stmtAsIfStmt = dynamic_cast<const syrec::IfStatement*>(&statement); stmtAsIfStmt != nullptr) {
        return isAnyModifiableParameterOrLocalModifiedInStatements(transformCollectionOfSharedPointersToReferences(stmtAsIfStmt->thenStatements), parameterAndLocalLookup)
            || isAnyModifiableParameterOrLocalModifiedInStatements(transformCollectionOfSharedPointersToReferences(stmtAsIfStmt->elseStatements), parameterAndLocalLookup);
    }
    if (const auto& stmtAsAssignmentStmt = dynamic_cast<const syrec::AssignStatement*>(&statement); stmtAsAssignmentStmt != nullptr) {
        if (const auto& assignmentRhsOperandAsConstant = tryEvaluateExpressionToConstant(*stmtAsAssignmentStmt->rhs); assignmentRhsOperandAsConstant.has_value() && syrec_operation::tryMapAssignmentOperationFlagToEnum(stmtAsAssignmentStmt->op).has_value()) {
            if (syrec_operation::isOperandUseAsRhsInOperationIdentityElement(*syrec_operation::tryMapAssignmentOperationFlagToEnum(stmtAsAssignmentStmt->op), *assignmentRhsOperandAsConstant)) {
                return false;
            }
        }
        return parameterAndLocalLookup.count(stmtAsAssignmentStmt->lhs->var->name) != 0;
    }
    if (const auto& stmtAsUnaryStmt = dynamic_cast<const syrec::UnaryStatement*>(&statement); stmtAsUnaryStmt != nullptr) {
        return parameterAndLocalLookup.count(stmtAsUnaryStmt->var->var->name) != 0;
    }
    if (const auto& stmtAsSwapStmt = dynamic_cast<const syrec::SwapStatement*>(&statement); stmtAsSwapStmt != nullptr) {
        return parameterAndLocalLookup.count(stmtAsSwapStmt->lhs->var->name) != 0 || parameterAndLocalLookup.count(stmtAsSwapStmt->rhs->var->name);
    }
    if (const auto& stmtAsCallStmt = dynamic_cast<const syrec::CallStatement*>(&statement); stmtAsCallStmt != nullptr) {
        if (const auto& activeSymbolTableScope = getActiveSymbolTableScope(); activeSymbolTableScope.has_value()) {
            if (const auto& matchingSymbolTableEntryForModule = activeSymbolTableScope->get()->tryGetOptimizedSignatureForModuleCall(stmtAsCallStmt->target->name, stmtAsCallStmt->parameters); matchingSymbolTableEntryForModule.has_value()) {
                std::unordered_set<std::size_t> indicesOfRemainingParameters;
                matchingSymbolTableEntryForModule->determineOptimizedCallSignature(&indicesOfRemainingParameters);
                return std::any_of(
                        indicesOfRemainingParameters.cbegin(),
                        indicesOfRemainingParameters.cend(),
                        [&parameterAndLocalLookup, &matchingSymbolTableEntryForModule, &stmtAsCallStmt](const std::size_t remainingParameterIndex) {
                            const auto& formalParameter      = matchingSymbolTableEntryForModule->declaredParameters.at(remainingParameterIndex);
                            const auto& actualParameterIdent = stmtAsCallStmt->parameters.at(remainingParameterIndex);
                            return isVariableReadOnly(*formalParameter) && parameterAndLocalLookup.count(actualParameterIdent) != 0;
                        });
            }    
        }
    }
    return false;
}

bool optimizations::Optimizer::isAnyModifiableParameterOrLocalModifiedInStatements(const std::vector<std::reference_wrapper<const syrec::Statement>>& statements, const std::unordered_set<std::string>& parameterAndLocalLookup) const {
    return std::any_of(statements.cbegin(), statements.cend(), [&](const syrec::Statement& statement) {
        return isAnyModifiableParameterOrLocalModifiedInStatement(statement, parameterAndLocalLookup);
    });
}

inline bool optimizations::Optimizer::isVariableReadOnly(const syrec::Variable& variable) {
    return variable.type == syrec::Variable::Types::In;
}

// TODO: 
std::optional<bool> optimizations::Optimizer::determineEquivalenceOfOperandsOfBinaryExpr(const syrec::BinaryExpression& binaryExpression) {
    if (const auto& definedBinaryOperation = syrec_operation::tryMapBinaryOperationFlagToEnum(binaryExpression.op);
        definedBinaryOperation.has_value() && (syrec_operation::isOperationRelationalOperation(*definedBinaryOperation) || syrec_operation::isOperationEquivalenceOperation(*definedBinaryOperation))) {
        return std::nullopt;
        /*switch (*definedBinaryOperation) {
            case syrec_operation::operation::GreaterEquals:
            case syrec_operation::operation::LessEquals:
            case syrec_operation::operation::Equals:
                return true;
            case syrec_operation::operation::GreaterThan:
            case syrec_operation::operation::LessThan:
            case syrec_operation::operation::NotEquals:
                return false;
            default:
                break;
        }*/
    }
    return std::nullopt;
}

bool optimizations::Optimizer::doesCurrentModuleIdentMatchUserDefinedMainModuleIdent(const std::string_view& currentModuleIdent, const std::optional<std::string>& userDefinedMainModuleIdent) {
    return userDefinedMainModuleIdent.value_or("") == currentModuleIdent;
}

bool optimizations::Optimizer::doesStatementDefineAssignmentThatChangesAssignedToSignal(const syrec::Statement& statement) {
    if (const auto& stmtAsLoopStatement = dynamic_cast<const syrec::ForStatement*>(&statement); stmtAsLoopStatement != nullptr) {
        return std::any_of(stmtAsLoopStatement->statements.cbegin(), stmtAsLoopStatement->statements.cend(), [&](const syrec::Statement::ptr& loopBodyStatement) {
            return doesStatementDefineAssignmentThatChangesAssignedToSignal(*loopBodyStatement);
        });
    }
    if (const auto& stmtAsIfStmt = dynamic_cast<const syrec::IfStatement*>(&statement); stmtAsIfStmt != nullptr) {
        return std::any_of(stmtAsIfStmt->thenStatements.cbegin(), stmtAsIfStmt->thenStatements.cend(), [&](const syrec::Statement::ptr& trueBranchStatement) {
            return doesStatementDefineAssignmentThatChangesAssignedToSignal(*trueBranchStatement);
        })
        || std::any_of(stmtAsIfStmt->elseStatements.cbegin(), stmtAsIfStmt->elseStatements.cend(), [&](const syrec::Statement::ptr& falseBranchStatement) {
            return doesStatementDefineAssignmentThatChangesAssignedToSignal(*falseBranchStatement);
        });
    }
    if (const auto& stmtAsAssignmentStmt = dynamic_cast<const syrec::AssignStatement*>(&statement); stmtAsAssignmentStmt != nullptr) {
        if (const auto& mappedToAssignmentOperationFromFlag = syrec_operation::tryMapAssignmentOperationFlagToEnum(stmtAsAssignmentStmt->op); mappedToAssignmentOperationFromFlag.has_value()) {
            if (const auto& rhsValueEvaluatedAsConstant = tryEvaluateExpressionToConstant(*stmtAsAssignmentStmt->rhs); rhsValueEvaluatedAsConstant.has_value()) {
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
            if (const auto& matchingSymbolTableEntryForModule = activeSymbolTableScope->get()->tryGetOptimizedSignatureForModuleCall(stmtAsCallStmt->target->name, stmtAsCallStmt->parameters); matchingSymbolTableEntryForModule.has_value()) {
                const auto& parametersOfOptimizedModuleSignature = matchingSymbolTableEntryForModule->determineOptimizedCallSignature(nullptr);
                return !std::any_of(parametersOfOptimizedModuleSignature.cbegin(), parametersOfOptimizedModuleSignature.cend(), [](const syrec::Variable::ptr& parameter) {
                    return isVariableReadOnly(*parameter);
                });
            }
        }
    }
    return false;
}
