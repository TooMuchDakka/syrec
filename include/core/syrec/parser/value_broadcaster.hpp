#ifndef VALUE_BROADCASTER_HPP
#define VALUE_BROADCASTER_HPP

#include <vector>

#include "core/syrec/variable.hpp"

namespace parser
{
    struct NumberOfValuesForDimensionMissmatch {
        const unsigned int dimensionIdx;
        const unsigned int expectedNumberOfValues;
        const unsigned int actualNumberOfValues;

        NumberOfValuesForDimensionMissmatch(const unsigned int dimensionIdx, const unsigned int expectedNumberOfValues, const unsigned int actualNumberOfValues):
            dimensionIdx(dimensionIdx), expectedNumberOfValues(expectedNumberOfValues), actualNumberOfValues(actualNumberOfValues) {}
        // disable changing the object through assignment
        NumberOfValuesForDimensionMissmatch& operator=(const NumberOfValuesForDimensionMissmatch&) = delete;
    };

    [[nodiscard]] bool requiresBroadcasting(const std::vector<unsigned int>& numVariablesPerDimensionOfOperandOne, const std::vector<unsigned int>& numVariablesPerDimensionOfOperandTwo);
    [[nodiscard]] std::vector<NumberOfValuesForDimensionMissmatch> getDimensionsWithMissmatchedNumberOfValues(const std::vector<unsigned int>& numVariablesPerDimensionOfOperandOne, const std::vector<unsigned int>& numVariablesPerDimensionOfOperandTwo);
    [[nodiscard]] std::vector<unsigned int>                        getSizeOfSignalAfterAccess(const syrec::VariableAccess::ptr& variableAccess);
}

#endif