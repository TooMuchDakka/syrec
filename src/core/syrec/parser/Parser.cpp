

#include <wchar.h>
#include "Parser.h"
#include "Scanner.h"


namespace syrec {


void Parser::SynErr(int n) {
	if (errDist >= minErrDist) errors->SynErr(la->line, la->col, n);
	errDist = 0;
}

void Parser::SemErr(const wchar_t* msg) {
	if (errDist >= minErrDist) errors->Error(t->line, t->col, msg);
	errDist = 0;
}

void Parser::Get() {
	for (;;) {
		t = la;
		la = scanner->Scan();
		if (la->kind <= maxT) { ++errDist; break; }

		if (dummyToken != t) {
			dummyToken->kind = t->kind;
			dummyToken->pos = t->pos;
			dummyToken->col = t->col;
			dummyToken->line = t->line;
			dummyToken->next = NULL;
			coco_string_delete(dummyToken->val);
			dummyToken->val = coco_string_create(t->val);
			t = dummyToken;
		}
		la = t;
	}
}

void Parser::Expect(int n) {
	if (la->kind==n) Get(); else { SynErr(n); }
}

void Parser::ExpectWeak(int n, int follow) {
	if (la->kind == n) Get();
	else {
		SynErr(n);
		while (!StartOf(follow)) Get();
	}
}

bool Parser::WeakSeparator(int n, int syFol, int repFol) {
	if (la->kind == n) {Get(); return true;}
	else if (StartOf(repFol)) {return false;}
	else {
		SynErr(n);
		while (!(StartOf(syFol) || StartOf(repFol) || StartOf(0))) {
			Get();
		}
		return StartOf(syFol);
	}
}

void Parser::Number() {
		if (la->kind == _int) {
			Get();
		} else if (la->kind == 3 /* "#" */) {
			Get();
			Expect(_ident);
		} else if (la->kind == 4 /* "$" */) {
			Get();
			Expect(_ident);
		} else if (la->kind == 5 /* "(" */) {
			Get();
			Number();
			if (la->kind == 6 /* "+" */) {
				Get();
			} else if (la->kind == 7 /* "-" */) {
				Get();
			} else if (la->kind == 8 /* "*" */) {
				Get();
			} else if (la->kind == 9 /* "/" */) {
				Get();
			} else SynErr(56);
			Number();
			Expect(10 /* ")" */);
		} else SynErr(57);
}

void Parser::SyReC() {
		module::ptr module = nullptr; 
		symbol_table::open_scope(current_symbol_table_scope);
		
		Module(module);
		symbol_table::close_scope(current_symbol_table_scope);
		if (nullptr != module) {
		current_symbol_table_scope->add_entry(module);
		}
		
		while (la->kind == 11 /* "module" */) {
			symbol_table::open_scope(current_symbol_table_scope);
			module = nullptr;
			
			Module(module);
			symbol_table::close_scope(current_symbol_table_scope);
			if (nullptr != module) {
			if (current_symbol_table_scope->contains(module)) {
			// TODO: GEN_ERROR 
			// TODO: Do not cancel parsing	
			}
			else {
			current_symbol_table_scope->add_entry(module);
			}
			}
			
		}
}

void Parser::Module(module::ptr &parsed_module) {
		std::vector<variable::ptr> locals {};	
		Expect(11 /* "module" */);
		Expect(_ident);
		const std::string module_name = convert_to_uniform_text_format(t->val);
		parsed_module = std::make_shared<module>(module(module_name));	
		
		Expect(5 /* "(" */);
		if (la->kind == 13 /* "in" */ || la->kind == 14 /* "out" */ || la->kind == 15 /* "inout" */) {
			ParameterList(parsed_module);
		}
		Expect(10 /* ")" */);
		while (la->kind == 16 /* "wire" */ || la->kind == 17 /* "signal" */) {
			SignalList(locals);
			parsed_module->variables = locals;	
		}
		StatementList();
}

void Parser::ParameterList(const module::ptr &module) {
		variable::ptr parameter = nullptr;	
		Parameter(parameter);
		if (nullptr != parameter) {
		module->add_parameter(parameter);
		}
		
		while (la->kind == 12 /* "," */) {
			parameter = nullptr;	
			Get();
			Parameter(parameter);
			if (nullptr != parameter) {
			// TODO: Check for duplicates
			if (current_symbol_table_scope->contains(parameter->name)) {
			// TODO: GEN_ERROR 
			// TODO: Do not cancel parsing
			}
			else {
			module->add_parameter(parameter);
			current_symbol_table_scope->add_entry(parameter);
			}
			}
			
		}
}

void Parser::SignalList(std::vector<variable::ptr> &signals ) {
		variable::types signal_type = variable::in;
		variable::ptr variable = nullptr;
		bool valid_signal_type = true;
		
		if (la->kind == 16 /* "wire" */) {
			Get();
			signal_type = variable::wire;		
		} else if (la->kind == 17 /* "signal" */) {
			Get();
			signal_type = variable::state;		
		} else SynErr(58);
		if (variable::wire != signal_type && variable::state != signal_type) {
		// TODO: GEN_ERROR ?
		// TODO: Do not cancel parsing
		valid_signal_type = false;
		}
		
		SignalDeclaration(signal_type, variable);
		if (nullptr != variable && valid_signal_type){
		if (current_symbol_table_scope->contains(variable->name)) {
		// TODO: GEN_ERROR 
		// TODO: Do not cancel parsing
		}
		else {
		signals.emplace_back(variable);
		current_symbol_table_scope->add_entry(variable);
		}
		}
		
		while (la->kind == 12 /* "," */) {
			variable = nullptr;					
			Get();
			SignalDeclaration(signal_type, variable);
			if (nullptr != variable && valid_signal_type){
			if (current_symbol_table_scope->contains(variable->name)) {
			// TODO: GEN_ERROR 
			// TODO: Do not cancel parsing
			}
			else {
			signals.emplace_back(variable);
			current_symbol_table_scope->add_entry(variable);
			}
			}
			
		}
}

void Parser::StatementList() {
		Statement();
		while (la->kind == 20 /* ";" */) {
			Get();
			Statement();
		}
}

void Parser::Parameter(variable::ptr &parameter) {
		variable::types parameter_type = variable::wire;
		bool valid_variable_type = true;
		
		if (la->kind == 13 /* "in" */) {
			Get();
			parameter_type = variable::in;	
		} else if (la->kind == 14 /* "out" */) {
			Get();
			parameter_type = variable::out;	
		} else if (la->kind == 15 /* "inout" */) {
			Get();
			parameter_type = variable::inout;	
		} else SynErr(59);
		if (variable::wire == parameter_type) {
		// TODO: GEN_ERROR 
		// TODO: Do not cancel parsing
		valid_variable_type = false;
		}
		
		SignalDeclaration(parameter_type, parameter);
		if (!valid_variable_type) {
		parameter = nullptr;
		}
		
}

void Parser::SignalDeclaration(variable::types variable_type, variable::ptr &declared_signal) {
		std::vector<unsigned int> dimensions{};
		// TODO: Use default bit width
		unsigned int signal_width = 0;	
		bool valid_declaration = true;
		
		Expect(_ident);
		const std::string signal_ident = convert_to_uniform_text_format(t->val);	
		while (la->kind == 18 /* "[" */) {
			Get();
			Expect(_int);
			const unsigned int dimension = std::stoul(convert_to_uniform_text_format(t->val));
			if (!dimension) {
			valid_declaration = false;
			// TODO: GEN_ERROR
			}
			else {
			dimensions.emplace_back(dimension);	
			}
			
			Expect(19 /* "]" */);
		}
		if (la->kind == 5 /* "(" */) {
			Get();
			Expect(_int);
			signal_width = std::stoul(convert_to_uniform_text_format(t->val));
			if (!signal_width) {
			// TODO: GEN_ERROR
			valid_declaration = false;
			}
			
			Expect(10 /* ")" */);
		}
		if (valid_declaration) {
		declared_signal = std::make_shared<variable>(variable(variable_type, signal_ident, dimensions, signal_width));	
		}
		else {
		declared_signal = nullptr;
		}
		
}

void Parser::Statement() {
		if (la->kind == 21 /* "call" */ || la->kind == 22 /* "uncall" */) {
			CallStatement();
		} else if (la->kind == 23 /* "for" */) {
			ForStatement();
		} else if (la->kind == 29 /* "if" */) {
			IfStatement();
		} else if (la->kind == 33 /* "~" */ || la->kind == 34 /* "++" */ || la->kind == 35 /* "--" */) {
			UnaryStatement();
		} else if (la->kind == 38 /* "skip" */) {
			SkipStatement();
		} else if (check_if_is_assign_statement()) {
			AssignStatement();
		} else if (la->kind == _ident) {
			SwapStatement();
		} else SynErr(60);
}

void Parser::CallStatement() {
		if (la->kind == 21 /* "call" */) {
			Get();
		} else if (la->kind == 22 /* "uncall" */) {
			Get();
		} else SynErr(61);
		Expect(_ident);
		Expect(5 /* "(" */);
		Expect(_ident);
		while (la->kind == 12 /* "," */) {
			Get();
			Expect(_ident);
		}
		Expect(10 /* ")" */);
}

void Parser::ForStatement() {
		Expect(23 /* "for" */);
		if (check_if_loop_iteration_range_start_is_defined()) {
			if (check_if_loop_variable_is_defined()) {
				Expect(4 /* "$" */);
				Expect(_ident);
				Expect(24 /* "=" */);
			}
			Number();
			Expect(25 /* "to" */);
		}
		Number();
		if (la->kind == 26 /* "step" */) {
			Get();
			if (la->kind == 7 /* "-" */) {
				Get();
			}
			Number();
		}
		Expect(27 /* "do" */);
		StatementList();
		Expect(28 /* "rof" */);
}

void Parser::IfStatement() {
		Expect(29 /* "if" */);
		Expression();
		Expect(30 /* "then" */);
		StatementList();
		Expect(31 /* "else" */);
		StatementList();
		Expect(32 /* "fi" */);
		Expression();
}

void Parser::UnaryStatement() {
		if (la->kind == 33 /* "~" */) {
			Get();
		} else if (la->kind == 34 /* "++" */) {
			Get();
		} else if (la->kind == 35 /* "--" */) {
			Get();
		} else SynErr(62);
		Expect(24 /* "=" */);
		Signal();
}

void Parser::SkipStatement() {
		Expect(38 /* "skip" */);
}

void Parser::AssignStatement() {
		Signal();
		if (la->kind == 36 /* "^" */) {
			Get();
		} else if (la->kind == 6 /* "+" */) {
			Get();
		} else if (la->kind == 7 /* "-" */) {
			Get();
		} else SynErr(63);
		Expect(24 /* "=" */);
		Expression();
}

void Parser::SwapStatement() {
		Signal();
		Expect(37 /* "<=>" */);
		Signal();
}

void Parser::Expression() {
		if (StartOf(1)) {
			if (check_if_expression_is_number()) {
				Number();
			} else if (check_if_expression_is_binary_expression()) {
				BinaryExpression();
			} else {
				ShiftExpression();
			}
		} else if (la->kind == _ident) {
			Signal();
		} else if (la->kind == 33 /* "~" */ || la->kind == 52 /* "!" */) {
			UnaryExpression();
		} else SynErr(64);
}

void Parser::Signal() {
		Expect(_ident);
		while (la->kind == 18 /* "[" */) {
			Get();
			Expression();
			Expect(19 /* "]" */);
		}
		if (la->kind == 39 /* "." */) {
			Get();
			Number();
			if (la->kind == 40 /* ":" */) {
				Get();
				Number();
			}
		}
}

void Parser::BinaryExpression() {
		Expect(5 /* "(" */);
		Expression();
		switch (la->kind) {
		case 6 /* "+" */: {
			Get();
			break;
		}
		case 7 /* "-" */: {
			Get();
			break;
		}
		case 36 /* "^" */: {
			Get();
			break;
		}
		case 8 /* "*" */: {
			Get();
			break;
		}
		case 9 /* "/" */: {
			Get();
			break;
		}
		case 41 /* "%" */: {
			Get();
			break;
		}
		case 42 /* "*>" */: {
			Get();
			break;
		}
		case 43 /* "&&" */: {
			Get();
			break;
		}
		case 44 /* "||" */: {
			Get();
			break;
		}
		case 45 /* "&" */: {
			Get();
			break;
		}
		case 46 /* "|" */: {
			Get();
			break;
		}
		case 47 /* "<" */: {
			Get();
			break;
		}
		case 48 /* ">" */: {
			Get();
			break;
		}
		case 24 /* "=" */: {
			Get();
			break;
		}
		case 49 /* "!=" */: {
			Get();
			break;
		}
		case 50 /* "<=" */: {
			Get();
			break;
		}
		case 51 /* ">=" */: {
			Get();
			break;
		}
		default: SynErr(65); break;
		}
		Expression();
		Expect(10 /* ")" */);
}

void Parser::ShiftExpression() {
		Expect(5 /* "(" */);
		Expression();
		if (la->kind == 53 /* "<<" */) {
			Get();
		} else if (la->kind == 54 /* ">>" */) {
			Get();
		} else SynErr(66);
		Number();
		Expect(10 /* ")" */);
}

void Parser::UnaryExpression() {
		if (la->kind == 52 /* "!" */) {
			Get();
		} else if (la->kind == 33 /* "~" */) {
			Get();
		} else SynErr(67);
		Expression();
}




// If the user declared a method Init and a mehtod Destroy they should
// be called in the contructur and the destructor respctively.
//
// The following templates are used to recognize if the user declared
// the methods Init and Destroy.

template<typename T>
struct ParserInitExistsRecognizer {
	template<typename U, void (U::*)() = &U::Init>
	struct ExistsIfInitIsDefinedMarker{};

