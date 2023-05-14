#ifndef NO_ADDITIONAL_LINE_ASSIGNMENT_HPP
#define NO_ADDITIONAL_LINE_ASSIGNMENT_HPP
#pragma once

#include "core/syrec/statement.hpp"
#include "core/syrec/parser/operation.hpp"

#include <optional>
#include <set>

namespace optimizations {
	class LineAwareOptimization {
	public:
        struct LineAwareOptimizationResult {
            const syrec::AssignStatement::vec        statements;
            const std::map<std::size_t, std::size_t> revertStatementLookup;


            LineAwareOptimizationResult(const syrec::AssignStatement::ptr& referenceStmt):
                statements({referenceStmt}) {}

            LineAwareOptimizationResult(syrec::AssignStatement::vec statements):
                statements(std::move(statements)) {}

            LineAwareOptimizationResult(syrec::AssignStatement::vec statements, std::map<std::size_t, std::size_t> revertStatementLookup):
                statements(std::move(statements)), revertStatementLookup(std::move(revertStatementLookup)) {}
        };

        [[nodiscard]] static LineAwareOptimizationResult optimize(const std::shared_ptr<syrec::AssignStatement>& assignmentStatement);

	protected:
		struct TreeTraversalNode {
            const bool isInternalNode;
			using LeafData = std::variant<std::shared_ptr<syrec::VariableExpression>, std::shared_ptr<syrec::NumericExpression>>;
            using ReferenceExpr = std::variant<std::shared_ptr<syrec::BinaryExpression>, std::shared_ptr<syrec::ShiftExpression>>;
            using InternalNodeData = std::tuple<ReferenceExpr, syrec_operation::operation>;
            static TreeTraversalNode CreateInternalNode(const InternalNodeData& internalNodeData) {
                return TreeTraversalNode(true, internalNodeData);
            }

            static TreeTraversalNode CreateLeafNode(const LeafData& leafData) {
                return TreeTraversalNode(false, leafData);
            }

            [[nodiscard]] std::optional<syrec::expression::ptr>      fetchStoredExpr() const;
            [[nodiscard]] std::optional<ReferenceExpr> fetchReferenceExpr() const;
            [[nodiscard]] std::optional<syrec_operation::operation>  fetchStoredOperation() const;
		private:
            const std::variant<LeafData, InternalNodeData> nodeData;

			explicit TreeTraversalNode(bool isInternalNode, std::variant<LeafData, InternalNodeData> nodeData):
                isInternalNode(isInternalNode), nodeData(std::move(nodeData)) { }
		};

        struct ParentChildTreeTraversalNodeRelation {
            const std::size_t parentNodeTraversalIdx;
            const std::pair<std::size_t, std::size_t> childNodeTraversalIndizes;

            explicit ParentChildTreeTraversalNodeRelation(std::size_t parentNodeTraversalIdx, std::pair<std::size_t, std::size_t> childNodeTraversalIndizes):
             parentNodeTraversalIdx(parentNodeTraversalIdx), childNodeTraversalIndizes(std::move(childNodeTraversalIndizes)) { }
        };

		struct PostOrderTreeTraversal {
            explicit PostOrderTreeTraversal(std::vector<TreeTraversalNode> nodesInPostOrder):
		        treeNodesInPostOrder(std::move(nodesInPostOrder)) {
                determineParentChildRelations(treeNodesInPostOrder);
            }

            [[nodiscard]] std::vector<std::size_t>                           getTraversalIndexOfAllNodes() const;
            [[nodiscard]] std::vector<std::size_t>                           getTraversalIndexOfAllLeafNodes() const;
            [[nodiscard]] std::vector<std::size_t>                           getTraversalIndexOfAllOperationNodes() const;
            [[nodiscard]] std::optional<std::size_t>                         getTraversalIdxOfParentOfNode(std::size_t postOrderTraversalIdx) const;
            [[nodiscard]] std::optional<const TreeTraversalNode>             getParentOfNode(std::size_t postOrderTraversalIdx) const;
            [[nodiscard]] std::optional<const TreeTraversalNode>             getNode(std::size_t postOrderTraversalIdx) const;
            [[nodiscard]] std::optional<std::pair<std::size_t, std::size_t>> getChildNodesForOperationNode(std::size_t postOrderTraversalIdxOfOperationNode) const;

