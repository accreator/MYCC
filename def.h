#ifndef FILE_DEF_H
#define FILE_DEF_H
typedef struct NODE_TREE
{
    enum
    {
        PROGRAM,
        DECLARATION,
        FUNCTION_DEFINITION,
        PARAMETER,
        DECLARATORS,
        INIT_DECLARATORS,
        INIT_DECLARATOR,
        INITIALIZER,
        EXPRESSIONS,
        EXPRESSION,
        TYPE_SPECIFIER,
        POSTFIX,
        PLAIN_DECLARATIONS,
        PLAIN_DECLARATION,
        ARGUMENTS,
        TYPE_NAME,
        STATEMENTS,
        STATEMENT,
        DECLARATOR,
        TYPEDEFNAME,
        IDENTIFIER,
        CONST,
        TYPEDEF,
        ELLIPSIS
    } type;
    union
    {
        int integer;
        int character;
        char *string;
    } record;
    struct
    {
        int x, y;
    } pos;
    int nxtnum;
    int nxtsize;
    struct NODE_TREE **nxt;
    struct NODE_TYPE *selftype;
    struct NODE_TRANS *trans;
} NODE_T;

#define INC_OP_U 500
#define DEC_OP_U 501
#define CAST_EXP 502
#define BRACKET_EXP 503
#define POSTFIX_EXP 504
#define EXP_STMT 505

//#define NOT_USE_FUNC_1
//#define NOT_USE_FUNC_2
//#define NOT_USE_FUNC_3
//#define NOT_USE_FUNC_4
//#define NOT_USE_FUNC_5
//#define NOT_USE_FUNC_6
//#define NOT_USE_FUNC_7
//#define NOT_USE_FUNC_8
//#define NOT_USE_FUNC_9
//#define NOT_USE_FUNC_10
//#define NOT_USE_FUNC_11
//#define USE_VLA
//#define OUTPUT_AST
//#define OUTPUT_TOKENS
#define EASY_INTERPRETER
#define OUTPUT_TRANS
#define FINAL_TEST
#endif
