#include "core/syrec/parser/optimizations/constantPropagation/signal_value_lookup.hpp"

#include <algorithm>

using namespace optimizations;

void SignalValueLookup::invalidateAllStoredValuesForSignal() const {
    valueLookup->applyRecursively([](std::map<unsigned int, std::optional<unsigned int>> layerValueLookup) {
        layerValueLookup.clear();
    });

    dimensionAccessRestrictions->layerData->blockSubstitutionForDimension(std::nullopt);
    dimensionAccessRestrictions->applyRecursively(
    [](const DimensionPropagationBlocker::ptr& dim) {
        dim->liftRestrictionForWholeDimension();
    });

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
    if (dimensionAccessRestrictions->doesPredicateHoldInDimensionThenCheckRecursivelyElseStop(
                0,
                accessedDimensions,
                [](const unsigned int, const std::optional<unsigned int>& accessedValueOfDimension, const DimensionPropagationBlocker::ptr& dimensionPropagationBlocker) {
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

    auto transformedBitRangeAccess = BitRangeAccessRestriction(bitRange);
    dimensionAccessRestrictions->applyOnLastAccessedDimensionIfPredicateDoesHold(
            0,
            accessedDimensions,
            [&transformedBitRangeAccess](const DimensionPropagationBlocker::ptr& dimensionPropagationBlocker, const std::vector<LayerData<DimensionPropagationBlocker::ptr>::ptr>& nextLayerLinks, const std::optional<unsigned int>& valueOfLastAccessedDimension, const std::optional<BitRangeAccessRestriction::BitRangeAccess>&) {
                for (const auto& restriction : transformedBitRangeAccess.getRestrictions()) {
                    dimensionPropagationBlocker->blockSubstitutionForBitRange(valueOfLastAccessedDimension, restriction);

                    // Since we are already blocking the bit range in the parent dimension and due to our assumption that restrictions in subsequent dimensions are always more specific,
                    // we can lift the bit range restriction from the latter.
                    for (const auto& nextLayerLink: nextLayerLinks) {
                        nextLayerLink->applyRecursively([&restriction, valueOfLastAccessedDimension](const DimensionPropagationBlocker::ptr& nextLayerDimensionPropagationBlocker) {
                            nextLayerDimensionPropagationBlocker->liftRestrictionForBitRange(valueOfLastAccessedDimension, restriction);
                        });
                    }
                }
            },
            [&transformedBitRangeAccess](const DimensionPropagationBlocker::ptr& dimensionPropagationBlocker, const std::optional<unsigned int>& valueOfAccessedDimension, const std::optional<BitRangeAccessRestriction::BitRangeAccess>& accessedBitRange) {
                // Due to our assumption that restrictions in subsequent dimensions are always more specific, we try to trim from to be restricted bit range all already existing restrictions and only apply the remaining part of the former if anything is still left
                if (dimensionPropagationBlocker->tryTrimAlreadyBlockedPartsFromRestriction(valueOfAccessedDimension, accessedBitRange, transformedBitRangeAccess)) {
                    return transformedBitRangeAccess.hasAnyRestrictions();
                }
                return true;
            });
}

void SignalValueLookup::liftRestrictionsOfDimensions(const std::vector<std::optional<unsigned int>>& accessedDimensions, const std::optional<BitRangeAccessRestriction::BitRangeAccess>& bitRange) {
    const BitRangeAccessRestriction::BitRangeAccess& transformedBitRangeAccess = bitRange.has_value() ? *bitRange : BitRangeAccessRestriction::BitRangeAccess(0, signalInformation.bitWidth - 1);
    dimensionAccessRestrictions->applyRecursivelyStartingFrom(
            0,
            accessedDimensions.front(),
            accessedDimensions,
            [transformedBitRangeAccess](const DimensionPropagationBlocker::ptr& blockerForDimension, const std::optional<unsigned int>& accessedValueOfDimension) {
                blockerForDimension->liftRestrictionForBitRange(accessedValueOfDimension, transformedBitRangeAccess);
            }
    );
    // We have to distinguish the following cases
    // I.   In case the whole dimension was blocked, we need to block the other values for the dimension other than the accessed one
    // II.  In case there existing a dimension wide bit range restriction, we need to lift the restriction for the given bitrange from the dimension ana recreated it for the other values of the dimension as in I.
    // III. Otherwise simple lift the restrictions
	
	/* When we lift the restriction for a bit range we might need to lift a restriction in a parent dimension (i.e. bit range a[$i].1:3 is locked in parent while a[0][2].4:5 is locked in child)
	 * if we would lift the restriction for a[0][2].1:5 we need to lift the one in the parent (where the dimension propagation should already handle the recreation for the remaining valus of the first dimension)
	 * and subsequently in the child. They same must be done when lifting a[$i][2].1:5 (but again for the parent dimension, the dimension propagation blocker should already do the majority of the lifting)
	*
	*/


    /*
     *  BitRangeAccessRestriction::BitRange transformedBitRange = bitRange.has_value()
     *      ? *bitRange
     *      : BitRangeAccessRestriction::BitRangeAccess();
     *
     *  BitRangeAccessRestriction::BitRange remainingRestrictionToApply = transformedBitRange;
     *
     *  const bool isWholeBitrangeAccessed = transformedBitRange.second - transformedBitRange.first == signalInformation->signalDimension.bitwidth;
     *
     *  const auto dimension;
     *  const std::optional<unsigned int> accessedValueOfDimension = accessedDimensions.at(dimension);
     *  const auto numValuesOfDimension = signalInformation->signalDimension.at(dimension);
     *
     * Apply the following steps as long as the remainingRestrictionToApply has anything left
     *
     * I. The whole dimension is blocked (or the dimension wide bit range restriction is as long as the signal bit width)
     *  We can lift the restriction in case the whole dimension is blocked when the whole signal width is accessed. If a specific value for the dimension was specified, we must recreate the restriction for the whole signal width
     *  for the remaining values of the dimension but can clear all restrictions from subsequent dimensions regardless of the value for the dimension.
     *
     *  In case the whole dimension is blocked and we are lifting the restriction for a specific bit range, we can need to recreate a dimension wide bit range restriction for the bit range minus the one we are trying to lift,
     *  if we are additionally lifting it only for a specific value of the dimension, we need to recreate the bitrange restriction for the lifted range for all other values of the dimension.
     *
     *  TODO: Only required later on
     *  Since restrictions in subsequent dimensions
     *  can only be more specific, we can lift the restrictions of the remaining dimension-wide restriction in all subsequent dimension and only if a specific value for the dimension was defined also do the same for the lifted bitrange.
     *
     *  if (isWholeBitrangeAccessed) {
     *      dimensionPropagationBlocker->liftRestrictionForWholeDimension();
     *      if (accessedValueOfDimension.has_value()) {
     *          for (std::size_t i = 0; i < numValuesOfDimension; ++i) {
     *              if (i != *accessedValueOfDimension)
     *                  dimensionPropagationBlocker->blockSubstitutionForDimension(i);
     *          }
     *      }
     *  }
     *  else {
     *      dimensionPropagationBlocker->liftRestrictionForBitRange(accessedValueOfDimension, transformedBitRange);
     *      if (accessedValueOfDimension.has_value()) {
     *          for (std::size_t i = 0; i < numValuesOfDimension; ++i) {
     *              if (i != *accessedValueOfDimension)
     *                  dimensionPropagationBlocker->blockSubstitutionForBitRange(i, transformedBitRange);
     *          }
     *      }
     *  }
     *  remainingRestrictionToApply->liftAllRestrictions();
     *
     * II. There exists a dimension wide bit range restriction that is smaller than the signal bit width (TODO: Fuse together signal restrictions to whole for bit range access restriction)
     *
     * In case there is no overlap with the dimension wide bit range restriction, we can skip further checks for this dimension.
     * Otherwise, lift the restriction for the given bit range dimension wide and in case that this should only be done for specific value of the dimension, recreate it for the remaining values of the dimension.
     * TODO: This does not apply ?: Since restrictions in subsequent dimension are always more specific, we can only stop our checks if the lifted bit range already update the whole dimension wide one.
     *
     * dimensionPropagationBlocker->liftRestrictionForBitRange(accessedValueOfDimension, transformedBitRange);
     *
     * III. There exists a bit range restriction for a value of the dimension
     *
     * dimensionPropagationBlocker->liftRestrictionForBitRange(accessedValueOfDimension, transformedBitRange);
     *
     */


    /*
     * PSEUDO CODE IMPLEMENTATION
     *
     * var numActualAccessedDimensions : number = accessedDimensions.size()
     * var numActualSignalDimensions : number = signal.dimensions.size()
     *
     * var transformedDimensionAccess : std::vector<unsigned int>(signal.dimensions.size(), std::nullopt)
     * for (std::size_t i = 0; i < numActualAccessedDimensions; ++i) {
     *  transformedDimensionAccess[i] = accessedDimensions.at(i)
     * }
     *
     *  dimensionPropagationBlocker->applyRecursivelyOnAccessedDimensions(
     *      0,
     *      transformedDimensionAccess,
     *      [](const unsigned int dimension, const std::vector<std::optional<unsigned int>> accessedValuePerDimension, const DimensionPropagationBlocker::ptr &dimensionPropagationBlocker){
     *          const auto& accessedValueOfDimension = accessedValuePerDimension.at(dimension);
     *
     *          const BitRangeAccessRestriction::BitRangeAccess bitRangeToUnblock = bitRange.has_value()
     *                  ? *bitRange
     *                  : BitRangeAccessRestriction::BitRangeAccess();
     *          // Is dimension completely blocked
     *          if (dimensionPropagationBlocker->isSubstitutionBlockedFor(std::nullopt, std::nullopt, ignoreNotFullwidthBitrangeRestrictions: true, ignoreSmallerThanAccessedBitranges: true)) {
     *              dimensionPropagationBlocker->liftRestrictionForBitRange(accessedValueOfDimension, bitRangeToUnblock);
     *          }
     *          // Exists dimension wide signal range restriction
     *          else if (const auto& dimensionWideBitrangeRestriction = dimensionPropagationBlocker->tryGetDimensionWideBitRangeAccessRestriction(); dimensionWideBitrangeRestriction.has_value()) {
     *              const bool shouldDimensionWideRestrictionBeUpdated = bitRange.has_value()
     *                  ? dimensionWideBitrangeRestriction->isAccessRestrictedTo(*bitRange)
     *                  : dimensionWideBitrangeRestriction->hasAnyRestrictions();
     *
     *              if (shouldDimensionWideRestrictionBeUpdate) {
     *                  dimensionPropagationBlocker->liftRestrictionForBitRange(accessedValueOfDimension, bitRangeToUnblock);
     *                  // If we are lifting a dimension wide bit range restriction for just one value of the dimension we need to recreate it for the remaining ones
     *                  if (accessedValueOfDimension.has_value()) {
     *                      for (std::size_t i = 0; i < numDefinedValuesOfDimension; ++i) {
     *                          if (i == *accessedValueOfDimension) {
     *                              dimensionPropagationBlocker->applyRecursivelyStartingFrom(
     *                                  currDimension,
     *                                  i,
     *                                  accessedDimensions,
     *                                  [transformedBitRange](const DimensionPropagationBlocker::ptr& propagationBlockerOfDimension) {
     *                                      propagationBlockerOfDimension->liftRestrictionForBitRange(std::nullopt, transformedBitRange);
     *                              });
     *                          }
     *
     *                          dimensionPropagationBlocker->applyRecursivelyStartingFrom(
     *                              currDimension,
     *                              i,
     *                              accessedDimensions,
     *                              [transformedBitRange](const DimensionPropagationBlocker::ptr& propagationBlockerOfDimension) {
     *                                  propagationBlockerOfDimension->blockSubstitutionForBitRange(std::nullopt, transformedBitRange);
     *                              });
     *                      }
     *                  }
     *              }
     *
     *              if (shouldDimensionWideRestrictionBeUpdated) {
     *                  if (bitRange.has_value()) {
     *                      dimensionWideBitRangeRestriction->liftRestrictionFor(*bitRange);
     *
     *                      std::vector<BitRangeAcessRestriction::BitRangeAccess> remainingRestrictionsAfterUnblock = dimensionBitRangeRestriction->getRestrictions();
     *                      remainingRestrictionsAfterUnblock.emplace_back(*bitRange);
     *
     *                      const auto numDefinedValuesOfDimension = signalInformation->signalDimension.at(dimension);
     *
     *                      if (accessedValueOfDimension.has_value()) {
     *                          for (std::size_t i = 0; i < numDefinedValuesOfDimension; ++i) {
     *                              if (i == *accessedValueOfDimension) {
     *                                  dimensionPropagationBlocker->liftRestrictionForBitRange(accessedValueOfDimension, 
     *                              }
     *                          }
     *
     *                          dimensionPropagationBlocker->applyRecursivelyStartingFrom(
     *                              currDimension,
     *
     *                          );
     *                      }
     *
     *                      for (std::size_t i = 0; i < numDefinedValuesOfDimension; ++i) {
     *                          
     *                      }
     *
     *                      dimensionPropagationBlocker->applyRecursivelyStartingFrom(currDimension, 0, accessedDimensions, []()
     *
     *                      if (dimensionWideBitRangeRestriction->hasAnyRestrictions()) {
     *                          
     *                      }
     *                  }
     *                  else {
     *                      dimensionWideBitRangeRestriction->liftAllRestrictions();
     *                  }
     *              }
     *
     *
     *              
     *
     *              if (dimensionWideBitrangeRestriction.isAccessRestrictedTo(
     *          }
     *          else {
     *              if (accessedValueOfDimension.has_value()) {
     *                  // No dimension wide signal range restriction was defined but there exists one for the accessed value of the dimension
     *                  if (const auto& bitrangeRestrictionForAccessedValueOfDimension = dimensionPropagationBlocker->tryGetValueOfDimenionBitRangeAccessRestriction(*accessedValueOfDimension); bitrangeRestrictionForAccessedValueOfDimension.has_value()) {
     *
     *                  }
     *              }
     *              else {
     *                  // No dimension wide signal range restriction was defined but due to the access on the whole dimension we need to check for restrictions on any value of the dimension
     *                  const auto numDefinedValuesOfDimension = signalInformation->signalDimension.at(dimension);
     *                  for (std::size_t i = 0; i < numDefinedValuesOfDimension; ++i) {
     *                      if (const auto& bitrangeRestrictionForAccessedValueOfDimension = dimensionPropagationBlocker->tryGetValueOfDimenionBitRangeAccessRestriction(*accessedValueOfDimension); bitrangeRestrictionForAccessedValueOfDimension.has_value()) {
     *
     *                      }
     *                  }
     *              }
     *          }
     *      }
     *  );
     *
     *
     * for (std::size_t i = 0; i < numActualSignalDimensions; ++i) {
     *  const auto& accessedValueForDimension = transformedDimensionAccess.at(i);
     *
     * }
     *
     * const auto numActualAccessedDimensions = accessedDimensions.size();
     * //for (Dimension d, idx i : actualDimensions.Take(numActualAccessedDimensions):
     * for (Dimension d, idx i : actualDimensions):
     *  const auto& actualValueOfDimension = accessedDimensions.at(i);
     *
     *  if (i >= numActualAccessedDimensions):
     *      if (!bitRange.has_value() || len(bitRange) == whole signal):
     *          unblockCompletely(d)
     *          continue
     *      else:
     *          break;
     *
     *  if (d is blocked completely):
     *      unblockCompletely(d)
     *
     *      for (Restriction perValueRestriction, idx j : valueRestrictions(d)):
     *          blockCompletely(d, j)
     *
     *          if (actualValueOfDimension.has_value() && j == *actualValueOfDimension):
     *              if (bitRange.has_value()):
     *                  unblock(d, j, bitRange)
     *              else:
     *                  unblock(d, j)
     *
     *  else if (isBlocked(d, bitRange, ignoreSmallerBitRanges: false)):
     *      unblock(d, bitRange)
     *  
     *      for (Restriction perValueRestriction, idx j : valueRestriction(d)):
     *          if (actualValueDimension.has_value() && j == *actualValueDimension):
     *              applyRecursivlyFrom(accessedDimensions, i + 1, j, unblock(bitRange))
     *              continue
     *
     *          block(d, j, bitRange)
     *          applyRecursively(accessedDimensions, i + 1, j, unblock(bitRange))
     *
     *  else if (actualValueOfDimension.has_value() && isBlocked(d, *actualValueOfDimension, bitRange)):
     *     const auto restrictionOfValueOfDimension = restriction(d, *actualValueOfDimension)
     *     if (restrictionOfValueOfDimension->isBlockedCompletely()):
     *       restrictionOfValueOfDimension->reset()
     *     else:
     *      restrictionOfValueOfDimension->unblock(*actualValueOfDimension, bitRange)
     *          
     *  if (numActualAccessedDimensions < numActualDimensions):
     *      if (!bitRange.has_value() || len(bitRange) == whole signal):
     *          
     *
     *
     *
     *
     *
     *
     * for (Dimension d, idx i : actualDimensions):
     *  if (i > numActualAccessedDimensions):
     *
     *
     * for (Dimension d, idx i : accessedDimensions):
     *  if (actualDimension.at(i)->isBlockedCompletely):
     *      actualDimension.at(i)->unblockCompletely()
     *
     *      if (accessedDimension.at(i).has_value()):
     *          for (ValueOfDimension v : actualDimension.at(i).where(x => x != accessedDimensions.at(i))):
     *              actualDimension.at(i)->blockCompletely(v, bitRange)
     *              // Apply recursively
     *              actualDimension.at(i + 1)->unblock(v, bitRange)
     *      else:
     *          // Block remaining bit range
     *          actualDimension.at(i)->block((0, bitRange.Start - 1), (bitRange.End + 1, signalEnd))
     *
     *          // Apply recursively
     *          // Unblock now blocked bit range from parent in subsequent dimensions
     *          actualDimension.at(i + 1)->liftRestrictionFor((0, bitRange.Start - 1), (bitRange.End + 1, signalEnd))
     *
     *
     *  if (actualDimension.at(i)->isBlocked(bitRange):
     *      // Lift the bit range restriction for the specific value of the dimension
     *      if (d.has_value()):
     *          actualDimension.at(i)->unblock(d.value(), bitRange)
     *          // But recreate it for the other values of the dimension since the previous restrictions was a dimension wide one
     *          for (ValueOfDimension v: actualDimension.at(i).where(x => x != accessedDimensions.at(i))):
     *              actualDimension.at(i)->block(v, bitRange)
     *      else:
     *          // Simply lift the bit range restriction for the whole dimension
     *          actualDimension.at(i)->unblock(bitRange)
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
    return dimensionAccessRestrictions->doesPredicateHoldInDimensionThenCheckRecursivelyElseStop(0, accessedDimensions, [bitRange](const unsigned int _, const std::optional<unsigned int> accessedValueOfDimension, const DimensionPropagationBlocker::ptr& dimensionPropagationBlocker) {
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