#include<stdio.h>
#ifndef FILE_SEMANTICS_H
#define FILE_SEMANTICS_H

struct NODE_TYPE;
struct NODE_ENV;

typedef enum
{
    TYPE,
    
    VOID,
    CHAR,
    INT,
    
    NAME,
    POINTER,
    ARRAY,
    STRUCT,
    UNION,
    FUNCTION
} ENUM_NODE_TYPE;

typedef struct
{
    char *name;
    struct NODE_ENV *env;
} NODE_TYPE_NAME;

typedef struct
{
    int capacity;
    struct NODE_TYPE *pointer;
} NODE_TYPE_ARRAY; 

typedef struct
{
    struct NODE_TYPE *type;
    char *name;
} NODE_TYPE_RECORD_FIELD;

typedef struct
{
    int size;
    NODE_TYPE_RECORD_FIELD *field;
} NODE_TYPE_RECORD;

typedef struct
{
    int size;
    struct NODE_TYPE *argumenttype;
    struct NODE_TYPE *returntype;
} NODE_TYPE_FUNCTION;

typedef struct NODE_TYPE
{
    ENUM_NODE_TYPE type;
    union
    {
        NODE_TYPE_NAME name;
        struct NODE_TYPE *pointer;
        NODE_TYPE_ARRAY array;
        NODE_TYPE_RECORD record;
        NODE_TYPE_FUNCTION function;
    } record;
    int size;
    int info;
    char isconst;
    char *id;
} NODE_TYPE; //若修改需要检查所有的copy函数

typedef struct
{
    enum { TYPEENTRY, FUNENTRY, VARENTRY } entrytype;
    char *symbol;
    NODE_TYPE *type;
    struct NODE_TRANS_RECORD rec;
    int def;
} NODE_ENTRY;

typedef struct NODE_ENV
{
    NODE_TYPE *selftype;
    int ventrysize;
    int ventrynum;
    NODE_ENTRY *ventry;
    int tentrysize;
    int tentrynum;
    NODE_ENTRY *tentry;
} NODE_ENV;

NODE_TYPE * node_type_name_new(char *name, struct NODE_ENV *env);
NODE_TYPE * node_type_copy_new(NODE_TYPE *x);
NODE_TYPE * node_type_copy_r_new(NODE_TYPE *x);
void node_type_cal_size_r(NODE_TYPE *x);
int node_type_cal_size_r_v(NODE_TYPE *x);
NODE_TYPE * node_type_type_new();
NODE_TYPE * node_type_void_new();
NODE_TYPE * node_type_int_new();
NODE_TYPE * node_type_char_new();
NODE_TYPE * node_type_pointer_new(NODE_TYPE *p);
NODE_TYPE * node_type_array_new(int capacity, NODE_TYPE *p);
NODE_TYPE * node_type_struct_new(int size, NODE_TYPE_RECORD_FIELD *p);
NODE_TYPE * node_type_union_new(int size, NODE_TYPE_RECORD_FIELD *p);
NODE_TYPE * node_type_function_new(int size, NODE_TYPE *argumenttype, NODE_TYPE *returntype);
void node_type_delete(NODE_TYPE *node);
NODE_ENV * node_env_new();
void node_env_enlarge(NODE_ENV *node);
void node_env_delete(NODE_ENV *node);
void tscenv_enlarge();
void tree_semantic_check_begin_scope();
void tree_semantic_check_end_scope();
void tree_semantic_check_hold_begin_scope();
void tree_semantic_check_hold_end_scope();
void tree_semantic_check_type_specifier_record(NODE_T *node);
int tree_semantic_check_same_type(NODE_TYPE *x, NODE_TYPE *y);
NODE_TYPE * tree_semantic_check_symbol(char *id);
NODE_TYPE * tree_semantic_check_symbol_ventry(char *id);
NODE_TYPE * tree_semantic_check_symbol_fun(char *id);
NODE_TYPE * tree_semantic_check_symbol_top_ventry(char *id);
NODE_TYPE * tree_semantic_check_symbol_top(char *id);
NODE_TYPE * tree_semantic_check_symbol_top_def(char *id);
void tree_semantic_check_add_funentry(char *id, NODE_TYPE *type, int flag);
void tree_semantic_check_add_typeentry(char *id, NODE_TYPE *type);
void tree_semantic_check_add_typeentry_n(char *id, NODE_TYPE *type);
void tree_semantic_check_add_varentry(char *id, NODE_TYPE *type);
void tree_semantic_check_fetch_identifier_type(NODE_T *node);
void tree_semantic_check_fetch_function_type(NODE_T *node);
void tree_semantic_check_plain_declaration(NODE_T *node);
void tree_semantic_check_declaration_2(NODE_T *node);
void tree_semantic_check_declaration_3(NODE_T *node);
void tree_semantic_check_funtion_definition(NODE_T *node);
void tree_semantic_check_init_declarator(NODE_T *node, int flag, NODE_TYPE *type, NODE_T *root);
void tree_semantic_check(NODE_T *node);
void tree_semantic_check_init();
void tree_semantic_check_end();

extern NODE_ENV *tscenv;
extern int tscenvnum;
extern int tscenvsize;

extern int tscaddfunentryflag;
extern char *tscaddfunentryid;
#endif
