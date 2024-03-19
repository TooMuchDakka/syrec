#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/temporary_assignments_container.hpp"
#include "core/syrec/parser/operation.hpp"
#include "core/syrec/parser/utils/copy_utils.hpp"
#include "core/syrec/parser/utils/signal_access_utils.hpp"

#include <algorithm>

using namespace noAdditionalLineSynthesis;

std::shared_ptr<syrec::VariableAccess> TemporaryAssignmentsContainer::InternalAssignmentContainer::getAssignedToSignalParts() const {
    if (std::holds_alternative<std::shared_ptr<syrec::AssignStatement>>(data)) {
        return std::get<std::shared_ptr<syrec::AssignStatement>>(data)->lhs;
    }
    return std::get<std::shared_ptr<syrec::UnaryStatement>>(data)->var;
}


std::optional<std::size_t> TemporaryAssignmentsContainer::getOverlappingActiveAssignmentForSignalAccessWithIdNotInRange(const syrec::VariableAccess& signalAccess, std::size_t exclusionRangeLowerBoundDefinedWithAssociatedOperationNodeId, std::size_t exclusionRangeUpperBoundDefinedWithAssociatedOperationNodeId, const parser::SymbolTable& symbolTableReference) const {
    const auto& foundMatchingActiveAssignment = std::find_if(
            activeAssignmentsLookupById.cbegin(),
            activeAssignmentsLookupById.cend(),
            [&](std::size_t activeAssignmentIdFromLookup) {
                const std::optional<InternalAssignmentContainer::ReadOnlyReference> activeAssignmentMatchingId = getMatchingActiveSubassignmentById(activeAssignmentIdFromLookup);
                if (!activeAssignmentMatchingId.has_value() || (activeAssignmentMatchingId->get()->associatedWithOperationNodeId.has_value() && activeAssignmentMatchingId->get()->associatedWithOperationNodeId >= exclusionRangeLowerBoundDefinedWithAssociatedOperationNodeId && activeAssignmentMatchingId->get()->associatedWithOperationNodeId <= exclusionRangeUpperBoundDefinedWithAssociatedOperationNodeId)) {
                    return false;
                }

                syrec::VariableAccess::ptr assignedToSignalPartsOfActiveAssignment;
                if (const std::optional<std::shared_ptr<syrec::AssignStatement>>& activeAssignmentAsBinaryAssignment = tryGetActiveAssignmentAsBinaryAssignment(activeAssignmentMatchingId->get()->data); activeAssignmentAsBinaryAssignment.has_value()) {
                    assignedToSignalPartsOfActiveAssignment = activeAssignmentAsBinaryAssignment->get()->lhs;
                } else if (const std::optional<std::shared_ptr<syrec::UnaryStatement>>& activeAssignmentAsUnaryAssignment = tryGetActiveAssignmentAsUnaryAssignment(activeAssignmentMatchingId->get()->data); activeAssignmentAsUnaryAssignment.has_value()) {
                    assignedToSignalPartsOfActiveAssignment = activeAssignmentAsUnaryAssignment->get()->var;
                } else {
                    return false;
                }

                const SignalAccessUtils::SignalAccessEquivalenceResult signalAccessEquivalenceResult = SignalAccessUtils::areSignalAccessesEqual(
                        *assignedToSignalPartsOfActiveAssignment, signalAccess,
                        SignalAccessUtils::SignalAccessComponentEquivalenceCriteria::DimensionAccess::Overlapping, SignalAccessUtils::SignalAccessComponentEquivalenceCriteria::BitRange::Overlapping, symbolTableReference);
                if (!(signalAccessEquivalenceResult.isResultCertain && signalAccessEquivalenceResult.equality == SignalAccessUtils::SignalAccessEquivalenceResult::NotEqual)) {
                    return true;
                }
                return false;
            });
    if (foundMatchingActiveAssignment != activeAssignmentsLookupById.cend()) {
        return getMatchingActiveSubassignmentById(*foundMatchingActiveAssignment)->get()->id;
    }
    return std::nullopt;
}

bool TemporaryAssignmentsContainer::existsActiveAssignmentHavingGivenAssignmentAsDataDependency(std::size_t assignmentIdWhichShallBeUsedAsADataDependency) const {
    for (const std::size_t activeAssignmentId: activeAssignmentsLookupById) {
        if (activeAssignmentId <= assignmentIdWhichShallBeUsedAsADataDependency) {
            return false;
        }
        if (const std::optional<InternalAssignmentContainer::ReadOnlyReference>& activeAssignmentReference = getMatchingActiveSubassignmentById(activeAssignmentId); activeAssignmentReference.has_value() && activeAssignmentReference->get()->dataDependencies.count(assignmentIdWhichShallBeUsedAsADataDependency)) {
            return true;
        }
    }
    return false;
}

TemporaryAssignmentsContainer::OrderedAssignmentIdContainer TemporaryAssignmentsContainer::getOverlappingActiveAssignmentsOrderedByLastAppearanceOfSignalAccessedDefinedInExpression(const syrec::Expression& expression, const parser::SymbolTable& symbolTableReference) const {
    OrderedAssignmentIdContainer resultContainer;
    for (const std::reference_wrapper<syrec::VariableAccess>& signalAccess : getSignalAccessesOfExpression(expression)) {
        resultContainer.merge(getOverlappingActiveAssignmentsOrderedByLastAppearanceOfSignalAccess(signalAccess, symbolTableReference));
    }
    return resultContainer;
}

TemporaryAssignmentsContainer::OrderedAssignmentIdContainer TemporaryAssignmentsContainer::getOverlappingActiveAssignmentsOrderedByLastAppearanceOfSignalAccess(const syrec::VariableAccess& signalAccess, const parser::SymbolTable& symbolTableReference) const {
    OrderedAssignmentIdContainer resultContainer;
    for (const std::size_t activeAssignmentId : activeAssignmentsLookupById) {
        const InternalAssignmentContainer::ReadOnlyReference activeAssignmentReference = internalAssignmentContainer.count(activeAssignmentId) ? internalAssignmentContainer.at(activeAssignmentId) : nullptr;
        if (!activeAssignmentReference || !activeAssignmentReference->isStillActive) {
            continue;
        }

        syrec::VariableAccess::ptr assignedToSignalPartsOfActiveAssignment = nullptr;
        if (const std::optional<std::shared_ptr<syrec::AssignStatement>>& activeAssignmentAsBinaryAssignment = tryGetActiveAssignmentAsBinaryAssignment(activeAssignmentReference->data); activeAssignmentAsBinaryAssignment.has_value()) {
            assignedToSignalPartsOfActiveAssignment = activeAssignmentAsBinaryAssignment->get()->lhs;
        } else if (const std::optional<std::shared_ptr<syrec::UnaryStatement>>& activeAssignmentAsUnaryAssignment = tryGetActiveAssignmentAsUnaryAssignment(activeAssignmentReference->data); activeAssignmentAsUnaryAssignment.has_value()) {
            assignedToSignalPartsOfActiveAssignment = activeAssignmentAsUnaryAssignment->get()->var;
        } else {
            continue;
        }

        const SignalAccessUtils::SignalAccessEquivalenceResult signalAccessEquivalenceResult = SignalAccessUtils::areSignalAccessesEqual(*assignedToSignalPartsOfActiveAssignment, signalAccess,
            SignalAccessUtils::SignalAccessComponentEquivalenceCriteria::DimensionAccess::Overlapping, SignalAccessUtils::SignalAccessComponentEquivalenceCriteria::BitRange::Overlapping, symbolTableReference);
        if (signalAccessEquivalenceResult.isResultCertain && signalAccessEquivalenceResult.equality == SignalAccessUtils::SignalAccessEquivalenceResult::NotEqual) {
            continue;
        }
        resultContainer.emplace(activeAssignmentReference->id);
    }
    return resultContainer;
}

