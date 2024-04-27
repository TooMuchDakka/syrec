#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/expression_traversal_helper.hpp"

#include <algorithm>

using namespace noAdditionalLineSynthesis;

bool ExpressionTraversalHelper::OperandNode::isLeafNode() const {
    return !operationNodeId.has_value();
}


std::optional<std::size_t> ExpressionTraversalHelper::OperationNode::getLeafNodeOperandId() const {
    if (!hasAnyLeafOperandNodes()) {
        return std::nullopt;
    }
    return lhsOperand.isLeafNode() ? lhsOperand.id : rhsOperand.id;
}

std::optional<std::pair<std::size_t, std::size_t>> ExpressionTraversalHelper::OperationNode::getLeafNodeOperandIds() const {
    if (areBothOperandsLeafNodes()) {
        return std::make_pair(lhsOperand.id, rhsOperand.id);
    }
    return std::nullopt;
}

bool ExpressionTraversalHelper::OperationNode::hasAnyLeafOperandNodes() const {
    return lhsOperand.isLeafNode() || rhsOperand.isLeafNode();
}

bool ExpressionTraversalHelper::OperationNode::areBothOperandsLeafNodes() const {
    return lhsOperand.isLeafNode() && rhsOperand.isLeafNode();
}

std::optional<syrec_operation::operation> ExpressionTraversalHelper::OperationNode::getOperationConsideringWhetherMarkedAsInverted() const {
    return isOneTimeOperationInversionFlagEnabled ? syrec_operation::invert(operation) : operation;
}

void ExpressionTraversalHelper::OperationNode::enabledIsOneTimeOperationInversionFlag() {
    isOneTimeOperationInversionFlagEnabled = true;
}

void ExpressionTraversalHelper::OperationNode::disableIsOneTimeOperationInversionFlag() {
    isOneTimeOperationInversionFlagEnabled = false;
}


std::optional<syrec::VariableExpression::ptr> ExpressionTraversalHelper::getOperandAsVariableExpr(std::size_t operandNodeId) const {
    if (std::optional<syrec::Expression::ptr> operandData = getDataOfOperand(operandNodeId); operandData.has_value() && std::dynamic_pointer_cast<syrec::VariableExpression>(*operandData)) {
        return operandData;
    }
    return std::nullopt;
}

std::optional<syrec::Expression::ptr> ExpressionTraversalHelper::getOperandAsExpr(std::size_t operandNodeId) const {
    if (std::optional<syrec::Expression::ptr> operandData = getDataOfOperand(operandNodeId); operandData.has_value() && !std::dynamic_pointer_cast<syrec::VariableExpression>(*operandData)) {
        return operandData;
    }
    return std::nullopt;
}


std::optional<ExpressionTraversalHelper::OperationNodeReference> ExpressionTraversalHelper::getNextOperationNode() {
    if (operationNodeTraversalQueueIdx >= operationNodeTraversalQueue.size()) {
        return std::nullopt;
    }

    const auto idOfNextOperationNodeInQueue = operationNodeTraversalQueue.at(operationNodeTraversalQueueIdx++);
    return getOperationNodeById(idOfNextOperationNodeInQueue);
}

std::optional<ExpressionTraversalHelper::OperationNodeReference> ExpressionTraversalHelper::peekNextOperationNode() const {
    if (operationNodeTraversalQueueIdx >= operationNodeTraversalQueue.size()) {
        return std::nullopt;
    }
    const auto idOfNextOperationNodeInQueue = operationNodeTraversalQueue.at(operationNodeTraversalQueueIdx);
    return getOperationNodeById(idOfNextOperationNodeInQueue);
}

std::optional<syrec_operation::operation> ExpressionTraversalHelper::getOperationOfOperationNode(std::size_t operationNodeId) const {
    if (!operationNodeDataLookup.count(operationNodeId)) {
        return std::nullopt;   
    }
    return operationNodeDataLookup.at(operationNodeId)->operation;
}

std::optional<std::size_t> ExpressionTraversalHelper::getOperandNodeIdOfNestedOperation(std::size_t parentOperationNodeId, std::size_t nestedOperationNodeId) const {
    if (const std::optional<OperationNodeReference> parentOperationNodeReference = getOperationNodeById(parentOperationNodeId); parentOperationNodeReference) {
        const OperandNode& lhsOperand = parentOperationNodeReference->get()->lhsOperand;
        if (lhsOperand.operationNodeId.has_value() && *lhsOperand.operationNodeId == nestedOperationNodeId) {
            return *lhsOperand.operationNodeId;
        }

        const OperandNode& rhsOperand = parentOperationNodeReference->get()->rhsOperand;
        if (rhsOperand.operationNodeId.has_value() && *rhsOperand.operationNodeId == nestedOperationNodeId) {
            return *rhsOperand.operationNodeId;
        }
    }
    return std::nullopt;
}


