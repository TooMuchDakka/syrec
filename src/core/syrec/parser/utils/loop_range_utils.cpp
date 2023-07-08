#include <cmath>
#include "core/syrec/parser/utils/loop_range_utils.hpp"

using namespace utils;

std::optional<unsigned> utils::determineNumberOfLoopIterations(unsigned int iterationRangeStartValue, unsigned int iterationRangeEndValue, unsigned int stepSize) {
    const auto iterationRange = (iterationRangeEndValue > iterationRangeStartValue ? iterationRangeEndValue - iterationRangeStartValue : iterationRangeStartValue - iterationRangeEndValue) + 1;
    if (iterationRange > 0) {
        const float numIterations = static_cast<float>(iterationRange) / stepSize;
        return std::make_optional<std::size_t>(std::ceil(numIterations));
    }
    return std::nullopt;
}
