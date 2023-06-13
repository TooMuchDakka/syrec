#ifndef BINARY_MULTIPLICATION_SIMPLIFIER_HPP
#define BINARY_MULTIPLICATION_SIMPLIFIER_HPP
#pragma once

#include "core/syrec/parser/optimizations/operationSimplification/base_multiplication_simplifier.hpp"

namespace optimizations {
    class BinaryMultiplicationSimplifier : public BaseMultiplicationSimplifier {
    public:
        /*
         *  IGNORE LEFTMOST 1 bit and start at leftmost bit ignoring more significant zero bits (i.e. bits to the right of the leftmost one bit)
            0101 1101 = 93 * n

            4n = n << 2
            5n = 4n + n
            10n = 5n << 1
            11n = 10n + n
            22n = 11n << 1
            23n = 22n + n
            92n = 23n << 2
            93n = 92n + n
         */
        [[nodiscard]] std::optional<syrec::expression::ptr> trySimplify(const std::shared_ptr<syrec::BinaryExpression>&) override;

    protected:
        struct ShiftAndAddOrSubOperationStep {
            const std::size_t shiftAmount;
            const bool        performAddition;

            explicit ShiftAndAddOrSubOperationStep(std::size_t shiftAmount, bool performAddition):
                shiftAmount(shiftAmount), performAddition(performAddition) {}
        };
        std::map<unsigned int, std::vector<ShiftAndAddOrSubOperationStep>> cache;

        [[nodiscard]] static std::vector<std::size_t>                   extractOneBitPositionsOfNumber(unsigned int number);
        [[nodiscard]] static std::size_t                                determineShiftAmount(std::size_t currentOneBitPosition, std::optional<std::size_t> previousOneBitPosition);
        static void                                                     reverseAndUpdateBitPositionsOfNumber(std::vector<std::size_t>& oneBitPositionsOfNumber);
        [[nodiscard]] static std::vector<ShiftAndAddOrSubOperationStep> generateOperationSteps(std::size_t constantToSimplify, bool aggregateOneBitSequences);
        [[nodiscard]] static syrec::BinaryExpression::ptr               generateExpressionFromSimplificationSteps(const std::vector<ShiftAndAddOrSubOperationStep>& simplificationStepsOfConstantOperand, const syrec::expression::ptr& nonConstantOperand);
    };
}

#endif