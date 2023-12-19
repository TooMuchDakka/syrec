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


std::optional<syrec::VariableExpression::ptr> ExpressionTraversalHelper::getOperandAsVariableExpr(std::size_t operandNodeId) const {
    if (std::optional<syrec::expression::ptr> operandData = getDataOfOperand(operandNodeId); operandData.has_value() && std::dynamic_pointer_cast<syrec::VariableExpression>(*operandData)) {
        return operandData;
    }
    return std::nullopt;
}

std::optional<syrec::expression::ptr> ExpressionTraversalHelper::getOperandAsExpr(std::size_t operandNodeId) const {
    if (std::optional<syrec::expression::ptr> operandData = getDataOfOperand(operandNodeId); operandData.has_value() && std::dynamic_pointer_cast<syrec::VariableExpression>(*operandData) == nullptr) {
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

void ExpressionTraversalHelper::backtrack() {
    if (backtrackOperationNodeIds.empty() || !operationNodeTraversalQueueIdx) {
        // If we do not reset the traversal index here after all backtracking checkpoints were removed, we would never reach the start of the traversal queue again through backtracking
        operationNodeTraversalQueueIdx = 0;
        return;
    }

    const auto& checkpointOperationNodeId = backtrackOperationNodeIds.back();
    backtrackOperationNodeIds.pop_back();

    bool foundMatch = false;
    // Since the operation node traversal queue index is always advanced when fetching the next operation node, we take the current value of the latter -1 as the index for the current operation node
    for (auto traversalQueueIterator = std::next(operationNodeTraversalQueue.cbegin(), operationNodeTraversalQueueIdx - 1); traversalQueueIterator != operationNodeTraversalQueue.cbegin() && !foundMatch; --traversalQueueIterator) {
        foundMatch = *traversalQueueIterator == checkpointOperationNodeId;
        operationNodeTraversalQueueIdx -= !foundMatch;
    }
}

void ExpressionTraversalHelper::buildTraversalQueue(const syrec::expression::ptr& expr, const parser::SymbolTable& symbolTableReference) {
    resetInternals();
    operationNodeIdGenerator.generateNextId();
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
        identsOfAssignableSignals.emplace(signalIdent);
    }
}

void ExpressionTraversalHelper::determineIdentsOfSignalsUsableInAssignmentLhs(const parser::SymbolTable& symbolTableReference) {
    for (const auto& [_, operandData] : operandNodeDataLookup) {
        if (const auto& operandDataAsVariableExpr = std::dynamic_pointer_cast<syrec::VariableExpression>(operandData); operandDataAsVariableExpr) {
            addSignalIdentAsUsableInAssignmentLhsIfAssignable(operandDataAsVariableExpr->var->var->name, symbolTableReference);
        }
    }
}

std::optional<syrec::expression::ptr> ExpressionTraversalHelper::getDataOfOperand(std::size_t operandNodeId) const {
    if (!operandNodeDataLookup.count(operandNodeId)) {
        return std::nullopt;   
    }
    return operandNodeDataLookup.at(operandNodeId);
}

ExpressionTraversalHelper::OperandNode ExpressionTraversalHelper::createOperandNode(const syrec::expression::ptr& expr, const std::optional<std::size_t>& parentOperationNodeId) {
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

ExpressionTraversalHelper::OperationNodeReference ExpressionTraversalHelper::createOperationNode(const syrec::expression::ptr& expr, const std::optional<std::size_t>& parentOperationNodeId) {
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
        nodeForRhsOperand = createOperandNode(createExpressionForNumber(exprAsShiftExpr->rhs, exprAsShiftExpr->lhs->bitwidth()), parentIdForNestedExpressions);
    } else if (const auto& exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr); exprAsBinaryExpr) {
        mappedToOperation = *syrec_operation::tryMapBinaryOperationFlagToEnum(exprAsBinaryExpr->op);
        nodeForLhsOperand = createOperandNode(exprAsBinaryExpr->lhs, parentIdForNestedExpressions);
        nodeForRhsOperand = createOperandNode(exprAsBinaryExpr->rhs, parentIdForNestedExpressions);
    }

    const auto  operationNode          = OperationNode({generatedIdForCurrentNode, parentOperationNodeId, mappedToOperation, nodeForLhsOperand, nodeForRhsOperand});
    const auto& operationNodeReference = std::make_shared<OperationNode>(operationNode);
    return operationNodeReference;
}

inline bool ExpressionTraversalHelper::doesExpressionContainNestedExprAsOperand(const syrec::expression& expr) {
    // We are assuming here that compile time constant expressions were already converted to binary expressions
    return dynamic_cast<const syrec::BinaryExpression*>(&expr) != nullptr || dynamic_cast<const syrec::ShiftExpression*>(&expr) != nullptr;
}

inline syrec::expression::ptr ExpressionTraversalHelper::createExpressionForNumber(const syrec::Number::ptr& number, unsigned expectedBitwidth) {
    const auto& exprForNumber = std::make_shared<syrec::NumericExpression>(number, expectedBitwidth);
    return exprForNumber;
}