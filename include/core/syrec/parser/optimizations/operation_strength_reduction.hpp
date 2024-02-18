#ifndef OPERATOR_STRENGTH_REDUCTION_HPP
#define OPERATOR_STRENGTH_REDUCTION_HPP
#pragma once

#include "core/syrec/expression.hpp"
#include "core/syrec/statement.hpp"

namespace operationStrengthReduction {
    [[maybe_unused]] bool tryPerformOperationStrengthReduction(syrec::Expression::ptr& expr);

    struct AssignmentOperationStrengthReductionResult {
        std::variant<std::unique_ptr<syrec::AssignStatement>, std::unique_ptr<syrec::UnaryStatement>> result;

        explicit AssignmentOperationStrengthReductionResult(std::variant<std::unique_ptr<syrec::AssignStatement>, std::unique_ptr<syrec::UnaryStatement>> result)
            : result(std::move(result)) {}

        [[nodiscard]] std::optional<std::unique_ptr<syrec::UnaryStatement>> getResultAsUnaryAssignment();
        [[nodiscard]] std::optional<std::unique_ptr<syrec::AssignStatement>> getResultAsBinaryAssignment();
    };
    [[nodiscard]] std::optional<AssignmentOperationStrengthReductionResult> performOperationStrengthReduction(const syrec::AssignStatement& assignment, const std::optional<unsigned int>& currentValueOfAssignedToSignalPriorToAssignment);
};

#endif
