#ifndef TEMPORARY_ASSIGNMENT_CONTAINER_HPP
#define TEMPORARY_ASSIGNMENT_CONTAINER_HPP
#pragma once

#include "core/syrec/statement.hpp"
#include "core/syrec/parser/symbol_table.hpp"

#include <memory>
#include <set>

namespace noAdditionalLineSynthesis {
    class TemporaryAssignmentsContainer {
    public:
        using ptr                              = std::shared_ptr<TemporaryAssignmentsContainer>;
        using AssignmentReferenceVariant       = std::variant<std::shared_ptr<syrec::AssignStatement>, std::shared_ptr<syrec::UnaryStatement>>;
        using OwningAssignmentReferenceVariant = std::variant<std::unique_ptr<syrec::AssignStatement>, std::unique_ptr<syrec::UnaryStatement>>;
        using OrderedAssignmentIdContainer     = std::set<std::size_t, std::greater<>>;

        [[nodiscard]] std::optional<std::size_t>                                   getOverlappingActiveAssignmentForSignalAccessWithIdNotInRange(const syrec::VariableAccess& signalAccess, std::size_t exclusionRangeLowerBoundDefinedWithAssociatedOperationNodeId, std::size_t exclusionRangeUpperBoundDefinedWithAssociatedOperationNodeId, const parser::SymbolTable& symbolTableReference) const;
        [[nodiscard]] bool                                                         existsActiveAssignmentHavingGivenAssignmentAsDataDependency(std::size_t assignmentIdWhichShallBeUsedAsADataDependency) const;
        [[nodiscard]] OrderedAssignmentIdContainer                                 getDataDependenciesOfAssignment(std::size_t assignmentId) const;
        [[nodiscard]] std::optional<std::size_t>                                   getIdOfLastGeneratedAssignment() const;
        [[nodiscard]] bool                                                         invertAssignmentAndDataDependencies(std::size_t assignmentId);
        [[maybe_unused]] std::optional<std::size_t>                                storeActiveAssignment(const AssignmentReferenceVariant& assignmentData, const std::optional<std::size_t>& idOfAssignmentFromWhichAssignedToSignalPartsWhereInherited, const OrderedAssignmentIdContainer& dataDependencies, std::size_t associatedWithOperationNodeId, const std::optional<std::size_t>& optionalIdOfInvertedAssignment);
        [[nodiscard]] bool                                                         storeInitializationForReplacementOfLeafNode(const syrec::AssignStatement::ptr& assignmentDefiningSubstitution, const std::optional<AssignmentReferenceVariant>& optionalRequiredResetOfSubstitutionLhsOperand);
        [[nodiscard]] bool                                                         storeReplacementAsActiveAssignment(std::size_t associatedOperationNodeId, const syrec::AssignStatement::ptr& assignmentDefiningSubstitution, const OrderedAssignmentIdContainer& dataDependenciesOfSubstitutionRhs, const std::optional<AssignmentReferenceVariant>& optionalRequiredPriorResetOfSignalUsedOnSubstitutionLhs);
        [[nodiscard]] std::vector<AssignmentReferenceVariant>                      getAssignments() const;
        [[nodiscard]] std::optional<std::vector<OwningAssignmentReferenceVariant>> getAssignmentsDefiningValueResetsOfGeneratedSubstitutions() const;
        [[nodiscard]] std::optional<std::vector<OwningAssignmentReferenceVariant>> getInvertedAssignmentsUndoingValueResetsOfGeneratedSubstitutions() const;
        [[nodiscard]] bool                                                         existsOverlappingAssignmentFor(const syrec::VariableAccess& assignedToSignal, const parser::SymbolTable& symbolTable) const;
        [[nodiscard]] std::size_t                                                  getNumberOfAssignments() const;

        void markCutoffForInvertibleAssignments();
        void popLastCutoffForInvertibleAssignments();
        void rollbackLastXAssignments(std::size_t numberOfAssignmentsToRemove);
        void rollbackAssignmentsMadeSinceLastCutoffAndOptionallyPopCutoff(bool popCutOff);
        void resetInternals();

    protected:
        struct InternalAssignmentContainer {
            using Reference         = std::shared_ptr<InternalAssignmentContainer>;
            using ReadOnlyReference = std::shared_ptr<const InternalAssignmentContainer>;

