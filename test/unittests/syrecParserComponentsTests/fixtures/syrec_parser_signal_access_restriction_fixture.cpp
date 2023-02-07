#include "syrec_parser_signal_access_restriction_fixture.hpp"

using namespace parser;

// TODO: Should there also be tests for using out of range indizes ?
// TODO: Tests to lift restrictions

/*
 * Test scenarios:
 * 1.   a += ...
 * 2.   a.0 += ...
 * 3.   a.0:5 += ...
 * 4.   a[0] += ...
 * 5.   a[0].0 += ...
 * 6.   a[0].0:5 += ...
 * 7.   a[$i] += ...
 * 8.   a[$i].0 += ...
 * 9.   a[$i].0:5 += ...
 */

INSTANTIATE_TEST_SUITE_P(
        SignalAccessRestrictionTests,
        SignalAccessRestrictionBaseTestForBitsOfSignal,
        ::testing::Values(
            std::make_tuple(
                    "restrictAccessOnWholeSignal",
                    [](const std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->blockAccessOnSignalCompletely(); },
                    [](const std::size_t&) { return true; }),
            std::make_tuple(
                    "restrictAccessToBits",
                    [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToBit(SignalAccessRestrictionBaseTest::restrictedBit); },
                    [](const std::size_t& bitPosition) { return bitPosition == SignalAccessRestrictionBaseTest::restrictedBit; }),
            std::make_tuple(
                    "restrictAccessToBitRange",
                    [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToBitRange(SignalAccessRestrictionBaseTest::restrictedBitRange); },
                        [](const std::size_t& bitPosition) { return SignalAccessRestrictionBaseTest::isBitWithinRange(bitPosition, SignalAccessRestrictionBaseTest::restrictedBitRange.start, SignalAccessRestrictionBaseTest::restrictedBitRange.stop); }),
            std::make_tuple(
                    "restrictAccessToValueOfDimension",
                    [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToValueOfDimension(SignalAccessRestrictionBaseTest::blockedDimension, SignalAccessRestrictionBaseTest::blockedValueForDimension); },
                    [](const std::size_t&) { return true; }),
            std::make_tuple(
                    "restrictAccessToBitOfValueOfDimension",
                    [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToBit(SignalAccessRestrictionBaseTest::blockedDimension, SignalAccessRestrictionBaseTest::blockedValueForDimension, SignalAccessRestrictionBaseTest::restrictedBit); },
                    [](const std::size_t& bitPosition) { return bitPosition == SignalAccessRestrictionBaseTest::restrictedBit; }),
            std::make_tuple(
                    "restrictAccessToBitRangeOfValueOfDimension",
                    [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToBitRange(SignalAccessRestrictionBaseTest::blockedDimension, SignalAccessRestrictionBaseTest::blockedValueForDimension, SignalAccessRestrictionBaseTest::restrictedBitRange); },
                    [](const std::size_t& bitPosition) { return SignalAccessRestrictionBaseTest::isBitWithinRange(bitPosition, SignalAccessRestrictionBaseTest::restrictedBitRange.start, SignalAccessRestrictionBaseTest::restrictedBitRange.stop); }),
            std::make_tuple(
                    "restrictAccessToDimension",
                    [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToDimension(SignalAccessRestrictionBaseTest::blockedDimension); },
                    [](const std::size_t&) { return true; }),
            std::make_tuple(
                    "restrictAccessToBitOfDimension",
                    [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToBit(SignalAccessRestrictionBaseTest::blockedDimension, SignalAccessRestrictionBaseTest::restrictedBit); },
                    [](const std::size_t& bitPosition) { return bitPosition == SignalAccessRestrictionBaseTest::restrictedBit; }),
            std::make_tuple(
                    "restrictAccessToBitRangeOfDimension",
                    [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToBitRange(SignalAccessRestrictionBaseTest::blockedDimension, SignalAccessRestrictionBaseTest::restrictedBitRange); },
                        [](const std::size_t& bitPosition) { return SignalAccessRestrictionBaseTest::isBitWithinRange(bitPosition, SignalAccessRestrictionBaseTest::restrictedBitRange.start, SignalAccessRestrictionBaseTest::restrictedBitRange.stop); })
        ),
        [](const testing::TestParamInfo<SignalAccessRestrictionBaseTestForBitsOfSignal::ParamType>& info) {
            return std::get<0>(info.param);
        });

