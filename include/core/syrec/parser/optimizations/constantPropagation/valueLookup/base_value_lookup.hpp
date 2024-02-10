#ifndef ABSTRACT_VALUE_LOOKUP_HPP
#define ABSTRACT_VALUE_LOOKUP_HPP
#pragma once

#include "core/syrec/parser/optimizations/constantPropagation/bit_range_access_restriction.hpp"
#include "core/syrec/parser/optimizations/constantPropagation/dimension_propagation_blocker.hpp"
#include "core/syrec/parser/optimizations/constantPropagation/shared_dimension_blocking_information.hpp"
#include "core/syrec/parser/optimizations/constantPropagation/valueLookup/layer_data.hpp"

#include <any>
#include <iterator>
#include <optional>
#include <vector>

namespace valueLookup {
    /*
     * Useful links:
     * https://isocpp.org/wiki/faq/templates#nondependent-name-lookup-members
     */
    template<typename Vt>
    class BaseValueLookup {
    public:
        typedef std::shared_ptr<BaseValueLookup> ptr;

        explicit BaseValueLookup(const unsigned signalBitwidth, const std::vector<unsigned int>& signalDimensions, const std::optional<Vt>& defaultValue):
            signalInformation(signalBitwidth, signalDimensions), dimensionAccessRestrictions(*initializeDimensionAccessRestrictionLayer(0)), valueLookup(*initializeValueLookupLayer(0, defaultValue)) {}

        virtual ~BaseValueLookup() = 0;

        /**
         * \brief Check whether an existing restriction exists that blocks any value lookup
         * \param accessedDimensions The accessed values per dimension, if none is provided for a dimension (std::nullopt), an access on all declared values of the dimension is assumed. It is assumed, that the user provided values per dimension are within range of the declared values per dimension of the accessed signal. For all not accessed dimension, an access on all values per dimension is assumed.
         * \param bitRange The accessed bit range, if none is provided an access on the whole signal bit range is assumed. Furthermore, it is assumed that the user provided bit range is within range of the declared signal bit range.
         * \return Whether an existing restriction for the accessed signal parts exists
         */
        [[nodiscard]] bool isValueLookupBlockedFor(const std::vector<std::optional<unsigned int>>& accessedDimensions, const std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess>& bitRange) const;

        void invalidateStoredValueFor(const std::vector<std::optional<unsigned int>>& accessedDimensions) const;
        void invalidateStoredValueForBitrange(const std::vector<std::optional<unsigned int>>& accessedDimensions, const optimizations::BitRangeAccessRestriction::BitRangeAccess& bitRange) const;
        void invalidateAllStoredValuesForSignal() const;

        void liftRestrictionsFromWholeSignal() const;
        void liftRestrictionsOfDimensions(const std::vector<std::optional<unsigned int>>& accessedDimensions, const std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess>& bitRange) const;

        [[nodiscard]] std::optional<Vt> tryFetchValueFor(const std::vector<std::optional<unsigned int>>& accessedDimensions, const std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess>& bitRange) const;
        void                            updateStoredValueFor(const std::vector<std::optional<unsigned int>>& accessedDimensions, const std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess>& bitRange, const Vt& newValue) const;

        void copyRestrictionsAndUnrestrictedValuesFrom(
            const std::vector<std::optional<unsigned int>>&                                accessedDimensionsOfLhsAssignmentOperand,
            const std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess>& optionallyAccessedBitRangeOfLhsAssignmentOperand,
            const std::vector<std::optional<unsigned int>>&                                accessedDimensionsOfRhsAssignmentOperand,
            const std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess>& optionallyAccessedBitRangeOfRhsAssignmentOperand,
            const BaseValueLookup<Vt>&                                                     valueLookupOfRhsOperand
        );

        // TODO: If a value of a dimension was invalidate once, the entry in the value lookup is removed and can only be restored/recreated by a swap with a known value.
        // The latter is currently not done.
        void swapValuesAndRestrictionsBetween(
                const std::vector<std::optional<unsigned int>>&                                accessedDimensionsOfLhsAssignmentOperand,
                const std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess>& optionallyAccessedBitRangeOfLhsAssignmentOperand,
                const std::vector<std::optional<unsigned int>>&                                accessedDimensionsOfRhsAssignmentOperand,
                const std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess>& optionallyAccessedBitRangeOfRhsAssignmentOperand,
                const BaseValueLookup<Vt>&                                                     other
        );

