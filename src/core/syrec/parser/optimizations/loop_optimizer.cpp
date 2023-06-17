#include "core/syrec/parser/optimizations/loop_optimizer.hpp"

#include <algorithm>

optimizations::LoopUnroller::UnrollInformation optimizations::LoopUnroller::tryUnrollLoop(const LoopOptimizationConfig& loopUnrollConfig, const std::shared_ptr<syrec::ForStatement>& loopStatement) {
    return tryUnrollLoop(loopUnrollConfig, 0, loopStatement);
}

optimizations::LoopUnroller::UnrollInformation optimizations::LoopUnroller::tryUnrollLoop(const LoopOptimizationConfig& loopUnrollConfig, const std::size_t loopNestingLevel, const std::shared_ptr<syrec::ForStatement>& loopStatement) {
    
    if (loopUnrollConfig.forceUnrollAll) {
        if (const auto& fullyUnrolledLoopInformation = canLoopBeFullyUnrolled(loopUnrollConfig, loopNestingLevel, loopStatement); fullyUnrolledLoopInformation.has_value()) {
            return UnrollInformation(*fullyUnrolledLoopInformation);   
        }
    } else if (const auto& fullyUnrolledLoopInformation = canLoopBeFullyUnrolled(loopUnrollConfig, loopNestingLevel, loopStatement); fullyUnrolledLoopInformation.has_value()) {
        return UnrollInformation(*fullyUnrolledLoopInformation);
    } else if (const auto& peeledLoopInformation = canLoopIterationsBePeeled(loopUnrollConfig, loopNestingLevel, loopStatement); peeledLoopInformation.has_value()) {
        return UnrollInformation(*peeledLoopInformation);
    } else if (const auto& partiallyUnrolledLoopInformation = canLoopBePartiallyUnrolled(loopUnrollConfig, loopNestingLevel, loopStatement); partiallyUnrolledLoopInformation.has_value()) {
        return UnrollInformation(*partiallyUnrolledLoopInformation);
    }
    return UnrollInformation(NotModifiedLoopInformation({loopStatement}));
}

std::optional<optimizations::LoopUnroller::FullyUnrolledLoopInformation> optimizations::LoopUnroller::canLoopBeFullyUnrolled(const LoopOptimizationConfig& loopUnrollConfig, std::size_t loopNestingLevel, const std::shared_ptr<syrec::ForStatement>& loopStatement) {
    const auto& numberOfLoopIterations = determineNumberOfLoopIterations(loopStatement);
    if (!numberOfLoopIterations.has_value()) {
        return std::nullopt;
    }

    if (!loopUnrollConfig.forceUnrollAll) {
        if (*numberOfLoopIterations > loopUnrollConfig.maxUnrollCountPerLoop || loopNestingLevel > loopUnrollConfig.maxAllowedNestingLevelOfInnerLoops) {
            return std::nullopt;
        }   
    }

    const auto&                                         declaredLoopVariable             = tryGetLoopVariable(loopStatement);
    const auto&                                         optionalLoopVariableValueLookup = declaredLoopVariable.has_value() ? std::make_optional(determineLoopIterationInfo(loopStatement)) : std::nullopt;
    std::vector<std::vector<UnrollInformation>>         unrollInformationOfNestedLoopsPerIteration(*numberOfLoopIterations);
    const auto&                                         nestedLoops = buildLoopStructure(loopStatement);

    if (nestedLoops.nestedLoops.empty()) {
        const auto unrollingInformation = FullyUnrolledLoopInformation(optionalLoopVariableValueLookup, unrollInformationOfNestedLoopsPerIteration, *numberOfLoopIterations);
        return std::make_optional(unrollingInformation);
    }

    auto startValue = loopStatement->range.first->evaluate(loopVariableValueLookup);

    for (std::size_t i = 0; i < *numberOfLoopIterations; ++i) {
        if (declaredLoopVariable.has_value()) {
            loopVariableValueLookup.insert_or_assign(*declaredLoopVariable, startValue++);
        }

        std::vector<UnrollInformation> unrolledNestedLoopForIteration;
        for (const auto& nestedLoop: nestedLoops.nestedLoops) {
            unrolledNestedLoopForIteration.emplace_back(tryUnrollLoop(loopUnrollConfig, loopNestingLevel + 1, nestedLoop.loopStmt));
        }
        unrollInformationOfNestedLoopsPerIteration[i] = unrolledNestedLoopForIteration;
        //unrollInformationOfNestedLoopsPerIteration.at(i) = unrolledNestedLoopForIteration;
    }

    if (declaredLoopVariable.has_value()) {
        loopVariableValueLookup.erase(*declaredLoopVariable);
    }

    const auto unrollingInformation = FullyUnrolledLoopInformation(optionalLoopVariableValueLookup, unrollInformationOfNestedLoopsPerIteration, *numberOfLoopIterations);
    return std::make_optional(unrollingInformation);
}

