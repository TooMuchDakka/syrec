#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/assignment_transformer.hpp"

using namespace noAdditionalLineSynthesis;

std::vector<AssignmentTransformer::SharedAssignmentReference> AssignmentTransformer::simplify(const std::vector<AssignmentTransformer::SharedAssignmentReference>& assignmentsToSimplify, const InitialSignalValueLookupCallback& initialSignalValueLookupCallback) {
    resetInternals();
    std::vector<SharedAssignmentReference> generatedAssignments;

    for (const SharedAssignmentReference& assignment: assignmentsToSimplify) {
        std::shared_ptr<syrec::AssignStatement> assignmentAsBinaryOne;
        std::shared_ptr<syrec::UnaryStatement>  assignmentAsUnaryOne;

        if (std::holds_alternative<std::shared_ptr<syrec::AssignStatement>>(assignment)) {
            assignmentAsBinaryOne = std::get<std::shared_ptr<syrec::AssignStatement>>(assignment);
        } else if (std::holds_alternative<std::shared_ptr<syrec::UnaryStatement>>(assignment)) {
            assignmentAsUnaryOne = std::get<std::shared_ptr<syrec::UnaryStatement>>(assignment);
        }

        if (!assignmentAsBinaryOne && !assignmentAsUnaryOne) {
            continue;
        }
        
        if ((assignmentAsBinaryOne && !syrec_operation::tryMapAssignmentOperationFlagToEnum(assignmentAsBinaryOne->op).has_value())
            || (assignmentAsUnaryOne && !syrec_operation::tryMapUnaryAssignmentOperationFlagToEnum(assignmentAsUnaryOne->op).has_value())) {
            continue;
        }

        if (assignmentAsBinaryOne) {
            if (areMoreInversionsRequiredToReachOriginalSignalValue(assignmentAsBinaryOne->lhs)) {
                if (doesAssignmentMatchWatchForInvertedAssignment(*assignmentAsBinaryOne)) {
                    tryRemoveWatchForInversionOfAssignment(*assignmentAsBinaryOne);
                }
                else {
                    addWatchForInversionOfAssignment(*assignmentAsBinaryOne);
                }
            } else {
                createSignalValueLookupCacheEntry(assignmentAsBinaryOne->lhs, initialSignalValueLookupCallback);
                addWatchForInversionOfAssignment(*assignmentAsBinaryOne);
            }

            syrec::AssignStatement::vec generatedSubAssignments = trySplitAssignmentRhsExpr(assignmentAsBinaryOne);
            for (const syrec::AssignStatement::ptr& generatedSubAssignment: generatedSubAssignments) {
                const std::shared_ptr<syrec::AssignStatement>   subAssignmentCasted                                 = std::dynamic_pointer_cast<syrec::AssignStatement>(generatedSubAssignment);
                const std::optional<syrec_operation::operation> mappedToOperationEnumValueForSubassignmentOperation = syrec_operation::tryMapAssignmentOperationFlagToEnum(subAssignmentCasted->op);
                tryReplaceAddAssignOperationWithXorOperation(*subAssignmentCasted);
                updateEntryInValueLookupCacheByPerformingAssignment(subAssignmentCasted->lhs, *mappedToOperationEnumValueForSubassignmentOperation, *subAssignmentCasted->rhs);

                if (const std::optional<std::shared_ptr<syrec::UnaryStatement>> binaryAssignmentAsUnaryOne = tryReplaceBinaryAssignmentWithUnaryOne(*subAssignmentCasted); binaryAssignmentAsUnaryOne.has_value()) {
                    generatedAssignments.emplace_back(*binaryAssignmentAsUnaryOne);   
                }
                else {
                    generatedAssignments.emplace_back(subAssignmentCasted);    
                }
            }   
        } else if (assignmentAsUnaryOne) {
            if (areMoreInversionsRequiredToReachOriginalSignalValue(assignmentAsUnaryOne->var)) {
                if (doesAssignmentMatchWatchForInvertedAssignment(*assignmentAsUnaryOne)) {
                    tryRemoveWatchForInversionOfAssignment(*assignmentAsUnaryOne);
                } else {
                    addWatchForInversionOfAssignment(*assignmentAsUnaryOne);
                }
            } else {
                createSignalValueLookupCacheEntry(assignmentAsUnaryOne->var, initialSignalValueLookupCallback);
                addWatchForInversionOfAssignment(*assignmentAsUnaryOne);
            }

            generatedAssignments.emplace_back(assignmentAsUnaryOne);
        }
    }
    return generatedAssignments.size() >= assignmentsToSimplify.size() ? generatedAssignments : assignmentsToSimplify;
}

