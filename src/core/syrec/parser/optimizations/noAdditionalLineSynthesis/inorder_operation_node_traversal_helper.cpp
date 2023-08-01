#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/inorder_operation_node_traversal_helper.hpp"

using namespace noAdditionalLineSynthesis;

std::optional<std::shared_ptr<syrec::NumericExpression>> InorderOperationNodeTraversalHelper::OperationNodeLeaf::getAsNumber()  const {
    if (std::holds_alternative<std::shared_ptr<syrec::NumericExpression>>(nodeVariantData)) {
        return std::make_optional(std::get<std::shared_ptr<syrec::NumericExpression>>(nodeVariantData));
    }
    return std::nullopt;
}

std::optional<std::shared_ptr<syrec::VariableExpression>> InorderOperationNodeTraversalHelper::OperationNodeLeaf::getAsSignalAccess() const {
    if (std::holds_alternative<std::shared_ptr<syrec::VariableExpression>>(nodeVariantData)) {
        return std::make_optional(std::get<std::shared_ptr<syrec::VariableExpression>>(nodeVariantData));
    }
    return std::nullopt;
}

syrec::expression::ptr InorderOperationNodeTraversalHelper::OperationNodeLeaf::getData() const {
    if (const auto& dataAsSignalAccess = getAsSignalAccess(); dataAsSignalAccess.has_value()) {
        return *dataAsSignalAccess;
    }
    return *getAsNumber();
}


std::optional<InorderOperationNodeTraversalHelper::OperationNodeReference> InorderOperationNodeTraversalHelper::getNextOperationNode() {
    while (!operationNodeTraversalQueue.empty()) {
        const auto& nextOperationNode = std::make_optional(operationNodesLookup.at(operationNodeTraversalQueue.front()));
        operationNodeTraversalQueue.pop();
        return nextOperationNode;
    }
    return std::nullopt;
}

std::optional<InorderOperationNodeTraversalHelper::OperationNodeReference> InorderOperationNodeTraversalHelper::peekNextOperationNode() const {
    if (operationNodeTraversalQueue.empty()) {
        return std::nullopt;
    }
    return std::make_optional(operationNodesLookup.at(operationNodeTraversalQueue.front()));
}

std::optional<std::size_t> InorderOperationNodeTraversalHelper::getNodeIdOfLastOperationIdOfCurrentSubexpression() const {
    if (operationNodeTraversalQueue.empty()) {
        return std::nullopt;
    }

    const auto& nextOperationNode = operationNodesLookup.at(operationNodeTraversalQueue.front());
    if (!nextOperationNode->parentNodeId.has_value()) {
        return std::make_optional(operationNodeTraversalQueue.front());
    }

    const auto& parentOperationNode = operationNodesLookup.at(*nextOperationNode->parentNodeId);
    if (parentOperationNode->lhs.nodeId == nextOperationNode->nodeId) {
        return parentOperationNode->rhs.nodeId - 1;
    }
    return parentOperationNode->lhs.nodeId - 1;
}


void InorderOperationNodeTraversalHelper::buildOperationNodesQueue(const syrec::expression::ptr& expr) {
    buildOperationNode(expr, std::nullopt);
}

InorderOperationNodeTraversalHelper::TraversalNode InorderOperationNodeTraversalHelper::buildOperationNode(const syrec::expression::ptr& expr, const std::optional<std::size_t>& parentNodeId) {
    if (const auto& exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr); exprAsBinaryExpr != nullptr) {
        const auto operationNodeId = operationNodeIdCounter++;
        operationNodeTraversalQueue.emplace(operationNodeId);
        const auto& lhsNode = buildOperationNode(exprAsBinaryExpr->lhs, std::make_optional(operationNodeId));
        const auto& rhsNode               = buildOperationNode(exprAsBinaryExpr->rhs, std::make_optional(operationNodeId));
        const auto& mappedBinaryOperation = *syrec_operation::tryMapBinaryOperationFlagToEnum(exprAsBinaryExpr->op);
        operationNodesLookup.insert(std::make_pair(operationNodeId, std::make_shared<OperationNode>(operationNodeId, parentNodeId, mappedBinaryOperation, lhsNode, rhsNode)));
        return TraversalNode(operationNodeId, true);
    }
    if (const auto& exprAsShiftExpr = std::dynamic_pointer_cast<syrec::ShiftExpression>(expr); exprAsShiftExpr != nullptr) {
        const auto operationNodeId = operationNodeIdCounter++;
        operationNodeTraversalQueue.emplace(operationNodeId);
        const auto& lhsNode               = buildOperationNode(exprAsShiftExpr->lhs, std::make_optional(operationNodeId));
        const auto& rhsNode              = buildOperationNode(exprAsShiftExpr->rhs, determineBitwidthOfExpr(exprAsShiftExpr->lhs));
        const auto& mappedShiftOperation  = *syrec_operation::tryMapShiftOperationFlagToEnum(exprAsShiftExpr->op);
        operationNodesLookup.insert(std::make_pair(operationNodeId, std::make_shared<OperationNode>(operationNodeId, parentNodeId, mappedShiftOperation, lhsNode, rhsNode)));
        return TraversalNode(operationNodeId, true);
    }

    const auto  leafNodeId           = leafNodeIdCounter++;
    const auto& exprAsConstantNumber = std::dynamic_pointer_cast<syrec::NumericExpression>(expr);

    std::variant<std::shared_ptr<syrec::NumericExpression>, std::shared_ptr<syrec::VariableExpression>> leafNodeData;
    if (const auto& exprAsSignalAccess = std::dynamic_pointer_cast<syrec::VariableExpression>(expr); exprAsSignalAccess != nullptr) {
        leafNodeData.emplace<std::shared_ptr<syrec::VariableExpression>>(exprAsSignalAccess);
    } else {
        leafNodeData.emplace<std::shared_ptr<syrec::NumericExpression>>(exprAsConstantNumber);
    }
    leafNodesLookup.insert(std::make_pair(leafNodeId, std::make_shared<OperationNodeLeaf>(leafNodeId, leafNodeData)));
    return TraversalNode(leafNodeId, false);
}

