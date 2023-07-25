#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/simple_additional_line_for_assignment_reducer.hpp"

#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/assignment_with_only_reversible_operations_simplifier.hpp"
#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/assignment_with_reversible_ops_and_multilevel_signal_occurrence_simplifier.hpp"

using namespace noAdditionalLineSynthesis;

syrec::AssignStatement::vec SimpleAdditionalLineForAssignmentReducer::tryReduceRequiredAdditionalLinesFor(const syrec::AssignStatement::ptr& assignmentStmt) {
    auto assignmentStmtCasted = std::dynamic_pointer_cast<syrec::AssignStatement>(assignmentStmt);
    if (assignmentStmtCasted == nullptr) {
        return {};
    }

    const auto assignmentOperation = tryMapAssignmentOperationFlagToEnum(assignmentStmtCasted->op);
    if (!assignmentOperation.has_value() 
        || !syrec_operation::invert(*assignmentOperation).has_value()
        || isExpressionEitherBinaryOrShiftExpression(assignmentStmtCasted->rhs)) {
        return {};
    }

    const auto assignmentStmtRhsExprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(assignmentStmtCasted->rhs);
    const auto assignmentStmtRhsExprAsShiftExpr = std::dynamic_pointer_cast<syrec::ShiftExpression>(assignmentStmtCasted->rhs);
    
    if (assignmentStmtRhsExprAsBinaryExpr == nullptr || assignmentStmtRhsExprAsShiftExpr == nullptr) {
        return {};
    }

    if (assignmentStmtRhsExprAsBinaryExpr != nullptr
        && ((isExpressionConstantNumber(assignmentStmtRhsExprAsBinaryExpr->lhs) || doesExpressionDefineSignalAccess(assignmentStmtRhsExprAsBinaryExpr->lhs))
        || (isExpressionConstantNumber(assignmentStmtRhsExprAsBinaryExpr->rhs) || doesExpressionDefineSignalAccess(assignmentStmtRhsExprAsBinaryExpr->rhs)))) {
        return {};
    }
    if (assignmentStmtRhsExprAsShiftExpr != nullptr
        && (isExpressionConstantNumber(assignmentStmtRhsExprAsShiftExpr->lhs) || doesExpressionDefineSignalAccess(assignmentStmtRhsExprAsShiftExpr->lhs))) {
        return {};
    }

    /*
     * Apply rule R2
     */
    if (canAddAssignmentBeReplacedWithXorAssignment(assignmentStmtCasted)) {
        const auto updatedAssignmentStatement = std::make_shared<syrec::AssignStatement>(
                assignmentStmtCasted->lhs,
                syrec::AssignStatement::Exor,
                assignmentStmtCasted->rhs);
        /*
         * Assignment of the form A += B with B being a variable access can optimized to A ^= B if A = 0 prior to the assignment
         */
        if (doesExpressionDefineSignalAccess(assignmentStmtCasted->rhs)) {
            return {updatedAssignmentStatement};
        }
        assignmentStmtCasted = updatedAssignmentStatement;
    }
    
    if (doesExpressionOnlyContainReversibleOperations(assignmentStmtCasted->rhs)) {
        // Check further preconditions
        //const auto operationOfAssignmentExprRhs = assignmentStmtRhsExprAsBinaryExpr != nullptr ? tryMapOperationFlagToEnum(assignmentStmtRhsExprAsBinaryExpr, assignmentStmtRhsExprAsBinaryExpr->op) : std::nullopt;
        //if (operationOfAssignmentExprRhs.has_value() && *operationOfAssignmentExprRhs == *assignmentOperation) {
        //    /*
        //    * TODO: Apply rule R1
        //    */
        //    return {};
        //}

        /*
         * Apply rule R1
         */
        const auto& assignmentStmtSimplifier = std::make_unique<AssignmentWithOnlyReversibleOperationsSimplifier>(symbolTable);
        if (const auto& generatedAssignments     = assignmentStmtSimplifier->simplify(assignmentStmt); !generatedAssignments.empty()) {
            return generatedAssignments;   
        }
        const auto& assignmentStmtSimplifierWithNonUniqueSignalAccesses = std::make_unique<AssignmentWithReversibleOpsAndMultiLevelSignalOccurrence>(symbolTable);
        if (const auto& generatedAssignmentsIfOriginalOneContainedNonUniqueSignalAccesses = assignmentStmtSimplifierWithNonUniqueSignalAccesses->simplify(assignmentStmt); !generatedAssignmentsIfOriginalOneContainedNonUniqueSignalAccesses.empty()) {
            return generatedAssignmentsIfOriginalOneContainedNonUniqueSignalAccesses;
        }
        return {};
    }

    syrec::expression::ptr     lhsExprOfTopMostExprOfAssignmentRhs;
    syrec::expression::ptr     rhsExprOfTopMostExprOfAssignmentRhs = nullptr;
    std::optional<syrec_operation::operation> topMostOperationNodeOfRhsExpr;

    if (assignmentStmtRhsExprAsBinaryExpr != nullptr) {
        topMostOperationNodeOfRhsExpr       = tryMapOperationFlagToEnum(assignmentStmtRhsExprAsBinaryExpr, assignmentStmtRhsExprAsBinaryExpr->op);
        lhsExprOfTopMostExprOfAssignmentRhs = assignmentStmtRhsExprAsBinaryExpr->lhs;
        rhsExprOfTopMostExprOfAssignmentRhs = assignmentStmtRhsExprAsBinaryExpr->rhs;
    } else {
        topMostOperationNodeOfRhsExpr = tryMapOperationFlagToEnum(assignmentStmtRhsExprAsShiftExpr, assignmentStmtRhsExprAsShiftExpr->op);
        lhsExprOfTopMostExprOfAssignmentRhs = assignmentStmtRhsExprAsShiftExpr->lhs;
    }

    const bool isLhsSignalAccessOfAssignmentRhs = doesExpressionDefineSignalAccess(lhsExprOfTopMostExprOfAssignmentRhs);
    const bool isRhsSignalAccessOfAssignmentRhs = rhsExprOfTopMostExprOfAssignmentRhs != nullptr ? doesExpressionDefineSignalAccess(rhsExprOfTopMostExprOfAssignmentRhs) : false;

    syrec::AssignStatement::vec generatedAssignments;
    if ((isLhsSignalAccessOfAssignmentRhs ^ isRhsSignalAccessOfAssignmentRhs) && assignmentStmtRhsExprAsBinaryExpr != nullptr && syrec_operation::invert(*topMostOperationNodeOfRhsExpr).has_value()) {
        bool isLhsRelevantSignalAccess = false;
        if (const auto exprContainingSignalAccessDefinedOnlyOnceInExpr = findExpressionContainingSignalAccessDefinedOnlyOnceInAssignmentRhs(assignmentStmtRhsExprAsBinaryExpr->rhs, isLhsRelevantSignalAccess); exprContainingSignalAccessDefinedOnlyOnceInExpr.has_value()) {
            if (const auto relevantExprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(*exprContainingSignalAccessDefinedOnlyOnceInExpr); relevantExprAsBinaryExpr != nullptr) {
                const auto relevantVariableAccess = std::dynamic_pointer_cast<syrec::VariableExpression>(isLhsRelevantSignalAccess ? relevantExprAsBinaryExpr->lhs : relevantExprAsBinaryExpr->rhs);

                // TODO: R4
                const auto extractedAssignmentStatement = std::make_shared<syrec::AssignStatement>(
                    relevantVariableAccess->var,
                    relevantExprAsBinaryExpr->op,
                    isLhsRelevantSignalAccess ? relevantExprAsBinaryExpr->rhs : relevantExprAsBinaryExpr->lhs
                );
                /*
                 * We cannot use the assigned to signals as potential replacement candidates in optimizations of the nested expressions
                 */
                markAccessedSignalPartsAsNotUsableForReplacement(assignmentStmtCasted->lhs);
                markAccessedSignalPartsAsNotUsableForReplacement(relevantVariableAccess->var);

                /*
                 * I.   Try to simplify the extracted assignment
                 * II.  Apply and aggregate the created assignments of step I.
                 * III. Invert, apply and aggregate the inverted assignments
                 */
                if (const auto& extractedAssignmentStatementSimplified = tryReduceRequiredAdditionalLinesFor(extractedAssignmentStatement); !extractedAssignmentStatementSimplified.empty()) {
                    generatedAssignments.insert(generatedAssignments.end(), extractedAssignmentStatementSimplified.begin(), extractedAssignmentStatementSimplified.end());

                    if (const auto invertedAssignments = invertAssignments(extractedAssignmentStatementSimplified); !invertedAssignments.empty()) {
                        generatedAssignments.insert(extractedAssignmentStatementSimplified.end(), invertedAssignments.begin(), invertedAssignments.end());
                    } else {
                        // Any error during the creation of the inverted assignment statements will also clear the assignment statements that should be inverted 
                        generatedAssignments.clear();
                    }

                    /*for (const auto& assignment: extractedAssignmentStatementSimplified) {
                        applyAssignment(assignment);
                        generatedAssignments.emplace_back(assignment);
                    }*/

                    // TODO: Updated original expr
                }
                
                /*
                 * But after the extracted assignment is created, the previously assigned to signals can now be used as a potential replacement
                 * TODO: When can we lift this restriction exactly, after having optimized the extracted assignment or the full one.
                 */
                markAccessedSignalPartsAsNotUsableForReplacement(assignmentStmtCasted->lhs);
                markAccessedSignalPartsAsUsableForReplacement(relevantVariableAccess->var);

            }
        }
    } else if (isLhsSignalAccessOfAssignmentRhs ^ isRhsSignalAccessOfAssignmentRhs && assignmentStmtRhsExprAsBinaryExpr != nullptr) {
        // TODO: R5
        return simplifyAssignmentBySubstitution(assignmentStmt);    
    }
    return generatedAssignments;
}

