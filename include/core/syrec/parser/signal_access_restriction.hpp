#ifndef SIGNAL_ACCESS_RESTRICTION_HPP
#define SIGNAL_ACCESS_RESTRICTION_HPP
#include "core/syrec/variable.hpp"

#include <algorithm>
#include <set>
#include <unordered_map>

/*
 * TODO: Is set notCompletelyBlockedValuesOfDimension inside of DimensionRestriction correctly updated when lifting or restricting access
 * TODO: Is set notCompletelyBlockedValueForDimension consider when checking the status
 */

namespace parser {
class SignalAccessRestriction {
public:
    struct SignalAccess {
        std::size_t start;
        std::size_t stop;

        explicit SignalAccess(const std::size_t start, const std::size_t stop):
            start(start), stop(stop) {}
    };

    explicit SignalAccessRestriction(const syrec::Variable::ptr& signal):
        isBlockedCompletely(false), signal(signal) {}

    void clearAllRestrictions();
    void blockAccessOnSignalCompletely();

    [[nodiscard]] bool isAccessRestrictedToBit(std::size_t bitPosition) const;
    [[nodiscard]] bool isAccessRestrictedToBit(std::size_t dimension, std::size_t bitPosition) const;
    [[nodiscard]] bool isAccessRestrictedToBit(std::size_t dimension, std::size_t valueForDimension, std::size_t bitPosition) const;

    [[nodiscard]] bool isAccessRestrictedToBitRange(const SignalAccess& bitRange) const;
    [[nodiscard]] bool isAccessRestrictedToBitRange(std::size_t dimension, const SignalAccess& bitRange) const;
    [[nodiscard]] bool isAccessRestrictedToBitRange(std::size_t, std::size_t valueForDimension, const SignalAccess& bitRange) const;

    [[nodiscard]] bool isAccessRestrictedToValueOfDimension(std::size_t dimension, std::size_t valueForDimension) const;
    [[nodiscard]] bool isAccessRestrictedToDimension(std::size_t dimension) const;

    void liftAccessRestrictionsForBit(std::size_t bitPosition);
    void liftAccessRestrictionsForBit(std::size_t dimension, std::size_t bitPosition);
    void liftAccessRestrictionsForBit(std::size_t dimension, std::size_t valueForDimension, std::size_t bitPosition);

    void liftAccessRestrictionForBitRange(const SignalAccess& bitRange);
    void liftAccessRestrictionForBitRange(std::size_t dimension, const SignalAccess& bitRange);
    void liftAccessRestrictionForBitRange(std::size_t dimension, std::size_t valueForDimension, const SignalAccess& bitRange);

    void liftAccessRestrictionForDimension(std::size_t dimension);
    void liftAccessRestrictionForValueOfDimension(std::size_t dimension, std::size_t valueForDimension);

    bool restrictAccessToBit(std::size_t bitPosition);
    bool restrictAccessToBit(std::size_t dimension, std::size_t bitPosition);
    bool restrictAccessToBit(std::size_t, std::size_t valueForDimension, std::size_t bitPosition);

    bool restrictAccessToBitRange(const SignalAccess& bitRange);
    bool restrictAccessToBitRange(std::size_t dimension, const SignalAccess& bitRange);
    bool restrictAccessToBitRange(std::size_t, std::size_t valueForDimension, const SignalAccess& bitRange);

    bool restrictAccessToValueOfDimension(std::size_t dimension, std::size_t valueForDimension);
    bool restrictAccessToDimension(std::size_t dimension);

private:
    struct SignalRestriction {
        explicit SignalRestriction(const std::size_t signalLength):
            isBlockedCompletely(false), signalLength(signalLength), restrictedRegions({}) {}

        void restrictAccessTo(const SignalAccess& signalPart) {
            if (isBlockedCompletely || signalPart.start > signalPart.stop || !isAccessWithinRange(signalPart)) {
                return;
            }

            if (isAccessOutsideOfBorderRestrictions(signalPart)) {
                const auto newRestriction = RestrictionRegion(signalPart.start, signalPart.stop - signalPart.start + 1);
                if (restrictedRegions.empty()) {
                    restrictedRegions.emplace_back(newRestriction);
                    return;
                }

                const auto& lowerBorderRestrictionRegion = restrictedRegions.at(0);
                if (signalPart.start < lowerBorderRestrictionRegion.startPosition) {
                    restrictedRegions.insert(restrictedRegions.begin(), newRestriction);
                } else {
                    restrictedRegions.insert(restrictedRegions.end(), newRestriction);
                }
                return;
            }
            
            for (auto existingRestrictionRegionIterator = restrictedRegions.begin(); existingRestrictionRegionIterator != restrictedRegions.end();) {
                if (signalPart.start >= existingRestrictionRegionIterator->startPosition && signalPart.stop <= existingRestrictionRegionIterator->endPosition) {
                    return;
                }

                if (signalPart.start < existingRestrictionRegionIterator->startPosition) {
                    if (signalPart.stop < existingRestrictionRegionIterator->startPosition) {
                        restrictedRegions.insert(existingRestrictionRegionIterator, RestrictionRegion(signalPart.start, signalPart.stop - signalPart.start + 1));
                        return;
                    }
                }

                if (signalPart.start <= existingRestrictionRegionIterator->startPosition) {
                    if (signalPart.stop <= existingRestrictionRegionIterator->endPosition) {
                        auto restrictionRegionWithSmallerStartingPosition = std::prev(existingRestrictionRegionIterator, -1);
                        if (restrictionRegionWithSmallerStartingPosition != restrictedRegions.begin() && restrictionRegionWithSmallerStartingPosition->doesAccessIntersectRegion(signalPart)) {
                            restrictionRegionWithSmallerStartingPosition->endPosition = existingRestrictionRegionIterator->endPosition;
                            restrictionRegionWithSmallerStartingPosition->updateLength();
                            restrictedRegions.erase(existingRestrictionRegionIterator++);

                        } else {
                            existingRestrictionRegionIterator->startPosition = signalPart.start;
                            return;
                        }
                    } else {
                        const auto& restrictionRegionWithLargerStartingPosition = std::next(existingRestrictionRegionIterator, 1);
                        if (restrictionRegionWithLargerStartingPosition == restrictedRegions.end()
                            || (restrictionRegionWithLargerStartingPosition != restrictedRegions.end() && signalPart.stop < restrictionRegionWithLargerStartingPosition->startPosition)) {
                            existingRestrictionRegionIterator->endPosition = signalPart.stop;
                            existingRestrictionRegionIterator->updateLength();
                            return;
                        }

                        existingRestrictionRegionIterator->endPosition = restrictionRegionWithLargerStartingPosition->endPosition;
                        existingRestrictionRegionIterator->updateLength();
                        restrictedRegions.erase(++existingRestrictionRegionIterator);
                        return;
                    }
                }
                ++existingRestrictionRegionIterator;
            }
        }

