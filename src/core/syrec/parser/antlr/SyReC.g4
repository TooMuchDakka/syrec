grammar SyReC;

/* Token defintions */
OP_PLUS : '+' ;
OP_MINUS : '-' ;
OP_MULTIPLY: '*' ;
OP_XOR: '^' ;
OP_UPPER_BIT_MULTIPLY: '>*' ;
OP_DIVISION: '/' ;
OP_MODULO: '%' ;

OP_GREATER_THAN: '>' ;
OP_GREATER_OR_EQUAL: '>=' ;
OP_LESS_THAN: '<' ;
OP_LESS_OR_EQUAL: '<=' ;
OP_EQUAL: '=';
OP_NOT_EQUAL: '!=';

OP_BITWISE_AND: '&' ;
OP_BITWISE_OR: '|' ;
OP_BITWISE_NEGATION: '~' ;

OP_LOGICAL_AND: '&&' ;
OP_LOGICAL_OR: '||' ;
OP_LOGICAL_NEGATION: '!' ;

OP_LEFT_SHIFT: '>>' ;
OP_RIGHT_SHIFT: '<<' ;

OP_INCREMENT: '++' ;
OP_DECREMENT: '--' ;

VAR_TYPE_IN: 'in' ;
VAR_TYPE_OUT: 'out' ;
VAR_TYPE_INOUT: 'inout' ;
VAR_TYPE_WIRE: 'wire' ;
VAR_TYPE_STATE: 'state' ;

LOOP_VARIABLE_PREFIX: '$' ;
SIGNAL_WIDTH_PREFIX: '#' ;

STATEMENT_DELIMITER: ';' ;
PARAMETER_DELIMITER: ',' ;

fragment LETTER : 'a'..'z' | 'A'..'Z' ;
fragment DIGIT : '0'..'9' ;
IDENT : ( '_' | LETTER ) ( '_' | LETTER | DIGIT )* ;
INT : DIGIT+ ;
SKIPABLEWHITSPACES : [ \t\r\n]+ -> skip ;	// Skip newline, tabulator and carriage return symbols


/* Number production */
number: 
	( INT 
	| SIGNAL_WIDTH_PREFIX IDENT 
	| LOOP_VARIABLE_PREFIX IDENT 
	| ( '(' lhsOperand=number ( OP_PLUS | OP_MINUS | OP_MULTIPLY | OP_DIVISION ) rhsOperand=number ')' ) ) ;


/* Program and modules productions */

program: module+ ;

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

callStatement: ( 'call' | 'uncall' ) moduleIdent=IDENT '(' calleArguments+=IDENT ( ',' calleeArguments+=IDENT )* ')' ;

forStatement: 'for' ( ( LOOP_VARIABLE_PREFIX variableIdent=IDENT OP_EQUAL )? startValue=number 'to' )? endValue=number ( 'step' ( OP_MINUS )? stepSize=number )? 'do' statementList 'rof' ;

ifStatement: 
	'if' guardCondition=expression 'then' 
		trueBranchStmts=statementList 
	'else' 
		falseBranchStmts=statementList 
	'fi' matchingGuardExpression=expression ;

unaryStatement:  ( OP_BITWISE_NEGATION | OP_INCREMENT | OP_DECREMENT ) OP_EQUAL signal ;

assignStatement: signal ( OP_XOR | OP_PLUS | OP_MINUS ) OP_EQUAL expression ;

swapStatement: 
	lhsOperand=signal '<=>' rhsOperand=signal ;

skipStatement: 'skip' ;

signal: IDENT ( '[' accessedDimensions+=expression ']' )* ( '.' bitStart=number ( ':' bitRangeEnd=number )? )? ;

/* Expression productions */

expression: 
	number
	| signal
	| binaryExpression
	| unaryExpression
	| shiftExpression
	;

binaryExpression:
	'(' lhsOperand=expression
		( OP_PLUS
		| OP_MINUS
		| OP_XOR
		| OP_MULTIPLY
		| OP_DIVISION
		| OP_MODULO
		| OP_UPPER_BIT_MULTIPLY
		| OP_LOGICAL_AND
		| OP_LOGICAL_OR
		| OP_BITWISE_AND
		| OP_BITWISE_OR
		| OP_LESS_THAN
		| OP_GREATER_THAN
		| OP_EQUAL
		| OP_NOT_EQUAL
		| OP_LESS_OR_EQUAL
		| OP_GREATER_OR_EQUAL
		)
	rhsOperand=expression  ')' 
	;

unaryExpression: ( OP_LOGICAL_NEGATION | OP_BITWISE_NEGATION ) expression ;

shiftExpression: '(' expression ( OP_RIGHT_SHIFT | OP_LEFT_SHIFT ) number ')' ;