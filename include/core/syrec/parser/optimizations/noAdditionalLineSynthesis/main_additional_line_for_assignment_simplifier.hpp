#ifndef MAIN_ADDITIONAL_LINE_FOR_ASSIGNMENT_SIMPLIFIER_HPP
#define MAIN_ADDITIONAL_LINE_FOR_ASSIGNMENT_SIMPLIFIER_HPP
#pragma once

#include "core/syrec/statement.hpp"
#include "core/syrec/parser/operation.hpp"
#include "core/syrec/parser/symbol_table.hpp"

namespace noAdditionalLineSynthesis {
    class MainAdditionalLineForAssignmentSimplifier {
    public:
        /**
         * \brief Callback used to perform further simplification of the given assignment statement
         * \param assignmentStmt The assignment statement to be simplified
         */
        using FurtherAssignmentStatementSimplificationCallback = void(*)(syrec::AssignStatement::ptr& assignmentStmt);
        /**
         * \brief Callback used to perform the actual assignment
         * \param assignmentStmt The statement defining the assignment to be performed
         * \param symbolTable TODO: REMOVE The symbol table instance that should be updated with the side effect of the assignment statement
         */
        //using AssignmentStatementApplicationCallback = void(*)(const syrec::AssignStatement::ptr& assignmentStmt, const parser::SymbolTable::ptr& symbolTable);
        using AssignmentStatementApplicationCallback = void(*)(const syrec::AssignStatement::ptr& assignmentStmt);

        struct NotUsableAsReplacementSignalParts {
            syrec::VariableAccess::ptr blockedSignalParts;

            explicit NotUsableAsReplacementSignalParts(syrec::VariableAccess::ptr& blockedSignalParts):
                blockedSignalParts(std::move(blockedSignalParts)) {}
        };        
        using LookupOfExcludedSignalsForReplacement = std::unique_ptr<std::map<std::string_view, std::vector<NotUsableAsReplacementSignalParts>>>;

        explicit MainAdditionalLineForAssignmentSimplifier(parser::SymbolTable::ptr symbolTable, FurtherAssignmentStatementSimplificationCallback assignmentStatementSimplificationCallback, AssignmentStatementApplicationCallback assignmentApplicationCallback)
            : symbolTable(std::move(symbolTable)), furtherAssignmentSimplificationCallback(assignmentStatementSimplificationCallback), assignmentApplicationCallback(assignmentApplicationCallback) {}

        [[nodiscard]] syrec::AssignStatement::vec tryReduceRequiredAdditionalLinesFor(const syrec::AssignStatement::ptr& assignmentStmt) const;

    private:
        using TransformedDimensionAccess = std::vector<std::optional<unsigned int>>;

        struct SignalSubstitution {
            syrec::VariableAccess::ptr toBeSubstitutedSignalAccess;
            syrec::VariableAccess::ptr replacement;
        };

        const parser::SymbolTable::ptr                                             symbolTable;
        const FurtherAssignmentStatementSimplificationCallback                     furtherAssignmentSimplificationCallback;
        const AssignmentStatementApplicationCallback                               assignmentApplicationCallback;

        std::map<std::string_view, std::vector<SignalSubstitution>>                activeSignalSubstitutions;
        std::map<std::string_view, std::vector<NotUsableAsReplacementSignalParts>> activeAssignments;

