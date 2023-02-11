#include "syrec_parser_signal_access_restriction_fixture.hpp"

using namespace parser;

// TODO: Should there also be tests for using out of range indizes ?
// TODO: Tests to lift restrictions

// TODO: Tests for bits and status of whole signal for restriction tests
/*
* Test scenarios:
* Base:
* 1.   a += ...
* 2.   a.0 += ...
* 3.   a.0:5 += ...
* 4.   a[0] += ...
* 5.   a[0].0 += ...
* 6.   a[0].0:5 += ...
* 7.   a[$i] += ...
* 8.   a[$i].0 += ...
* 9.   a[$i].0:5 += ...
*
* Extension of restriction test:
* 1.   a.0 + a.2
* 2.   a.0:2 + a.2:5
* 3.   a[0] + a[1]   
* 4.   a[0] + a[$i]
* 5.   a[0] + a[x][0]
* 6.   a[$i] + a[x][$j]
* 7.   a[$i].0 + a[x][$j].0
* 8.   a[0].0 + a[x][0].0
* 9.   a[0].0:2 + a[$i].0:2
* 5.   a[0].0:2 + a[0].2:5
* 6.   a[0].0:2 + a[1].2:5
* 9.   a[0].0:2 + a[$i].2:5
* 9.   a[$i].0:2 + a.0:2
* 10.  a[0].0:2 + a.0:2
* 11.  a[0].0 + a
* 12.  a[0].0:2 + a
* 13.  a[0] + a
* 14.  a[$i] + a 
*/
INSTANTIATE_TEST_SUITE_P(
    SignalAccessRestrictionTests,
    SignalAccessRestrictionTest,
    ::testing::Values(
        std::make_tuple(
            "createRestrictionForWholeSignal",
            [](std::shared_ptr<SignalAccessRestriction>& restriction) {
                restriction->blockAccessOnSignalCompletely();
            },
            [](std::shared_ptr<SignalAccessRestriction>&) {},
            [](const SignalAccessRestriction::SignalAccess&) {
                return true;
            },
            [](const std::size_t&) {
                return true;
            },
            [](const std::size_t&, const std::size_t&) {
                return true;
            },
            [](const SignalAccessRestriction::SignalAccess&, const std::size_t&, const std::size_t&) {
                return true;
            }),
        std::make_tuple(
            "createRestrictionForBitOfWholeSignal",
            [](std::shared_ptr<SignalAccessRestriction>& restriction) {
                restriction->restrictAccessToBit(SignalAccessRestrictionTest::restrictedBit);
            },
            [](std::shared_ptr<SignalAccessRestriction>&) {},
            [](const SignalAccessRestriction::SignalAccess& bitRange) {
                return SignalAccessRestrictionTest::isBitWithinRange(SignalAccessRestrictionTest::restrictedBit, bitRange);    
            },
            [](const std::size_t&) {
                return true;
            },
            [](const std::size_t&, const std::size_t&) {
                return true;    
            },
            [](const SignalAccessRestriction::SignalAccess& bitRange, const std::size_t&, const std::size_t&) {
                return SignalAccessRestrictionTest::isBitWithinRange(SignalAccessRestrictionTest::restrictedBit, bitRange);    
            }),
        std::make_tuple(
            "createRestrictionForBitRangeOfWholeSignal",
            [](std::shared_ptr<SignalAccessRestriction>& restriction) {
                restriction->restrictAccessToBitRange(SignalAccessRestrictionTest::restrictedBitRange);
            },
            [](std::shared_ptr<SignalAccessRestriction>&) {},
            [](const SignalAccessRestriction::SignalAccess& bitRange) {
                return SignalAccessRestrictionTest::doesAccessIntersectRegion(SignalAccessRestrictionTest::restrictedBitRange, bitRange);
            },
            [](const std::size_t&) {
                return true;
            },
            [](const std::size_t&, const std::size_t&) {
                return true;    
            },
            [](const SignalAccessRestriction::SignalAccess& bitRange, const std::size_t&, const std::size_t&) {
                return SignalAccessRestrictionTest::doesAccessIntersectRegion(SignalAccessRestrictionTest::restrictedBitRange, bitRange);
            }),
        std::make_tuple(
            "createRestrictionForValueOfDimension",
            [](std::shared_ptr<SignalAccessRestriction>& restriction) {
                restriction->restrictAccessToValueOfDimension(SignalAccessRestrictionTest::blockedDimension, SignalAccessRestrictionTest::blockedValueForDimension);
            },
            [](std::shared_ptr<SignalAccessRestriction>&) {},
            [](const SignalAccessRestriction::SignalAccess&) {
                return true;    
            },
            [](const std::size_t& dimension) {
                return dimension == SignalAccessRestrictionTest::blockedDimension;
            },
            [](const std::size_t& dimension, const std::size_t& valueForDimension) {
                return dimension == SignalAccessRestrictionTest::blockedDimension && valueForDimension == SignalAccessRestrictionTest::blockedValueForDimension;
            },
            [](const SignalAccessRestriction::SignalAccess&, const std::size_t& dimension, const std::size_t& valueForDimension) {
                return dimension == SignalAccessRestrictionTest::blockedDimension && valueForDimension == SignalAccessRestrictionTest::blockedValueForDimension;
            }),
        std::make_tuple(
            "createRestrictionForBitOfValueOfDimension",
            [](std::shared_ptr<SignalAccessRestriction>& restriction) {
                restriction->restrictAccessToBit(SignalAccessRestrictionTest::blockedDimension, SignalAccessRestrictionTest::blockedValueForDimension, SignalAccessRestrictionTest::restrictedBit);
            },
            [](std::shared_ptr<SignalAccessRestriction>&) {},
            [](const SignalAccessRestriction::SignalAccess& bitRange) {
                return SignalAccessRestrictionTest::isBitWithinRange(SignalAccessRestrictionTest::restrictedBit, bitRange);
            },
            [](const std::size_t& dimension) {
                return dimension == SignalAccessRestrictionTest::blockedDimension;
            },
            [](const std::size_t& dimension, const std::size_t& valueForDimension) {
                return dimension == SignalAccessRestrictionTest::blockedDimension && valueForDimension == SignalAccessRestrictionTest::blockedValueForDimension;
            },
            [](const SignalAccessRestriction::SignalAccess& bitRange, const std::size_t& dimension, const std::size_t& valueForDimension) {
                return dimension == SignalAccessRestrictionTest::blockedDimension && valueForDimension == SignalAccessRestrictionTest::blockedValueForDimension
                    && SignalAccessRestrictionTest::isBitWithinRange(SignalAccessRestrictionTest::restrictedBit, bitRange);
            }),
        std::make_tuple(
            "createRestrictionForBitRangeOfValueOfDimension",
            [](std::shared_ptr<SignalAccessRestriction>& restriction) {
                restriction->restrictAccessToBitRange(SignalAccessRestrictionTest::blockedDimension, SignalAccessRestrictionTest::blockedValueForDimension, SignalAccessRestrictionTest::restrictedBitRange);
            },
            [](std::shared_ptr<SignalAccessRestriction>&) {},
            [](const SignalAccessRestriction::SignalAccess& bitRange) {
                return SignalAccessRestrictionTest::doesAccessIntersectRegion(SignalAccessRestrictionTest::restrictedBitRange, bitRange);
            },
            [](const std::size_t& dimension) {
                return dimension == SignalAccessRestrictionTest::blockedDimension;
            },
            [](const std::size_t& dimension, const std::size_t& valueForDimension) {
                return dimension == SignalAccessRestrictionTest::blockedDimension && valueForDimension == SignalAccessRestrictionTest::blockedValueForDimension;
            },
            [](const SignalAccessRestriction::SignalAccess& bitRange, const std::size_t& dimension, const std::size_t& valueForDimension) {
                 return dimension == SignalAccessRestrictionTest::blockedDimension && valueForDimension == SignalAccessRestrictionTest::blockedValueForDimension
                    && SignalAccessRestrictionTest::doesAccessIntersectRegion(SignalAccessRestrictionTest::restrictedBitRange, bitRange);
            }),
        std::make_tuple(
            "createRestrictionWholeDimension",
            [](std::shared_ptr<SignalAccessRestriction>& restriction) {
                restriction->restrictAccessToDimension(SignalAccessRestrictionTest::blockedDimension);
            },
            [](std::shared_ptr<SignalAccessRestriction>&) {},
            [](const SignalAccessRestriction::SignalAccess&) {
                return true;
            },
            [](const std::size_t& dimension) {
                return dimension == SignalAccessRestrictionTest::blockedDimension;
            },
            [](const std::size_t& dimension, const std::size_t&) {
                return dimension == SignalAccessRestrictionTest::blockedDimension;
            },
            [](const SignalAccessRestriction::SignalAccess&, const std::size_t& dimension, const std::size_t&) {
                return dimension == SignalAccessRestrictionTest::blockedDimension;
            }),
        std::make_tuple(
            "createRestrictionForBitOfWholeDimension",
            [](std::shared_ptr<SignalAccessRestriction>& restriction) {
                restriction->restrictAccessToBit(SignalAccessRestrictionTest::blockedDimension, SignalAccessRestrictionTest::restrictedBit);
            },
            [](std::shared_ptr<SignalAccessRestriction>&) {},
            [](const SignalAccessRestriction::SignalAccess& bitRange) {
                return SignalAccessRestrictionTest::isBitWithinRange(SignalAccessRestrictionTest::restrictedBit, bitRange);
            },
            [](const std::size_t& dimension) {
                return dimension == SignalAccessRestrictionTest::blockedDimension;
            },
            [](const std::size_t& dimension, const std::size_t&) {
                return dimension == SignalAccessRestrictionTest::blockedDimension;
            },
            [](const SignalAccessRestriction::SignalAccess& bitRange, const std::size_t& dimension, const std::size_t&) {
                return dimension == SignalAccessRestrictionTest::blockedDimension
                    && SignalAccessRestrictionTest::isBitWithinRange(SignalAccessRestrictionTest::restrictedBit, bitRange);
            }),
        std::make_tuple(
            "createRestrictionForBitRangeOfDimension",
            [](std::shared_ptr<SignalAccessRestriction>& restriction) {
                restriction->restrictAccessToBitRange(SignalAccessRestrictionTest::blockedDimension, SignalAccessRestrictionTest::restrictedBitRange);
            },
            [](std::shared_ptr<SignalAccessRestriction>&) {},
            [](const SignalAccessRestriction::SignalAccess& bitRange) {
                return SignalAccessRestrictionTest::doesAccessIntersectRegion(SignalAccessRestrictionTest::restrictedBitRange, bitRange);
            },
            [](const std::size_t& dimension) {
                return dimension == SignalAccessRestrictionTest::blockedDimension;
            },
            [](const std::size_t& dimension, const std::size_t&) {
                return dimension == SignalAccessRestrictionTest::blockedDimension;
            },
            [](const SignalAccessRestriction::SignalAccess& bitRange, const std::size_t& dimension, const std::size_t&) {
                return dimension == SignalAccessRestrictionTest::blockedDimension
                    && SignalAccessRestrictionTest::doesAccessIntersectRegion(SignalAccessRestrictionTest::restrictedBitRange, bitRange);
            }),
        std::make_tuple(
            "extendBitRestrictionForWholeSignal",
            [](std::shared_ptr<SignalAccessRestriction>& restriction) {
                restriction->restrictAccessToBit(SignalAccessRestrictionTest::restrictedBit);
            },
            [](std::shared_ptr<SignalAccessRestriction>& restriction) {
                restriction->restrictAccessToBitRange(SignalAccessRestrictionTest::restrictedBitRange);
            },
            [](const SignalAccessRestriction::SignalAccess& bitRange) {
                return SignalAccessRestrictionTest::doesAccessIntersectRegion(SignalAccessRestrictionTest::restrictedBitRange, bitRange)
                    || SignalAccessRestrictionTest::isBitWithinRange(SignalAccessRestrictionTest::restrictedBit, bitRange);
            },
            [](const std::size_t&) {
                return true;
            },
            [](const std::size_t&, const std::size_t&) {
                return true;
            },
            [](const SignalAccessRestriction::SignalAccess& bitRange, const std::size_t&, const std::size_t&) {
                return SignalAccessRestrictionTest::doesAccessIntersectRegion(SignalAccessRestrictionTest::restrictedBitRange, bitRange)
                    || SignalAccessRestrictionTest::isBitWithinRange(SignalAccessRestrictionTest::restrictedBit, bitRange);
            }),
        std::make_tuple(
            "extendBitRangeRestrictionForWholeSignal",
            [](const std::shared_ptr<SignalAccessRestriction>& restriction) {
                restriction->restrictAccessToBitRange(SignalAccessRestrictionTest::restrictedBitRange);
            },
            [](const std::shared_ptr<SignalAccessRestriction>& restriction) {
                restriction->restrictAccessToBitRange(SignalAccessRestrictionTest::bitRangeBeforeRestrictedBit);
            },
            [](const SignalAccessRestriction::SignalAccess& bitRange) {
                return SignalAccessRestrictionTest::doesAccessIntersectRegion(SignalAccessRestrictionTest::restrictedBitRange, bitRange)
                    || SignalAccessRestrictionTest::doesAccessIntersectRegion(SignalAccessRestrictionTest::bitRangeBeforeRestrictedBit, bitRange);
            },
            [](const std::size_t&) {
                return true;
            },
            [](const std::size_t&, const std::size_t&) {
                return true;
            },
            [](const SignalAccessRestriction::SignalAccess& bitRange, const std::size_t&, const std::size_t&) {
                return SignalAccessRestrictionTest::doesAccessIntersectRegion(SignalAccessRestrictionTest::restrictedBitRange, bitRange)
                    || SignalAccessRestrictionTest::doesAccessIntersectRegion(SignalAccessRestrictionTest::bitRangeBeforeRestrictedBit, bitRange);           
            }),
        std::make_tuple(
            "extendValueForDimensionRestrictionToOtherValue",
            [](const std::shared_ptr<SignalAccessRestriction>& restriction) {
                restriction->restrictAccessToValueOfDimension(SignalAccessRestrictionTest::blockedDimension, SignalAccessRestrictionTest::blockedValueForDimension);
            },
            [](const std::shared_ptr<SignalAccessRestriction>& restriction) {
                restriction->restrictAccessToValueOfDimension(SignalAccessRestrictionTest::blockedDimension, SignalAccessRestrictionTest::otherBlockedValueForDimension);
            },
            [](const SignalAccessRestriction::SignalAccess&) {
                return true;
            },
            [](const std::size_t& dimension) {
                return dimension == SignalAccessRestrictionTest::blockedDimension;
            },
            [](const std::size_t& dimension, const std::size_t& valueForDimension) {
                return dimension == SignalAccessRestrictionTest::blockedDimension
                    && (valueForDimension == SignalAccessRestrictionTest::blockedValueForDimension || valueForDimension == SignalAccessRestrictionTest::otherBlockedValueForDimension);
            },
            [](const SignalAccessRestriction::SignalAccess&, const std::size_t& dimension, const std::size_t& valueForDimension) {
                return dimension == SignalAccessRestrictionTest::blockedDimension && (valueForDimension == SignalAccessRestrictionTest::blockedValueForDimension || valueForDimension == SignalAccessRestrictionTest::otherBlockedValueForDimension);
            }),
        std::make_tuple(
            "extendValueForDimensionRestrictionToWholeDimension",
            [](const std::shared_ptr<SignalAccessRestriction>& restriction) {
                restriction->restrictAccessToValueOfDimension(SignalAccessRestrictionTest::blockedDimension, SignalAccessRestrictionTest::blockedValueForDimension);
            },
            [](const std::shared_ptr<SignalAccessRestriction>& restriction) {
                restriction->restrictAccessToDimension(SignalAccessRestrictionTest::blockedDimension);
            },
            [](const SignalAccessRestriction::SignalAccess&) {
                return true;
            },
            [](const std::size_t& dimension) {
                return dimension == SignalAccessRestrictionTest::blockedDimension;
            },
            [](const std::size_t& dimension, const std::size_t&) {
                return dimension == SignalAccessRestrictionTest::blockedDimension;
            },
            [](const SignalAccessRestriction::SignalAccess&, const std::size_t& dimension, const std::size_t&) {
                return dimension == SignalAccessRestrictionTest::blockedDimension;
            }),
        std::make_tuple(
            "extendValueForDimensionRestrictionToAnotherDimension",
            [](const std::shared_ptr<SignalAccessRestriction>& restriction) {
                restriction->restrictAccessToValueOfDimension(SignalAccessRestrictionTest::blockedDimension, SignalAccessRestrictionTest::blockedValueForDimension);
            },
            [](const std::shared_ptr<SignalAccessRestriction>& restriction) {
                restriction->restrictAccessToValueOfDimension(SignalAccessRestrictionTest::otherBlockedDimension, SignalAccessRestrictionTest::blockedValueForOtherDimension);
            },
            [](const SignalAccessRestriction::SignalAccess&) {
                return true;
            },
            [](const std::size_t& dimension) {
                return dimension == SignalAccessRestrictionTest::blockedDimension
                    || dimension == SignalAccessRestrictionTest::otherBlockedDimension;
            },
            [](const std::size_t& dimension, const std::size_t& valueForDimension) {
                return (dimension == SignalAccessRestrictionTest::blockedDimension && valueForDimension == SignalAccessRestrictionTest::blockedValueForDimension)
                    || (dimension == SignalAccessRestrictionTest::otherBlockedDimension && valueForDimension == SignalAccessRestrictionTest::blockedValueForOtherDimension);
            },
            [](const SignalAccessRestriction::SignalAccess&, const std::size_t& dimension, const std::size_t& valueForDimension) {
                return (dimension == SignalAccessRestrictionTest::blockedDimension && valueForDimension == SignalAccessRestrictionTest::blockedValueForDimension)
                    || (dimension == SignalAccessRestrictionTest::otherBlockedDimension && valueForDimension == SignalAccessRestrictionTest::blockedValueForOtherDimension);
            }),
        std::make_tuple(
            "extendDimensionRestrictionToAnotherDimension",
            [](const std::shared_ptr<SignalAccessRestriction>& restriction) {
                restriction->restrictAccessToDimension(SignalAccessRestrictionTest::blockedDimension);
            },
            [](const std::shared_ptr<SignalAccessRestriction>& restriction) {
                restriction->restrictAccessToDimension(SignalAccessRestrictionTest::otherBlockedDimension);
            },
            [](const SignalAccessRestriction::SignalAccess&) {
                return true;
            },
            [](const std::size_t& dimension) {
                return dimension == SignalAccessRestrictionTest::blockedDimension
                    || dimension == SignalAccessRestrictionTest::otherBlockedDimension;
            },
            [](const std::size_t& dimension, const std::size_t&) {
                return dimension == SignalAccessRestrictionTest::blockedDimension
                    || dimension == SignalAccessRestrictionTest::otherBlockedDimension;
            },
            [](const SignalAccessRestriction::SignalAccess&, const std::size_t& dimension, const std::size_t&) {
                return dimension == SignalAccessRestrictionTest::blockedDimension
                    || dimension == SignalAccessRestrictionTest::otherBlockedDimension;
            }),
        std::make_tuple(
            "extendBitRestrictionForDimensionToAnotherDimension",
            [](const std::shared_ptr<SignalAccessRestriction>& restriction) {
                restriction->restrictAccessToBit(SignalAccessRestrictionTest::blockedDimension, SignalAccessRestrictionTest::restrictedBit);
            },
            [](const std::shared_ptr<SignalAccessRestriction>& restriction) {
                restriction->restrictAccessToBit(SignalAccessRestrictionTest::otherBlockedDimension, SignalAccessRestrictionTest::restrictedBit);
            },
            [](const SignalAccessRestriction::SignalAccess& bitRange) {
                return SignalAccessRestrictionTest::isBitWithinRange(SignalAccessRestrictionTest::restrictedBit, bitRange);
            },
            [](const std::size_t& dimension) {
                return dimension == SignalAccessRestrictionTest::blockedDimension
                    || dimension == SignalAccessRestrictionTest::otherBlockedDimension;
            },
            [](const std::size_t& dimension, const std::size_t&) {
                return dimension == SignalAccessRestrictionTest::blockedDimension
                    || dimension == SignalAccessRestrictionTest::otherBlockedDimension;
            },
            [](const SignalAccessRestriction::SignalAccess& bitRange, const std::size_t&, const std::size_t&) {
                return SignalAccessRestrictionTest::isBitWithinRange(SignalAccessRestrictionTest::restrictedBit, bitRange);
            }),
        std::make_tuple(
            "extendBitRangeRestrictionForValueOfDimensionToValueOfAnotherDimension",
            [](const std::shared_ptr<SignalAccessRestriction>& restriction) {
                restriction->restrictAccessToBitRange(SignalAccessRestrictionTest::blockedDimension, SignalAccessRestrictionTest::blockedValueForDimension, SignalAccessRestrictionTest::bitRangeBeforeRestrictedBit);
            },
            [](const std::shared_ptr<SignalAccessRestriction>& restriction) {
                restriction->restrictAccessToBitRange(SignalAccessRestrictionTest::otherBlockedDimension, SignalAccessRestrictionTest::blockedValueForOtherDimension, SignalAccessRestrictionTest::bitRangeBeforeRestrictedBit);
            },
            [](const SignalAccessRestriction::SignalAccess& bitRange) {
                return SignalAccessRestrictionTest::doesAccessIntersectRegion(SignalAccessRestrictionTest::bitRangeBeforeRestrictedBit, bitRange);
            },
            [](const std::size_t& dimension) {
                return dimension == SignalAccessRestrictionTest::blockedDimension
                    || dimension == SignalAccessRestrictionTest::otherBlockedDimension;
            },
            [](const std::size_t& dimension, const std::size_t& valueForDimension) {
                return (dimension == SignalAccessRestrictionTest::blockedDimension && valueForDimension == SignalAccessRestrictionTest::blockedValueForDimension)
                    || (dimension == SignalAccessRestrictionTest::otherBlockedDimension && valueForDimension == SignalAccessRestrictionTest::blockedValueForOtherDimension);
            },
            [](const SignalAccessRestriction::SignalAccess& bitRange, const std::size_t& dimension, const std::size_t& valueForDimension) {
                return ((dimension == SignalAccessRestrictionTest::blockedDimension && valueForDimension == SignalAccessRestrictionTest::blockedValueForDimension) || (dimension == SignalAccessRestrictionTest::otherBlockedDimension && valueForDimension == SignalAccessRestrictionTest::blockedValueForOtherDimension))
                    && SignalAccessRestrictionTest::doesAccessIntersectRegion(SignalAccessRestrictionTest::bitRangeBeforeRestrictedBit, bitRange);
            }),
        std::make_tuple(
            "extendBitRangeForValueOfDimensionRestrictionToAllValuesForDimension",
            [](const std::shared_ptr<SignalAccessRestriction>& restriction) {
                restriction->restrictAccessToBitRange(SignalAccessRestrictionTest::blockedDimension, SignalAccessRestrictionTest::blockedValueForDimension, SignalAccessRestrictionTest::bitRangeBeforeRestrictedBit);
            },
            [](const std::shared_ptr<SignalAccessRestriction>& restriction) {
                restriction->restrictAccessToBitRange(SignalAccessRestrictionTest::blockedDimension, SignalAccessRestrictionTest::bitRangeBeforeRestrictedBit);
            },
            [](const SignalAccessRestriction::SignalAccess& bitRange) {
                return SignalAccessRestrictionTest::doesAccessIntersectRegion(SignalAccessRestrictionTest::bitRangeBeforeRestrictedBit, bitRange);
            },
            [](const std::size_t& dimension) {
                return dimension == SignalAccessRestrictionTest::blockedDimension;
            },
            [](const std::size_t& dimension, const std::size_t&) {
                return dimension == SignalAccessRestrictionTest::blockedDimension;
            },
            [](const SignalAccessRestriction::SignalAccess& bitRange, const std::size_t& dimension, const std::size_t&) {
                return dimension == SignalAccessRestrictionTest::blockedDimension
                    && SignalAccessRestrictionTest::doesAccessIntersectRegion(SignalAccessRestrictionTest::bitRangeBeforeRestrictedBit, bitRange);
            }),
        std::make_tuple(
            "extendBitRangeForValueOfDimension",
            [](const std::shared_ptr<SignalAccessRestriction>& restriction) {
                restriction->restrictAccessToBitRange(SignalAccessRestrictionTest::blockedDimension, SignalAccessRestrictionTest::blockedValueForDimension, SignalAccessRestrictionTest::bitRangeBeforeRestrictedBit);
            },
            [](const std::shared_ptr<SignalAccessRestriction>& restriction) {
                restriction->restrictAccessToBitRange(SignalAccessRestrictionTest::blockedDimension, SignalAccessRestrictionTest::blockedValueForDimension, SignalAccessRestrictionTest::restrictedBitRange);
            },
            [](const SignalAccessRestriction::SignalAccess& bitRange) {
                return SignalAccessRestrictionTest::doesAccessIntersectRegion(SignalAccessRestrictionTest::bitRangeBeforeRestrictedBit, bitRange)
                    || SignalAccessRestrictionTest::doesAccessIntersectRegion(SignalAccessRestrictionTest::restrictedBitRange, bitRange);
            },
            [](const std::size_t& dimension) {
                return dimension == SignalAccessRestrictionTest::blockedDimension;
            },
            [](const std::size_t& dimension, const std::size_t& valueForDimension) {
                return dimension == SignalAccessRestrictionTest::blockedDimension
                    && valueForDimension == SignalAccessRestrictionTest::blockedValueForDimension;
            },
            [](const SignalAccessRestriction::SignalAccess& bitRange, const std::size_t& dimension, const std::size_t& valueForDimension) {
                return (dimension == SignalAccessRestrictionTest::blockedDimension && valueForDimension == SignalAccessRestrictionTest::blockedValueForDimension)
                    && (SignalAccessRestrictionTest::doesAccessIntersectRegion(SignalAccessRestrictionTest::bitRangeBeforeRestrictedBit, bitRange) || SignalAccessRestrictionTest::doesAccessIntersectRegion(SignalAccessRestrictionTest::restrictedBitRange, bitRange));
            }),
        std::make_tuple(
            "extendBitRangeForValueOfDimensionRestrictingByAnotherBitRangeForAnotherValueOfAnotherDimensionRestriction",
            [](const std::shared_ptr<SignalAccessRestriction>& restriction) {
                restriction->restrictAccessToBitRange(SignalAccessRestrictionTest::blockedDimension, SignalAccessRestrictionTest::blockedValueForDimension, SignalAccessRestrictionTest::bitRangeBeforeRestrictedBit);
            },
            [](const std::shared_ptr<SignalAccessRestriction>& restriction) {
                restriction->restrictAccessToBitRange(SignalAccessRestrictionTest::otherBlockedDimension, SignalAccessRestrictionTest::blockedValueForOtherDimension, SignalAccessRestrictionTest::restrictedBitRange);
            },
            [](const SignalAccessRestriction::SignalAccess& bitRange) {
                return SignalAccessRestrictionTest::doesAccessIntersectRegion(SignalAccessRestrictionTest::bitRangeBeforeRestrictedBit, bitRange)
                    || SignalAccessRestrictionTest::doesAccessIntersectRegion(SignalAccessRestrictionTest::restrictedBitRange, bitRange);
            },
            [](const std::size_t& dimension) {
                return dimension == SignalAccessRestrictionTest::blockedDimension
                    || dimension == SignalAccessRestrictionTest::otherBlockedDimension;
            },
            [](const std::size_t& dimension, const std::size_t& valueForDimension) {
                return (dimension == SignalAccessRestrictionTest::blockedDimension && valueForDimension == SignalAccessRestrictionTest::blockedValueForDimension)
                    || (dimension == SignalAccessRestrictionTest::otherBlockedDimension && valueForDimension == SignalAccessRestrictionTest::blockedValueForOtherDimension);
            },
            [](const SignalAccessRestriction::SignalAccess& bitRange, const std::size_t& dimension, const std::size_t& valueForDimension) {
                return (dimension == SignalAccessRestrictionTest::blockedDimension && valueForDimension == SignalAccessRestrictionTest::blockedValueForDimension && SignalAccessRestrictionTest::doesAccessIntersectRegion(SignalAccessRestrictionTest::bitRangeBeforeRestrictedBit, bitRange))
                    || (dimension == SignalAccessRestrictionTest::otherBlockedDimension && valueForDimension == SignalAccessRestrictionTest::blockedValueForOtherDimension && SignalAccessRestrictionTest::doesAccessIntersectRegion(SignalAccessRestrictionTest::restrictedBitRange, bitRange));
            }),
        std::make_tuple(
            "extendBitRangeForValueOfDimensionRestrictionByAnotherBitRangeForAllValuesOfSameDimensionRestriction",
            [](const std::shared_ptr<SignalAccessRestriction>& restriction) {
                restriction->restrictAccessToBitRange(SignalAccessRestrictionTest::blockedDimension, SignalAccessRestrictionTest::blockedValueForDimension, SignalAccessRestrictionTest::bitRangeBeforeRestrictedBit);
            },
            [](const std::shared_ptr<SignalAccessRestriction>& restriction) {
                restriction->restrictAccessToBitRange(SignalAccessRestrictionTest::blockedDimension, SignalAccessRestrictionTest::restrictedBitRange);
            },
            [](const SignalAccessRestriction::SignalAccess& bitRange) {
                return SignalAccessRestrictionTest::doesAccessIntersectRegion(SignalAccessRestrictionTest::bitRangeBeforeRestrictedBit, bitRange)
                    || SignalAccessRestrictionTest::doesAccessIntersectRegion(SignalAccessRestrictionTest::restrictedBitRange, bitRange);
            },
            [](const std::size_t& dimension) {
                return dimension == SignalAccessRestrictionTest::blockedDimension;
            },
            [](const std::size_t& dimension, const std::size_t&) {
                return dimension == SignalAccessRestrictionTest::blockedDimension;
            },
            [](const SignalAccessRestriction::SignalAccess& bitRange, const std::size_t& dimension, const std::size_t&) {
                return dimension == SignalAccessRestrictionTest::blockedDimension
                    && (SignalAccessRestrictionTest::doesAccessIntersectRegion(SignalAccessRestrictionTest::bitRangeBeforeRestrictedBit, bitRange) || SignalAccessRestrictionTest::doesAccessIntersectRegion(SignalAccessRestrictionTest::restrictedBitRange, bitRange));
            }),
        std::make_tuple(
            "extendBitRangeRestrictionForOneDimensionToBitRangeOfWholeSignal",
            [](const std::shared_ptr<SignalAccessRestriction>& restriction) {
                restriction->restrictAccessToBitRange(SignalAccessRestrictionTest::blockedDimension, SignalAccessRestrictionTest::blockedValueForDimension, SignalAccessRestrictionTest::bitRangeBeforeRestrictedBit);
            },
            [](const std::shared_ptr<SignalAccessRestriction>& restriction) {
                restriction->restrictAccessToBitRange(SignalAccessRestrictionTest::bitRangeBeforeRestrictedBit);
            },
            [](const SignalAccessRestriction::SignalAccess& bitRange) {
                return SignalAccessRestrictionTest::doesAccessIntersectRegion(SignalAccessRestrictionTest::bitRangeBeforeRestrictedBit, bitRange);
            },
            [](const std::size_t&) {
                return true;
            },
            [](const std::size_t&, const std::size_t&) {
                return true;
            },
            [](const SignalAccessRestriction::SignalAccess& bitRange, const std::size_t&, const std::size_t&) {
                return SignalAccessRestrictionTest::doesAccessIntersectRegion(SignalAccessRestrictionTest::bitRangeBeforeRestrictedBit, bitRange);
            }),
        std::make_tuple(
            "extendBitRangeForDimensionRestrictionToBitRangeOfWholeSignal",
            [](const std::shared_ptr<SignalAccessRestriction>& restriction) {
                restriction->restrictAccessToBitRange(SignalAccessRestrictionTest::blockedDimension, SignalAccessRestrictionTest::blockedValueForDimension, SignalAccessRestrictionTest::bitRangeBeforeRestrictedBit);
            },
            [](const std::shared_ptr<SignalAccessRestriction>& restriction) {
                restriction->restrictAccessToBitRange(SignalAccessRestrictionTest::bitRangeBeforeRestrictedBit);
            },
            [](const SignalAccessRestriction::SignalAccess& bitRange) {
                return SignalAccessRestrictionTest::doesAccessIntersectRegion(SignalAccessRestrictionTest::bitRangeBeforeRestrictedBit, bitRange);
            },
            [](const std::size_t&) {
                return true;
            },
            [](const std::size_t&, const std::size_t&) {
                return true;
            },
            [](const SignalAccessRestriction::SignalAccess& bitRange, const std::size_t&, const std::size_t&) {
                return SignalAccessRestrictionTest::doesAccessIntersectRegion(SignalAccessRestrictionTest::bitRangeBeforeRestrictedBit, bitRange);
            }),
        std::make_tuple(
            "extendBitRestrictionToWholeSignal",
            [](const std::shared_ptr<SignalAccessRestriction>& restriction) {
                restriction->restrictAccessToBit(SignalAccessRestrictionTest::blockedDimension, SignalAccessRestrictionTest::blockedValueForDimension, SignalAccessRestrictionTest::bitRangeBeforeRestrictedBit.start);
            },
            [](const std::shared_ptr<SignalAccessRestriction>& restriction) {
                restriction->blockAccessOnSignalCompletely();
            },
            [](const SignalAccessRestriction::SignalAccess&) {
                return true;
            },
            [](const std::size_t&) {
                return true;
            },
            [](const std::size_t&, const std::size_t&) {
                return true;
            },
            [](const SignalAccessRestriction::SignalAccess&, const std::size_t&, const std::size_t&) {
                return true;
            }),
        std::make_tuple(
            "extendBitRangeRestrictionToWholeSignal",
            [](const std::shared_ptr<SignalAccessRestriction>& restriction) {
                restriction->restrictAccessToBitRange(SignalAccessRestrictionTest::blockedDimension, SignalAccessRestrictionTest::blockedValueForDimension, SignalAccessRestrictionTest::bitRangeBeforeRestrictedBit);
            },
            [](const std::shared_ptr<SignalAccessRestriction>& restriction) {
                restriction->blockAccessOnSignalCompletely();
            },
            [](const SignalAccessRestriction::SignalAccess&) {
                return true;
            },
            [](const std::size_t&) {
                return true;
            },
            [](const std::size_t&, const std::size_t&) {
                return true;
            },
            [](const SignalAccessRestriction::SignalAccess&, const std::size_t&, const std::size_t&) {
                return true;
            }),
        std::make_tuple(
            "extendValueForDimensionRestrictionToWholeSignal",
            [](const std::shared_ptr<SignalAccessRestriction>& restriction) {
                restriction->restrictAccessToValueOfDimension(SignalAccessRestrictionTest::blockedDimension, SignalAccessRestrictionTest::blockedValueForDimension);
            },
            [](const std::shared_ptr<SignalAccessRestriction>& restriction) {
                restriction->blockAccessOnSignalCompletely();
            },
            [](const SignalAccessRestriction::SignalAccess&) {
                return true;
            },
            [](const std::size_t&) {
                return true;
            },
            [](const std::size_t&, const std::size_t&) {
                return true;
            },
            [](const SignalAccessRestriction::SignalAccess&, const std::size_t&, const std::size_t&) {
                return true;
            }),
        std::make_tuple(
            "extendCompletelyBlockedDimensionToWholeSignal",
            [](const std::shared_ptr<SignalAccessRestriction>& restriction) {
                restriction->restrictAccessToDimension(SignalAccessRestrictionTest::blockedDimension);
            },
            [](const std::shared_ptr<SignalAccessRestriction>& restriction) {
                restriction->blockAccessOnSignalCompletely();
            },
            [](const SignalAccessRestriction::SignalAccess&) {
                return true;
            },
            [](const std::size_t&) {
                return true;
            },
            [](const std::size_t&, const std::size_t&) {
                return true;
            },
            [](const SignalAccessRestriction::SignalAccess&, const std::size_t&, const std::size_t&) {
                return true;
            })
    ),
    [](const testing::TestParamInfo<SignalAccessRestrictionTest::ParamType>& info) {
        return std::get<0>(info.param);
    });
