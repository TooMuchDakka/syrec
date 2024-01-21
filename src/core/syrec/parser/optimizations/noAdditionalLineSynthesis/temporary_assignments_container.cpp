#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/temporary_assignments_container.hpp"
#include "core/syrec/parser/operation.hpp"
#include "core/syrec/parser/utils/copy_utils.hpp"
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

void TemporaryAssignmentsContainer::storeInitializationForReplacementOfLeafNode(const syrec::AssignStatement::ptr& assignmentDefiningSubstitution, const std::optional<syrec::AssignStatement::ptr>& optionalRequiredResetOfSubstitutionLhsOperand) {
    const std::shared_ptr<syrec::AssignStatement> castedAssignmentDefiningSubstitution = std::dynamic_pointer_cast<syrec::AssignStatement>(assignmentDefiningSubstitution);
    const std::shared_ptr<syrec::AssignStatement> castedRequiredResetAssignment        = optionalRequiredResetOfSubstitutionLhsOperand.has_value() ? std::dynamic_pointer_cast<syrec::AssignStatement>(*optionalRequiredResetOfSubstitutionLhsOperand) : nullptr;

    if (!castedAssignmentDefiningSubstitution || (optionalRequiredResetOfSubstitutionLhsOperand.has_value() && !castedRequiredResetAssignment)) {
        return;
    }

    if (castedRequiredResetAssignment) {
        initializationAssignmentsForGeneratedSubstitutions.emplace_back(castedRequiredResetAssignment);   
    }
    initializationAssignmentsForGeneratedSubstitutions.emplace_back(castedAssignmentDefiningSubstitution);
}

void TemporaryAssignmentsContainer::storeReplacementAsActiveAssignment(const syrec::AssignStatement::ptr& assignmentDefiningSubstitution, const std::optional<syrec::AssignStatement::ptr>& optionalRequiredResetOfSubstitutionLhsOperand) {
    const std::shared_ptr<syrec::AssignStatement> castedAssignmentDefiningSubstitution = std::dynamic_pointer_cast<syrec::AssignStatement>(assignmentDefiningSubstitution);
    const std::shared_ptr<syrec::AssignStatement> castedRequiredResetAssignment        = optionalRequiredResetOfSubstitutionLhsOperand.has_value() ? std::dynamic_pointer_cast<syrec::AssignStatement>(*optionalRequiredResetOfSubstitutionLhsOperand) : nullptr;

    if (!castedAssignmentDefiningSubstitution || (optionalRequiredResetOfSubstitutionLhsOperand.has_value() && !castedRequiredResetAssignment)) {
        return;
    }

    if (castedRequiredResetAssignment) {
        initializationAssignmentsForGeneratedSubstitutions.emplace_back(castedRequiredResetAssignment);
    }
    storeActiveAssignment(castedAssignmentDefiningSubstitution);
}

void TemporaryAssignmentsContainer::markCutoffForInvertibleAssignments() {
    cutOffIndicesForInvertibleAssignments.emplace_back(generatedAssignments.size());
}