bool SimpleAdditionalLineForAssignmentReducer::isExpressionEitherBinaryOrShiftExpression(const syrec::expression::ptr& expr) {
    return std::dynamic_pointer_cast<syrec::BinaryExpression>(expr) != nullptr || std::dynamic_pointer_cast<syrec::ShiftExpression>(expr) != nullptr;
}

bool SimpleAdditionalLineForAssignmentReducer::doesExpressionOnlyContainReversibleOperations(const syrec::expression::ptr& expr) {
    if (const auto exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr); exprAsBinaryExpr != nullptr) {
        const auto mappedToOperationFlagOfBinaryExpr = tryMapOperationFlagToEnum(exprAsBinaryExpr, exprAsBinaryExpr->op);
        if (!mappedToOperationFlagOfBinaryExpr.has_value() || syrec_operation::invert(*mappedToOperationFlagOfBinaryExpr).has_value() || !syrec_operation::getMatchingAssignmentOperationForOperation(*mappedToOperationFlagOfBinaryExpr).has_value()) {
            return false;
        }

        bool doesLhsExprContainOnlyReversibleOperations = false;
        if (const auto lhsExprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(exprAsBinaryExpr->lhs); lhsExprAsBinaryExpr != nullptr) {
            doesLhsExprContainOnlyReversibleOperations = doesExpressionOnlyContainReversibleOperations(lhsExprAsBinaryExpr);
        } else if (const auto lhsExprAsVariableAccess = std::dynamic_pointer_cast<syrec::VariableExpression>(exprAsBinaryExpr->lhs); lhsExprAsVariableAccess != nullptr) {
            doesLhsExprContainOnlyReversibleOperations = true;    
        }

        if (doesLhsExprContainOnlyReversibleOperations) {
            if (const auto rhsExprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(exprAsBinaryExpr->rhs); rhsExprAsBinaryExpr != nullptr) {
                return doesExpressionOnlyContainReversibleOperations(rhsExprAsBinaryExpr);
            } if (const auto rhsExprAsVariableAccess = std::dynamic_pointer_cast<syrec::VariableExpression>(exprAsBinaryExpr->rhs); rhsExprAsVariableAccess != nullptr) {
                return true;
            }   
        }
    }
    return false;
}

