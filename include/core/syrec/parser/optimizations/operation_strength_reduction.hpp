#ifndef OPERATOR_STRENGTH_REDUCTION_HPP
#define OPERATOR_STRENGTH_REDUCTION_HPP
#pragma once

#include "core/syrec/expression.hpp"

namespace optimizations {
    [[maybe_unused]] bool tryPerformOperationStrengthReduction(syrec::Expression::ptr& expr);
};

#endif
