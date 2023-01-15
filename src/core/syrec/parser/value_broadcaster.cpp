#include "core/syrec/parser/value_broadcaster.hpp"

using namespace parser;

bool parser::requiresBroadcasting(const std::vector<unsigned int>& numVariablesPerDimensionOfOperandOne, const std::vector<unsigned int>& numVariablesPerDimensionOfOperandTwo) {
    return numVariablesPerDimensionOfOperandOne.size() != numVariablesPerDimensionOfOperandTwo.size();
}

// TODO:
std::vector<unsigned int> parser::getSizeOfSignalAfterAccess(const syrec::VariableAccess::ptr& variableAccess) {
    const size_t              numRemainingDimensions = variableAccess->var->dimensions.size() - variableAccess->indexes.size();
    if (numRemainingDimensions == 0) {
        return {1};   
    }

    const std::vector<unsigned int>& variableDimensions = variableAccess->var->dimensions;
    std::vector<unsigned int> numValuesPerDimension(numRemainingDimensions, 0);

    size_t variableDimensionsOffset = variableAccess->indexes.size();
    for (size_t i = 0; i < numRemainingDimensions; ++i) {
        numValuesPerDimension.at(i) = variableDimensions.at(variableDimensionsOffset++);    
    }

    return numValuesPerDimension;
}

std::vector<NumberOfValuesForDimensionMissmatch> parser::getDimensionsWithMissmatchedNumberOfValues(const std::vector<unsigned int>& numVariablesPerDimensionOfOperandOne, const std::vector<unsigned int>& numVariablesPerDimensionOfOperandTwo) {
    std::vector<NumberOfValuesForDimensionMissmatch> dimensionsWithMissmatchedNumberOfValues;
    for (size_t i = 0; i < numVariablesPerDimensionOfOperandOne.size(); ++i) {
        if (numVariablesPerDimensionOfOperandOne.at(i) != numVariablesPerDimensionOfOperandTwo.at(i)) {
            dimensionsWithMissmatchedNumberOfValues.emplace_back(NumberOfValuesForDimensionMissmatch(i, numVariablesPerDimensionOfOperandOne.at(i), numVariablesPerDimensionOfOperandTwo.at(i)));
        }
    }

    return dimensionsWithMissmatchedNumberOfValues;
}