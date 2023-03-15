#ifndef REASSOCIATE_EXPRESSION_HPP
#define REASSOCIATE_EXPRESSION_HPP
#pragma once

#include "core/syrec/expression.hpp"

namespace optimization {
    syrec::expression::ptr simplifyBinaryExpression(const syrec::expression::ptr& expr);
}

#endif