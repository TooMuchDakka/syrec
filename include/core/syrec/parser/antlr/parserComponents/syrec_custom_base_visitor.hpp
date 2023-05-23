#ifndef SYREC_CUSTOM_BASE_VISITOR_HPP
#define SYREC_CUSTOM_BASE_VISITOR_HPP
#pragma once

#include "antlr4-runtime.h"
#include "SyReCBaseVisitor.h"
#include "core/syrec/parser/antlr/parserComponents/syrec_custom_base_visitor_shared_data.hpp"
#include "core/syrec/parser/expression_evaluation_result.hpp"
#include "core/syrec/parser/operation.hpp"
#include "core/syrec/parser/signal_access_restriction.hpp"

#include <optional>
#include <string>

namespace parser {
    class SyReCCustomBaseVisitor : public SyReCBaseVisitor {
    public:
        explicit SyReCCustomBaseVisitor(std::shared_ptr<SharedVisitorData> sharedVisitorData) :
            sharedData(std::move(sharedVisitorData))
        {}

    // TODO: We need one shared object holding all shared information (i.e. symbol table, etc.)
    protected:
        std::shared_ptr<SharedVisitorData> sharedData;

        static constexpr std::size_t fallbackErrorLinePosition   = 0;
        static constexpr std::size_t fallbackErrorColumnPosition = 0;

        struct TokenPosition {
            std::size_t line;
            std::size_t column;

            explicit TokenPosition(const std::size_t line, const std::size_t column):
                line(line), column(column) {
            }

            static TokenPosition createFallbackPosition() {
                return TokenPosition(fallbackErrorLinePosition, fallbackErrorColumnPosition);
            }
        };

        [[nodiscard]] static TokenPosition determineContextStartTokenPositionOrUseDefaultOne(const antlr4::ParserRuleContext* context);
        [[nodiscard]] static TokenPosition mapAntlrTokenPosition(const antlr4::Token* token);

        void                 createError(const TokenPosition& tokenPosition, const std::string& errorMessage);
        void                 createWarning(const TokenPosition& tokenPosition, const std::string& warningMessage);

        bool                                                    checkIfSignalWasDeclaredOrLogError(const std::string& signalIdent, const bool isLoopVariable, const TokenPosition& signalIdentTokenPosition);
        [[nodiscard]] bool isSignalAssignableOtherwiseCreateError(const antlr4::Token* signalIdentToken, const syrec::VariableAccess::ptr& assignedToVariable);
        [[nodiscard]] std::optional<syrec_operation::operation> getDefinedOperation(const antlr4::Token* definedOperationToken);
        bool                                                    checkIfNumberOfValuesPerDimensionMatchOrLogError(const TokenPosition& positionOfOptionalError, const std::vector<unsigned int>& lhsOperandNumValuesPerDimension, const std::vector<unsigned int>& rhsOperandNumValuesPerDimension);
        [[nodiscard]] std::optional<unsigned int>               tryDetermineBitwidthAfterVariableAccess(const syrec::VariableAccess::ptr& variableAccess, const TokenPosition& evaluationErrorPositionHelper);
        [[nodiscard]] std::optional<unsigned int>               tryDetermineExpressionBitwidth(const syrec::expression::ptr& expression, const TokenPosition& evaluationErrorPosition);

        [[nodiscard]] bool                                      canEvaluateNumber(const syrec::Number::ptr& number) const;
        [[nodiscard]] std::optional<unsigned int> tryEvaluateNumber(const syrec::Number::ptr& numberContainer, const TokenPosition& evaluationErrorPositionHelper);
        [[nodiscard]] std::optional<unsigned int> tryEvaluateCompileTimeExpression(const syrec::Number::CompileTimeConstantExpression& compileTimeExpression, const TokenPosition& evaluationErrorPositionHelper);
        [[nodiscard]] std::optional<unsigned int>            applyBinaryOperation(syrec_operation::operation operation, unsigned int leftOperand, unsigned int rightOperand, const TokenPosition& potentialErrorPosition);
        void                                                    insertSkipStatementIfStatementListIsEmpty(syrec::Statement::vec& statementList) const;

