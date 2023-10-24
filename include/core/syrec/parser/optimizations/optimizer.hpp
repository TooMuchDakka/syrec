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

/*
 * TODO: Support for VHDL like signal access (i.e. bit range structured as .bitRangeEnd:bitRangeStart instead of .bitRangeStart:bitRangeEnd)
 */
namespace optimizations {
    class Optimizer {
    public:
        explicit Optimizer(const parser::ParserConfig& parserConfig, const parser::SymbolTable::ptr& sharedSymbolTableReference):
            parserConfig(parserConfig), activeSymbolTableScope({!sharedSymbolTableReference ? std::make_shared<parser::SymbolTable>() : sharedSymbolTableReference}), activeDataFlowValuePropagationRestrictions(std::make_unique<LoopBodyValuePropagationBlocker>(activeSymbolTableScope)) {}

        
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
        [[nodiscard]] OptimizationResult<syrec::expression>   handleExpr(const syrec::expression& expression) const;
        [[nodiscard]] OptimizationResult<syrec::Number>       handleNumber(const syrec::Number& number) const;
    protected:
        parser::ParserConfig                                          parserConfig;
        parser::SymbolTable::ptr                                      activeSymbolTableScope;

        std::size_t internalIfStatementNestingLevel = 0;
        struct IfStatementBranchSymbolTableBackupScope {
            std::size_t ifStatementNestingLevel;
            std::shared_ptr<parser::SymbolTableBackupHelper> backupScope;
        };
        std::vector<IfStatementBranchSymbolTableBackupScope>          symbolTableBackupScopeContainers;
        std::unique_ptr<LoopBodyValuePropagationBlocker>              activeDataFlowValuePropagationRestrictions;
        std::unordered_set<std::string>                               evaluableLoopVariableLookup;

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
        [[nodiscard]] static OptimizationResult<syrec::Statement>        handleSkipStmt();

        [[nodiscard]] OptimizationResult<syrec::expression>     handleBinaryExpr(const syrec::BinaryExpression& expression) const;
        [[nodiscard]] OptimizationResult<syrec::expression>     handleShiftExpr(const syrec::ShiftExpression& expression) const;
        [[nodiscard]] OptimizationResult<syrec::expression>     handleVariableExpr(const syrec::VariableExpression& expression) const;
        [[nodiscard]] OptimizationResult<syrec::expression>     handleNumericExpr(const syrec::NumericExpression& numericExpr) const;

        [[nodiscard]] static std::unique_ptr<syrec::expression>                                               trySimplifyExpr(const syrec::expression& expr, const parser::SymbolTable& symbolTable, bool simplifyExprByReassociation, bool performOperationStrengthReduction, std::vector<syrec::VariableAccess::ptr>* optimizedAwaySignalAccesses);
        [[nodiscard]] static std::unique_ptr<syrec::expression>                                               transformCompileTimeConstantExpressionToNumber(const syrec::Number::CompileTimeConstantExpression& compileTimeConstantExpr);
        [[nodiscard]] static std::optional<syrec_operation::operation>                                        tryMapCompileTimeConstantOperation(syrec::Number::CompileTimeConstantExpression::Operation compileTimeConstantExprOperation);
        [[nodiscard]] static std::optional<syrec::Number::CompileTimeConstantExpression::Operation>           tryMapOperationToCompileTimeConstantOperation(syrec_operation::operation operation);
        [[nodiscard]] static std::unique_ptr<syrec::expression>                                               tryMapCompileTimeConstantOperandToExpr(const syrec::Number& number);
        [[nodiscard]] static std::unique_ptr<syrec::Number>                                                   tryMapExprToCompileTimeConstantExpr(const syrec::expression& expr);
        [[nodiscard]] static std::optional<std::variant<unsigned int, std::unique_ptr<syrec::Number>>>        trySimplifyNumber(const syrec::Number& number, const parser::SymbolTable& symbolTable, bool simplifyExprByReassociation, bool performOperationStrengthReduction, std::vector<syrec::VariableAccess::ptr>* optimizedAwaySignalAccesses);
        
        
        void updateReferenceCountOf(const std::string_view& signalIdent, parser::SymbolTable::ReferenceCountUpdate typeOfUpdate) const;
        void updateReferenceCountsOfSignalIdentsUsedIn(const syrec::expression& expr, parser::SymbolTable::ReferenceCountUpdate typeOfUpdate) const;
        void updateReferenceCountsOfSignalIdentsUsedIn(const syrec::Number& number, parser::SymbolTable::ReferenceCountUpdate typeOfUpdate) const;
        
        void                                                  createSymbolTableEntryForVariable(const syrec::Variable& variable) const;
        void                                                  createSymbolTableEntriesForModuleParametersAndLocalVariables(const syrec::Module& module) const;
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
        [[nodiscard]] static std::optional<std::unique_ptr<syrec::expression>> tryConvertSimplifiedIndexValueToUsableSignalAccessIndex(SignalAccessIndexEvaluationResult<syrec::expression>& index);
        

