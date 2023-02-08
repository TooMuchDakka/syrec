#include "syrec_parser_signal_access_restriction_fixture.hpp"

using namespace parser;

// TODO: Should there also be tests for using out of range indizes ?
// TODO: Tests to lift restrictions

/*
 * Test scenarios (restrict):
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
                        [](const std::size_t& bitPosition) { return SignalAccessRestrictionBaseTest::isBitWithinRange(bitPosition, SignalAccessRestrictionBaseTest::restrictedBitRange); }),
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
                    [](const std::size_t& bitPosition) { return SignalAccessRestrictionBaseTest::isBitWithinRange(bitPosition, SignalAccessRestrictionBaseTest::restrictedBitRange); }),
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
                        [](const std::size_t& bitPosition) { return SignalAccessRestrictionBaseTest::isBitWithinRange(bitPosition, SignalAccessRestrictionBaseTest::restrictedBitRange); })
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
                        [](const SignalAccessRestriction::SignalAccess& bitRange) { return SignalAccessRestrictionBaseTest::isBitWithinRange(SignalAccessRestrictionBaseTest::restrictedBit, bitRange); }),
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
                        [](const SignalAccessRestriction::SignalAccess& bitRange) { return SignalAccessRestrictionBaseTest::isBitWithinRange(SignalAccessRestrictionBaseTest::restrictedBit, bitRange); }),
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
                        [](const SignalAccessRestriction::SignalAccess& bitRange) { return SignalAccessRestrictionBaseTest::isBitWithinRange(SignalAccessRestrictionBaseTest::restrictedBit, bitRange); }),
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
                        [](const std::size_t bitPosition, const std::size_t&, const std::size_t&) { return SignalAccessRestrictionBaseTest::isBitWithinRange(bitPosition, SignalAccessRestrictionBaseTest::restrictedBitRange); }),
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
                        [](const std::size_t bitPosition, const std::size_t& dimension, const std::size_t& valueForDimension) { return dimension == SignalAccessRestrictionBaseTest::blockedDimension && valueForDimension == SignalAccessRestrictionBaseTest::blockedValueForDimension && SignalAccessRestrictionBaseTest::isBitWithinRange(bitPosition, SignalAccessRestrictionBaseTest::restrictedBitRange); }),
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
                        [](const std::size_t bitPosition, const std::size_t& dimension, const std::size_t&) { return dimension == SignalAccessRestrictionBaseTest::blockedDimension && SignalAccessRestrictionBaseTest::isBitWithinRange(bitPosition, SignalAccessRestrictionBaseTest::restrictedBitRange); })
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
                        [](const SignalAccessRestriction::SignalAccess& bitRange, const std::size_t&, const std::size_t&) { return SignalAccessRestrictionBaseTest::isBitWithinRange(SignalAccessRestrictionBaseTest::restrictedBit, bitRange); }),
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
                        [](const SignalAccessRestriction::SignalAccess& bitRange, const std::size_t& dimension, const std::size_t& valueForDimension) { return dimension == SignalAccessRestrictionBaseTest::blockedDimension && valueForDimension == SignalAccessRestrictionBaseTest::blockedValueForDimension && SignalAccessRestrictionBaseTest::isBitWithinRange(SignalAccessRestrictionBaseTest::restrictedBit, bitRange); }),
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
                        [](const SignalAccessRestriction::SignalAccess& bitRange, const std::size_t& dimension, const std::size_t&) { return dimension == SignalAccessRestrictionBaseTest::blockedDimension && SignalAccessRestrictionBaseTest::isBitWithinRange(SignalAccessRestrictionBaseTest::restrictedBit, bitRange); }),
                std::make_tuple(
                        "restrictAccessToBitRangeOfDimension",
                        [](std::shared_ptr<SignalAccessRestriction>& restriction) { restriction->restrictAccessToBitRange(SignalAccessRestrictionBaseTest::blockedDimension, SignalAccessRestrictionBaseTest::restrictedBitRange); },
                        [](const SignalAccessRestriction::SignalAccess& bitRange, const std::size_t& dimension, const std::size_t&) { return dimension == SignalAccessRestrictionBaseTest::blockedDimension && SignalAccessRestrictionBaseTest::doesAccessIntersectRegion(SignalAccessRestrictionBaseTest::restrictedBitRange, bitRange); })
        ),
        [](const testing::TestParamInfo<SignalAccessRestrictionBaseTestForBitRangesForDimensions::ParamType>& info) {
            return std::get<0>(info.param);
        });

// TODO: Test that extend the restriction across multiple levels (i.e. a[0].0:5 += ...; a[1].2:3 += ...; extend with e.g. a.0 += ...)

/*
 * Test scenarios (extend restriction):
 * 1.   a.2 += ...; a.0 += ...; a.3 += ...;
 * 2.   a.2:5 += ...; a.0:1 += ...; a.6:10 += ....;
 * 3.   a[0] += ...; a[1] += ...;
 * 4.   a[0].0 += ...; a[0].2 += ...; a[0].1 += ...; a[1].0 += ...; a[1].2 += ...; a[1].1 += ...;
 * 5.   a[0].0 += ...; a[0].1 += ...; a[1].1 += ...; a[1].2 += ...;
 * 6.   a[0].2:5 += ...; a[0].0:1 += ...; a[0].6:10 += ...; a[1].2:5 += ...; a[1].0:1 += ...; a[1].6:10 += ...;
 * 7.   a[$i].0 += ...; a[$i].2 += ...; a[$i].1 += ...;
 * 8.   a[$i].2:5 += ...; a[$i].0:1 += ...; a[$i].6:10 += ...;   
 */
