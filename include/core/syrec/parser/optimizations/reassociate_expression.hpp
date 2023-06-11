#ifndef REASSOCIATE_EXPRESSION_HPP
#define REASSOCIATE_EXPRESSION_HPP
#pragma once

#include "core/syrec/expression.hpp"
#include "operationSimplification/base_multiplication_simplifier.hpp"

namespace optimizations {
    syrec::expression::ptr simplifyBinaryExpression(const syrec::expression::ptr& expr, bool operationStrengthReductionEnabled, const std::optional<std::unique_ptr<optimizations::BaseMultiplicationSimplifier>>& optionalMultiplicationSimplifier);
}

#endif