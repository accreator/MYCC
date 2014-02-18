%{
/* Please read README for more information */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "def.h"
#include "ast.h"
#include "utility.h"
#include "translate.h"
#include "semantics.h"

typedef union
{
    NODE_T *node;
    int integer;
    int character;
    char *string;
} yystype;

#define YYSTYPE yystype
%}

%token TYPEDEF_T VOID_T CHAR_T INT_T STRUCT_T UNION_T TYPEDEF_NAME
%token IF_C ELSE_C WHILE_C FOR_C CONTINUE_C BREAK_C RETURN_C
%token SIZEOF_OP
%token IDENTIFIER_O INT_CONSTANT CHAR_CONSTANT STRING_CONSTANT
%token SHL_ASSIGN SHR_ASSIGN MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN ADD_ASSIGN SUB_ASSIGN AND_ASSIGN XOR_ASSIGN OR_ASSIGN
%token OR_OP AND_OP EQ_OP NE_OP LE_OP GE_OP SHL_OP SHR_OP INC_OP DEC_OP PTR_OP ELLIPSIS_O

%left ','
%right '=' SHL_ASSIGN SHR_ASSIGN MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN ADD_ASSIGN SUB_ASSIGN AND_ASSIGN XOR_ASSIGN OR_ASSIGN
%left OR_OP
%left AND_OP
%left '|'
%left '^'
%left EQ_OP NE_OP
%left '<' '>' LE_OP GE_OP
%left '*' '/' '%'
%right SIZEOF_OP '!' '~'
%%

program
    : declaration                   { treeroot = $$.node = create_node_program(1, $1.node); }
    | function_definition           { treeroot = $$.node = create_node_program(1, $1.node); }
    | program declaration           { treeroot = $$.node = create_node_program(2, $1.node, $2.node); }
    | program function_definition   { treeroot = $$.node = create_node_program(2, $1.node, $2.node); }
    ;
declaration
    : TYPEDEF_T type_specifier declarators ';'      { $$.node = create_node_declaration(3, create_node_typedef(), $2.node, $3.node); add_typedef_name($3.node); }
    | type_specifier init_declarators ';'           { $$.node = create_node_declaration(2, $1.node, $2.node); }
    | type_specifier ';'                            { $$.node = create_node_declaration(1, $1.node); }
    ;
function_definition
    : type_specifier plain_declarator '(' parameters ')' compound_statement     { $$.node = create_node_function_definition(4, $1.node, $2.node, $4.node, $6.node); }
    | type_specifier plain_declarator '(' ')' compound_statement                { $$.node = create_node_function_definition(3, $1.node, $2.node, $5.node); }
    ;
parameters
    : parameters_ne                 { $$.node = $1.node; }
    | parameters_ne ',' ELLIPSIS_O  { $$.node = create_node_parameter(2, $1.node, create_node_ellipsis()); }
    ;
parameters_ne
    : plain_declaration                     { $$.node = create_node_parameter(1, $1.node); }
    | parameters_ne ',' plain_declaration   { $$.node = create_node_parameter(2, $1.node, $3.node); }
    ;
declarators
    : declarator                    { $$.node = create_node_declarators(1, $1.node); }
    | declarators ',' declarator    { $$.node = create_node_declarators(2, $1.node, $3.node); }
    ;
init_declarators
    : init_declarator                       { $$.node = create_node_init_declarators(1, $1.node); }
    | init_declarators ',' init_declarator  { $$.node = create_node_init_declarators(2, $1.node, $3.node); }
    ;
init_declarator
    : declarator '=' initializer    { $$.node = create_node_init_declarator(2, $1.node, $3.node); }
    | declarator                    { $$.node = create_node_init_declarator(1, $1.node); }
    ;
initializer
    : assignment_expression     { $$.node = $1.node; }
    | '{' initializer_p '}'     { $$.node = create_node_initializer(1, $2.node); }
    ;
initializer_p
    : initializer                   { $$.node = create_node_initializers(1, $1.node); }
    | initializer_p ',' initializer { $$.node = create_node_initializers(2, $1.node, $3.node); }
    ;
type_specifier
    : VOID_T                                                    { $$.node = create_node_type_specifier(VOID_T, 0); }
    | CHAR_T                                                    { $$.node = create_node_type_specifier(CHAR_T, 0); }
    | INT_T                                                     { $$.node = create_node_type_specifier(INT_T, 0); }
    | TYPEDEF_NAME                                              { $$.node = create_node_type_specifier(TYPEDEF_NAME, 1, create_node_typedef_name($1.string, 0)); free($1.string); }
    | struct_or_union IDENTIFIER_O '{' plain_declarations '}'   { $$.node = create_node_type_specifier($1.integer, 2, create_node_identifier($2.string, 0), $4.node); free($2.string); }
    | struct_or_union '{' plain_declarations '}'                { $$.node = create_node_type_specifier($1.integer, 1, $3.node); }
    | struct_or_union IDENTIFIER_O                              { $$.node = create_node_type_specifier($1.integer, 1, create_node_identifier($2.string, 0)); free($2.string); }
    ;