        struct DimensionAccessEvaluationResult {
            const bool                                                              wasTrimmed;
            std::vector<SignalAccessIndexEvaluationResult<syrec::expression>> valuePerDimension;
            const bool                                                              wasAnyExprSimplified;
        };
        [[nodiscard]] DimensionAccessEvaluationResult                      evaluateUserDefinedDimensionAccess(const std::string_view& accessedSignalIdent, const std::vector<std::reference_wrapper<const syrec::expression>>& accessedValuePerDimension) const;
        [[nodiscard]] static std::optional<unsigned int>                   tryEvaluateNumberAsConstant(const syrec::Number& number, const parser::SymbolTable* symbolTableUsedForEvaluation, bool shouldPerformConstantPropagation, const std::unordered_set<std::string>& evaluableLoopVariablesIfConstantPropagationIsDisabled, bool *wasOriginalNumberSimplified);
        [[nodiscard]] static std::optional<unsigned int>                   tryEvaluateExpressionToConstant(const syrec::expression& expr, const parser::SymbolTable* symbolTableUsedForEvaluation, bool shouldPerformConstantPropagation, const std::unordered_set<std::string>& evaluableLoopVariablesIfConstantPropagationIsDisabled, bool *wasOriginalExprSimplified);

        struct BitRangeEvaluationResult {
            SignalAccessIndexEvaluationResult<syrec::Number> rangeStartEvaluationResult;
            SignalAccessIndexEvaluationResult<syrec::Number> rangeEndEvaluationResult;
        };
        [[nodiscard]] std::optional<BitRangeEvaluationResult>         evaluateUserDefinedBitRangeAccess(const std::string_view& accessedSignalIdent, const std::optional<std::pair<std::reference_wrapper<const syrec::Number>, std::reference_wrapper<const syrec::Number>>>& accessedBitRange) const;
        [[nodiscard]] static std::unique_ptr<syrec::BinaryExpression> createBinaryExprFromCompileTimeConstantExpr(const syrec::Number& number);

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
        [[nodiscard]] bool                                             checkIfOperandsOfBinaryExpressionAreSignalAccessesAndEqual(const syrec::expression& lhsOperand, const syrec::expression& rhsOperand) const;


        [[nodiscard]] OptimizationResult<syrec::VariableAccess> handleSignalAccess(const syrec::VariableAccess& signalAccess, bool performConstantPropagationForAccessedSignal, std::optional<unsigned int>* fetchedValue) const;
        static void filterAssignmentsThatDoNotChangeAssignedToSignal(syrec::Statement::vec& assignmentsToCheck);

        struct SignalIdentCountLookup {
            std::map<std::string_view, std::size_t, std::less<void>> lookup;
        };
        static void                                     determineUsedSignalIdentsIn(const syrec::expression& expr, SignalIdentCountLookup& lookupContainer);
        static void                                     determineUsedSignalIdentsIn(const syrec::Number& number, SignalIdentCountLookup& lookupContainer);
        static void                                     addSignalIdentToLookup(const std::string_view& signalIdent, SignalIdentCountLookup& lookupContainer);
        [[nodiscard]] static std::vector<std::reference_wrapper<const syrec::Statement>> transformCollectionOfSharedPointersToReferences(const syrec::Statement::vec& statements);
        void                                                                             decrementReferenceCountsOfLoopHeaderComponents(const syrec::Number& iterationRangeStart, const std::optional<std::reference_wrapper<const syrec::Number>>& iterationRangeEnd, const syrec::Number& stepSize) const;
        [[nodiscard]] static syrec::Statement::vec                                       createStatementListFrom(const syrec::Statement::vec& originalStmtList, std::vector<std::unique_ptr<syrec::Statement>> simplifiedStmtList);
        void                                                                             updateValueOfLoopVariable(const std::string_view& loopVariableIdent, std::optional<unsigned int> value) const;
        void                                                                             removeLoopVariableFromSymbolTable(const std::string_view& loopVariableIdent) const;
        [[nodiscard]] bool                                                               isAnyModifiableParameterOrLocalModifiedInModuleBody(const syrec::Module& module, bool areCallerArgumentsBasedOnOptimizedSignature) const;
        [[nodiscard]] bool                                                               isAnyModifiableParameterOrLocalModifiedInStatements(const std::vector<std::reference_wrapper<const syrec::Statement>>& statements, const std::unordered_set<std::string>& parameterAndLocalLookup, bool areCallerArgumentsBasedOnOptimizedSignature) const;
        [[nodiscard]] bool                                                               isAnyModifiableParameterOrLocalModifiedInStatement(const syrec::Statement& statement, const std::unordered_set<std::string>& parameterAndLocalLookup, bool areCallerArgumentsBasedOnOptimizedSignature) const;
        [[nodiscard]] static bool                                                        isVariableReadOnly(const syrec::Variable& variable);
        [[nodiscard]] static std::optional<bool>                                         determineEquivalenceOfOperandsOfBinaryExpr(const syrec::BinaryExpression& binaryExpression);
        [[nodiscard]] bool                                                               doesStatementDefineAssignmentThatChangesAssignedToSignal(const syrec::Statement& statement, bool areCallerArgumentsBasedOnOptimizedSignature);
        void                                                                             emplaceSingleSkipStatementIfContainerIsEmpty(std::optional<std::vector<std::unique_ptr<syrec::Statement>>>& statementsContainer);
        [[nodiscard]] std::size_t                                                        incrementInternalIfStatementNestingLevelCounter();
        void                                                                             decrementInternalIfStatementNestingLevelCounter();