INSTANTIATE_TEST_SUITE_P(
        SignalAccessRestrictionTests,
        SignalAccessRestrictionBaseTestForBitRanges,
        ::testing::Values(
                std::make_tuple(
                        "restrictAccessOnWholeSignal",
                        [](const std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->blockAccessOnSignalCompletely(); },
                        [](const SignalAccessRestriction::SignalAccess&) { return true; }),
                std::make_tuple(
                        "restrictAccessToBits",
                        [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToBit(SignalAccessRestrictionBaseTest::restrictedBit); },
                        [](const SignalAccessRestriction::SignalAccess& bitRange) { return SignalAccessRestrictionBaseTest::isBitWithinRange(SignalAccessRestrictionBaseTest::restrictedBit, bitRange.start, bitRange.stop); }),
                std::make_tuple(
                        "restrictAccessToBitRange",
                        [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToBitRange(SignalAccessRestrictionBaseTest::restrictedBitRange); },
                        [](const SignalAccessRestriction::SignalAccess& bitRange) { return SignalAccessRestrictionBaseTest::doesAccessIntersectRegion(SignalAccessRestrictionBaseTest::restrictedBitRange, bitRange); }),
                std::make_tuple(
                        "restrictAccessToValueOfDimension",
                        [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToValueOfDimension(SignalAccessRestrictionBaseTest::blockedDimension, SignalAccessRestrictionBaseTest::blockedValueForDimension); },
                        [](const SignalAccessRestriction::SignalAccess&) { return true; }),
                std::make_tuple(
                        "restrictAccessToBitOfValueOfDimension",
                        [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToBit(SignalAccessRestrictionBaseTest::blockedDimension, SignalAccessRestrictionBaseTest::blockedValueForDimension, SignalAccessRestrictionBaseTest::restrictedBit); },
                        [](const SignalAccessRestriction::SignalAccess& bitRange) { return SignalAccessRestrictionBaseTest::isBitWithinRange(SignalAccessRestrictionBaseTest::restrictedBit, bitRange.start, bitRange.stop); }),
                std::make_tuple(
                        "restrictAccessToBitRangeOfValueOfDimension",
                        [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToBitRange(SignalAccessRestrictionBaseTest::blockedDimension, SignalAccessRestrictionBaseTest::blockedValueForDimension, SignalAccessRestrictionBaseTest::restrictedBitRange); },
                        [](const SignalAccessRestriction::SignalAccess& bitRange) { return SignalAccessRestrictionBaseTest::doesAccessIntersectRegion(SignalAccessRestrictionBaseTest::restrictedBitRange, bitRange); }),
                std::make_tuple(
                        "restrictAccessToDimension",
                        [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToDimension(SignalAccessRestrictionBaseTest::blockedDimension); },
                        [](const SignalAccessRestriction::SignalAccess&) { return true; }),
                std::make_tuple(
                        "restrictAccessToBitOfDimension",
                        [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToBit(SignalAccessRestrictionBaseTest::blockedDimension, SignalAccessRestrictionBaseTest::restrictedBit); },
                        [](const SignalAccessRestriction::SignalAccess& bitRange) { return SignalAccessRestrictionBaseTest::isBitWithinRange(SignalAccessRestrictionBaseTest::restrictedBit, bitRange.start, bitRange.stop); }),
                std::make_tuple(
                        "restrictAccessToBitRangeOfDimension",
                        [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToBitRange(SignalAccessRestrictionBaseTest::blockedDimension, SignalAccessRestrictionBaseTest::restrictedBitRange); },
                        [](const SignalAccessRestriction::SignalAccess& bitRange) { return SignalAccessRestrictionBaseTest::doesAccessIntersectRegion(SignalAccessRestrictionBaseTest::restrictedBitRange, bitRange); })
        ),
        [](const testing::TestParamInfo<SignalAccessRestrictionBaseTestForBitRanges::ParamType>& info) {
            return std::get<0>(info.param);
        });

