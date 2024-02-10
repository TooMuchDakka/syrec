#include "core/syrec/parser/optimizations/no_additional_line_assignment.hpp"
#include "core/syrec/parser/operation.hpp"

#include <algorithm>
#include <set>
#include <stack>

using namespace optimizations;

std::optional<syrec::Expression::ptr> LineAwareOptimization::TreeTraversalNode::fetchStoredExpr() const {
    if (!isInternalNode && std::holds_alternative<LeafData>(nodeData)) {
        const auto& leafData = std::get<LeafData>(nodeData);

        if (std::holds_alternative<std::shared_ptr<syrec::VariableExpression>>(leafData)) {
            return std::make_optional<syrec::Expression::ptr>(std::get<std::shared_ptr<syrec::VariableExpression>>(leafData));
        }
        if (std::holds_alternative<std::shared_ptr<syrec::NumericExpression>>(leafData)) {
            return std::make_optional<syrec::Expression::ptr>(std::get<std::shared_ptr<syrec::NumericExpression>>(leafData));
        }
    }
    return std::nullopt;
}

std::optional<syrec_operation::operation> LineAwareOptimization::TreeTraversalNode::fetchStoredOperation() const {
    return (isInternalNode && std::holds_alternative<InternalNodeData>(nodeData))
        ? std::make_optional(std::get<syrec_operation::operation>(std::get<InternalNodeData>(nodeData)))
        : std::nullopt;
}

std::optional<LineAwareOptimization::TreeTraversalNode::ReferenceExpr> LineAwareOptimization::TreeTraversalNode::fetchReferenceExpr() const {
    return (isInternalNode && std::holds_alternative<InternalNodeData>(nodeData))
        ? std::make_optional(std::get<LineAwareOptimization::TreeTraversalNode::ReferenceExpr>(std::get<InternalNodeData>(nodeData)))
        : std::nullopt;
}

std::vector<std::size_t> LineAwareOptimization::PostOrderTreeTraversal::getTraversalIndexOfAllNodes() const {
    std::vector<std::size_t> indizes(treeNodesInPostOrder.size(), 0);
    for (std::size_t i = 0; i < indizes.size(); ++i) {
        indizes[i] = i;
    }
    return indizes;
}


std::vector<std::size_t> LineAwareOptimization::PostOrderTreeTraversal::getTraversalIndexOfAllLeafNodes() const {
    return getEitherInternalOrLeafNodes(true);
}

std::vector<std::size_t> LineAwareOptimization::PostOrderTreeTraversal::getTraversalIndexOfAllOperationNodes() const {
    return getEitherInternalOrLeafNodes(false);
}

std::optional<std::size_t> LineAwareOptimization::PostOrderTreeTraversal::getTraversalIdxOfParentOfNode(std::size_t postOrderTraversalIdx) const {
    return postOrderTraversalIdx < parentPerNodeLookup.size() && parentPerNodeLookup.at(postOrderTraversalIdx).has_value()
        ? std::make_optional(*parentPerNodeLookup[postOrderTraversalIdx])
        : std::nullopt;
}

std::optional<const LineAwareOptimization::TreeTraversalNode> LineAwareOptimization::PostOrderTreeTraversal::getParentOfNode(std::size_t postOrderTraversalIdx) const {
    const auto& traversalIdxOfParentNode = getTraversalIdxOfParentOfNode(postOrderTraversalIdx);
    return traversalIdxOfParentNode.has_value() 
        ? std::make_optional(treeNodesInPostOrder[*traversalIdxOfParentNode])
        : std::nullopt;
}


std::optional<const LineAwareOptimization::TreeTraversalNode> LineAwareOptimization::PostOrderTreeTraversal::getNode(std::size_t postOrderTraversalIdx) const {
    return postOrderTraversalIdx < treeNodesInPostOrder.size()
        ? std::make_optional(treeNodesInPostOrder[postOrderTraversalIdx])
        : std::nullopt;
}

std::optional<std::pair<std::size_t, std::size_t>> LineAwareOptimization::PostOrderTreeTraversal::getChildNodesForOperationNode(std::size_t postOrderTraversalIdxOfOperationNode) const {
    return parentChildRelationLookup.count(postOrderTraversalIdxOfOperationNode) != 0
        ? std::make_optional(parentChildRelationLookup.at(postOrderTraversalIdxOfOperationNode).childNodeTraversalIndizes)
        : std::nullopt;
}

bool LineAwareOptimization::PostOrderTreeTraversal::areOperandsOnlyAdditionSubtractionOrXor() const {
    return std::all_of(
    treeNodesInPostOrder.cbegin(),
    treeNodesInPostOrder.cend(),
    [](const TreeTraversalNode& treeNode) {
        if (!treeNode.isInternalNode) {
            return true;
        }
        return isOperationAdditionSubtractionOrXor(*treeNode.fetchStoredOperation());
    });
}

