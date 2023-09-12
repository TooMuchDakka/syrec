#include "core/syrec/parser/optimizations/reassociate_expression.hpp"
#include "core/syrec/parser/utils/binary_expression_simplifier.hpp"
#include "core/syrec/parser/operation.hpp"
#include "core/syrec/parser/utils/signal_access_utils.hpp"

#include <tuple>

using namespace optimizations;

// TODO: Would the transformation (<subExpr_1> - (<subExpr_2> - <subExpr_3>)) to (<subExpr_1> + (<subExpr_3> - <subExpr_2>)) lead to better results ?
// TODO: If we introduce additional signal accesses, reference counts of newly added signal accesses need to be incremented
static void addOptimizedAwaySignalAccessToContainerIfNoExactMatchExists(const syrec::VariableAccess::ptr& optimizedAwaySignalAccess, std::vector<syrec::VariableAccess::ptr>* droppedSignalAccesses, const parser::SymbolTable& symbolTable) {
    if (!droppedSignalAccesses) {
        return;
    }

    const auto wasSignalAccessAlreadyDefined = std::any_of(
            droppedSignalAccesses->cbegin(),
            droppedSignalAccesses->cend(),
            [&optimizedAwaySignalAccess, &symbolTable](const syrec::VariableAccess::ptr& alreadyDefinedSignalAccess) {
                const auto& signalAccessEquivalenceResult = SignalAccessUtils::areSignalAccessesEqual(
                        *optimizedAwaySignalAccess,
                        *alreadyDefinedSignalAccess,
                        SignalAccessUtils::SignalAccessComponentEquivalenceCriteria::DimensionAccess::Equal,
                        SignalAccessUtils::SignalAccessComponentEquivalenceCriteria::BitRange::Equal,
                        symbolTable);
                return signalAccessEquivalenceResult.isResultCertain && signalAccessEquivalenceResult.equality == SignalAccessUtils::SignalAccessEquivalenceResult::Equal;
            });
    if (wasSignalAccessAlreadyDefined) {
        return;
    }
    droppedSignalAccesses->emplace_back(optimizedAwaySignalAccess);
}

static void addOptimizedAwaySignalAccessesToContainer(const syrec::expression::ptr& optimizedAwayExpr, std::vector<syrec::VariableAccess::ptr>* droppedSignalAccesses, const parser::SymbolTable& symbolTable) {
    if (const auto& exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(optimizedAwayExpr); exprAsBinaryExpr != nullptr) {
        addOptimizedAwaySignalAccessesToContainer(exprAsBinaryExpr->lhs, droppedSignalAccesses, symbolTable);
        addOptimizedAwaySignalAccessesToContainer(exprAsBinaryExpr->rhs, droppedSignalAccesses, symbolTable);
    } else if (const auto& exprAsShiftExpr = std::dynamic_pointer_cast<syrec::ShiftExpression>(optimizedAwayExpr); exprAsShiftExpr != nullptr) {
        addOptimizedAwaySignalAccessesToContainer(exprAsShiftExpr->lhs, droppedSignalAccesses, symbolTable);
    } else if (const auto& exprAsVariableExpr = std::dynamic_pointer_cast<syrec::VariableExpression>(optimizedAwayExpr); exprAsVariableExpr != nullptr) {
        addOptimizedAwaySignalAccessToContainerIfNoExactMatchExists(exprAsVariableExpr->var, droppedSignalAccesses, symbolTable);
    }
}

inline std::optional<std::shared_ptr<syrec::BinaryExpression>> tryConvertExpressionToBinaryOne(const syrec::expression::ptr& expr) {
    if (const auto& exprAsBinaryExpression = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr); exprAsBinaryExpression != nullptr) {
        return std::make_optional(exprAsBinaryExpression);
    }
    return std::nullopt;
}

inline syrec::expression::ptr createBinaryExpression(const syrec::expression::ptr& lhsOperand, const unsigned int binaryOperation, const syrec::expression::ptr& rhsOperand) {
    return std::make_shared<syrec::BinaryExpression>(lhsOperand, binaryOperation, rhsOperand);
}

