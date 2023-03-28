#ifndef SIGNAL_VALUE_LOOKUP_HPP
#define SIGNAL_VALUE_LOOKUP_HPP
#pragma once

#include "core/syrec/parser/optimizations/constantPropagation/potential_value_storage.hpp"
#include "core/syrec/parser/optimizations/constantPropagation/dimension_propagation_blocker.hpp"
#include "core/syrec/parser/optimizations/constantPropagation/shared_dimension_blocking_information.hpp"

#include <memory>
#include <vector>

namespace optimizations {
    class SignalValueLookup {
    public:

        typedef std::shared_ptr<SignalValueLookup> ptr;

        explicit SignalValueLookup(unsigned int signalBitWidth, const std::vector<unsigned int>& signalDimensions, unsigned int defaultValue):
            signalInformation(SignalDimensionInformation(signalBitWidth, signalDimensions)){
            valueLookup = initValueLookupWithDefaultValues(defaultValue);
            dimensionAccessRestrictions = initAccessRestrictions();
        }

        void invalidateStoredValueFor(const std::vector<std::optional<unsigned int>>& accessedDimensions);
        void invalidateStoredValueForBitrange(const std::vector<std::optional<unsigned int>>& accessedDimensions, const BitRangeAccessRestriction::BitRangeAccess& bitRange);
        void invalidateStoredValueForSignal();

        /*void liftRestrictionsFromValueOfDimension(const std::vector<std::optional<unsigned int>>& accessedDimensions);
        void liftRestrictionFromBitRange(const std::vector<std::optional<unsigned int>>& accessedDimensions, const BitRangeAccessRestriction::BitRangeAccess& bitRange);
        void liftRestrictionsForWholeSignal();*/

        [[nodiscard]] std::optional<unsigned int> tryFetchValueFor(const std::vector<std::optional<unsigned int>>& accessedDimensions, const std::optional<BitRangeAccessRestriction::BitRangeAccess>& bitRange) const;

        void updateStoredValueFor(const std::vector<std::optional<unsigned int>>& accessedDimensions, const BitRangeAccessRestriction::BitRangeAccess& bitRange, unsigned int newValue);

    private:
        const SignalDimensionInformation signalInformation;

        /*
         * a[2][4][3]
         *
         * For this signal we create the following empty restrictions during initialization (assuming the created dimension restriction is only a pointer to a map that contains the actual information for the dimension)
         * I.  For the first dimension we only need one dimension restriction (the latter is a lookup map with 2 entries)
         * II. For the second dimension we need 2 dimension restrictions since the subsequent dimensions of the signal are independent from each other when accessing any value for the first dimension
         *      (i.e. when accessing a[0] the resulting signal would be of dimension [4][3] and independent from the [4][3] dimensional signal of a[1]) [each created dimension restriction consists of a lookup map of 4 entries)
         * III. The third dimension requires 4 dimension restrictions
         */
        std::vector<std::vector<DimensionPropagationBlocker::ptr>> dimensionAccessRestrictions;
        std::optional<PotentialValueStorage>                               valueLookup;

        std::optional<PotentialValueStorage> initValueLookupWithDefaultValues(unsigned int defaultValue);
        PotentialValueStorage initValueLookupLayer(unsigned int currDimension, unsigned int valueDimension, unsigned int defaultValue);

        [[nodiscard]] std::vector<std::vector<DimensionPropagationBlocker::ptr>> initAccessRestrictions() const;
        [[nodiscard]] std::vector<DimensionPropagationBlocker::ptr>              initEmptyAccessRestrictionsForDimension(unsigned int dimension) const;
        [[nodiscard]] bool isValueStorableInBitrange(const BitRangeAccessRestriction::BitRangeAccess& availableStorageSpace, unsigned int value);
    };
}; // namespace optimizations

#endif