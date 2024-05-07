#include "core/syrec/parser/utils/signal_access_utils.hpp"
#include "core/syrec/expression.hpp"
#include "core/syrec/parser/operation.hpp"

using namespace SignalAccessUtils;

std::optional<unsigned int> tryFetchValueOfExpr(const syrec::Expression& expr, const parser::SymbolTable& symbolTable) {
    if (const auto& exprAsNumericExpr = dynamic_cast<const syrec::NumericExpression*>(&expr); exprAsNumericExpr != nullptr) {
        return tryEvaluateNumber(*exprAsNumericExpr->value, symbolTable);
    }
    if (const auto& exprAsVariableExpr = dynamic_cast<const syrec::VariableExpression*>(&expr); exprAsVariableExpr != nullptr) {
        return symbolTable.tryFetchValueForLiteral(exprAsVariableExpr->var);
    }
    if (const auto& exprAsBinaryExpr = dynamic_cast<const syrec::BinaryExpression*>(&expr); exprAsBinaryExpr != nullptr) {
        const auto& lhsEvaluated = tryFetchValueOfExpr(*exprAsBinaryExpr->lhs, symbolTable);
        const auto& rhsEvaluated = tryFetchValueOfExpr(*exprAsBinaryExpr->rhs, symbolTable);
        const auto& mappedOperationFlagToEnum = syrec_operation::tryMapBinaryOperationFlagToEnum(exprAsBinaryExpr->op);

        if (!mappedOperationFlagToEnum.has_value()) {
            return std::nullopt;
        }
        return syrec_operation::apply(*mappedOperationFlagToEnum, lhsEvaluated, rhsEvaluated);
    }
    if (const auto& exprAsShiftExpr = dynamic_cast<const syrec::ShiftExpression*>(&expr); exprAsShiftExpr != nullptr) {
        const auto& lhsEvaluated = tryFetchValueOfExpr(*exprAsShiftExpr->lhs, symbolTable);
        const auto& rhsEvaluated = tryEvaluateNumber(*exprAsShiftExpr->rhs, symbolTable);
        const auto& mappedOperationFlagToEnum = syrec_operation::tryMapShiftOperationFlagToEnum(exprAsShiftExpr->op);

        if (!mappedOperationFlagToEnum.has_value()) {
            return std::nullopt;
        }
        if (lhsEvaluated.has_value() && !*lhsEvaluated) {
            return std::make_optional(0);
        }
        if (!lhsEvaluated.has_value() || !rhsEvaluated.has_value()) {
            return std::nullopt;
        }
        return syrec_operation::apply(*syrec_operation::tryMapShiftOperationFlagToEnum(exprAsShiftExpr->op), *lhsEvaluated, *rhsEvaluated);
    }
    return std::nullopt;
}

