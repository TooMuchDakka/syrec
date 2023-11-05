#include "core/syrec/parser/optimizations/loop_optimizer.hpp"

#include "core/syrec/parser/operation.hpp"
#include "core/syrec/parser/utils/copy_utils.hpp"
#include "core/syrec/parser/utils/loop_range_utils.hpp"

// TODO: Returned loop header information should only contain simplified expression values if constant propagation is enabled ?
void optimizations::LoopUnroller::resetInternals(const LoopOptimizationConfig& newUnrollConfig, const syrec::Number::loop_variable_mapping& newKnownLoopVariableValueMappings) {
    knownLoopVariableValueMappings = newKnownLoopVariableValueMappings;
    unrollConfig                   = newUnrollConfig;
}

bool optimizations::LoopUnroller::canLoopBeFullyUnrolled(const LoopHeaderInformation& loopHeaderInformation) const {
    const auto& numberOfIterationsOfLoop = tryDetermineNumberOfLoopIterations(loopHeaderInformation);
    return unrollConfig.forceUnroll || (numberOfIterationsOfLoop.has_value() && (*numberOfIterationsOfLoop <= unrollConfig.maxNumberOfUnrolledIterationsPerLoop || (*numberOfIterationsOfLoop == 1 && unrollConfig.autoUnrollLoopsWithOneIteration)));
}

bool optimizations::LoopUnroller::doesCurrentLoopNestingLevelAllowUnroll(std::size_t currentLoopNestingLevel) const {
    return currentLoopNestingLevel <= unrollConfig.maxAllowedNestingLevelOfInnerLoops;
}

std::optional<optimizations::LoopUnroller::FullyUnrolledLoopInformation> optimizations::LoopUnroller::tryPerformFullLoopUnroll(const LoopHeaderInformation& loopHeader, const syrec::Statement::vec& loopBodyStatements) {
    const auto& numberOfIterations = tryDetermineNumberOfLoopIterations(loopHeader);
    if (!numberOfIterations.has_value()) {
        return std::nullopt;
    }

    auto copiesOfLoopBodyStatements = *numberOfIterations ? copyUtils::createCopyOfStatements(loopBodyStatements) : std::vector<std::unique_ptr<syrec::Statement>>();
    if (*numberOfIterations && copiesOfLoopBodyStatements.empty()) {
        return std::nullopt;
    }

    std::optional<LoopVariableValueInformation> loopVariableValueInformation;
    FullyUnrolledLoopInformation fullyUnrolledLoopInformation;
    fullyUnrolledLoopInformation.numUnrolledIterations = *numberOfIterations;

    if (loopHeader.loopVariableIdent.has_value() && *numberOfIterations) {
        std::vector<unsigned int> loopVariableValuePerIteration(*numberOfIterations, 0);
        loopVariableValuePerIteration[0] = loopHeader.initialLoopVariableValue;

        if (*numberOfIterations > 1) {
            for (std::size_t i = 1; i < loopVariableValuePerIteration.size(); ++i) {
                loopVariableValuePerIteration[i] = loopVariableValuePerIteration[i - 1] + loopHeader.stepSize;
            }   
        }
        loopVariableValueInformation = LoopVariableValueInformation({*loopHeader.loopVariableIdent, loopVariableValuePerIteration});
    }
    fullyUnrolledLoopInformation.unrolledIterations = UnrolledIterationInformation({std::move(copiesOfLoopBodyStatements), loopVariableValueInformation});
    return fullyUnrolledLoopInformation;
}

