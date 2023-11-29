#ifndef TEMPORARY_ASSIGNMENT_CONTAINER_HPP
#define TEMPORARY_ASSIGNMENT_CONTAINER_HPP
#pragma once

#include "core/syrec/statement.hpp"
#include "core/syrec/parser/symbol_table.hpp"

#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace noAdditionalLineSynthesis {
    class TemporaryAssignmentsContainer {
    public:
        using ptr = std::shared_ptr<TemporaryAssignmentsContainer>;

        void storeActiveAssignment(const syrec::AssignStatement::ptr& assignment);
        void markCutoffForInvertibleAssignments();
        /**
         * \brief Invert all assignments starting from the last generated one up to the last marked cutoff position and optionally exclude a fixed number of assignments or all matching a given active assignment
         * \param numberOfAssignmentToExcludeFromInversionStartingFromLastGeneratedOne The number of assignment to not invert starting from the last active one
         * \param optionalExcludedFromInversionAssignmentsFilter An optional filter defining an active assignment which with all of its sub-assignments should be kept active
         */
        void invertAllAssignmentsUpToLastCutoff(std::size_t numberOfAssignmentToExcludeFromInversionStartingFromLastGeneratedOne, const syrec::VariableAccess::ptr* optionalExcludedFromInversionAssignmentsFilter);
        void popLastCutoffForInvertibleAssignments();
        void rollbackLastXAssignments(std::size_t numberOfAssignmentsToRemove);
        void rollbackAssignmentsMadeSinceLastCutoffAndOptionallyPopCutoff(bool popCutOff);
        void resetInternals();

        [[nodiscard]] syrec::AssignStatement::vec getAssignments() const;
        [[nodiscard]] bool                        existsOverlappingAssignmentFor(const syrec::VariableAccess& assignedToSignal, const parser::SymbolTable& symbolTable) const;
        [[nodiscard]] std::size_t                 getNumberOfAssignments() const;
    protected:
        std::vector<std::size_t>                                                        cutOffIndicesForInvertibleAssignments;
        std::vector<std::size_t>                                                        indicesOfActiveAssignments;
        std::vector<syrec::AssignStatement::ptr>                                        generatedAssignments;
        std::unordered_map<std::string, std::unordered_set<syrec::VariableAccess::ptr>> activeAssignmentsLookup;

        void                                                             invertAssignmentAndStoreAndMarkOriginalAsInactive(std::size_t indexOfAssignmentToInvert);
        [[nodiscard]] static syrec::AssignStatement::ptr                 invertAssignment(const syrec::AssignStatement& assignment);
        [[nodiscard]] std::vector<std::size_t>                           determineIndicesOfInvertibleAssignmentsStartingFrom(std::size_t firstRelevantAssignmentIndex) const;
        void                                                             removeActiveAssignmentFromLookup(const syrec::VariableAccess::ptr& assignedToSignal);
    };
}

#endif