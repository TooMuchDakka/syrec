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
        enum SwapRhsOperandSignalSize {
            FULL,
            FOR1DACCESS,
            FORINTERMEDIATEACCESS,
        };

        const static inline std::vector<unsigned int> defaultSignalDimensions = {2, 4, 2};
        constexpr static unsigned int                 defaultSignalBitwidth   = 8;
        constexpr static unsigned int                 defaultLhsValue            = 230;
        constexpr static unsigned int                 defaultRhsValue         = 170;
        constexpr static unsigned int                 defaultRhsBitRangeSizedValue = 5;

        const static inline std::vector<unsigned int> defaultRhsIntermediateSignalDimensions = {4, 2};
        const static inline std::vector<unsigned int> defaultRhs1DSignalDimensions = {1};

        constexpr static unsigned int lhsLockedValueOfFirstDimension = 1;
        constexpr static unsigned int lhsLockedValueOfSecondDimension = 3;
        constexpr static unsigned int lhsLockedValueOfThirdDimension  = 0;

        constexpr static unsigned int lhsNotLockedValueOfFirstDimension  = 0;
        constexpr static unsigned int lhsNotLockedValueOfSecondDimension = 1;
        constexpr static unsigned int lhsNotLockedValueOfThirdDimension  = 1;

        constexpr static unsigned int rhsLockedValueOfFirstDimension  = 0;
        constexpr static unsigned int rhsLockedValueOfSecondDimension = 1;
        constexpr static unsigned int rhsLockedValueOfThirdDimension  = 1;

        constexpr static unsigned int rhsNotLockedValueOfFirstDimension  = 1;
        constexpr static unsigned int rhsNotLockedValueOfSecondDimension = 2;
        constexpr static unsigned int rhsNotLockedValueOfThirdDimension  = 0;

        constexpr static ::optimizations::BitRangeAccessRestriction::BitRangeAccess lhsDefaultLockedBitRange = ::optimizations::BitRangeAccessRestriction::BitRangeAccess(0, 3);
        constexpr static ::optimizations::BitRangeAccessRestriction::BitRangeAccess rhsDefaultLockedBitRange = ::optimizations::BitRangeAccessRestriction::BitRangeAccess(4, 7);

        constexpr static ::optimizations::BitRangeAccessRestriction::BitRangeAccess lhsDefaultPartOfNonLockedBitRange = ::optimizations::BitRangeAccessRestriction::BitRangeAccess(5, 6);
        constexpr static ::optimizations::BitRangeAccessRestriction::BitRangeAccess lhsDefaultNonLockedBitRange = ::optimizations::BitRangeAccessRestriction::BitRangeAccess(4, 7);
        constexpr static ::optimizations::BitRangeAccessRestriction::BitRangeAccess rhsDefaultPartOfNonLockedBitRange = ::optimizations::BitRangeAccessRestriction::BitRangeAccess(2, 3);
        constexpr static ::optimizations::BitRangeAccessRestriction::BitRangeAccess rhsDefaultNonLockedBitRange       = ::optimizations::BitRangeAccessRestriction::BitRangeAccess(0, 3);

        constexpr static ::optimizations::BitRangeAccessRestriction::BitRangeAccess lhsNotLockedAndRhsLockedBitRange = ::optimizations::BitRangeAccessRestriction::BitRangeAccess(6, 7);
        constexpr static ::optimizations::BitRangeAccessRestriction::BitRangeAccess lhsLockedAndRhsNotLockedBitRange = ::optimizations::BitRangeAccessRestriction::BitRangeAccess(2, 3);

        [[nodiscard]] static SignalValueLookup::ptr createDefaultInitializedLhsSignalValueLookup() {
            SignalValueLookup::ptr signalValueLookup = createSignalValueLookup(defaultSignalBitwidth, defaultSignalDimensions, defaultLhsValue);
            signalValueLookup->invalidateStoredValueForBitrange(
                { lhsLockedValueOfFirstDimension, lhsLockedValueOfSecondDimension, lhsLockedValueOfThirdDimension }, lhsDefaultLockedBitRange
            );
            return signalValueLookup;
        }

        [[nodiscard]] static SignalValueLookup::ptr createDefaultInitializedRhsSignalValueLookup(const SwapRhsOperandSignalSize rhsOperandSize, const bool createWithFullSignalBitwidth) {
            const auto signalBitWidth = createWithFullSignalBitwidth ? defaultSignalBitwidth : ((rhsDefaultLockedBitRange.second - rhsDefaultLockedBitRange.first) + 1);
            const auto defaultValue   = createWithFullSignalBitwidth ? defaultRhsValue : defaultRhsBitRangeSizedValue;

            std::vector<unsigned int> dimensions;
            switch (rhsOperandSize) {
                case FOR1DACCESS:
                    dimensions = defaultRhs1DSignalDimensions;
                    break;
                case FORINTERMEDIATEACCESS:
                    dimensions = defaultRhsIntermediateSignalDimensions;
                    break;
                default:
                    dimensions = defaultSignalDimensions;
                    break;
            }

            auto signalValueLookup = createSignalValueLookup(signalBitWidth, dimensions, defaultValue);
            switch (rhsOperandSize) {
                case FOR1DACCESS:
                    signalValueLookup->invalidateStoredValueForBitrange({rhsLockedValueOfFirstDimension }, rhsDefaultLockedBitRange);
                    break;
                case FORINTERMEDIATEACCESS:
                    signalValueLookup->invalidateStoredValueForBitrange({rhsLockedValueOfSecondDimension, rhsLockedValueOfThirdDimension}, rhsDefaultLockedBitRange);
                    break;
                default:
                    signalValueLookup->invalidateStoredValueForBitrange({rhsLockedValueOfFirstDimension, rhsLockedValueOfSecondDimension, rhsLockedValueOfThirdDimension}, rhsDefaultLockedBitRange);
                    break;
            }
            return signalValueLookup;
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

        ASSERT_TRUE(lhsSwapOperand != nullptr) << "LHs operand of swap operation cannot be null";
        ASSERT_TRUE(rhsSwapOperand != nullptr) << "RHs operand of swap operation cannot be null";

        swapOperation(lhsSwapOperand, rhsSwapOperand);

        ASSERT_NO_FATAL_FAILURE(checkThatNoValueCanBeFetchedForGlobalBitRanges(lhsSwapOperand)) << "Expected not being able to fetch a value for access on any bit / bit ranges of the lhs operand";
        ASSERT_NO_FATAL_FAILURE(checkThatNoValueCanBeFetchedForGlobalBitRanges(rhsSwapOperand)) << "Expected not being able to fetch a value for access on any bit / bit ranges of the rhs operand";
        ASSERT_NO_FATAL_FAILURE(checkThatNoValueCanBeFetchedForValuesOrBitRangesOfIntermediateDimensions(lhsSwapOperand)) << "Expected not being able to fetch a value for access on any bit / bit ranges of intermediate dimensions for the lhs operand";
        ASSERT_NO_FATAL_FAILURE(checkThatNoValueCanBeFetchedForValuesOrBitRangesOfIntermediateDimensions(rhsSwapOperand)) << "Expected not being able to fetch a value for access on any bit / bit ranges of intermediate dimensions for the rhs operand";

        ASSERT_NO_FATAL_FAILURE(
                createAndCheckConditionForAllDimensionAndBitrangeCombinations(
                        lhsSwapOperand,
                        lhsOperandRestrictionStatusLookupDefinition,
                        lhsOperandExpectedValueLookupDefinition))
                << "Conditions for fetching value for fully specified access on lhs operand";

        ASSERT_NO_FATAL_FAILURE(
                createAndCheckConditionForAllDimensionAndBitrangeCombinations(
                        rhsSwapOperand,
                        rhsOperandRestrictionStatusLookupDefinition,
                        rhsOperandExpectedValueLookupDefinition))
                << "Conditions for fetching value for fully specified access on rhs operand";

    }
};

#endif