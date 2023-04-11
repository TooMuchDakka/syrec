#include "core/syrec/parser/optimizations/constantPropagation/dimension_propagation_blocker.hpp"

#include <algorithm>

using namespace optimizations;

void DimensionPropagationBlocker::blockSubstitutionForDimension(const std::optional<unsigned>& valueOfDimensionToBlock) {
    if (isDimensionCompletelyBlocked || (dimensionBitRangeAccessRestriction.has_value() && (*dimensionBitRangeAccessRestriction)->isAccessCompletelyRestricted())) {
        return;
    }

    if (!valueOfDimensionToBlock.has_value()) {
        isDimensionCompletelyBlocked = true;
        dimensionBitRangeAccessRestriction.reset();
        perValueOfDimensionBitRangeAccessRestrictionLookup.clear();
    } else {
        perValueOfDimensionBitRangeAccessRestrictionLookup.insert_or_assign(*valueOfDimensionToBlock, std::make_shared<BitRangeAccessRestriction>(BitRangeAccessRestriction(signalInformation.bitWidth)));
    }
}

void DimensionPropagationBlocker::blockSubstitutionForBitRange(const std::optional<unsigned>& valueOfDimensionToBlock, const BitRangeAccessRestriction::BitRangeAccess& bitRangeToBlock) {
    if (isDimensionCompletelyBlocked || (dimensionBitRangeAccessRestriction.has_value() && (*dimensionBitRangeAccessRestriction)->isAccessRestrictedTo(bitRangeToBlock))) {
        return;
    }

    const auto blockedBitRangeSize              = (bitRangeToBlock.second - bitRangeToBlock.first) + 1;
    const bool isWholeSignalwidthBlocked = blockedBitRangeSize == signalInformation.bitWidth;

    if (!valueOfDimensionToBlock.has_value()) {
        if (isWholeSignalwidthBlocked) {
            blockSubstitutionForDimension(valueOfDimensionToBlock);
        } else {
            if (!dimensionBitRangeAccessRestriction.has_value()) {
                dimensionBitRangeAccessRestriction = std::make_shared<BitRangeAccessRestriction>(BitRangeAccessRestriction(signalInformation.bitWidth, bitRangeToBlock));
            } else {
                (*dimensionBitRangeAccessRestriction)->restrictAccessTo(bitRangeToBlock);
            }
        }

        for (auto it = perValueOfDimensionBitRangeAccessRestrictionLookup.begin(); it != perValueOfDimensionBitRangeAccessRestrictionLookup.end();) {
            const auto& restrictionForValueOfDimension = it->second;

            bool canRestrictionBeRemoved = isWholeSignalwidthBlocked;
            if (!canRestrictionBeRemoved && restrictionForValueOfDimension->isAccessRestrictedTo(bitRangeToBlock)) {
                restrictionForValueOfDimension->liftRestrictionFor(bitRangeToBlock);
                canRestrictionBeRemoved = restrictionForValueOfDimension->hasAnyRestrictions();
            }

            if (canRestrictionBeRemoved) {
                it = perValueOfDimensionBitRangeAccessRestrictionLookup.erase(it);    
            } else {
                ++it;
            }
        }
    } else {
        if (perValueOfDimensionBitRangeAccessRestrictionLookup.count(*valueOfDimensionToBlock) != 0) {
            const auto& restrictionForValueOfDimension = perValueOfDimensionBitRangeAccessRestrictionLookup[*valueOfDimensionToBlock];
            restrictionForValueOfDimension->restrictAccessTo(bitRangeToBlock);
        } else {
            perValueOfDimensionBitRangeAccessRestrictionLookup.insert(std::pair(*valueOfDimensionToBlock, std::make_shared<BitRangeAccessRestriction>(signalInformation.bitWidth, bitRangeToBlock)));
        }
    }
}

void DimensionPropagationBlocker::liftRestrictionForWholeDimension() {
    isDimensionCompletelyBlocked = false;
    dimensionBitRangeAccessRestriction.reset();
    perValueOfDimensionBitRangeAccessRestrictionLookup.clear();
}