TemporaryAssignmentsContainer::OrderedAssignmentIdContainer TemporaryAssignmentsContainer::getDataDependenciesOfAssignment(std::size_t assignmentId) const {
    if (!internalAssignmentContainer.count(assignmentId)) {
        return {};
    }
    return internalAssignmentContainer.at(assignmentId)->dataDependencies;
}

std::optional<std::size_t> TemporaryAssignmentsContainer::getIdOfLastGeneratedAssignment() const {
    return internalAssignmentContainer.empty() ? std::nullopt : std::make_optional(internalAssignmentContainer.cbegin()->first);
}

bool TemporaryAssignmentsContainer::invertAssignmentsWithIdInRange(std::size_t idOfAssignmentDefiningLowerBoundOfOpenInterval, const std::optional<std::size_t>& idOfAssignmentDefiningUpperBoundOfOpenInterval) {
    if (internalAssignmentContainer.empty() || activeAssignmentsLookupById.empty()) {
        return true;
    }

    const std::size_t                   smallestIdOfInvertibleActiveAssignments = *activeAssignmentsLookupById.rbegin();
    const std::size_t                   largestIdOfInvertibleActiveAssignments  = *activeAssignmentsLookupById.cbegin();
    std::pair<std::size_t, std::size_t> closedIntervalOfIdsOfAssignmentsToInvert;
    if (idOfAssignmentDefiningLowerBoundOfOpenInterval >= largestIdOfInvertibleActiveAssignments) {
        return true;
    }
    if (idOfAssignmentDefiningUpperBoundOfOpenInterval.has_value()) {
        if (idOfAssignmentDefiningLowerBoundOfOpenInterval > *idOfAssignmentDefiningUpperBoundOfOpenInterval) {
            return false;
        }
        if (*idOfAssignmentDefiningUpperBoundOfOpenInterval > largestIdOfInvertibleActiveAssignments + 1) {
            closedIntervalOfIdsOfAssignmentsToInvert.second = largestIdOfInvertibleActiveAssignments + 1;
        } else {
            closedIntervalOfIdsOfAssignmentsToInvert.second = !*idOfAssignmentDefiningUpperBoundOfOpenInterval ? 0 : (*idOfAssignmentDefiningUpperBoundOfOpenInterval - 1);
        }
    }
    else {
        closedIntervalOfIdsOfAssignmentsToInvert.second = largestIdOfInvertibleActiveAssignments + 1;
    }

    if (idOfAssignmentDefiningLowerBoundOfOpenInterval < smallestIdOfInvertibleActiveAssignments - 1) {
        closedIntervalOfIdsOfAssignmentsToInvert.first = smallestIdOfInvertibleActiveAssignments - 1;
    }
    else {
        closedIntervalOfIdsOfAssignmentsToInvert.first = idOfAssignmentDefiningLowerBoundOfOpenInterval + 1;
    }

    bool inversionOfAssignmentsOk = true;
    for (std::size_t assignmentId = closedIntervalOfIdsOfAssignmentsToInvert.second; assignmentId >= closedIntervalOfIdsOfAssignmentsToInvert.first && inversionOfAssignmentsOk; --assignmentId) {
        if (!activeAssignmentsLookupById.count(assignmentId)) {
            continue;
        }

        if (!replayNotActiveDataDependenciesOfAssignment(assignmentId, false)) {
            return false;
        }
        
        const std::optional<InternalAssignmentContainer::Reference>& matchingAssignmentForId = getAssignmentById(assignmentId);
        if (!matchingAssignmentForId.has_value()) {
            return false;
        }

        std::optional<AssignmentReferenceVariant> assignmentData;
        if (std::holds_alternative<std::shared_ptr<syrec::AssignStatement>>(matchingAssignmentForId.value()->data)) {
            if (const std::shared_ptr<syrec::Statement> copyOfOriginalAssignment = copyUtils::createCopyOfStmt(*std::get<std::shared_ptr<syrec::AssignStatement>>(matchingAssignmentForId->get()->data)); copyOfOriginalAssignment) {
                std::shared_ptr<syrec::AssignStatement> castedCopyOfOriginalAssignment = std::dynamic_pointer_cast<syrec::AssignStatement>(copyOfOriginalAssignment);
                invertAssignment(*castedCopyOfOriginalAssignment);
                assignmentData                                                         = castedCopyOfOriginalAssignment;
            }
        } else if (std::holds_alternative<std::shared_ptr<syrec::UnaryStatement>>(matchingAssignmentForId.value()->data)) {
            if (const std::shared_ptr<syrec::Statement> copyOfOriginalUnaryAssignment = copyUtils::createCopyOfStmt(*std::get<std::shared_ptr<syrec::UnaryStatement>>(matchingAssignmentForId->get()->data)); copyOfOriginalUnaryAssignment) {
                std::shared_ptr<syrec::UnaryStatement> castedCopyOfOriginalUnaryAssignment = std::dynamic_pointer_cast<syrec::UnaryStatement>(copyOfOriginalUnaryAssignment);
                invertAssignment(*castedCopyOfOriginalUnaryAssignment);
                assignmentData                                                             = castedCopyOfOriginalUnaryAssignment;
            }
        }

        const std::size_t                 idOfLastGeneratedAssignment                = getIdOfLastGeneratedAssignment().value_or(0);
        const std::optional<std::size_t>& idOfGeneratedInversionOfOriginalAssignment = storeActiveAssignment(*assignmentData, std::nullopt, matchingAssignmentForId->get()->dataDependencies, 0, assignmentId);
        if (!idOfGeneratedInversionOfOriginalAssignment.has_value()) {
            return false;
        }
        markAssignmentAsInactiveInLookup(assignmentId);
        matchingAssignmentForId->get()->idOfReferencedAssignment = *idOfGeneratedInversionOfOriginalAssignment;
        inversionOfAssignmentsOk = invertAssignmentsWithIdInRange(idOfLastGeneratedAssignment, *idOfGeneratedInversionOfOriginalAssignment);
    }
    return inversionOfAssignmentsOk;
}

std::optional<std::size_t> TemporaryAssignmentsContainer::findDataDependencyOfCurrentAndChildAssignmentsFittingCriteria(const std::vector<std::size_t>& assignmentsToCheck, DataDependencySortCriteria sortCriteria) const {
    OrderedAssignmentIdContainer alreadyCheckedAssignments;
    std::optional<std::size_t>   foundDataDependencyMatchingCriteria;

    for (const std::size_t assignmentId : assignmentsToCheck) {
        const std::optional<std::size_t> dataDependencyMatchingCriteriaOfAssignment = findDataDependencyOfCurrentAndChildAssignmentsFittingCriteria(assignmentId, sortCriteria, alreadyCheckedAssignments);
        if (dataDependencyMatchingCriteriaOfAssignment.has_value()) {
            if (!foundDataDependencyMatchingCriteria.has_value() || doesAssignmentIdFulfillSortCriteria(foundDataDependencyMatchingCriteria.value_or(SIZE_MAX), *dataDependencyMatchingCriteriaOfAssignment, sortCriteria)) {
                foundDataDependencyMatchingCriteria = dataDependencyMatchingCriteriaOfAssignment;
            }
        } else if (foundDataDependencyMatchingCriteria.has_value()) {
            if (doesAssignmentIdFulfillSortCriteria(*foundDataDependencyMatchingCriteria, assignmentId, sortCriteria)) {
                foundDataDependencyMatchingCriteria = assignmentId;
            }
        } else {
            foundDataDependencyMatchingCriteria = assignmentId;
        }
    }
    return foundDataDependencyMatchingCriteria;
}


