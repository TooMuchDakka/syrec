#include "core/syrec/parser/optimizations/optimizer.hpp"

#include "core/syrec/parser/module_call_guess.hpp"
#include "core/syrec/parser/operation.hpp"
#include "core/syrec/parser/range_check.hpp"
#include "core/syrec/parser/optimizations/reassociate_expression.hpp"
#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/main_additional_line_for_assignment_simplifier.hpp"
#include "core/syrec/parser/utils/binary_expression_simplifier.hpp"

// TODO: Refactoring if case switches with visitor pattern as its available in std::visit call

optimizations::Optimizer::OptimizationResult<syrec::Module> optimizations::Optimizer::optimizeProgram(const std::vector<std::reference_wrapper<syrec::Module>>& modules) {
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

optimizations::Optimizer::OptimizationResult<syrec::Statement> optimizations::Optimizer::handleStatements(const std::vector<std::reference_wrapper<syrec::Statement>>& statements) {
    std::vector<std::unique_ptr<syrec::Statement>> optimizedStmts;
    bool optimizedAnyStmt = false;
    for (const auto& stmt: statements) {
        auto&& optimizedStatement = handleStatement(stmt);

        const auto& stmtOptimizationResultFlag = optimizedStatement.getStatusOfResult();
        optimizedAnyStmt                       = stmtOptimizationResultFlag == OptimizationResultFlag::WasOptimized;
        if (stmtOptimizationResultFlag == OptimizationResultFlag::WasOptimized) {
            if (auto optimizationResultOfStatement = optimizedStatement.tryTakeOwnershipOfOptimizationResult(); optimizationResultOfStatement.has_value()) {
                for (auto&& optimizedStmt : *optimizationResultOfStatement) {
                    optimizedStmts.push_back(std::move(optimizedStmt));
                }
            }
        } else if (stmtOptimizationResultFlag == OptimizationResultFlag::IsUnchanged) {
            optimizedStmts.push_back(createCopyOfStmt(stmt));
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
    if (const auto& evaluatedAssignedToSignal = tryEvaluateDefinedSignalAccess(*assignedToSignalPartsAfterSimplification); evaluatedAssignedToSignal.has_value()) {
        const auto& currentValueOfAssignedToSignal      = tryFetchValueFromEvaluatedSignalAccess(*evaluatedAssignedToSignal);

        if (currentValueOfAssignedToSignal.has_value() && mappedToAssignmentOperationFromFlag.has_value() && rhsOperandEvaluatedAsConstant.has_value()) {
            performAssignment(*evaluatedAssignedToSignal, *mappedToAssignmentOperationFromFlag, *rhsOperandEvaluatedAsConstant);
        }
        else {
            invalidateValueOfAccessedSignalParts(*evaluatedAssignedToSignal);
        }
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
                remainingSimplifiedAssignments.push_back(std::make_unique<syrec::AssignStatement>(subAssignmentCasted->lhs, subAssignmentCasted->op, subAssignmentCasted->rhs));
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
        bool                              calleeArgumentsOk = true;
        std::vector<syrec::Variable::ptr> symbolTableEntryForCalleeArguments(callStatement.parameters.size());
        for (std::size_t i = 0; i < symbolTableEntryForCalleeArguments.size() && calleeArgumentsOk; ++i) {
            if (const auto& symbolTableEntryForCalleeArgument = symbolTable->getVariable(callStatement.parameters.at(i)); symbolTableEntryForCalleeArgument.has_value() && std::holds_alternative<syrec::Variable::ptr>(*symbolTableEntryForCalleeArgument)) {
                const auto matchingSignalForCallleeArgument = std::get<syrec::Variable::ptr>(*symbolTableEntryForCalleeArgument);
                symbolTableEntryForCalleeArguments.emplace_back(matchingSignalForCallleeArgument);
                moduleCallGuessResolver->get()->refineGuessWithNextParameter(*matchingSignalForCallleeArgument);
                calleeArgumentsOk = true;
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
                    const auto calleeArgumentAsSignalAccess = std::make_unique<syrec::VariableAccess>();
                    calleeArgumentAsSignalAccess->var       = calleeArgument;
                    calleeArgumentAsSignalAccess->indexes   = {};
                    calleeArgumentAsSignalAccess->range     = std::nullopt;

                    invalidateValueOfAccessedSignalParts(*tryEvaluateDefinedSignalAccess(*calleeArgumentAsSignalAccess));
                }
            } else {
                const auto setOfOptimizedModuleParameterLookup = std::set<std::size_t>(modulesMatchingCallSignature.front().indicesOfOptimizedAwayParameters.begin(), modulesMatchingCallSignature.front().indicesOfOptimizedAwayParameters.end());
                const auto& calledModule = modulesMatchingCallSignature.front();

                for (std::size_t i = 0; i < calledModule.declaredParameters.size(); i++) {
                    if (setOfOptimizedModuleParameterLookup.count(i) != 0) {
                        continue;   
                    }

                    const auto& signalTypeOfCallerParameter = calledModule.declaredParameters.at(i)->type;
                    if (signalTypeOfCallerParameter == syrec::Variable::Types::In) {
                        continue;
                    }

                    const auto calleeArgumentAsSignalAccess = std::make_unique<syrec::VariableAccess>();
                    calleeArgumentAsSignalAccess->var       = symbolTableEntryForCalleeArguments.at(i);
                    calleeArgumentAsSignalAccess->indexes   = {};
                    calleeArgumentAsSignalAccess->range     = std::nullopt;

                    invalidateValueOfAccessedSignalParts(*tryEvaluateDefinedSignalAccess(*calleeArgumentAsSignalAccess));
                }
            }
        }
    }
    return OptimizationResult<syrec::Statement>::asUnchangedOriginal();
}

optimizations::Optimizer::OptimizationResult<syrec::Statement> optimizations::Optimizer::handleIfStmt(const syrec::IfStatement& ifStatement) {
    return OptimizationResult<syrec::Statement>::asUnchangedOriginal();
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
    
    const auto& evaluatedSwapOperationLhsSignalAccess = tryEvaluateDefinedSignalAccess(*lhsOperandAfterSimplification);
    const auto& evaluatedSwapOperationRhsSignalAccess = tryEvaluateDefinedSignalAccess(*rhsOperandAfterSimplification);

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

    if (const auto& evaluatedUnaryOperandSignalAccess = tryEvaluateDefinedSignalAccess(*signalAccessOperand); evaluatedUnaryOperandSignalAccess.has_value()) {
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

// TODO: Reassociate expression simplification
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

    if (auto simplificationResultOfExpr = trySimplifyExpr(*simplifiedBinaryExpr); simplificationResultOfExpr != nullptr) {
        simplifiedBinaryExpr = std::move(simplificationResultOfExpr);
        wasOriginalExprModified = true;
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

// TODO: Index checks due to new values being available ?
// TODO: Bitwidth ?
optimizations::Optimizer::OptimizationResult<syrec::expression> optimizations::Optimizer::handleVariableExpr(const syrec::VariableExpression& expression) const {
    if (const auto& fetchedValueForSignal = symbolTable->tryFetchValueForLiteral(expression.var); fetchedValueForSignal.has_value() && parserConfig.performConstantPropagation) {
        updateReferenceCountOf(expression.var->var->name, parser::SymbolTable::ReferenceCountUpdate::Decrement);
        return OptimizationResult<syrec::expression>::fromOptimizedContainer(std::make_unique<syrec::NumericExpression>(std::make_unique<syrec::Number>(*fetchedValueForSignal), expression.bitwidth()));
    }
    updateReferenceCountOf(expression.var->var->name, parser::SymbolTable::ReferenceCountUpdate::Increment);
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
        const auto mappedToBinaryExpr = transformCompileTimeConstantExpressionToNumber(number.getExpression());
        if (const auto& simplificationResultOfBinaryExpr = trySimplifyExpr(*mappedToBinaryExpr); simplificationResultOfBinaryExpr != nullptr) {
            if (auto simplificationResultAsCompileTimeExpr = tryMapExprToCompileTimeConstantExpr(*simplificationResultOfBinaryExpr); simplificationResultAsCompileTimeExpr != nullptr) {
                return OptimizationResult<syrec::Number>::fromOptimizedContainer(std::move(simplificationResultAsCompileTimeExpr));
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

std::unique_ptr<syrec::expression> optimizations::Optimizer::trySimplifyExpr(const syrec::expression& expr) const {
    std::unique_ptr<syrec::expression> simplificationResult;
    std::vector<syrec::VariableAccess::ptr> droppedSignalAccesses;
    const std::shared_ptr<syrec::expression> toBeSimplifiedExpr = createCopyOfExpression(expr);
    if (parserConfig.reassociateExpressionsEnabled) {
        
        if (const auto& reassociateExpressionResult = optimizations::simplifyBinaryExpression(toBeSimplifiedExpr, parserConfig.operationStrengthReductionEnabled, std::nullopt, symbolTable, droppedSignalAccesses); reassociateExpressionResult != nullptr) {
            simplificationResult = createCopyOfExpression(*reassociateExpressionResult);
        }
    } else {
        if (const auto& generalBinaryExpressionSimplificationResult = optimizations::trySimplify(toBeSimplifiedExpr, parserConfig.operationStrengthReductionEnabled, symbolTable, droppedSignalAccesses); generalBinaryExpressionSimplificationResult.couldSimplify) {
            simplificationResult = createCopyOfExpression(*generalBinaryExpressionSimplificationResult.simplifiedExpression);
        }
    }

    std::for_each(
            droppedSignalAccesses.cbegin(),
            droppedSignalAccesses.cend(),
            [&](const syrec::VariableAccess::ptr& droppedSignalAccess) {
                updateReferenceCountOf(droppedSignalAccess->var->name, parser::SymbolTable::ReferenceCountUpdate::Decrement);
            });
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
        return std::make_unique<syrec::NumericExpression>(std::make_unique<syrec::Number>(number.evaluate({})), 1);
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

    if (const auto& evaluatedSignalAccess = tryEvaluateDefinedSignalAccess(accessedSignal); evaluatedSignalAccess.has_value()) {
        return tryFetchValueFromEvaluatedSignalAccess(*evaluatedSignalAccess);
    }
    return std::nullopt;
}

optimizations::Optimizer::DimensionAccessEvaluationResult optimizations::Optimizer::evaluateUserDefinedDimensionAccess(const std::string_view& accessedSignalIdent, const std::vector<std::reference_wrapper<const syrec::expression>>& accessedValuePerDimension) const {
    const auto& signalData = std::get<syrec::Variable::ptr>(*symbolTable->getVariable(accessedSignalIdent));
    if (accessedValuePerDimension.empty()) {
        return DimensionAccessEvaluationResult({false, std::vector<std::pair<IndexValidityStatus, std::optional<unsigned int>>>()});
    }

    const auto wasUserDefinedAccessTrimmed = accessedValuePerDimension.size() > signalData->dimensions.size();
    const auto numEntriesToTransform       = wasUserDefinedAccessTrimmed ? signalData->dimensions.size() : accessedValuePerDimension.size();
    
    std::vector<std::pair<IndexValidityStatus, std::optional<unsigned int>>> evaluatedValuePerDimension;
    evaluatedValuePerDimension.reserve(numEntriesToTransform);

    for (std::size_t i = 0; i < numEntriesToTransform; ++i) {
        const auto& userDefinedValueOfDimension = accessedValuePerDimension.at(i);
        if (const auto& valueOfDimensionEvaluated   = tryEvaluateExpressionToConstant(userDefinedValueOfDimension); valueOfDimensionEvaluated.has_value()) {
            const auto isAccessedValueOfDimensionWithinRange = parser::isValidDimensionAccess(signalData, i, *valueOfDimensionEvaluated);
            evaluatedValuePerDimension.emplace_back(isAccessedValueOfDimensionWithinRange ? IndexValidityStatus::Valid : IndexValidityStatus::OutOfRange, *valueOfDimensionEvaluated);
        } else {
            evaluatedValuePerDimension.emplace_back(IndexValidityStatus::Unknown, std::nullopt);
        }
    }
    return DimensionAccessEvaluationResult({wasUserDefinedAccessTrimmed, evaluatedValuePerDimension});
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

optimizations::Optimizer::BitRangeEvaluationResult optimizations::Optimizer::evaluateUserDefinedBitRangeAccess(const std::string_view& accessedSignalIdent, const std::optional<std::pair<std::reference_wrapper<const syrec::Number>, std::reference_wrapper<const syrec::Number>>>& accessedBitRange) const {
    const auto& signalData = std::get<syrec::Variable::ptr>(*symbolTable->getVariable(accessedSignalIdent));
    if (!accessedBitRange.has_value()) {
        return BitRangeEvaluationResult(IndexValidityStatus::Valid, std::make_optional(0), IndexValidityStatus::Valid, std::make_optional(signalData->bitwidth - 1));
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
    return BitRangeEvaluationResult(bitRangeStartEvaluationStatus, bitRangeStartEvaluated, bitRangeEndEvaluationStatus, bitRangeEndEvaluated);
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

        return std::make_optional(optimizations::Optimizer::EvaluatedSignalAccess({
            accessedSignalParts.var->name,
            evaluateUserDefinedDimensionAccess(accessedSignalIdent, accessedValueOfDimensionWithoutOwnership),
            evaluateUserDefinedBitRangeAccess(accessedSignalIdent, accessedBitRangeWithoutOwnership)})
        );
    }
    return std::nullopt;
}


/*
 * TODO: Removed additional pointer conversion when symbol table can be called with references instead of smart pointers
 */
void optimizations::Optimizer::invalidateValueOfAccessedSignalParts(const EvaluatedSignalAccess& accessedSignalParts) const {
    if (auto transformedSignalAccess = transformEvaluatedSignalAccess(accessedSignalParts, nullptr, nullptr); transformedSignalAccess != nullptr) {
        const std::shared_ptr<syrec::VariableAccess> sharedTransformedSignalAccess = std::move(transformedSignalAccess);
        symbolTable->invalidateStoredValuesFor(sharedTransformedSignalAccess);
    }
}

/*
 * TODO: Removed additional pointer conversion when symbol table can be called with references instead of smart pointers
 */
void optimizations::Optimizer::performAssignment(const EvaluatedSignalAccess& assignedToSignalParts, syrec_operation::operation assignmentOperation, unsigned assignmentRhsValue) const {
    std::optional<bool> areAnyComponentsOfAssignedToSignalAccessOutOfRange;
    bool                areAllComponentsOfAssignedToSignalConstants;
    if (auto transformedSignalAccess = transformEvaluatedSignalAccess(assignedToSignalParts, &areAnyComponentsOfAssignedToSignalAccessOutOfRange, &areAllComponentsOfAssignedToSignalConstants); transformedSignalAccess != nullptr) {
        const std::shared_ptr<syrec::VariableAccess> sharedTransformedSignalAccess = std::move(transformedSignalAccess);
        if (const auto& currentValueOfAssignedToSignal = tryFetchValueFromEvaluatedSignalAccess(assignedToSignalParts); currentValueOfAssignedToSignal.has_value()) {
            if (const auto evaluationResultOfOperation = syrec_operation::apply(assignmentOperation, currentValueOfAssignedToSignal, assignmentRhsValue); evaluationResultOfOperation.has_value()) {
                symbolTable->updateStoredValueFor(sharedTransformedSignalAccess, *evaluationResultOfOperation);
                return;
            }
        }
        symbolTable->invalidateStoredValuesFor(sharedTransformedSignalAccess);
    }
}

/*
 * TODO: Removed additional pointer conversion when symbol table can be called with references instead of smart pointers
 */
void optimizations::Optimizer::performAssignment(const EvaluatedSignalAccess& assignmentLhsOperand, syrec_operation::operation assignmentOperation, const EvaluatedSignalAccess& assignmentRhsOperand) const {
    if (const auto fetchedValueForAssignmentRhsOperand = tryFetchValueFromEvaluatedSignalAccess(assignmentRhsOperand); fetchedValueForAssignmentRhsOperand.has_value()) {
        performAssignment(assignmentLhsOperand, assignmentOperation, *fetchedValueForAssignmentRhsOperand);
        updateReferenceCountOf(assignmentRhsOperand.accessedSignalIdent, parser::SymbolTable::ReferenceCountUpdate::Decrement);
        return;
    }
    auto                                         transformedLhsOperandSignalAccess       = transformEvaluatedSignalAccess(assignmentLhsOperand, nullptr, nullptr);
    const std::shared_ptr<syrec::VariableAccess> sharedTransformedLhsOperandSignalAccess = std::move(transformedLhsOperandSignalAccess);
    symbolTable->invalidateStoredValuesFor(sharedTransformedLhsOperandSignalAccess);
}

void optimizations::Optimizer::performSwap(const EvaluatedSignalAccess& swapOperationLhsOperand, const EvaluatedSignalAccess& swapOperationRhsOperand) const {
    std::optional<bool> areAnyComponentsOfSignalAccessOfSwapLhsOperandOutOfRange;
    std::optional<bool> areAnyComponentsOfSignalAccessOfSwapRhsOperandOutOfRange;

    auto transformedSignalAccessForLhsOperand = transformEvaluatedSignalAccess(swapOperationLhsOperand, &areAnyComponentsOfSignalAccessOfSwapLhsOperandOutOfRange, nullptr);
    auto transformedSignalAccessForRhsOperand = transformEvaluatedSignalAccess(swapOperationRhsOperand, &areAnyComponentsOfSignalAccessOfSwapRhsOperandOutOfRange, nullptr);

    const std::shared_ptr<syrec::VariableAccess> sharedTransformedSwapLhsOperand = std::move(transformedSignalAccessForLhsOperand);
    const std::shared_ptr<syrec::VariableAccess> sharedTransformedSwapRhsOperand = std::move(transformedSignalAccessForRhsOperand);

    if (areAnyComponentsOfSignalAccessOfSwapLhsOperandOutOfRange.has_value() && !*areAnyComponentsOfSignalAccessOfSwapLhsOperandOutOfRange && areAnyComponentsOfSignalAccessOfSwapRhsOperandOutOfRange.has_value() && !*areAnyComponentsOfSignalAccessOfSwapRhsOperandOutOfRange) {
        symbolTable->swap(sharedTransformedSwapLhsOperand, sharedTransformedSwapRhsOperand);
    }

    if (sharedTransformedSwapLhsOperand) {
        symbolTable->invalidateStoredValuesFor(sharedTransformedSwapLhsOperand);
    }
    if (sharedTransformedSwapRhsOperand) {
        symbolTable->invalidateStoredValuesFor(sharedTransformedSwapRhsOperand);
    }
}

std::optional<unsigned> optimizations::Optimizer::tryFetchValueFromEvaluatedSignalAccess(const EvaluatedSignalAccess& accessedSignalParts) const {
    std::optional<bool> areAnyComponentsOfAssignedToSignalAccessOutOfRange;
    bool                areAllComponentsOfAssignedToSignalConstants;
    if (auto transformedSignalAccess = transformEvaluatedSignalAccess(accessedSignalParts, &areAnyComponentsOfAssignedToSignalAccessOutOfRange, &areAllComponentsOfAssignedToSignalConstants); transformedSignalAccess != nullptr) {
        const std::shared_ptr<syrec::VariableAccess> sharedTransformedSignalAccess = std::move(transformedSignalAccess);
        if (areAnyComponentsOfAssignedToSignalAccessOutOfRange.has_value() && !*areAnyComponentsOfAssignedToSignalAccessOutOfRange && areAllComponentsOfAssignedToSignalConstants) {
            return symbolTable->tryFetchValueForLiteral(sharedTransformedSignalAccess);
        }
    }
    return std::nullopt;
}

std::unique_ptr<syrec::VariableAccess> optimizations::Optimizer::transformEvaluatedSignalAccess(const EvaluatedSignalAccess& accessedSignalParts, std::optional<bool>* wasAnyAccessedComponentOutOfRange, bool* areAllAccessedSignalAccessComponentsConstants) const {
    if (const auto& symbolTableEntryForAccessedSignal = symbolTable->getVariable(accessedSignalParts.accessedSignalIdent); symbolTableEntryForAccessedSignal.has_value() && std::holds_alternative<syrec::Variable::ptr>(*symbolTableEntryForAccessedSignal)) {
        if (areAllAccessedSignalAccessComponentsConstants != nullptr) {
            *areAllAccessedSignalAccessComponentsConstants = true;   
        }

        std::vector<syrec::expression::ptr> transformedAccessedValuePerDimension(accessedSignalParts.accessedValuePerDimension.valuePerDimension.size(), nullptr);
        const std::string                   loopVariableToUsedToIndicateAllValueForDimensionAccessed = "test";

        for (std::size_t i = 0; i < transformedAccessedValuePerDimension.size(); ++i) {
            const auto& [definedValueValidityStatus, evaluatedValueForDimension] = accessedSignalParts.accessedValuePerDimension.valuePerDimension.at(i);

            const auto accessedValueForDimension = definedValueValidityStatus == IndexValidityStatus::Valid
                ? std::make_shared<syrec::NumericExpression>(std::make_unique<syrec::Number>(*evaluatedValueForDimension), 1)
                : std::make_shared<syrec::NumericExpression>(std::make_shared<syrec::Number>(loopVariableToUsedToIndicateAllValueForDimensionAccessed), 1);
            transformedAccessedValuePerDimension.push_back(accessedValueForDimension);

            if (wasAnyAccessedComponentOutOfRange != nullptr && wasAnyAccessedComponentOutOfRange->has_value()) {
                *wasAnyAccessedComponentOutOfRange = *wasAnyAccessedComponentOutOfRange || definedValueValidityStatus == IndexValidityStatus::OutOfRange;
            }
            if (areAllAccessedSignalAccessComponentsConstants != nullptr) {
                *areAllAccessedSignalAccessComponentsConstants &= definedValueValidityStatus == IndexValidityStatus::Valid;   
            }
        }

        std::optional<std::pair<syrec::Number::ptr, syrec::Number::ptr>> transformedAccessedBitRange;
        const auto& [validityStatusOfBitRangeStart, evaluateValueForBitRangeStart] = accessedSignalParts.accessedBitRange.rangeStartEvaluationResult;
        const auto& [validityStatusOfBitRangeEnd, evaluateValueForBitRangeEnd]     = accessedSignalParts.accessedBitRange.rangeEndEvaluationResult;

        if (validityStatusOfBitRangeStart == IndexValidityStatus::Valid && validityStatusOfBitRangeEnd == IndexValidityStatus::Valid) {
            transformedAccessedBitRange = std::make_pair(std::make_unique<syrec::Number>(*evaluateValueForBitRangeStart), std::make_unique<syrec::Number>(*evaluateValueForBitRangeEnd));
        } else {
            const auto& bitWidthOfAccessedSignal = std::get<syrec::Variable::ptr>(*symbolTableEntryForAccessedSignal)->bitwidth;

            unsigned int transformedBitRangeStart = 0;
            unsigned int transformedBitRangeEnd   = bitWidthOfAccessedSignal - 1;
            if (validityStatusOfBitRangeStart == IndexValidityStatus::Valid) {
                transformedBitRangeStart = *evaluateValueForBitRangeStart;
            }
            transformedAccessedBitRange = std::make_pair(std::make_unique<syrec::Number>(transformedBitRangeStart), std::make_unique<syrec::Number>(transformedBitRangeEnd));
        }

        if (wasAnyAccessedComponentOutOfRange != nullptr && wasAnyAccessedComponentOutOfRange->has_value()) {
            *wasAnyAccessedComponentOutOfRange = *wasAnyAccessedComponentOutOfRange && (validityStatusOfBitRangeStart == IndexValidityStatus::OutOfRange || validityStatusOfBitRangeEnd == IndexValidityStatus::OutOfRange);
        }
        if (areAllAccessedSignalAccessComponentsConstants != nullptr) {
            *areAllAccessedSignalAccessComponentsConstants &= validityStatusOfBitRangeStart == IndexValidityStatus::Valid && validityStatusOfBitRangeEnd == IndexValidityStatus::Valid;   
        }

        auto generatedSignalAccess     = std::make_unique<syrec::VariableAccess>();
        generatedSignalAccess->var     = std::get<syrec::Variable::ptr>(*symbolTableEntryForAccessedSignal);
        generatedSignalAccess->indexes = transformedAccessedValuePerDimension;
        generatedSignalAccess->range   = transformedAccessedBitRange;
        return generatedSignalAccess;
    }
    return nullptr;
}

optimizations::Optimizer::OptimizationResult<syrec::VariableAccess> optimizations::Optimizer::handleSignalAccess(const syrec::VariableAccess& signalAccess, bool performConstantPropagationForAccessedSignal, std::optional<unsigned int>* fetchedValue) const {
    if (const auto& evaluatedSignalAccess = tryEvaluateDefinedSignalAccess(signalAccess); evaluatedSignalAccess.has_value()) {
        std::vector<OptimizationResult<syrec::expression>> simplifiedValuePerDimensionAccess;
        simplifiedValuePerDimensionAccess.reserve(signalAccess.indexes.size());
        bool wasAnyValueOfDimensionSimplified = false;

        for (const auto& userDefinedValueOfDimension : signalAccess.indexes) {
            const auto& simplificationResultOfValueOfDimension = handleExpr(*userDefinedValueOfDimension);
            wasAnyValueOfDimensionSimplified                   = simplificationResultOfValueOfDimension.getStatusOfResult() == OptimizationResultFlag::WasOptimized;
            simplifiedValuePerDimensionAccess.push_back(handleExpr(*userDefinedValueOfDimension));
        }

        std::optional<OptimizationResult<syrec::Number>> bitRangeStartSimplified = signalAccess.range.has_value() ? std::make_optional(handleNumber(*signalAccess.range->first)) : std::nullopt;
        std::optional<OptimizationResult<syrec::Number>> bitRangeEndSimplified = signalAccess.range.has_value() ? std::make_optional(handleNumber(*signalAccess.range->second)) : std::nullopt;

        std::optional<std::pair<syrec::Number::ptr, syrec::Number::ptr>> simplifiedBitRangeAccess;
        const auto                                             wasBitRangeStartSimplified = signalAccess.range.has_value() && bitRangeStartSimplified.has_value() && bitRangeStartSimplified->getStatusOfResult() == OptimizationResultFlag::WasOptimized;
        const auto                                             wasBitRangeEndSimplified = signalAccess.range.has_value() && bitRangeEndSimplified.has_value() && bitRangeEndSimplified->getStatusOfResult() == OptimizationResultFlag::WasOptimized;
        if (wasBitRangeStartSimplified || wasBitRangeEndSimplified) {
            const syrec::Number::ptr finalBitRangeStart = wasBitRangeStartSimplified ? std::make_shared<syrec::Number>(*bitRangeStartSimplified->tryTakeOwnershipOfOptimizationResult()->front()) : signalAccess.range->first;
            const syrec::Number::ptr finalBitRangeEnd   = wasBitRangeEndSimplified ? std::make_shared<syrec::Number>(*bitRangeEndSimplified->tryTakeOwnershipOfOptimizationResult()->front()) : signalAccess.range->second;
            simplifiedBitRangeAccess                    = std::make_pair(finalBitRangeStart, finalBitRangeEnd);
        }

        auto simplifiedSignalAccess = std::make_shared<syrec::VariableAccess>(signalAccess);
        if (wasAnyValueOfDimensionSimplified || wasBitRangeStartSimplified || wasBitRangeEndSimplified) {
            simplifiedSignalAccess = syrec::VariableAccess::ptr();
            simplifiedSignalAccess->var       = signalAccess.var;
            simplifiedSignalAccess->range     = simplifiedBitRangeAccess;

            if (wasAnyValueOfDimensionSimplified) {
                syrec::expression::vec simplifiedDimensionAccesses(signalAccess.indexes.size(), nullptr);
                for (std::size_t i = 0; i < simplifiedDimensionAccesses.size(); ++i) {
                    if (simplifiedValuePerDimensionAccess.at(i).getStatusOfResult() == OptimizationResultFlag::WasOptimized) {
                        simplifiedDimensionAccesses.push_back(std::move(simplifiedValuePerDimensionAccess.at(i).tryTakeOwnershipOfOptimizationResult()->front()));
                    }
                    else {
                        simplifiedDimensionAccesses.push_back(signalAccess.indexes.at(i));
                    }
                }
                simplifiedSignalAccess->indexes = simplifiedDimensionAccesses;
            }
            else {
                simplifiedSignalAccess->indexes = signalAccess.indexes;
            }
        }

        bool        skipReferenceCountUpdate        = false;
        const auto& evaluatedSimplifiedSignalAccess = *tryEvaluateDefinedSignalAccess(*simplifiedSignalAccess);
        if (performConstantPropagationForAccessedSignal && fetchedValue != nullptr) {
            if (const auto& fetchedValueForSimplifiedSignalAccess = tryFetchValueFromEvaluatedSignalAccess(evaluatedSimplifiedSignalAccess); fetchedValueForSimplifiedSignalAccess.has_value()) {
                *fetchedValue = *fetchedValueForSimplifiedSignalAccess;
                skipReferenceCountUpdate = true;
            }
        }

        if (!skipReferenceCountUpdate) {
            updateReferenceCountOf(simplifiedSignalAccess->var->name, parser::SymbolTable::ReferenceCountUpdate::Increment);
        }

        if (performConstantPropagationForAccessedSignal && fetchedValue != nullptr && fetchedValue->has_value()) {
            return OptimizationResult<syrec::VariableAccess>::asOptimizedAwayContainer();
        }

        if (wasAnyValueOfDimensionSimplified || wasBitRangeStartSimplified || wasBitRangeEndSimplified) {
            return OptimizationResult<syrec::VariableAccess>::fromOptimizedContainer(std::make_unique<syrec::VariableAccess>(*simplifiedSignalAccess));
        }
        return OptimizationResult<syrec::VariableAccess>::asUnchangedOriginal();
    }
    updateReferenceCountOf(signalAccess.var->name, parser::SymbolTable::ReferenceCountUpdate::Increment);
    return OptimizationResult<syrec::VariableAccess>::asUnchangedOriginal();
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
