#ifndef BASE_ASSIGNMENT_SIMPLIFIER_HPP
#define BASE_ASSIGNMENT_SIMPLIFIER_HPP
#pragma once

#include "core/syrec/statement.hpp"
#include "core/syrec/parser/symbol_table.hpp"

// TODO: Handling of compile time constant expressions in both simplifiers as well as traversal helpers
namespace noAdditionalLineSynthesis {
    class BaseAssignmentSimplifier {
    public:
        // TODO: We can only perform the simplification of a += ... to a ^= ... iff a = 0 prior to the assignment (if value propagation is not blocked by data flow analysis [thus an additional parameter is required])
        // We are assuming that the given assignment statement does conform to the grammar and all semantic checks are valid
        [[nodiscard]] syrec::Statement::vec simplify(const syrec::AssignStatement::ptr& assignmentStmt, bool isValueOfAssignedToSignalBlockedByDataFlowAnalysis);

        explicit BaseAssignmentSimplifier(parser::SymbolTable::ptr symbolTable):
            symbolTable(std::move(symbolTable)) {}

        virtual ~BaseAssignmentSimplifier() = 0;
    protected:
        using RestrictionMap = std::map<std::string, std::vector<syrec::VariableAccess::ptr>>;
        /*
         * TODO: Use this additional data structure to check whether the value of a signal is zero (beside the symbol table lookup) to enable optimizations for subexpressions that are currently not possible
         * due to the assumption that signals are not zero inside any of the subexpressions (this lookup map should also only store signals that were actually zero before any assignment) to reduce the amount of required storage
         * and prevent the need to actually modify the entries in the symbol table for these checks
         */
        using LocallyDisabledSignalMap = std::map<std::string, std::vector<syrec::VariableAccess::ptr>>;
        using LocallyDisabledSignalMapReference = std::unique_ptr<LocallyDisabledSignalMap>;
        const parser::SymbolTable::ptr symbolTable;
        LocallyDisabledSignalMapReference locallyDisabledSignalWithValueOfZeroMap;

        [[nodiscard]] static bool                        isExprConstantNumber(const syrec::Expression::ptr& expr);
        [[nodiscard]] static bool                        doesExprDefineVariableAccess(const syrec::Expression::ptr& expr);
        [[nodiscard]] static bool                        doesExprDefineNestedExpr(const syrec::Expression::ptr& expr);
        [[nodiscard]] static bool                        doesExprOnlyContainReversibleOperations(const syrec::Expression::ptr& expr);
        [[nodiscard]] static syrec::Statement::vec       invertAssignmentsButIgnoreSome(const syrec::Statement::vec& assignmentsToInvert, std::size_t numStatementsToIgnoreStartingFromLastOne);
        
        [[nodiscard]] bool wasAccessOnSignalAlreadyDefined(const syrec::VariableAccess::ptr& accessedSignalParts, const RestrictionMap& notUsableSignals) const;
        [[nodiscard]] bool wasAnyAccessedSignalPartPreviouslyNotAccessed(const syrec::VariableAccess::ptr& accessedSignalParts, const RestrictionMap& previouslyAccessedSignalPartsLookup) const;
        
        // TODO: These two functions should probably be renamed to doesExprOnlyContainUniqueSignalAccesses
        /** @brief Check that every lhs Operand of any binary expression is only defined once on every level of the ast. <br>
        * Additionally perform required transformations of the form (2 + a) <=> (a + 2) if possible
        * @returns Whether the given condition was true, is std::nullopt is returned the given binary expression cannot be simplified
        */
        [[nodiscard]] std::optional<bool> isEveryLhsOperandOfAnyBinaryExprDefinedOnceOnEveryLevelOfTheAST(const syrec::BinaryExpression::ptr& rootExpr) const;
        [[nodiscard]] std::optional<bool> isEveryLhsOperandOfAnyBinaryExprDefinedOnceOnEveryLevelOfTheAST(const syrec::BinaryExpression::ptr& exprToCheck, bool isRootExpr, RestrictionMap& notUsableSignals) const;
        [[nodiscard]] std::optional<bool> doesExprOnlyContainUniqueSignalAccesses(const syrec::Expression::ptr&, RestrictionMap& alreadyDefinedSignalAccesses);

        // TODO: Define new function that every lhs operand of any binary expr is defined once on every lhs of any expr (how do we deal with switches correctly ?)

        [[nodiscard]] std::optional<bool> isValueOfAssignedToSignalZero(const syrec::VariableAccess::ptr& assignedToSignal) const;
        void                              invalidateSignalWithPreviousValueOfZero(const syrec::VariableAccess::ptr& assignedToSignal) const;
        void                              markSignalAccessAsNotUsableInExpr(const syrec::VariableAccess::ptr& accessedSignalParts, RestrictionMap& notUsableSignals) const;

        [[nodiscard]] virtual syrec::Statement::vec simplifyWithoutPreconditionCheck(const syrec::AssignStatement::ptr& assignmentStmt, bool isValueOfAssignedToSignalBlockedByDataFlowAnalysis) = 0;
        [[nodiscard]] virtual bool                  simplificationPrecondition(const syrec::AssignStatement::ptr& assignmentStmt)       = 0;
        virtual void                                resetInternals();
    };
}

#endif