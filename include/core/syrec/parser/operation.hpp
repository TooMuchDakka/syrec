#ifndef OPERATION_HPP
#define OPERATION_HPP
#include "core/syrec/number.hpp"

#include <optional>

namespace syrec_operation {
    enum class operation {
        // Binary expression operations
        // BEGIN: Number operations
        Addition,
        Subtraction,
        Multiplication,
        Division,
        // END: Number operations
        Modulo,
        UpperBitsMultiplication,
        BitwiseXor,
        LogicalAnd,
        LogicalOr,
        BitwiseAnd,
        BitwiseOr,
        LessThan,
        GreaterThan,
        Equals,
        NotEquals,
        LessEquals,
        GreaterEquals,
        // Unary statement operations
        IncrementAssign,
        DecrementAssign,
        InvertAssign,
        //Assign statement operations
        AddAssign,
        MinusAssign,
        XorAssign,
        // Unary expression operations
        BitwiseNegation,
        LogicalNegation,
        // Shift expression operations
        ShiftLeft,
        ShiftRight
    };

    [[nodiscard]] bool                        isOperandUsedAsLhsInOperationIdentityElement(operation operation, unsigned int operand) noexcept;
    [[nodiscard]] bool                        isOperandUseAsRhsInOperationIdentityElement(operation operation, unsigned int operand) noexcept;
    [[nodiscard]] std::optional<unsigned int> apply(operation operation, unsigned int leftOperand, unsigned int rightOperand) noexcept;
    [[nodiscard]] std::optional<unsigned int> apply(operation operation, const std::optional<unsigned int>& leftOperand, const std::optional<unsigned int>& rightOperand) noexcept;
    [[nodiscard]] std::optional<unsigned int> apply(operation operation, unsigned int operand) noexcept;
    [[nodiscard]] bool                        isCommutative(operation operation) noexcept;
    [[nodiscard]] std::optional<operation>    invert(operation operation) noexcept;
    [[nodiscard]] std::optional<operation>    getMatchingAssignmentOperationForOperation(operation operation) noexcept;

    [[nodiscard]] std::optional<operation> tryMapBinaryOperationFlagToEnum(unsigned int binaryOperation) noexcept;
    [[nodiscard]] std::optional<operation> tryMapShiftOperationFlagToEnum(unsigned int shiftOperation) noexcept;
    [[nodiscard]] std::optional<operation> tryMapAssignmentOperationFlagToEnum(unsigned int assignmentOperation) noexcept;
    [[nodiscard]] std::optional<operation> tryMapUnaryAssignmentOperationFlagToEnum(unsigned int unaryAssignmentOperation) noexcept;
    [[nodiscard]] std::optional<operation> tryMapCompileTimeConstantExprOperationFlagToEnum(syrec::Number::CompileTimeConstantExpression::Operation compileTimeConstantExprOperation) noexcept;
    // TODO: Add mappings for unary expression if support for it is added within syrec
    //[[nodiscard]] std::optional<unsigned int> tryMapUnaryExpressionOperationEnumToFlag(operation unaryExpressionOperation) noexcept;

    [[nodiscard]] std::optional<unsigned int> tryMapBinaryOperationEnumToFlag(operation binaryOperation) noexcept;
    [[nodiscard]] std::optional<unsigned int> tryMapShiftOperationEnumToFlag(operation shiftOperation) noexcept;
    [[nodiscard]] std::optional<unsigned int> tryMapAssignmentOperationEnumToFlag(operation assignmentOperation) noexcept;
    [[nodiscard]] std::optional<unsigned int> tryMapUnaryAssignmentOperationEnumToFlag(operation unaryAssignmentOperation) noexcept;
    // TODO: Add mappings for unary expression if support for it is added within syrec
    //[[nodiscard]] std::optional<unsigned int> tryMapUnaryAssignmentOperationEnumToFlag(operation unaryAssignmentOperation) noexcept;

    [[nodiscard]] bool isOperationAssignmentOperation(operation operationToCheck) noexcept;
    [[nodiscard]] bool isOperationUnaryAssignmentOperation(operation operationToCheck) noexcept;
    [[nodiscard]] bool isOperationRelationalOperation(operation operationToCheck) noexcept;
    [[nodiscard]] bool isOperationEquivalenceOperation(operation operationToCheck) noexcept;

    [[nodiscard]] std::optional<bool> determineBooleanResultIfOperandsOfBinaryExprMatchForOperation(operation operationToCheck) noexcept;
};

#endif