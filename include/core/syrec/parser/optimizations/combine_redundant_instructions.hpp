#ifndef COMBINE_REDUNDANT_INSTRUCTIONS_HPP
#define COMBINE_REDUNDANT_INSTRUCTIONS_HPP
#pragma once
#include "core/syrec/statement.hpp"

namespace combineRedundantInstructions {
    class RedundantInstructionCombiner {
    public:
        void combine(syrec::Statement::vec& statements);
    };
};
#endif