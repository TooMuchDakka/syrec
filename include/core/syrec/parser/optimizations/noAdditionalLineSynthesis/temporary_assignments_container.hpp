#ifndef TEMPORARY_ASSIGNMENT_CONTAINER_HPP
#define TEMPORARY_ASSIGNMENT_CONTAINER_HPP
#pragma once

#include "core/syrec/statement.hpp"
#include "core/syrec/parser/symbol_table.hpp"

#include <memory>
#include <set>
#include <unordered_set>

namespace noAdditionalLineSynthesis {
    class TemporaryAssignmentsContainer {
    public:
        using ptr = std::shared_ptr<TemporaryAssignmentsContainer>;
        using AssignmentReferenceVariant = std::variant<std::shared_ptr<syrec::AssignStatement>, std::shared_ptr<syrec::UnaryStatement>>;
        using OwningAssignmentReferenceVariant = std::variant<std::unique_ptr<syrec::AssignStatement>, std::unique_ptr<syrec::UnaryStatement>>;
        using OrderedAssignmentIdContainer = std::set<std::size_t, std::greater<>>;

        [[nodiscard]] std::optional<std::size_t>                                        getOverlappingActiveAssignmentForSignalAccessWithIdNotInRange(const syrec::VariableAccess& signalAccess, std::size_t exclusionRangeLowerBoundDefinedWithAssociatedOperationNodeId, std::size_t exclusionRangeUpperBoundDefinedWithAssociatedOperationNodeId, const parser::SymbolTable& symbolTableReference) const;
        [[nodiscard]] bool                                                              existsActiveAssignmentHavingGivenAssignmentAsDataDependency(std::size_t assignmentIdWhichShallBeUsedAsADataDependency) const;
        [[nodiscard]] OrderedAssignmentIdContainer                                      getOverlappingActiveAssignmentsOrderedByLastAppearanceOfSignalAccessedDefinedInExpression(const syrec::Expression& expression, const parser::SymbolTable& symbolTableReference) const;
        [[nodiscard]] OrderedAssignmentIdContainer                                      getOverlappingActiveAssignmentsOrderedByLastAppearanceOfSignalAccess(const syrec::VariableAccess& signalAccess, const parser::SymbolTable& symbolTableReference) const;
        [[nodiscard]] OrderedAssignmentIdContainer                                      getDataDependenciesOfAssignment(std::size_t assignmentId) const;
        [[nodiscard]] std::optional<std::size_t>                                        getIdOfLastGeneratedAssignment() const;
        // Interval is (start, end) 
        [[nodiscard]] bool invertAssignmentsWithIdInRange(std::size_t idOfAssignmentDefiningLowerBoundOfOpenInterval, const std::optional<std::size_t>& idOfAssignmentDefiningUpperBoundOfOpenInterval);

        enum DataDependencySortCriteria {
            Smallest,
            Largest
        };
        [[nodiscard]] std::optional<std::size_t> findDataDependencyOfCurrentAndChildAssignmentsFittingCriteria(const std::vector<std::size_t>& assignmentsToCheck, DataDependencySortCriteria sortCriteria) const;
        

        [[nodiscard]] OrderedAssignmentIdContainer determineIdsOfAssignmentFormingCompleteInheritanceChainFromEarliestToLastOverlappingAssignment(const syrec::VariableAccess& signalAccessForWhichOverlapsShouldBeFound, const parser::SymbolTable& symbolTableReference) const;

        enum AssignmentCreationTimeRelativeToLastCreatedOne {
            Earliest,
            Latest
        };
        [[nodiscard]] std::optional<std::size_t> determineIdOfOverlappingAssignmentBasedOnCreationTime(const syrec::VariableAccess& signalAccessForWhichOverlapsShouldBeFound, AssignmentCreationTimeRelativeToLastCreatedOne relativeAssignmentCreationTime, const parser::SymbolTable& symbolTableReference) const;