plain_declarations
    : plain_declaration ';'                     { $$.node = create_node_plain_declarations(1, $1.node); }
    | plain_declarations plain_declaration ';'  { $$.node = create_node_plain_declarations(2, $1.node, $2.node); }
    ;
struct_or_union
    : STRUCT_T  { $$.integer = STRUCT_T; }
    | UNION_T   { $$.integer = UNION_T; }
    ;
plain_declaration
    : type_specifier declarator  { $$.node = create_node_plain_declaration(2, $1.node, $2.node); }
    ;
declarator
    : plain_declarator '(' parameters ')'   { $$.node = create_node_declarator('(', 2, create_node_declarator(0, 1, $1.node), $3.node); }
    | plain_declarator '(' ')'              { $$.node = create_node_declarator('(', 1, $1.node); }
    | declarator_ce                         { $$.node = $1.node; }
    ;
declarator_ce
    : plain_declarator                          { $$.node = create_node_declarator(0, 1, $1.node); }
    | declarator_ce '[' constant_expression ']' { $$.node = create_node_declarator('[', 2, create_node_declarator(0, 1, $1.node), $3.node); }
    ;
plain_declarator
    : IDENTIFIER_O          { $$.node = create_node_identifier($1.string, 0); free($1.string); }
    | '*' plain_declarator  { $$.node = create_node_declarator('*', 1, $2.node); }
    ;
statement
    : expression_statement  { $$.node = $1.node; }
    | compound_statement    { $$.node = $1.node; }
    | selection_statement   { $$.node = $1.node; }
    | iteration_statement   { $$.node = $1.node; }
    | jump_statement        { $$.node = $1.node; }
    ;
expression_statement
    : expression ';'    { $$.node = create_node_statement(EXP_STMT, 1, $1); }
    | ';'               { $$.node = create_node_statement(EXP_STMT, 0); }
    ;
compound_statement
    : begin_scope compound_statement_d compound_statement_s end_scope { $$.node = create_node_statements(2, create_node_statements(2, create_node_statements(2, create_node_statements(1, create_node_statement('{', 0)), $2.node), $3.node), create_node_statement('}', 0)); }
    | begin_scope compound_statement_d end_scope                      { $$.node = create_node_statements(2, create_node_statements(2, create_node_statements(1, create_node_statement('{', 0)), $2.node), create_node_statement('}', 0)); }
    | begin_scope compound_statement_s end_scope                      { $$.node = create_node_statements(2, create_node_statements(2, create_node_statements(1, create_node_statement('{', 0)), $2.node), create_node_statement('}', 0)); }
    | begin_scope end_scope                                           { $$.node = create_node_statements(2, create_node_statements(1, create_node_statement('{', 0)), create_node_statement('}', 0)); }
    ;
begin_scope
    : '{'   { tree_build_begin_scope(); }
    ;
end_scope
    : '}'   { tree_build_end_scope(); }
    ;
compound_statement_d
    : declaration                       { $$.node = create_node_statements(1, $1.node); }
    | compound_statement_d declaration  { $$.node = create_node_statements(2, $1.node, $2.node); }
    ;
compound_statement_s
    : statement                         { $$.node = create_node_statements(1, $1.node); }
    | compound_statement_s statement    { $$.node = create_node_statements(2, $1.node, $2.node); }
    ;
selection_statement
    : IF_C '(' expression ')' statement ELSE_C statement    { $$.node = create_node_statement(IF_C, 3, $3.node, $5.node, $7.node); }
    | IF_C '(' expression ')' statement                     { $$.node = create_node_statement(IF_C, 2, $3.node, $5.node); }
    ;
iteration_statement
    : WHILE_C '(' expression ')' statement                                          { $$.node = create_node_statement(WHILE_C, 2, $3.node, $5.node); }
    | FOR_C '(' expression_statement expression_statement expression ')' statement  { $$.node = create_node_statement(FOR_C, 4, $3.node, $4.node, $5.node, $7.node); }
    | FOR_C '(' expression_statement expression_statement ')' statement             { $$.node = create_node_statement(FOR_C, 3, $3.node, $4.node, $6.node); }
    ;
jump_statement
    : CONTINUE_C ';'            { $$.node = create_node_statement(CONTINUE_C, 0); }
    | BREAK_C ';'               { $$.node = create_node_statement(BREAK_C, 0); }
    | RETURN_C expression ';'   { $$.node = create_node_statement(RETURN_C, 1, $2.node); }
    | RETURN_C ';'              { $$.node = create_node_statement(RETURN_C, 0); }
    ;
