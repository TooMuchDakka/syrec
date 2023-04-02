#ifndef POTENTIAL_VALUE_STORAGE_HPP
#define POTENTIAL_VALUE_STORAGE_HPP
#pragma once

#include "core/syrec/parser/optimizations/constantPropagation/bit_range_access_restriction.hpp"

#include <variant>
#include <vector>
#include <optional>

namespace optimizations {
    struct PotentialValueStorage {
        enum LayerType {
            Intermediate,
            Value
        };

        const LayerType                                                               layerType;
        std::vector<std::variant<PotentialValueStorage, std::optional<unsigned int>>> layerData;

        explicit PotentialValueStorage() : layerType(Intermediate), layerData({}) {}
        explicit PotentialValueStorage(const LayerType type, std::vector<std::variant<PotentialValueStorage, std::optional<unsigned int>>> data) : layerType(type), layerData(std::move(data)) {}

        [[nodiscard]] static PotentialValueStorage initAsIntermediateLayer(unsigned int entries);
        [[nodiscard]] static PotentialValueStorage initAsValueLookupLayer(unsigned int numEntries, unsigned int defaultValue);
        
        void tryUpdateStoredValue(unsigned int layerDataEntryIdx, unsigned int newValue);
        void tryUpdatePartOfStoredValue(unsigned int layerDataEntryIdx, unsigned int newValue, const BitRangeAccessRestriction::BitRangeAccess& partsToUpdate);

        void                                      tryInvalidateStoredValue(const std::optional<unsigned int>& layerDataEntryIdx);
        [[nodiscard]] std::optional<unsigned int> tryFetchStoredValue(unsigned int layerDataEntryIdx, const std::optional<BitRangeAccessRestriction::BitRangeAccess>& bitRangeToFetch) const;

        void invalidateLayer();

    private:
        [[nodiscard]] static std::vector<std::variant<PotentialValueStorage, std::optional<unsigned int>>> createLayerData(std::size_t numEntries, unsigned int defaultValue);
        [[nodiscard]] static std::vector<std::variant<PotentialValueStorage, std::optional<unsigned int>>> createLayerData(std::size_t numEntries);

        [[nodiscard]] static unsigned int extractPartsOfValue(unsigned int value, const BitRangeAccessRestriction::BitRangeAccess& partToFetch);
    };
};
#endif