INSTANTIATE_TEST_SUITE_P(
        SignalAccessRestrictionTests,
        SignalAccessRestrictionBaseTestForDimensions,
        ::testing::Values(
                std::make_tuple(
                        "restrictAccessOnWholeSignal",
                        [](const std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->blockAccessOnSignalCompletely(); },
                        [](const std::size_t&) { return true; }),
                std::make_tuple(
                        "restrictAccessToBits",
                        [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToBit(SignalAccessRestrictionBaseTest::restrictedBit); },
                        [](const std::size_t&) { return true; }),
                std::make_tuple(
                        "restrictAccessToBitRange",
                        [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToBitRange(SignalAccessRestrictionBaseTest::restrictedBitRange); },
                        [](const std::size_t&) { return true; }),
                std::make_tuple(
                        "restrictAccessToValueOfDimension",
                        [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToValueOfDimension(SignalAccessRestrictionBaseTest::blockedDimension, SignalAccessRestrictionBaseTest::blockedValueForDimension); },
                        [](const std::size_t& dimension) { return dimension == SignalAccessRestrictionBaseTest::blockedDimension; }),
                std::make_tuple(
                        "restrictAccessToBitOfValueOfDimension",
                        [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToBit(SignalAccessRestrictionBaseTest::blockedDimension, SignalAccessRestrictionBaseTest::blockedValueForDimension, SignalAccessRestrictionBaseTest::restrictedBit); },
                        [](const std::size_t& dimension) { return dimension == SignalAccessRestrictionBaseTest::blockedDimension; }),
                std::make_tuple(
                        "restrictAccessToBitRangeOfValueOfDimension",
                        [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToBitRange(SignalAccessRestrictionBaseTest::blockedDimension, SignalAccessRestrictionBaseTest::blockedValueForDimension, SignalAccessRestrictionBaseTest::restrictedBitRange); },
                        [](const std::size_t& dimension) { return dimension == SignalAccessRestrictionBaseTest::blockedDimension; }),
                std::make_tuple(
                        "restrictAccessToDimension",
                        [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToDimension(SignalAccessRestrictionBaseTest::blockedDimension); },
                        [](const std::size_t& dimension) { return dimension == SignalAccessRestrictionBaseTest::blockedDimension; }),
                std::make_tuple(
                        "restrictAccessToBitOfDimension",
                        [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToBit(SignalAccessRestrictionBaseTest::blockedDimension, SignalAccessRestrictionBaseTest::restrictedBit); },
                        [](const std::size_t& dimension) { return dimension == SignalAccessRestrictionBaseTest::blockedDimension; }),
                std::make_tuple(
                        "restrictAccessToBitRangeOfDimension",
                        [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToBitRange(SignalAccessRestrictionBaseTest::blockedDimension, SignalAccessRestrictionBaseTest::restrictedBitRange); },
                        [](const std::size_t& dimension) { return dimension == SignalAccessRestrictionBaseTest::blockedDimension; })
        ),
        [](const testing::TestParamInfo<SignalAccessRestrictionBaseTestForDimensions::ParamType>& info) {
            return std::get<0>(info.param);
        });

