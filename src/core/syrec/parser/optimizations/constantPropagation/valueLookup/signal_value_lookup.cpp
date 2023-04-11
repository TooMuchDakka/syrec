#include "core/syrec/parser/optimizations/constantPropagation/valueLookup/signal_value_lookup.hpp"

using namespace valueLookup;

bool SignalValueLookup::canStoreValue(const std::any& value, const optimizations::BitRangeAccessRestriction::BitRangeAccess& availableStorageSpace) const {
    return isValueStorableInBitrange(std::any_cast<unsigned int>(value), availableStorageSpace);
}

bool SignalValueLookup::isValueStorableInBitrange(const unsigned int value, const optimizations::BitRangeAccessRestriction::BitRangeAccess& availableStorageSpace) {
    const auto resultContainerSize = (availableStorageSpace.second - availableStorageSpace.first) + 1;
    const auto maxStorableValue    = UINT_MAX >> (32 - resultContainerSize);
    return value <= maxStorableValue;
}

std::any SignalValueLookup::extractPartsOfValue(const std::any& value, const optimizations::BitRangeAccessRestriction::BitRangeAccess& availableStorageSpace) const {
    return extractPartsOfSignalValue(std::any_cast<unsigned int>(value), availableStorageSpace);
}

/*
 * TODO: We are assuming that the maximum signal length is equal to 32 bits ?
 */
unsigned int SignalValueLookup::extractPartsOfSignalValue(const unsigned int value, const optimizations::BitRangeAccessRestriction::BitRangeAccess& partToFetch) {
    if (partToFetch.first == partToFetch.second) {
        return value & (1 << partToFetch.first);
    }

    /*
 * Assuming we are working with a 8-bit wide signal:                        00011111 (0x1F)
 * Initially our layer mask is:                                             11111111
 * We are trying to access the bit range 2.4:                       
 * I.   Shift out bits before accessed bit range (L_SHIFT by (2 - 0)):      00111111
 * II.  Shift back layer mask to (R_SHIFT by (2 - 0)):                      11111100
 * III. Shift out bits after accessed bit range (R_SHIFT by (8 - 1) - 4):   11100000
 * IV. Shift back to create final layer mask (L_SHIFT by (8 - 1) - 4):      00011100
 */
    unsigned int layerMask = UINT_MAX;
    if (partToFetch.first > 0) {
        layerMask >>= partToFetch.first;
        layerMask <<= partToFetch.first;
    }

    if (partToFetch.second > 0) {
        const std::size_t shiftAmount = (32 - 1) - partToFetch.second;
        layerMask <<= shiftAmount;
        layerMask >>= shiftAmount;
    }

    if (partToFetch.first > 0) {
        return (value & layerMask) >> partToFetch.first;
    }
    return value & layerMask;
}

std::any SignalValueLookup::transformExistingValueByMergingWithNewOne(const std::any& currentValue, const std::any& newValue, const optimizations::BitRangeAccessRestriction::BitRangeAccess& partsToUpdate) const {
    return std::any_cast<unsigned int>(
            transformExistingSignalValueByMergingWithNewOne(
                    std::any_cast<unsigned int>(currentValue),
                    std::any_cast<unsigned int>(newValue),
                    partsToUpdate));
}


unsigned int SignalValueLookup::transformExistingSignalValueByMergingWithNewOne(const unsigned int currentValue, const unsigned int newValue, const optimizations::BitRangeAccessRestriction::BitRangeAccess& partsToUpdate) {
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

    unsigned int layerMask                              = UINT_MAX;
    unsigned int layerMaskForBitsBeforeAccessedBitRange = UINT_MAX;
    if (partsToUpdate.second > 0) {
        layerMask >>= partsToUpdate.second;
        layerMask <<= partsToUpdate.second;
    }

    if (partsToUpdate.first > 0) {
        const auto shiftAmount = (32 - 1) - partsToUpdate.first;
        layerMaskForBitsBeforeAccessedBitRange <<= shiftAmount;
        layerMaskForBitsBeforeAccessedBitRange >>= shiftAmount;
    }

    const auto currentValueWithAccessBitrangeZeroed = (currentValue & layerMask) | (currentValue & layerMaskForBitsBeforeAccessedBitRange);
    const auto newValueShifted                      = newValue << partsToUpdate.first;
    return currentValueWithAccessBitrangeZeroed | newValueShifted;
}