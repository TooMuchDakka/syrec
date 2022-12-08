#include "core/syrec/parser/custom_semantic_errors.hpp"

namespace parser {
    /* Variable declaration and access error messages */
    extern const std::string UndeclaredIdent                 = "undeclared ident '{0:s}'";
    extern const std::string DuplicateDeclarationOfIdent     = "variable '{0:s}' was already declared";
    extern const std::string BitAccessOutOfRange             = "access on bit {0:d} is out of range for variable '{1:s}', valid value range is [{2:d}, {3:d}]";
    extern const std::string BitRangeOutOfRange              = "bit range {0:d}:{1:d} is out of range for variable '{2:s}', valid value range is [{3:d}, {4:d}]";
    extern const std::string BitRangeStartLargerThanEnd      = "bit range start value {0:d} cannot be larger than the end value {1:d}";
    extern const std::string DimensionOutOfRange             = "dimension with index {0:d} is out of range for variable '{1:s}' since it only defined {2:d} dimensions";
    extern const std::string DimensionValueOutOfRange        = "value {0:d} for dimension {1:d} is out of range for variable '{2:s}', valid value range is [{3:d}, {4:d}]";
    extern const std::string VariableIsNotLoopVariable       = "variable '{0:s}' is not a loop variable";
    extern const std::string InvalidParameterType            = "invalid parameter type";
    extern const std::string InvalidLocalType                = "invalid local type";
    extern const std::string DuplicateModuleIdentDeclaration = "module '{0:s}' was already declared";

    // TODO: Remove ?
    extern const std::string BrokeReversibilityOfAssignment = "Broke reversibility of assignment operation when using the assigned to variable '{0:s}' in the right hand side of the assignment";

    extern const std::string AssignmentToReadonlyVariable = "variable '{0:s}' is readonly";

    extern const std::string EmptyStatementList = "statement list must not be empty";

    extern const std::string IfAndFiConditionMissmatch = "Missmatch between expression used in if and corresponding fi condition";

    extern const std::string ShiftAmountNegative = "expected a shift by a positive integer but instead was {0:d}";

    extern const std::string FullSignalStructureMissmatch = "when applying an operation on two full signals, the number as well as the value per dimension must match";

    extern const std::string InvalidLoopVariableValueRange = "invalid loop variable value range for given values start: {0:d}, end: {1:d}, stepsize: {2:d}";

    extern const std::string InvalidSwapSignalWidthMissmatch       = "invalid swap signal width missmatch, first operand had a signal width of {0:d} while the other had a signal width of {1:d}";
    extern const std::string InvalidSwapNumDimensionsMissmatch     = "invalid swap number of dimension missmatch, first operand had {0:d} while the other had {1:d}";
    extern const std::string InvalidSwapValueForDimensionMissmatch = "invalid swap missmatch in value for dimension {0:d}, first operand had a value of {0:d} while the other had a value of {2:d}";

    /* Invalid operation error messages */
    extern const std::string InvalidAssignOperation                    = "Invalid assign operation";
    extern const std::string InvalidBinaryOperation                    = "invalid binary operation";
    extern const std::string InvalidUnaryOperation                     = "invalid unary operation";
    extern const std::string InvalidShiftOperation                     = "invalid shift operation";
    extern const std::string InvalidOrMissingNumberExpressionOperation = "invalid or missing number expression operation";

    /* Call | uncall error messages */
    extern const std::string PreviousCallWasNotUncalled              = "Tried to issue a call with one/many previous calls not being uncalled";
    extern const std::string UncallWithoutPreviousCall               = "Tried to uncall a module that was not previously called";
    extern const std::string MissmatchOfModuleIdentBetweenCalledAndUncall = "Missmatch between module ident '{0:s}' used in call does not match the uncalled one '{1:s}'";
    extern const std::string NumberOfFormalParametersDoesNotMatchActual = "module '{0:s}' defined {1:d} formal parameters but uncall/call provided {2:d} parameter values";
    extern const std::string MissingUncall                           = "missing uncall for module '{0:s}' called with arguments {1:s}";

    extern const std::string ParameterValueMissmatch         = "Index: {0:d} - Formal: {1:s} | Actual: {2:s}";
    extern const std::string CallAndUncallArgumentsMissmatch         = "call and uncall of module '{0:s}' defined different parameter values - {1:s} ";

    extern const std::string AmbigousCall = "Ambigous call/uncall for module with ident '{0:s}'";
    extern const std::string NoMatchForGuessWithNActualParameters = "Non of the overloads accepts {0:d} parameters";

    /* Arithmetic errors */
    extern const std::string DivisionByZero = "invalid division by zero";

    /* Internal parser errors */
    extern const std::string NoMappingForLoopVariable = "no mapping of loop variable {0:s} to a value was found";
} // namespace parser
