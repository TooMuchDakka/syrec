#ifndef ASSIGNMENT_WITHOUT_ADDITIONAL_LINES_SIMPLIFIER_HPP
#define ASSIGNMENT_WITHOUT_ADDITIONAL_LINES_SIMPLIFIER_HPP
#pragma once

#include "assignment_transformer.hpp"
#include "expression_to_subassignment_splitter.hpp"
#include "temporary_expressions_container.hpp"
#include "core/syrec/statement.hpp"
#include "core/syrec/parser/parser_config.hpp"
#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/expression_traversal_helper.hpp"
#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/temporary_assignments_container.hpp"

#include <functional>

// TODO: General overhaul of enums to scoped enums if possible (see https://en.cppreference.com/w/cpp/language/enum)
// TODO: Support for unary expressions if SyReC IR support such expressions
namespace noAdditionalLineSynthesis {
    class AssignmentWithoutAdditionalLineSimplifier {
    public:
        struct SimplificationResult {
            syrec::AssignStatement::vec generatedAssignments;
        };
        using SignalValueLookupCallback = std::function<std::optional<unsigned int>(const syrec::VariableAccess&)>;
        using SimplificationResultReference = std::unique_ptr<SimplificationResult>;
        [[nodiscard]] SimplificationResultReference simplify(const syrec::AssignStatement& assignmentStatement, const SignalValueLookupCallback& signalValueLookupCallback);

         virtual ~AssignmentWithoutAdditionalLineSimplifier() = default;
        AssignmentWithoutAdditionalLineSimplifier(const parser::SymbolTable::ptr& symbolTableReference, const std::optional<parser::NoAdditionalLineSynthesisConfig>& config) {
            generatedAssignmentsContainer                     = std::make_shared<TemporaryAssignmentsContainer>();
            temporaryExpressionsContainer                     = std::make_unique<TemporaryExpressionsContainer>(symbolTableReference);
            expressionTraversalHelper                         = std::make_shared<ExpressionTraversalHelper>();
            this->symbolTableReference                        = symbolTableReference;
            expressionToSubAssignmentSplitterReference        = std::make_unique<ExpressionToSubAssignmentSplitter>();
            learnedConflictsLookup                            = std::make_unique<LearnedConflictsLookup>();
            disabledValueLookupToggle                         = false;
            assignmentTransformer                             = std::make_unique<AssignmentTransformer>();
            if (config.has_value()) {
                internalConfig = *config;
            }
        }

    protected:
        struct Decision {
            enum class ChoosenOperand :unsigned char {
                Left  = 0x1,
                Right = 0x2,
                None = 0x3
            };

            std::size_t            operationNodeId;
            ChoosenOperand         choosenOperand;
            std::size_t            numExistingAssignmentsPriorToDecision;
            syrec::expression::ptr backupOfExpressionRequiringFixup;
        };
        using DecisionReference = std::shared_ptr<Decision>;
        std::vector<DecisionReference>         pastDecisions;
        TemporaryAssignmentsContainer::ptr     generatedAssignmentsContainer;
        TemporaryExpressionsContainer::ptr     temporaryExpressionsContainer;
        ExpressionTraversalHelper::ptr         expressionTraversalHelper;
        parser::SymbolTable::ptr               symbolTableReference;
        ExpressionToSubAssignmentSplitter::ptr expressionToSubAssignmentSplitterReference;
        bool                                   disabledValueLookupToggle;
        AssignmentTransformer::ptr             assignmentTransformer;
        parser::NoAdditionalLineSynthesisConfig internalConfig;
        
        using LearnedConflictsLookupKey = std::pair<std::size_t, Decision::ChoosenOperand>;
        struct LearnedConflictsLookupKeyHasher {
            std::size_t operator()(const LearnedConflictsLookupKey& key) const {
                return std::hash<std::size_t>()(key.first) ^ std::hash<Decision::ChoosenOperand>()(key.second);
            }
        };

        // Since the std::unordered_set does not work with std::pair out of the box, we need to provide a custom hasher function
        using LearnedConflictsLookup = std::unordered_set<LearnedConflictsLookupKey, LearnedConflictsLookupKeyHasher>;
        using LearnedConflictsLookupReference = std::unique_ptr<LearnedConflictsLookup>;
        LearnedConflictsLookupReference learnedConflictsLookup;

        std::optional<std::size_t> operationNodeCausingConflictAndBacktrack;
        class OperationOperandSimplificationResult {
        public:
            [[nodiscard]] std::optional<syrec::VariableAccess::ptr> getAssignedToSignalOfAssignment() const {
                return std::holds_alternative<syrec::VariableAccess::ptr>(data) ? std::make_optional(std::get<syrec::VariableAccess::ptr>(data)) : std::nullopt;
            }

