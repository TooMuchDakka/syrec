#include "core/syrec/parser/optimizations/constantPropagation/signal_value_lookup.hpp"

#include <algorithm>

using namespace optimizations;

void SignalValueLookup::invalidateAllStoredValuesForSignal() const {
    valueLookup->applyRecursively([](std::map<unsigned int, std::optional<unsigned int>> layerValueLookup) {
        layerValueLookup.clear();
    });

    dimensionAccessRestrictions->layerData->blockSubstitutionForDimension(std::nullopt);

    const auto& nextLayerRestrictions = dimensionAccessRestrictions->nextLayerLinks;
    for (const auto& nextLayerRestriction : nextLayerRestrictions) {
        nextLayerRestriction->applyRecursively([](const DimensionPropagationBlocker::ptr& dim) {
            dim->liftRestrictionForWholeDimension();
        });
    }

    valueLookup->applyOnLastLayer(signalInformation.valuesPerDimension.size() - 1, [](std::map<unsigned int, std::optional<unsigned int>>& perValueOfDimensionValueLookup) {
        perValueOfDimensionValueLookup.clear();
    });
}

void SignalValueLookup::invalidateStoredValueFor(const std::vector<std::optional<unsigned>>& accessedDimensions) const {
    if (accessedDimensions.empty()) {
        invalidateAllStoredValuesForSignal();
        return;
    }

    // Skip extra work if accessed parts of signal are already blocked
    if (dimensionAccessRestrictions->doesPredicateHoldInDimensionThenCheckRecursivelyOtherwiseStop(
                0,
                accessedDimensions,
                [](const unsigned int _, const std::optional<unsigned int>& accessedValueOfDimension, const DimensionPropagationBlocker::ptr& dimensionPropagationBlocker) {
                    return !dimensionPropagationBlocker->isSubstitutionBlockedFor(accessedValueOfDimension, std::nullopt, true, true);
                })) {
        return;
    }

    dimensionAccessRestrictions->applyOnLastAccessedDimension(
        0,
        accessedDimensions,
        [](const DimensionPropagationBlocker::ptr& dimensionPropagationBlocker, const std::vector<LayerData<DimensionPropagationBlocker::ptr>::ptr>& nextLayerLinks, const std::optional<unsigned int>& valueOfLastAccessedDimension, const std::optional<BitRangeAccessRestriction::BitRangeAccess>& accessedBitRange) {
            dimensionPropagationBlocker->blockSubstitutionForDimension(valueOfLastAccessedDimension);
            for (const auto& nextLayerLink: nextLayerLinks) {
                nextLayerLink->applyRecursively([](const DimensionPropagationBlocker::ptr& nextLayerDimensionPropagationBlocker) {
                    nextLayerDimensionPropagationBlocker->liftRestrictionForWholeDimension();
                });
            }
        });

    valueLookup->applyOnLastAccessedDimension(
        0,
        accessedDimensions,
        [](std::map<unsigned int, std::optional<unsigned int>>& layerValueLookup, const std::vector<LayerData<std::map<unsigned int, std::optional<unsigned int>>>::ptr>& nextLayerLinks, const std::optional<unsigned int>& valueOfLastAccessedDimension, const std::optional<BitRangeAccessRestriction::BitRangeAccess>& accessedBitRange) {
            if (valueOfLastAccessedDimension.has_value()) {
                if (layerValueLookup.count(*valueOfLastAccessedDimension) > 0) {
                    layerValueLookup.erase(*valueOfLastAccessedDimension);
                }
            } else {
                layerValueLookup.clear();
            }
        });
}

void SignalValueLookup::invalidateStoredValueForBitrange(const std::vector<std::optional<unsigned>>& accessedDimensions, const BitRangeAccessRestriction::BitRangeAccess& bitRange) const {
    if (accessedDimensions.empty()) {
        if ((bitRange.second - bitRange.first) + 1 == signalInformation.bitWidth) {
            invalidateAllStoredValuesForSignal();
            return;
        }
    }

     // Skip extra work if accessed parts of signal are already blocked
    if (dimensionAccessRestrictions->doesPredicateHoldInDimensionThenCheckRecursivelyOtherwiseStop(
                0,
                accessedDimensions,
                [bitRange](const unsigned int _, const std::optional<unsigned int>& accessedValueOfDimension, const DimensionPropagationBlocker::ptr& dimensionPropagationBlocker) {
                    return !dimensionPropagationBlocker->isSubstitutionBlockedFor(accessedValueOfDimension, bitRange, false, true);
                })) {
        return;
    }

     dimensionAccessRestrictions->applyOnLastAccessedDimension(
            0,
            accessedDimensions,
            [](const DimensionPropagationBlocker::ptr& dimensionPropagationBlocker, const std::vector<LayerData<DimensionPropagationBlocker::ptr>::ptr>& nextLayerLinks, const std::optional<unsigned int>& valueOfLastAccessedDimension, const std::optional<BitRangeAccessRestriction::BitRangeAccess>& accessedBitRange) {
                const auto& bitRangeEvaluated = *accessedBitRange;
                dimensionPropagationBlocker->blockSubstitutionForBitRange(valueOfLastAccessedDimension, *accessedBitRange);

                // Since we already blocked the bit range at the "parent" dimension, we can lift the restriction on the bit range from all subsequent dimensions
                for (const auto& nextLayerLink : nextLayerLinks) {
                    nextLayerLink->applyRecursively([bitRangeEvaluated](const DimensionPropagationBlocker::ptr& nextLayerDimensionPropagationBlocker) {
                        nextLayerDimensionPropagationBlocker->liftRestrictionForBitRange(std::nullopt, bitRangeEvaluated);
                    });
                }
            });
}

