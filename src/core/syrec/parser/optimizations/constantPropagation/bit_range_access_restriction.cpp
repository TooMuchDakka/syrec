#include "core/syrec/parser/optimizations/constantPropagation/bit_range_access_restriction.hpp"

#include <algorithm>
#include <optional>

using namespace optimizations;

bool BitRangeAccessRestriction::hasAnyRestrictions() const {
    return !restrictionRegions.empty();
}


bool BitRangeAccessRestriction::isAccessCompletelyRestricted() const {
    return false;
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

        if (restrictedRegionsIterator->doesIntersectWith(specificBitRange) && restrictedRegionsIterator->isEmptyAfterTrim(specificBitRange)) {
            restrictionRegions.erase(restrictedRegionsIterator);
        } else {
            ++restrictedRegionsIterator;
        }
    }
}

void BitRangeAccessRestriction::restrictAccessTo(const BitRangeAccess& specificBitRange) {
    std::vector<RestrictionRegion>::iterator firstOverlappingRestrictionRegion;
    for (auto restrictionRegionIterator = restrictionRegions.begin(); restrictionRegionIterator != restrictionRegions.end();) {
        if (restrictionRegionIterator->doesIntersectWith(specificBitRange)) {
            firstOverlappingRestrictionRegion = restrictionRegionIterator;
            break;
        }
        ++restrictionRegionIterator;
    }

    if (firstOverlappingRestrictionRegion == restrictionRegions.end()) {
        restrictionRegions.emplace_back(RestrictionRegion(specificBitRange.first, specificBitRange.second));
        return;
    }

    std::vector<RestrictionRegion>::iterator lastOverlappingRestrictedRegion = firstOverlappingRestrictionRegion;
    for (auto remainingRestrictionRegionIterator = std::next(restrictionRegions.begin(), std::distance(restrictionRegions.begin(), firstOverlappingRestrictionRegion) + 1);
         remainingRestrictionRegionIterator != restrictionRegions.end();) {
        if (!remainingRestrictionRegionIterator->doesIntersectWith(specificBitRange) && remainingRestrictionRegionIterator->start > specificBitRange.first) {
            lastOverlappingRestrictedRegion = std::next(restrictionRegions.begin(), std::distance(restrictionRegions.begin(), remainingRestrictionRegionIterator) - 1);
            break;
        }
        ++remainingRestrictionRegionIterator;
    }

    const std::optional<RestrictionRegion> prefixRegion = (specificBitRange.second < firstOverlappingRestrictionRegion->start) ? std::make_optional(RestrictionRegion(specificBitRange.first, specificBitRange.second)) : std::nullopt;
    const std::optional<RestrictionRegion> postfixRegion = specificBitRange.first > lastOverlappingRestrictedRegion->end ? std::make_optional(RestrictionRegion(specificBitRange.second, 0)) : std::nullopt;

    restrictionRegions.erase(std::next(firstOverlappingRestrictionRegion), lastOverlappingRestrictedRegion);
    if (prefixRegion.has_value() || postfixRegion.has_value()) {
        firstOverlappingRestrictionRegion->resize(
                prefixRegion.has_value() ? (*prefixRegion).start : firstOverlappingRestrictionRegion->start,
                postfixRegion.has_value() ? (*postfixRegion).end : lastOverlappingRestrictedRegion->end);   
    }
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

/*if (restrictedRegionsIterator->start >= specificBitRange.first && restrictedRegionsIterator->end <= specificBitRange.second) {
                restrictionRegions.erase(restrictedRegionsIterator);
            } else if (specificBitRange.second < restrictedRegionsIterator->end) {
                
            } else {
                
            }*/
bool BitRangeAccessRestriction::RestrictionRegion::isEmptyAfterTrim(const BitRangeAccess& bitRangeAccess) {
    const auto firstOverlappingBit = std::max(start, bitRangeAccess.first);
    const auto lastOverlappingBit = std::min(end, bitRangeAccess.second);

    if (firstOverlappingBit == start && lastOverlappingBit == end) {
        return true;
    }

    resize(firstOverlappingBit, lastOverlappingBit);
    return false;
}



