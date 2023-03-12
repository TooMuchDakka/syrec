#ifndef REASSOCIATE_EXPRESSION_HPP
#define REASSOCIATE_EXPRESSION_HPP
#pragma once

#include "core/syrec/expression.hpp"
#include "core/syrec/parser/operation.hpp"

#include <optional>

namespace optimization {
    std::optional<unsigned int> trySimplifyConstantExpression(unsigned int lOperand, syrec_operation::operation operation, unsigned int rOperand);
    syrec::expression::ptr simplifyBinaryExpression(const syrec::BinaryExpression::ptr& expr);
}

#endif