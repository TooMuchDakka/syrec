#include "core/syrec/parser/optimizations/constantPropagation/valueLookup/base_value_lookup.hpp"

using namespace valueLookup;

template<typename Vt>
BaseValueLookup<Vt>::~BaseValueLookup() {
    return;   
}

template<typename Vt>
std::optional<LayerData<optimizations::DimensionPropagationBlocker::ptr>::ptr> BaseValueLookup<Vt>::initializeDimensionAccessRestrictionLayer(const unsigned dimension) {
    if (dimension >= signalInformation.valuesPerDimension.size()) {
        return std::nullopt;
    }
    
    std::vector<LayerData<optimizations::DimensionPropagationBlocker::ptr>::ptr> nextLayerLinks;
    if (dimension < signalInformation.valuesPerDimension.size() - 1) {
        const unsigned int numValuesOfDimension = signalInformation.valuesPerDimension.at(dimension);
        nextLayerLinks.resize(numValuesOfDimension);

        for (unsigned int valueOfDimension = 0; valueOfDimension < numValuesOfDimension; ++valueOfDimension) {
            nextLayerLinks[valueOfDimension] = *initializeDimensionAccessRestrictionLayer(dimension + 1);
        }
    }

    return std::make_optional(std::make_unique<LayerData<optimizations::DimensionPropagationBlocker::ptr>>(
        std::make_shared<optimizations::DimensionPropagationBlocker>(dimension, signalInformation), nextLayerLinks    
    ));
}

template<typename Vt>
std::optional<std::shared_ptr<LayerData<std::map<unsigned, std::optional<Vt>>>>> BaseValueLookup<Vt>::initializeValueLookupLayer(const unsigned dimension, const std::optional<Vt>& defaultValue) {
    if (dimension >= signalInformation.valuesPerDimension.size()) {
        return std::nullopt;
    }

    std::vector<std::shared_ptr<LayerData<std::map<unsigned, std::optional<Vt>>>>> nextLayerLinks;
    std::map<unsigned, std::optional<Vt>>                         lookupLayerData;

    const unsigned int numValuesOfDimension = signalInformation.valuesPerDimension.at(dimension);
    if (dimension < signalInformation.valuesPerDimension.size() - 1) {
        nextLayerLinks.resize(numValuesOfDimension);

        for (unsigned int valueOfDimension = 0; valueOfDimension < numValuesOfDimension; ++valueOfDimension) {
            nextLayerLinks[valueOfDimension] = *initializeValueLookupLayer(dimension + 1, defaultValue);
        }
    } else {
        for (unsigned int valueOfDimension = 0; valueOfDimension < numValuesOfDimension; ++valueOfDimension) {
            lookupLayerData.insert(std::pair(valueOfDimension, defaultValue));
        }
    }

    return std::make_optional(std::make_unique<LayerData<std::map<unsigned, std::optional<Vt>>>>(
        LayerData<std::map<unsigned, std::optional<Vt>>>(lookupLayerData, nextLayerLinks)
        ));
}

template<typename Vt>
void BaseValueLookup<Vt>::invalidateAllStoredValuesForSignal() const {
    valueLookup->applyRecursively([](std::map<unsigned int, std::optional<unsigned int>> layerValueLookup) {
        layerValueLookup.clear();
    });

    dimensionAccessRestrictions->layerData->blockSubstitutionForDimension(std::nullopt);
    dimensionAccessRestrictions->applyRecursively(
            [](const optimizations::DimensionPropagationBlocker::ptr& dim) {
                dim->liftRestrictionForWholeDimension();
            });

    valueLookup->applyOnLastLayer(signalInformation.valuesPerDimension.size() - 1, [](std::map<unsigned int, std::optional<unsigned int>>& perValueOfDimensionValueLookup) {
        perValueOfDimensionValueLookup.clear();
    });
}

