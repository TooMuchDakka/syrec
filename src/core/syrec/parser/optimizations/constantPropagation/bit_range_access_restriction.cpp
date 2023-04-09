#include "core/syrec/parser/optimizations/constantPropagation/bit_range_access_restriction.hpp"

#include <algorithm>
#include <optional>

using namespace optimizations;

bool BitRangeAccessRestriction::hasAnyRestrictions() const {
    return !restrictionRegions.empty();
}

bool BitRangeAccessRestriction::isAccessCompletelyRestricted() const {
    return isAccessRestrictedToWholeRange(BitRangeAccess(0, validBitRange - 1));
}

bool BitRangeAccessRestriction::isAccessRestrictedTo(const BitRangeAccess& specificBitRange) const {
    return std::any_of(
    restrictionRegions.cbegin(),
    restrictionRegions.cend(),
    [&specificBitRange](const RestrictionRegion& restrictionRegion) {
        return restrictionRegion.doesIntersectWith(specificBitRange);
    });
}

bool BitRangeAccessRestriction::isAccessRestrictedToWholeRange(const BitRangeAccess& specificBitRange) const {
    std::vector<RestrictionRegion>::const_iterator firstOverlappingRestrictionRegion;
    for (auto restrictionRegionIterator = restrictionRegions.cbegin(); restrictionRegionIterator != restrictionRegions.cend();) {
        if (restrictionRegionIterator->doesIntersectWith(specificBitRange)) {
            firstOverlappingRestrictionRegion = restrictionRegionIterator;
            break;
        }
        ++restrictionRegionIterator;
    }

    if (firstOverlappingRestrictionRegion == restrictionRegions.cend()) {
        return false;
    }

    std::size_t remainingNotCoveredBits = (specificBitRange.second - specificBitRange.first) + 1;
    remainingNotCoveredBits -= firstOverlappingRestrictionRegion->getNumberOfOverlappingBitsWith(specificBitRange);
    if (remainingNotCoveredBits == 0) {
        return true;
    }

    for (auto remainingRestrictionRegionIterator = std::next(restrictionRegions.cbegin(), std::distance(restrictionRegions.cbegin(), firstOverlappingRestrictionRegion) + 1);
         remainingRestrictionRegionIterator != restrictionRegions.cend();) {
        remainingNotCoveredBits -= remainingRestrictionRegionIterator->getNumberOfOverlappingBitsWith(specificBitRange);
        if (remainingNotCoveredBits == 0) {
            break;
        }
        ++remainingRestrictionRegionIterator;
    }
    
    return remainingNotCoveredBits == 0;
}

std::vector<BitRangeAccessRestriction::BitRangeAccess> BitRangeAccessRestriction::getRestrictions() const {
    if (restrictionRegions.empty()) {
        return {};
    }

    std::vector<BitRangeAccess> transformedRestrictions(restrictionRegions.size());
    std::transform(
        restrictionRegions.cbegin(),
        restrictionRegions.cend(),
        transformedRestrictions.begin(),
        [](const RestrictionRegion& restrictedRegion) {
            return BitRangeAccess(std::make_pair(restrictedRegion.start, restrictedRegion.end));
        });
    return transformedRestrictions;
}

void BitRangeAccessRestriction::liftAllRestrictions() {
    restrictionRegions.clear();
}

