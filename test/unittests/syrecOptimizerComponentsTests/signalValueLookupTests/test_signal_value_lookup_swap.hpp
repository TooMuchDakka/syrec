#ifndef SIGNAL_VALUE_LOOKUP_SWAP_HPP
#define SIGNAL_VALUE_LOOKUP_SWAP_HPP
#pragma once

#include "test_base_signal_value_lookup.hpp"

namespace valueLookup {
    using ValueLookupCreation = std::function<SignalValueLookup::ptr()>;
    using SwapOperation = std::function<void(const SignalValueLookup::ptr& signalValueLookup, const SignalValueLookup::ptr& otherSignalValueLookup)>;

	class SignalValueLookupSwapTest:
	    public BaseSignalValueLookupTest,
	    public testing::Test,
	    public testing::WithParamInterface<
                std::tuple<std::string,
                            ValueLookupCreation,
                            ValueLookupCreation,
                            SwapOperation,
                            ExpectedRestrictionStatusLookup,
                            ExpectedValueLookup,
                            ExpectedRestrictionStatusLookup,
                            ExpectedValueLookup>> {
	public:

        const static inline std::vector<unsigned int> defaultSignalDimensions = {2, 4, 2};
        constexpr static unsigned int                 defaultSignalBitwidth   = 8;
        constexpr static unsigned int                 defaultValue            = 230;

        void SetUp() override {
            signalValueLookup = createSignalValueLookup(defaultSignalBitwidth, defaultSignalDimensions, defaultValue);
        }
	};

    TEST_P(SignalValueLookupSwapTest, SwapTests) {
        const auto& testParamInfo                = GetParam();
        const auto& lhsOperandSetup = std::get<1>(testParamInfo);
        const auto& rhsOperandSetup = std::get<2>(testParamInfo);
        const auto& swapOperation = std::get<3>(testParamInfo);

        const auto& lhsOperandRestrictionStatusLookupDefinition = std::get<4>(testParamInfo);
        const auto& lhsOperandExpectedValueLookupDefinition = std::get<5>(testParamInfo);
        const auto& rhsOperandRestrictionStatusLookupDefinition = std::get<6>(testParamInfo);
        const auto& rhsOperandExpectedValueLookupDefinition     = std::get<7>(testParamInfo);

        const SignalValueLookup::ptr lhsSwapOperand = lhsOperandSetup();
        const SignalValueLookup::ptr rhsSwapOperand = rhsOperandSetup();
        ASSERT_FALSE(lhsSwapOperand == nullptr) << "LHs operand of swap operation cannot be null";
        ASSERT_FALSE(rhsSwapOperand == nullptr) << "RHs operand of swap operation cannot be null";

        swapOperation(lhsSwapOperand, rhsSwapOperand);

        ASSERT_NO_FATAL_FAILURE(checkThatNoValueCanBeFetchedForGlobalBitRanges(lhsSwapOperand));
        ASSERT_NO_FATAL_FAILURE(checkThatNoValueCanBeFetchedForGlobalBitRanges(rhsSwapOperand));
        ASSERT_NO_FATAL_FAILURE(checkThatNoValueCanBeFetchedForValuesOrBitRangesOfIntermediateDimensions(lhsSwapOperand));
        ASSERT_NO_FATAL_FAILURE(checkThatNoValueCanBeFetchedForValuesOrBitRangesOfIntermediateDimensions(rhsSwapOperand));

        ASSERT_NO_FATAL_FAILURE(
                createAndCheckConditionForAllDimensionAndBitrangeCombinations(
                        lhsSwapOperand,
                        lhsOperandRestrictionStatusLookupDefinition,
                        lhsOperandExpectedValueLookupDefinition));

        ASSERT_NO_FATAL_FAILURE(
                createAndCheckConditionForAllDimensionAndBitrangeCombinations(
                        rhsSwapOperand,
                        rhsOperandRestrictionStatusLookupDefinition,
                        rhsOperandExpectedValueLookupDefinition));

    }
};

#endif