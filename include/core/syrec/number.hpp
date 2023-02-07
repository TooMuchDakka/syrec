#pragma once

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <variant>

template<class... Ts>
struct Overloaded: Ts... {
    using Ts::operator()...;
};
template<class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

namespace syrec {

    class Number {
    public:
        using ptr = std::shared_ptr<Number>;

        using loop_variable_mapping = std::map<std::string, unsigned int>;

        enum Operation {
            Addition,
            Subtraction,
            Multiplication,
            Division
        };

        struct CompileTimeConstantExpression {
            ptr lhsOperand;
            Operation operation;
            ptr     rhsOperand;

            explicit CompileTimeConstantExpression(const ptr& lhsOperand, Operation operation, const ptr& rhsOperand):
                lhsOperand(lhsOperand), operation(operation), rhsOperand(rhsOperand) {
            }

            // TODO: Handling of division by zero
            [[nodiscard]] unsigned evaluate(const loop_variable_mapping& loopVariableMapping) const {
                const unsigned int lhsOperandEvaluated = lhsOperand->evaluate(loopVariableMapping);
                const unsigned int rhsOperandEvaluated = rhsOperand->evaluate(loopVariableMapping);
                
                switch (operation) {
                    case Addition:
                         return lhsOperandEvaluated + rhsOperandEvaluated;
                    case Subtraction:
                        return lhsOperandEvaluated - rhsOperandEvaluated;
                    case Multiplication: 
                        return lhsOperandEvaluated * rhsOperandEvaluated;
                    default:
                        return lhsOperandEvaluated / rhsOperandEvaluated;
                }
            }
        };


        explicit Number(std::variant<unsigned, std::string, CompileTimeConstantExpression> number):
            numberVar(std::move(number)) {}

        explicit Number(unsigned value):
            numberVar(value) {
        }

        explicit Number(const std::string& value):
            numberVar(value) {
        }

        explicit Number(const CompileTimeConstantExpression& value):
            numberVar(value) {
        }

        ~Number() = default;

        [[nodiscard]] bool isLoopVariable() const {
            return std::holds_alternative<std::string>(numberVar);
        }

        [[nodiscard]] bool isConstant() const {
            return std::holds_alternative<unsigned>(numberVar);
        }

        [[nodiscard]] bool isCompileTimeConstantExpression() const {
            return std::holds_alternative<CompileTimeConstantExpression>(numberVar);
        }

        [[nodiscard]] const std::string& variableName() const {
            return std::get<std::string>(numberVar);
        }
        
        [[nodiscard]] CompileTimeConstantExpression getExpression() const {
            return std::get<CompileTimeConstantExpression>(numberVar);
        }

        [[nodiscard]] unsigned evaluate(const loop_variable_mapping& map) const {
            return std::visit(Overloaded{
                                      [](unsigned arg) { return arg; },
                                      [&map](const std::string& value) { return map.find(value)->second; },
                                      [&map](const CompileTimeConstantExpression& expr) { return expr.evaluate(map); }},
                              numberVar);
        }

    private:
        std::variant<unsigned, std::string, CompileTimeConstantExpression> numberVar;
    };

} // namespace syrec
