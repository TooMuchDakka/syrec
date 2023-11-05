#include "core/syrec/parser/utils/loop_body_value_propagation_blocker.hpp"

#include "core/syrec/parser/operation.hpp"
#include "core/syrec/parser/optimizations/reassociate_expression.hpp"
#include "core/syrec/parser/expression_comparer.hpp"
#include "core/syrec/parser/utils/dangerous_component_check_utils.hpp"
#include "core/syrec/parser/utils/loop_range_utils.hpp"

void optimizations::LoopBodyValuePropagationBlocker::openNewScopeAndAppendDataDataFlowAnalysisResult(const std::vector<std::reference_wrapper<const syrec::Statement>>& stmtsToAnalyze, bool* wereAnyAssignmentsPerformedInCurrentScope) {
    if (stmtsToAnalyze.empty()) {
        if (wereAnyAssignmentsPerformedInCurrentScope) {
            *wereAnyAssignmentsPerformedInCurrentScope = false;
        }
        return;
    }
    handleStatements(stmtsToAnalyze, wereAnyAssignmentsPerformedInCurrentScope);
}

void optimizations::LoopBodyValuePropagationBlocker::closeScopeAndDiscardDataFlowAnalysisResult() {
    if (!uniqueAssignmentsPerScope.empty()) {
        for (auto& [signalIdent, uniqueAssignmentsToSignalOfScope]: uniqueAssignmentsPerScope.top()) {
            if (restrictionStatusPerSignal.count(signalIdent)) {
                const auto& restrictionStatusOfSignal = restrictionStatusPerSignal.at(signalIdent);
                for (auto& uniqueAssignment: uniqueAssignmentsToSignalOfScope) {
                    restrictionStatusOfSignal->liftRestrictionsOfDimensions(uniqueAssignment.dimensionAccess, uniqueAssignment.bitRange);
                }
                // TODO: Remove entry if no restrictions remain   
            }
        }
        uniqueAssignmentsPerScope.pop();
    }
}

bool optimizations::LoopBodyValuePropagationBlocker::isAccessBlockedFor(const syrec::VariableAccess& accessedPartsOfSignal) const {
    return restrictionStatusPerSignal.count(accessedPartsOfSignal.var->name)
        && restrictionStatusPerSignal.at(accessedPartsOfSignal.var->name)->isValueLookupBlockedFor(transformUserDefinedDimensionAccess(accessedPartsOfSignal), transformUserDefinedBitRangeAccess(accessedPartsOfSignal));
}

// TODO: Perform dead code elimination ?
void optimizations::LoopBodyValuePropagationBlocker::handleStatements(const std::vector<std::reference_wrapper<const syrec::Statement>>& stmtsToAnalyze, bool* wasUniqueAssignmentDefined) {
    for (const auto& stmt : stmtsToAnalyze) {
        handleStatement(stmt, wasUniqueAssignmentDefined);
    }
}

/*
 * TODO: Perform loop unroll and add constant propagation of loop variable if loop performs only one iteration (which would also mean to perform all other optimization like dead-code or constant propagation for all effected statements again)
 * TODO: Perform simplification of expressions
 * TODO: Perform constant propagation
 */
