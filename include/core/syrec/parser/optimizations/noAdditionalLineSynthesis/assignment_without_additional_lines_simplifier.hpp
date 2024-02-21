#ifndef ASSIGNMENT_WITHOUT_ADDITIONAL_LINES_SIMPLIFIER_HPP
#define ASSIGNMENT_WITHOUT_ADDITIONAL_LINES_SIMPLIFIER_HPP
#pragma once

#include "assignment_transformer.hpp"
#include "expression_substitution_generator.hpp"
#include "expression_to_subassignment_splitter.hpp"
#include "no_additional_line_synthesis_config.hpp"
#include "temporary_expressions_container.hpp"
#include "core/syrec/statement.hpp"
#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/expression_traversal_helper.hpp"
#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/temporary_assignments_container.hpp"

#include <functional>

// TODO: General overhaul of enums to scoped enums if possible (see https://en.cppreference.com/w/cpp/language/enum)
// TODO: Support for unary expressions if SyReC IR support such expressions
namespace noAdditionalLineSynthesis {
    class AssignmentWithoutAdditionalLineSimplifier {
    public:
        struct SimplificationResult {
            using OwningCopyOfAssignment   = std::variant<std::unique_ptr<syrec::AssignStatement>, std::unique_ptr<syrec::UnaryStatement>>;
            using OwningCopiesOfAssignment = std::vector<OwningCopyOfAssignment>;
            using OwningReference = std::unique_ptr<SimplificationResult>;

            syrec::Variable::vec     newlyGeneratedReplacementSignalDefinitions;
            OwningCopiesOfAssignment generatedAssignments;
            OwningCopiesOfAssignment requiredValueResetsForReplacementsTargetingExistingSignals;
            OwningCopiesOfAssignment requiredInversionsOfValuesResetsForReplacementsTargetingExistingSignals;

            [[nodiscard]] static SimplificationResult asEmptyResult() {
                return {};
            }

            [[nodiscard]] static SimplificationResult asSingleStatement(OwningCopyOfAssignment assignment) {
                OwningCopiesOfAssignment containerForAssignments;
                containerForAssignments.reserve(1);
                containerForAssignments.push_back(std::move(assignment));

                auto result = SimplificationResult();
                result.generatedAssignments = std::move(containerForAssignments);
                return result;
            }

            [[nodiscard]] static SimplificationResult asCopyOf(SimplificationResult&& simplificationResult) noexcept {
                return {std::move(simplificationResult)};
            }

            SimplificationResult& operator=(const SimplificationResult& other) = delete;
            SimplificationResult& operator=(SimplificationResult&& other) noexcept {
                newlyGeneratedReplacementSignalDefinitions                              = std::move(other.newlyGeneratedReplacementSignalDefinitions);
                generatedAssignments                                                    = std::move(other.generatedAssignments);
                requiredValueResetsForReplacementsTargetingExistingSignals              = std::move(other.requiredValueResetsForReplacementsTargetingExistingSignals);
                requiredInversionsOfValuesResetsForReplacementsTargetingExistingSignals = std::move(other.requiredInversionsOfValuesResetsForReplacementsTargetingExistingSignals);
                return *this;
            }

            SimplificationResult()                            = default;
            SimplificationResult(SimplificationResult& other) = delete;
            SimplificationResult(SimplificationResult&& other) noexcept :
                newlyGeneratedReplacementSignalDefinitions(std::move(other.newlyGeneratedReplacementSignalDefinitions)),
                generatedAssignments(std::move(other.generatedAssignments)),
                requiredValueResetsForReplacementsTargetingExistingSignals(std::move(other.requiredValueResetsForReplacementsTargetingExistingSignals)),
                requiredInversionsOfValuesResetsForReplacementsTargetingExistingSignals(std::move(other.requiredInversionsOfValuesResetsForReplacementsTargetingExistingSignals))
            {}

