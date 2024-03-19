#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/expression_substitution_generator.hpp"

#include "core/syrec/parser/operation.hpp"
#include "core/syrec/parser/utils/bit_helpers.hpp"

void noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::updateRestrictions(const std::vector<syrec::VariableAccess::ptr>& restrictionsDefiningNotUsableReplacementCandidates, RestrictionLifetime lifeTimeOfRestrictions, RestrictionUpdate typeOfUpdate) {
    for (const syrec::VariableAccess::ptr& restrictionDefiningNotUsableReplacementCandidate : restrictionsDefiningNotUsableReplacementCandidates) {
        updateRestriction(*restrictionDefiningNotUsableReplacementCandidate, lifeTimeOfRestrictions, typeOfUpdate);
    }   
}

void noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::loadLastNewlyGeneratedReplacementSignalInformation() {
    if (!symbolTable) {
        lastGeneratedReplacementCandidateCounter.reset();
        return;
    }
    lastGeneratedReplacementCandidateCounter = loadLastGeneratedTemporarySignalNameFromSymbolTable();
}

void noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::clearAllRestrictions() {
    dimensionReplacementStatusLookup.clear();
}

void noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::resetInternals(bool reloadGeneratableReplacementCandidateNameFromSymbolTable) {
    clearAllRestrictions();
    orderedReplacementCandidatesBitwidthCache.clear();
    if (reloadGeneratableReplacementCandidateNameFromSymbolTable && symbolTable) {
        loadLastNewlyGeneratedReplacementSignalInformation();   
    }
    generatedReplacementsLookup.clear();
}

void noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::defineSymbolTable(const parser::SymbolTable::ptr& symbolTable) {
    this->symbolTable = symbolTable;
}

std::optional<noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::ReplacementResult> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::generateReplacementFor(unsigned requiredBitwidthOfReplacement, const SignalValueLookupCallback& existingSignalValueLookupCallback) {
    if (!symbolTable) {
        return std::nullopt;
    }

    if (const std::optional<syrec::VariableAccess::ptr> foundReplacement = findReplacementCandidateBasedOnBitwidth(requiredBitwidthOfReplacement, existingSignalValueLookupCallback); foundReplacement.has_value()) {
        const std::optional<unsigned int> currentValueOfFoundReplacement = existingSignalValueLookupCallback(**foundReplacement);
        if (!currentValueOfFoundReplacement.has_value()) {
            return std::nullopt;
        }

        ReplacementResult::OptionalResetAssignment requiredResetPriorToUsageOfReplacement;
        bool                                       wasGenerationOfReplacementSuccessful = true;
        if (*currentValueOfFoundReplacement) {
            /*
             * For chosen replacement candidate signal R with a current value of 1, using an unary assignment statement of the form --= R should be more optimal than an assignment of the form R ^= 1.
             * The reasoning behind this was derived from traditional programming languages like C where such an assignment would be equal to R = R ^ 1, whether our assumption is correct needs to be verified.
             */
            if (currentValueOfFoundReplacement == 1) {
                if (const std::optional<unsigned int> mappedToAssignmentFlagForEnum = syrec_operation::tryMapUnaryAssignmentOperationEnumToFlag(syrec_operation::operation::DecrementAssign); mappedToAssignmentFlagForEnum.has_value()) {
                    const std::shared_ptr<syrec::UnaryStatement> temporaryContainerForRequiredReset = std::make_shared<syrec::UnaryStatement>(*mappedToAssignmentFlagForEnum, *foundReplacement);
                    requiredResetPriorToUsageOfReplacement                                          = temporaryContainerForRequiredReset;
                    wasGenerationOfReplacementSuccessful                                            = temporaryContainerForRequiredReset != nullptr;
                }
                else {
                    wasGenerationOfReplacementSuccessful = false;
                }
            }
            else {
                // Resetting the value of a signal with current value X can be done either via a subtraction or via an XOR operation and since the latter is a cheaper operation it is the preferred operation for such a reset
                if (const std::optional<unsigned int> mappedToAssignmentFlagForEnum = syrec_operation::tryMapAssignmentOperationEnumToFlag(syrec_operation::operation::XorAssign); mappedToAssignmentFlagForEnum.has_value()) {
                    const syrec::Number::ptr containerForAssignmentRhsValue = std::make_shared<syrec::Number>(*currentValueOfFoundReplacement);
                    if (const syrec::NumericExpression::ptr containerForAssignmentRhsValueExpr = containerForAssignmentRhsValue ? std::make_shared<syrec::NumericExpression>(containerForAssignmentRhsValue, BitHelpers::getRequiredBitsToStoreValue(*currentValueOfFoundReplacement)) : nullptr) {
                        const std::shared_ptr<syrec::AssignStatement> temporaryContainerForRequiredReset = std::make_shared<syrec::AssignStatement>(*foundReplacement, *mappedToAssignmentFlagForEnum, containerForAssignmentRhsValueExpr);
                        requiredResetPriorToUsageOfReplacement                                           = temporaryContainerForRequiredReset;
                        wasGenerationOfReplacementSuccessful                                             = temporaryContainerForRequiredReset != nullptr;
                    }
                    else {
                        wasGenerationOfReplacementSuccessful = false;
                    }
                } else {
                    wasGenerationOfReplacementSuccessful = false;
                }
            }
        }
        if (wasGenerationOfReplacementSuccessful) {
            updateRestriction(**foundReplacement, RestrictionLifetime::Temporary, RestrictionUpdate::Activation);
            return ReplacementResult({.foundReplacement = *foundReplacement, .requiredResetOfReplacement = requiredResetPriorToUsageOfReplacement, .doesGeneratedReplacementReferenceExistingSignal = true});   
        }
    }
    else if (const std::optional<syrec::VariableAccess::ptr> newlyGeneratedReplacement = generateNewTemporarySignalAsReplacementCandidate(requiredBitwidthOfReplacement); newlyGeneratedReplacement.has_value()) {
        return ReplacementResult({.foundReplacement = *newlyGeneratedReplacement, .requiredResetOfReplacement = std::nullopt, .doesGeneratedReplacementReferenceExistingSignal = false});
    }
    return std::nullopt;
}

syrec::Variable::vec noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::getDefinitionsOfReplacementsCreatedFromNewlyGeneratedSignals() const {
    return generatedReplacementsLookup;
}


/*
 * NON_PUBLIC DATASTRUCTURE INTERNALS
 */
void noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::DimensionReplacementStatus::LeafLayerData::clearAllTemporaryRestrictions() const {
    temporaryRestrictions->liftAllRestrictions();
}

void noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::DimensionReplacementStatus::LeafLayerData::updateRestriction(const optimizations::BitRangeAccessRestriction::BitRangeAccess& restriction, RestrictionLifetime lifeTimeOfRestriction, RestrictionUpdate typeOfUpdate) const {
    if (lifeTimeOfRestriction == RestrictionLifetime::Temporary) {
        if (typeOfUpdate == RestrictionUpdate::Activation) {
            temporaryRestrictions->restrictAccessTo(restriction);
        } else if (typeOfUpdate == RestrictionUpdate::Deactivation) {
            temporaryRestrictions->liftRestrictionFor(restriction);
        }
    } else if (lifeTimeOfRestriction == RestrictionLifetime::Permanent) {
        if (typeOfUpdate == RestrictionUpdate::Activation) {
            permanentRestrictions->restrictAccessTo(restriction);
        } else if (typeOfUpdate == RestrictionUpdate::Deactivation) {
            permanentRestrictions->liftRestrictionFor(restriction);
        }
    }
}

bool noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::DimensionReplacementStatus::LeafLayerData::isAccessRestrictedToBitrange(const optimizations::BitRangeAccessRestriction::BitRangeAccess& bitRange) const {
    return permanentRestrictions->isAccessRestrictedTo(bitRange) || temporaryRestrictions->isAccessRestrictedTo(bitRange);
}


std::optional<noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::DimensionReplacementStatus::ptr> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::DimensionReplacementStatus::LayerDataEntry::getIntermediateLayerData() const {
    if (std::holds_alternative<DimensionReplacementStatus::ptr>(data)) {
        return std::get<DimensionReplacementStatus::ptr>(data);
    }
    return std::nullopt;
}

std::optional<noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::DimensionReplacementStatus::LeafLayerData::ptr> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::DimensionReplacementStatus::LayerDataEntry::getLeafLayerData() const {
    if (std::holds_alternative<LeafLayerData::ptr>(data)) {
        return std::get<LeafLayerData::ptr>(data);
    }
    return std::nullopt;
}


std::optional<noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::DimensionReplacementStatus::ptr> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::DimensionReplacementStatus::getIntermediateLayerDataForValueOfDimension(std::size_t valueOfDimension) const {
    if (const std::optional<LayerDataEntry::ptr> dataForValueOfDimension = getDataOfValueOfDimension(valueOfDimension); dataForValueOfDimension.has_value()) {
        return dataForValueOfDimension->get()->getIntermediateLayerData();
    }
    return std::nullopt;
}

std::optional<noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::DimensionReplacementStatus::LeafLayerData::ptr> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::DimensionReplacementStatus::getLeafLayerDataForValueOfDimension(std::size_t valueOfDimension) const {
    if (const std::optional<LayerDataEntry::ptr> dataForValueOfDimension = getDataOfValueOfDimension(valueOfDimension); dataForValueOfDimension.has_value()) {
        return dataForValueOfDimension->get()->getLeafLayerData();
    }
    return std::nullopt;
}

std::optional<noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::DimensionReplacementStatus::LayerDataEntry::ptr> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::DimensionReplacementStatus::getDataOfValueOfDimension(std::size_t valueOfDimension) const {
    const auto& dataForValueOfDimension = std::find_if(dataPerValueOfDimension.begin(), dataPerValueOfDimension.end(), [valueOfDimension](const LayerDataEntry::ptr& dataOfValueOfDimension) {
        return dataOfValueOfDimension->referencedValueOfDimension == valueOfDimension;
    });

    if (dataForValueOfDimension == dataPerValueOfDimension.end()) {
        return std::nullopt;
    }
    return *dataForValueOfDimension;
}


void noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::DimensionReplacementStatus::makeValueOfDimensionAvailable(const LayerDataEntry::ptr& valueOfDimensionData) {
    if (std::optional<LayerDataEntry::ptr> dataForValueOfDimension = getDataOfValueOfDimension(valueOfDimensionData->referencedValueOfDimension); dataForValueOfDimension.has_value()) {
        *dataForValueOfDimension = valueOfDimensionData;
    } else {
        const std::size_t referencedValueOfDimensionForNewData          = valueOfDimensionData->referencedValueOfDimension;
        const auto&       insertPositionForContainerForValueOfDimension = std::find_if(
                      dataPerValueOfDimension.cbegin(),
                      dataPerValueOfDimension.cend(),
                      [referencedValueOfDimensionForNewData](const DimensionReplacementStatus::LayerDataEntry::ptr& containerForAlreadyLoadedValueOfDimension) {
                    return containerForAlreadyLoadedValueOfDimension->referencedValueOfDimension > referencedValueOfDimensionForNewData;
                });

        if (insertPositionForContainerForValueOfDimension == dataPerValueOfDimension.cend()) {
            dataPerValueOfDimension.emplace_back(valueOfDimensionData);
        } else {
            dataPerValueOfDimension.insert(insertPositionForContainerForValueOfDimension, valueOfDimensionData);
        }
    }

    if (!wasValueOfDimensionAlreadyLoaded(valueOfDimensionData->referencedValueOfDimension)) {
        alreadyLoadedValuesOfDimension.insert(valueOfDimensionData->referencedValueOfDimension);
    }
}

bool noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::DimensionReplacementStatus::wasValueOfDimensionAlreadyLoaded(std::size_t valueOfDimension) const {
    return alreadyLoadedValuesOfDimension.count(valueOfDimension);
}

/*
 * START OF NON_PUBLIC FUNCTIONALITY
 */
void noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::updateRestriction(const syrec::VariableAccess& restriction, RestrictionLifetime lifetimeOfRestriction, RestrictionUpdate typeOfUpdate) {
    if (!symbolTable) {
        return;
    }

    const std::optional<syrec::Variable::ptr>    symbolTableInformationOfReferencedSignal = getSignalInformationFromSymbolTable(restriction.var->name);
    const std::optional<TransformedSignalAccess> transformedSignalAccess                  = symbolTableInformationOfReferencedSignal.has_value() ? transformSignalAccess(restriction, **symbolTableInformationOfReferencedSignal) : std::nullopt;
    if (!transformedSignalAccess.has_value()) {
        return;
    }

    const bool shouldMissingEntriesBeCreated = typeOfUpdate == RestrictionUpdate::Activation;
    const std::optional<DimensionReplacementStatus::ptr> topMostRestrictionLayerForCandidate = shouldMissingEntriesBeCreated
        ? getOrCreateEntryForRestriction(**symbolTableInformationOfReferencedSignal)
        : getEntryForRestriction(**symbolTableInformationOfReferencedSignal);

    if (!topMostRestrictionLayerForCandidate.has_value()) {
        return;
    }

    if (const std::optional<DimensionReplacementStatus::ptr> leafLayerForCandidate = advanceRestrictionLookupToLeafLayer(*topMostRestrictionLayerForCandidate, transformedSignalAccess->accessedValuePerDimension, restriction.var->bitwidth, shouldMissingEntriesBeCreated); leafLayerForCandidate.has_value()) {
        if (const DimensionReplacementStatus::LeafLayerData::ptr leafLayerDataForCandidate = leafLayerForCandidate->get()->getLeafLayerDataForValueOfDimension(transformedSignalAccess->accessedValuePerDimension.back()).value_or(nullptr); leafLayerDataForCandidate) {
            leafLayerDataForCandidate->updateRestriction(transformedSignalAccess->accessedBitRange, lifetimeOfRestriction, typeOfUpdate);
        }
    }
}

void noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::cacheSignalBitwidth(const std::string& signalIdent, unsigned signalBitwidth) {
    std::optional<std::reference_wrapper<SignalBitwidthGroup>> matchingGroupForSignalBitwidth = getOrCreateCacheEntryForSignalBitwdith(signalBitwidth);
    if (!matchingGroupForSignalBitwidth.has_value()) {
        return;
    }

    std::vector<std::string> cachedSignalIdentsInGroup = matchingGroupForSignalBitwidth->get().signalIdents;
    if (std::find_if(cachedSignalIdentsInGroup.cbegin(), cachedSignalIdentsInGroup.cend(), [&signalIdent](const std::string& cachedSignalIdent) { return signalIdent == cachedSignalIdent; }) == cachedSignalIdentsInGroup.cend()) {
        cachedSignalIdentsInGroup.emplace_back(signalIdent);
    }
}

void noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::incrementLastNewlyGeneratedSignalCounter() {
    if (!lastGeneratedReplacementCandidateCounter.has_value()) {
        return;
    }

    if (*lastGeneratedReplacementCandidateCounter == UINT_MAX) {
        lastGeneratedReplacementCandidateCounter.reset();
        return;
    }
    lastGeneratedReplacementCandidateCounter = *lastGeneratedReplacementCandidateCounter + 1;
}


std::optional<std::reference_wrapper<noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::SignalBitwidthGroup>> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::getOrCreateCacheEntryForSignalBitwdith(unsigned int bitwidth) {
    if (!bitwidth) {
        return std::nullopt;
    }

    const auto& matchingGroupForSignalBitwidth = std::find_if(
    orderedReplacementCandidatesBitwidthCache.begin(),
    orderedReplacementCandidatesBitwidthCache.end(),
    [bitwidth](const SignalBitwidthGroup& signalBitwidthGroup) {
        return signalBitwidthGroup.bitwidth >= bitwidth;
    });

    if (matchingGroupForSignalBitwidth == orderedReplacementCandidatesBitwidthCache.end()) {
        auto newCacheEntry = SignalBitwidthGroup({.bitwidth = bitwidth, .signalIdents = {}});
        orderedReplacementCandidatesBitwidthCache.emplace_back(newCacheEntry);
        return orderedReplacementCandidatesBitwidthCache.back();
    }
    return *matchingGroupForSignalBitwidth;
}

std::unordered_set<std::string> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::determineCachedSignalIdentsHavingGivenBitwidth(unsigned bitwidth) const {
    if (!bitwidth) {
        return {};
    }
    
    const auto& matchingGroupForSignalBitwidth = std::find_if(
    orderedReplacementCandidatesBitwidthCache.cbegin(),
    orderedReplacementCandidatesBitwidthCache.cend(),
    [bitwidth](const SignalBitwidthGroup& signalBitwidthGroup) {
        return signalBitwidthGroup.bitwidth == bitwidth;
    });
    if (matchingGroupForSignalBitwidth == orderedReplacementCandidatesBitwidthCache.cend()) {
        return {};
    }
    return {matchingGroupForSignalBitwidth->signalIdents.cbegin(), matchingGroupForSignalBitwidth->signalIdents.cend()};
}


std::optional<unsigned> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::getSignalBitwidth(const std::string& signalIdent) const {
    if (const std::optional<syrec::Variable::ptr> symbolTableInformationOfReferencedSignal = getSignalInformationFromSymbolTable(signalIdent); symbolTableInformationOfReferencedSignal.has_value()) {
        return symbolTableInformationOfReferencedSignal->get()->bitwidth;
    }
    return std::nullopt;
}

std::optional<syrec::Variable::ptr> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::getSignalInformationFromSymbolTable(const std::string_view& signalIdent) const {
    std::optional<std::variant<syrec::Variable::ptr, syrec::Number::ptr>> symbolTableEntryForSignalIdent = symbolTable->getVariable(signalIdent);
    if (symbolTableEntryForSignalIdent.has_value() && std::holds_alternative<syrec::Variable::ptr>(*symbolTableEntryForSignalIdent)) {
        return std::get<syrec::Variable::ptr>(*symbolTableEntryForSignalIdent);
    }
    return std::nullopt;
}

std::optional<noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::TransformedSignalAccess> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::transformSignalAccess(const syrec::VariableAccess& signalAccess, const syrec::Variable& symbolTableEntryForReferencedSignal) const {
    std::vector<unsigned int> transformedDimensionAccess    = std::vector(symbolTableEntryForReferencedSignal.dimensions.size(), 0u);
    std::size_t               transformedDimensionAccessIdx                = 0;
    for (const syrec::Expression::ptr& exprDefiningAccessedValueOfDimension : signalAccess.indexes) {
        if (const auto& exprAsNumericExpr = std::dynamic_pointer_cast<const syrec::NumericExpression>(exprDefiningAccessedValueOfDimension); exprAsNumericExpr && exprAsNumericExpr->value->isConstant()) {
            transformedDimensionAccess.at(transformedDimensionAccessIdx++) = exprAsNumericExpr->value->evaluate({});
        } else {
            return std::nullopt;
        }
    }

    auto transformedBitrangeAccess = optimizations::BitRangeAccessRestriction::BitRangeAccess(0, symbolTableEntryForReferencedSignal.bitwidth - 1);
    if (signalAccess.range.has_value()) {
        const syrec::Number::ptr accessedBitRangeStart = std::dynamic_pointer_cast<syrec::Number>(signalAccess.range->first);
        const syrec::Number::ptr accessedBitRangeEnd = accessedBitRangeStart ? std::dynamic_pointer_cast<syrec::Number>(signalAccess.range->second) : nullptr;
        if (!accessedBitRangeEnd || !accessedBitRangeStart->isConstant() || !accessedBitRangeEnd->isConstant()) {
            return std::nullopt;
        }
        transformedBitrangeAccess.first = accessedBitRangeStart->evaluate({});
        transformedBitrangeAccess.second = accessedBitRangeEnd->evaluate({});
    }
    return TransformedSignalAccess({.signalIdent = signalAccess.var->name, .accessedValuePerDimension = transformedDimensionAccess, .accessedBitRange = transformedBitrangeAccess});
}

