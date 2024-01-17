#ifndef EXPRESSION_SUBSTITUTION_GENERATOR_HPP
#define EXPRESSION_SUBSTITUTION_GENERATOR_HPP
#pragma once

#include "core/syrec/parser/symbol_table.hpp"
#include "core/syrec/variable.hpp"

#include <optional>

// TODO: This implementation does probably not work during function unrolling since only the symbol table entries of the current scope are considered.
namespace noAdditionalLineSynthesis {
    class ExpressionSubstitutionGenerator {
    public:
        using ptr = std::unique_ptr<ExpressionSubstitutionGenerator>;

        explicit ExpressionSubstitutionGenerator(parser::SymbolTable::ptr symbolTable, const std::string& optionalUserProvidedNewCandidateSignalIdentPrefix):
            symbolTable(std::move(symbolTable)), lastGeneratedReplacementCandidateCounter(0) {
            if (optionalUserProvidedNewCandidateSignalIdentPrefix.empty() || optionalUserProvidedNewCandidateSignalIdentPrefix.rfind("__") == std::string::npos) {
                generatedReplacementCandidatePrefix = DEFAULT_GENERATED_REPLACEMENT_CANDIDATE_PREFIX;   
            }
            else {
                generatedReplacementCandidatePrefix = optionalUserProvidedNewCandidateSignalIdentPrefix;
            }
        }

        struct ReplacementResult {
            syrec::VariableAccess::ptr                 foundReplacement;
            std::optional<syrec::AssignStatement::ptr> requiredResetOfReplacement;
            bool                                       doesGeneratedReplacementReferenceExistingSignal;
        };

        // TODO: Return type could be rework to bool to give the user a chance to handle not creatable restrictions during initialization
        void                                           redefineNotUsableReplacementCandidates(const std::vector<syrec::VariableAccess::ptr>& notUsableReplacementCandidates);
        void                                           activateTemporaryRestrictions(const std::vector<syrec::VariableAccess::ptr>& temporaryNotUsableReplacementCandidates);
        void                                           clearAllRestrictions() const;
        void                                           resetInternals();
        // TODO: How does a generated reset of a found replacement effect an existing data flow analysis result (should the reset function as a restriction, etc.)
        [[nodiscard]] std::optional<ReplacementResult> generateReplacementFor(unsigned int requiredBitwidthOfReplacement);
        [[nodiscard]] syrec::Variable::vec             getDefinitionsForGeneratedReplacements() const;

    protected:
        using AccessedValueOfDimension = std::optional<std::size_t>;
        struct DimensionReplacementStatus {
            using ptr = std::shared_ptr<DimensionReplacementStatus>;

            std::size_t declaredValuesOfDimension;
            std::size_t lastLoadedValueOfDimension;
            bool        isIntermediateDimension;

            struct LeafLayerData {
                using ptr = std::shared_ptr<LeafLayerData>;

                optimizations::BitRangeAccessRestriction::ptr permanentRestrictions;
                optimizations::BitRangeAccessRestriction::ptr temporaryRestrictions;

                void               clearAllTemporaryRestrictions() const;
                void               activatePermanentRestriction(const optimizations::BitRangeAccessRestriction::BitRangeAccess& restriction) const;
                void               activateTemporaryRestriction(const optimizations::BitRangeAccessRestriction::BitRangeAccess& restriction) const;
                [[nodiscard]] bool isAccessRestrictedToBitrange(const optimizations::BitRangeAccessRestriction::BitRangeAccess& bitRange) const;
            };

            struct LayerDataEntry {
                using ptr = std::shared_ptr<LayerDataEntry>;

                std::size_t                                                       referencedValueOfDimension;
                std::variant<DimensionReplacementStatus::ptr, LeafLayerData::ptr> data;

                [[nodiscard]] std::optional<DimensionReplacementStatus::ptr> getIntermediateLayerData() const;
                [[nodiscard]] std::optional<LeafLayerData::ptr>              getLeafLayerData() const;
            };

            std::vector<LayerDataEntry::ptr> dataPerValueOfDimension;
            std::unordered_set<std::size_t>  alreadyLoadedValuesOfDimension;