        // TODO: Implement me with updated implementation (see previous one for reference)
        void removeRestrictionFor(const SignalAccess& signalPart) {
            return;
        }

        [[nodiscard]] bool isAccessRestricted(std::size_t bitPosition) const {
            return isAccessRestricted(SignalAccess(bitPosition, bitPosition));
        }

        [[nodiscard]] bool isAccessRestricted(const SignalAccess& signalPart) const {
            return isBlockedCompletely || 
                (!isAccessOutsideOfBorderRestrictions(signalPart)
                && std::any_of(
                    restrictedRegions.cbegin(),
                    restrictedRegions.cend(),
                    [signalPart](const RestrictionRegion& restrictedRegion) {
                        return restrictedRegion.doesAccessIntersectRegion(signalPart);
                    }));
        }

        [[nodiscard]] bool isAccessOutsideOfBorderRestrictions(const SignalAccess& signalPart) const {
            if (restrictedRegions.empty()) {
                return true;
            }
            const auto& lowerBorderRestrictionRegion = restrictedRegions.at(0);
            if (!lowerBorderRestrictionRegion.doesAccessIntersectRegion(signalPart)) {
                return true;
            }
            
            const auto& upperBorderRestrictionRegion = restrictedRegions.size() > 1 ? restrictedRegions.at(restrictedRegions.size() - 1) : lowerBorderRestrictionRegion;
            return !upperBorderRestrictionRegion.doesAccessIntersectRegion(signalPart);
        }

        void blockCompletely() {
            isBlockedCompletely = true;
            restrictedRegions.clear();
        }

        void unblockCompletely() {
            isBlockedCompletely = false;
            restrictedRegions.clear();
        }

        private:
            struct RestrictionRegion {
                std::size_t startPosition;
                std::size_t length;
                std::size_t endPosition;

                explicit RestrictionRegion(std::size_t startPosition, std::size_t length):
                    startPosition(startPosition), length(length), endPosition(startPosition + length - 1) {}
                
                [[nodiscard]] bool doesAccessIntersectRegion(const SignalAccess& signalAccess) const {
                    return !((signalAccess.start < startPosition && signalAccess.stop < endPosition) || signalAccess.start > endPosition);
                }

                void updateLength() {
                    length = endPosition - startPosition + 1;
                }
            };

            bool isBlockedCompletely;
            std::size_t                        signalLength;

            std::vector<RestrictionRegion> restrictedRegions;
            
            [[nodiscard]] bool isAccessWithinRange(const SignalAccess& signalAccess) const {
                return signalAccess.start <= signalLength && signalAccess.stop <= signalLength;
            }
    };

    struct DimensionRestriction {
        bool                                     areAllValuesForDimensionBlocked;
        std::size_t                              numValuesForDimension;
        std::set<std::size_t> notCompletelyBlockedValuesOfDimension;
        std::unordered_map<std::size_t, SignalRestriction> restrictionsPerValueOfDimension;
        std::optional<SignalRestriction>         dimensionSignalRestriction;

        explicit DimensionRestriction(std::size_t numValuesForDimension):
            areAllValuesForDimensionBlocked(false), numValuesForDimension(numValuesForDimension) {}
    };


    bool                                                  isBlockedCompletely;
    const syrec::Variable::ptr                            signal;
    std::unordered_map<std::size_t, DimensionRestriction> dimensionRestrictions;
    std::optional<SignalRestriction>                      globalSignalRestriction;

    [[nodiscard]] DimensionRestriction createDimensionRestriction(std::size_t dimension) const {
        return DimensionRestriction(signal->dimensions.at(dimension));
    }
    
    [[nodiscard]] bool isDimensionWithinRange(std::size_t dimension) const;
    [[nodiscard]] bool isValueForDimensionWithinRange(std::size_t dimension, std::size_t valueForDimension) const;
    [[nodiscard]] bool isBitWithinRange(std::size_t bitPosition) const;
    [[nodiscard]] bool isBitRangeWithinRange(const SignalAccess& bitRange) const;
};
}

#endif