TemporaryAssignmentsContainer::OrderedAssignmentIdContainer TemporaryAssignmentsContainer::determineIdsOfAssignmentFormingCompleteInheritanceChainFromEarliestToLastOverlappingAssignment(const syrec::VariableAccess& signalAccessForWhichOverlapsShouldBeFound, const parser::SymbolTable& symbolTableReference) const {
    const std::optional<std::size_t> idOfLatestOverlappingAssignmentRelativeToLastCreatedOne   = determineIdOfOverlappingAssignmentBasedOnCreationTime(signalAccessForWhichOverlapsShouldBeFound, AssignmentCreationTimeRelativeToLastCreatedOne::Latest, symbolTableReference);
    const std::optional<std::size_t> idOfEarliestOverlappingAssignmentRelativeToLastCreatedOne = determineIdOfOverlappingAssignmentBasedOnCreationTime(signalAccessForWhichOverlapsShouldBeFound, AssignmentCreationTimeRelativeToLastCreatedOne::Earliest, symbolTableReference);
    if (!idOfLatestOverlappingAssignmentRelativeToLastCreatedOne.has_value() || !idOfEarliestOverlappingAssignmentRelativeToLastCreatedOne.has_value()) {
        return {};
    }

    OrderedAssignmentIdContainer idsOfAssignmentsFormingInheritanceChainOrderFromEarliestToLatestOverlappingAssignmentRelativeToLastCreatedOne;
    const auto&                  inheritanceChainUpperBoundIdRestriction = std::next(internalAssignmentContainer.find(*idOfEarliestOverlappingAssignmentRelativeToLastCreatedOne));
    auto                         internalAssignmentContainerIterator     = internalAssignmentContainer.find(*idOfLatestOverlappingAssignmentRelativeToLastCreatedOne);
    do {
        if (internalAssignmentContainerIterator->second->assignmentType == InternalAssignmentContainer::InversionOfSubAssignment) {
            continue;
        }
        idsOfAssignmentsFormingInheritanceChainOrderFromEarliestToLatestOverlappingAssignmentRelativeToLastCreatedOne.emplace(internalAssignmentContainerIterator->first);
    } while (++internalAssignmentContainerIterator != inheritanceChainUpperBoundIdRestriction);
    return idsOfAssignmentsFormingInheritanceChainOrderFromEarliestToLatestOverlappingAssignmentRelativeToLastCreatedOne;
}

std::optional<std::size_t> TemporaryAssignmentsContainer::determineIdOfOverlappingAssignmentBasedOnCreationTime(const syrec::VariableAccess& signalAccessForWhichOverlapsShouldBeFound, AssignmentCreationTimeRelativeToLastCreatedOne relativeAssignmentCreationTime, const parser::SymbolTable& symbolTableReference) const {
    switch (relativeAssignmentCreationTime) {
        case TemporaryAssignmentsContainer::AssignmentCreationTimeRelativeToLastCreatedOne::Earliest: {
            const auto& earliestFoundOverlappingAssignment = std::find_if(
                    internalAssignmentContainer.cbegin(),
                    internalAssignmentContainer.cend(),
                    [&signalAccessForWhichOverlapsShouldBeFound, &symbolTableReference](const std::pair<std::size_t, InternalAssignmentContainer::Reference>& internalContainerEntry) {
                        const syrec::VariableAccess::ptr& assignedToSignalPartsOfLookupEntry = internalContainerEntry.second->getAssignedToSignalParts();
                        return assignedToSignalPartsOfLookupEntry && doSignalAccessesOverlap(*assignedToSignalPartsOfLookupEntry, signalAccessForWhichOverlapsShouldBeFound, symbolTableReference);
                    });
            if (earliestFoundOverlappingAssignment != internalAssignmentContainer.cend()) {
                return earliestFoundOverlappingAssignment->first;
            }
            return std::nullopt;
        }
        case TemporaryAssignmentsContainer::AssignmentCreationTimeRelativeToLastCreatedOne::Latest: {
            const auto& latestFoundOverlappingAssignment = std::find_if(
                    internalAssignmentContainer.crbegin(),
                    internalAssignmentContainer.crend(),
                    [&signalAccessForWhichOverlapsShouldBeFound, &symbolTableReference](const std::pair<std::size_t, InternalAssignmentContainer::Reference>& internalContainerEntry) {
                        const syrec::VariableAccess::ptr& assignedToSignalPartsOfLookupEntry = internalContainerEntry.second->getAssignedToSignalParts();
                        return assignedToSignalPartsOfLookupEntry && doSignalAccessesOverlap(*assignedToSignalPartsOfLookupEntry, signalAccessForWhichOverlapsShouldBeFound, symbolTableReference);
                    });
            if (latestFoundOverlappingAssignment != internalAssignmentContainer.crend()) {
                return latestFoundOverlappingAssignment->first;
            }
            return std::nullopt;
        }
        default:
            return std::nullopt;
    }
}




void TemporaryAssignmentsContainer::replayNotActiveDataDependencies(const OrderedAssignmentIdContainer& dataDependencies, std::size_t associatedWithOperationNodeId) {
    OrderedAssignmentIdContainer notActiveDataDependenciesOrderedByEarliestOccurrenceRelativeToLastActiveOneFirst;
    for (const std::size_t idOfAssignmentIdentifiedAsDataDependency: dataDependencies) {
        notActiveDataDependenciesOrderedByEarliestOccurrenceRelativeToLastActiveOneFirst.merge(getDirectAndChildDataDependenciesOfAssignment(idOfAssignmentIdentifiedAsDataDependency, DataDependencyActiveFlag::Inactive));
    }
    const std::vector<std::size_t> idsOfToBeReplayedAssignmentsOrderedByLatestOccurrenceRelativeToLastActiveOneFirst = std::vector(notActiveDataDependenciesOrderedByEarliestOccurrenceRelativeToLastActiveOneFirst.rend(), notActiveDataDependenciesOrderedByEarliestOccurrenceRelativeToLastActiveOneFirst.rbegin());
    // If an assignment is active, its child elements should also be still be active
    for (const std::size_t idOfToBeReplayedAssignment: idsOfToBeReplayedAssignmentsOrderedByLatestOccurrenceRelativeToLastActiveOneFirst) {
        if (const std::optional<InternalAssignmentContainer::Reference> toBeReplayedAssignment = getAssignmentById(idOfToBeReplayedAssignment); toBeReplayedAssignment.has_value()) {
            const std::optional<std::size_t>                            idOfLatestInversionOfToBeReplayedAssignment = getIdOfLastInversionOfAssignment(toBeReplayedAssignment->get()->idOfReferencedAssignment);
            const std::optional<InternalAssignmentContainer::Reference> latestInversionOfToBeReplayedAssignment     = idOfLatestInversionOfToBeReplayedAssignment.has_value() ? getAssignmentById(*idOfLatestInversionOfToBeReplayedAssignment) : std::nullopt;
            if (latestInversionOfToBeReplayedAssignment.has_value()) {
                const AssignmentReferenceVariant   toBeReplayedAssignmentData               = toBeReplayedAssignment->get()->data;
                const OrderedAssignmentIdContainer dataDependenciesOfToBeReplayedAssignment = toBeReplayedAssignment->get()->dataDependencies;
                if (const std::optional<std::size_t> idOfReplayedAssignment = storeActiveAssignment(toBeReplayedAssignmentData, toBeReplayedAssignment->get()->idOfAssignmentFromWhichAssignedToSignalPartsWhereInherited, dataDependenciesOfToBeReplayedAssignment, associatedWithOperationNodeId, std::nullopt); idOfReplayedAssignment.has_value()) {
                    latestInversionOfToBeReplayedAssignment->get()->idOfReferencedAssignment = *idOfReplayedAssignment;
                    latestInversionOfToBeReplayedAssignment->get()->isStillActive           = true;
                }
            }
        }
    }
}