std::optional<unsigned> AssignmentTransformer::SignalValueLookupCacheEntry::getValue() const {
    if (getTypeOfActiveValue() == None) {
        return std::nullopt;
    }
    return getTypeOfActiveValue() == Original ? valuePriorToAnyAssignment : currentValue;
}

AssignmentTransformer::SignalValueLookupCacheEntry::ActiveValueEntry AssignmentTransformer::SignalValueLookupCacheEntry::getTypeOfActiveValue() const {
    return activeValueEntry;
}

void AssignmentTransformer::SignalValueLookupCacheEntry::invalidateCurrentValue() {
    activeValueEntry = None;
}

void AssignmentTransformer::SignalValueLookupCacheEntry::markInitialValueAsActive() {
    activeValueEntry = Original;
    currentValue     = activeValueEntry;
}

void AssignmentTransformer::SignalValueLookupCacheEntry::markValueAsModifiable() {
    activeValueEntry = Modifiable;
}


void AssignmentTransformer::SignalValueLookupCacheEntry::updateValueTo(const std::optional<unsigned>& newValue) {
    markValueAsModifiable();
    if (!newValue.has_value()) {
        invalidateCurrentValue();
    }
    else {
        currentValue = newValue;    
    }
}


void AssignmentTransformer::resetInternals() {
    awaitedInversionsOfBinaryAssignmentsLookup.clear();
    signalValueLookupCache.clear();
}

void AssignmentTransformer::addWatchForInversionOfAssignment(const syrec::AssignStatement& assignment) {
    const std::optional<syrec_operation::operation> mappedToAssignmentOperationEnumValueFromFlag = syrec_operation::tryMapAssignmentOperationFlagToEnum(assignment.op);
    const std::optional<syrec_operation::operation> invertedAssignmentOperation                  = mappedToAssignmentOperationEnumValueFromFlag.has_value() ? syrec_operation::invert(*mappedToAssignmentOperationEnumValueFromFlag) : std::nullopt;
        
    if (!awaitedInversionsOfBinaryAssignmentsLookup.count(assignment.lhs)) {
        auto awaitedInversionOfAssignedToSignalParts = AwaitedInversionsByBinaryAssignmentLookupEntry();
        if (invertedAssignmentOperation.has_value()) {
            awaitedInversionOfAssignedToSignalParts.emplace(std::make_pair(assignment.rhs, *invertedAssignmentOperation));
        }
        awaitedInversionsOfBinaryAssignmentsLookup.emplace(std::make_pair(assignment.lhs, awaitedInversionOfAssignedToSignalParts));
    } else {
        AwaitedInversionsByBinaryAssignmentLookupEntry& existingWaitsOnInversion = awaitedInversionsOfBinaryAssignmentsLookup.at(assignment.lhs);
        if (!invertedAssignmentOperation.has_value()) {
            awaitedInversionsOfBinaryAssignmentsLookup.erase(assignment.lhs);
        } else {
            existingWaitsOnInversion.emplace(std::make_pair(assignment.rhs, *invertedAssignmentOperation));
        }
    }
}

void AssignmentTransformer::addWatchForInversionOfAssignment(const syrec::UnaryStatement& assignment) {
    const std::optional<syrec_operation::operation> mappedToUnaryAssignmentOperationEnumValueFromFlag = syrec_operation::tryMapUnaryAssignmentOperationFlagToEnum(assignment.op);
    const std::optional<syrec_operation::operation> invertedUnaryAssignmentOperation                  = mappedToUnaryAssignmentOperationEnumValueFromFlag.has_value() ? syrec_operation::invert(*mappedToUnaryAssignmentOperationEnumValueFromFlag) : std::nullopt;

    if (!awaitedInversionsOfUnaryAssignmentsLookup.count(assignment.var)) {
        const auto newWaitsForInversionOfAssignedToSignalParts = AwaitedInversionsByUnaryAssignmentLookupEntry({.awaitedInvertedAssignmentOperation = *invertedUnaryAssignmentOperation, .numWaits = 1});
        awaitedInversionsOfUnaryAssignmentsLookup.emplace(std::make_pair(assignment.var, newWaitsForInversionOfAssignedToSignalParts));
    } else {
        AwaitedInversionsByUnaryAssignmentLookupEntry& existingWaitsForAssignedToSignalParts = awaitedInversionsOfUnaryAssignmentsLookup.at(assignment.var);
        if (!invertedUnaryAssignmentOperation.has_value()) {
            awaitedInversionsOfUnaryAssignmentsLookup.erase(assignment.var);
        } else {
            ++existingWaitsForAssignedToSignalParts.numWaits;
        }
    }
}


