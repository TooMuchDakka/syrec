#include "core/syrec/parser/optimizations/constantPropagation/dimension_propagation_blocker.hpp"

using namespace optimizations;

void DimensionPropagationBlocker::blockSubstitutionForDimension(const std::optional<unsigned>& valueOfDimensionToBlock) {
    if (sharedBlockerInformation.isDimensionCompletelyBlocked || (sharedBlockerInformation.sharedDimensionBitRangeAccessRestriction.has_value() && (*sharedBlockerInformation.sharedDimensionBitRangeAccessRestriction)->isAccessCompletelyRestricted())) {
        return;
    }

    if (!valueOfDimensionToBlock.has_value()) {
        sharedBlockerInformation.isDimensionCompletelyBlocked = true;
        sharedBlockerInformation.sharedDimensionBitRangeAccessRestriction.reset();
        perValueOfDimensionBitRangeAccessRestrictionLookup.clear();
    } else {
        perValueOfDimensionBitRangeAccessRestrictionLookup.insert_or_assign(*valueOfDimensionToBlock, std::make_shared<BitRangeAccessRestriction>(BitRangeAccessRestriction()));
    }
}

void DimensionPropagationBlocker::blockSubstitutionForBitRange(const std::optional<unsigned>& valueOfDimensionToBlock, const BitRangeAccessRestriction::BitRangeAccess& bitRangeToBlock) {
    if (sharedBlockerInformation.isDimensionCompletelyBlocked || (sharedBlockerInformation.sharedDimensionBitRangeAccessRestriction.has_value() && (*sharedBlockerInformation.sharedDimensionBitRangeAccessRestriction)->isAccessRestrictedTo(bitRangeToBlock))) {
        return;
    }

    const auto blockedBitRangeSize              = bitRangeToBlock.second - bitRangeToBlock.first;
    const bool isWholeSignalwidthBlocked = blockedBitRangeSize == sharedBlockerInformation.signalInformation.bitWidth;

    if (!valueOfDimensionToBlock.has_value()) {
        if (isWholeSignalwidthBlocked) {
            blockSubstitutionForDimension(valueOfDimensionToBlock);
        } else {
            if (!sharedBlockerInformation.sharedDimensionBitRangeAccessRestriction.has_value()) {
                sharedBlockerInformation.sharedDimensionBitRangeAccessRestriction = std::make_shared<BitRangeAccessRestriction>(BitRangeAccessRestriction(bitRangeToBlock));
            } else {
                (*sharedBlockerInformation.sharedDimensionBitRangeAccessRestriction)->restrictAccessTo(bitRangeToBlock);
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
            perValueOfDimensionBitRangeAccessRestrictionLookup.insert(std::pair(*valueOfDimensionToBlock, std::make_shared<BitRangeAccessRestriction>(bitRangeToBlock)));
        }
    }
}

void DimensionPropagationBlocker::liftRestrictionForWholeDimension() {
    sharedBlockerInformation.isDimensionCompletelyBlocked = false;
    sharedBlockerInformation.sharedDimensionBitRangeAccessRestriction.reset();
    perValueOfDimensionBitRangeAccessRestrictionLookup.clear();
}

void DimensionPropagationBlocker::liftRestrictionForBitRange(const std::optional<unsigned>& blockedValueOfDimension, const BitRangeAccessRestriction::BitRangeAccess& blockedBitRange) {
    const auto blockedBitRangeSize       = blockedBitRange.second - blockedBitRange.first;
    const bool isWholeSignalwidthBlocked = blockedBitRangeSize == sharedBlockerInformation.signalInformation.bitWidth;

    if (isWholeSignalwidthBlocked) {
        if (blockedValueOfDimension.has_value()) {
            liftRestrictionForValueOfDimension(*blockedValueOfDimension);    
        } else {
            liftRestrictionForWholeDimension();
        }
        return;
    }

    if (!sharedBlockerInformation.isDimensionCompletelyBlocked) {
        /*
         * Check if we are changing an existing dimension wide restriction
         */
        if (sharedBlockerInformation.sharedDimensionBitRangeAccessRestriction.has_value() && (*sharedBlockerInformation.sharedDimensionBitRangeAccessRestriction)->isAccessRestrictedTo(blockedBitRange)) {
            auto& dimensionBitRangeRestriction = sharedBlockerInformation.sharedDimensionBitRangeAccessRestriction;
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
                        perValueOfDimensionBitRangeAccessRestrictionLookup.insert(std::pair(valueOfDimension, std::make_shared<BitRangeAccessRestriction>(blockedBitRange)));
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
            if (!sharedBlockerInformation.sharedDimensionBitRangeAccessRestriction.has_value()) {
                sharedBlockerInformation.sharedDimensionBitRangeAccessRestriction = std::make_shared<BitRangeAccessRestriction>();
                (*sharedBlockerInformation.sharedDimensionBitRangeAccessRestriction)->liftRestrictionFor(blockedBitRange);
            }
        } else {
            BitRangeAccessRestriction remainingRestriction = BitRangeAccessRestriction();
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
    if (sharedBlockerInformation.isDimensionCompletelyBlocked) {
        sharedBlockerInformation.isDimensionCompletelyBlocked = false;
        for (unsigned int valueOfDimensionToBlockCompletely = 0; valueOfDimensionToBlockCompletely < numValuesForDimension; ++valueOfDimensionToBlockCompletely) {
            if (valueOfDimensionToBlockCompletely != blockedValueOfDimension) {
                perValueOfDimensionBitRangeAccessRestrictionLookup.insert_or_assign(valueOfDimensionToBlockCompletely, std::make_shared<BitRangeAccessRestriction>());
            }
        }
    }

    if (perValueOfDimensionBitRangeAccessRestrictionLookup.count(blockedValueOfDimension) != 0) {
        perValueOfDimensionBitRangeAccessRestrictionLookup.erase(blockedValueOfDimension);
    }
}

bool DimensionPropagationBlocker::isSubstitutionBlockedFor(const std::optional<unsigned>& valueOfDimension, const std::optional<BitRangeAccessRestriction::BitRangeAccess>& bitRange) const {
    if (sharedBlockerInformation.isDimensionCompletelyBlocked) {
        return true;
    }

    if (!bitRange.has_value()) {
        if (valueOfDimension.has_value()) {
            return sharedBlockerInformation.sharedDimensionBitRangeAccessRestriction.has_value() || !perValueOfDimensionBitRangeAccessRestrictionLookup.empty();
        }
        return sharedBlockerInformation.sharedDimensionBitRangeAccessRestriction.has_value() && (*sharedBlockerInformation.sharedDimensionBitRangeAccessRestriction)->hasAnyRestrictions();    
    }

    if (valueOfDimension.has_value()) {
        return (sharedBlockerInformation.sharedDimensionBitRangeAccessRestriction.has_value() && (*sharedBlockerInformation.sharedDimensionBitRangeAccessRestriction)->isAccessRestrictedTo(*bitRange))
            || (perValueOfDimensionBitRangeAccessRestrictionLookup.count(*valueOfDimension) != 0 && perValueOfDimensionBitRangeAccessRestrictionLookup.at(*valueOfDimension)->isAccessRestrictedTo(*bitRange));
    }

    const bool hasDimensionWideRestrictionForBitRange = sharedBlockerInformation.sharedDimensionBitRangeAccessRestriction.has_value() && (*sharedBlockerInformation.sharedDimensionBitRangeAccessRestriction)->isAccessRestrictedTo(*bitRange);
    bool isBlockedInValueOfDimension            = hasDimensionWideRestrictionForBitRange;
    for (unsigned int currValueOfDimension = 0; currValueOfDimension < numValuesForDimension && !isBlockedInValueOfDimension; ++currValueOfDimension) {
        if (perValueOfDimensionBitRangeAccessRestrictionLookup.count(currValueOfDimension) != 0) {
            isBlockedInValueOfDimension |= perValueOfDimensionBitRangeAccessRestrictionLookup.at(currValueOfDimension)->isAccessRestrictedTo(*bitRange);    
        }
    }
    return isBlockedInValueOfDimension;
}
