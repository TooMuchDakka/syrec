#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/temporary_assignments_container.hpp"
#include "core/syrec/parser/operation.hpp"
#include "core/syrec/parser/utils/signal_access_utils.hpp"

#include <algorithm>

using namespace noAdditionalLineSynthesis;

void TemporaryAssignmentsContainer::storeActiveAssignment(const syrec::AssignStatement::ptr& assignment) {
    if (const auto& assignmentCasted = std::dynamic_pointer_cast<syrec::AssignStatement>(assignment); assignmentCasted) {
        const auto& assignedToSignalIdent = assignmentCasted->lhs->var->name;
        if (!activeAssignmentsLookup.count(assignedToSignalIdent)) {
            std::unordered_set<syrec::VariableAccess::ptr> assignedToSignalParts;
            assignedToSignalParts.insert(assignmentCasted->lhs);
            activeAssignmentsLookup.insert(std::make_pair(assignedToSignalIdent, assignedToSignalParts));
        } else {
            auto& existingAssignedToSignalParts = activeAssignmentsLookup.at(assignedToSignalIdent);
            if (!existingAssignedToSignalParts.count(assignmentCasted->lhs)) {
                existingAssignedToSignalParts.insert(assignmentCasted->lhs);
            }
        }
        generatedAssignments.emplace_back(assignment);
        indicesOfActiveAssignments.emplace_back(generatedAssignments.size() - 1);
    }
}

void TemporaryAssignmentsContainer::markCutoffForInvertibleAssignments() {
    cutOffIndicesForInvertibleAssignments.emplace_back(generatedAssignments.size());
}

void TemporaryAssignmentsContainer::invertAllAssignmentsUpToLastCutoff(std::size_t numberOfAssignmentToExcludeFromInversionStartingFromLastGeneratedOne) {
    std::vector<std::size_t> relevantIndicesOfActiveAssignments = cutOffIndicesForInvertibleAssignments.empty() ? indicesOfActiveAssignments : determineIndicesOfInvertibleAssignmentsStartingFrom(cutOffIndicesForInvertibleAssignments.back());

    if (numberOfAssignmentToExcludeFromInversionStartingFromLastGeneratedOne && !relevantIndicesOfActiveAssignments.empty()) {
        const std::size_t numEntriesToPop = std::min(numberOfAssignmentToExcludeFromInversionStartingFromLastGeneratedOne, relevantIndicesOfActiveAssignments.size());
        relevantIndicesOfActiveAssignments.erase(std::next(relevantIndicesOfActiveAssignments.begin(), relevantIndicesOfActiveAssignments.size() - numEntriesToPop), relevantIndicesOfActiveAssignments.end());
    }

    for (auto activeAssignmentIndicesIterator = relevantIndicesOfActiveAssignments.rbegin(); activeAssignmentIndicesIterator != relevantIndicesOfActiveAssignments.rend(); ++activeAssignmentIndicesIterator) {
        const syrec::AssignStatement::ptr& referenceActiveAssignment = generatedAssignments.at(*activeAssignmentIndicesIterator);
        if (const auto& referenceAssignmentCasted = std::dynamic_pointer_cast<syrec::AssignStatement>(referenceActiveAssignment); referenceAssignmentCasted) {
            invertAssignmentAndStoreAndMarkOriginalAsInactive(*referenceAssignmentCasted);
        }
    }
}

void TemporaryAssignmentsContainer::popLastCutoffForInvertibleAssignments() {
    if (!cutOffIndicesForInvertibleAssignments.empty()) {
        cutOffIndicesForInvertibleAssignments.pop_back();
    }
}


