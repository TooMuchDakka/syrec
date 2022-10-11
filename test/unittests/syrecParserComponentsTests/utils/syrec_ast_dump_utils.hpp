#ifndef SYREC_AST_DUMP_UTILS
#define SYREC_AST_DUMP_UTILS
#pragma once

#include "core/syrec/module.hpp"

namespace syrecAstDumpUtils {
    [[nodiscard]] std::string stringifyModules(const syrec::Module::vec& modules);
    [[nodiscard]] std::string stringifyModule(const syrec::Module::ptr& moduleToStringify);

    [[nodiscard]] std::string stringifyStatement(const syrec::Statement::ptr& statement);
    [[nodiscard]] std::string stringifySwapStatement(const syrec::SwapStatement& swapStmt);
    [[nodiscard]] std::string stringifyAssignStatement(const syrec::AssignStatement& assignStmt);
    [[nodiscard]] std::string stringifyCallStatement(const syrec::CallStatement &callStmt);
    [[nodiscard]] std::string stringifyUncallStatement(const syrec::UncallStatement& uncallStmt);
    [[nodiscard]] std::string stringifyUnaryStatement(const syrec::UnaryStatement& unaryStmt);
    [[nodiscard]] std::string stringifySkipStatement(const syrec::SkipStatement& skipStmt);
    [[nodiscard]] std::string stringifyForStatement(const syrec::ForStatement& forStmt);
    [[nodiscard]] std::string stringifyIfStatement(const syrec::IfStatement& ifStmt);

    [[nodiscard]] std::string stringifyExpression(const syrec::expression::ptr& expression);
    [[nodiscard]] std::string stringifyBinaryExpression(const syrec::BinaryExpression::ptr& binaryExpr);
    [[nodiscard]] std::string stringifyNumericExpression(const syrec::NumericExpression::ptr& numericExpr);
    [[nodiscard]] std::string stringifyShiftExpression(const syrec::ShiftExpression::ptr& shiftExpr);
    [[nodiscard]] std::string stringifyVariableExpression(const syrec::VariableExpression&& variableExpression);

    [[nodiscard]] std::string stringifyVariable(const syrec::Variable& variable);
    [[nodiscard]] std::string stringifyVariableAccess(const syrec::VariableAccess& variableAccess);
    [[nodiscard]] std::string stringifyNumber(const syrec::Number& number);
}

#endif