            enum AssignmentType {
                SubAssignmentGeneration,
                InversionOfSubAssignment
            };
            
            AssignmentType               assignmentType;
            std::size_t                  id;
            std::size_t                  idOfReferencedAssignment;
            std::optional<std::size_t>   idOfAssignmentFromWhichAssignedToSignalPartsWhereInherited;
            std::optional<std::size_t>   associatedWithOperationNodeId;
            AssignmentReferenceVariant   data;
            OrderedAssignmentIdContainer dataDependencies;

            explicit InternalAssignmentContainer(AssignmentType assignmentType, std::size_t id):
                assignmentType(assignmentType), id(id), idOfReferencedAssignment(0), associatedWithOperationNodeId(0) {}

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
                assignment.associatedWithOperationNodeId                              = associatedWithOperationNodeId;
                assignment.data                                                       = data;
                assignment.dataDependencies                                           = dataDependencies;
                return assignment;
            }

            [[nodiscard]] std::shared_ptr<syrec::VariableAccess> getAssignedToSignalParts() const;
        };

        OrderedAssignmentIdContainer                                                  activeAssignmentsLookupById;
        std::map<std::size_t, InternalAssignmentContainer::Reference, std::greater<>> internalAssignmentContainer;
        std::vector<std::size_t>                                                      cutOffIndicesForInvertibleAssignments;
        std::vector<std::shared_ptr<syrec::AssignStatement>>                          generatedAssignmentsDefiningPermanentSubstitutionsOfOperandsOfOperationNode;
        std::vector<AssignmentReferenceVariant>                                       initializationAssignmentsForGeneratedSubstitutions;

        void                                                                              markAssignmentAsActiveInLookup(std::size_t assigmentId);
        void                                                                              markAssignmentAsInactiveInLookup(std::size_t assignmentId);
        void                                                                              removeActiveAssignmentFromLookup(std::size_t assignmentId);
        [[nodiscard]] std::optional<InternalAssignmentContainer::Reference>               getAssignmentById(std::size_t assignmentId) const;
        [[nodiscard]] std::optional<InternalAssignmentContainer::ReadOnlyReference>       getMatchingActiveSubassignmentById(std::size_t assignmentId) const;
        [[nodiscard]] bool                                                                isAssignmentActive(std::size_t assignmentId) const;
        [[nodiscard]] std::optional<std::size_t>                                          replayNotActiveAssignment(std::size_t assignmentId);

        static void                                                                       invertAssignment(syrec::AssignStatement& assignment);
        static void                                                                       invertAssignment(syrec::UnaryStatement& assignment);
        [[nodiscard]] static std::optional<AssignmentReferenceVariant>                    createCopyOfAssignmentFromInternalContainerEntry(const InternalAssignmentContainer& assignmentToCopy);
        [[nodiscard]] static std::optional<OwningAssignmentReferenceVariant>              createOwningCopyOfAssignment(const syrec::AssignStatement& assignment);
        [[nodiscard]] static std::optional<OwningAssignmentReferenceVariant>              createOwningCopyOfAssignment(const syrec::UnaryStatement& assignment);
        [[nodiscard]] static std::optional<std::vector<OwningAssignmentReferenceVariant>> createOwningCopiesOfAssignments(const std::vector<AssignmentReferenceVariant>& assignments);
        [[nodiscard]] static std::vector<std::reference_wrapper<syrec::VariableAccess>>   getSignalAccessesOfExpression(const syrec::Expression& expression);
        [[nodiscard]] static std::optional<std::shared_ptr<syrec::AssignStatement>>       tryGetActiveAssignmentAsBinaryAssignment(const AssignmentReferenceVariant& assignmentData);
        [[nodiscard]] static std::optional<std::shared_ptr<syrec::UnaryStatement>>        tryGetActiveAssignmentAsUnaryAssignment(const AssignmentReferenceVariant& assignmentData);
        [[nodiscard]] static bool                                                         doSignalAccessesOverlap(const syrec::VariableAccess& first, const syrec::VariableAccess& second, const parser::SymbolTable& symbolTableReference);
    };
} // namespace noAdditionalLineSynthesis



#endif