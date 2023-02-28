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

    if (isAccessOnDimensionBlocked(dimension) || (dimensionRestrictions.at(dimension).dimensionSignalRestriction.has_value() && dimensionRestrictions.at(dimension).dimensionSignalRestriction->isAccessRestricted(bitRange))) {
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
        || isAccessOnValueOfDimensionBlocked(dimension, valueForDimension, false)
        || (dimensionRestrictions.count(dimension) != 0 
                && (dimensionRestrictions.at(dimension).restrictionsPerValueOfDimension.count(valueForDimension) != 0 
                    && dimensionRestrictions.at(dimension).restrictionsPerValueOfDimension.at(valueForDimension).isAccessRestricted(bitRange)));
}

bool SignalAccessRestriction::isAccessRestrictedToValueOfDimension(const std::size_t dimension, const std::size_t valueForDimension) const {
    return isAccessOnValueOfDimensionBlocked(dimension, valueForDimension, true);
}

bool SignalAccessRestriction::isAccessRestrictedToDimension(const std::size_t dimension) const {
    return isAccessOnDimensionBlocked(dimension, true);
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

    if (globalSignalRestriction.has_value()) {
        if (globalSignalRestriction->isCompletelyBlocked()) {
            return true;
        }
    } else {
        globalSignalRestriction.emplace(SignalRestriction(signal->bitwidth));
    }
    
    globalSignalRestriction->restrictAccessTo(bitRange);
    updateExistingBitRangeRestrictions(bitRange);
    return true;
}

bool SignalAccessRestriction::restrictAccessToBitRange(const std::size_t dimension, const SignalAccess& bitRange) {
    if (!isDimensionWithinRange(dimension) || !isBitRangeWithinRange(bitRange)) {
        return false;
    }

    // We do not check if access to the bit range restricted globally, for the dimension because we could otherwise not extend any existing restriction
    if (isAccessOnDimensionBlocked(dimension)) {
        return true;
    }

    checkAndCreateDimensionRestriction(dimension);
    auto& existingDimensionRestriction = dimensionRestrictions.at(dimension);
    // TODO: Remove ?
    //if (existingDimensionRestriction.areAllValuesForDimensionBlocked || (existingDimensionRestriction.dimensionSignalRestriction.has_value() && existingDimensionRestriction.dimensionSignalRestriction->isAccessRestricted(bitRange))) {
    if (existingDimensionRestriction.areAllValuesForDimensionBlocked){
        return true;
    }

    if (!existingDimensionRestriction.dimensionSignalRestriction.has_value()){
        existingDimensionRestriction.dimensionSignalRestriction.emplace(signal->bitwidth);
    }

    existingDimensionRestriction.dimensionSignalRestriction->restrictAccessTo(bitRange);   
    updateExistingBitRangeRestrictions(bitRange, dimension);
    return true;
}

bool SignalAccessRestriction::restrictAccessToBitRange(const std::size_t dimension, const std::size_t valueForDimension, const SignalAccess& bitRange) {
    if (!isValueForDimensionWithinRange(dimension, valueForDimension) || !isBitRangeWithinRange(bitRange)) {
        return false;
    }

    // We do not check if access to the bit range restricted globally, for the dimension or the value for the dimension because we could otherwise not extend any existing restriction
    if (isAccessOnValueOfDimensionBlocked(dimension, valueForDimension)) {
        return true;
    }
    
    checkAndCreateDimensionRestriction(dimension);
    auto& existingDimensionRestriction = dimensionRestrictions.at(dimension);

    if (existingDimensionRestriction.restrictionsPerValueOfDimension.count(valueForDimension) == 0) {
        existingDimensionRestriction.restrictionsPerValueOfDimension.insert({valueForDimension, SignalRestriction(signal->bitwidth)});
    }
    
    auto& existingDimensionValueRestriction = existingDimensionRestriction.restrictionsPerValueOfDimension.at(valueForDimension);
    existingDimensionValueRestriction.restrictAccessTo(bitRange);
    return true;
}

bool SignalAccessRestriction::restrictAccessToValueOfDimension(const std::size_t dimension, const std::size_t valueForDimension) {
    if (!isValueForDimensionWithinRange(dimension, valueForDimension)) {
        return false;
    }

    if (isAccessOnValueOfDimensionBlocked(dimension, valueForDimension)) {
        return true;
    }

    checkAndCreateDimensionRestriction(dimension);
    auto& existingDimensionRestriction = dimensionRestrictions.at(dimension);
    if (existingDimensionRestriction.restrictionsPerValueOfDimension.count(valueForDimension) == 0) {
        existingDimensionRestriction.restrictionsPerValueOfDimension.insert({valueForDimension, SignalRestriction(signal->bitwidth)});
    }

    auto& existingDimensionValueRestriction = existingDimensionRestriction.restrictionsPerValueOfDimension.at(valueForDimension);
    existingDimensionValueRestriction.blockCompletely();
    return true;
}