void optimizations::LoopBodyValuePropagationBlocker::handleStatement(const syrec::Statement& stmt, bool* wasUniqueAssignmentDefined) {
    // TODO: Perform loop unroll
    if (const auto& stmtAsLoopStmt = stmtCastedAs<syrec::ForStatement>(stmt); stmtAsLoopStmt) {
        const auto& iterationRangeStartEvaluated = tryEvaluateNumberToConstant(*stmtAsLoopStmt->range.first);
        const auto& iterationRangeEndEvaluated   = tryEvaluateNumberToConstant(*stmtAsLoopStmt->range.second);

        /*
         * Ignore loops that do not perform any iterations
         */
        if (const auto&     iterationRangeStepSizeEvaluated = tryEvaluateNumberToConstant(*stmtAsLoopStmt->step); iterationRangeStartEvaluated.has_value() && iterationRangeEndEvaluated.has_value() && iterationRangeStepSizeEvaluated.has_value()) {
            if (const auto& numberOfIterations              = utils::determineNumberOfLoopIterations(*iterationRangeStartEvaluated, *iterationRangeEndEvaluated, *iterationRangeStepSizeEvaluated); numberOfIterations.has_value() && *numberOfIterations < 1) {
                return;
            }
        }

        handleStatements(transformCollectionOfSharedPointersToReferences(stmtAsLoopStmt->statements), wasUniqueAssignmentDefined);
    }
    else if (const auto& stmtAsIfStmt = stmtCastedAs<syrec::IfStatement>(stmt); stmtAsIfStmt) {
        std::optional<unsigned int> evaluatedValueOfGuardExpr = tryEvaluateExpressionToConstant(*stmtAsIfStmt->condition);

        /*
         * Try to simplify the user defined guard expression to determine whether a branch of the if statement can be skipped.
         */
        if (!evaluatedValueOfGuardExpr.has_value()) {
            if (const auto& simplificationResultOfBinaryExpr = optimizations::simplifyBinaryExpression(stmtAsIfStmt->condition, true, std::nullopt, *symbolTableReference, nullptr); simplificationResultOfBinaryExpr) {
                evaluatedValueOfGuardExpr = tryEvaluateExpressionToConstant(*simplificationResultOfBinaryExpr);
            }
        }
        if (const auto& guardConditionAsBinaryExpr = std::dynamic_pointer_cast<const syrec::BinaryExpression>(stmtAsIfStmt->condition); guardConditionAsBinaryExpr && !evaluatedValueOfGuardExpr.has_value()) {
            if (const auto& mappedToBinaryOperationFromFlag = syrec_operation::tryMapBinaryOperationFlagToEnum(guardConditionAsBinaryExpr->op); parser::areSyntacticallyEquivalent(guardConditionAsBinaryExpr->lhs, guardConditionAsBinaryExpr->rhs) && mappedToBinaryOperationFromFlag.has_value()) {
                evaluatedValueOfGuardExpr = syrec_operation::determineBooleanResultIfOperandsOfBinaryExprMatchForOperation(*mappedToBinaryOperationFromFlag);
            }
        }

        if (!evaluatedValueOfGuardExpr.has_value()) {
            handleStatements(transformCollectionOfSharedPointersToReferences(stmtAsIfStmt->thenStatements), wasUniqueAssignmentDefined);
            handleStatements(transformCollectionOfSharedPointersToReferences(stmtAsIfStmt->elseStatements), wasUniqueAssignmentDefined && *wasUniqueAssignmentDefined ? nullptr : wasUniqueAssignmentDefined);            
        } else if (evaluatedValueOfGuardExpr.has_value() && *evaluatedValueOfGuardExpr) {
            handleStatements(transformCollectionOfSharedPointersToReferences(stmtAsIfStmt->thenStatements), wasUniqueAssignmentDefined);
        } else {
            handleStatements(transformCollectionOfSharedPointersToReferences(stmtAsIfStmt->elseStatements), wasUniqueAssignmentDefined);
        }

    }
    else if (const auto& stmtAsSwapStmt = stmtCastedAs<syrec::SwapStatement>(stmt); stmtAsSwapStmt) {
        if (const auto& uniqueAssignmentResolverResultOfLhsOperand = handleAssignment(*stmtAsSwapStmt->lhs); uniqueAssignmentResolverResultOfLhsOperand) {
            if (wasUniqueAssignmentDefined && !*wasUniqueAssignmentDefined) {
                *wasUniqueAssignmentDefined = true;
            }
            storeUniqueAssignmentForCurrentScope(uniqueAssignmentResolverResultOfLhsOperand->first, uniqueAssignmentResolverResultOfLhsOperand->second);
            restrictAccessTo(*stmtAsSwapStmt->lhs);
        }
        if (const auto& uniqueAssignmentResolverResultOfRhsOperand = handleAssignment(*stmtAsSwapStmt->rhs); uniqueAssignmentResolverResultOfRhsOperand) {
            if (wasUniqueAssignmentDefined && !*wasUniqueAssignmentDefined) {
                *wasUniqueAssignmentDefined = true;
            }
            storeUniqueAssignmentForCurrentScope(uniqueAssignmentResolverResultOfRhsOperand->first, uniqueAssignmentResolverResultOfRhsOperand->second);
            restrictAccessTo(*stmtAsSwapStmt->rhs);
        }
    }
    else if (const auto& stmtAsAssignmentStmt = stmtCastedAs<syrec::AssignStatement>(stmt); stmtAsAssignmentStmt) {
        /*
         * Ignore assignments that are considered dead code (i.e. the value of the rhs expr is equal to the identity element of the defined assignment operand
         */
        if (const auto& rhsEvaluatedAsConstant = tryEvaluateExpressionToConstant(*stmtAsAssignmentStmt->rhs); rhsEvaluatedAsConstant.has_value()) {
            if (const auto& mappedToOperationEnumValueForFlag = syrec_operation::tryMapAssignmentOperationFlagToEnum(stmtAsAssignmentStmt->op); mappedToOperationEnumValueForFlag.has_value()) {
                if (syrec_operation::isOperandUseAsRhsInOperationIdentityElement(*mappedToOperationEnumValueForFlag, *rhsEvaluatedAsConstant)) {
                    return;
                }            
            }
        }

        if (const auto& uniqueAssignmentResolverResultOfAssignedToSignal = handleAssignment(*stmtAsAssignmentStmt->lhs); uniqueAssignmentResolverResultOfAssignedToSignal) {
            if (wasUniqueAssignmentDefined && !*wasUniqueAssignmentDefined) {
                *wasUniqueAssignmentDefined = true;
            }
            storeUniqueAssignmentForCurrentScope(uniqueAssignmentResolverResultOfAssignedToSignal->first, uniqueAssignmentResolverResultOfAssignedToSignal->second);
            restrictAccessTo(*stmtAsAssignmentStmt->lhs);
        }
    }
    else if (const auto& stmtAsUnaryAssignmntStmt = stmtCastedAs<syrec::UnaryStatement>(stmt); stmtAsUnaryAssignmntStmt) {
        if (const auto& uniqueAssignmentResolverResultOfAssignedToSignal = handleAssignment(*stmtAsUnaryAssignmntStmt->var); uniqueAssignmentResolverResultOfAssignedToSignal) {
            if (wasUniqueAssignmentDefined && !*wasUniqueAssignmentDefined) {
                *wasUniqueAssignmentDefined = true;
            }
            storeUniqueAssignmentForCurrentScope(uniqueAssignmentResolverResultOfAssignedToSignal->first, uniqueAssignmentResolverResultOfAssignedToSignal->second);
            restrictAccessTo(*stmtAsUnaryAssignmntStmt->var);
        }
    }
    else if (const auto& stmtAsCallStmt = stmtCastedAs<syrec::CallStatement>(stmt); stmtAsCallStmt) {
        if (const auto& matchingModulesForGivenSignature = symbolTableReference->tryGetOptimizedSignatureForModuleCall(stmtAsCallStmt->target->name, stmtAsCallStmt->parameters, false); matchingModulesForGivenSignature.has_value()) {
            std::unordered_set<std::size_t> indicesOfRemainingCallerArguments;
            matchingModulesForGivenSignature->determineOptimizedCallSignature(&indicesOfRemainingCallerArguments);

            /*
             * Ignore modules (based on the optimized module version if it exists) that contain only read-only parameters or do not perform any assignments to modifiable parameters.
             */
            bool                 callerArgumentOk = true;
            syrec::Variable::vec remainingCallerArguments;
            remainingCallerArguments.reserve(indicesOfRemainingCallerArguments.size());
            
            for (const auto& indexOfRemainingCallerArgument : indicesOfRemainingCallerArguments) {
                const auto& symbolTableEntryForCallerArgument = symbolTableReference->getVariable(stmtAsCallStmt->parameters.at(indexOfRemainingCallerArgument));
                if (symbolTableEntryForCallerArgument.has_value() && std::holds_alternative<syrec::Variable::ptr>(*symbolTableEntryForCallerArgument)) {
                    remainingCallerArguments.emplace_back(std::get<syrec::Variable::ptr>(*symbolTableEntryForCallerArgument));
                }
                else {
                    callerArgumentOk = false;
                    break;
                }
            }

            if (callerArgumentOk) {
                const parser::SymbolTable::ModuleCallSignature moduleCallSignature({stmtAsCallStmt->target->name, remainingCallerArguments});
                if (dangerousComponentCheckUtils::doesOptimizedModuleSignatureContainNoParametersOrOnlyReadonlyOnes(moduleCallSignature) || !dangerousComponentCheckUtils::doesOptimizedModuleBodyContainAssignmentToModifiableParameter(*stmtAsCallStmt->target, *symbolTableReference)) {
                    return;
                }
            }

            for (const auto& i : indicesOfRemainingCallerArguments) {
                if (const auto& callerParameter = matchingModulesForGivenSignature->declaredParameters.at(i); callerParameter->type == syrec::Variable::Types::Inout ||callerParameter->type == syrec::Variable::Types::Out) {
                    if (wasUniqueAssignmentDefined && !*wasUniqueAssignmentDefined) {
                        *wasUniqueAssignmentDefined = true;
                    }
                    storeUniqueAssignmentForCurrentScope(stmtAsCallStmt->parameters.at(i), ScopeLocalAssignmentParts({.dimensionAccess = {}, .bitRange = std::nullopt}));
                    restrictAccessTo(*remainingCallerArguments.at(i));
                }
            }
        }
    }
}

