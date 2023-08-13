#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/main_additional_line_for_assignment_simplifier.hpp"

#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/assignment_with_none_reversible_operations_and_unique_signal_occurrences_simplifier.hpp"
#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/assignment_with_only_reversible_operations_simplifier.hpp"
#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/assignment_with_reversible_ops_and_multilevel_signal_occurrence_simplifier.hpp"


// TODO: Determine whether there is a difference in the gate cost between the statements a += (b - c) and a += b; a -= c (or in other words whether the split of a binary expression into unary assignments make sense)
// TODO: Determine whether the optimization of converting an assignment of the form a += 1 to ++= a is an actual optimization (since the are semantically equivalent and would probabily generate the same code in assembly)
using namespace noAdditionalLineSynthesis;

syrec::AssignStatement::vec MainAdditionalLineForAssignmentSimplifier::tryReduceRequiredAdditionalLinesFor(const syrec::AssignStatement::ptr& assignmentStmt, bool isValueOfAssignedToSignalBlockedByDataFlowAnalysis) const {
    auto assignmentStmtCasted = std::dynamic_pointer_cast<syrec::AssignStatement>(assignmentStmt);
    if (assignmentStmtCasted == nullptr) {
        return {};
    }

    if (const auto& assignmentRhsAsNumericExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(assignmentStmtCasted->rhs); assignmentRhsAsNumericExpr != nullptr && assignmentRhsAsNumericExpr->value->isCompileTimeConstantExpression()) {
        const auto& convertedCompileTimeConstantExpr = tryMapNumberToExpression(assignmentRhsAsNumericExpr->value, 0, symbolTable);
        if (!convertedCompileTimeConstantExpr.has_value()) {
            return {};
        }

        assignmentStmtCasted = std::make_shared<syrec::AssignStatement>(
            assignmentStmtCasted->lhs,
            assignmentStmtCasted->op,
            *convertedCompileTimeConstantExpr
        );
    }

    if (const auto& reorderExprResult = tryPerformReorderingOfSubexpressions(assignmentStmtCasted->rhs); reorderExprResult.has_value()) {
        assignmentStmtCasted = std::make_shared<syrec::AssignStatement>(
                assignmentStmtCasted->lhs,
                assignmentStmtCasted->op,
                *reorderExprResult);
    }

    const auto assignmentOperation = syrec_operation::tryMapAssignmentOperationFlagToEnum(assignmentStmtCasted->op);
    if (!assignmentOperation.has_value() 
        || !syrec_operation::invert(*assignmentOperation).has_value()
        || !isExpressionEitherBinaryOrShiftExpression(assignmentStmtCasted->rhs)) {
        return {};
    }

    const auto assignmentStmtRhsExprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(assignmentStmtCasted->rhs);
    const auto assignmentStmtRhsExprAsShiftExpr = std::dynamic_pointer_cast<syrec::ShiftExpression>(assignmentStmtCasted->rhs);
    
    if (assignmentStmtRhsExprAsBinaryExpr == nullptr && assignmentStmtRhsExprAsShiftExpr == nullptr) {
        return {};
    }

    /*
     * Apply rule R2
     */
    //if (canAddAssignmentBeReplacedWithXorAssignment(assignmentStmtCasted, isValueOfAssignedToSignalBlockedByDataFlowAnalysis)) {
    //    const auto updatedAssignmentStatement = std::make_shared<syrec::AssignStatement>(
    //            assignmentStmtCasted->lhs,
    //            syrec::AssignStatement::Exor,
    //            assignmentStmtCasted->rhs);
    //    /*
    //     * Assignment of the form A += B with B being a variable access can optimized to A ^= B if A = 0 prior to the assignment
    //     */
    //    if (doesExpressionDefineSignalAccess(assignmentStmtCasted->rhs)) {
    //        return {updatedAssignmentStatement};
    //    }
    //    assignmentStmtCasted = updatedAssignmentStatement;
    //}

    const bool isExprShiftExprWithoutNestedSubexpressions = assignmentStmtRhsExprAsBinaryExpr != nullptr
        && ((isExpressionConstantNumber(assignmentStmtRhsExprAsBinaryExpr->lhs) || doesExpressionDefineSignalAccess(assignmentStmtRhsExprAsBinaryExpr->lhs)) 
            && (isExpressionConstantNumber(assignmentStmtRhsExprAsBinaryExpr->rhs) || doesExpressionDefineSignalAccess(assignmentStmtRhsExprAsBinaryExpr->rhs)));
    const bool isExprBinaryExprWithoutNestedSubexpressions = assignmentStmtRhsExprAsShiftExpr != nullptr
        && (isExpressionConstantNumber(assignmentStmtRhsExprAsShiftExpr->lhs) || doesExpressionDefineSignalAccess(assignmentStmtRhsExprAsShiftExpr->lhs));

    if (isExprShiftExprWithoutNestedSubexpressions || isExprBinaryExprWithoutNestedSubexpressions) {
        /*
        * Apply rule R2
        */
        if (canAddAssignmentBeReplacedWithXorAssignment(assignmentStmtCasted, isValueOfAssignedToSignalBlockedByDataFlowAnalysis)) {
            const auto updatedAssignmentStatement = std::make_shared<syrec::AssignStatement>(
                    assignmentStmtCasted->lhs,
                    syrec::AssignStatement::Exor,
                    assignmentStmtCasted->rhs);
            return {updatedAssignmentStatement};
        }
        return {};
    }
    
    //if (doesExpressionOnlyContainReversibleOperations(assignmentStmtCasted->rhs)) {
    //    // Check further preconditions
    //    //const auto operationOfAssignmentExprRhs = assignmentStmtRhsExprAsBinaryExpr != nullptr ? tryMapOperationFlagToEnum(assignmentStmtRhsExprAsBinaryExpr, assignmentStmtRhsExprAsBinaryExpr->op) : std::nullopt;
    //    //if (operationOfAssignmentExprRhs.has_value() && *operationOfAssignmentExprRhs == *assignmentOperation) {
    //    //    /*
    //    //    * TODO: Apply rule R1
    //    //    */
    //    //    return {};
    //    //}

    //    /*
    //     * Apply rule R1
    //     */
    //    const auto& assignmentStmtSimplifier = std::make_unique<AssignmentWithOnlyReversibleOperationsSimplifier>(symbolTable);
    //    if (const auto& generatedAssignments     = assignmentStmtSimplifier->simplify(assignmentStmt, isValueOfAssignedToSignalBlockedByDataFlowAnalysis); !generatedAssignments.empty()) {
    //        return generatedAssignments;   
    //    }
    //    const auto& assignmentStmtSimplifierWithNonUniqueSignalAccesses = std::make_unique<AssignmentWithReversibleOpsAndMultiLevelSignalOccurrence>(symbolTable);
    //    if (const auto& generatedAssignmentsIfOriginalOneContainedNonUniqueSignalAccesses = assignmentStmtSimplifierWithNonUniqueSignalAccesses->simplify(assignmentStmt, isValueOfAssignedToSignalBlockedByDataFlowAnalysis); !generatedAssignmentsIfOriginalOneContainedNonUniqueSignalAccesses.empty()) {
    //        return generatedAssignmentsIfOriginalOneContainedNonUniqueSignalAccesses;
    //    }
    //    return {};
    //}

    //const auto& assignmentWithNonReversibleOperationsSimplifier = std::make_unique<AssignmentWithNonReversibleOperationsAndUniqueSignalOccurrencesSimplifier>(symbolTable);
    //if (const auto& generatedAssignments = assignmentWithNonReversibleOperationsSimplifier->simplify(assignmentStmt, isValueOfAssignedToSignalBlockedByDataFlowAnalysis); !generatedAssignments.empty()) {
    //    return generatedAssignments;
    //}

    /*const auto& assignmentStmtSimplifierWithNonUniqueSignalAccesses = std::make_unique<AssignmentWithReversibleOpsAndMultiLevelSignalOccurrence>(symbolTable);
    if (const auto& generatedAssignmentsIfOriginalOneContainedNonUniqueSignalAccesses = assignmentStmtSimplifierWithNonUniqueSignalAccesses->simplify(assignmentStmt, isValueOfAssignedToSignalBlockedByDataFlowAnalysis); !generatedAssignmentsIfOriginalOneContainedNonUniqueSignalAccesses.empty()) {
        return generatedAssignmentsIfOriginalOneContainedNonUniqueSignalAccesses;
    } else {
        const auto& assignmentWithNonReversibleOperationsSimplifier = std::make_unique<AssignmentWithNonReversibleOperationsAndUniqueSignalOccurrencesSimplifier>(symbolTable);
        if (const auto& generatedAssignments = assignmentWithNonReversibleOperationsSimplifier->simplify(assignmentStmt, isValueOfAssignedToSignalBlockedByDataFlowAnalysis); !generatedAssignments.empty()) {
            return generatedAssignments;
        }    
    }*/

    const auto& assignmentWithNonReversibleOperationsSimplifier = std::make_unique<AssignmentWithNonReversibleOperationsAndUniqueSignalOccurrencesSimplifier>(symbolTable);
    if (const auto& generatedAssignments = assignmentWithNonReversibleOperationsSimplifier->simplify(assignmentStmt, isValueOfAssignedToSignalBlockedByDataFlowAnalysis); !generatedAssignments.empty()) {
        return generatedAssignments;
    }
    /*
     * We should not apply rule R2 before trying to simplify the rhs expression as this could prevent some optimizations:
     * i.e. replacing the += operation with ^= in the assignment a += (<subExpr_2> - <subExpr_1>) will prevent the split of the rhs expr that would otherwise create the subassignments
     * a += <subExpr_2>; a -= <subExpr_1>
     */
    if (canAddAssignmentBeReplacedWithXorAssignment(assignmentStmtCasted, isValueOfAssignedToSignalBlockedByDataFlowAnalysis)) {
        const auto updatedAssignmentStatement = std::make_shared<syrec::AssignStatement>(
                assignmentStmtCasted->lhs,
                syrec::AssignStatement::Exor,
                assignmentStmtCasted->rhs);
        return {updatedAssignmentStatement};
    }

    return {assignmentStmtCasted};
    
    // TODO: Ignore this for now
    /*syrec::expression::ptr     lhsExprOfTopMostExprOfAssignmentRhs;
    syrec::expression::ptr     rhsExprOfTopMostExprOfAssignmentRhs = nullptr;
    std::optional<syrec_operation::operation> topMostOperationNodeOfRhsExpr;

    if (assignmentStmtRhsExprAsBinaryExpr != nullptr) {
        topMostOperationNodeOfRhsExpr       = syrec_operation::tryMapBinaryOperationFlagToEnum(assignmentStmtRhsExprAsBinaryExpr->op);
        lhsExprOfTopMostExprOfAssignmentRhs = assignmentStmtRhsExprAsBinaryExpr->lhs;
        rhsExprOfTopMostExprOfAssignmentRhs = assignmentStmtRhsExprAsBinaryExpr->rhs;
    } else {
        topMostOperationNodeOfRhsExpr = syrec_operation::tryMapShiftOperationFlagToEnum(assignmentStmtRhsExprAsShiftExpr->op);
        lhsExprOfTopMostExprOfAssignmentRhs = assignmentStmtRhsExprAsShiftExpr->lhs;
    }*/

    //const bool isLhsSignalAccessOfAssignmentRhs = doesExpressionDefineSignalAccess(lhsExprOfTopMostExprOfAssignmentRhs);
    //const bool isRhsSignalAccessOfAssignmentRhs = rhsExprOfTopMostExprOfAssignmentRhs != nullptr ? doesExpressionDefineSignalAccess(rhsExprOfTopMostExprOfAssignmentRhs) : false;

    //syrec::AssignStatement::vec generatedAssignments;
    //if ((isLhsSignalAccessOfAssignmentRhs ^ isRhsSignalAccessOfAssignmentRhs) && assignmentStmtRhsExprAsBinaryExpr != nullptr && syrec_operation::invert(*topMostOperationNodeOfRhsExpr).has_value()) {
    //    bool isLhsRelevantSignalAccess = false;
    //    if (const auto exprContainingSignalAccessDefinedOnlyOnceInExpr = findExpressionContainingSignalAccessDefinedOnlyOnceInAssignmentRhs(assignmentStmtRhsExprAsBinaryExpr->rhs, isLhsRelevantSignalAccess); exprContainingSignalAccessDefinedOnlyOnceInExpr.has_value()) {
    //        if (const auto relevantExprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(*exprContainingSignalAccessDefinedOnlyOnceInExpr); relevantExprAsBinaryExpr != nullptr) {
    //            const auto relevantVariableAccess = std::dynamic_pointer_cast<syrec::VariableExpression>(isLhsRelevantSignalAccess ? relevantExprAsBinaryExpr->lhs : relevantExprAsBinaryExpr->rhs);

    //            // TODO: R4
    //            const auto extractedAssignmentStatement = std::make_shared<syrec::AssignStatement>(
    //                relevantVariableAccess->var,
    //                relevantExprAsBinaryExpr->op,
    //                isLhsRelevantSignalAccess ? relevantExprAsBinaryExpr->rhs : relevantExprAsBinaryExpr->lhs
    //            );
    //            /*
    //             * We cannot use the assigned to signals as potential replacement candidates in optimizations of the nested expressions
    //             */
    //            markAccessedSignalPartsAsNotUsableForReplacement(assignmentStmtCasted->lhs);
    //            markAccessedSignalPartsAsNotUsableForReplacement(relevantVariableAccess->var);

    //            /*
    //             * I.   Try to simplify the extracted assignment
    //             * II.  Apply and aggregate the created assignments of step I.
    //             * III. Invert, apply and aggregate the inverted assignments
    //             */
    //            if (const auto& extractedAssignmentStatementSimplified = tryReduceRequiredAdditionalLinesFor(extractedAssignmentStatement); !extractedAssignmentStatementSimplified.empty()) {
    //                generatedAssignments.insert(generatedAssignments.end(), extractedAssignmentStatementSimplified.begin(), extractedAssignmentStatementSimplified.end());

    //                if (const auto invertedAssignments = invertAssignments(extractedAssignmentStatementSimplified); !invertedAssignments.empty()) {
    //                    generatedAssignments.insert(extractedAssignmentStatementSimplified.end(), invertedAssignments.begin(), invertedAssignments.end());
    //                } else {
    //                    // Any error during the creation of the inverted assignment statements will also clear the assignment statements that should be inverted 
    //                    generatedAssignments.clear();
    //                }

    //                /*for (const auto& assignment: extractedAssignmentStatementSimplified) {
    //                    applyAssignment(assignment);
    //                    generatedAssignments.emplace_back(assignment);
    //                }*/

    //                // TODO: Updated original expr
    //            }
    //            
    //            /*
    //             * But after the extracted assignment is created, the previously assigned to signals can now be used as a potential replacement
    //             * TODO: When can we lift this restriction exactly, after having optimized the extracted assignment or the full one.
    //             */
    //            markAccessedSignalPartsAsNotUsableForReplacement(assignmentStmtCasted->lhs);
    //            markAccessedSignalPartsAsUsableForReplacement(relevantVariableAccess->var);

    //        }
    //    }
    //} else if (isLhsSignalAccessOfAssignmentRhs ^ isRhsSignalAccessOfAssignmentRhs && assignmentStmtRhsExprAsBinaryExpr != nullptr) {
    //    // TODO: R5
    //    return simplifyAssignmentBySubstitution(assignmentStmt);    
    //}
    //return generatedAssignments;
}

