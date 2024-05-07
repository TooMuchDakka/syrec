#ifndef OPTIMIZER_HPP
#define OPTIMIZER_HPP
#pragma once

#include "core/syrec/statement.hpp"
#include "core/syrec/parser/operation.hpp"
#include "core/syrec/parser/parser_config.hpp"
#include "core/syrec/parser/symbol_table.hpp"
#include "core/syrec/parser/symbol_table_backup_helper.hpp"
#include "core/syrec/parser/utils/loop_body_value_propagation_blocker.hpp"
#include "core/syrec/parser/utils/signal_access_utils.hpp"
#include "noAdditionalLineSynthesis/assignment_without_additional_lines_simplifier.hpp"

/*
 * TODO: Support for VHDL like signal access (i.e. bit range structured as .bitRangeEnd:bitRangeStart instead of .bitRangeStart:bitRangeEnd)
 */
namespace optimizations {
    class Optimizer {
    public:
        explicit Optimizer(const parser::ParserConfig& parserConfig, const parser::SymbolTable::ptr& sharedSymbolTableReference):
            parserConfig(parserConfig), activeSymbolTableScope({!sharedSymbolTableReference ? std::make_shared<parser::SymbolTable>() : sharedSymbolTableReference}), loopUnrollerInstance(std::make_unique<optimizations::LoopUnroller>()), activeDataFlowValuePropagationRestrictions(std::make_unique<LoopBodyValuePropagationBlocker>(activeSymbolTableScope)),
            assignmentWithoutAdditionalLineSynthesisOptimizer(parserConfig.noAdditionalLineOptimizationConfig.has_value() ? std::make_unique<noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier>(*parserConfig.noAdditionalLineOptimizationConfig) : nullptr) {}

        // TODO: Add new result flag FAILED?
        enum OptimizationResultFlag {
            WasOptimized,
            IsUnchanged,
            RemovedByOptimization
        };
        template<typename ToBeOptimizedContainerType>
        class OptimizationResult {
        public:
            
            [[nodiscard]] static OptimizationResult fromOptimizedContainer(std::vector<std::unique_ptr<ToBeOptimizedContainerType>> optimizationResult) {
                return OptimizationResult(OptimizationResultFlag::WasOptimized, std::move(optimizationResult));
            }

            [[nodiscard]] static OptimizationResult fromOptimizedContainer(std::unique_ptr<ToBeOptimizedContainerType> optimizationResult) {
                return OptimizationResult(OptimizationResultFlag::WasOptimized, std::move(optimizationResult));
            }

            [[nodiscard]] static OptimizationResult asOptimizedAwayContainer() {
                return OptimizationResult(OptimizationResultFlag::RemovedByOptimization, nullptr);    
            }

            [[nodiscard]] static OptimizationResult asUnchangedOriginal() {
                return OptimizationResult(OptimizationResultFlag::IsUnchanged, nullptr);
            }

            [[nodiscard]] std::optional<std::vector<std::unique_ptr<ToBeOptimizedContainerType>>> tryTakeOwnershipOfOptimizationResult() {
                switch (status) {
                    case OptimizationResultFlag::IsUnchanged:
                        return std::nullopt;
                    case OptimizationResultFlag::RemovedByOptimization:
                        return std::nullopt;
                    case OptimizationResultFlag::WasOptimized:
                        return std::move(result);
                    default:
                        return std::nullopt;
                }
            }

            [[nodiscard]] OptimizationResultFlag getStatusOfResult() const {
                return status;
            }

            OptimizationResult(OptimizationResultFlag resultFlag, std::unique_ptr<ToBeOptimizedContainerType> optimizationResult):
                status(resultFlag) {
                if (optimizationResult) {
                    std::vector<std::unique_ptr<ToBeOptimizedContainerType>> container;
                    container.reserve(1);
                    container.emplace_back(std::move(optimizationResult));
                    result = std::move(container);
                }
            }

            OptimizationResult(OptimizationResultFlag resultFlag, std::vector<std::unique_ptr<ToBeOptimizedContainerType>> optimizationResult):
                status(resultFlag), result(std::make_optional(std::move(optimizationResult))) {}