///*
// * Test scenarios (extend restriction with restriction that will not extend the existing one)
// * 1.   a += ...; a.0 += ...; a.2:5 += ...; a[0] += ...; a[0].2 += ...; a[0].2:10 += ...; a[$i] += ...; a[$i].0 += ...; a[$i].0:8 += ...
// * 2.   a.0 += ...; a[0].0 += ...; a[$i].0 += ...
// * 3.   a.2:5 += ...; a.3 += ...; a.5 += ...; a:2:4 += ...; a[0].2 += ...; a[0].2:5 += ...; a[$i].2 += ...; a[$i].2:5 += ...
// * 4.   a[0] += ...; a[0].2 += ...; a[0].2:5 += ...; a[0] += ...
// * 5.   a[$i] += ...; a.0 += ...; a.2:5 += ...; a[0] += ...; a[0].2 += ...; a[0].2:10 += ...; a[$i] += ...; a[$i].0 += ...; a[$i].0:8 += ...
// * 6.   a[0].0 += ...; a[0].0 += ...
// * 7.   a[0].2:5 += ...; a[0].2 += ...; a[0].2:4 += ...;
// * 8.   a[$i].0 += ...; a[0].0 += ...; a[$i].0 += ...;
// * 9.   a[$i].2:5 += ...; a[0].2 += ...; a[0].4 += ...; a[$i].2 += ...; a[$i].4 += ...;      
// */
// INSTANTIATE_TEST_SUITE_P(
//    SignalAccessRestrictionTests,
//    SignalAccessRestrictionTestThatWillLeaveItUnchanged,
//    ::testing::Values(
//            std::make_tuple(
//                "extendGlobalSignalRestriction",
//                [](SignalAccessRestrictionTest::RestrictionPtr& restriction) {
//                    restriction->blockAccessOnSignalCompletely();
//                },
//                [](SignalAccessRestrictionTest::RestrictionPtr& restriction) {
//                    restriction->restrictAccessToBit(SignalAccessRestrictionTestThatWillLeaveItUnchanged::restrictedBit);
//                    restriction->restrictAccessToBitRange(SignalAccessRestrictionTestThatWillLeaveItUnchanged::bitRangeBeforeRestrictedBit);
//                    restriction->restrictAccessToValueOfDimension(SignalAccessRestrictionTestThatWillLeaveItUnchanged::blockedDimension, SignalAccessRestrictionTestThatWillLeaveItUnchanged::blockedValueForDimension);
//                    restriction->restrictAccessToBit(SignalAccessRestrictionTestThatWillLeaveItUnchanged::blockedDimension, SignalAccessRestrictionTestThatWillLeaveItUnchanged::blockedValueForDimension, SignalAccessRestrictionTestThatWillLeaveItUnchanged::restrictedBit);
//                    restriction->restrictAccessToBitRange(SignalAccessRestrictionTestThatWillLeaveItUnchanged::blockedDimension, SignalAccessRestrictionTestThatWillLeaveItUnchanged::blockedValueForDimension, SignalAccessRestrictionTestThatWillLeaveItUnchanged::bitRangeBeforeRestrictedBit);
//                    restriction->restrictAccessToDimension(SignalAccessRestrictionTestThatWillLeaveItUnchanged::blockedDimension);
//                    restriction->restrictAccessToBit(SignalAccessRestrictionTestThatWillLeaveItUnchanged::blockedDimension, SignalAccessRestrictionTestThatWillLeaveItUnchanged::restrictedBit);
//                    restriction->restrictAccessToBitRange(SignalAccessRestrictionTestThatWillLeaveItUnchanged::blockedDimension, SignalAccessRestrictionTestThatWillLeaveItUnchanged::bitRangeBeforeRestrictedBit);
//                },
//                [](const std::size_t& bitPosition, const std::size_t&, const std::size_t&) {
//                    return SignalAccessRestrictionTest::isBitWithinRange(
//                        bitPosition,
//                        SignalAccessRestrictionTestThatWillLeaveItUnchanged::restrictedBitRange);
//                })
//        ),
//        [](const testing::TestParamInfo<SignalAccessRestrictionTestThatWillLeaveItUnchanged::ParamType>& info) {
//            return std::get<0>(info.param);
//        });
/*
 * Test scenarios (lift restrictions):
 * 1.   
 * 2.
 * 3.
 */