std::optional<noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::DimensionReplacementStatus::ptr> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::getOrCreateEntryForRestriction(const syrec::Variable& symbolTableEntryForReferencedSignal) {
    if (std::optional<DimensionReplacementStatus::ptr> fetchedExistingEntry = getEntryForRestriction(symbolTableEntryForReferencedSignal); fetchedExistingEntry.has_value()) {
        return fetchedExistingEntry;
    }

    const auto defaultAccessedValuePerDimension = std::vector(symbolTableEntryForReferencedSignal.dimensions.size(), 0u);
    if (const std::optional<DimensionReplacementStatus::ptr> generatedLookupForGivenAccess = generateDefaultDimensionReplacementStatusForIntermediateLayer(defaultAccessedValuePerDimension, 0, symbolTableEntryForReferencedSignal.bitwidth); generatedLookupForGivenAccess.has_value()) {
        dimensionReplacementStatusLookup.insert(std::make_pair(symbolTableEntryForReferencedSignal.name, *generatedLookupForGivenAccess));
        if (const std::optional<std::reference_wrapper<SignalBitwidthGroup>>& groupOfSignalsSharingSameBitwidth = getOrCreateCacheEntryForSignalBitwdith(symbolTableEntryForReferencedSignal.bitwidth); groupOfSignalsSharingSameBitwidth.has_value()) {
            groupOfSignalsSharingSameBitwidth->get().signalIdents.emplace_back(symbolTableEntryForReferencedSignal.name);
        }

        return *generatedLookupForGivenAccess;
    }
    return std::nullopt;
}

std::optional<noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::DimensionReplacementStatus::ptr> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::getEntryForRestriction(const syrec::Variable& symbolTableEntryForReferencedSignal) const {
    if (dimensionReplacementStatusLookup.count(symbolTableEntryForReferencedSignal.name)) {
        return dimensionReplacementStatusLookup.at(symbolTableEntryForReferencedSignal.name);
    }
    return std::nullopt;
}

std::optional<noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::DimensionReplacementStatus::ptr> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::generateDefaultDimensionReplacementStatusForIntermediateLayer(const std::vector<unsigned>& declaredValuesPerDimension, std::size_t currDimensionToProcess, unsigned int bitwidthOfAccessedSignal) {
    if (currDimensionToProcess > declaredValuesPerDimension.size()) {
        return std::nullopt;
    }

    DimensionReplacementStatus::ptr generatedContainerForIntermediateLayer = std::make_shared<DimensionReplacementStatus>();
    if (!generatedContainerForIntermediateLayer) {
        return std::nullopt;
    }
    generatedContainerForIntermediateLayer->lastLoadedValueOfDimension     = 0;
    generatedContainerForIntermediateLayer->alreadyLoadedValuesOfDimension = std::unordered_set<std::size_t>();

    const bool  isIntermediateDimension       = currDimensionToProcess == declaredValuesPerDimension.size();
    bool        didProcessingCompleteNormally = false;
    if (!isIntermediateDimension) {
        if (const std::optional<DimensionReplacementStatus::LeafLayerData::ptr>& generatedDataForValueOfLeafLayer = generateDefaultDimensionReplacementStatusForValueOfDimensionInLeafLayer(bitwidthOfAccessedSignal); generatedDataForValueOfLeafLayer.has_value()) {
            if (const DimensionReplacementStatus::LayerDataEntry::ptr container = std::make_shared<DimensionReplacementStatus::LayerDataEntry>(DimensionReplacementStatus::LayerDataEntry({0, *generatedDataForValueOfLeafLayer})); container) {
                generatedContainerForIntermediateLayer->makeValueOfDimensionAvailable(container);
                didProcessingCompleteNormally = true;
            }
        }
    } else {
        if (const std::optional<DimensionReplacementStatus::ptr>& generatedDataForValueOfIntermediateLayer = generateDefaultDimensionReplacementStatusForIntermediateLayer(declaredValuesPerDimension, ++currDimensionToProcess, bitwidthOfAccessedSignal); generatedDataForValueOfIntermediateLayer.has_value()) {
            if (const DimensionReplacementStatus::LayerDataEntry::ptr container = std::make_shared<DimensionReplacementStatus::LayerDataEntry>(DimensionReplacementStatus::LayerDataEntry({0, *generatedDataForValueOfIntermediateLayer})); container) {
                generatedContainerForIntermediateLayer->makeValueOfDimensionAvailable(container);
                didProcessingCompleteNormally = true;
            }
        }
    }

    if (!didProcessingCompleteNormally) {
        return std::nullopt;
    }
    return generatedContainerForIntermediateLayer;
}

std::optional<noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::DimensionReplacementStatus::LeafLayerData::ptr> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::generateDefaultDimensionReplacementStatusForValueOfDimensionInLeafLayer(unsigned bitwidthOfAccessedSignal) {
    const optimizations::BitRangeAccessRestriction::ptr permanentPotentialReplacementRestrictionStatus = std::make_shared<optimizations::BitRangeAccessRestriction>(bitwidthOfAccessedSignal);
    const optimizations::BitRangeAccessRestriction::ptr temporaryPotentialReplacementRestrictionStatus = std::make_shared<optimizations::BitRangeAccessRestriction>(bitwidthOfAccessedSignal);
    if (!temporaryPotentialReplacementRestrictionStatus) {
        return std::nullopt;
    }
    if (const auto& generatedLeafDataForValueOfDimension = std::make_shared<DimensionReplacementStatus::LeafLayerData>(DimensionReplacementStatus::LeafLayerData({permanentPotentialReplacementRestrictionStatus, temporaryPotentialReplacementRestrictionStatus})); generatedLeafDataForValueOfDimension) {
        return generatedLeafDataForValueOfDimension;
    }
    return std::nullopt;
}

