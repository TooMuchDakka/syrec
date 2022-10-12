#ifndef SYREC_AST_DUMP_UTILS
#define SYREC_AST_DUMP_UTILS
#pragma once

#include "algorithm"
#include "sstream"

#include "core/syrec/module.hpp"
#include "infix_iterator.hpp"

namespace syrecAstDumpUtils {
    template<class T>
    std::string stringifyAndJoinMany(const std::vector<T>& elements, const char* delimiter, std::string(*stringifyFunc)(const T& t)) {
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
        
        std::copy(stringifiedElements.cbegin(), stringifiedElements.cend(), infix_ostream_iterator<std::string>(resultBuffer, delimiter));
        return resultBuffer.str();
    }

    template<class T>
    const auto stringifyAndJoinManyComplex = stringifyAndJoinMany<std::shared_ptr<T>>;

    class SyrecASTDumper {
    private:
        inline static const std::string newlineSequence = "\n";
        inline static const std::string identationSequence  = "\t";
        inline static const std::string stmtDelimiter       = ";\n";
        inline static const std::string parameterDelimiter  = ", ";
        std::size_t       identationLevel     = 0;

    public:
        [[nodiscard]] std::string stringifyModules(const syrec::Module::vec& modules);
    protected:
        [[nodiscard]] std::string stringifyModule(const syrec::Module::ptr& moduleToStringify);

        [[nodiscard]] std::string stringifyStatements(const syrec::Statement::vec& statements);
        [[nodiscard]] std::string stringifyStatement(const syrec::Statement::ptr& statement);
        [[nodiscard]] static std::string stringifySwapStatement(const syrec::SwapStatement& swapStmt);
        [[nodiscard]] static std::string stringifyAssignStatement(const syrec::AssignStatement& assignStmt);
        [[nodiscard]] static std::string stringifyCallStatement(const syrec::CallStatement& callStmt);
        [[nodiscard]] static std::string stringifyUncallStatement(const syrec::UncallStatement& uncallStmt);
        [[nodiscard]] static std::string stringifyUnaryStatement(const syrec::UnaryStatement& unaryStmt);
        [[nodiscard]] static std::string stringifySkipStatement();
        [[nodiscard]] std::string stringifyForStatement(const syrec::ForStatement& forStmt);
        [[nodiscard]] std::string stringifyIfStatement(const syrec::IfStatement& ifStmt);

        [[nodiscard]] static std::string stringifyExpression(const syrec::expression::ptr& expression);
        [[nodiscard]] static std::string stringifyBinaryExpression(const syrec::BinaryExpression& binaryExpr);
        [[nodiscard]] static std::string stringifyNumericExpression(const syrec::NumericExpression& numericExpr);
        [[nodiscard]] static std::string stringifyShiftExpression(const syrec::ShiftExpression& shiftExpr);
        [[nodiscard]] static std::string stringifyVariableExpression(const syrec::VariableExpression& variableExpression);

        [[nodiscard]] static std::string stringifyVariable(const syrec::Variable::ptr& variable);
        [[nodiscard]] static std::string stringifyVariableAccess(const syrec::VariableAccess::ptr& variableAccess);
        [[nodiscard]] static std::string stringifyNumber(const syrec::Number::ptr& number);

        [[nodiscard]] static std::string stringifyDimensionExpression(const syrec::expression::ptr& expr);
        [[nodiscard]] static std::string stringifyDimension(const unsigned int& dimension);

        [[nodiscard]] static std::string repeatNTimes(const std::string& str, const size_t numRepetitions);
    };
}

#endif