#ifndef POTENTIAL_VALUE_STORAGE_HPP
#define POTENTIAL_VALUE_STORAGE_HPP
#pragma once

#include <variant>
#include <vector>
#include <optional>

namespace optimizations {
    struct PotentialValueStorage {
        std::vector<std::variant<PotentialValueStorage, std::optional<unsigned int>>> layerData;

        void tryInvalidateStoredValue();
        [[nodiscard]] static PotentialValueStorage initAsIntermediateLayer(unsigned int entries);
        [[nodiscard]] static PotentialValueStorage initAsValueLookupLayer(unsigned int numEntries, unsigned int defaultValue);

        void invalidateCurrentAndSubsequentLayers();
    };
};
#endif