void SignalValueLookup::liftRestrictionsOfDimensions(const std::vector<unsigned int>& accessedDimension, const std::optional<BitRangeAccessRestriction::BitRangeAccess>& bitRange) {
    const auto isWholeDimensionBlocked = dimensionAccessRestrictions->layerData->isSubstitutionBlockedFor(std::nullopt, std::nullopt);

    // We have to distinguish the following cases
    // I.   In case the whole dimension was blocked, we need to block the other values for the dimension other than the accessed one
    // II.  In case there existing a dimension wide bit range restriction, we need to lift the restriction for the given bitrange from the dimension ana recreated it for the other values of the dimension as in I.
    // III. Otherwise simple lift the restrictions
	
	/* When we lift the restriction for a bit range we might need to lift a restriction in a parent dimension (i.e. bit range a[$i].1:3 is locked in parent while a[0][2].4:5 is locked in child)
	 * if we would lift the restriction for a[0][2].1:5 we need to lift the one in the parent (where the dimension propagation should already handle the recreation for the remaining valus of the first dimension)
	 * and subsequently in the child. They same must be done when lifting a[$i][2].1:5 (but again for the parent dimension, the dimension propagation blocker should already do the majority of the lifting)
	*
	*/
}

inline void SignalValueLookup::liftRestrictionsFromWholeSignal() const {
    dimensionAccessRestrictions->applyRecursively([](const DimensionPropagationBlocker::ptr& dimensionPropgationBlocker) {
        dimensionPropgationBlocker->liftRestrictionForWholeDimension();
    });
}


std::optional<unsigned> SignalValueLookup::tryFetchValueFor(const std::vector<std::optional<unsigned>>& accessedDimensions, const std::optional<BitRangeAccessRestriction::BitRangeAccess>& bitRange) const {
    const auto& transformedAccessedDimensions = transformAccessOnDimensions(accessedDimensions);
    if (!transformedAccessedDimensions.has_value()) {
         return std::nullopt;
    }

    bool                                                                canFetchValue             = true;
    LayerData<DimensionPropagationBlocker::ptr>::ptr                    lastAccessedDimensionData = dimensionAccessRestrictions;
    LayerData<std::map<unsigned int, std::optional<unsigned int>>>::ptr valueLookupLayer          = valueLookup;
    

    for (std::size_t dimension = 1; dimension < accessedDimensions.size() && canFetchValue; ++dimension) {
        const auto accessedValueOfDimension  = (*transformedAccessedDimensions).at(dimension - 1);
        canFetchValue             = lastAccessedDimensionData->layerData->isSubstitutionBlockedFor(std::make_optional(accessedValueOfDimension), bitRange);
        lastAccessedDimensionData = lastAccessedDimensionData->nextLayerLinks.at(accessedValueOfDimension);
        valueLookupLayer          = valueLookupLayer->nextLayerLinks.at(accessedValueOfDimension);
    }

    const unsigned int accessedValueOfDimension = accessedDimensions.size() == 1 ? (*transformedAccessedDimensions).front() : (*transformedAccessedDimensions).at(accessedDimensions.size() - 2);
    canFetchValue &= lastAccessedDimensionData->layerData->isSubstitutionBlockedFor(std::make_optional(accessedValueOfDimension), bitRange);
    if (!canFetchValue) {
        return std::nullopt;
    }

    const std::optional<unsigned int> storedValue = valueLookupLayer->layerData.count(accessedValueOfDimension) == 0 ? std::nullopt : valueLookupLayer->layerData[accessedValueOfDimension];
    if (storedValue.has_value() && bitRange.has_value()) {
        return std::make_optional(extractPartsOfValue(*storedValue, *bitRange));
    }
    return storedValue;
}

