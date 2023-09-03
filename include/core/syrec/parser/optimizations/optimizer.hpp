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

        [[nodiscard]] std::vector<std::unique_ptr<syrec::Module>> optimizeProgram(const std::vector<std::reference_wrapper<syrec::Module>>& modules);
        [[nodiscard]] ListOfStatementReferences handleStatements(const std::vector<std::reference_wrapper<syrec::Statement>>& statements);
        [[nodiscard]] ListOfStatementReferences handleStatement(const syrec::Statement& stmt);
        [[nodiscard]] std::unique_ptr<syrec::expression> handleExpr(const syrec::expression& expression) const;
        [[nodiscard]] std::unique_ptr<syrec::Number> handleNumber(const syrec::Number& number) const;
    protected:
        parser::ParserConfig                 parserConfig;
        const parser::SymbolTable::ptr       symbolTable;
        
        [[nodiscard]] std::unique_ptr<syrec::Module> handleModule(const syrec::Module& module);

        [[nodiscard]] std::unique_ptr<syrec::Statement> handleUnaryStmt(const syrec::UnaryStatement& unaryStmt);
        [[nodiscard]] ListOfStatementReferences         handleAssignmentStmt(const syrec::AssignStatement& assignmentStmt);
        [[nodiscard]] ListOfStatementReferences         handleCallStmt(const syrec::CallStatement& callStatement);
        [[nodiscard]] ListOfStatementReferences         handleIfStmt(const syrec::IfStatement& ifStatement);
        [[nodiscard]] ListOfStatementReferences         handleLoopStmt(const syrec::ForStatement& forStatement);
        [[nodiscard]] std::unique_ptr<syrec::Statement> handleSwapStmt(const syrec::SwapStatement& swapStatement);
        [[nodiscard]] std::unique_ptr<syrec::Statement> handleSkipStmt(const syrec::SkipStatement& skipStatement);

        [[nodiscard]] std::unique_ptr<syrec::expression>        handleBinaryExpr(const syrec::BinaryExpression& expression) const;
        [[nodiscard]] std::unique_ptr<syrec::expression>        handleShiftExpr(const syrec::ShiftExpression& expression) const;
        [[nodiscard]] std::unique_ptr<syrec::expression>        handleVariableExpr(const syrec::VariableExpression& expression) const;
        [[nodiscard]] std::unique_ptr<syrec::NumericExpression> handleNumericExpr(const syrec::NumericExpression& numericExpr) const;

        enum ReferenceCountUpdate {
            Increment,
            Decrement
        };
        void updateReferenceCountOf(const std::string_view& signalIdent, ReferenceCountUpdate typeOfUpdate) const;
        [[nodiscard]] std::optional<unsigned int> tryFetchValueForAccessedSignal(const syrec::VariableAccess& accessedSignal) const;

        struct DimensionAccessEvaluationResult {
            using ValueOfDimension = std::variant<std::shared_ptr<syrec::expression>, unsigned int>;
            enum ValueOfDimensionValidity {
                Valid,
                Unknown,
                OutOfRange
            };

            bool wasTrimmed;
            std::vector<std::pair<ValueOfDimensionValidity, ValueOfDimension>> valuePerDimension;
        };

        [[nodiscard]] std::optional<DimensionAccessEvaluationResult> tryEvaluateUserDefinedDimensionAccess(const std::string_view& accessedSignalIdent, const std::vector<std::reference_wrapper<const syrec::expression>>& accessedValuePerDimension) const;
        [[nodiscard]] std::optional<std::pair<unsigned int, unsigned int>> tryEvaluateBitRangeAccessComponents(const std::pair<std::reference_wrapper<const syrec::Number>, std::reference_wrapper<const syrec::Number>>& accessedBitRange) const;
        [[nodiscard]] std::optional<unsigned int>                          tryEvaluateNumberAsConstant(const syrec::Number& number) const;
        [[nodiscard]] std::optional<unsigned int>                          tryEvaluateExpressionToConstant(const syrec::expression& expr) const;

        struct BitRangeEvaluationResult {
            enum BitRangeComponentEvaluationResultStatus {
                Valid,
                Unknown,
                OutOfRange
            };

            std::pair<BitRangeComponentEvaluationResultStatus, std::optional<unsigned int>> rangeStartEvaluationResult;
            std::pair<BitRangeComponentEvaluationResultStatus, std::optional<unsigned int>> rangeEndEvaluationResult;

            explicit BitRangeEvaluationResult(
                const BitRangeComponentEvaluationResultStatus rangeStartEvaluationResultFlag, std::optional<unsigned int> rangeStartEvaluated,
                const BitRangeComponentEvaluationResultStatus rangeEndEvaluationResultFlag, std::optional<unsigned int> rangeEndEvaluated) {
                rangeStartEvaluationResult = std::make_pair(rangeStartEvaluationResultFlag, rangeStartEvaluated);
                rangeEndEvaluationResult = std::make_pair(rangeEndEvaluationResultFlag, rangeEndEvaluated);
            }
        };

        [[nodiscard]] std::optional<BitRangeEvaluationResult> tryEvaluateUserDefinedBitRangeAccess(const std::string_view& accessedSignalIdent, const std::optional<std::pair<std::reference_wrapper<const syrec::Number>, std::reference_wrapper<const syrec::Number>>>& accessedBitRange) const;
        [[nodiscard]] std::unique_ptr<syrec::expression>      createCopyOfExpr(const syrec::expression& expr) const;
        /*using RelevantStatementTypes = std::variant<syrec::AssignStatement, syrec::CallStatement, syrec::ForStatement, syrec::IfStatement, syrec::SwapStatement, syrec::UnaryStatement>;

        template<class... Ts>
        struct overloaded : Ts... { using Ts::operator()...; };*/
    private:
    };
}
#endif