        void                                        replayNotActiveDataDependencies(const OrderedAssignmentIdContainer& dataDependencies, std::size_t associatedWithOperationNodeId);
        void                                        invertAssignmentAndDataDependencies(std::size_t assignmentId);
        void                                        invertAssignmentsButLeaveDataDependenciesUnchanged(const OrderedAssignmentIdContainer& orderedIdsOfAssignmentToInvert);
        [[nodiscard]] OrderedAssignmentIdContainer  getIdsOfAssignmentsDefiningInheritanceChainToCurrentOne(std::size_t idOfLastAssignmentInheritingAssignedToSignalParts) const;
        [[maybe_unused]] std::optional<std::size_t> storeActiveAssignment(const AssignmentReferenceVariant& assignmentData, const std::optional<std::size_t>& idOfAssignmentFromWhichAssignedToSignalPartsWhereInherited, const OrderedAssignmentIdContainer& dataDependencies, std::size_t associatedWithOperationNodeId, const std::optional<std::size_t>& optionalIdOfInvertedAssignment);

        
        [[nodiscard]] bool storeInitializationForReplacementOfLeafNode(const syrec::AssignStatement::ptr& assignmentDefiningSubstitution, const std::optional<AssignmentReferenceVariant>& optionalRequiredResetOfSubstitutionLhsOperand);
        [[nodiscard]] bool storeReplacementAsActiveAssignment(std::size_t associatedOperationNodeId, const syrec::AssignStatement::ptr& assignmentDefiningSubstitution, const OrderedAssignmentIdContainer& dataDependenciesOfSubstitutionRhs, const std::optional<AssignmentReferenceVariant>& optionalRequiredPriorResetOfSignalUsedOnSubstitutionLhs);
        void               markCutoffForInvertibleAssignments();
        void               invertAllActiveAssignmentsExcludingFirstActiveOne();
        void               popLastCutoffForInvertibleAssignments();
        void               rollbackLastXAssignments(std::size_t numberOfAssignmentsToRemove);
        void               rollbackAssignmentsMadeSinceLastCutoffAndOptionallyPopCutoff(bool popCutOff);
        void               resetInternals();

        [[nodiscard]] std::vector<AssignmentReferenceVariant>                             getAssignments() const;
        [[nodiscard]] std::optional<std::vector<OwningAssignmentReferenceVariant>>        getAssignmentsDefiningValueResetsOfGeneratedSubstitutions() const;
        [[nodiscard]] std::optional<std::vector<OwningAssignmentReferenceVariant>>        getInvertedAssignmentsUndoingValueResetsOfGeneratedSubstitutions() const;
        [[nodiscard]] bool                                                                existsOverlappingAssignmentFor(const syrec::VariableAccess& assignedToSignal, const parser::SymbolTable& symbolTable) const;
        [[nodiscard]] std::size_t                                                         getNumberOfAssignments() const;
    protected:
        struct InternalAssignmentContainer {
            using Reference         = std::shared_ptr<InternalAssignmentContainer>;
            using ReadOnlyReference = std::shared_ptr<const InternalAssignmentContainer>;

            enum AssignmentType {
                SubAssignmentGeneration,
                InversionOfSubAssignment
            };

            bool                         isStillActive;
            AssignmentType               assignmentType;
            std::size_t                  id;
            std::size_t                  idOfReferencedAssignment;
            std::optional<std::size_t>   idOfAssignmentFromWhichAssignedToSignalPartsWhereInherited;
            std::optional<std::size_t>   associatedWithOperationNodeId;
            AssignmentReferenceVariant   data;
            OrderedAssignmentIdContainer dataDependencies;

            explicit InternalAssignmentContainer(AssignmentType assignmentType, std::size_t id):
                isStillActive(true), assignmentType(assignmentType), id(id), idOfReferencedAssignment(0), associatedWithOperationNodeId(0) {}

            [[nodiscard]] static InternalAssignmentContainer asInversionOfSubAssignment(std::size_t idOfAssignment, std::size_t idOfInvertedAssignment, const AssignmentReferenceVariant& data, const OrderedAssignmentIdContainer& dataDependencies) {
                InternalAssignmentContainer assignment(AssignmentType::InversionOfSubAssignment, idOfAssignment);
                assignment.idOfReferencedAssignment = idOfInvertedAssignment;
                assignment.data                     = data;
                assignment.dataDependencies         = dataDependencies;
                return assignment;
            }

            [[nodiscard]] static InternalAssignmentContainer asSubAssignment(std::size_t idOfAssignment, std::size_t associatedWithOperationNodeId, const std::optional<std::size_t>& idOfAssignmentFromWhichAssignedToSignalPartsWhereInherited, const AssignmentReferenceVariant& data, const OrderedAssignmentIdContainer& dataDependencies) {
                InternalAssignmentContainer assignment(AssignmentType::SubAssignmentGeneration, idOfAssignment);
                assignment.idOfAssignmentFromWhichAssignedToSignalPartsWhereInherited = idOfAssignmentFromWhichAssignedToSignalPartsWhereInherited;
                assignment.associatedWithOperationNodeId = associatedWithOperationNodeId;
                assignment.data                     = data;
                assignment.dataDependencies         = dataDependencies;
                return assignment;
            }

