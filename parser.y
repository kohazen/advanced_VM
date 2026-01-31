%{
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"

extern ASTNode* root;
extern int yylineno; // Get line number from Lexer
int yylex();
void yyerror(const char* s);
%}

%define api.value.type {ASTNode*}

%token INTEGER IDENTIFIER VAR
%token IF ELSE WHILE
%token PRINT
%token PLUS MINUS MULT DIV ASSIGN SEMICOLON
%token EQ NEQ LT GT LE GE
%token LBRACE RBRACE LPAREN RPAREN

%expect 1

%left EQ NEQ LT GT LE GE
%left PLUS MINUS
%left MULT DIV

%%

program:
    statement_list { root = $1; }
    ;

statement_list:
    statement { $$ = $1; }
    | statement_list statement {
        $$ = createNode(NODE_SEQ, $1, $2);
        $$->line_number = yylineno;
    }
    ;

statement:
    variable_decl
    | assignment
    | if_statement
    | while_statement
    | print_statement
    | block
    | expression SEMICOLON { $$ = $1; }
    ;

block:
    LBRACE statement_list RBRACE { $$ = $2; }
    ;

variable_decl:
    VAR IDENTIFIER ASSIGN expression SEMICOLON {
        $$ = createNode(NODE_DECL, $4, NULL);
        $$->varName = $2->varName;
        $$->line_number = yylineno;
    }
    | VAR IDENTIFIER SEMICOLON {
        $$ = createNode(NODE_DECL, createIntNode(0), NULL);
        $$->varName = $2->varName;
        $$->line_number = yylineno;
    }
    ;

assignment:
    IDENTIFIER ASSIGN expression SEMICOLON {
        $$ = createNode(NODE_ASSIGN, $3, NULL);
        $$->varName = $1->varName;
        $$->line_number = yylineno;
    }
    ;

if_statement:
    IF LPAREN expression RPAREN statement {
        $$ = createNode(NODE_IF, $3, $5);
        $$->line_number = yylineno;
    }
    | IF LPAREN expression RPAREN statement ELSE statement {
        $$ = createNode(NODE_IF, $3, $5);
        $$->extra = $7;
        $$->line_number = yylineno;
    }
    ;

while_statement:
    WHILE LPAREN expression RPAREN statement {
        $$ = createNode(NODE_WHILE, $3, $5);
        $$->line_number = yylineno;
    }
    ;

/* LAB6 CHANGE: print statement rule */
print_statement:
    PRINT LPAREN expression RPAREN SEMICOLON {
        $$ = make_print($3);
        $$->line_number = yylineno;
    }
    ;

expression:
    expression PLUS expression  { $$ = createNode(NODE_OP, $1, $3); $$->value = OP_ADD; $$->line_number = yylineno; }
    | expression MINUS expression { $$ = createNode(NODE_OP, $1, $3); $$->value = OP_SUB; $$->line_number = yylineno; }
    | expression MULT expression  { $$ = createNode(NODE_OP, $1, $3); $$->value = OP_MUL; $$->line_number = yylineno; }
    | expression DIV expression   { $$ = createNode(NODE_OP, $1, $3); $$->value = OP_DIV; $$->line_number = yylineno; }

    | expression EQ expression    { $$ = createNode(NODE_OP, $1, $3); $$->value = OP_EQ; $$->line_number = yylineno; }
    | expression NEQ expression   { $$ = createNode(NODE_OP, $1, $3); $$->value = OP_NEQ; $$->line_number = yylineno; }
    | expression LT expression    { $$ = createNode(NODE_OP, $1, $3); $$->value = OP_LT; $$->line_number = yylineno; }
    | expression GT expression    { $$ = createNode(NODE_OP, $1, $3); $$->value = OP_GT; $$->line_number = yylineno; }
    | expression LE expression    { $$ = createNode(NODE_OP, $1, $3); $$->value = OP_LE; $$->line_number = yylineno; }
    | expression GE expression    { $$ = createNode(NODE_OP, $1, $3); $$->value = OP_GE; $$->line_number = yylineno; }

    | LPAREN expression RPAREN  { $$ = $2; }
    | INTEGER                   { $$ = $1; }
    | IDENTIFIER                { $$ = $1; }
    ;

%%

void yyerror(const char* s) {
    fprintf(stderr, "Syntax Error at line %d: %s\n", yylineno, s);
}