bool LineAwareOptimization::PostOrderTreeTraversal::doOperandsOperateOnLeafNodesOnly(const std::vector<syrec_operation::operation>& operands) const {
    const std::set operandsOfInterest(operands.cbegin(), operands.cend());

    const auto& operationNodeTraversalIndizes = getTraversalIndexOfAllOperationNodes();
    return std::all_of(
    operationNodeTraversalIndizes.cbegin(),
    operationNodeTraversalIndizes.cend(),
    [this, &operandsOfInterest](const std::size_t operationNodeTraversalIdx) {
        const auto  operation          = *getNode(operationNodeTraversalIdx)->fetchStoredOperation();
        if (operandsOfInterest.count(operation) == 0) {
            return true;
        }

        const auto& operandsOfOperationTraversalIndizes = getChildNodesForOperationNode(operationNodeTraversalIdx);
        const auto& operandsOfOperation                 = std::make_pair(*getNode(operandsOfOperationTraversalIndizes->first), *getNode(operandsOfOperationTraversalIndizes->second));
        return !operandsOfOperation.first.isInternalNode && !operandsOfOperation.second.isInternalNode;
    });
}

bool LineAwareOptimization::PostOrderTreeTraversal::doTwoConsecutiveOperationNodesHaveNotAdditionSubtractionOrXorAsOperand() const {
    const auto& operationNodesIdxs = getTraversalIndexOfAllOperationNodes();
    return std::any_of(
        operationNodesIdxs.cbegin(),
        operationNodesIdxs.cend(),
        [this](const std::size_t operationNodeTraversalIdx) {
            const auto& parentNode = getParentOfNode(operationNodeTraversalIdx);
            if (parentNode.has_value()) {
                const auto operationOfCurrentNode = *getNode(operationNodeTraversalIdx)->fetchStoredOperation();
                const auto operationOfParentNode  = *parentNode->fetchStoredOperation();
                return !isOperationAdditionSubtractionOrXor(operationOfCurrentNode) && !isOperationAdditionSubtractionOrXor(operationOfParentNode);
            }
            return true;
        }
    );
}

void LineAwareOptimization::PostOrderTreeTraversal::determineParentChildRelations(const std::vector<TreeTraversalNode>& nodesInPostOrder) {
    std::stack<std::size_t>                 processedNodeIdxStack;
    parentPerNodeLookup.resize(nodesInPostOrder.size(), std::nullopt);

    std::size_t currentTraversalIdx = 0;
    for (const auto& node: nodesInPostOrder) {
        if (node.isInternalNode) {
            const auto rightChildIdx = processedNodeIdxStack.top();
            processedNodeIdxStack.pop();
            const auto& leftChildIdx = processedNodeIdxStack.top();
            processedNodeIdxStack.pop();

            parentPerNodeLookup[rightChildIdx] = currentTraversalIdx;
            parentPerNodeLookup[leftChildIdx]  = currentTraversalIdx;
            parentChildRelationLookup.insert({currentTraversalIdx, ParentChildTreeTraversalNodeRelation(currentTraversalIdx, std::make_pair(leftChildIdx, rightChildIdx))});
        }
        processedNodeIdxStack.push(currentTraversalIdx++);
    }
}

std::vector<std::size_t> LineAwareOptimization::PostOrderTreeTraversal::getEitherInternalOrLeafNodes(bool getLeafNodes) const {
    std::vector<std::size_t> matchingNodes;
    for (std::size_t i = 0; i < treeNodesInPostOrder.size(); ++i) {
        if (getLeafNodes != treeNodesInPostOrder.at(i).isInternalNode) {
            matchingNodes.emplace_back(i);
        }
    }
    return matchingNodes;
}


LineAwareOptimization::LineAwareOptimizationResult LineAwareOptimization::optimize(const std::shared_ptr<syrec::AssignStatement>& assignmentStatement) {
    const auto& usedAssignmentOperation = tryMapAssignmentOperand(assignmentStatement->op);
    const auto& rhsAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(assignmentStatement->rhs);
    if (!usedAssignmentOperation.has_value() || rhsAsBinaryExpr == nullptr) {
        return LineAwareOptimizationResult(assignmentStatement);
    }
    
    const auto postOrderRepresentation = createPostOrderRepresentation(assignmentStatement->rhs);
    if (!postOrderRepresentation.has_value()) {
        return LineAwareOptimizationResult(assignmentStatement);
    }

    if (canOptimizeAssignStatement(*usedAssignmentOperation, *postOrderRepresentation)) {
        // TODO: Add 'simple' base optimization algorithm ???
        if (postOrderRepresentation->areOperandsOnlyAdditionSubtractionOrXor()) {
            const auto& optimizedAssignmentStatements = optimizeComplexAssignStatement(*usedAssignmentOperation, assignmentStatement->lhs, *postOrderRepresentation);
            return LineAwareOptimizationResult(optimizedAssignmentStatements);
            //return optimizeAssignStatementWithOnlyAdditionSubtractionOrXorOperands(*usedAssignmentOperation, assignmentStatement->lhs, postOrderRepresentation);
        }
        return optimizeAssignStatementWithRhsContainingOperationsWithoutAssignEquivalent(assignmentStatement, *postOrderRepresentation);
    }
    return LineAwareOptimizationResult(assignmentStatement);
}

std::optional<syrec_operation::operation> LineAwareOptimization::tryMapAssignmentOperand(unsigned assignmentOperand) {
    switch (assignmentOperand) {
        case syrec::AssignStatement::Add:
            return std::make_optional(syrec_operation::operation::AddAssign);
        case syrec::AssignStatement::Subtract:
            return std::make_optional(syrec_operation::operation::MinusAssign);
        case syrec::AssignStatement::Exor: 
            return std::make_optional(syrec_operation::operation::XorAssign);
    }
    return std::nullopt;
}

