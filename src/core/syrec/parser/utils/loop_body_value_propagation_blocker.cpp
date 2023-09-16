#include "core/syrec/parser/utils/loop_body_value_propagation_blocker.hpp"

#include "core/syrec/parser/utils/signal_access_utils.hpp"

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
            const auto& restrictionStatusOfSignal = restrictionStatusPerSignal.at(signalIdent);
            for (auto& uniqueAssignment : uniqueAssignmentsToSignalOfScope) {
                restrictionStatusOfSignal->liftRestrictionsOfDimensions(uniqueAssignment.dimensionAccess, uniqueAssignment.bitRange);
            }
            // TODO: Remove entry if no restrictions remain
        }
        uniqueAssignmentsPerScope.pop();
    }
}

bool optimizations::LoopBodyValuePropagationBlocker::isAccessBlockedFor(const syrec::VariableAccess& accessedPartsOfSignal) const {
    if (restrictionStatusPerSignal.count(accessedPartsOfSignal.var->name) != 0) {
        return restrictionStatusPerSignal.at(accessedPartsOfSignal.var->name)->tryFetchValueFor(transformUserDefinedDimensionAccess(accessedPartsOfSignal), transformUserDefinedBitRangeAccess(accessedPartsOfSignal)).has_value();
    }
    return false;
}

// TODO: Perform dead code elimination ?
void optimizations::LoopBodyValuePropagationBlocker::handleStatements(const std::vector<std::reference_wrapper<const syrec::Statement>>& stmtsToAnalyze, bool* wasUniqueAssignmentDefined) {
    for (const auto& stmt : stmtsToAnalyze) {
        handleStatement(stmt, wasUniqueAssignmentDefined);
    }
}