std::optional<optimizations::LoopUnroller::PeeledIterationsOfLoopInformation> optimizations::LoopUnroller::canLoopIterationsBePeeled(const LoopOptimizationConfig& loopUnrollConfig, std::size_t loopNestingLevel, const std::shared_ptr<syrec::ForStatement>& loopStatement) {
    if (loopUnrollConfig.forceUnrollAll) {
        return std::nullopt;
    }

    const auto& numberOfLoopIterations = determineNumberOfLoopIterations(loopStatement);
    if (!numberOfLoopIterations.has_value() || (loopUnrollConfig.maxUnrollCountPerLoop < numberOfLoopIterations && !loopUnrollConfig.allowRemainderLoop) || loopNestingLevel > loopUnrollConfig.maxAllowedNestingLevelOfInnerLoops) {
        return std::nullopt;
    }

    const auto numPeeledIterations = std::min(loopUnrollConfig.maxUnrollCountPerLoop, *numberOfLoopIterations);
    const auto numNotPeeledIterations = *numberOfLoopIterations - numPeeledIterations;

    const auto&                                 nestedLoops          = buildLoopStructure(loopStatement);
    const auto&                                 declaredLoopVariable = tryGetLoopVariable(loopStatement);
    auto                                        peeledLoopVariableValueLookup = determineLoopIterationInfo(loopStatement);
    std::vector<std::vector<UnrollInformation>> unrolledNestedLoopInformation(numPeeledIterations);
    std::vector<UnrollInformation>              nestedLoopInformationOfNotPeeledIterations;

    for (std::size_t peeledIterationIdx = 0; peeledIterationIdx < numPeeledIterations; ++peeledIterationIdx) {
        if (declaredLoopVariable.has_value()) {
            loopVariableValueLookup.insert_or_assign(*declaredLoopVariable, peeledLoopVariableValueLookup.initialLoopVariableValue + (peeledIterationIdx * peeledLoopVariableValueLookup.stepSize));
        }

        std::vector<UnrollInformation> unrolledNestedLoopForPeeledIteration;
        for (const auto& nestedLoop: nestedLoops.nestedLoops) {
            unrolledNestedLoopForPeeledIteration.emplace_back(tryUnrollLoop(loopUnrollConfig, loopNestingLevel + 1, nestedLoop.loopStmt));
            loopNestingLevel--;
        }
        unrolledNestedLoopInformation.at(peeledIterationIdx) = unrolledNestedLoopForPeeledIteration;
    }

    if (numNotPeeledIterations > 0) {
        /*if (declaredLoopVariable.has_value()) {
            loopVariableValueLookup.insert_or_assign(*declaredLoopVariable, startValue++);
        }*/
        for (const auto& nestedLoop: nestedLoops.nestedLoops) {
            nestedLoopInformationOfNotPeeledIterations.emplace_back(tryUnrollLoop(loopUnrollConfig, loopNestingLevel + 1, nestedLoop.loopStmt));
            loopNestingLevel--;
        }
    }

    /*for (std::size_t notPeeledIterationIdx = 0; notPeeledIterationIdx < numNotPeeledIterations; ++notPeeledIterationIdx) {
        if (declaredLoopVariable.has_value()) {
            loopVariableValueLookup.insert_or_assign(*declaredLoopVariable, startValue++);
        }
        std::vector<UnrollInformation> unrolledNestedLoopForNotPeeledIteration;
        for (const auto& nestedLoop: nestedLoops.nestedLoops) {
            unrolledNestedLoopForNotPeeledIteration.emplace_back(tryUnrollLoop(loopUnrollConfig, loopNestingLevel++, nestedLoop.loopStmt));
            loopNestingLevel--;
        }
        nestedLoopInformationOfNotPeeledIterations.at(notPeeledIterationIdx) = unrolledNestedLoopForNotPeeledIteration;
    }*/

    if (declaredLoopVariable.has_value()) {
        loopVariableValueLookup.erase(*declaredLoopVariable);
    }

    if (numNotPeeledIterations == 0) {
        return std::make_optional(PeeledIterationsOfLoopInformation({peeledLoopVariableValueLookup, unrolledNestedLoopInformation, std::nullopt}));
    }
    
    return std::make_optional(PeeledIterationsOfLoopInformation(
            {peeledLoopVariableValueLookup,
             unrolledNestedLoopInformation,
             std::make_optional(std::make_pair(determineLoopHeaderInformationForNotPeeledLoopRemainder(peeledLoopVariableValueLookup, numPeeledIterations), nestedLoopInformationOfNotPeeledIterations))}));
    
}