std::optional<LineAwareOptimization::PostOrderTreeTraversal> LineAwareOptimization::createPostOrderRepresentation(const syrec::BinaryExpression::ptr& topLevelBinaryExpr) {
    std::vector<TreeTraversalNode> postOrderNodeTraversalStorage;
    bool                           wasTraversalOk = true;
    traverseExpressionOperand(topLevelBinaryExpr, postOrderNodeTraversalStorage, wasTraversalOk);
    if (!wasTraversalOk) {
        return std::nullopt;
    }
    return PostOrderTreeTraversal(postOrderNodeTraversalStorage);
}

// TODO:
void LineAwareOptimization::traverseExpressionOperand(const syrec::Expression::ptr& expr, std::vector<TreeTraversalNode>& postOrderTraversalContainer, bool& canContinueTraversal) {
    if (isExpressionOperandLeafNode(expr)) {
        if (const auto& exprAsVariableAccess = std::dynamic_pointer_cast<syrec::VariableExpression>(expr); exprAsVariableAccess != nullptr) {
            postOrderTraversalContainer.emplace_back(TreeTraversalNode::CreateLeafNode(exprAsVariableAccess));
        } else if (const auto& numericExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(expr); numericExpr != nullptr) {
            postOrderTraversalContainer.emplace_back(TreeTraversalNode::CreateLeafNode(numericExpr));
        }
    }
    else {
        TreeTraversalNode::ReferenceExpr referenceExprVariant;
        syrec::Expression::ptr lhsExprOfCurrentExpr;
        syrec::Expression::ptr rhsExprOfCurrentExpr;
        syrec_operation::operation operationOfOperationNode;
        if (const auto& exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr); exprAsBinaryExpr != nullptr) {
            referenceExprVariant     = exprAsBinaryExpr;
            lhsExprOfCurrentExpr = exprAsBinaryExpr->lhs;
            rhsExprOfCurrentExpr = exprAsBinaryExpr->rhs;
            operationOfOperationNode = *syrec_operation::tryMapBinaryOperationFlagToEnum(exprAsBinaryExpr->op);
        }
        else {
            const auto& exprAsShiftExpr = std::dynamic_pointer_cast<syrec::ShiftExpression>(expr);
            referenceExprVariant        = exprAsShiftExpr;
            lhsExprOfCurrentExpr        = exprAsShiftExpr->lhs;
            rhsExprOfCurrentExpr        = std::make_shared<syrec::NumericExpression>(exprAsShiftExpr->rhs, exprAsShiftExpr->bitwidth());
            operationOfOperationNode    = *syrec_operation::tryMapBinaryOperationFlagToEnum(exprAsShiftExpr->op);
        }

        /*
         * Since this optimization often starts at the deepest operation node while processing the expression tree in post order,
         * we flip the operands of binary expressions of the form constant op BinaryExpr where op is commutative
         */
        const auto& lhsAsConstantExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(lhsExprOfCurrentExpr);
        const auto isRhsExprEitherBinaryOrShiftExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(rhsExprOfCurrentExpr) != nullptr
            || std::dynamic_pointer_cast<syrec::ShiftExpression>(rhsExprOfCurrentExpr) != nullptr;
        canContinueTraversal                         = !(lhsAsConstantExpr != nullptr && isRhsExprEitherBinaryOrShiftExpr);
        
        if (!canContinueTraversal) {
            return;
        }

        if (lhsAsConstantExpr != nullptr && isRhsExprEitherBinaryOrShiftExpr && (syrec_operation::isCommutative(operationOfOperationNode) || syrec_operation::invert(operationOfOperationNode).has_value())) {
            if (!syrec_operation::isCommutative(operationOfOperationNode)) {
                operationOfOperationNode = *syrec_operation::invert(operationOfOperationNode);
            }
            traverseExpressionOperand(rhsExprOfCurrentExpr, postOrderTraversalContainer, canContinueTraversal);
            traverseExpressionOperand(lhsExprOfCurrentExpr, postOrderTraversalContainer, canContinueTraversal);
        }
        else {
            traverseExpressionOperand(lhsExprOfCurrentExpr, postOrderTraversalContainer, canContinueTraversal);
            traverseExpressionOperand(rhsExprOfCurrentExpr, postOrderTraversalContainer, canContinueTraversal);            
        }

        postOrderTraversalContainer.emplace_back(TreeTraversalNode::CreateInternalNode(TreeTraversalNode::InternalNodeData(referenceExprVariant, operationOfOperationNode)));   
    }
}

bool LineAwareOptimization::isExpressionOperandLeafNode(const syrec::Expression::ptr& expr) {
    if (const auto& exprAsVariableAccess = std::dynamic_pointer_cast<syrec::VariableExpression>(expr); exprAsVariableAccess != nullptr) {
        return true;
    }
    if (const auto& numericExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(expr); numericExpr != nullptr) {
        return true;
    }
    return false;
}

bool LineAwareOptimization::isOperationAdditionSubtractionOrXor(syrec_operation::operation operation) {
    switch (operation) {
        case syrec_operation::operation::Addition:
        case syrec_operation::operation::Subtraction:
        case syrec_operation::operation::BitwiseXor:
            return true;
        default:
            return false;
    }
}

bool LineAwareOptimization::isOperationAssociative(syrec_operation::operation operation) {
    switch (operation) {
        case syrec_operation::operation::Addition:
        case syrec_operation::operation::BitwiseXor:
        case syrec_operation::operation::BitwiseAnd:
        case syrec_operation::operation::BitwiseOr:
        case syrec_operation::operation::Multiplication:
        // TODO:
        //case syrec_operation::operation::upper_bits_multiplication:
        case syrec_operation::operation::Equals:
        case syrec_operation::operation::NotEquals:
        case syrec_operation::operation::LogicalAnd:
        case syrec_operation::operation::LogicalOr:
            return true;
        default:
            return false;
    }
}