bool SignalAccessRestriction::restrictAccessToDimension(const std::size_t dimension) {
    if (!isDimensionWithinRange(dimension)) {
        return false;
    }

    if (isAccessOnDimensionBlocked(dimension)) {
        return true;
    }

    checkAndCreateDimensionRestriction(dimension);
    dimensionRestrictions.at(dimension).blockCompletely();
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

bool SignalAccessRestriction::isAccessOnDimensionBlocked(const std::size_t& dimension, const bool& considerBitRestrictions) const {
    if (isAccessRestrictedOnWholeSignal() || (considerBitRestrictions ? globalSignalRestriction.has_value() : false)) {
        return true;
    }

    if (dimensionRestrictions.count(dimension) == 0) {
        return false;
    }

    const auto& restrictionsForDimension = dimensionRestrictions.at(dimension);
    const bool  isBlocked               = restrictionsForDimension.areAllValuesForDimensionBlocked;
    if (!isBlocked && considerBitRestrictions) {
        if (restrictionsForDimension.dimensionSignalRestriction.has_value()) {
            return true;
        }
        const auto& valuesForDimension = createIndexSequenceExcludingEnd(0, signal->dimensions.at(dimension));
        return std::any_of(
                valuesForDimension.cbegin(),
                valuesForDimension.cend(),
                [&](const auto& valueForDimension) {
                    return isAccessOnValueOfDimensionBlocked(dimension, valueForDimension, true);
                });   
    }
    return isBlocked;
}

bool SignalAccessRestriction::isAccessOnValueOfDimensionBlocked(const std::size_t& dimension, const std::size_t& valueForDimension, const bool& considerBitRestrictions) const {
    if (isAccessRestrictedOnWholeSignal() || (considerBitRestrictions ? globalSignalRestriction.has_value() : false)) {
        return true;
    }

    if (dimensionRestrictions.count(dimension) == 0) {
        return false;
    }

    const auto& restrictionsForDimension = dimensionRestrictions.at(dimension);
    const bool  isBlocked                = isAccessOnDimensionBlocked(dimension, false) || restrictionsForDimension.areAllValuesForDimensionBlocked;
    if (!isBlocked && restrictionsForDimension.restrictionsPerValueOfDimension.count(valueForDimension) != 0) {
        const auto& restrictionsForValueOfDimension = restrictionsForDimension.restrictionsPerValueOfDimension.at(valueForDimension);
        if (considerBitRestrictions) {
            return restrictionsForDimension.dimensionSignalRestriction.has_value() || restrictionsForDimension.restrictionsPerValueOfDimension.at(valueForDimension).isAccessRestricted(SignalAccess(0, signal->bitwidth - 1));
        }
        return restrictionsForValueOfDimension.isCompletelyBlocked();
    }
    return isBlocked || (considerBitRestrictions ? restrictionsForDimension.dimensionSignalRestriction.has_value() : false);
}

void SignalAccessRestriction::updateExistingBitRangeRestrictions(const SignalAccessRestriction::SignalAccess& bitRange) {
    for (auto dimensionRestrictionEntry = dimensionRestrictions.begin(); dimensionRestrictionEntry != dimensionRestrictions.end();) {
        const auto dimension = dimensionRestrictionEntry->first;
        updateExistingBitRangeRestrictions(bitRange, dimension);
        if (dimensionRestrictionEntry->second.restrictionsPerValueOfDimension.empty()) {
            dimensionRestrictions.erase(dimension);
        }
        
        ++dimensionRestrictionEntry;
    }
}

void SignalAccessRestriction::updateExistingBitRangeRestrictions(const SignalAccessRestriction::SignalAccess& bitRange, const std::size_t dimension) {
    if (dimensionRestrictions.count(dimension) == 0) {
        return;
    }

    auto dimensionRestriction = dimensionRestrictions.at(dimension);

    if ((dimensionRestriction.dimensionSignalRestriction.has_value() && dimensionRestriction.dimensionSignalRestriction->isCompletelyBlocked()) || dimensionRestriction.areAllValuesForDimensionBlocked || dimensionRestriction.restrictionsPerValueOfDimension.empty()) {
        return;
    }

    for (auto valueOfDimensionRestrictionEntry = dimensionRestriction.restrictionsPerValueOfDimension.begin(); valueOfDimensionRestrictionEntry != dimensionRestriction.restrictionsPerValueOfDimension.end();) {
        auto valueOfDimensionRestriction = valueOfDimensionRestrictionEntry->second;
        if (!valueOfDimensionRestriction.hasAnyRestrictions() || valueOfDimensionRestriction.isCompletelyBlocked() || !valueOfDimensionRestriction.isAccessRestricted(bitRange)) {
            ++valueOfDimensionRestrictionEntry;
            continue;
        }

        valueOfDimensionRestriction.removeRestrictionFor(bitRange);
        if (!valueOfDimensionRestriction.hasAnyRestrictions()) {
            const auto valueOfDimensionToBeRemoved = valueOfDimensionRestrictionEntry->first;
            ++valueOfDimensionRestrictionEntry;

            dimensionRestriction.restrictionsPerValueOfDimension.erase(valueOfDimensionToBeRemoved);
        } else {
            ++valueOfDimensionRestrictionEntry;            
        }
    }
}