            [[nodiscard]] std::optional<DimensionReplacementStatus::ptr> getIntermediateLayerDataForValueOfDimension(std::size_t valueOfDimension) const;
            [[nodiscard]] std::optional<LeafLayerData::ptr>              getLeafLayerDataForValueOfDimension(std::size_t valueOfDimension) const;
            [[nodiscard]] bool                                           wasValueOfDimensionAlreadyLoaded(std::size_t valueOfDimension) const;
            [[nodiscard]] std::optional<LayerDataEntry::ptr>             getDataOfValueOfDimension(std::size_t valueOfDimension) const;
            void                                                         makeValueOfDimensionAvailable(const LayerDataEntry::ptr& valueOfDimensionData);
        };
        using DimensionReplacementStatusLookup = std::map<std::string, std::shared_ptr<DimensionReplacementStatus>>;
        std::unique_ptr<DimensionReplacementStatusLookup> dimensionReplacementStatusLookup;

        struct SignalBitwidthGroup {
            unsigned int             bitwidth;
            std::vector<std::string> signalIdents;
        };
        std::vector<SignalBitwidthGroup> orderedReplacementCandidatesBitwidthCache;

        const parser::SymbolTable::ptr symbolTable;

        // TODO: Add checks that double underscore cannot be used as a signal ident in the parser
        const std::string_view     DEFAULT_GENERATED_REPLACEMENT_CANDIDATE_PREFIX = "__noAddLineSynCandidate_";
        std::string                generatedReplacementCandidatePrefix;
        syrec::Variable::vec       generatedReplacementsLookup;
        std::optional<std::size_t> lastGeneratedReplacementCandidateCounter;

        struct TransformedSignalAccess {
            std::string                                              signalIdent;
            std::vector<unsigned int>                                accessedValuePerDimension;
            optimizations::BitRangeAccessRestriction::BitRangeAccess accessedBitRange;
        };

        void                                                                                  activateRestriction(const syrec::VariableAccess& restrictionToCreate, bool shouldRestrictionLiveUntilNextReset);
        void                                                                                  cacheSignalBitwidth(const std::string& signalIdent, unsigned int signalBitwidth);
        [[nodiscard]] std::optional<std::reference_wrapper<SignalBitwidthGroup>>              getOrCreateCacheEntryForSignalBitwdith(unsigned int bitwidth);
        [[nodiscard]] std::optional<unsigned int>                                             getSignalBitwidth(const std::string& signalIdent) const;
        [[nodiscard]] std::optional<syrec::Variable::ptr>                                     getSignalInformationFromSymbolTable(const std::string_view& signalIdent) const;
        [[nodiscard]] std::optional<TransformedSignalAccess>                                  transformSignalAccess(const syrec::VariableAccess& signalAccess, const syrec::Variable& symbolTableEntryForReferencedSignal) const;
        
        [[maybe_unused]] std::optional<DimensionReplacementStatus::ptr>                                 getOrCreateEntryForRestriction(const syrec::Variable& symbolTableEntryForReferencedSignal);
        [[nodiscard]] static std::optional<DimensionReplacementStatus::ptr>                             generateDefaultDimensionReplacementStatusForIntermediateLayer(const std::vector<unsigned int>& declaredValuesPerDimension, std::size_t currDimensionToProcess, unsigned int bitwidthOfAccessedSignal);
        [[nodiscard]] static std::optional<DimensionReplacementStatus::LeafLayerData::ptr>              generateDefaultDimensionReplacementStatusForValueOfDimensionInLeafLayer(unsigned int bitwidthOfAccessedSignal);
        [[maybe_unused]] static std::optional<DimensionReplacementStatus::ptr>                          advanceRestrictionLookupToLeafLayer(const DimensionReplacementStatus::ptr& topMostRestrictionLayer, const std::vector<unsigned int>& dimensionAccessDefiningPathToLeafLayer, unsigned int bitwidthOfAccessedSignal);
        
        struct CandidateBasedOnBitwidthSearchResult {
            std::string signalIdent;
            std::size_t offsetOfCandidateGroup;
            std::size_t offsetInCandidateGroup;
        };