void TemporaryAssignmentsContainer::rollbackLastXAssignments(std::size_t numberOfAssignmentsToRemove) {
    const std::size_t actualNumberOfAssignmentsRemoved = std::min(numberOfAssignmentsToRemove, generatedAssignments.size());
    if (!actualNumberOfAssignmentsRemoved) {
        return;
    }

    std::size_t indexOfFirstRemovedAssignment = generatedAssignments.size() - actualNumberOfAssignmentsRemoved;
    indicesOfActiveAssignments.erase(std::remove_if(
            indicesOfActiveAssignments.begin(),
            indicesOfActiveAssignments.end(),
            [&indexOfFirstRemovedAssignment](const std::size_t activeAssignmentIndex) {
                return activeAssignmentIndex >= indexOfFirstRemovedAssignment;
    }), indicesOfActiveAssignments.end());
    
    for (auto generatedAssignmentsToBeRemovedIterator = std::next(generatedAssignments.begin(), indexOfFirstRemovedAssignment); generatedAssignmentsToBeRemovedIterator != generatedAssignments.end();) {
        if (const auto& referencedAssignmentCasted = std::dynamic_pointer_cast<syrec::AssignStatement>(*generatedAssignmentsToBeRemovedIterator); referencedAssignmentCasted) {
            removeActiveAssignmentFromLookup(referencedAssignmentCasted->lhs);   
        }
        generatedAssignmentsToBeRemovedIterator = generatedAssignments.erase(generatedAssignmentsToBeRemovedIterator);
    }
}

void TemporaryAssignmentsContainer::rollbackAssignmentsMadeSinceLastCutoffAndOptionallyPopCutoff(bool popCutOff) {
    std::size_t       indexOfFirstRemovedAssignment = 0;
    if (!cutOffIndicesForInvertibleAssignments.empty()) {
        indexOfFirstRemovedAssignment = cutOffIndicesForInvertibleAssignments.back();
        if (popCutOff) {
            cutOffIndicesForInvertibleAssignments.pop_back();   
        }
    }
    rollbackLastXAssignments(generatedAssignments.size() - indexOfFirstRemovedAssignment);
}


void TemporaryAssignmentsContainer::resetInternals() {
    generatedAssignments.clear();
    indicesOfActiveAssignments.clear();
    activeAssignmentsLookup.clear();
    cutOffIndicesForInvertibleAssignments.clear();
}

syrec::AssignStatement::vec TemporaryAssignmentsContainer::getAssignments() const {
    return generatedAssignments;
}

bool TemporaryAssignmentsContainer::existsOverlappingAssignmentFor(const syrec::VariableAccess& assignedToSignal, const parser::SymbolTable& symbolTable) const {
    if (!activeAssignmentsLookup.count(assignedToSignal.var->name)) {
        return false;
    }

    const auto& matchingActiveAssignmentsForSignalIdent = activeAssignmentsLookup.at(assignedToSignal.var->name);
    return !std::all_of(
            matchingActiveAssignmentsForSignalIdent.cbegin(),
            matchingActiveAssignmentsForSignalIdent.cend(),
            [&assignedToSignal, symbolTable](const syrec::VariableAccess::ptr& activeAssignedToSignals) {
                const auto& equivalenceResult = SignalAccessUtils::areSignalAccessesEqual(
                        assignedToSignal, *activeAssignedToSignals,
                        SignalAccessUtils::SignalAccessComponentEquivalenceCriteria::DimensionAccess::Overlapping,
                        SignalAccessUtils::SignalAccessComponentEquivalenceCriteria::BitRange::Overlapping, symbolTable);
                return equivalenceResult.isResultCertain && equivalenceResult.equality == SignalAccessUtils::SignalAccessEquivalenceResult::NotEqual;
            });
}

std::size_t TemporaryAssignmentsContainer::getNumberOfAssignments() const {
    return generatedAssignments.size();
}