void TemporaryAssignmentsContainer::invertAssignmentAndDataDependencies(std::size_t assignmentId) {
    const OrderedAssignmentIdContainer idsOfToBeInvertedAssignmentsOrderedByEarliestOccurrenceRelativeToLastActiveOneFirst = getDirectAndChildDataDependenciesOfAssignment(assignmentId, DataDependencyActiveFlag::Active);
    for (const std::size_t idOfToBeInvertedAssignment: idsOfToBeInvertedAssignmentsOrderedByEarliestOccurrenceRelativeToLastActiveOneFirst) {
        invertAssignmentAndStoreAndMarkOriginalAsInactive(idOfToBeInvertedAssignment);
    }
    invertAssignmentAndStoreAndMarkOriginalAsInactive(assignmentId);
}

void TemporaryAssignmentsContainer::invertAssignmentsButLeaveDataDependenciesUnchanged(const OrderedAssignmentIdContainer& orderedIdsOfAssignmentToInvert) {
    for (const std::size_t assignmentId : orderedIdsOfAssignmentToInvert) {
        invertAssignmentAndStoreAndMarkOriginalAsInactive(assignmentId);
    }
}

TemporaryAssignmentsContainer::OrderedAssignmentIdContainer TemporaryAssignmentsContainer::getIdsOfAssignmentsDefiningInheritanceChainToCurrentOne(std::size_t idOfLastAssignmentInheritingAssignedToSignalParts) const {
    OrderedAssignmentIdContainer orderedInheritanceChainStartingFromMostRecentOne;

    std::size_t currentAssignmentToCheck = idOfLastAssignmentInheritingAssignedToSignalParts;
    while (currentAssignmentToCheck) {
        const std::optional<InternalAssignmentContainer::ReadOnlyReference> matchingActiveAssignmentForId = getMatchingActiveSubassignmentById(currentAssignmentToCheck);
        orderedInheritanceChainStartingFromMostRecentOne.emplace(idOfLastAssignmentInheritingAssignedToSignalParts);
        if (matchingActiveAssignmentForId.has_value()) {
            currentAssignmentToCheck = matchingActiveAssignmentForId->get()->idOfAssignmentFromWhichAssignedToSignalPartsWhereInherited.value_or(0);   
        } else {
            currentAssignmentToCheck = 0;
        }
    }
    return orderedInheritanceChainStartingFromMostRecentOne;
}


bool TemporaryAssignmentsContainer::storeInitializationForReplacementOfLeafNode(const syrec::AssignStatement::ptr& assignmentDefiningSubstitution, const std::optional<AssignmentReferenceVariant>& optionalRequiredResetOfSubstitutionLhsOperand) {
    const std::shared_ptr<syrec::AssignStatement> castedAssignmentDefiningSubstitution = std::dynamic_pointer_cast<syrec::AssignStatement>(assignmentDefiningSubstitution);
    if (!castedAssignmentDefiningSubstitution || (optionalRequiredResetOfSubstitutionLhsOperand.has_value() && !castedAssignmentDefiningSubstitution)) {
        return false;
    }

    if (optionalRequiredResetOfSubstitutionLhsOperand.has_value()) {
        initializationAssignmentsForGeneratedSubstitutions.emplace_back(*optionalRequiredResetOfSubstitutionLhsOperand);
    }
    generatedAssignmentsDefiningPermanentSubstitutionsOfOperandsOfOperationNode.emplace_back(castedAssignmentDefiningSubstitution);
    return true;
}

bool TemporaryAssignmentsContainer::storeReplacementAsActiveAssignment(std::size_t associatedOperationNodeId, const syrec::AssignStatement::ptr& assignmentDefiningSubstitution, const OrderedAssignmentIdContainer& dataDependenciesOfSubstitutionRhs, const std::optional<AssignmentReferenceVariant>& optionalRequiredPriorResetOfSignalUsedOnSubstitutionLhs) {
    const std::shared_ptr<syrec::AssignStatement> castedAssignmentDefiningSubstitution = std::dynamic_pointer_cast<syrec::AssignStatement>(assignmentDefiningSubstitution);
    if (!castedAssignmentDefiningSubstitution || !storeActiveAssignment(castedAssignmentDefiningSubstitution, std::nullopt, dataDependenciesOfSubstitutionRhs, associatedOperationNodeId, std::nullopt).has_value()) {
        return false;
    }
    
    if (optionalRequiredPriorResetOfSignalUsedOnSubstitutionLhs.has_value()) {
        initializationAssignmentsForGeneratedSubstitutions.emplace_back(*optionalRequiredPriorResetOfSignalUsedOnSubstitutionLhs);
    }
    return true;
}

void TemporaryAssignmentsContainer::markCutoffForInvertibleAssignments() {
    cutOffIndicesForInvertibleAssignments.emplace_back(internalAssignmentContainer.size());
}

/*
 * END OF REWORK
 */
void TemporaryAssignmentsContainer::invertAllActiveAssignmentsExcludingFirstActiveOne() {
    if (activeAssignmentsLookupById.empty() || activeAssignmentsLookupById.size() == 1) {
        return;
    }

    const OrderedAssignmentIdContainer assignmentsToInvert(std::next(activeAssignmentsLookupById.begin()), activeAssignmentsLookupById.end());
    for (const auto& activeAssignmentId: assignmentsToInvert) {
        const std::optional<InternalAssignmentContainer::Reference> matchingAssignmentForId = getAssignmentById(activeAssignmentId);
        invertAssignmentAndDataDependencies(activeAssignmentId);
    }
}

void TemporaryAssignmentsContainer::popLastCutoffForInvertibleAssignments() {
    if (!cutOffIndicesForInvertibleAssignments.empty()) {
        cutOffIndicesForInvertibleAssignments.pop_back();
    }
}