template<typename Vt>
void BaseValueLookup<Vt>::invalidateStoredValueFor(const std::vector<std::optional<unsigned>>& accessedDimensions) const {
    if (accessedDimensions.empty()) {
        invalidateAllStoredValuesForSignal();
        return;
    }

    // Skip extra work if accessed parts of signal are already blocked
    if (!dimensionAccessRestrictions->doesPredicateHoldInDimensionThenCheckRecursivelyElseStop(
                0,
                accessedDimensions,
                [](const unsigned int, const std::optional<unsigned int>& accessedValueOfDimension, const optimizations::DimensionPropagationBlocker::ptr& dimensionPropagationBlocker) {
                    return !dimensionPropagationBlocker->isSubstitutionBlockedFor(accessedValueOfDimension, std::nullopt, true, true);
                })) {
        return;
    }

    dimensionAccessRestrictions->applyOnLastAccessedDimension(
            0,
            accessedDimensions,
            [](const optimizations::DimensionPropagationBlocker::ptr& dimensionPropagationBlocker, const std::vector<LayerData<optimizations::DimensionPropagationBlocker::ptr>::ptr>& nextLayerLinks, const std::optional<unsigned int>& valueOfLastAccessedDimension, const std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess>& accessedBitRange) {
                dimensionPropagationBlocker->blockSubstitutionForDimension(valueOfLastAccessedDimension);
                for (const auto& nextLayerLink: nextLayerLinks) {
                    nextLayerLink->applyRecursively([](const optimizations::DimensionPropagationBlocker::ptr& nextLayerDimensionPropagationBlocker) {
                        nextLayerDimensionPropagationBlocker->liftRestrictionForWholeDimension();
                    });
                }
            });

    valueLookup->applyOnLastAccessedDimension(
            0,
            accessedDimensions,
            [](std::map<unsigned int, std::optional<unsigned int>>& layerValueLookup, const std::vector<LayerData<std::map<unsigned int, std::optional<unsigned int>>>::ptr>& nextLayerLinks, const std::optional<unsigned int>& valueOfLastAccessedDimension, const std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess>& accessedBitRange) {
                if (valueOfLastAccessedDimension.has_value()) {
                    if (layerValueLookup.count(*valueOfLastAccessedDimension) > 0) {
                        layerValueLookup.erase(*valueOfLastAccessedDimension);
                    }
                } else {
                    layerValueLookup.clear();
                }
            });
}

template<typename Vt>
void BaseValueLookup<Vt>::invalidateStoredValueForBitrange(const std::vector<std::optional<unsigned>>& accessedDimensions, const optimizations::BitRangeAccessRestriction::BitRangeAccess& bitRange) const {
    if (accessedDimensions.empty()) {
        if ((bitRange.second - bitRange.first) + 1 == signalInformation.bitWidth) {
            invalidateAllStoredValuesForSignal();
            return;
        }

        auto transformedBitRangeAccess = optimizations::BitRangeAccessRestriction(signalInformation.bitWidth, bitRange);
        const bool _ = dimensionAccessRestrictions->layerData->tryTrimAlreadyBlockedPartsFromRestriction(std::nullopt, std::make_optional(bitRange), transformedBitRangeAccess);
        if (transformedBitRangeAccess.hasAnyRestrictions()) {
            for (const auto& remainingRestriction: transformedBitRangeAccess.getRestrictions()) {
                dimensionAccessRestrictions->layerData->blockSubstitutionForBitRange(std::nullopt, remainingRestriction);    
            }

            for (const auto& nextLayerLink: dimensionAccessRestrictions->nextLayerLinks) {
                nextLayerLink->applyRecursively([&transformedBitRangeAccess](const optimizations::DimensionPropagationBlocker::ptr& nextLayerDimensionPropagationBlocker) {
                    for (const auto& remainingRestriction : transformedBitRangeAccess.getRestrictions()) {
                        nextLayerDimensionPropagationBlocker->liftRestrictionForBitRange(std::nullopt, remainingRestriction);    
                    }
                });
            }
        }
        return;
    }

    auto transformedBitRangeAccess = optimizations::BitRangeAccessRestriction(signalInformation.bitWidth, bitRange);
    dimensionAccessRestrictions->applyOnLastAccessedDimensionIfPredicateDoesHold(
            0,
            accessedDimensions,
            [&transformedBitRangeAccess](const optimizations::DimensionPropagationBlocker::ptr& dimensionPropagationBlocker, const std::vector<LayerData<optimizations::DimensionPropagationBlocker::ptr>::ptr>& nextLayerLinks, const std::optional<unsigned int>& valueOfLastAccessedDimension, const std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess>&) {
                for (const auto& restriction: transformedBitRangeAccess.getRestrictions()) {
                    dimensionPropagationBlocker->blockSubstitutionForBitRange(valueOfLastAccessedDimension, restriction);

                    // Since we are already blocking the bit range in the parent dimension and due to our assumption that restrictions in subsequent dimensions are always more specific,
                    // we can lift the bit range restriction from the latter.
                    for (const auto& nextLayerLink: nextLayerLinks) {
                        nextLayerLink->applyRecursively([&restriction, valueOfLastAccessedDimension](const optimizations::DimensionPropagationBlocker::ptr& nextLayerDimensionPropagationBlocker) {
                            nextLayerDimensionPropagationBlocker->liftRestrictionForBitRange(valueOfLastAccessedDimension, restriction);
                        });
                    }
                }
            },
            [&transformedBitRangeAccess](const optimizations::DimensionPropagationBlocker::ptr& dimensionPropagationBlocker, const std::optional<unsigned int>& valueOfAccessedDimension, const std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess>& accessedBitRange) {
                // Due to our assumption that restrictions in subsequent dimensions are always more specific, we try to trim from to be restricted bit range all already existing restrictions and only apply the remaining part of the former if anything is still left
                if (dimensionPropagationBlocker->tryTrimAlreadyBlockedPartsFromRestriction(valueOfAccessedDimension, accessedBitRange, transformedBitRangeAccess)) {
                    return transformedBitRangeAccess.hasAnyRestrictions();
                }
                return true;
            });
}

