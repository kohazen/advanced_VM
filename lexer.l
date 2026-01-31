%option noinput
%option nounput
%option yylineno

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "parser.tab.h"
%}

%%

[0-9]+ {
    yylval = createIntNode(atoi(yytext));
    yylval->line_number = yylineno;
    return INTEGER;
}

"var"     { return VAR; }
"if"      { return IF; }
"else"    { return ELSE; }
"while"   { return WHILE; }
"print"   { return PRINT; }

[a-zA-Z_][a-zA-Z0-9_]* {
    ASTNode *n = createNode(NODE_VAR, NULL, NULL);
    n->varName = strdup(yytext);
    n->line_number = yylineno;
    yylval = n;
    return IDENTIFIER;
}

"=="  { return EQ; }
"!="  { return NEQ; }
"<="  { return LE; }
">="  { return GE; }
"<"   { return LT; }
">"   { return GT; }

"+"   { return PLUS; }
"-"   { return MINUS; }
"*"   { return MULT; }
"/"   { return DIV; }

"="   { return ASSIGN; }
";"   { return SEMICOLON; }

"("   { return LPAREN; }
")"   { return RPAREN; }
"{"   { return LBRACE; }
"}"   { return RBRACE; }

[ \t\r\n]+   ;
.            { return yytext[0]; }

%%