bool LineAwareOptimization::canOptimizeAssignStatement(syrec_operation::operation assignmentOperand, const PostOrderTreeTraversal& postOrderTraversalContainer) {
    bool canOptimize = assignmentOperand != syrec_operation::operation::Addition
        ? postOrderTraversalContainer.doOperandsOperateOnLeafNodesOnly({syrec_operation::operation::Subtraction, syrec_operation::operation::BitwiseXor})
        : true;

    if (!postOrderTraversalContainer.areOperandsOnlyAdditionSubtractionOrXor()) {
        canOptimize &= postOrderTraversalContainer.doTwoConsecutiveOperationNodesHaveNotAdditionSubtractionOrXorAsOperand();
    }
    return canOptimize;
    
}

syrec::AssignStatement::vec LineAwareOptimization::optimizeAssignStatementWithOnlyAdditionSubtractionOrXorOperands(syrec_operation::operation assignmentOperand, const syrec::VariableAccess::ptr& assignStmtLhs, const PostOrderTreeTraversal& postOrderTraversalContainer) {
    syrec::AssignStatement::vec createdAssignStatements;

    std::set<std::size_t> visitedStatusPerNode;
    std::map<std::size_t, syrec::Expression::ptr> exprPerInternalNodeLookup;
    /*
     * The deepest node in the post order traversal is also the first node in our container storing the elements in post order
     */
    const auto traversalIdxOfDeepestOperationNode = *postOrderTraversalContainer.getTraversalIdxOfParentOfNode(0);
    const auto deepestOperationNode               = *postOrderTraversalContainer.getNode(traversalIdxOfDeepestOperationNode);
    const auto& traversalIndizesOfChildNodesOfDeepestOperationNode   = *postOrderTraversalContainer.getChildNodesForOperationNode(traversalIdxOfDeepestOperationNode);
    const std::pair childNodesOfDeepestOperationNode                   = std::make_pair(
      *postOrderTraversalContainer.getNode(traversalIndizesOfChildNodesOfDeepestOperationNode.first),
      *postOrderTraversalContainer.getNode(traversalIndizesOfChildNodesOfDeepestOperationNode.second)
    );

    const auto binaryExprOfInitialAssignment = std::make_shared<syrec::BinaryExpression>(
            *childNodesOfDeepestOperationNode.first.fetchStoredExpr(),
            *syrec_operation::tryMapBinaryOperationEnumToFlag(*deepestOperationNode.fetchStoredOperation()),
            *childNodesOfDeepestOperationNode.second.fetchStoredExpr());

    const auto initialAssignmentFromDeepestBinaryExpr = std::make_shared<syrec::AssignStatement>(
        assignStmtLhs,
        *syrec_operation::tryMapAssignmentOperationEnumToFlag(assignmentOperand),
        binaryExprOfInitialAssignment);
    
    createdAssignStatements.emplace_back(initialAssignmentFromDeepestBinaryExpr);

    for (const auto& traversalIdx : postOrderTraversalContainer.getTraversalIndexOfAllNodes()) {
        const auto& parentOperationNodeTraversalIdx = postOrderTraversalContainer.getTraversalIdxOfParentOfNode(traversalIdx);
        if (visitedStatusPerNode.count(traversalIdx) != 0 || !parentOperationNodeTraversalIdx.has_value()) {
            continue;
        }

        const auto childNodeIndizes = *postOrderTraversalContainer.getChildNodesForOperationNode(*parentOperationNodeTraversalIdx);
        const std::pair childNodesOfOperationNode = std::make_pair(*postOrderTraversalContainer.getNode(childNodeIndizes.first), *postOrderTraversalContainer.getNode(childNodeIndizes.second));

        const bool needToInvertAssignmentOperation = assignmentOperand == syrec_operation::operation::MinusAssign;
        syrec_operation::operation assignmentOperandToUse          = assignmentOperand;
        syrec::Expression::ptr     rhsExprOfAssignment;

        if (!childNodesOfOperationNode.first.isInternalNode || !childNodesOfOperationNode.second.isInternalNode) {
            const auto& childLeafNode = !childNodesOfOperationNode.first.isInternalNode
                ? childNodesOfOperationNode.first
                : childNodesOfOperationNode.second;

            assignmentOperandToUse = *postOrderTraversalContainer.getNode(*parentOperationNodeTraversalIdx)->fetchStoredOperation();
            rhsExprOfAssignment    = *childLeafNode.fetchStoredExpr();

            // TODO:
            visitedStatusPerNode.insert(childNodeIndizes.first);
        }
        else {
            const auto& grandParentOfCurrentNodeTraversalIdx = *postOrderTraversalContainer.getTraversalIdxOfParentOfNode(*parentOperationNodeTraversalIdx);
            const auto  binaryExprOperand                    = *postOrderTraversalContainer.getNode(grandParentOfCurrentNodeTraversalIdx)->fetchStoredOperation();

            const auto lhsOperandOfBinaryExpr = exprPerInternalNodeLookup.count(childNodeIndizes.first) != 0
                ? exprPerInternalNodeLookup[childNodeIndizes.first]
                : *childNodesOfOperationNode.first.fetchStoredExpr();

            const auto rhsOperandOfBinaryExpr = exprPerInternalNodeLookup.count(childNodeIndizes.second) != 0
                ? exprPerInternalNodeLookup[childNodeIndizes.second]
                : *childNodesOfOperationNode.second.fetchStoredExpr();

            const auto binaryExpr = std::make_shared<syrec::BinaryExpression>(
                    lhsOperandOfBinaryExpr,
                    *syrec_operation::tryMapBinaryOperationEnumToFlag(binaryExprOperand),
                    rhsOperandOfBinaryExpr
            );

            rhsExprOfAssignment = binaryExpr;
            visitedStatusPerNode.insert(childNodeIndizes.first);
            visitedStatusPerNode.insert(childNodeIndizes.first);
        }

        if (needToInvertAssignmentOperation) {
            assignmentOperandToUse = *syrec_operation::invert(assignmentOperandToUse);
        }
        const auto createdAssignmentStatement = std::make_shared<syrec::AssignStatement>(
                assignStmtLhs,
                *syrec_operation::tryMapAssignmentOperationEnumToFlag(assignmentOperandToUse),
                rhsExprOfAssignment);

        visitedStatusPerNode.insert(*parentOperationNodeTraversalIdx);
        exprPerInternalNodeLookup.insert({*parentOperationNodeTraversalIdx, rhsExprOfAssignment});
    }

    return createdAssignStatements;
}