void AssignmentTransformer::tryRemoveWatchForInversionOfAssignment(const syrec::AssignStatement& assignment) {
    if (!awaitedInversionsOfBinaryAssignmentsLookup.count(assignment.lhs)) {
        return;
    }

    const std::optional<syrec_operation::operation> mappedToAssignmentOperationEnumValueFromFlag = syrec_operation::tryMapAssignmentOperationFlagToEnum(assignment.op);
    const std::optional<syrec_operation::operation> mappedToInversionOfAssignmentOperation       = mappedToAssignmentOperationEnumValueFromFlag.has_value() ? syrec_operation::invert(*mappedToAssignmentOperationEnumValueFromFlag) : std::nullopt;
    if (!mappedToInversionOfAssignmentOperation.has_value()) {
        return;
    }

    AwaitedInversionsByBinaryAssignmentLookupEntry& existingWaitsForAssignedToSignal = awaitedInversionsOfBinaryAssignmentsLookup.at(assignment.lhs);
    if(existingWaitsForAssignedToSignal.count(assignment.rhs) && *mappedToInversionOfAssignmentOperation == existingWaitsForAssignedToSignal.at(assignment.rhs)) {
        existingWaitsForAssignedToSignal.erase(assignment.rhs);
        if (existingWaitsForAssignedToSignal.empty()) {
            awaitedInversionsOfBinaryAssignmentsLookup.erase(assignment.lhs);
        }
    }
}

void AssignmentTransformer::tryRemoveWatchForInversionOfAssignment(const syrec::UnaryStatement& assignment) {
    if (!awaitedInversionsOfUnaryAssignmentsLookup.count(assignment.var)) {
        return;
    }

    const std::optional<syrec_operation::operation> mappedToAssignmentOperationEnumValueFromFlag = syrec_operation::tryMapUnaryAssignmentOperationFlagToEnum(assignment.op);
    const std::optional<syrec_operation::operation> mappedToInversionOfAssignmentOperation       = mappedToAssignmentOperationEnumValueFromFlag.has_value() ? syrec_operation::invert(*mappedToAssignmentOperationEnumValueFromFlag) : std::nullopt;
    if (!mappedToInversionOfAssignmentOperation.has_value()) {
        return;
    }

    AwaitedInversionsByUnaryAssignmentLookupEntry& existingWaitsForAssignedToSignal = awaitedInversionsOfUnaryAssignmentsLookup.at(assignment.var);
    if (existingWaitsForAssignedToSignal.numWaits == 1) {
        --existingWaitsForAssignedToSignal.numWaits;
        awaitedInversionsOfUnaryAssignmentsLookup.erase(assignment.var);
    } else {
        --existingWaitsForAssignedToSignal.numWaits;
    }
}


void AssignmentTransformer::updateEntryInValueLookupCacheByPerformingAssignment(const syrec::VariableAccess::ptr& assignedToSignalParts, const std::optional<syrec_operation::operation>& assignmentOperation, const syrec::expression& expr) const {
    std::optional<unsigned int> valueOfExpr;
    if (const auto& exprAsNumericExpr = dynamic_cast<const syrec::NumericExpression*>(&expr); exprAsNumericExpr && exprAsNumericExpr->value->isConstant()) {
        valueOfExpr = exprAsNumericExpr->value->evaluate({});
    }

    if (!getSignalValueCacheEntryFor(assignedToSignalParts)) {
        return;
    }

    const SignalValueLookupCacheEntryReference signalValueCacheEntry                = getSignalValueCacheEntryFor(assignedToSignalParts);
    const std::optional<unsigned int>          currentValueOfAssignedToSignalParts  = signalValueCacheEntry->getValue();
    const std::optional<unsigned int>          newValueAfterApplicationOfAssignment = assignmentOperation.has_value() ? syrec_operation::apply(*assignmentOperation, currentValueOfAssignedToSignalParts, valueOfExpr) : std::nullopt;
    signalValueCacheEntry->updateValueTo(newValueAfterApplicationOfAssignment);
}