bool ExpressionTraversalHelper::canSignalBeUsedOnAssignmentLhs(const std::string& signalIdent) const {
    return identsOfAssignableSignals.count(signalIdent);
}


void ExpressionTraversalHelper::markOperationNodeAsPotentialBacktrackOption(std::size_t operationNodeId) {
    if (const auto& fetchedOperationNodeForId = getOperationNodeById(operationNodeId); fetchedOperationNodeForId.has_value()) {
        if (!std::any_of(backtrackOperationNodeIds.crbegin(), backtrackOperationNodeIds.crend(), [&operationNodeId](const std::size_t existingCheckpointOperationNodeId) { return existingCheckpointOperationNodeId == operationNodeId; })) {
            backtrackOperationNodeIds.emplace_back(operationNodeId);
        }
    }
}

std::optional<ExpressionTraversalHelper::OperationNodeReference> ExpressionTraversalHelper::getOperationNodeById(std::size_t operationNodeId) const {
    if (!operationNodeDataLookup.count(operationNodeId)) {
        return std::nullopt;
    }
    return operationNodeDataLookup.at(operationNodeId);
}

bool ExpressionTraversalHelper::updateOperandData(std::size_t operationNodeId, bool updateLhsOperandData, const syrec::Expression::ptr& newOperandData) {
    const std::optional<OperationNodeReference>      referencedOperationNode      = getOperationNodeById(operationNodeId);
    const std::shared_ptr<syrec::VariableExpression> newOperandDataAsVariableExpr = std::dynamic_pointer_cast<syrec::VariableExpression>(newOperandData);
    if (!newOperandData || !newOperandDataAsVariableExpr || !referencedOperationNode.has_value() 
        || (updateLhsOperandData && !referencedOperationNode->get()->lhsOperand.isLeafNode()) || (!updateLhsOperandData && !referencedOperationNode->get()->rhsOperand.isLeafNode())) {
        return false;
    }

    const std::size_t idOfOperandToUpdate = updateLhsOperandData ? referencedOperationNode->get()->lhsOperand.id : referencedOperationNode->get()->rhsOperand.id;
    if (!operandNodeDataLookup.count(idOfOperandToUpdate)) {
        return false;
    }
    operandNodeDataLookup.at(idOfOperandToUpdate) = newOperandData;
    // Since this class offers a function to determine whether a signal can be used at the left-hand side of an assignment, we also need to
    // update the lookup used by this function to also include the generated replacement signal.
    markSignalAsAssignable(newOperandDataAsVariableExpr->var->var->name);
    return true;
}

bool ExpressionTraversalHelper::updateOperation(std::size_t operationNodeId, syrec_operation::operation newOperation) const {
    const std::optional<OperationNodeReference> referencedOperationNode = getOperationNodeById(operationNodeId);
    if (!referencedOperationNode.has_value()) {
        return false;
    }
    referencedOperationNode->get()->operation = newOperation;
    return true;
}


void ExpressionTraversalHelper::markSignalAsAssignable(const std::string& assignableSignalIdent) {
    identsOfAssignableSignals.emplace(assignableSignalIdent);
}

std::optional<std::size_t> ExpressionTraversalHelper::getOperationNodeIdOfRightOperand(std::size_t operationNodeId) const {
    const std::optional<OperationNodeReference> referencedOperationNode = getOperationNodeById(operationNodeId);
    if (!referencedOperationNode.has_value()) {
        return std::nullopt;
    }
    return referencedOperationNode->get()->rhsOperand.operationNodeId;
}

std::optional<std::size_t> ExpressionTraversalHelper::getOperationNodeIdOfFirstEntryInQueue() const {
    if (operationNodeTraversalQueue.empty()) {
        return std::nullopt;
    }
    return operationNodeTraversalQueue.front();
}


void ExpressionTraversalHelper::removeOperationNodeAsPotentialBacktrackOperation(std::size_t operationNodeId) {
    if (const auto& fetchedOperationNodeForId = getOperationNodeById(operationNodeId); fetchedOperationNodeForId.has_value()) {
        const auto& foundMatchForOperationNode = std::find_if(
            backtrackOperationNodeIds.crbegin(), 
            backtrackOperationNodeIds.crend(), 
            [&operationNodeId](const std::size_t existingCheckpointOperationNodeId) {
                return existingCheckpointOperationNodeId == operationNodeId;
            });
        if (foundMatchForOperationNode != backtrackOperationNodeIds.crend()) {
            backtrackOperationNodeIds.erase(std::next(foundMatchForOperationNode).base());
        }
    }
}

void ExpressionTraversalHelper::backtrackToNode(std::size_t operationNodeId) {
    backtrackToNode(operationNodeId, false);
}