std::optional<noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::DimensionReplacementStatus::ptr> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::advanceRestrictionLookupToLeafLayer(const DimensionReplacementStatus::ptr& topMostRestrictionLayer, const std::vector<unsigned>& dimensionAccessDefiningPathToLeafLayer, unsigned int bitwidthOfAccessedSignal, bool generateMissingEntries) {
    DimensionReplacementStatus::ptr currentDimensionReplacementStatus = topMostRestrictionLayer;
    for (std::size_t i = 0; i < dimensionAccessDefiningPathToLeafLayer.size(); ++i) {
        const std::size_t accessedValueOfDimension = dimensionAccessDefiningPathToLeafLayer.at(i);
        const bool        isIntermediateDimension  = i < (dimensionAccessDefiningPathToLeafLayer.size() - 1);
        if (!currentDimensionReplacementStatus->getDataOfValueOfDimension(accessedValueOfDimension).has_value()) {
            if (generateMissingEntries) {
                DimensionReplacementStatus::LayerDataEntry::ptr containerForLoadedValueOfDimension;
                if (isIntermediateDimension) {
                    std::vector<unsigned int> accessedValuePerDimensionOfRemainingDimensionAccess = std::vector(std::next(dimensionAccessDefiningPathToLeafLayer.cbegin(), i + 1), dimensionAccessDefiningPathToLeafLayer.cend());
                    if (const std::optional<DimensionReplacementStatus::ptr> containerForIntermediateDimensions = generateDefaultDimensionReplacementStatusForIntermediateLayer(accessedValuePerDimensionOfRemainingDimensionAccess, 0, bitwidthOfAccessedSignal); containerForIntermediateDimensions.has_value()) {
                        containerForLoadedValueOfDimension = std::make_shared<DimensionReplacementStatus::LayerDataEntry>(DimensionReplacementStatus::LayerDataEntry({accessedValueOfDimension, *containerForIntermediateDimensions}));
                    }
                } else {
                    if (const std::optional<DimensionReplacementStatus::LeafLayerData::ptr> containerForLeafLayerDimensions = generateDefaultDimensionReplacementStatusForValueOfDimensionInLeafLayer(bitwidthOfAccessedSignal); containerForLeafLayerDimensions.has_value()) {
                        containerForLoadedValueOfDimension = std::make_shared<DimensionReplacementStatus::LayerDataEntry>(DimensionReplacementStatus::LayerDataEntry({accessedValueOfDimension, *containerForLeafLayerDimensions}));
                    }
                }

                if (!containerForLoadedValueOfDimension) {
                    break;
                }
                currentDimensionReplacementStatus->makeValueOfDimensionAvailable(containerForLoadedValueOfDimension);    
            } else {
                return std::nullopt;
            }
        }
        if (!isIntermediateDimension) {
            return currentDimensionReplacementStatus;
        }

        if (const std::optional<DimensionReplacementStatus::ptr> dataOfNextDimension = currentDimensionReplacementStatus->getIntermediateLayerDataForValueOfDimension(accessedValueOfDimension); dataOfNextDimension.has_value()) {
            currentDimensionReplacementStatus = *dataOfNextDimension;
        } else {
            break;
        }
    }
    return std::nullopt;
}


std::optional<syrec::VariableAccess::ptr> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::findReplacementCandidateBasedOnBitwidth(unsigned requiredMinimumBitwidth, const SignalValueLookupCallback& existingSignalValueLookupCallback) {
    auto                                       currentGroupOfSignalsMatchingBitwidth                              = getFirstGroupOfSignalsWithEqualOrLargerBitwidth(requiredMinimumBitwidth);
    std::vector<unsigned int>                  potentialLoadableFutureReplacementCandidateGroupsFromSymbolTable   = symbolTable->fetchBitwidthsPerSharedBitwidthSignalGroupsWithAssignableSignals(requiredMinimumBitwidth);
    auto                                       lastLoadedPotentialLoadableFutureReplacementCandidateGroupIterator = potentialLoadableFutureReplacementCandidateGroupsFromSymbolTable.begin();

    // TODO: Check implementation
    if (currentGroupOfSignalsMatchingBitwidth != orderedReplacementCandidatesBitwidthCache.end()) {
        do {
            for (const std::string& signalIdentInGroup: currentGroupOfSignalsMatchingBitwidth->signalIdents) {
                if (const std::optional<syrec::Variable::ptr> symbolTableInformationForSignal = getSignalInformationFromSymbolTable(signalIdentInGroup); symbolTableInformationForSignal.has_value()) {
                    if (const std::optional<syrec::VariableAccess::ptr> createdReplacement = createReplacementFromCandidate(*symbolTableInformationForSignal, requiredMinimumBitwidth, existingSignalValueLookupCallback); createdReplacement.has_value()) {
                        return *createdReplacement;
                    }
                }
            }

            if (lastLoadedPotentialLoadableFutureReplacementCandidateGroupIterator != potentialLoadableFutureReplacementCandidateGroupsFromSymbolTable.end() && *lastLoadedPotentialLoadableFutureReplacementCandidateGroupIterator == currentGroupOfSignalsMatchingBitwidth->bitwidth) {
                if (const std::optional<syrec::VariableAccess::ptr> createdReplacement = searchForReplacementInPotentialReplacementCandidatesFromSymbolTableWithMatchingBitwidth(*lastLoadedPotentialLoadableFutureReplacementCandidateGroupIterator, requiredMinimumBitwidth, existingSignalValueLookupCallback); createdReplacement.has_value()) {
                    return *createdReplacement;
                }
                ++lastLoadedPotentialLoadableFutureReplacementCandidateGroupIterator;
            }
        } while (advanceIteratorToNextGroup(currentGroupOfSignalsMatchingBitwidth));
    }
    
    while (lastLoadedPotentialLoadableFutureReplacementCandidateGroupIterator != potentialLoadableFutureReplacementCandidateGroupsFromSymbolTable.end()) {
        if (const std::optional<syrec::VariableAccess::ptr> createdReplacement = searchForReplacementInPotentialReplacementCandidatesFromSymbolTableWithMatchingBitwidth(*lastLoadedPotentialLoadableFutureReplacementCandidateGroupIterator, requiredMinimumBitwidth, existingSignalValueLookupCallback); createdReplacement.has_value()) {
            return *createdReplacement;
        }
        ++lastLoadedPotentialLoadableFutureReplacementCandidateGroupIterator;
    }
    return std::nullopt;
}

std::vector<noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::SignalBitwidthGroup>::iterator noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::getFirstGroupOfSignalsWithEqualOrLargerBitwidth(unsigned bitwidth) {
    return std::find_if(
    orderedReplacementCandidatesBitwidthCache.begin(),
    orderedReplacementCandidatesBitwidthCache.end(),
    [bitwidth](const SignalBitwidthGroup& signalBitwidthGroup) {
        return signalBitwidthGroup.bitwidth >= bitwidth;
    });
}

bool noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::advanceIteratorToNextGroup(std::vector<SignalBitwidthGroup>::iterator& iterator) const {
    return ++iterator != orderedReplacementCandidatesBitwidthCache.end();
}

std::optional<syrec::VariableAccess> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::generateSignalAccessForNotLoadedEntryFromSymbolTableForGivenBitwidth(const syrec::Variable::ptr& symbolTableEntry, unsigned requiredBitwidth) {
    syrec::VariableAccess generatedSignalAccess;
    generatedSignalAccess.var     = symbolTableEntry;
    generatedSignalAccess.indexes = std::vector<syrec::Expression::ptr>(symbolTableEntry->dimensions.size(), nullptr);
    for (std::size_t i = 0; i < generatedSignalAccess.indexes.size(); ++i) {
        const syrec::Number::ptr            accessedValueOfDimension                   = std::make_shared<syrec::Number>(0);
        const syrec::NumericExpression::ptr containerForExprOfAccessedValueOfDimension = accessedValueOfDimension ? std::make_shared<syrec::NumericExpression>(accessedValueOfDimension, 1) : nullptr;
        if (!containerForExprOfAccessedValueOfDimension) {
            return std::nullopt;
        }
        generatedSignalAccess.indexes.at(i)                                            = containerForExprOfAccessedValueOfDimension;
    }

    const syrec::Number::ptr bitRangeAccessStart = std::make_shared<syrec::Number>(0);
    const syrec::Number::ptr bitRangeAccessEnd   = std::make_shared<syrec::Number>(requiredBitwidth - 1);
    if (!bitRangeAccessStart || !bitRangeAccessEnd) {
        return std::nullopt;
    }
    generatedSignalAccess.range = std::make_pair(bitRangeAccessStart, bitRangeAccessEnd);
    return generatedSignalAccess;
}