syrec::AssignStatement::vec LineAwareOptimization::optimizeComplexAssignStatement(syrec_operation::operation assignmentOperand, const syrec::VariableAccess::ptr& assignStmtLhs, const PostOrderTreeTraversal& postOrderTraversalContainer) {
    const auto& operationNodesInPostOrder = postOrderTraversalContainer.getTraversalIndexOfAllOperationNodes();

    const auto& rootOperationNode = std::find_if(
    operationNodesInPostOrder.cbegin(),
    operationNodesInPostOrder.cend(),
    [&postOrderTraversalContainer](const std::size_t operationNodeTraversalIdx) {
        return postOrderTraversalContainer.getParentOfNode(operationNodeTraversalIdx).has_value();
    });

    /*
     * We can invert a Subtraction to an Addition by using the inverted inputs of the Subtraction (i.e. (a - b) => (~a + ~b) ???)
     * Since our optimizations only work when the Subtraction and xor operation operate on leaf nodes only, we need to invert all other operation nodes with non-leaf nodes that use such operations
     * Thus we need to identify which internal operation nodes use the Subtraction operation and need to be inverted (i.e. x += (a - (b - (d - e))) => x+= (~a + (~b + (~d + ~e)))
     */
    std::set<std::size_t> operationNodesToInvert;
    
    syrec::AssignStatement::vec generatedAssignmentStatement;
    for (std::size_t i = 0; i < operationNodesInPostOrder.size(); ++i) {
        const auto& operationNodeTraversalIdx = operationNodesInPostOrder.at(i);

        const auto& operationNode             = *postOrderTraversalContainer.getNode(operationNodeTraversalIdx);
        const auto& childNodeTraversalIndizes = *postOrderTraversalContainer.getChildNodesForOperationNode(operationNodeTraversalIdx);
        const auto& childNodes = std::pair(*postOrderTraversalContainer.getNode(childNodeTraversalIndizes.first), *postOrderTraversalContainer.getNode(childNodeTraversalIndizes.second));

        const bool hasAnyLeafNodesAsChildren = !childNodes.first.isInternalNode || !childNodes.second.isInternalNode;
        const bool isLeftChildALeaf          = hasAnyLeafNodesAsChildren && !childNodes.first.isInternalNode;
        const bool isRightChildALeaf         = hasAnyLeafNodesAsChildren && !childNodes.second.isInternalNode;

        const auto                 operationOfCurrentOperationNode = *operationNode.fetchStoredOperation();
        const bool                 useInvertedAssignmentOperation = assignmentOperand == syrec_operation::operation::MinusAssign;
        syrec_operation::operation assignmentOperationToUse       = useInvertedAssignmentOperation ? *syrec_operation::invert(assignmentOperand) : assignmentOperand;
        
        if (!hasAnyLeafNodesAsChildren) {   
            continue;
        }

        syrec::Expression::ptr     assignmentStmtRhsOperand;
        syrec::VariableAccess::ptr assignmentStmtLhsOperand = assignStmtLhs;

        if (isLeftChildALeaf ^ isRightChildALeaf) {
            const auto& leafChildNode       = isLeftChildALeaf ? childNodes.first : childNodes.second;
            assignmentOperationToUse = useInvertedAssignmentOperation ? *syrec_operation::invert(operationOfCurrentOperationNode) : operationOfCurrentOperationNode;
            /*
             * Created assignment statement is: lhs a_op leaf_child (if a_op in { +=, ^= }
             * Otherwise: lhs a_op^(-1) leaf_child
             */
            assignmentStmtRhsOperand = *leafChildNode.fetchStoredExpr();
        } else {
            /*
             * The assignment statement for the operation node first in the post order traversal order uses the assignment operation of the original assignment statement
             */
            if (i == 0) {
                assignmentOperationToUse = assignmentOperand;
                assignmentStmtRhsOperand = std::make_shared<syrec::BinaryExpression>(
                        *childNodes.first.fetchStoredExpr(),
                        *syrec_operation::tryMapBinaryOperationEnumToFlag(*operationNode.fetchStoredOperation()),
                        *childNodes.second.fetchStoredExpr());
            } else {
                const auto& traversalIndexOfParentOfCurrentOperationNode = *postOrderTraversalContainer.getTraversalIdxOfParentOfNode(operationNodeTraversalIdx);
                const auto  parentNodeOfOperationNode                    = *postOrderTraversalContainer.getNode(traversalIndexOfParentOfCurrentOperationNode);
                const auto  operationOfParentNode                        = *parentNodeOfOperationNode.fetchStoredOperation();

                /*
                * Assuming op_top is the operation of the parent operation node of the current operation node,
                * the created assignment statement is lhs op_top lhs_rhs_expr op_rhs rhs_rhs_expr if (a_op in { +=, ^= }
                * Otherwise: lhs op_top^(-1) lhs_rhs_expr op_rhs rhs_rhs_expr
                */
                assignmentOperationToUse = useInvertedAssignmentOperation ? *syrec_operation::invert(operationOfParentNode) : operationOfParentNode;
                assignmentStmtRhsOperand = std::make_shared<syrec::BinaryExpression>(
                        *childNodes.first.fetchStoredExpr(),
                        *syrec_operation::tryMapBinaryOperationEnumToFlag(*operationNode.fetchStoredOperation()),
                        *childNodes.second.fetchStoredExpr());
            }
        }

        const auto createdAssignmentStatement = std::make_shared<syrec::AssignStatement>(
                assignmentStmtLhsOperand,
                *syrec_operation::tryMapAssignmentOperationEnumToFlag(assignmentOperationToUse),
                assignmentStmtRhsOperand);

        const auto potentiallySplitAssignmentStatement = splitAssignmentStatementIfAssignmentAndRhsExpressionOperationMatch(createdAssignmentStatement);
        for (const auto& finalAssignmentStatement : potentiallySplitAssignmentStatement) {
            generatedAssignmentStatement.emplace_back(finalAssignmentStatement);
        }
    }
    
    return generatedAssignmentStatement;
}

