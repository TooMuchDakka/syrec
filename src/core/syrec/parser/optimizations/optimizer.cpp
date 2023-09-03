#include "core/syrec/parser/optimizations/optimizer.hpp"

#include "core/syrec/parser/range_check.hpp"

std::vector<std::unique_ptr<syrec::Module>> optimizations::Optimizer::optimizeProgram(const std::vector<std::reference_wrapper<syrec::Module>>& modules) {
    std::vector<std::unique_ptr<syrec::Module>> optimizedModules;
    for (const auto& module : modules) {
        if (auto&& optimizedModule = handleModule(module); optimizedModule != nullptr) {
            optimizedModules.emplace_back(std::move(optimizedModule));
        }
    }
    return optimizedModules;
}

std::unique_ptr<syrec::Module> optimizations::Optimizer::handleModule(const syrec::Module& module) {
    return std::make_unique<syrec::Module>(module);
}

optimizations::Optimizer::ListOfStatementReferences optimizations::Optimizer::handleStatements(const std::vector<std::reference_wrapper<syrec::Statement>>& statements) {
    ListOfStatementReferences optimizedStatements;
    for (const auto& statement : statements) {
        if (auto&& optimizedStatement = handleStatement(statement); !optimizedStatement.empty()) {
            std::move(optimizedStatement.begin(), optimizedStatement.end(), optimizedStatements.end());
        }
    }
    return optimizedStatements;
}

// TODO: Refactoring using visitor pattern
optimizations::Optimizer::ListOfStatementReferences optimizations::Optimizer::handleStatement(const syrec::Statement& stmt) {
    if (typeid(stmt) == typeid(syrec::SkipStatement)) {
        if (auto&& optimizedSkipStmt = handleSkipStmt(stmt); optimizedSkipStmt != nullptr) {
            ListOfStatementReferences stmtContainer;
            stmtContainer.emplace_back(std::make_unique<syrec::SkipStatement>());
            return stmtContainer;
        } 
    }

    if (const auto& statementAsAssignmentStmt = dynamic_cast<const syrec::AssignStatement*>(&stmt); statementAsAssignmentStmt != nullptr) {
        return handleAssignmentStmt(*statementAsAssignmentStmt);
    }
    if (const auto& statementAsUnaryAssignmentStmt = dynamic_cast<const syrec::UnaryStatement*>(&stmt); statementAsUnaryAssignmentStmt != nullptr) {
        if (auto&& simplifiedUnaryAssignmentStmt = handleUnaryStmt(*statementAsUnaryAssignmentStmt); simplifiedUnaryAssignmentStmt != nullptr) {
            ListOfStatementReferences stmtContainer;
            stmtContainer.emplace_back(std::make_unique<syrec::UnaryStatement>(statementAsUnaryAssignmentStmt->op, statementAsUnaryAssignmentStmt->var));
            return stmtContainer;
        }
    }
    if (const auto& statementAsIfStatement = dynamic_cast<const syrec::IfStatement*>(&stmt); statementAsIfStatement != nullptr) {
        return handleIfStmt(*statementAsIfStatement);
    }
    if (const auto& statementAsLoopStatement = dynamic_cast<const syrec::ForStatement*>(&stmt); statementAsLoopStatement != nullptr) {
        return handleLoopStmt(*statementAsLoopStatement);
    }
    if (const auto& statementAsSwapStatement = dynamic_cast<const syrec::SwapStatement*>(&stmt); statementAsSwapStatement != nullptr) {
        ListOfStatementReferences stmtContainer;
        stmtContainer.emplace_back(std::make_unique<syrec::SwapStatement>(statementAsSwapStatement->lhs, statementAsSwapStatement->rhs));
        return stmtContainer;
    }
    if (const auto& statementAsCallStatement = dynamic_cast<const syrec::CallStatement*>(&stmt); statementAsCallStatement != nullptr) {
        return handleCallStmt(*statementAsCallStatement);
    }
    /*if (const auto& statementAsUncallStatement = dynamic_cast<const syrec::UncallStatement*>(&stmt); statementAsUncallStatement != nullptr) {
        
    }*/
    return {};
}

optimizations::Optimizer::ListOfStatementReferences optimizations::Optimizer::handleAssignmentStmt(const syrec::AssignStatement& assignmentStmt) {
    return {};
}

std::unique_ptr<syrec::Statement> optimizations::Optimizer::handleUnaryStmt(const syrec::UnaryStatement& unaryStmt) {
    return nullptr;
}