bool SimpleAdditionalLineForAssignmentReducer::doesExpressionDefineSignalAccess(const syrec::expression::ptr& expr) {
    return std::dynamic_pointer_cast<syrec::VariableExpression>(expr) != nullptr;
}

bool SimpleAdditionalLineForAssignmentReducer::isExpressionConstantNumber(const syrec::expression::ptr& expr) {
    if (const auto exprAsNumericExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(expr); exprAsNumericExpr != nullptr) {
        return exprAsNumericExpr->value->isConstant();
    }
    return false;
}

std::optional<syrec_operation::operation> SimpleAdditionalLineForAssignmentReducer::tryMapAssignmentOperationFlagToEnum(unsigned operationFlag) {
    switch (operationFlag) {
        case syrec::AssignStatement::Add:
            return std::make_optional(syrec_operation::operation::add_assign);
        case syrec::AssignStatement::Subtract:
            return std::make_optional(syrec_operation::operation::minus_assign);
        case syrec::AssignStatement::Exor:
            return std::make_optional(syrec_operation::operation::xor_assign);
        default:
            return std::nullopt;
    }
}

std::optional<syrec_operation::operation> SimpleAdditionalLineForAssignmentReducer::tryMapBinaryOperationFlagToEnum(unsigned operationFlag) {
    switch (operationFlag) {
        case syrec::BinaryExpression::Add:
            return std::make_optional(syrec_operation::operation::addition);
        case syrec::BinaryExpression::Subtract:
            return std::make_optional(syrec_operation::operation::subtraction);
        case syrec::BinaryExpression::Exor:
            return std::make_optional(syrec_operation::operation::bitwise_xor);
        case syrec::BinaryExpression::Multiply:
            return std::make_optional(syrec_operation::operation::multiplication);
        case syrec::BinaryExpression::Divide:
            return std::make_optional(syrec_operation::operation::division);
        case syrec::BinaryExpression::Modulo:
            return std::make_optional(syrec_operation::operation::modulo);
        case syrec::BinaryExpression::FracDivide:
            return std::make_optional(syrec_operation::operation::upper_bits_multiplication);
        case syrec::BinaryExpression::LogicalAnd:
            return std::make_optional(syrec_operation::operation::logical_and);
        case syrec::BinaryExpression::LogicalOr:
            return std::make_optional(syrec_operation::operation::logical_or);
        case syrec::BinaryExpression::BitwiseAnd:
            return std::make_optional(syrec_operation::operation::bitwise_and);
        case syrec::BinaryExpression::BitwiseOr:
            return std::make_optional(syrec_operation::operation::bitwise_or);
        case syrec::BinaryExpression::LessThan:
            return std::make_optional(syrec_operation::operation::less_than);
        case syrec::BinaryExpression::GreaterThan:
            return std::make_optional(syrec_operation::operation::greater_than);
        case syrec::BinaryExpression::Equals:
            return std::make_optional(syrec_operation::operation::equals);
        case syrec::BinaryExpression::NotEquals:
            return std::make_optional(syrec_operation::operation::not_equals);
        case syrec::BinaryExpression::LessEquals:
            return std::make_optional(syrec_operation::operation::less_equals);
        case syrec::BinaryExpression::GreaterEquals:
            return std::make_optional(syrec_operation::operation::greater_equals);
        default:
            return std::nullopt;
    }
}

