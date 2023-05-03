#include "test_base_signal_value_lookup.hpp"
#include "test_signal_value_lookup_swap.hpp"

#include "core/syrec/parser/utils/bit_helpers.hpp"

using namespace valueLookup;

using UserDefinedDimensionAccess = std::vector<std::optional<unsigned int>>;
using OptionalBitRangeAccess     = std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess>;

// TODO: CONSTANT_PROPAGATION: Decide if commented tests should be implemented

INSTANTIATE_TEST_SUITE_P(
    SignalValueLookupTests,
    SignalValueLookupSwapTest,
    testing::Values(
        std::make_tuple(
            "SwapCompleteSignals",
            []() { return SignalValueLookupSwapTest::createDefaultInitializedLhsSignalValueLookup();  },
            []() { return SignalValueLookupSwapTest::createDefaultInitializedRhsSignalValueLookup(SignalValueLookupSwapTest::SwapRhsOperandSignalSize::FULL, true); },
            [](const SignalValueLookup::ptr& lhsSwapSignalValueLookup, const SignalValueLookup::ptr& rhsSwapSignalValueLookup) { 
                lhsSwapSignalValueLookup->swapValuesAndRestrictionsBetween(
                    {}, std::nullopt,
                    {}, std::nullopt,
                    *rhsSwapSignalValueLookup
                );
            },
            [](const UserDefinedDimensionAccess& lhsAccessedDimensions, const OptionalBitRangeAccess& lhsAccessedBitRange) { 
                return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(lhsAccessedDimensions)
                    || (*lhsAccessedDimensions.at(0) == SignalValueLookupSwapTest::rhsLockedValueOfFirstDimension
                        && *lhsAccessedDimensions.at(1) == SignalValueLookupSwapTest::rhsLockedValueOfSecondDimension
                        && *lhsAccessedDimensions.at(2) == SignalValueLookupSwapTest::rhsLockedValueOfThirdDimension
                            && (lhsAccessedBitRange.has_value() && BaseSignalValueLookupTest::doesBitrangeOverlapOther(*lhsAccessedBitRange, SignalValueLookupSwapTest::rhsDefaultLockedBitRange))); 
            },
            [](const UserDefinedDimensionAccess& lhsAccessedDimensions, const OptionalBitRangeAccess& lhsAccessedBitRange) -> std::optional<unsigned int> { 
                if (!lhsAccessedBitRange.has_value()) 
                    return std::make_optional(SignalValueLookupSwapTest::defaultRhsValue);

                return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupSwapTest::defaultRhsValue, *lhsAccessedBitRange));
            },
            [](const UserDefinedDimensionAccess& rhsAccessedDimensions, const OptionalBitRangeAccess& rhsAccessedBitRange) { 
                return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(rhsAccessedDimensions)
                    || (*rhsAccessedDimensions.at(0) == SignalValueLookupSwapTest::lhsLockedValueOfFirstDimension
                        && *rhsAccessedDimensions.at(1) == SignalValueLookupSwapTest::lhsLockedValueOfSecondDimension
                        && *rhsAccessedDimensions.at(2) == SignalValueLookupSwapTest::lhsLockedValueOfThirdDimension
                        && (rhsAccessedBitRange.has_value() && BaseSignalValueLookupTest::doesBitrangeOverlapOther(*rhsAccessedBitRange, SignalValueLookupSwapTest::lhsDefaultLockedBitRange))); 
            },
            [](const UserDefinedDimensionAccess& rhsAccessedDimensions, const OptionalBitRangeAccess& rhsAccessedBitRange) -> std::optional<unsigned int> {
                if (!rhsAccessedBitRange.has_value()) 
                    return std::make_optional(SignalValueLookupSwapTest::defaultLhsValue);

                return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupSwapTest::defaultLhsValue, *rhsAccessedBitRange));
             }),
        std::make_tuple(
            "SwapGlobalBitWithOtherGlobalBit",
            []() { return SignalValueLookupSwapTest::createDefaultInitializedLhsSignalValueLookup();  },
            []() { return SignalValueLookupSwapTest::createDefaultInitializedRhsSignalValueLookup(SignalValueLookupSwapTest::SwapRhsOperandSignalSize::FULL, true); },
            [](const SignalValueLookup::ptr& lhsSwapSignalValueLookup, const SignalValueLookup::ptr& rhsSwapSignalValueLookup) {
                lhsSwapSignalValueLookup->swapValuesAndRestrictionsBetween(
                    {}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupSwapTest::lhsDefaultNonLockedBitRange.first, SignalValueLookupSwapTest::lhsDefaultNonLockedBitRange.first),
                    {}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupSwapTest::rhsDefaultLockedBitRange.first, SignalValueLookupSwapTest::rhsDefaultLockedBitRange.first),
                *rhsSwapSignalValueLookup);

                lhsSwapSignalValueLookup->swapValuesAndRestrictionsBetween(
                    {}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupSwapTest::lhsLockedAndRhsNotLockedBitRange.first, SignalValueLookupSwapTest::lhsLockedAndRhsNotLockedBitRange.first),
                    {}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupSwapTest::lhsLockedAndRhsNotLockedBitRange.first, SignalValueLookupSwapTest::lhsLockedAndRhsNotLockedBitRange.first),
                *rhsSwapSignalValueLookup);
            },
            [](const UserDefinedDimensionAccess& lhsAccessedDimensions, const OptionalBitRangeAccess& lhsAccessedBitRange) {
                if (!BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(lhsAccessedDimensions)) {
                    return true;
                }

                if (*lhsAccessedDimensions.at(0) == SignalValueLookupSwapTest::rhsLockedValueOfFirstDimension 
                    && *lhsAccessedDimensions.at(1) == SignalValueLookupSwapTest::rhsLockedValueOfSecondDimension
                    && *lhsAccessedDimensions.at(2) == SignalValueLookupSwapTest::rhsLockedValueOfThirdDimension) {
                    return !lhsAccessedBitRange.has_value() || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*lhsAccessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupSwapTest::rhsDefaultLockedBitRange.first, SignalValueLookupSwapTest::rhsDefaultLockedBitRange.first));
                }
                if (*lhsAccessedDimensions.at(0) == SignalValueLookupSwapTest::lhsLockedValueOfFirstDimension 
                    && *lhsAccessedDimensions.at(1) == SignalValueLookupSwapTest::lhsLockedValueOfSecondDimension 
                    && *lhsAccessedDimensions.at(2) == SignalValueLookupSwapTest::lhsLockedValueOfThirdDimension) {

                    auto modifiedLhsLockedBitRange = ::optimizations::BitRangeAccessRestriction(SignalValueLookupSwapTest::defaultSignalBitwidth, SignalValueLookupSwapTest::lhsDefaultLockedBitRange);
                    modifiedLhsLockedBitRange.liftRestrictionFor(::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupSwapTest::lhsLockedAndRhsNotLockedBitRange.first, SignalValueLookupSwapTest::lhsLockedAndRhsNotLockedBitRange.first));
                    return !lhsAccessedBitRange.has_value() || modifiedLhsLockedBitRange.isAccessRestrictedTo(*lhsAccessedBitRange);
                }

                return false;
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess& lhsAccessedBitRange) -> std::optional<unsigned int> {
                auto expectedDefaultValue = (SignalValueLookupSwapTest::defaultLhsValue & ~(1 << SignalValueLookupSwapTest::lhsLockedAndRhsNotLockedBitRange.first));
                const auto bitValueOfNotLockedRhs = BitHelpers::extractBitsFromValue(SignalValueLookupSwapTest::defaultRhsValue, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupSwapTest::lhsLockedAndRhsNotLockedBitRange.first, SignalValueLookupSwapTest::lhsLockedAndRhsNotLockedBitRange.first));
                expectedDefaultValue |= (bitValueOfNotLockedRhs << SignalValueLookupSwapTest::lhsLockedAndRhsNotLockedBitRange.first);

                if (!lhsAccessedBitRange.has_value()) 
                    return std::make_optional(expectedDefaultValue);

                return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(expectedDefaultValue, *lhsAccessedBitRange));
            },
            [](const UserDefinedDimensionAccess& rhsAccessedDimensions, const OptionalBitRangeAccess& rhsAccessedBitRange) {
                if (!BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(rhsAccessedDimensions)) {
                    return true;
                }

                if (*rhsAccessedDimensions.at(0) == SignalValueLookupSwapTest::rhsLockedValueOfFirstDimension
                        && *rhsAccessedDimensions.at(1) == SignalValueLookupSwapTest::rhsLockedValueOfSecondDimension
                        && *rhsAccessedDimensions.at(2) == SignalValueLookupSwapTest::rhsLockedValueOfThirdDimension) {
                    auto modifiedRhsLockedBitRange = ::optimizations::BitRangeAccessRestriction(SignalValueLookupSwapTest::defaultSignalBitwidth, SignalValueLookupSwapTest::rhsDefaultLockedBitRange);
                    modifiedRhsLockedBitRange.liftRestrictionFor(::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupSwapTest::rhsDefaultLockedBitRange.first, SignalValueLookupSwapTest::rhsDefaultLockedBitRange.first));
                    modifiedRhsLockedBitRange.restrictAccessTo(::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupSwapTest::lhsLockedAndRhsNotLockedBitRange.first, SignalValueLookupSwapTest::lhsLockedAndRhsNotLockedBitRange.first));
                    return !rhsAccessedBitRange.has_value() || modifiedRhsLockedBitRange.isAccessRestrictedTo(*rhsAccessedBitRange);
                }
                if (*rhsAccessedDimensions.at(0) == SignalValueLookupSwapTest::lhsLockedValueOfFirstDimension 
                    && *rhsAccessedDimensions.at(1) == SignalValueLookupSwapTest::lhsLockedValueOfSecondDimension 
                    && *rhsAccessedDimensions.at(2) == SignalValueLookupSwapTest::lhsLockedValueOfThirdDimension) {
                    return !rhsAccessedBitRange.has_value() || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*rhsAccessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupSwapTest::lhsLockedAndRhsNotLockedBitRange.first, SignalValueLookupSwapTest::lhsLockedAndRhsNotLockedBitRange.first));
                }
                return false;
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess& rhsAccessedBitRange) -> std::optional<unsigned int> {
                auto       expectedDefaultValue   = (SignalValueLookupSwapTest::defaultRhsValue & ~(1 << SignalValueLookupSwapTest::lhsLockedAndRhsNotLockedBitRange.first));
                const auto bitValueOfNotLockedLhs = BitHelpers::extractBitsFromValue(SignalValueLookupSwapTest::defaultLhsValue, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupSwapTest::lhsLockedAndRhsNotLockedBitRange.first, SignalValueLookupSwapTest::lhsLockedAndRhsNotLockedBitRange.first));
                expectedDefaultValue |= (bitValueOfNotLockedLhs << SignalValueLookupSwapTest::lhsLockedAndRhsNotLockedBitRange.first);

                if (!rhsAccessedBitRange.has_value())
                    return std::make_optional(expectedDefaultValue);

                return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(expectedDefaultValue, *rhsAccessedBitRange));
            }),
        std::make_tuple(
            "SwapGlobalBitRangeWithOtherGlobaBitRangeAtSamePosition",
            []() { return SignalValueLookupSwapTest::createDefaultInitializedLhsSignalValueLookup();  },
            []() { return SignalValueLookupSwapTest::createDefaultInitializedRhsSignalValueLookup(SignalValueLookupSwapTest::SwapRhsOperandSignalSize::FULL, true); },
            [](const SignalValueLookup::ptr& lhsSwapSignalValueLookup, const SignalValueLookup::ptr& rhsSwapSignalValueLookup) {
                lhsSwapSignalValueLookup->swapValuesAndRestrictionsBetween(
                    {}, SignalValueLookupSwapTest::lhsDefaultLockedBitRange, 
                    {}, SignalValueLookupSwapTest::rhsDefaultNonLockedBitRange,
                *rhsSwapSignalValueLookup);

                rhsSwapSignalValueLookup->swapValuesAndRestrictionsBetween(
                    {}, SignalValueLookupSwapTest::rhsDefaultLockedBitRange, 
                    {}, SignalValueLookupSwapTest::lhsDefaultNonLockedBitRange,
                *lhsSwapSignalValueLookup);
            },
            [](const UserDefinedDimensionAccess& lhsAccessedDimensions, const OptionalBitRangeAccess& lhsAccessedBitRange) { 
                 return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(lhsAccessedDimensions)
                    || (*lhsAccessedDimensions.at(0) == SignalValueLookupSwapTest::lhsLockedValueOfFirstDimension
                        && *lhsAccessedDimensions.at(1) == SignalValueLookupSwapTest::lhsLockedValueOfSecondDimension
                        && *lhsAccessedDimensions.at(2) == SignalValueLookupSwapTest::lhsLockedValueOfThirdDimension
                            && (!lhsAccessedBitRange.has_value() || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*lhsAccessedBitRange, SignalValueLookupSwapTest::lhsDefaultNonLockedBitRange)));
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess& lhsAccessedBitRange) -> std::optional<unsigned int> { 
                if (!lhsAccessedBitRange.has_value()) 
                    return std::make_optional(SignalValueLookupSwapTest::defaultRhsValue);

                return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupSwapTest::defaultRhsValue, *lhsAccessedBitRange));
            },
            [](const UserDefinedDimensionAccess& rhsAccessedDimensions, const OptionalBitRangeAccess& rhsAccessedBitRange) { 
                return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(rhsAccessedDimensions)
                    || (*rhsAccessedDimensions.at(0) == SignalValueLookupSwapTest::rhsLockedValueOfFirstDimension
                        && *rhsAccessedDimensions.at(1) == SignalValueLookupSwapTest::rhsLockedValueOfSecondDimension
                        && *rhsAccessedDimensions.at(2) == SignalValueLookupSwapTest::rhsLockedValueOfThirdDimension
                            && (!rhsAccessedBitRange.has_value() || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*rhsAccessedBitRange, SignalValueLookupSwapTest::rhsDefaultLockedBitRange)));
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess& rhsAccessedBitRange) -> std::optional<unsigned int> { 
                if (!rhsAccessedBitRange.has_value()) 
                    return std::make_optional(SignalValueLookupSwapTest::defaultLhsValue);

                return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupSwapTest::defaultLhsValue, *rhsAccessedBitRange));
            }),
        std::make_tuple(
            "SwapIncompleteDimensionAccessWithAnotherIncompleteDimensionAccess",
            []() { return SignalValueLookupSwapTest::createDefaultInitializedLhsSignalValueLookup();  },
            []() { return SignalValueLookupSwapTest::createDefaultInitializedRhsSignalValueLookup(SignalValueLookupSwapTest::SwapRhsOperandSignalSize::FULL, true); },
            [](const SignalValueLookup::ptr& lhsSwapSignalValueLookup, const SignalValueLookup::ptr& rhsSwapSignalValueLookup) {
                lhsSwapSignalValueLookup->swapValuesAndRestrictionsBetween(
                    { SignalValueLookupSwapTest::lhsLockedValueOfFirstDimension, SignalValueLookupSwapTest::lhsLockedValueOfSecondDimension }, std::nullopt,        // No locked values in last dimension
                    { SignalValueLookupSwapTest::rhsNotLockedValueOfFirstDimension, SignalValueLookupSwapTest::rhsNotLockedValueOfSecondDimension }, std::nullopt,  // Locked value in last dimension is at lhsLockedValueOfThirdDimension
                *rhsSwapSignalValueLookup);

                lhsSwapSignalValueLookup->swapValuesAndRestrictionsBetween(
                    { SignalValueLookupSwapTest::lhsLockedValueOfFirstDimension }, std::nullopt,
                    { SignalValueLookupSwapTest::rhsLockedValueOfFirstDimension }, std::nullopt,
                *rhsSwapSignalValueLookup);
            },
            [](const UserDefinedDimensionAccess& lhsAccessedDimensions, const OptionalBitRangeAccess&) { 
                return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(lhsAccessedDimensions);
            },
            [](const UserDefinedDimensionAccess& lhsAccessedDimensions, const OptionalBitRangeAccess& lhsAccessedBitRange) -> std::optional<unsigned int> {
                const auto expectedDefaultValue = *lhsAccessedDimensions.at(0) == SignalValueLookupSwapTest::lhsLockedValueOfFirstDimension
                    ? SignalValueLookupSwapTest::defaultRhsValue
                    : SignalValueLookupSwapTest::defaultLhsValue;

                if (!lhsAccessedBitRange.has_value()) 
                    return std::make_optional(expectedDefaultValue);

                return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(expectedDefaultValue, *lhsAccessedBitRange));
            },
            [](const UserDefinedDimensionAccess& rhsAccessedDimensions, const OptionalBitRangeAccess& rhsAccessedBitRange) { 
                return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(rhsAccessedDimensions)
                    || (*rhsAccessedDimensions.at(0) == SignalValueLookupSwapTest::rhsNotLockedValueOfFirstDimension
                    && *rhsAccessedDimensions.at(1) == SignalValueLookupSwapTest::rhsNotLockedValueOfSecondDimension
                    && *rhsAccessedDimensions.at(2) == SignalValueLookupSwapTest::lhsLockedValueOfThirdDimension
                    && (!rhsAccessedBitRange.has_value() && BaseSignalValueLookupTest::doesBitrangeOverlapOther(*rhsAccessedBitRange, SignalValueLookupSwapTest::lhsDefaultLockedBitRange)));
            },
            [](const UserDefinedDimensionAccess& rhsAccessedDimensions, const OptionalBitRangeAccess& rhsAccessedBitRange) -> std::optional<unsigned int> { 
                const auto expectedDefaultValue = *rhsAccessedDimensions.at(0) == SignalValueLookupSwapTest::rhsLockedValueOfFirstDimension
                    ? SignalValueLookupSwapTest::defaultLhsValue
                    : SignalValueLookupSwapTest::defaultRhsValue;

                if (!rhsAccessedBitRange.has_value())
                    return std::make_optional(expectedDefaultValue);

                return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(expectedDefaultValue, *rhsAccessedBitRange));
            }),
        std::make_tuple(
            "SwapBitRangesOfIncompleteDimensionAccesses",
            []() -> SignalValueLookup::ptr { return SignalValueLookupSwapTest::createDefaultInitializedLhsSignalValueLookup(); },
            []() -> SignalValueLookup::ptr { return SignalValueLookupSwapTest::createDefaultInitializedRhsSignalValueLookup(SignalValueLookupSwapTest::SwapRhsOperandSignalSize::FORINTERMEDIATEACCESS, true); },
            [](const SignalValueLookup::ptr& lhsSwapSignalValueLookup, const SignalValueLookup::ptr& rhsSwapSignalValueLookup) {
                lhsSwapSignalValueLookup->swapValuesAndRestrictionsBetween(
                    { SignalValueLookupSwapTest::lhsLockedValueOfFirstDimension, SignalValueLookupSwapTest::lhsLockedValueOfSecondDimension }, SignalValueLookupSwapTest::lhsDefaultLockedBitRange,
                    { SignalValueLookupSwapTest::rhsLockedValueOfSecondDimension }, SignalValueLookupSwapTest::rhsDefaultLockedBitRange,
                *rhsSwapSignalValueLookup);
            },
            [](const UserDefinedDimensionAccess& lhsAccessedDimensions, const OptionalBitRangeAccess& lhsAccessedBitRange) { 
                return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(lhsAccessedDimensions)
                    || (*lhsAccessedDimensions.at(0) == SignalValueLookupSwapTest::lhsLockedValueOfFirstDimension
                    && *lhsAccessedDimensions.at(1) == SignalValueLookupSwapTest::lhsLockedValueOfSecondDimension
                    && *lhsAccessedDimensions.at(2) == SignalValueLookupSwapTest::rhsLockedValueOfThirdDimension
                    && (!lhsAccessedBitRange.has_value() || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*lhsAccessedBitRange, SignalValueLookupSwapTest::lhsDefaultLockedBitRange)));
            },
            [](const UserDefinedDimensionAccess& lhsAccessedDimensions, const OptionalBitRangeAccess& lhsAccessedBitRange) -> std::optional<unsigned int> { 
                auto expectedValue = SignalValueLookupSwapTest::defaultLhsValue;
                if (*lhsAccessedDimensions.at(0) == SignalValueLookupSwapTest::lhsLockedValueOfFirstDimension && *lhsAccessedDimensions.at(1) == SignalValueLookupSwapTest::lhsLockedValueOfSecondDimension) {
                    const auto swappedPartsOfRhsValue = BitHelpers::extractBitsFromValue(SignalValueLookupSwapTest::defaultRhsValue, SignalValueLookupSwapTest::rhsDefaultNonLockedBitRange);
                    expectedValue = BitHelpers::mergeValues(expectedValue, swappedPartsOfRhsValue, SignalValueLookupSwapTest::lhsDefaultLockedBitRange);
                }
                
                if (!lhsAccessedBitRange.has_value()) 
                    return std::make_optional(expectedValue);

                return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(expectedValue, *lhsAccessedBitRange));
            },
            [](const UserDefinedDimensionAccess& rhsAccessedDimensions, const OptionalBitRangeAccess& rhsAccessedBitRange) { 
                return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(rhsAccessedDimensions)
                    || (*rhsAccessedDimensions.at(0) == SignalValueLookupSwapTest::rhsLockedValueOfSecondDimension
                    && *rhsAccessedDimensions.at(1) == SignalValueLookupSwapTest::lhsLockedValueOfThirdDimension
                    && (!rhsAccessedBitRange.has_value() || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*rhsAccessedBitRange, SignalValueLookupSwapTest::rhsDefaultLockedBitRange)));
            },
            [](const UserDefinedDimensionAccess& rhsAccessedDimensions, const OptionalBitRangeAccess& rhsAccessedBitRange) -> std::optional<unsigned int> { 
                auto expectedValue = SignalValueLookupSwapTest::defaultRhsValue;
                if (*rhsAccessedDimensions.at(0) == SignalValueLookupSwapTest::rhsLockedValueOfSecondDimension) {
                    const auto swappedPartsOfLhsValue = BitHelpers::extractBitsFromValue(SignalValueLookupSwapTest::defaultLhsValue, SignalValueLookupSwapTest::lhsDefaultNonLockedBitRange);
                    expectedValue = BitHelpers::mergeValues(expectedValue, swappedPartsOfLhsValue, SignalValueLookupSwapTest::rhsDefaultLockedBitRange);
                }
                
                if (!rhsAccessedBitRange.has_value()) 
                    return std::make_optional(expectedValue);

                return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(expectedValue, *rhsAccessedBitRange));
            }),
        std::make_tuple(
            "SwapBitsOfIncompleteDimensionAccesses",
            []() -> SignalValueLookup::ptr { return SignalValueLookupSwapTest::createDefaultInitializedLhsSignalValueLookup(); },
            []() -> SignalValueLookup::ptr { return SignalValueLookupSwapTest::createDefaultInitializedRhsSignalValueLookup(SignalValueLookupSwapTest::SwapRhsOperandSignalSize::FORINTERMEDIATEACCESS, true); },
            [](const SignalValueLookup::ptr& lhsSwapSignalValueLookup, const SignalValueLookup::ptr& rhsSwapSignalValueLookup) {
                lhsSwapSignalValueLookup->swapValuesAndRestrictionsBetween(
                    { SignalValueLookupSwapTest::lhsLockedValueOfFirstDimension, SignalValueLookupSwapTest::lhsLockedValueOfSecondDimension }, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupSwapTest::lhsNotLockedAndRhsLockedBitRange.first, SignalValueLookupSwapTest::lhsNotLockedAndRhsLockedBitRange.first),
                    { SignalValueLookupSwapTest::rhsLockedValueOfSecondDimension}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupSwapTest::lhsNotLockedAndRhsLockedBitRange.first, SignalValueLookupSwapTest::lhsNotLockedAndRhsLockedBitRange.first),
                *rhsSwapSignalValueLookup);
            },
            [](const UserDefinedDimensionAccess& lhsAccessedDimensions, const OptionalBitRangeAccess& lhsAccessedBitRange) { 
                if  (!BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(lhsAccessedDimensions))
                    return true;

                if (*lhsAccessedDimensions.at(0) == SignalValueLookupSwapTest::lhsLockedValueOfFirstDimension 
                    && *lhsAccessedDimensions.at(1) == SignalValueLookupSwapTest::lhsLockedValueOfSecondDimension
                    && *lhsAccessedDimensions.at(2) == SignalValueLookupSwapTest::rhsLockedValueOfThirdDimension) {
                        auto modifiedLhsLockedBitRange = ::optimizations::BitRangeAccessRestriction(SignalValueLookupSwapTest::defaultSignalBitwidth, SignalValueLookupSwapTest::lhsDefaultLockedBitRange);
                        modifiedLhsLockedBitRange.restrictAccessTo(::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupSwapTest::lhsNotLockedAndRhsLockedBitRange.first, SignalValueLookupSwapTest::lhsNotLockedAndRhsLockedBitRange.first));
                        return !lhsAccessedBitRange.has_value() || modifiedLhsLockedBitRange.isAccessRestrictedTo(*lhsAccessedBitRange);
                }
                return false; 
            },
            [](const UserDefinedDimensionAccess& lhsAccessedDimensions, const OptionalBitRangeAccess& lhsAccessedBitRange) -> std::optional<unsigned int> { 
                auto expectedDefaultValue = SignalValueLookupSwapTest::defaultLhsValue;
                if (*lhsAccessedDimensions.at(0) == SignalValueLookupSwapTest::lhsLockedValueOfFirstDimension
                    && *lhsAccessedDimensions.at(1) == SignalValueLookupSwapTest::lhsLockedValueOfSecondDimension) {
                        expectedDefaultValue = (SignalValueLookupSwapTest::defaultLhsValue & ~(1 << SignalValueLookupSwapTest::lhsNotLockedAndRhsLockedBitRange.first));
                        const auto bitValueOfNotLockedRhs = BitHelpers::extractBitsFromValue(SignalValueLookupSwapTest::defaultRhsValue, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupSwapTest::lhsNotLockedAndRhsLockedBitRange.first, SignalValueLookupSwapTest::lhsNotLockedAndRhsLockedBitRange.first));
                        expectedDefaultValue |= (bitValueOfNotLockedRhs << SignalValueLookupSwapTest::lhsLockedAndRhsNotLockedBitRange.first);
                }
                
                if (!lhsAccessedBitRange.has_value()) 
                    return std::make_optional(expectedDefaultValue);

                return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(expectedDefaultValue, *lhsAccessedBitRange));
            },
            [](const UserDefinedDimensionAccess& rhsAccessedDimensions, const OptionalBitRangeAccess& rhsAccessedBitRange) { 
                if  (!BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(rhsAccessedDimensions))
                    return true;

                if (*rhsAccessedDimensions.at(0) == SignalValueLookupSwapTest::rhsLockedValueOfSecondDimension 
                    && *rhsAccessedDimensions.at(1) == SignalValueLookupSwapTest::lhsLockedValueOfThirdDimension) {
                        auto modifiedRhsLockedBitRange = ::optimizations::BitRangeAccessRestriction(SignalValueLookupSwapTest::defaultSignalBitwidth, SignalValueLookupSwapTest::rhsDefaultLockedBitRange);
                        modifiedRhsLockedBitRange.liftRestrictionFor(::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupSwapTest::lhsNotLockedAndRhsLockedBitRange.first, SignalValueLookupSwapTest::lhsNotLockedAndRhsLockedBitRange.first));
                        return !rhsAccessedBitRange.has_value() || modifiedRhsLockedBitRange.isAccessRestrictedTo(*rhsAccessedBitRange);
                }
                return false; 
            },
            [](const UserDefinedDimensionAccess& rhsAccessedDimensions, const OptionalBitRangeAccess& rhsAccessedBitRange) -> std::optional<unsigned int> { 
                auto expectedDefaultValue = SignalValueLookupSwapTest::defaultRhsValue;
                if (*rhsAccessedDimensions.at(0) == SignalValueLookupSwapTest::rhsLockedValueOfSecondDimension 
                    && *rhsAccessedDimensions.at(1) == SignalValueLookupSwapTest::lhsLockedValueOfThirdDimension) {
                    expectedDefaultValue = (SignalValueLookupSwapTest::defaultRhsValue & ~(1 << SignalValueLookupSwapTest::lhsNotLockedAndRhsLockedBitRange.first));
                    const auto bitValueOfNotLockedLhs = BitHelpers::extractBitsFromValue(SignalValueLookupSwapTest::defaultLhsValue, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupSwapTest::lhsNotLockedAndRhsLockedBitRange.first, SignalValueLookupSwapTest::lhsNotLockedAndRhsLockedBitRange.first));
                    expectedDefaultValue |= (bitValueOfNotLockedLhs << SignalValueLookupSwapTest::lhsLockedAndRhsNotLockedBitRange.first);
                }
                
                if (!rhsAccessedBitRange.has_value()) 
                    return std::make_optional(expectedDefaultValue);

                return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(expectedDefaultValue, *rhsAccessedBitRange));
            }),
        // std::make_tuple(
        //     "SwapBitRangeOfIncompleteDimensionAccessWithBitRangeOfCompleteDimensionAccess",
        //     []() -> SignalValueLookup::ptr { return nullptr; },
        //     []() -> SignalValueLookup::ptr { return nullptr; },
        //     [](const SignalValueLookup::ptr& lhsSwapSignalValueLookup, const SignalValueLookup::ptr& rhsSwapSignalValueLookup) {},
        //     [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
        //     [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { return std::nullopt; },
        //     [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
        //     [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { return std::nullopt; }),
        // std::make_tuple(
        //     "SwapBitOfIncompleteDimensionAccessWithBitOfCompleteDimensionAccess",
        //     []() -> SignalValueLookup::ptr { return nullptr; },
        //     []() -> SignalValueLookup::ptr { return nullptr; },
        //     [](const SignalValueLookup::ptr& lhsSwapSignalValueLookup, const SignalValueLookup::ptr& rhsSwapSignalValueLookup) {},
        //     [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
        //     [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { return std::nullopt; },
        //     [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
        //     [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { return std::nullopt; }),
        std::make_tuple(
            "SwapIncompleteDimensionAccessWithCompleteSignal",
            []() { return SignalValueLookupSwapTest::createDefaultInitializedLhsSignalValueLookup();  },
            []() { return SignalValueLookupSwapTest::createDefaultInitializedRhsSignalValueLookup(SignalValueLookupSwapTest::SwapRhsOperandSignalSize::FORINTERMEDIATEACCESS, true); },
            [](const SignalValueLookup::ptr& lhsSwapSignalValueLookup, const SignalValueLookup::ptr& rhsSwapSignalValueLookup) {
                lhsSwapSignalValueLookup->swapValuesAndRestrictionsBetween(
                    { SignalValueLookupSwapTest::lhsLockedValueOfFirstDimension }, std::nullopt,
                    {}, std::nullopt,
                *rhsSwapSignalValueLookup);
            },
            [](const UserDefinedDimensionAccess& lhsAccessedDimensions, const OptionalBitRangeAccess& lhsAccessedBitRange) {
                if (!BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(lhsAccessedDimensions)) {
                    return true;
                }

                if (*lhsAccessedDimensions.at(0) != SignalValueLookupSwapTest::lhsLockedValueOfFirstDimension) {
                    return false;
                }

                if (*lhsAccessedDimensions.at(1) == SignalValueLookupSwapTest::lhsLockedValueOfSecondDimension
                    && *lhsAccessedDimensions.at(2) == SignalValueLookupSwapTest::lhsLockedValueOfThirdDimension) {
                    return !lhsAccessedBitRange.has_value() || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*lhsAccessedBitRange, SignalValueLookupSwapTest::lhsDefaultLockedBitRange);
                }

                if (*lhsAccessedDimensions.at(1) == SignalValueLookupSwapTest::rhsLockedValueOfSecondDimension 
                    && *lhsAccessedDimensions.at(2) == SignalValueLookupSwapTest::rhsLockedValueOfThirdDimension) {
                    return !lhsAccessedBitRange.has_value() || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*lhsAccessedBitRange, SignalValueLookupSwapTest::rhsDefaultLockedBitRange);
                }

                return false;
            },
            [](const UserDefinedDimensionAccess& lhsAccessedDimensions, const OptionalBitRangeAccess& lhsAccessedBitRange) -> std::optional<unsigned int> { 
                const auto expectedDefaultValue = *lhsAccessedDimensions.at(0) == SignalValueLookupSwapTest::lhsLockedValueOfFirstDimension
                    ? SignalValueLookupSwapTest::defaultRhsValue
                    : SignalValueLookupSwapTest::defaultLhsValue;

                if (!lhsAccessedBitRange.has_value()) 
                    return std::make_optional(expectedDefaultValue);

                return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(expectedDefaultValue, *lhsAccessedBitRange));
            },
            [](const UserDefinedDimensionAccess& rhsAccessedDimensions, const OptionalBitRangeAccess& rhsAccessedBitRange) { 
                if (!BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(rhsAccessedDimensions)) {
                    return true;
                }

                if (*rhsAccessedDimensions.at(0) == SignalValueLookupSwapTest::lhsLockedValueOfSecondDimension && *rhsAccessedDimensions.at(1) == SignalValueLookupSwapTest::lhsLockedValueOfThirdDimension) {
                    return !rhsAccessedBitRange.has_value() || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*rhsAccessedBitRange, SignalValueLookupSwapTest::lhsDefaultLockedBitRange);
                }
                return false;
            },
            [](const UserDefinedDimensionAccess& rhsAccessedDimensions, const OptionalBitRangeAccess& rhsAccessedBitRange) -> std::optional<unsigned int> { 
                if (!rhsAccessedBitRange.has_value()) 
                    return std::make_optional(SignalValueLookupSwapTest::defaultLhsValue);

                return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupSwapTest::defaultLhsValue, *rhsAccessedBitRange));
            }),
        //std::make_tuple(
        //    "SwapBitOfIncompleteDimensionAccessWithGlobalBitOf1DSignal",
        //    []() -> SignalValueLookup::ptr { return SignalValueLookupSwapTest::createDefaultInitializedLhsSignalValueLookup();  },
        //    []() -> SignalValueLookup::ptr { return SignalValueLookupSwapTest::createDefaultInitializedRhsSignalValueLookup(SignalValueLookupSwapTest::SwapRhsOperandSignalSize::FOR1DACCESS, true); },
        //    [](const SignalValueLookup::ptr& lhsSwapSignalValueLookup, const SignalValueLookup::ptr& rhsSwapSignalValueLookup) {
        //        lhsSwapSignalValueLookup->swapValuesAndRestrictionsBetween(
        //            { SignalValueLookupSwapTest::lhsNotLockedValueOfFirstDimension, SignalValueLookupSwapTest::lhsNotLockedValueOfSecondDimension, SignalValueLookupSwapTest::lhsNotLockedValueOfThirdDimension }, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupSwapTest::lhsDefaultNonLockedBitRange.first, SignalValueLookupSwapTest::lhsDefaultNonLockedBitRange.first),
        //            {}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupSwapTest::rhsDefaultLockedBitRange.first, SignalValueLookupSwapTest::rhsDefaultLockedBitRange.first)
        //        *rhsSwapSignalValueLookup);
        //    },
        //    [](const UserDefinedDimensionAccess& lhsAccessedDimensions, const OptionalBitRangeAccess& lhsAccessedBitRange) { 
        //        return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(lhsAccessedDimensions)
        //            || (*lhsAccessedDimensions.at(0) == SignalValueLookupSwapTest::lhsLockedValueOfFirstDimension
        //                && *lhsAccessedDimensions.at(1) == SignalValueLookupSwapTest::lhsLockedValueOfSecondDimension
        //                && *lhsAccessedDimensions.at(2) == SignalValueLookupSwapTest::lhsLockedValueOfThirdDimension
        //                && (!lhsAccessedBitRange.has_value() && BaseSignalValueLookupTest::doesBitrangeOverlapOther(*lhsAccessedBitRange, SignalValueLookupSwapTest::lhsDefaultLockedBitRange)))
        //            || (*lhsAccessedDimensions.at(0) == SignalValueLookupSwapTest::lhsNotLockedValueOfFirstDimension
        //                && *lhsAccessedDimensions.at(1) == SignalValueLookupSwapTest::lhsNotLockedValueOfSecondDimension
        //                && *lhsAccessedDimensions.at(2) == SignalValueLookupSwapTest::lhsNotLockedValueOfThirdDimension
        //                && (!lhsAccessedBitRange.has_value() && BaseSignalValueLookupTest::doesBitrangeOverlapOther(*lhsAccessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupSwapTest::lhsDefaultNonLockedBitRange.first, SignalValueLookupSwapTest::lhsDefaultNonLockedBitRange.first))));
        //        return false; 
        //    },
        //    [](const UserDefinedDimensionAccess& lhsAccessedDimensions, const OptionalBitRangeAccess& lhsAccessedBitRange) -> std::optional<unsigned int> { 
        //        if (!lhsAccessedBitRange.has_value()) 
        //            return std::make_optional(SignalValueLookupSwapTest::defaultLhsValue);

        //        return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultLhsValue, *lhsAccessedBitRange));
        //    },
        //    [](const UserDefinedDimensionAccess& rhsAccessedDimensions, const OptionalBitRangeAccess& rhsAccessedBitRange) { 
        //        auto lockedBitRange = SignalValueLookupSwapTest::rhsDefaultLockedBitRange;
        //        lockedBitRange->liftRestrictionFor(::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupSwapTest::rhsDefaultLockedBitRange.first, SignalValueLookupSwapTest::rhsDefaultLockedBitRange.first));

        //        return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(rhsAccessedDimensions)
        //            && *rhsAccessedDimensions.at(0) == SignalValueLookupSwapTest::rhsLockedValueOfFirstDimension
        //            && (!rhsAccessedBitRange.has_value() || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*rhsAccessedBitRange, lockedBitRange));
        //    },
        //    [](const UserDefinedDimensionAccess& rhsAccessedDimensions, const OptionalBitRangeAccess& rhsAccessedBitRange) -> std::optional<unsigned int> { 
        //        if (*rhsAccessedDimensions.at(0) == SignalValueLookupSwapTest::rhsLockedValueOfFirstDimension
        //            && (*rhsAccessedBitRange.second - *rhsAccessedBitRange.first) == 0
        //            && *rhsAccessedBitRange.first == SignalValueLookupSwapTest::rhsDefaultLockedBitRange.first) {
        //            return BaseSignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupSwapTest::defaultLhsValue, *rhsAccessedBitRange);
        //        }

        //        if (!rhsAccessedBitRange.has_value()) 
        //            return std::make_optional(SignalValueLookupSwapTest::defaultRhsValue);

        //        return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultRhsValue, *rhsAccessedBitRange));
        //    }),
        //std::make_tuple(
        //    "SwapBitOfIncompleteDimensionAccessWithAnotherIncompleteDimensionAccessOnOtherSignal",
        //    []() -> SignalValueLookup::ptr { return nullptr; },
        //    []() -> SignalValueLookup::ptr { return nullptr; },
        //    [](const SignalValueLookup::ptr& lhsSwapSignalValueLookup, const SignalValueLookup::ptr& rhsSwapSignalValueLookup) {},
        //    [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
        //    [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { return std::nullopt; },
        //    [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
        //    [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { return std::nullopt; }),
        //std::make_tuple(
        //    "SwapBitIncompleteDimensionAccessWithCompleteSignal",
        //    []() -> SignalValueLookup::ptr { return nullptr; },
        //    []() -> SignalValueLookup::ptr { return nullptr; },
        //    [](const SignalValueLookup::ptr& lhsSwapSignalValueLookup, const SignalValueLookup::ptr& rhsSwapSignalValueLookup) {},
        //    [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
        //    [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { return std::nullopt; },
        //    [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
        //    [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { return std::nullopt; }),

        //std::make_tuple(
        //    "SwapBitRangeOfIncompleteDimensionAccessWithCompleteSignal",
        //    []() -> SignalValueLookup::ptr { return nullptr; },
        //    []() -> SignalValueLookup::ptr { return nullptr; },
        //    [](const SignalValueLookup::ptr& lhsSwapSignalValueLookup, const SignalValueLookup::ptr& rhsSwapSignalValueLookup) {},
        //    [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
        //    [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { return std::nullopt; },
        //    [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
        //    [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { return std::nullopt; }),
        //std::make_tuple(
        //    "SwapBitRangeOfIncompleteDimensionAccessWithAnotherIncompleteDimensionAccess",
        //    []() -> SignalValueLookup::ptr { return nullptr; },
        //    []() -> SignalValueLookup::ptr { return nullptr; },
        //    [](const SignalValueLookup::ptr& lhsSwapSignalValueLookup, const SignalValueLookup::ptr& rhsSwapSignalValueLookup) {},
        //    [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
        //    [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { return std::nullopt; },
        //    [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
        //    [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { return std::nullopt; }),
        //std::make_tuple(
        //    "SwapBitRangeOfIncompleteDimensionAccessCompleteSignalWithAnotherIncompleteDimensionAccessBitRange",
        //    []() -> SignalValueLookup::ptr { return nullptr; },
        //    []() -> SignalValueLookup::ptr { return nullptr; },
        //    [](const SignalValueLookup::ptr& lhsSwapSignalValueLookup, const SignalValueLookup::ptr& rhsSwapSignalValueLookup) {},
        //    [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
        //    [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { return std::nullopt; },
        //    [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
        //    [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { return std::nullopt; }),
        //std::make_tuple(
        //    "SwapBitRangeOfIncompleteDimensionAccessWithCompleteDimensionAccess",
        //    []() -> SignalValueLookup::ptr { return nullptr; },
        //    []() -> SignalValueLookup::ptr { return nullptr; },
        //    [](const SignalValueLookup::ptr& lhsSwapSignalValueLookup, const SignalValueLookup::ptr& rhsSwapSignalValueLookup) {},
        //    [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
        //    [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { return std::nullopt; },
        //    [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
        //    [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { return std::nullopt; }),
        //std::make_tuple(
        //    "SwapBitRangeOfIncompleteDimensionAccessWithCompleteDimensionAccessAndBitRange",
        //    []() -> SignalValueLookup::ptr { return nullptr; },
        //    []() -> SignalValueLookup::ptr { return nullptr; },
        //    [](const SignalValueLookup::ptr& lhsSwapSignalValueLookup, const SignalValueLookup::ptr& rhsSwapSignalValueLookup) {},
        //    [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
        //    [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { return std::nullopt; },
        //    [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
        //    [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { return std::nullopt; }),
        std::make_tuple(
            "SwapValueOfDimensionFullDimensionAccess",
            []() { return SignalValueLookupSwapTest::createDefaultInitializedLhsSignalValueLookup();  },
            []() { return SignalValueLookupSwapTest::createDefaultInitializedRhsSignalValueLookup(SignalValueLookupSwapTest::SwapRhsOperandSignalSize::FULL, true); },
            [](const SignalValueLookup::ptr& lhsSwapSignalValueLookup, const SignalValueLookup::ptr& rhsSwapSignalValueLookup) {
                lhsSwapSignalValueLookup->swapValuesAndRestrictionsBetween(
                    { SignalValueLookupSwapTest::lhsNotLockedValueOfFirstDimension, SignalValueLookupSwapTest::lhsNotLockedValueOfSecondDimension, SignalValueLookupSwapTest::lhsNotLockedValueOfThirdDimension }, std::nullopt,
                    { SignalValueLookupSwapTest::rhsLockedValueOfFirstDimension, SignalValueLookupSwapTest::rhsLockedValueOfSecondDimension, SignalValueLookupSwapTest::rhsLockedValueOfThirdDimension }, std::nullopt,
                *rhsSwapSignalValueLookup);
            },
            [](const UserDefinedDimensionAccess& lhsAccessedDimensions, const OptionalBitRangeAccess& lhsAccessedBitRange) { 
                return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(lhsAccessedDimensions)
                    || (*lhsAccessedDimensions.at(0) == SignalValueLookupSwapTest::lhsLockedValueOfFirstDimension
                        && *lhsAccessedDimensions.at(1) == SignalValueLookupSwapTest::lhsLockedValueOfSecondDimension
                        && *lhsAccessedDimensions.at(2) == SignalValueLookupSwapTest::lhsLockedValueOfThirdDimension
                        && (!lhsAccessedBitRange.has_value() || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*lhsAccessedBitRange, SignalValueLookupSwapTest::lhsDefaultLockedBitRange)))
                    || (*lhsAccessedDimensions.at(0) == SignalValueLookupSwapTest::lhsNotLockedValueOfFirstDimension
                        && *lhsAccessedDimensions.at(1) == SignalValueLookupSwapTest::lhsNotLockedValueOfSecondDimension
                        && *lhsAccessedDimensions.at(2) == SignalValueLookupSwapTest::lhsNotLockedValueOfThirdDimension
                        && (!lhsAccessedBitRange.has_value() || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*lhsAccessedBitRange, SignalValueLookupSwapTest::rhsDefaultLockedBitRange)));
                return false; 
            },
            [](const UserDefinedDimensionAccess& lhsAccessedDimensions, const OptionalBitRangeAccess& lhsAccessedBitRange) -> std::optional<unsigned int> { 
                const auto expectedValue = (*lhsAccessedDimensions.at(0) == SignalValueLookupSwapTest::lhsNotLockedValueOfFirstDimension
                        && *lhsAccessedDimensions.at(1) == SignalValueLookupSwapTest::lhsNotLockedValueOfSecondDimension
                        && *lhsAccessedDimensions.at(2) == SignalValueLookupSwapTest::lhsNotLockedValueOfThirdDimension)
                        ? SignalValueLookupSwapTest::defaultRhsValue
                        : SignalValueLookupSwapTest::defaultLhsValue;

                if (!lhsAccessedBitRange.has_value()) 
                    return std::make_optional(expectedValue);

                return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(expectedValue, *lhsAccessedBitRange));
            },
            [](const UserDefinedDimensionAccess& rhsAccessedDimensions, const OptionalBitRangeAccess&) { 
                return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(rhsAccessedDimensions);
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess& rhsAccessedBitRange) -> std::optional<unsigned int> { 
                if (!rhsAccessedBitRange.has_value()) 
                    return std::make_optional(SignalValueLookupSwapTest::defaultRhsValue);

                return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupSwapTest::defaultRhsValue, *rhsAccessedBitRange));
             }),
        std::make_tuple(
            "SwapBitOfFullDimensionAccess",
            []() { return SignalValueLookupSwapTest::createDefaultInitializedLhsSignalValueLookup();  },
            []() { return SignalValueLookupSwapTest::createDefaultInitializedRhsSignalValueLookup(SignalValueLookupSwapTest::SwapRhsOperandSignalSize::FULL, true); },
            [](const SignalValueLookup::ptr& lhsSwapSignalValueLookup, const SignalValueLookup::ptr& rhsSwapSignalValueLookup) {
                lhsSwapSignalValueLookup->swapValuesAndRestrictionsBetween(
                    { SignalValueLookupSwapTest::lhsLockedValueOfFirstDimension, SignalValueLookupSwapTest::lhsLockedValueOfSecondDimension, SignalValueLookupSwapTest::lhsLockedValueOfThirdDimension }, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupSwapTest::lhsNotLockedAndRhsLockedBitRange.first, SignalValueLookupSwapTest::lhsNotLockedAndRhsLockedBitRange.first),
                    { SignalValueLookupSwapTest::rhsLockedValueOfFirstDimension, SignalValueLookupSwapTest::rhsLockedValueOfSecondDimension, SignalValueLookupSwapTest::rhsLockedValueOfThirdDimension }, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupSwapTest::lhsNotLockedAndRhsLockedBitRange.first, SignalValueLookupSwapTest::lhsNotLockedAndRhsLockedBitRange.first),
                *rhsSwapSignalValueLookup);
            },
            [](const UserDefinedDimensionAccess& lhsAccessedDimensions, const OptionalBitRangeAccess& lhsAccessedBitRange) {
                auto modifiedRhsLockedBitRange = ::optimizations::BitRangeAccessRestriction(SignalValueLookupSwapTest::defaultSignalBitwidth, SignalValueLookupSwapTest::lhsDefaultLockedBitRange);
                modifiedRhsLockedBitRange.restrictAccessTo(::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupSwapTest::lhsNotLockedAndRhsLockedBitRange.first, SignalValueLookupSwapTest::lhsNotLockedAndRhsLockedBitRange.first));

                return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(lhsAccessedDimensions)
                    || (*lhsAccessedDimensions.at(0) == SignalValueLookupSwapTest::lhsLockedValueOfFirstDimension
                        && *lhsAccessedDimensions.at(1) == SignalValueLookupSwapTest::lhsLockedValueOfSecondDimension
                        && *lhsAccessedDimensions.at(2) == SignalValueLookupSwapTest::lhsLockedValueOfThirdDimension
                        && (!lhsAccessedBitRange.has_value() || modifiedRhsLockedBitRange.isAccessRestrictedTo(*lhsAccessedBitRange)));
            },
            [](const UserDefinedDimensionAccess& lhsAccessedDimensions, const OptionalBitRangeAccess& lhsAccessedBitRange) -> std::optional<unsigned int> { 
                if (!lhsAccessedBitRange.has_value()) 
                    return std::make_optional(SignalValueLookupSwapTest::defaultLhsValue);

                return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupSwapTest::defaultLhsValue, *lhsAccessedBitRange));
            },
            [](const UserDefinedDimensionAccess& rhsAccessedDimensions, const OptionalBitRangeAccess& rhsAccessedBitRange) {
                auto modifiedRhsLockedBitRange = ::optimizations::BitRangeAccessRestriction(SignalValueLookupSwapTest::defaultSignalBitwidth, SignalValueLookupSwapTest::rhsDefaultLockedBitRange);
                modifiedRhsLockedBitRange.liftRestrictionFor(::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupSwapTest::lhsNotLockedAndRhsLockedBitRange.first, SignalValueLookupSwapTest::lhsNotLockedAndRhsLockedBitRange.first));

                return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(rhsAccessedDimensions)
                    || (*rhsAccessedDimensions.at(0) == SignalValueLookupSwapTest::rhsLockedValueOfFirstDimension
                        && *rhsAccessedDimensions.at(1) == SignalValueLookupSwapTest::rhsLockedValueOfSecondDimension
                        && *rhsAccessedDimensions.at(2) == SignalValueLookupSwapTest::rhsLockedValueOfThirdDimension
                        && (!rhsAccessedBitRange.has_value() || modifiedRhsLockedBitRange.isAccessRestrictedTo(*rhsAccessedBitRange)));
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess& rhsAccessedBitRange) -> std::optional<unsigned int> { 
                if (!rhsAccessedBitRange.has_value()) 
                    return std::make_optional(SignalValueLookupSwapTest::defaultRhsValue);

                return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupSwapTest::defaultRhsValue, *rhsAccessedBitRange));
            }),
        std::make_tuple(
            "SwapBitRangeOfFullDimensionAccess",
            []() { return SignalValueLookupSwapTest::createDefaultInitializedLhsSignalValueLookup();  },
            []() { return SignalValueLookupSwapTest::createDefaultInitializedRhsSignalValueLookup(SignalValueLookupSwapTest::SwapRhsOperandSignalSize::FULL, true); },
            [](const SignalValueLookup::ptr& lhsSwapSignalValueLookup, const SignalValueLookup::ptr& rhsSwapSignalValueLookup) {
                lhsSwapSignalValueLookup->swapValuesAndRestrictionsBetween(
                    { SignalValueLookupSwapTest::lhsLockedValueOfFirstDimension, SignalValueLookupSwapTest::lhsLockedValueOfSecondDimension, SignalValueLookupSwapTest::lhsLockedValueOfThirdDimension }, SignalValueLookupSwapTest::lhsDefaultLockedBitRange,
                    { SignalValueLookupSwapTest::rhsLockedValueOfFirstDimension, SignalValueLookupSwapTest::rhsLockedValueOfSecondDimension, SignalValueLookupSwapTest::rhsLockedValueOfThirdDimension }, SignalValueLookupSwapTest::rhsDefaultLockedBitRange,
                *rhsSwapSignalValueLookup);
            },
            [](const UserDefinedDimensionAccess& lhsAccessedDimensions, const OptionalBitRangeAccess&) { 
                return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(lhsAccessedDimensions)
                    || (*lhsAccessedDimensions.at(0) == SignalValueLookupSwapTest::lhsLockedValueOfFirstDimension 
                        && *lhsAccessedDimensions.at(1) == SignalValueLookupSwapTest::lhsLockedValueOfSecondDimension 
                        && *lhsAccessedDimensions.at(2) == SignalValueLookupSwapTest::lhsLockedValueOfThirdDimension);
            },
            [](const UserDefinedDimensionAccess& lhsAccessedDimensions, const OptionalBitRangeAccess& lhsAccessedBitRange) -> std::optional<unsigned int> { 
                if (!lhsAccessedBitRange.has_value()) 
                    return std::make_optional(SignalValueLookupSwapTest::defaultLhsValue);

                return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupSwapTest::defaultLhsValue, *lhsAccessedBitRange));
            },
            [](const UserDefinedDimensionAccess& rhsAccessedDimensions, const OptionalBitRangeAccess& rhsAccessedBitRange) { 
                return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(rhsAccessedDimensions)
                    || (*rhsAccessedDimensions.at(0) == SignalValueLookupSwapTest::rhsLockedValueOfFirstDimension 
                        && *rhsAccessedDimensions.at(1) == SignalValueLookupSwapTest::rhsLockedValueOfSecondDimension 
                        && *rhsAccessedDimensions.at(2) == SignalValueLookupSwapTest::rhsLockedValueOfThirdDimension);
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess& rhsAccessedBitRange) -> std::optional<unsigned int> { 
                if (!rhsAccessedBitRange.has_value()) 
                    return std::make_optional(SignalValueLookupSwapTest::defaultRhsValue);

                return std::make_optional(BaseSignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupSwapTest::defaultRhsValue, *rhsAccessedBitRange));
            })
    ),
    [](const testing::TestParamInfo<SignalValueLookupSwapTest::ParamType>& info) {
        auto testNameToTransform = std::get<0>(info.param);
        std::replace(testNameToTransform.begin(), testNameToTransform.end(), '-', '_');
        return testNameToTransform;
    });