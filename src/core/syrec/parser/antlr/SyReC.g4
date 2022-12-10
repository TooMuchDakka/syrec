grammar SyReC;

/* Token defintions */
OP_INCREMENT_ASSIGN: '++=' ;
OP_DECREMENT_ASSIGN: '--=' ;
OP_INVERT_ASSIGN: '~=' ;

OP_ADD_ASSIGN: '+=' ;
OP_SUB_ASSIGN: '-=' ;
OP_XOR_ASSIGN: '^=' ;

OP_PLUS : '+' ;
OP_MINUS : '-' ;
OP_MULTIPLY: '*' ;
OP_UPPER_BIT_MULTIPLY: '*>' ;
OP_DIVISION: '/' ;
OP_MODULO: '%' ;

OP_LEFT_SHIFT: '<<' ;
OP_RIGHT_SHIFT: '>>' ;

OP_GREATER_OR_EQUAL: '>=' ;
OP_LESS_OR_EQUAL: '<=' ;
OP_GREATER_THAN: '>' ;
OP_LESS_THAN: '<' ;
OP_EQUAL: '=';
OP_NOT_EQUAL: '!=';

OP_LOGICAL_AND: '&&' ;
OP_LOGICAL_OR: '||' ;
OP_LOGICAL_NEGATION: '!' ;

OP_BITWISE_AND: '&' ;
OP_BITWISE_NEGATION: '~' ;
OP_BITWISE_OR: '|' ;
OP_BITWISE_XOR: '^' ;

OP_CALL: 'call' ;
OP_UNCALL: 'uncall' ;

VAR_TYPE_IN: 'in' ;
VAR_TYPE_OUT: 'out' ;
VAR_TYPE_INOUT: 'inout' ;
VAR_TYPE_WIRE: 'wire' ;
VAR_TYPE_STATE: 'state' ;

LOOP_VARIABLE_PREFIX: '$' ;
SIGNAL_WIDTH_PREFIX: '#' ;

STATEMENT_DELIMITER: ';' ;
PARAMETER_DELIMITER: ',' ;

/* LINE_COMMENT: '#' ~[\r\n]* -> skip ; */
/* unicode whitespace \u000B */
SKIPABLEWHITSPACES : [ \t\r\n]+ -> channel(HIDDEN) ;	// Skip newline, tabulator and carriage return symbols
LINE_COMMENT: '//' .*? ('\n'|EOF) -> channel(HIDDEN) ;
MULTI_LINE_COMMENT: '/*' .*? '*/' -> channel(HIDDEN) ;

fragment LETTER : 'a'..'z' | 'A'..'Z' ;
fragment DIGIT : '0'..'9' ;
IDENT : ( '_' | LETTER ) ( '_' | LETTER | DIGIT )* ;
INT : DIGIT+ ;

/* Number production */
number: 
	INT								# NumberFromConstant
	| SIGNAL_WIDTH_PREFIX IDENT		# NumberFromSignalwidth
	| LOOP_VARIABLE_PREFIX IDENT	# NumberFromLoopVariable
	| ( '(' lhsOperand=number op=( OP_PLUS | OP_MINUS | OP_MULTIPLY | OP_DIVISION ) rhsOperand=number ')' ) # NumberFromExpression
	;

/* Program and modules productions */

program: module+ EOF;

module :
	'module' 
	IDENT													
	'(' 
		parameterList?
	')' 
	signalList*										
	statementList										
	;

parameterList:
	parameter
	(													
		PARAMETER_DELIMITER parameter
	)* 
	;

parameter:
	(
		VAR_TYPE_IN
		| VAR_TYPE_OUT
		| VAR_TYPE_INOUT
	)														
	signalDeclaration
	;

signalList: 
	(
		VAR_TYPE_WIRE
		| VAR_TYPE_STATE
	)
	signalDeclaration
	(															
		PARAMETER_DELIMITER signalDeclaration
	)* 
	;

signalDeclaration:	
IDENT																		
( 
	'[' 
		dimensionTokens+=INT						
	']' 
)* 
(
	'(' 
		signalWidthToken=INT												
	')' 
)?															
;

/* Statements productions */

statementList: stmts+=statement ( STATEMENT_DELIMITER stmts+=statement )* ;

statement:
	callStatement
	| forStatement
	| ifStatement
	| unaryStatement
	| assignStatement
	| swapStatement
	| skipStatement	
	;

callStatement: ( OP_CALL | OP_UNCALL ) moduleIdent=IDENT '(' calleeArguments+=IDENT ( ',' calleeArguments+=IDENT )* ')' ;

loopVariableDefinition: LOOP_VARIABLE_PREFIX variableIdent=IDENT OP_EQUAL ;
loopStepsizeDefinition: 'step' OP_MINUS? number ;
forStatement: 'for' ( loopVariableDefinition? startValue=number 'to' )? endValue=number loopStepsizeDefinition? 'do' statementList 'rof' ;

ifStatement: 
	'if' guardCondition=expression 'then' 
		trueBranchStmts=statementList 
	'else' 
		falseBranchStmts=statementList 
	'fi' matchingGuardExpression=expression ;

unaryStatement:  unaryOp=( OP_INVERT_ASSIGN | OP_INCREMENT_ASSIGN | OP_DECREMENT_ASSIGN ) signal ;

assignStatement: signal assignmentOp=( OP_ADD_ASSIGN | OP_SUB_ASSIGN | OP_XOR_ASSIGN ) expression ;

swapStatement: 
	lhsOperand=signal '<=>' rhsOperand=signal ;

skipStatement: 'skip' ;

signal: IDENT ( '[' accessedDimensions+=expression ']' )* ( '.' bitStart=number ( ':' bitRangeEnd=number )? )? ;

/* Expression productions */

expression: 
	number						# ExpressionFromNumber
	| signal					# ExpressionFromSignal
	| binaryExpression			# ExpressionFromBinaryExpression
	| unaryExpression			# ExpressionFromUnaryExpression
	| shiftExpression			# ExpressionFromShiftExpression
	;

binaryExpression:
	'(' lhsOperand=expression
		binaryOperation=( OP_PLUS
		| OP_MINUS
		| OP_MULTIPLY
		| OP_DIVISION
		| OP_MODULO
		| OP_UPPER_BIT_MULTIPLY
		| OP_LOGICAL_AND
		| OP_LOGICAL_OR
		| OP_BITWISE_AND
		| OP_BITWISE_OR
		| OP_BITWISE_XOR
		| OP_LESS_THAN
		| OP_GREATER_THAN
		| OP_EQUAL
		| OP_NOT_EQUAL
		| OP_LESS_OR_EQUAL
		| OP_GREATER_OR_EQUAL
		)
	rhsOperand=expression  ')' 
	;

unaryExpression: unaryOperation=( OP_LOGICAL_NEGATION | OP_BITWISE_NEGATION ) expression ;

shiftExpression: '(' expression shiftOperation=( OP_RIGHT_SHIFT | OP_LEFT_SHIFT ) number ')' ;