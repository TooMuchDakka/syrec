#include "core/syrec/parser/optimizations/constantPropagation/signal_value_lookup.hpp"

using namespace optimizations;

std::vector<DimensionPropagationBlocker::ptr> SignalValueLookup::initEmptyAccessRestrictionsForDimension(const unsigned int dimension) const {
    const unsigned int numValuesForDimension = dimension == 0 ? 1 : signalInformation.valuesPerDimension.at(dimension - 1);
    std::vector<DimensionPropagationBlocker::ptr> accessRestrictionsForDimension(numValuesForDimension);

    auto sharedDimensionBitRangeRestriction = std::optional<BitRangeAccessRestriction::ptr>(std::nullopt);
    auto sharedBlockingInformation          = DimensionPropagationBlocker::SharedBlockerInformation(signalInformation, sharedDimensionBitRangeRestriction);

    for (unsigned int valueOfDimension = 0; valueOfDimension < numValuesForDimension; ++valueOfDimension) {
        accessRestrictionsForDimension[valueOfDimension] = std::make_shared<DimensionPropagationBlocker>(DimensionPropagationBlocker(dimension, sharedBlockingInformation));
    }

    return accessRestrictionsForDimension;
}

std::vector<std::vector<DimensionPropagationBlocker::ptr>> SignalValueLookup::initAccessRestrictions() const {
    const auto                                                 numDimensions = signalInformation.valuesPerDimension.size();
    std::vector<std::vector<DimensionPropagationBlocker::ptr>> accessRestrictions(numDimensions);
    for (std::size_t dimension = 0; dimension < numDimensions; ++dimension) {
        accessRestrictions[dimension] = initEmptyAccessRestrictionsForDimension(dimension);
    }
    return accessRestrictions;
}

 std::optional<PotentialValueStorage> SignalValueLookup::initValueLookupWithDefaultValues(unsigned defaultValue) {
    const std::size_t numDimensions = signalInformation.valuesPerDimension.size();

    if (numDimensions == 0) {
        return std::nullopt;
    }

    const unsigned int valueDimension = numDimensions == 1 ? 0 : numDimensions - 1;
    return initValueLookupLayer(0, valueDimension, defaultValue);
}

PotentialValueStorage SignalValueLookup::initValueLookupLayer(const unsigned currDimension, const unsigned int valueDimension, const unsigned defaultValue) {
    const auto numEntriesForDimension = signalInformation.valuesPerDimension.at(currDimension);
    if (currDimension == valueDimension) {
        return PotentialValueStorage::initAsValueLookupLayer(numEntriesForDimension, defaultValue);
    }

    PotentialValueStorage intermediateLayerData = PotentialValueStorage::initAsIntermediateLayer(numEntriesForDimension);
    for (unsigned int valueOfDimension = 0; valueOfDimension < numEntriesForDimension; ++valueOfDimension) {
        std::get<PotentialValueStorage>(intermediateLayerData.layerData.at(valueOfDimension)) = initValueLookupLayer(currDimension + 1, valueDimension, defaultValue);
    }
    return intermediateLayerData;
}

