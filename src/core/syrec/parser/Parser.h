

#if !defined(syrec_COCO_PARSER_H__)
#define syrec_COCO_PARSER_H__

#include <cwchar>
#include <set>
#include <string>
#include <vector>

#include "core/syrec/module.hpp"
#include "core/syrec/parser/parser_error_message_generator.hpp"
#include "core/syrec/parser/symbol_table.hpp"
#include "core/syrec/parser/text_utils.hpp"


#include "Scanner.h"

namespace syrec {


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

module::vec modules;
	parser_error_message_generator error_message_generator;
	symbol_table::ptr current_symbol_table_scope;

	// This method will be called by the contructor if it exits.
	// This support is specific to the C++ version of Coco/R.
	void Init() {
		// nothing to do
		current_symbol_table_scope = nullptr;
	}

	// Uncomment this method if cleanup is necessary,
	// this method will be called by the destructor if it exists.
	// This support is specific to the C++ version of Coco/R.
	// void Destroy() {
		// nothing to do
	// }

	[[nodiscard]] bool are_token_values_equal(const wchar_t *my_token_value, const wchar_t *other_token_value) const {
		if (my_token_value == nullptr || other_token_value == nullptr) {
			// TODO: Could also throw exception
			return false;
		}
		return !wcscmp(my_token_value, other_token_value);
	}

	[[nodiscard]] bool token_matches_one_of(const Token *tokenToCheck, std::vector<std::wstring> one_of_many_token_values) const {
        const std::set<std::wstring> set_of_matching_token_values{std::begin(one_of_many_token_values), std::end(one_of_many_token_values)};
        return set_of_matching_token_values.count(tokenToCheck->val);
	}

	[[nodiscard]] bool find_matching_token(std::vector<std::wstring> matching_token_values, std::vector<std::wstring> token_values_allowing_stop_of_search) const {
		const std::set<std::wstring> set_of_matching_token_values { std::begin(matching_token_values), std::end(matching_token_values) };
		const std::set<std::wstring> set_of_token_values_allowing_stop_of_search { std::begin(token_values_allowing_stop_of_search), std::end(token_values_allowing_stop_of_search) };
		bool found_matching_operator = false;
		bool can_cancel_search = false;

		while (!can_cancel_search) {
			const Token *peeked_token = scanner->Peek();
			found_matching_operator = set_of_matching_token_values.count(peeked_token->val);
			can_cancel_search = found_matching_operator || peeked_token->kind == _EOF || set_of_token_values_allowing_stop_of_search.count(peeked_token->val);
		}
		return found_matching_operator;
	}

	[[nodiscard]] bool check_if_is_assign_statement() const {
		const std::vector<std::wstring> matching_tokens = {L"^", L"+", L"-"};
		const std::vector<std::wstring> tokens_resulting_in_check_being_false = {L";", L"<=>"};

		return find_matching_token(matching_tokens, tokens_resulting_in_check_being_false);
	}

	[[nodiscard]] bool check_if_expression_is_binary_expression() const {
		// Since first LL(1) conflict resolver already started peeking and failed to resolve the conflict, we need to reset the peek position to original one
		scanner->ResetPeek();

		const std::set<std::wstring> tokens_resulting_in_check_being_false {L"+", L"-", L"^", L"*", L"/", L"%", L"*>", L"&&", L"||", L"&", L"|", L"<", L">", L"=", L"!=", L"<=", L">="};
		const std::set<std::wstring> set_of_operator_token_values_of_alternative {L"<<", L">>"};

		bool found_matching_operator = false;
		bool can_cancel_search = false;
		int expression_nesting_level = 0;

		while (!can_cancel_search) {
			const Token *peeked_token = scanner->Peek();

			if (are_token_values_equal(peeked_token->val, L"(")) {
				expression_nesting_level++;
			} 
			else if (are_token_values_equal(peeked_token->val, L")")) {
				expression_nesting_level--;
			}
			else {
				found_matching_operator = expression_nesting_level == 0 && tokens_resulting_in_check_being_false.count(peeked_token->val);
				can_cancel_search = found_matching_operator || set_of_operator_token_values_of_alternative.count(peeked_token->val) || peeked_token->kind == _EOF;			
			}

		}
		return found_matching_operator;
	}

	[[nodiscard]] bool is_token_not_nested_number(const Token *token) const {
		if (token == nullptr) {
			return false;
		}
		return token->kind == _int || are_token_values_equal(token->val, L"#") || are_token_values_equal(token->val, L"$");
	}

	[[nodiscard]] bool check_if_expression_is_number(const Token *first_token_of_expression) const {
        bool found_matching_operator;
        const std::vector<std::wstring> matching_tokens = {L"^", L"+", L"-", L"*"};

        if (are_token_values_equal(first_token_of_expression->val, L"(")) {
            found_matching_operator = check_if_expression_is_number(scanner->Peek())
                && token_matches_one_of(scanner->Peek(), matching_tokens)
                && check_if_expression_is_number(scanner->Peek())
                && are_token_values_equal(scanner->Peek()->val, L")");
        } else {
            found_matching_operator = is_token_not_nested_number(first_token_of_expression);
        }
        return found_matching_operator;
	}

	[[nodiscard]] bool check_if_expression_is_number() const {
        return la != nullptr && check_if_expression_is_number(la);
	}

	[[nodiscard]] bool check_if_loop_variable_is_defined() const {
		// Since first LL(1) conflict resolver already started peeking and failed to resolve the conflict, we need to reset the peek position to original one
		scanner->ResetPeek();
		const std::vector<std::wstring> matching_tokens = {L"="};
		const std::vector<std::wstring> tokens_resulting_in_check_being_false = {L"rof"};

		return find_matching_token(matching_tokens, tokens_resulting_in_check_being_false);
	}

	[[nodiscard]] bool check_if_loop_iteration_range_start_is_defined() const {
		const std::vector<std::wstring> matching_tokens = {L"to"};
		const std::vector<std::wstring> tokens_resulting_in_check_being_false = {L"rof"};

		return find_matching_token(matching_tokens, tokens_resulting_in_check_being_false);
	}

/*-------------------------------------------------------------------------*/



	Parser(Scanner *scanner);
	~Parser();
	void SemErr(const wchar_t* msg);

	void Number();
	void SyReC();
	void Module(module::ptr &parsed_module);
	void ParameterList(const module::ptr &module);
	void SignalList(std::vector<variable::ptr> &signals );
	void StatementList();
	void Parameter(variable::ptr &parameter);
	void SignalDeclaration(variable::types variable_type, variable::ptr &declared_signal);
	void Statement();
	void CallStatement();
	void ForStatement();
	void IfStatement();
	void UnaryStatement();
	void SkipStatement();
	void AssignStatement();
	void SwapStatement();
	void Expression();
	void Signal();
	void BinaryExpression();
	void ShiftExpression();
	void UnaryExpression();

	void Parse();

}; // end Parser

} // namespace


#endif

