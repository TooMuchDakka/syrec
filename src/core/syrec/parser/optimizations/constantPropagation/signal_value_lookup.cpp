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
            [](std::map<unsigned int, std::optional<unsigned int>> layerValueLookup, const std::vector<LayerData<std::map<unsigned int, std::optional<unsigned int>>>::ptr>& nextLayerLinks, const std::optional<unsigned int>& valueOfLastAccessedDimension, const std::optional<BitRangeAccessRestriction::BitRangeAccess>& accessedBitRange) {
                if (valueOfLastAccessedDimension.has_value()) {
                    if (layerValueLookup.count(*valueOfLastAccessedDimension) > 0) {
                        layerValueLookup.erase(*valueOfLastAccessedDimension);
                    }
                }
                else {
                    layerValueLookup.clear();
                }
            });
}

void SignalValueLookup::invalidateStoredValueForBitrange(const std::vector<std::optional<unsigned>>& accessedDimensions, const BitRangeAccessRestriction::BitRangeAccess& bitRange) const {
    if (accessedDimensions.empty()) {
        if (bitRange.second - bitRange.first == signalInformation.bitWidth) {
            invalidateAllStoredValuesForSignal();
            return;
        }
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

    unsigned int accessedValueOfDimension = accessedDimensions.size() == 1 ? (*transformedAccessedDimensions).front() : (*transformedAccessedDimensions).at(accessedDimensions.size() - 2);
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

void SignalValueLookup::updateStoredValueFor(const std::vector<std::optional<unsigned>>& accessedDimensions, const std::optional<BitRangeAccessRestriction::BitRangeAccess>& bitRange, const unsigned int newValue) const {
    const auto& transformedAccessedDimensions = !isValueLookupBlockedFor(accessedDimensions, bitRange) ? transformAccessOnDimensions(accessedDimensions) : std::vector<unsigned int>(0);
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

std::optional<std::vector<unsigned>> SignalValueLookup::transformAccessOnDimensions(const std::vector<std::optional<unsigned>>& accessedDimensions) const {
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
            transformedAccessOnDimensions,
            [](const std::optional<unsigned int> accessedValueOfDimension) {
                return accessedValueOfDimension.has_value() ? *accessedValueOfDimension : 0;
            });
    return transformedAccessOnDimensions;
}

inline bool SignalValueLookup::isValueLookupBlockedFor(const std::vector<std::optional<unsigned int>>& accessedDimensions, const std::optional<BitRangeAccessRestriction::BitRangeAccess>& bitRange) const {
    return dimensionAccessRestrictions->doesPredicateHoldThenCheckRecursivelyOtherwiseStop([bitRange](const DimensionPropagationBlocker::ptr& dimensionPropagationBlocker, const std::optional<unsigned int>& accessedValueOfDimension) {
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



//
//void SignalValueLookup::invalidateAllStoredValuesForSignal() {
//    if (dimensionAccessRestrictions.empty()) {
//        return;
//    }
//
//    dimensionAccessRestrictions.at(0).at(0)->blockSubstitutionForDimension(std::nullopt);
//    for (std::size_t dimension = 1; dimension < dimensionAccessRestrictions.size(); ++dimension) {
//        for (const auto& restriction : dimensionAccessRestrictions.at(dimension)) {
//            restriction->liftRestrictionForWholeDimension();
//        }
//    }
//
//    if (valueLookup.has_value()) {
//        (*valueLookup).invalidateLayer();
//    }
//}
//
//void SignalValueLookup::invalidateStoredValueFor(const std::vector<std::optional<unsigned>>& accessedDimensions) const {
//    if (dimensionAccessRestrictions.empty()) {
//        return;
//    }
//
//    /*
//     * In case that we want to invalidate stored values based one some incomplete signal access (i.e. accessing a[2][1] of the signal a[2][4][3][5])
//     * we have to distinguish between two cases,
//     */
//    if (accessedDimensions.size() < dimensionAccessRestrictions.size()) {
//        const auto firstWholeDimensionAccess = std::find(
//                accessedDimensions.cbegin(),
//                accessedDimensions.cend(),
//                [](const std::optional<unsigned int>& accessedDimension) {
//                    return accessedDimension.has_value();
//                });
//
//        /*
//         * If the defined signal access accesses a whole dimension, we will invalidate all entries for this dimension and all subsequent ones
//         */
//        if (firstWholeDimensionAccess != accessedDimensions.end()) {
//            if (firstWholeDimensionAccess == accessedDimensions.begin()) {
//                dimensionAccessRestrictions
//                    .front()
//                    .front()->blockSubstitutionForDimension(std::nullopt);
//                return;
//            }
//
//            const auto firstWholeDimensionAccessIndex = std::distance(accessedDimensions.begin(), firstWholeDimensionAccess);
//            for (std::size_t dimension = 1; dimension <= firstWholeDimensionAccessIndex; ++dimension) {
//                const auto& prevAccessedDimensionValue = *accessedDimensions.at(dimension - 1);
//                dimensionAccessRestrictions
//                    .at(dimension)
//                    .at(prevAccessedDimensionValue)->blockSubstitutionForDimension(accessedDimensions.at(dimension));
//            }
//
//            for (std::size_t remainingDimension = firstWholeDimensionAccessIndex + 1; remainingDimension < dimensionAccessRestrictions.size(); ++remainingDimension) {
//                for (const auto& dimensionRestriction: dimensionAccessRestrictions.at(remainingDimension)) {
//                    dimensionRestriction->liftRestrictionForWholeDimension();
//                }
//            }
//        } else {
//            /*
//             * In case that no whole dimension access is defined, we can restrict access specified values of the defined dimensions
//             * and remove all restrictions from the subsequent dimensions
//             */
//
//            const auto lastAccessedDimensionIdx = accessedDimensions.size() - 1;
//            for (std::size_t dimension = 0; dimension < dimensionAccessRestrictions.size(); ++dimension) {
//                if (dimension < lastAccessedDimensionIdx) {
//                    const auto effectedDimensionRestrictionIdx = *accessedDimensions.at(dimension - 1);
//                    dimensionAccessRestrictions
//                            .at(dimension)
//                            .at(effectedDimensionRestrictionIdx)
//                            ->blockSubstitutionForDimension(accessedDimensions.at(dimension));   
//                }
//                else {
//                    for (const auto& dimensionRestriction: dimensionAccessRestrictions.at(dimension)) {
//                        dimensionRestriction->liftRestrictionForWholeDimension();
//                    }
//                }
//            }
//        }
//    }
//    else {
//        /*
//         *
//         */
//        const auto  indexOfLastAccessedDimension           = accessedDimensions.size() - 1;
//        const auto& valueOfLastAccessedDimension  = accessedDimensions.at(indexOfLastAccessedDimension);
//        auto&       effectedDimensionRestrictions          = dimensionAccessRestrictions.at(indexOfLastAccessedDimension);
//        const std::optional<unsigned int> selectorForEffectedDimensionRestrictionsOfLastAccessedDimension = indexOfLastAccessedDimension > 1 ? accessedDimensions.at(indexOfLastAccessedDimension - 1) : std::nullopt;
//
//        if (selectorForEffectedDimensionRestrictionsOfLastAccessedDimension.has_value()) {
//            effectedDimensionRestrictions.at(*selectorForEffectedDimensionRestrictionsOfLastAccessedDimension)->blockSubstitutionForDimension(valueOfLastAccessedDimension);    
//        }
//        else {
//            for (auto& effectedDimensionRestriction: effectedDimensionRestrictions) {
//                effectedDimensionRestriction->blockSubstitutionForDimension(valueOfLastAccessedDimension);
//            }    
//        }
//
//        const auto& firstAccessedDimension = accessedDimensions.front();
//        dimensionAccessRestrictions
//                .front()
//                .front()
//                ->blockSubstitutionForDimension(firstAccessedDimension);
//
//        /*
//         * For every dimension after the first one we can identify which dimension restriction shall be updated by the value of the previously accessed dimension
//         * If the previous one access the whole dimension, we need to update the all restrictions for the current dimension, otherwise just the accessed one
//         *
//         * Example:
//         * Assume the declared signal: a[2][4][3][2], the dimension restrictions have the following structure: (DIM #, # DIM_RESTRICTIONS_n - # LOOKUP_VALUES): 0: 1 - 2, 1: 2 - 4, 2: 4 - 3, 3: 3 - 2
//         * We are now invalidating the stored values for the signal access: a[$i][3][$j][2].
//         *
//         * I.   We invalidate all entries of dimension 0
//         * II.  Since the first dimension access the whole dimension, we need to update all restrictions of the second dimension (and here the 3rd value of the dimension).
//         * III. We will update the 3rd dimension restriction by invalidating all values for all values of the dimension
//         * IV.  Since the 3rd accessed dimension was accessed as a whole, we need to update all dimension restrictions of the last dimension by invalidating the values for the 2nd value of the dimension
//         */
//        for (std::size_t dimension = 1; dimension < dimensionAccessRestrictions.size(); ++dimension) {
//            const auto  valueForPrevDimension         = accessedDimensions.at(dimension - 1);
//            const auto& accessedValueForDimension     = accessedDimensions.at(dimension);
//            auto& effectedDimensionRestrictions = dimensionAccessRestrictions.at(dimension);
//
//            if (valueForPrevDimension.has_value()) {
//                effectedDimensionRestrictions.at(*valueForPrevDimension)->blockSubstitutionForDimension(accessedValueForDimension);
//            }
//            else {
//                for (const auto& effectedDimensionRestriction: effectedDimensionRestrictions) {
//                    effectedDimensionRestriction->blockSubstitutionForDimension(accessedValueForDimension);
//                }                
//            }
//        }
//    }
//}
//
//void SignalValueLookup::invalidateStoredValueForBitrange(const std::vector<std::optional<unsigned>>& accessedDimensions, const BitRangeAccessRestriction::BitRangeAccess& bitRange) const {
//    /*if (dimensionAccessRestrictions.empty()) {
//        return;
//    }
//
//    const auto& bitRangeLength = bitRange.second - bitRange.first;
//    if (bitRangeLength == signalInformation.bitWidth) {
//        invalidateStoredValueFor(accessedDimensions);
//        return;
//    }
//
//    if (accessedDimensions.size() < dimensionAccessRestrictions.size()) {
//        for (std::size_t dimension = 0; dimension < accessedDimensions.size(); ++dimension) {
//            const auto accessedValueOfDimension = accessedDimensions.at(dimension);
//            const auto accessedValueOfPrevDimension = dimension > 1 ? accessedDimensions.at(dimension) : std::nullopt;
//
//
//        }
//
//
//        const auto& firstAccessedWholeDimension = std::find(
//                accessedDimensions.cbegin(),
//                accessedDimensions.cend(),
//                [](const std::optional<unsigned int>& accessedValueOfDimension) { return accessedValueOfDimension.has_value(); }
//        );
//
//        if (firstAccessedWholeDimension == accessedDimensions.begin()) {
//            for (auto& valueOfDimensionRestriction: dimensionAccessRestrictions.front()) {
//                valueOfDimensionRestriction->blockSubstitutionForBitRange(std::nullopt, bitRange);
//            }
//        }
//
//        const auto& firstAccessedWholeDimensionIdx = std::distance(accessedDimensions.cbegin(), firstAccessedWholeDimension);
//        for ()
//    }
//    else {
//        
//    }*/
//}