            [[nodiscard]] std::optional<syrec::expression::ptr> getGeneratedExpr() const {
                return std::holds_alternative<syrec::expression::ptr>(data) ? std::make_optional(std::get<syrec::expression::ptr>(data)) : std::nullopt;
            }

            [[nodiscard]] std::size_t getNumberOfCreatedAssignments() const {
                return numGeneratedAssignments;
            }

            [[nodiscard]] bool wasResultManuallyCreated() const {
                return wasManuallyCreated;
            }

            [[nodiscard]] static OperationOperandSimplificationResult createManuallyFrom(const std::variant<syrec::VariableAccess::ptr, syrec::expression::ptr>& data) {
                return OperationOperandSimplificationResult(0, data, true);
            }

            explicit OperationOperandSimplificationResult(std::size_t numGeneratedAssignments, std::variant<syrec::VariableAccess::ptr, syrec::expression::ptr> data, bool wasManuallyCreated = false)
                : numGeneratedAssignments(numGeneratedAssignments), data(std::move(data)), wasManuallyCreated(wasManuallyCreated) {}
        private:
            std::size_t                                                      numGeneratedAssignments;
            std::variant<syrec::VariableAccess::ptr, syrec::expression::ptr> data;
            bool                                                             wasManuallyCreated;
        };
        using OwningOperationOperandSimplificationResultReference = std::unique_ptr<OperationOperandSimplificationResult>;

        [[nodiscard]] std::optional<OwningOperationOperandSimplificationResultReference> handleOperationNode(const ExpressionTraversalHelper::OperationNodeReference& operationNode, const SignalValueLookupCallback& signalValueLookupCallback);
        [[nodiscard]] std::optional<OwningOperationOperandSimplificationResultReference> handleOperationNodeWithNoLeafNodes(const ExpressionTraversalHelper::OperationNodeReference& operationNode, const SignalValueLookupCallback& signalValueLookupCallback);
        [[nodiscard]] std::optional<OwningOperationOperandSimplificationResultReference> handleOperationNodeWithOneLeafNode(const ExpressionTraversalHelper::OperationNodeReference& operationNode, const SignalValueLookupCallback& signalValueLookupCallback);
        [[nodiscard]] std::optional<OwningOperationOperandSimplificationResultReference> handleOperationNodeWithOnlyLeafNodes(const ExpressionTraversalHelper::OperationNodeReference& operationNode);

        // This function should probably be refactored since the usage of a signal access can only determine a conflict and not the choice when trying to create an assignments
        // Only active assignments contribute to the set of potential conflicts.
        // Determining whether a conflict exists would happen when parsing a signal access used in an expression by checking whether an active assignment for any of accessed signal parts exists.
        // If so rollback to the conflicting decision and try again now excluding the previous choice (or creating an replacement for it [i.e. the choice for the assignment if the size of the replacement can be determined])

        // Backtrack to last overlapping decision, add additional field backtrackingToNode to determine whether a conflict or a backtrack is in progress when std::nullopt is returned during handling of operation node
        // Add lookup for learned "clauses" i.e. previous conflicts that can be erased during backtracking
        [[nodiscard]] bool                             doesAssignmentToSignalLeadToConflict(const syrec::VariableAccess& assignedToSignal) const;
        [[nodiscard]] std::optional<DecisionReference> tryGetDecisionForOperationNode(const std::size_t& operationNodeId) const;
        
        [[nodiscard]] DecisionReference           makeDecision(const ExpressionTraversalHelper::OperationNodeReference& operationNode, const std::pair<std::reference_wrapper<const OperationOperandSimplificationResult>, std::reference_wrapper<const OperationOperandSimplificationResult>>& potentialChoices);
        [[nodiscard]] std::optional<std::size_t>  determineOperationNodeIdCausingConflict(const syrec::VariableAccess& choiceOfAssignedToSignalTriggeringSearchForCauseOfConflict) const;
        [[nodiscard]] bool                        isOperationNodeSourceOfConflict(std::size_t operationNodeId) const;
        void                                      markOperationNodeAsSourceOfConflict(std::size_t operationNodeId);
        void                                      markSourceOfConflictReached();
        [[nodiscard]] bool                        shouldBacktrackDueToConflict() const;

        void                                      considerExpressionInFutureDecisions(const syrec::expression::ptr& expr) const;
        void                                      revokeConsiderationOfExpressionForFutureDecisions(const syrec::expression::ptr& expr) const;
        [[nodiscard]] bool                        isChoiceOfSignalAccessBlockedByAnyActiveExpression(const syrec::VariableAccess& chosenOperand) const;

