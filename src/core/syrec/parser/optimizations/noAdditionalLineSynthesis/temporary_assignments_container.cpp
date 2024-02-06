#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/temporary_assignments_container.hpp"
#include "core/syrec/parser/operation.hpp"
#include "core/syrec/parser/utils/copy_utils.hpp"
#include "core/syrec/parser/utils/signal_access_utils.hpp"

#include <algorithm>

using namespace noAdditionalLineSynthesis;

void TemporaryAssignmentsContainer::storeActiveAssignment(const SharedAssignmentReference& assignment) {
    std::optional<std::string> assignedToSignalIdent;
    std::optional<syrec::VariableAccess::ptr> assignedToSignalParts;
    if (std::holds_alternative<std::shared_ptr<syrec::AssignStatement>>(assignment)) {
        if (const auto& assignmentCastedAsBinaryOne = std::get<std::shared_ptr<syrec::AssignStatement>>(assignment); assignmentCastedAsBinaryOne) {
            assignedToSignalParts = assignmentCastedAsBinaryOne->lhs;
            assignedToSignalIdent = assignmentCastedAsBinaryOne->lhs->var->name;
        }
    } else if (std::holds_alternative<std::shared_ptr<syrec::UnaryStatement>>(assignment)) {
        if (const auto& assignmentCastedAsUnaryOne = std::get<std::shared_ptr<syrec::UnaryStatement>>(assignment); assignmentCastedAsUnaryOne) {
            assignedToSignalParts = assignmentCastedAsUnaryOne->var;
            assignedToSignalIdent = assignmentCastedAsUnaryOne->var->var->name;
        }
    }

    if (!assignedToSignalIdent.has_value() || !assignedToSignalParts.has_value()) {
        return;
    }
    
    if (!activeAssignmentsLookup.count(*assignedToSignalIdent)) {
        std::unordered_set<syrec::VariableAccess::ptr> lookupForAssignedToPartsOfSignal;
        lookupForAssignedToPartsOfSignal.insert(*assignedToSignalParts);
        activeAssignmentsLookup.insert(std::make_pair(*assignedToSignalIdent, lookupForAssignedToPartsOfSignal));
    } else {
        auto& existingAssignedToSignalParts = activeAssignmentsLookup.at(*assignedToSignalIdent);
        if (!existingAssignedToSignalParts.count(*assignedToSignalParts)) {
            existingAssignedToSignalParts.insert(*assignedToSignalParts);
        }
    }
    generatedAssignments.emplace_back(assignment);
    indicesOfActiveAssignments.emplace_back(generatedAssignments.size() - 1);
}

void TemporaryAssignmentsContainer::storeInitializationForReplacementOfLeafNode(const syrec::AssignStatement::ptr& assignmentDefiningSubstitution, const std::optional<SharedAssignmentReference>& optionalRequiredResetOfSubstitutionLhsOperand) {
    const std::shared_ptr<syrec::AssignStatement> castedAssignmentDefiningSubstitution = std::dynamic_pointer_cast<syrec::AssignStatement>(assignmentDefiningSubstitution);
    if (!castedAssignmentDefiningSubstitution || (optionalRequiredResetOfSubstitutionLhsOperand.has_value() && !castedAssignmentDefiningSubstitution)) {
        return;
    }
    
    if (optionalRequiredResetOfSubstitutionLhsOperand.has_value()) {
        initializationAssignmentsForGeneratedSubstitutions.emplace_back(*optionalRequiredResetOfSubstitutionLhsOperand);   
    }
    generatedAssignmentsDefiningPermanentSubstitutionsOfOperandsOfOperationNode.emplace_back(castedAssignmentDefiningSubstitution);
}

