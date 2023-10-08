#ifndef SYREC_CUSTOM_BASE_VISITOR_HPP
#define SYREC_CUSTOM_BASE_VISITOR_HPP
#pragma once

#include "antlr4-runtime.h"
#include "SyReCBaseVisitor.h"
#include "core/syrec/parser/antlr/parserComponents/syrec_custom_base_visitor_shared_data.hpp"
#include "core/syrec/parser/expression_evaluation_result.hpp"
#include <core/syrec/parser/utils/message_utils.hpp>
#include "core/syrec/parser/operation.hpp"
#include "core/syrec/parser/value_broadcaster.hpp"

#include <optional>
#include <string>

namespace parser {
    class SyReCCustomBaseVisitor : public SyReCBaseVisitor {
    public:
        explicit SyReCCustomBaseVisitor(SharedVisitorData::ptr sharedVisitorData) :
            sharedData(std::move(sharedVisitorData))
        {}
        
    protected:
        SharedVisitorData::ptr sharedData;
        
        [[nodiscard]] static messageUtils::Message::Position determineContextStartTokenPositionOrUseDefaultOne(const antlr4::ParserRuleContext* context);
        [[nodiscard]] static messageUtils::Message::Position mapAntlrTokenPosition(const antlr4::Token* token);


        template<typename ...T>
        void createMessage(const messageUtils::Message::Position& position, messageUtils::Message::Severity severity, const std::string_view& formatString, T&&... args) {
            switch (severity) {
                case messageUtils::Message::Error:
                    sharedData->errors.emplace_back(sharedData->messageFactory.createMessage(messageUtils::Message::ErrorCategory::Semantic, position, severity, formatString, std::forward<T>(args)...));
                    break;
                case messageUtils::Message::Warning:
                    sharedData->warnings.emplace_back(sharedData->messageFactory.createMessage(messageUtils::Message::ErrorCategory::Semantic, position, severity, formatString, std::forward<T>(args)...));
                    break;
                default:
                    break;
            }
        }

        template<typename ...T>
        void createError(const messageUtils::Message::Position& position, const std::string_view& formatString, T&&... args) {
            createMessage(position, messageUtils::Message::Severity::Error, formatString, std::forward<T>(args)...);
        }

        template<typename ...T>
        void createMessageAtUnknownPosition(messageUtils::Message::Severity severity, const std::string& formatString, T&&... args) {
            createMessage(determineContextStartTokenPositionOrUseDefaultOne(nullptr), severity, formatString, std::forward<T>(args)...);
        }
        
        [[maybe_unused]] bool                                   checkIfSignalWasDeclaredOrLogError(const std::string_view& signalIdent, const messageUtils::Message::Position& signalIdentTokenPosition);
        [[nodiscard]] bool                                      isSignalAssignableOtherwiseCreateError(unsigned int signalType, const std::string_view& signalIdent, const messageUtils::Message::Position& signalIdentTokenPosition);
        [[nodiscard]] std::optional<syrec_operation::operation> getDefinedOperation(const antlr4::Token* definedOperationToken);
        [[nodiscard]] std::optional<unsigned int>               tryDetermineBitwidthAfterVariableAccess(const syrec::VariableAccess& variableAccess, const messageUtils::Message::Position* evaluationErrorPositionHelper);
        [[nodiscard]] std::optional<unsigned int>               tryDetermineExpressionBitwidth(const syrec::expression& expression, const messageUtils::Message::Position& evaluationErrorPosition);

        [[nodiscard]] bool                                      canEvaluateNumber(const syrec::Number& number) const;
        [[nodiscard]] std::optional<unsigned int>               tryEvaluateNumber(const syrec::Number& numberContainer, const messageUtils::Message::Position* evaluationErrorPositionHelper);
        [[nodiscard]] std::optional<unsigned int>               tryEvaluateCompileTimeExpression(const syrec::Number::CompileTimeConstantExpression& compileTimeExpression, const messageUtils::Message::Position* evaluationErrorPositionHelper);
        [[nodiscard]] std::optional<unsigned int>               applyBinaryOperation(syrec_operation::operation operation, unsigned int leftOperand, unsigned int rightOperand, const messageUtils::Message::Position& potentialErrorPosition);
        void                                                    insertSkipStatementIfStatementListIsEmpty(syrec::Statement::vec& statementList) const;
        
