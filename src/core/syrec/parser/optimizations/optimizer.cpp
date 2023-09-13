#include "core/syrec/parser/optimizations/optimizer.hpp"

#include "core/syrec/parser/module_call_guess.hpp"
#include "core/syrec/parser/operation.hpp"
#include "core/syrec/parser/range_check.hpp"
#include "core/syrec/parser/optimizations/reassociate_expression.hpp"
#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/main_additional_line_for_assignment_simplifier.hpp"
#include "core/syrec/parser/utils/binary_expression_simplifier.hpp"
#include "core/syrec/parser/utils/bit_helpers.hpp"

#include <unordered_set>

// TODO: Refactoring if case switches with visitor pattern as its available in std::visit call

optimizations::Optimizer::OptimizationResult<syrec::Module> optimizations::Optimizer::optimizeProgram(const std::vector<std::reference_wrapper<const syrec::Module>>& modules) {
    std::vector<std::unique_ptr<syrec::Module>> optimizedModules;
    bool                                        optimizedAnyModule = false;
    for (const auto& module: modules) {
        auto&& optimizedModule = handleModule(module);
        const auto optimizationResultStatus = optimizedModule.getStatusOfResult();
        optimizedAnyModule                  = optimizationResultStatus == OptimizationResultFlag::WasOptimized;

        if (optimizationResultStatus == OptimizationResultFlag::WasOptimized) {
            optimizedModules.emplace_back(std::move(optimizedModule.tryTakeOwnershipOfOptimizationResult()->front()));
        } else if (optimizedModule.getStatusOfResult() == OptimizationResultFlag::IsUnchanged) {
            optimizedModules.emplace_back(createCopyOfModule(module));
        }
    }

    if (optimizedAnyModule) {
        return !optimizedModules.empty() ? OptimizationResult<syrec::Module>::fromOptimizedContainer(std::move(optimizedModules)) : OptimizationResult<syrec::Module>::asOptimizedAwayContainer();
    }
    return OptimizationResult<syrec::Module>::asUnchangedOriginal();
}

optimizations::Optimizer::OptimizationResult<syrec::Module> optimizations::Optimizer::handleModule(const syrec::Module& module) {
    return OptimizationResult<syrec::Module>::asUnchangedOriginal();
}

optimizations::Optimizer::OptimizationResult<syrec::Statement> optimizations::Optimizer::handleStatements(const std::vector<std::reference_wrapper<const syrec::Statement>>& statements) {
    std::vector<std::unique_ptr<syrec::Statement>> optimizedStmts;
    bool optimizedAnyStmt = false;
    for (const auto& stmt: statements) {
        auto&& optimizedStatement = handleStatement(stmt);

        const auto& stmtOptimizationResultFlag = optimizedStatement.getStatusOfResult();
        optimizedAnyStmt                       = stmtOptimizationResultFlag == OptimizationResultFlag::WasOptimized;
        if (stmtOptimizationResultFlag == OptimizationResultFlag::WasOptimized) {
            if (auto optimizationResultOfStatement = optimizedStatement.tryTakeOwnershipOfOptimizationResult(); optimizationResultOfStatement.has_value()) {
                for (auto&& optimizedStmt : *optimizationResultOfStatement) {
                    optimizedStmts.emplace_back(std::move(optimizedStmt));
                }
            }
        } else if (stmtOptimizationResultFlag == OptimizationResultFlag::IsUnchanged) {
            optimizedStmts.emplace_back(createCopyOfStmt(stmt));
        }
    }

    if (optimizedAnyStmt) {
        return !optimizedStmts.empty() ? OptimizationResult<syrec::Statement>::fromOptimizedContainer(std::move(optimizedStmts)) : OptimizationResult<syrec::Statement>::asOptimizedAwayContainer();
    }
    return OptimizationResult<syrec::Statement>::asUnchangedOriginal();
}

optimizations::Optimizer::OptimizationResult<syrec::Statement> optimizations::Optimizer::handleStatement(const syrec::Statement& stmt) {
    std::optional<OptimizationResult<syrec::Statement>> optimizationResultContainerOfStmt;
    if (typeid(stmt) == typeid(syrec::SkipStatement)) {
        optimizationResultContainerOfStmt = handleSkipStmt(stmt);
    }
    else if (const auto& statementAsAssignmentStmt = dynamic_cast<const syrec::AssignStatement*>(&stmt); statementAsAssignmentStmt != nullptr) {
        optimizationResultContainerOfStmt = handleAssignmentStmt(*statementAsAssignmentStmt);
    }
    else if (const auto& statementAsUnaryAssignmentStmt = dynamic_cast<const syrec::UnaryStatement*>(&stmt); statementAsUnaryAssignmentStmt != nullptr) {
        optimizationResultContainerOfStmt = handleUnaryStmt(*statementAsUnaryAssignmentStmt);
    }
    else if (const auto& statementAsIfStatement = dynamic_cast<const syrec::IfStatement*>(&stmt); statementAsIfStatement != nullptr) {
        optimizationResultContainerOfStmt = handleIfStmt(*statementAsIfStatement);
    }
    else if (const auto& statementAsLoopStatement = dynamic_cast<const syrec::ForStatement*>(&stmt); statementAsLoopStatement != nullptr) {
        optimizationResultContainerOfStmt = handleLoopStmt(*statementAsLoopStatement);
    }
    else if (const auto& statementAsSwapStatement = dynamic_cast<const syrec::SwapStatement*>(&stmt); statementAsSwapStatement != nullptr) {
        optimizationResultContainerOfStmt = handleSwapStmt(*statementAsSwapStatement);
    }
    else if (const auto& statementAsCallStatement = dynamic_cast<const syrec::CallStatement*>(&stmt); statementAsCallStatement != nullptr) {
        optimizationResultContainerOfStmt = handleCallStmt(*statementAsCallStatement);
    }

    if (!optimizationResultContainerOfStmt.has_value()) {
        return OptimizationResult<syrec::Statement>::asUnchangedOriginal();
    }
    const auto& optimizationStatusFlag = optimizationResultContainerOfStmt->getStatusOfResult();
    if (optimizationStatusFlag == OptimizationResultFlag::RemovedByOptimization) {
        return OptimizationResult<syrec::Statement>::asOptimizedAwayContainer();
    }
    if (optimizationStatusFlag == OptimizationResultFlag::WasOptimized) {
        return OptimizationResult<syrec::Statement>::fromOptimizedContainer(std::move(*optimizationResultContainerOfStmt->tryTakeOwnershipOfOptimizationResult()));
    }
    return OptimizationResult<syrec::Statement>::asUnchangedOriginal();
}