            [[nodiscard]] static OwningReference createOwningReferenceOf(SimplificationResult&& simplification) {
                return std::make_unique<SimplificationResult>(std::move(simplification));
            }
        };

        /**
         * \brief A callback used to determine the value of a given signal access returning an empty std::optional if the value is not known or blocked by any previous data flow analysis, otherwise a std::optional<unsigned int> with a value present.
         */
        using SignalValueLookupCallback = std::function<std::optional<unsigned int>(const syrec::VariableAccess&)>;
        [[nodiscard]] SimplificationResult::OwningReference simplify(const syrec::AssignStatement& assignmentStatement, const SignalValueLookupCallback& signalValueLookupCallback);
        void                                                defineSymbolTable(const parser::SymbolTable::ptr& activeSymbolTableScope);
        void                                                reloadGenerateableReplacementSignalName();

         virtual ~AssignmentWithoutAdditionalLineSimplifier() = default;
        AssignmentWithoutAdditionalLineSimplifier(NoAdditionalLineSynthesisConfig config) {
            generatedAssignmentsContainer                     = std::make_shared<TemporaryAssignmentsContainer>();
            temporaryExpressionsContainer                     = std::make_unique<TemporaryExpressionsContainer>();
            expressionTraversalHelper                         = std::make_shared<ExpressionTraversalHelper>();
            expressionToSubAssignmentSplitterReference        = std::make_unique<ExpressionToSubAssignmentSplitter>();

            internalConfig = config;
            if (config.optionalNewReplacementSignalIdentsPrefix.has_value()) {
                substitutionGenerator = std::make_unique<ExpressionSubstitutionGenerator>(config.optionalNewReplacementSignalIdentsPrefix.value());
            }
            assignmentTransformer                             = std::make_unique<AssignmentTransformer>();
            learnedConflictsLookup                            = std::make_unique<LearnedConflictsLookup>();
            disabledValueLookupToggle                         = false;
        }

    protected:
        struct Decision {
            enum class ChoosenOperand :unsigned char {
                Left  = 0x1,
                Right = 0x2,
                None = 0x3
            };

            enum ChoiceRepetition {
                None,
                UntilReset,
                Always
            };

            std::size_t                operationNodeId;
            std::size_t                numExistingAssignmentsPriorToDecision;
            ChoosenOperand             choosenOperand;
            ChoiceRepetition           shouldChoiceBeRepeated;
            syrec::VariableAccess::ptr inheritedOperandDataForChoice;
        };
        using DecisionReference = std::shared_ptr<Decision>;
        std::vector<DecisionReference>          pastDecisions;
        TemporaryAssignmentsContainer::ptr      generatedAssignmentsContainer;
        TemporaryExpressionsContainer::ptr      temporaryExpressionsContainer;
        ExpressionTraversalHelper::ptr          expressionTraversalHelper;
        parser::SymbolTable::ptr                symbolTableReference;
        ExpressionToSubAssignmentSplitter::ptr  expressionToSubAssignmentSplitterReference;
        ExpressionSubstitutionGenerator::ptr    substitutionGenerator;
        bool                                    disabledValueLookupToggle;
        AssignmentTransformer::ptr              assignmentTransformer;
        NoAdditionalLineSynthesisConfig internalConfig;
        std::optional<unsigned int>             determinedBitWidthOfAssignmentToSimplify;
        std::unordered_map<std::size_t, syrec::AssignStatement::ptr> wholeExpressionOfOperationNodeReplacementLookup;
        
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
            using OwningReference = std::unique_ptr<OperationOperandSimplificationResult>;

            [[nodiscard]] std::optional<syrec::VariableAccess::ptr> getAssignedToSignalOfAssignment() const {
                if (data.has_value() && std::holds_alternative<syrec::VariableAccess::ptr>(*data)) {
                    return std::get<syrec::VariableAccess::ptr>(*data);
                }
                return std::nullopt;
            }