			[[nodiscard]] bool areOperandsOnlyAdditionSubtractionOrXor() const;
            [[nodiscard]] bool doOperandsOperateOnLeafNodesOnly(const std::vector<syrec_operation::operation>& operands) const;
            [[nodiscard]] bool doTwoConsecutiveOperationNodesHaveNotAdditionSubtractionOrXorAsOperand() const;
		private:
            std::vector<TreeTraversalNode>          treeNodesInPostOrder;
            std::map<std::size_t, ParentChildTreeTraversalNodeRelation> parentChildRelationLookup;
            std::vector<std::optional<std::size_t>> parentPerNodeLookup;

			void determineParentChildRelations(const std::vector<TreeTraversalNode>& nodesInPostOrder);
            [[nodiscard]] std::vector<std::size_t>                       getEitherInternalOrLeafNodes(bool getLeafNodes) const;
        };

        [[nodiscard]] static std::optional<syrec_operation::operation> tryMapAssignmentOperand(unsigned int assignmentOperand);
        [[nodiscard]] static std::optional<PostOrderTreeTraversal>     createPostOrderRepresentation(const syrec::BinaryExpression::ptr& topLevelBinaryExpr);
        static void                                                    traverseExpressionOperand(const syrec::expression::ptr& expr, std::vector<TreeTraversalNode>& postOrderTraversalContainer, bool& canContinueTraversal);
        [[nodiscard]] static bool                                      isExpressionOperandLeafNode(const syrec::expression::ptr& expr);
        [[nodiscard]] static bool                                      isOperationAdditionSubtractionOrXor(syrec_operation::operation operation);
        [[nodiscard]] static bool                                      isOperationAssociative(syrec_operation::operation operation);

		[[nodiscard]] static bool                        canOptimizeAssignStatement(syrec_operation::operation assignmentOperand, const PostOrderTreeTraversal& postOrderTraversalContainer);
        [[nodiscard]] static syrec::AssignStatement::vec optimizeAssignStatementWithOnlyAdditionSubtractionOrXorOperands(syrec_operation::operation assignmentOperand, const syrec::VariableAccess::ptr& assignStmtLhs, const PostOrderTreeTraversal& postOrderTraversalContainer);
        [[nodiscard]] static syrec::AssignStatement::vec optimizeComplexAssignStatement(syrec_operation::operation assignmentOperand, const syrec::VariableAccess::ptr& assignStmtLhs, const PostOrderTreeTraversal& postOrderTraversalContainer);
        [[nodiscard]] static LineAwareOptimization::LineAwareOptimizationResult optimizeAssignStatementWithRhsContainingOperationsWithoutAssignEquivalent(const std::shared_ptr<syrec::AssignStatement>& assignStmt, const PostOrderTreeTraversal& postOrderTraversalContainer);
        [[nodiscard]] static bool                        doAssignmentAndRhsExpressionOperationMatch(syrec_operation::operation assignmentOperation, syrec_operation::operation rhsOperation);
        
        /**
         * \brief If the assignment operation and the operation of the rhs expression match, we can create two assignment statements (using the same assignment operation) per operand of the latter. Otherwise, the assignment statement remains unchanged.
         * \n Example: lhs a_op (e_1 op e_2) = lhs a_op e_1; lhs a_op e_2 if (a_op == op).
         *
         * \param assignmentStatement The assignment statement to check
         * \return The created assignment statements
         */
        [[nodiscard]] static syrec::AssignStatement::vec splitAssignmentStatementIfAssignmentAndRhsExpressionOperationMatch(const std::shared_ptr<syrec::AssignStatement>& assignmentStatement);

        // TODO:
        struct OperationNodeInversionStatusBuilder {
        private:
            std::set<std::size_t> nodesToInvert;
        };
	};

};
#endif