        [[nodiscard]] std::optional<syrec::VariableAccess::ptr>   findReplacementCandidateBasedOnBitwidth(unsigned int requiredMinimumBitwidth);
        [[nodiscard]] std::vector<SignalBitwidthGroup>::iterator  getFirstGroupOfSignalsWithEqualOrLargerBitwidth(unsigned int bitwidth);
        [[nodiscard]] bool                                        advanceIteratorToNextGroup(std::vector<SignalBitwidthGroup>::iterator& iterator) const;
        [[nodiscard]] std::optional<syrec::VariableAccess::ptr>   searchForReplacementInPotentialReplacementCandidatesFromSymbolTableWithMatchingBitwidth(unsigned int bitwidth);
        [[nodiscard]] static std::optional<syrec::VariableAccess> generateSignalAccessForNotLoadedEntryFromSymbolTableForGivenBitwidth(const syrec::Variable::ptr& symbolTableEntry, unsigned int requiredBitwidth);

        struct SignalAccessReplacementSearchData {
            std::size_t                                              currentlyProcessedDimension;
            std::size_t                                              numPossibleBitRangeWindowAdvances;
            std::vector<unsigned int>                                currentValuePerAccessedDimension;
            optimizations::BitRangeAccessRestriction::BitRangeAccess activeBitRangeAccess;
            syrec::Variable::ptr                                     symbolTableInformationOfReplacementCandidate;

            [[nodiscard]] static std::optional<SignalAccessReplacementSearchData> createFrom(const syrec::Variable::ptr& symbolTableInformationOfReplacementCandidate, unsigned int requiredBitwidthForReplacement) {
                if (!symbolTableInformationOfReplacementCandidate || !symbolTableInformationOfReplacementCandidate->bitwidth || symbolTableInformationOfReplacementCandidate->bitwidth < requiredBitwidthForReplacement) {
                    return std::nullopt;
                }
                return SignalAccessReplacementSearchData({
                    .currentlyProcessedDimension                  = 0,
                    .numPossibleBitRangeWindowAdvances            = (symbolTableInformationOfReplacementCandidate->bitwidth - requiredBitwidthForReplacement) + 1,
                    .currentValuePerAccessedDimension             = std::vector(symbolTableInformationOfReplacementCandidate->dimensions.size(), 0u),
                    .activeBitRangeAccess                         = optimizations::BitRangeAccessRestriction::BitRangeAccess(0, requiredBitwidthForReplacement - 1),
                    .symbolTableInformationOfReplacementCandidate = symbolTableInformationOfReplacementCandidate}
                );
            }

            [[nodiscard]] bool                                                                    incrementProcessedValueOfDimensionAndCheckForOverflow();
            [[nodiscard]] bool                                                                    advanceToNextProcessableValueOfDimension();
            [[nodiscard]] std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess> determineBitrangeUsableForReplacement(const DimensionReplacementStatus::ptr& topMostRestrictionLayer);
            [[nodiscard]] TransformedSignalAccess                                                 transformToInternalSignalAccess() const {
                return TransformedSignalAccess({.signalIdent = symbolTableInformationOfReplacementCandidate->name, .accessedValuePerDimension = currentValuePerAccessedDimension, .accessedBitRange = activeBitRangeAccess});
            }
        };
        [[nodiscard]] std::optional<syrec::VariableAccess::ptr> createReplacementFromCandidate(const syrec::Variable::ptr& symbolTableEntryForReplacementCandidate, unsigned int requiredMinimumBitwidthOfReplacement);
        [[nodiscard]] std::optional<std::size_t>                loadLastGeneratedTemporarySignalNameFromSymbolTable() const;
        [[nodiscard]] std::optional<std::string>                generateNextTemporarySignalName();
        [[nodiscard]] std::optional<syrec::VariableAccess::ptr> generateNewTemporarySignalAsReplacementCandidate(unsigned int requiredBitwidth);
        [[nodiscard]] std::optional<std::size_t>                determinePostfixOfNewlyGeneratedReplacementSignalIdent(const std::string_view& signalIdent) const;
        [[nodiscard]] static std::optional<std::size_t>         determineNextTemporarySignalPostfix(const std::optional<std::size_t>& lastGeneratedTemporarySignalPostfix);
    };
} // namespace noAdditionalLineSynthesis
#endif