optimizations::Optimizer::ListOfStatementReferences optimizations::Optimizer::handleIfStmt(const syrec::IfStatement& ifStatement) {
    return {};
}

optimizations::Optimizer::ListOfStatementReferences optimizations::Optimizer::handleLoopStmt(const syrec::ForStatement& forStatement) {
    return {};
}

std::unique_ptr<syrec::Statement> optimizations::Optimizer::handleSwapStmt(const syrec::SwapStatement& swapStatement) {
    return nullptr;
}

std::unique_ptr<syrec::Statement> optimizations::Optimizer::handleSkipStmt(const syrec::SkipStatement& skipStatement) {
    return nullptr;
}

optimizations::Optimizer::ListOfStatementReferences optimizations::Optimizer::handleCallStmt(const syrec::CallStatement& callStatement) {
    return {};
}

std::unique_ptr<syrec::expression> optimizations::Optimizer::handleExpr(const syrec::expression& expression) const {
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
    return nullptr;
    //return std::make_unique<syrec::expression>(expression);
}

std::unique_ptr<syrec::expression> optimizations::Optimizer::handleBinaryExpr(const syrec::BinaryExpression& expression) const {
    return nullptr;
}

std::unique_ptr<syrec::expression> optimizations::Optimizer::handleShiftExpr(const syrec::ShiftExpression& expression) const {
    return nullptr;
}

std::unique_ptr<syrec::NumericExpression> optimizations::Optimizer::handleNumericExpr(const syrec::NumericExpression& numericExpr) const {
    return nullptr;
}

std::unique_ptr<syrec::expression> optimizations::Optimizer::handleVariableExpr(const syrec::VariableExpression& expression) const {
    return nullptr;
}

std::unique_ptr<syrec::Number> optimizations::Optimizer::handleNumber(const syrec::Number& number) const {
    if (number.isConstant()) {
        return std::make_unique<syrec::Number>(number);
    }
    if (number.isLoopVariable()) {
        return std::make_unique<syrec::Number>(number);
    }
    if (number.isCompileTimeConstantExpression()) {
        return std::make_unique<syrec::Number>(number);
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
            [](const std::pair<DimensionAccessEvaluationResult::ValueOfDimensionValidity, DimensionAccessEvaluationResult::ValueOfDimension>& valueOfDimensionEvaluationResult) {
                    return valueOfDimensionEvaluationResult.first == DimensionAccessEvaluationResult::ValueOfDimensionValidity::Valid;
    });

   
    std::optional<std::pair<std::reference_wrapper<syrec::Number>, std::reference_wrapper<syrec::Number>>> transformedBitRangeAccess;
    if (accessedSignal.range.has_value()) {
        transformedBitRangeAccess.emplace(std::make_pair<std::reference_wrapper<syrec::Number>, std::reference_wrapper<syrec::Number>>(*accessedSignal.range->first, *accessedSignal.range->second));
    }

    const auto& evaluatedBitRangeAccess = tryEvaluateUserDefinedBitRangeAccess(accessedSignal.var->name, transformedBitRangeAccess);
    const auto areBitRangeComponentsKnownAndValid = evaluatedBitRangeAccess.has_value()
        ? evaluatedBitRangeAccess->rangeStartEvaluationResult.first == BitRangeEvaluationResult::Valid && evaluatedBitRangeAccess->rangeEndEvaluationResult.first == BitRangeEvaluationResult::Valid
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
        return std::make_optional(DimensionAccessEvaluationResult());
    }

    const auto wasUserDefinedAccessTrimmed = accessedValuePerDimension.size() > signalData->dimensions.size();
    const auto numEntriesToTransform       = wasUserDefinedAccessTrimmed ? signalData->dimensions.size() : accessedValuePerDimension.size();

    DimensionAccessEvaluationResult result;
    result.valuePerDimension.reserve(numEntriesToTransform);

    for (std::size_t i = 0; i < numEntriesToTransform; ++i) {
        const auto& userDefinedValueOfDimension = accessedValuePerDimension.at(i);
        if (const auto& valueOfDimensionEvaluated   = tryEvaluateExpressionToConstant(userDefinedValueOfDimension); valueOfDimensionEvaluated.has_value()) {
            const auto isAccessedValueOfDimensionWithinRange = parser::isValidDimensionAccess(signalData, i, *valueOfDimensionEvaluated);
            result.valuePerDimension.emplace_back(isAccessedValueOfDimensionWithinRange ? DimensionAccessEvaluationResult::ValueOfDimensionValidity::Valid : DimensionAccessEvaluationResult::ValueOfDimensionValidity::OutOfRange, *valueOfDimensionEvaluated);
        } else {
            result.valuePerDimension.emplace_back(DimensionAccessEvaluationResult::ValueOfDimensionValidity::Unknown, createCopyOfExpr(userDefinedValueOfDimension));
        }
    }
    return std::make_optional(result);
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
    const auto& numberEvaluated = handleNumber(number);
    return numberEvaluated != nullptr && numberEvaluated->isConstant() ? std::make_optional(numberEvaluated->evaluate({})) : std::nullopt;
}