	struct InitIsMissingType {
		char dummy1;
	};
	
	struct InitExistsType {
		char dummy1; char dummy2;
	};

	// exists always
	template<typename U>
	static InitIsMissingType is_here(...);

	// exist only if ExistsIfInitIsDefinedMarker is defined
	template<typename U>
	static InitExistsType is_here(ExistsIfInitIsDefinedMarker<U>*);

	enum { InitExists = (sizeof(is_here<T>(NULL)) == sizeof(InitExistsType)) };
};

template<typename T>
struct ParserDestroyExistsRecognizer {
	template<typename U, void (U::*)() = &U::Destroy>
	struct ExistsIfDestroyIsDefinedMarker{};

	struct DestroyIsMissingType {
		char dummy1;
	};
	
	struct DestroyExistsType {
		char dummy1; char dummy2;
	};

	// exists always
	template<typename U>
	static DestroyIsMissingType is_here(...);

	// exist only if ExistsIfDestroyIsDefinedMarker is defined
	template<typename U>
	static DestroyExistsType is_here(ExistsIfDestroyIsDefinedMarker<U>*);

	enum { DestroyExists = (sizeof(is_here<T>(NULL)) == sizeof(DestroyExistsType)) };
};

// The folloing templates are used to call the Init and Destroy methods if they exist.

// Generic case of the ParserInitCaller, gets used if the Init method is missing
template<typename T, bool = ParserInitExistsRecognizer<T>::InitExists>
struct ParserInitCaller {
	static void CallInit(T *t) {
		// nothing to do
	}
};

// True case of the ParserInitCaller, gets used if the Init method exists
template<typename T>
struct ParserInitCaller<T, true> {
	static void CallInit(T *t) {
		t->Init();
	}
};

// Generic case of the ParserDestroyCaller, gets used if the Destroy method is missing
template<typename T, bool = ParserDestroyExistsRecognizer<T>::DestroyExists>
struct ParserDestroyCaller {
	static void CallDestroy(T *t) {
		// nothing to do
	}
};

// True case of the ParserDestroyCaller, gets used if the Destroy method exists
template<typename T>
struct ParserDestroyCaller<T, true> {
	static void CallDestroy(T *t) {
		t->Destroy();
	}
};

void Parser::Parse() {
	t = NULL;
	la = dummyToken = new Token();
	la->val = coco_string_create(L"Dummy Token");
	Get();
	SyReC();
	Expect(0);
}

Parser::Parser(Scanner *scanner) {
	maxT = 55;

	ParserInitCaller<Parser>::CallInit(this);
	dummyToken = NULL;
	t = la = NULL;
	minErrDist = 2;
	errDist = minErrDist;
	this->scanner = scanner;
	errors = new Errors();
}

bool Parser::StartOf(int s) {
	const bool T = true;
	const bool x = false;

	static bool set[2][57] = {
		{T,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x},
		{x,x,T,T, T,T,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x}
	};



	return set[s][la->kind];
}

Parser::~Parser() {
	ParserDestroyCaller<Parser>::CallDestroy(this);
	delete errors;
	delete dummyToken;
}

Errors::Errors() {
	count = 0;
}

void Errors::SynErr(int line, int col, int n) {
	wchar_t* s;
	switch (n) {
			case 0: s = coco_string_create(L"EOF expected"); break;
			case 1: s = coco_string_create(L"ident expected"); break;
			case 2: s = coco_string_create(L"int expected"); break;
			case 3: s = coco_string_create(L"\"#\" expected"); break;
			case 4: s = coco_string_create(L"\"$\" expected"); break;
			case 5: s = coco_string_create(L"\"(\" expected"); break;
			case 6: s = coco_string_create(L"\"+\" expected"); break;
			case 7: s = coco_string_create(L"\"-\" expected"); break;
			case 8: s = coco_string_create(L"\"*\" expected"); break;
			case 9: s = coco_string_create(L"\"/\" expected"); break;
			case 10: s = coco_string_create(L"\")\" expected"); break;
			case 11: s = coco_string_create(L"\"module\" expected"); break;
			case 12: s = coco_string_create(L"\",\" expected"); break;
			case 13: s = coco_string_create(L"\"in\" expected"); break;
			case 14: s = coco_string_create(L"\"out\" expected"); break;
			case 15: s = coco_string_create(L"\"inout\" expected"); break;
			case 16: s = coco_string_create(L"\"wire\" expected"); break;
			case 17: s = coco_string_create(L"\"signal\" expected"); break;
			case 18: s = coco_string_create(L"\"[\" expected"); break;
			case 19: s = coco_string_create(L"\"]\" expected"); break;
			case 20: s = coco_string_create(L"\";\" expected"); break;
			case 21: s = coco_string_create(L"\"call\" expected"); break;
			case 22: s = coco_string_create(L"\"uncall\" expected"); break;
			case 23: s = coco_string_create(L"\"for\" expected"); break;
			case 24: s = coco_string_create(L"\"=\" expected"); break;
			case 25: s = coco_string_create(L"\"to\" expected"); break;
			case 26: s = coco_string_create(L"\"step\" expected"); break;
			case 27: s = coco_string_create(L"\"do\" expected"); break;
			case 28: s = coco_string_create(L"\"rof\" expected"); break;
			case 29: s = coco_string_create(L"\"if\" expected"); break;
			case 30: s = coco_string_create(L"\"then\" expected"); break;
			case 31: s = coco_string_create(L"\"else\" expected"); break;
			case 32: s = coco_string_create(L"\"fi\" expected"); break;
			case 33: s = coco_string_create(L"\"~\" expected"); break;
			case 34: s = coco_string_create(L"\"++\" expected"); break;
			case 35: s = coco_string_create(L"\"--\" expected"); break;
			case 36: s = coco_string_create(L"\"^\" expected"); break;
			case 37: s = coco_string_create(L"\"<=>\" expected"); break;
			case 38: s = coco_string_create(L"\"skip\" expected"); break;
			case 39: s = coco_string_create(L"\".\" expected"); break;
			case 40: s = coco_string_create(L"\":\" expected"); break;
			case 41: s = coco_string_create(L"\"%\" expected"); break;
			case 42: s = coco_string_create(L"\"*>\" expected"); break;
			case 43: s = coco_string_create(L"\"&&\" expected"); break;
			case 44: s = coco_string_create(L"\"||\" expected"); break;
			case 45: s = coco_string_create(L"\"&\" expected"); break;
			case 46: s = coco_string_create(L"\"|\" expected"); break;
			case 47: s = coco_string_create(L"\"<\" expected"); break;
			case 48: s = coco_string_create(L"\">\" expected"); break;
			case 49: s = coco_string_create(L"\"!=\" expected"); break;
			case 50: s = coco_string_create(L"\"<=\" expected"); break;
			case 51: s = coco_string_create(L"\">=\" expected"); break;
			case 52: s = coco_string_create(L"\"!\" expected"); break;
			case 53: s = coco_string_create(L"\"<<\" expected"); break;
			case 54: s = coco_string_create(L"\">>\" expected"); break;
			case 55: s = coco_string_create(L"??? expected"); break;
			case 56: s = coco_string_create(L"invalid Number"); break;
			case 57: s = coco_string_create(L"invalid Number"); break;
			case 58: s = coco_string_create(L"invalid SignalList"); break;
			case 59: s = coco_string_create(L"invalid Parameter"); break;
			case 60: s = coco_string_create(L"invalid Statement"); break;
			case 61: s = coco_string_create(L"invalid CallStatement"); break;
			case 62: s = coco_string_create(L"invalid UnaryStatement"); break;
			case 63: s = coco_string_create(L"invalid AssignStatement"); break;
			case 64: s = coco_string_create(L"invalid Expression"); break;
			case 65: s = coco_string_create(L"invalid BinaryExpression"); break;
			case 66: s = coco_string_create(L"invalid ShiftExpression"); break;
			case 67: s = coco_string_create(L"invalid UnaryExpression"); break;

		default:
		{
			wchar_t format[20];
			coco_swprintf(format, 20, L"error %d", n);
			s = coco_string_create(format);
		}
		break;
	}
	wprintf(L"-- line %d col %d: %ls\n", line, col, s);
	coco_string_delete(s);
	count++;
}

void Errors::Error(int line, int col, const wchar_t *s) {
	wprintf(L"-- line %d col %d: %ls\n", line, col, s);
	count++;
}

void Errors::Warning(int line, int col, const wchar_t *s) {
	wprintf(L"-- line %d col %d: %ls\n", line, col, s);
}

void Errors::Warning(const wchar_t *s) {
	wprintf(L"%ls\n", s);
}

void Errors::Exception(const wchar_t* s) {
	wprintf(L"%ls", s); 
	exit(1);
}

} // namespace

