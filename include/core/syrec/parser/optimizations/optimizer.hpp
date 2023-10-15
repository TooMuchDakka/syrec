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

        [[nodiscard]] OptimizationResult<syrec::Module>       optimizeProgram(const std::vector<std::reference_wrapper<const syrec::Module>>& modules, const std::optional<std::string>& userDefinedExpectedMainModuleIdent);
        [[nodiscard]] OptimizationResult<syrec::Statement>    handleStatements(const std::vector<std::reference_wrapper<const syrec::Statement>>& statements);
        [[nodiscard]] OptimizationResult<syrec::Statement>    handleStatement(const syrec::Statement& stmt);
        [[nodiscard]] OptimizationResult<syrec::expression>   handleExpr(const syrec::expression& expression) const;
        [[nodiscard]] OptimizationResult<syrec::Number>       handleNumber(const syrec::Number& number) const;
    protected:
        parser::ParserConfig                                         parserConfig;
        parser::SymbolTable::ptr                                     activeSymbolTableScope;
        std::stack<std::unique_ptr<parser::SymbolTableBackupHelper>> symbolTableBackupScopeStack;
        std::unique_ptr<LoopBodyValuePropagationBlocker>             activeDataFlowValuePropagationRestrictions;

        void openNewSymbolTableBackupScope();
        void updateBackupOfValuesChangedInScopeAndResetMadeChanges() const;
        [[nodiscard]] std::optional<std::reference_wrapper<const parser::SymbolTableBackupHelper>> peekCurrentSymbolTableBackupScope() const;

        void                                                                          mergeAndMakeLocalChangesGlobal(const parser::SymbolTableBackupHelper& backupOfSignalsChangedInFirstBranch, const parser::SymbolTableBackupHelper& backupOfSignalsChangedInSecondBranch) const;
        void                                                                          makeLocalChangesGlobal(const parser::SymbolTableBackupHelper& backupOfCurrentValuesOfSignalsAtEndOfBranch, const parser::SymbolTableBackupHelper& backupCurrentValuesOfSignalsInToBeOmittedBranch) const;
        void                                                                          destroySymbolTableBackupScope();
        void                                                                          createBackupOfAssignedToSignal(const syrec::VariableAccess& assignedToVariableAccess) const;
        [[nodiscard]] std::optional<std::unique_ptr<parser::SymbolTableBackupHelper>> popCurrentSymbolTableBackupScope();

        [[nodiscard]] OptimizationResult<syrec::Module> handleModule(const syrec::Module& module, const std::optional<std::string>& optionalMainModuleIdent);

        [[nodiscard]] OptimizationResult<syrec::Statement>               handleUnaryStmt(const syrec::UnaryStatement& unaryStmt) const;
        [[nodiscard]] OptimizationResult<syrec::Statement>               handleAssignmentStmt(const syrec::AssignStatement& assignmentStmt) const;
        [[nodiscard]] OptimizationResult<syrec::Statement>               handleCallStmt(const syrec::CallStatement& callStatement) const;
        [[nodiscard]] OptimizationResult<syrec::Statement>               handleUncallStmt(const syrec::UncallStatement& callStatement) const;
        [[nodiscard]] OptimizationResult<syrec::Statement>               handleIfStmt(const syrec::IfStatement& ifStatement);
        [[nodiscard]] OptimizationResult<syrec::Statement>               handleLoopStmt(const syrec::ForStatement& forStatement);
        [[nodiscard]] OptimizationResult<syrec::Statement>               handleSwapStmt(const syrec::SwapStatement& swapStatement) const;
        [[nodiscard]] static OptimizationResult<syrec::Statement>        handleSkipStmt();

        [[nodiscard]] OptimizationResult<syrec::expression>     handleBinaryExpr(const syrec::BinaryExpression& expression) const;
        [[nodiscard]] OptimizationResult<syrec::expression>     handleShiftExpr(const syrec::ShiftExpression& expression) const;
        [[nodiscard]] OptimizationResult<syrec::expression>     handleVariableExpr(const syrec::VariableExpression& expression) const;
        [[nodiscard]] OptimizationResult<syrec::expression>     handleNumericExpr(const syrec::NumericExpression& numericExpr) const;

        [[nodiscard]] static std::vector<std::unique_ptr<syrec::Statement>> createCopyOfStatements(const syrec::Statement::vec& statements);
        [[nodiscard]] static std::unique_ptr<syrec::Statement>  createCopyOfStmt(const syrec::Statement& stmt);
        [[nodiscard]] static std::unique_ptr<syrec::expression> createCopyOfExpression(const syrec::expression& expr);
        [[nodiscard]] static std::unique_ptr<syrec::Number>     createCopyOfNumber(const syrec::Number& number);
        [[nodiscard]] static std::unique_ptr<syrec::Module>     createCopyOfModule(const syrec::Module& module);


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
        //void updateReferenceCountsOfSignalIdentsUsedIn(const syrec::Statement& statement, parser::SymbolTable::ReferenceCountUpdate typeOfUpdate) const;

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

            [[nodiscard]] static SignalAccessIndexEvaluationResult<T> createFromConstantValue(IndexValidityStatus validityStatus, unsigned int constantValue) {
                return SignalAccessIndexEvaluationResult<T>({.validityStatus = validityStatus, .constantValue = constantValue, .simplifiedNonConstantValue = nullptr, .originalValue = nullptr });
            }

            [[nodiscard]] static SignalAccessIndexEvaluationResult<T> createFromSimplifiedNonConstantValue(IndexValidityStatus validityStatus, std::unique_ptr<T> nonConstantValue) {
                return SignalAccessIndexEvaluationResult<T>({.validityStatus = validityStatus, .constantValue = std::nullopt, .simplifiedNonConstantValue = std::move(nonConstantValue), .originalValue = nullptr });
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
        };
        [[nodiscard]] DimensionAccessEvaluationResult                      evaluateUserDefinedDimensionAccess(const std::string_view& accessedSignalIdent, const std::vector<std::reference_wrapper<const syrec::expression>>& accessedValuePerDimension) const;
        [[nodiscard]] std::optional<std::pair<unsigned int, unsigned int>> tryEvaluateBitRangeAccessComponents(const std::pair<std::reference_wrapper<const syrec::Number>, std::reference_wrapper<const syrec::Number>>& accessedBitRange) const;
        [[nodiscard]] std::optional<unsigned int>                          tryEvaluateNumberAsConstant(const syrec::Number& number) const;
        [[nodiscard]] std::optional<unsigned int>                          tryEvaluateExpressionToConstant(const syrec::expression& expr) const;

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

        void                                                        invalidateValueOfAccessedSignalParts(const EvaluatedSignalAccess& accessedSignalParts) const;
        void                                                        invalidateValueOfWholeSignal(const std::string_view& signalIdent) const;
        void                                                        performAssignment(const EvaluatedSignalAccess& assignedToSignalParts, syrec_operation::operation assignmentOperation, const std::optional<unsigned int>& assignmentRhsValue) const;
        void                                                        performAssignment(EvaluatedSignalAccess& assignmentLhsOperand, syrec_operation::operation assignmentOperation, EvaluatedSignalAccess& assignmentRhsOperand) const;
        void                                                        performSwap(const EvaluatedSignalAccess& swapOperationLhsOperand, const EvaluatedSignalAccess& swapOperationRhsOperand) const;
        void                                                        invalidateStoredValueFor(const syrec::VariableAccess::ptr& assignedToSignal) const;
        void                                                        updateStoredValueOf(const syrec::VariableAccess::ptr& assignedToSignal, unsigned int newValueOfAssignedToSignal) const;
        void                                                        performSwapAndCreateBackupOfOperands(const syrec::VariableAccess::ptr& swapLhsOperand, const syrec::VariableAccess::ptr& swapRhsOperand) const;

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
        [[nodiscard]] bool                                                               isAnyModifiableParameterOrLocalModifiedInModuleBody(const syrec::Module& module) const;
        [[nodiscard]] bool                                                               isAnyModifiableParameterOrLocalModifiedInStatements(const std::vector<std::reference_wrapper<const syrec::Statement>>& statements, const std::unordered_set<std::string>& parameterAndLocalLookup) const;
        [[nodiscard]] bool                                                               isAnyModifiableParameterOrLocalModifiedInStatement(const syrec::Statement& statement, const std::unordered_set<std::string>& parameterAndLocalLookup) const;
        [[nodiscard]] static bool                                                        isVariableReadOnly(const syrec::Variable& variable);
        [[nodiscard]] static std::optional<bool>                                         determineEquivalenceOfOperandsOfBinaryExpr(const syrec::BinaryExpression& binaryExpression);
        [[nodiscard]] static bool                                                        doesCurrentModuleIdentMatchUserDefinedMainModuleIdent(const std::string_view& currentModuleIdent, const std::optional<std::string>& userDefinedMainModuleIdent);
        [[nodiscard]] bool                                                               doesStatementDefineAssignmentThatChangesAssignedToSignal(const syrec::Statement& statement);

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
    };
}
#endif