            OptimizationResult(OptimizationResultFlag resultFlag, std::nullptr_t):
                status(resultFlag), result(std::nullopt) {}
        private:
            OptimizationResultFlag                                                  status;
            std::optional<std::vector<std::unique_ptr<ToBeOptimizedContainerType>>> result;
        };

        struct ExpectedMainModuleCallSignature {
            std::string moduleIdent;
            std::vector<std::string> parameterIdentsOfUnoptimizedModuleVersion;
        };

        [[nodiscard]] OptimizationResult<syrec::Module>       optimizeProgram(const std::vector<std::reference_wrapper<const syrec::Module>>& modules, const std::string_view& defaultMainModuleIdent = "main");
        [[nodiscard]] OptimizationResult<syrec::Statement>    handleStatements(const std::vector<std::reference_wrapper<const syrec::Statement>>& statements);
        [[nodiscard]] OptimizationResult<syrec::Statement>    handleStatement(const syrec::Statement& stmt);
        [[nodiscard]] OptimizationResult<syrec::Expression>   handleExpr(const syrec::Expression& expression) const;
        [[nodiscard]] OptimizationResult<syrec::Number>       handleNumber(const syrec::Number& number) const;
    protected:
        using InternalOwningAssignmentType = std::variant<std::unique_ptr<syrec::AssignStatement>, std::unique_ptr<syrec::UnaryStatement>>;

        parser::ParserConfig                                          parserConfig;
        parser::SymbolTable::ptr                                      activeSymbolTableScope;
        parser::SymbolTable::ModuleIdentifier                         internalIdentifierOfCurrentlyProcessedModule;
        std::unique_ptr<LoopUnroller>                                 loopUnrollerInstance;
        std::size_t                                                   internalIfStatementNestingLevel = 0;
        std::size_t                                                   internalLoopNestingLevel             = 0;
        std::optional<bool>                                           internalReferenceCountModificationEnabledFlag;

        struct IfStatementBranchSymbolTableBackupScope {
            std::size_t ifStatementNestingLevel;
            std::shared_ptr<parser::SymbolTableBackupHelper> backupScope;
        };
        std::vector<IfStatementBranchSymbolTableBackupScope>                                  symbolTableBackupScopeContainers;
        std::unique_ptr<LoopBodyValuePropagationBlocker>                                      activeDataFlowValuePropagationRestrictions;
        syrec::Number::loop_variable_mapping                                                  evaluableLoopVariableLookup;
        std::unique_ptr<noAdditionalLineSynthesis::AssignmentWithoutAdditionalLineSimplifier> assignmentWithoutAdditionalLineSynthesisOptimizer;

        void                                                                                       openNewIfStatementBranchSymbolTableBackupScope(std::size_t nestingLevel);
        void                                                                                       updateBackupOfValuesChangedInScopeAndOptionallyResetMadeChanges(bool resetLocalChanges) const;
        void                                                                                       transferBackupOfValuesChangedInCurrentScopeToParentScope();

        // Return smart pointers instead ?
        [[nodiscard]] std::optional<std::reference_wrapper<const IfStatementBranchSymbolTableBackupScope>> peekCurrentSymbolTableBackupScope() const;
        [[nodiscard]] std::optional<const std::reference_wrapper<IfStatementBranchSymbolTableBackupScope>> peekModifiableCurrentSymbolTableBackupScope();
        [[nodiscard]] std::optional<const std::reference_wrapper<IfStatementBranchSymbolTableBackupScope>> peekPredecessorOfCurrentSymbolTableBackupScope();

