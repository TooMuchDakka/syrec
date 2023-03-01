#ifndef SYREC_PARSER_SIGNAL_ACCESS_RESTRICTION_SUCCESS_FIXTURE_HPP
#define SYREC_PARSER_SIGNAL_ACCESS_RESTRICTION_SUCCESS_FIXTURE_HPP
#pragma once

#include <googletest/googletest/include/gtest/gtest.h>
#include "gtest/gtest.h"

#include "core/syrec/program.hpp"
#include "syrec_signal_operand_builder.hpp"

namespace SignalAccessRestrictionParserTests {
    enum ExcludedCases {
        NoneExcluded,
        OverlappingBitRanges,
        RuntimeConstantBitRanges
    };

    class SyrecParserSignalAccessRestrictionSuccessFixture:
        public ::testing::Test,
        public testing::WithParamInterface<std::tuple<std::string, SignalOperandBuilder, ExcludedCases>> {
    };
} // namespace parser
#endif