void TemporaryAssignmentsContainer::invertAllAssignmentsUpToLastCutoff(std::size_t numberOfAssignmentToExcludeFromInversionStartingFromLastGeneratedOne, const syrec::VariableAccess::ptr* optionalExcludedFromInversionAssignmentsFilter) {
    std::vector<std::size_t> relevantIndicesOfActiveAssignments = cutOffIndicesForInvertibleAssignments.empty() ? indicesOfActiveAssignments : determineIndicesOfInvertibleAssignmentsStartingFrom(cutOffIndicesForInvertibleAssignments.back());

    if (numberOfAssignmentToExcludeFromInversionStartingFromLastGeneratedOne && !relevantIndicesOfActiveAssignments.empty()) {
        const std::size_t numEntriesToPop = std::min(numberOfAssignmentToExcludeFromInversionStartingFromLastGeneratedOne, relevantIndicesOfActiveAssignments.size());
        relevantIndicesOfActiveAssignments.erase(std::next(relevantIndicesOfActiveAssignments.begin(), relevantIndicesOfActiveAssignments.size() - numEntriesToPop), relevantIndicesOfActiveAssignments.end());
    }

    for (auto activeAssignmentIndicesIterator = relevantIndicesOfActiveAssignments.rbegin(); activeAssignmentIndicesIterator != relevantIndicesOfActiveAssignments.rend(); ++activeAssignmentIndicesIterator) {
        const syrec::AssignStatement::ptr& referenceActiveAssignment = generatedAssignments.at(*activeAssignmentIndicesIterator);
        if (const auto& referenceAssignmentCasted = std::dynamic_pointer_cast<syrec::AssignStatement>(referenceActiveAssignment); referenceAssignmentCasted) {
            if (!optionalExcludedFromInversionAssignmentsFilter || referenceAssignmentCasted->lhs != *optionalExcludedFromInversionAssignmentsFilter) {
                invertAssignmentAndStoreAndMarkOriginalAsInactive(*activeAssignmentIndicesIterator);   
            }
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
    initializationAssignmentsForGeneratedSubstitutions.clear();
}

syrec::AssignStatement::vec TemporaryAssignmentsContainer::getAssignments() const {
    return generatedAssignments;
}

std::optional<std::vector<std::unique_ptr<syrec::AssignStatement>>> TemporaryAssignmentsContainer::getAssignmentsDefiningValueResetsOfGeneratedSubstitutions() const {
    return createOwningCopiesOfAssignments(initializationAssignmentsForGeneratedSubstitutions);
}

std::optional<std::vector<std::unique_ptr<syrec::AssignStatement>>> TemporaryAssignmentsContainer::getInvertedAssignmentsUndoingValueResetsOfGeneratedSubstitutions() const {
    std::vector<std::unique_ptr<syrec::AssignStatement>> containerForOwningCopiesOfAssignments;
    if (initializationAssignmentsForGeneratedSubstitutions.size() > (UINT_MAX / 2)) {
        return std::nullopt;
    }

    containerForOwningCopiesOfAssignments.reserve(initializationAssignmentsForGeneratedSubstitutions.size() * 2);
    if (std::optional<std::vector<std::unique_ptr<syrec::AssignStatement>>> owningCopiesOfInitializationAssignments = getAssignmentsDefiningValueResetsOfGeneratedSubstitutions(); owningCopiesOfInitializationAssignments.has_value()) {
        containerForOwningCopiesOfAssignments.insert(containerForOwningCopiesOfAssignments.end(), std::make_move_iterator(owningCopiesOfInitializationAssignments->begin()), std::make_move_iterator(owningCopiesOfInitializationAssignments->end()));

        for (auto&& initializationStatement: *owningCopiesOfInitializationAssignments) {
            if (std::unique_ptr<syrec::AssignStatement> owningContainerForInversionAssignment = std::make_unique<syrec::AssignStatement>(*initializationStatement); owningContainerForInversionAssignment) {
                invertAssignment(*owningContainerForInversionAssignment);
                containerForOwningCopiesOfAssignments.push_back(std::move(owningContainerForInversionAssignment));
                continue;
            }
            return std::nullopt;
        }
    }
    return containerForOwningCopiesOfAssignments;
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
std::vector<std::size_t> TemporaryAssignmentsContainer::determineIndicesOfInvertibleAssignmentsStartingFrom(std::size_t firstRelevantAssignmentIndex) const {
    const auto& indexOfFirstActiveAssignmentWithLargerIndex = std::find_if(
            indicesOfActiveAssignments.cbegin(),
            indicesOfActiveAssignments.cend(),
            [&firstRelevantAssignmentIndex](const std::size_t activeAssignmentIndex) {
                return activeAssignmentIndex >= firstRelevantAssignmentIndex;
            });
    if (indexOfFirstActiveAssignmentWithLargerIndex == indicesOfActiveAssignments.cend()) {
        return {};
    }

    std::vector<std::size_t> containerForIndicesOfActiveAssignments;
    containerForIndicesOfActiveAssignments.assign(indexOfFirstActiveAssignmentWithLargerIndex, indicesOfActiveAssignments.cend());
    return containerForIndicesOfActiveAssignments;
}

void TemporaryAssignmentsContainer::invertAssignmentAndStoreAndMarkOriginalAsInactive(std::size_t indexOfAssignmentToInvert) {
    /*
     * Perform a search for the index of the reference active assignment and mark it as inactive by removing said index from the indices of the active assignments
     */
    const auto& referenceGeneratedAssignment = generatedAssignments.at(indexOfAssignmentToInvert);
    if (const auto& referenceAssignmentCasted = std::dynamic_pointer_cast<syrec::AssignStatement>(referenceGeneratedAssignment); referenceAssignmentCasted) {
        const auto& indexOfActiveAssignmentToRemove = std::find_if(
                indicesOfActiveAssignments.cbegin(),
                indicesOfActiveAssignments.cend(),
                [&indexOfAssignmentToInvert](const std::size_t indexOfActiveAssignment) {
                    return indexOfAssignmentToInvert == indexOfActiveAssignment;
                });
        if (indexOfActiveAssignmentToRemove != indicesOfActiveAssignments.cend()) {
            indicesOfActiveAssignments.erase(indexOfActiveAssignmentToRemove);
        }

        removeActiveAssignmentFromLookup(referenceAssignmentCasted->lhs);

        std::shared_ptr<syrec::AssignStatement> generatedInvertedAssignment = copyUtils::createDeepCopyOfAssignmentStmt(*referenceAssignmentCasted);
        invertAssignment(*generatedInvertedAssignment);
        generatedAssignments.emplace_back(generatedInvertedAssignment);
    }
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

void TemporaryAssignmentsContainer::invertAssignment(syrec::AssignStatement& assignment) {
    const auto& matchingAssignmentOperationEnumForFlag              = *syrec_operation::tryMapAssignmentOperationFlagToEnum(assignment.op);
    const auto& invertedAssignmentOperation                         = *syrec_operation::invert(matchingAssignmentOperationEnumForFlag);
    const auto& mappedToAssignmentOperationFlagForInvertedOperation = syrec_operation::tryMapAssignmentOperationEnumToFlag(invertedAssignmentOperation);
    assignment.op                                                   = *mappedToAssignmentOperationFlagForInvertedOperation;
}

std::optional<std::unique_ptr<syrec::AssignStatement>> TemporaryAssignmentsContainer::createOwningCopyOfAssignment(const syrec::AssignStatement& assignment) {
    if (std::unique_ptr<syrec::AssignStatement> owningCopyOfAssignment = std::make_unique<syrec::AssignStatement>(assignment); owningCopyOfAssignment) {
        return owningCopyOfAssignment;
    }
    return std::nullopt;
}

std::optional<std::vector<std::unique_ptr<syrec::AssignStatement>>> TemporaryAssignmentsContainer::createOwningCopiesOfAssignments(const syrec::AssignStatement::vec& assignments) {
    std::vector<std::unique_ptr<syrec::AssignStatement>> containerForCopies;
    containerForCopies.reserve(assignments.size());
    
    for (const auto& statementToTreatAsAssignment: assignments) {
        if (const std::shared_ptr<syrec::AssignStatement> statementCastedAsAssignment = std::dynamic_pointer_cast<syrec::AssignStatement>(statementToTreatAsAssignment); statementCastedAsAssignment) {
            if (std::optional<std::unique_ptr<syrec::AssignStatement>> copyOfAssignment = createOwningCopyOfAssignment(*statementCastedAsAssignment); copyOfAssignment.has_value()) {
                containerForCopies.push_back(std::move(*copyOfAssignment));
                continue;
            }
        }
        return std::nullopt;
    }
    return containerForCopies;
}