template<typename Vt>
void BaseValueLookup<Vt>::liftRestrictionsOfDimensions(const std::vector<std::optional<unsigned int>>& accessedDimensions, const std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess>& bitRange) {
    const optimizations::BitRangeAccessRestriction::BitRangeAccess& transformedBitRangeAccess = bitRange.has_value() ? *bitRange : optimizations::BitRangeAccessRestriction::BitRangeAccess(0, signalInformation.bitWidth - 1);
    dimensionAccessRestrictions->applyRecursivelyStartingFrom(
            0,
            accessedDimensions.front(),
            accessedDimensions,
            [transformedBitRangeAccess](const optimizations::DimensionPropagationBlocker::ptr& blockerForDimension, const std::optional<unsigned int>& accessedValueOfDimension) {
                blockerForDimension->liftRestrictionForBitRange(accessedValueOfDimension, transformedBitRangeAccess);
            });
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
     *  optimizations::BitRangeAccessRestriction::BitRange transformedBitRange = bitRange.has_value()
     *      ? *bitRange
     *      : optimizations::BitRangeAccessRestriction::BitRangeAccess();
     *
     *  optimizations::BitRangeAccessRestriction::BitRange remainingRestrictionToApply = transformedBitRange;
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
     *          const optimizations::BitRangeAccessRestriction::BitRangeAccess bitRangeToUnblock = bitRange.has_value()
     *                  ? *bitRange
     *                  : optimizations::BitRangeAccessRestriction::BitRangeAccess();
     *          // Is dimension completely blocked
     *          if (dimensionPropagationBlocker->isSubstitutionBlockedFor(std::nullopt, std::nullopt, ignoreNotFullwidthBitrangeRestrictions: true, ignoreSmallerThanAccessedBitranges: true)) {
     *              dimensionPropagationBlocker->liftRestrictionForBitRange(accessedValueOfDimension, bitRangeToUnblock);
     *          }
     *          // Exists dimension wide signal range restriction
     *          else if (const auto& dimensionWideBitrangeRestriction = dimensionPropagationBlocker->tryGetDimensionWideoptimizations::BitRangeAccessRestriction(); dimensionWideBitrangeRestriction.has_value()) {
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
     *                  if (const auto& bitrangeRestrictionForAccessedValueOfDimension = dimensionPropagationBlocker->tryGetValueOfDimenionoptimizations::BitRangeAccessRestriction(*accessedValueOfDimension); bitrangeRestrictionForAccessedValueOfDimension.has_value()) {
     *
     *                  }
     *              }
     *              else {
     *                  // No dimension wide signal range restriction was defined but due to the access on the whole dimension we need to check for restrictions on any value of the dimension
     *                  const auto numDefinedValuesOfDimension = signalInformation->signalDimension.at(dimension);
     *                  for (std::size_t i = 0; i < numDefinedValuesOfDimension; ++i) {
     *                      if (const auto& bitrangeRestrictionForAccessedValueOfDimension = dimensionPropagationBlocker->tryGetValueOfDimenionoptimizations::BitRangeAccessRestriction(*accessedValueOfDimension); bitrangeRestrictionForAccessedValueOfDimension.has_value()) {
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

template<typename Vt>
void BaseValueLookup<Vt>::liftRestrictionsFromWholeSignal() const {
    dimensionAccessRestrictions->applyRecursively([](const optimizations::DimensionPropagationBlocker::ptr& dimensionPropgationBlocker) {
        dimensionPropgationBlocker->liftRestrictionForWholeDimension();
    });
}

template<typename Vt>
std::optional<Vt> BaseValueLookup<Vt>::tryFetchValueFor(const std::vector<std::optional<unsigned int>>& accessedDimensions, const std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess>& bitRange) const {
    if (isValueLookupBlockedFor(accessedDimensions, bitRange)) {
        return std::nullopt;
    }

    const auto& transformedAccessedDimensions = transformAccessOnDimensions(accessedDimensions);
    if (!transformedAccessedDimensions.has_value()) {
        return std::nullopt;
    }

    bool                                                                canFetchValue             = true;
    LayerData<optimizations::DimensionPropagationBlocker::ptr>::ptr     lastAccessedDimensionData = dimensionAccessRestrictions;
    std::shared_ptr<LayerData<std::map<unsigned int, std::optional<Vt>>>> valueLookupLayer          = valueLookup;

    for (std::size_t dimension = 1; dimension < accessedDimensions.size() && canFetchValue; ++dimension) {
        const auto accessedValueOfDimension = (*transformedAccessedDimensions).at(dimension - 1);
        canFetchValue                       = !lastAccessedDimensionData->layerData->isSubstitutionBlockedFor(std::make_optional(accessedValueOfDimension), bitRange);
        lastAccessedDimensionData           = lastAccessedDimensionData->nextLayerLinks.at(accessedValueOfDimension);
        valueLookupLayer                    = valueLookupLayer->nextLayerLinks.at(accessedValueOfDimension);
    }

    const unsigned int accessedValueOfDimension = accessedDimensions.size() == 1 ? (*transformedAccessedDimensions).front() : (*transformedAccessedDimensions).at(accessedDimensions.size() - 2);
    canFetchValue &= !lastAccessedDimensionData->layerData->isSubstitutionBlockedFor(std::make_optional(accessedValueOfDimension), bitRange);
    if (!canFetchValue) {
        return std::nullopt;
    }

    const std::optional<Vt> storedValue = valueLookupLayer->layerData.count(accessedValueOfDimension) == 0 ? std::nullopt : valueLookupLayer->layerData[accessedValueOfDimension];
    if (storedValue.has_value() && bitRange.has_value()) {
        return std::make_optional(std::any_cast<Vt>(extractPartsOfValue(*storedValue, *bitRange)));
    }
    return storedValue;
}

template<typename Vt>
// TODO: Should we check that the new value is actually storable in the given bit range (and what should happen to the stored value if this is not the case, should we truncate the new value, clear the existing one, etc.)
void BaseValueLookup<Vt>::updateStoredValueFor(const std::vector<std::optional<unsigned>>& accessedDimensions, const std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess>& bitRange, const Vt& newValue) const {
    const auto& transformedAccessedDimensions = !isValueLookupBlockedFor(accessedDimensions, bitRange) ? transformAccessOnDimensions(accessedDimensions) : std::vector<unsigned int>(0);
    // TODO: What should happen in case that one tries to update more than one value of a dimension ?
    if (!transformedAccessedDimensions.has_value()) {
        return;
    }

    // Since we are trying to update an entry in a lookup the defined access on the dimension must be fully specified (i.e. for every dimension of the signal) and must not access all values of a dimension for any dimension
    std::shared_ptr<LayerData<std::map<unsigned int, std::optional<Vt>>>> valueLookupLayer = valueLookup;

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
            currentValue.emplace(std::any_cast<Vt>(transformExistingValueByMergingWithNewOne(*currentValue, newValue, *bitRange)));
        } else {
            currentValue.emplace(newValue);
        }
    }
}

template<typename Vt>
std::optional<std::vector<unsigned int>> BaseValueLookup<Vt>::transformAccessOnDimensions(const std::vector<std::optional<unsigned int>>& accessedDimensions) const {
    if (accessedDimensions.size() < signalInformation.valuesPerDimension.size()) {
        return std::nullopt;
    }

    const bool accessesAllValueForAnyDimension = std::any_of(
            accessedDimensions.cbegin(),
            accessedDimensions.cend(),
            [](const std::optional<unsigned int>& accessedValueOfDimension) {
                return !accessedValueOfDimension.has_value();
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

template<typename Vt>
bool BaseValueLookup<Vt>::isValueLookupBlockedFor(const std::vector<std::optional<unsigned int>>& accessedDimensions, const std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess>& bitRange) const {
    /*
     * We need to check all accessed dimensions since subsequent dimensions could define more specific restrictions
     */
    return !dimensionAccessRestrictions->doesPredicateHoldInDimensionThenCheckRecursivelyElseStop(0, accessedDimensions, [bitRange](const unsigned int, const std::optional<unsigned int> accessedValueOfDimension, const optimizations::DimensionPropagationBlocker::ptr& dimensionPropagationBlocker) {
        return !dimensionPropagationBlocker->isSubstitutionBlockedFor(accessedValueOfDimension, bitRange);
    });
}