bool MainAdditionalLineForAssignmentSimplifier::isExpressionEitherBinaryOrShiftExpression(const syrec::expression::ptr& expr) {
    return std::dynamic_pointer_cast<syrec::BinaryExpression>(expr) != nullptr || std::dynamic_pointer_cast<syrec::ShiftExpression>(expr) != nullptr;
}

bool MainAdditionalLineForAssignmentSimplifier::doesExpressionOnlyContainReversibleOperations(const syrec::expression::ptr& expr) {
    if (const auto exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr); exprAsBinaryExpr != nullptr) {
        const auto mappedToOperationFlagOfBinaryExpr = syrec_operation::tryMapBinaryOperationFlagToEnum(exprAsBinaryExpr->op);
        if (!mappedToOperationFlagOfBinaryExpr.has_value() || !syrec_operation::getMatchingAssignmentOperationForOperation(*mappedToOperationFlagOfBinaryExpr).has_value() 
            || !syrec_operation::invert(*syrec_operation::getMatchingAssignmentOperationForOperation(*mappedToOperationFlagOfBinaryExpr)).has_value()
            || !syrec_operation::invert(*mappedToOperationFlagOfBinaryExpr).has_value()) {
            return false;
        }

        bool doesLhsExprContainOnlyReversibleOperations = false;
        if (const auto& lhsExprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(exprAsBinaryExpr->lhs); lhsExprAsBinaryExpr != nullptr) {
            doesLhsExprContainOnlyReversibleOperations = doesExpressionOnlyContainReversibleOperations(lhsExprAsBinaryExpr);
        } else if (const auto& lhsExprAsVariableAccess = std::dynamic_pointer_cast<syrec::VariableExpression>(exprAsBinaryExpr->lhs); lhsExprAsVariableAccess != nullptr) {
            doesLhsExprContainOnlyReversibleOperations = true;    
        } else if (const auto& lhsExprAsNumericExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(exprAsBinaryExpr->lhs); lhsExprAsNumericExpr != nullptr) {
            doesLhsExprContainOnlyReversibleOperations = true;
        }

        if (doesLhsExprContainOnlyReversibleOperations) {
            if (const auto& rhsExprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(exprAsBinaryExpr->rhs); rhsExprAsBinaryExpr != nullptr) {
                return doesExpressionOnlyContainReversibleOperations(rhsExprAsBinaryExpr);
            } if (const auto& rhsExprAsVariableAccess = std::dynamic_pointer_cast<syrec::VariableExpression>(exprAsBinaryExpr->rhs); rhsExprAsVariableAccess != nullptr) {
                return true;
            } if (const auto& rhsExprAsNumericExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(exprAsBinaryExpr->rhs); rhsExprAsNumericExpr != nullptr) {
                return true;
            }   
        }
    }
    return false;
}

