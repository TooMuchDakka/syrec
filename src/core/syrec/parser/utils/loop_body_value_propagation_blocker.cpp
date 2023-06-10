#include "core/syrec/parser/utils/loop_body_value_propagation_blocker.hpp"


optimizations::LoopBodyValuePropagationBlocker::LoopBodyValuePropagationBlocker(const syrec::Statement::vec& stmtBlock, const parser::SymbolTable::ptr& symbolTable, const std::optional<std::shared_ptr<LoopBodyValuePropagationBlocker>>& aggregateOfExistingLoopBodyValueRestrictions):
    symbolTableReference(symbolTable) {

    if (aggregateOfExistingLoopBodyValueRestrictions.has_value()) {
        for (const auto& [signalIdent, restrictionsForSignal]: aggregateOfExistingLoopBodyValueRestrictions.value()->assignedToSignalsInLoopBody) {
            assignedToSignalsInLoopBody.insert(std::make_pair(signalIdent, restrictionsForSignal));
        }   
    }

    for (const auto& stmt: stmtBlock) {
        handleStatement(stmt);
    }
}

bool optimizations::LoopBodyValuePropagationBlocker::isAccessBlockedFor(const syrec::VariableAccess::ptr& accessedPartsOfSignal) const {
    if (assignedToSignalsInLoopBody.count(accessedPartsOfSignal->var->name) == 0) {
        return false;
    }

    return !assignedToSignalsInLoopBody
        .at(accessedPartsOfSignal->var->name)->tryFetchValueFor(transformDimensionAccess(accessedPartsOfSignal), transformAccessedBitRange(accessedPartsOfSignal))
        .has_value();
}

void optimizations::LoopBodyValuePropagationBlocker::handleStatement(const syrec::Statement::ptr& stmt) {
    if (const auto& stmtAsAssignmentStmt = tryConvertStmtToStmtOfOtherType<syrec::AssignStatement>(stmt); stmtAsAssignmentStmt != nullptr) {
        handleAssignment(stmtAsAssignmentStmt->lhs);
    } else if (const auto& stmtAsUnaryAssignStmt = tryConvertStmtToStmtOfOtherType<syrec::UnaryStatement>(stmt); stmtAsUnaryAssignStmt != nullptr) {
        handleAssignment(stmtAsUnaryAssignStmt->var);
    } else if (const auto& stmtAsCallStmt = tryConvertStmtToStmtOfOtherType<syrec::CallStatement>(stmt); stmtAsCallStmt != nullptr) {
        const auto& calledModule = stmtAsCallStmt->target;

        for (std::size_t i = 0; i < calledModule->parameters.size(); ++i) {
            if (calledModule->parameters.at(i)->type == syrec::Variable::Types::In) {
                continue;   
            }
            const auto& variableAccessForCalleeArgument = std::make_shared<syrec::VariableAccess>();
            variableAccessForCalleeArgument->setVar(calledModule->parameters.at(i));
            handleAssignment(variableAccessForCalleeArgument);
        }
    } else if (const auto& stmtAsSwapStmt = tryConvertStmtToStmtOfOtherType<syrec::SwapStatement>(stmt); stmtAsSwapStmt != nullptr) {
        handleAssignment(stmtAsSwapStmt->lhs);
        handleAssignment(stmtAsSwapStmt->rhs);
    } else if (const auto& stmtAsIfStmt = tryConvertStmtToStmtOfOtherType<syrec::IfStatement>(stmt); stmtAsIfStmt != nullptr) {
        for (const auto& trueBranchStmt : stmtAsIfStmt->thenStatements) {
            handleStatement(trueBranchStmt);
        }

        for (const auto& falseBranchStmt: stmtAsIfStmt->elseStatements) {
            handleStatement(falseBranchStmt);
        }
    } else if (const auto& stmtAsForStmt = tryConvertStmtToStmtOfOtherType<syrec::ForStatement>(stmt); stmtAsForStmt != nullptr) {
        for (const auto& loopBodyStmt : stmtAsForStmt->statements) {
            handleStatement(loopBodyStmt);
        }
    }

    /*else if (const auto& stmtAsSwapStmt = std::dynamic_pointer_cast<syrec::SwapStatement::ptr>(stmt); stmtAsSwapStmt != nullptr) {

    }*/
}