        void                                                                                  mergeAndMakeLocalChangesGlobal(const parser::SymbolTableBackupHelper& backupOfSignalsChangedInFirstBranch, const parser::SymbolTableBackupHelper& backupOfSignalsChangedInSecondBranch) const;
        void                                                                                  makeLocalChangesGlobal(const parser::SymbolTableBackupHelper& backupOfCurrentValuesOfSignalsAtEndOfBranch, const parser::SymbolTableBackupHelper& backupCurrentValuesOfSignalsInToBeOmittedBranch) const;
        void                                                                                  destroySymbolTableBackupScope();
        void                                                                                  createBackupOfAssignedToSignal(const syrec::VariableAccess& assignedToVariableAccess);
        [[nodiscard]] std::optional<std::unique_ptr<IfStatementBranchSymbolTableBackupScope>> popCurrentSymbolTableBackupScope();

        [[nodiscard]] OptimizationResult<syrec::Module> handleModule(const syrec::Module& module, const ExpectedMainModuleCallSignature& expectedMainModuleSignature);

        [[nodiscard]] OptimizationResult<syrec::Statement>               handleUnaryStmt(const syrec::UnaryStatement& unaryStmt);
        [[nodiscard]] OptimizationResult<syrec::Statement>               handleAssignmentStmt(const syrec::AssignStatement& assignmentStmt);
        [[nodiscard]] OptimizationResult<syrec::Statement>               handleCallStmt(const syrec::CallStatement& callStatement);
        [[nodiscard]] OptimizationResult<syrec::Statement>               handleUncallStmt(const syrec::UncallStatement& callStatement) const;
        [[nodiscard]] OptimizationResult<syrec::Statement>               handleIfStmt(const syrec::IfStatement& ifStatement);
        [[nodiscard]] OptimizationResult<syrec::Statement>               handleLoopStmt(const syrec::ForStatement& forStatement);
        [[nodiscard]] OptimizationResult<syrec::Statement>               handleSwapStmt(const syrec::SwapStatement& swapStatement);
        [[nodiscard]] OptimizationResult<syrec::Statement>               handleSkipStmt();

        [[nodiscard]] OptimizationResult<syrec::Expression>     handleBinaryExpr(const syrec::BinaryExpression& expression) const;
        [[nodiscard]] OptimizationResult<syrec::Expression>     handleShiftExpr(const syrec::ShiftExpression& expression) const;
        [[nodiscard]] OptimizationResult<syrec::Expression>     handleVariableExpr(const syrec::VariableExpression& expression) const;
        [[nodiscard]] OptimizationResult<syrec::Expression>     handleNumericExpr(const syrec::NumericExpression& numericExpr) const;

        [[nodiscard]] static std::unique_ptr<syrec::Expression>                                               trySimplifyExpr(const syrec::Expression& expr, const parser::SymbolTable& symbolTable, bool simplifyExprByReassociation, bool performOperationStrengthReduction, std::vector<syrec::VariableAccess::ptr>* optimizedAwaySignalAccesses);
        [[nodiscard]] static std::unique_ptr<syrec::Expression>                                               transformCompileTimeConstantExpressionToNumber(const syrec::Number::CompileTimeConstantExpression& compileTimeConstantExpr);
        [[nodiscard]] static std::optional<syrec_operation::operation>                                        tryMapCompileTimeConstantOperation(syrec::Number::CompileTimeConstantExpression::Operation compileTimeConstantExprOperation);
        [[nodiscard]] static std::optional<syrec::Number::CompileTimeConstantExpression::Operation>           tryMapOperationToCompileTimeConstantOperation(syrec_operation::operation operation);
        [[nodiscard]] static std::unique_ptr<syrec::Expression>                                               tryMapCompileTimeConstantOperandToExpr(const syrec::Number& number);
        [[nodiscard]] static std::unique_ptr<syrec::Number>                                                   tryMapExprToCompileTimeConstantExpr(const syrec::Expression& expr);
        [[nodiscard]] std::optional<std::variant<unsigned int, std::unique_ptr<syrec::Number>>>               trySimplifyNumber(const syrec::Number& number) const;
        
        
        void updateReferenceCountOf(const std::string_view& signalIdent, parser::SymbolTable::ReferenceCountUpdate typeOfUpdate) const;
        void updateReferenceCountsOfSignalIdentsUsedIn(const syrec::Expression& expr, parser::SymbolTable::ReferenceCountUpdate typeOfUpdate) const;
        void updateReferenceCountsOfSignalIdentsUsedIn(const syrec::Number& number, parser::SymbolTable::ReferenceCountUpdate typeOfUpdate) const;
        