void TemporaryAssignmentsContainer::storeReplacementAsActiveAssignment(const syrec::AssignStatement::ptr& assignmentDefiningSubstitution, const std::optional<SharedAssignmentReference>& optionalRequiredResetOfSubstitutionLhsOperand) {
    const std::shared_ptr<syrec::AssignStatement> castedAssignmentDefiningSubstitution = std::dynamic_pointer_cast<syrec::AssignStatement>(assignmentDefiningSubstitution);
    if (!castedAssignmentDefiningSubstitution || (optionalRequiredResetOfSubstitutionLhsOperand.has_value() && !castedAssignmentDefiningSubstitution)) {
        return;
    }
    
    if (optionalRequiredResetOfSubstitutionLhsOperand.has_value()) {
        initializationAssignmentsForGeneratedSubstitutions.emplace_back(*optionalRequiredResetOfSubstitutionLhsOperand);
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
        const SharedAssignmentReference& activeReferenceAssignment = generatedAssignments.at(*activeAssignmentIndicesIterator);
        if (std::holds_alternative<std::shared_ptr<syrec::AssignStatement>>(activeReferenceAssignment)) {
            const std::shared_ptr<syrec::AssignStatement> activeReferenceAssignmentAsBinaryOne = std::get<std::shared_ptr<syrec::AssignStatement>>(activeReferenceAssignment);
            if (activeReferenceAssignmentAsBinaryOne && (!optionalExcludedFromInversionAssignmentsFilter || activeReferenceAssignmentAsBinaryOne->lhs != *optionalExcludedFromInversionAssignmentsFilter)) {
                invertAssignmentAndStoreAndMarkOriginalAsInactive(*activeAssignmentIndicesIterator);   
            }

        } else if (std::holds_alternative<std::shared_ptr<syrec::UnaryStatement>>(activeReferenceAssignment)) {
            const std::shared_ptr<syrec::UnaryStatement> activeReferenceAssignmentAsUnaryOne = std::get<std::shared_ptr<syrec::UnaryStatement>>(activeReferenceAssignment);
            if (activeReferenceAssignmentAsUnaryOne && (!optionalExcludedFromInversionAssignmentsFilter || activeReferenceAssignmentAsUnaryOne->var != *optionalExcludedFromInversionAssignmentsFilter)) {
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
        std::optional<syrec::VariableAccess::ptr> assignedToSignalPartsToBeRemovedFromLookup;
        if (std::holds_alternative<std::shared_ptr<syrec::AssignStatement>>(*generatedAssignmentsToBeRemovedIterator)) {
            assignedToSignalPartsToBeRemovedFromLookup = std::get<std::shared_ptr<syrec::AssignStatement>>(*generatedAssignmentsToBeRemovedIterator)->lhs;
        } else if (std::holds_alternative<std::shared_ptr<syrec::UnaryStatement>>(*generatedAssignmentsToBeRemovedIterator)) {
            assignedToSignalPartsToBeRemovedFromLookup = std::get<std::shared_ptr<syrec::UnaryStatement>>(*generatedAssignmentsToBeRemovedIterator)->var;
        }

        if (assignedToSignalPartsToBeRemovedFromLookup.has_value()) {
            removeActiveAssignmentFromLookup(*assignedToSignalPartsToBeRemovedFromLookup);   
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
    generatedAssignmentsDefiningPermanentSubstitutionsOfOperandsOfOperationNode.clear();
}

//// TODO: What should happen if we cannot fit all assignments into the container (i.e. a number larger than UINT_MAX)
std::vector<TemporaryAssignmentsContainer::SharedAssignmentReference> TemporaryAssignmentsContainer::getAssignments() const {
    if (generatedAssignmentsDefiningPermanentSubstitutionsOfOperandsOfOperationNode.empty()) {
        return generatedAssignments;
    }
    
    std::vector<SharedAssignmentReference> combinationOfGeneratedAssignmentsAndSubstitutionsOfOperationNodeOperandData;
    combinationOfGeneratedAssignmentsAndSubstitutionsOfOperationNodeOperandData.reserve(generatedAssignments.size() + (generatedAssignmentsDefiningPermanentSubstitutionsOfOperandsOfOperationNode.size() * 2));

    combinationOfGeneratedAssignmentsAndSubstitutionsOfOperationNodeOperandData.insert(combinationOfGeneratedAssignmentsAndSubstitutionsOfOperationNodeOperandData.end(), generatedAssignmentsDefiningPermanentSubstitutionsOfOperandsOfOperationNode.cbegin(), generatedAssignmentsDefiningPermanentSubstitutionsOfOperandsOfOperationNode.cend());
    combinationOfGeneratedAssignmentsAndSubstitutionsOfOperationNodeOperandData.insert(combinationOfGeneratedAssignmentsAndSubstitutionsOfOperationNodeOperandData.end(), generatedAssignments.cbegin(), generatedAssignments.cend());
    /*
     * To allow a reuse of the generated replacements, we need to reset our generated assignments defining the substitution of an operand of an operation node. Since the assignments defining the substitutions all use the XOR assignment operation,
     * the inversion of said statements is achieved by simply re-adding them to our container.
     */
    combinationOfGeneratedAssignmentsAndSubstitutionsOfOperationNodeOperandData.insert(combinationOfGeneratedAssignmentsAndSubstitutionsOfOperationNodeOperandData.end(), generatedAssignmentsDefiningPermanentSubstitutionsOfOperandsOfOperationNode.crbegin(), generatedAssignmentsDefiningPermanentSubstitutionsOfOperandsOfOperationNode.crend());
    return combinationOfGeneratedAssignmentsAndSubstitutionsOfOperationNodeOperandData;
}

std::optional<std::vector<TemporaryAssignmentsContainer::OwningAssignmentReference>> TemporaryAssignmentsContainer::getAssignmentsDefiningValueResetsOfGeneratedSubstitutions() const {
    return createOwningCopiesOfAssignments(initializationAssignmentsForGeneratedSubstitutions);
}

std::optional < std::vector < TemporaryAssignmentsContainer::OwningAssignmentReference>> TemporaryAssignmentsContainer::getInvertedAssignmentsUndoingValueResetsOfGeneratedSubstitutions() const {
    const std::optional<unsigned int> mappedToInversionOperationForBinaryAssignments = syrec_operation::tryMapAssignmentOperationEnumToFlag(syrec_operation::operation::XorAssign);
    const std::optional<unsigned int> mappedToInversionOperationForUnaryAssignments   = syrec_operation::tryMapUnaryAssignmentOperationEnumToFlag(syrec_operation::operation::IncrementAssign);
    if (initializationAssignmentsForGeneratedSubstitutions.empty() || !mappedToInversionOperationForBinaryAssignments.has_value() || !mappedToInversionOperationForUnaryAssignments.has_value()) {
        // Since the empty brace initialization is also applicable to the std::optional type, we need to use the construct below to initialize a non-empty std::optional of an empty vector
        return std::optional<std::vector<OwningAssignmentReference>>(std::in_place);
    }

    std::vector<OwningAssignmentReference> containerForOwningCopiesOfAssignments;
    containerForOwningCopiesOfAssignments.reserve(initializationAssignmentsForGeneratedSubstitutions.size());
    if (std::optional<std::vector<OwningAssignmentReference>> owningCopiesOfInitializationAssignments = getAssignmentsDefiningValueResetsOfGeneratedSubstitutions(); owningCopiesOfInitializationAssignments.has_value()) {
        containerForOwningCopiesOfAssignments.insert(containerForOwningCopiesOfAssignments.end(), std::make_move_iterator(owningCopiesOfInitializationAssignments->begin()), std::make_move_iterator(owningCopiesOfInitializationAssignments->end()));
        for (auto&& initializationStatement: containerForOwningCopiesOfAssignments) {
            OwningAssignmentReference temporaryOwnershipOfInitializationStatement = std::move(initializationStatement);
            
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
        return std::optional<std::vector<OwningAssignmentReference>>(std::in_place);
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

//// START OF NON-PUBLIC FUNCTIONS
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
    const auto& indexOfActiveAssignmentToRemove = std::find_if(
            indicesOfActiveAssignments.cbegin(),
            indicesOfActiveAssignments.cend(),
            [&indexOfAssignmentToInvert](const std::size_t indexOfActiveAssignment) {
                return indexOfAssignmentToInvert == indexOfActiveAssignment;
            });
    if (indexOfActiveAssignmentToRemove != indicesOfActiveAssignments.cend()) {
        indicesOfActiveAssignments.erase(indexOfActiveAssignmentToRemove);
    }

    if (std::holds_alternative<std::shared_ptr<syrec::AssignStatement>>(referenceGeneratedAssignment)) {
        const std::shared_ptr<syrec::AssignStatement> castedBinaryAssignmentStatement = std::get<std::shared_ptr<syrec::AssignStatement>>(referenceGeneratedAssignment);
        removeActiveAssignmentFromLookup(castedBinaryAssignmentStatement->lhs);

        if (std::shared_ptr<syrec::Statement> copyOfOriginalAssignment = copyUtils::createCopyOfStmt(*castedBinaryAssignmentStatement); copyOfOriginalAssignment) {
            if (std::shared_ptr<syrec::AssignStatement> generatedInvertedAssignment = std::dynamic_pointer_cast<syrec::AssignStatement>(copyOfOriginalAssignment); generatedInvertedAssignment) {
                invertAssignment(*generatedInvertedAssignment);
                generatedAssignments.emplace_back(generatedInvertedAssignment);
            }
        }
    } else if (std::holds_alternative<std::shared_ptr<syrec::UnaryStatement>>(referenceGeneratedAssignment)) {
        const std::shared_ptr<syrec::UnaryStatement> castedUnaryAssignmentStatement = std::get<std::shared_ptr<syrec::UnaryStatement>>(referenceGeneratedAssignment);
        removeActiveAssignmentFromLookup(castedUnaryAssignmentStatement->var);

        const std::shared_ptr<syrec::Statement> invertedUnaryAssignmentStatement = copyUtils::createCopyOfStmt(*castedUnaryAssignmentStatement);
        if (!invertedUnaryAssignmentStatement) {
            return;
        }

        std::shared_ptr<syrec::UnaryStatement> castedInvertedUnaryAssignmentStatement = std::dynamic_pointer_cast<syrec::UnaryStatement>(invertedUnaryAssignmentStatement);
        if (!castedInvertedUnaryAssignmentStatement) {
            return;
        }
        invertAssignment(*castedInvertedUnaryAssignmentStatement);
        generatedAssignments.emplace_back(castedInvertedUnaryAssignmentStatement);
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

std::optional<TemporaryAssignmentsContainer::OwningAssignmentReference> TemporaryAssignmentsContainer::createOwningCopyOfAssignment(const syrec::AssignStatement& assignment) {
    if (std::unique_ptr<syrec::AssignStatement> owningCopyOfAssignment = std::make_unique<syrec::AssignStatement>(assignment); owningCopyOfAssignment) {
        return owningCopyOfAssignment;
    }
    return std::nullopt;
}

std::optional<TemporaryAssignmentsContainer::OwningAssignmentReference> TemporaryAssignmentsContainer::createOwningCopyOfAssignment(const syrec::UnaryStatement& assignment) {
    if (std::unique_ptr<syrec::UnaryStatement> owningCopyOfAssignment = std::make_unique<syrec::UnaryStatement>(assignment); owningCopyOfAssignment) {
        return owningCopyOfAssignment;
    }
    return std::nullopt;
}


std::optional<std::vector<TemporaryAssignmentsContainer::OwningAssignmentReference>> TemporaryAssignmentsContainer::createOwningCopiesOfAssignments(const std::vector<SharedAssignmentReference>& assignments) {
    if (assignments.empty()) {
        // Since the empty brace initialization is also applicable to the std::optional type, we need to use the construct below to initialize a non-empty std::optional of an empty vector
        return std::optional<std::vector<OwningAssignmentReference>>(std::in_place);
    }

    std::vector<OwningAssignmentReference> containerForCopies;
    containerForCopies.reserve(assignments.size());

    for (const auto& statementToTreatAsAssignment: assignments) {
        std::optional<OwningAssignmentReference> copyOfAssignment;
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