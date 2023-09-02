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

        [[nodiscard]] ListOfStatementReferences handleStatements(const std::vector<const syrec::Statement&>& statements);
        [[nodiscard]] ListOfStatementReferences handleStatement(const syrec::Statement& stmt);
    protected:
        parser::ParserConfig                 parserConfig;
        const parser::SymbolTable::ptr       symbolTable;
        
        [[nodiscard]] std::unique_ptr<syrec::Module> handleModule(const syrec::Module& module);

        [[nodiscard]] std::unique_ptr<syrec::Statement> handleUnaryStmt(const syrec::UnaryStatement& unaryStmt);
        [[nodiscard]] ListOfStatementReferences         handleAssignmentStmt(const syrec::AssignStatement& assignmentStmt);
        [[nodiscard]] ListOfStatementReferences         handleCallStmt(const syrec::CallStatement& callStatement);
        [[nodiscard]] ListOfStatementReferences         handleIfStmt(const syrec::IfStatement& ifStatement);
        [[nodiscard]] ListOfStatementReferences         handleLoopStmt(const syrec::ForStatement& forStatement);
        [[nodiscard]] ListOfStatementReferences         handleSwapStmt(const syrec::SwapStatement& swapStatement);

        [[nodiscard]] std::unique_ptr<syrec::expression>        handleExpr(const syrec::expression& expression);
        [[nodiscard]] std::unique_ptr<syrec::expression>        handleBinaryExpr(const syrec::BinaryExpression& expression);
        [[nodiscard]] std::unique_ptr<syrec::expression>        handleShiftExpr(const syrec::ShiftExpression& expression);
        [[nodiscard]] std::unique_ptr<syrec::expression>        handleVariableExpr(const syrec::VariableExpression& expression);
        [[nodiscard]] std::unique_ptr<syrec::NumericExpression> handleNumericExpr(const syrec::NumericExpression& numericExpr);

        [[nodiscard]] std::unique_ptr<syrec::Number> handleNumber(const syrec::Number& number);

        enum ReferenceCountUpdate {
            Increment,
            Decrement
        };
        void updateReferenceCountOf(const std::string_view& signalIdent, ReferenceCountUpdate typeOfUpdate);

        /*std::vector<std::string> errors;
        std::vector<std::string> warnings;
        std::vector<std::string> information;

        enum MessageSeverity {
            Error,
            Warning,
            Info
        };
        void createMessage(const std::string& message, MessageSeverity severity);*/
    private:
    };
}
#endif