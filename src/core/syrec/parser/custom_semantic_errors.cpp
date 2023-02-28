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
    extern const std::string AccessingRestrictedPartOfSignal = "Accessing restricted part of signal '{0:s}'";

    extern const std::string AssignmentToReadonlyVariable = "variable '{0:s}' is readonly";

    extern const std::string EmptyStatementList = "statement list must not be empty";

    extern const std::string IfAndFiConditionMissmatch = "Expressions used in if and corresponding fi condition did not match";

    extern const std::string ShiftAmountNegative = "expected a shift by a positive integer but instead was {0:d}";

    extern const std::string FullSignalStructureMissmatch = "when applying an operation on two full signals, the number as well as the value per dimension must match";

    extern const std::string InvalidLoopVariableValueRangeWithPositiveStepsize = "invalid loop variable value range, start value cannot be larger than the end (start: {0:d}, end: {1:d}, stepsize: {2:d})";
    extern const std::string InvalidLoopVariableValueRangeWithNegativeStepsize = "invalid loop variable value range, end value cannot be larger than the start (start: {0:d}, end: {1:d}, stepsize: {2:d})";
    extern const std::string CannotReferenceLoopVariableInInitalValueDefinition = "cannot reference loop variable '{0:s}' inside of the expression defining the initial value of the latter";

    extern const std::string InvalidSwapSignalWidthMissmatch       = "invalid swap signal width missmatch, first operand had a signal width of {0:d} while the other had a signal width of {1:d}";
    extern const std::string InvalidSwapNumDimensionsMissmatch     = "invalid swap number of dimension missmatch, first operand had {0:d} while the other had {1:d}";
    extern const std::string InvalidSwapValueForDimensionMissmatch = "invalid swap missmatch in value for dimension {0:d}, first operand had a value of {1:d} while the other had a value of {2:d}";

    /* Invalid operation error messages */
    extern const std::string InvalidAssignOperation                    = "Invalid assign operation";
    extern const std::string InvalidBinaryOperation                    = "invalid binary operation";
    extern const std::string InvalidUnaryOperation                     = "invalid unary operation";
    extern const std::string InvalidShiftOperation                     = "invalid shift operation";
    extern const std::string InvalidOrMissingNumberExpressionOperation = "invalid or missing number expression operation";

    /* Broadcasting error messages */
    extern const std::string MissmatchedNumberOfDimensionsBetweenOperands = "Missmatch of number of dimensions of operands, left operand was a {0:d}-d signal while the right one was a {1:d}-d signal";
    extern const std::string MissmatchedNumberOfValuesForDimension           = "<{0:d} | {1:d} | {2:d}>";
    // TODO: Wording of error text, index of dimensions starts after any previous access (i.e. a[0][1][3] and b[0][1][0] would result in an error at dimension 0)
    extern const std::string MissmatchedNumberOfValuesForDimensionsBetweenOperands = "Missmatch between number of values for some dimensions for the given operands (dimensionIdx is relative to last accessed dimension of signal, format <dimensionIdx | leftOperandNumValues | rightOperandNumValues>): {0:s}";

    /* Call | uncall error messages */
    extern const std::string PreviousCallWasNotUncalled              = "Tried to issue a call with one/many previous calls not being uncalled";
    extern const std::string UncallWithoutPreviousCall               = "Tried to uncall a module that was not previously called";
    extern const std::string MissmatchOfModuleIdentBetweenCalledAndUncall = "Missmatch between module ident '{0:s}' used in call does not match the uncalled one '{1:s}'";
    extern const std::string NumberOfParametersMissmatchBetweenCallAndUncall = "Module '{0:s}' was called with {1:d} parameters while the uncall was called with {2:d}";
    extern const std::string NumberOfFormalParametersDoesNotMatchActual = "module '{0:s}' defined {1:d} formal parameters but uncall/call provided {2:d} parameter values";
    extern const std::string MissingUncall                           = "missing uncall for module '{0:s}' called with arguments '{1:s}'";

    extern const std::string ParameterValueMissmatch         = "Index: {0:d} - Formal: {1:s} | Actual: {2:s}";
    extern const std::string CallAndUncallArgumentsMissmatch         = "call and uncall of module '{0:s}' defined different parameter values - '{1:s}'";

    extern const std::string AmbigousCall = "Ambigous call/uncall of module '{0:s}'";
    extern const std::string NoMatchForGuessWithNActualParameters = "None of the overloads for module '{0:s}' accepts {1:d} parameters";

    /* Arithmetic errors */
    extern const std::string DivisionByZero = "invalid division by zero";

    /* Internal parser errors */
    extern const std::string NoMappingForLoopVariable = "no mapping of loop variable {0:s} to a value was found";

    extern const std::string NoMappingForNumberOperation = "no mapping for number operation '{0:s}' was defined";
    extern const std::string InvalidOperationForNumberExpressionDefined = "invalid operation '{0:s}' for number, valid operations are: {+, -, *, /}";
} // namespace parser