            [[nodiscard]] std::optional<syrec::Expression::ptr> getGeneratedExpr() const {
                if (data.has_value() && std::holds_alternative<syrec::Expression::ptr>(*data)) {
                    return std::get<syrec::Expression::ptr>(*data);
                }
                return std::nullopt;
            }

            [[nodiscard]] std::size_t getNumberOfCreatedAssignments() const {
                return numGeneratedAssignments;
            }

            [[nodiscard]] bool wasResultManuallyCreated() const {
                return wasManuallyCreated;
            }

            [[nodiscard]] bool isResultUnknown() const {
                return resultUnknown;
            }

            void updateData(const std::variant<syrec::VariableAccess::ptr, syrec::Expression::ptr>& updatedData) {
                data = updatedData;
            }

            [[nodiscard]] static OperationOperandSimplificationResult createManuallyFrom(const std::variant<syrec::VariableAccess::ptr, syrec::Expression::ptr>& data) noexcept {
                return OperationOperandSimplificationResult(0, data, true);
            }

            [[nodiscard]] static OperationOperandSimplificationResult createAsUnknownResult() noexcept {
                return OperationOperandSimplificationResult(0, std::nullopt, true);
            }

            [[nodiscard]] static OperationOperandSimplificationResult createFrom(std::size_t numGeneratedAssignments, std::variant<syrec::VariableAccess::ptr, syrec::Expression::ptr> data) noexcept {
                return OperationOperandSimplificationResult(numGeneratedAssignments, std::move(data), false);
            }

            [[nodiscard]] static OwningReference createOwningReferenceFrom(OperationOperandSimplificationResult&& data) {
                return std::make_unique<OperationOperandSimplificationResult>(data);
            }

            explicit OperationOperandSimplificationResult(std::size_t numGeneratedAssignments, std::optional<std::variant<syrec::VariableAccess::ptr, syrec::Expression::ptr>> data, bool wasManuallyCreated = false) noexcept
                : numGeneratedAssignments(numGeneratedAssignments), data(std::move(data)), wasManuallyCreated(wasManuallyCreated), resultUnknown(false) {}
        private:
            std::size_t                                                                     numGeneratedAssignments;
            std::optional<std::variant<syrec::VariableAccess::ptr, syrec::Expression::ptr>> data;
            bool                                                                            wasManuallyCreated;
            bool                                                                            resultUnknown;
        };
        

        [[nodiscard]] std::optional<OperationOperandSimplificationResult::OwningReference> handleOperationNode(const ExpressionTraversalHelper::OperationNodeReference& operationNode, const SignalValueLookupCallback& signalValueLookupCallback);
        [[nodiscard]] std::optional<OperationOperandSimplificationResult::OwningReference> handleOperationNodeWithNoLeafNodes(const ExpressionTraversalHelper::OperationNodeReference& operationNode, const SignalValueLookupCallback& signalValueLookupCallback);
        [[nodiscard]] std::optional<OperationOperandSimplificationResult::OwningReference> handleOperationNodeWithOneLeafNode(const ExpressionTraversalHelper::OperationNodeReference& operationNode, const SignalValueLookupCallback& signalValueLookupCallback);
        [[nodiscard]] std::optional<OperationOperandSimplificationResult::OwningReference> handleOperationNodeWithOnlyLeafNodes(const ExpressionTraversalHelper::OperationNodeReference& operationNode, const SignalValueLookupCallback& signalValueLookupCallback);

        // This function should probably be refactored since the usage of a signal access can only determine a conflict and not the choice when trying to create an assignments
        // Only active assignments contribute to the set of potential conflicts.
        // Determining whether a conflict exists would happen when parsing a signal access used in an expression by checking whether an active assignment for any of accessed signal parts exists.
        // If so rollback to the conflicting decision and try again now excluding the previous choice (or creating an replacement for it [i.e. the choice for the assignment if the size of the replacement can be determined])

