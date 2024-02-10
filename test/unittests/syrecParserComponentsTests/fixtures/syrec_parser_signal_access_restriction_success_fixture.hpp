#ifndef SYREC_PARSER_SIGNAL_ACCESS_RESTRICTION_SUCCESS_FIXTURE_HPP
#define SYREC_PARSER_SIGNAL_ACCESS_RESTRICTION_SUCCESS_FIXTURE_HPP
#pragma once

#include "gtest/gtest.h"
#include "syrec_signal_operand_builder.hpp"

namespace parser {
    namespace SignalAccessRestrictionParserTests {
        enum ExcludedCases {
            NoneExcluded,
            OverlappingBitRanges,
            RuntimeConstantBitRanges
        };

        class SyrecParserSignalAccessRestrictionSuccessFixture:
            public ::testing::Test,
            public testing::WithParamInterface<std::tuple<std::string, ::SignalAccessRestrictionParserTests::SignalOperandBuilder, ExcludedCases>> {
        };
    } 
}// namespace SignalAccessRestrictionParserTests
#endif