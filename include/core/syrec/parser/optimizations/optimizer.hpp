#ifndef OPTIMIZER_HPP
#define OPTIMIZER_HPP
#pragma once

#include "core/syrec/statement.hpp"
#include "core/syrec/parser/parser_config.hpp"
#include "core/syrec/parser/symbol_table.hpp"

namespace optimizations {
    class Optimizer {
    public:
        using ListOfStatementReferences = std::vector<std::unique_ptr<syrec::Statement>>;

        explicit Optimizer(const parser::ParserConfig& parserConfig, parser::SymbolTable::ptr sharedSymbolTableReference):
            parserConfig(parserConfig), symbolTable(std::move(sharedSymbolTableReference)) {}

        
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
                status(resultFlag), result(optimizationResult) {
                if (optimizationResult) {
                    result = std::make_optional(std::vector<std::unique_ptr<ToBeOptimizedContainerType>>(std::move(optimizationResult)));   
                }
            }

            OptimizationResult(OptimizationResultFlag resultFlag, std::vector<std::unique_ptr<ToBeOptimizedContainerType>> optimizationResult):
                status(resultFlag), result(std::make_optional(std::move(optimizationResult))) {}

            OptimizationResult(OptimizationResultFlag resultFlag, std::nullptr_t emptyOptimizationResult):
                status(resultFlag), result(std::nullopt) {}
        private:
            OptimizationResultFlag                                                  status;
            std::optional<std::vector<std::unique_ptr<ToBeOptimizedContainerType>>> result;
        };

        [[nodiscard]] OptimizationResult<syrec::Module>       optimizeProgram(const std::vector<std::reference_wrapper<syrec::Module>>& modules);
        [[nodiscard]] OptimizationResult<syrec::Statement>    handleStatements(const std::vector<std::reference_wrapper<syrec::Statement>>& statements);
        [[nodiscard]] OptimizationResult<syrec::Statement>    handleStatement(const syrec::Statement& stmt);
        [[nodiscard]] OptimizationResult<syrec::expression>   handleExpr(const syrec::expression& expression) const;
        [[nodiscard]] OptimizationResult<syrec::Number>       handleNumber(const syrec::Number& number) const;
    protected:
        parser::ParserConfig                 parserConfig;
        const parser::SymbolTable::ptr       symbolTable;
        
        [[nodiscard]] OptimizationResult<syrec::Module> handleModule(const syrec::Module& module);

        [[nodiscard]] OptimizationResult<syrec::Statement>               handleUnaryStmt(const syrec::UnaryStatement& unaryStmt);
        [[nodiscard]] OptimizationResult<syrec::Statement>               handleAssignmentStmt(const syrec::AssignStatement& assignmentStmt);
        [[nodiscard]] OptimizationResult<syrec::Statement>               handleCallStmt(const syrec::CallStatement& callStatement);
        [[nodiscard]] OptimizationResult<syrec::Statement>               handleIfStmt(const syrec::IfStatement& ifStatement);
        [[nodiscard]] OptimizationResult<syrec::Statement>               handleLoopStmt(const syrec::ForStatement& forStatement);
        [[nodiscard]] OptimizationResult<syrec::Statement>               handleSwapStmt(const syrec::SwapStatement& swapStatement);
        [[nodiscard]] OptimizationResult<syrec::Statement>               handleSkipStmt(const syrec::SkipStatement& skipStatement);

        [[nodiscard]] OptimizationResult<syrec::expression>     handleBinaryExpr(const syrec::BinaryExpression& expression) const;
        [[nodiscard]] OptimizationResult<syrec::expression>     handleShiftExpr(const syrec::ShiftExpression& expression) const;
        [[nodiscard]] OptimizationResult<syrec::expression>     handleVariableExpr(const syrec::VariableExpression& expression) const;
        [[nodiscard]] OptimizationResult<syrec::expression>     handleNumericExpr(const syrec::NumericExpression& numericExpr) const;

        [[nodiscard]] static std::unique_ptr<syrec::Statement> createCopyOfStmt(const syrec::Statement& stmt);
        [[nodiscard]] static std::unique_ptr<syrec::expression> createCopyOfExpression(const syrec::expression& expr);
        [[nodiscard]] static std::unique_ptr<syrec::Number>     createCopyOfNumber(const syrec::Number& number);
        [[nodiscard]] static std::unique_ptr<syrec::Module>     createCopyOfModule(const syrec::Module& module);

        enum ReferenceCountUpdate {
            Increment,
            Decrement
        };
        void updateReferenceCountOf(const std::string_view& signalIdent, ReferenceCountUpdate typeOfUpdate) const;
        [[nodiscard]] std::optional<unsigned int> tryFetchValueForAccessedSignal(const syrec::VariableAccess& accessedSignal) const;

        enum IndexValidityStatus {
            Valid,
            Unknown,
            OutOfRange
        };

        struct DimensionAccessEvaluationResult {
            const bool wasTrimmed;
            const std::vector<std::pair<IndexValidityStatus, std::optional<unsigned int>>> valuePerDimension;
        };
        [[nodiscard]] std::optional<DimensionAccessEvaluationResult> tryEvaluateUserDefinedDimensionAccess(const std::string_view& accessedSignalIdent, const std::vector<std::reference_wrapper<const syrec::expression>>& accessedValuePerDimension) const;
        [[nodiscard]] std::optional<std::pair<unsigned int, unsigned int>> tryEvaluateBitRangeAccessComponents(const std::pair<std::reference_wrapper<const syrec::Number>, std::reference_wrapper<const syrec::Number>>& accessedBitRange) const;
        [[nodiscard]] std::optional<unsigned int>                          tryEvaluateNumberAsConstant(const syrec::Number& number) const;
        [[nodiscard]] std::optional<unsigned int>                          tryEvaluateExpressionToConstant(const syrec::expression& expr) const;

        struct BitRangeEvaluationResult {
            const std::pair<IndexValidityStatus, std::optional<unsigned int>> rangeStartEvaluationResult;
            const std::pair<IndexValidityStatus, std::optional<unsigned int>> rangeEndEvaluationResult;

            BitRangeEvaluationResult(
                    const IndexValidityStatus rangeStartEvaluationResultFlag, std::optional<unsigned int> rangeStartEvaluated,
                    const IndexValidityStatus rangeEndEvaluationResultFlag, std::optional<unsigned int> rangeEndEvaluated):
                rangeStartEvaluationResult(std::make_pair(rangeStartEvaluationResultFlag, rangeStartEvaluated)),
                rangeEndEvaluationResult(std::make_pair(rangeEndEvaluationResultFlag, rangeEndEvaluated)) {}
        };
        [[nodiscard]] std::optional<BitRangeEvaluationResult> tryEvaluateUserDefinedBitRangeAccess(const std::string_view& accessedSignalIdent, const std::optional<std::pair<std::reference_wrapper<const syrec::Number>, std::reference_wrapper<const syrec::Number>>>& accessedBitRange) const;
    private:
        template<class... Fs>
        struct Overload: Fs... { using Fs::operator()...; };

        template<class... Fs>
        Overload(Fs...) -> Overload<Fs...>;
    };
}
#endif