        // This call should be responsible to determine conflicts, during a decision their should not arise any conflicts
        // The check for a conflict should take place in the operations nodes with either one or two leaf nodes by using this call before making any decisions
        // If a conflict is detected, the first decision starting from the initial one shall be our backtrack destination and all overlapping assignments for the found first decision
        // shall be recorded as learned conflicts.
        [[nodiscard]] bool                                                                                                 wereAccessedSignalPartsModifiedByActiveAssignment(const syrec::VariableAccess& accessedSignalParts) const;
        [[nodiscard]] std::vector<DecisionReference>                                                                       determineDecisionsOverlappingAccessedSignalPartsOmittingAlreadyRecordedOnes(const syrec::VariableAccess& accessedSignalParts) const;
        void                                                                                                               handleConflict(std::size_t associatedOperationNodeIdOfAccessedSignalPartsOperand, const syrec::VariableAccess& accessedSignalPartsUsedInCheckForConflict);
        [[nodiscard]] std::size_t                                                                                          determineEarliestSharedParentOperationNodeIdBetweenCurrentAndConflictOperationNodeId(std::size_t operationNodeIdOfEarliestSourceOfConflict, std::size_t operationNodeIdWhereConflictWasDetected) const;

        /**
         * \brief Determine whether another choice for a previously made decision could be made. This calls excludes the last made decision under the assumption that this call is made after a conflict was derived at the current operation node
         * \return Whether another choice at any previous decision could be made
         */
        [[nodiscard]] bool                             couldAnotherChoiceBeMadeAtPreviousDecision(const std::optional<std::size_t>& pastDecisionForOperationNodeWithIdToExclude) const;
        
        void                                                                                             removeDecisionFor(std::size_t operationNodeId);
        [[nodiscard]] std::optional<DecisionReference>                                                   tryGetLastDecision() const;
        [[nodiscard]] std::optional<DecisionReference>                                                   tryGetSecondToLastDecision() const;
        virtual void                                                                                     resetInternals();
        [[nodiscard]] virtual std::unique_ptr<syrec::AssignStatement>                                    transformAssignmentPriorToSimplification(const syrec::AssignStatement& assignmentToSimplify, bool applyHeuristicsForSubassignmentGeneration) const;
        void                                                                                             transformExpressionPriorToSimplification(syrec::expression& expr, bool applyHeuristicsForSubassignmentGeneration) const;
        void                                                                                             rememberConflict(std::size_t operationNodeId, Decision::ChoosenOperand chosenOperandAtOperationNode) const;
        [[nodiscard]] bool                                                                               didPreviousDecisionMatchingChoiceCauseConflict(const LearnedConflictsLookupKey& lookupKeyRepresentingSearchedForPreviousDecision) const;
        void                                                                                             disableValueLookup();
        void                                                                                             enabledValueLookup();
        void                                                                                             backtrack(std::size_t operationNodeIdAtOriginOfBacktrack) const;
        [[nodiscard]] bool                                                                               canAlternativeByChosenInOperationNode(std::size_t operationNodeId, Decision::ChoosenOperand toBeChosenAlternative, const syrec::VariableAccess& signalAccess, const syrec::expression& alternativeToCheckAsExpr, const parser::SymbolTable& symbolTable) const;
        [[nodiscard]] bool                                                                               canAlternativeByChosenInOperationNode(std::size_t operationNodeId, Decision::ChoosenOperand toBeChosenAlternative, const syrec::VariableAccess& signalAccess, const syrec::VariableAccess& alternativeToCheckAsSignalAccess, const parser::SymbolTable& symbolTable) const;
        [[nodiscard]] bool                                                                               isAccessedSignalAssignable(const std::string_view& accessedSignalIdent) const;
        [[nodiscard]] std::optional<double>                                                              determineCostOfExpr(const syrec::expression& expr, std::size_t currentNestingLevel) const;
        [[nodiscard]] std::optional<double>                                                              determineCostOfAssignment(const syrec::AssignStatement& assignment) const;
        [[nodiscard]] std::optional<double>                                                              determineCostOfNumber(const syrec::Number& number, std::size_t currentNestingLevel) const;
        [[nodiscard]] std::optional<double>                                                              determineCostOfAssignments(const syrec::AssignStatement::vec& assignments) const;
        [[nodiscard]] syrec::AssignStatement::vec                                                        determineMostViableAlternativeBasedOnCost(const syrec::AssignStatement::vec& generatedSimplifiedAssignments, const std::shared_ptr<syrec::AssignStatement>& originalAssignmentUnoptimized, const SignalValueLookupCallback& signalValueCallback) const;

