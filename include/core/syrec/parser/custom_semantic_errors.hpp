#ifndef CUSTOM_SEMANTIC_ERRORS_HPP
#define CUSTOM_SEMANTIC_ERRORS_HPP
#pragma once

#include <string>

namespace parser {
    /* Variable declaration and access error messages */
    extern const std::string UndeclaredIdent;
    extern const std::string DuplicateDeclarationOfIdent;
    extern const std::string BitAccessOutOfRange;
    extern const std::string BitRangeOutOfRange;
    extern const std::string BitRangeStartLargerThanEnd;
    extern const std::string DimensionOutOfRange;
    extern const std::string DimensionValueOutOfRange;
    extern const std::string VariableIsNotLoopVariable;
    extern const std::string InvalidParameterType;
    extern const std::string InvalidLocalType;
    extern const std::string DuplicateModuleIdentDeclaration;
    extern const std::string AccessingRestrictedPartOfSignal;

    extern const std::string AssignmentToReadonlyVariable;

    extern const std::string EmptyStatementList;

    extern const std::string IfAndFiConditionMissmatch;

    extern const std::string ShiftAmountNegative;

    extern const std::string FullSignalStructureMissmatch;

    extern const std::string InvalidLoopVariableValueRangeWithPositiveStepsize;
    extern const std::string InvalidLoopVariableValueRangeWithNegativeStepsize;
    extern const std::string InvalidLoopIterationRangeStepSizeCannotBeZeroWhenStartAndEndValueDiffer;
    extern const std::string CannotReferenceLoopVariableInInitalValueDefinition;

    extern const std::string InvalidSwapSignalWidthMissmatch;
    extern const std::string InvalidSwapNumDimensionsMissmatch;
    extern const std::string InvalidSwapValueForDimensionMissmatch;

    /* Invalid operation error messages */
    extern const std::string InvalidAssignOperation;
    extern const std::string InvalidBinaryOperation;
    extern const std::string InvalidUnaryOperation;
    extern const std::string InvalidShiftOperation;
    extern const std::string InvalidOrMissingNumberExpressionOperation;

    /* Broadcasting error messages */
    extern const std::string MissmatchedNumberOfDimensionsBetweenOperands;
    extern const std::string MissmatchedNumberOfValuesForDimension;
    extern const std::string MissmatchedNumberOfValuesForDimensionsBetweenOperands;

    /* Call | uncall error messages */
    extern const std::string PreviousCallWasNotUncalled;
    extern const std::string UncallWithoutPreviousCall;
    extern const std::string MissmatchOfModuleIdentBetweenCalledAndUncall;
    extern const std::string NumberOfParametersMissmatchBetweenCallAndUncall;
    extern const std::string NumberOfFormalParametersDoesNotMatchActual;
    extern const std::string MissingUncall;

    extern const std::string ParameterValueMissmatch;
    extern const std::string CallAndUncallArgumentsMissmatch;

    extern const std::string AmbigousCall;
    extern const std::string NoMatchForGuessWithNActualParameters;

    /* Arithmetic errors */
    extern const std::string DivisionByZero;

    /* Internal parser errors */
    extern const std::string NoMappingForLoopVariable;

    extern const std::string NoMappingForNumberOperation;
    extern const std::string InvalidOperationForNumberExpressionDefined;
}

#endif