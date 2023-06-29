#include "core/syrec/parser/optimizations/deadStoreElimination/dead_store_status_lookup.hpp"

using namespace deadStoreElimination;

std::shared_ptr<valueLookup::BaseValueLookup<bool>> DeadStoreStatusLookup::clone() {
    const auto& signalInformation = getSignalInformation();
    auto        copy              = std::make_shared<DeadStoreStatusLookup>(signalInformation.bitWidth, signalInformation.valuesPerDimension, 0);
    auto        dimensionAccess   = std::vector<std::optional<unsigned int>>(signalInformation.valuesPerDimension.size(), std::nullopt);

    copy->copyRestrictionsAndUnrestrictedValuesFrom(
            {},
            std::nullopt,
            {},
            std::nullopt,
            *this);

    return copy;
}

bool DeadStoreStatusLookup::areAccessedSignalPartsDead(const std::vector<std::optional<unsigned>>& accessedDimensions, const std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess>& accessedBitRange) const {
    return isValueLookupBlockedFor(accessedDimensions, accessedBitRange);
}


bool DeadStoreStatusLookup::canStoreValue(const std::any& value, const optimizations::BitRangeAccessRestriction::BitRangeAccess& availableStorageSpace) const {
    return false;
}

std::any DeadStoreStatusLookup::extractPartsOfValue(const std::any& value, const optimizations::BitRangeAccessRestriction::BitRangeAccess& availableStorageSpace) const {
    return false;
}

std::any DeadStoreStatusLookup::transformExistingValueByMergingWithNewOne(const std::any& currentValue, const std::any& newValue, const optimizations::BitRangeAccessRestriction::BitRangeAccess& partsToUpdate) const {
    return false;
}

std::any DeadStoreStatusLookup::wrapValueOnOverflow(const std::any& value, unsigned numBitsOfStorage) const {
    return false;
}