// TODO: Should we check that the new value is actually storable in the given bit range (and what should happen to the stored value if this is not the case, should we truncate the new value, clear the existing one, etc.)
void SignalValueLookup::updateStoredValueFor(const std::vector<std::optional<unsigned>>& accessedDimensions, const std::optional<BitRangeAccessRestriction::BitRangeAccess>& bitRange, const unsigned int newValue) const {
    const auto& transformedAccessedDimensions = !isValueLookupBlockedFor(accessedDimensions, bitRange) ? transformAccessOnDimensions(accessedDimensions) : std::vector<unsigned int>(0);
    // TODO: What should happen in case that one tries to update more than one value of a dimension ?
    if (!transformedAccessedDimensions.has_value()) {
        return;
    }

    // Since we are trying to update an entry in a lookup the defined access on the dimension must be fully specified (i.e. for every dimension of the signal) and must not access all values of a dimension for any dimension
    LayerData<std::map<unsigned int, std::optional<unsigned int>>>::ptr valueLookupLayer = valueLookup;

    for (std::size_t dimension = 1; dimension < signalInformation.valuesPerDimension.size() - 1; ++dimension) {
        valueLookupLayer = valueLookupLayer->nextLayerLinks[(*transformedAccessedDimensions).at(dimension - 1)];
    }

    const auto accessedValueOfLastDimension = (*transformedAccessedDimensions).back();
    if (valueLookupLayer->layerData.count(accessedValueOfLastDimension) == 0) {
        valueLookupLayer->layerData.insert(std::pair(accessedValueOfLastDimension, newValue));
    } else {
        auto& currentValue = valueLookupLayer->layerData[accessedValueOfLastDimension];
        // The bit range to be update for the signal must be used to update the corresponding bit range in the stored value 
        if (currentValue.has_value() && bitRange.has_value()) {
            currentValue.emplace(transformExistingValueByMergingWithNewOne(*currentValue, newValue, *bitRange));
        } else {
            currentValue.emplace(newValue);
        }
    }
}

bool SignalValueLookup::isValueStorableInBitrange(const BitRangeAccessRestriction::BitRangeAccess& availableStorageSpace, const unsigned int value) {
    const auto resultContainerSize = (availableStorageSpace.second - availableStorageSpace.first) + 1;
    const auto maxStorableValue    = UINT_MAX >> (32 - resultContainerSize);
    return value <= maxStorableValue;
}


std::optional<std::vector<unsigned int>> SignalValueLookup::transformAccessOnDimensions(const std::vector<std::optional<unsigned int>>& accessedDimensions) const {
    if (accessedDimensions.size() < signalInformation.valuesPerDimension.size()) {
        return std::nullopt;
    }

    const bool accessesAllValueForAnyDimension = std::any_of(
            accessedDimensions.cbegin(),
            accessedDimensions.cend(),
            [](const std::optional<unsigned int>& accessedValueOfDimension) {
                return accessedValueOfDimension.has_value();
            });

    const bool isAccessOnAllValuesForAnyDimensionOk = std::all_of(
            signalInformation.valuesPerDimension.cbegin(),
            signalInformation.valuesPerDimension.cend(),
            [](const unsigned int numValuesOfDimension) {
                return numValuesOfDimension == 1;
            });

    if (accessesAllValueForAnyDimension && !isAccessOnAllValuesForAnyDimensionOk) {
        return std::nullopt;
    }

    std::vector<unsigned int> transformedAccessOnDimensions(accessedDimensions.size());
    std::transform(
            accessedDimensions.cbegin(),
            accessedDimensions.cend(),
            transformedAccessOnDimensions.begin(),
            [](const std::optional<unsigned int> accessedValueOfDimension) {
                return accessedValueOfDimension.has_value() ? *accessedValueOfDimension : 0;
            });
    return transformedAccessOnDimensions;
}

inline bool SignalValueLookup::isValueLookupBlockedFor(const std::vector<std::optional<unsigned int>>& accessedDimensions, const std::optional<BitRangeAccessRestriction::BitRangeAccess>& bitRange) const {
    return dimensionAccessRestrictions->doesPredicateHoldInDimensionThenCheckRecursivelyOtherwiseStop(0, accessedDimensions, [bitRange](const unsigned int _, const std::optional<unsigned int> accessedValueOfDimension, const DimensionPropagationBlocker::ptr& dimensionPropagationBlocker) {
        return dimensionPropagationBlocker->isSubstitutionBlockedFor(accessedValueOfDimension, bitRange);
    });
}

/*
 * TODO: We are assuming that the maximum signal length is equal to 32 bits ?
 */
unsigned int SignalValueLookup::extractPartsOfValue(const unsigned int value, const BitRangeAccessRestriction::BitRangeAccess& partToFetch) {
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

unsigned int SignalValueLookup::transformExistingValueByMergingWithNewOne(const unsigned int currentValue, const unsigned int newValue, const BitRangeAccessRestriction::BitRangeAccess& partsToUpdate) {
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
    const auto newValueShifted                              = newValue << partsToUpdate.first;
    return currentValueWithAccessBitrangeZeroed | newValueShifted;
}