#ifndef BIT_RANGE_ACCESS_RESTRICTION_HPP
#define BIT_RANGE_ACCESS_RESTRICTION_HPP
#pragma once

#include <memory>
#include <utility>

namespace optimizations {
    class BitRangeAccessRestriction {
    public:
        typedef std::pair<unsigned int, unsigned int> BitRangeAccess;
        typedef std::shared_ptr<BitRangeAccessRestriction> ptr;

        [[nodiscard]] bool isAccessCompletelyRestricted() const;
        [[nodiscard]] bool isAccessRestrictedTo(const BitRangeAccess& specificBitRange) const;
        [[nodiscard]] bool isAccessRestrictedToWholeRange(const BitRangeAccess& specificBitRange) const;
        [[nodiscard]] bool hasAnyRestrictions() const;

        void liftRestrictionFor(const BitRangeAccess& specificBitRange);
        void liftAllRestrictions();
        void restrictAccessTo(const BitRangeAccess& specificBitRange);

        explicit BitRangeAccessRestriction() = default;
        explicit BitRangeAccessRestriction(const BitRangeAccess& initialBitRangeRestriction) {}
    };
}; // namespace optimizations

#endif