void TemporaryAssignmentsContainer::rollbackLastXAssignments(std::size_t numberOfAssignmentsToRemove) {
    const std::size_t actualNumberOfAssignmentsRemoved = std::min(numberOfAssignmentsToRemove, internalAssignmentContainer.size());
    if (!actualNumberOfAssignmentsRemoved) {
        return;
    }
    
    std::size_t numRemovedAssignments   = 0;
    for (auto generatedAssignmentLookupEntry = internalAssignmentContainer.begin(); generatedAssignmentLookupEntry != internalAssignmentContainer.end() && numRemovedAssignments < actualNumberOfAssignmentsRemoved;) {
        if (const std::optional<InternalAssignmentContainer::Reference> dataOfInvertedAssignment = getAssignmentById(generatedAssignmentLookupEntry->second->idOfReferencedAssignment); dataOfInvertedAssignment.has_value()) {
            dataOfInvertedAssignment->get()->idOfReferencedAssignment = 0;
            dataOfInvertedAssignment->get()->isStillActive           = true;
            if (generatedAssignmentLookupEntry->second->assignmentType == InternalAssignmentContainer::AssignmentType::SubAssignmentGeneration) {
                activeAssignmentsLookupById.emplace(dataOfInvertedAssignment->get()->id);   
            }
        }
        if (generatedAssignmentLookupEntry->second->isStillActive) {
            markAssignmentAsInactiveInLookup(generatedAssignmentLookupEntry->first);
        }
        generatedAssignmentLookupEntry = internalAssignmentContainer.erase(generatedAssignmentLookupEntry);
        numRemovedAssignments++;
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
    rollbackLastXAssignments(internalAssignmentContainer.size() - indexOfFirstRemovedAssignment);
}

void TemporaryAssignmentsContainer::resetInternals() {
    activeAssignmentsLookupById.clear();
    internalAssignmentContainer.clear();
    cutOffIndicesForInvertibleAssignments.clear();
    initializationAssignmentsForGeneratedSubstitutions.clear();
    generatedAssignmentsDefiningPermanentSubstitutionsOfOperandsOfOperationNode.clear();
}

//// TODO: What should happen if we cannot fit all assignments into the container (i.e. a number larger than UINT_MAX)
std::vector<TemporaryAssignmentsContainer::AssignmentReferenceVariant> TemporaryAssignmentsContainer::getAssignments() const {
    if (generatedAssignmentsDefiningPermanentSubstitutionsOfOperandsOfOperationNode.empty()) {
        std::vector<AssignmentReferenceVariant> resultContainer;
        resultContainer.reserve(internalAssignmentContainer.size());

        for (const auto& generatedAssignmentIterator: internalAssignmentContainer) {
            resultContainer.emplace(resultContainer.begin(),generatedAssignmentIterator.second->data);
        }
        return resultContainer;
    }
    
    std::vector<AssignmentReferenceVariant> combinationOfGeneratedAssignmentsAndSubstitutionsOfOperationNodeOperandData;
    combinationOfGeneratedAssignmentsAndSubstitutionsOfOperationNodeOperandData.reserve(internalAssignmentContainer.size() + (generatedAssignmentsDefiningPermanentSubstitutionsOfOperandsOfOperationNode.size() * 2));

    combinationOfGeneratedAssignmentsAndSubstitutionsOfOperationNodeOperandData.insert(combinationOfGeneratedAssignmentsAndSubstitutionsOfOperationNodeOperandData.end(), generatedAssignmentsDefiningPermanentSubstitutionsOfOperandsOfOperationNode.cbegin(), generatedAssignmentsDefiningPermanentSubstitutionsOfOperandsOfOperationNode.cend());
    auto insertPositionOfGeneratedAssignments = std::next(combinationOfGeneratedAssignmentsAndSubstitutionsOfOperationNodeOperandData.begin(), generatedAssignmentsDefiningPermanentSubstitutionsOfOperandsOfOperationNode.size());
    for (const auto& generatedAssignmentIterator: internalAssignmentContainer) {
        combinationOfGeneratedAssignmentsAndSubstitutionsOfOperationNodeOperandData.emplace(insertPositionOfGeneratedAssignments++, generatedAssignmentIterator.second->data);
    }
    /*
     * To allow a reuse of the generated replacements, we need to reset our generated assignments defining the substitution of an operand of an operation node. Since the assignments defining the substitutions all use the XOR assignment operation,
     * the inversion of said statements is achieved by simply re-adding them to our container.
     */
    combinationOfGeneratedAssignmentsAndSubstitutionsOfOperationNodeOperandData.insert(combinationOfGeneratedAssignmentsAndSubstitutionsOfOperationNodeOperandData.end(), generatedAssignmentsDefiningPermanentSubstitutionsOfOperandsOfOperationNode.crbegin(), generatedAssignmentsDefiningPermanentSubstitutionsOfOperandsOfOperationNode.crend());
    return combinationOfGeneratedAssignmentsAndSubstitutionsOfOperationNodeOperandData;
}

std::optional<std::vector<TemporaryAssignmentsContainer::OwningAssignmentReferenceVariant>> TemporaryAssignmentsContainer::getAssignmentsDefiningValueResetsOfGeneratedSubstitutions() const {
    return createOwningCopiesOfAssignments(initializationAssignmentsForGeneratedSubstitutions);
}

std::optional<std::vector<TemporaryAssignmentsContainer::OwningAssignmentReferenceVariant>> TemporaryAssignmentsContainer::getInvertedAssignmentsUndoingValueResetsOfGeneratedSubstitutions() const {
    const std::optional<unsigned int> mappedToInversionOperationForBinaryAssignments = syrec_operation::tryMapAssignmentOperationEnumToFlag(syrec_operation::operation::XorAssign);
    const std::optional<unsigned int> mappedToInversionOperationForUnaryAssignments   = syrec_operation::tryMapUnaryAssignmentOperationEnumToFlag(syrec_operation::operation::IncrementAssign);
    if (initializationAssignmentsForGeneratedSubstitutions.empty() || !mappedToInversionOperationForBinaryAssignments.has_value() || !mappedToInversionOperationForUnaryAssignments.has_value()) {
        // Since the empty brace initialization is also applicable to the std::optional type, we need to use the construct below to initialize a non-empty std::optional of an empty vector
        return std::optional<std::vector<OwningAssignmentReferenceVariant>>(std::in_place);
    }

    std::vector<OwningAssignmentReferenceVariant> containerForOwningCopiesOfAssignments;
    containerForOwningCopiesOfAssignments.reserve(initializationAssignmentsForGeneratedSubstitutions.size());
    if (std::optional<std::vector<OwningAssignmentReferenceVariant>> owningCopiesOfInitializationAssignments = getAssignmentsDefiningValueResetsOfGeneratedSubstitutions(); owningCopiesOfInitializationAssignments.has_value()) {
        containerForOwningCopiesOfAssignments.insert(containerForOwningCopiesOfAssignments.end(), std::make_move_iterator(owningCopiesOfInitializationAssignments->begin()), std::make_move_iterator(owningCopiesOfInitializationAssignments->end()));
        for (auto&& initializationStatement: containerForOwningCopiesOfAssignments) {
            OwningAssignmentReferenceVariant temporaryOwnershipOfInitializationStatement = std::move(initializationStatement);
            
            /*
             * All assignments created in this function restore the initial value of the replacements generated from existing signals thus we can use the XOR assignment operation for this step
             * since the current value of the replacement signal is zero (and under our assumption that all signal values are non-negative integers) this XOR assignment operation is equal to the ADD assignment operation.
             */
            if (std::holds_alternative<std::unique_ptr<syrec::AssignStatement>>(temporaryOwnershipOfInitializationStatement)) {
                std::unique_ptr<syrec::AssignStatement> temporaryOwnerOfBinaryAssignment = std::get<std::unique_ptr<syrec::AssignStatement>>(std::move(temporaryOwnershipOfInitializationStatement));
                temporaryOwnerOfBinaryAssignment->op                                     = *mappedToInversionOperationForBinaryAssignments;
                initializationStatement                                                  = std::move(temporaryOwnerOfBinaryAssignment);
            } else if (std::holds_alternative<std::unique_ptr<syrec::UnaryStatement>>(temporaryOwnershipOfInitializationStatement)) {
                std::unique_ptr<syrec::UnaryStatement> temporaryOwnerOfUnaryAssignment = std::get<std::unique_ptr<syrec::UnaryStatement>>(std::move(temporaryOwnershipOfInitializationStatement));
                temporaryOwnerOfUnaryAssignment->op                                    = *mappedToInversionOperationForUnaryAssignments;
                initializationStatement                                                = std::move(temporaryOwnerOfUnaryAssignment);
            }
        }
    } else {
        // Since the empty brace initialization is also applicable to the std::optional type, we need to use the construct below to initialize a non-empty std::optional of an empty vector
        return std::optional<std::vector<OwningAssignmentReferenceVariant>>(std::in_place);
    }
    return containerForOwningCopiesOfAssignments;
}

bool TemporaryAssignmentsContainer::existsOverlappingAssignmentFor(const syrec::VariableAccess& assignedToSignal, const parser::SymbolTable& symbolTable) const {
    if (activeAssignmentsLookupById.empty()) {
        return false;
    }

    for (const std::size_t activeAssignmentId : activeAssignmentsLookupById) {
        if (const std::optional<InternalAssignmentContainer::ReadOnlyReference> referencedAssignment = getMatchingActiveSubassignmentById(activeAssignmentId); referencedAssignment.has_value()) {
            const syrec::VariableAccess::ptr assignedToSignalPartsOfActiveAssignment = referencedAssignment->get()->getAssignedToSignalParts();
            if (assignedToSignalPartsOfActiveAssignment->var->name == assignedToSignal.var->name) {
                return doSignalAccessesOverlap(assignedToSignal, *assignedToSignalPartsOfActiveAssignment, symbolTable);
            }
        }
    }
    return false;
}

std::size_t TemporaryAssignmentsContainer::getNumberOfAssignments() const {
    return internalAssignmentContainer.size();
}

//// START OF NON-PUBLIC FUNCTIONS
///

bool TemporaryAssignmentsContainer::replayNotActiveDataDependenciesOfAssignment(std::size_t assignmentId, bool replayAssignmentItself) {
    const OrderedAssignmentIdContainer dataDependenciesOfAssignment = getDataDependenciesOfAssignment(assignmentId);
    for (const std::size_t dataDependency : dataDependenciesOfAssignment) {
        if (const std::optional<bool> isDataDependencyActive = isAssignmentActive(dataDependency); isDataDependencyActive.has_value()) {
            if (*isDataDependencyActive) {
                continue;
            }
            if (!replayNotActiveDataDependenciesOfAssignment(dataDependency, true)) {
                return false;
            }
        } else {
            return false;
        }
    }

    if (!replayAssignmentItself) {
        return true;
    }

    const std::optional<InternalAssignmentContainer::Reference>& matchingAssignmentForId = getAssignmentById(assignmentId);
    if (!matchingAssignmentForId.has_value()) {
        return false;
    }

    std::optional<AssignmentReferenceVariant> assignmentData;
    if (std::holds_alternative<std::shared_ptr<syrec::AssignStatement>>(matchingAssignmentForId.value()->data)) {
        if (const std::shared_ptr<syrec::Statement> copyOfOriginalAssignment = copyUtils::createCopyOfStmt(*std::get<std::shared_ptr<syrec::AssignStatement>>(matchingAssignmentForId->get()->data)); copyOfOriginalAssignment) {
            std::shared_ptr<syrec::AssignStatement> castedCopyOfOriginalAssignment = std::dynamic_pointer_cast<syrec::AssignStatement>(copyOfOriginalAssignment);
            assignmentData                                                         = castedCopyOfOriginalAssignment;
        }
    } else if (std::holds_alternative<std::shared_ptr<syrec::UnaryStatement>>(matchingAssignmentForId.value()->data)) {
        if (const std::shared_ptr<syrec::Statement> copyOfOriginalUnaryAssignment = copyUtils::createCopyOfStmt(*std::get<std::shared_ptr<syrec::UnaryStatement>>(matchingAssignmentForId->get()->data)); copyOfOriginalUnaryAssignment) {
            std::shared_ptr<syrec::UnaryStatement> castedCopyOfOriginalUnaryAssignment = std::dynamic_pointer_cast<syrec::UnaryStatement>(copyOfOriginalUnaryAssignment);
            assignmentData                                                             = castedCopyOfOriginalUnaryAssignment;
        }
    }
    const std::optional<std::size_t>& idOfGeneratedInversionOfOriginalAssignment = storeActiveAssignment(*assignmentData, std::nullopt, dataDependenciesOfAssignment, 0, std::nullopt);
    /*
     *  const std::optional<std::size_t>                            idOfLatestInversionOfToBeReplayedAssignment = getIdOfLastInversionOfAssignment(toBeReplayedAssignment->get()->idOfReferencedAssignment);
            const std::optional<InternalAssignmentContainer::Reference> latestInversionOfToBeReplayedAssignment     = idOfLatestInversionOfToBeReplayedAssignment.has_value() ? getAssignmentById(*idOfLatestInversionOfToBeReplayedAssignment) : std::nullopt;
            if (latestInversionOfToBeReplayedAssignment.has_value()) {
            
     */
    return idOfGeneratedInversionOfOriginalAssignment.has_value();
}

std::optional<bool> TemporaryAssignmentsContainer::isAssignmentActive(std::size_t assignmentId) const {
    const std::optional<InternalAssignmentContainer::Reference>& matchingAssignmentForId = getAssignmentById(assignmentId);
    return matchingAssignmentForId.has_value() && matchingAssignmentForId->get()->isStillActive;
}

std::optional<std::size_t> TemporaryAssignmentsContainer::findDataDependencyOfCurrentAndChildAssignmentsFittingCriteria(std::size_t assignmentId, DataDependencySortCriteria sortCriteria, OrderedAssignmentIdContainer& alreadyCheckedAssignmentIds) const {
    if (alreadyCheckedAssignmentIds.count(assignmentId)) {
        return std::nullopt;
    }

    std::optional<std::size_t> foundDataDependencyMatchingCriteria;
    for (const std::size_t dataDependency: getDataDependenciesOfAssignment(assignmentId)) {
        if (alreadyCheckedAssignmentIds.count(dataDependency)) {
            continue;
        }

        const std::optional<std::size_t> dataDependencyMatchingCriteriaOfAssignment = findDataDependencyOfCurrentAndChildAssignmentsFittingCriteria(dataDependency, sortCriteria, alreadyCheckedAssignmentIds);
        if (dataDependencyMatchingCriteriaOfAssignment.has_value()) {
            if (!foundDataDependencyMatchingCriteria.has_value() || doesAssignmentIdFulfillSortCriteria(foundDataDependencyMatchingCriteria.value_or(SIZE_MAX), *dataDependencyMatchingCriteriaOfAssignment, sortCriteria)) {
                foundDataDependencyMatchingCriteria = dataDependencyMatchingCriteriaOfAssignment;
            }
        } else if (foundDataDependencyMatchingCriteria.has_value()) {
            if (doesAssignmentIdFulfillSortCriteria(*foundDataDependencyMatchingCriteria, assignmentId, sortCriteria)) {
                foundDataDependencyMatchingCriteria = assignmentId;
            }
        } else {
            foundDataDependencyMatchingCriteria = assignmentId;
        }
        alreadyCheckedAssignmentIds.emplace(dataDependency);
    }
    return foundDataDependencyMatchingCriteria;
}

TemporaryAssignmentsContainer::OrderedAssignmentIdContainer TemporaryAssignmentsContainer::getDirectAndChildDataDependenciesOfAssignment(std::size_t assignmentId, DataDependencyActiveFlag isActiveFilterOnlyReturningMatchingElements) const {
    OrderedAssignmentIdContainer activeAssignmentContainer;

    if (const std::optional<InternalAssignmentContainer::Reference>& referencedAssignment = getAssignmentById(assignmentId); referencedAssignment.has_value() && isActiveFilterOnlyReturningMatchingElements == referencedAssignment->get()->isStillActive) {
        std::unordered_set<std::size_t> alreadyProcessedAssignmentLookup;
        for (const std::size_t dataDependencyOfReferencedAssignment: referencedAssignment->get()->dataDependencies) {
            const OrderedAssignmentIdContainer childDependenciesOfDataDependency = getDirectAndChildDataDependenciesOfAssignment(dataDependencyOfReferencedAssignment, isActiveFilterOnlyReturningMatchingElements, alreadyProcessedAssignmentLookup);
            activeAssignmentContainer.merge(getDirectAndChildDataDependenciesOfAssignment(dataDependencyOfReferencedAssignment, isActiveFilterOnlyReturningMatchingElements, alreadyProcessedAssignmentLookup));
        }
    }
    return activeAssignmentContainer;
}

TemporaryAssignmentsContainer::OrderedAssignmentIdContainer TemporaryAssignmentsContainer::getDirectAndChildDataDependenciesOfAssignment(std::size_t assignmentId, DataDependencyActiveFlag isActiveFilterOnlyReturningMatchingElements, std::unordered_set<std::size_t>& idsOfAlreadyProcessedAssignments) const {
    if (idsOfAlreadyProcessedAssignments.count(assignmentId)) {
        return {};
    }

    if (const std::optional<InternalAssignmentContainer::Reference>& referencedAssignment = getAssignmentById(assignmentId); referencedAssignment.has_value() && referencedAssignment->get()->isStillActive == isActiveFilterOnlyReturningMatchingElements) {
        idsOfAlreadyProcessedAssignments.emplace(assignmentId);
        OrderedAssignmentIdContainer resultContainer;
        resultContainer.emplace(assignmentId);

        for (const std::size_t dataDependencyOfReferencedAssignment: referencedAssignment->get()->dataDependencies) {
            resultContainer.merge(getDirectAndChildDataDependenciesOfAssignment(dataDependencyOfReferencedAssignment, isActiveFilterOnlyReturningMatchingElements, idsOfAlreadyProcessedAssignments));
        }
        return resultContainer;
    }
    return {};
}

std::optional<std::size_t> TemporaryAssignmentsContainer::getIdOfLastInversionOfAssignment(std::size_t assignmentId) const {
    if (const std::optional<InternalAssignmentContainer::Reference> assignmentData = getAssignmentById(assignmentId); assignmentData.has_value() && !assignmentData->get()->isStillActive) {
        return getIdOfLastInversionOfAssignment(assignmentData->get()->idOfReferencedAssignment);
    }
    return std::nullopt;
}

std::optional<std::size_t> TemporaryAssignmentsContainer::storeActiveAssignment(const AssignmentReferenceVariant& assignmentData, const std::optional<std::size_t>& idOfAssignmentFromWhichAssignedToSignalPartsWhereInherited, const OrderedAssignmentIdContainer& dataDependencies, std::size_t associatedWithOperationNodeId, const std::optional<std::size_t>& optionalIdOfInvertedAssignment) {
    std::size_t chosenAssignmentId = 1;
    if (!internalAssignmentContainer.empty()) {
        if (internalAssignmentContainer.begin()->first == SIZE_MAX) {
            return std::nullopt;
        }
        chosenAssignmentId = internalAssignmentContainer.begin()->first + 1;
    }

    if (const InternalAssignmentContainer::Reference createdAssignment = std::make_shared<InternalAssignmentContainer>(optionalIdOfInvertedAssignment.has_value() 
        ? InternalAssignmentContainer::asInversionOfSubAssignment(chosenAssignmentId, *optionalIdOfInvertedAssignment, assignmentData, dataDependencies)
        : InternalAssignmentContainer::asSubAssignment(chosenAssignmentId, associatedWithOperationNodeId, idOfAssignmentFromWhichAssignedToSignalPartsWhereInherited, assignmentData, dataDependencies)); createdAssignment) {
        internalAssignmentContainer.emplace(chosenAssignmentId, createdAssignment);
        if (createdAssignment->assignmentType == InternalAssignmentContainer::AssignmentType::SubAssignmentGeneration) {
            activeAssignmentsLookupById.emplace(chosenAssignmentId);       
        }
        return chosenAssignmentId;
    }
    return std::nullopt;
}


void TemporaryAssignmentsContainer::invertAssignmentAndStoreAndMarkOriginalAsInactive(std::size_t idOfToBeInvertedAssignment) {
    const std::optional<InternalAssignmentContainer::Reference>& matchingAssignmentForId = getAssignmentById(idOfToBeInvertedAssignment);
    if (!matchingAssignmentForId.has_value()) {
        return;
    }

    std::optional<AssignmentReferenceVariant> invertedAssignmentData;
    if (std::holds_alternative<std::shared_ptr<syrec::AssignStatement>>(matchingAssignmentForId.value()->data)) {
        if (const std::shared_ptr<syrec::Statement> copyOfOriginalAssignment = copyUtils::createCopyOfStmt(*std::get<std::shared_ptr<syrec::AssignStatement>>(matchingAssignmentForId->get()->data)); copyOfOriginalAssignment) {
            std::shared_ptr<syrec::AssignStatement> castedCopyOfOriginalAssignment = std::dynamic_pointer_cast<syrec::AssignStatement>(copyOfOriginalAssignment);
            invertAssignment(*castedCopyOfOriginalAssignment);
            invertedAssignmentData = castedCopyOfOriginalAssignment;
        } 
    } else if (std::holds_alternative<std::shared_ptr<syrec::UnaryStatement>>(matchingAssignmentForId.value()->data)) {
        if (const std::shared_ptr<syrec::Statement> copyOfOriginalUnaryAssignment = copyUtils::createCopyOfStmt(*std::get<std::shared_ptr<syrec::UnaryStatement>>(matchingAssignmentForId->get()->data)); copyOfOriginalUnaryAssignment) {
            std::shared_ptr<syrec::UnaryStatement> castedCopyOfOriginalUnaryAssignment = std::dynamic_pointer_cast<syrec::UnaryStatement>(copyOfOriginalUnaryAssignment);
            invertAssignment(*castedCopyOfOriginalUnaryAssignment);
            invertedAssignmentData = castedCopyOfOriginalUnaryAssignment;
        }
    }
    
    if (const std::optional<std::size_t>& idOfGeneratedInversionOfOriginalAssignment = storeActiveAssignment(*invertedAssignmentData, std::nullopt, matchingAssignmentForId->get()->dataDependencies, 0, idOfToBeInvertedAssignment); idOfGeneratedInversionOfOriginalAssignment.has_value()) {
        markAssignmentAsInactiveInLookup(idOfToBeInvertedAssignment);
        matchingAssignmentForId->get()->isStillActive           = false;
        matchingAssignmentForId->get()->idOfReferencedAssignment = *idOfGeneratedInversionOfOriginalAssignment;
    }
}

void TemporaryAssignmentsContainer::markAssignmentAsInactiveInLookup(std::size_t assignmentId) {
    activeAssignmentsLookupById.erase(assignmentId);
}

void TemporaryAssignmentsContainer::removeActiveAssignmentFromLookup(std::size_t assignmentId) {
    markAssignmentAsInactiveInLookup(assignmentId);
    internalAssignmentContainer.erase(assignmentId);
}

std::optional<TemporaryAssignmentsContainer::InternalAssignmentContainer::Reference> TemporaryAssignmentsContainer::getAssignmentById(std::size_t assignmentId) const {
    if (internalAssignmentContainer.count(assignmentId)) {
        return internalAssignmentContainer.at(assignmentId);
    }
    return std::nullopt;
}

std::optional<TemporaryAssignmentsContainer::InternalAssignmentContainer::ReadOnlyReference> TemporaryAssignmentsContainer::getMatchingActiveSubassignmentById(std::size_t assignmentId) const {
    if (!activeAssignmentsLookupById.count(assignmentId)) {
        return std::nullopt;
    }

    if (const std::optional<TemporaryAssignmentsContainer::InternalAssignmentContainer::Reference> matchingAssignmentForId = getAssignmentById(assignmentId); matchingAssignmentForId.has_value() && matchingAssignmentForId->get()->isStillActive
        && matchingAssignmentForId->get()->assignmentType == InternalAssignmentContainer::SubAssignmentGeneration) {
        return *matchingAssignmentForId;
    }
    return std::nullopt;
}

void TemporaryAssignmentsContainer::invertAssignment(syrec::AssignStatement& assignment) {
    const std::optional<syrec_operation::operation> matchingAssignmentOperationEnumForFlag              = syrec_operation::tryMapAssignmentOperationFlagToEnum(assignment.op);
    const std::optional<syrec_operation::operation> invertedAssignmentOperation                         = matchingAssignmentOperationEnumForFlag.has_value() ? syrec_operation::invert(*matchingAssignmentOperationEnumForFlag) : std::nullopt;
    if (const std::optional<unsigned int> mappedToAssignmentOperationFlagForInvertedOperation = invertedAssignmentOperation.has_value() ? syrec_operation::tryMapAssignmentOperationEnumToFlag(*invertedAssignmentOperation) : std::nullopt; mappedToAssignmentOperationFlagForInvertedOperation.has_value()) {
        assignment.op = *mappedToAssignmentOperationFlagForInvertedOperation;   
    }
}

void TemporaryAssignmentsContainer::invertAssignment(syrec::UnaryStatement& assignment) {
    const std::optional<syrec_operation::operation> matchingUnaryAssignmentOperationEnumForFlag     = syrec_operation::tryMapUnaryAssignmentOperationFlagToEnum(assignment.op);
    const std::optional<syrec_operation::operation> invertedUnaryAssignmentOperation                = matchingUnaryAssignmentOperationEnumForFlag.has_value() ? syrec_operation::invert(*matchingUnaryAssignmentOperationEnumForFlag) : std::nullopt;
    const std::optional<unsigned int>               mappedToFlagForInvertedUnaryAssignmentOperation = invertedUnaryAssignmentOperation.has_value() ? syrec_operation::tryMapUnaryAssignmentOperationEnumToFlag(*invertedUnaryAssignmentOperation) : std::nullopt;
    if (mappedToFlagForInvertedUnaryAssignmentOperation.has_value()) {
        assignment.op = *mappedToFlagForInvertedUnaryAssignmentOperation;
    }
}

std::optional<TemporaryAssignmentsContainer::OwningAssignmentReferenceVariant> TemporaryAssignmentsContainer::createOwningCopyOfAssignment(const syrec::AssignStatement& assignment) {
    if (std::unique_ptr<syrec::AssignStatement> owningCopyOfAssignment = std::make_unique<syrec::AssignStatement>(assignment); owningCopyOfAssignment) {
        return owningCopyOfAssignment;
    }
    return std::nullopt;
}

std::optional<TemporaryAssignmentsContainer::OwningAssignmentReferenceVariant> TemporaryAssignmentsContainer::createOwningCopyOfAssignment(const syrec::UnaryStatement& assignment) {
    if (std::unique_ptr<syrec::UnaryStatement> owningCopyOfAssignment = std::make_unique<syrec::UnaryStatement>(assignment); owningCopyOfAssignment) {
        return owningCopyOfAssignment;
    }
    return std::nullopt;
}

std::optional<std::vector<TemporaryAssignmentsContainer::OwningAssignmentReferenceVariant>> TemporaryAssignmentsContainer::createOwningCopiesOfAssignments(const std::vector<AssignmentReferenceVariant>& assignments) {
    if (assignments.empty()) {
        // Since the empty brace initialization is also applicable to the std::optional type, we need to use the construct below to initialize a non-empty std::optional of an empty vector
        return std::optional<std::vector<OwningAssignmentReferenceVariant>>(std::in_place);
    }

    std::vector<OwningAssignmentReferenceVariant> containerForCopies;
    containerForCopies.reserve(assignments.size());

    for (const auto& statementToTreatAsAssignment: assignments) {
        std::optional<OwningAssignmentReferenceVariant> copyOfAssignment;
        if (std::holds_alternative<std::shared_ptr<syrec::AssignStatement>>(statementToTreatAsAssignment)) {
            if (const std::shared_ptr<syrec::AssignStatement> statementCastedAsBinaryAssignment = std::get<std::shared_ptr<syrec::AssignStatement>>(statementToTreatAsAssignment); statementCastedAsBinaryAssignment) {
                copyOfAssignment = createOwningCopyOfAssignment(*statementCastedAsBinaryAssignment);
            }
        } else if (std::holds_alternative<std::shared_ptr<syrec::UnaryStatement>>(statementToTreatAsAssignment)) {
            if (const std::shared_ptr<syrec::UnaryStatement> statementCastedAsUnaryAssignment = std::get<std::shared_ptr<syrec::UnaryStatement>>(statementToTreatAsAssignment); statementCastedAsUnaryAssignment) {
                copyOfAssignment = createOwningCopyOfAssignment(*statementCastedAsUnaryAssignment);
            }
        }
        if (!copyOfAssignment.has_value()) {
            return std::nullopt;
        }
        containerForCopies.push_back(std::move(*copyOfAssignment));
    }
    return containerForCopies;
}

std::vector<std::reference_wrapper<syrec::VariableAccess>> TemporaryAssignmentsContainer::getSignalAccessesOfExpression(const syrec::Expression& expression) {
    std::vector<std::reference_wrapper<syrec::VariableAccess>> foundSignalAccessContainer;

    if (const auto& expressionAsVariableExpr = dynamic_cast<const syrec::VariableExpression*>(&expression); expressionAsVariableExpr) {
        return {*expressionAsVariableExpr->var};
    }
    if (const auto& exprAsBinaryExpr = dynamic_cast<const syrec::BinaryExpression*>(&expression); exprAsBinaryExpr) {
        std::vector<std::reference_wrapper<syrec::VariableAccess>>       foundSignalAccessOfLhsExpr = getSignalAccessesOfExpression(*exprAsBinaryExpr->lhs);
        const std::vector<std::reference_wrapper<syrec::VariableAccess>> foundSignalAccessOfRhsExpr = getSignalAccessesOfExpression(*exprAsBinaryExpr->rhs);
        foundSignalAccessOfLhsExpr.insert(foundSignalAccessOfLhsExpr.end(), foundSignalAccessOfRhsExpr.begin(), foundSignalAccessOfRhsExpr.end());
        return foundSignalAccessOfLhsExpr;
    }
    if (const auto& exprAsShiftExpr = dynamic_cast<const syrec::ShiftExpression*>(&expression); exprAsShiftExpr) {
        return getSignalAccessesOfExpression(*exprAsShiftExpr->lhs);
    }
    return {};
}

std::optional<std::shared_ptr<syrec::AssignStatement>> TemporaryAssignmentsContainer::tryGetActiveAssignmentAsBinaryAssignment(const AssignmentReferenceVariant& assignmentData) {
    if (!std::holds_alternative<std::shared_ptr<syrec::AssignStatement>>(assignmentData)) {
        return std::nullopt;
    }
    return std::get<std::shared_ptr<syrec::AssignStatement>>(assignmentData);
}

std::optional<std::shared_ptr<syrec::UnaryStatement>> TemporaryAssignmentsContainer::tryGetActiveAssignmentAsUnaryAssignment(const AssignmentReferenceVariant& assignmentData) {
    if (!std::holds_alternative<std::shared_ptr<syrec::UnaryStatement>>(assignmentData)) {
        return std::nullopt;
    }
    return std::get<std::shared_ptr<syrec::UnaryStatement>>(assignmentData);
}

bool TemporaryAssignmentsContainer::doSignalAccessesOverlap(const syrec::VariableAccess& first, const syrec::VariableAccess& second, const parser::SymbolTable& symbolTableReference) {
    const SignalAccessUtils::SignalAccessEquivalenceResult equivalenceResult = SignalAccessUtils::areSignalAccessesEqual(
            first, second,
            SignalAccessUtils::SignalAccessComponentEquivalenceCriteria::DimensionAccess::Overlapping,
            SignalAccessUtils::SignalAccessComponentEquivalenceCriteria::BitRange::Overlapping, symbolTableReference);
    if (!(equivalenceResult.isResultCertain && equivalenceResult.equality == SignalAccessUtils::SignalAccessEquivalenceResult::NotEqual)) {
        return true;
    }
    return false;
}

bool TemporaryAssignmentsContainer::doesAssignmentIdFulfillSortCriteria(std::size_t referenceAssignmentId, std::size_t newAssignmentId, DataDependencySortCriteria sortCriteria) {
    switch (sortCriteria) {
        case TemporaryAssignmentsContainer::DataDependencySortCriteria::Smallest:
            return newAssignmentId < referenceAssignmentId;
        case TemporaryAssignmentsContainer::DataDependencySortCriteria::Largest:
            return newAssignmentId > referenceAssignmentId;
        default:
            return false;
    }
}
