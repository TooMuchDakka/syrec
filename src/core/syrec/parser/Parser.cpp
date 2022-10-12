

#include <wchar.h>
#include "core/syrec/parser/Parser.h"
#include "core/syrec/parser/Scanner.h"


namespace parser {


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

void Parser::Number(std::optional<syrec::Number::ptr> &parsedNumber, const bool simplifyIfPossible ) {
		if (la->kind == _int) {
			Get();
			const std::optional<unsigned int> conversion_result = convert_token_value_to_number(*t);
			if (conversion_result.has_value()) {
			const syrec::Number::ptr result = std::make_shared<syrec::Number>(syrec::Number(conversion_result.value())); 
			parsedNumber.emplace(result);
			}
			
		} else if (la->kind == 3 /* "#" */) {
			Get();
			Expect(_ident);
			const std::string signalIdent = convert_to_uniform_text_format(t->val);
			if (!checkIdentWasDeclaredOrLogError(signalIdent)) {
			return;
			}
			
			const auto &symTabEntry = currSymTabScope->getVariable(signalIdent);
			if (symTabEntry.has_value() && std::holds_alternative<syrec::Variable::ptr>(symTabEntry.value())) {
			parsedNumber.emplace(std::make_shared<syrec::Number>(syrec::Number(std::get<syrec::Variable::ptr>(symTabEntry.value())->bitwidth)));
			}
			else {
			// TODO: GEN_ERROR, this should not happen but check anyways
			return;
			}
			
		} else if (la->kind == 4 /* "$" */) {
			Get();
			Expect(_ident);
			const std::string signalIdent = convert_to_uniform_text_format(t->val);
			if (!checkIdentWasDeclaredOrLogError(signalIdent)) {
			return;
			}
			
			const auto &symTabEntry = currSymTabScope->getVariable(signalIdent);
			if (symTabEntry.has_value() && std::holds_alternative<syrec::Number::ptr>(symTabEntry.value())) {
			// TODO: An optimization that can be done here would be the replacement of the loop variable with its current value from the mapping
			parsedNumber.emplace(std::get<syrec::Number::ptr>(symTabEntry.value()));
			}
			else {
			// TODO: GEN_ERROR, this should not happen but check anyways
			}
			
		} else if (la->kind == 5 /* "(" */) {
			std::optional<syrec::Number::ptr> lhsOperand, rhsOperand;  
			std::optional<syrec_operation::operation> op;
			
			Get();
			Number(lhsOperand, simplifyIfPossible);
			if (la->kind == 6 /* "+" */) {
				Get();
				op.emplace(syrec_operation::operation::addition);		
			} else if (la->kind == 7 /* "-" */) {
				Get();
				op.emplace(syrec_operation::operation::subtraction);	
			} else if (la->kind == 8 /* "*" */) {
				Get();
				op.emplace(syrec_operation::operation::multiplication);	
			} else if (la->kind == 9 /* "/" */) {
				Get();
				op.emplace(syrec_operation::operation::division);		
			} else SynErr(56);
			Number(rhsOperand, simplifyIfPossible);
			if (!op.has_value()) {
			// TODO: GEN_ERROR Expected one of n operand but not of them was specified
			return;
			}
			
			if (!lhsOperand.has_value() || !rhsOperand.has_value()){
			return;
			}
			
			const std::optional<unsigned int> lhsEvaluated = evaluateNumberContainer(lhsOperand.value());
			const std::optional<unsigned int> rhsEvaluated = evaluateNumberContainer(rhsOperand.value());
			
			if (lhsEvaluated.has_value() && rhsEvaluated.has_value()) {
			const std::optional<unsigned int> evaluationResult = applyBinaryOperation(op.value(), lhsEvaluated.value(), rhsEvaluated.value());
			
			if (evaluationResult.has_value()) {
			const syrec::Number::ptr result = std::make_shared<syrec::Number>(syrec::Number(evaluationResult.value())); 
			parsedNumber.emplace(result);
			}
			}
			
			Expect(10 /* ")" */);
		} else SynErr(57);
}

void Parser::SyReC() {
		std::optional<syrec::Module::ptr> module;	
		Module(module);
		if (module.has_value()) {
		currSymTabScope->addEntry(module.value());
		this->modules.emplace_back(module.value());
		}
		
		while (la->kind == 11 /* "module" */) {
			module.reset();	
			Module(module);
			if (module.has_value()) {
			const syrec::Module::ptr well_formed_module = module.value();
			if (currSymTabScope->contains(well_formed_module)) {
			// TODO: GEN_ERROR 
			// TODO: Do not cancel parsing	
			}
			else {
			currSymTabScope->addEntry(well_formed_module);
			this->modules.emplace_back(module.value());
			}
			}
			
		}
}

void Parser::Module(std::optional<syrec::Module::ptr> &parsedModule	) {
		SymbolTable::openScope(currSymTabScope);
		std::optional<std::vector<syrec::Variable::ptr>> locals;	
		bool isValidModuleDefinition = false;
		syrec::Statement::vec moduleStmts {};
		
		Expect(11 /* "module" */);
		Expect(_ident);
		const std::string moduleName = convert_to_uniform_text_format(t->val);	
		syrec::Module::ptr userDefinedModule = std::make_shared<syrec::Module>(syrec::Module(moduleName));
		
		Expect(5 /* "(" */);
		if (la->kind == 13 /* "in" */ || la->kind == 14 /* "out" */ || la->kind == 15 /* "inout" */) {
			ParameterList(isValidModuleDefinition, userDefinedModule);
		}
		Expect(10 /* ")" */);
		while (la->kind == 16 /* "wire" */ || la->kind == 17 /* "state" */) {
			SignalList(userDefinedModule, isValidModuleDefinition);
		}
		StatementList(moduleStmts);
		SymbolTable::closeScope(currSymTabScope);
		if (moduleStmts.empty()) {
		isValidModuleDefinition = false;
		// TODO: GEN_ERROR
		}
		
		if (isValidModuleDefinition) {
		for (const auto &statement : moduleStmts) {
		userDefinedModule->addStatement(statement);
		}
		parsedModule.emplace(userDefinedModule);
		}
		
}

void Parser::ParameterList(bool &is_valid_module_definition, const syrec::Module::ptr &module) {
		std::optional<syrec::Variable::ptr> parameter;	
		Parameter(parameter);
		is_valid_module_definition = parameter.has_value();
		if (is_valid_module_definition) {
		module->addParameter(parameter.value());
		currSymTabScope->addEntry(parameter.value());
		}
		
		while (la->kind == 12 /* "," */) {
			parameter.reset();	
			Get();
			Parameter(parameter);
			is_valid_module_definition = parameter.has_value();
			if (is_valid_module_definition) {	
			const syrec::Variable::ptr &well_formed_parameter = parameter.value();
			if (!currSymTabScope->contains(well_formed_parameter->name)) {
			module->addParameter(well_formed_parameter);
			currSymTabScope->addEntry(well_formed_parameter);
			}
			else {
			is_valid_module_definition = false;
			// TODO: GEN_ERROR 
			}
			}
			
		}
}

void Parser::SignalList(const syrec::Module::ptr& module, bool &isValidModuleDefinition ) {
		syrec::Variable::Types signalType = syrec::Variable::In;
		std::optional<syrec::Variable::ptr> declaredSignal;
		bool isValidSignalType = true;
		
		if (la->kind == 16 /* "wire" */) {
			Get();
			signalType = syrec::Variable::Wire;		
		} else if (la->kind == 17 /* "state" */) {
			Get();
			signalType = syrec::Variable::State;		
		} else SynErr(58);
		if (syrec::Variable::Wire != signalType && syrec::Variable::State != signalType) {
		// TODO: GEN_ERROR ?
		// TODO: Do not cancel parsing
		isValidSignalType = false;
		}
		
		SignalDeclaration(signalType, declaredSignal);
		if (isValidSignalType && declaredSignal.has_value()) {
		const syrec::Variable::ptr &validSignalDeclaration = declaredSignal.value();
		if (currSymTabScope->contains(validSignalDeclaration->name)) {
		// TODO: GEN_ERROR 
		// TODO: Do not cancel parsing
		isValidModuleDefinition = false;
		}
		else {
		currSymTabScope->addEntry(validSignalDeclaration);
		if (isValidModuleDefinition) {
		module->variables.emplace_back(validSignalDeclaration);
		}
		}
		}
		else {
		isValidModuleDefinition = false;
		}
		
		while (la->kind == 12 /* "," */) {
			declaredSignal.reset();			
			Get();
			SignalDeclaration(signalType, declaredSignal);
			if (isValidSignalType && declaredSignal.has_value()) {
			const syrec::Variable::ptr &validSignalDeclaration = declaredSignal.value();
			if (currSymTabScope->contains(validSignalDeclaration->name)) {
			// TODO: GEN_ERROR 
			// TODO: Do not cancel parsing
			isValidModuleDefinition = false;
			}
			else {
			currSymTabScope->addEntry(validSignalDeclaration);
			if (isValidModuleDefinition) {
			module->variables.emplace_back(validSignalDeclaration);
			}
			}
			}
			else {
			isValidModuleDefinition = false;
			}
			
		}
}

void Parser::StatementList(syrec::Statement::vec &statements ) {
		std::optional<syrec::Statement::ptr> user_defined_statement;	
		Statement(user_defined_statement);
		if (user_defined_statement.has_value()) {
		statements.emplace_back(user_defined_statement.value());
		}
		
		while (la->kind == 20 /* ";" */) {
			user_defined_statement.reset();	
			Get();
			Statement(user_defined_statement);
			if (user_defined_statement.has_value()) {
			statements.emplace_back(user_defined_statement.value());
			}
			
		}
}

void Parser::Parameter(std::optional<syrec::Variable::ptr> &parameter ) {
		syrec::Variable::Types parameter_type = syrec::Variable::Wire;
		bool valid_variable_type = true;
		
		if (la->kind == 13 /* "in" */) {
			Get();
			parameter_type = syrec::Variable::In;	
		} else if (la->kind == 14 /* "out" */) {
			Get();
			parameter_type = syrec::Variable::Out;	
		} else if (la->kind == 15 /* "inout" */) {
			Get();
			parameter_type = syrec::Variable::Inout;	
		} else SynErr(59);
		if (syrec::Variable::Wire == parameter_type) {
		// TODO: GEN_ERROR 
		// TODO: Do not cancel parsing
		valid_variable_type = false;
		}
		
		SignalDeclaration(parameter_type, parameter);
		if (!valid_variable_type) {
		parameter.reset();
		}
		
}

void Parser::SignalDeclaration(const syrec::Variable::Types variableType, std::optional<syrec::Variable::ptr> &signalDeclaration ) {
		std::vector<unsigned int> dimensions{};
		unsigned int signalWidth = config.cDefaultSignalWidth;	
		bool isValidSignalDeclaration = true;
		
		Expect(_ident);
		const std::string signalIdent = convert_to_uniform_text_format(t->val);	
		while (la->kind == 18 /* "[" */) {
			Get();
			Expect(_int);
			const std::optional<unsigned int> dimension = convert_token_value_to_number(*t);
			if (!dimension.has_value()) {
			isValidSignalDeclaration = false;
			// TODO: GEN_ERROR
			}
			else {
			dimensions.emplace_back(dimension.value());	
			}
			
			Expect(19 /* "]" */);
		}
		if (la->kind == 5 /* "(" */) {
			Get();
			Expect(_int);
			const std::optional<unsigned int> customSignalWidth = convert_token_value_to_number(*t);
			if (!customSignalWidth.has_value()) {
			// TODO: GEN_ERROR
			isValidSignalDeclaration = false;
			}
			else {
			signalWidth = customSignalWidth.value();
			}
			
			Expect(10 /* ")" */);
		}
		if (isValidSignalDeclaration) {
		signalDeclaration.emplace(std::make_shared<syrec::Variable>(syrec::Variable(variableType, signalIdent, dimensions, signalWidth)));
		}
		
}

void Parser::Statement(std::optional<syrec::Statement::ptr> &user_defined_statement ) {
		if (la->kind == 21 /* "call" */ || la->kind == 22 /* "uncall" */) {
			CallStatement(user_defined_statement);
		} else if (la->kind == 23 /* "for" */) {
			ForStatement(user_defined_statement);
		} else if (la->kind == 29 /* "if" */) {
			IfStatement(user_defined_statement);
		} else if (la->kind == 33 /* "~" */ || la->kind == 34 /* "++" */ || la->kind == 35 /* "--" */) {
			UnaryStatement(user_defined_statement);
		} else if (la->kind == 38 /* "skip" */) {
			SkipStatement(user_defined_statement);
		} else if (shouldTakeAssignInsteadOfSwapAlternative(this)) {
			AssignStatement(user_defined_statement);
		} else if (la->kind == _ident) {
			SwapStatement(user_defined_statement);
		} else SynErr(60);
}

void Parser::CallStatement(std::optional<syrec::Statement::ptr> &statement ) {
		std::optional<bool> isCallStatement;
		std::vector<std::string> calleeArguments {};
		std::size_t numActualParameters = 0;
		bool isValidCallOperation = true;
		
		if (la->kind == 21 /* "call" */) {
			Get();
			isCallStatement.emplace(true);	
		} else if (la->kind == 22 /* "uncall" */) {
			Get();
			isCallStatement.emplace(false);	
		} else SynErr(61);
		isValidCallOperation = isCallStatement.has_value();	
		Expect(_ident);
		std::string methodIdent = convert_to_uniform_text_format(t->val);
		const MethodCallGuess::ptr guessesForPossibleCall = std::make_shared<MethodCallGuess>(MethodCallGuess(currSymTabScope, methodIdent));
		if (!guessesForPossibleCall->hasSomeMatches()) {
		// TODO: GEN_ERROR no method for ident declared
		isValidCallOperation = false;
		}
		
		Expect(5 /* "(" */);
		Expect(_ident);
		std::string parameterIdent = convert_to_uniform_text_format(t->val);
		isValidCallOperation &= checkIdentWasDeclaredOrLogError(parameterIdent);
		
		if (isValidCallOperation) {
		const std::optional<std::variant<syrec::Variable::ptr, syrec::Number::ptr>> paramSymTabEntry = currSymTabScope->getVariable(parameterIdent);
		isValidCallOperation = paramSymTabEntry.has_value() && std::holds_alternative<syrec::Variable::ptr>(paramSymTabEntry.value());
		
		if (isValidCallOperation) {
		guessesForPossibleCall->refineWithNextParameter(std::get<syrec::Variable::ptr>(paramSymTabEntry.value()));
		calleeArguments.emplace_back(parameterIdent);
		}
		else {
		// TODO: GEN_ERROR Declared identifier was not a variable (but a loop variable)
		}
		}
		else {
		// TODO: GEN_ERROR Use of undeclared identifier
		}
		numActualParameters++;
		
		while (la->kind == 12 /* "," */) {
			Get();
			Expect(_ident);
			std::string parameterIdent = convert_to_uniform_text_format(t->val);
			isValidCallOperation &= checkIdentWasDeclaredOrLogError(parameterIdent);
			
			if (isValidCallOperation) {
			const std::optional<std::variant<syrec::Variable::ptr, syrec::Number::ptr>> paramSymTabEntry = currSymTabScope->getVariable(parameterIdent);
			isValidCallOperation = paramSymTabEntry.has_value() && std::holds_alternative<syrec::Variable::ptr>(paramSymTabEntry.value());
			
			if (isValidCallOperation) {
			guessesForPossibleCall->refineWithNextParameter(std::get<syrec::Variable::ptr>(paramSymTabEntry.value()));
			calleeArguments.emplace_back(parameterIdent);
			}
			else {
			// TODO: GEN_ERROR Declared identifier was not a variable (but a loop variable)
			}
			}
			else {
			// TODO: GEN_ERROR Use of undeclared identifier
			}
			numActualParameters++;
			
		}
		Expect(10 /* ")" */);
		if (!isValidCallOperation){
		return;
		}
		
		if (!guessesForPossibleCall->hasSomeMatches()) {
		return;		
		}
		
		guessesForPossibleCall->discardGuessesWithMoreThanNParameters(numActualParameters);
		if (!guessesForPossibleCall->hasSomeMatches()) {
		// TODO: GEN_ERROR All of the declared methods that matched had more than n parameters
		return;
		}
		
		syrec::Module::vec possibleCalls = guessesForPossibleCall->getMatchesForGuess().value();
		if (possibleCalls.size() > 1) {
		// TODO: GEN_ERROR Ambiguous call, more than one match for current setups
		return;
		}
		const syrec::Module::ptr matchingModuleForCall = possibleCalls.at(0);
		if (isCallStatement.value()) {
		statement.emplace(std::make_shared<syrec::CallStatement>(syrec::CallStatement(matchingModuleForCall, calleeArguments)));
		}
		else {
		statement.emplace(std::make_shared<syrec::UncallStatement>(syrec::UncallStatement(matchingModuleForCall, calleeArguments)));
		}
		
}

void Parser::ForStatement(std::optional<syrec::Statement::ptr> &statement ) {
		std::optional<std::string> loopVariableIdent;
		std::optional<syrec::Number::ptr> iterationRangeStart;
		std::optional<syrec::Number::ptr> iterationRangeEnd;
		std::optional<syrec::Number::ptr> customStepSize;
		bool negativeStepSize = false;
		syrec::Statement::vec loopBody{};
		bool explicitRangeStartDefined = false;
		bool explicitStepSizeDefined = false;
		
		Expect(23 /* "for" */);
		if (checkIsLoopInitialValueExplicitlyDefined(this)) {
			if (checkIsLoopVariableExplicitlyDefined(this)) {
				Expect(4 /* "$" */);
				Expect(_ident);
				const std::string &loopVarIdent = convert_to_uniform_text_format(t->val);
				if (!checkIdentWasDeclaredOrLogError(loopVarIdent)) {
				loopVariableIdent.emplace(convert_to_uniform_text_format(t->val));
				SymbolTable::openScope(currSymTabScope);
				const syrec::Number::ptr loopVariableEntry = std::make_shared<syrec::Number>(syrec::Number(loopVarIdent));
				currSymTabScope->addEntry(loopVariableEntry);
				}
				
				Expect(24 /* "=" */);
			}
			Number(iterationRangeStart, true);
			explicitRangeStartDefined = true;	
			Expect(25 /* "to" */);
		}
		Number(iterationRangeEnd, true);
		if (!explicitRangeStartDefined){
		iterationRangeStart = iterationRangeEnd;
		}
		
		if (la->kind == 26 /* "step" */) {
			Get();
			explicitStepSizeDefined = true;	
			if (la->kind == 7 /* "-" */) {
				Get();
				negativeStepSize = true;	
			}
			Number(customStepSize, true);
			if (customStepSize.has_value() && !customStepSize.value()->evaluate({})) {
			// TODO: GEN_ERROR step size cannot be zero ?
			}
			
		}
		if (!explicitStepSizeDefined) {
		customStepSize.emplace(std::make_shared<syrec::Number>(syrec::Number(1)));
		}
		
		bool validLoopHeader = loopVariableIdent.has_value() 
						&& (explicitRangeStartDefined ? iterationRangeStart.has_value() : true)
						&& iterationRangeEnd.has_value()
						&& (explicitStepSizeDefined ? customStepSize.has_value() : true);
		if (validLoopHeader) {
		const unsigned int iterationRangeStartValue = iterationRangeStart.value()->evaluate({});
		const unsigned int iterationRangeEndValue = iterationRangeEnd.value()->evaluate({});
		const unsigned int stepSize = customStepSize.value()->evaluate({});
		
		unsigned int numIterations;
		if ((negativeStepSize && iterationRangeEndValue > iterationRangeStartValue)
		|| (!negativeStepSize && iterationRangeStartValue > iterationRangeEndValue)
		|| !stepSize) {
		// TODO: Either generate error or warning
		numIterations = 0;	
		validLoopHeader = false;
		}
		else {
		numIterations = negativeStepSize 
		? (iterationRangeStartValue - iterationRangeEndValue)
		: (iterationRangeEndValue - iterationRangeStartValue);
		numIterations = (numIterations + 1) / stepSize;
		}
		}
		
		Expect(27 /* "do" */);
		StatementList(loopBody);
		if (loopBody.empty()) {
		// TODO: GEN_ERROR
		}
		
		Expect(28 /* "rof" */);
		if (loopVariableIdent.has_value()) {
		SymbolTable::closeScope(currSymTabScope);
		}
		
		// TODO: If a statement must be generated, one could create a skip statement instead of simply returning 
		if (!validLoopHeader || loopBody.empty()) {
		return;
		}
		
		const std::shared_ptr<syrec::ForStatement> loopStatement = std::make_shared<syrec::ForStatement>();
		loopStatement->loopVariable = loopVariableIdent.value();
		loopStatement->range = std::pair(iterationRangeStart.value(), iterationRangeEnd.value());
		loopStatement->step = customStepSize.value();
		loopStatement->statements = loopBody;
		statement.emplace(loopStatement);
		
}

void Parser::IfStatement(std::optional<syrec::Statement::ptr> &statement ) {
		const ExpressionEvaluationResult::ptr condition = createExpressionEvalutionResultContainer();
		const ExpressionEvaluationResult::ptr closingCondition = createExpressionEvalutionResultContainer();
		syrec::Statement::vec trueBranch{};
		syrec::Statement::vec falseBranch{};
		
		Expect(29 /* "if" */);
		Expression(condition, 1u, false);
		Expect(30 /* "then" */);
		StatementList(trueBranch);
		Expect(31 /* "else" */);
		StatementList(falseBranch);
		Expect(32 /* "fi" */);
		Expression(closingCondition, 1u, false);
		const bool conditionalWellFormed = condition->hasValue() 
		&& closingCondition->hasValue()
		&& !trueBranch.empty()
		&& !falseBranch.empty();
		if (!conditionalWellFormed) {
		return;
		}
		
		const std::shared_ptr<syrec::IfStatement> &conditional = std::make_shared<syrec::IfStatement>();
		conditional->condition = condition->getOrConvertToExpression(1u).value();
		conditional->fiCondition = closingCondition->getOrConvertToExpression(1u).value();
		conditional->thenStatements                    = trueBranch;
		conditional->elseStatements                    = falseBranch;
		statement.emplace(conditional);
		
}

void Parser::UnaryStatement(std::optional<syrec::Statement::ptr> &statement ) {
		SignalEvaluationResult unaryStmtOperand;
		std::optional<syrec_operation::operation> unaryOperation;	
		
		if (la->kind == 33 /* "~" */) {
			Get();
			unaryOperation.emplace(syrec_operation::operation::negate_assign);	
		} else if (la->kind == 34 /* "++" */) {
			Get();
			unaryOperation.emplace(syrec_operation::operation::increment_assign);	
		} else if (la->kind == 35 /* "--" */) {
			Get();
			unaryOperation.emplace(syrec_operation::operation::decrement_assign);	
		} else SynErr(62);
		Expect(24 /* "=" */);
		Signal(unaryStmtOperand, false);
		bool allSemanticChecksOk = true;
		if (!unaryOperation.has_value()){
		allSemanticChecksOk = false;
		// TODO: GEN_ERROR Expected a valid unary operand
		}
		if (unaryStmtOperand.isValid() && !unaryStmtOperand.isVariableAccess()) {
		allSemanticChecksOk = false;
		// TODO: GEN_ERROR Operand can only be variable access
		}
		
		const std::optional<unsigned int> mappedOperation = map_operation_to_unary_operation(unaryOperation.value());
		allSemanticChecksOk &= mappedOperation.has_value();
		if (!allSemanticChecksOk) {
		return;
		}
		
		const syrec::VariableAccess::ptr unaryOperandAsVarAccess = unaryStmtOperand.getAsVariableAccess().value();
		statement.emplace(std::make_shared<syrec::UnaryStatement>(syrec::UnaryStatement(mappedOperation.value(), unaryOperandAsVarAccess)));
		
}

void Parser::SkipStatement(std::optional<syrec::Statement::ptr> &statement ) {
		Expect(38 /* "skip" */);
		statement.emplace(std::make_shared<syrec::SkipStatement>(syrec::SkipStatement()));	
}

void Parser::AssignStatement(std::optional<syrec::Statement::ptr> &statement ) {
		SignalEvaluationResult assignStmtLhs;
		const ExpressionEvaluationResult::ptr assignStmtRhs = createExpressionEvalutionResultContainer();
		std::optional<syrec_operation::operation> assignOperation;
		unsigned int expressionBitwidth = 1u;
		
		std::optional<syrec::VariableAccess::ptr> assigned_to_obj;
		bool allSemanticChecksOk = true;
		
		Signal(assignStmtLhs, false);
		allSemanticChecksOk = assignStmtLhs.isValid();
		if (allSemanticChecksOk){
		if (!assignStmtLhs.isVariableAccess()) {
		allSemanticChecksOk = false;
		// TODO: GEN_ERROR: Lhs operand must be variable access
		}
		else {
		expressionBitwidth = assignStmtLhs.getAsVariableAccess().value()->bitwidth();
		}
		}
		
		if (la->kind == 36 /* "^" */) {
			Get();
			assignOperation.emplace(syrec_operation::operation::xor_assign);	
		} else if (la->kind == 6 /* "+" */) {
			Get();
			assignOperation.emplace(syrec_operation::operation::add_assign);	
		} else if (la->kind == 7 /* "-" */) {
			Get();
			assignOperation.emplace(syrec_operation::operation::minus_assign);	
		} else SynErr(63);
		Expect(24 /* "=" */);
		Expression(assignStmtRhs, expressionBitwidth, false);
		if (!allSemanticChecksOk || !assignOperation.has_value() || !assignStmtRhs->hasValue()) {
		return;
		}
		
		// TODO: To not break reversability of operation, check that expression does not contain the assigned to signal 
		const std::optional<unsigned int> mappedOperation = map_operation_to_assign_operation(assignOperation.value());
		if (mappedOperation.has_value()) {
		statement.emplace(std::make_shared<syrec::AssignStatement>(syrec::AssignStatement(assignStmtLhs.getAsVariableAccess().value(),
		mappedOperation.value(),
		assignStmtRhs->getOrConvertToExpression(expressionBitwidth).value())));
		}
		
}

void Parser::SwapStatement(std::optional<syrec::Statement::ptr> &statement ) {
		SignalEvaluationResult swapMe, swapOther;
		bool isSwapOperatorDefined = false;
		
		Signal(swapMe, false);
		Expect(37 /* "<=>" */);
		isSwapOperatorDefined = true;	
		Signal(swapOther, false);
		bool allSemanticChecksOk = true;
		if (swapMe.isValid() && !swapMe.isVariableAccess()) {
		allSemanticChecksOk = false;
		// TODO: GEN_ERROR: Lhs operand must be variable access
		}
		if (swapOther.isValid() && !swapOther.isVariableAccess()){
		allSemanticChecksOk = false;
		// TODO: GEN_ERROR: Lhs operand must be variable access
		}
		
		if (isSwapOperatorDefined && allSemanticChecksOk) {
		statement.emplace(std::make_shared<syrec::SwapStatement>(syrec::SwapStatement(swapMe.getAsVariableAccess().value(),
		swapOther.getAsVariableAccess().value())));
		}
		
}

void Parser::Expression(const ExpressionEvaluationResult::ptr &userDefinedExpression, const unsigned int bitwidth, const bool simplifyIfPossible) {
		SignalEvaluationResult signal;
		std::optional<syrec::Number::ptr> number;
		
		if (StartOf(1)) {
			if (shouldTakeNumberInsteadOfBinaryAlternative(this)) {
				Number(number, simplifyIfPossible);
				if (number.has_value()) {
				const syrec::NumericExpression::ptr expr = std::make_shared<syrec::NumericExpression>(syrec::NumericExpression(number.value(), bitwidth));
				userDefinedExpression->setResult(expr);
				}
				
			} else if (shouldTakeBinaryInsteadOfShiftAlternative(this)) {
				BinaryExpression(userDefinedExpression, bitwidth, simplifyIfPossible);
			} else {
				ShiftExpression(userDefinedExpression, bitwidth, simplifyIfPossible);
			}
		} else if (la->kind == _ident) {
			Signal(signal, simplifyIfPossible);
			if (!signal.isValid()) {
			return;
			}
			
			if (signal.isVariableAccess()) {
			userDefinedExpression->setResult(std::make_shared<syrec::VariableExpression>(syrec::VariableExpression(signal.getAsVariableAccess().value())));
			}
			else if (signal.isConstant()) {
			userDefinedExpression->setResult(std::make_shared<syrec::NumericExpression>(syrec::NumericExpression(signal.getAsNumber().value(), bitwidth)));
			}
			
		} else if (la->kind == 33 /* "~" */ || la->kind == 52 /* "!" */) {
			UnaryExpression(userDefinedExpression, bitwidth, simplifyIfPossible);
		} else SynErr(64);
}

void Parser::Signal(SignalEvaluationResult &signalAccess, const bool simplifyIfPossible) {
		std::optional<syrec::VariableAccess::ptr> accessedSignal;
		const unsigned int defaultIndexExpressionBitwidth = 1u;
		unsigned int indexExpressionBitwidth = defaultIndexExpressionBitwidth;
		
		// TODO: Using global zero_based indexing flag to initialize default value
		std::size_t accessedDimensionIdx = 0;
		
		Expect(_ident);
		const std::string signalIdent = convert_to_uniform_text_format(t->val);
		if (checkIdentWasDeclaredOrLogError(signalIdent)) {
		const auto symbolTableEntryForSignal = currSymTabScope->getVariable(signalIdent);
		if (symbolTableEntryForSignal.has_value() && std::holds_alternative<syrec::Variable::ptr>(symbolTableEntryForSignal.value())) {
		const syrec::VariableAccess::ptr container = std::make_shared<syrec::VariableAccess>(syrec::VariableAccess());
		container->setVar(std::get<syrec::Variable::ptr>(symbolTableEntryForSignal.value()));
		accessedSignal.emplace(container);
		indexExpressionBitwidth = accessedSignal.value()->bitwidth();
		}
		}
		bool isValidSignalAccess = accessedSignal.has_value();
		bool indexExpressionsSemanticallyOk = isValidSignalAccess;
		
		while (la->kind == 18 /* "[" */) {
			Get();
			ExpressionEvaluationResult::ptr dimensionExpression = createExpressionEvalutionResultContainer(); 
			Expression(dimensionExpression, indexExpressionBitwidth, simplifyIfPossible);
			indexExpressionsSemanticallyOk &= dimensionExpression->hasValue();
			
			if (indexExpressionsSemanticallyOk) {
			const std::optional<unsigned int> &constantValueForAccessedDimension = dimensionExpression->getAsConstant();
			if (constantValueForAccessedDimension.has_value()) {
			// TODO: Using global flag indicating zero_based indexing or not
			indexExpressionsSemanticallyOk = isValidDimensionAccess(accessedSignal.value()->getVar(), constantValueForAccessedDimension.value(), true);
			
			if (!indexExpressionsSemanticallyOk) {
			// TODO: Using global flag indicating zero_based indexing or not
			const IndexAccessRangeConstraint constraintForCurrentDimension = getConstraintsForValidDimensionAccess(accessedSignal.value()->getVar(), accessedDimensionIdx, true).value();
			// TODO: GEN_ERROR
			}
			}
			
			if (indexExpressionsSemanticallyOk) {
			accessedSignal.value()->indexes.emplace_back(dimensionExpression->getOrConvertToExpression(indexExpressionBitwidth).value());
			}
			}
			accessedDimensionIdx++;
			
			Expect(19 /* "]" */);
		}
		isValidSignalAccess = indexExpressionsSemanticallyOk;	
		if (la->kind == 39 /* "." */) {
			std::optional<syrec::Number::ptr> bitRangeStart;
			std::optional<syrec::Number::ptr> bitRangeEnd;
			bool rangeExplicitlyDefined = false;
			
			Get();
			Number(bitRangeStart, false);
			if (la->kind == 40 /* ":" */) {
				Get();
				Number(bitRangeEnd, false);
				rangeExplicitlyDefined = true;	
			}
			isValidSignalAccess &= bitRangeStart.has_value() && (rangeExplicitlyDefined ? bitRangeEnd.has_value() : true);
			if (isValidSignalAccess) {
			const std::pair<syrec::Number::ptr, syrec::Number::ptr> bitRange(bitRangeStart.value(), rangeExplicitlyDefined ? bitRangeEnd.value() : bitRangeStart.value());
			const std::pair<std::size_t, std::size_t> bitRangeEvaluated(bitRange.first->evaluate({}), bitRange.second->evaluate({}));
			
			const syrec::Variable::ptr &accessedVariable = accessedSignal.value()->getVar();
			if (rangeExplicitlyDefined) {
			// TODO: Using global zero_based indexing flag
			if (!isValidBitRangeAccess(accessedVariable, bitRangeEvaluated, true)){
			isValidSignalAccess = false;
			// TODO: GEN_ERROR
			}
			}
			else {
			// TODO: Using global zero_based indexing flag	
			if (!isValidBitAccess(accessedVariable, bitRangeEvaluated.first, true)){
			isValidSignalAccess = false;
			// TODO: GEN_ERROR
			}
			}
			
			if (isValidSignalAccess) {
			accessedSignal.value()->range.emplace(bitRange);
			}
			}
			
		}
		if (isValidSignalAccess) {
		signalAccess.updateResultToVariableAccess(accessedSignal.value());
		}
		
}

void Parser::BinaryExpression(const ExpressionEvaluationResult::ptr &parsedBinaryExpression, const unsigned int bitwidth, const bool simplifyIfPossible) {
		ExpressionEvaluationResult::ptr binaryExprLhs = createExpressionEvalutionResultContainer();
		ExpressionEvaluationResult::ptr binaryExprRhs = createExpressionEvalutionResultContainer();
		std::optional<syrec_operation::operation> binaryOperation;
		unsigned int expectedBitWidth = bitwidth;
		
		Expect(5 /* "(" */);
		Expression(binaryExprLhs, bitwidth, simplifyIfPossible);
		if (binaryExprLhs->hasValue()) {
		expectedBitWidth = std::max(binaryExprLhs->getOrConvertToExpression(std::nullopt).value()->bitwidth(), expectedBitWidth);
		}
		
		switch (la->kind) {
		case 6 /* "+" */: {
			Get();
			binaryOperation.emplace(syrec_operation::operation::addition);	
			break;
		}
		case 7 /* "-" */: {
			Get();
			binaryOperation.emplace(syrec_operation::operation::subtraction);	
			break;
		}
		case 36 /* "^" */: {
			Get();
			binaryOperation.emplace(syrec_operation::operation::bitwise_xor);	
			break;
		}
		case 8 /* "*" */: {
			Get();
			binaryOperation.emplace(syrec_operation::operation::multiplication);	
			break;
		}
		case 9 /* "/" */: {
			Get();
			binaryOperation.emplace(syrec_operation::operation::division);	
			break;
		}
		case 41 /* "%" */: {
			Get();
			binaryOperation.emplace(syrec_operation::operation::modulo);	
			break;
		}
		case 42 /* "*>" */: {
			Get();
			binaryOperation.emplace(syrec_operation::operation::upper_bits_multiplication);	
			break;
		}
		case 43 /* "&&" */: {
			Get();
			binaryOperation.emplace(syrec_operation::operation::logical_and);	
			break;
		}
		case 44 /* "||" */: {
			Get();
			binaryOperation.emplace(syrec_operation::operation::logical_or);	
			break;
		}
		case 45 /* "&" */: {
			Get();
			binaryOperation.emplace(syrec_operation::operation::bitwise_and);	
			break;
		}
		case 46 /* "|" */: {
			Get();
			binaryOperation.emplace(syrec_operation::operation::bitwise_or);	
			break;
		}
		case 47 /* "<" */: {
			Get();
			binaryOperation.emplace(syrec_operation::operation::less_than);	
			break;
		}
		case 48 /* ">" */: {
			Get();
			binaryOperation.emplace(syrec_operation::operation::greater_than);	
			break;
		}
		case 24 /* "=" */: {
			Get();
			binaryOperation.emplace(syrec_operation::operation::equals);	
			break;
		}
		case 49 /* "!=" */: {
			Get();
			binaryOperation.emplace(syrec_operation::operation::not_equals);	
			break;
		}
		case 50 /* "<=" */: {
			Get();
			binaryOperation.emplace(syrec_operation::operation::less_equals);	
			break;
		}
		case 51 /* ">=" */: {
			Get();
			binaryOperation.emplace(syrec_operation::operation::greater_equals);	
			break;
		}
		default: SynErr(65); break;
		}
		if (!binaryOperation.has_value()) {
		// TODO: GEN_ERROR Expected one of n possible operands but non was specified
		}
		
		Expression(binaryExprRhs, expectedBitWidth, simplifyIfPossible);
		Expect(10 /* ")" */);
		if (binaryExprLhs->hasValue() && binaryOperation.has_value() && binaryExprRhs->hasValue()) {
		const std::optional<unsigned int> mappedOperation = map_operation_to_binary_operation(binaryOperation.value());
		if (!mappedOperation.has_value()) {
		return;
		}
		
		if (binaryExprLhs->evaluatedToConstant() && binaryExprRhs->evaluatedToConstant()) {
		const unsigned int lhsValueEvaluated = binaryExprLhs->getAsConstant().value();
		const unsigned int rhsValueEvaluated = binaryExprRhs->getAsConstant().value();
		const std::optional<unsigned int> operationApplicationResult = applyBinaryOperation(binaryOperation.value(), lhsValueEvaluated, rhsValueEvaluated);
		if (operationApplicationResult.has_value()) {
		parsedBinaryExpression->setResult(operationApplicationResult.value(), expectedBitWidth);
		}
		}
		else {
		const syrec::expression::ptr lhsOperand = binaryExprLhs->getOrConvertToExpression(std::nullopt).value();
		const syrec::expression::ptr rhsOperand = binaryExprRhs->getOrConvertToExpression(std::nullopt).value();
		parsedBinaryExpression->setResult(std::make_shared<syrec::BinaryExpression>(syrec::BinaryExpression(lhsOperand, 
																				 mappedOperation.value(),
																				 rhsOperand)));
		}
		}
		
}

void Parser::ShiftExpression(const ExpressionEvaluationResult::ptr &userDefinedShiftExpression, const unsigned int bitwidth, const bool simplifyIfPossible) {
		ExpressionEvaluationResult::ptr shiftExpressionLhs = createExpressionEvalutionResultContainer();
		std::optional<syrec::Number::ptr> shiftAmount;
		std::optional<syrec_operation::operation> shiftOperation;
		
		Expect(5 /* "(" */);
		Expression(shiftExpressionLhs, bitwidth, simplifyIfPossible);
		if (la->kind == 53 /* "<<" */) {
			Get();
			shiftOperation.emplace(syrec_operation::operation::shift_left);	
		} else if (la->kind == 54 /* ">>" */) {
			Get();
			shiftOperation.emplace(syrec_operation::operation::shift_right);	
		} else SynErr(66);
		Number(shiftAmount, simplifyIfPossible);
		if (!shiftExpressionLhs->hasValue() || !shiftOperation.has_value() || !shiftAmount.has_value()) {
		return;
		}
		const std::optional<unsigned int> mappedShiftOperation = map_operation_to_shift_operation(shiftOperation.value());
		const syrec::expression::ptr lhsOperandExpression = shiftExpressionLhs->getOrConvertToExpression(bitwidth).value();
		
		// One could replace the shift statement with a skip statement if the shift amount is zero 
		const std::optional<unsigned int> shiftAmountValueEvaluated = evaluateNumberContainer(shiftAmount.value());
		const std::optional<unsigned int> lhsOperandValueEvaluated = shiftExpressionLhs->getAsConstant();
		
		if (shiftAmountValueEvaluated.has_value() && lhsOperandValueEvaluated.has_value()) {
		const std::optional<unsigned int> shiftApplicationResult = syrec_operation::apply(shiftOperation.value(), lhsOperandValueEvaluated.value(), shiftAmountValueEvaluated.value());
		if (shiftApplicationResult.has_value()) {
		userDefinedShiftExpression->setResult(shiftApplicationResult.value(), bitwidth);
		}
		else {
		// TODO: GEN_ERROR
		return;
		}
		}
		else {
		userDefinedShiftExpression->setResult(std::make_shared<syrec::ShiftExpression>(lhsOperandExpression,
		mappedShiftOperation.value(),
		shiftAmount.value()));
		}
		
		Expect(10 /* ")" */);
}

void Parser::UnaryExpression(const ExpressionEvaluationResult::ptr &unaryExpression, const unsigned int bitwidth, const bool simplifyIfPossible) {
		ExpressionEvaluationResult::ptr unaryExpressionOperand = createExpressionEvalutionResultContainer();
		std::optional<syrec_operation::operation> unaryOperation;
		
		if (la->kind == 52 /* "!" */) {
			Get();
			unaryOperation.emplace(syrec_operation::operation::logical_negation);	
		} else if (la->kind == 33 /* "~" */) {
			Get();
			unaryOperation.emplace(syrec_operation::operation::bitwise_negation);	
		} else SynErr(67);
		Expression(unaryExpressionOperand, bitwidth, simplifyIfPossible);
		return;
		
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
			case 17: s = coco_string_create(L"\"state\" expected"); break;
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