std::optional<syrec_operation::operation> SimpleAdditionalLineForAssignmentReducer::tryMapShiftOperationFlagToEnum(unsigned operationFlag) {
    if (operationFlag == syrec::ShiftExpression::Left) {
        return std::make_optional(syrec_operation::operation::shift_left);
    }
    if (operationFlag == syrec::ShiftExpression::Right) {
        return std::make_optional(syrec_operation::operation::shift_right);
    }
    return std::nullopt;
}

std::optional<syrec_operation::operation> SimpleAdditionalLineForAssignmentReducer::tryMapOperationFlagToEnum(const syrec::expression::ptr& expr, unsigned operationFlag) {
    if (const auto& exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr); exprAsBinaryExpr != nullptr) {
        return tryMapBinaryOperationFlagToEnum(operationFlag);
    }
    if (const auto& exprAsShiftExpr = std::dynamic_pointer_cast<syrec::ShiftExpression>(expr); exprAsShiftExpr != nullptr) {
        return tryMapShiftOperationFlagToEnum(operationFlag);
    }
    return std::nullopt;
}

std::optional<unsigned> SimpleAdditionalLineForAssignmentReducer::tryMapAssignmentOperationEnumToFlag(syrec_operation::operation assignmentOperation) {
    switch (assignmentOperation) {
        case syrec_operation::operation::add_assign:
            return std::make_optional(syrec::AssignStatement::Add);
        case syrec_operation::operation::minus_assign:
            return std::make_optional(syrec::AssignStatement::Subtract);
        case syrec_operation::operation::xor_assign:
            return std::make_optional(syrec::AssignStatement::Exor);
        default:
            return std::nullopt;
    }
}

