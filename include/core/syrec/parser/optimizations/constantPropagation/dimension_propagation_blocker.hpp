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

        struct SharedBlockerInformation {
            bool                                           isDimensionCompletelyBlocked;
            const SignalDimensionInformation& signalInformation;
            std::optional<BitRangeAccessRestriction::ptr>& sharedDimensionBitRangeAccessRestriction;

            explicit SharedBlockerInformation(const SignalDimensionInformation& sharedSignalInformation, std::optional<BitRangeAccessRestriction::ptr>& sharedDimensionBitRangeAccessRestriction):
                isDimensionCompletelyBlocked(false), signalInformation(sharedSignalInformation), sharedDimensionBitRangeAccessRestriction(sharedDimensionBitRangeAccessRestriction) {}
        };

        explicit DimensionPropagationBlocker(const unsigned int dimension, SharedBlockerInformation& sharedBlockerInformation):
            sharedBlockerInformation(sharedBlockerInformation), perValueOfDimensionBitRangeAccessRestrictionLookup({}), numValuesForDimension(sharedBlockerInformation.signalInformation.valuesPerDimension.at(dimension)) {}

        [[nodiscard]] bool isSubstitutionBlockedFor(const std::optional<unsigned int>& valueOfDimension, const std::optional<BitRangeAccessRestriction::BitRangeAccess>& bitRange) const;

        void blockSubstitutionForBitRange(const std::optional<unsigned int>& valueOfDimensionToBlock, const BitRangeAccessRestriction::BitRangeAccess& bitRangeToBlock);
        void blockSubstitutionForDimension(const std::optional<unsigned int>& valueOfDimensionToBlock);

        void liftRestrictionForWholeDimension();
        void liftRestrictionForBitRange(const std::optional<unsigned int>& blockedValueOfDimension, const BitRangeAccessRestriction::BitRangeAccess& blockedBitRange);
        void liftRestrictionForValueOfDimension(unsigned int blockedValueOfDimension);

    private:
        SharedBlockerInformation& sharedBlockerInformation;
        std::map<unsigned int, BitRangeAccessRestriction::ptr> perValueOfDimensionBitRangeAccessRestrictionLookup;
        const unsigned int numValuesForDimension;
    };
};

#endif