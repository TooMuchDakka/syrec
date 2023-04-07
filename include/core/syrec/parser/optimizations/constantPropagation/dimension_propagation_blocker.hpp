#ifndef DIMENSION_PROPAGATION_BLOCKER_HPP
#define DIMENSION_PROPAGATION_BLOCKER_HPP
#pragma once

#include "core/syrec/parser/optimizations/constantPropagation/bit_range_access_restriction.hpp"
#include "core/syrec/parser/optimizations/constantPropagation/shared_dimension_blocking_information.hpp"

#include <map>
#include <optional>
#include <vector>

namespace optimizations {
    class DimensionPropagationBlocker {
    public:
        typedef std::shared_ptr<DimensionPropagationBlocker> ptr;
        
        explicit DimensionPropagationBlocker(const unsigned int dimension, const SignalDimensionInformation& signalInformation):
            isDimensionCompletelyBlocked(false), signalInformation(signalInformation), dimensionBitRangeAccessRestriction(std::nullopt), perValueOfDimensionBitRangeAccessRestrictionLookup({}), numValuesForDimension(signalInformation.valuesPerDimension.at(dimension)) {}

        [[nodiscard]] bool isSubstitutionBlockedFor(const std::optional<unsigned int>& valueOfDimension, const std::optional<BitRangeAccessRestriction::BitRangeAccess>& bitRange, bool ignoreNotFullwidthBitRangeRestrictions = false, bool ignoreSmallerThanAccessedBitranges = false) const;

        void blockSubstitutionForBitRange(const std::optional<unsigned int>& valueOfDimensionToBlock, const BitRangeAccessRestriction::BitRangeAccess& bitRangeToBlock);
        void blockSubstitutionForDimension(const std::optional<unsigned int>& valueOfDimensionToBlock);

        void liftRestrictionForWholeDimension();
        void liftRestrictionForBitRange(const std::optional<unsigned int>& blockedValueOfDimension, const BitRangeAccessRestriction::BitRangeAccess& blockedBitRange);
        void liftRestrictionForValueOfDimension(unsigned int blockedValueOfDimension);

        [[nodiscard]] bool tryTrimAlreadyBlockedPartsFromRestriction(const std::optional<unsigned int>& accessedValueOfDimension, const std::optional<BitRangeAccessRestriction::BitRangeAccess>& accessedBitRange, BitRangeAccessRestriction& bitRangeAccess) const;

    private:
        bool                                                   isDimensionCompletelyBlocked;
        const SignalDimensionInformation&                      signalInformation;
        std::optional<BitRangeAccessRestriction::ptr>         dimensionBitRangeAccessRestriction;
        std::map<unsigned int, BitRangeAccessRestriction::ptr> perValueOfDimensionBitRangeAccessRestrictionLookup;
        const unsigned int numValuesForDimension;
    };
};

#endif