// TODO: Implement me
bool SimpleAdditionalLineForAssignmentReducer::doVariableAccessesOverlap(const syrec::VariableAccess::ptr& signalPartsToCheck, const syrec::VariableAccess::ptr& signalPartsToBeCheckedForOverlap) {
    return false;
}

syrec::Statement::vec SimpleAdditionalLineForAssignmentReducer::invertAssignments(const syrec::AssignStatement::vec& assignments) {
    syrec::Statement::vec invertedAssignments;
    std::transform(
            assignments.cbegin(),
            assignments.cend(),
            std::back_inserter(invertedAssignments),
            [](const syrec::AssignStatement::ptr& assignmentStmt) {
                const auto assignmentCasted = std::dynamic_pointer_cast<syrec::AssignStatement>(assignmentStmt);
                return std::make_shared<syrec::AssignStatement>(
                        assignmentCasted->lhs,
                        *tryMapAssignmentOperationEnumToFlag(*syrec_operation::invert(*tryMapAssignmentOperationFlagToEnum(assignmentCasted->op))),
                        assignmentCasted->rhs);
            });
    return invertedAssignments;
}

std::optional<unsigned> SimpleAdditionalLineForAssignmentReducer::tryEvaluateNumberAsConstant(const syrec::Number::ptr& number) {
    return number->isConstant() ? std::make_optional(number->evaluate({})) : std::nullopt;
}

bool SimpleAdditionalLineForAssignmentReducer::doBitRangesOverlap(const optimizations::BitRangeAccessRestriction::BitRangeAccess& thisBitRange, const optimizations::BitRangeAccessRestriction::BitRangeAccess& thatBitRange) {
    const bool doesAssignedToBitRangePrecedeAccessedBitRange = thisBitRange.first < thatBitRange.first;
    const bool doesAssignedToBitRangeExceedAccessedBitRange  = thisBitRange.second > thatBitRange.second;
    /*
     * Check cases:
     * Assigned TO: |---|
     * Accessed BY:      |---|
     *
     * and
     * Assigned TO:      |---|
     * Accessed BY: |---|
     *
     */
    if ((doesAssignedToBitRangePrecedeAccessedBitRange && thisBitRange.second < thatBitRange.first) || (doesAssignedToBitRangeExceedAccessedBitRange && thisBitRange.first > thatBitRange.second)) {
        return false;
    }
    return true;
}