        // Backtrack to last overlapping decision, add additional field backtrackingToNode to determine whether a conflict or a backtrack is in progress when std::nullopt is returned during handling of operation node
        // Add lookup for learned "clauses" i.e. previous conflicts that can be erased during backtracking
        [[nodiscard]] bool                             doesAssignmentToSignalLeadToConflict(const syrec::VariableAccess& assignedToSignal) const;
        [[nodiscard]] std::optional<DecisionReference> tryGetDecisionForOperationNode(const std::size_t& operationNodeId) const;
        
        [[nodiscard]] DecisionReference           makeDecision(const ExpressionTraversalHelper::OperationNodeReference& operationNode, OperationOperandSimplificationResult& simplificationResultOfFirstOperand, OperationOperandSimplificationResult& simplificationResultOfSecondOperand, const SignalValueLookupCallback& callbackForValueLookupOfExistingSymbolTableSignals);
        [[nodiscard]] std::optional<std::size_t>  determineOperationNodeIdCausingConflict(const syrec::VariableAccess& choiceOfAssignedToSignalTriggeringSearchForCauseOfConflict) const;
        [[nodiscard]] bool                        isOperationNodeSourceOfConflict(std::size_t operationNodeId) const;
        void                                      markOperationNodeAsSourceOfConflict(std::size_t operationNodeId);
        void                                      markSourceOfConflictReached();
        [[nodiscard]] bool                        shouldBacktrackDueToConflict() const;

        void                                                                            considerExpressionInFutureDecisions(const syrec::Expression::ptr& expr) const;
        void                                                                            revokeConsiderationOfExpressionForFutureDecisions(const syrec::Expression::ptr& expr) const;
        [[nodiscard]] std::vector<syrec::VariableAccess::ptr>                           defineNotUsableReplacementCandidatesFromAssignmentForGenerator(const syrec::AssignStatement& assignment) const;
        void                                                                            clearNotUsableReplacementCandidatesFromAssignmentForGenerator(const std::vector<syrec::VariableAccess::ptr>& replacementCandidateRestrictionsStemmingFromAssignment) const;
        [[nodiscard]] bool                                                              isChoiceOfSignalAccessBlockedByAnyActiveExpression(const syrec::VariableAccess& chosenOperand) const;
        [[nodiscard]] std::optional<syrec::VariableAccess::ptr>                         createReplacementForChosenOperand(const DecisionReference& decisionToReplace, const SignalValueLookupCallback& callbackForValueLookupOfExistingSymbolTableSignals) const;
        [[nodiscard]] std::optional<syrec::VariableAccess::ptr>                         getAndActivateReplacementForOperationNode(std::size_t referencedOperationNodeId, syrec_operation::operation definedOperationInOperationNode, const OperationOperandSimplificationResult& lhsOperandDataChoicesAfterSimplification, const OperationOperandSimplificationResult& rhsOperandDataChoicesAfterSimplification, bool* wasExistingReplacementForOperationNodeEntryUpdated, const SignalValueLookupCallback& callbackForValueLookupOfExistingSymbolTableSignals);
        [[nodiscard]] static std::optional<bool>                                        tryUpdateOperandsOfExistingReplacementForWholeBinaryExpressionIfChangedAndReturnWhetherUpdateTookPlace(syrec::BinaryExpression& replacementRhsAsBinaryExpr, const OperationOperandSimplificationResult& potentiallyNewLhsOperandOfBinaryExpr, const OperationOperandSimplificationResult& potentiallyNewRhsOperandOfBinaryExpr);
        [[nodiscard]] static std::optional<bool>                                        tryUpdateOperandsOfExistingReplacementForWholeBinaryExpressionIfChangedAndReturnWhetherUpdateTookPlace(syrec::ShiftExpression& replacementRhsAsShiftExpr, const OperationOperandSimplificationResult& potentiallyNewLhsOperandOfShiftExpr, const OperationOperandSimplificationResult& potentiallyNewRhsOperandOfShiftExpr);