void ExpressionTraversalHelper::backtrackOnePastNode(std::size_t operationNodeId) {
    backtrackToNode(operationNodeId, true);
}

void ExpressionTraversalHelper::buildTraversalQueue(const syrec::Expression::ptr& expr, const parser::SymbolTable& symbolTableReference) {
    resetInternals();
    const auto& generatedOperationNode = createOperationNode(expr, std::nullopt);
    operationNodeDataLookup.insert(std::make_pair(generatedOperationNode->id, generatedOperationNode));
    handleOperationNodeDuringTraversalQueueInit(generatedOperationNode);
    determineIdentsOfSignalsUsableInAssignmentLhs(symbolTableReference);
}

void ExpressionTraversalHelper::resetInternals() {
    operandNodeDataLookup.clear();
    operationNodeDataLookup.clear();
    operationNodeTraversalQueue.clear();
    backtrackOperationNodeIds.clear();
    operationNodeTraversalQueueIdx = 0;
    operationNodeIdGenerator.reset();
    operandNodeIdGenerator.reset();
    identsOfAssignableSignals.clear();
}

// START OF NON-PUBLIC FUNCTION IMPLEMENTATIONS
std::size_t ExpressionTraversalHelper::IdGenerator::generateNextId() {
    return lastGeneratedId++;
}

void ExpressionTraversalHelper::IdGenerator::reset() {
    lastGeneratedId = 0;
}

void ExpressionTraversalHelper::handleOperationNodeDuringTraversalQueueInit(const OperationNodeReference& operationNode) {
    // POST-ORDER TRAVERSAL
   /* if (!operationNode->lhsOperand.isLeafNode()) {
        handleOperationNodeDuringTraversalQueueInit(*getOperationNodeById(operationNode->lhsOperand.id));
    }
    if (!operationNode->rhsOperand.isLeafNode()) {
        handleOperationNodeDuringTraversalQueueInit(*getOperationNodeById(operationNode->rhsOperand.id));
    }
    operationNodeTraversalQueue.emplace_back(operationNode->id);*/

    // PRE-ORDER TRAVERSAL
    operationNodeTraversalQueue.emplace_back(operationNode->id);
    if (!operationNode->lhsOperand.isLeafNode()) {
        handleOperationNodeDuringTraversalQueueInit(*getOperationNodeById(*operationNode->lhsOperand.operationNodeId));
    }
    if (!operationNode->rhsOperand.isLeafNode()) {
        handleOperationNodeDuringTraversalQueueInit(*getOperationNodeById(*operationNode->rhsOperand.operationNodeId));
    }
}

void ExpressionTraversalHelper::addSignalIdentAsUsableInAssignmentLhsIfAssignable(const std::string& signalIdent, const parser::SymbolTable& symbolTableReference) {
    if (!identsOfAssignableSignals.count(signalIdent) && symbolTableReference.canSignalBeAssignedTo(signalIdent).value_or(false)) {
        markSignalAsAssignable(signalIdent);
    }
}

void ExpressionTraversalHelper::determineIdentsOfSignalsUsableInAssignmentLhs(const parser::SymbolTable& symbolTableReference) {
    for (const auto& [_, operandData] : operandNodeDataLookup) {
        if (const auto& operandDataAsVariableExpr = std::dynamic_pointer_cast<syrec::VariableExpression>(operandData); operandDataAsVariableExpr) {
            addSignalIdentAsUsableInAssignmentLhsIfAssignable(operandDataAsVariableExpr->var->var->name, symbolTableReference);
        }
    }
}

void ExpressionTraversalHelper::backtrackToNode(std::size_t operationNodeId, bool removeBacktrackEntryForNode) {
    if (!operationNodeTraversalQueueIdx) {
        return;
    }

    const auto& indexOfOperationNodeInQueue = std::find_if(operationNodeTraversalQueue.cbegin(), operationNodeTraversalQueue.cend(), [&operationNodeId](const std::size_t operationQueueEntryId) { return operationQueueEntryId == operationNodeId; });
    if (indexOfOperationNodeInQueue == operationNodeTraversalQueue.cend()) {
        return;
    }

    operationNodeTraversalQueueIdx = std::distance(operationNodeTraversalQueue.cbegin(), indexOfOperationNodeInQueue);
    if (removeBacktrackEntryForNode) {
        backtrackOperationNodeIds.erase(
                std::remove_if(
                        backtrackOperationNodeIds.begin(),
                        backtrackOperationNodeIds.end(),
                        [&operationNodeId](const std::size_t backtrackOperationNodeId) { return backtrackOperationNodeId >= operationNodeId; }),
                backtrackOperationNodeIds.end());
    } else {
        ++operationNodeTraversalQueueIdx;
        backtrackOperationNodeIds.erase(
                std::remove_if(
                        backtrackOperationNodeIds.begin(),
                        backtrackOperationNodeIds.end(),
                        [&operationNodeId](const std::size_t backtrackOperationNodeId) { return backtrackOperationNodeId > operationNodeId; }),
                backtrackOperationNodeIds.end());
    }
}