void DimensionPropagationBlocker::liftRestrictionForBitRange(const std::optional<unsigned>& blockedValueOfDimension, const BitRangeAccessRestriction::BitRangeAccess& blockedBitRange) {
    const auto blockedBitRangeSize       = (blockedBitRange.second - blockedBitRange.first) + 1;
    const bool isWholeSignalwidthBlocked = blockedBitRangeSize == signalInformation.bitWidth;

    if (isWholeSignalwidthBlocked) {
        if (blockedValueOfDimension.has_value()) {
            liftRestrictionForValueOfDimension(*blockedValueOfDimension);    
        } else {
            liftRestrictionForWholeDimension();
        }
        return;
    }

    if (!isDimensionCompletelyBlocked) {
        /*
         * Check if we are changing an existing dimension wide restriction
         */
        if (dimensionBitRangeAccessRestriction.has_value() && (*dimensionBitRangeAccessRestriction)->isAccessRestrictedTo(blockedBitRange)) {
            auto& dimensionBitRangeRestriction = dimensionBitRangeAccessRestriction;
            (*dimensionBitRangeRestriction)->liftRestrictionFor(blockedBitRange);
            if (!(*dimensionBitRangeRestriction)->hasAnyRestrictions()) {
                dimensionBitRangeRestriction->reset();
            }

            /*
             * If we removed an existing bit range restriction for a specific value of a dimension that was originally set for the whole dimension, we need to recreate the restriction for the remaining values of the dimension
             */
            if (blockedValueOfDimension.has_value()) {
                for (unsigned int valueOfDimension = 0; valueOfDimension < numValuesForDimension; ++valueOfDimension) {
                    if (perValueOfDimensionBitRangeAccessRestrictionLookup.count(valueOfDimension) == 0) {
                        perValueOfDimensionBitRangeAccessRestrictionLookup.insert(std::pair(valueOfDimension, std::make_shared<BitRangeAccessRestriction>(signalInformation.bitWidth, blockedBitRange)));
                    } else {
                        perValueOfDimensionBitRangeAccessRestrictionLookup[valueOfDimension]->restrictAccessTo(blockedBitRange);
                    }
                }
            }
        } else {
            /*
             * There existed no dimension wide block of the given bit range to unblocked so try to unblock it on a per value of dimension basis
             */
            if (blockedValueOfDimension.has_value()) {
                if (perValueOfDimensionBitRangeAccessRestrictionLookup.count(*blockedValueOfDimension) != 0 && perValueOfDimensionBitRangeAccessRestrictionLookup[*blockedValueOfDimension]->isAccessRestrictedTo(blockedBitRange)) {
                    const auto& restrictionForValueOfDimension = perValueOfDimensionBitRangeAccessRestrictionLookup[*blockedValueOfDimension];
                    restrictionForValueOfDimension->liftRestrictionFor(blockedBitRange);
                    if (!restrictionForValueOfDimension->hasAnyRestrictions()) {
                        perValueOfDimensionBitRangeAccessRestrictionLookup.erase(*blockedValueOfDimension);
                    }
                }
            } else {
                for (auto it = perValueOfDimensionBitRangeAccessRestrictionLookup.begin(); it != perValueOfDimensionBitRangeAccessRestrictionLookup.end();) {
                    const auto restrictionOfValueOfDimension = it->second;
                    if (restrictionOfValueOfDimension->isAccessRestrictedTo(blockedBitRange)) {
                        restrictionOfValueOfDimension->liftRestrictionFor(blockedBitRange);
                        if (!restrictionOfValueOfDimension->hasAnyRestrictions()) {
                            it = perValueOfDimensionBitRangeAccessRestrictionLookup.erase(it);
                            continue;
                        }
                    }
                    ++it;
                }                   
            }
        }
    }
    else {
        /*
         * Since the unblocked bit range leaves some of the signal still blocked, we need to either create the remaining restriction on a dimension or per value of dimension wide level
         */
        if (!blockedValueOfDimension.has_value()) {
            if (!dimensionBitRangeAccessRestriction.has_value()) {
                dimensionBitRangeAccessRestriction = std::make_shared<BitRangeAccessRestriction>(signalInformation.bitWidth);
                (*dimensionBitRangeAccessRestriction)->liftRestrictionFor(blockedBitRange);
            }
        } else {
            BitRangeAccessRestriction remainingRestriction = BitRangeAccessRestriction(signalInformation.bitWidth);
            remainingRestriction.liftRestrictionFor(blockedBitRange);

            for (unsigned int valueOfDimension = 0; valueOfDimension < numValuesForDimension; ++valueOfDimension) {
                if (valueOfDimension != *blockedValueOfDimension) {
                    perValueOfDimensionBitRangeAccessRestrictionLookup.insert(std::make_pair(valueOfDimension, std::make_shared<BitRangeAccessRestriction>(remainingRestriction)));   
                }
            }
        }
    }
}

