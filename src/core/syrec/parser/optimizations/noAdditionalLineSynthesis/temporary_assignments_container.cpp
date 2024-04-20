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

std::optional<std::size_t> TemporaryAssignmentsContainer::getOverlappingActiveAssignmentForSignalAccess(const syrec::VariableAccess& signalAccess, const std::vector<SearchSpaceIntervalBounds>& searchSpacesToConsiderBasedOnOperationNodeIds, const parser::SymbolTable& symbolTableReference) const {
    const auto& foundMatchingActiveAssignment = std::find_if(
            activeAssignmentsLookupById.cbegin(),
            activeAssignmentsLookupById.cend(),
            [&](std::size_t activeAssignmentIdFromLookup) {
                const std::optional<InternalAssignmentContainer::ReadOnlyReference> activeAssignmentMatchingId = getMatchingActiveSubassignmentById(activeAssignmentIdFromLookup);
                if (!activeAssignmentMatchingId.has_value() || !activeAssignmentMatchingId->get()->associatedWithOperationNodeId.has_value() || !isIdIncludedInSearchSpaceInterval(*activeAssignmentMatchingId->get()->associatedWithOperationNodeId, searchSpacesToConsiderBasedOnOperationNodeIds)) {
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

TemporaryAssignmentsContainer::OrderedAssignmentIdContainer TemporaryAssignmentsContainer::getDataDependenciesOfAssignment(std::size_t assignmentId) const {
    if (!internalAssignmentContainer.count(assignmentId)) {
        return {};
    }
    return internalAssignmentContainer.at(assignmentId)->dataDependencies;
}

std::optional<std::size_t> TemporaryAssignmentsContainer::getIdOfLastGeneratedAssignment() const {
    return internalAssignmentContainer.empty() ? std::nullopt : std::make_optional(internalAssignmentContainer.cbegin()->first);
}

bool TemporaryAssignmentsContainer::invertAssignmentAndDataDependencies(std::size_t assignmentId) {
    const std::optional<InternalAssignmentContainer::Reference> matchingAssignmentForId = getAssignmentById(assignmentId);
    if (!matchingAssignmentForId.has_value() || !isAssignmentActive(assignmentId)) {
        return true;
    }

    const OrderedAssignmentIdContainer dataDependenciesOfAssignmentToBeInverted = getDataDependenciesOfAssignment(assignmentId);
    OrderedAssignmentIdContainer dataDependenciesOfInvertedAssignment;
    for (const std::size_t dataDependency : dataDependenciesOfAssignmentToBeInverted) {
        if (isAssignmentActive(dataDependency)) {
            dataDependenciesOfInvertedAssignment.emplace(dataDependency);
            continue;
        }

        const std::optional<std::size_t> idOfReplayedDataDependency = replayNotActiveAssignment(dataDependency);
        if (!idOfReplayedDataDependency.has_value()) {
            return false;
        }
        dataDependenciesOfInvertedAssignment.emplace(*idOfReplayedDataDependency);
    }

    std::optional<AssignmentReferenceVariant> generatedCopyOfAssignment = createCopyOfAssignmentFromInternalContainerEntry(**matchingAssignmentForId);
    if (!generatedCopyOfAssignment.has_value()) {
        return false;
    }

    if (std::holds_alternative<std::shared_ptr<syrec::AssignStatement>>(*generatedCopyOfAssignment)) {
        const std::shared_ptr<syrec::AssignStatement> generatedCopyAsAssignmentStatement = std::get<std::shared_ptr<syrec::AssignStatement>>(*generatedCopyOfAssignment);
        invertAssignment(*generatedCopyAsAssignmentStatement);
    } else if (std::holds_alternative<std::shared_ptr<syrec::UnaryStatement>>(*generatedCopyOfAssignment)){
        const std::shared_ptr<syrec::UnaryStatement> generatedCopyAsUnaryStatement = std::get<std::shared_ptr<syrec::UnaryStatement>>(*generatedCopyOfAssignment);
        invertAssignment(*generatedCopyAsUnaryStatement);
    } else {
        generatedCopyOfAssignment.reset();
    }

    const std::optional<std::size_t> idOfInversionAssignment = storeActiveAssignment(*generatedCopyOfAssignment, std::nullopt, dataDependenciesOfInvertedAssignment, 0, assignmentId);
    if (!idOfInversionAssignment.has_value() 
        || !std::all_of(dataDependenciesOfInvertedAssignment.cbegin(), dataDependenciesOfInvertedAssignment.cend(), [&](const std::size_t dataDependency) { return invertAssignmentAndDataDependencies(dataDependency); })) {
        return false;
    }
    markAssignmentAsInactiveInLookup(assignmentId);
    matchingAssignmentForId->get()->idOfReferencedAssignment = *idOfInversionAssignment;

    if (matchingAssignmentForId->get()->idOfAssignmentFromWhichAssignedToSignalPartsWhereInherited.has_value()) {
        return invertAssignmentAndDataDependencies(*matchingAssignmentForId->get()->idOfAssignmentFromWhichAssignedToSignalPartsWhereInherited);
    }
    return true;
}

std::optional<std::size_t> TemporaryAssignmentsContainer::replayNotActiveAssignment(std::size_t assignmentId) {
    const std::optional<InternalAssignmentContainer::Reference> matchingAssignmentForId = getAssignmentById(assignmentId);

    if (!matchingAssignmentForId.has_value() || isAssignmentActive(assignmentId)) {
        return assignmentId;
    }

    const OrderedAssignmentIdContainer dataDependenciesOfAssignmentToBeReplayed = getDataDependenciesOfAssignment(assignmentId);
    OrderedAssignmentIdContainer       dataDependenciesOfReplayedAssignment;
    for (const std::size_t dataDependency: dataDependenciesOfAssignmentToBeReplayed) {
        if (isAssignmentActive(dataDependency)) {
            dataDependenciesOfReplayedAssignment.emplace(dataDependency);
            continue;
        }

        const std::optional<std::size_t> idOfReplayedDataDependency = replayNotActiveAssignment(dataDependency);
        if (!idOfReplayedDataDependency.has_value()) {
            return std::nullopt;
        }
        dataDependenciesOfReplayedAssignment.emplace(*idOfReplayedDataDependency);
    }

    const std::optional<AssignmentReferenceVariant> generatedCopyOfAssignment = createCopyOfAssignmentFromInternalContainerEntry(**matchingAssignmentForId);
    if (!generatedCopyOfAssignment.has_value()) {
        return std::nullopt;
    }

    const std::optional<std::size_t> idOfReplayedAssignment = storeActiveAssignment(*generatedCopyOfAssignment, std::nullopt, dataDependenciesOfReplayedAssignment, 0, std::nullopt);
    if (!idOfReplayedAssignment.has_value()
        || !std::all_of(dataDependenciesOfReplayedAssignment.cbegin(), dataDependenciesOfReplayedAssignment.cend(), [&](const std::size_t dataDependency) { return invertAssignmentAndDataDependencies(dataDependency); })) {
        return std::nullopt;
    }

    if (matchingAssignmentForId->get()->idOfAssignmentFromWhichAssignedToSignalPartsWhereInherited.has_value()) {
        const std::optional<InternalAssignmentContainer::Reference> matchingReplayedAssignment        = getAssignmentById(*idOfReplayedAssignment);
        matchingReplayedAssignment->get()->idOfAssignmentFromWhichAssignedToSignalPartsWhereInherited = replayNotActiveAssignment(*matchingAssignmentForId->get()->idOfAssignmentFromWhichAssignedToSignalPartsWhereInherited);
        if (!matchingReplayedAssignment->get()->idOfAssignmentFromWhichAssignedToSignalPartsWhereInherited.has_value()) {
            return std::nullopt;
        }
    }
    return idOfReplayedAssignment;
}

std::optional<TemporaryAssignmentsContainer::AssignmentReferenceVariant> TemporaryAssignmentsContainer::createCopyOfAssignmentFromInternalContainerEntry(const InternalAssignmentContainer& assignmentToCopy) {
    if (const std::shared_ptr<syrec::AssignStatement> assignmentToCopyAsAssignStatement = std::holds_alternative<std::shared_ptr<syrec::AssignStatement>>(assignmentToCopy.data) ? std::get<std::shared_ptr<syrec::AssignStatement>>(assignmentToCopy.data) : nullptr; assignmentToCopyAsAssignStatement) {
        if (std::optional<OwningAssignmentReferenceVariant> owningCopyOfAssignStatement = createOwningCopyOfAssignment(*assignmentToCopyAsAssignStatement); owningCopyOfAssignStatement.has_value() && std::holds_alternative<std::unique_ptr<syrec::AssignStatement>>(*owningCopyOfAssignStatement)) {
            std::shared_ptr<syrec::AssignStatement> finalOwningCopyOfAssignAssignment = std::move(std::get<std::unique_ptr<syrec::AssignStatement>>(*owningCopyOfAssignStatement));
            return finalOwningCopyOfAssignAssignment;
        }
    } else if (const std::shared_ptr<syrec::UnaryStatement> assignmentToCopyAsUnaryStatement = std::holds_alternative<std::shared_ptr<syrec::UnaryStatement>>(assignmentToCopy.data) ? std::get<std::shared_ptr<syrec::UnaryStatement>>(assignmentToCopy.data) : nullptr; assignmentToCopyAsUnaryStatement) {
        if (std::optional<OwningAssignmentReferenceVariant> owningCopyOfUnaryStatement = createOwningCopyOfAssignment(*assignmentToCopyAsUnaryStatement); owningCopyOfUnaryStatement.has_value() && std::holds_alternative<std::unique_ptr<syrec::UnaryStatement>>(*owningCopyOfUnaryStatement)) {
            std::shared_ptr<syrec::UnaryStatement> finalOwningCopyOfUnaryAssignment = std::move(std::get<std::unique_ptr<syrec::UnaryStatement>>(*owningCopyOfUnaryStatement));
            return finalOwningCopyOfUnaryAssignment;
        }
    }
    return std::nullopt;
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

std::optional<std::size_t> TemporaryAssignmentsContainer::storeReplacementAsActiveAssignment(std::size_t associatedOperationNodeId, const syrec::AssignStatement::ptr& assignmentDefiningSubstitution, const OrderedAssignmentIdContainer& dataDependenciesOfSubstitutionRhs, const std::optional<AssignmentReferenceVariant>& optionalRequiredPriorResetOfSignalUsedOnSubstitutionLhs) {
    const std::shared_ptr<syrec::AssignStatement> castedAssignmentDefiningSubstitution = std::dynamic_pointer_cast<syrec::AssignStatement>(assignmentDefiningSubstitution);
    if (!castedAssignmentDefiningSubstitution) {
        return std::nullopt;
    }
    if (const std::optional<std::size_t>& idOfGeneratedAssignment = storeActiveAssignment(castedAssignmentDefiningSubstitution, std::nullopt, dataDependenciesOfSubstitutionRhs, associatedOperationNodeId, std::nullopt); idOfGeneratedAssignment.has_value()) {
        if (optionalRequiredPriorResetOfSignalUsedOnSubstitutionLhs.has_value()) {
            initializationAssignmentsForGeneratedSubstitutions.emplace_back(*optionalRequiredPriorResetOfSignalUsedOnSubstitutionLhs);
        }
        return idOfGeneratedAssignment;
    }
    return std::nullopt;
}

void TemporaryAssignmentsContainer::markCutoffForInvertibleAssignments() {
    cutOffIndicesForInvertibleAssignments.emplace_back(internalAssignmentContainer.size());
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
            if (generatedAssignmentLookupEntry->second->assignmentType == InternalAssignmentContainer::AssignmentType::SubAssignmentGeneration) {
                markAssignmentAsActiveInLookup(dataOfInvertedAssignment->get()->id);
            }
        }
        markAssignmentAsInactiveInLookup(generatedAssignmentLookupEntry->first);
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

    /*
     * Due to the fact that the internal assignment container is sorted by the associated operation node id ascending which means more recently generated assignments are found at smaller indices of the container.
     * Since we need to return the generated assignment in the order in which they were created we need to iterate our assignment container in reverse
     */
    for (auto generatedSubAssignmentsIterator = internalAssignmentContainer.crbegin(); generatedSubAssignmentsIterator != internalAssignmentContainer.crend(); ++generatedSubAssignmentsIterator) {
        combinationOfGeneratedAssignmentsAndSubstitutionsOfOperationNodeOperandData.emplace_back(generatedSubAssignmentsIterator->second->data);
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

std::optional<syrec::VariableAccess::ptr> TemporaryAssignmentsContainer::getAssignedToSignalPartsOfAssignmentById(std::size_t assignmentId) const {
    if (const std::optional<InternalAssignmentContainer::Reference> matchingAssignmentForId = getAssignmentById(assignmentId); matchingAssignmentForId.has_value()) {
        return matchingAssignmentForId->get()->getAssignedToSignalParts();
    }
    return std::nullopt;
}

std::optional<std::size_t> TemporaryAssignmentsContainer::getAssociatedOperationNodeIdForAssignmentById(std::size_t assignmentId) const {
    if (const std::optional<InternalAssignmentContainer::Reference> matchingAssignmentForId = getAssignmentById(assignmentId); matchingAssignmentForId.has_value()) {
        return matchingAssignmentForId->get()->associatedWithOperationNodeId;
    }
    return std::nullopt;
}

std::vector<syrec::VariableAccess::ptr> TemporaryAssignmentsContainer::getSignalAccessesDefinedInAssignmentRhsExcludingDataDependencies(std::size_t assignmentId) const {
    if (const std::optional<InternalAssignmentContainer::Reference> matchingAssignmentForId = getAssignmentById(assignmentId); matchingAssignmentForId.has_value()) {
        if (std::holds_alternative<std::shared_ptr<syrec::AssignStatement>>(matchingAssignmentForId->get()->data)) {
            const auto& assignmentAsNonUnaryOne = std::get<std::shared_ptr<syrec::AssignStatement>>(matchingAssignmentForId->get()->data);

            const std::vector<syrec::VariableAccess::ptr>  determinedSignalAccessesOfAssignmentRhsExpr = getSignalAccessesOfExpression(*assignmentAsNonUnaryOne->rhs);
            std::unordered_set<syrec::VariableAccess::ptr> signalAccessesOfAssignmentRhsExpr(determinedSignalAccessesOfAssignmentRhsExpr.begin(), determinedSignalAccessesOfAssignmentRhsExpr.end());

            for (const std::size_t dataDependency : matchingAssignmentForId->get()->dataDependencies) {
                const std::optional<InternalAssignmentContainer::Reference> matchingAssignmentForDataDependency = getAssignmentById(dataDependency);
                const syrec::VariableAccess::ptr assignedToSignalPartsOfDataDependency = matchingAssignmentForDataDependency->get()->getAssignedToSignalParts();
                signalAccessesOfAssignmentRhsExpr.erase(assignedToSignalPartsOfDataDependency);
            }

            if (!signalAccessesOfAssignmentRhsExpr.empty()) {
                std::vector<syrec::VariableAccess::ptr> resultContainer(signalAccessesOfAssignmentRhsExpr.size());
                std::copy(signalAccessesOfAssignmentRhsExpr.begin(), signalAccessesOfAssignmentRhsExpr.end(), resultContainer.begin());
                return resultContainer;
            }
        }
    }
    return {};
}

std::optional<std::size_t> TemporaryAssignmentsContainer::getIdOfOperationNodeWhereOriginOfInheritanceChainIsLocated(std::size_t assignmentId) const {
    if (const std::optional<InternalAssignmentContainer::Reference> matchingAssignmentForId = getAssignmentById(assignmentId); matchingAssignmentForId.has_value()) {
        if (matchingAssignmentForId->get()->idOfAssignmentFromWhichAssignedToSignalPartsWhereInherited.has_value()) {
            return getIdOfOperationNodeWhereOriginOfInheritanceChainIsLocated(*matchingAssignmentForId->get()->idOfAssignmentFromWhichAssignedToSignalPartsWhereInherited);
        }
        return matchingAssignmentForId->get()->associatedWithOperationNodeId;
    }
    return std::nullopt;
}

std::size_t TemporaryAssignmentsContainer::determineLengthOfInheritanceChainLengthOfAssignment(std::size_t assignmentId) const {
    if (const std::optional<InternalAssignmentContainer::Reference> matchingAssignmentForId = getAssignmentById(assignmentId); matchingAssignmentForId.has_value()) {
        if (matchingAssignmentForId->get()->idOfAssignmentFromWhichAssignedToSignalPartsWhereInherited.has_value()) {
            return determineLengthOfInheritanceChainLengthOfAssignment(*matchingAssignmentForId->get()->idOfAssignmentFromWhichAssignedToSignalPartsWhereInherited) + 1;    
        }
        return 1;
    }
    return 0;
}


std::optional<TemporaryAssignmentsContainer::IsAssignmentLiveAndTypeInformation> TemporaryAssignmentsContainer::isAssignmentLiveAndInversionOfOtherAssignment(std::size_t assignmentId) const {
    if (const std::optional<InternalAssignmentContainer::Reference> matchingAssignmentForId = getAssignmentById(assignmentId); matchingAssignmentForId.has_value()) {
        IsAssignmentLiveAndTypeInformation information{};
        information.isLive = activeAssignmentsLookupById.count(assignmentId);
        information.isInversionOfOtherAssignment = matchingAssignmentForId->get()->assignmentType == InternalAssignmentContainer::AssignmentType::InversionOfSubAssignment;
        return information;
    }
    return std::nullopt;
}


TemporaryAssignmentsContainer::OrderedBasicActiveAssignmentDataLookup TemporaryAssignmentsContainer::getActiveAssignments(const std::vector<TemporaryAssignmentsContainer::SearchSpaceIntervalBounds>& searchSpacesToConsider) const {
    OrderedBasicActiveAssignmentDataLookup basicActiveAssignmentDataLookup;
    if (searchSpacesToConsider.empty()) {
        return basicActiveAssignmentDataLookup;
    }

    for (const std::size_t activeAssignmentId : activeAssignmentsLookupById) {
        const std::optional<InternalAssignmentContainer::Reference> activeAssignmentData = getAssignmentById(activeAssignmentId);
        
        if (!activeAssignmentData.has_value() || activeAssignmentData->get()->assignmentType == InternalAssignmentContainer::AssignmentType::InversionOfSubAssignment || !activeAssignmentData->get()->associatedWithOperationNodeId.has_value()){
            continue;   
        }

        const std::size_t associatedWithOperationNodeId = *activeAssignmentData->get()->associatedWithOperationNodeId;
        if (!std::any_of(searchSpacesToConsider.cbegin(), searchSpacesToConsider.cend(), [associatedWithOperationNodeId](const SearchSpaceIntervalBounds& searchSpaceToConsider) {
                return associatedWithOperationNodeId >= searchSpaceToConsider.lowerBoundInclusive && associatedWithOperationNodeId <= searchSpaceToConsider.upperBoundInclusive; 
        })) {
            continue;
        }
        
        BasicActiveAssignmentData data;
        data.associatedOperationNodeId = activeAssignmentData->get()->associatedWithOperationNodeId.value_or(0);
        data.assignedToSignalParts     = activeAssignmentData->get()->getAssignedToSignalParts();
        data.dataDependencies          = activeAssignmentData->get()->dataDependencies;

        basicActiveAssignmentDataLookup.emplace(activeAssignmentData->get()->id, data);
    }
    return basicActiveAssignmentDataLookup;
}


//// START OF NON-PUBLIC FUNCTIONS
///
inline bool TemporaryAssignmentsContainer::isAssignmentActive(std::size_t assignmentId) const {
    return activeAssignmentsLookupById.count(assignmentId);
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
        /*if (createdAssignment->assignmentType == InternalAssignmentContainer::AssignmentType::SubAssignmentGeneration) {
            activeAssignmentsLookupById.emplace(chosenAssignmentId);       
        }*/
        activeAssignmentsLookupById.emplace(chosenAssignmentId);       
        return chosenAssignmentId;
    }
    return std::nullopt;
}

inline void TemporaryAssignmentsContainer::markAssignmentAsActiveInLookup(std::size_t assigmentId) {
    activeAssignmentsLookupById.emplace(assigmentId);
}


inline void TemporaryAssignmentsContainer::markAssignmentAsInactiveInLookup(std::size_t assignmentId) {
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
    if (!isAssignmentActive(assignmentId)) {
        return std::nullopt;
    }

    if (const std::optional<TemporaryAssignmentsContainer::InternalAssignmentContainer::Reference> matchingAssignmentForId = getAssignmentById(assignmentId); matchingAssignmentForId.has_value()
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

std::vector<syrec::VariableAccess::ptr> TemporaryAssignmentsContainer::getSignalAccessesOfExpression(const syrec::Expression& expression) {
    std::vector<std::reference_wrapper<syrec::VariableAccess>> foundSignalAccessContainer;

    if (const auto& expressionAsVariableExpr = dynamic_cast<const syrec::VariableExpression*>(&expression); expressionAsVariableExpr) {
        return {expressionAsVariableExpr->var};
    }
    if (const auto& exprAsBinaryExpr = dynamic_cast<const syrec::BinaryExpression*>(&expression); exprAsBinaryExpr) {
        std::vector<syrec::VariableAccess::ptr>       foundSignalAccessOfLhsExpr = getSignalAccessesOfExpression(*exprAsBinaryExpr->lhs);
        const std::vector<syrec::VariableAccess::ptr> foundSignalAccessOfRhsExpr = getSignalAccessesOfExpression(*exprAsBinaryExpr->rhs);
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

bool TemporaryAssignmentsContainer::isIdIncludedInSearchSpaceInterval(std::size_t operationNodeId, const std::vector<SearchSpaceIntervalBounds>& searchSpacesToInclude) {
    return std::any_of(searchSpacesToInclude.cbegin(), searchSpacesToInclude.cend(), [operationNodeId](const SearchSpaceIntervalBounds& searchSpace) { return operationNodeId >= searchSpace.lowerBoundInclusive && operationNodeId <= searchSpace.upperBoundInclusive; });
}
