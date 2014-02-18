#ifndef FILE_AST_H
#define FILE_AST_H

#include "def.h"
#include <stdio.h>

typedef struct
{
    char *string;
    struct
    {
        int x, y;
    } pos;
    int depth;
} TYPEDEF_NAME_HASH;

void tree_output_ast(NODE_T *node, int depth);
NODE_T *node_t_new();
void node_t_delete(NODE_T *node);
void node_t_delete_r(NODE_T *node);
void node_t_enlarge_nxt(NODE_T *node);
NODE_T * create_node_program(int num, ...);
NODE_T * create_node_declaration(int num, ...);
NODE_T * create_node_function_definition(int num, ...);
NODE_T * create_node_parameter(int num, ...);
NODE_T * create_node_declarators(int num, ...);
NODE_T * create_node_init_declarators(int num, ...);
NODE_T * create_node_init_declarator(int num, ...);
NODE_T * create_node_initializers(int num, ...);
NODE_T * create_node_initializer(int num, ...);
NODE_T * create_node_expression(int record, int num, ...);
NODE_T * create_node_type_specifier(int record, int num, ...);
NODE_T * create_node_identifier(char *str, int num, ...);
NODE_T * create_node_const(char *str, int num, ...);
NODE_T * create_node_typedef_name(char *str, int num, ...);
NODE_T * create_node_postfix(int record, int num, ...);
NODE_T * create_node_plain_declarations(int num, ...);
NODE_T * create_node_plain_declaration(int num, ...);
NODE_T * create_node_arguments(int num, ...);
NODE_T * create_node_type_name(int record, int num, ...);
NODE_T * create_node_statements(int num, ...);
NODE_T * create_node_statement(int record, int num, ...);
NODE_T * create_node_declarator(int record, int num, ...);
NODE_T * create_node_ellipsis();
NODE_T * create_node_typedef();

void tnhash_init();
void tnhash_end();
void tnhash_enlarge();
void tnhash_add(char *string, int depth);
int tnhash_query(char *string);
void tree_build_begin_scope();
void tree_build_end_scope();
void add_typedef_name(NODE_T *node);

void tree_output_ast(NODE_T *node, int depth);

extern NODE_T *treeroot;
extern FILE *foutast, *foutcst;
#endif
