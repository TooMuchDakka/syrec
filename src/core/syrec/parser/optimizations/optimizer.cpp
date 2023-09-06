#include "core/syrec/parser/optimizations/optimizer.hpp"

#include "core/syrec/parser/range_check.hpp"

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

optimizations::Optimizer::OptimizationResult<syrec::Statement> optimizations::Optimizer::handleAssignmentStmt(const syrec::AssignStatement& assignmentStmt) {
    return OptimizationResult<syrec::Statement>::asUnchangedOriginal();
}

optimizations::Optimizer::OptimizationResult<syrec::Statement> optimizations::Optimizer::handleCallStmt(const syrec::CallStatement& callStatement) {
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

optimizations::Optimizer::OptimizationResult<syrec::Statement> optimizations::Optimizer::handleSwapStmt(const syrec::SwapStatement& swapStatement) {
    return OptimizationResult<syrec::Statement>::asUnchangedOriginal();
}

optimizations::Optimizer::OptimizationResult<syrec::Statement> optimizations::Optimizer::handleUnaryStmt(const syrec::UnaryStatement& unaryStmt) {
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
    return OptimizationResult<syrec::expression>::asUnchangedOriginal();
}

optimizations::Optimizer::OptimizationResult<syrec::expression> optimizations::Optimizer::handleNumericExpr(const syrec::NumericExpression& numericExpr) const {
    return OptimizationResult<syrec::expression>::asUnchangedOriginal();
}

optimizations::Optimizer::OptimizationResult<syrec::expression> optimizations::Optimizer::handleVariableExpr(const syrec::VariableExpression& expression) const {
    return OptimizationResult<syrec::expression>::asUnchangedOriginal();
}

optimizations::Optimizer::OptimizationResult<syrec::expression> optimizations::Optimizer::handleShiftExpr(const syrec::ShiftExpression& expression) const {
    return OptimizationResult<syrec::expression>::asUnchangedOriginal();
}

optimizations::Optimizer::OptimizationResult<syrec::Number> optimizations::Optimizer::handleNumber(const syrec::Number& number) const {
    if (number.isConstant()) {
        return OptimizationResult<syrec::Number>::asUnchangedOriginal();
    }
    if (number.isLoopVariable()) {
        return OptimizationResult<syrec::Number>::asUnchangedOriginal();
    }
    if (number.isCompileTimeConstantExpression()) {
        return OptimizationResult<syrec::Number>::asUnchangedOriginal();
    }
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

void optimizations::Optimizer::updateReferenceCountOf(const std::string_view& signalIdent, ReferenceCountUpdate typeOfUpdate) const {
    if (typeOfUpdate == ReferenceCountUpdate::Increment) {
        symbolTable->incrementLiteralReferenceCount(signalIdent);
    } else if (typeOfUpdate == ReferenceCountUpdate::Decrement) {
        symbolTable->decrementLiteralReferenceCount(signalIdent);
    }
}

std::optional<unsigned> optimizations::Optimizer::tryFetchValueForAccessedSignal(const syrec::VariableAccess& accessedSignal) const {
    if (!parserConfig.performConstantPropagation) {
        return std::nullopt;
    }

    std::vector<std::reference_wrapper<const syrec::expression>> accessedValueOfDimensionWithoutOwnership;
    accessedValueOfDimensionWithoutOwnership.reserve(accessedSignal.indexes.size());

    for (std::size_t i = 0; i < accessedValueOfDimensionWithoutOwnership.size(); ++i) {
        accessedValueOfDimensionWithoutOwnership.emplace_back(*accessedSignal.indexes.at(i));
    }

    const auto& evaluatedDimensionAccess = tryEvaluateUserDefinedDimensionAccess(accessedSignal.var->name, accessedValueOfDimensionWithoutOwnership);
    const auto  isEveryValueOfDimensionKnownAndValid = evaluatedDimensionAccess.has_value() && !evaluatedDimensionAccess->wasTrimmed
        && std::all_of(
            evaluatedDimensionAccess->valuePerDimension.cbegin(), 
            evaluatedDimensionAccess->valuePerDimension.cend(),
            [](const std::pair<IndexValidityStatus, std::optional<unsigned int>>& valueOfDimensionEvaluationResult) {
                    return valueOfDimensionEvaluationResult.first == IndexValidityStatus::Valid && valueOfDimensionEvaluationResult.second.has_value();
    });

   
    std::optional<std::pair<std::reference_wrapper<syrec::Number>, std::reference_wrapper<syrec::Number>>> transformedBitRangeAccess;
    if (accessedSignal.range.has_value()) {
        transformedBitRangeAccess.emplace(std::make_pair<std::reference_wrapper<syrec::Number>, std::reference_wrapper<syrec::Number>>(*accessedSignal.range->first, *accessedSignal.range->second));
    }

    const auto& evaluatedBitRangeAccess = tryEvaluateUserDefinedBitRangeAccess(accessedSignal.var->name, transformedBitRangeAccess);
    const auto areBitRangeComponentsKnownAndValid = evaluatedBitRangeAccess.has_value()
        ? evaluatedBitRangeAccess->rangeStartEvaluationResult.first == IndexValidityStatus::Valid && evaluatedBitRangeAccess->rangeEndEvaluationResult.first == IndexValidityStatus::Valid
        : true;

    if (!isEveryValueOfDimensionKnownAndValid || !areBitRangeComponentsKnownAndValid) {
        return std::nullopt;
    }

    return symbolTable->tryFetchValueForLiteral(std::make_shared<syrec::VariableAccess>(accessedSignal));
}

std::optional<optimizations::Optimizer::DimensionAccessEvaluationResult> optimizations::Optimizer::tryEvaluateUserDefinedDimensionAccess(const std::string_view& accessedSignalIdent, const std::vector<std::reference_wrapper<const syrec::expression>>& accessedValuePerDimension) const {
    const auto& symbolTableEntryForAccessedSignal = symbolTable->getVariable(accessedSignalIdent);
    if (!symbolTableEntryForAccessedSignal.has_value() || !std::holds_alternative<syrec::Variable::ptr>(*symbolTableEntryForAccessedSignal)) {
        return std::nullopt;
    }

    const auto& signalData = std::get<syrec::Variable::ptr>(*symbolTableEntryForAccessedSignal);
    if (accessedValuePerDimension.empty()) {
        return std::make_optional(DimensionAccessEvaluationResult({false, std::vector<std::pair<IndexValidityStatus, std::optional<unsigned int>>>()}));
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
    return std::make_optional(DimensionAccessEvaluationResult({wasUserDefinedAccessTrimmed, evaluatedValuePerDimension}));
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
    auto numberEvaluated = handleNumber(number);
    if (const auto& optimizationResultOfNumber = numberEvaluated.tryTakeOwnershipOfOptimizationResult(); numberEvaluated.getStatusOfResult() == OptimizationResultFlag::WasOptimized && optimizationResultOfNumber.has_value() && optimizationResultOfNumber->front()->isConstant()) {
        return std::make_optional(optimizationResultOfNumber->front()->evaluate({}));
    }
    return std::nullopt;
}

std::optional<optimizations::Optimizer::BitRangeEvaluationResult> optimizations::Optimizer::tryEvaluateUserDefinedBitRangeAccess(const std::string_view& accessedSignalIdent, const std::optional<std::pair<std::reference_wrapper<const syrec::Number>, std::reference_wrapper<const syrec::Number>>>& accessedBitRange) const {
    const auto& symbolTableEntryForAccessedSignal = symbolTable->getVariable(accessedSignalIdent);
    if (!symbolTableEntryForAccessedSignal.has_value() || !std::holds_alternative<syrec::Variable::ptr>(*symbolTableEntryForAccessedSignal)) {
        return std::nullopt;
    }

    const auto& signalData = std::get<syrec::Variable::ptr>(*symbolTableEntryForAccessedSignal);
    if (!accessedBitRange.has_value()) {
        return std::make_optional(BitRangeEvaluationResult(IndexValidityStatus::Valid, std::make_optional(0), IndexValidityStatus::Valid, std::make_optional(signalData->bitwidth - 1)));
    }

    const auto& accessedBitRangeEvaluated = tryEvaluateBitRangeAccessComponents(*accessedBitRange);
    if (accessedBitRangeEvaluated.has_value()) {
        const auto& isBitRangeStartOutOfRange            = !parser::isValidBitAccess(signalData, accessedBitRangeEvaluated->first);
        const auto& isBitRangeEndOutOfRange              = !parser::isValidBitAccess(signalData, accessedBitRangeEvaluated->second);
        return std::make_optional(BitRangeEvaluationResult(
                isBitRangeStartOutOfRange ? IndexValidityStatus::OutOfRange : IndexValidityStatus::Valid, accessedBitRangeEvaluated->first,
                isBitRangeEndOutOfRange ? IndexValidityStatus::OutOfRange : IndexValidityStatus::Valid, accessedBitRangeEvaluated->second));
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
    return std::make_optional(BitRangeEvaluationResult(bitRangeStartEvaluationStatus, bitRangeStartEvaluated, bitRangeEndEvaluationStatus, bitRangeEndEvaluated));
}

std::optional<unsigned> optimizations::Optimizer::tryEvaluateExpressionToConstant(const syrec::expression& expr) const {
    if (auto exprOptimizationResult = handleExpr(expr); exprOptimizationResult.getStatusOfResult() == OptimizationResultFlag::WasOptimized) {
        const auto& optimizedExpr = exprOptimizationResult.tryTakeOwnershipOfOptimizationResult();
        if (optimizedExpr.has_value() && optimizedExpr->size() == 1 && dynamic_cast<syrec::NumericExpression*>(optimizedExpr->front().get()) != nullptr) {
            const auto& optimizedExprAsNumericExpr = static_cast<syrec::NumericExpression*>(optimizedExpr->front().get());
            return tryEvaluateNumberAsConstant(*optimizedExprAsNumericExpr->value);
        }
    }
    return std::nullopt;
}