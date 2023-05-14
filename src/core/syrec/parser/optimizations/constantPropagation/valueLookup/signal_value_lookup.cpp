#include "core/syrec/parser/optimizations/constantPropagation/valueLookup/signal_value_lookup.hpp"
#include "core/syrec/parser/utils/bit_helpers.hpp"

using namespace valueLookup;

std::shared_ptr<BaseValueLookup<unsigned>> SignalValueLookup::clone() {
    const auto& signalInformation = getSignalInformation();
    auto        copy              = std::make_shared<SignalValueLookup>(signalInformation.bitWidth, signalInformation.valuesPerDimension, 0);
    auto        dimensionAccess   = std::vector<std::optional<unsigned int>>(signalInformation.valuesPerDimension.size(), std::nullopt);

    copy->applyToBitsOfLastLayer(
    {},
    std::nullopt,
    [&dimensionAccess, &copy, this](const std::vector<unsigned int>& relativeDimensionAccess, unsigned int relativeBitIdx) {
        copyRelativeDimensionAccess(0, dimensionAccess, relativeDimensionAccess);
        const auto& accessedBitRange(std::pair(relativeBitIdx, relativeBitIdx));
        const auto& valueForBit  = tryFetchValueFor(dimensionAccess, accessedBitRange);

        if (valueForBit.has_value()) {
            copy->updateStoredValueFor(dimensionAccess, accessedBitRange, *valueForBit);
        } else {
            copy->invalidateStoredValueForBitrange(dimensionAccess, accessedBitRange);
        }
    });
    return copy;
}

bool SignalValueLookup::canStoreValue(const std::any& value, const optimizations::BitRangeAccessRestriction::BitRangeAccess& availableStorageSpace) const {
    return BitHelpers::isValueStorableInBitRange(std::any_cast<unsigned int>(value), availableStorageSpace);
}

std::any SignalValueLookup::extractPartsOfValue(const std::any& value, const optimizations::BitRangeAccessRestriction::BitRangeAccess& availableStorageSpace) const {
    return BitHelpers::extractBitsFromValue(std::any_cast<unsigned int>(value), availableStorageSpace);
}

std::any SignalValueLookup::transformExistingValueByMergingWithNewOne(const std::any& currentValue, const std::any& newValue, const optimizations::BitRangeAccessRestriction::BitRangeAccess& partsToUpdate) const {
    return std::any_cast<unsigned int>(
            BitHelpers::mergeValues(
                    std::any_cast<unsigned int>(currentValue),
                    std::any_cast<unsigned int>(newValue),
                    partsToUpdate));
}

// TODO: CONSTANT_PROPAGATION: Find a better way of calculating remaining value after wrap around
std::any SignalValueLookup::wrapValueOnOverflow(const std::any& value, const unsigned numBitsOfStorage) const {
    const auto valueToCheck                        = std::any_cast<unsigned int>(value);
    const auto maximumValueStorableInBitsOfStorage = BitHelpers::getMaximumStorableValueInBitRange(numBitsOfStorage);
    if (valueToCheck <= maximumValueStorableInBitsOfStorage) {
        return value;
    }

    /*
     * We need to add 1 to the maximum storeable value in the given bit range to account for the fact that MAX + 1 = 0
     */ 
    return valueToCheck % (maximumValueStorableInBitsOfStorage + 1);
}