        // This call should be responsible to determine conflicts, during a decision their should not arise any conflicts
        // The check for a conflict should take place in the operations nodes with either one or two leaf nodes by using this call before making any decisions
        // If a conflict is detected, the first decision starting from the initial one shall be our backtrack destination and all overlapping assignments for the found first decision
        // shall be recorded as learned conflicts.
        [[nodiscard]] bool                                                                                                 wereAccessedSignalPartsModifiedByActiveAssignment(const syrec::VariableAccess& accessedSignalParts) const;
        [[nodiscard]] std::optional<DecisionReference>                                                                     determineEarliestDecisionsOverlappingAccessedSignalPartsOmittingAlreadyRecordedOnes(const syrec::VariableAccess& accessedSignalParts) const;
        void                                                                                                               handleConflict(std::size_t associatedOperationNodeIdOfAccessedSignalPartsOperand, Decision::ChoosenOperand chosenOperandLeadingToDetectionOfConflict, const syrec::VariableAccess& accessedSignalPartsUsedInCheckForConflict);
        [[nodiscard]] std::size_t                                                                                          determineEarliestSharedParentOperationNodeIdBetweenCurrentAndConflictOperationNodeId(std::size_t operationNodeIdOfEarliestSourceOfConflict, std::size_t operationNodeIdWhereConflictWasDetected, std::unordered_set<std::size_t>* hotPathContainerFromSourceToSharedOrigin) const;
        void                                                                                                               forceReuseOfPreviousDecisionsOnceAtAllDecisionsForChildrenOfNodeButExcludeHotpath(std::size_t parentOperationNodeId, const std::unordered_set<std::size_t>* idOfOperationNodesOnHotPath) const;

        /**
         * \brief Determine whether another choice for a previously made decision could be made. This calls excludes the last made decision under the assumption that this call is made after a conflict was derived at the current operation node
         * \return Whether another choice at any previous decision could be made
         */
        [[nodiscard]] bool                             couldAnotherChoiceBeMadeAtPreviousDecision(const std::optional<std::size_t>& pastDecisionForOperationNodeWithIdToExclude) const;
        
        void                                                          removeDecisionFor(std::size_t operationNodeId);
        [[nodiscard]] std::optional<DecisionReference>                tryGetLastDecision() const;
        [[nodiscard]] std::optional<DecisionReference>                tryGetSecondToLastDecision() const;
        virtual void                                                  resetInternals();
        [[nodiscard]] virtual std::unique_ptr<syrec::AssignStatement> transformAssignmentPriorToSimplification(const syrec::AssignStatement& assignmentToSimplify, bool applyHeuristicsForSubassignmentGeneration) const;
        void                                                          transformExpressionPriorToSimplification(syrec::Expression& expr, bool applyHeuristicsForSubassignmentGeneration) const;
        void                                                          rememberConflict(std::size_t operationNodeId, Decision::ChoosenOperand chosenOperandAtOperationNode) const;
        [[nodiscard]] bool                                            didPreviousDecisionMatchingChoiceCauseConflict(const LearnedConflictsLookupKey& lookupKeyRepresentingSearchedForPreviousDecision) const;
        void                                                          disableValueLookup();
        void                                                          enabledValueLookup();
        void                                                          backtrack(std::size_t operationNodeIdAtOriginOfBacktrack, bool onlyUpToOperationNode) const;
        /**
         * \brief Records the made decision by storing it in the internal container that records the made decision ordered by their operation node id descending.
         * \param decision The decision to store
         */
        void                                                          recordDecision(const DecisionReference& decision);

        struct OperationNodeProcessingResult {
            std::optional<OperationOperandSimplificationResult::OwningReference> simplificationResult;
            bool                                                               derivedConflictInOtherNode;
            bool                                                               derivedConflictInThisNode;

            OperationNodeProcessingResult() :
                simplificationResult(std::nullopt), derivedConflictInOtherNode(false), derivedConflictInThisNode(false) {}