bool MainAdditionalLineForAssignmentSimplifier::doesExpressionDefineSignalAccess(const syrec::expression::ptr& expr) {
    return std::dynamic_pointer_cast<syrec::VariableExpression>(expr) != nullptr;
}

bool MainAdditionalLineForAssignmentSimplifier::isExpressionConstantNumber(const syrec::expression::ptr& expr) {
    if (const auto exprAsNumericExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(expr); exprAsNumericExpr != nullptr) {
        return exprAsNumericExpr->value->isConstant();
    }
    return false;
}

// TODO: Implement me
bool MainAdditionalLineForAssignmentSimplifier::doVariableAccessesOverlap(const syrec::VariableAccess::ptr& signalPartsToCheck, const syrec::VariableAccess::ptr& signalPartsToBeCheckedForOverlap) {
    return false;
}

syrec::Statement::vec MainAdditionalLineForAssignmentSimplifier::invertAssignments(const syrec::AssignStatement::vec& assignments) {
    syrec::Statement::vec invertedAssignments;
    std::transform(
            assignments.cbegin(),
            assignments.cend(),
            std::back_inserter(invertedAssignments),
            [](const syrec::AssignStatement::ptr& assignmentStmt) {
                const auto assignmentCasted = std::dynamic_pointer_cast<syrec::AssignStatement>(assignmentStmt);
                return std::make_shared<syrec::AssignStatement>(
                        assignmentCasted->lhs,
                        *syrec_operation::tryMapAssignmentOperationEnumToFlag(*syrec_operation::invert(*syrec_operation::tryMapAssignmentOperationFlagToEnum(assignmentCasted->op))),
                        assignmentCasted->rhs);
            });
    return invertedAssignments;
}