INSTANTIATE_TEST_SUITE_P(
        SignalAccessRestrictionTests,
        SignalAccessRestrictionBaseTestForValuesOfDimensions,
        ::testing::Values(
                std::make_tuple(
                        "restrictAccessOnWholeSignal",
                        [](const std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->blockAccessOnSignalCompletely(); },
                        [](const std::size_t&, const std::size_t&) { return true; }),
                std::make_tuple(
                        "restrictAccessToBits",
                        [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToBit(SignalAccessRestrictionBaseTest::restrictedBit); },
                        [](const std::size_t&, const std::size_t&) { return true; }),
                std::make_tuple(
                        "restrictAccessToBitRange",
                        [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToBitRange(SignalAccessRestrictionBaseTest::restrictedBitRange); },
                        [](const std::size_t&, const std::size_t&) { return true; }),
                std::make_tuple(
                        "restrictAccessToValueOfDimension",
                        [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToValueOfDimension(SignalAccessRestrictionBaseTest::blockedDimension, SignalAccessRestrictionBaseTest::blockedValueForDimension); },
                        [](const std::size_t& dimension, const std::size_t& valueForDimension) { return dimension == SignalAccessRestrictionBaseTest::blockedDimension && valueForDimension == SignalAccessRestrictionBaseTest::blockedValueForDimension; }),
                std::make_tuple(
                        "restrictAccessToBitOfValueOfDimension",
                        [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToBit(SignalAccessRestrictionBaseTest::blockedDimension, SignalAccessRestrictionBaseTest::blockedValueForDimension, SignalAccessRestrictionBaseTest::restrictedBit); },
                        [](const std::size_t& dimension, const std::size_t& valueForDimension) { return dimension == SignalAccessRestrictionBaseTest::blockedDimension && valueForDimension == SignalAccessRestrictionBaseTest::blockedValueForDimension; }),
                std::make_tuple(
                        "restrictAccessToBitRangeOfValueOfDimension",
                        [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToBitRange(SignalAccessRestrictionBaseTest::blockedDimension, SignalAccessRestrictionBaseTest::blockedValueForDimension, SignalAccessRestrictionBaseTest::restrictedBitRange); },
                        [](const std::size_t& dimension, const std::size_t& valueForDimension) { return dimension == SignalAccessRestrictionBaseTest::blockedDimension && valueForDimension == SignalAccessRestrictionBaseTest::blockedValueForDimension; }),
                std::make_tuple(
                        "restrictAccessToDimension",
                        [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToDimension(SignalAccessRestrictionBaseTest::blockedDimension); },
                        [](const std::size_t& dimension, const std::size_t&) { return dimension == SignalAccessRestrictionBaseTest::blockedDimension; }),
                std::make_tuple(
                        "restrictAccessToBitOfDimension",
                        [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToBit(SignalAccessRestrictionBaseTest::blockedDimension, SignalAccessRestrictionBaseTest::restrictedBit); },
                        [](const std::size_t& dimension, const std::size_t&) { return dimension == SignalAccessRestrictionBaseTest::blockedDimension; }),
                std::make_tuple(
                        "restrictAccessToBitRangeOfDimension",
                        [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToBitRange(SignalAccessRestrictionBaseTest::blockedDimension, SignalAccessRestrictionBaseTest::restrictedBitRange); },
                        [](const std::size_t& dimension, const std::size_t&) { return dimension == SignalAccessRestrictionBaseTest::blockedDimension; })
        ),
        [](const testing::TestParamInfo<SignalAccessRestrictionBaseTestForValuesOfDimensions::ParamType>& info) {
            return std::get<0>(info.param);
        });

