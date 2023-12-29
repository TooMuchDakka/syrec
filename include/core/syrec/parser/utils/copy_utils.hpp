#ifndef COPY_UTILS_HPP
#define COPY_UTILS_HPP
#pragma once

#include "core/syrec/module.hpp"

namespace copyUtils {
    [[nodiscard]] std::unique_ptr<syrec::Module>                 createCopyOfModule(const syrec::Module& module);
    [[nodiscard]] std::vector<std::unique_ptr<syrec::Statement>> createCopyOfStatements(const syrec::Statement::vec& statements);
    [[nodiscard]] std::unique_ptr<syrec::Statement>              createCopyOfStmt(const syrec::Statement& stmt);
    [[nodiscard]] std::unique_ptr<syrec::expression>             createCopyOfExpression(const syrec::expression& expr);
    [[nodiscard]] std::unique_ptr<syrec::Number>                 createCopyOfNumber(const syrec::Number& number);

    [[nodiscard]] std::unique_ptr<syrec::AssignStatement> createDeepCopyOfAssignmentStmt(const syrec::AssignStatement& stmt);
    [[nodiscard]] std::unique_ptr<syrec::expression>      createDeepCopyOfExpression(const syrec::expression& expr);
};
#endif