std::optional<std::pair<std::string, optimizations::LoopBodyValuePropagationBlocker::ScopeLocalAssignmentParts>> optimizations::LoopBodyValuePropagationBlocker::handleAssignment(const syrec::VariableAccess& assignedToSignalParts) const {
    if (const auto& scopeLocalAssignmentParts = determineScopeLocalAssignmentFrom(assignedToSignalParts); scopeLocalAssignmentParts.has_value()) {
        return std::make_optional(std::make_pair(assignedToSignalParts.var->name, *scopeLocalAssignmentParts));
    }
    return std::nullopt;
}

std::optional<optimizations::LoopBodyValuePropagationBlocker::ScopeLocalAssignmentParts> optimizations::LoopBodyValuePropagationBlocker::determineScopeLocalAssignmentFrom(const syrec::VariableAccess& assignedToSignal) const {
    std::optional<BitRangeAccessRestriction::BitRangeAccess> evaluatedBitRangeAccess;
    if (assignedToSignal.range.has_value()) {
        const auto& bitRangeStartEvaluated = tryEvaluateNumberToConstant(*assignedToSignal.range->first);
        const auto& bitRangeEndEvaluated = tryEvaluateNumberToConstant(*assignedToSignal.range->second);
        if (bitRangeStartEvaluated.has_value() && bitRangeEndEvaluated.has_value()) {
            evaluatedBitRangeAccess = BitRangeAccessRestriction::BitRangeAccess(*bitRangeStartEvaluated, *bitRangeEndEvaluated);   
        }
    }

    std::vector<std::optional<unsigned int>> transformedAccessedValuePerDimension;
    std::transform(
    assignedToSignal.indexes.cbegin(),
    assignedToSignal.indexes.cend(),
    std::back_inserter(transformedAccessedValuePerDimension),
    [](const syrec::expression::ptr& accessedValueOfDimension) -> std::optional<unsigned int> {
        if (const auto& exprAsNumericExpr = dynamic_cast<const syrec::NumericExpression*>(&*accessedValueOfDimension); exprAsNumericExpr) {
            return tryEvaluateNumberToConstant(*exprAsNumericExpr->value);
        }
        return std::nullopt;
    });

    return LoopBodyValuePropagationBlocker::ScopeLocalAssignmentParts({transformedAccessedValuePerDimension, evaluatedBitRangeAccess});
}

