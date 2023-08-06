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
        [[nodiscard]] syrec::Statement::vec simplify(const syrec::AssignStatement::ptr& assignmentStmt);

        explicit BaseAssignmentSimplifier(parser::SymbolTable::ptr symbolTable):
            symbolTable(std::move(symbolTable)) {}

        virtual ~BaseAssignmentSimplifier() = 0;
    protected:
        using RestrictionMap = std::map<std::string, std::vector<syrec::VariableAccess::ptr>>;
        const parser::SymbolTable::ptr symbolTable;

        [[nodiscard]] static bool                        isExprConstantNumber(const syrec::expression::ptr& expr);
        [[nodiscard]] static bool                        doesExprDefineVariableAccess(const syrec::expression::ptr& expr);
        [[nodiscard]] static bool                        doesExprDefineNestedExpr(const syrec::expression::ptr& expr);
        [[nodiscard]] static std::optional<unsigned int> tryFetchValueOfNumber(const syrec::Number::ptr& number);
        [[nodiscard]] static std::optional<unsigned int> tryFetchValueOfExpr(const syrec::expression::ptr& expr);
        [[nodiscard]] static syrec::Statement::vec       invertAssignmentsButIgnoreSome(const syrec::Statement::vec& assignmentsToInvert, std::size_t numStatementsToIgnoreStartingFromLastOne);

        [[nodiscard]] bool                                                                    doAccessedBitRangesOverlap(const syrec::VariableAccess::ptr& accessedSignalParts, const syrec::VariableAccess::ptr& potentiallyEnclosingSignalAccess, bool shouldAccessedBitRangeBeFullyEnclosed) const;
        [[nodiscard]] bool                                                                    doAccessedSignalPartsOverlap(const syrec::VariableAccess::ptr& accessedSignalPartsOfLhs, const syrec::VariableAccess::ptr& accessedSignalPartsOfRhs) const;
        [[nodiscard]] bool                                                                    wasAccessOnSignalAlreadyDefined(const syrec::VariableAccess::ptr& accessedSignalParts, const RestrictionMap& notUsableSignals) const;
        [[nodiscard]] bool                                                                    wasAnyAccessedSignalPartPreviouslyNotAccessed(const syrec::VariableAccess::ptr& accessedSignalParts, const RestrictionMap& previouslyAccessedSignalPartsLookup) const;
        [[nodiscard]] std::optional<unsigned int>                                             fetchBitWidthOfSignal(const std::string_view& signalIdent) const;
        [[nodiscard]] std::optional<syrec::Variable::ptr>                                     fetchSymbolTableEntryForSignalAccess(const std::string_view& signalIdent) const;
        [[nodiscard]] std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess> transformUserDefinedBitRangeAccess(const syrec::VariableAccess::ptr& accessedSignalParts) const;

        /** @brief Check that every lhs Operand of any binary expression is only defined once on every level of the ast. <br>
        * Additionally perform required transformations of the form (2 + a) <=> (a + 2) if possible
        * @returns Whether the given condition was true, is std::nullopt is returned the given binary expression cannot be simplified
        */
        [[nodiscard]] std::optional<bool> isEveryLhsOperandOfAnyBinaryExprDefinedOnceOnEveryLevelOfTheAST(const syrec::BinaryExpression::ptr& rootExpr) const;
        [[nodiscard]] std::optional<bool> isEveryLhsOperandOfAnyBinaryExprDefinedOnceOnEveryLevelOfTheAST(const syrec::BinaryExpression::ptr& expr, bool isRootExpr, RestrictionMap& notUsableSignals) const;
        void                              markSignalAccessAsNotUsableInExpr(const syrec::VariableAccess::ptr& accessedSignalParts, RestrictionMap& notUsableSignals) const;

        [[nodiscard]] virtual syrec::Statement::vec simplifyWithoutPreconditionCheck(const syrec::AssignStatement::ptr& assignmentStmt) = 0;
        [[nodiscard]] virtual bool                  simplificationPrecondition(const syrec::AssignStatement::ptr& assignmentStmt)       = 0;
        virtual void                                resetInternals();
    };
}

#endif