std::optional<InorderOperationNodeTraversalHelper::OperationNodeLeafReference> InorderOperationNodeTraversalHelper::getLeafNodeOfOperationNode(const OperationNodeReference& operationNode) const {
    if (operationNode->lhs.isOperationNode && operationNode->rhs.isOperationNode) {
        return std::nullopt;
    }
    return std::make_optional(leafNodesLookup.at(!operationNode->lhs.isOperationNode ? operationNode->lhs.nodeId : operationNode->rhs.nodeId));
}

std::optional<InorderOperationNodeTraversalHelper::OperationNodeReference> InorderOperationNodeTraversalHelper::getOperationNodeForId(std::size_t operationNodeId) const {
    return operationNodeId < operationNodesLookup.size() ? std::make_optional(operationNodesLookup.at(operationNodeId)) : std::nullopt;
}

InorderOperationNodeTraversalHelper::TraversalNode InorderOperationNodeTraversalHelper::buildOperationNode(const syrec::Number::ptr& number, unsigned int expectedBitwidthOfOperation) {
    const auto  leafNodeId           = leafNodeIdCounter++;
    const auto& numericExpr = std::make_shared<syrec::NumericExpression>(number, expectedBitwidthOfOperation);
    std::variant<std::shared_ptr<syrec::NumericExpression>, std::shared_ptr<syrec::VariableExpression>> leafNodeData;
    leafNodeData.emplace<std::shared_ptr<syrec::NumericExpression>>(numericExpr);
    leafNodesLookup.insert(std::make_pair(leafNodeId, std::make_shared<OperationNodeLeaf>(leafNodeId, leafNodeData)));
    return TraversalNode(leafNodeId, false);
}

std::optional<bool> InorderOperationNodeTraversalHelper::areAllOperandsOfOperationNodeLeafNodes(const OperationNodeReference& operationNode) const {
    if (operationNodesLookup.size() >= operationNode->nodeId) {
        return std::nullopt;
    }

    return operationNode->lhs.isOperationNode && operationNode->rhs.isOperationNode;
}

std::optional<std::pair<InorderOperationNodeTraversalHelper::OperationNodeLeafReference, InorderOperationNodeTraversalHelper::OperationNodeLeafReference>> InorderOperationNodeTraversalHelper::getLeafNodesOfOperationNode(const OperationNodeReference& operationNode) const {
    if (operationNode->lhs.isOperationNode || operationNode->rhs.isOperationNode) {
        return std::nullopt;
    }
    return std::make_optional(std::make_pair(leafNodesLookup.at(operationNode->lhs.nodeId), leafNodesLookup.at(operationNode->rhs.nodeId)));
}

bool InorderOperationNodeTraversalHelper::doesExprDefineSignalAccess(const syrec::expression::ptr& expr) {
    return std::dynamic_pointer_cast<syrec::VariableExpression>(expr) != nullptr;
}

unsigned InorderOperationNodeTraversalHelper::determineBitwidthOfExpr(const syrec::expression::ptr& expr) const {
    if (const auto& exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr); exprAsBinaryExpr != nullptr) {
        return determineBitwidthOfExpr(exprAsBinaryExpr->lhs);
    }
    if (const auto& exprAsShiftExpr = std::dynamic_pointer_cast<syrec::ShiftExpression>(expr); exprAsShiftExpr != nullptr) {
        return determineBitwidthOfExpr(exprAsShiftExpr->lhs);
    }
    if (const auto& exprAsSignalAccess = std::dynamic_pointer_cast<syrec::VariableExpression>(expr); exprAsSignalAccess != nullptr) {
        return determineBitwidthOfSignalAccess(exprAsSignalAccess->var);
    }
    if (const auto& exprAsNumericExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(expr); exprAsNumericExpr != nullptr) {
        return exprAsNumericExpr->bwidth;   
    }
    return 0;
}

unsigned InorderOperationNodeTraversalHelper::determineBitwidthOfSignalAccess(const syrec::VariableAccess::ptr& signalAccess) const {
    const auto& symbolTableEntryForSignal = std::get<syrec::Variable::ptr>(*symbolTableReference->getVariable(signalAccess->var->name));
    if (signalAccess->range.has_value() && signalAccess->range->first->isConstant() && signalAccess->range->second->isConstant()) {
        return signalAccess->range->second->evaluate({}) - signalAccess->range->first->evaluate({});
    }
    return symbolTableEntryForSignal->bitwidth;
}

unsigned InorderOperationNodeTraversalHelper::mapBinaryOperationValueToFlag(syrec_operation::operation operation) {
    switch (operation) {
        
    }
}

unsigned InorderOperationNodeTraversalHelper::mapAssignmentEnumValueToFlag(syrec_operation::operation operation) {
    switch (operation) {
        case syrec_operation::operation::AddAssign:
            return syrec::AssignStatement::Add;
        case syrec_operation::operation::MinusAssign:
            return syrec::AssignStatement::Subtract;
        default:
            return syrec::AssignStatement::Exor; 
    }
}