void DimensionPropagationBlocker::liftRestrictionForValueOfDimension(const unsigned blockedValueOfDimension) {
    if (isDimensionCompletelyBlocked) {
        isDimensionCompletelyBlocked = false;
        for (unsigned int valueOfDimensionToBlockCompletely = 0; valueOfDimensionToBlockCompletely < numValuesForDimension; ++valueOfDimensionToBlockCompletely) {
            if (valueOfDimensionToBlockCompletely != blockedValueOfDimension) {
                perValueOfDimensionBitRangeAccessRestrictionLookup.insert_or_assign(valueOfDimensionToBlockCompletely, std::make_shared<BitRangeAccessRestriction>(signalInformation.bitWidth));
            }
        }
    }

    if (perValueOfDimensionBitRangeAccessRestrictionLookup.count(blockedValueOfDimension) != 0) {
        perValueOfDimensionBitRangeAccessRestrictionLookup.erase(blockedValueOfDimension);
    }
}

bool DimensionPropagationBlocker::isSubstitutionBlockedFor(const std::optional<unsigned>& valueOfDimension, const std::optional<BitRangeAccessRestriction::BitRangeAccess>& bitRange, bool ignoreNotFullwidthBitRangeRestrictions, bool ignoreSmallerThanAccessedBitrange) const {
    if (isDimensionCompletelyBlocked) {
        return true;
    }

    if (!bitRange.has_value()) {
        if (valueOfDimension.has_value()) {
            if (ignoreNotFullwidthBitRangeRestrictions) {
                return (dimensionBitRangeAccessRestriction.has_value() && (*dimensionBitRangeAccessRestriction)->isAccessCompletelyRestricted())
                    || std::any_of(
                        perValueOfDimensionBitRangeAccessRestrictionLookup.cbegin(),
                        perValueOfDimensionBitRangeAccessRestrictionLookup.cend(),
                        [](const auto& mapEntry) {
                            const BitRangeAccessRestriction::ptr perValueOfDimensionRestriction = mapEntry.second;
                            return perValueOfDimensionRestriction->isAccessCompletelyRestricted();
                    });
            }
            return dimensionBitRangeAccessRestriction.has_value() || !perValueOfDimensionBitRangeAccessRestrictionLookup.empty();
        }

        const bool isBlockedForWholeDimension = dimensionBitRangeAccessRestriction.has_value()
            && (ignoreNotFullwidthBitRangeRestrictions 
                ? (*dimensionBitRangeAccessRestriction)->isAccessCompletelyRestricted()
                : (*dimensionBitRangeAccessRestriction)->hasAnyRestrictions());

        const bool isBlockedInAnyValueOfDimension = isBlockedForWholeDimension
            || std::any_of(
              perValueOfDimensionBitRangeAccessRestrictionLookup.cbegin(),
              perValueOfDimensionBitRangeAccessRestrictionLookup.cend(),
              [ignoreNotFullwidthBitRangeRestrictions](const auto& mapEntry) {
                  const BitRangeAccessRestriction::ptr perValueOfDimensionRestriction = mapEntry.second;
                  return ignoreNotFullwidthBitRangeRestrictions ? perValueOfDimensionRestriction->isAccessCompletelyRestricted() : perValueOfDimensionRestriction->hasAnyRestrictions();
              });
        return isBlockedInAnyValueOfDimension;
    }

    const auto accessedBitRange                    = *bitRange;
    bool isAccessRestrictedForWhileDimension;
    if (ignoreNotFullwidthBitRangeRestrictions) {
        isAccessRestrictedForWhileDimension = dimensionBitRangeAccessRestriction.has_value() && (*dimensionBitRangeAccessRestriction)->isAccessCompletelyRestricted();
    }
    else if (ignoreSmallerThanAccessedBitrange) {
        isAccessRestrictedForWhileDimension = dimensionBitRangeAccessRestriction.has_value() && (*dimensionBitRangeAccessRestriction)->isAccessRestrictedToWholeRange(accessedBitRange);
    } else {
        isAccessRestrictedForWhileDimension = dimensionBitRangeAccessRestriction.has_value() && (*dimensionBitRangeAccessRestriction)->isAccessRestrictedTo(accessedBitRange);
    }

    if (valueOfDimension.has_value()) {
        if (isAccessRestrictedForWhileDimension || perValueOfDimensionBitRangeAccessRestrictionLookup.count(*valueOfDimension) == 0) {
            return isAccessRestrictedForWhileDimension;   
        }
        const auto& restrictionForValueOfDimension = perValueOfDimensionBitRangeAccessRestrictionLookup.at(*valueOfDimension);
        if (ignoreNotFullwidthBitRangeRestrictions) {
            return restrictionForValueOfDimension->isAccessCompletelyRestricted();
        }
        if (ignoreSmallerThanAccessedBitrange) {
            return restrictionForValueOfDimension->isAccessRestrictedToWholeRange(accessedBitRange);
        }
        return restrictionForValueOfDimension->isAccessRestrictedTo(accessedBitRange);
    }

    if (isAccessRestrictedForWhileDimension) {
        return true;
    }

    return std::any_of(
    perValueOfDimensionBitRangeAccessRestrictionLookup.cbegin(),
    perValueOfDimensionBitRangeAccessRestrictionLookup.cend(),
    [ignoreNotFullwidthBitRangeRestrictions, ignoreSmallerThanAccessedBitrange, accessedBitRange](const auto& mapEntry) {
        const BitRangeAccessRestriction::ptr perValueOfDimensionRestriction = mapEntry.second;
                if (ignoreNotFullwidthBitRangeRestrictions) {
                    return perValueOfDimensionRestriction->isAccessCompletelyRestricted();
                }
                if (ignoreSmallerThanAccessedBitrange) {
                    return perValueOfDimensionRestriction->isAccessRestrictedToWholeRange(accessedBitRange);
                }
                return perValueOfDimensionRestriction->isAccessRestrictedTo(accessedBitRange);
                
    });
}