optimizations::Optimizer::OptimizationResult<syrec::Statement> optimizations::Optimizer::handleAssignmentStmt(const syrec::AssignStatement& assignmentStmt) const {
    auto simplificationResultOfAssignedToSignal = handleSignalAccess(*assignmentStmt.lhs, false, nullptr);
    const auto& assignedToSignalPartsAfterSimplification = simplificationResultOfAssignedToSignal.getStatusOfResult() == OptimizationResultFlag::WasOptimized
        ? std::make_shared<syrec::VariableAccess>(*simplificationResultOfAssignedToSignal.tryTakeOwnershipOfOptimizationResult()->front())
        : assignmentStmt.lhs;

    auto simplificationResultOfRhsExpr = handleExpr(*assignmentStmt.rhs);
    syrec::expression::ptr rhsExprAfterSimplification    = assignmentStmt.rhs;
    if (simplificationResultOfRhsExpr.getStatusOfResult() == OptimizationResultFlag::WasOptimized) {
        rhsExprAfterSimplification = std::move(simplificationResultOfRhsExpr.tryTakeOwnershipOfOptimizationResult()->front());
    }

    const auto& rhsOperandEvaluatedAsConstant = tryEvaluateExpressionToConstant(*assignmentStmt.rhs);
    const auto& mappedToAssignmentOperationFromFlag = syrec_operation::tryMapAssignmentOperationFlagToEnum(assignmentStmt.op);

    if (mappedToAssignmentOperationFromFlag.has_value() && rhsOperandEvaluatedAsConstant.has_value() && syrec_operation::isOperandUseAsRhsInOperationIdentityElement(*mappedToAssignmentOperationFromFlag, *rhsOperandEvaluatedAsConstant)
        && parserConfig.deadCodeEliminationEnabled) {
        return OptimizationResult<syrec::Statement>::asOptimizedAwayContainer();
    }

    const auto skipCheckForSplitOfAssignmentInSubAssignments = !parserConfig.noAdditionalLineOptimizationEnabled || rhsOperandEvaluatedAsConstant.has_value();
    if (auto evaluatedAssignedToSignal = tryEvaluateDefinedSignalAccess(*assignedToSignalPartsAfterSimplification); evaluatedAssignedToSignal.has_value()) {
        performAssignment(*evaluatedAssignedToSignal, *mappedToAssignmentOperationFromFlag, *rhsOperandEvaluatedAsConstant);
    }

    if (!skipCheckForSplitOfAssignmentInSubAssignments) {
        const auto& noAdditionalLineAssignmentSimplifier = std::make_unique<noAdditionalLineSynthesis::MainAdditionalLineForAssignmentSimplifier>(symbolTable, nullptr, nullptr);
        const auto assignmentStmtToSimplify = assignedToSignalPartsAfterSimplification != assignmentStmt.lhs || rhsExprAfterSimplification != assignmentStmt.rhs
            ? std::make_unique<syrec::AssignStatement>(assignedToSignalPartsAfterSimplification, assignmentStmt.op, rhsExprAfterSimplification)
            : std::make_shared<syrec::AssignStatement>(assignmentStmt);

        const auto isValueOfAssignedToSignalBlockedPriorToAssignmentByDataFlowAnalysis = false;
        if (auto generatedSimplifierAssignments = noAdditionalLineAssignmentSimplifier->tryReduceRequiredAdditionalLinesFor(assignmentStmtToSimplify, isValueOfAssignedToSignalBlockedPriorToAssignmentByDataFlowAnalysis); !generatedSimplifierAssignments.empty()) {
            filterAssignmentsThatDoNotChangeAssignedToSignal(generatedSimplifierAssignments);
            if (generatedSimplifierAssignments.empty()) {
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

    if (assignedToSignalPartsAfterSimplification != assignmentStmt.lhs || rhsExprAfterSimplification != assignmentStmt.rhs) {
        return OptimizationResult<syrec::Statement>::fromOptimizedContainer(std::make_unique<syrec::AssignStatement>(assignedToSignalPartsAfterSimplification, assignmentStmt.op, rhsExprAfterSimplification));
    }
    return OptimizationResult<syrec::Statement>::asUnchangedOriginal();
}

// TODO: Should the semantic checks that we assume to be already done in the parser be repeated again, i.e. does a matching module exist, etc.
optimizations::Optimizer::OptimizationResult<syrec::Statement> optimizations::Optimizer::handleCallStmt(const syrec::CallStatement& callStatement) const {
    if (const auto moduleCallGuessResolver = parser::ModuleCallGuess::tryInitializeWithModulesMatchingName(symbolTable, callStatement.target->name); moduleCallGuessResolver.has_value()) {
        const auto moduleIdsOfInitialGuess = moduleCallGuessResolver->get()->getInternalIdsOfModulesMatchingGuess();

        bool                              calleeArgumentsOk = true;
        std::vector<syrec::Variable::ptr> symbolTableEntryForCalleeArguments(callStatement.parameters.size());
        for (std::size_t i = 0; i < symbolTableEntryForCalleeArguments.size(); ++i) {
            if (const auto& symbolTableEntryForCalleeArgument = symbolTable->getVariable(callStatement.parameters.at(i)); symbolTableEntryForCalleeArgument.has_value() && std::holds_alternative<syrec::Variable::ptr>(*symbolTableEntryForCalleeArgument)) {
                const auto matchingSignalForCalleeArgument = std::get<syrec::Variable::ptr>(*symbolTableEntryForCalleeArgument);
                symbolTableEntryForCalleeArguments.emplace_back(matchingSignalForCalleeArgument);
                moduleCallGuessResolver->get()->refineGuessWithNextParameter(*matchingSignalForCalleeArgument);
                calleeArgumentsOk &= true;
            }
        }

        if (calleeArgumentsOk) {
            /*
             * If the called module is not resolved already by the provided call statement, we also need to take special care when more than one module matches the given signature.
             * TODO: Invalidation of provided modifiable caller parameters given the user provided callee arguments
             */
            const auto modulesMatchingCallSignature = moduleCallGuessResolver->get()->getMatchesForGuess();
            if (modulesMatchingCallSignature.empty() || modulesMatchingCallSignature.size() > 1) {
                for (const auto& calleeArgument : symbolTableEntryForCalleeArguments) {
                    invalidateValueOfWholeSignal(calleeArgument->name);
                }
                symbolTable->updateReferenceCountOfModulesMatchingSignature(callStatement.target->name, moduleCallGuessResolver->get()->getInternalIdsOfModulesMatchingGuess(), parser::SymbolTable::ReferenceCountUpdate::Increment);
            } else {
                const auto setOfOptimizedModuleParameterLookup = std::set<std::size_t>(modulesMatchingCallSignature.front().indicesOfOptimizedAwayParameters.begin(), modulesMatchingCallSignature.front().indicesOfOptimizedAwayParameters.end());
                const auto& signatureOfCalledModuleInSymbolTable = modulesMatchingCallSignature.front();

                for (std::size_t i = 0; i < signatureOfCalledModuleInSymbolTable.declaredParameters.size(); i++) {
                    if (setOfOptimizedModuleParameterLookup.count(i) != 0) {
                        continue;   
                    }

                    const auto& signalTypeOfCallerParameter = signatureOfCalledModuleInSymbolTable.declaredParameters.at(i)->type;
                    if (signalTypeOfCallerParameter == syrec::Variable::Types::In) {
                        continue;
                    }
                    invalidateValueOfWholeSignal(symbolTableEntryForCalleeArguments.at(i)->name);
                }
                symbolTable->updateReferenceCountOfModulesMatchingSignature(callStatement.target->name, {signatureOfCalledModuleInSymbolTable.internalModuleId}, parser::SymbolTable::ReferenceCountUpdate::Increment);
            }

        } else {
            /*
             * We are assuming that the all defined callee arguments refer to previously declared signals, thus this branch should not be relevant but could be kept as a fail-safe in case our assumed precondition does not hold.
             * Additionally, on which guess basis should our update start from? The only choice would be the initial one since an invalid callee argument is not supported by the module call guess resolver (but could be supported in the future)
             * and thus our guesses are not further refined (based only on the data of the valid callee arguments and the number of actually defined parameters).
             * TODO: Check whether this branch is required
             */
            symbolTable->updateReferenceCountOfModulesMatchingSignature(callStatement.target->name, moduleCallGuessResolver->get()->getInternalIdsOfModulesMatchingGuess(), parser::SymbolTable::ReferenceCountUpdate::Increment);
        }

        /*
         * In case that our precondition does not hold, invalid callee arguments will not produce an increment of the reference count of all declared signals in all reachable scopes of the symbol table.
         * TODO: Is this assumption OK?
         */
        for (const auto& calleeArgument: callStatement.parameters) {
            symbolTable->updateReferenceCountOfLiteral(calleeArgument, parser::SymbolTable::ReferenceCountUpdate::Increment);
        }
    }
    return OptimizationResult<syrec::Statement>::asUnchangedOriginal();
}

optimizations::Optimizer::OptimizationResult<syrec::Statement> optimizations::Optimizer::handleIfStmt(const syrec::IfStatement& ifStatement) {
    syrec::expression::ptr simplifiedGuardExpr = ifStatement.condition;
    if (auto simplificationResultOfGuardExpr = handleExpr(*ifStatement.condition); simplificationResultOfGuardExpr.getStatusOfResult() == OptimizationResultFlag::WasOptimized) {
        simplifiedGuardExpr = std::move(simplificationResultOfGuardExpr.tryTakeOwnershipOfOptimizationResult()->front());
    }
    
    auto                                 canTrueBranchBeOmitted  = false;
    auto                                 canFalseBranchBeOmitted = false;
    auto                                 wereAnyStmtsInTrueBranchModified = false;
    auto                                 wereAnyStmtsInFalseBranchModified = false;

    if (parserConfig.deadCodeEliminationEnabled) {
        if (const auto& constantValueOfGuardCondition = tryEvaluateExpressionToConstant(*simplifiedGuardExpr); constantValueOfGuardCondition.has_value()) {
            canTrueBranchBeOmitted = *constantValueOfGuardCondition == 0;
            canFalseBranchBeOmitted = *constantValueOfGuardCondition;
        }
    }

    std::optional<std::vector<std::unique_ptr<syrec::Statement>>> simplifiedTrueBranchStmtsContainer;
    if (!canTrueBranchBeOmitted) {
        openNewSymbolTableBackupScope();
        auto simplificationResultOfTrueBranchStmts        = handleStatements(transformCollectionOfSharedPointersToReferences(ifStatement.thenStatements));
        simplifiedTrueBranchStmtsContainer            = simplificationResultOfTrueBranchStmts.tryTakeOwnershipOfOptimizationResult();
        const auto  isTrueBranchEmptyAfterOptimization    = (simplificationResultOfTrueBranchStmts.getStatusOfResult() == OptimizationResultFlag::WasOptimized && simplifiedTrueBranchStmtsContainer.has_value() && simplifiedTrueBranchStmtsContainer->empty())
            || simplificationResultOfTrueBranchStmts.getStatusOfResult() == OptimizationResultFlag::RemovedByOptimization;
        wereAnyStmtsInTrueBranchModified                 = simplificationResultOfTrueBranchStmts.getStatusOfResult() == OptimizationResultFlag::IsUnchanged;

        if (isTrueBranchEmptyAfterOptimization) {
            simplifiedTrueBranchStmtsContainer.reset();
            if (!canFalseBranchBeOmitted) {
                simplifiedTrueBranchStmtsContainer->emplace_back(std::make_unique<syrec::SkipStatement>());
            }
            discardChangesMadeInCurrentSymbolTableBackupScope();
            destroySymbolTableBackupScope();
        }
    }

    std::optional<std::vector<std::unique_ptr<syrec::Statement>>> simplifiedFalseBranchStmtsContainer;
    if (!canFalseBranchBeOmitted) {
        openNewSymbolTableBackupScope();
        auto        simplificationResultOfFalseBranchStmts = handleStatements(transformCollectionOfSharedPointersToReferences(ifStatement.elseStatements));
        simplifiedFalseBranchStmtsContainer    = simplificationResultOfFalseBranchStmts.tryTakeOwnershipOfOptimizationResult();
        const auto isFalseBranchEmptyAfterOptimization     = (simplificationResultOfFalseBranchStmts.getStatusOfResult() == OptimizationResultFlag::WasOptimized && simplifiedFalseBranchStmtsContainer.has_value() && simplifiedFalseBranchStmtsContainer->empty())
            || simplificationResultOfFalseBranchStmts.getStatusOfResult() == OptimizationResultFlag::RemovedByOptimization;
        wereAnyStmtsInFalseBranchModified                  = simplificationResultOfFalseBranchStmts.getStatusOfResult() == OptimizationResultFlag::IsUnchanged;

        if (isFalseBranchEmptyAfterOptimization) {
            simplifiedFalseBranchStmtsContainer.reset();
            if (!canTrueBranchBeOmitted) {
                simplifiedFalseBranchStmtsContainer->emplace_back(std::make_unique<syrec::SkipStatement>());
            }
            discardChangesMadeInCurrentSymbolTableBackupScope();
            destroySymbolTableBackupScope();
        }
    }

    if (!canTrueBranchBeOmitted && !canFalseBranchBeOmitted) {
        const auto backupScopeOfFalseBranch = popCurrentSymbolTableBackupScope();
        const auto backupScopeOfTrueBranch  = popCurrentSymbolTableBackupScope();

        if (backupScopeOfFalseBranch.has_value() && backupScopeOfTrueBranch.has_value()) {
            mergeAndMakeLocalChangesGlobal(**backupScopeOfTrueBranch, **backupScopeOfFalseBranch);   
        }
    }

    if (!wereAnyStmtsInTrueBranchModified && !wereAnyStmtsInFalseBranchModified) {
        updateReferenceCountsOfSignalIdentsUsedIn(*simplifiedGuardExpr, parser::SymbolTable::ReferenceCountUpdate::Increment);
        return OptimizationResult<syrec::Statement>::asUnchangedOriginal();        
    }
    if (!simplifiedTrueBranchStmtsContainer.has_value() && !simplifiedFalseBranchStmtsContainer.has_value()) {
        return OptimizationResult<syrec::Statement>::asOptimizedAwayContainer();
    }
    if (canTrueBranchBeOmitted || canFalseBranchBeOmitted) {
        return OptimizationResult<syrec::Statement>::fromOptimizedContainer(std::move(canTrueBranchBeOmitted ? *simplifiedFalseBranchStmtsContainer : *simplifiedTrueBranchStmtsContainer));
    }

    auto simplifiedIfStatement     = std::make_unique<syrec::IfStatement>();
    simplifiedIfStatement->condition      = simplifiedGuardExpr;
    simplifiedIfStatement->fiCondition    = simplifiedGuardExpr;
    simplifiedIfStatement->thenStatements.reserve(simplifiedTrueBranchStmtsContainer->size());
    for (auto& simplifiedTrueBranchSmt : *simplifiedTrueBranchStmtsContainer) {
        simplifiedIfStatement->thenStatements.emplace_back(std::move(simplifiedTrueBranchSmt));
    }

    simplifiedIfStatement->elseStatements.reserve(simplifiedFalseBranchStmtsContainer->size());
    for (auto& simplifiedFalseBranchSmt: *simplifiedFalseBranchStmtsContainer) {
        simplifiedIfStatement->thenStatements.emplace_back(std::move(simplifiedFalseBranchSmt));
    }

    updateReferenceCountsOfSignalIdentsUsedIn(*simplifiedGuardExpr, parser::SymbolTable::ReferenceCountUpdate::Increment);
    return OptimizationResult<syrec::Statement>::fromOptimizedContainer(std::move(simplifiedIfStatement));
}

optimizations::Optimizer::OptimizationResult<syrec::Statement> optimizations::Optimizer::handleLoopStmt(const syrec::ForStatement& forStatement) {
    return OptimizationResult<syrec::Statement>::asUnchangedOriginal();
}

optimizations::Optimizer::OptimizationResult<syrec::Statement> optimizations::Optimizer::handleSkipStmt(const syrec::SkipStatement& skipStatement) {
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
        updateReferenceCountOf(evaluatedSwapOperationLhsSignalAccess->accessedSignalIdent, parser::SymbolTable::ReferenceCountUpdate::Increment);
        updateReferenceCountOf(evaluatedSwapOperationRhsSignalAccess->accessedSignalIdent, parser::SymbolTable::ReferenceCountUpdate::Increment);
        performSwap(*evaluatedSwapOperationLhsSignalAccess, *evaluatedSwapOperationRhsSignalAccess);
    } else {
        if (evaluatedSwapOperationLhsSignalAccess.has_value()) {
            updateReferenceCountOf(evaluatedSwapOperationLhsSignalAccess->accessedSignalIdent, parser::SymbolTable::ReferenceCountUpdate::Increment);
            invalidateValueOfAccessedSignalParts(*evaluatedSwapOperationLhsSignalAccess);
        }
        if (evaluatedSwapOperationRhsSignalAccess.has_value()) {
            updateReferenceCountOf(evaluatedSwapOperationRhsSignalAccess->accessedSignalIdent, parser::SymbolTable::ReferenceCountUpdate::Increment);
            invalidateValueOfAccessedSignalParts(*evaluatedSwapOperationRhsSignalAccess);
        }        
    }

    if (lhsOperandAfterSimplification != swapStatement.lhs || rhsOperandAfterSimplification != swapStatement.rhs) {
        return OptimizationResult<syrec::Statement>::fromOptimizedContainer(std::make_unique<syrec::SwapStatement>(lhsOperandAfterSimplification, rhsOperandAfterSimplification));
    }
    return OptimizationResult<syrec::Statement>::asUnchangedOriginal();
}

optimizations::Optimizer::OptimizationResult<syrec::Statement> optimizations::Optimizer::handleUnaryStmt(const syrec::UnaryStatement& unaryStmt) const {
    auto simplificationResultOfUnaryOperandSignalAccess = handleSignalAccess(*unaryStmt.var, false, nullptr);
    syrec::VariableAccess::ptr signalAccessOperand                            = unaryStmt.var;
    if (simplificationResultOfUnaryOperandSignalAccess.getStatusOfResult() == OptimizationResultFlag::WasOptimized) {
        signalAccessOperand = std::move(simplificationResultOfUnaryOperandSignalAccess.tryTakeOwnershipOfOptimizationResult()->front());
    }

    if (auto evaluatedUnaryOperandSignalAccess = tryEvaluateDefinedSignalAccess(*signalAccessOperand); evaluatedUnaryOperandSignalAccess.has_value()) {
        updateReferenceCountOf(evaluatedUnaryOperandSignalAccess->accessedSignalIdent, parser::SymbolTable::ReferenceCountUpdate::Increment);
        const auto& fetchedValueForAssignedToSignal = tryFetchValueFromEvaluatedSignalAccess(*evaluatedUnaryOperandSignalAccess);
        const auto& mappedToUnaryOperationFromFlag  = syrec_operation::tryMapUnaryAssignmentOperationFlagToEnum(unaryStmt.op);

        if (fetchedValueForAssignedToSignal.has_value() && mappedToUnaryOperationFromFlag.has_value()) {
            performAssignment(*evaluatedUnaryOperandSignalAccess, *mappedToUnaryOperationFromFlag, 1);
        } else {
            invalidateValueOfAccessedSignalParts(*evaluatedUnaryOperandSignalAccess);
        }
    }

    if (simplificationResultOfUnaryOperandSignalAccess.getStatusOfResult() == OptimizationResultFlag::WasOptimized) {
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
    auto simplificationResultOfLhsExpr = handleExpr(*expression.lhs);
    auto simplificationResultOfRhsExpr = handleExpr(*expression.rhs);
    auto wasOriginalExprModified       = simplificationResultOfLhsExpr.getStatusOfResult() != OptimizationResultFlag::IsUnchanged || simplificationResultOfRhsExpr.getStatusOfResult() != OptimizationResultFlag::IsUnchanged;

    auto simplifiedLhsExpr = simplificationResultOfLhsExpr.getStatusOfResult() == OptimizationResultFlag::WasOptimized
        ? std::move(simplificationResultOfLhsExpr.tryTakeOwnershipOfOptimizationResult()->front())
        : createCopyOfExpression(*expression.lhs);

    auto simplifiedRhsExpr = simplificationResultOfRhsExpr.getStatusOfResult() == OptimizationResultFlag::WasOptimized
        ? std::move(simplificationResultOfRhsExpr.tryTakeOwnershipOfOptimizationResult()->front())
        : createCopyOfExpression(*expression.rhs);

    std::unique_ptr<syrec::expression> simplifiedBinaryExpr;
    const auto& evaluatedSimplifiedLhsExpr = tryEvaluateExpressionToConstant(*simplifiedLhsExpr);
    const auto& evaluatedSimplifiedRhsExpr = tryEvaluateExpressionToConstant(*simplifiedRhsExpr);

    const auto mappedToBinaryOperation = syrec_operation::tryMapBinaryOperationFlagToEnum(expression.op);
    if (mappedToBinaryOperation.has_value()) {
        if (evaluatedSimplifiedLhsExpr.has_value() && syrec_operation::isOperandUsedAsLhsInOperationIdentityElement(*mappedToBinaryOperation, *evaluatedSimplifiedLhsExpr)) {
            simplifiedBinaryExpr =  std::move(simplifiedRhsExpr);
            wasOriginalExprModified = true;
        }
        if (evaluatedSimplifiedRhsExpr.has_value() && syrec_operation::isOperandUseAsRhsInOperationIdentityElement(*mappedToBinaryOperation, *evaluatedSimplifiedRhsExpr)) {
            simplifiedBinaryExpr = std::move(simplifiedLhsExpr);
            wasOriginalExprModified = true;
        }
        if (evaluatedSimplifiedLhsExpr.has_value() && evaluatedSimplifiedRhsExpr.has_value()) {
            if (const auto evaluationResultOfBinaryOperation = syrec_operation::apply(*mappedToBinaryOperation, *evaluatedSimplifiedLhsExpr, *evaluatedSimplifiedRhsExpr); evaluationResultOfBinaryOperation.has_value()) {
                simplifiedBinaryExpr = std::make_unique<syrec::NumericExpression>(std::make_unique<syrec::Number>(*evaluationResultOfBinaryOperation), expression.bitwidth());
                wasOriginalExprModified = true;
            }
        }
    }

    if (!simplifiedBinaryExpr) {
        simplifiedBinaryExpr = std::make_unique<syrec::BinaryExpression>(std::move(simplifiedLhsExpr), expression.op, std::move(simplifiedRhsExpr));   
    }

    std::vector<syrec::VariableAccess::ptr> optimizedAwaySignalAccesses;
    if (auto simplificationResultOfExpr = trySimplifyExpr(*simplifiedBinaryExpr, *symbolTable, parserConfig.reassociateExpressionsEnabled, parserConfig.operationStrengthReductionEnabled, &optimizedAwaySignalAccesses); simplificationResultOfExpr != nullptr) {
        simplifiedBinaryExpr = std::move(simplificationResultOfExpr);
        wasOriginalExprModified = true;
    }

    for (const auto& optimizedAwaySignalAccess : optimizedAwaySignalAccesses) {
        updateReferenceCountOf(optimizedAwaySignalAccess->var->name, parser::SymbolTable::ReferenceCountUpdate::Decrement);
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
    auto simplificationResultOfToBeShiftedExpr = handleExpr(*expression.lhs);
    auto simplificationResultOfShiftAmount     = handleNumber(*expression.rhs);

    auto simplifiedToBeShiftedExpr = simplificationResultOfToBeShiftedExpr.getStatusOfResult() == OptimizationResultFlag::WasOptimized
        ? std::move(simplificationResultOfToBeShiftedExpr.tryTakeOwnershipOfOptimizationResult()->front())
        : createCopyOfExpression(*expression.lhs);

    const auto simplifiedShiftAmount = simplificationResultOfShiftAmount.getStatusOfResult() == OptimizationResultFlag::WasOptimized
        ? std::move(simplificationResultOfShiftAmount.tryTakeOwnershipOfOptimizationResult()->front())
        : createCopyOfNumber(*expression.rhs);

    const auto evaluatedToValueOfToBeShiftedExpr = tryEvaluateExpressionToConstant(*simplifiedToBeShiftedExpr);
    const auto evaluatedToValueOfShiftAmount     = tryEvaluateNumberAsConstant(*simplifiedShiftAmount);

    const auto& mappedToShiftOperation = syrec_operation::tryMapShiftOperationFlagToEnum(expression.op);
    if (!mappedToShiftOperation.has_value()) {
        return OptimizationResult<syrec::expression>::asUnchangedOriginal();
    }
    
    std::unique_ptr<syrec::expression> finalSimplificationResult;
    if (evaluatedToValueOfShiftAmount.has_value()) {
        if (syrec_operation::isOperandUseAsRhsInOperationIdentityElement(*mappedToShiftOperation, *evaluatedToValueOfShiftAmount)) {
            finalSimplificationResult = std::move(simplifiedToBeShiftedExpr);
            updateReferenceCountsOfSignalIdentsUsedIn(*expression.rhs, parser::SymbolTable::ReferenceCountUpdate::Decrement);
        } else if (evaluatedToValueOfToBeShiftedExpr.has_value()) {
            if (const auto& evaluatedShiftOperationResult = syrec_operation::apply(*mappedToShiftOperation, evaluatedToValueOfToBeShiftedExpr, evaluatedToValueOfShiftAmount)) {
                finalSimplificationResult = std::make_unique<syrec::NumericExpression>(std::make_unique<syrec::Number>(*evaluatedShiftOperationResult), expression.bitwidth());
            }
            updateReferenceCountsOfSignalIdentsUsedIn(*expression.lhs, parser::SymbolTable::ReferenceCountUpdate::Decrement);
            updateReferenceCountsOfSignalIdentsUsedIn(*expression.rhs, parser::SymbolTable::ReferenceCountUpdate::Decrement);
        }
    } else if (evaluatedToValueOfToBeShiftedExpr.has_value() && syrec_operation::isOperandUsedAsLhsInOperationIdentityElement(*mappedToShiftOperation, *evaluatedToValueOfToBeShiftedExpr)) {
        finalSimplificationResult = std::move(simplifiedToBeShiftedExpr);
        updateReferenceCountsOfSignalIdentsUsedIn(*expression.rhs, parser::SymbolTable::ReferenceCountUpdate::Decrement);
    }

    if (finalSimplificationResult) {
        return OptimizationResult<syrec::expression>::fromOptimizedContainer(std::move(finalSimplificationResult));   
    }
    return OptimizationResult<syrec::expression>::asUnchangedOriginal();
}

optimizations::Optimizer::OptimizationResult<syrec::Number> optimizations::Optimizer::handleNumber(const syrec::Number& number) const {
    if (number.isConstant()) {
        return OptimizationResult<syrec::Number>::asUnchangedOriginal();
    }
    if (number.isLoopVariable()) {
        if (const auto& fetchedValueOfLoopVariable = symbolTable->tryFetchValueOfLoopVariable(number.variableName()); parserConfig.performConstantPropagation && fetchedValueOfLoopVariable.has_value()) {
            updateReferenceCountOf(number.variableName(), parser::SymbolTable::ReferenceCountUpdate::Decrement);
            return OptimizationResult<syrec::Number>::fromOptimizedContainer(std::make_unique<syrec::Number>(*fetchedValueOfLoopVariable));   
        }
        updateReferenceCountOf(number.variableName(), parser::SymbolTable::ReferenceCountUpdate::Increment);
        return OptimizationResult<syrec::Number>::asUnchangedOriginal();
    }
    if (number.isCompileTimeConstantExpression()) {
        if (auto simplificationResult = trySimplifyNumber(number, *symbolTable, parserConfig.reassociateExpressionsEnabled, parserConfig.operationStrengthReductionEnabled, nullptr); simplificationResult.has_value()) {
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
        if (const auto& reassociateExpressionResult = optimizations::simplifyBinaryExpression(toBeSimplifiedExpr, performOperationStrengthReduction, std::nullopt, symbolTable, optimizedAwaySignalAccesses); reassociateExpressionResult != nullptr) {
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
    symbolTable->updateReferenceCountOfLiteral(signalIdent, typeOfUpdate);
}

std::optional<unsigned> optimizations::Optimizer::tryFetchValueForAccessedSignal(const syrec::VariableAccess& accessedSignal) const {
    if (!parserConfig.performConstantPropagation) {
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

    const auto& signalData = std::get<syrec::Variable::ptr>(*symbolTable->getVariable(accessedSignalIdent));
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
    const auto& signalData = std::get<syrec::Variable::ptr>(*symbolTable->getVariable(accessedSignalIdent));
    if (!accessedBitRange.has_value()) {
        return std::nullopt;
    }
    
    const auto& bitRangeStartEvaluated = tryEvaluateNumberAsConstant(accessedBitRange->first);
    const auto& bitRangeEndEvaluated   = tryEvaluateNumberAsConstant(accessedBitRange->second);

    auto bitRangeStartEvaluationStatus = IndexValidityStatus::Unknown;
    if (bitRangeStartEvaluated.has_value()) {
        bitRangeStartEvaluationStatus = parser::isValidBitAccess(signalData, *bitRangeStartEvaluated) ? IndexValidityStatus::Valid : IndexValidityStatus::OutOfRange;
    }
    auto bitRangeEndEvaluationStatus = IndexValidityStatus::Unknown;
    if (bitRangeEndEvaluated.has_value()) {
        bitRangeEndEvaluationStatus = parser::isValidBitAccess(signalData, *bitRangeEndEvaluated) ? IndexValidityStatus::Valid : IndexValidityStatus::OutOfRange;
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
    if (const auto& symbolTableEntryForAccessedSignal = symbolTable->getVariable(accessedSignalParts.var->name); symbolTableEntryForAccessedSignal.has_value() && std::holds_alternative<syrec::Variable::ptr>(*symbolTableEntryForAccessedSignal)) {
        const std::string_view& accessedSignalIdent = accessedSignalParts.var->name;

        std::vector<std::reference_wrapper<const syrec::expression>> accessedValueOfDimensionWithoutOwnership;
        accessedValueOfDimensionWithoutOwnership.reserve(accessedSignalParts.indexes.size());

        for (std::size_t i = 0; i < accessedValueOfDimensionWithoutOwnership.size(); ++i) {
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

std::unique_ptr<syrec::VariableAccess> optimizations::Optimizer::transformEvaluatedSignalAccess(EvaluatedSignalAccess& evaluatedSignalAccess, std::optional<bool>* wasAnyDefinedIndexOutOfRange, bool* didAllDefinedIndicesEvaluateToConstants) const {
    if (const auto& matchingSymbolTableEntryForIdent = symbolTable->getVariable(evaluatedSignalAccess.accessedSignalIdent); matchingSymbolTableEntryForIdent.has_value() && std::holds_alternative<syrec::Variable::ptr>(*matchingSymbolTableEntryForIdent)) {
        std::optional<std::pair<syrec::Number::ptr, syrec::Number::ptr>> definedBitRangeAccess;
        std::vector<syrec::expression::ptr>                              definedDimensionAccess(evaluatedSignalAccess.accessedValuePerDimension.valuePerDimension.size(), nullptr);

        if (evaluatedSignalAccess.accessedBitRange.has_value()) {
            syrec::Number::ptr bitRangeStart;
            syrec::Number::ptr bitRangeEnd;

            if (const auto& constantValueOfBitRangeStart = tryFetchConstantValueOfIndex(evaluatedSignalAccess.accessedBitRange->rangeStartEvaluationResult); constantValueOfBitRangeStart.has_value()) {
                bitRangeStart = std::make_shared<syrec::Number>(*constantValueOfBitRangeStart);
                if (didAllDefinedIndicesEvaluateToConstants) {
                    *didAllDefinedIndicesEvaluateToConstants = true;
                }
            } else {
                bitRangeStart = *tryFetchAndTakeOwnershipOfNonConstantValueOfIndex(evaluatedSignalAccess.accessedBitRange->rangeStartEvaluationResult);
            }

            if (const auto& constantValueOfBitRangeEnd = tryFetchConstantValueOfIndex(evaluatedSignalAccess.accessedBitRange->rangeEndEvaluationResult); constantValueOfBitRangeEnd.has_value()) {
                bitRangeEnd = std::make_shared<syrec::Number>(*constantValueOfBitRangeEnd);
                if (didAllDefinedIndicesEvaluateToConstants) {
                    *didAllDefinedIndicesEvaluateToConstants &= true;
                }
            } else {
                bitRangeEnd = *tryFetchAndTakeOwnershipOfNonConstantValueOfIndex(evaluatedSignalAccess.accessedBitRange->rangeEndEvaluationResult);
            }
            definedBitRangeAccess = std::make_pair(bitRangeStart, bitRangeEnd);
        }

        for (auto& evaluatedValueOfDimension : evaluatedSignalAccess.accessedValuePerDimension.valuePerDimension) {
            if (const auto& constantValueOfValueOfDimension = tryFetchConstantValueOfIndex(evaluatedValueOfDimension); constantValueOfValueOfDimension.has_value()) {
                definedDimensionAccess.emplace_back(std::make_shared<syrec::NumericExpression>(std::make_shared<syrec::Number>(*constantValueOfValueOfDimension), BitHelpers::getRequiredBitsToStoreValue(*constantValueOfValueOfDimension)));
                if (didAllDefinedIndicesEvaluateToConstants) {
                    *didAllDefinedIndicesEvaluateToConstants = true;
                }
            }
            definedDimensionAccess.emplace_back(*tryFetchAndTakeOwnershipOfNonConstantValueOfIndex(evaluatedValueOfDimension));
        }

        auto transformedToSignalAccess = std::make_unique<syrec::VariableAccess>();
        transformedToSignalAccess->var = std::get<syrec::Variable::ptr>(*matchingSymbolTableEntryForIdent);
        transformedToSignalAccess->indexes = definedDimensionAccess;
        transformedToSignalAccess->range   = definedBitRangeAccess;
        return transformedToSignalAccess;
    }
    return nullptr;
}

/*
 * TODO: Removed additional pointer conversion when symbol table can be called with references instead of smart pointers
 */
void optimizations::Optimizer::invalidateValueOfAccessedSignalParts(EvaluatedSignalAccess& accessedSignalParts) const {
    if (auto transformedSignalAccess = transformEvaluatedSignalAccess(accessedSignalParts, nullptr, nullptr); transformedSignalAccess != nullptr) {
        const std::shared_ptr<syrec::VariableAccess> sharedTransformedSignalAccess = std::move(transformedSignalAccess);
        invalidateStoredValueFor(sharedTransformedSignalAccess);
    }
}

void optimizations::Optimizer::invalidateValueOfWholeSignal(const std::string_view& signalIdent) const {
    if (const auto& matchingSymbolTableEntry = symbolTable->getVariable(signalIdent); matchingSymbolTableEntry.has_value() && std::holds_alternative<syrec::Variable::ptr>(*matchingSymbolTableEntry)) {
        const auto& sharedTransformedSignalAccess = std::make_shared<syrec::VariableAccess>();
        sharedTransformedSignalAccess->var        = std::get<syrec::Variable::ptr>(*matchingSymbolTableEntry);
        invalidateStoredValueFor(sharedTransformedSignalAccess);
    }
}

/*
 * TODO: Removed additional pointer conversion when symbol table can be called with references instead of smart pointers
 */
void optimizations::Optimizer::performAssignment(EvaluatedSignalAccess& assignedToSignalParts, syrec_operation::operation assignmentOperation, unsigned assignmentRhsValue) const {
    std::optional<bool> areAnyComponentsOfAssignedToSignalAccessOutOfRange;
    bool                didAllDefinedIndicesEvaluateToConstants = false;
    if (auto transformedSignalAccess = transformEvaluatedSignalAccess(assignedToSignalParts, &areAnyComponentsOfAssignedToSignalAccessOutOfRange, &didAllDefinedIndicesEvaluateToConstants); transformedSignalAccess != nullptr) {
        const std::shared_ptr<syrec::VariableAccess> sharedTransformedSignalAccess = std::move(transformedSignalAccess);

        if (!(areAnyComponentsOfAssignedToSignalAccessOutOfRange.has_value() && *areAnyComponentsOfAssignedToSignalAccessOutOfRange) || !didAllDefinedIndicesEvaluateToConstants) {
            if (const auto& currentValueOfAssignedToSignal = tryFetchValueFromEvaluatedSignalAccess(assignedToSignalParts); currentValueOfAssignedToSignal.has_value()) {
                if (const auto evaluationResultOfOperation = syrec_operation::apply(assignmentOperation, currentValueOfAssignedToSignal, assignmentRhsValue); evaluationResultOfOperation.has_value()) {
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
        performAssignment(assignmentLhsOperand, assignmentOperation, *fetchedValueForAssignmentRhsOperand);
        updateReferenceCountOf(assignmentRhsOperand.accessedSignalIdent, parser::SymbolTable::ReferenceCountUpdate::Decrement);
        return;
    }
    auto                                         transformedLhsOperandSignalAccess       = transformEvaluatedSignalAccess(assignmentLhsOperand, nullptr, nullptr);
    const std::shared_ptr<syrec::VariableAccess> sharedTransformedLhsOperandSignalAccess = std::move(transformedLhsOperandSignalAccess);
    invalidateStoredValueFor(sharedTransformedLhsOperandSignalAccess);
}

void optimizations::Optimizer::performSwap(EvaluatedSignalAccess& swapOperationLhsOperand, EvaluatedSignalAccess& swapOperationRhsOperand) const {
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
    createBackupOfAssignedToSignal(*assignedToSignal);
    symbolTable->updateStoredValueFor(assignedToSignal, newValueOfAssignedToSignal);
}

void optimizations::Optimizer::invalidateStoredValueFor(const syrec::VariableAccess::ptr& assignedToSignal) const {
    createBackupOfAssignedToSignal(*assignedToSignal);
    symbolTable->invalidateStoredValuesFor(assignedToSignal);
}

void optimizations::Optimizer::performSwapAndCreateBackupOfOperands(const syrec::VariableAccess::ptr& swapLhsOperand, const syrec::VariableAccess::ptr& swapRhsOperand) const {
    createBackupOfAssignedToSignal(*swapLhsOperand);
    createBackupOfAssignedToSignal(*swapRhsOperand);
    symbolTable->swap(swapLhsOperand, swapRhsOperand);
}


std::optional<unsigned> optimizations::Optimizer::tryFetchValueFromEvaluatedSignalAccess(EvaluatedSignalAccess& accessedSignalParts) const {
    std::optional<bool> areAnyComponentsOfAssignedToSignalAccessOutOfRange;
    bool                didAllDefinedIndicesEvaluateToConstants;
    if (auto transformedSignalAccess = transformEvaluatedSignalAccess(accessedSignalParts, &areAnyComponentsOfAssignedToSignalAccessOutOfRange, &didAllDefinedIndicesEvaluateToConstants); transformedSignalAccess != nullptr) {
        const std::shared_ptr<syrec::VariableAccess> sharedTransformedSignalAccess = std::move(transformedSignalAccess);
        if (!areAnyComponentsOfAssignedToSignalAccessOutOfRange.has_value() && !*areAnyComponentsOfAssignedToSignalAccessOutOfRange && didAllDefinedIndicesEvaluateToConstants) {
            return symbolTable->tryFetchValueForLiteral(sharedTransformedSignalAccess);
        }
    }
    return std::nullopt;
}

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
                                                                                                                          return valueOfDimensionEvaluationResult.validityStatus != IndexValidityStatus::OutOfRange;
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

void optimizations::Optimizer::discardChangesMadeInCurrentSymbolTableBackupScope() const {
    if (symbolTableBackupScopeStack.empty()) {
        return;
    }
    for (const auto& [signalIdent, valueAndRestrictionBackup]: symbolTableBackupScopeStack.top()->getBackedUpEntries()) {
        symbolTable->restoreValuesFromBackup(signalIdent, valueAndRestrictionBackup);
    }
}

void optimizations::Optimizer::mergeAndMakeLocalChangesGlobal(const parser::SymbolTableBackupHelper& firstLocalBackupScope, const parser::SymbolTableBackupHelper& secondLocalBackupScope) const {
    const auto& modifiedSignalInFirstScope = firstLocalBackupScope.getIdentsOfChangedSignals();
    const auto& modifiedSignalsInSecondScope = secondLocalBackupScope.getIdentsOfChangedSignals();

    std::unordered_set<std::string> setOfSignalsRequiringMergeOfChanges;
    std::set_intersection(
            modifiedSignalInFirstScope.cbegin(), modifiedSignalInFirstScope.cend(),
            modifiedSignalsInSecondScope.cbegin(), modifiedSignalsInSecondScope.cend(),
            std::inserter(setOfSignalsRequiringMergeOfChanges, setOfSignalsRequiringMergeOfChanges.begin()));

    for (const auto& signalRequiringMergeOfChanges : setOfSignalsRequiringMergeOfChanges) {
        const auto& initialValueAtStartOfFirstScope        = *firstLocalBackupScope.getBackedUpEntryFor(signalRequiringMergeOfChanges);
        const auto& currentValueOfSignalAtEndOfFirstScope  = *secondLocalBackupScope.getBackedUpEntryFor(signalRequiringMergeOfChanges);
        const auto& currentValueOfSignalAtEndOfSecondScope = *symbolTable->createBackupOfValueOfSignal(signalRequiringMergeOfChanges);

        initialValueAtStartOfFirstScope->copyRestrictionsAndMergeValuesFromAlternatives(currentValueOfSignalAtEndOfFirstScope, currentValueOfSignalAtEndOfSecondScope);
        symbolTable->restoreValuesFromBackup(signalRequiringMergeOfChanges, initialValueAtStartOfFirstScope);
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

    if (const auto& backupOfAssignedToSignal = symbolTable->createBackupOfValueOfSignal(assignedToSignalIdent); backupOfAssignedToSignal.has_value()) {
        symbolTableBackupScopeStack.top()->storeBackupOf(std::string(assignedToSignalIdent), *backupOfAssignedToSignal);
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