void AssignmentTransformer::tryReplaceAddAssignOperationWithXorOperation(syrec::AssignStatement& assignment) const {
    const std::optional<syrec_operation::operation> mappedToAssignmentOperation = syrec_operation::tryMapAssignmentOperationFlagToEnum(assignment.op);
    if (!mappedToAssignmentOperation.has_value()) {
        return;
    }

    if (const SignalValueLookupCacheEntryReference signalValueLookupCacheEntry = getSignalValueCacheEntryFor(assignment.lhs); signalValueLookupCacheEntry) {
        const std::optional<unsigned int> currentSignalValue = signalValueLookupCacheEntry->getValue();
        if (currentSignalValue.has_value() && !*currentSignalValue && *mappedToAssignmentOperation == syrec_operation::operation::AddAssign) {
            if (const std::optional<unsigned int> mappedToFlagForXorAssignOperation = syrec_operation::tryMapAssignmentOperationEnumToFlag(syrec_operation::operation::XorAssign); mappedToFlagForXorAssignOperation.has_value()) {
                assignment.op = *mappedToFlagForXorAssignOperation;    
            }
        }
    }
}

void AssignmentTransformer::invalidateSignalValueLookupEntryFor(const syrec::VariableAccess::ptr& assignedToSignalParts) const {
    if (const SignalValueLookupCacheEntryReference signalValueCacheEntry = getSignalValueCacheEntryFor(assignedToSignalParts); signalValueCacheEntry) {
        signalValueCacheEntry->invalidateCurrentValue();
    }
}

void AssignmentTransformer::handleAllExpectedInversionsOfAssignedToSignalPartsDefined(const syrec::VariableAccess::ptr& assignedToSignalParts) const {
    if (const SignalValueLookupCacheEntryReference signalValueCacheEntry = getSignalValueCacheEntryFor(assignedToSignalParts); signalValueCacheEntry) {
        signalValueCacheEntry->markInitialValueAsActive();
    }
}

void AssignmentTransformer::createSignalValueLookupCacheEntry(const syrec::VariableAccess::ptr& assignedToSignalParts, const InitialSignalValueLookupCallback& initialSignalValueLookupCallback) {
    if (getSignalValueCacheEntryFor(assignedToSignalParts)) {
        return;
    }

    const std::optional<unsigned int>    fetchedInitialValueOfAssignedToSignalParts = initialSignalValueLookupCallback(*assignedToSignalParts);
    SignalValueLookupCacheEntryReference signalValueCacheEntry                      = std::make_shared<SignalValueLookupCacheEntry>(fetchedInitialValueOfAssignedToSignalParts);
    signalValueLookupCache.emplace(std::make_pair(assignedToSignalParts, signalValueCacheEntry));
}


bool AssignmentTransformer::areMoreInversionsRequiredToReachOriginalSignalValue(const syrec::VariableAccess::ptr& accessedSignalParts) const {
    return awaitedInversionsOfBinaryAssignmentsLookup.count(accessedSignalParts) || awaitedInversionsOfUnaryAssignmentsLookup.count(accessedSignalParts);
}

bool AssignmentTransformer::doesAssignmentMatchWatchForInvertedAssignment(const syrec::AssignStatement& assignment) const {
    if (!awaitedInversionsOfBinaryAssignmentsLookup.count(assignment.lhs)) {
        return false;
    }

    const std::optional<syrec_operation::operation> mappedToAssignmentOperationEnumValueFromFlag = syrec_operation::tryMapAssignmentOperationFlagToEnum(assignment.op);
    if (!mappedToAssignmentOperationEnumValueFromFlag.has_value() || !awaitedInversionsOfBinaryAssignmentsLookup.at(assignment.lhs).count(assignment.rhs)) {
        return false;
    }

    const syrec_operation::operation expectedInvertedAssignmentOperation = awaitedInversionsOfBinaryAssignmentsLookup.at(assignment.lhs).at(assignment.rhs);
    return expectedInvertedAssignmentOperation == *mappedToAssignmentOperationEnumValueFromFlag;
}