// START OF NON-PUBLIC FUNCTIONS
void TemporaryAssignmentsContainer::invertAssignmentAndStoreAndMarkOriginalAsInactive(const syrec::AssignStatement& assignmentToInvert) {
    const std::vector<std::size_t> activeAssignmentsForCurrentCutoff = cutOffIndicesForInvertibleAssignments.empty()
        ? indicesOfActiveAssignments
        : determineIndicesOfInvertibleAssignmentsStartingFrom(cutOffIndicesForInvertibleAssignments.back());

    /*
     * Perform a search for the index of the reference active assignment and mark it as inactive by removing said index from the indices of the active assignments
     */
    for (auto activeAssignmentIndexIterator = activeAssignmentsForCurrentCutoff.rbegin(); activeAssignmentIndexIterator != activeAssignmentsForCurrentCutoff.rend();) {
        const auto& referenceGeneratedAssignment = generatedAssignments.at(*activeAssignmentIndexIterator);
        if (const auto& referenceAssignmentCasted = std::dynamic_pointer_cast<syrec::AssignStatement>(referenceGeneratedAssignment); referenceAssignmentCasted) {
            if (referenceAssignmentCasted->lhs == assignmentToInvert.lhs) {
                const auto& index = std::distance(activeAssignmentsForCurrentCutoff.rbegin(), activeAssignmentIndexIterator);
                indicesOfActiveAssignments.erase(std::next(indicesOfActiveAssignments.begin(), index));
                break;
            }
        }
        ++activeAssignmentIndexIterator;
    }
    removeActiveAssignmentFromLookup(assignmentToInvert.lhs);
    syrec::AssignStatement::ptr generatedInvertedAssignment = invertAssignment(assignmentToInvert);
    generatedAssignments.emplace_back(generatedInvertedAssignment);
}

syrec::AssignStatement::ptr TemporaryAssignmentsContainer::invertAssignment(const syrec::AssignStatement& assignment) {
    const auto& matchingAssignmentOperationEnumForFlag = *syrec_operation::tryMapAssignmentOperationFlagToEnum(assignment.op);
    const auto& invertedAssignmentOperation            = *syrec_operation::invert(matchingAssignmentOperationEnumForFlag);
    const auto& mappedToAssignmentOperationFlagForInvertedOperation = syrec_operation::tryMapAssignmentOperationEnumToFlag(invertedAssignmentOperation);
    syrec::AssignStatement::ptr invertedAssignment                                  = std::make_shared<syrec::AssignStatement>(
        assignment.lhs, *mappedToAssignmentOperationFlagForInvertedOperation, assignment.rhs);
    return invertedAssignment;
}

std::vector<std::size_t> TemporaryAssignmentsContainer::determineIndicesOfInvertibleAssignmentsStartingFrom(std::size_t firstRelevantAssignmentIndex) const {
    const auto& indexOfFirstActiveAssignmentWithLargerIndex = std::find_if(
            indicesOfActiveAssignments.cbegin(),
            indicesOfActiveAssignments.cend(),
            [&firstRelevantAssignmentIndex](const std::size_t activeAssignmentIndex) {
                return activeAssignmentIndex > firstRelevantAssignmentIndex;
            });
    if (indexOfFirstActiveAssignmentWithLargerIndex == indicesOfActiveAssignments.cend()) {
        return {};
    }

    std::vector<std::size_t> containerForIndicesOfActiveAssignments;
    containerForIndicesOfActiveAssignments.assign(indexOfFirstActiveAssignmentWithLargerIndex, indicesOfActiveAssignments.cend());
    return containerForIndicesOfActiveAssignments;
}

void TemporaryAssignmentsContainer::removeActiveAssignmentFromLookup(const syrec::VariableAccess::ptr& assignedToSignal) {
    if (!activeAssignmentsLookup.count(assignedToSignal->var->name)) {
        return;
    }

    auto& activeAssignmentsForAssignedToSignalIdent = activeAssignmentsLookup.at(assignedToSignal->var->name);
    activeAssignmentsForAssignedToSignalIdent.erase(assignedToSignal);
    if (activeAssignmentsForAssignedToSignalIdent.empty()) {
        activeAssignmentsLookup.erase(assignedToSignal->var->name);
    }
}
