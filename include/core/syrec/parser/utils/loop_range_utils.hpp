#ifndef LOOP_RANGE_UTILS_HPP
#define LOOP_RANGE_UTILS_HPP

#include <optional>

namespace utils {
    std::optional<unsigned int> determineNumberOfLoopIterations(unsigned int iterationRangeStartValue, unsigned int iterationRangeEndValue, unsigned int stepSize);
}

#endif