INSTANTIATE_TEST_SUITE_P(
    SignalAccessRestrictionTests,
    SignalAccessRestrictionBaseTestToExtendRestriction,
    ::testing::Values(
            std::make_tuple(
                "extendGlobalSignalRestriction",
                [](std::shared_ptr<SignalAccessRestriction>& restriction) {
                    restriction->restrictAccessToBit(SignalAccessRestrictionBaseTest::restrictedBit);
                },
                [](std::shared_ptr<SignalAccessRestriction>& restriction) {
                    restriction->restrictAccessToBitRange(SignalAccessRestrictionBaseTestToExtendRestriction::restrictedBitRange);
                },
                [](const std::size_t& bitPosition, const std::size_t&, const std::size_t&) {
                    return SignalAccessRestrictionBaseTestToExtendRestriction::isBitWithinRange(
                        bitPosition,
                        SignalAccessRestrictionBaseTestToExtendRestriction::restrictedBitRange
                    );
                }),
            std::make_tuple(
                "extendGlobalSignalRangeRestriction",
                [](SignalAccessRestrictionBaseTest::RestrictionPtr& restriction) {
                    restriction->restrictAccessToBitRange(SignalAccessRestrictionBaseTestToExtendRestriction::restrictedBitRange);
                },
                [](SignalAccessRestrictionBaseTest::RestrictionPtr& restriction) {
                    restriction->restrictAccessToBitRange(SignalAccessRestrictionBaseTestToExtendRestriction::bitRangeBeforeRestrictedBit);
                    restriction->restrictAccessToBitRange(SignalAccessRestrictionBaseTestToExtendRestriction::bitRangeAfterGap);
                },
                [](const std::size_t& bitPosition, const std::size_t&, const std::size_t&) {
                    return SignalAccessRestrictionBaseTestToExtendRestriction::isBitWithinRange(
                        bitPosition,
                        SignalAccessRestrictionBaseTestToExtendRestriction::restrictedBitRange)
                    || SignalAccessRestrictionBaseTestToExtendRestriction::isBitWithinRange(
                        bitPosition, 
                        SignalAccessRestrictionBaseTestToExtendRestriction::bitRangeBeforeRestrictedBit)
                    || SignalAccessRestrictionBaseTestToExtendRestriction::isBitWithinRange(
                        bitPosition,
                        SignalAccessRestrictionBaseTestToExtendRestriction::bitRangeAfterGap);
                }),
            std::make_tuple(
                "extendDimensionRestriction",
                [](SignalAccessRestrictionBaseTest::RestrictionPtr& restriction) {
                        restriction->restrictAccessToValueOfDimension(
                            SignalAccessRestrictionBaseTestToExtendRestriction::blockedDimension,
                            SignalAccessRestrictionBaseTestToExtendRestriction::blockedValueForDimension);
                },
                [](SignalAccessRestrictionBaseTest::RestrictionPtr& restriction) {
                    restriction->restrictAccessToValueOfDimension(
                        SignalAccessRestrictionBaseTestToExtendRestriction::otherBlockedDimension,
                        SignalAccessRestrictionBaseTestToExtendRestriction::blockedValueForOtherDimension);
                },
                [](const std::size_t&, const std::size_t& dimension, const std::size_t& valueForDimension) {
                    return (dimension == SignalAccessRestrictionBaseTestToExtendRestriction::blockedDimension && valueForDimension == SignalAccessRestrictionBaseTestToExtendRestriction::blockedValueForDimension)
                    || (dimension == SignalAccessRestrictionBaseTestToExtendRestriction::otherBlockedDimension && valueForDimension == SignalAccessRestrictionBaseTestToExtendRestriction::blockedValueForOtherDimension);
                }),
            std::make_tuple(
                "extendBitRestrictionForDimension",
                [](SignalAccessRestrictionBaseTest::RestrictionPtr& restriction) {
                    restriction->restrictAccessToBit(
                        SignalAccessRestrictionBaseTestToExtendRestriction::blockedDimension,
                        SignalAccessRestrictionBaseTestToExtendRestriction::blockedValueForDimension,
                        SignalAccessRestrictionBaseTestToExtendRestriction::bitRangeBeforeRestrictedBit.start);
                },
                [](SignalAccessRestrictionBaseTest::RestrictionPtr& restriction) {
                    restriction->restrictAccessToBitRange(
                        SignalAccessRestrictionBaseTestToExtendRestriction::blockedDimension,
                        SignalAccessRestrictionBaseTestToExtendRestriction::blockedValueForDimension,
                        SignalAccessRestrictionBaseTestToExtendRestriction::bitRangeBeforeRestrictedBit);
                    restriction->restrictAccessToBitRange(
                        SignalAccessRestrictionBaseTestToExtendRestriction::otherBlockedDimension,
                        SignalAccessRestrictionBaseTestToExtendRestriction::blockedValueForOtherDimension,
                        SignalAccessRestrictionBaseTestToExtendRestriction::bitRangeBeforeRestrictedBit);
                },
                [](const std::size_t& bitPosition, const std::size_t& dimension, const std::size_t& valueForDimension) {
                    return ((dimension == SignalAccessRestrictionBaseTestToExtendRestriction::blockedDimension && valueForDimension == SignalAccessRestrictionBaseTestToExtendRestriction::blockedValueForDimension) 
                        || (dimension == SignalAccessRestrictionBaseTestToExtendRestriction::otherBlockedDimension && valueForDimension == SignalAccessRestrictionBaseTestToExtendRestriction::blockedValueForOtherDimension))
                    && SignalAccessRestrictionBaseTestToExtendRestriction::isBitWithinRange(bitPosition, SignalAccessRestrictionBaseTestToExtendRestriction::bitRangeBeforeRestrictedBit);
                }),
            std::make_tuple(
                    "extendBitRestrictionWithSmallOverlapWithOtherDimension",
                    [](SignalAccessRestrictionBaseTest::RestrictionPtr& restriction) {
                        restriction->restrictAccessToBit(
                                SignalAccessRestrictionBaseTestToExtendRestriction::blockedDimension,
                                SignalAccessRestrictionBaseTestToExtendRestriction::blockedValueForDimension,
                                SignalAccessRestrictionBaseTestToExtendRestriction::bitRangeBeforeRestrictedBit.start);
                    },
                    [](SignalAccessRestrictionBaseTest::RestrictionPtr& restriction) {
                        restriction->restrictAccessToBitRange(
                                SignalAccessRestrictionBaseTestToExtendRestriction::blockedDimension,
                                SignalAccessRestrictionBaseTestToExtendRestriction::blockedValueForDimension,
                                SignalAccessRestrictionBaseTestToExtendRestriction::bitRangeBeforeRestrictedBit);
                        restriction->restrictAccessToBit(
                                SignalAccessRestrictionBaseTestToExtendRestriction::otherBlockedDimension,
                                SignalAccessRestrictionBaseTestToExtendRestriction::blockedValueForOtherDimension,
                                SignalAccessRestrictionBaseTestToExtendRestriction::bitRangeBeforeRestrictedBit.stop);
                        restriction->restrictAccessToBitRange(
                                SignalAccessRestrictionBaseTestToExtendRestriction::otherBlockedDimension,
                                SignalAccessRestrictionBaseTestToExtendRestriction::blockedValueForOtherDimension,
                                SignalAccessRestrictionBaseTestToExtendRestriction::restrictedBitRange);
                    },
                    [](const std::size_t& bitPosition, const std::size_t& dimension, const std::size_t& valueForDimension) {
                        return
                            ((dimension == SignalAccessRestrictionBaseTestToExtendRestriction::blockedDimension && valueForDimension == SignalAccessRestrictionBaseTestToExtendRestriction::blockedValueForDimension) 
                                && SignalAccessRestrictionBaseTestToExtendRestriction::isBitWithinRange(bitPosition, SignalAccessRestrictionBaseTestToExtendRestriction::bitRangeBeforeRestrictedBit))
                            || ((dimension == SignalAccessRestrictionBaseTestToExtendRestriction::otherBlockedDimension && valueForDimension == SignalAccessRestrictionBaseTestToExtendRestriction::blockedValueForOtherDimension)) 
                                && (bitPosition == SignalAccessRestrictionBaseTestToExtendRestriction::bitRangeBeforeRestrictedBit.stop || SignalAccessRestrictionBaseTestToExtendRestriction::isBitWithinRange(bitPosition, SignalAccessRestrictionBaseTestToExtendRestriction::restrictedBitRange));
                    }),
            std::make_tuple(
                "extendBitRangeRestrictionForDimension",
                [](SignalAccessRestrictionBaseTest::RestrictionPtr& restriction) {
                    restriction->restrictAccessToBitRange(
                        SignalAccessRestrictionBaseTestToExtendRestriction::blockedDimension,
                        SignalAccessRestrictionBaseTestToExtendRestriction::blockedValueForDimension,
                        SignalAccessRestrictionBaseTestToExtendRestriction::bitRangeBeforeRestrictedBit);
                },
                [](SignalAccessRestrictionBaseTest::RestrictionPtr& restriction) {
                    restriction->restrictAccessToBitRange(
                        SignalAccessRestrictionBaseTestToExtendRestriction::blockedDimension,
                        SignalAccessRestrictionBaseTestToExtendRestriction::blockedValueForDimension,
                        SignalAccessRestrictionBaseTestToExtendRestriction::restrictedBitRange);
                    restriction->restrictAccessToBitRange(
                        SignalAccessRestrictionBaseTestToExtendRestriction::blockedDimension,
                        SignalAccessRestrictionBaseTestToExtendRestriction::blockedValueForDimension,
                        SignalAccessRestrictionBaseTestToExtendRestriction::bitRangeAfterGap);

                    restriction->restrictAccessToBitRange(
                        SignalAccessRestrictionBaseTestToExtendRestriction::otherBlockedDimension,
                        SignalAccessRestrictionBaseTestToExtendRestriction::blockedValueForOtherDimension,
                        SignalAccessRestrictionBaseTestToExtendRestriction::restrictedBitRange);
                    restriction->restrictAccessToBitRange(
                        SignalAccessRestrictionBaseTestToExtendRestriction::otherBlockedDimension,
                        SignalAccessRestrictionBaseTestToExtendRestriction::blockedValueForOtherDimension,
                        SignalAccessRestrictionBaseTestToExtendRestriction::bitRangeAfterGap);
                    restriction->restrictAccessToBitRange(
                        SignalAccessRestrictionBaseTestToExtendRestriction::otherBlockedDimension,
                        SignalAccessRestrictionBaseTestToExtendRestriction::blockedValueForOtherDimension,
                        SignalAccessRestrictionBaseTestToExtendRestriction::bitRangeBeforeRestrictedBit);
                },
                [](const std::size_t& bitPosition, const std::size_t& dimension, const std::size_t& valueForDimension) {
                    return ((dimension == SignalAccessRestrictionBaseTestToExtendRestriction::blockedDimension && valueForDimension == SignalAccessRestrictionBaseTestToExtendRestriction::blockedValueForDimension) || (dimension == SignalAccessRestrictionBaseTestToExtendRestriction::otherBlockedDimension && valueForDimension == SignalAccessRestrictionBaseTestToExtendRestriction::blockedValueForOtherDimension))
                    && (SignalAccessRestrictionBaseTestToExtendRestriction::isBitWithinRange(bitPosition, SignalAccessRestrictionBaseTestToExtendRestriction::bitRangeBeforeRestrictedBit)
                    || SignalAccessRestrictionBaseTestToExtendRestriction::isBitWithinRange(bitPosition, SignalAccessRestrictionBaseTestToExtendRestriction::restrictedBitRange) 
                    || SignalAccessRestrictionBaseTestToExtendRestriction::isBitWithinRange(bitPosition, SignalAccessRestrictionBaseTestToExtendRestriction::bitRangeAfterGap));
                }),
                // TODO: This should change nothing for the restriction
            std::make_tuple(
                "extendRestrictionToCompletelyRestrictedDimension",
                [](SignalAccessRestrictionBaseTest::RestrictionPtr& restriction) {
                    restriction->restrictAccessToDimension(SignalAccessRestrictionBaseTestToExtendRestriction::blockedDimension);
                },
                [](SignalAccessRestrictionBaseTest::RestrictionPtr& restriction) {
                    restriction->restrictAccessToValueOfDimension(SignalAccessRestrictionBaseTestToExtendRestriction::blockedDimension, 0);
                    restriction->restrictAccessToValueOfDimension(
                        SignalAccessRestrictionBaseTestToExtendRestriction::blockedDimension, 
                        SignalAccessRestrictionBaseTestToExtendRestriction::blockedValueForDimension);
                },
                [](const std::size_t&, const std::size_t& dimension, const std::size_t&) {
                    return dimension == SignalAccessRestrictionBaseTestToExtendRestriction::blockedDimension;
            }),
            std::make_tuple(
                "extendBitRestrictionToCompletelyRestrictedDimension",
                [](SignalAccessRestrictionBaseTest::RestrictionPtr& restriction) {
                    restriction->restrictAccessToBit(
                        SignalAccessRestrictionBaseTestToExtendRestriction::blockedDimension,
                        SignalAccessRestrictionBaseTestToExtendRestriction::restrictedBitRange.start);
                },
                [](SignalAccessRestrictionBaseTest::RestrictionPtr& restriction) {
                    restriction->restrictAccessToBitRange(
                            SignalAccessRestrictionBaseTestToExtendRestriction::blockedDimension,
                            SignalAccessRestrictionBaseTestToExtendRestriction::restrictedBitRange);
                },
                [](const std::size_t& bitPosition, const std::size_t& dimension, const std::size_t&) {
                    return SignalAccessRestrictionBaseTestToExtendRestriction::isBitWithinRange(bitPosition, SignalAccessRestrictionBaseTestToExtendRestriction::restrictedBitRange)
                    && dimension == SignalAccessRestrictionBaseTestToExtendRestriction::blockedDimension;
                })
    ),
    [](const testing::TestParamInfo<SignalAccessRestrictionBaseTestToExtendRestriction::ParamType>& info) {
        return std::get<0>(info.param);
    });

