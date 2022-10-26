

#if !defined(parser_COCO_PARSER_H__)
#define parser_COCO_PARSER_H__

#include <fmt/format.h>
#include <optional>
#include <string>
#include <vector>

#include "core/syrec/module.hpp"
#include "core/syrec/parser/custom_semantic_errors.hpp"
#include "core/syrec/parser/expression_evaluation_result.hpp"
#include "core/syrec/parser/grammar_conflict_resolver.hpp"
#include "core/syrec/parser/method_call_guess.hpp"
#include "core/syrec/parser/operation.hpp"
#include "core/syrec/parser/range_check.hpp"
#include "core/syrec/parser/parser_config.hpp"
#include "core/syrec/parser/signal_evaluation_result.hpp"
#include "core/syrec/parser/symbol_table.hpp"
#include "core/syrec/parser/text_utils.hpp"


#include "core/syrec/parser/Scanner.h"

namespace parser {


class Errors {
public:
	int count;			// number of errors detected

	Errors();
	void SynErr(int line, int col, int n);
	void Error(int line, int col, const wchar_t *s);
	void Warning(int line, int col, const wchar_t *s);
	void Warning(const wchar_t *s);
	void Exception(const wchar_t *s);

}; // Errors

class Parser {
private:
	enum {
		_EOF=0,
		_ident=1,
		_int=2
	};
	int maxT;

	Token *dummyToken;
	int errDist;
	int minErrDist;

	void SynErr(int n);
	void Get();
	void Expect(int n);
	bool StartOf(int s);
	void ExpectWeak(int n, int follow);
	bool WeakSeparator(int n, int syFol, int repFol);

public:
	Scanner *scanner;
	Errors  *errors;

	Token *t;			// last recognized token
	Token *la;			// lookahead token

syrec::Module::vec modules;
	SymbolTable::ptr currSymTabScope;
	ParserConfig config;

	syrec::Number::loop_variable_mapping loop_variable_mapping_lookup;

	// This method will be called by the contructor if it exits.
	// This support is specific to the C++ version of Coco/R.
	void Init() {
		// nothing to do
		SymbolTable::openScope(currSymTabScope);
	}

	// Uncomment this method if cleanup is necessary,
	// this method will be called by the destructor if it exists.
	// This support is specific to the C++ version of Coco/R.
	// void Destroy() {
		// nothing to do
	// }

	[[nodiscard]] std::optional<unsigned int> convert_token_value_to_number(const Token &token) {
		std::optional<unsigned int> token_as_number;
		try {
			token_as_number.emplace(std::stoul(convert_to_uniform_text_format(token.val)));
		} 
		catch (std::invalid_argument const &ex) {
			// TODO: GEN_ERROR
		}
		catch (std::out_of_range const &ex) {
			// TODO: GEN_ERROR
		}
		return token_as_number;
	}

	[[nodiscard]] static std::optional<unsigned int> map_operation_to_assign_operation(const syrec_operation::operation operation) {
		std::optional<unsigned int> mapping_result;
		switch (operation) {
			case syrec_operation::operation::add_assign:
				mapping_result.emplace(syrec::AssignStatement::Add);
				break;
			case syrec_operation::operation::minus_assign:
				mapping_result.emplace(syrec::AssignStatement::Subtract);
				break;
			case syrec_operation::operation::xor_assign:
				mapping_result.emplace(syrec::AssignStatement::Exor);
				break;
			default:
				// TODO: GEN_ERROR ?
				break;
		}
		return mapping_result;
	}

	[[nodiscard]] static std::optional<unsigned int> map_operation_to_unary_operation(const syrec_operation::operation operation) {
		std::optional<unsigned int> mapping_result;
		switch (operation) {
			case syrec_operation::operation::increment_assign:
				mapping_result.emplace(syrec::UnaryStatement::Increment);
				break;
			case syrec_operation::operation::decrement_assign:
				mapping_result.emplace(syrec::UnaryStatement::Decrement);
				break;
			case syrec_operation::operation::negate_assign:
				mapping_result.emplace(syrec::UnaryStatement::Invert);
				break;
			default:
				// TODO: GEN_ERROR ?
				break;
		}
		return mapping_result;
	}

	[[nodiscard]] static std::optional<unsigned int> map_operation_to_shift_operation(const syrec_operation::operation operation) {
		std::optional<unsigned int> mapping_result;
		switch (operation) {
			case syrec_operation::operation::shift_left:
				mapping_result.emplace(syrec::ShiftExpression::Left);
				break;
			case syrec_operation::operation::shift_right:
				mapping_result.emplace(syrec::ShiftExpression::Right);
				break;
			default:
				// TODO: GEN_ERROR ?
				break;
		}
		return mapping_result;
	}

