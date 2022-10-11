#ifndef SYREC_AST_DUMP_UTILS
#define SYREC_AST_DUMP_UTILS
#pragma once

#include "algorithm"
#include "sstream"

#include "core/syrec/module.hpp"

namespace syrecAstDumpUtils {
    template<class T>
    std::string stringifyAndJoinMany(const std::vector<T>& elements, const char* delimiter, std::string (*stringifyFunc)(const T& t)) {
        if (elements.empty()) {
            return "";
        }

        std::ostringstream       resultBuffer{};
        std::vector<std::string> stringifiedElements{};

        std::transform(
                elements.cbegin(),
                elements.cend(),
                std::back_inserter(stringifiedElements),
                [stringifyFunc](const T& elem) { return stringifyFunc(elem); });

        std::copy(stringifiedElements.cbegin(), stringifiedElements.cend(), std::ostream_iterator<std::string>(resultBuffer, delimiter));
        return resultBuffer.str();
    }

    template<class T>
    const auto stringifyAndJoinManyComplex = stringifyAndJoinMany<std::shared_ptr<T>>;

    [[nodiscard]] std::string stringifyModules(const syrec::Module::vec& modules);
    [[nodiscard]] std::string stringifyModule(const syrec::Module::ptr& moduleToStringify);

    [[nodiscard]] std::string stringifyStatements(const syrec::Statement::vec& statements);
    [[nodiscard]] std::string stringifyStatement(const syrec::Statement::ptr& statement);
    [[nodiscard]] std::string stringifySwapStatement(const syrec::SwapStatement& swapStmt);
    [[nodiscard]] std::string stringifyAssignStatement(const syrec::AssignStatement& assignStmt);
    [[nodiscard]] std::string stringifyCallStatement(const syrec::CallStatement& callStmt);
    [[nodiscard]] std::string stringifyUncallStatement(const syrec::UncallStatement& uncallStmt);
    [[nodiscard]] std::string stringifyUnaryStatement(const syrec::UnaryStatement& unaryStmt);
    [[nodiscard]] std::string stringifySkipStatement(const syrec::SkipStatement& skipStmt);
    [[nodiscard]] std::string stringifyForStatement(const syrec::ForStatement& forStmt);
    [[nodiscard]] std::string stringifyIfStatement(const syrec::IfStatement& ifStmt);

    [[nodiscard]] std::string stringifyExpression(const syrec::expression::ptr& expression);
    [[nodiscard]] std::string stringifyBinaryExpression(const syrec::BinaryExpression& binaryExpr);
    [[nodiscard]] std::string stringifyNumericExpression(const syrec::NumericExpression& numericExpr);
    [[nodiscard]] std::string stringifyShiftExpression(const syrec::ShiftExpression& shiftExpr);
    [[nodiscard]] std::string stringifyVariableExpression(const syrec::VariableExpression& variableExpression);

    [[nodiscard]] std::string stringifyVariable(const syrec::Variable::ptr& variable);
    [[nodiscard]] std::string stringifyVariableAccess(const syrec::VariableAccess::ptr& variableAccess);
    [[nodiscard]] std::string stringifyNumber(const syrec::Number::ptr& number);
}

#endif