inline syrec::expression::ptr simplifyBinaryExpressionIfSimplificationOfOperandsResultsInNewOperands(const std::shared_ptr<syrec::BinaryExpression>& binaryExpr, bool operationStrengthReductionEnabled, const std::optional<std::unique_ptr<optimizations::BaseMultiplicationSimplifier>>& optionalMultiplicationSimplifier, const parser::SymbolTable& symbolTable, std::vector<syrec::VariableAccess::ptr>* droppedSignalAccesses) {
    const auto simplificationResultOfLhsOperand = simplifyBinaryExpression(binaryExpr->lhs, operationStrengthReductionEnabled, optionalMultiplicationSimplifier, symbolTable, droppedSignalAccesses);
    const auto simplificationResultOfRhsOperand = simplifyBinaryExpression(binaryExpr->rhs, operationStrengthReductionEnabled, optionalMultiplicationSimplifier, symbolTable, droppedSignalAccesses);
    if (simplificationResultOfLhsOperand == binaryExpr->lhs && simplificationResultOfRhsOperand == binaryExpr->rhs) {
        return binaryExpr;
    }
    return simplifyBinaryExpression(createBinaryExpression(simplificationResultOfLhsOperand, binaryExpr->op, simplificationResultOfRhsOperand), operationStrengthReductionEnabled, optionalMultiplicationSimplifier, symbolTable, droppedSignalAccesses);
}

inline bool isArithmeticOperation(syrec_operation::operation operation) {
    switch (operation) {
        case syrec_operation::operation::Addition:
        case syrec_operation::operation::Subtraction:
        case syrec_operation::operation::Division:
        case syrec_operation::operation::Multiplication:
            return true;
        default:
            return false;
    }
}

bool isOperationCombinedWithMultiplicationDistributive(syrec_operation::operation operation) {
    switch (operation) {
        case syrec_operation::operation::Addition:
        case syrec_operation::operation::Subtraction:
        case syrec_operation::operation::Multiplication:
            return true;
        default:
            return false;
    }
}

std::optional<syrec::expression::ptr> trySimplifyExprIfOneOperandIsIdentityElement(const syrec::expression::ptr& referenceExpr, const std::optional<unsigned int>& constOperandValue, bool isLhsOperandConst, bool isReferenceExprBinaryOne, syrec_operation::operation definedOperationForExpr, const parser::SymbolTable& symbolTable, std::vector<syrec::VariableAccess::ptr>* droppedSignalAccesses) {
    if (referenceExpr == nullptr || !constOperandValue.has_value()) {
        return std::nullopt;
    }

    if (isLhsOperandConst && syrec_operation::isOperandUsedAsLhsInOperationIdentityElement(definedOperationForExpr, *constOperandValue)) {
        if (isReferenceExprBinaryOne) {
            addOptimizedAwaySignalAccessesToContainer(std::static_pointer_cast<syrec::BinaryExpression>(referenceExpr)->lhs, droppedSignalAccesses, symbolTable);
            return std::make_optional(std::static_pointer_cast<syrec::BinaryExpression>(referenceExpr)->rhs);
        }
        addOptimizedAwaySignalAccessesToContainer(std::static_pointer_cast<syrec::ShiftExpression>(referenceExpr)->lhs, droppedSignalAccesses, symbolTable);
        return std::make_optional(std::make_shared<syrec::NumericExpression>(std::static_pointer_cast<syrec::ShiftExpression>(referenceExpr)->rhs, referenceExpr->bitwidth()));
    }
    if (!isLhsOperandConst && syrec_operation::isOperandUseAsRhsInOperationIdentityElement(definedOperationForExpr, *constOperandValue)) {
        if (isReferenceExprBinaryOne) {
            addOptimizedAwaySignalAccessesToContainer(std::static_pointer_cast<syrec::BinaryExpression>(referenceExpr)->rhs, droppedSignalAccesses, symbolTable);
            return std::make_optional(std::static_pointer_cast<syrec::BinaryExpression>(referenceExpr)->lhs);
        }
        // TODO: Handling of optimized away syrec::Number containers
        return std::make_optional(std::static_pointer_cast<syrec::ShiftExpression>(referenceExpr)->lhs);
    }
    return std::nullopt;
}

