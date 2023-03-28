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

        explicit DimensionPropagationBlocker(const unsigned int dimension, const SharedBlockerInformation& sharedBlockerInformation):
            sharedBlockerInformation(sharedBlockerInformation), perValueOfDimensionBitRangeAccessRestrictionLookup({}) {
            const auto numValuesForDimension                   = sharedBlockerInformation.signalInformation.valuesPerDimension.at(dimension);
            for (unsigned int valueOfDimension = 0; valueOfDimension < numValuesForDimension; ++valueOfDimension) {
                perValueOfDimensionBitRangeAccessRestrictionLookup.insert(std::pair(valueOfDimension, std::nullopt));
            }
        }

        [[nodiscard]] bool isSubstitutionBlockedFor(const std::optional<unsigned int>& valueOfDimension, const std::optional<BitRangeAccessRestriction::BitRangeAccess>& bitRange) const;

        void blockSubstitutionForBitRange(const std::optional<unsigned int>& valueOfDimensionToBlock, const BitRangeAccessRestriction::BitRangeAccess& bitRangeToBlock);
        void blockSubstitutionForDimension(const std::optional<unsigned int>& valueOfDimensionToBlock);
        void liftRestrictionFor(const std::optional<unsigned int>& blockedValueOfDimension, const std::optional<BitRangeAccessRestriction::BitRangeAccess>& blockedBitRange);

    private:
        const SharedBlockerInformation& sharedBlockerInformation;
        std::map<unsigned int, std::optional<BitRangeAccessRestriction::ptr>> perValueOfDimensionBitRangeAccessRestrictionLookup;
    };
};

#endif