        [[nodiscard]] static bool isExpressionEitherBinaryOrShiftExpression(const syrec::expression::ptr& expr);
        /**
         * \brief Precondition for transformation rule R1
         * \param expr The expression to check
         * \return Whether the expression only contained reversible assignment operations
         */
        [[nodiscard]] static bool                                      doesExpressionOnlyContainReversibleOperations(const syrec::expression::ptr& expr);
        [[nodiscard]] static bool                                      doesExpressionDefineSignalAccess(const syrec::expression::ptr& expr);
        [[nodiscard]] static bool                                      isExpressionConstantNumber(const syrec::expression::ptr& expr);
        [[nodiscard]] static bool                                      doVariableAccessesOverlap(const syrec::VariableAccess::ptr& signalPartsToCheck, const syrec::VariableAccess::ptr& signalPartsToBeCheckedForOverlap);
        [[nodiscard]] static syrec::Statement::vec                     invertAssignments(const syrec::AssignStatement::vec& assignments);
        [[nodiscard]] static std::optional<unsigned int>               tryEvaluateNumberAsConstant(const syrec::Number::ptr& number);
        [[nodiscard]] static bool                                      doBitRangesOverlap(const optimizations::BitRangeAccessRestriction::BitRangeAccess& thisBitRange, const optimizations::BitRangeAccessRestriction::BitRangeAccess& thatBitRange);
        [[nodiscard]] static bool                                      doDimensionAccessesOverlap(const TransformedDimensionAccess& thisDimensionAccess, const TransformedDimensionAccess& thatDimensionAccess);
        [[nodiscard]] static std::optional<syrec::expression::ptr>     tryConvertCompileTimeConstantExpressionToBinaryExpression(const syrec::Number::ptr& potentialCompileTimeExpression, unsigned int expectedBitWidth, const parser::SymbolTable::ptr& symbolTable);
        [[nodiscard]] static std::optional<syrec_operation::operation> tryMapCompileTimeConstantOperation(const syrec::Number::CompileTimeConstantExpression& compileTimeConstantExpression);
        [[nodiscard]] static std::optional<syrec::expression::ptr>     tryMapNumberToExpression(const syrec::Number::ptr& number, unsigned int expectedBitWidth, const parser::SymbolTable::ptr& symbolTable);
        [[nodiscard]] static std::optional<syrec::expression::ptr>     tryPerformReorderingOfSubexpressions(const syrec::expression::ptr& expr);

        struct VariableAccessCountPair {
            std::size_t                accessCount;
            syrec::VariableAccess::ptr accessedSignalParts;

            explicit VariableAccessCountPair(std::size_t accessCount, syrec::VariableAccess::ptr accessedSignalParts)
                :accessCount(accessCount), accessedSignalParts(std::move(accessedSignalParts)) {}
        };

        struct VariableAccessCountLookup {
            std::map<std::string, std::vector<VariableAccessCountPair>> lookup;
        };

        struct EstimatedSignalAccessSize {
            std::vector<unsigned int> dimensions;
            unsigned int              bitWidth;

            explicit EstimatedSignalAccessSize(std::vector<unsigned int> dimensions, unsigned int bitWidth):
                dimensions(std::move(dimensions)), bitWidth(bitWidth) {}

            bool operator==(const EstimatedSignalAccessSize& other) const {
                return dimensions.size() == other.dimensions.size()
                    && std::mismatch(
                        dimensions.cbegin(),
                        dimensions.cend(),
                        other.dimensions.cbegin(),
                        other.dimensions.cend(), 
                        [](unsigned int sizeOfThisDimension, unsigned int sizeOfThatDimension) {
                            return sizeOfThisDimension != sizeOfThatDimension;
                        }).first == dimensions.cend()
                && bitWidth == other.bitWidth;
            }
        };
        
        [[nodiscard]] std::shared_ptr<VariableAccessCountLookup> buildVariableAccessCountsForExpr(const syrec::expression::ptr& expr, const EstimatedSignalAccessSize& requiredSizeForSignalAccessToBeConsidered) const;
        void                                                     buildVariableAccessCountsForExpr(const syrec::expression::ptr& expr, std::shared_ptr<VariableAccessCountLookup>& lookupToFill, const EstimatedSignalAccessSize& requiredSizeForSignalAccessToBeConsidered) const;
        [[nodiscard]] bool                                       doesSignalAccessMatchExpectedSize(const syrec::VariableAccess::ptr& signalAccessToCheck, const EstimatedSignalAccessSize& requiredSizeForSignalAccessToBeConsidered) const;
        [[nodiscard]] EstimatedSignalAccessSize                  getSizeOfSignalAccess(const syrec::VariableAccess::ptr& signalAccess) const;

        /**
         * \brief R2: S += E can be transformed to S ^= E if S = 0
         * \param assignmentStmt The assignment statement to check
         * \return Whether a given add assignment operation can be replaced with a xor assignment
         */
        [[nodiscard]] bool canAddAssignmentBeReplacedWithXorAssignment(const syrec::AssignStatement::ptr& assignmentStmt) const;
        