syrec::expression::ptr createExpressionForNumber(const unsigned int number, unsigned int expectedBitWidthOfCreatedExpression) {
    return std::make_shared<syrec::NumericExpression>(std::make_shared<syrec::Number>(number), expectedBitWidthOfCreatedExpression);
}

std::optional<unsigned int> tryGetConstantValueOfExpression(const syrec::expression::ptr& expr) {
    std::optional<unsigned int> valueOfExpression;
    if (const auto exprAsNumericExpression = std::dynamic_pointer_cast<syrec::NumericExpression>(expr); exprAsNumericExpression != nullptr) {
        if (exprAsNumericExpression->value->isConstant()) {
            valueOfExpression.emplace(exprAsNumericExpression->value->evaluate({}));
        }
    }
    return valueOfExpression;
}

inline bool isExpressionConstantOrSignalAccess(const syrec::expression::ptr& expression) {
    return std::dynamic_pointer_cast<syrec::NumericExpression>(expression) != nullptr || std::dynamic_pointer_cast<syrec::VariableExpression>(expression) != nullptr;
}

std::optional<std::tuple<unsigned int, syrec::expression::ptr>> trySplitExpressionIntoConstantAndNonExprOperand(const std::shared_ptr<syrec::BinaryExpression>& binaryExpr) {
    if (!isExpressionConstantOrSignalAccess(binaryExpr->lhs) && !isExpressionConstantOrSignalAccess(binaryExpr->rhs)) {
        return std::nullopt;
    }

    std::optional<unsigned int> constOperandOfLhsExpr    = tryGetConstantValueOfExpression(binaryExpr->lhs);
    syrec::expression::ptr      nonConstOperandOfLhsExpr = binaryExpr->rhs;
    if (!constOperandOfLhsExpr.has_value()) {
        constOperandOfLhsExpr    = tryGetConstantValueOfExpression(binaryExpr->rhs);
        nonConstOperandOfLhsExpr = binaryExpr->lhs;
    }

    if (constOperandOfLhsExpr.has_value()) {
        return std::make_optional(std::make_tuple(*constOperandOfLhsExpr, nonConstOperandOfLhsExpr));   
    }
    return std::nullopt;
}

std::optional<syrec::expression::ptr> trySimplifyMultiplicationOfBinaryExpression(const std::shared_ptr<syrec::BinaryExpression>& binaryExprWithMultiplicationOperation, const std::optional<std::unique_ptr<optimizations::BaseMultiplicationSimplifier>>& optionalMultiplicationSimplifier) {
    if (optionalMultiplicationSimplifier.has_value()) {
        return optionalMultiplicationSimplifier.value()->trySimplify(binaryExprWithMultiplicationOperation);
    }
    return std::nullopt;
}

/*
 * TODO: More tests with subexpressions being not optimizable or simply testing the other reordering rules currently not covered
 * TODO: Handling of cases (a + b) * c would be expanded to (a * c) + (b * c) which could introduce additional lines into our circuit,
 *          which in turn is a contradiction (for reversivel circuits)
 * TODO: There is room for further optimizations where one would 'accumulate' signals (i.e. a + (2 + a) => (2 * a) + 2)
 */
