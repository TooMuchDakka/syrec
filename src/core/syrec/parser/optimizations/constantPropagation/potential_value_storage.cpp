#include "core/syrec/parser/optimizations/constantPropagation/potential_value_storage.hpp"

using namespace optimizations;

PotentialValueStorage PotentialValueStorage::initAsIntermediateLayer(unsigned numEntries) {
    PotentialValueStorage layer;
    layer.layerData             = std::vector<std::variant<PotentialValueStorage, std::optional<unsigned int>>>(numEntries, PotentialValueStorage());
    return layer;
}

PotentialValueStorage PotentialValueStorage::initAsValueLookupLayer(unsigned numEntries, unsigned defaultValue) {
    PotentialValueStorage layer;
    layer.layerData             = std::vector<std::variant<PotentialValueStorage, std::optional<unsigned int>>>(numEntries, defaultValue);
    return layer;
}