std::optional<syrec::VariableAccess::ptr> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::searchForReplacementInPotentialReplacementCandidatesFromSymbolTableWithMatchingBitwidth(unsigned int bitwidthOfGroupToBeSearched, unsigned int requiredBitwidthOfReplacement, const SignalValueLookupCallback& existingSignalValueLookupCallback) {
    const std::optional<std::unordered_set<std::string>> existingSignalsSharingSameBitwidth = std::make_optional(determineCachedSignalIdentsHavingGivenBitwidth(bitwidthOfGroupToBeSearched));
    for (const syrec::Variable::ptr& potentialFutureReplacementCandidateFromSymbolTable: symbolTable->fetchedDeclaredAssignableSignalsHavingMatchingBitwidth(bitwidthOfGroupToBeSearched, existingSignalsSharingSameBitwidth)) {
        const std::optional<syrec::VariableAccess>   accessOnFutureReplacementCandidate                  = generateSignalAccessForNotLoadedEntryFromSymbolTableForGivenBitwidth(potentialFutureReplacementCandidateFromSymbolTable, requiredBitwidthOfReplacement);
        const std::optional<TransformedSignalAccess> transformedSignalAccessOnFutureReplacementCandidate = accessOnFutureReplacementCandidate.has_value() ? transformSignalAccess(*accessOnFutureReplacementCandidate, *potentialFutureReplacementCandidateFromSymbolTable) : std::nullopt;

        if (transformedSignalAccessOnFutureReplacementCandidate.has_value()) {
            getOrCreateEntryForRestriction(*potentialFutureReplacementCandidateFromSymbolTable);
            if (const std::optional<syrec::VariableAccess::ptr> createdReplacement = createReplacementFromCandidate(potentialFutureReplacementCandidateFromSymbolTable, requiredBitwidthOfReplacement, existingSignalValueLookupCallback); createdReplacement.has_value()) {
                return *createdReplacement;
            }
        }
    }
    return std::nullopt;
}

std::optional<syrec::VariableAccess::ptr> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::createReplacementFromCandidate(const syrec::Variable::ptr& symbolTableEntryForReplacementCandidate, unsigned requiredMinimumBitwidthOfReplacement, const SignalValueLookupCallback& existingSignalValueLookupCallback) {
    std::optional<SignalAccessReplacementSearchData> signalReplacementSearchData = SignalAccessReplacementSearchData::init(symbolTableEntryForReplacementCandidate, requiredMinimumBitwidthOfReplacement);
    if (!signalReplacementSearchData.has_value()) {
        return std::nullopt;
    }

    const std::optional<DimensionReplacementStatus::ptr> topMostRestrictionLayer = getOrCreateEntryForRestriction(*symbolTableEntryForReplacementCandidate);
    if (!topMostRestrictionLayer.has_value()) {
        return std::nullopt;
    }

    if (const std::optional<syrec::VariableAccess::ptr>& foundReplacementCandidate = signalReplacementSearchData->findReplacement(*topMostRestrictionLayer, existingSignalValueLookupCallback); foundReplacementCandidate.has_value()) {
        updateRestriction(**foundReplacementCandidate, RestrictionLifetime::Temporary, RestrictionUpdate::Activation);
        return foundReplacementCandidate;
    }
    return std::nullopt;
}

std::optional<noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::SignalAccessReplacementSearchData> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::SignalAccessReplacementSearchData::init(const syrec::Variable::ptr& symbolTableInformationOfReplacementCandidate, unsigned requiredBitwidthForReplacement) {
    if (!symbolTableInformationOfReplacementCandidate || !symbolTableInformationOfReplacementCandidate->bitwidth || symbolTableInformationOfReplacementCandidate->bitwidth < requiredBitwidthForReplacement) {
        return std::nullopt;
    }
    if (const syrec::VariableAccess::ptr containerForInternalSignalAccess = std::make_shared<syrec::VariableAccess>(); containerForInternalSignalAccess) {
        containerForInternalSignalAccess->var     = symbolTableInformationOfReplacementCandidate;
        containerForInternalSignalAccess->indexes = std::vector<syrec::Expression::ptr>(symbolTableInformationOfReplacementCandidate->dimensions.size(), nullptr);

        const syrec::Number::ptr containerForBitRangeStartValue = std::make_shared<syrec::Number>(0u);
        const syrec::Number::ptr containerForBitRangeEndValue   = containerForBitRangeStartValue ? std::make_shared<syrec::Number>(requiredBitwidthForReplacement - 1) : nullptr;
        if (!containerForBitRangeEndValue) {
            return std::nullopt;
        }
        containerForInternalSignalAccess->range = std::make_pair(containerForBitRangeStartValue, containerForBitRangeEndValue);

        for (std::size_t i = 0; i < containerForInternalSignalAccess->indexes.size(); ++i) {
            const syrec::Number::ptr     containerForInitiallyAccessedValueOfDimension   = std::make_shared<syrec::Number>(0);
            const syrec::Expression::ptr containerForExpressionDefiningAccessOnDimension = containerForInitiallyAccessedValueOfDimension ? std::make_shared<syrec::NumericExpression>(containerForInitiallyAccessedValueOfDimension, BitHelpers::getRequiredBitsToStoreValue(0)) : nullptr;
            if (!containerForExpressionDefiningAccessOnDimension) {
                return std::nullopt;
            }
            containerForInternalSignalAccess->indexes.at(i) = containerForExpressionDefiningAccessOnDimension;   
        }
        return SignalAccessReplacementSearchData(containerForInternalSignalAccess, requiredBitwidthForReplacement);
    }
    return std::nullopt;
}

/*
 * TODO: Both shift operations have a high allocation overhead since the allocate new shared pointers on each iteration. Can we maybe reduce these allocations by only performing them if we found a gap based on the restriction status instead
 */