std::vector<std::reference_wrapper<const syrec::Statement>> optimizations::LoopBodyValuePropagationBlocker::transformCollectionOfSharedPointersToReferences(const syrec::Statement::vec& statements) {
    std::vector<std::reference_wrapper<const syrec::Statement>> resultContainer;
    resultContainer.reserve(statements.size());

    for (const auto& stmt: statements) {
        resultContainer.emplace_back(*stmt.get());
    }
    return resultContainer;
}

std::optional<unsigned> optimizations::LoopBodyValuePropagationBlocker::tryEvaluateNumberToConstant(const syrec::Number& number) {
    return number.isConstant() ? std::make_optional(number.evaluate({})) : std::nullopt;
}

std::optional<unsigned> optimizations::LoopBodyValuePropagationBlocker::tryEvaluateExpressionToConstant(const syrec::expression& expr) {
    if (const auto& exprAsNumericExpr = dynamic_cast<const syrec::NumericExpression*>(&expr); exprAsNumericExpr) {
        return tryEvaluateNumberToConstant(*exprAsNumericExpr->value);
    }
    return std::nullopt;
}


// TODO: Merge accesses, etc.
void optimizations::LoopBodyValuePropagationBlocker::storeUniqueAssignmentForCurrentScope(const std::string& assignedToSignalIdent, const ScopeLocalAssignmentParts& uniqueAssignment) {
    if (uniqueAssignmentsPerScope.empty()) {
        uniqueAssignmentsPerScope.push({});
    }

    auto& uniqueAssignmentLookupForScope = uniqueAssignmentsPerScope.top();
    if (!uniqueAssignmentLookupForScope.count(assignedToSignalIdent)) {
        uniqueAssignmentLookupForScope.insert(std::make_pair(assignedToSignalIdent, std::vector<ScopeLocalAssignmentParts>({uniqueAssignment})));
    } else {
        uniqueAssignmentLookupForScope.at(assignedToSignalIdent).emplace_back(uniqueAssignment);
    }
}

