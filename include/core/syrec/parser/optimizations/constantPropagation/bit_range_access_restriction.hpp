#ifndef BIT_RANGE_ACCESS_RESTRICTION_HPP
#define BIT_RANGE_ACCESS_RESTRICTION_HPP
#pragma once

#include <memory>
#include <utility>
#include <vector>

namespace optimizations {
    // TODO: Since we are note throwing any exceptions, should we simply trim the given bit range from a parameter to a valid range ?
    class BitRangeAccessRestriction {
    public:
        typedef std::pair<unsigned int, unsigned int> BitRangeAccess;
        typedef std::shared_ptr<BitRangeAccessRestriction> ptr;

        [[nodiscard]] bool isAccessCompletelyRestricted() const;
        [[nodiscard]] bool isAccessRestrictedTo(const BitRangeAccess& specificBitRange) const;
        [[nodiscard]] bool isAccessRestrictedToWholeRange(const BitRangeAccess& specificBitRange) const;
        [[nodiscard]] bool hasAnyRestrictions() const;
        [[nodiscard]] std::vector<BitRangeAccess> getRestrictions() const;
        [[nodiscard]] std::vector<BitRangeAccess> getRestrictionsOverlappingBitRangeAccess(const BitRangeAccess& specificBitRange) const;

        void liftRestrictionFor(const BitRangeAccess& specificBitRange);
        void liftAllRestrictions();
        void restrictAccessTo(const BitRangeAccess& specificBitRange);

        explicit BitRangeAccessRestriction(const unsigned int validBitRange)
            : validBitRange(validBitRange), restrictionRegions({}) {}
         

        explicit BitRangeAccessRestriction(const unsigned int validBitRange, const BitRangeAccess& initialBitRangeRestriction)
            : BitRangeAccessRestriction(validBitRange)
        {
            restrictAccessTo(initialBitRangeRestriction);
        }

    private:
        struct RestrictionRegion {
            unsigned int start;
            unsigned int end;

            explicit RestrictionRegion(const unsigned int start, const unsigned int end) :
                start(start), end(end) {}

            [[nodiscard]] bool doesIntersectWith(const BitRangeAccess& bitRangeAccess) const;
            [[nodiscard]] std::size_t getNumberOfOverlappingBitsWith(const BitRangeAccess& bitRangeAccess) const;
            void                      resize(const unsigned int newStartIndex, const unsigned int newEndIndex);
        };

        const unsigned int             validBitRange;
        std::vector<RestrictionRegion> restrictionRegions;
    };
}; // namespace optimizations

#endif