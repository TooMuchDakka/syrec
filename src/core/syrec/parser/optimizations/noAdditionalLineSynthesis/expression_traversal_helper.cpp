#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/expression_traversal_helper.hpp"

#include <algorithm>

using namespace noAdditionalLineSynthesis;

bool ExpressionTraversalHelper::OperandNode::isLeafNode() const {
    return !referencedOperationNodeId.has_value();
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
    if (const auto& parentOperationNodeReference = getOperationNodeById(parentOperationNodeId); parentOperationNodeReference.has_value()) {
        const auto& lhsOperand = parentOperationNodeReference->get()->lhsOperand;
        if (lhsOperand.referencedOperationNodeId.has_value() && *lhsOperand.referencedOperationNodeId == nestedOperationNodeId) {
            return lhsOperand.id;
        }
        const auto& rhsOperand = parentOperationNodeReference->get()->rhsOperand;
        if (rhsOperand.referencedOperationNodeId.has_value() && *rhsOperand.referencedOperationNodeId == nestedOperationNodeId) {
            return rhsOperand.id;
        }
    }
    return std::nullopt;
}


bool ExpressionTraversalHelper::canSignalBeUsedOnAssignmentLhs(const std::string& signalIdent) const {
    return !identsOfAssignableSignals.count(signalIdent);
}


void ExpressionTraversalHelper::markOperationNodeAsPotentialBacktrackOption(std::size_t operationNodeId) {
    if (const auto& fetchedOperationNodeForId = getOperationNodeById(operationNodeId); fetchedOperationNodeForId.has_value()) {
        if (!std::any_of(backtrackOperationNodeIds.crbegin(), backtrackOperationNodeIds.crend(), [&operationNodeId](const std::size_t existingCheckpointOperationNodeId) { return existingCheckpointOperationNodeId == operationNodeId; })) {
            backtrackOperationNodeIds.emplace_back(operationNodeId);
        }
    }
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
    if (backtrackOperationNodeIds.empty()) {
        return;
    }

    const auto& checkpointOperationNodeId = backtrackOperationNodeIds.back();
    backtrackOperationNodeIds.pop_back();

    std::size_t index      = operationNodeTraversalQueue.size() - 1;
    bool        foundMatch = false;
    for (auto traversalQueueIterator = operationNodeTraversalQueue.rbegin(); traversalQueueIterator != operationNodeTraversalQueue.rend() && !foundMatch; ++traversalQueueIterator) {
        foundMatch = *traversalQueueIterator == checkpointOperationNodeId;
        if (foundMatch) {
            operationNodeTraversalQueueIdx = index;
        } else {
            --index;
        }
    }
}

void ExpressionTraversalHelper::buildTraversalQueue(const syrec::expression::ptr& expr, const parser::SymbolTable& symbolTableReference) {
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
    
    operationNodeTraversalQueue.emplace_back(operationNode->id);
    if (!operationNode->lhsOperand.isLeafNode()) {
        handleOperationNodeDuringTraversalQueueInit(*getOperationNodeById(operationNode->lhsOperand.id));
    }
    if (!operationNode->rhsOperand.isLeafNode()) {
        handleOperationNodeDuringTraversalQueueInit(*getOperationNodeById(operationNode->rhsOperand.id));
    }
}