        [[nodiscard]] optimizations::SignalDimensionInformation    getSignalInformation() const;
        [[nodiscard]] std::vector<std::optional<unsigned int>>     copyDimensionAccessAndPadNotAccessedDimensionWithAccessToAllValuesOfDimension(const std::vector<std::optional<unsigned int>>& actualDimensionAccess) const;
        [[nodiscard]] virtual std::shared_ptr<BaseValueLookup<Vt>> clone() = 0;
        void                                                       copyRestrictionsAndMergeValuesFromAlternatives(const BaseValueLookup::ptr& alternativeOne, const BaseValueLookup::ptr& alternativeTwo);
        void                                                       copyRestrictionsAndInvalidateChangedValuesFrom(const BaseValueLookup::ptr& other);

    protected:
        const optimizations::SignalDimensionInformation                             signalInformation;
        const LayerData<optimizations::DimensionPropagationBlocker::ptr>::ptr       dimensionAccessRestrictions;
        const std::shared_ptr<LayerData<std::map<unsigned int, std::optional<Vt>>>> valueLookup;
        
        [[nodiscard]] std::optional<LayerData<optimizations::DimensionPropagationBlocker::ptr>::ptr>       initializeDimensionAccessRestrictionLayer(const unsigned int dimension);
        [[nodiscard]] std::optional<std::shared_ptr<LayerData<std::map<unsigned int, std::optional<Vt>>>>> initializeValueLookupLayer(const unsigned int dimension, const std::optional<Vt>& defaultValue);

        [[nodiscard]] std::optional<std::vector<unsigned int>> transformAccessOnDimensions(const std::vector<std::optional<unsigned int>>& accessedDimensions) const;
        /*
         * To have both static and dynamic polymorphism we need to use type erase to keep the dynamic part here
         * https://www.artima.com/articles/on-the-tension-between-object-oriented-and-generic-programming-in-c
         */
        
        [[nodiscard]] virtual bool     canStoreValue(const std::any& value, const optimizations::BitRangeAccessRestriction::BitRangeAccess& availableStorageSpace) const                                                      = 0;
        [[nodiscard]] virtual std::any extractPartsOfValue(const std::any& value, const optimizations::BitRangeAccessRestriction::BitRangeAccess& availableStorageSpace) const                                                = 0;
        [[nodiscard]] virtual std::any transformExistingValueByMergingWithNewOne(const std::any& currentValue, const std::any& newValue, const optimizations::BitRangeAccessRestriction::BitRangeAccess& partsToUpdate) const = 0;
        [[nodiscard]] virtual std::any wrapValueOnOverflow(const std::any& value, unsigned int numBitsOfStorage) const                                                                                                        = 0;                                                                                             

        template<typename Fn>
        void applyToBitsOfLastLayer(const std::vector<std::optional<unsigned int>>& accessedDimensions, const std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess>& accessedBitRange, Fn&& applyLambda);

        template<typename Fn>
        void applyToLastLayerBitsWithRelativeDimensionAccessInformation(std::size_t currentDimension, const std::vector<std::optional<unsigned int>>& accessedDimensions, std::size_t numAccessedBits, std::vector<unsigned int>& relativeDimensionAccess, Fn&& applyLambda);

        template<typename Fn>
        void applyToBitsOfLastLayerWithRelativeDimensionAccessInformationStartingFrom(std::size_t dimensionToStartFrom, std::size_t currentDimension, const std::vector<std::optional<unsigned int>>& accessedDimensions, std::size_t numAccessedBits, std::vector<unsigned int>& relativeDimensionAccess, Fn&& applyLambda);

        static void                                     mergeActualWithRelativeDimensionAccess(std::vector<std::optional<unsigned int>>& dimensionAccessContainer, std::size_t offsetToRelativeDimensionInOriginalDimensionAccess, const std::vector<unsigned int>& relativeDimensionAccess);
        [[nodiscard]] static std::vector<std::optional<unsigned int>> transformRelativeToActualDimensionAccess(const std::vector<unsigned int>& relativeDimensionAccess);
    };
    /*
     * Since we would also like the split into definition/implementation for this templated class,
     * we will use a .tpp file to provide the implementation for the latter:
     * https://stackoverflow.com/questions/44774036/why-use-a-tpp-file-when-implementing-templated-functions-and-classes-defined-i
     *
     * Otherwise the implementation of the template must be provided in the header file to be
     * available for all compilation units:
     * https://stackoverflow.com/questions/495021/why-can-templates-only-be-implemented-in-the-header-file
     */
    #include "core/syrec/parser/optimizations/constantPropagation/valueLookup/base_value_lookup.tpp"
}; // namespace valueLookup
#endif