std::optional<syrec::VariableAccess::ptr> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::SignalAccessReplacementSearchData::findReplacement(const DimensionReplacementStatus::ptr& topMostRestrictionLayer, const SignalValueLookupCallback& existingSignalValueLookupCallback) {
    bool shouldContinueSearch;
    do {
        do {
            std::optional<DimensionReplacementStatus::ptr>                leafLayerData                                   = advanceRestrictionLookupToLeafLayer(topMostRestrictionLayer, currentValuePerAccessedDimension, (activeBitRangeAccess.second - activeBitRangeAccess.first) + 1);
            std::optional<DimensionReplacementStatus::LeafLayerData::ptr> restrictionStatusForCurrentInternalSignalAccess = leafLayerData.has_value() ? leafLayerData->get()->getLeafLayerDataForValueOfDimension(currentValuePerAccessedDimension.at(currentlyProcessedDimension)) : std::nullopt;
            shouldContinueSearch                                                                                          = restrictionStatusForCurrentInternalSignalAccess.has_value();
            
            if (!restrictionStatusForCurrentInternalSignalAccess->get()->isAccessRestrictedToBitrange(activeBitRangeAccess)
                && existingSignalValueLookupCallback(*internalContainerHoldingCurrentSignalAccess).has_value()) {
                // I.e. move the value fetch check inside and perform the allocations fixing the new signal access here instead of performing a reallocation on every iteration
                shouldContinueSearch = false;
            }
        } while (shouldContinueSearch && shiftBitRangeWindowToTheRightAndResetOnOverflow().value_or(false));
    } while (shouldContinueSearch && shiftDimensionAccessOneDimensionToTheLeftOnOverflow().value_or(false));

    if (internalContainerHoldingCurrentSignalAccess) {
        const unsigned int bitwidthOfGeneratedReplacement = (activeBitRangeAccess.second - activeBitRangeAccess.first) + 1;
        // As a small simplification we can omit specifying the accessed bitrange if it is equal to the full signal bitwidth of the replacement candidate
        if (bitwidthOfGeneratedReplacement == internalContainerHoldingCurrentSignalAccess->var->bitwidth) {
            internalContainerHoldingCurrentSignalAccess->range.reset();
        } else if (internalContainerHoldingCurrentSignalAccess->range.has_value()) {
            // Additionally, a bit range defined with same start and end bit can be simplified to a bit access
            // All bit range accessed generated have the start and end bit set to a constant value, thus the evaluate call of the syrec::Number should not throw when being passed an empty lookup for loop variable values.
            const unsigned int accessedBitRangeStart = internalContainerHoldingCurrentSignalAccess->range->first->evaluate({});
            const unsigned int accessedBitRangeEnd   = internalContainerHoldingCurrentSignalAccess->range->second->evaluate({});
            if (accessedBitRangeStart == accessedBitRangeEnd) {
                // Pointing the containers storing the borders of the accessed bit range to the same object might lead to problems further down the line but is required for the AST dumper
                // to simplify a bit range access with the same start and end bit to a bit access
                internalContainerHoldingCurrentSignalAccess->range->first = internalContainerHoldingCurrentSignalAccess->range->second;
            } 
        }
        
        // We can omit explicitly specifying accessing the first value of a signal with one dimension where said dimension only defines one value (i.e. the two signal accesses 'a[0]' and 'a' are the same for a signal declaration 'in a[1](16)') 
        if (internalContainerHoldingCurrentSignalAccess->var->dimensions.size() == 1 && internalContainerHoldingCurrentSignalAccess->var->dimensions.at(0) == 1) {
            internalContainerHoldingCurrentSignalAccess->indexes.clear();
        }
        return internalContainerHoldingCurrentSignalAccess;
    }
    return std::nullopt;
}

bool noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::SignalAccessReplacementSearchData::setAccessedBitRange(unsigned accessedBitRangeStart, unsigned accessedBitRangeEnd) {
    const syrec::Number::ptr containerForBitRangeStartValue = std::make_shared<syrec::Number>(accessedBitRangeStart);
    const syrec::Number::ptr containerForBitRangeEndValue   = containerForBitRangeStartValue ? std::make_shared<syrec::Number>(accessedBitRangeEnd) : nullptr;
    if (!containerForBitRangeEndValue) {
        return false;
    }
    internalContainerHoldingCurrentSignalAccess->range = std::make_pair(containerForBitRangeStartValue, containerForBitRangeEndValue);
    activeBitRangeAccess                               = std::make_pair(accessedBitRangeStart, accessedBitRangeEnd);
    return true;
}


std::optional<bool> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::SignalAccessReplacementSearchData::shiftBitRangeWindowToTheRightAndResetOnOverflow() {
    const bool doesIncrementLeadToOverflow = numMadeBitRangeWindowAdvances >= numPossibleBitRangeWindowAdvances;
    unsigned int nextBitRangeStartBit;
    unsigned int nextBitRangeEndBit;
    if (doesIncrementLeadToOverflow) {
        nextBitRangeStartBit = 0;
        nextBitRangeEndBit   = activeBitRangeAccess.second - activeBitRangeAccess.first;
        numMadeBitRangeWindowAdvances = 0;
    }
    else {
        nextBitRangeStartBit = activeBitRangeAccess.first + 1;
        nextBitRangeEndBit   = activeBitRangeAccess.second + 1;
        numMadeBitRangeWindowAdvances++;
    }

    if (!setAccessedBitRange(nextBitRangeStartBit, nextBitRangeEndBit)) {
        return std::nullopt;
    }
    return !doesIncrementLeadToOverflow;
}

std::optional<bool> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::SignalAccessReplacementSearchData::shiftDimensionAccessOneDimensionToTheLeftOnOverflow() {
    if (!doesIncrementOfValueOfDimensionLeadToOverflow(currentlyProcessedDimension)) {
        if (!setValueOfAccessedDimensionTo(currentlyProcessedDimension, currentValuePerAccessedDimension.at(currentlyProcessedDimension) + 1)) {
            return std::nullopt;
        }
        return true;
    }

    if (!currentlyProcessedDimension) {
        internalContainerHoldingCurrentSignalAccess.reset();
        return false;
    }

    bool foundDimensionWithIncrementableValueOfDimension = false;
    for (std::size_t i = 0; i <= currentlyProcessedDimension && !foundDimensionWithIncrementableValueOfDimension; ++i) {
        --currentlyProcessedDimension;
        foundDimensionWithIncrementableValueOfDimension = !doesIncrementOfValueOfDimensionLeadToOverflow(currentlyProcessedDimension);
    }

    if (!foundDimensionWithIncrementableValueOfDimension) {
        return true;
    }

    if (!setValueOfAccessedDimensionTo(currentlyProcessedDimension, currentValuePerAccessedDimension.at(currentlyProcessedDimension) + 1)) {
        return std::nullopt;
    }
    const std::size_t numDimensionsToReset = (internalContainerHoldingCurrentSignalAccess->var->dimensions.size() - currentlyProcessedDimension) + 1;
    bool              wasResetDueToOverflowSuccessful = true;
    for (std::size_t i = 0; i < numDimensionsToReset && wasResetDueToOverflowSuccessful; ++i) {
        wasResetDueToOverflowSuccessful = setValueOfAccessedDimensionTo(currentlyProcessedDimension, currentValuePerAccessedDimension.at(currentlyProcessedDimension) + 1);
        currentlyProcessedDimension++;
    }

    return wasResetDueToOverflowSuccessful ? std::make_optional(true) : std::nullopt;
}