bool SimpleAdditionalLineForAssignmentReducer::doDimensionAccessesOverlap(const TransformedDimensionAccess& thisDimensionAccess, const TransformedDimensionAccess& thatDimensionAccess) {
    return std::mismatch(
        thisDimensionAccess.cbegin(),
        thisDimensionAccess.cend(),
        thatDimensionAccess.cbegin(),
        thatDimensionAccess.cend(),
   [](const std::optional<unsigned int>& accessedValueOfDimensionOfAssignedToSignal, const std::optional<unsigned int>& accessedValueOfDimensionOfAccessedToSignal) {
            if (accessedValueOfDimensionOfAssignedToSignal.has_value() && accessedValueOfDimensionOfAccessedToSignal.has_value()) {
                return *accessedValueOfDimensionOfAssignedToSignal != *accessedValueOfDimensionOfAccessedToSignal;
            }
            return false;
        }).first != thisDimensionAccess.cend();
}

std::shared_ptr<SimpleAdditionalLineForAssignmentReducer::VariableAccessCountLookup> SimpleAdditionalLineForAssignmentReducer::buildVariableAccessCountsForExpr(const syrec::expression::ptr& expr, const EstimatedSignalAccessSize& requiredSizeForSignalAccessToBeConsidered) const {
    auto buildLookup = std::make_shared<VariableAccessCountLookup>();
    buildVariableAccessCountsForExpr(expr, buildLookup, requiredSizeForSignalAccessToBeConsidered);
    return buildLookup;
}

void SimpleAdditionalLineForAssignmentReducer::buildVariableAccessCountsForExpr(const syrec::expression::ptr& expr, std::shared_ptr<VariableAccessCountLookup>& lookupToFill, const EstimatedSignalAccessSize& requiredSizeForSignalAccessToBeConsidered) const {
    if (const auto& exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr); exprAsBinaryExpr != nullptr) {
        buildVariableAccessCountsForExpr(exprAsBinaryExpr->lhs, lookupToFill, requiredSizeForSignalAccessToBeConsidered);
        buildVariableAccessCountsForExpr(exprAsBinaryExpr->rhs, lookupToFill, requiredSizeForSignalAccessToBeConsidered);
    } else if (const auto& exprAsShiftExpr = std::dynamic_pointer_cast<syrec::ShiftExpression>(expr); exprAsShiftExpr != nullptr) {
        buildVariableAccessCountsForExpr(exprAsShiftExpr->lhs, lookupToFill, requiredSizeForSignalAccessToBeConsidered);
    } else if (const auto& exprAsVariableExpr = std::dynamic_pointer_cast<syrec::VariableExpression>(expr); exprAsVariableExpr != nullptr) {
        if (doesSignalAccessMatchExpectedSize(exprAsVariableExpr->var, requiredSizeForSignalAccessToBeConsidered)) {
            auto& variableAccessCountsLookup = lookupToFill->lookup;
            const auto& accessedSignalIdentInExpr        = exprAsVariableExpr->var->var->name;
            if (variableAccessCountsLookup.count(accessedSignalIdentInExpr) == 0) {
                variableAccessCountsLookup.emplace(std::make_pair(accessedSignalIdentInExpr, std::vector(1, VariableAccessCountPair(1, exprAsVariableExpr->var))));
            } else {
                /*
                 * Either increment the usage count of an already found signal access that matches the required size or insert a new entry for the accessed signal
                 */
                auto&       alreadyFoundMatchingSignalAccessesForAccessedSignal = variableAccessCountsLookup.at(accessedSignalIdentInExpr);
                const auto& findExistingOverlappingVariableAccess               = std::find_if(
              alreadyFoundMatchingSignalAccessesForAccessedSignal.begin(),
              alreadyFoundMatchingSignalAccessesForAccessedSignal.end(),
              [&exprAsVariableExpr](const VariableAccessCountPair& signalAccess) {
                        return doVariableAccessesOverlap(signalAccess.accessedSignalParts, exprAsVariableExpr->var);
                    });

                if (findExistingOverlappingVariableAccess == alreadyFoundMatchingSignalAccessesForAccessedSignal.end()) {
                    alreadyFoundMatchingSignalAccessesForAccessedSignal.emplace_back(VariableAccessCountPair(1, exprAsVariableExpr->var));
                } else {
                    findExistingOverlappingVariableAccess->accessCount++;
                }
            }
        }
    }
}