            [[nodiscard]] std::shared_ptr<syrec::VariableAccess> getAssignedToSignalParts() const;
        };

        OrderedAssignmentIdContainer                                                        activeAssignmentsLookupById;
        std::map<std::size_t, InternalAssignmentContainer::Reference, std::greater<>> internalAssignmentContainer;
        std::vector<std::size_t>                                                            cutOffIndicesForInvertibleAssignments;

        std::vector<std::shared_ptr<syrec::AssignStatement>> generatedAssignmentsDefiningPermanentSubstitutionsOfOperandsOfOperationNode;
        std::vector<AssignmentReferenceVariant>              initializationAssignmentsForGeneratedSubstitutions;

        [[nodiscard]] bool                replayNotActiveDataDependenciesOfAssignment(std::size_t assignmentId, bool replayAssignmentItself);
        [[nodiscard]] std::optional<bool> isAssignmentActive(std::size_t assignmentId) const;
        [[nodiscard]] std::optional<std::size_t> findDataDependencyOfCurrentAndChildAssignmentsFittingCriteria(std::size_t assignmentId, DataDependencySortCriteria sortCriteria, OrderedAssignmentIdContainer& alreadyCheckedAssignmentIds) const;

        enum DataDependencyActiveFlag {
            Inactive = 0,
            Active   = 1
        };
        [[nodiscard]] OrderedAssignmentIdContainer getDirectAndChildDataDependenciesOfAssignment(std::size_t assignmentId, DataDependencyActiveFlag isActiveFilterOnlyReturningMatchingElements) const;
        [[nodiscard]] OrderedAssignmentIdContainer getDirectAndChildDataDependenciesOfAssignment(std::size_t assignmentId, DataDependencyActiveFlag isActiveFilterOnlyReturningMatchingElements, std::unordered_set<std::size_t>& idsOfAlreadyProcessedAssignments) const;

        void                                                                                     invertAssignmentAndStoreAndMarkOriginalAsInactive(std::size_t idOfToBeInvertedAssignment);
        void                                                                                     markAssignmentAsInactiveInLookup(std::size_t assignmentId);
        void                                                                                     removeActiveAssignmentFromLookup(std::size_t assignmentId);
        [[nodiscard]] std::optional<InternalAssignmentContainer::Reference>                      getAssignmentById(std::size_t assignmentId) const;
        [[nodiscard]] std::optional<InternalAssignmentContainer::ReadOnlyReference>              getMatchingActiveSubassignmentById(std::size_t assignmentId) const;
        [[nodiscard]] std::optional<std::size_t>                                                 getIdOfLastInversionOfAssignment(std::size_t assignmentId) const;
        static void                                                                              invertAssignment(syrec::AssignStatement& assignment);
        static void                                                                              invertAssignment(syrec::UnaryStatement& assignment);
        [[nodiscard]] static std::optional<OwningAssignmentReferenceVariant>                     createOwningCopyOfAssignment(const syrec::AssignStatement& assignment);
        [[nodiscard]] static std::optional<OwningAssignmentReferenceVariant>                     createOwningCopyOfAssignment(const syrec::UnaryStatement& assignment);
        [[nodiscard]] static std::optional<std::vector<OwningAssignmentReferenceVariant>>        createOwningCopiesOfAssignments(const std::vector<AssignmentReferenceVariant>& assignments);
        [[nodiscard]] static std::vector<std::reference_wrapper<syrec::VariableAccess>>          getSignalAccessesOfExpression(const syrec::Expression& expression);
        [[nodiscard]] static std::optional<std::shared_ptr<syrec::AssignStatement>>              tryGetActiveAssignmentAsBinaryAssignment(const AssignmentReferenceVariant& assignmentData);
        [[nodiscard]] static std::optional<std::shared_ptr<syrec::UnaryStatement>>               tryGetActiveAssignmentAsUnaryAssignment(const AssignmentReferenceVariant& assignmentData);

        [[nodiscard]] static bool doSignalAccessesOverlap(const syrec::VariableAccess& first, const syrec::VariableAccess& second, const parser::SymbolTable& symbolTableReference);
        [[nodiscard]] static bool doesAssignmentIdFulfillSortCriteria(std::size_t referenceAssignmentId, std::size_t newAssignmentId, DataDependencySortCriteria sortCriteria);
    };
}

#endif