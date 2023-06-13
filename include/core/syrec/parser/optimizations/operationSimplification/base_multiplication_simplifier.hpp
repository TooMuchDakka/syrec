#ifndef BASE_MULTIPLICATION_SIMPLIFIER_HPP
#define BASE_MULTIPLICATION_SIMPLIFIER_HPP
#pragma once

#include "core/syrec/expression.hpp"

namespace optimizations {
	enum MultiplicationSimplificationMethod {
	    BinarySimplification,
		BinarySimplificationWithFactoring,
		None
	};

	class BaseMultiplicationSimplifier {
    public:
        virtual ~BaseMultiplicationSimplifier()                                                                                             = default;
        [[nodiscard]] virtual std::optional<syrec::expression::ptr> trySimplify(const std::shared_ptr<syrec::BinaryExpression>& binaryExpr) = 0;

	protected:
        [[nodiscard]] bool isOperationOfExpressionMultiplicationAndHasAtleastOneConstantOperand(const std::shared_ptr<syrec::BinaryExpression>& binaryExpr) const;
        [[nodiscard]] bool isOperandConstant(const syrec::expression::ptr& expressionOperand) const;
        [[nodiscard]] std::optional<unsigned int> tryDetermineValueOfConstant(const syrec::expression::ptr& operandOfBinaryExpr) const;
        [[nodiscard]] std::optional<syrec::expression::ptr> replaceMultiplicationWithShiftIfConstantTermIsPowerOfTwo(unsigned int constantTerm, const syrec::expression::ptr& nonConstantTerm) const;
	};
}

#endif