	[[nodiscard]] static std::optional<unsigned int> map_operation_to_binary_operation(const syrec_operation::operation operation) {
		std::optional<unsigned int> mapping_result;
		switch (operation) {
			case syrec_operation::operation::addition:
				mapping_result.emplace(syrec::BinaryExpression::Add);
				break;
			case syrec_operation::operation::subtraction:
				mapping_result.emplace(syrec::BinaryExpression::Subtract);
				break;
				case syrec_operation::operation::multiplication:
				mapping_result.emplace(syrec::BinaryExpression::Multiply);
				break;		
			case syrec_operation::operation::division:
				mapping_result.emplace(syrec::BinaryExpression::Divide);
				break;			
			case syrec_operation::operation::modulo:
				mapping_result.emplace(syrec::BinaryExpression::Modulo);
				break;		
			case syrec_operation::operation::upper_bits_multiplication:
				mapping_result.emplace(syrec::BinaryExpression::FracDivide);
				break;		
			case syrec_operation::operation::bitwise_xor:
				mapping_result.emplace(syrec::BinaryExpression::Exor);
				break;		
			case syrec_operation::operation::logical_and:
				mapping_result.emplace(syrec::BinaryExpression::LogicalAnd);
				break;		
			case syrec_operation::operation::logical_or:
				mapping_result.emplace(syrec::BinaryExpression::LogicalOr);
				break;		
			case syrec_operation::operation::bitwise_and:
				mapping_result.emplace(syrec::BinaryExpression::BitwiseAnd);
				break;	
			case syrec_operation::operation::bitwise_or:
				mapping_result.emplace(syrec::BinaryExpression::BitwiseOr);
				break;	
			case syrec_operation::operation::less_than:
				mapping_result.emplace(syrec::BinaryExpression::LessThan);
				break;	
			case syrec_operation::operation::greater_than:
				mapping_result.emplace(syrec::BinaryExpression::GreaterThan);
				break;	
			case syrec_operation::operation::equals:
				mapping_result.emplace(syrec::BinaryExpression::Equals);
				break;		
			case syrec_operation::operation::not_equals:
				mapping_result.emplace(syrec::BinaryExpression::NotEquals);
				break;		
			case syrec_operation::operation::less_equals:
				mapping_result.emplace(syrec::BinaryExpression::LessEquals);
				break;		
			case syrec_operation::operation::greater_equals:
				mapping_result.emplace(syrec::BinaryExpression::GreaterEquals);
				break;
			default:
				// TODO: GEN_ERROR ?
				break;
		}
		return mapping_result;
	}

	[[nodiscard]] const wchar_t* convertErrorMsgToRequiredFormat(const std::string& errorMsg) {
		return std::wstring(errorMsg.begin(), errorMsg.end()).c_str();
	}

	bool checkIdentWasDeclaredOrLogError(const std::string_view& ident) {
		if (!currSymTabScope->contains(ident)) {
			// TODO: GEN_ERROR
			SemErr(convertErrorMsgToRequiredFormat(fmt::format(UndeclaredIdent, ident)));
			return false;
		}
		return true;
	}

	[[nodiscard]] std::optional<unsigned int> applyBinaryOperation(const syrec_operation::operation operation, const unsigned int leftOperand, const unsigned int rightOperand) {
		if (operation == syrec_operation::operation::division && rightOperand == 0) {
			// TODO: GEN_ERROR
			SemErr(convertErrorMsgToRequiredFormat(DivisionByZero));
			return std::nullopt;	
		}
		else {
			return syrec_operation::apply(operation, leftOperand, rightOperand);	
		}
	}

	[[nodiscard]] std::optional<unsigned int> evaluateNumberContainer(const syrec::Number::ptr &numberContainer) {
		if (numberContainer->isLoopVariable()) {
			const std::string& loopVariableIdentToResolve = numberContainer->variableName();
			if (loop_variable_mapping_lookup.find(loopVariableIdentToResolve) == loop_variable_mapping_lookup.end()) {
				// TODO: GEN_ERROR
				SemErr(convertErrorMsgToRequiredFormat(fmt::format(NoMappingForLoopVariable, loopVariableIdentToResolve)));
				return std::nullopt;
			}
		}
		return std::optional(numberContainer->evaluate(loop_variable_mapping_lookup));
	}

	[[nodiscard]] ExpressionEvaluationResult::ptr createExpressionEvalutionResultContainer() const {
		return std::make_shared<ExpressionEvaluationResult>(ExpressionEvaluationResult());
	}

	void setConfig(const ParserConfig& customConfig) {
		this->config = customConfig;
	}

/*-------------------------------------------------------------------------*/



	Parser(Scanner *scanner);
	~Parser();
	void SemErr(const wchar_t* msg);

	void Number(std::optional<syrec::Number::ptr> &parsedNumber, const bool simplifyIfPossible );
	void SyReC();
	void Module(std::optional<syrec::Module::ptr> &parsedModule	);
	void ParameterList(const syrec::Module::ptr &module, bool &isValidModuleDefinition);
	void SignalList(const syrec::Module::ptr& module, bool &isValidModuleDefinition );
	void StatementList(syrec::Statement::vec &statements );
	void Parameter(std::optional<syrec::Variable::ptr> &parameter );
	void SignalDeclaration(const std::optional<syrec::Variable::Types> variableType, std::optional<syrec::Variable::ptr> &signalDeclaration );
	void Statement(std::optional<syrec::Statement::ptr> &user_defined_statement );
	void CallStatement(std::optional<syrec::Statement::ptr> &statement );
	void ForStatement(std::optional<syrec::Statement::ptr> &statement );
	void IfStatement(std::optional<syrec::Statement::ptr> &statement );
	void UnaryStatement(std::optional<syrec::Statement::ptr> &statement );
	void SkipStatement(std::optional<syrec::Statement::ptr> &statement );
	void AssignStatement(std::optional<syrec::Statement::ptr> &statement );
	void SwapStatement(std::optional<syrec::Statement::ptr> &statement );
	void Expression(const ExpressionEvaluationResult::ptr &userDefinedExpression, const unsigned int bitwidth, const bool simplifyIfPossible);
	void Signal(SignalEvaluationResult &signalAccess, const bool simplifyIfPossible);
	void BinaryExpression(const ExpressionEvaluationResult::ptr &parsedBinaryExpression, const unsigned int bitwidth, const bool simplifyIfPossible);
	void ShiftExpression(const ExpressionEvaluationResult::ptr &userDefinedShiftExpression, const unsigned int bitwidth, const bool simplifyIfPossible);
	void UnaryExpression(const ExpressionEvaluationResult::ptr &unaryExpression, const unsigned int bitwidth, const bool simplifyIfPossible);

	void Parse();

}; // end Parser

} // namespace


#endif