            [[nodiscard]] bool isResultUnknown() const {
                return !simplificationResult.has_value() && !derivedConflictInOtherNode && !derivedConflictInThisNode;
            }

            [[nodiscard]] static OperationNodeProcessingResult fromUnknownResult() {
                return {};
            }

            [[nodiscard]] static OperationNodeProcessingResult fromResult(std::optional<OperationOperandSimplificationResult::OwningReference>&& simplificationResult) {
                OperationNodeProcessingResult result;
                result.simplificationResult = std::move(simplificationResult);
                return result;
            }

            [[nodiscard]] static OperationNodeProcessingResult fromConflictInOtherNode() {
                OperationNodeProcessingResult result;
                result.derivedConflictInOtherNode = true;
                return result;
            }

            [[nodiscard]] static OperationNodeProcessingResult fromFromConflictInSameNode() {
                OperationNodeProcessingResult result;
                result.derivedConflictInThisNode = true;
                return result;
            }
        };
        [[nodiscard]] OperationNodeProcessingResult                                                   processNextOperationNode(const SignalValueLookupCallback& signalValueLookupCallback);
        [[nodiscard]] bool                                                                            canAlternativeByChosenInOperationNode(std::size_t operationNodeId, Decision::ChoosenOperand toBeChosenAlternative, const syrec::VariableAccess& signalAccess, const syrec::Expression& alternativeToCheckAsExpr, const parser::SymbolTable& symbolTable) const;
        [[nodiscard]] bool                                                                            canAlternativeByChosenInOperationNode(std::size_t operationNodeId, Decision::ChoosenOperand toBeChosenAlternative, const syrec::VariableAccess& signalAccess, const syrec::VariableAccess& alternativeToCheckAsSignalAccess, const parser::SymbolTable& symbolTable) const;
        [[nodiscard]] bool                                                                            isAccessedSignalAssignable(const std::string_view& accessedSignalIdent) const;
        [[nodiscard]] std::optional<double>                                                           determineCostOfExpr(const syrec::Expression& expr, std::size_t currentNestingLevel) const;
        [[nodiscard]] std::optional<double>                                                           determineCostOfAssignment(const syrec::AssignStatement& assignment) const;
        [[nodiscard]] std::optional<double>                                                           determineCostOfNumber(const syrec::Number& number, std::size_t currentNestingLevel) const;
        [[nodiscard]] std::optional<double>                                                           determineCostOfAssignments(const std::vector<AssignmentTransformer::SharedAssignmentReference>& assignmentsToCheck) const;
        [[nodiscard]] std::optional<double>                                                           determineCostOfAssignments(SimplificationResult::OwningCopiesOfAssignment& assignments) const;
        [[nodiscard]] SimplificationResult::OwningReference                                           determineMostViableAlternativeBasedOnCost(SimplificationResult::OwningReference& generatedSimplifiedAssignments, const std::shared_ptr<syrec::AssignStatement>& originalAssignmentUnoptimized, const SignalValueLookupCallback& signalValueCallback) const;
        [[nodiscard]] std::optional<std::variant<syrec::VariableAccess::ptr, syrec::Expression::ptr>> tryCreateFinalSimplificationResultAfterDecisionWasMade(const DecisionReference& madeDecision, const OperationOperandSimplificationResult& lhsOperandOfOperationNode, syrec_operation::operation definedOperationAtOperationNode, const OperationOperandSimplificationResult& rhsOperandOfOperationNode, const SignalValueLookupCallback& callbackForValueLookupOfExistingSymbolTableSignals);