        /**
         * \brief Precondition for transformation rule <br>
         * R3: S a_op (... (T op E) ...) can be replaced with: <br>
         *  T op E; <br>
         *  S a_op (... T ...); <br>
         *  T op^(-1) E <br>
         * iff op and a_op are both reversible operations
         * \param accessedSignalParts The accessed signal parts to check
         * \param expr The expression to be checked 
         * \return Whether a given expression contains any signal access that overlaps the given accessed signal parts
         */
        //[[nodiscard]] static bool                                  areAccessedSignalPartsOnlyAccessedOnce(const syrec::VariableAccess::ptr& accessedSignalParts, const syrec::expression::ptr& expr);

        [[nodiscard]] std::optional<syrec::expression::ptr>      findExpressionContainingSignalAccessDefinedOnlyOnceInAssignmentRhs(const syrec::BinaryExpression::ptr& assignmentStatement, bool& isLhsRelevantSignalAccess);

        [[nodiscard]] std::vector<unsigned int>                                               transformDimensionAccess(const syrec::VariableAccess::ptr& accessedSignalParts) const;
        [[nodiscard]] std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess> transformBitRangeAccess(unsigned int bitWidthOfAccessedSignal, const syrec::VariableAccess::ptr& accessedSignalParts) const;

        /**
         * \brief Create a lookup of signal accesses which cannot be used as a replacement for another signal of the same size
         * \param expr The expression for which the lookup should be created
         * \return The created lookup containing the currently existing assignments as well as all non overlapping signal accesses of the expression
         */
        [[nodiscard]] LookupOfExcludedSignalsForReplacement     createLookupForSignalsNotUsableAsReplacementsFor(const syrec::expression::ptr& expr) const;
        void                                                    createLookupForSignalsNotUsableAsReplacementsFor(const syrec::expression::ptr& expr, LookupOfExcludedSignalsForReplacement& lookupToFill) const;
        [[nodiscard]] bool                                      doesAssignmentToAccessedSignalPartsAlreadyExists(const syrec::VariableAccess::ptr& accessedSignalParts) const;
        [[nodiscard]] std::optional<syrec::VariableAccess::ptr> tryCreateSubstituteForExpr(const syrec::expression::ptr& expr, const LookupOfExcludedSignalsForReplacement& signalPartsToExcludeAsPotentialReplacements);

        /**
         * \brief R1: S op_a (E_1 op_a E_2) can be replaced with: <br>
         * S op_a E_1;  <br>
         * S op_a E_2  <br>
         * iff op_a = {+, -, ^}  <br>
         * \param assignmentStmt The assignment statement to be simplified
         * \return The created assignments statements if any simplification took place, otherwise an empty vector is returned
         */
        [[nodiscard]] syrec::AssignStatement::vec simplifyAssignmentContainingOnlyReversibleOperations(const syrec::AssignStatement::ptr& assignmentStmt);

        /**
         * \brief Precondition for transformation rule <br>
         * R3: S a_op (... (T op E) ...) can be replaced with: <br>
         *  T op E; <br>
         *  S a_op (... T ...); <br>
         *  T op^(-1) E <br>
         * iff op and a_op are both reversible operations
         * \param assignmentStmt The assignment statement that fulfills the given precondition
         * \return The created assignment statements to perform the substitution
         */
        [[nodiscard]] syrec::AssignStatement::vec simplifyAssignmentBySubstitution(const syrec::AssignStatement::ptr& assignmentStmt);
        
        void activateSignalSubstitutionFor(const syrec::VariableAccess::ptr& signalToBeSubstituted, const syrec::VariableAccess::ptr& substitute) const;
        void deactivateSignalSubstituteFor(const syrec::VariableAccess::ptr& substitute) const;

        void markAccessedSignalPartsAsNotUsableForReplacement(const syrec::VariableAccess::ptr& accessedSignalParts) const;
        void markAccessedSignalPartsAsUsableForReplacement(const syrec::VariableAccess::ptr& accessedSignalParts) const;
    };

}

#endif