std::vector<std::optional<unsigned>> optimizations::LoopBodyValuePropagationBlocker::transformUserDefinedDimensionAccess(const syrec::VariableAccess& accessedPartsOfSignal) {
    std::vector<std::optional<unsigned>> transformedDimensionAccessContainer;
    std::transform(
    accessedPartsOfSignal.indexes.cbegin(),
    accessedPartsOfSignal.indexes.cend(),
    std::back_inserter(transformedDimensionAccessContainer),
    [](const auto& accessedValueOfDimension) -> std::optional<unsigned int> {
        if (const auto& exprOfAccessedValueOfDimensionAsNumericOne = std::dynamic_pointer_cast<syrec::NumericExpression>(accessedValueOfDimension); exprOfAccessedValueOfDimensionAsNumericOne != nullptr) {
            if (exprOfAccessedValueOfDimensionAsNumericOne->value->isConstant()) {
                return std::make_optional(exprOfAccessedValueOfDimensionAsNumericOne->value->evaluate({}));
            }
        }
        return std::nullopt;
    });
    return transformedDimensionAccessContainer;
}

std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess> optimizations::LoopBodyValuePropagationBlocker::transformUserDefinedBitRangeAccess(const syrec::VariableAccess& accessedPartsOfSignal) {
    if (!accessedPartsOfSignal.range.has_value() || !accessedPartsOfSignal.range->first->isConstant() || !accessedPartsOfSignal.range->second->isConstant()) {
        return std::nullopt;
    }

    const auto& accessedBitRangeStart = accessedPartsOfSignal.range->first->evaluate({});
    const auto& accessedBitRangeEnd   = accessedPartsOfSignal.range->second->evaluate({});
    return std::make_optional(BitRangeAccessRestriction::BitRangeAccess(std::make_pair(accessedBitRangeStart, accessedBitRangeEnd)));
}