        static void                                           createSymbolTableEntryForVariable(parser::SymbolTable& symbolTable, const syrec::Variable& variable);
        static void                                           createSymbolTableEntriesForModuleParametersAndLocalVariables(parser::SymbolTable& symbolTable, const syrec::Module& module);
        [[nodiscard]] std::optional<syrec::Variable::ptr>     getSymbolTableEntryForVariable(const std::string_view& signalLiteralIdent) const;
        [[nodiscard]] std::optional<syrec::Number::ptr>       getSymbolTableEntryForLoopVariable(const std::string_view& loopVariableIdent) const;
        [[nodiscard]] std::optional<parser::SymbolTable::ptr> getActiveSymbolTableScope() const;
        void                                                  openNewSymbolTableScope();
        void                                                  closeActiveSymbolTableScope();

        [[nodiscard]] std::optional<unsigned int> tryFetchValueForAccessedSignal(const syrec::VariableAccess& accessedSignal) const;

        enum IndexValidityStatus {
            Valid,
            Unknown,
            OutOfRange
        };

        template<class T>
        struct SignalAccessIndexEvaluationResult {
            const IndexValidityStatus                          validityStatus;
            const std::optional<unsigned int>                  constantValue;
            std::unique_ptr<T>                                 simplifiedNonConstantValue;
            std::unique_ptr<T>                                 originalValue;
            const bool                                         wasToBeEvaluatedIndexSimplified;

            [[nodiscard]] static SignalAccessIndexEvaluationResult<T> createFromConstantValue(IndexValidityStatus validityStatus, bool wasToBeEvaluatedIndexSimplified, unsigned int constantValue) {
                return SignalAccessIndexEvaluationResult<T>({.validityStatus = validityStatus, .constantValue = constantValue, .simplifiedNonConstantValue = nullptr, .originalValue = nullptr, .wasToBeEvaluatedIndexSimplified = wasToBeEvaluatedIndexSimplified});
            }

            [[nodiscard]] static SignalAccessIndexEvaluationResult<T> createFromSimplifiedNonConstantValue(IndexValidityStatus validityStatus, bool wasToBeEvaluatedIndexSimplified, std::unique_ptr<T> nonConstantValue) {
                return SignalAccessIndexEvaluationResult<T>({.validityStatus = validityStatus, .constantValue = std::nullopt, .simplifiedNonConstantValue = std::move(nonConstantValue), .originalValue = nullptr, .wasToBeEvaluatedIndexSimplified = wasToBeEvaluatedIndexSimplified });
            }

            [[nodiscard]] static SignalAccessIndexEvaluationResult<T> createAsUnchangedNonConstantValue(IndexValidityStatus validityStatus, std::unique_ptr<T> originalNonConstantValue) {
                return SignalAccessIndexEvaluationResult<T>({.validityStatus = validityStatus, .constantValue = std::nullopt, .simplifiedNonConstantValue = nullptr, .originalValue = std::move(originalNonConstantValue), .wasToBeEvaluatedIndexSimplified = false});
            }
        };

        template<class T>
        [[nodiscard]] static std::optional<unsigned int> tryFetchConstantValueOfIndex(const SignalAccessIndexEvaluationResult<T>& index) {
            return index.constantValue;
        }

        template<class T>
        [[nodiscard]] static std::optional<std::unique_ptr<T>> tryFetchAndTakeOwnershipOfNonConstantValueOfIndex(SignalAccessIndexEvaluationResult<T>& index) {
            if (index.simplifiedNonConstantValue) {
                return std::make_optional(std::move(index.simplifiedNonConstantValue));
            }
            if (index.originalValue) {
                return std::make_optional(std::move(index.originalValue));
            }
            return std::nullopt;
        }

