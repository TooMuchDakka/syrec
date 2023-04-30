#include "core/syrec/parser/utils/bit_helpers.hpp"
#include <cmath>

using namespace BitHelpers;

// TODO: Check that layer mask is correctly created when updating and extracting values to correctly filter out bits outside of the bit range (i.e. larger than the MSB of the accessed range)

bool BitHelpers::isValueStorableInBitRange(const unsigned int value, const ::optimizations::BitRangeAccessRestriction::BitRangeAccess& accessedBitRange) {
    const auto resultContainerSize = (accessedBitRange.second - accessedBitRange.first) + 1;
    return value <= getMaximumStorableValueInBitRange(resultContainerSize);
}

unsigned int BitHelpers::extractBitsFromValue(const unsigned int value, const ::optimizations::BitRangeAccessRestriction::BitRangeAccess& accessedBitRange) {
    if (accessedBitRange.first == accessedBitRange.second) {
        return (value >> accessedBitRange.first) & 1;
    }
    
    /*
     * Our approach is simple, we at first remove all bits prior to the first accessed one in our current value by a left shift by x positions (where x is our first accessed bit position)
     * To remove all bits after our last accessed bit, we again perform a left shift by y positions (with y being our last accessed bit) of an all 1-mask that is as long as our signal.
     * As a final extract step we AND combine our initially shifted value with our shifted 1-mask to extract the value.
     */
    unsigned int layerMask = UINT_MAX;
    if (accessedBitRange.second > 0) {
        const std::size_t lengthOfBitRange = (accessedBitRange.second - accessedBitRange.first) + 1;
        layerMask >>= (32 - lengthOfBitRange);    
    }

    return (value >> accessedBitRange.first) & layerMask;
}

unsigned int BitHelpers::getMaximumStorableValueInBitRange(const unsigned int numAccessedBits) {
    if (numAccessedBits == 0) {
        return 0;
    }

    return static_cast<unsigned int>(std::pow(2, numAccessedBits)) - 1;
}

unsigned int BitHelpers::mergeValues(const unsigned int currentValue, const unsigned int newValue, const ::optimizations::BitRangeAccessRestriction::BitRangeAccess& partsToUpdate) {
    if (partsToUpdate.first == partsToUpdate.second) {

        /*
         * We at first create the layer mask to zero the accessed bit in the current value
         * We then simply apply an OR operation of the value from the previous step with the
         * new value shifted to the accessed bit position. We are assuming that new value is either 0 or 1
         */
        constexpr auto oneBit                      = static_cast<unsigned int>(1);
        const unsigned int layerMask                   = ~(oneBit << partsToUpdate.first);
        const unsigned int extractValueOfBitOfNewValue = newValue & oneBit;
        return (currentValue & layerMask) | (extractValueOfBitOfNewValue << partsToUpdate.first);
    }

    /*
     * Assume for we are working with an 8-bit signal that currently has a value of 00011111 (0x1F) and we are updating the bit range .2:5 with the value 5 (101)
     * We at first zero out the accessed bit range in the stored value with the help of a layer mask
     * I. Initialize the layer mask to 11111111 (0xFF)
     * II. To extract the value after the accessed bit range, we perform a right shift by (8 - 5)                                               11000000    B1
     * III. A second bit mask is created to extract the bits before the start of the accessed bit range (by a left shift by (X - 1 - 2):        00000011    B2
     * IV. With the help of these two bitmask we can zero out the accessed bit range from the currently stored value:                           (00011111 & B1) | (00011111 & B2) = 00000011
     * V. We need to shift the new value to the now zeroed out bit range of the currently stored value (by a left shift by 2):                  00000101 << 2 = 00010100
     * VI. We can now calculate the new value by a simply OR operation of the result of IV. and V:                                              00000011 | 00010100 = 00010111
     */

    const std::size_t sizeOfRangeToUpdate = (partsToUpdate.second - partsToUpdate.first) + 1;
    const std::size_t  zeroMaskShiftAmountForEffectedRangeToBeSet = (32 - sizeOfRangeToUpdate);
    const unsigned int zeroMask = ~((UINT_MAX >> zeroMaskShiftAmountForEffectedRangeToBeSet) << partsToUpdate.first);

    return (currentValue & zeroMask) | (newValue << partsToUpdate.first);
}