INSTANTIATE_TEST_SUITE_P(
        SignalAccessRestrictionTests,
        SignalAccessRestrictionBaseTestForBitsOfDimension,
        ::testing::Values(
                std::make_tuple(
                        "restrictAccessOnWholeSignal",
                        [](const std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->blockAccessOnSignalCompletely(); },
                        [](const std::size_t, const std::size_t&, const std::size_t&) { return true; }),
                std::make_tuple(
                        "restrictAccessToBits",
                        [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToBit(SignalAccessRestrictionBaseTest::restrictedBit); },
                        [](const std::size_t bitPosition, const std::size_t&, const std::size_t&) { return bitPosition == SignalAccessRestrictionBaseTest::restrictedBit; }),
                std::make_tuple(
                        "restrictAccessToBitRange",
                        [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToBitRange(SignalAccessRestrictionBaseTest::restrictedBitRange); },
                        [](const std::size_t bitPosition, const std::size_t&, const std::size_t&) { return SignalAccessRestrictionBaseTest::isBitWithinRange(bitPosition, SignalAccessRestrictionBaseTest::restrictedBitRange.start, SignalAccessRestrictionBaseTest::restrictedBitRange.stop); }),
                std::make_tuple(
                        "restrictAccessToValueOfDimension",
                        [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToValueOfDimension(SignalAccessRestrictionBaseTest::blockedDimension, SignalAccessRestrictionBaseTest::blockedValueForDimension); },
                        [](const std::size_t, const std::size_t& dimension, const std::size_t& valueForDimension) { return dimension == SignalAccessRestrictionBaseTest::blockedDimension && valueForDimension == SignalAccessRestrictionBaseTest::blockedValueForDimension; }),
                std::make_tuple(
                        "restrictAccessToBitOfValueOfDimension",
                        [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToBit(SignalAccessRestrictionBaseTest::blockedDimension, SignalAccessRestrictionBaseTest::blockedValueForDimension, SignalAccessRestrictionBaseTest::restrictedBit); },
                        [](const std::size_t bitPosition, const std::size_t& dimension, const std::size_t& valueForDimension) { return dimension == SignalAccessRestrictionBaseTest::blockedDimension && valueForDimension == SignalAccessRestrictionBaseTest::blockedValueForDimension && bitPosition == SignalAccessRestrictionBaseTest::restrictedBit; }),
                std::make_tuple(
                        "restrictAccessToBitRangeOfValueOfDimension",
                        [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToBitRange(SignalAccessRestrictionBaseTest::blockedDimension, SignalAccessRestrictionBaseTest::blockedValueForDimension, SignalAccessRestrictionBaseTest::restrictedBitRange); },
                        [](const std::size_t bitPosition, const std::size_t& dimension, const std::size_t& valueForDimension) { return dimension == SignalAccessRestrictionBaseTest::blockedDimension && valueForDimension == SignalAccessRestrictionBaseTest::blockedValueForDimension && SignalAccessRestrictionBaseTest::isBitWithinRange(bitPosition, SignalAccessRestrictionBaseTest::restrictedBitRange.start, SignalAccessRestrictionBaseTest::restrictedBitRange.stop); }),
                std::make_tuple(
                        "restrictAccessToDimension",
                        [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToDimension(SignalAccessRestrictionBaseTest::blockedDimension); },
                        [](const std::size_t, const std::size_t& dimension, const std::size_t&) { return dimension == SignalAccessRestrictionBaseTest::blockedDimension; }),
                std::make_tuple(
                        "restrictAccessToBitOfDimension",
                        [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToBit(SignalAccessRestrictionBaseTest::blockedDimension, SignalAccessRestrictionBaseTest::restrictedBit); },
                        [](const std::size_t bitPosition, const std::size_t& dimension, const std::size_t&) { return dimension == SignalAccessRestrictionBaseTest::blockedDimension && bitPosition == SignalAccessRestrictionBaseTest::restrictedBit; }),
                std::make_tuple(
                        "restrictAccessToBitRangeOfDimension",
                        [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToBitRange(SignalAccessRestrictionBaseTest::blockedDimension, SignalAccessRestrictionBaseTest::restrictedBitRange); },
                        [](const std::size_t bitPosition, const std::size_t& dimension, const std::size_t&) { return dimension == SignalAccessRestrictionBaseTest::blockedDimension && SignalAccessRestrictionBaseTest::isBitWithinRange(bitPosition, SignalAccessRestrictionBaseTest::restrictedBitRange.start, SignalAccessRestrictionBaseTest::restrictedBitRange.stop); })
        ),
        [](const testing::TestParamInfo<SignalAccessRestrictionBaseTestForBitsOfDimension::ParamType>& info) {
            return std::get<0>(info.param);
        });