std::optional<syrec_operation::operation> tryMapCompileTimeConstantOperationToEnum(syrec::Number::CompileTimeConstantExpression::Operation compileTimeConstantExprOperation) {
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

std::pair<std::optional<unsigned int>, std::optional<unsigned int>> tryEvaluateBitRangeAccess(const syrec::VariableAccess& signalAccess, const parser::SymbolTable& symbolTable) {
    if (!signalAccess.range.has_value()) {
        if (const auto& symbolTableEntryForAccessedSignal = symbolTable.getVariable(signalAccess.var->name); symbolTableEntryForAccessedSignal.has_value() && std::holds_alternative<syrec::Variable::ptr>(*symbolTableEntryForAccessedSignal)) {
            return std::make_pair(std::make_optional(0), std::make_optional(std::get<syrec::Variable::ptr>(*symbolTableEntryForAccessedSignal)->bitwidth - 1));
        }
        return std::make_pair(std::nullopt, std::nullopt);
    }
    return std::make_pair(tryEvaluateNumber(*signalAccess.range->first, symbolTable), tryEvaluateNumber(*signalAccess.range->second, symbolTable));
}

bool areBitRangesEqualAccordingToCriteria(const std::pair<unsigned int, unsigned int>& thisBitRangeAccess, const std::pair<unsigned int, unsigned int>& thatBitRangeAccess, SignalAccessComponentEquivalenceCriteria::BitRange expectedBitRangeEquivalence) {
    switch (expectedBitRangeEquivalence) {
        case SignalAccessComponentEquivalenceCriteria::BitRange::Enclosed:
            return thisBitRangeAccess.first >= thatBitRangeAccess.first && thisBitRangeAccess.second <= thatBitRangeAccess.second;
        case SignalAccessComponentEquivalenceCriteria::BitRange::Overlapping: {
            const bool doesAssignedToBitRangePrecedeAccessedBitRange = thisBitRangeAccess.first < thatBitRangeAccess.first;
            const bool doesAssignedToBitRangeExceedAccessedBitRange  = thisBitRangeAccess.second > thatBitRangeAccess.second;
            /*
            * Check cases:
            * Assigned TO: |---|
            * Accessed BY:      |---|
            *
            * and
            * Assigned TO:      |---|
            * Accessed BY: |---|
            *
            */
            if ((doesAssignedToBitRangePrecedeAccessedBitRange && thisBitRangeAccess.second < thatBitRangeAccess.first) || (doesAssignedToBitRangeExceedAccessedBitRange && thisBitRangeAccess.first > thatBitRangeAccess.second)) {
                return false;
            }
            return true;       
        }
        case SignalAccessComponentEquivalenceCriteria::BitRange::Equal:
            return thisBitRangeAccess.first == thatBitRangeAccess.second && thisBitRangeAccess.second == thatBitRangeAccess.second;
        default:
            return false;
    }
}

std::optional<std::vector<unsigned int>> tryEvaluateDimensionAccess(const syrec::Expression::vec& definedDimensionAccess, const parser::SymbolTable& symbolTable){
    std::vector<unsigned int> evaluatedDimensionAccess(definedDimensionAccess.size(), 0);
    for (std::size_t i = 0; i < definedDimensionAccess.size(); ++i) {
        if (const auto& valueOfDimensionEvaluated = tryFetchValueOfExpr(*definedDimensionAccess.at(i), symbolTable); valueOfDimensionEvaluated.has_value()) {
            evaluatedDimensionAccess.at(i) = *valueOfDimensionEvaluated;
        } else {
            return std::nullopt;
        }
    }
    return evaluatedDimensionAccess;
}

std::optional<unsigned int> SignalAccessUtils::tryEvaluateNumber(const syrec::Number& number, const parser::SymbolTable& symbolTable) {
    if (number.isConstant()) {
        return std::make_optional(number.evaluate({}));
    }
    if (number.isLoopVariable()) {
        return symbolTable.tryFetchValueOfLoopVariable(number.variableName());
    }
    if (number.isCompileTimeConstantExpression()) {
        return SignalAccessUtils::tryEvaluateCompileTimeConstantExpression(number.getExpression(), symbolTable);
    }
    return std::nullopt;
}

std::optional<unsigned int> SignalAccessUtils::tryEvaluateCompileTimeConstantExpression(const syrec::Number::CompileTimeConstantExpression& compileTimeConstantExpression, const parser::SymbolTable& symbolTable) {
    const auto& evaluationResultOfLhsOperand = tryEvaluateNumber(*compileTimeConstantExpression.lhsOperand, symbolTable);
    const auto& evaluationResultOfRhsOperand = tryEvaluateNumber(*compileTimeConstantExpression.rhsOperand, symbolTable);
    const auto& mappedToBinaryOperation      = tryMapCompileTimeConstantOperationToEnum(compileTimeConstantExpression.operation);

    if (!mappedToBinaryOperation.has_value()) {
        return std::nullopt;
    }
    return syrec_operation::apply(*mappedToBinaryOperation, evaluationResultOfLhsOperand, evaluationResultOfRhsOperand);
}

SignalAccessEquivalenceResult SignalAccessUtils::areAccessedBitRangesEqual(const syrec::VariableAccess& accessedSignalParts, const syrec::VariableAccess& referenceSignalAccess, SignalAccessComponentEquivalenceCriteria::BitRange expectedBitRangeEquivalenceCriteria, const parser::SymbolTable& symbolTable) {
    if (accessedSignalParts.var->name != referenceSignalAccess.var->name) {
        return SignalAccessEquivalenceResult(SignalAccessEquivalenceResult::Equality::NotEqual, true);
    }

    const auto& accessedBitRange  = tryEvaluateBitRangeAccess(accessedSignalParts, symbolTable);
    const auto& referenceBitRange = tryEvaluateBitRangeAccess(referenceSignalAccess, symbolTable);

    const auto couldDetermineBitRangeOfAccessedSignal = accessedBitRange.first.has_value() && accessedBitRange.second.has_value();
    const auto couldDetermineBitRangeOfReferenceSignal = referenceBitRange.first.has_value() && referenceBitRange.second.has_value();
    if (!couldDetermineBitRangeOfAccessedSignal || !couldDetermineBitRangeOfReferenceSignal) {
        return SignalAccessEquivalenceResult(SignalAccessEquivalenceResult::Equality::Maybe, false);
    }

    const auto areBitRangesEqual = areBitRangesEqualAccordingToCriteria(
            std::make_pair(*accessedBitRange.first, *accessedBitRange.second),
            std::make_pair(*referenceBitRange.first, *referenceBitRange.second),
            expectedBitRangeEquivalenceCriteria
    );
    return SignalAccessEquivalenceResult(areBitRangesEqual ? SignalAccessEquivalenceResult::Equality::Equal : SignalAccessEquivalenceResult::Equality::NotEqual, true);
}

SignalAccessEquivalenceResult SignalAccessUtils::areDimensionAccessesEqual(const syrec::VariableAccess& accessedSignalParts, const syrec::VariableAccess& referenceSignalAccess, SignalAccessComponentEquivalenceCriteria::DimensionAccess dimensionAccessEquivalenceCriteria, const parser::SymbolTable& symbolTable) {
    if (accessedSignalParts.var->name != referenceSignalAccess.var->name) {
        return SignalAccessEquivalenceResult(SignalAccessEquivalenceResult::Equality::NotEqual, true);
    }

    if (accessedSignalParts.indexes.empty() && referenceSignalAccess.indexes.empty()) {
        return SignalAccessEquivalenceResult(SignalAccessEquivalenceResult::Equality::Equal, true);
    }

    if (accessedSignalParts.indexes.size() != referenceSignalAccess.indexes.size() && dimensionAccessEquivalenceCriteria == SignalAccessComponentEquivalenceCriteria::DimensionAccess::Equal) {
        if (const auto& symbolTableEntryForAccessedSignal = symbolTable.getVariable(accessedSignalParts.var->name); symbolTableEntryForAccessedSignal.has_value() && std::holds_alternative<syrec::Variable::ptr>(*symbolTableEntryForAccessedSignal)) {
            
        }
        return SignalAccessEquivalenceResult(SignalAccessEquivalenceResult::Equality::NotEqual, true);
    }

    const auto& accessedSignalDimensionAccessEvaluated = tryEvaluateDimensionAccess(accessedSignalParts.indexes, symbolTable);
    const auto& referenceSignalDimensionAccessEvaluated = tryEvaluateDimensionAccess(referenceSignalAccess.indexes, symbolTable);
    if (accessedSignalDimensionAccessEvaluated.has_value() ^ referenceSignalDimensionAccessEvaluated.has_value()) {
        return SignalAccessEquivalenceResult(SignalAccessEquivalenceResult::Equality::Maybe, false);
    }

    /*
     * Handling omitting dimension access and specifying redundant access on first dimension for 1-D signal with only one value for the first dimension defined (i.e. a[1])
     */
    if ((accessedSignalDimensionAccessEvaluated->empty() && referenceSignalDimensionAccessEvaluated->size() == 1 && referenceSignalDimensionAccessEvaluated->front() == 0)
        || (referenceSignalDimensionAccessEvaluated->empty() && accessedSignalDimensionAccessEvaluated->size() == 1 && accessedSignalDimensionAccessEvaluated->front() == 0)) {
        if (const auto& symbolTableEntryForAccessedSignal = symbolTable.getVariable(accessedSignalParts.var->name); symbolTableEntryForAccessedSignal.has_value() && std::holds_alternative<syrec::Variable::ptr>(*symbolTableEntryForAccessedSignal)) {
            if (std::get<syrec::Variable::ptr>(*symbolTableEntryForAccessedSignal)->dimensions.size() == 1) {
                return SignalAccessEquivalenceResult(SignalAccessEquivalenceResult::Equality::Equal, true);
            }
            return SignalAccessEquivalenceResult(SignalAccessEquivalenceResult::Equality::NotEqual, true);
        }
        return SignalAccessEquivalenceResult(SignalAccessEquivalenceResult::Equality::Maybe, false);
    }

    const auto areAllAccessedValuesOfDimensionEqual = std::find_first_of(
            accessedSignalDimensionAccessEvaluated->cbegin(),
            accessedSignalDimensionAccessEvaluated->cend(),
            referenceSignalDimensionAccessEvaluated->cbegin(),
            referenceSignalDimensionAccessEvaluated->cend(),
            [](const unsigned int thisValueOfDimension, const unsigned int thatValueOfDimension) {
                return thisValueOfDimension == thatValueOfDimension;
            }) != accessedSignalDimensionAccessEvaluated->cend();
    return SignalAccessEquivalenceResult(areAllAccessedValuesOfDimensionEqual ? SignalAccessEquivalenceResult::Equality::Equal : SignalAccessEquivalenceResult::Equality::NotEqual, true);
}

SignalAccessEquivalenceResult SignalAccessUtils::areSignalAccessesEqual(const syrec::VariableAccess& accessedSignalParts, const syrec::VariableAccess& referenceSignalAccess, SignalAccessComponentEquivalenceCriteria::DimensionAccess dimensionAccessEquivalenceCriteria, SignalAccessComponentEquivalenceCriteria::BitRange bitRangeEquivalenceCriteria, const parser::SymbolTable& symbolTable) {
    if (accessedSignalParts.var->name != referenceSignalAccess.var->name) {
        return SignalAccessEquivalenceResult(SignalAccessEquivalenceResult::Equality::NotEqual, true);
    }

    const auto dimensionAccessEquivalenceResult = areDimensionAccessesEqual(accessedSignalParts, referenceSignalAccess, dimensionAccessEquivalenceCriteria, symbolTable);
    if (dimensionAccessEquivalenceResult.equality != SignalAccessEquivalenceResult::Equality::Equal) {
        return dimensionAccessEquivalenceResult;
    }
    return areAccessedBitRangesEqual(accessedSignalParts, referenceSignalAccess, bitRangeEquivalenceCriteria, symbolTable);
}