syrec::expression::ptr optimizations::simplifyBinaryExpression(const syrec::expression::ptr& expr, bool operationStrengthReductionEnabled, const std::optional<std::unique_ptr<optimizations::BaseMultiplicationSimplifier>>& optionalMultiplicationSimplifier, const parser::SymbolTable& symbolTable, std::vector<syrec::VariableAccess::ptr>* droppedSignalAccesses) {
    const auto exprAsShiftExpr = std::dynamic_pointer_cast<syrec::ShiftExpression>(expr);
    const auto binaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr);
    if (binaryExpr == nullptr && exprAsShiftExpr == nullptr) {
        return expr;
    }

    std::optional<unsigned int> lOperandConstValue;
    std::optional<unsigned int> rOperandConstValue;
    std::optional<syrec_operation::operation> mappedFlagToEnum;

    if (binaryExpr != nullptr) {
        lOperandConstValue = tryGetConstantValueOfExpression(binaryExpr->lhs);
        rOperandConstValue = tryGetConstantValueOfExpression(binaryExpr->rhs);
        mappedFlagToEnum   = syrec_operation::tryMapBinaryOperationFlagToEnum(binaryExpr->op);
        if (!mappedFlagToEnum.has_value()) {
            return expr;
        }
    }
    else {
        lOperandConstValue = tryGetConstantValueOfExpression(exprAsShiftExpr->lhs);
        const auto& rhsOperandOfShiftExpr = std::make_shared<syrec::NumericExpression>(exprAsShiftExpr->rhs, exprAsShiftExpr->bitwidth());
        rOperandConstValue = tryGetConstantValueOfExpression(rhsOperandOfShiftExpr);
        mappedFlagToEnum   = syrec_operation::tryMapShiftOperationFlagToEnum(exprAsShiftExpr->op);
        if (!mappedFlagToEnum.has_value()) {
            return expr;
        }
    }
    
    std::optional<unsigned int> constOperandOfParentExpr;
    if (lOperandConstValue.has_value()) {
        constOperandOfParentExpr = lOperandConstValue;
    } else if (rOperandConstValue.has_value()) {
        constOperandOfParentExpr = rOperandConstValue;
    }

    const auto binaryOperationOfExpr = mappedFlagToEnum.value();
    if (lOperandConstValue.has_value() || rOperandConstValue.has_value()) {
        if (const auto& simplifiedExprIfConstOperandIsIdentityElement = trySimplifyExprIfOneOperandIsIdentityElement(expr, lOperandConstValue.has_value() ? lOperandConstValue : rOperandConstValue, lOperandConstValue.has_value(), binaryExpr != nullptr, *mappedFlagToEnum, symbolTable, droppedSignalAccesses); simplifiedExprIfConstOperandIsIdentityElement.has_value()) {
            return simplifyBinaryExpression(*simplifiedExprIfConstOperandIsIdentityElement, operationStrengthReductionEnabled, optionalMultiplicationSimplifier, symbolTable, droppedSignalAccesses);
        }

        /*
         * The following preconditions must hold before we attempt to simplify a Multiplication operation of a binary expression
         * I. We need to be able to determine if the defined operation of the expression was the 'Multiplication' operation
         * II. One of the operands is a constant while the other must not be a constant
         *
         * After our simplification of the Multiplication operation, we try to again simplify the generated binary expression
         */
        if (binaryExpr != nullptr 
            && mappedFlagToEnum.has_value() 
            && mappedFlagToEnum == syrec_operation::operation::Multiplication 
            && (!lOperandConstValue || !rOperandConstValue)) {
            if (const auto& exprWithSimplifiedBinaryExpr = trySimplifyMultiplicationOfBinaryExpression(binaryExpr, optionalMultiplicationSimplifier); exprWithSimplifiedBinaryExpr.has_value()) {
                return simplifyBinaryExpression(*exprWithSimplifiedBinaryExpr, operationStrengthReductionEnabled, optionalMultiplicationSimplifier, symbolTable, droppedSignalAccesses);
            }
        }

        const auto& simplificationResult = trySimplify(expr, operationStrengthReductionEnabled, symbolTable, droppedSignalAccesses);
        if (simplificationResult.couldSimplify) {
            return simplifyBinaryExpression(simplificationResult.simplifiedExpression, operationStrengthReductionEnabled, optionalMultiplicationSimplifier, symbolTable, droppedSignalAccesses);
        }
        /*
         * If we could not simplify the given shift expression we can only try and simplify the lhs operand of the shift operations
         * because the rhs can only be a number (with the curretn version of the specification [12.02.2023])
         */
        if (exprAsShiftExpr != nullptr) {
            const auto& lhsOperandSimplified = simplifyBinaryExpression(exprAsShiftExpr->lhs, operationStrengthReductionEnabled, optionalMultiplicationSimplifier, symbolTable, droppedSignalAccesses);
            if (lhsOperandSimplified != exprAsShiftExpr->lhs) {
                return std::make_shared<syrec::ShiftExpression>(
                        lhsOperandSimplified,
                        exprAsShiftExpr->op,
                        exprAsShiftExpr->rhs);
            }
            return exprAsShiftExpr;
        }
    }

    if ((binaryOperationOfExpr != syrec_operation::operation::Multiplication && binaryOperationOfExpr != syrec_operation::operation::Addition) 
        || (!isExpressionConstantOrSignalAccess(binaryExpr->lhs) && !isExpressionConstantOrSignalAccess(binaryExpr->rhs))) {
        if (binaryOperationOfExpr == syrec_operation::operation::Addition) {
            const auto lhsExprAsBinaryExpr = tryConvertExpressionToBinaryOne(binaryExpr->lhs);
            const auto rhsExprAsBinaryExpr = tryConvertExpressionToBinaryOne(binaryExpr->rhs);

            if (lhsExprAsBinaryExpr.has_value() && rhsExprAsBinaryExpr.has_value()) {
                if ((*lhsExprAsBinaryExpr)->op == syrec::BinaryExpression::Add && (*rhsExprAsBinaryExpr)->op == syrec::BinaryExpression::Add) {
                    const auto lhsExprDividedIntoConstantAndNonExprOperand = trySplitExpressionIntoConstantAndNonExprOperand(*lhsExprAsBinaryExpr);
                    const auto rhsExprDividedIntoConstantAndNonExprOperand = trySplitExpressionIntoConstantAndNonExprOperand(*rhsExprAsBinaryExpr);

                    /*
                     * We can simplify the expression (c1 + x) + (c2 + y) into (c1 + c2) + (x + y)
                     */
                    if (lhsExprDividedIntoConstantAndNonExprOperand.has_value() && rhsExprDividedIntoConstantAndNonExprOperand.has_value()) {
                        return createBinaryExpression(
                                createExpressionForNumber(
                                        *syrec_operation::apply(
                                                syrec_operation::operation::Addition,
                                                std::get<unsigned int>(*lhsExprDividedIntoConstantAndNonExprOperand),
                                                std::get<unsigned int>(*rhsExprDividedIntoConstantAndNonExprOperand)),
                                        std::get<syrec::expression::ptr>(*lhsExprDividedIntoConstantAndNonExprOperand)->bitwidth()),
                                syrec::BinaryExpression::Add,
                                createBinaryExpression(
                                        std::get<syrec::expression::ptr>(*lhsExprDividedIntoConstantAndNonExprOperand),
                                        syrec::BinaryExpression::Add,
                                        std::get<syrec::expression::ptr>(*rhsExprDividedIntoConstantAndNonExprOperand)));
                    }
                }
            }
        }
        return simplifyBinaryExpressionIfSimplificationOfOperandsResultsInNewOperands(binaryExpr, operationStrengthReductionEnabled, optionalMultiplicationSimplifier, symbolTable, droppedSignalAccesses);
    }

    std::optional<syrec::expression::ptr> nonNestedExpressionOperandOfParentExpr;
    syrec::expression::ptr                exprToCheckForDistributivity;
    if (isExpressionConstantOrSignalAccess(binaryExpr->lhs)) {
        nonNestedExpressionOperandOfParentExpr = binaryExpr->lhs;
        exprToCheckForDistributivity           = binaryExpr->rhs;
    } else {
        nonNestedExpressionOperandOfParentExpr = binaryExpr->rhs;
        exprToCheckForDistributivity           = binaryExpr->lhs;
    }

    // Both operands of expression are expressions themselves so we recursively apply the simplification operation on both operands
    if (!nonNestedExpressionOperandOfParentExpr.has_value()) {
        return createBinaryExpression(simplifyBinaryExpression(binaryExpr->lhs, operationStrengthReductionEnabled, optionalMultiplicationSimplifier, symbolTable, droppedSignalAccesses), binaryExpr->op, simplifyBinaryExpression(binaryExpr->rhs, operationStrengthReductionEnabled, optionalMultiplicationSimplifier, symbolTable, droppedSignalAccesses));
    }

    std::optional<std::shared_ptr<syrec::BinaryExpression>> childBinaryExpression;
    if (const auto childOperandAsBinaryExpression = std::dynamic_pointer_cast<syrec::BinaryExpression>(exprToCheckForDistributivity); childOperandAsBinaryExpression != nullptr) {
        childBinaryExpression.emplace(childOperandAsBinaryExpression);
    }

    const std::optional<syrec_operation::operation> childBinaryExprOperation = childBinaryExpression.has_value()
        ? syrec_operation::tryMapBinaryOperationFlagToEnum((*childBinaryExpression)->op)
        : std::nullopt;

    if (!childBinaryExpression.has_value() || !childBinaryExprOperation.has_value() || !isArithmeticOperation(*childBinaryExprOperation)) {
        /*
         * In case our expression is of the form t op c where op is a commutative operation we rewrite our expression as
         * c op t and retry our simplification
         */ 
        if (rOperandConstValue.has_value()) {
            return simplifyBinaryExpression(createBinaryExpression(binaryExpr->rhs, binaryExpr->op, binaryExpr->lhs), operationStrengthReductionEnabled, optionalMultiplicationSimplifier, symbolTable, droppedSignalAccesses);
        }
        
        return simplifyBinaryExpressionIfSimplificationOfOperandsResultsInNewOperands(binaryExpr, operationStrengthReductionEnabled, optionalMultiplicationSimplifier, symbolTable, droppedSignalAccesses);
    }

    const auto splitOfChildBinaryExpr        = trySplitExpressionIntoConstantAndNonExprOperand(*childBinaryExpression);
    const auto constOperandOfChildBinaryExpr = splitOfChildBinaryExpr.has_value() ? std::make_optional(std::get<unsigned int>(*splitOfChildBinaryExpr)) : std::nullopt;
    
    if (binaryOperationOfExpr == syrec_operation::operation::Multiplication && isOperationCombinedWithMultiplicationDistributive(*childBinaryExprOperation)) {
        if (constOperandOfParentExpr.has_value() && constOperandOfChildBinaryExpr.has_value()) {
            /*
            * Cases:
            * I.   (c1 * (c1 * y))
            * II.  (c1 * (x * c2))
            * III. ((c2 * y) * c1)
            * IV.  ((x * c2) * c1)
            * all simplify to: ((c1 * c2) * x)
            *
            */
            const auto productOfParentAndChildConstOperand = syrec_operation::apply(
                    syrec_operation::operation::Multiplication,
                    *constOperandOfParentExpr,
                    *constOperandOfChildBinaryExpr);

            if (*childBinaryExprOperation == syrec_operation::operation::Multiplication) {
                return createBinaryExpression(
                        createExpressionForNumber(*productOfParentAndChildConstOperand, std::get<syrec::expression::ptr>(*splitOfChildBinaryExpr)->bitwidth()),
                        (*childBinaryExpression)->op,
                        std::get<syrec::expression::ptr>(*splitOfChildBinaryExpr));
            }

            /*
            * Cases:
            * I.   (c1 * (c1 op y))
            * II.  (c1 * (x op c2))
            * III. ((c2 op y) * c1)
            * IV.  ((x op c2) * c1)
            * all simplify to: ((c1 op c2) * (c1 op x))
            *
            */
            return createBinaryExpression(
                    createExpressionForNumber(*productOfParentAndChildConstOperand, std::get<syrec::expression::ptr>(*splitOfChildBinaryExpr)->bitwidth()),
                    (*childBinaryExpression)->op,
                    simplifyBinaryExpression(
                            createBinaryExpression(
                                    *nonNestedExpressionOperandOfParentExpr,
                                    syrec::BinaryExpression::Multiply,
                                    std::get<syrec::expression::ptr>(*splitOfChildBinaryExpr)), 
                        operationStrengthReductionEnabled, optionalMultiplicationSimplifier, symbolTable, droppedSignalAccesses));
        }

        if (*childBinaryExprOperation == syrec_operation::operation::Multiplication) {
            /*
            * If no other simplification rule can be applied we rewrite our expression (t * (t1 * t2)) as ((t * t1) * t2)
            */
            if (*childBinaryExpression == binaryExpr->rhs) {
                return createBinaryExpression(
                        simplifyBinaryExpression(
                                createBinaryExpression(
                                        binaryExpr->lhs,
                                        syrec::BinaryExpression::Multiply,
                                        (*childBinaryExpression)->lhs),
                                operationStrengthReductionEnabled, optionalMultiplicationSimplifier, symbolTable, droppedSignalAccesses),
                        syrec::BinaryExpression::Multiply,
                        (*childBinaryExpression)->rhs);
            }
        }

        /*
        * Otherwise apply distributivity rule:
        * I.  (x op y) * a
        * II. a * (x op y)
        * all 'simplify' to: ((x * a) op (y * a))
        */
        if (constOperandOfParentExpr.has_value()) {
            return createBinaryExpression(
                    simplifyBinaryExpression(
                            createBinaryExpression(
                                    *nonNestedExpressionOperandOfParentExpr,
                                    syrec::BinaryExpression::Multiply,
                                    (*childBinaryExpression)->lhs),
                            operationStrengthReductionEnabled, optionalMultiplicationSimplifier, symbolTable, droppedSignalAccesses),
                    (*childBinaryExpression)->op,
                    simplifyBinaryExpression(
                            createBinaryExpression(
                                    *nonNestedExpressionOperandOfParentExpr,
                                    syrec::BinaryExpression::Multiply,
                                    (*childBinaryExpression)->rhs), 
                        operationStrengthReductionEnabled, optionalMultiplicationSimplifier, symbolTable, droppedSignalAccesses));
        }
    }

    if (binaryOperationOfExpr == syrec_operation::operation::Addition && childBinaryExprOperation == syrec_operation::operation::Addition) {
        if (constOperandOfChildBinaryExpr.has_value() && constOperandOfParentExpr.has_value()) {
            /*
            * The cases:
            * I.   c1 + (c2 + y)
            * II.  c1 + (x + c2)
            * III. (x + c2) + c1
            * IV.  (c2 + x) + c1
            * all simplify to:
            * ((c1 + c2) + x)
            */
            const auto sumOfParentAndChildConstOperand = syrec_operation::apply(
                    syrec_operation::operation::Addition,
                    *constOperandOfParentExpr,
                    *constOperandOfChildBinaryExpr);

            return simplifyBinaryExpression(
                    createBinaryExpression(
                            createExpressionForNumber(*sumOfParentAndChildConstOperand, std::get<syrec::expression::ptr>(*splitOfChildBinaryExpr)->bitwidth()),
                            syrec::BinaryExpression::Add,
                            std::get<syrec::expression::ptr>(*splitOfChildBinaryExpr)), 
                operationStrengthReductionEnabled, optionalMultiplicationSimplifier, symbolTable, droppedSignalAccesses);
        }

        /*
         * If no other simplification rule can be applied we rewrite our expression (t + (t1 + t2)) as ((t + t1) + t2)
         */
        if (*childBinaryExpression == binaryExpr->rhs) {
            return createBinaryExpression(
                    simplifyBinaryExpression(createBinaryExpression(binaryExpr->lhs, syrec::BinaryExpression::Add, (*childBinaryExpression)->lhs), operationStrengthReductionEnabled, optionalMultiplicationSimplifier, symbolTable, droppedSignalAccesses),
                    syrec::BinaryExpression::Add,
                    (*childBinaryExpression)->rhs);    
        }
    }

    /*
     * In case our expression is of the form t op c where op is a commutative operation we rewrite our expression as
     * c op t and retry our simplification
     */
    if (rOperandConstValue.has_value()) {
        return simplifyBinaryExpression(createBinaryExpression(binaryExpr->rhs, binaryExpr->op, binaryExpr->lhs), operationStrengthReductionEnabled, optionalMultiplicationSimplifier, symbolTable, droppedSignalAccesses);
    }

    return binaryExpr;
}