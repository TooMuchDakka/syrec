#ifndef BINARY_FACTORING_MULTIPLICATION_SIMPLIFIER_HPP
#define BINARY_FACTORING_MULTIPLICATION_SIMPLIFIER_HPP
#pragma once

#include "core/syrec/parser/optimizations/operationSimplification/binary_multiplication_simplifier.hpp"

namespace optimizations {
    class BinaryFactoringMultiplicationSimplifier : public BinaryMultiplicationSimplifier {
    public:
        [[nodiscard]] std::optional<syrec::expression::ptr> trySimplify(const std::shared_ptr<syrec::BinaryExpression>&) override;
    protected:
        std::map<unsigned int, std::vector<unsigned int>> factorizingCache;

        /**
         * \brief Factorize the given integer with factors of the form (2^i +/- 1) as recommended by Bernstein et al (see further: (https://inria.hal.science/inria-00072430/file/RR-4192.pdf)
         * \param integer The integer to be factorized
         * \return The found factors
         */
        [[nodiscard]] std::vector<unsigned int>             factorizeInteger(unsigned int integer);
        [[nodiscard]] static bool                           isNumberDividedByWithoutRemainder(unsigned int integer, unsigned int divisor);
        [[nodiscard]] static std::optional<unsigned int>    returnVariationOfFactorThatDividesNumber(unsigned int integer, unsigned int potentialFactorBase);
        static void                                         combineDuplicateFactors(std::vector<unsigned int>& foundFactors);
    };   
}
#endif