        template<class T>
        [[nodiscard]] static std::optional<std::reference_wrapper<const T>> tryFetchNonOwningReferenceOfNonConstantValueOfIndex(const SignalAccessIndexEvaluationResult<T>& index) {
            if (index.simplifiedNonConstantValue) {
                return std::make_optional(std::cref(*index.simplifiedNonConstantValue));
            }
            if (index.originalValue) {
                return std::make_optional(std::cref(*index.originalValue));
            }
            return std::nullopt;
        }

        template<class T>
        [[nodiscard]] static bool couldUserDefinedIndexBySimplified(const SignalAccessIndexEvaluationResult<T>& index) {
            return index.constantValue.has_value() || index.simplifiedNonConstantValue != nullptr;
        }
        
        [[nodiscard]] static std::optional<std::unique_ptr<syrec::Number>> tryConvertSimplifiedIndexValueToUsableSignalAccessIndex(SignalAccessIndexEvaluationResult<syrec::Number>& index);
        [[nodiscard]] static std::optional<std::unique_ptr<syrec::Expression>> tryConvertSimplifiedIndexValueToUsableSignalAccessIndex(SignalAccessIndexEvaluationResult<syrec::Expression>& index);
        

        struct DimensionAccessEvaluationResult {
            const bool                                                        wasTrimmed;
            std::vector<SignalAccessIndexEvaluationResult<syrec::Expression>> valuePerDimension;
            const bool                                                        wasAnyExprSimplified;
        };
        [[nodiscard]] DimensionAccessEvaluationResult    evaluateUserDefinedDimensionAccess(const std::string_view& accessedSignalIdent, const std::vector<std::reference_wrapper<const syrec::Expression>>& accessedValuePerDimension) const;
        [[nodiscard]] static std::optional<unsigned int> tryEvaluateNumberAsConstant(const syrec::Number& number, const parser::SymbolTable* symbolTableUsedForEvaluation, bool shouldPerformConstantPropagation, const syrec::Number::loop_variable_mapping& evaluableLoopVariablesIfConstantPropagationIsDisabled,  bool* wasOriginalNumberSimplified);
        [[nodiscard]] static std::optional<unsigned int> tryEvaluateExpressionToConstant(const syrec::Expression& expr, const parser::SymbolTable* symbolTableUsedForEvaluation, bool shouldPerformConstantPropagation, const syrec::Number::loop_variable_mapping& evaluableLoopVariablesIfConstantPropagationIsDisabled, bool* wasOriginalExprSimplified);

        struct BitRangeEvaluationResult {
            SignalAccessIndexEvaluationResult<syrec::Number> rangeStartEvaluationResult;
            SignalAccessIndexEvaluationResult<syrec::Number> rangeEndEvaluationResult;
        };
        [[nodiscard]] std::optional<BitRangeEvaluationResult>         evaluateUserDefinedBitRangeAccess(const std::string_view& accessedSignalIdent, const std::optional<std::pair<const syrec::Number*, const syrec::Number*>>& accessedBitRange) const;
        [[nodiscard]] static std::unique_ptr<syrec::BinaryExpression> createBinaryExprFromCompileTimeConstantExpr(const syrec::Number::CompileTimeConstantExpression& compileTimeConstantExpression);

        struct EvaluatedSignalAccess {
            const std::string_view                        accessedSignalIdent;
            DimensionAccessEvaluationResult               accessedValuePerDimension;
            std::optional<BitRangeEvaluationResult>       accessedBitRange;
            const bool                                    wereAnyIndicesSimplified;
        };
        [[nodiscard]] std::optional<EvaluatedSignalAccess>   tryEvaluateDefinedSignalAccess(const syrec::VariableAccess& accessedSignalParts) const;
        [[nodiscard]] std::unique_ptr<syrec::VariableAccess> transformEvaluatedSignalAccess(const EvaluatedSignalAccess& evaluatedSignalAccess, std::optional<bool>* wasAnyDefinedIndexOutOfRange, bool* didAllDefinedIndicesEvaluateToConstants) const;

