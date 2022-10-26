#include "core/syrec/parser/custom_semantic_errors.hpp"

namespace parser {
    /* Variable declaration and access error messages */
    extern const std::string          UndeclaredIdent                 = "undeclared ident '{0:s}'";
    extern const std::string DuplicateDeclarationOfIdent     = "variable '{0:s}' was already declared";
    extern const std::string BitAccessOutOfRange             = "access on bit {0:d} is out of range for variable '{1:s}', valid value range is [{2:d}, {3:d}]";
    extern const std::string BitRangeOutOfRange              = "bit range {0:d}:{1:d} is out of range for variable '{2:s}', valid value range is [{3:d}, {4:d}]";
    extern const std::string BitRangeStartLargerThanEnd      = "bit range end value {0:d} cannot be larger than the start value {0:d}";
    extern const std::string DimensionOutOfRange             = "dimension with index {0:d} is out of range for variable '{1:s}' since it only defined {2:d} dimensions";
    extern const std::string DimensionValueOutOfRange        = "value {0:d} for dimension {1:d} is out of range for variable '{2:s}', valid value range is [{3:d}, {4:d}]";
    extern const std::string VariableIsNotLoopVariable       = "variable '{0:s}' is not a loop variable";
    extern const std::string InvalidParameterType            = "invalid parameter type";
    extern const std::string InvalidLocalType                = "invalid local type";
    extern const std::string DuplicateModuleIdentDeclaration = "module '{0:s}' was already declared";

    extern const std::string AssignmentToReadonlyVariable = "variable '{0:s}' is readonly";

    extern const std::string EmptyStatementList = "statement list must not be empty";

    extern const std::string IfAndFiConditionMissmatch = "Missmatch between expression used in if and corresponding fi condition";

    extern const std::string ShiftAmountNegative = "expected a shift by a positive integer but instead was {0:d}";

    extern const std::string FullSignalStructureMissmatch = "when applying an operation on two full signals, the number as well as the value per dimension must match";

    extern const std::string InvalidLoopVariableValueRange = "invalid loop variable value range for given values start: {0:d}, end: {1:d}, stepsize: {2:d}";

    extern const std::string InvalidSwapSignalWidthMissmatch       = "invalid swap signal width missmatch, first operand had a signal width of {0:d} while the other had a signal width of {1:d}";
    extern const std::string InvalidSwapNumDimensionsMissmatch     = "invalid swap number of dimension missmatch, first operand had {0:d} while the other had {1:d}";
    extern const std::string InvalidSwapValueForDimensionMissmatch = "invalid swap missmatch in value for dimension, first operand had a value of {0:d} defined for dimension {1:d} while the other had a value of {2:d}";

    /* Invalid operation error messages */
    extern const std::string InvalidBinaryOperation                    = "invalid binary operation";
    extern const std::string InvalidUnaryOperation                     = "invalid unary operation";
    extern const std::string InvalidShiftOperation                     = "invalid shift operation";
    extern const std::string InvalidOrMissingNumberExpressionOperation = "invalid or missing number expression operation";

    /* Call | uncall error messages */
    extern const std::string CallWithInvalidNumberOfArguments        = "module '{0:s}' defined {1:d} formal parameters but call only provided {2:d} parameter values";
    extern const std::string UncallWithInvalidNumberOfArguments      = "module '{0:s}' defined {1:d} formal parameters but uncall only provided {2:d} parameter values";
    extern const std::string MissingUncall                           = "missing uncall for module '{0:s}' called with arguments {1:s}";
    extern const std::string CallAndUncallNumberOfArgumentsMissmatch = "missmatch between number of parameter values for call and uncall of module '{0:s}', call had {0:d} parameters while uncall had {1:d}";
    extern const std::string CallAndUncallArgumentsMissmatch         = "call and uncall of module '{0:s} defined different parameter values ";

    /* Arithmetic errors */
    extern const std::string DivisionByZero = "invalid division by zero";

    /* Internal parser errors */
    extern const std::string NoMappingForLoopVariable = "no mapping of loop variable {0:s} to a value was found";
} // namespace parser