std::optional<optimizations::LoopUnroller::PartiallyUnrolledLoopInformation> optimizations::LoopUnroller::tryPerformPartialUnrollOfLoop(const std::size_t maxNumberOfUnrolledIterationsPerLoop, bool isLoopRemainderAllowed, const LoopHeaderInformation& loopHeader, const syrec::Statement::vec& loopBodyStatements) {
    const auto& numberOfIterations = tryDetermineNumberOfLoopIterations(loopHeader);
    if (!numberOfIterations.has_value() || !*numberOfIterations || *numberOfIterations <= maxNumberOfUnrolledIterationsPerLoop || (*numberOfIterations > maxNumberOfUnrolledIterationsPerLoop && !isLoopRemainderAllowed)) {
        return std::nullopt;
    }
    
    auto       copiesOfLoopBodyStatements      = *numberOfIterations ? copyUtils::createCopyOfStatements(loopBodyStatements) : std::vector<std::unique_ptr<syrec::Statement>>();
    if (*numberOfIterations && copiesOfLoopBodyStatements.empty()) {
        return std::nullopt;
    }

    auto copiesOfRemainingLoopBodyStatements = *numberOfIterations ? copyUtils::createCopyOfStatements(loopBodyStatements) : std::vector<std::unique_ptr<syrec::Statement>>();
    if (*numberOfIterations && copiesOfRemainingLoopBodyStatements.empty()) {
        return std::nullopt;
    }

    const auto                                  numberOfRemainingLoopIterations = *numberOfIterations - maxNumberOfUnrolledIterationsPerLoop;
    const auto                                  numberOfUnrolledLoopIterations  = *numberOfIterations - numberOfRemainingLoopIterations;
    std::optional<LoopVariableValueInformation> loopVariableValueInformation;
    if (loopHeader.loopVariableIdent.has_value() && numberOfUnrolledLoopIterations) {
        std::vector<unsigned int> loopVariableValuePerIteration(numberOfUnrolledLoopIterations, 0);
        loopVariableValuePerIteration[0] = loopHeader.initialLoopVariableValue;
        for (std::size_t i = 1; i < loopVariableValuePerIteration.size(); ++i) {
            loopVariableValuePerIteration[i] = loopVariableValuePerIteration[i - 1] + loopHeader.stepSize;
        }
        loopVariableValueInformation = LoopVariableValueInformation({*loopHeader.loopVariableIdent, loopVariableValuePerIteration});
    }

    const unsigned int loopRemainderIterationRangeStartValue = loopVariableValueInformation.has_value()
        ? loopVariableValueInformation->valuePerIteration.back() + loopHeader.stepSize
        : loopHeader.initialLoopVariableValue + (numberOfUnrolledLoopIterations * loopHeader.stepSize);

    PartiallyUnrolledLoopInformation partiallyUnrolledLoopInformation;
    partiallyUnrolledLoopInformation.numUnrolledIterations = numberOfUnrolledLoopIterations;
    partiallyUnrolledLoopInformation.unrolledIterations    = UnrolledIterationInformation({std::move(copiesOfLoopBodyStatements), loopVariableValueInformation});
    partiallyUnrolledLoopInformation.remainderLoopHeaderInformation = LoopHeaderInformation({loopHeader.loopVariableIdent, loopRemainderIterationRangeStartValue, loopHeader.finalLoopVariableValue, loopHeader.stepSize});
    partiallyUnrolledLoopInformation.remainderLoopBodyStatements    = std::move(copiesOfRemainingLoopBodyStatements);
    return partiallyUnrolledLoopInformation;
}

std::optional<unsigned> optimizations::LoopUnroller::tryEvaluateNumberToConstant(const syrec::Number& numberContainer, const syrec::Number::loop_variable_mapping& knownLoopVariableValueMappings) {
    if (numberContainer.isConstant()) {
        return numberContainer.evaluate(knownLoopVariableValueMappings);
    }
    if (numberContainer.isLoopVariable() && knownLoopVariableValueMappings.count(numberContainer.variableName())) {
        return numberContainer.evaluate(knownLoopVariableValueMappings);
    }
    if (numberContainer.isCompileTimeConstantExpression()) {
        return tryEvaluateCompileTimeConstantExpr(numberContainer.getExpression(), knownLoopVariableValueMappings);
    }
    return std::nullopt;
}

std::optional<unsigned> optimizations::LoopUnroller::tryEvaluateCompileTimeConstantExpr(const syrec::Number::CompileTimeConstantExpression& compileTimeConstantExpr, const syrec::Number::loop_variable_mapping& knownLoopVariableValueMappings) {
    const auto& lhsOperandEvaluated = tryEvaluateNumberToConstant(*compileTimeConstantExpr.lhsOperand, knownLoopVariableValueMappings);
    const auto& rhsOperandEvaluated = tryEvaluateNumberToConstant(*compileTimeConstantExpr.rhsOperand, knownLoopVariableValueMappings);
    const auto& mappedToBinaryOperation = syrec_operation::tryMapCompileTimeConstantExprOperationFlagToEnum(compileTimeConstantExpr.operation);
    if (!(lhsOperandEvaluated.has_value() && rhsOperandEvaluated.has_value() && mappedToBinaryOperation.has_value())) {
        return std::nullopt;
    }
    return syrec_operation::apply(*mappedToBinaryOperation, lhsOperandEvaluated, rhsOperandEvaluated);
}