/*
 * Test scenarios (extend restriction with restriction that will not extend the existing one)
 * 1.   a += ...; a.0 += ...; a.2:5 += ...; a[0] += ...; a[0].2 += ...; a[0].2:10 += ...; a[$i] += ...; a[$i].0 += ...; a[$i].0:8 += ...
 * 2.   a.0 += ...; a[0].0 += ...; a[$i].0 += ...
 * 3.   a.2:5 += ...; a.3 += ...; a.5 += ...; a:2:4 += ...; a[0].2 += ...; a[0].2:5 += ...; a[$i].2 += ...; a[$i].2:5 += ...
 * 4.   a[0] += ...; a[0].2 += ...; a[0].2:5 += ...; a[0] += ...
 * 5.   a[$i] += ...; a.0 += ...; a.2:5 += ...; a[0] += ...; a[0].2 += ...; a[0].2:10 += ...; a[$i] += ...; a[$i].0 += ...; a[$i].0:8 += ...
 * 6.   a[0].0 += ...; a[0].0 += ...
 * 7.   a[0].2:5 += ...; a[0].2 += ...; a[0].2:4 += ...;
 * 8.   a[$i].0 += ...; a[0].0 += ...; a[$i].0 += ...;
 * 9.   a[$i].2:5 += ...; a[0].2 += ...; a[0].4 += ...; a[$i].2 += ...; a[$i].4 += ...;      
 */
 INSTANTIATE_TEST_SUITE_P(
    SignalAccessRestrictionTests,
    SignalAccessRestrictionBaseTestToExtendRestrictionThatWillLeaveItUnchanged,
    ::testing::Values(
            std::make_tuple(
                "extendGlobalSignalRestriction",
                [](SignalAccessRestrictionBaseTest::RestrictionPtr& restriction) {
                    restriction->blockAccessOnSignalCompletely();
                },
                [](SignalAccessRestrictionBaseTest::RestrictionPtr& restriction) {
                    restriction->restrictAccessToBit(SignalAccessRestrictionBaseTestToExtendRestrictionThatWillLeaveItUnchanged::restrictedBit);
                    restriction->restrictAccessToBitRange(SignalAccessRestrictionBaseTestToExtendRestrictionThatWillLeaveItUnchanged::bitRangeBeforeRestrictedBit);
                    restriction->restrictAccessToValueOfDimension(SignalAccessRestrictionBaseTestToExtendRestrictionThatWillLeaveItUnchanged::blockedDimension, SignalAccessRestrictionBaseTestToExtendRestrictionThatWillLeaveItUnchanged::blockedValueForDimension);
                    restriction->restrictAccessToBit(SignalAccessRestrictionBaseTestToExtendRestrictionThatWillLeaveItUnchanged::blockedDimension, SignalAccessRestrictionBaseTestToExtendRestrictionThatWillLeaveItUnchanged::blockedValueForDimension, SignalAccessRestrictionBaseTestToExtendRestrictionThatWillLeaveItUnchanged::restrictedBit);
                    restriction->restrictAccessToBitRange(SignalAccessRestrictionBaseTestToExtendRestrictionThatWillLeaveItUnchanged::blockedDimension, SignalAccessRestrictionBaseTestToExtendRestrictionThatWillLeaveItUnchanged::blockedValueForDimension, SignalAccessRestrictionBaseTestToExtendRestrictionThatWillLeaveItUnchanged::bitRangeBeforeRestrictedBit);
                    restriction->restrictAccessToDimension(SignalAccessRestrictionBaseTestToExtendRestrictionThatWillLeaveItUnchanged::blockedDimension);
                    restriction->restrictAccessToBit(SignalAccessRestrictionBaseTestToExtendRestrictionThatWillLeaveItUnchanged::blockedDimension, SignalAccessRestrictionBaseTestToExtendRestrictionThatWillLeaveItUnchanged::restrictedBit);
                    restriction->restrictAccessToBitRange(SignalAccessRestrictionBaseTestToExtendRestrictionThatWillLeaveItUnchanged::blockedDimension, SignalAccessRestrictionBaseTestToExtendRestrictionThatWillLeaveItUnchanged::bitRangeBeforeRestrictedBit);
                },
                [](const std::size_t& bitPosition, const std::size_t&, const std::size_t&) {
                    return SignalAccessRestrictionBaseTestToExtendRestriction::isBitWithinRange(
                        bitPosition,
                        SignalAccessRestrictionBaseTestToExtendRestrictionThatWillLeaveItUnchanged::restrictedBitRange);
                })
        ),
        [](const testing::TestParamInfo<SignalAccessRestrictionBaseTestToExtendRestrictionThatWillLeaveItUnchanged::ParamType>& info) {
            return std::get<0>(info.param);
        });
/*
 * Test scenarios (lift restrictions):
 * 1.   
 * 2.
 * 3.
 */