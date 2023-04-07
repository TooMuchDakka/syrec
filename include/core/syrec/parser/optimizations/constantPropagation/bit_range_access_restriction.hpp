#ifndef BIT_RANGE_ACCESS_RESTRICTION_HPP
#define BIT_RANGE_ACCESS_RESTRICTION_HPP
#pragma once

#include <memory>
#include <utility>
#include <vector>

namespace optimizations {
    class BitRangeAccessRestriction {
    public:
        typedef std::pair<unsigned int, unsigned int> BitRangeAccess;
        typedef std::shared_ptr<BitRangeAccessRestriction> ptr;

        [[nodiscard]] bool isAccessCompletelyRestricted() const;
        [[nodiscard]] bool isAccessRestrictedTo(const BitRangeAccess& specificBitRange) const;
        [[nodiscard]] bool isAccessRestrictedToWholeRange(const BitRangeAccess& specificBitRange) const;
        [[nodiscard]] bool hasAnyRestrictions() const;
        [[nodiscard]] std::vector<BitRangeAccess> getRestrictions() const;

        void liftRestrictionFor(const BitRangeAccess& specificBitRange);
        void liftAllRestrictions();
        void restrictAccessTo(const BitRangeAccess& specificBitRange);

        explicit BitRangeAccessRestriction() = default;
        explicit BitRangeAccessRestriction(const BitRangeAccess& initialBitRangeRestriction) {}

    private:
        struct RestrictionRegion {
            unsigned int start;
            unsigned int end;

            explicit RestrictionRegion(const unsigned int start, const unsigned int end) :
                start(start), end(end) {}

            [[nodiscard]] bool doesIntersectWith(const BitRangeAccess& bitRangeAccess) const;
            [[nodiscard]] bool isEmptyAfterTrim(const BitRangeAccess& bitRangeAccess);
            [[nodiscard]] std::size_t getNumberOfOverlappingBitsWith(const BitRangeAccess& bitRangeAccess) const;
            void                      resize(const unsigned int newStartIndex, const unsigned int newEndIndex);
        };

        std::vector<RestrictionRegion> restrictionRegions;
    };
}; // namespace optimizations

#endif