expression
    : assignment_expression                 { $$.node = $1.node; }
    | assignment_expression ',' expression  { $$.node = create_node_expression(',', 2, $1.node, $3.node); }
    ;
assignment_expression
    : logical_or_expression                                         { $$.node = $1.node; }
    | unary_expression assignment_operator assignment_expression    { $$.node = create_node_expression($2.integer, 2, $1.node, $3.node); }
    ;
assignment_operator
    : '='           { $$.integer = '='; }
    | MUL_ASSIGN    { $$.integer = MUL_ASSIGN; }
    | DIV_ASSIGN    { $$.integer = DIV_ASSIGN; }
    | MOD_ASSIGN    { $$.integer = MOD_ASSIGN; }
    | ADD_ASSIGN    { $$.integer = ADD_ASSIGN; }
    | SUB_ASSIGN    { $$.integer = SUB_ASSIGN; }
    | SHL_ASSIGN    { $$.integer = SHL_ASSIGN; }
    | SHR_ASSIGN    { $$.integer = SHR_ASSIGN; }
    | AND_ASSIGN    { $$.integer = AND_ASSIGN; }
    | XOR_ASSIGN    { $$.integer = XOR_ASSIGN; }
    | OR_ASSIGN     { $$.integer = OR_ASSIGN; }
    ;
constant_expression
    : logical_or_expression { $$.node = $1.node; }
    ;
logical_or_expression
    : logical_and_expression                                { $$.node = $1.node; }
    | logical_or_expression OR_OP logical_and_expression    { $$.node = create_node_expression(OR_OP, 2, $1.node, $3.node); }
    ;
logical_and_expression
    : inclusive_or_expression                               { $$.node = $1.node; }
    | logical_and_expression AND_OP inclusive_or_expression { $$.node = create_node_expression(AND_OP, 2, $1.node, $3.node); }
    ;
inclusive_or_expression
    : exclusive_or_expression                               { $$.node = $1.node; }
    | inclusive_or_expression '|' exclusive_or_expression   { $$.node = create_node_expression('|', 2, $1.node, $3.node); }
    ;
exclusive_or_expression
    : and_expression                                { $$.node = $1.node; }
    | exclusive_or_expression '^' and_expression    { $$.node = create_node_expression('^', 2, $1.node, $3.node); }
    ;
and_expression
    : equality_expression                       { $$.node = $1.node; }
    | and_expression '&' equality_expression    { $$.node = create_node_expression('&', 2, $1.node, $3.node); }
    ;
equality_expression
    : relational_expression                                         { $$.node = $1.node; }
    | equality_expression equality_operator relational_expression   { $$.node = create_node_expression($2.integer, 2, $1.node, $3.node); }
    ;
equality_operator
    : EQ_OP { $$.integer = EQ_OP; }
    | NE_OP { $$.integer = NE_OP; }
    ;
relational_expression
    : shift_expression                                              { $$.node = $1.node; }
    | relational_expression relational_operator shift_expression    { $$.node = create_node_expression($2.integer, 2, $1.node, $3.node); }
    ;
relational_operator
    : '<'       { $$.integer = '<'; }
    | '>'       { $$.integer = '>'; }
    | LE_OP     { $$.integer = LE_OP; }
    | GE_OP     { $$.integer = GE_OP; }
    ;
shift_expression
    : additive_expression                                   { $$.node = $1.node; }
    | shift_expression shift_operator additive_expression   { $$.node = create_node_expression($2.integer, 2, $1.node, $3.node); }
    ;
shift_operator
    : SHL_OP    { $$.integer = SHL_OP; }
    | SHR_OP    { $$.integer = SHR_OP; }
    ;
additive_expression
    : multiplicative_expression                                         { $$.node = $1.node; }
    | additive_expression additive_operator multiplicative_expression   { $$.node = create_node_expression($2.integer, 2, $1.node, $3.node); }
    ;
additive_operator
    : '+'   { $$.integer = '+'; }
    | '-'   { $$.integer = '-'; }
    ;
multiplicative_expression
    : cast_expression                                                       { $$.node = $1.node; }
    | multiplicative_expression multiplicative_operator cast_expression     { $$.node = create_node_expression($2.integer, 2, $1.node, $3.node); }
    ;
multiplicative_operator
    : '*'   { $$.integer = '*'; }
    | '/'   { $$.integer = '/'; }
    | '%'   { $$.integer = '%'; }
    ;
cast_expression
    : unary_expression                  { $$.node = $1.node; }
    | '(' type_name ')' cast_expression { $$.node = create_node_expression(CAST_EXP, 2, $2.node, $4.node); }
    ;
