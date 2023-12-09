#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/assignment_transformer.hpp"

using namespace noAdditionalLineSynthesis;

syrec::AssignStatement::vec AssignmentTransformer::simplify(const syrec::AssignStatement::vec& assignmentsToSimplify, const InitialSignalValueLookupCallback& initialSignalValueLookupCallback) {
    syrec::AssignStatement::vec generatedAssignments;

    for (const syrec::AssignStatement::ptr& assignment: assignmentsToSimplify) {
        const std::shared_ptr<syrec::AssignStatement> assignmentCasted = std::dynamic_pointer_cast<syrec::AssignStatement>(assignment);
        if (!assignmentCasted) {
            continue;
        }

        const std::optional<syrec_operation::operation> matchingAssignmentOperationEnumValueForFlag = syrec_operation::tryMapAssignmentOperationFlagToEnum(assignmentCasted->op);
        if (!matchingAssignmentOperationEnumValueForFlag.has_value()) {
            continue;
        }

        if (areMoreInversionsRequiredToReachOriginalSignalValue(assignmentCasted->lhs)) {
            if (doesAssignmentMatchWatchForInvertedAssignment(*assignmentCasted)) {
                tryRemoveWatchForInversionOfAssignment(*assignmentCasted);
            } else {
                addWatchForInversionOfAssignment(*assignmentCasted);
            }
        } else {
            createSignalValueLookupCacheEntry(assignmentCasted->lhs, initialSignalValueLookupCallback);
            addWatchForInversionOfAssignment(*assignmentCasted);
        }

        syrec::AssignStatement::vec generatedSubAssignments = trySplitAssignmentRhsExpr(assignment);
        for (const syrec::AssignStatement::ptr& generatedSubAssignment: generatedSubAssignments) {
            const std::shared_ptr<syrec::AssignStatement>   subAssignmentCasted                                 = std::dynamic_pointer_cast<syrec::AssignStatement>(generatedSubAssignment);
            const std::optional<syrec_operation::operation> mappedToOperationEnumValueForSubassignmentOperation = syrec_operation::tryMapAssignmentOperationFlagToEnum(subAssignmentCasted->op);
            tryReplaceAddAssignOperationWithXorOperation(*subAssignmentCasted);
            updateEntryInValueLookupCacheByPerformingAssignment(subAssignmentCasted->lhs, *mappedToOperationEnumValueForSubassignmentOperation, *subAssignmentCasted->rhs);
            generatedAssignments.emplace_back(subAssignmentCasted);
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
    awaitedAssignmentsLookup.clear();
    signalValueLookupCache.clear();
}

void AssignmentTransformer::addWatchForInversionOfAssignment(const syrec::AssignStatement& assignment) {
    const std::optional<syrec_operation::operation> mappedToAssignmentOperationEnumValueFromFlag = syrec_operation::tryMapAssignmentOperationFlagToEnum(assignment.op);
    const std::optional<syrec_operation::operation> invertedAssignmentOperation                  = mappedToAssignmentOperationEnumValueFromFlag.has_value() ? syrec_operation::invert(*mappedToAssignmentOperationEnumValueFromFlag) : std::nullopt;
        
    if (!awaitedAssignmentsLookup.count(assignment.lhs)) {
        AwaitedInversionsOfAssignedToSignalsParts awaitedInversionOfAssignedToSignalParts = std::make_shared<std::unordered_map<syrec::expression::ptr, syrec_operation::operation>>();
        if (awaitedInversionOfAssignedToSignalParts && invertedAssignmentOperation.has_value()) {
            awaitedInversionOfAssignedToSignalParts->emplace(std::make_pair(assignment.rhs, *invertedAssignmentOperation));
        }
        awaitedAssignmentsLookup.emplace(std::make_pair(assignment.lhs, awaitedInversionOfAssignedToSignalParts));
    } else if (AwaitedInversionsOfAssignedToSignalsParts existingWaitsOnInversion = awaitedAssignmentsLookup.at(assignment.lhs); existingWaitsOnInversion) {
        if (!invertedAssignmentOperation.has_value()) {
            existingWaitsOnInversion = nullptr;
        } else {
            existingWaitsOnInversion->emplace(std::make_pair(assignment.rhs, *invertedAssignmentOperation));
        }
    }
}

void AssignmentTransformer::tryRemoveWatchForInversionOfAssignment(const syrec::AssignStatement& assignment) const {
    if (!awaitedAssignmentsLookup.count(assignment.lhs)) {
        return;
    }

    const std::optional<syrec_operation::operation> mappedToAssignmentOperationEnumValueFromFlag = syrec_operation::tryMapAssignmentOperationFlagToEnum(assignment.op);
    if (const AwaitedInversionsOfAssignedToSignalsParts existingWaitsOnInversion = awaitedAssignmentsLookup.at(assignment.lhs); existingWaitsOnInversion && mappedToAssignmentOperationEnumValueFromFlag.has_value()) {
        if (existingWaitsOnInversion->count(assignment.rhs) && existingWaitsOnInversion->at(assignment.rhs) == *mappedToAssignmentOperationEnumValueFromFlag) {
            existingWaitsOnInversion->erase(assignment.rhs);
        }
    }
}

void AssignmentTransformer::updateEntryInValueLookupCacheByPerformingAssignment(const syrec::VariableAccess::ptr& assignedToSignalParts, const std::optional<syrec_operation::operation>& assignmentOperation, const syrec::expression& expr) const {
    std::optional<unsigned int> valueOfExpr;
    if (const auto& exprAsNumericExpr = dynamic_cast<const syrec::NumericExpression*>(&expr); exprAsNumericExpr && exprAsNumericExpr->value->isConstant()) {
        valueOfExpr = exprAsNumericExpr->value->evaluate({});
    }

    SignalValueLookupCacheEntryReference signalValueCacheEntry;
    if (!getSignalValueCacheEntryFor(assignedToSignalParts)) {
        return;
    }

    signalValueCacheEntry = getSignalValueCacheEntryFor(assignedToSignalParts);
    const std::optional<unsigned int> currentValueOfAssignedToSignalParts = signalValueCacheEntry->getValue();
    const std::optional<unsigned int> newValueAfterApplicationOfAssignment = assignmentOperation.has_value() ? syrec_operation::apply(*assignmentOperation, currentValueOfAssignedToSignalParts, valueOfExpr) : std::nullopt;
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
    if (!awaitedAssignmentsLookup.count(accessedSignalParts)) {
        return false;
    }

    const AwaitedInversionsOfAssignedToSignalsParts stillAwaitedAssignmentInversions = awaitedAssignmentsLookup.at(accessedSignalParts);
    return stillAwaitedAssignmentInversions ? !stillAwaitedAssignmentInversions->empty() : true;
}

bool AssignmentTransformer::doesAssignmentMatchWatchForInvertedAssignment(const syrec::AssignStatement& assignment) const {
    if (!awaitedAssignmentsLookup.count(assignment.lhs)) {
        return false;
    }

    const std::optional<syrec_operation::operation> mappedToAssignmentOperationEnumValueFromFlag = syrec_operation::tryMapAssignmentOperationFlagToEnum(assignment.op);
    if (!mappedToAssignmentOperationEnumValueFromFlag.has_value() || !awaitedAssignmentsLookup.at(assignment.lhs)->count(assignment.rhs)) {
        return false;
    }

    const syrec_operation::operation expectedInvertedAssignmentOperation = awaitedAssignmentsLookup.at(assignment.lhs)->at(assignment.rhs);
    return expectedInvertedAssignmentOperation == *mappedToAssignmentOperationEnumValueFromFlag;
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