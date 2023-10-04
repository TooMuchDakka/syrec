#ifndef SYREC_EXPRESSION_VISITOR_HPP
#define SYREC_EXPRESSION_VISITOR_HPP
#pragma once

#include "core/syrec/parser/antlr/parserComponents/syrec_custom_base_visitor.hpp"

namespace parser {
    class SyReCExpressionVisitor: public SyReCCustomBaseVisitor {
    public:
        explicit SyReCExpressionVisitor(const SharedVisitorData::ptr& sharedVisitorData):
            SyReCCustomBaseVisitor(sharedVisitorData) {}

        /*
        * Expression production visitors
        */
        std::any visitExpressionFromNumber(SyReCParser::ExpressionFromNumberContext* context) override;
        std::any visitExpressionFromSignal(SyReCParser::ExpressionFromSignalContext* context) override;
        std::any visitBinaryExpression(SyReCParser::BinaryExpressionContext* context) override;
        std::any visitUnaryExpression(SyReCParser::UnaryExpressionContext* context) override;
        std::any visitShiftExpression(SyReCParser::ShiftExpressionContext* context) override;
    private:
        [[nodiscard]] static bool isValidBinaryOperation(syrec_operation::operation userDefinedOperation) noexcept;
    };
} // namespace parser
#endif