#include "core/syrec/parser/signal_access_restriction.hpp"

using namespace parser;

bool SignalAccessRestriction::isAccessRestrictedToBitRangeGlobally(const SignalAccess& bitRange) const {
    return isBitRangeWithinRange(bitRange) && (isAccessRestrictedOnWholeSignal() || (globalSignalRestriction.has_value() && globalSignalRestriction->isAccessRestricted(bitRange)));
}

void SignalAccessRestriction::clearAllRestrictions() {
    this->isBlockedCompletely = false;
    this->dimensionRestrictions.clear();
}

void SignalAccessRestriction::blockAccessOnSignalCompletely() {
    this->isBlockedCompletely = true;
    this->dimensionRestrictions.clear();
}

bool SignalAccessRestriction::isAccessRestrictedOnWholeSignal() const {
    return isBlockedCompletely;
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
    if (!isBitRangeWithinRange(bitRange)) {
        return false;
    }

    if (isAccessRestrictedToBitRangeGlobally(bitRange)) {
        return true;
    }
    const auto& iterableDimension = createIndexSequenceExcludingEnd(0, signal->dimensions.size());
    return std::any_of(
         iterableDimension.cbegin(),
         iterableDimension.cend(),
         [&](const auto& dimension) { return isAccessRestrictedToBitRange(dimension, bitRange); }
    );
}

bool SignalAccessRestriction::isAccessRestrictedToBitRange(const std::size_t& dimension, const SignalAccess& bitRange, const bool checkAllValuesForDimension) const {
    if (!isDimensionWithinRange(dimension) || !isBitRangeWithinRange(bitRange)) {
        return false;
    }

    if (isAccessRestrictedToBitRangeGlobally(bitRange)) {
        return true;
    }

    if (dimensionRestrictions.count(dimension) == 0) {
        return false;
    }

    if (dimensionRestrictions.at(dimension).areAllValuesForDimensionBlocked || (dimensionRestrictions.at(dimension).dimensionSignalRestriction.has_value() && dimensionRestrictions.at(dimension).dimensionSignalRestriction->isAccessRestricted(bitRange))) {
        return true;
    }

    if (checkAllValuesForDimension) {
        const auto& valuesForDimension = createIndexSequenceExcludingEnd(0, signal->dimensions.at(dimension));
        return std::any_of(
                valuesForDimension.cbegin(),
                valuesForDimension.cend(),
                [&](const auto& valueForDimension) {
                    return isAccessRestrictedToBitRange(dimension, valueForDimension, bitRange);
                });   
    }
    return false;
}

bool SignalAccessRestriction::isAccessRestrictedToBitRange(const std::size_t dimension, const SignalAccess& bitRange) const {
    return isAccessRestrictedToBitRange(dimension, bitRange, true);
}

bool SignalAccessRestriction::isAccessRestrictedToBitRange(const std::size_t dimension, const std::size_t valueForDimension, const SignalAccess& bitRange) const {
    if (!isValueForDimensionWithinRange(dimension, valueForDimension) || !isBitRangeWithinRange(bitRange)) {
        return false;
    }

    return isAccessRestrictedToBitRange(dimension, bitRange, false)
        || (dimensionRestrictions.count(dimension) != 0 
                && (dimensionRestrictions.at(dimension).restrictionsPerValueOfDimension.count(valueForDimension) != 0 
                    && dimensionRestrictions.at(dimension).restrictionsPerValueOfDimension.at(valueForDimension).isAccessRestricted(bitRange)));
}

bool SignalAccessRestriction::isAccessRestrictedToValueOfDimension(const std::size_t dimension, const std::size_t valueForDimension) const {
    return isValueForDimensionWithinRange(dimension, valueForDimension)
        && (isAccessRestrictedToBitRangeGlobally(SignalAccess(0, signal->bitwidth-1))
            || isAccessRestrictedToBitRange(dimension, SignalAccess(0, signal->bitwidth-1), false)
            || (dimensionRestrictions.count(dimension) != 0 && dimensionRestrictions.at(dimension).restrictionsPerValueOfDimension.count(valueForDimension) != 0));
}

bool SignalAccessRestriction::isAccessRestrictedToDimension(const std::size_t dimension) const {
    return isDimensionWithinRange(dimension) && (isAccessRestrictedToBitRangeGlobally(SignalAccess(0, signal->bitwidth-1)) || dimensionRestrictions.count(dimension) != 0);
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
    if (!isBitRangeWithinRange(bitRange) || isAccessRestrictedOnWholeSignal() || !globalSignalRestriction.has_value()) {
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
        || isAccessRestrictedOnWholeSignal()
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

    if (!globalSignalRestriction->isAccessRestricted(bitRange)) {
        globalSignalRestriction->restrictAccessTo(bitRange);   
    }
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

    if (!existingDimensionRestriction.dimensionSignalRestriction->isAccessRestricted(bitRange)) {
        existingDimensionRestriction.dimensionSignalRestriction->restrictAccessTo(bitRange);   
    }
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
    if (!existingDimensionValueRestriction.isAccessRestricted(bitRange)) {
        existingDimensionValueRestriction.restrictAccessTo(bitRange);
    }
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
        SignalRestriction restrictedRegion = SignalRestriction(signal->bitwidth);
        existingDimensionRestriction.restrictionsPerValueOfDimension.insert({valueForDimension, restrictedRegion});
    }

    auto& existingDimensionValueRestriction = existingDimensionRestriction.restrictionsPerValueOfDimension.at(valueForDimension);
    const auto& bitRangeToCheck                   = SignalAccess(0, signal->bitwidth - 1);
    if (!existingDimensionValueRestriction.isAccessRestricted(bitRangeToCheck)) {
        existingDimensionValueRestriction.restrictAccessTo(bitRangeToCheck);
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