void optimizations::LoopBodyValuePropagationBlocker::handleAssignment(const syrec::VariableAccess::ptr& assignedToSignalParts) {
    const auto& fetchedExistingSignalPropagationBlocker = fetchAndAddNewAssignedToSignalToLookupIfNoEntryExists(assignedToSignalParts);
    if (fetchedExistingSignalPropagationBlocker.has_value()) {
        const auto& transformedBitRange = transformAccessedBitRange(assignedToSignalParts);
        if (transformedBitRange.has_value()) {
            (*fetchedExistingSignalPropagationBlocker)->invalidateStoredValueForBitrange(transformDimensionAccess(assignedToSignalParts), *transformAccessedBitRange(assignedToSignalParts));
        } else {
            (*fetchedExistingSignalPropagationBlocker)->invalidateStoredValueFor(transformDimensionAccess(assignedToSignalParts));
        }
    }
}

[[nodiscard]] std::optional<valueLookup::SignalValueLookup::ptr> optimizations::LoopBodyValuePropagationBlocker::fetchAndAddNewAssignedToSignalToLookupIfNoEntryExists(const syrec::VariableAccess::ptr& assignedToSignalParts) {
    if (!symbolTableReference->contains(assignedToSignalParts->var->name)) {
        return std::nullopt;
    }

    if (assignedToSignalsInLoopBody.count(assignedToSignalParts->var->name) != 0) {
        return std::make_optional(assignedToSignalsInLoopBody.at(assignedToSignalParts->var->name));
    }
    
    const auto& symbolTableEntryForAccessedSignal = *symbolTableReference->getVariable(assignedToSignalParts->var->name);
    if (!std::holds_alternative<syrec::Variable::ptr>(symbolTableEntryForAccessedSignal)) {
        return std::nullopt;
    }

    const auto& validSymbolTableEntryForAccessedSignal = std::get<syrec::Variable::ptr>(symbolTableEntryForAccessedSignal);
    const auto& signalValueLookupForAccessedSignal     = std::make_shared<valueLookup::SignalValueLookup>(
            validSymbolTableEntryForAccessedSignal->bitwidth,
            validSymbolTableEntryForAccessedSignal->dimensions,
            0);
    assignedToSignalsInLoopBody.insert(std::make_pair(assignedToSignalParts->var->name, signalValueLookupForAccessedSignal));
    return std::make_optional(signalValueLookupForAccessedSignal);
}


std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess> optimizations::LoopBodyValuePropagationBlocker::transformAccessedBitRange(const syrec::VariableAccess::ptr& accessedPartsOfSignal) {
    if (!accessedPartsOfSignal->range.has_value()) {
        return std::nullopt;
    }

    if (!accessedPartsOfSignal->range->first->isConstant() || !accessedPartsOfSignal->range->second->isConstant()) {
        return std::nullopt;
    }

    const auto& accessedBitRangeStart = accessedPartsOfSignal->range->first->evaluate({});
    const auto& accessedBitRangeEnd   = accessedPartsOfSignal->range->second->evaluate({});
    return std::make_optional(BitRangeAccessRestriction::BitRangeAccess(std::make_pair(accessedBitRangeStart, accessedBitRangeEnd)));
}

std::vector<std::optional<unsigned>> optimizations::LoopBodyValuePropagationBlocker::transformDimensionAccess(const syrec::VariableAccess::ptr& accessedPartsOfSignal) {
    std::vector<std::optional<unsigned>> transformedDimensionAccessContainer;
    std::transform(
    accessedPartsOfSignal->indexes.cbegin(),
    accessedPartsOfSignal->indexes.cend(),
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
