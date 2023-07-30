#ifndef INORDER_OPERATION_NODE_TRAVERSAL_HELPER_HPP
#define INORDER_OPERATION_NODE_TRAVERSAL_HELPER_HPP
#include "core/syrec/expression.hpp"
#include "core/syrec/parser/operation.hpp"
#include "core/syrec/parser/symbol_table.hpp"

#include <memory>
#include <optional>
#include <queue>

namespace noAdditionalLineSynthesis {
    class InorderOperationNodeTraversalHelper {
    public:
        explicit InorderOperationNodeTraversalHelper(const syrec::expression::ptr& expr, parser::SymbolTable::ptr symbolTable):
            symbolTableReference(std::move(symbolTable))
        {
            buildOperationNodesQueue(expr);
        }

        struct TraversalNode {
            std::size_t nodeId;
            bool        isOperationNode;

            explicit TraversalNode(std::size_t nodeId, bool isOperationNode)
                : nodeId(nodeId), isOperationNode(isOperationNode) {}
        };

        class OperationNodeLeaf {
        public:
            std::size_t leafNodeId;

            explicit OperationNodeLeaf(std::size_t leafNodeId, std::variant<std::shared_ptr<syrec::NumericExpression>, std::shared_ptr<syrec::VariableExpression>> leafNodeData)
                : leafNodeId(leafNodeId), nodeVariantData(std::move(leafNodeData)) {}

            [[nodiscard]] std::optional<syrec::NumericExpression::ptr>  getAsNumber() const;
            [[nodiscard]] std::optional<syrec::VariableExpression::ptr> getAsSignalAccess() const;
        private:
            std::variant<std::shared_ptr<syrec::NumericExpression>, std::shared_ptr<syrec::VariableExpression>> nodeVariantData;
        };

        struct OperationNode {
            std::size_t                nodeId;
            syrec_operation::operation operation;
            TraversalNode              lhs;
            TraversalNode              rhs;

            explicit OperationNode(std::size_t nodeId, syrec_operation::operation operation, const TraversalNode lhs, const TraversalNode rhs):
                nodeId(nodeId), operation(operation), lhs(lhs), rhs(rhs) {}
        };

        using OperationNodeReference = std::shared_ptr<OperationNode>;
        using OperationNodeLeafReference = std::shared_ptr<OperationNodeLeaf>;

        std::optional<OperationNodeReference> getNextOperationNode();
    private:
        std::size_t                             operationNodeIdCounter = 0;
        std::size_t                             leafNodeIdCounter      = 0;

        std::map<std::size_t, OperationNodeReference> operationNodesLookup;
        std::map<std::size_t, OperationNodeLeafReference> leafNodesLookup;
        std::queue<std::size_t>                 operationNodeTraversalQueue;
        const parser::SymbolTable::ptr          symbolTableReference;

        void                                                                                               buildOperationNodesQueue(const syrec::expression::ptr& expr);
        [[maybe_unused]] TraversalNode                                                                     buildOperationNode(const syrec::expression::ptr& expr);
        [[nodiscard]] TraversalNode                                                                        buildOperationNode(const syrec::Number::ptr& number, unsigned int expectedBitwidthOfOperation);
        [[nodiscard]] std::optional<bool>                                                                  areAllOperandsOfOperationNodeLeafNodes(const OperationNodeReference& operationNode) const;
        [[nodiscard]] std::optional<std::pair<OperationNodeLeafReference, OperationNodeLeafReference>>     getLeafNodesOfOperationNode(const OperationNodeReference& operationNode) const;
        [[nodiscard]] std::optional<OperationNodeLeafReference>                                            getLeafNodeOfOperationNode(const OperationNodeReference& operationNode, bool fetchLeftOperand) const;
        [[nodiscard]] std::optional<OperationNodeReference>                                                getOperationNodeForId(std::size_t operationNodeId) const;
        [[nodiscard]] unsigned int                                                                         determineBitwidthOfExpr(const syrec::expression::ptr& expr) const;
        [[nodiscard]] unsigned int                                                                         determineBitwidthOfSignalAccess(const syrec::VariableAccess::ptr& signalAccess) const;

        [[nodiscard]] static std::optional<syrec_operation::operation> tryMapShiftOperationExprFlagToEnum(const std::shared_ptr<syrec::ShiftExpression>& shiftExpr);
        [[nodiscard]] static std::optional<syrec_operation::operation> tryMapBinaryExprOperationFlagToEnum(const std::shared_ptr<syrec::BinaryExpression>& binaryExpr);
        [[nodiscard]] static bool                                      doesExprDefineSignalAccess(const syrec::expression::ptr& expr);
    };
}
#endif