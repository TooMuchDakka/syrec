#include "core/syrec/parser/signal_access_restriction.hpp"

using namespace parser;

void SignalAccessRestriction::clearAllRestrictions() {
    this->isBlockedCompletely = false;
    this->dimensionRestrictions.clear();
}

void SignalAccessRestriction::blockAccessOnSignalCompletely() {
    this->isBlockedCompletely = true;
    this->dimensionRestrictions.clear();
}

bool SignalAccessRestriction::isAccessRestrictedToBit(const std::size_t bitPosition) const {
    return isAccessRestrictedToBitRange(SignalAccess(bitPosition, bitPosition));
}

bool SignalAccessRestriction::isAccessRestrictedToBit(const std::size_t dimension, const std::size_t bitPosition) const {
    return isAccessRestrictedToBitRange(dimension, SignalAccess(bitPosition, bitPosition));
}

bool SignalAccessRestriction::isAccessRestrictedToBit(const std::size_t dimension, const std::size_t valueForDimension, const std::size_t bitPosition) const {
    return isAccessRestrictedToBitRange(dimension, valueForDimension, SignalAccess(bitPosition, bitPosition));
}

bool SignalAccessRestriction::isAccessRestrictedToBitRange(const SignalAccess& bitRange) const {
    return isBitRangeWithinRange(bitRange)
        && (isBlockedCompletely || (globalSignalRestriction.has_value() && globalSignalRestriction->isAccessRestricted(bitRange)));
}

bool SignalAccessRestriction::isAccessRestrictedToBitRange(const std::size_t dimension, const SignalAccess& bitRange) const {
    return isDimensionWithinRange(dimension)
        && isBitRangeWithinRange(bitRange)
        && (isAccessRestrictedToBitRange(bitRange)
            || (dimensionRestrictions.count(dimension) != 0
                && (dimensionRestrictions.at(dimension).areAllValuesForDimensionBlocked 
                    || (dimensionRestrictions.at(dimension).dimensionSignalRestriction.has_value() && dimensionRestrictions.at(dimension).dimensionSignalRestriction->isAccessRestricted(bitRange)))));
}

bool SignalAccessRestriction::isAccessRestrictedToBitRange(const std::size_t dimension, const std::size_t valueForDimension, const SignalAccess& bitRange) const {
    return isValueForDimensionWithinRange(dimension, valueForDimension)
        && isBitRangeWithinRange(bitRange)
        && (isAccessRestrictedToBitRange(dimension, bitRange) 
            || (dimensionRestrictions.count(dimension) != 0 
                && (dimensionRestrictions.at(dimension).areAllValuesForDimensionBlocked 
                    || (dimensionRestrictions.at(dimension).dimensionSignalRestriction.has_value() && dimensionRestrictions.at(dimension).dimensionSignalRestriction->isAccessRestricted(bitRange))
                    || (dimensionRestrictions.at(dimension).restrictionsPerValueOfDimension.count(valueForDimension) != 0
                    && dimensionRestrictions.at(dimension).restrictionsPerValueOfDimension.at(valueForDimension).isAccessRestricted(bitRange)))));
}

bool SignalAccessRestriction::isAccessRestrictedToValueOfDimension(const std::size_t dimension, const std::size_t valueForDimension) const {
    return isDimensionWithinRange(dimension)
        && (isBlockedCompletely 
            || (dimensionRestrictions.count(dimension) != 0
                && (dimensionRestrictions.at(dimension).areAllValuesForDimensionBlocked
                || dimensionRestrictions.at(dimension).restrictionsPerValueOfDimension.count(valueForDimension) != 0)));
}

bool SignalAccessRestriction::isAccessRestrictedToDimension(const std::size_t dimension) const {
    return isDimensionWithinRange(dimension) && (isBlockedCompletely || dimensionRestrictions.count(dimension) != 0);
}

void SignalAccessRestriction::liftAccessRestrictionsForBit(const std::size_t bitPosition) {
    return liftAccessRestrictionForBitRange(SignalAccess(bitPosition, bitPosition));
}

void SignalAccessRestriction::liftAccessRestrictionsForBit(const std::size_t dimension, const std::size_t bitPosition) {
    return liftAccessRestrictionForBitRange(dimension, SignalAccess(bitPosition, bitPosition));
}