        std::optional<SignalAccessRestriction::SignalAccess> tryEvaluateBitOrRangeAccess(const std::pair<syrec::Number::ptr, syrec::Number::ptr>& accessedBits, const TokenPosition& optionalEvaluationErrorPosition);

        std::any visitSignal(SyReCParser::SignalContext* context) override;
        std::any visitNumberFromConstant(SyReCParser::NumberFromConstantContext* context) override;
        std::any visitNumberFromSignalwidth(SyReCParser::NumberFromSignalwidthContext* context) override;
        std::any visitNumberFromExpression(SyReCParser::NumberFromExpressionContext* context) override;
        std::any visitNumberFromLoopVariable(SyReCParser::NumberFromLoopVariableContext* context) override;

        /**
         * \brief Try to convert the return value of the given production to an instance of std::optional<T>.
         * \tparam T The expected type of the value wrapped inside of a std::optional returned by the production. IMPORTANT: This must be the exact type of the class, currently polymorphism does not work with std::any because of type erasure by std::any (see https://stackoverflow.com/questions/70313749/combining-static-cast-and-stdany-cast)
         * \param productionReturnType The expected type of the value wrapped inside of a std::optional returned by the production
         * \return A std::optional<T> object containing the casted return value of the production
         */
        template<typename T>
        [[nodiscard]] std::optional<T> tryConvertProductionReturnValue(std::any productionReturnType) const {
            if (!productionReturnType.has_value()) {
                return std::nullopt;
            }

            try {
                return std::any_cast<std::optional<T>>(productionReturnType);
            } catch (std::bad_any_cast& ex) {
                // TODO: Better error handling, i.e. logging or returning C-style error code to check whether cast or something else failed
                return std::nullopt;
            }
        }

        /**
         * \brief If the context is not null, try to call the corresponding visitor and try to convert the return value of the given production to an instance of std::optional<T>.
         * \tparam T The expected type of the value wrapped inside of a std::optional returned by the production. IMPORTANT: This must be the exact type of the class, currently polymorphism does not work with std::any because of type erasure by std::any (see https://stackoverflow.com/questions/70313749/combining-static-cast-and-stdany-cast)
         * \param production The expected type of the value wrapped inside of a std::optional returned by the production
         * \return A std::optional<T> object containing the casted return value of the production
         */
        template<typename T>
        [[nodiscard]] std::optional<T> tryVisitAndConvertProductionReturnValue(antlr4::tree::ParseTree* production) {
            if (production == nullptr) {
                return std::nullopt;
            }
            return tryConvertProductionReturnValue<T>(visit(production));
        }

        template<class InputIt, class OutputIt, class Pred, class Fct>
        static void transform_if(InputIt first, InputIt last, OutputIt dest, Pred pred, Fct transform) {
            while (first != last) {
                if (pred(*first))
                    *dest++ = transform(*first);

                ++first;
            }
        }
    private:
        [[nodiscard]] bool canEvaluateCompileTimeExpression(const syrec::Number::CompileTimeConstantExpression& compileTimeExpression) const;
        [[nodiscard]] bool validateSemanticChecksIfDimensionExpressionIsConstant(const antlr4::Token* dimensionToken, size_t accessedDimensionIdx, const syrec::Variable::ptr& accessedSignal, const std::optional<ExpressionEvaluationResult::ptr>& expressionEvaluationResult);
        [[nodiscard]] std::optional<std::pair<syrec::Number::ptr, syrec::Number::ptr>> isBitOrRangeAccessDefined(SyReCParser::NumberContext* bitRangeStartToken, SyReCParser::NumberContext* bitRangeEndToken);
        [[nodiscard]] bool                                                             validateBitOrRangeAccessOnSignal(const antlr4::Token* bitOrRangeStartToken, const syrec::Variable::ptr& accessedVariable, const std::pair<syrec::Number::ptr, syrec::Number::ptr>& bitOrRangeAccess);
        [[nodiscard]] bool                                                             isAccessToAccessedSignalPartRestricted(const syrec::VariableAccess::ptr& accessedSignalPart, const TokenPosition& optionalEvaluationErrorPosition) const;
    };
}
#endif