        void                                                        invalidateValueOfAccessedSignalParts(const EvaluatedSignalAccess& accessedSignalParts);
        void                                                        invalidateValueOfWholeSignal(const std::string_view& signalIdent);
        void                                                        performAssignment(const EvaluatedSignalAccess& assignedToSignalParts, syrec_operation::operation assignmentOperation, const std::optional<unsigned int>& assignmentRhsValue);
        void                                                        performAssignment(const EvaluatedSignalAccess& assignmentLhsOperand, syrec_operation::operation assignmentOperation, const EvaluatedSignalAccess& assignmentRhsOperand);
        void                                                        performSwap(const EvaluatedSignalAccess& swapOperationLhsOperand, const EvaluatedSignalAccess& swapOperationRhsOperand);
        void                                                        invalidateStoredValueFor(const syrec::VariableAccess::ptr& assignedToSignal);
        void                                                        updateStoredValueOf(const syrec::VariableAccess::ptr& assignedToSignal, unsigned int newValueOfAssignedToSignal);
        void                                                        performSwapAndCreateBackupOfOperands(const syrec::VariableAccess::ptr& swapLhsOperand, const syrec::VariableAccess::ptr& swapRhsOperand);

        [[nodiscard]] std::optional<unsigned int>                      tryFetchValueFromEvaluatedSignalAccess(const EvaluatedSignalAccess& accessedSignalParts) const;
        [[nodiscard]] bool                                             isValueLookupBlockedByDataFlowAnalysisRestriction(const syrec::VariableAccess& accessedSignalParts) const;
        [[nodiscard]] SignalAccessUtils::SignalAccessEquivalenceResult areEvaluatedSignalAccessEqualAtCompileTime(const EvaluatedSignalAccess& thisAccessedSignalParts, const EvaluatedSignalAccess& thatAccessedSignalParts) const;
        [[nodiscard]] bool                                             checkIfOperandsOfBinaryExpressionAreSignalAccessesAndEqual(const syrec::Expression& lhsOperand, const syrec::Expression& rhsOperand) const;


        [[nodiscard]] OptimizationResult<syrec::VariableAccess> handleSignalAccess(const syrec::VariableAccess& signalAccess, bool performConstantPropagationForAccessedSignal, std::optional<unsigned int>* fetchedValue) const;
        static void filterAssignmentsThatDoNotChangeAssignedToSignal(syrec::Statement::vec& assignmentsToCheck);

        struct SignalIdentCountLookup {
            std::map<std::string_view, std::size_t, std::less<void>> lookup;
        };
        static void                                     determineUsedSignalIdentsIn(const syrec::Expression& expr, SignalIdentCountLookup& lookupContainer);
        static void                                     determineUsedSignalIdentsIn(const syrec::Number& number, SignalIdentCountLookup& lookupContainer);
        static void                                     addSignalIdentToLookup(const std::string_view& signalIdent, SignalIdentCountLookup& lookupContainer);
        [[nodiscard]] static std::vector<std::reference_wrapper<const syrec::Statement>> transformCollectionOfSharedPointersToReferences(const syrec::Statement::vec& statements);
        void                                                                             decrementReferenceCountsOfLoopHeaderComponents(const syrec::Number& iterationRangeStart, const std::optional<std::reference_wrapper<const syrec::Number>>& iterationRangeEnd, const syrec::Number& stepSize) const;
        [[nodiscard]] static syrec::Statement::vec                                       createStatementListFrom(const syrec::Statement::vec& originalStmtList, std::vector<std::unique_ptr<syrec::Statement>> simplifiedStmtList);
        void                                                                             updateValueOfLoopVariable(const std::string& loopVariableIdent, const std::optional<unsigned int>& value);
        void                                                                             addSymbolTableEntryForLoopVariable(const std::string_view& loopVariableIdent, const std::optional<unsigned int>& expectedMaximumValue) const;
        void                                                                             removeLoopVariableFromSymbolTable(const std::string& loopVariableIdent);
        [[nodiscard]] std::optional<unsigned int>                                        tryDetermineMaximumValueOfLoopVariable(const std::string_view loopVariableIdent, const std::optional<unsigned int>& initialValueOfLoopVariable, const syrec::Number& maximumValueContainerToBeEvaluated);