std::optional<unsigned> MainAdditionalLineForAssignmentSimplifier::tryEvaluateNumberAsConstant(const syrec::Number::ptr& number) {
    return number->isConstant() ? std::make_optional(number->evaluate({})) : std::nullopt;
}

bool MainAdditionalLineForAssignmentSimplifier::doBitRangesOverlap(const optimizations::BitRangeAccessRestriction::BitRangeAccess& thisBitRange, const optimizations::BitRangeAccessRestriction::BitRangeAccess& thatBitRange) {
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

bool MainAdditionalLineForAssignmentSimplifier::doDimensionAccessesOverlap(const TransformedDimensionAccess& thisDimensionAccess, const TransformedDimensionAccess& thatDimensionAccess) {
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

std::optional<syrec::expression::ptr> MainAdditionalLineForAssignmentSimplifier::tryConvertCompileTimeConstantExpressionToBinaryExpression(const syrec::Number::ptr& potentialCompileTimeExpression, unsigned int expectedBitWidth, const parser::SymbolTable::ptr& symbolTable) {
    if (!potentialCompileTimeExpression->isCompileTimeConstantExpression()) {
        return std::nullopt;
    }

    const auto& compileTimeConstantExpression = potentialCompileTimeExpression->getExpression();
    const auto& mappedToOperation             = tryMapCompileTimeConstantOperation(compileTimeConstantExpression);
    if (!mappedToOperation.has_value()) {
        return std::nullopt;
    }

    const auto lhsOperandConverted = tryMapNumberToExpression(compileTimeConstantExpression.lhsOperand, expectedBitWidth, symbolTable);
    const auto rhsOperandConverted = tryMapNumberToExpression(compileTimeConstantExpression.rhsOperand, expectedBitWidth, symbolTable);
    if (!lhsOperandConverted.has_value() || !rhsOperandConverted.has_value()) {
        return std::nullopt;
    }

    return std::make_optional(
        std::make_shared<syrec::BinaryExpression>(
            *lhsOperandConverted, *syrec_operation::tryMapBinaryOperationEnumToFlag(*mappedToOperation), *rhsOperandConverted
        )
    );
}

std::optional<syrec_operation::operation> MainAdditionalLineForAssignmentSimplifier::tryMapCompileTimeConstantOperation(const syrec::Number::CompileTimeConstantExpression& compileTimeConstantExpression) {
    switch (compileTimeConstantExpression.operation) {
        case syrec::Number::Operation::Addition:
            return std::make_optional(syrec_operation::operation::Addition);
        case syrec::Number::Operation::Subtraction:
            return std::make_optional(syrec_operation::operation::Subtraction);
        case syrec::Number::Operation::Multiplication:
            return std::make_optional(syrec_operation::operation::Multiplication);
        case syrec::Number::Operation::Division:
            return std::make_optional(syrec_operation::operation::Division);
        default:
            return std::nullopt;
    }
}

std::optional<syrec::expression::ptr> MainAdditionalLineForAssignmentSimplifier::tryMapNumberToExpression(const syrec::Number::ptr& number, unsigned int expectedBitWidth, const parser::SymbolTable::ptr& symbolTable) {
    if (number->isConstant()) {
        return std::make_shared<syrec::NumericExpression>(number, expectedBitWidth);
    }
    if (number->isLoopVariable()) {
        const auto& loopVariableIdent = number->variableName();
        if (const auto& symbolTableEntryForIdent = symbolTable->getVariable(loopVariableIdent); symbolTableEntryForIdent.has_value() && std::holds_alternative<syrec::Variable::ptr>(*symbolTableEntryForIdent)) {
            const auto& backingVariableForLoopVariable = std::get<syrec::Variable::ptr>(*symbolTableEntryForIdent);

            const auto& generatedSignalAccessForLoopVariable = std::make_shared<syrec::VariableAccess>();
            generatedSignalAccessForLoopVariable->var        = backingVariableForLoopVariable;
            return std::make_optional(std::make_shared<syrec::VariableExpression>(generatedSignalAccessForLoopVariable));    
        }
        return std::nullopt;
    }
    return tryConvertCompileTimeConstantExpressionToBinaryExpression(number, expectedBitWidth, symbolTable);
}

std::optional<syrec::expression::ptr> MainAdditionalLineForAssignmentSimplifier::tryPerformReorderingOfSubexpressions(const syrec::expression::ptr& expr) {
    const auto& exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr);
    const auto& exprAsShiftExpr = exprAsBinaryExpr == nullptr ? std::dynamic_pointer_cast<syrec::ShiftExpression>(expr) : nullptr;

    if (exprAsBinaryExpr == nullptr && exprAsShiftExpr == nullptr) {
        return std::make_optional(expr);
    }

    if (exprAsShiftExpr != nullptr) {
        if (const auto& reorderResultOfLhs = tryPerformReorderingOfSubexpressions(exprAsShiftExpr->lhs); reorderResultOfLhs.has_value()) {
            return std::make_optional(std::make_shared<syrec::ShiftExpression>(
                *reorderResultOfLhs,
                exprAsShiftExpr->op,
                exprAsShiftExpr->rhs
            ));
        }
        return std::make_optional(expr);
    }

    const auto& mappedToOperationFromFlag = syrec_operation::tryMapAssignmentOperationFlagToEnum(exprAsBinaryExpr->op);
    if (doesExpressionDefineSignalAccess(exprAsBinaryExpr->rhs) 
        && !doesExpressionDefineSignalAccess(exprAsBinaryExpr->lhs) 
        && mappedToOperationFromFlag.has_value()
        && syrec_operation::isCommutative(*mappedToOperationFromFlag)) {

        syrec::expression::ptr newExprLhs = exprAsBinaryExpr->rhs;
        syrec::expression::ptr newExprRhs = exprAsBinaryExpr->lhs;
        if (const auto& reorderResultOfRhs = tryPerformReorderingOfSubexpressions(exprAsBinaryExpr->rhs); reorderResultOfRhs.has_value()) {
            newExprRhs = *reorderResultOfRhs;
        }
        return std::make_optional(std::make_shared<syrec::BinaryExpression>(
                newExprLhs,
                exprAsBinaryExpr->op,
                newExprRhs));
    }
    return std::make_optional(expr);
}


std::shared_ptr<MainAdditionalLineForAssignmentSimplifier::VariableAccessCountLookup> MainAdditionalLineForAssignmentSimplifier::buildVariableAccessCountsForExpr(const syrec::expression::ptr& expr, const EstimatedSignalAccessSize& requiredSizeForSignalAccessToBeConsidered) const {
    auto buildLookup = std::make_shared<VariableAccessCountLookup>();
    buildVariableAccessCountsForExpr(expr, buildLookup, requiredSizeForSignalAccessToBeConsidered);
    return buildLookup;
}

void MainAdditionalLineForAssignmentSimplifier::buildVariableAccessCountsForExpr(const syrec::expression::ptr& expr, std::shared_ptr<VariableAccessCountLookup>& lookupToFill, const EstimatedSignalAccessSize& requiredSizeForSignalAccessToBeConsidered) const {
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

bool MainAdditionalLineForAssignmentSimplifier::doesSignalAccessMatchExpectedSize(const syrec::VariableAccess::ptr& signalAccessToCheck, const EstimatedSignalAccessSize& requiredSizeForSignalAccessToBeConsidered) const {
    return getSizeOfSignalAccess(signalAccessToCheck) == requiredSizeForSignalAccessToBeConsidered;
}

MainAdditionalLineForAssignmentSimplifier::EstimatedSignalAccessSize MainAdditionalLineForAssignmentSimplifier::getSizeOfSignalAccess(const syrec::VariableAccess::ptr& signalAccess) const {
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

bool MainAdditionalLineForAssignmentSimplifier::canAddAssignmentBeReplacedWithXorAssignment(const syrec::AssignStatement::ptr& assignmentStmt, bool isValueOfAssignedToSignalBlockedByDataFlowAnalysis) const {
    if (isValueOfAssignedToSignalBlockedByDataFlowAnalysis) {
        return false;
    }

    const auto assignmentStmtCasted = std::dynamic_pointer_cast<syrec::AssignStatement>(assignmentStmt);
    if (assignmentStmtCasted == nullptr) {
        return false;
    }

    const auto definedAssignmentOperation = syrec_operation::tryMapAssignmentOperationFlagToEnum(assignmentStmtCasted->op);
    if (!definedAssignmentOperation.has_value() || *definedAssignmentOperation != syrec_operation::operation::AddAssign) {
        return false;
    }

    const auto fetchedValueForAssignedToSignalPartsPriorToAssignment = symbolTable->tryFetchValueForLiteral(assignmentStmtCasted->lhs);
    return fetchedValueForAssignedToSignalPartsPriorToAssignment.has_value() && fetchedValueForAssignedToSignalPartsPriorToAssignment.value() == 0;
}

// TODO: Implement me
std::optional<syrec::expression::ptr> MainAdditionalLineForAssignmentSimplifier::findExpressionContainingSignalAccessDefinedOnlyOnceInAssignmentRhs(const syrec::BinaryExpression::ptr& assignmentStatement, bool& isLhsRelevantSignalAccess) {
    return std::nullopt;
}

MainAdditionalLineForAssignmentSimplifier::LookupOfExcludedSignalsForReplacement MainAdditionalLineForAssignmentSimplifier::createLookupForSignalsNotUsableAsReplacementsFor(const syrec::expression::ptr& expr) const {
    LookupOfExcludedSignalsForReplacement createdLookup = std::make_unique<std::map<std::string_view, std::vector<NotUsableAsReplacementSignalParts>>>();
    for (const auto& [signalIdent, existingAssignmentsForSignal] : activeAssignments) {
        createdLookup->insert(std::make_pair(signalIdent, existingAssignmentsForSignal));
    }
    createLookupForSignalsNotUsableAsReplacementsFor(expr, createdLookup);
    return createdLookup;
}

void MainAdditionalLineForAssignmentSimplifier::createLookupForSignalsNotUsableAsReplacementsFor(const syrec::expression::ptr& expr, LookupOfExcludedSignalsForReplacement& lookupToFill) const {
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
bool MainAdditionalLineForAssignmentSimplifier::doesAssignmentToAccessedSignalPartsAlreadyExists(const syrec::VariableAccess::ptr& accessedSignalParts) const {
    return false;
}

// TODO: Implement me
std::optional<syrec::VariableAccess::ptr> MainAdditionalLineForAssignmentSimplifier::tryCreateSubstituteForExpr(const syrec::expression::ptr& expr, const LookupOfExcludedSignalsForReplacement& signalPartsToExcludeAsPotentialReplacements) {
    return std::nullopt;
}