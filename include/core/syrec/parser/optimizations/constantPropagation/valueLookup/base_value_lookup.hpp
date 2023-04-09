#ifndef ABSTRACT_VALUE_LOOKUP_HPP
#define ABSTRACT_VALUE_LOOKUP_HPP
#pragma once

#include "core/syrec/parser/optimizations/constantPropagation/bit_range_access_restriction.hpp"
#include "core/syrec/parser/optimizations/constantPropagation/dimension_propagation_blocker.hpp"
#include "core/syrec/parser/optimizations/constantPropagation/shared_dimension_blocking_information.hpp"
#include "core/syrec/parser/optimizations/constantPropagation/valueLookup/layer_data.hpp"

#include <any>
#include <optional>
#include <vector>

namespace valueLookup {
    template<typename Vt>
    class BaseValueLookup {
    public:
        typedef std::shared_ptr<BaseValueLookup> ptr;

        explicit BaseValueLookup(const unsigned signalBitwidth, const std::vector<unsigned int>& signalDimensions, const std::optional<Vt>& defaultValue):
            signalInformation(signalBitwidth, signalDimensions), dimensionAccessRestrictions(*initializeDimensionAccessRestrictionLayer(0)), valueLookup(*initializeValueLookupLayer(0, defaultValue)) {}

        virtual ~BaseValueLookup() = 0;

        void invalidateStoredValueFor(const std::vector<std::optional<unsigned int>>& accessedDimensions) const;
        void invalidateStoredValueForBitrange(const std::vector<std::optional<unsigned int>>& accessedDimensions, const optimizations::BitRangeAccessRestriction::BitRangeAccess& bitRange) const;
        void invalidateAllStoredValuesForSignal() const;

        void liftRestrictionsFromWholeSignal() const;
        void liftRestrictionsOfDimensions(const std::vector<std::optional<unsigned int>>& accessedDimensions, const std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess>& bitRange);

        [[nodiscard]] std::optional<Vt> tryFetchValueFor(const std::vector<std::optional<unsigned int>>& accessedDimensions, const std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess>& bitRange) const;
        void                            updateStoredValueFor(const std::vector<std::optional<unsigned int>>& accessedDimensions, const std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess>& bitRange, const Vt& newValue) const;

    protected:
        const optimizations::SignalDimensionInformation signalInformation;
        const LayerData<optimizations::DimensionPropagationBlocker::ptr>::ptr dimensionAccessRestrictions;
        const std::shared_ptr<LayerData<std::map<unsigned int, std::optional<Vt>>>> valueLookup;

        [[nodiscard]] std::optional<LayerData<optimizations::DimensionPropagationBlocker::ptr>::ptr> initializeDimensionAccessRestrictionLayer(const unsigned int dimension);
        [[nodiscard]] std::optional<std::shared_ptr<LayerData<std::map<unsigned int, std::optional<Vt>>>>> initializeValueLookupLayer(const unsigned int dimension, const std::optional<Vt>& defaultValue);

        [[nodiscard]] std::optional<std::vector<unsigned int>> transformAccessOnDimensions(const std::vector<std::optional<unsigned int>>& accessedDimensions) const;
        [[nodiscard]] bool isValueLookupBlockedFor(const std::vector<std::optional<unsigned int>>& accessedDimensions, const std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess>& bitRange) const;

        [[nodiscard]] virtual bool canStoreValue(const std::any& value, const optimizations::BitRangeAccessRestriction::BitRangeAccess& availableStorageSpace) const = 0;
        [[nodiscard]] virtual std::any extractPartsOfValue(const std::any& value, const optimizations::BitRangeAccessRestriction::BitRangeAccess& availableStorageSpace) const = 0;
        [[nodiscard]] virtual std::any transformExistingValueByMergingWithNewOne(const std::any& currentValue, const std::any& newValue, const optimizations::BitRangeAccessRestriction::BitRangeAccess& partsToUpdate) const = 0;
    };

    /*
     * Since we would also like the split into definition/implementation for this templated class,
     * we will use a .tpp file to provide the implementation for the latter:
     * https://stackoverflow.com/questions/44774036/why-use-a-tpp-file-when-implementing-templated-functions-and-classes-defined-i
     *
     * Otherwise the implementation of the template must be provided in the header file to be
     * available for all compilation units:
     * https://stackoverflow.com/questions/495021/why-can-templates-only-be-implemented-in-the-header-file
     */
    #include "core/syrec/parser/optimizations/constantPropagation/valueLookup/base_value_lookup.tpp"
}; // namespace valueLookup
#endif