        [[nodiscard]] static std::optional<syrec::Expression::ptr>       fuseExpressions(const syrec::Expression::ptr& lhsOperand, syrec_operation::operation op, const syrec::Expression::ptr& rhsOperand);
        [[nodiscard]] static std::optional<syrec::AssignStatement::ptr>  tryCreateAssignmentForOperationNode(const syrec::VariableAccess::ptr& assignmentLhs, syrec_operation::operation op, const syrec::Expression::ptr& assignmentRhs);
        [[nodiscard]] static syrec::Expression::ptr                      createExpressionFromOperandSimplificationResult(const OperationOperandSimplificationResult& operandSimplificationResult);
        [[nodiscard]] static bool                                        doesExpressionDefineNestedSplitableExpr(const syrec::Expression& expr);
        [[nodiscard]] static bool                                        doesExpressionDefineNumber(const syrec::Expression& expr);
        [[nodiscard]] static bool                                        doesExprDefineSignalAccess(const syrec::Expression& expr);
        [[nodiscard]] static std::optional<syrec::AssignStatement::ptr>  tryCreateAssignmentFromOperands(Decision::ChoosenOperand chosenOperandAsAssignedToSignal, const OperationOperandSimplificationResult& simplificationResultOfFirstOperand, syrec_operation::operation operationNodeOperation, const OperationOperandSimplificationResult& simplificationResultOfSecondOperand);
        [[nodiscard]] static std::optional<syrec::Expression::ptr>       tryCreateExpressionFromOperationNodeOperandSimplifications(const OperationOperandSimplificationResult& simplificationResultOfFirstOperand, syrec_operation::operation operationNodeOperation, const OperationOperandSimplificationResult& simplificationResultOfSecondOperand);
        [[nodiscard]] static std::optional<syrec::Expression::ptr>       tryCreateExpressionFromOperationNodeOperandSimplification(const OperationOperandSimplificationResult& simplificationResultOfOperand);
        [[nodiscard]] static std::optional<syrec::Number::ptr>           tryCreateNumberFromOperationNodeOperandSimplification(const OperationOperandSimplificationResult& simplificationResultOfOperand);
        [[nodiscard]] static bool                                        areAssignedToSignalPartsZero(const syrec::VariableAccess& accessedSignalParts, const SignalValueLookupCallback& signalValueLookupCallback);
        [[nodiscard]] static bool                                        doesExprOnlyDefineReversibleOperationsAndNoBitwiseXorOperationWithNoLeafNodes(const syrec::Expression& expr);
        static void                                                      tryConvertNumericToBinaryExpr(syrec::Expression::ptr& expr);
        [[nodiscard]] static std::optional<syrec::BinaryExpression::ptr> convertNumericExprToBinary(const syrec::NumericExpression& numericExpr);
        [[nodiscard]] static std::optional<syrec_operation::operation>   tryMapCompileTimeConstantExprOperationToBinaryOperation(syrec::Number::CompileTimeConstantExpression::Operation operation);
        [[nodiscard]] static syrec::Expression::ptr                      convertNumberToExpr(const syrec::Number::ptr& number, unsigned int expectedBitwidth);
        [[nodiscard]] static std::optional<syrec::BinaryExpression::ptr> convertCompileTimeConstantExprToBinaryExpr(const syrec::Number::CompileTimeConstantExpression& compileTimeConstantExpr, unsigned int expectedBitwidth);
        [[nodiscard]] static syrec::Expression::ptr                      transformExprBeforeProcessing(const syrec::Expression::ptr& initialExpr);
        
