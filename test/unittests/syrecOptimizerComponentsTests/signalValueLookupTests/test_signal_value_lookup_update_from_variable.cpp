#include "test_signal_value_lookup.hpp"

using namespace valueLookup;

using UserDefinedDimensionAccess = std::vector<std::optional<unsigned int>>;
using OptionalBitRangeAccess     = std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess>;

// TODO: Test cases for lifting unrelated restrictions
INSTANTIATE_TEST_SUITE_P(
    SignalValueLookupTests,
    SignalValueLookupTest,
    testing::Values(
        std::make_tuple(
            "LockEmptyWholeSignalByAssigningUnknownValueOfSameSize",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                const auto& other = SignalValueLookupTest::createZeroedSignalValueLookup();
                other->invalidateAllStoredValuesForSignal();
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                    *other);
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return true; },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) -> std::optional<unsigned int> { return std::nullopt; }),
        std::make_tuple(
            "LockEmptySignalByAssigningUnknownValueToValueOfDimension",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                const auto& other = SignalValueLookupTest::createZeroedSignalValueLookup();
                other->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension, SignalValueLookupTest::lockedValueOfThirdDimension});
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                    *other
                );
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess&) {
                return !SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || *accessedDimensions.front() != SignalValueLookupTest::lockedValueOfFirstDimension
                    || *accessedDimensions.at(1) != SignalValueLookupTest::lockedValueOfSecondDimension
                    || *accessedDimensions.at(2) != SignalValueLookupTest::lockedValueOfThirdDimension;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions) 
                    || *accessedDimensions.front() != SignalValueLookupTest::lockedValueOfFirstDimension 
                    || *accessedDimensions.at(1) != SignalValueLookupTest::lockedValueOfSecondDimension 
                    || *accessedDimensions.at(2) != SignalValueLookupTest::lockedValueOfThirdDimension) {
                    return std::nullopt;
                }

                if (accessedBitRange.has_value()) {
                    return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),
        std::make_tuple(
            "LockEmptySignalByAssigningUnknownValueToGlobalBit",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                const auto& other = SignalValueLookupTest::createZeroedSignalValueLookup();
                other->invalidateStoredValueForBitrange({}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                    *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                return !SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || !accessedBitRange.has_value() 
                    || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions) 
                    || !accessedBitRange.has_value() 
                    || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange))) {
                    return std::nullopt;
                }
                return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
            }),
        std::make_tuple(
            "LockEmptySignalByAssigningUnknownValueToGlobalBitRange",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                const auto& other = SignalValueLookupTest::createZeroedSignalValueLookup();
                other->invalidateStoredValueForBitrange({}, SignalValueLookupTest::defaultBlockedBitRange);
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt, 
                    {}, std::nullopt,
                    *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                return !SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || !accessedBitRange.has_value()
                    || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions) || !accessedBitRange.has_value() || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange)) {
                    return std::nullopt;
                }
                return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
            }),
        std::make_tuple(
            "LockEmptySignalByAssigningWithIncompleteDimensionAccess",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                const auto& other = SignalValueLookupTest::createZeroedSignalValueLookup();
                other->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, std::nullopt});
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                    *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess&) {
                return !SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || *accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions) || *accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension) {
                    return std::nullopt;
                }

                if (accessedBitRange.has_value()) {
                    return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),
        std::make_tuple(
            "LockEmptySignalByAssigningWithIncompleteBitDimensionAccess",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                const auto& other = SignalValueLookupTest::createZeroedSignalValueLookup();
                other->invalidateStoredValueForBitrange({SignalValueLookupTest::lockedValueOfFirstDimension, std::nullopt}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                    *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                return !SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension 
                        && (!accessedBitRange.has_value() 
                            || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange))));
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions) 
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension 
                        && (!accessedBitRange.has_value() 
                            || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange))))) {
                    return std::nullopt;
                }

                if (accessedBitRange.has_value()) {
                    return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),
        std::make_tuple(
            "LockEmptySignalByAssigningWithIncompleteBitRangeDimensionAccess",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                const auto& other = SignalValueLookupTest::createZeroedSignalValueLookup();
                other->invalidateStoredValueForBitrange({SignalValueLookupTest::lockedValueOfFirstDimension, std::nullopt}, SignalValueLookupTest::defaultBlockedBitRange);
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                        {}, std::nullopt,
                        {}, std::nullopt,
                        *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                return !SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension 
                        && (!accessedBitRange.has_value() 
                            || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange)));
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions) 
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension 
                        && (!accessedBitRange.has_value() 
                            || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange)))) {
                    return std::nullopt;
                }

                if (accessedBitRange.has_value()) {
                    return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),

        std::make_tuple(
            "ExtendLockOfGlobalBitByLockingWholeSignal",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                const auto& other = SignalValueLookupTest::createZeroedSignalValueLookup();
                other->invalidateStoredValueForBitrange({}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                    *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions) || !accessedBitRange.has_value()) {
                    return false;
                }
                return SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions) || !accessedBitRange.has_value()){
                    return std::nullopt;
                }

                return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
            }),
        std::make_tuple(
            "ExtendLockOfGlobalBitByLockingAnotherGlobalBitOfSignal",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                const auto& other = SignalValueLookupTest::createZeroedSignalValueLookup();
                signalValueLookup->invalidateStoredValueForBitrange({}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));
                other->invalidateStoredValueForBitrange({}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::lastBlockedBitOfDefaultBitRange, SignalValueLookupTest::lastBlockedBitOfDefaultBitRange));
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                    *other
                );
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) { 
                return !SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions) 
                    || !accessedBitRange.has_value() 
                    || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange))
                    || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::lastBlockedBitOfDefaultBitRange, SignalValueLookupTest::lastBlockedBitOfDefaultBitRange));
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions) 
                    || !accessedBitRange.has_value()
                    || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange))
                    || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::lastBlockedBitOfDefaultBitRange, SignalValueLookupTest::lastBlockedBitOfDefaultBitRange))) {
                    return std::nullopt;
                }
                return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
            }),
         std::make_tuple(
            "ExtendLockOfGlobalBitByLockingGlobalBitRangeOfSignal",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                const auto& other = SignalValueLookupTest::createZeroedSignalValueLookup();
                signalValueLookup->invalidateStoredValueForBitrange({}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));
                other->invalidateStoredValueForBitrange({}, SignalValueLookupTest::defaultBlockedBitRange);
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                    *other
                );
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) { 
                return !SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions) 
                    || !accessedBitRange.has_value()
                    || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions) 
                    || !accessedBitRange.has_value()
                    || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange)) {
                        return std::nullopt;
                    }
                return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
             }),
         std::make_tuple(
            "ExtendLockOfGlobalBitByLockingValueOfDimension",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                const auto& other = SignalValueLookupTest::createZeroedSignalValueLookup();
                signalValueLookup->invalidateStoredValueForBitrange({}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));
                other->invalidateStoredValueFor({ SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension, SignalValueLookupTest::lockedValueOfThirdDimension });
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                    *other
                );
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                return !SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || !accessedBitRange.has_value()
                    || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange))
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension
                        && *accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension
                        && *accessedDimensions.at(2) == SignalValueLookupTest::lockedValueOfThirdDimension);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                || !accessedBitRange.has_value()
                    || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange))
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension
                        && *accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension
                        && *accessedDimensions.at(2) == SignalValueLookupTest::lockedValueOfThirdDimension)){
                            return std::nullopt;
                }
                return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
            }),
        std::make_tuple(
            "ExtendLockOfGlobalBitByLockingBitOfSubdimension",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                signalValueLookup->invalidateStoredValueForBitrange({}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));
                const auto& other = SignalValueLookupTest::createZeroedSignalValueLookup();
                other->invalidateStoredValueForBitrange({ SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension }, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::lastBlockedBitOfDefaultBitRange, SignalValueLookupTest::lastBlockedBitOfDefaultBitRange));
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                { SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension }, std::nullopt,
                { SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension }, std::nullopt,
                *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) { 
                return !SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension && *accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension)
                    || !accessedBitRange.has_value()
                    || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange))
                    || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::lastBlockedBitOfDefaultBitRange, SignalValueLookupTest::lastBlockedBitOfDefaultBitRange)); 
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension && *accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension)
                    || !accessedBitRange.has_value()
                    || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange))
                    || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::lastBlockedBitOfDefaultBitRange, SignalValueLookupTest::lastBlockedBitOfDefaultBitRange))){
                        return std::nullopt; 
                    }

                if (accessedBitRange.has_value()) {
                    return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),
        std::make_tuple(
            "ExtendLockOfGlobalBitByLockingBitRangeOfSubdimension",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                signalValueLookup->invalidateStoredValueForBitrange({}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));
                const auto& other = SignalValueLookupTest::createZeroedSignalValueLookup();
                other->invalidateStoredValueForBitrange({ SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension }, SignalValueLookupTest::defaultNonOverlappingBitRange);
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                { SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension }, std::nullopt,
                { SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension }, std::nullopt,
                *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) { 
                return !SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension && *accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension)
                    || !accessedBitRange.has_value()
                    || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange))
                    || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultNonOverlappingBitRange); 
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension && *accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension)
                    || !accessedBitRange.has_value()
                    || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange))
                    || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultNonOverlappingBitRange)){
                        return std::nullopt; 
                    }

                if (accessedBitRange.has_value()) {
                    return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),
        std::make_tuple(
            "ExtendLockOfGlobalBitByLockingValueOfIncompleteSubdimensionAccess",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                SignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                const auto& other = SignalValueLookupTest::createZeroedSignalValueLookup();
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { return std::nullopt; }),

        std::make_tuple(
            "ExtendLockOfGlobalBitRangeByLockingWholeSignal",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                /*
                 * TODO: On could also test assigning restrictions from one dimension to another or copying parts of a completely locked signal, etc.
                 */
                signalValueLookup->invalidateStoredValueForBitrange({}, SignalValueLookupTest::defaultBlockedBitRange);
                const auto& other = SignalValueLookupTest::createZeroedSignalValueLookup();
                other->invalidateAllStoredValuesForSignal();
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                *other);
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) -> std::optional<unsigned int> { return std::nullopt; }),
        std::make_tuple(
            "ExtendLockOfGlobalBitRangeByLockingAnotherGlobalBitOfSignal",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                signalValueLookup->invalidateStoredValueForBitrange({}, SignalValueLookupTest::defaultBlockedBitRange);
                const auto& other = SignalValueLookupTest::createZeroedSignalValueLookup();
                other->invalidateStoredValueForBitrange({}, SignalValueLookupTest::defaultNonOverlappingBitRange);
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                    *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) { 
                return !SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || accessedBitRange.has_value()
                    || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange)
                    || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultNonOverlappingBitRange); 
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || accessedBitRange.has_value()
                    || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange)
                    || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultNonOverlappingBitRange)){
                        return std::nullopt;
                } 
                if (accessedBitRange.has_value()) {
                    return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),
        std::make_tuple(
            "ExtendLockOfGlobalBitRangeByLockingOverlappingGlobalBitOfSignal",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                signalValueLookup->invalidateStoredValueForBitrange({}, SignalValueLookupTest::defaultBlockedBitRange);
                const auto& other = SignalValueLookupTest::createZeroedSignalValueLookup();
                other->invalidateStoredValueForBitrange({}, SignalValueLookupTest::defaultOverlappingBitRange);
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                    *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) { 
                return !SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || accessedBitRange.has_value()
                    || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange)
                    || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultOverlappingBitRange); 
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || accessedBitRange.has_value()
                    || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange)
                    || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultOverlappingBitRange)){
                        return std::nullopt;
                } 
                if (accessedBitRange.has_value()) {
                    return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),
        std::make_tuple(
            "ExtendLockOfGlobalBitRangeByLockingValueOfDimension",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                signalValueLookup->invalidateStoredValueForBitrange({}, SignalValueLookupTest::defaultBlockedBitRange);
                const auto& other = SignalValueLookupTest::createZeroedSignalValueLookup();
                other->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension});
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                    *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) { 
                return !SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension 
                        && SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange))
                    || !accessedBitRange.has_value(); 
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension 
                        && SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange))
                    || !accessedBitRange.has_value()){
                    return std::nullopt;
                }

                return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
            }),
        std::make_tuple(
            "ExtendLockOfGlobalBitRangeByLockingBitOfSubdimension",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                SignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                const auto& other = SignalValueLookupTest::createZeroedSignalValueLookup();
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { return std::nullopt; }),
        std::make_tuple(
            "ExtendLockOfGlobalBitRangeByLockingNonOverlappingBitRangeOfSubdimension",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                SignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                const auto& other = SignalValueLookupTest::createZeroedSignalValueLookup();
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { return std::nullopt; }),
        std::make_tuple(
            "ExtendLockOfGlobalBitRangeByLockingOverlappingBitRangeOfSubdimension",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                SignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                const auto& other = SignalValueLookupTest::createZeroedSignalValueLookup();
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { return std::nullopt; }),
        std::make_tuple(
            "ExtendLockOfGlobalBitRangeByLockingBitOfIncompleteSubdimensionAccess",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                SignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                const auto& other = SignalValueLookupTest::createZeroedSignalValueLookup();
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { return std::nullopt; }),
        std::make_tuple(
            "ExtendLockOfGlobalBitRangeByLockingNonOverlappingBitRangeOfIncompleteSubdimensionAccess",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                SignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                const auto& other = SignalValueLookupTest::createZeroedSignalValueLookup();
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { return std::nullopt; }),
        std::make_tuple(
            "ExtendLockOfGlobalBitRangeByLockingOverlappingBitRangeOfIncompleteSubdimensionAccess",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                SignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                const auto& other = SignalValueLookupTest::createZeroedSignalValueLookup();
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { return std::nullopt; }),
        std::make_tuple(
            "ExtendLockOfGlobalBitRangeByLockingValueOfIncompleteSubdimensionAccess",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                SignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                const auto& other = SignalValueLookupTest::createZeroedSignalValueLookup();
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { return std::nullopt; }),

        std::make_tuple(
            "ExtendLockOfValueOfDimensionByLockingWholeSignal",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                signalValueLookup->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension});
                const auto& other = SignalValueLookupTest::createZeroedSignalValueLookup();
                other->invalidateAllStoredValuesForSignal();
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                *other);
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) -> std::optional<unsigned int> { return std::nullopt; }),
        std::make_tuple(
            "ExtendLockOfValueOfDimensionByBitGlobally",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                signalValueLookup->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension});
                const auto& other = SignalValueLookupTest::createZeroedSignalValueLookup();
                other->invalidateStoredValueForBitrange({}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) { 
                return !SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || !accessedBitRange.has_value()
                    || accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange)); 
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || !accessedBitRange.has_value()
                    || *accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension
                    || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange))){
                        return std::nullopt; 
                    }

                    return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
            }),
        std::make_tuple(
            "ExtendLockOfValueOfDimensionByBitRangeGlobally",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                signalValueLookup->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension});
                const auto& other = SignalValueLookupTest::createZeroedSignalValueLookup();
                other->invalidateStoredValueForBitrange({}, SignalValueLookupTest::defaultBlockedBitRange);
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) { 
                return !SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || !accessedBitRange.has_value()
                    || *accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange); 
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || !accessedBitRange.has_value()
                    || *accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension
                    || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange)){
                        return std::nullopt; 
                    }

                    return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
            }),
        std::make_tuple(
            "ExtendLockOfValueOfDimensionByAnotherValueOfSameDimension",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                signalValueLookup->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension});
                const auto& other = SignalValueLookupTest::createZeroedSignalValueLookup();
                other->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfSecondDimension});
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) { 
                return !SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || *accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension
                    || *accessedDimensions.front() == SignalValueLookupTest::lockedValueOfSecondDimension; 
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || *accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension
                    || *accessedDimensions.front() == SignalValueLookupTest::lockedValueOfSecondDimension){
                    return std::nullopt; 
                }

                if (accessedBitRange.has_value()){
                    return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),
        std::make_tuple(
            "ExtendLockOfValueOfDimensionByValueOfOtherDimension",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                signalValueLookup->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension});
                const auto& other = SignalValueLookupTest::createZeroedSignalValueLookup();
                other->invalidateStoredValueFor({std::nullopt, SignalValueLookupTest::lockedValueOfSecondDimension});
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess&) { 
                return !SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || *accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension
                    || *accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension; 
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || *accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension
                    || *accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension){
                    return std::nullopt; 
                }
                if (accessedBitRange.has_value()){
                    return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),
        std::make_tuple(
            "ExtendLockOfValueOfDimensionByIncompleteValueOfDimensionAccess",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                signalValueLookup->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension});
                const auto& other = SignalValueLookupTest::createZeroedSignalValueLookup();
                other->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfSecondDimension});
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess&) { 
                return !SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || *accessedDimensions.front() == SignalValueLookupTest::lockedValueOfSecondDimension
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension && *accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension); 
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || *accessedDimensions.front() == SignalValueLookupTest::lockedValueOfSecondDimension
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension && *accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension)){
                    return std::nullopt; 
                }
                if (accessedBitRange.has_value()){
                    return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),
        std::make_tuple(
            "ExtendLockOfValueOfDimensionByIncompleteBitOfDimensionAccess",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                signalValueLookup->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension});
                const auto& other = SignalValueLookupTest::createZeroedSignalValueLookup();
                other->invalidateStoredValueForBitrange({SignalValueLookupTest::lockedValueOfSecondDimension}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) { 
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension && *accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension)){
                    return true;
                }

                if (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfSecondDimension){
                    return accessedBitRange.has_value() 
                        ? SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange)) 
                        : false;
                }
                return false; 
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension && *accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension)){
                    return std::nullopt;
                }

                if (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfSecondDimension){
                    if (!accessedBitRange.has_value() || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange))){
                        return std::nullopt;
                    }
                }

                if (accessedBitRange.has_value()){
                    return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),
        std::make_tuple(
            "ExtendLockOfValueOfDimensionByIncompleteBitRangeOfDimensionAccess",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                signalValueLookup->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension});
                const auto& other = SignalValueLookupTest::createZeroedSignalValueLookup();
                other->invalidateStoredValueForBitrange({SignalValueLookupTest::lockedValueOfSecondDimension}, SignalValueLookupTest::defaultBlockedBitRange);
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) { 
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension && *accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension)){
                    return true;
                }

                if (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfSecondDimension){
                    return accessedBitRange.has_value() ? SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange) : false;
                }
                return false; 
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension && *accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension)){
                    return std::nullopt;
                }

                if (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfSecondDimension){
                    if (!accessedBitRange.has_value() || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange)){
                        return std::nullopt;
                    }
                }

                if (accessedBitRange.has_value()){
                    return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),
        std::make_tuple(
            "ExtendLockOfValueOfDimensionByLockingValueOfParentDimension",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                signalValueLookup->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension});
                const auto& other = SignalValueLookupTest::createZeroedSignalValueLookup();
                other->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfSecondDimension});
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess&) { 
                return !SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || *accessedDimensions.front() == SignalValueLookupTest::lockedValueOfSecondDimension
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension && *accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension); 
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || *accessedDimensions.front() == SignalValueLookupTest::lockedValueOfSecondDimension
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension && *accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension)){
                    return std::nullopt; 
                }

                if (accessedBitRange.has_value()){
                    return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),   
        std::make_tuple(
            "ExtendLockOfValueOfDimensionByLockingBitOfParentDimension",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                signalValueLookup->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension});
                const auto& other = SignalValueLookupTest::createZeroedSignalValueLookup();
                other->invalidateStoredValueForBitrange({SignalValueLookupTest::lockedValueOfSecondDimension}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) { 
                 return !SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfSecondDimension && (accessedBitRange.has_value() ? SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange)) : true))
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension && *accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension); 
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfSecondDimension && (accessedBitRange.has_value() ? SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange)) : true))
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension && *accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension)){
                    return std::nullopt; 
                }

                if (accessedBitRange.has_value()){
                    return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),
        std::make_tuple(
            "ExtendLockOfValueOfDimensionByLockingBitRangeOfParentDimension",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                signalValueLookup->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension});
                const auto& other = SignalValueLookupTest::createZeroedSignalValueLookup();
                other->invalidateStoredValueForBitrange({SignalValueLookupTest::lockedValueOfSecondDimension}, SignalValueLookupTest::defaultBlockedBitRange);
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) { 
                return !SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfSecondDimension && (accessedBitRange.has_value() ? SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange) : true))
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension && *accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension); 
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfSecondDimension && (accessedBitRange.has_value() ? SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange) : true))
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension && *accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension)){
                    return std::nullopt; 
                }

                if (accessedBitRange.has_value()){
                    return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),

        std::make_tuple(
            "ExtendLockOfBitOfValueOfDimensionByLockingWholeSignal",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                signalValueLookup->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension});
                const auto& other = SignalValueLookupTest::createZeroedSignalValueLookup();
                other->invalidateAllStoredValuesForSignal();
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                *other);
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return true; },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) -> std::optional<unsigned int> { return std::nullopt; }),

        std::make_tuple(
            "ExtendLockOfBitOfValueOfDimensionByLockingAnotherGlobalBit",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                signalValueLookup->invalidateStoredValueForBitrange({SignalValueLookupTest::lockedValueOfFirstDimension}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));
                const auto& other = SignalValueLookupTest::createZeroedSignalValueLookup();
                other->invalidateStoredValueForBitrange({}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::lastBlockedBitOfDefaultBitRange, SignalValueLookupTest::lastBlockedBitOfDefaultBitRange));
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) { 
                return !SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || !accessedBitRange.has_value()
                    || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange))
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension && SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::lastBlockedBitOfDefaultBitRange, SignalValueLookupTest::lastBlockedBitOfDefaultBitRange)));
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || !accessedBitRange.has_value()
                    || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange))
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension && SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::lastBlockedBitOfDefaultBitRange, SignalValueLookupTest::lastBlockedBitOfDefaultBitRange)))){
                    return std::nullopt; 
                }

                if (accessedBitRange.has_value()){
                    return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),

        std::make_tuple(
            "ExtendLockOfBitOfValueOfDimensionByLockingGlobalBitRange",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                signalValueLookup->invalidateStoredValueForBitrange({SignalValueLookupTest::lockedValueOfFirstDimension}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));
                const auto& other = SignalValueLookupTest::createZeroedSignalValueLookup();
                other->invalidateStoredValueForBitrange({}, SignalValueLookupTest::defaultNonOverlappingBitRange);
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) { 
                return !SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || !accessedBitRange.has_value()
                    || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange))
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension && SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange));
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || !accessedBitRange.has_value()
                    || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange))
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension && SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange))){
                    return std::nullopt; 
                }

                if (accessedBitRange.has_value()){
                    return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),

        std::make_tuple(
            "ExtendLockOfBitOfValueOfDimensionByLockingBitRangesInBothTheSameAndOtherDimension",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                signalValueLookup->invalidateStoredValueForBitrange({SignalValueLookupTest::lockedValueOfFirstDimension}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));
                const auto& other = SignalValueLookupTest::createZeroedSignalValueLookup();
                other->invalidateStoredValueForBitrange({SignalValueLookupTest::lockedValueOfFirstDimension}, SignalValueLookupTest::defaultBlockedBitRange);
                other->invalidateStoredValueForBitrange({SignalValueLookupTest::lockedValueOfSecondDimension, SignalValueLookupTest::lockedValueOfFirstDimension}, SignalValueLookupTest::defaultNonOverlappingBitRange);
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) { 
                if (SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)){
                    return true;
                }

                if (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension){
                    return !accessedBitRange.has_value()
                        || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange))
                        || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange);
                }
                if (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfSecondDimension && *accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfFirstDimension){
                    return !accessedBitRange.has_value() || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultNonOverlappingBitRange);
                }
                return false;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                if (SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)){
                    return std::nullopt;
                }

                if (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension
                    && (!accessedBitRange.has_value()
                        || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange))
                        || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange))){
                    return std::nullopt;
                }
                if (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfSecondDimension 
                    && *accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfFirstDimension
                    && (!accessedBitRange.has_value() || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultNonOverlappingBitRange))){
                    return std::nullopt;
                }

                if (accessedBitRange.has_value()){
                    return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),

        std::make_tuple(
            "ExtendLockOfBitOfValueOfDimensionByLockingBitAndBitRangesInParentDimension",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                signalValueLookup->invalidateStoredValueForBitrange({SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));
                const auto& other = SignalValueLookupTest::createZeroedSignalValueLookup();
                other->invalidateStoredValueForBitrange({0}, SignalValueLookupTest::defaultBlockedBitRange);
                other->invalidateStoredValueForBitrange({SignalValueLookupTest::lockedValueOfSecondDimension}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::lastBlockedBitOfDefaultBitRange, SignalValueLookupTest::lastBlockedBitOfDefaultBitRange));
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) { 
                if (SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)){
                    return true;
                }

                if (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension && *accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension){
                    return !accessedBitRange.has_value()
                        || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));
                }
                if (*accessedDimensions.front() == 0) {
                    return !accessedBitRange.has_value()
                        || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange);
                }
                if (accessedDimensions.front() == SignalValueLookupTest::lockedValueOfSecondDimension){
                    return !accessedBitRange.has_value()
                        || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::lastBlockedBitOfDefaultBitRange, SignalValueLookupTest::lastBlockedBitOfDefaultBitRange));
                }
                return false;
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                if (SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)){
                    return std::nullopt;
                }

                if (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension && *accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension 
                    && (!accessedBitRange.has_value() || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange)))) {
                    return std::nullopt;
                }
                if (*accessedDimensions.front() == 0 && 
                    (!accessedBitRange.has_value() || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange))){
                    return std::nullopt;
                }
                if (accessedDimensions.front() == SignalValueLookupTest::lockedValueOfSecondDimension && 
                    (!accessedBitRange.has_value()
                        || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::lastBlockedBitOfDefaultBitRange, SignalValueLookupTest::lastBlockedBitOfDefaultBitRange)))){
                            return std::nullopt;
                }

                if (accessedBitRange.has_value()){
                    return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),


        std::make_tuple(
            "CopyingBitFromValueOfOtherDimensionIsDoneCorrectly",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                const auto& other = SignalValueLookupTest::createZeroedSignalValueLookup();
                other->invalidateStoredValueForBitrange({SignalValueLookupTest::lockedValueOfFirstDimension}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {SignalValueLookupTest::lockedValueOfFirstDimension + 1}, std::nullopt,
                    {SignalValueLookupTest::lockedValueOfFirstDimension}, std::nullopt,
                *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) { 
                return !SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || (*accessedDimensions.front() == (SignalValueLookupTest::lockedValueOfFirstDimension + 1) && (accessedBitRange.has_value() ? SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange)) : true));
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)){
                    return std::nullopt;
                }

                if (*accessedDimensions.front() == (SignalValueLookupTest::lockedValueOfFirstDimension + 1)){
                    if (!accessedBitRange.has_value() || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange))){
                        return std::nullopt;
                    }
                }

                if (accessedBitRange.has_value()){
                    return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),

        std::make_tuple(
            "CopyingBitRangesFromValueOfOtherDimensionIsDoneCorrectly",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                const auto& other = SignalValueLookupTest::createZeroedSignalValueLookup();
                other->invalidateStoredValueForBitrange({SignalValueLookupTest::lockedValueOfFirstDimension}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(3, 5));
                other->invalidateStoredValueForBitrange({0}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(0, 2));
                other->invalidateStoredValueForBitrange({}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(0, 0));
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {SignalValueLookupTest::lockedValueOfFirstDimension - 1}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(0, 2),
                    {SignalValueLookupTest::lockedValueOfFirstDimension}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(3, 5),
                *other);

                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {SignalValueLookupTest::lockedValueOfFirstDimension - 1}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(3, 5),
                    {0}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(0, 2),
                *other);

                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {SignalValueLookupTest::lockedValueOfFirstDimension - 1}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::defaultSignalBitwidth - 1, SignalValueLookupTest::defaultSignalBitwidth - 1),
                    {0}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(0, 0),
                *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) { 
                if (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions) || !accessedBitRange.has_value()){
                    return true;
                }

                return (!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions) || !accessedBitRange.has_value())
                    || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::defaultSignalBitwidth - 1, SignalValueLookupTest::defaultSignalBitwidth - 1))
                    || (*accessedDimensions.front() == (SignalValueLookupTest::lockedValueOfFirstDimension - 1) 
                        && (SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(0, 2))
                            || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(3, 5))
                            || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::defaultSignalBitwidth - 1, SignalValueLookupTest::defaultSignalBitwidth - 1))));
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                if ((!SignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions) || !accessedBitRange.has_value())
                    || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::defaultSignalBitwidth - 1, SignalValueLookupTest::defaultSignalBitwidth - 1))
                    || (*accessedDimensions.front() == (SignalValueLookupTest::lockedValueOfFirstDimension - 1) 
                        && (SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(0, 2))
                            || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(3, 5))
                            || SignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::defaultSignalBitwidth - 1, SignalValueLookupTest::defaultSignalBitwidth - 1))))) {
                    return std::nullopt;
                }

                if (accessedBitRange.has_value()){
                    return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue); 
            })
    ),

    /*
    * Test to copy from a signal that is blocked completely, either a bit / bit range / value of dimension and check that it is correctly recreated in the other signal
    */

    [](const testing::TestParamInfo<SignalValueLookupTest::ParamType>& info) {
        auto testNameToTransform = std::get<0>(info.param);
        std::replace(testNameToTransform.begin(), testNameToTransform.end(), '-', '_');
        return testNameToTransform;
    });