bool AssignmentTransformer::doesAssignmentMatchWatchForInvertedAssignment(const syrec::UnaryStatement& assignment) const {
    if (!awaitedInversionsOfUnaryAssignmentsLookup.count(assignment.var)) {
        return false;
    }

    const std::optional<syrec_operation::operation> mappedToUnaryAssignmentOperationEnumValueFromFlag = syrec_operation::tryMapUnaryAssignmentOperationFlagToEnum(assignment.op);
    const std::optional<syrec_operation::operation> invertedUnaryAssignmentOperationEnumValueFromFlag = mappedToUnaryAssignmentOperationEnumValueFromFlag.has_value() ? syrec_operation::invert(*mappedToUnaryAssignmentOperationEnumValueFromFlag) : std::nullopt;
    return invertedUnaryAssignmentOperationEnumValueFromFlag.has_value() && awaitedInversionsOfUnaryAssignmentsLookup.at(assignment.var).awaitedInvertedAssignmentOperation == *invertedUnaryAssignmentOperationEnumValueFromFlag;
}


std::optional<unsigned> AssignmentTransformer::fetchCurrentValueOfAssignedToSignal(const syrec::VariableAccess::ptr& accessedSignalParts) const {
    if (signalValueLookupCache.count(accessedSignalParts)) {
        return signalValueLookupCache.at(accessedSignalParts)->currentValue;
    }
    return std::nullopt;
}


syrec::AssignStatement::vec AssignmentTransformer::trySplitAssignmentRhsExpr(const syrec::AssignStatement::ptr& assignment) const {
    if (const auto& assignmentCasted = std::dynamic_pointer_cast<syrec::AssignStatement>(assignment); assignmentCasted) {
        return assignmentRhsExprSplitter->createSubAssignmentsBySplitOfExpr(assignment, fetchCurrentValueOfAssignedToSignal(assignmentCasted->lhs));    
    }
    return {assignment};
}

AssignmentTransformer::SignalValueLookupCacheEntryReference AssignmentTransformer::getSignalValueCacheEntryFor(const syrec::VariableAccess::ptr& assignedToSignalParts) const {
    return signalValueLookupCache.count(assignedToSignalParts) ? signalValueLookupCache.at(assignedToSignalParts) : nullptr;
}

std::optional<std::shared_ptr<syrec::UnaryStatement>> AssignmentTransformer::tryReplaceBinaryAssignmentWithUnaryOne(const syrec::AssignStatement& assignment) {
    const std::optional<syrec_operation::operation> mappedToAssignmentOperation = syrec_operation::tryMapAssignmentOperationFlagToEnum(assignment.op);
    if (!mappedToAssignmentOperation.has_value()) {
        return std::nullopt;
    }

    std::optional<unsigned int> mappedToUnaryAssignmentOperationEnumValueToFlag;
    if (const std::shared_ptr<syrec::NumericExpression> assignmentRhsExprAsNumericExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(assignment.rhs); assignmentRhsExprAsNumericExpr) {
        if (!assignmentRhsExprAsNumericExpr->value->isConstant() || assignmentRhsExprAsNumericExpr->value->evaluate({}) != 1) {
            return std::nullopt;
        }
        
        if (*mappedToAssignmentOperation == syrec_operation::operation::AddAssign) {
            mappedToUnaryAssignmentOperationEnumValueToFlag = syrec_operation::tryMapUnaryAssignmentOperationEnumToFlag(syrec_operation::operation::IncrementAssign);
        } else if (*mappedToAssignmentOperation == syrec_operation::operation::MinusAssign) {
            mappedToUnaryAssignmentOperationEnumValueToFlag = syrec_operation::tryMapUnaryAssignmentOperationEnumToFlag(syrec_operation::operation::DecrementAssign);
        }
    }

    if (!mappedToUnaryAssignmentOperationEnumValueToFlag.has_value()) {
        return std::nullopt;
    }
    if (const std::shared_ptr<syrec::UnaryStatement> generatedUnaryAssignment = std::make_shared<syrec::UnaryStatement>(*mappedToUnaryAssignmentOperationEnumValueToFlag, assignment.lhs); generatedUnaryAssignment) {
        return generatedUnaryAssignment;
    }
    return std::nullopt;
}