        [[nodiscard]] static bool                                    doesExprContainOverlappingAccessOnGivenSignalAccess(const syrec::Expression& expr, const syrec::VariableAccess& signalAccess, const parser::SymbolTable& symbolTable);
        [[nodiscard]] static bool                                    doesNumberContainOverlappingAccessOnGivenSignalAccess(const syrec::Number& number, const syrec::VariableAccess& signalAccess, const parser::SymbolTable& symbolTable);
        [[nodiscard]] static bool                                    doSignalAccessesOverlap(const syrec::VariableAccess& firstSignalAccess, const syrec::VariableAccess& otherSignalAccess, const parser::SymbolTable& symbolTable);
        [[nodiscard]] static bool                                    areSignalAccessesSyntacticallyEquivalent(const syrec::VariableAccess& firstSignalAccess, const syrec::VariableAccess& otherSignalAccess, const parser::SymbolTable& symbolTable);
        [[nodiscard]] static std::optional<unsigned int>             determineBitwidthOfSignalAccess(const syrec::VariableAccess& signalAccess);
        [[nodiscard]] static std::optional<unsigned int>             evaluateNumber(const syrec::Number& numberToEvaluate);
        [[nodiscard]] static bool                                    doesOperandSimplificationResultMatchExpression(const OperationOperandSimplificationResult& operandSimplificationResult, const syrec::Expression::ptr& exprToCheck);
        [[nodiscard]] static bool                                                          doesOperandSimplificationResultMatchNumber(const OperationOperandSimplificationResult& operandSimplificationResult, const syrec::Number::ptr& numberToCheck);
        [[nodiscard]] static std::optional<SimplificationResult::OwningCopyOfAssignment> createOwningCopyOfAssignment(const syrec::AssignStatement& assignment);
        [[nodiscard]] static std::optional<SimplificationResult::OwningCopyOfAssignment>   createOwningCopyOfAssignment(const syrec::UnaryStatement& assignment);
        [[nodiscard]] static std::optional<SimplificationResult::OwningCopiesOfAssignment> createOwningCopiesOfAssignments(const std::vector<TemporaryAssignmentsContainer::SharedAssignmentReference>& assignments);
         static void                                                          tryAddCosts(std::optional<double>& currentSumOfCosts, const std::optional<double>& costsToAdd);
        /**
         * \brief Determines the signal parts defined in the given expression which cannot be used as potential replacement candidates
         * \param expr The expression which shall be checked
         * \param symbolTable The symbol table used to lookup values during the comparision of signal accesses
         * \remarks Only assignable signals are considered as potential replacement candidates. We are also assuming that numeric expression were converted to their binary expression counterpart.
         * Unary expressions are currently not supported since SyReC does not provide a fitting container for them.
         * \return The found exclusion vector containing all non-usable replacement candidate parts
         */
        [[nodiscard]] static std::vector<syrec::VariableAccess::ptr> determineSignalPartsNotUsableAsPotentialReplacementCandidates(const syrec::Expression& expr, const parser::SymbolTable& symbolTable);

        [[nodiscard]] static constexpr bool shouldLogMessageBePrinted();
        [[nodiscard]] static std::string    stringifyChosenOperandForLogMessage(Decision::ChoosenOperand chosenOperand);
        [[nodiscard]] static std::string    stringifyRepetitionOfChoice(Decision::ChoiceRepetition setRepetitionOfChoice);
        static void                         logDecision(const DecisionReference& madeDecision);
        static void                         logConflict(std::size_t operationNodeId, Decision::ChoosenOperand operandCausingConflict, const syrec::VariableAccess& signalAccessCausingConflict, const std::optional<std::size_t>& idOfEarliestDecisionInvolvedInConflict);
        static void                         logStartOfProcessingOfOperationNode(std::size_t operationNodeId);
        static void                         logEndOfProcessingOfOperationNode(std::size_t operationNodeId);
        static void                         logBacktrackingResult(std::size_t operationNodeIdAtStartOfBacktracking, std::size_t nextOperationNodeAfterBacktrackingFinished);
        static void                         logMarkingOfOperationNodeAsSourceOfConflict(std::size_t operationNodeId);
        static void                         logMessage(const std::string& msg);
        static void                         logCreationOfSubstitutionOfOperandOfOperationNode(std::size_t operationNodeId, Decision::ChoosenOperand substitutedOperand, const syrec::VariableAccess& generatedSubstitution);
        static void                         logCreationOfSubstitutionOfExprOfOperationNode(std::size_t operationNodeId, const std::string& replacementSignalIdent, bool wasExistingEntryUpdated);
        static void                         logLearnedConflict(std::size_t operationNodeId, Decision::ChoosenOperand learnedConflictingChoiceOfOperand);
    };
}

#endif