void optimizations::LoopBodyValuePropagationBlocker::handleStatement(const syrec::Statement& stmt, bool* wasUniqueAssignmentDefined) {
    if (const auto& stmtAsLoopStmt = stmtCastedAs<syrec::ForStatement>(stmt); stmtAsLoopStmt != nullptr) {
        handleStatements(transformCollectionOfSharedPointersToReferences(stmtAsLoopStmt->statements), wasUniqueAssignmentDefined);
    }
    if (const auto& stmtAsIfStmt = stmtCastedAs<syrec::IfStatement>(stmt); stmtAsIfStmt != nullptr) {
        handleStatements(transformCollectionOfSharedPointersToReferences(stmtAsIfStmt->thenStatements), wasUniqueAssignmentDefined);
        handleStatements(transformCollectionOfSharedPointersToReferences(stmtAsIfStmt->elseStatements), wasUniqueAssignmentDefined && *wasUniqueAssignmentDefined ? nullptr : wasUniqueAssignmentDefined);
    }
    if (const auto& stmtAsSwapStmt = stmtCastedAs<syrec::SwapStatement>(stmt); stmtAsSwapStmt != nullptr) {
        if (const auto& uniqueAssignmentResolverResultOfLhsOperand = handleAssignment(*stmtAsSwapStmt->lhs); uniqueAssignmentResolverResultOfLhsOperand) {
            if (wasUniqueAssignmentDefined && !*wasUniqueAssignmentDefined) {
                *wasUniqueAssignmentDefined = true;
            }
            storeUniqueAssignmentForCurrentScope(uniqueAssignmentResolverResultOfLhsOperand->first, uniqueAssignmentResolverResultOfLhsOperand->second);
        }
        if (const auto& uniqueAssignmentResolverResultOfRhsOperand = handleAssignment(*stmtAsSwapStmt->rhs); uniqueAssignmentResolverResultOfRhsOperand) {
            if (wasUniqueAssignmentDefined && !*wasUniqueAssignmentDefined) {
                *wasUniqueAssignmentDefined = true;
            }
            storeUniqueAssignmentForCurrentScope(uniqueAssignmentResolverResultOfRhsOperand->first, uniqueAssignmentResolverResultOfRhsOperand->second);
        }
    }
    if (const auto& stmtAsAssignmentStmt = stmtCastedAs<syrec::AssignStatement>(stmt); stmtAsAssignmentStmt != nullptr) {
        if (const auto& uniqueAssignmentResolverResultOfAssignedToSignal = handleAssignment(*stmtAsAssignmentStmt->lhs); uniqueAssignmentResolverResultOfAssignedToSignal) {
            if (wasUniqueAssignmentDefined && !*wasUniqueAssignmentDefined) {
                *wasUniqueAssignmentDefined = true;
            }
            storeUniqueAssignmentForCurrentScope(uniqueAssignmentResolverResultOfAssignedToSignal->first, uniqueAssignmentResolverResultOfAssignedToSignal->second);
        }
    }
    if (const auto& stmtAsUnaryAssignmntStmt = stmtCastedAs<syrec::UnaryStatement>(stmt); stmtAsUnaryAssignmntStmt != nullptr) {
        if (const auto& uniqueAssignmentResolverResultOfAssignedToSignal = handleAssignment(*stmtAsUnaryAssignmntStmt->var); uniqueAssignmentResolverResultOfAssignedToSignal) {
            if (wasUniqueAssignmentDefined && !*wasUniqueAssignmentDefined) {
                *wasUniqueAssignmentDefined = true;
            }
            storeUniqueAssignmentForCurrentScope(uniqueAssignmentResolverResultOfAssignedToSignal->first, uniqueAssignmentResolverResultOfAssignedToSignal->second);
        }
    }
    if (const auto& stmtAsCallStmt = stmtCastedAs<syrec::CallStatement>(stmt); stmtAsCallStmt != nullptr) {
        if (const auto& matchingModulesForGivenSignature = symbolTableReference->tryGetOptimizedSignatureForModuleCall(stmtAsCallStmt->target->name, stmtAsCallStmt->parameters); matchingModulesForGivenSignature.has_value()) {
            for (const auto& declaredCalleeArgument : matchingModulesForGivenSignature->determineOptimizedCallSignature()) {
                if (declaredCalleeArgument->type == syrec::Variable::Types::Inout || declaredCalleeArgument->type == syrec::Variable::Out) {
                    if (wasUniqueAssignmentDefined && !*wasUniqueAssignmentDefined) {
                        *wasUniqueAssignmentDefined = true;
                    }
                    storeUniqueAssignmentForCurrentScope(declaredCalleeArgument->name, ScopeLocalAssignmentParts({.dimensionAccess = {}, .bitRange = std::nullopt}));
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
    return std::nullopt;
}

std::vector<std::reference_wrapper<const syrec::Statement>> optimizations::LoopBodyValuePropagationBlocker::transformCollectionOfSharedPointersToReferences(const syrec::Statement::vec& statements) {
    std::vector<std::reference_wrapper<const syrec::Statement>> resultContainer;
    resultContainer.reserve(statements.size());

    for (const auto& stmt: statements) {
        resultContainer.emplace_back(*stmt.get());
    }
    return resultContainer;
}


// TODO: Merge accesses, etc.
void optimizations::LoopBodyValuePropagationBlocker::storeUniqueAssignmentForCurrentScope(const std::string& assignedToSignalIdent, const ScopeLocalAssignmentParts& uniqueAssignment) {
    if (uniqueAssignmentsPerScope.empty()) {
        return;
    }

    auto& uniqueAssignmentLookupForScope = uniqueAssignmentsPerScope.top();
    if (uniqueAssignmentLookupForScope.count(assignedToSignalIdent) == 0) {
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
    if (!accessedPartsOfSignal.range.has_value()) {
        return std::nullopt;
    }

    if (!accessedPartsOfSignal.range->first->isConstant() || !accessedPartsOfSignal.range->second->isConstant()) {
        return std::nullopt;
    }

    const auto& accessedBitRangeStart = accessedPartsOfSignal.range->first->evaluate({});
    const auto& accessedBitRangeEnd   = accessedPartsOfSignal.range->second->evaluate({});
    return std::make_optional(BitRangeAccessRestriction::BitRangeAccess(std::make_pair(accessedBitRangeStart, accessedBitRangeEnd)));
}