        [[nodiscard]] static syrec::expression::ptr                      fuseExpressions(const syrec::expression::ptr& lhsOperand, syrec_operation::operation op, const syrec::expression::ptr& rhsOperand);
        [[nodiscard]] static std::optional<syrec::AssignStatement::ptr>  tryCreateAssignmentForOperationNode(const syrec::VariableAccess::ptr& assignmentLhs, syrec_operation::operation op, const syrec::expression::ptr& assignmentRhs);
        [[nodiscard]] static syrec::expression::ptr                      createExpressionFrom(const syrec::expression::ptr& lhsOperand, syrec_operation::operation op, const syrec::expression::ptr& rhsOperand);
        [[nodiscard]] static syrec::expression::ptr                      createExpressionFromOperationNode(const ExpressionTraversalHelper::ptr& expressionTraversalHelper, const ExpressionTraversalHelper::OperationNodeReference& operationNode);
        [[nodiscard]] static syrec::expression::ptr                      createExpressionFromOperandSimplificationResult(const OperationOperandSimplificationResult& operandSimplificationResult);
        [[nodiscard]] static bool                                        doesExpressionDefineNestedSplitableExpr(const syrec::expression& expr);
        [[nodiscard]] static bool                                        doesExpressionDefineNumber(const syrec::expression& expr);
        [[nodiscard]] static bool                                        doesExprDefineSignalAccess(const syrec::expression& expr);
        [[nodiscard]] static std::optional<syrec::AssignStatement::ptr>  tryCreateAssignmentFromOperands(Decision::ChoosenOperand chosenOperandAsAssignedToSignal, const OperationOperandSimplificationResult& simplificationResultOfFirstOperand, syrec_operation::operation operationNodeOperation, const OperationOperandSimplificationResult& simplificationResultOfSecondOperand);
        [[nodiscard]] static std::optional<syrec::expression::ptr>       tryCreateExpressionFromOperationNodeOperandSimplifications(const OperationOperandSimplificationResult& simplificationResultOfFirstOperand, syrec_operation::operation operationNodeOperation, const OperationOperandSimplificationResult& simplificationResultOfSecondOperand);
        [[nodiscard]] static bool                                        areAssignedToSignalPartsZero(const syrec::VariableAccess& accessedSignalParts, const SignalValueLookupCallback& signalValueLookupCallback);
        [[nodiscard]] static bool                                        doesExprOnlyDefineReversibleOperationsAndNoBitwiseXorOperationWithNoLeafNodes(const syrec::expression& expr);
        static void                                                      tryConvertNumericToBinaryExpr(syrec::expression::ptr& expr);
        [[nodiscard]] static std::optional<syrec::BinaryExpression::ptr> convertNumericExprToBinary(const syrec::NumericExpression& numericExpr);
        [[nodiscard]] static std::optional<syrec_operation::operation>   tryMapCompileTimeConstantExprOperationToBinaryOperation(syrec::Number::CompileTimeConstantExpression::Operation operation);
        [[nodiscard]] static syrec::expression::ptr                      convertNumberToExpr(const syrec::Number::ptr& number, unsigned int expectedBitwidth);
        [[nodiscard]] static std::optional<syrec::BinaryExpression::ptr> convertCompileTimeConstantExprToBinaryExpr(const syrec::Number::CompileTimeConstantExpression& compileTimeConstantExpr, unsigned int expectedBitwidth);
        [[nodiscard]] static syrec::expression::ptr                      transformExprBeforeProcessing(const syrec::expression::ptr& initialExpr);
        
        [[nodiscard]] static bool                                        doesExprContainOverlappingAccessOnGivenSignalAccess(const syrec::expression& expr, const syrec::VariableAccess& signalAccess, const parser::SymbolTable& symbolTable);
        [[nodiscard]] static bool                                        doesNumberContainOverlappingAccessOnGivenSignalAccess(const syrec::Number& number, const syrec::VariableAccess& signalAccess, const parser::SymbolTable& symbolTable);
        [[nodiscard]] static bool                                        doSignalAccessesOverlap(const syrec::VariableAccess& firstSignalAccess, const syrec::VariableAccess& otherSignalAccess, const parser::SymbolTable& symbolTable);

        
        [[nodiscard]] static constexpr bool shouldLogMessageBePrinted();
        [[nodiscard]] static std::string    stringifyChosenOperandForLogMessage(Decision::ChoosenOperand chosenOperand);
        static void                         logDecision(const DecisionReference& madeDecision);
        static void                         logConflict(std::size_t operationNodeId, Decision::ChoosenOperand operandCausingConflict, const syrec::VariableAccess& signalAccessCausingConflict, const std::optional<std::size_t>& idOfEarliestDecisionInvolvedInConflict);
        static void                         logStartOfProcessingOfOperationNode(std::size_t operationNodeId);
        static void                         logEndOfProcessingOfOperationNode(std::size_t operationNodeId);
        static void                         logBacktrackingResult(std::size_t operationNodeIdAtStartOfBacktracking, std::size_t nextOperationNodeAfterBacktrackingFinished);
        static void                         logMarkingOfOperationNodeAsSourceOfConflict(std::size_t operationNodeId);
        static void                         logMessage(const std::string& msg);
    };
}

#endif