LineAwareOptimization::LineAwareOptimizationResult LineAwareOptimization::optimizeAssignStatementWithRhsContainingOperationsWithoutAssignEquivalent(const std::shared_ptr<syrec::AssignStatement>& assignStmt, const PostOrderTreeTraversal& postOrderTraversalContainer) {
    const auto                        assignmentOperand = *syrec_operation::tryMapAssignmentOperationFlagToEnum(assignStmt->op);
    const syrec::VariableAccess::ptr& assignStmtLhs     = assignStmt->lhs;

    const auto& operationNodesInPostOrder = postOrderTraversalContainer.getTraversalIndexOfAllOperationNodes();

    const auto& rootOperationNode = std::find_if(
            operationNodesInPostOrder.cbegin(),
            operationNodesInPostOrder.cend(),
            [&postOrderTraversalContainer](const std::size_t operationNodeTraversalIdx) {
                return postOrderTraversalContainer.getParentOfNode(operationNodeTraversalIdx).has_value();
            });

    /*
     * We can invert a Subtraction to an Addition by using the inverted inputs of the Subtraction (i.e. (a - b) => (~a + ~b) ???)
     * Since our optimizations only work when the Subtraction and xor operation operate on leaf nodes only, we need to invert all other operation nodes with non-leaf nodes that use such operations
     * Thus we need to identify which internal operation nodes use the Subtraction operation and need to be inverted (i.e. x += (a - (b - (d - e))) => x+= (~a + (~b + (~d + ~e)))
     */
    std::set<std::size_t> operationNodesToInvert;

    std::set<std::size_t>       visitedOperationNodes;
    syrec::AssignStatement::vec generatedAssignmentStatement;
    syrec::AssignStatement::vec assignmentStatementsToRevert;
    std::map<std::size_t, std::size_t> lookupOfRevertStatementIfAssignmentNeedsToBeReverted;

    for (std::size_t i = 0; i < operationNodesInPostOrder.size(); ++i) {
        bool        doesAssignmentStatementNeedToBeReset = false;
        const auto& operationNodeTraversalIdx            = operationNodesInPostOrder.at(i);
        if (visitedOperationNodes.count(operationNodeTraversalIdx) != 0) {
            continue;
        }
        visitedOperationNodes.insert(operationNodeTraversalIdx);

        const auto& operationNode             = *postOrderTraversalContainer.getNode(operationNodeTraversalIdx);
        const auto& childNodeTraversalIndizes = *postOrderTraversalContainer.getChildNodesForOperationNode(operationNodeTraversalIdx);
        const auto& childNodes                = std::pair(*postOrderTraversalContainer.getNode(childNodeTraversalIndizes.first), *postOrderTraversalContainer.getNode(childNodeTraversalIndizes.second));

        const bool hasAnyLeafNodesAsChildren = !childNodes.first.isInternalNode || !childNodes.second.isInternalNode;
        const bool isLeftChildALeaf          = hasAnyLeafNodesAsChildren && !childNodes.first.isInternalNode;
        const bool isRightChildALeaf         = hasAnyLeafNodesAsChildren && !childNodes.second.isInternalNode;

        const auto                 operationOfCurrentOperationNode = *operationNode.fetchStoredOperation();
        const bool                 useInvertedAssignmentOperation  = assignmentOperand == syrec_operation::operation::MinusAssign;
        syrec_operation::operation assignmentOperationToUse        = useInvertedAssignmentOperation ? *syrec_operation::invert(assignmentOperand) : assignmentOperand;

        /*
         * e += (a / ((c * d) + b))
         *
         * b += (c * d)
         * e += (a / b)
         * b -= (c * d)
         */
        if (!isOperationAdditionSubtractionOrXor(operationOfCurrentOperationNode)) {
            if (i == 0) {
                assignmentOperationToUse = assignmentOperand;
            } else {
                /*
                 * This call crashes if parent node is traversed (see test other)
                 */
                const auto& traversalIndexOfParentOfCurrentOperationNode = *postOrderTraversalContainer.getTraversalIdxOfParentOfNode(operationNodeTraversalIdx);
                const auto  parentNodeOfOperationNode                    = *postOrderTraversalContainer.getNode(traversalIndexOfParentOfCurrentOperationNode);
                const auto  operationOfParentNode                        = *parentNodeOfOperationNode.fetchStoredOperation();
                assignmentOperationToUse                                 = useInvertedAssignmentOperation ? *syrec_operation::invert(operationOfParentNode) : operationOfParentNode;
            }

            const auto& referenceExpr = *operationNode.fetchReferenceExpr();
            syrec::Expression::ptr lhsExpr;
            syrec::Expression::ptr rhsExpr;
            if (std::holds_alternative<std::shared_ptr<syrec::BinaryExpression>>(referenceExpr)) {
                const auto& referenceExprAsBinaryOne = std::get<std::shared_ptr<syrec::BinaryExpression>>(referenceExpr);
                lhsExpr                              = referenceExprAsBinaryOne->lhs;
                rhsExpr                              = referenceExprAsBinaryOne->rhs;
            } else {
                const auto& referenceExprAsShiftExpr = std::get<std::shared_ptr<syrec::ShiftExpression>>(referenceExpr);
                lhsExpr                              = referenceExprAsShiftExpr->lhs;
                rhsExpr                              = std::make_shared<syrec::NumericExpression>(referenceExprAsShiftExpr->rhs, referenceExprAsShiftExpr->bitwidth());
            }

            assignmentOperationToUse            = operationOfCurrentOperationNode;
            const auto assignmentStmtRhsOperand = std::make_shared<syrec::BinaryExpression>(
                    lhsExpr,
                    *syrec_operation::tryMapBinaryOperationEnumToFlag(operationOfCurrentOperationNode),
                    rhsExpr);
            const auto& assignmentStmt = std::make_shared<syrec::AssignStatement>(
                    assignStmtLhs,
                    *syrec_operation::tryMapAssignmentOperationEnumToFlag(assignmentOperand),
                    assignmentStmtRhsOperand);
            generatedAssignmentStatement.emplace_back(assignmentStmt);
            continue;
        }

        if (!hasAnyLeafNodesAsChildren) {
            continue;
        }

        syrec::Expression::ptr     assignmentStmtRhsOperand;
        syrec::VariableAccess::ptr assignmentStmtLhsOperand = assignStmtLhs;

        if (isLeftChildALeaf ^ isRightChildALeaf) {
            const auto& leafChildNode = isLeftChildALeaf ? childNodes.first : childNodes.second;
            assignmentOperationToUse  = useInvertedAssignmentOperation ? *syrec_operation::invert(operationOfCurrentOperationNode) : operationOfCurrentOperationNode;
            /*
             * Created assignment statement is: lhs a_op leaf_child (if a_op in { +=, ^= }
             * Otherwise: lhs a_op^(-1) leaf_child
             */
            assignmentStmtRhsOperand = *leafChildNode.fetchStoredExpr();
        } else {
            /*
             * The assignment statement for the operation node first in the post order traversal order uses the assignment operation of the original assignment statement
             */
            if (i == 0) {
                assignmentOperationToUse = assignmentOperand;
                assignmentStmtRhsOperand = std::make_shared<syrec::BinaryExpression>(
                        *childNodes.first.fetchStoredExpr(),
                        *syrec_operation::tryMapBinaryOperationEnumToFlag(*operationNode.fetchStoredOperation()),
                        *childNodes.second.fetchStoredExpr());
            } else {
                if (isOperationAdditionSubtractionOrXor(operationOfCurrentOperationNode)) {
                    assignmentOperationToUse                        = useInvertedAssignmentOperation ? *syrec_operation::invert(operationOfCurrentOperationNode) : operationOfCurrentOperationNode;
                    const auto& lhsOperandOfRhsExprAsVariableAccess = std::dynamic_pointer_cast<syrec::VariableExpression>(*childNodes.first.fetchStoredExpr());

                    /*
                     * If the current operation node has to leaf nodes as children and the operation op is either {+=, -= or ^= }, we add the assignment statement e_left op e_right
                     * Additionally, this generated assignment statement needs to be reverted after the complete initial assignment was calculated to revert the made changes to the values of the operands
                     */
                    assignmentStmtLhsOperand             = lhsOperandOfRhsExprAsVariableAccess->var;
                    assignmentStmtRhsOperand             = *childNodes.second.fetchStoredExpr();
                    doesAssignmentStatementNeedToBeReset = true;
                } else {
                    const auto& traversalIndexOfParentOfCurrentOperationNode = *postOrderTraversalContainer.getTraversalIdxOfParentOfNode(operationNodeTraversalIdx);
                    const auto  parentNodeOfOperationNode                    = *postOrderTraversalContainer.getNode(traversalIndexOfParentOfCurrentOperationNode);
                    const auto  operationOfParentNode                        = *parentNodeOfOperationNode.fetchStoredOperation();

                    /*
                    * Assuming op_top is the operation of the parent operation node of the current operation node,
                    * the created assignment statement is lhs op_top lhs_rhs_expr op_rhs rhs_rhs_expr if (a_op in { +=, ^= }
                    * Otherwise: lhs op_top^(-1) lhs_rhs_expr op_rhs rhs_rhs_expr
                    */
                    assignmentOperationToUse = useInvertedAssignmentOperation ? *syrec_operation::invert(operationOfParentNode) : operationOfParentNode;
                    assignmentStmtRhsOperand = std::make_shared<syrec::BinaryExpression>(
                            *childNodes.first.fetchStoredExpr(),
                            *syrec_operation::tryMapBinaryOperationEnumToFlag(*operationNode.fetchStoredOperation()),
                            *childNodes.second.fetchStoredExpr());
                }
            }
        }

        const auto createdAssignmentStatement = std::make_shared<syrec::AssignStatement>(
                assignmentStmtLhsOperand,
                *syrec_operation::tryMapAssignmentOperationEnumToFlag(assignmentOperationToUse),
                assignmentStmtRhsOperand);

        const auto potentiallySplitAssignmentStatement = splitAssignmentStatementIfAssignmentAndRhsExpressionOperationMatch(createdAssignmentStatement);
        for (const auto& finalAssignmentStatement: potentiallySplitAssignmentStatement) {
            generatedAssignmentStatement.emplace_back(finalAssignmentStatement);
            if (doesAssignmentStatementNeedToBeReset) {
                const auto& assignmentStmtToCheck       = std::dynamic_pointer_cast<syrec::AssignStatement>(finalAssignmentStatement);
                const auto  invertedAssignmentOperation = *syrec_operation::invert(*tryMapAssignmentOperand(assignmentStmtToCheck->op));
                const auto  invertedAssignmentStmt      = std::make_shared<syrec::AssignStatement>(
                        assignmentStmtToCheck->lhs,
                        *syrec_operation::tryMapAssignmentOperationEnumToFlag(invertedAssignmentOperation),
                        assignmentStmtToCheck->rhs);
                assignmentStatementsToRevert.emplace_back(invertedAssignmentStmt);
                // Added index for revert statement is only relative and needs to be corrected after all assignment statements were added
                lookupOfRevertStatementIfAssignmentNeedsToBeReverted.insert({i, i});
            }
        }
    }

    if (!lookupOfRevertStatementIfAssignmentNeedsToBeReverted.empty()) {
        for (auto& lookupEntryKvPair : lookupOfRevertStatementIfAssignmentNeedsToBeReverted) {
            lookupEntryKvPair.second += generatedAssignmentStatement.size();
        }
    }

    for (const auto& invertedAssignmentStmt: assignmentStatementsToRevert) {
        generatedAssignmentStatement.emplace_back(invertedAssignmentStmt);
    }

    return LineAwareOptimizationResult(generatedAssignmentStatement, lookupOfRevertStatementIfAssignmentNeedsToBeReverted);
}