void optimizations::LoopBodyValuePropagationBlocker::restrictAccessTo(const syrec::VariableAccess& assignedToSignalParts) {
    const auto& identOfAssignedToSignal                      = assignedToSignalParts.var->name;
    const auto& potentialSymbolTableEntryForAssignedToSignal = symbolTableReference->getVariable(identOfAssignedToSignal);
    if (!potentialSymbolTableEntryForAssignedToSignal.has_value() || !std::holds_alternative<syrec::Variable::ptr>(*potentialSymbolTableEntryForAssignedToSignal)) {
        return;
    }

    const auto& symbolTableEntryForAssignedToSignal = std::get<syrec::Variable::ptr>(*potentialSymbolTableEntryForAssignedToSignal);
    if (!restrictionStatusPerSignal.count(identOfAssignedToSignal)) {
        const auto& generatedRestrictionLookupForAssignedToSignal = std::make_shared<valueLookup::SignalValueLookup>(symbolTableEntryForAssignedToSignal->bitwidth, symbolTableEntryForAssignedToSignal->dimensions, 0);
        restrictionStatusPerSignal.insert(std::make_pair(identOfAssignedToSignal, generatedRestrictionLookupForAssignedToSignal));
    }

    std::optional<BitRangeAccessRestriction::BitRangeAccess> evaluatedBitRangeAccess;
    if (assignedToSignalParts.range.has_value()) {
        const auto& bitRangeStartEvaluated = tryEvaluateNumberToConstant(*assignedToSignalParts.range->first);
        const auto& bitRangeEndEvaluated   = tryEvaluateNumberToConstant(*assignedToSignalParts.range->second);
        if (bitRangeStartEvaluated.has_value() && bitRangeEndEvaluated.has_value()) {
            evaluatedBitRangeAccess = BitRangeAccessRestriction::BitRangeAccess(*bitRangeStartEvaluated, *bitRangeEndEvaluated);
        }
    }

    std::vector<std::optional<unsigned int>> transformedAccessedValuePerDimension;
    std::transform(
            assignedToSignalParts.indexes.cbegin(),
            assignedToSignalParts.indexes.cend(),
            std::back_inserter(transformedAccessedValuePerDimension),
            [](const syrec::expression::ptr& accessedValueOfDimension) -> std::optional<unsigned int> {
                if (const auto& exprAsNumericExpr = dynamic_cast<const syrec::NumericExpression*>(&*accessedValueOfDimension); exprAsNumericExpr) {
                    return tryEvaluateNumberToConstant(*exprAsNumericExpr->value);
                }
                return std::nullopt;
            });

    if (evaluatedBitRangeAccess.has_value()) {
        restrictionStatusPerSignal.at(identOfAssignedToSignal)->invalidateStoredValueForBitrange(transformedAccessedValuePerDimension, *evaluatedBitRangeAccess);   
    } else {
        restrictionStatusPerSignal.at(identOfAssignedToSignal)->invalidateStoredValueFor(transformedAccessedValuePerDimension);
    }
}

void optimizations::LoopBodyValuePropagationBlocker::restrictAccessTo(const syrec::Variable& assignedToSignal) {
    auto assignedToSignalAccess = syrec::VariableAccess();
    assignedToSignalAccess.var                   = std::make_shared<syrec::Variable>(assignedToSignal);
    restrictAccessTo(assignedToSignalAccess);
}