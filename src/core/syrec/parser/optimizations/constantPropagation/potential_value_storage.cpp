#include "core/syrec/parser/optimizations/constantPropagation/potential_value_storage.hpp"

using namespace optimizations;

inline PotentialValueStorage PotentialValueStorage::initAsIntermediateLayer(const unsigned numEntries) {
    return PotentialValueStorage(Intermediate, createLayerData(numEntries));
}

inline PotentialValueStorage PotentialValueStorage::initAsValueLookupLayer(const unsigned numEntries, const unsigned defaultValue) {
    return PotentialValueStorage(Value, createLayerData(numEntries, defaultValue));
}

inline std::vector<std::variant<PotentialValueStorage, std::optional<unsigned>>> PotentialValueStorage::createLayerData(std::size_t numEntries) {
    return std::vector<std::variant<PotentialValueStorage, std::optional<unsigned int>>>(numEntries, PotentialValueStorage());
}

inline std::vector<std::variant<PotentialValueStorage, std::optional<unsigned>>> PotentialValueStorage::createLayerData(std::size_t numEntries, const unsigned defaultValue) {
    return std::vector<std::variant<PotentialValueStorage, std::optional<unsigned int>>>(numEntries, defaultValue);
}

void PotentialValueStorage::tryInvalidateStoredValue(const std::optional<unsigned int>& layerDataEntryIdx) {
    if (layerType == Intermediate) {
        return;
    }

    if (layerDataEntryIdx.has_value()) {
        if (layerDataEntryIdx < layerData.size()) {
            auto& layerDataEntry = layerData.at(*layerDataEntryIdx);
            if (std::holds_alternative<std::optional<unsigned int>>(layerDataEntry)) {
                auto& layerDataValue = std::get<std::optional<unsigned int>>(layerDataEntry);
                layerDataValue.reset();
            }
        }
    } else {
        for (auto& layerDataEntry : layerData) {
            if (std::holds_alternative<std::optional<unsigned int>>(layerDataEntry)) {
                auto& layerDataValue = std::get<std::optional<unsigned int>>(layerDataEntry);
                layerDataValue.reset();
            }
        }
    }
}

void PotentialValueStorage::tryUpdateStoredValue(const unsigned int layerDataEntryIdx, const unsigned newValue) {
    if (layerType == Intermediate || layerDataEntryIdx >= layerData.size()) {
        return;
    }

    auto& layerDataEntry = layerData.at(layerDataEntryIdx);
    if (std::holds_alternative<PotentialValueStorage>(layerDataEntry)) {
        return;
    }

    std::get<std::optional<unsigned int>>(layerDataEntry).emplace(newValue);
}

std::optional<unsigned> PotentialValueStorage::tryFetchStoredValue(unsigned layerDataEntryIdx, const std::optional<BitRangeAccessRestriction::BitRangeAccess>& bitRangeToFetch) const {
    if (layerType == Intermediate || layerDataEntryIdx >= layerData.size()) {
        return std::nullopt;
    }

    auto& layerDataEntry = layerData.at(layerDataEntryIdx);
    if (std::holds_alternative<PotentialValueStorage>(layerDataEntry)) {
        return std::nullopt;
    }

    const auto storedValueForEntry = std::get<std::optional<unsigned int>>(layerDataEntry);
    if (!bitRangeToFetch.has_value()) {
        return storedValueForEntry;
    }
    return std::make_optional(extractPartsOfValue(*storedValueForEntry, *bitRangeToFetch));
}

/*
 * TODO: We are assuming that the maximum signal length is equal to 32 bits ?
 */
inline unsigned PotentialValueStorage::extractPartsOfValue(const unsigned int value, const BitRangeAccessRestriction::BitRangeAccess& partToFetch) {
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

    return value & layerMask;
}

// TODO: Again we are assuming a maximum bit range of 32
void PotentialValueStorage::tryUpdatePartOfStoredValue(const unsigned int layerDataEntryIdx, const unsigned int newValue, const BitRangeAccessRestriction::BitRangeAccess& partsToUpdate) {
    if (layerType == Intermediate || layerDataEntryIdx >= layerData.size()) {
        return;
    }

    auto& layerDataEntry = layerData.at(layerDataEntryIdx);
    if (std::holds_alternative<PotentialValueStorage>(layerDataEntry)) {
        return;
    }

    auto& storedValueForEntry = std::get<std::optional<unsigned int>>(layerDataEntry);
    if (!storedValueForEntry.has_value()) {
        storedValueForEntry.emplace(0);
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

    unsigned int layerMask = UINT_MAX;
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

    const auto currentlyStoredValueWithAccessBitrangeZeroed = (*storedValueForEntry & layerMask) | (*storedValueForEntry & layerMaskForBitsBeforeAccessedBitRange);
    const auto newValueShifted                              = newValue << partsToUpdate.first;
    storedValueForEntry.emplace(currentlyStoredValueWithAccessBitrangeZeroed | newValueShifted);
}