std::optional<optimizations::Optimizer::BitRangeEvaluationResult> optimizations::Optimizer::tryEvaluateUserDefinedBitRangeAccess(const std::string_view& accessedSignalIdent, const std::optional<std::pair<std::reference_wrapper<const syrec::Number>, std::reference_wrapper<const syrec::Number>>>& accessedBitRange) const {
    const auto& symbolTableEntryForAccessedSignal = symbolTable->getVariable(accessedSignalIdent);
    if (!symbolTableEntryForAccessedSignal.has_value() || !std::holds_alternative<syrec::Variable::ptr>(*symbolTableEntryForAccessedSignal)) {
        return std::nullopt;
    }

    const auto& signalData = std::get<syrec::Variable::ptr>(*symbolTableEntryForAccessedSignal);
    if (!accessedBitRange.has_value()) {
        return std::make_optional(BitRangeEvaluationResult(BitRangeEvaluationResult::Valid, std::make_optional(0), BitRangeEvaluationResult::Valid, std::make_optional(signalData->bitwidth - 1)));
    }

    const auto& accessedBitRangeEvaluated = tryEvaluateBitRangeAccessComponents(*accessedBitRange);
    if (accessedBitRangeEvaluated.has_value()) {
        const auto& isBitRangeStartOutOfRange            = !parser::isValidBitAccess(signalData, accessedBitRangeEvaluated->first);
        const auto& isBitRangeEndOutOfRange              = !parser::isValidBitAccess(signalData, accessedBitRangeEvaluated->second);
        return std::make_optional(BitRangeEvaluationResult(
                isBitRangeStartOutOfRange ? BitRangeEvaluationResult::OutOfRange : BitRangeEvaluationResult::Valid, accessedBitRangeEvaluated->first,
                isBitRangeEndOutOfRange ? BitRangeEvaluationResult::OutOfRange : BitRangeEvaluationResult::Valid, accessedBitRangeEvaluated->second));
    }

    const auto& bitRangeStartEvaluated = tryEvaluateNumberAsConstant(accessedBitRange->first);
    const auto& bitRangeEndEvaluated   = tryEvaluateNumberAsConstant(accessedBitRange->second);

    auto bitRangeStartEvaluationStatus = BitRangeEvaluationResult::Unknown;
    if (bitRangeStartEvaluated.has_value()) {
        bitRangeStartEvaluationStatus = parser::isValidBitAccess(signalData, *bitRangeStartEvaluated) ? BitRangeEvaluationResult::Valid : BitRangeEvaluationResult::OutOfRange;
    }
    auto bitRangeEndEvaluationStatus = BitRangeEvaluationResult::Unknown;
    if (bitRangeEndEvaluated.has_value()) {
        bitRangeEndEvaluationStatus = parser::isValidBitAccess(signalData, *bitRangeEndEvaluated) ? BitRangeEvaluationResult::Valid : BitRangeEvaluationResult::OutOfRange;
    }
    return std::make_optional(BitRangeEvaluationResult(bitRangeStartEvaluationStatus, bitRangeStartEvaluated, bitRangeEndEvaluationStatus, bitRangeEndEvaluated));
}

std::optional<unsigned> optimizations::Optimizer::tryEvaluateExpressionToConstant(const syrec::expression& expr) const {
    if (const auto& optimizedExpr = handleExpr(expr); optimizedExpr != nullptr && dynamic_cast<syrec::NumericExpression*>(optimizedExpr.get()) != nullptr) {
        const auto& optimizedExprAsNumericExpr = static_cast<syrec::NumericExpression*>(optimizedExpr.get());
        return tryEvaluateNumberAsConstant(*optimizedExprAsNumericExpr->value);
    }
    return std::nullopt;
}

std::unique_ptr<syrec::expression> optimizations::Optimizer::createCopyOfExpr(const syrec::expression& expr) const {
    return nullptr;
}
