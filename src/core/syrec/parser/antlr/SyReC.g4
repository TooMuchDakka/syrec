grammar SyReC;

@header {
#include <vector>
#include <string>
}

@parser::members {
public:
	void Parse() {}

private:
	void Test() {}
}

/* Token defintions */
fragment LETTER : [a-zA-Z] ;
fragment DIGIT : [0-9] ;
IDENT : ( '_' | LETTER ) ( '_' | LETTER | DIGIT )* ;
INT : DIGIT+ ;
SKIPABLEWHITSPACES : [\t\r\n]+ ->skip ;	// Skip newline, tabulator and carriage return symbols

/* Number production */
number: 
	( INT 
	| '#' IDENT 
	| '$' IDENT 
	| ( '(' number ( '+' | '-' | '*' | '/' ) number ')' ) ) ;


/* Program and modules productions */

program: module+ ;

module: 'module' IDENT '(' parameterList ')' signalList* statementList ;

parameterList: parameter (',' parameter )* ;

parameter: ( 'in' | 'out' | 'inout' ) signalDeclaration ;

signalList: ( 'wire' | 'state' ) signalDeclaration ( ',' signalDeclaration )* ;

signalDeclaration: IDENT ( '[' INT ']' )* ( '(' INT ')' )? ;

/* Statements productions */

statementList: statement ( ';' statement )* ;

statement:
	callStatement
	| forStatement
	| ifStatement
	| unaryStatement
	| assignStatement
	| swapStatement
	| skipStatement	
	;

callStatement: ( 'call' | 'uncall' ) IDENT '(' IDENT ( ',' IDENT )* ')' ;

forStatement: 'for' ( ( '$' IDENT '=' )? number 'to' )? number ( 'step' ( '-' )? number )? statementList 'rof' ;

ifStatement: 'if' expression 'then' statementList 'else' statementList 'fi' expression ;

unaryStatement:  ( '~' | '++' | '--' ) '=' signal ;

assignStatement: signal ( '^' | '+' | '-' ) '=' expression ;

swapStatement: signal '<=>' signal ;

skipStatement: 'skip' ;

signal: IDENT ( '[' expression ']' )* ( '.' number ( ':' number )? )? ;

/* Expression productions */

expression: 
	number
	| signal
	| binaryExpression
	| unaryExpression
	| shiftExpression
	;

binaryExpression:
	'(' expression
		( '+'
		| '-'
		| '^'
		| '*'
		| '/'
		| '%'
		| '*>'
		| '&&'
		| '||'
		| '&'
		| '|'
		| '<'
		| '>'
		| '='
		| '!='
		| '<='
		| '>='
		)
	expression  ')' 
	;

unaryExpression: ( '!' | '~' ) expression ;

shiftExpression: '(' expression ( '<<' | '>>' ) number ')' ;

/*
module[std::vector<std::string>>, int x inputStrings] returns [std::vector<int> x] locals [int t = 0] :
	IDENT 'Test' '=' INT ';'
	;
*/