        [[nodiscard]] bool                                                               isAnyModifiableParameterOrLocalModifiedInModuleBody(const syrec::Module& module, bool areCallerArgumentsBasedOnOptimizedSignature) const;
        [[nodiscard]] bool                                                               isAnyModifiableParameterOrLocalModifiedInStatements(const std::vector<std::reference_wrapper<const syrec::Statement>>& statements, const std::unordered_set<std::string>& parameterAndLocalLookup, bool areCallerArgumentsBasedOnOptimizedSignature) const;
        [[nodiscard]] bool                                                               isAnyModifiableParameterOrLocalModifiedInStatement(const syrec::Statement& statement, const std::unordered_set<std::string>& parameterAndLocalLookup, bool areCallerArgumentsBasedOnOptimizedSignature) const;
        [[nodiscard]] static bool                                                        isVariableReadOnly(const syrec::Variable& variable);
        [[nodiscard]] static std::optional<bool>                                         determineEquivalenceOfOperandsOfBinaryExpr(const syrec::BinaryExpression& binaryExpression);
        [[nodiscard]] bool                                                               doesStatementDefineAssignmentThatChangesAssignedToSignal(const syrec::Statement& statement, bool areCallerArgumentsBasedOnOptimizedSignature);
        void                                                                             emplaceSingleSkipStatementIfContainerIsEmpty(std::optional<std::vector<std::unique_ptr<syrec::Statement>>>& statementsContainer);
        [[nodiscard]] std::size_t                                                        incrementInternalIfStatementNestingLevelCounter();
        void                                                                             decrementInternalIfStatementNestingLevelCounter();
        [[maybe_unused]] std::size_t                                                     incrementInternalLoopNestingLevel();
        void                                                                             decrementInternalLoopNestingLevel();
        void                                                                             resetInternalNestingLevelsForIfAndLoopStatements();

        [[nodiscard]] std::unordered_set<std::size_t>                              determineIndicesOfUnusedModules(const std::vector<parser::SymbolTable::ModuleCallSignature>& unoptimizedModuleCallSignatures) const;
        [[nodiscard]] OptimizationResult<syrec::Module>                            removeUnusedModulesFrom(const std::vector<std::reference_wrapper<const syrec::Module>>& unoptimizedModules, const ExpectedMainModuleCallSignature& expectedMainModuleSignature) const;
        [[nodiscard]] OptimizationResult<syrec::Module>                            removeUnusedModulesFrom(const std::vector<parser::SymbolTable::ModuleCallSignature>& unoptimizedModuleCallSignatures, std::vector<std::unique_ptr<syrec::Module>>& optimizedModules, const ExpectedMainModuleCallSignature& expectedMainModuleSignature) const;
        [[nodiscard]] bool                                                         canModuleCallBeRemovedWhenIgnoringReferenceCount(const parser::SymbolTable::ModuleIdentifier& moduleIdentifier, const parser::SymbolTable::ModuleCallSignature& userProvidedCallSignature) const;
        [[nodiscard]] static std::optional<unsigned int>                           tryFetchValueOfLoopVariable(const std::string& loopVariableIdent, const parser::SymbolTable& activeSymbolTableScope, bool isConstantPropagationEnabled, const syrec::Number::loop_variable_mapping& evaluableLoopVariablesIfConstantPropagationIsDisabled);
        [[nodiscard]] static bool                                                  doesLoopBodyDefineAnyPotentiallyDangerousStmt(const syrec::Statement::vec& unoptimizedLoopBodyStmts, const std::optional<std::vector<std::unique_ptr<syrec::Statement>>>& optimizedLoopBodyStmts, const parser::SymbolTable& activeSymbolTableScope);
        [[nodiscard]] static bool                                                  doesModuleMatchMainModuleSignature(const parser::SymbolTable::ModuleCallSignature& moduleSignature, const ExpectedMainModuleCallSignature& expectedMainModuleSignature);
        [[nodiscard]] static std::optional<ExpectedMainModuleCallSignature>        determineActualMainModuleName(const std::vector<std::reference_wrapper<const syrec::Module>>& unoptimizedModules, const std::string_view& defaultMainModuleIdent);
        [[nodiscard]] static std::vector<parser::SymbolTable::ModuleCallSignature> createCallSignaturesFrom(const std::vector<std::reference_wrapper<const syrec::Module>>& modules);
        static void                                                                insertStatementCopiesInto(syrec::Statement::vec& containerForCopies, std::vector<std::unique_ptr<syrec::Statement>> copiesOfStatements);
        [[nodiscard]] bool                                                         optimizePeeledLoopBodyStatements(syrec::Statement::vec& containerForResult, const std::vector<std::unique_ptr<syrec::Statement>>& peeledLoopBodyStatements);
        [[nodiscard]] bool                                                         optimizeUnrolledLoopBodyStatements(syrec::Statement::vec& containerForResult, std::size_t numUnrolledIterations, optimizations::LoopUnroller::UnrolledIterationInformation& unrolledLoopBodyStatementsInformation);
        [[nodiscard]] static bool                                                  makeNewlyGeneratedSignalsAvailableInSymbolTableScope(parser::SymbolTable& symbolTableScope, const parser::SymbolTable::ModuleIdentifier& identifierOfModuleToWhichSignalsWillBeAddedTo, const syrec::Variable::vec& newlyGeneratedSignalsUsedForReplacements);
        static void                                                                moveOwningCopiesOfStatementsBetweenContainers(std::vector<std::unique_ptr<syrec::Statement>>& toBeMovedToContainer, std::vector<InternalOwningAssignmentType>&& toBeMovedFromContainer);