type_name
    : type_specifier    { $$.node = create_node_type_name(0, 1, $1.node); }
    | type_name '*'     { $$.node = create_node_type_name('*', 1, $1.node); }
    ;
unary_expression
    : postfix_expression                { $$.node = $1.node; }
    | INC_OP unary_expression           { $$.node = create_node_expression(INC_OP_U, 1, $2.node); }
    | DEC_OP unary_expression           { $$.node = create_node_expression(DEC_OP_U, 1, $2.node); }
    | unary_operator cast_expression    { $$.node = create_node_expression($1.integer, 1, $2.node); }
    | SIZEOF_OP unary_expression        { $$.node = create_node_expression(SIZEOF_OP, 1, $2.node); }
    | SIZEOF_OP '(' type_name ')'       { $$.node = create_node_expression(SIZEOF_OP, 1, $3.node); }
    ;
unary_operator
    : '&'   { $$.integer = '&'; }
    | '*'   { $$.integer = '*'; }
    | '+'   { $$.integer = '+'; }
    | '-'   { $$.integer = '-'; }
    | '~'   { $$.integer = '~'; }
    | '!'   { $$.integer = '!'; }
    ;
postfix_expression
    : primary_expression            { $$.node = $1.node; }
    | postfix_expression postfix    { $$.node = create_node_expression(POSTFIX_EXP, 2, $1.node, $2.node); }
    ;
postfix
    : '[' expression ']'    { $$.node = create_node_postfix('[', 1, $2.node); }
    | '(' arguments ')'     { $$.node = create_node_postfix('(', 1, $2.node); }
    | '(' ')'               { $$.node = create_node_postfix('(', 0); }
    | '.' IDENTIFIER_O      { $$.node = create_node_postfix('.', 1, create_node_identifier($2.string, 0)); free($2.string); }
    | PTR_OP IDENTIFIER_O   { $$.node = create_node_postfix(PTR_OP, 1, create_node_identifier($2.string, 0)); free($2.string); }
    | INC_OP                { $$.node = create_node_postfix(INC_OP, 0); }
    | DEC_OP                { $$.node = create_node_postfix(DEC_OP, 0); }
    ;
arguments
    : assignment_expression                 { $$.node = create_node_arguments(1, $1.node); }
    | arguments ',' assignment_expression   { $$.node = create_node_arguments(2, $1.node, $3.node); }
    ;
primary_expression
    : IDENTIFIER_O          { $$.node = create_node_identifier($1.string, 0); }
    | constant              { $$.node = $1.node; }
    | STRING_CONSTANT       { $$.node = create_node_const($1.string, 0); free($1.string); }
    | '(' expression ')'    { $$.node = create_node_expression(BRACKET_EXP, 1, $2.node); }
    ;
constant
    : INT_CONSTANT      { $$.node = create_node_const($1.string, 0); free($1.string); }
    | CHAR_CONSTANT     { $$.node = create_node_const($1.string, 0); free($1.string); }
    ;
    
%%
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "lex.yy.c"
#include "def.h"
#include "ast.h"
#include "utility.h"
#include "semantics.h"

void init_myc(int argc, char *argv[])
{
    char fn[80];
    if(argc >= 3)
        reporterrorflag = 1;
    else
        reporterrorflag = 0;
    tnhash_init();    
#ifdef OUTPUT_AST
    sprintf(fn, "%s.ast", argv[1]);
    foutast = fopen(fn, "w");
#endif
#ifdef OUTPUT_TOKENS
    sprintf(fn, "%s.tokens", argv[1]);
    yyout = fopen(fn, "w");
#endif
#ifdef OUTPUT_TRANS
#ifndef FINAL_TEST
    sprintf(fn, "%s.trans", argv[1]);
    fouttrans = fopen(fn, "w");
#else
    fouttrans = stdout;
#endif
#endif
    yyin = fopen(argv[1], "r");
}

void exit_myc()
{
    tnhash_end();
    
    fclose(yyin);
#ifdef OUTPUT_TOKENS
    fclose(yyout);
#endif
#ifdef OUTPUT_AST
    fclose(foutast);
#endif
#ifdef OUTPUT_TRANS
    fclose(fouttrans);
#endif
}

void main_myc()
{
    if(yyparse())
    {
        report_error(-1, "Parser failed", "");
    }
    else
    {
#ifndef FINAL_TEST
        printf("Done\n");
#endif
    }
#ifdef OUTPUT_AST
    tree_output_ast(treeroot, 0);
    fprintf(foutast, "\n");
#endif
    tree_semantic_check_init();    
    tree_semantic_check(treeroot);
    tree_semantic_check_end();
}

int main(int argc, char *argv[])
{
    init_myc(argc, argv);
    main_myc();
    exit_myc();
    return 0;
}

void yyerror(char *str)
{
    report_error(yylineno, str, yytext);
}