bool noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::SignalAccessReplacementSearchData::doesIncrementOfValueOfDimensionLeadToOverflow(std::size_t indexOfDimensionToCheck) const {
    return currentValuePerAccessedDimension.at(indexOfDimensionToCheck) >= internalContainerHoldingCurrentSignalAccess->var->dimensions.at(indexOfDimensionToCheck);
}

bool noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::SignalAccessReplacementSearchData::setValueOfAccessedDimensionTo(std::size_t indexOfUpdatedDimension, unsigned accessedValueOfDimension) {
    const syrec::Number::ptr            containerForNextValueOfDimension             = std::make_shared<syrec::Number>(accessedValueOfDimension);
    const syrec::NumericExpression::ptr containerForExprDefiningNextValueOfDimension = containerForNextValueOfDimension ? std::make_shared<syrec::NumericExpression>(containerForNextValueOfDimension, BitHelpers::getRequiredBitsToStoreValue(accessedValueOfDimension)) : nullptr;
    if (!containerForExprDefiningNextValueOfDimension) {
        return false;
    }
    currentValuePerAccessedDimension.at(indexOfUpdatedDimension)                     = accessedValueOfDimension;
    internalContainerHoldingCurrentSignalAccess->indexes.at(indexOfUpdatedDimension) = containerForExprDefiningNextValueOfDimension;
    return true;
}

std::optional<std::size_t> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::loadLastGeneratedTemporarySignalNameFromSymbolTable() const {
    if (!generatedReplacementCandidatePrefix.has_value()) {
        return std::nullopt;
    }

    const std::vector<std::string> symbolTableEntriesMatchingPrefix = symbolTable->fetchIdentsOfSignalsStartingWith(*generatedReplacementCandidatePrefix);
    std::vector<std::size_t>       extractedPostfixesOfSymbolTableEntries;
    for (const std::string& symbolTableEntryIdent : symbolTableEntriesMatchingPrefix) {
        if (const std::optional<std::size_t> splitOfPostfixFromSignalIdent = determinePostfixOfNewlyGeneratedReplacementSignalIdent(symbolTableEntryIdent); splitOfPostfixFromSignalIdent.has_value()) {
            extractedPostfixesOfSymbolTableEntries.emplace_back(*splitOfPostfixFromSignalIdent);
        }
        else {
            return std::nullopt;
        }
    }

    // Sorts the container in ascending order
    std::sort(extractedPostfixesOfSymbolTableEntries.begin(), extractedPostfixesOfSymbolTableEntries.end());
    if (extractedPostfixesOfSymbolTableEntries.empty()) {
        return 0;
    }
    return extractedPostfixesOfSymbolTableEntries.back();
}

std::optional<std::string> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::generateNextTemporarySignalName() const {
    if (!generatedReplacementCandidatePrefix.has_value()) {
        return std::nullopt;
    }

    const std::optional<std::size_t> generatedSignalPostfix = determineNextTemporarySignalPostfix(lastGeneratedReplacementCandidateCounter);
    if (generatedSignalPostfix.has_value()) {
        return std::string(*generatedReplacementCandidatePrefix) + std::to_string(*generatedSignalPostfix);
    }
    return std::nullopt;
}

std::optional<syrec::VariableAccess::ptr> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::generateNewTemporarySignalAsReplacementCandidate(unsigned int requiredBitwidth) {
    std::optional<std::reference_wrapper<SignalBitwidthGroup>> matchingBitwidthSignalGroup = getOrCreateCacheEntryForSignalBitwdith(requiredBitwidth);
    if (!matchingBitwidthSignalGroup.has_value()) {
        return std::nullopt;
    }

    const std::optional<std::string> temporarySignalName = generateNextTemporarySignalName();
    if (!temporarySignalName.has_value()) {
        return std::nullopt;
    }

    /*
     * When accessing the the first and only value of a 1-D signal the indices for the dimension access most not be explicit specified and can be omitted.
     * Since all of our newly generated replacement signals are only 1-D signals we can omit the dimension access.
     */
    const syrec::Variable::ptr generatedReplacementSignal         = std::make_shared<syrec::Variable>(syrec::Variable::Types::Wire, *temporarySignalName, std::vector(1, 1u), requiredBitwidth);
    syrec::VariableAccess::ptr generatedSignalAccessOnReplacement = generatedReplacementSignal ? std::make_shared<syrec::VariableAccess>() : nullptr;
    if (!generatedSignalAccessOnReplacement) {
        return std::nullopt;
    }

    generatedSignalAccessOnReplacement->var = generatedReplacementSignal;
    generatedSignalAccessOnReplacement->range = std::nullopt;

    incrementLastNewlyGeneratedSignalCounter();
    cacheSignalBitwidth(generatedSignalAccessOnReplacement->var->name, requiredBitwidth);
    /*
     * To prevent that the generated replacement is again considered as a candidate in a new search for a replacement, we generate a temporary restriction for the found replacement of the candidate
     * that will be active until the next reset.
     */
    updateRestriction(*generatedSignalAccessOnReplacement, RestrictionLifetime::Temporary, RestrictionUpdate::Activation);

    generatedReplacementsLookup.emplace_back(generatedReplacementSignal);
    return generatedSignalAccessOnReplacement;
}

std::optional<std::size_t> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::determinePostfixOfNewlyGeneratedReplacementSignalIdent(const std::string& signalIdent) const {
    if (generatedReplacementCandidatePrefix.has_value() && !signalIdent.rfind(*generatedReplacementCandidatePrefix, 0)) {
        return convertStringToNumericValue(signalIdent.substr(generatedReplacementCandidatePrefix->size()));    
    }
    return std::nullopt;
}

std::optional<std::size_t> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::determineNextTemporarySignalPostfix(const std::optional<std::size_t>& lastGeneratedTemporarySignalPostfix) {
    if (!lastGeneratedTemporarySignalPostfix.has_value() || *lastGeneratedTemporarySignalPostfix == UINT_MAX) {
        return std::nullopt;
    }
    return *lastGeneratedTemporarySignalPostfix + 1;
}

std::optional<std::size_t> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::convertStringToNumericValue(const std::string& stringToConvert) {
    if (stringToConvert == "0") {
        return 0;
    }

    const auto determinedNumericValue = strtoul(stringToConvert.c_str(), nullptr, 10);
    if (errno == ERANGE || !determinedNumericValue || determinedNumericValue > UINT_MAX) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(determinedNumericValue);
}