bool LineAwareOptimization::doAssignmentAndRhsExpressionOperationMatch(syrec_operation::operation assignmentOperation, syrec_operation::operation rhsOperation) {
    switch (assignmentOperation) {
        case syrec_operation::operation::AddAssign:
            return rhsOperation == syrec_operation::operation::Addition;
        case syrec_operation::operation::MinusAssign:
            return rhsOperation == syrec_operation::operation::Subtraction;
        case syrec_operation::operation::XorAssign:
            return rhsOperation == syrec_operation::operation::BitwiseXor;
        default:
            return false;
    }
}

syrec::AssignStatement::vec LineAwareOptimization::splitAssignmentStatementIfAssignmentAndRhsExpressionOperationMatch(const std::shared_ptr<syrec::AssignStatement>& assignmentStatement) {
    syrec::AssignStatement::vec createdAssignmentStatements;
    if (const auto& rhsAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(assignmentStatement->rhs); rhsAsBinaryExpr != nullptr) {
        const auto assignmentOperation = *tryMapAssignmentOperand(assignmentStatement->op);
        const auto rhsOperation        = *syrec_operation::tryMapBinaryOperationFlagToEnum(rhsAsBinaryExpr->op);
        if (doAssignmentAndRhsExpressionOperationMatch(assignmentOperation, rhsOperation)) {
            const auto assignmentStmtWithLhsOperandOfRhsExpr = std::make_shared<syrec::AssignStatement>(
                    assignmentStatement->lhs,
                    *syrec_operation::tryMapAssignmentOperationEnumToFlag(assignmentOperation),
                    rhsAsBinaryExpr->lhs);

            const auto assignmentStmtWithRhsOperandOfRhsExpr = std::make_shared<syrec::AssignStatement>(
                    assignmentStatement->lhs,
                    *syrec_operation::tryMapAssignmentOperationEnumToFlag(assignmentOperation),
                    rhsAsBinaryExpr->rhs);
            createdAssignmentStatements.emplace_back(assignmentStmtWithLhsOperandOfRhsExpr);
            createdAssignmentStatements.emplace_back(assignmentStmtWithRhsOperandOfRhsExpr);
        }
        else {
            createdAssignmentStatements.emplace_back(assignmentStatement);
        }

    } else {
        createdAssignmentStatements.emplace_back(assignmentStatement);
    }
    return createdAssignmentStatements;
}