        [[nodiscard]] static std::optional<unsigned int> tryConvertTextToNumber(const std::string_view& text);
        [[nodiscard]] std::vector<std::optional<unsigned int>> determineAccessedValuePerDimensionFromSignalAccess(const syrec::VariableAccess& signalAccess);
        [[nodiscard]] static std::pair<std::vector<unsigned int>, std::size_t> determineNumAccessedValuesPerDimensionAndFirstNotExplicitlyAccessedDimensionIndex(const syrec::VariableAccess& signalAccess);
        [[nodiscard]] bool                                                     doOperandsRequireBroadcastingBasedOnDimensionAccessAndLogError(std::size_t numDeclaredDimensionsOfLhsOperand, std::size_t firstNoExplicitlyAccessedDimensionOfLhsOperand, const std::vector<unsigned int>& numAccessedValuesPerDimensionOfLhsOperand,
                                                                                                                       std::size_t numDeclaredDimensionsOfRhsOperand, std::size_t firstNoExplicitlyAccessedDimensionOfRhsOperand, const std::vector<unsigned int>& numAccessedValuesPerDimensionOfRhsOperand, const messageUtils::Message::Position* broadcastingErrorPosition, valueBroadcastCheck::DimensionAccessMissmatchResult* missmatchResult);

        [[nodiscard]] bool doOperandsRequiredBroadcastingBasedOnBitwidthAndLogError(std::size_t accessedBitRangeWidthOfLhsOperand, std::size_t accessedBitRangeWidthOfRhsOperand, const messageUtils::Message::Position* broadcastingErrorPosition);
        void fixExpectedBitwidthToValueIfLatterIsLargerThanCurrentOne(unsigned int potentialNewExpectedExpressionBitwidth) const;

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
            return production ? tryConvertProductionReturnValue<T>(visit(production)) : std::nullopt;
        }

        template<class InputIt, class OutputIt, class Pred, class Fct>
        static void transformAndFilter(InputIt first, InputIt last, OutputIt dest, Fct inputTransformer, Pred transformedResultPredicate) {
            for (auto iterator = first; iterator != last; ++iterator) {
                if (const auto& transformedEntry = inputTransformer(*iterator); transformedResultPredicate(transformedEntry)) {
                    *dest++ = *transformedEntry;
                }
            }
        }

    private:
        [[nodiscard]] bool                                                             wasSyntaxErrorDetectedBetweenTokens(const messageUtils::Message::Position& rangeOfInterestStartPosition, const messageUtils::Message::Position& rangeOfInterestEndPosition) const;
        [[nodiscard]] bool                                                             canEvaluateCompileTimeExpression(const syrec::Number::CompileTimeConstantExpression& compileTimeExpression) const;
        [[nodiscard]] std::optional<bool>                                              isAccessedValueOfDimensionWithinRange(const messageUtils::Message::Position& valueOfDimensionTokenPosition, size_t accessedDimensionIdx, const syrec::Variable::ptr& accessedSignal, const ExpressionEvaluationResult& expressionEvaluationResult);
        [[nodiscard]] std::optional<std::pair<syrec::Number::ptr, syrec::Number::ptr>> isBitOrRangeAccessDefined(SyReCParser::NumberContext* bitRangeStartToken, SyReCParser::NumberContext* bitRangeEndToken);
        [[nodiscard]] bool                                                             validateBitOrRangeAccessOnSignal(const antlr4::Token* bitOrRangeStartToken, const syrec::Variable::ptr& accessedVariable, const std::pair<syrec::Number::ptr, syrec::Number::ptr>& bitOrRangeAccess);
        [[nodiscard]] bool                                                             existsRestrictionForAccessedSignalParts(const syrec::VariableAccess& accessedSignalPart, const messageUtils::Message::Position* optionalEvaluationErrorPosition);
        [[nodiscard]] static std::string                                               stringifyCompileTimeConstantExpressionOperation(syrec::Number::CompileTimeConstantExpression::Operation ctcOperation);
        [[nodiscard]] static std::optional<syrec::BinaryExpression::ptr>               tryConvertCompileTimeConstantExpressionToBinaryExpr(const syrec::Number::CompileTimeConstantExpression& compileTimeConstantExpression, const parser::SymbolTable& symbolTable);
        [[nodiscard]] static std::optional<syrec::Number::ptr>                         tryConvertExpressionToCompileTimeConstantOne(const syrec::expression& expr);
        [[nodiscard]] static std::optional<syrec::expression::ptr>                     tryConvertNumberToExpr(const syrec::Number& number, const parser::SymbolTable& symbolTable);
        [[nodiscard]] static std::string                                               combineValuePerDimensionMissmatch(const std::vector<valueBroadcastCheck::AccessedValuesOfDimensionMissmatch>& valuePerNotExplicitlyAccessedDimensionMissmatch);
    };
}
#endif