bool SimpleAdditionalLineForAssignmentReducer::doesSignalAccessMatchExpectedSize(const syrec::VariableAccess::ptr& signalAccessToCheck, const EstimatedSignalAccessSize& requiredSizeForSignalAccessToBeConsidered) const {
    return getSizeOfSignalAccess(signalAccessToCheck) == requiredSizeForSignalAccessToBeConsidered;
}

SimpleAdditionalLineForAssignmentReducer::EstimatedSignalAccessSize SimpleAdditionalLineForAssignmentReducer::getSizeOfSignalAccess(const syrec::VariableAccess::ptr& signalAccess) const {
    const auto matchingEntryInSymbolTable = symbolTable->getVariable(signalAccess->var->name);
    if (!matchingEntryInSymbolTable.has_value() || std::holds_alternative<syrec::Variable::ptr>(*matchingEntryInSymbolTable)) {
        return EstimatedSignalAccessSize({}, 0);
    }

    const auto& matchingSignal = std::get<syrec::Variable::ptr>(*matchingEntryInSymbolTable);
    std::vector<unsigned int> sizeOfDimensions;

    const auto numNotDefinedValuesOfDimensions = signalAccess->indexes.size() >= matchingSignal->dimensions.size() ? 0 : matchingSignal->dimensions.size() - signalAccess->indexes.size();
    if (numNotDefinedValuesOfDimensions == 0) {
        sizeOfDimensions.emplace_back(1);    
    } else {
        for (std::size_t i = signalAccess->indexes.size(); i < matchingSignal->dimensions.size(); ++i) {
            sizeOfDimensions.emplace_back(matchingSignal->dimensions.at(i));
        }
    }

    unsigned int bitWidth = matchingSignal->bitwidth;
    if (signalAccess->range.has_value()) {
        const auto bitRangeStartEvaluated = tryEvaluateNumberAsConstant(signalAccess->range->first);
        const auto bitRangeEndEvaluated   = tryEvaluateNumberAsConstant(signalAccess->range->second);

        // TODO: We are currently not supporting bit range accesses of the form x:15.2 (used in VHDL, etc.)
        // TODO: We could assume that all bit range accesses are well formed but that could lead to some errors
        if (bitRangeStartEvaluated.has_value() && bitRangeEndEvaluated.has_value() && *bitRangeStartEvaluated < *bitRangeEndEvaluated) {
            bitWidth = (*bitRangeEndEvaluated - *bitRangeStartEvaluated) + 1;
        }
    }
    return EstimatedSignalAccessSize(sizeOfDimensions, bitWidth);
}

bool SimpleAdditionalLineForAssignmentReducer::canAddAssignmentBeReplacedWithXorAssignment(const syrec::AssignStatement::ptr& assignmentStmt) const {
    const auto assignmentStmtCasted = std::dynamic_pointer_cast<syrec::AssignStatement>(assignmentStmt);
    if (assignmentStmtCasted == nullptr) {
        return false;
    }

    const auto definedAssignmentOperation = tryMapAssignmentOperationFlagToEnum(assignmentStmtCasted->op);
    if (!definedAssignmentOperation.has_value() || *definedAssignmentOperation != syrec_operation::operation::add_assign) {
        return false;
    }

    const auto fetchedValueForAssignedToSignalPartsPriorToAssignment = symbolTable->tryFetchValueForLiteral(assignmentStmtCasted->lhs);
    return fetchedValueForAssignedToSignalPartsPriorToAssignment.has_value() && fetchedValueForAssignedToSignalPartsPriorToAssignment.value() == 0;
}