        template<typename T>
        void removeElementsAtIndices(std::vector<T>& vectorToModify, const std::unordered_set<std::size_t>& indicesOfElementsToRemove) {
            if (indicesOfElementsToRemove.empty() || vectorToModify.empty()) {
                return;
            }

            std::vector<std::size_t> containerOfSortedIndices;
            containerOfSortedIndices.reserve(indicesOfElementsToRemove.size());
            containerOfSortedIndices.insert(containerOfSortedIndices.end(), indicesOfElementsToRemove.begin(), indicesOfElementsToRemove.end());
            std::sort(containerOfSortedIndices.begin(), containerOfSortedIndices.end());

            for (auto indicesIterator = containerOfSortedIndices.rbegin(); indicesIterator != containerOfSortedIndices.rend(); ++indicesIterator) {
                vectorToModify.erase(std::next(vectorToModify.begin(), *indicesIterator));
            }
        }

        struct ReferenceCountLookupForStatement {
            struct ReferenceCountDifferences {
                std::unordered_map<std::string, std::size_t> lookupOfSignalsWithIncrementedReferenceCounts;
                std::unordered_map<std::string, std::size_t> lookupOfSignalsWithDecrementedReferenceCounts;
            };
            std::unordered_map<std::string, std::size_t> signalReferenceCountLookup;

            [[nodiscard]] ReferenceCountDifferences               getDifferencesBetweenThisAndOther(const ReferenceCountLookupForStatement& other) const;
            [[nodiscard]] ReferenceCountLookupForStatement&       mergeWith(const ReferenceCountLookupForStatement& other);
            [[nodiscard]] static ReferenceCountLookupForStatement createFromStatement(const syrec::Statement& statement);
            [[nodiscard]] static ReferenceCountLookupForStatement createFromExpression(const syrec::Expression& expression);
            [[nodiscard]] static ReferenceCountLookupForStatement createFromNumber(const syrec::Number& number);
            [[nodiscard]] static ReferenceCountLookupForStatement createFromSignalAccess(const syrec::VariableAccess& signalAccess);
        };
        static void updateReferenceCountsOfOptimizedAssignments(const parser::SymbolTable& symbolTable, const ReferenceCountLookupForStatement::ReferenceCountDifferences& referenceCountDifferences);
    private:
        [[nodiscard]] const parser::SymbolTable* getActiveSymbolTableForEvaluation() const;
    };
}
#endif