void SignalAccessRestriction::liftAccessRestrictionsForBit(const std::size_t dimension, const std::size_t valueForDimension, const std::size_t bitPosition) {
    return liftAccessRestrictionForBitRange(dimension, valueForDimension, SignalAccess(bitPosition, bitPosition));
}

void SignalAccessRestriction::liftAccessRestrictionForBitRange(const SignalAccess& bitRange) {
    if (!isBitRangeWithinRange(bitRange) || isBlockedCompletely || !globalSignalRestriction.has_value()) {
        return;
    }

    globalSignalRestriction->removeRestrictionFor(bitRange);
}

void SignalAccessRestriction::liftAccessRestrictionForBitRange(const std::size_t dimension, const SignalAccess& bitRange) {
    if (!isBitRangeWithinRange(bitRange) || !isDimensionWithinRange(dimension) || isAccessRestrictedToBitRange(bitRange)
        || dimensionRestrictions.count(dimension) == 0
        || !dimensionRestrictions.at(dimension).dimensionSignalRestriction.has_value()) {
        return;
    }

    dimensionRestrictions.at(dimension).dimensionSignalRestriction->removeRestrictionFor(bitRange);
}

void SignalAccessRestriction::liftAccessRestrictionForBitRange(const std::size_t dimension, const std::size_t valueForDimension, const SignalAccess& bitRange) {
    if (!isBitRangeWithinRange(bitRange) || isValueForDimensionWithinRange(dimension, valueForDimension)
        || isAccessRestrictedToBitRange(dimension, bitRange)
        || dimensionRestrictions.count(dimension) == 0
        || dimensionRestrictions.at(dimension).restrictionsPerValueOfDimension.count(valueForDimension) == 0) {
        return;
    }

    dimensionRestrictions.at(dimension).restrictionsPerValueOfDimension.at(valueForDimension).removeRestrictionFor(bitRange);
}

void SignalAccessRestriction::liftAccessRestrictionForDimension(const std::size_t dimension) {
    if (!isDimensionWithinRange(dimension)
        || isBlockedCompletely
        || dimensionRestrictions.count(dimension) == 0) {
        return;
    }

    dimensionRestrictions.erase(dimension);
}

void SignalAccessRestriction::liftAccessRestrictionForValueOfDimension(const std::size_t dimension, const std::size_t valueForDimension) {
    if (!isValueForDimensionWithinRange(dimension, valueForDimension)
        || isAccessRestrictedToDimension(dimension) 
        || dimensionRestrictions.count(dimension) == 0
        || dimensionRestrictions.at(dimension).areAllValuesForDimensionBlocked
        || dimensionRestrictions.at(dimension).restrictionsPerValueOfDimension.count(valueForDimension) == 0) {
        return;
    }

    dimensionRestrictions.at(dimension).restrictionsPerValueOfDimension.erase(valueForDimension);
}

bool SignalAccessRestriction::restrictAccessToBit(const std::size_t bitPosition) {
    return restrictAccessToBitRange(SignalAccess(bitPosition, bitPosition));
}

bool SignalAccessRestriction::restrictAccessToBit(const std::size_t dimension, const std::size_t bitPosition) {
    return restrictAccessToBitRange(dimension, SignalAccess(bitPosition, bitPosition));
}

bool SignalAccessRestriction::restrictAccessToBit(const std::size_t dimension, const std::size_t valueForDimension, const std::size_t bitPosition) {
    return restrictAccessToBitRange(dimension, valueForDimension, SignalAccess(bitPosition, bitPosition));
}

bool SignalAccessRestriction::restrictAccessToBitRange(const SignalAccess& bitRange) {
    if (!isBitRangeWithinRange(bitRange)) {
        return false;
    }

    if (isAccessRestrictedToBitRange(bitRange)) {
        return true;
    }

    if (!globalSignalRestriction.has_value()) {
        globalSignalRestriction.emplace(SignalRestriction(signal->bitwidth));
    }
    globalSignalRestriction->restrictAccessTo(bitRange);
    return true;
}