INSTANTIATE_TEST_SUITE_P(
        SignalAccessRestrictionTests,
        SignalAccessRestrictionBaseTestForBitRangesForDimensions,
        ::testing::Values(
                std::make_tuple(
                        "restrictAccessOnWholeSignal",
                        [](const std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->blockAccessOnSignalCompletely(); },
                        [](const SignalAccessRestriction::SignalAccess&, const std::size_t&, const std::size_t&) { return true; }),
                std::make_tuple(
                        "restrictAccessToBits",
                        [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToBit(SignalAccessRestrictionBaseTest::restrictedBit); },
                        [](const SignalAccessRestriction::SignalAccess& bitRange, const std::size_t&, const std::size_t&) { return SignalAccessRestrictionBaseTest::isBitWithinRange(SignalAccessRestrictionBaseTest::restrictedBit, bitRange.start, bitRange.stop); }),
                std::make_tuple(
                        "restrictAccessToBitRange",
                        [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToBitRange(SignalAccessRestrictionBaseTest::restrictedBitRange); },
                        [](const SignalAccessRestriction::SignalAccess& bitRange, const std::size_t&, const std::size_t&) { return SignalAccessRestrictionBaseTest::doesAccessIntersectRegion(SignalAccessRestrictionBaseTest::restrictedBitRange, bitRange); }),
                std::make_tuple(
                        "restrictAccessToValueOfDimension",
                        [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToValueOfDimension(SignalAccessRestrictionBaseTest::blockedDimension, SignalAccessRestrictionBaseTest::blockedValueForDimension); },
                        [](const SignalAccessRestriction::SignalAccess&, const std::size_t& dimension, const std::size_t& valueForDimension) { return dimension == SignalAccessRestrictionBaseTest::blockedDimension && valueForDimension == SignalAccessRestrictionBaseTest::blockedValueForDimension; }),
                std::make_tuple(
                        "restrictAccessToBitOfValueOfDimension",
                        [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToBit(SignalAccessRestrictionBaseTest::blockedDimension, SignalAccessRestrictionBaseTest::blockedValueForDimension, SignalAccessRestrictionBaseTest::restrictedBit); },
                        [](const SignalAccessRestriction::SignalAccess& bitRange, const std::size_t& dimension, const std::size_t& valueForDimension) { return dimension == SignalAccessRestrictionBaseTest::blockedDimension && valueForDimension == SignalAccessRestrictionBaseTest::blockedValueForDimension && SignalAccessRestrictionBaseTest::isBitWithinRange(SignalAccessRestrictionBaseTest::restrictedBit, bitRange.start, bitRange.stop); }),
                std::make_tuple(
                        "restrictAccessToBitRangeOfValueOfDimension",
                        [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToBitRange(SignalAccessRestrictionBaseTest::blockedDimension, SignalAccessRestrictionBaseTest::blockedValueForDimension, SignalAccessRestrictionBaseTest::restrictedBitRange); },
                        [](const SignalAccessRestriction::SignalAccess& bitRange, const std::size_t& dimension, const std::size_t& valueForDimension) { return dimension == SignalAccessRestrictionBaseTest::blockedDimension && valueForDimension == SignalAccessRestrictionBaseTest::blockedValueForDimension && SignalAccessRestrictionBaseTest::doesAccessIntersectRegion(SignalAccessRestrictionBaseTest::restrictedBitRange, bitRange); }),
                std::make_tuple(
                        "restrictAccessToDimension",
                        [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToDimension(SignalAccessRestrictionBaseTest::blockedDimension); },
                        [](const SignalAccessRestriction::SignalAccess&, const std::size_t& dimension, const std::size_t&) { return dimension == SignalAccessRestrictionBaseTest::blockedDimension; }),
                std::make_tuple(
                        "restrictAccessToBitOfDimension",
                        [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToBit(SignalAccessRestrictionBaseTest::blockedDimension, SignalAccessRestrictionBaseTest::restrictedBit); },
                        [](const SignalAccessRestriction::SignalAccess& bitRange, const std::size_t& dimension, const std::size_t&) { return dimension == SignalAccessRestrictionBaseTest::blockedDimension && SignalAccessRestrictionBaseTest::isBitWithinRange(SignalAccessRestrictionBaseTest::restrictedBit, bitRange.start, bitRange.stop); }),
                std::make_tuple(
                        "restrictAccessToBitRangeOfDimension",
                        [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToBitRange(SignalAccessRestrictionBaseTest::blockedDimension, SignalAccessRestrictionBaseTest::restrictedBitRange); },
                        [](const SignalAccessRestriction::SignalAccess& bitRange, const std::size_t& dimension, const std::size_t&) { return dimension == SignalAccessRestrictionBaseTest::blockedDimension && SignalAccessRestrictionBaseTest::doesAccessIntersectRegion(SignalAccessRestrictionBaseTest::restrictedBitRange, bitRange); })
        ),
        [](const testing::TestParamInfo<SignalAccessRestrictionBaseTestForBitRangesForDimensions::ParamType>& info) {
            return std::get<0>(info.param);
        });