        [[nodiscard]] std::unordered_set<std::size_t>                              determineIndicesOfUnusedModules(const std::vector<parser::SymbolTable::ModuleCallSignature>& unoptimizedModuleCallSignatures) const;
        [[nodiscard]] OptimizationResult<syrec::Module>                            removeUnusedModulesFrom(const std::vector<std::reference_wrapper<const syrec::Module>>& unoptimizedModules, const ExpectedMainModuleCallSignature& expectedMainModuleSignature) const;
        [[nodiscard]] OptimizationResult<syrec::Module>                            removeUnusedModulesFrom(const std::vector<parser::SymbolTable::ModuleCallSignature>& unoptimizedModuleCallSignatures, std::vector<std::unique_ptr<syrec::Module>>& optimizedModules, const ExpectedMainModuleCallSignature& expectedMainModuleSignature) const;
        [[nodiscard]] bool                                                         canModuleCallBeRemovedWhenIgnoringReferenceCount(const parser::SymbolTable::ModuleIdentifier& moduleIdentifier, const parser::SymbolTable::ModuleCallSignature& userProvidedCallSignature) const;
        void                                                                       markLoopVariableAsEvaluableIfConstantPropagationIsDisabled(const std::string& loopVariableIdent);
        void                                                                       unmarkLoopVariableAsEvaluableIfConstantPropagationIsDisabled(const std::string& loopVariableIdent);
        [[nodiscard]] static bool                                                  canLoopVariableBeEvaluated(const std::string& loopVariableIdent, bool isConstantPropagationEnabled, const std::unordered_set<std::string>& evaluableLoopVariablesIfConstantPropagationIsDisabled);
        [[nodiscard]] static bool                                                  doesModuleMatchMainModuleSignature(const parser::SymbolTable::ModuleCallSignature& moduleSignature, const ExpectedMainModuleCallSignature& expectedMainModuleSignature);
        [[nodiscard]] static std::optional<ExpectedMainModuleCallSignature>        determineActualMainModuleName(const std::vector<std::reference_wrapper<const syrec::Module>>& unoptimizedModules, const std::string_view& defaultMainModuleIdent);
        [[nodiscard]] static std::vector<parser::SymbolTable::ModuleCallSignature> createCallSignaturesFrom(const std::vector<std::reference_wrapper<const syrec::Module>>& modules);

        [[nodiscard]] static bool doesNumberContainPotentiallyDangerousComponent(const syrec::Number& number);
        [[nodiscard]] static bool doesSignalAccessContainPotentiallyDangerousComponent(const syrec::VariableAccess& signalAccess);
        [[nodiscard]] static bool doesExprContainPotentiallyDangerousComponent(const syrec::expression& expr);
        [[nodiscard]] static bool doesStatementContainPotentiallyDangerousComponent(const syrec::Statement& statement, const parser::SymbolTable& symbolTable);
        [[nodiscard]] static bool doesModuleContainPotentiallyDangerousComponent(const syrec::Module& module, const parser::SymbolTable& symbolTable);
        [[nodiscard]] static bool doesOptimizedModuleSignatureContainNoParametersOrOnlyReadonlyOnes(const parser::SymbolTable::ModuleCallSignature& moduleCallSignature);
        [[nodiscard]] static bool doesOptimizedModuleBodyContainAssignmentToModifiableParameter(const syrec::Module& module, const parser::SymbolTable& symbolTable);
        [[nodiscard]] static bool doesStatementDefineAssignmentToModifiableParameter(const syrec::Statement& statement, const std::unordered_set<std::string>& identsOfModifiableParameters, const parser::SymbolTable& symbolTable);
        [[nodiscard]] static bool isModifiableParameterAccessed(const syrec::VariableAccess& signalAccess, const std::unordered_set<std::string>& identsOfModifiableParameters);

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
    private:
        [[nodiscard]] const parser::SymbolTable* getActiveSymbolTableForEvaluation() const;
    };
}
#endif