bool DimensionPropagationBlocker::tryTrimAlreadyBlockedPartsFromRestriction(const std::optional<unsigned>& accessedValueOfDimension, const std::optional<BitRangeAccessRestriction::BitRangeAccess>& accessedBitRange, BitRangeAccessRestriction& bitRangeAccess) const {
    if (isDimensionCompletelyBlocked) {
        bitRangeAccess.liftAllRestrictions();
        return true;
    }

    if (dimensionBitRangeAccessRestriction.has_value()) {
        if (accessedBitRange.has_value() && (*dimensionBitRangeAccessRestriction)->isAccessRestrictedTo(*accessedBitRange)) {
            
        } else if (!accessedBitRange.has_value() && (*dimensionBitRangeAccessRestriction)->isAccessCompletelyRestricted()) {
            bitRangeAccess.liftAllRestrictions();
            return true;
        }
    }

    if ((!accessedValueOfDimension.has_value() && perValueOfDimensionBitRangeAccessRestrictionLookup.empty())
        || (accessedValueOfDimension.has_value() && perValueOfDimensionBitRangeAccessRestrictionLookup.count(*accessedValueOfDimension) == 0)) {
        return false;
    }

    if (accessedValueOfDimension.has_value()) {
        const BitRangeAccessRestriction::ptr& perValueOfDimensionBitRangeRestriction = perValueOfDimensionBitRangeAccessRestrictionLookup.at(*accessedValueOfDimension);
        if (perValueOfDimensionBitRangeRestriction->isAccessCompletelyRestricted()) {
            bitRangeAccess.liftAllRestrictions();
            return true;
        }

        if (accessedBitRange.has_value() && perValueOfDimensionBitRangeRestriction->isAccessRestrictedTo(*accessedBitRange)) {
            for (const auto& restriction : perValueOfDimensionBitRangeRestriction->getRestrictions()) {
                bitRangeAccess.liftRestrictionFor(restriction);
            }
            return true;
        }
        return false;
    }

    // TODO: In case that no specific value of the dimension is specified, we can only update the given bit range access if it is fully covered in all values of the dimension
    const bool isAccessedBitRangeCoveredInAllValuesOfDimension = std::all_of(
            perValueOfDimensionBitRangeAccessRestrictionLookup.cbegin(),
            perValueOfDimensionBitRangeAccessRestrictionLookup.cend(),
            [&accessedBitRange](const auto& mapEntry) {
                const BitRangeAccessRestriction::ptr& perValueOfDimensionBitRangeRestriction = mapEntry.second;
                if (accessedBitRange.has_value()) {
                    return perValueOfDimensionBitRangeRestriction->isAccessRestrictedToWholeRange(*accessedBitRange);
                }
                return perValueOfDimensionBitRangeRestriction->isAccessCompletelyRestricted();
            });
    if (isAccessedBitRangeCoveredInAllValuesOfDimension) {
        bitRangeAccess.liftAllRestrictions();
        return true;
    }
    return false;
}