void ExpressionTraversalHelper::addSignalIdentAsUsableInAssignmentLhsIfAssignable(const std::string& signalIdent, const parser::SymbolTable& symbolTableReference) {
    if (!identsOfAssignableSignals.count(signalIdent) && symbolTableReference.canSignalBeAssignedTo(signalIdent)) {
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

std::optional<ExpressionTraversalHelper::OperationNodeReference> ExpressionTraversalHelper::getOperationNodeById(std::size_t operationNodeId) const {
    if (!operationNodeDataLookup.count(operationNodeId)) {
        return std::nullopt;
    }
    return operationNodeDataLookup.at(operationNodeId);
}

ExpressionTraversalHelper::OperandNode ExpressionTraversalHelper::createOperandNode(const syrec::expression::ptr& expr, const std::optional<std::size_t>& parentOperationNodeId) {
    if (doesExpressionContainNestedExprAsOperand(*expr)) {
        const auto& generatedOperationNode = createOperationNode(expr, parentOperationNodeId);
        operationNodeDataLookup.insert(std::make_pair(generatedOperationNode->id, generatedOperationNode));
        return OperandNode({generatedOperationNode->id, parentOperationNodeId});
    }

    const auto& generatedIdForExprOperand = operandNodeIdGenerator.generateNextId();
    operandNodeDataLookup.insert(std::make_pair(generatedIdForExprOperand, expr));
    return OperandNode({generatedIdForExprOperand, std::nullopt});
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
        nodeForLhsOperand = createOperandNode(exprAsShiftExpr->lhs, parentOperationNodeId);
        nodeForRhsOperand = createOperandNode(createExpressionForNumber(exprAsShiftExpr->rhs, exprAsShiftExpr->lhs->bitwidth()), parentIdForNestedExpressions);
    } else if (const auto& exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr); exprAsBinaryExpr) {
        mappedToOperation = *syrec_operation::tryMapBinaryOperationFlagToEnum(exprAsBinaryExpr->op);
        nodeForLhsOperand = createOperandNode(exprAsBinaryExpr->lhs, parentIdForNestedExpressions);
        nodeForRhsOperand = createOperandNode(exprAsBinaryExpr->rhs, parentIdForNestedExpressions);
    }

    const auto  operationNode          = OperationNode({generatedIdForCurrentNode, parentIdForNestedExpressions, mappedToOperation, nodeForLhsOperand, nodeForRhsOperand});
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



//#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/expression_traversal_helper.hpp"
//
//#include <algorithm>
//
//using namespace noAdditionalLineSynthesis;
//
//void ExpressionTraversalHelper::switchOperandsOfOperation(std::size_t operationNodeId) const {
//    if (const auto& operationNode = getOperationNodeById(operationNodeId); operationNode.has_value()) {
//        const auto copyOfLhsNode = operationNode->get()->lhs;
//        operationNode->get()->lhs = operationNode->get()->rhs;
//        operationNode->get()->rhs = copyOfLhsNode;
//    }
//}
//
//std::optional<ExpressionTraversalHelper::OperationNodeReference> ExpressionTraversalHelper::tryGetNextOperationNode() {
//    if (operationNodeTraversalQueueIndex >= operationNodeTraversalQueue.size()) {
//        return std::nullopt;
//    }
//
//    const auto idOfNextOperationNodeInQueue = operationNodeTraversalQueue.at(operationNodeTraversalQueueIndex++);
//    return getOperationNodeById(idOfNextOperationNodeInQueue);
//}
//
//std::optional<ExpressionTraversalHelper::OperationNodeReference> ExpressionTraversalHelper::peekNextOperationNode() const {
//    if (operationNodeTraversalQueueIndex >= operationNodeTraversalQueue.size()) {
//        return std::nullopt;
//    }
//    const auto idOfNextOperationNodeInQueue = operationNodeTraversalQueue.at(operationNodeTraversalQueueIndex);
//    return getOperationNodeById(idOfNextOperationNodeInQueue);
//}
//
//std::optional<std::pair<ExpressionTraversalHelper::OperandNode, ExpressionTraversalHelper::OperandNode>> ExpressionTraversalHelper::tryGetNonNestedOperationOperands(std::size_t operationNodeId) const {
//    if (const auto& operationNodeForId = getOperationNodeById(operationNodeId); operationNodeForId.has_value() && !operationNodeForId->get()->lhs.isLeafNode() && !operationNodeForId->get()->rhs.isLeafNode()) {
//        return std::make_pair(operationNodeForId->get()->lhs, operationNodeForId->get()->rhs);
//    }
//    return std::nullopt;
//}
//
//std::optional<ExpressionTraversalHelper::OperandNode> ExpressionTraversalHelper::tryGetNonNestedOperationOperand(std::size_t operationNodeId) const {
//    if (const auto& operationNodeForId = getOperationNodeById(operationNodeId); operationNodeForId.has_value()) {
//        if (operationNodeForId->get()->lhs.isLeafNode()) {
//            return operationNodeForId->get()->lhs;
//        }
//        if (operationNodeForId->get()->rhs.isLeafNode()) {
//            return operationNodeForId->get()->rhs;
//        }
//    }
//    return std::nullopt;
//}
//
//std::optional<syrec::expression::ptr> ExpressionTraversalHelper::getDataOfOperandNode(std::size_t operandNodeId) const {
//    if (operandNodeDataLookup.count(operandNodeId)) {
//        return operandNodeDataLookup.at(operandNodeId);
//    }
//    return std::nullopt;
//}
//
//bool ExpressionTraversalHelper::createCheckPointAtOperationNode(std::size_t operationNodeId) {
//    if (const auto& fetchedOperationNodeForId = getOperationNodeById(operationNodeId); fetchedOperationNodeForId.has_value()) {
//        if (!std::any_of(checkpoints.cbegin(), checkpoints.cend(), [&operationNodeId](const std::size_t existingCheckpointOperationNodeId) { return existingCheckpointOperationNodeId == operationNodeId; })) {
//            checkpoints.emplace_back(operationNodeId);
//            return true;
//        }
//    }
//    return false;
//}
//
//bool ExpressionTraversalHelper::tryBacktrackToLastCheckpoint() {
//    if (checkpoints.empty()) {
//        return true;
//    }
//
//    const auto& checkpointOperationNodeId = checkpoints.back();
//    checkpoints.pop_back();
//
//    std::size_t index = operationNodeTraversalQueue.size() - 1;
//    bool        foundMatch = false;
//    for (auto traversalQueueIterator = operationNodeTraversalQueue.rbegin(); traversalQueueIterator != operationNodeTraversalQueue.rend() && !foundMatch; ++traversalQueueIterator) {
//        foundMatch = *traversalQueueIterator == checkpointOperationNodeId;
//        if (foundMatch) {
//            operationNodeTraversalQueueIndex = index;
//        } else {
//            --index;
//        }
//    }
//    return foundMatch;
//}
//
//// START OF IMPLEMENTATION OF PRIVATE FUNCTIONS
//std::size_t ExpressionTraversalHelper::IdGenerator::generateNextId() {
//    return lastGeneratedId++;
//}
//
//void ExpressionTraversalHelper::IdGenerator::reset() {
//    lastGeneratedId = 0;
//}
//
//void ExpressionTraversalHelper::resetInternals() {
//    operandNodeDataLookup.clear();
//    operationNodeDataLookup.clear();
//    operationNodeTraversalQueue.clear();
//    checkpoints.clear();
//    operationNodeTraversalQueueIndex = 0;
//    operationNodeIdGenerator.reset();
//    operandNodeIdGenerator.reset();
//}
//
//ExpressionTraversalHelper::OperandNode ExpressionTraversalHelper::createNodeFor(const syrec::expression::ptr& expr, const std::optional<std::size_t>& parentOperationNodeId) {
//    if (doesExpressionDefineExprOperand(*expr)) {
//        const auto& generatedOperationNode = createOperationNodeFor(expr, parentOperationNodeId);
//        operationNodeDataLookup.insert(std::make_pair(generatedOperationNode->id, generatedOperationNode));
//        return OperandNode({generatedOperationNode->id, false});
//    }
//
//    const auto& generatedIdForExprOperand = operandNodeIdGenerator.generateNextId();
//    operandNodeDataLookup.insert(std::make_pair(generatedIdForExprOperand, expr));
//    return OperandNode({generatedIdForExprOperand, true});
//}
//
//ExpressionTraversalHelper::OperationNodeReference ExpressionTraversalHelper::createOperationNodeFor(const syrec::expression::ptr& expr, const std::optional<std::size_t>& parentOperationNodeId) {
//    /*
//     * We are assuming that this function is only called when trying to create an operation node for either a binary or shift expression, which we can guarantee
//     * to a high degree of confidence by making this function only available to this class as well as for derived classes and placing the responsibility for a
//     * correct call on the caller. The initialization of the mappedToOperation is needed to prevent a compiler warning due to the variable not being initialized
//     * if none of the conditions of the if statement below hold.
//     */ 
//    syrec_operation::operation mappedToOperation = syrec_operation::operation::Addition;
//    const auto                 generatedIdForCurrentNode = operationNodeIdGenerator.generateNextId();
//    OperandNode                nodeForLhsOperand;
//    OperandNode                nodeForRhsOperand;
//
//    const auto parentIdForNestedExpressions = std::make_optional(generatedIdForCurrentNode);
//    if (const auto& exprAsShiftExpr = std::dynamic_pointer_cast<syrec::ShiftExpression>(expr); exprAsShiftExpr) {
//        mappedToOperation = *syrec_operation::tryMapShiftOperationFlagToEnum(exprAsShiftExpr->op);
//        nodeForLhsOperand = createNodeFor(exprAsShiftExpr->lhs, parentOperationNodeId);
//        nodeForRhsOperand = createNodeFor(createExprForNumberOperand(exprAsShiftExpr->rhs, exprAsShiftExpr->lhs->bitwidth()), parentIdForNestedExpressions);
//    } else if (const auto& exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr); exprAsBinaryExpr) {
//        mappedToOperation = *syrec_operation::tryMapBinaryOperationFlagToEnum(exprAsBinaryExpr->op);
//        nodeForLhsOperand = createNodeFor(exprAsShiftExpr->lhs, parentIdForNestedExpressions);
//        nodeForRhsOperand = createNodeFor(createExprForNumberOperand(exprAsShiftExpr->rhs, exprAsShiftExpr->lhs->bitwidth()), parentIdForNestedExpressions);
//    }
//
//    const auto  operationNode          = OperationNode({generatedIdForCurrentNode, parentIdForNestedExpressions, mappedToOperation, nodeForLhsOperand, nodeForRhsOperand});
//    const auto& operationNodeReference = std::make_shared<OperationNode>(operationNode);
//    return operationNodeReference;
//}
//
//std::optional<ExpressionTraversalHelper::OperationNodeReference> ExpressionTraversalHelper::getOperationNodeById(std::size_t operationNodeId) const {
//    if (operationNodeDataLookup.count(operationNodeId)) {
//        return operationNodeDataLookup.at(operationNodeId);
//    }
//    return std::nullopt;
//}
//
//bool ExpressionTraversalHelper::doesExpressionDefineExprOperand(const syrec::expression& expr) {
//    // We are assuming here that compile time constant expressions were already converted to binary expressions
//    return dynamic_cast<const syrec::NumericExpression*>(&expr) != nullptr || dynamic_cast<const syrec::VariableExpression*>(&expr) != nullptr;
//}
//
//syrec::expression::ptr ExpressionTraversalHelper::createExprForNumberOperand(const syrec::Number::ptr& number, unsigned expectedBitwidth) {
//    const auto& exprForNumber = std::make_shared<syrec::NumericExpression>(number, expectedBitwidth);
//    return exprForNumber;
//}