std::optional<syrec::Expression::ptr> ExpressionTraversalHelper::getDataOfOperand(std::size_t operandNodeId) const {
    if (!operandNodeDataLookup.count(operandNodeId)) {
        return std::nullopt;   
    }
    return operandNodeDataLookup.at(operandNodeId);
}

ExpressionTraversalHelper::OperandNode ExpressionTraversalHelper::createOperandNode(const syrec::Expression::ptr& expr, const std::optional<std::size_t>& parentOperationNodeId) {
    std::size_t generatedOperandNodeId;
    std::optional<std::size_t> referencedOperationNodeId;

    if (doesExpressionContainNestedExprAsOperand(*expr)) {
        const auto& generatedOperationNode = createOperationNode(expr, parentOperationNodeId);
        operationNodeDataLookup.insert(std::make_pair(generatedOperationNode->id, generatedOperationNode));
        generatedOperandNodeId    = operandNodeIdGenerator.generateNextId();
        referencedOperationNodeId = generatedOperationNode->id;
    }
    else {
        generatedOperandNodeId = operandNodeIdGenerator.generateNextId();
    }
    operandNodeDataLookup.insert(std::make_pair(generatedOperandNodeId, expr));
    return OperandNode({generatedOperandNodeId, referencedOperationNodeId});
}

ExpressionTraversalHelper::OperationNodeReference ExpressionTraversalHelper::createOperationNode(const syrec::Expression::ptr& expr, const std::optional<std::size_t>& parentOperationNodeId) {
    /*
     * We are assuming that this function is only called when trying to create an operation node for either a binary or shift expression, which we can guarantee
     * to a high degree of confidence by making this function only available to this class as well as for derived classes and placing the responsibility for a
     * correct call on the caller. The initialization of the mappedToOperation is needed to prevent a compiler warning due to the variable not being initialized
     * if none of the conditions of the if statement below hold.
     */
    syrec_operation::operation mappedToOperation = syrec_operation::operation::Addition;
    const std::size_t          generatedIdForCurrentNode = operationNodeIdGenerator.generateNextId();
    OperandNode                nodeForLhsOperand;
    OperandNode                nodeForRhsOperand;

    const auto parentIdForNestedExpressions = std::make_optional(generatedIdForCurrentNode);
    if (const auto& exprAsShiftExpr = std::dynamic_pointer_cast<syrec::ShiftExpression>(expr); exprAsShiftExpr) {
        mappedToOperation = *syrec_operation::tryMapShiftOperationFlagToEnum(exprAsShiftExpr->op);
        nodeForLhsOperand = createOperandNode(exprAsShiftExpr->lhs, parentIdForNestedExpressions);
        /*
         * Since the SyReC grammar does not allow signal accesses to be defined in the shift amount, we also do not split a potentially nested "compile time constant" expression
         * defined here and thus will only create one operand node for the whole shift amount.
         */
        nodeForRhsOperand = createOperandNode(createExpressionForNumber(exprAsShiftExpr->rhs, exprAsShiftExpr->lhs->bitwidth()), parentIdForNestedExpressions);
    } else if (const auto& exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr); exprAsBinaryExpr) {
        mappedToOperation = *syrec_operation::tryMapBinaryOperationFlagToEnum(exprAsBinaryExpr->op);
        nodeForLhsOperand = createOperandNode(exprAsBinaryExpr->lhs, parentIdForNestedExpressions);
        nodeForRhsOperand = createOperandNode(exprAsBinaryExpr->rhs, parentIdForNestedExpressions);
    }

    const auto  operationNode          = OperationNode({generatedIdForCurrentNode, parentOperationNodeId, mappedToOperation, nodeForLhsOperand, nodeForRhsOperand, false});
    const auto& operationNodeReference = std::make_shared<OperationNode>(operationNode);
    return operationNodeReference;
}

inline bool ExpressionTraversalHelper::doesExpressionContainNestedExprAsOperand(const syrec::Expression& expr) {
    // We are assuming here that compile time constant expressions were already converted to binary expressions
    return dynamic_cast<const syrec::BinaryExpression*>(&expr) != nullptr || dynamic_cast<const syrec::ShiftExpression*>(&expr) != nullptr;
}

inline syrec::Expression::ptr ExpressionTraversalHelper::createExpressionForNumber(const syrec::Number::ptr& number, unsigned expectedBitwidth) {
    const auto& exprForNumber = std::make_shared<syrec::NumericExpression>(number, expectedBitwidth);
    return exprForNumber;
}