std::optional<optimizations::LoopUnroller::LoopHeaderInformation> optimizations::LoopUnroller::tryDetermineIterationRangeForLoopHeader(const syrec::ForStatement& loopStatement, const syrec::Number::loop_variable_mapping& knownLoopVariableValueMappings) {
    const auto& iterationRangeStartValueEvaluated = tryEvaluateNumberToConstant(*loopStatement.range.first, knownLoopVariableValueMappings);
    const auto& iterationRangeEndValueEvaluated   = tryEvaluateNumberToConstant(*loopStatement.range.second, knownLoopVariableValueMappings);
    const auto& iterationRangeStepSizeValueEvaluated = tryEvaluateNumberToConstant(*loopStatement.step, knownLoopVariableValueMappings);

    if (!(iterationRangeStartValueEvaluated.has_value() && iterationRangeEndValueEvaluated.has_value() && iterationRangeStepSizeValueEvaluated.has_value())) {
        return std::nullopt;
    }

    return optimizations::LoopUnroller::LoopHeaderInformation({loopStatement.loopVariable.empty() ? std::nullopt : std::make_optional(loopStatement.loopVariable), *iterationRangeStartValueEvaluated, *iterationRangeEndValueEvaluated, *iterationRangeStepSizeValueEvaluated});
}

std::optional<unsigned> optimizations::LoopUnroller::tryDetermineNumberOfLoopIterations(const LoopHeaderInformation& loopHeader) {
    return utils::determineNumberOfLoopIterations(loopHeader.initialLoopVariableValue, loopHeader.finalLoopVariableValue, loopHeader.stepSize);
}

std::unique_ptr<syrec::ForStatement> optimizations::LoopUnroller::createLoopStatements(const LoopHeaderInformation& loopHeader, const std::vector<std::reference_wrapper<const syrec::Statement>>& loopBody) {
    auto generatedLoopStatement = std::make_unique<syrec::ForStatement>();
    generatedLoopStatement->loopVariable = loopHeader.loopVariableIdent.value_or("");

    const auto& generatedContainerForStepSize = std::make_shared<syrec::Number>(loopHeader.stepSize);
    const auto& generatedContainerForIterationRangeStart = std::make_shared<syrec::Number>(loopHeader.initialLoopVariableValue);
    const auto& generatedContainerForIterationRangeEnd = std::make_shared<syrec::Number>(loopHeader.finalLoopVariableValue);

    if (!generatedLoopStatement || !generatedContainerForStepSize || !generatedContainerForIterationRangeStart || !generatedContainerForIterationRangeEnd) {
        return nullptr;
    }

    generatedLoopStatement->statements.reserve(loopBody.size());
    for (std::size_t i = 0; i < loopBody.size(); ++i) {
        if (syrec::Statement::ptr generatedCopyOfStatement = copyUtils::createCopyOfStmt(loopBody.at(i)); generatedCopyOfStatement) {
            generatedLoopStatement->addStatement(generatedCopyOfStatement);
        }
    }
    return generatedLoopStatement;
}

std::unique_ptr<optimizations::LoopUnroller::BaseUnrolledLoopInformation> optimizations::LoopUnroller::tryUnrollLoop(std::size_t currentLoopNestingLevel, const syrec::ForStatement& loopStatement, const syrec::Number::loop_variable_mapping& knownLoopVariableValueMappings, const LoopOptimizationConfig& unrollConfigToUse) {
    resetInternals(unrollConfigToUse, knownLoopVariableValueMappings);
    const auto& determinedLoopHeaderInformation = tryDetermineIterationRangeForLoopHeader(loopStatement, knownLoopVariableValueMappings);
    if (!determinedLoopHeaderInformation.has_value() || !doesCurrentLoopNestingLevelAllowUnroll(currentLoopNestingLevel)) {
        return std::make_unique<UnmodifiedLoopInformation>();
    }

    if (canLoopBeFullyUnrolled(*determinedLoopHeaderInformation)) {
        if (auto fullLoopUnrollingResult = tryPerformFullLoopUnroll(*determinedLoopHeaderInformation, loopStatement.statements); fullLoopUnrollingResult.has_value()) {
            return std::make_unique<FullyUnrolledLoopInformation>(std::move(*fullLoopUnrollingResult));
        }
    } else if (auto partialLoopUnrollingResult = tryPerformPartialUnrollOfLoop(unrollConfig.maxNumberOfUnrolledIterationsPerLoop, unrollConfig.isRemainderLoopAllowed, *determinedLoopHeaderInformation, loopStatement.statements); partialLoopUnrollingResult.has_value()) {
        return std::make_unique<PartiallyUnrolledLoopInformation>(std::move(*partialLoopUnrollingResult));
    }
    return std::make_unique<UnmodifiedLoopInformation>();
}