bool SignalAccessRestriction::restrictAccessToBitRange(const std::size_t dimension, const SignalAccess& bitRange) {
    if (!isDimensionWithinRange(dimension) || !isBitRangeWithinRange(bitRange)) {
        return false;
    }

    if (isAccessRestrictedToBitRange(bitRange) || isAccessRestrictedToDimension(dimension)) {
        return true;
    }

    if (dimensionRestrictions.count(dimension) == 0) {
        dimensionRestrictions.insert({dimension, createDimensionRestriction(dimension)});
    }

    auto& existingDimensionRestriction = dimensionRestrictions.at(dimension);
    if (existingDimensionRestriction.areAllValuesForDimensionBlocked) {
        return true;
    }

    if (!existingDimensionRestriction.dimensionSignalRestriction.has_value()){
        existingDimensionRestriction.dimensionSignalRestriction.emplace(signal->bitwidth);
    }

    existingDimensionRestriction.dimensionSignalRestriction->restrictAccessTo(bitRange);
    return true;
}

bool SignalAccessRestriction::restrictAccessToBitRange(const std::size_t dimension, const std::size_t valueForDimension, const SignalAccess& bitRange) {
    if (!isValueForDimensionWithinRange(dimension, valueForDimension) || !isBitRangeWithinRange(bitRange)) {
        return false;
    }

    if (isAccessRestrictedToBitRange(bitRange) || isAccessRestrictedToValueOfDimension(dimension, valueForDimension)) {
        return true;
    }

    if (dimensionRestrictions.count(dimension) == 0) {
        dimensionRestrictions.insert({dimension, createDimensionRestriction(dimension)});
    }

    auto& existingDimensionRestriction = dimensionRestrictions.at(dimension);
    if (existingDimensionRestriction.dimensionSignalRestriction.has_value() && existingDimensionRestriction.dimensionSignalRestriction->isAccessRestricted(bitRange)) {
        return true;
    }

    if (existingDimensionRestriction.restrictionsPerValueOfDimension.count(valueForDimension) == 0) {
        existingDimensionRestriction.restrictionsPerValueOfDimension.insert({valueForDimension, SignalRestriction(signal->bitwidth)});
    }
    
    auto& existingDimensionValueRestriction = existingDimensionRestriction.restrictionsPerValueOfDimension.at(valueForDimension);
    if (existingDimensionValueRestriction.isAccessRestricted(bitRange)) {
        return true;
    }
    existingDimensionValueRestriction.restrictAccessTo(bitRange);
    return true;
}

bool SignalAccessRestriction::restrictAccessToValueOfDimension(const std::size_t dimension, const std::size_t valueForDimension) {
    if (!isValueForDimensionWithinRange(dimension, valueForDimension)) {
        return false;
    }

    if (isAccessRestrictedToValueOfDimension(dimension, valueForDimension)) {
        return true;
    }

    if (dimensionRestrictions.count(dimension) == 0) {
        dimensionRestrictions.insert({dimension, createDimensionRestriction(dimension)});
    }
    auto& existingDimensionRestriction = dimensionRestrictions.at(dimension);
    if (existingDimensionRestriction.restrictionsPerValueOfDimension.count(valueForDimension) == 0) {
        existingDimensionRestriction.restrictionsPerValueOfDimension.insert({valueForDimension, SignalRestriction(signal->bitwidth)});
    }
    return true;
}

bool SignalAccessRestriction::restrictAccessToDimension(const std::size_t dimension) {
    if (!isDimensionWithinRange(dimension)) {
        return false;
    }

    if (isAccessRestrictedToDimension(dimension)) {
        return true;
    }

    if (dimensionRestrictions.count(dimension) == 0) {
        dimensionRestrictions.insert({dimension, createDimensionRestriction(dimension)});
    }
    dimensionRestrictions.at(dimension).areAllValuesForDimensionBlocked = true;
    return true;
}

bool SignalAccessRestriction::isDimensionWithinRange(std::size_t dimension) const {
    return dimension < this->signal->dimensions.size();
}

bool SignalAccessRestriction::isValueForDimensionWithinRange(std::size_t dimension, std::size_t valueForDimension) const {
    return isDimensionWithinRange(dimension) && valueForDimension < signal->dimensions.at(dimension);
}

bool SignalAccessRestriction::isBitWithinRange(std::size_t bitPosition) const {
    return bitPosition < signal->bitwidth;
}

 bool SignalAccessRestriction::isBitRangeWithinRange(const SignalAccess& bitRange) const {
    return isBitWithinRange(bitRange.start) && isBitWithinRange(bitRange.stop);
}