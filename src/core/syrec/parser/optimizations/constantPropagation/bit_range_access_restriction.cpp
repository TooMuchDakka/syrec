#include "core/syrec/parser/optimizations/constantPropagation/bit_range_access_restriction.hpp"

using namespace optimizations;

bool BitRangeAccessRestriction::hasAnyRestrictions() const {
    return false;
}


bool BitRangeAccessRestriction::isAccessCompletelyRestricted() const {
    return false;
}

bool BitRangeAccessRestriction::isAccessRestrictedTo(const BitRangeAccess& specificBitRange) const {
    return false;
}

void BitRangeAccessRestriction::liftAllRestrictions() {
    
}

void BitRangeAccessRestriction::liftRestrictionFor(const BitRangeAccess& specificBitRange) {
    
}

void BitRangeAccessRestriction::restrictAccessTo(const BitRangeAccess& specificBitRange) {
    
}