std::optional<optimizations::LoopUnroller::PartiallyUnrolledLoopInformation> optimizations::LoopUnroller::canLoopBePartiallyUnrolled(const LoopOptimizationConfig& loopUnrollConfig, std::size_t loopNestingLevel, const std::shared_ptr<syrec::ForStatement>& loopStatement) {
    return std::nullopt;
}

std::optional<std::size_t> optimizations::LoopUnroller::estimateUnrolledLoopSize(const LoopOptimizationConfig& loopUnrollConfig, std::size_t loopNestingLevel, const std::shared_ptr<syrec::ForStatement>& loopStatement) {
    return std::nullopt;
}

std::optional<std::size_t> optimizations::LoopUnroller::determineNumberOfLoopIterations(const std::shared_ptr<syrec::ForStatement>& loopStatement) const {
    const auto& startValue             = loopStatement->range.first->evaluate(loopVariableValueLookup);
    const auto& endValue   = loopStatement->range.second->evaluate(loopVariableValueLookup);
    const auto& stepSize               = loopStatement->step->evaluate(loopVariableValueLookup);

    if (endValue > startValue) {
        return std::make_optional(((endValue - startValue) + 1) / stepSize);
    }
    return std::make_optional(((startValue - endValue) + 1) / stepSize);
}

std::optional<std::string> optimizations::LoopUnroller::tryGetLoopVariable(const std::shared_ptr<syrec::ForStatement>& loopStatement) {
    return loopStatement->loopVariable.empty() ? std::nullopt : std::make_optional(loopStatement->loopVariable);
}

optimizations::LoopUnroller::LoopIterationInfo optimizations::LoopUnroller::determineLoopIterationInfo(const std::shared_ptr<syrec::ForStatement>& loopStatement) const {
    const auto& startValue = loopStatement->range.first->evaluate(loopVariableValueLookup);
    const auto& endValue   = loopStatement->range.second->evaluate(loopVariableValueLookup);
    const auto& stepSize   = loopStatement->step->evaluate(loopVariableValueLookup);
    return LoopIterationInfo({tryGetLoopVariable(loopStatement), startValue, endValue, stepSize});
}

optimizations::LoopUnroller::LoopIterationInfo optimizations::LoopUnroller::determineLoopHeaderInformationForNotPeeledLoopRemainder(const LoopIterationInfo& initialLoopIterationInformation, std::size_t numPeeledIterations) {
    if (numPeeledIterations == 0) {
        return initialLoopIterationInformation;
    }

    const auto& modifiedStartValue = initialLoopIterationInformation.initialLoopVariableValue + (numPeeledIterations * initialLoopIterationInformation.stepSize);
    return LoopIterationInfo({initialLoopIterationInformation.loopVariableIdent, modifiedStartValue, initialLoopIterationInformation.finalLoopVariableValue, initialLoopIterationInformation.stepSize});
}


optimizations::LoopUnroller::NestedLoopInformation optimizations::LoopUnroller::buildLoopStructure(const std::shared_ptr<syrec::ForStatement>& loopStatement) {
    std::vector<NestedLoopInformation> nestedLoops;

    for (const auto& stmt : loopStatement->statements) {
        if (const auto& stmtAsLoopStmt = std::dynamic_pointer_cast<syrec::ForStatement>(stmt); stmtAsLoopStmt != nullptr) {
            nestedLoops.emplace_back(buildLoopStructure(stmtAsLoopStmt));
        }
    }
    return {loopStatement, nestedLoops};
}