// TODO: Implement me
std::optional<syrec::expression::ptr> SimpleAdditionalLineForAssignmentReducer::findExpressionContainingSignalAccessDefinedOnlyOnceInAssignmentRhs(const syrec::BinaryExpression::ptr& assignmentStatement, bool& isLhsRelevantSignalAccess) {
    return std::nullopt;
}

SimpleAdditionalLineForAssignmentReducer::LookupOfExcludedSignalsForReplacement SimpleAdditionalLineForAssignmentReducer::createLookupForSignalsNotUsableAsReplacementsFor(const syrec::expression::ptr& expr) const {
    LookupOfExcludedSignalsForReplacement createdLookup = std::make_unique<std::map<std::string_view, std::vector<NotUsableAsReplacementSignalParts>>>();
    for (const auto& [signalIdent, existingAssignmentsForSignal] : activeAssignments) {
        createdLookup->insert(std::make_pair(signalIdent, existingAssignmentsForSignal));
    }
    createLookupForSignalsNotUsableAsReplacementsFor(expr, createdLookup);
    return createdLookup;
}

void SimpleAdditionalLineForAssignmentReducer::createLookupForSignalsNotUsableAsReplacementsFor(const syrec::expression::ptr& expr, LookupOfExcludedSignalsForReplacement& lookupToFill) const {
    if (const auto& exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr); exprAsBinaryExpr != nullptr) {
        createLookupForSignalsNotUsableAsReplacementsFor(exprAsBinaryExpr->lhs, lookupToFill);
        createLookupForSignalsNotUsableAsReplacementsFor(exprAsBinaryExpr->rhs, lookupToFill);
    } else if (const auto& exprAsShiftExpr = std::dynamic_pointer_cast<syrec::ShiftExpression>(expr); exprAsShiftExpr != nullptr) {
        createLookupForSignalsNotUsableAsReplacementsFor(exprAsShiftExpr->lhs, lookupToFill);
    } else if (const auto& exprAsVariableExpr = std::dynamic_pointer_cast<syrec::VariableExpression>(expr); exprAsVariableExpr != nullptr) {
        if (!doesAssignmentToAccessedSignalPartsAlreadyExists(exprAsVariableExpr->var)) {
            const auto& accessedSignalIdent = exprAsVariableExpr->var->var->name;
            if (lookupToFill->count(accessedSignalIdent) == 0) {
                lookupToFill->insert(std::make_pair(accessedSignalIdent, std::vector(1, NotUsableAsReplacementSignalParts(exprAsVariableExpr->var))));
            } else {
                auto&      blockedSignalParts                 = lookupToFill->at(accessedSignalIdent);
                const auto existingEntryMatchingAccessedParts = std::find_if(
                blockedSignalParts.cbegin(),
                blockedSignalParts.cend(),
                [&exprAsVariableExpr](const NotUsableAsReplacementSignalParts& existingEntry) {
                    return doVariableAccessesOverlap(existingEntry.blockedSignalParts, exprAsVariableExpr->var);
                });

                if (existingEntryMatchingAccessedParts == blockedSignalParts.cend()) {
                    blockedSignalParts.emplace_back(exprAsVariableExpr->var);
                }
            }
        }
    }
}

// TODO: Implement me
bool SimpleAdditionalLineForAssignmentReducer::doesAssignmentToAccessedSignalPartsAlreadyExists(const syrec::VariableAccess::ptr& accessedSignalParts) const {
    return false;
}

// TODO: Implement me
std::optional<syrec::VariableAccess::ptr> SimpleAdditionalLineForAssignmentReducer::tryCreateSubstituteForExpr(const syrec::expression::ptr& expr, const LookupOfExcludedSignalsForReplacement& signalPartsToExcludeAsPotentialReplacements) {
    return std::nullopt;
}