void BitRangeAccessRestriction::liftRestrictionFor(const BitRangeAccess& specificBitRange) {
    for (auto restrictedRegionsIterator = restrictionRegions.begin(); restrictedRegionsIterator != restrictionRegions.end();) {
        if (restrictedRegionsIterator->start > specificBitRange.second) {
            break;
        }

        if (!restrictedRegionsIterator->doesIntersectWith(specificBitRange)) {
            ++restrictedRegionsIterator;
        } else {
            const auto lastOverlappingBit = std::min(specificBitRange.second, restrictedRegionsIterator->end);
            if (specificBitRange.first < restrictedRegionsIterator->start) {
                if (lastOverlappingBit == restrictedRegionsIterator->end) {
                    restrictionRegions.erase(restrictedRegionsIterator);
                }
                else {
                    restrictedRegionsIterator->resize(specificBitRange.second + 1, restrictedRegionsIterator->end - 1);
                    ++restrictedRegionsIterator;
                }
            } else {
                const auto firstOverlappingBit                     = std::max(specificBitRange.first, restrictedRegionsIterator->start);
                const bool hasRemainingRestrictionPriorToLiftedOne = firstOverlappingBit > restrictedRegionsIterator->start;
                const bool hasRemainingRestrictionAfterLiftedOne   = lastOverlappingBit < restrictedRegionsIterator->end;

                if (!hasRemainingRestrictionPriorToLiftedOne && !hasRemainingRestrictionAfterLiftedOne) {
                    restrictionRegions.erase(restrictedRegionsIterator);
                }
                else if (hasRemainingRestrictionPriorToLiftedOne && !hasRemainingRestrictionAfterLiftedOne) {
                    restrictedRegionsIterator->resize(restrictedRegionsIterator->start, firstOverlappingBit - 1);
                } else if (!hasRemainingRestrictionPriorToLiftedOne && hasRemainingRestrictionAfterLiftedOne) {
                    restrictedRegionsIterator->resize(lastOverlappingBit + 1, restrictedRegionsIterator->end);
                } else {
                    restrictedRegionsIterator->resize(restrictedRegionsIterator->start, firstOverlappingBit - 1);
                    restrictionRegions.insert(std::next(restrictedRegionsIterator), RestrictionRegion(lastOverlappingBit + 1, restrictedRegionsIterator->end));
                }

                if (hasRemainingRestrictionPriorToLiftedOne || hasRemainingRestrictionAfterLiftedOne) {
                    ++restrictedRegionsIterator;
                }
            }
        }
    }
}

void BitRangeAccessRestriction::restrictAccessTo(const BitRangeAccess& specificBitRange) {
    const auto& firstOverlappingRestrictionRegion = std::find_if(
            restrictionRegions.begin(),
            restrictionRegions.end(),
            [&specificBitRange](const RestrictionRegion& restrictedRegion) {
                return restrictedRegion.doesIntersectWith(specificBitRange);
            }
    );

    if (firstOverlappingRestrictionRegion == restrictionRegions.end()) {
        restrictionRegions.emplace_back(RestrictionRegion(specificBitRange.first, specificBitRange.second));
        return;
    }

    auto lastOverlappingRestrictedRegion = firstOverlappingRestrictionRegion;
    for (auto remainingRestrictionsIterator = std::next(firstOverlappingRestrictionRegion); remainingRestrictionsIterator != restrictionRegions.end();) {
        if (!remainingRestrictionsIterator->doesIntersectWith(specificBitRange)) {
            lastOverlappingRestrictedRegion = std::prev(remainingRestrictionsIterator);
            break;
        }
        ++remainingRestrictionsIterator;
    }

    std::optional<RestrictionRegion> copyOfLastOverlappingRegion;
    if (firstOverlappingRestrictionRegion != lastOverlappingRestrictedRegion) {
        copyOfLastOverlappingRegion.emplace(lastOverlappingRestrictedRegion->start, lastOverlappingRestrictedRegion->end);
        restrictionRegions.erase(std::next(firstOverlappingRestrictionRegion), std::prev(lastOverlappingRestrictedRegion));    
    }

    firstOverlappingRestrictionRegion->resize(
            std::min(specificBitRange.first, firstOverlappingRestrictionRegion->start),
            std::max(specificBitRange.second, copyOfLastOverlappingRegion.has_value() ? (*copyOfLastOverlappingRegion).end : firstOverlappingRestrictionRegion->end)
    );
}

void BitRangeAccessRestriction::RestrictionRegion::resize(const unsigned newStartIndex, const unsigned newEndIndex) {
    start = newStartIndex;
    end   = newEndIndex;
}

bool BitRangeAccessRestriction::RestrictionRegion::doesIntersectWith(const BitRangeAccess& bitRangeAccess) const {
    return start >= bitRangeAccess.first && end <= bitRangeAccess.second;
}

std::size_t BitRangeAccessRestriction::RestrictionRegion::getNumberOfOverlappingBitsWith(const BitRangeAccess& bitRangeAccess) const {
    if (!doesIntersectWith(bitRangeAccess)) {
        return 0;
    }

    return (std::min(end, bitRangeAccess.second) - std::max(start, bitRangeAccess.first)) + 1;
}