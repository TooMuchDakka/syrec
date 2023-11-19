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
        void invertAllAssignmentsUpToLastCutoff(bool excludeLastGeneratedAssignment);
        void popLastCutoffForInvertibleAssignments();
        void rollbackLastXAssignments(std::size_t numberOfAssignmentsToRemove);
        void rollbackAssignmentsMadeSinceLastCutoffAndPopCutoff();
        void resetInternals();

        [[nodiscard]] syrec::AssignStatement::vec getAssignments() const;
        [[nodiscard]] bool                        existsOverlappingAssignmentFor(const syrec::VariableAccess& assignedToSignal, const parser::SymbolTable& symbolTable) const;
        [[nodiscard]] std::size_t                 getNumberOfAssignments() const;
    protected:
        std::vector<std::size_t>                                                        cutOffIndicesForInvertibleAssignments;
        std::vector<std::size_t>                                                        indicesOfActiveAssignments;
        std::vector<syrec::AssignStatement::ptr>                                        generatedAssignments;
        std::unordered_map<std::string, std::unordered_set<syrec::VariableAccess::ptr>> activeAssignmentsLookup;

        void                                                             invertAssignmentAndStoreAndMarkOriginalAsInactive(const syrec::AssignStatement& assignmentToInvert);
        [[nodiscard]] static syrec::AssignStatement::ptr                 invertAssignment(const syrec::AssignStatement& assignment);
        [[nodiscard]] std::vector<std::size_t>                           determineIndicesOfInvertibleAssignmentsStartingFrom(std::size_t firstRelevantAssignmentIndex) const;
        void                                                             removeActiveAssignmentFromLookup(const syrec::VariableAccess::ptr& assignedToSignal);
    };
}

#endif