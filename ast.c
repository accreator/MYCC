#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "y.tab.h"
#include "def.h"
#include "ast.h"
#include "utility.h"

NODE_T *treeroot;
FILE *foutast, *foutcst;

NODE_T *node_t_new()
{
    NODE_T *node;
    node = (NODE_T *)malloc(sizeof(NODE_T));
    node->nxtsize = 1;
    node->nxt = (NODE_T **)malloc(sizeof(NODE_T *));
    node->nxtnum = 0;
    return node;
}
void node_t_delete(NODE_T *node)
{
    free(node->nxt);
    free(node);
}
void node_t_delete_r(NODE_T *node)
{
    int i;
    for(i=0; i<node->nxtnum; i++)
    {
        node_t_delete_r(node->nxt[i]);
    }
    node_t_delete(node);
}
void node_t_enlarge_nxt(NODE_T *node)
{
    NODE_T **p;
    int i;
    if(node->nxtnum < node->nxtsize) return;
    while(node->nxtnum >= node->nxtsize) node->nxtsize *= 2;
    p = (NODE_T **)malloc(sizeof(NODE_T *) * node->nxtsize);
    for(i=0; i<node->nxtnum; i++)
    {
        p[i] = node->nxt[i];
    }
    free(node->nxt);
    node->nxt = p;
}

NODE_T * create_node_program(int num, ...)
{
    va_list ap;
    NODE_T *node;
    va_start(ap, num);
    if(num == 0)
    {
        node = node_t_new();
        node->type = PROGRAM;
        node->nxtnum = 0;
    }
    else if(num == 1)
    {
        node = node_t_new();
        node->type = PROGRAM;
        node_t_enlarge_nxt(node);
        node->nxt[0] = va_arg(ap, NODE_T*);
        node->nxtnum = 1;
    }
    else
    {
        node = va_arg(ap, NODE_T*);
        node_t_enlarge_nxt(node);
        node->nxt[node->nxtnum] = va_arg(ap, NODE_T*);
        node->nxtnum ++;
    }
    return node;
}

NODE_T * create_node_declaration(int num, ...)
{
    va_list ap;
    int i;
    NODE_T *node;
    va_start(ap, num);
    node = node_t_new();
    node->type = DECLARATION;
    node->nxtnum = num;
    for(i=0; i<num; i++)
    {
        node_t_enlarge_nxt(node);
        node->nxt[i] = va_arg(ap, NODE_T*);
    }
    return node;
}

NODE_T * create_node_function_definition(int num, ...)
{
    va_list ap;
    int i;
    NODE_T *node;
    va_start(ap, num);
    node = node_t_new();
    node->type = FUNCTION_DEFINITION;
    node->nxtnum = num;
    for(i=0; i<num; i++)
    {
        node_t_enlarge_nxt(node);
        node->nxt[i] = va_arg(ap, NODE_T*);
    }
    return node;
}

NODE_T * create_node_parameter(int num, ...)
{
    va_list ap;
    NODE_T *node;
    va_start(ap, num);
    if(num == 0)
    {
        node = node_t_new();
        node->type = PARAMETER;
        node->nxtnum = 0;
    }
    else if(num == 1)
    {
        node = node_t_new();
        node->type = PARAMETER;
        node_t_enlarge_nxt(node);
        node->nxt[0] = va_arg(ap, NODE_T*);
        node->nxtnum = 1;
    }
    else
    {
        node = va_arg(ap, NODE_T*);
        node_t_enlarge_nxt(node);
        node->nxt[node->nxtnum] = va_arg(ap, NODE_T*);
        node->nxtnum ++;
    }
    return node;
}

NODE_T * create_node_declarators(int num, ...)
{
    va_list ap;
    NODE_T *node;
    va_start(ap, num);
    if(num == 0)
    {
        node = node_t_new();
        node->type = DECLARATORS;
        node->nxtnum = 0;
    }
    else if(num == 1)
    {
        node = node_t_new();
        node->type = DECLARATORS;
        node_t_enlarge_nxt(node);
        node->nxt[0] = va_arg(ap, NODE_T*);
        node->nxtnum = 1;
    }
    else
    {
        node = va_arg(ap, NODE_T*);
        node_t_enlarge_nxt(node);
        node->nxt[node->nxtnum] = va_arg(ap, NODE_T*);
        node->nxtnum ++;
    }
    return node;
}

NODE_T * create_node_init_declarators(int num, ...)
{
    va_list ap;
    NODE_T *node;
    va_start(ap, num);
    if(num == 0)
    {
        node = node_t_new();
        node->type = INIT_DECLARATORS;
        node->nxtnum = 0;
    }
    else if(num == 1)
    {
        node = node_t_new();
        node->type = INIT_DECLARATORS;
        node_t_enlarge_nxt(node);
        node->nxt[0] = va_arg(ap, NODE_T*);
        node->nxtnum = 1;
    }
    else
    {
        node = va_arg(ap, NODE_T*);
        node_t_enlarge_nxt(node);
        node->nxt[node->nxtnum] = va_arg(ap, NODE_T*);
        node->nxtnum ++;
    }
    return node;
}

NODE_T * create_node_init_declarator(int num, ...)
{
    va_list ap;
    int i;
    NODE_T *node;
    va_start(ap, num);
    node = node_t_new();
    node->type = INIT_DECLARATOR;
    node->nxtnum = num;
    for(i=0; i<num; i++)
    {
        node_t_enlarge_nxt(node);
        node->nxt[i] = va_arg(ap, NODE_T*);
    }
    return node;
}

NODE_T * create_node_initializers(int num, ...)
{
    va_list ap;
    NODE_T *node;
    va_start(ap, num);
    if(num == 0)
    {
        node = node_t_new();
        node->type = EXPRESSIONS;
        node->nxtnum = 0;
    }
    else if(num == 1)
    {
        node = node_t_new();
        node->type = EXPRESSIONS;
        node_t_enlarge_nxt(node);
        node->nxt[0] = va_arg(ap, NODE_T*);
        node->nxtnum = 1;
    }
    else
    {
        node = va_arg(ap, NODE_T*);
        node_t_enlarge_nxt(node);
        node->nxt[node->nxtnum] = va_arg(ap, NODE_T*);
        node->nxtnum ++;
    }
    return node;
}

NODE_T * create_node_initializer(int num, ...)
{
    va_list ap;
    int i;
    NODE_T *node;
    va_start(ap, num);
    node = node_t_new();
    node->type = INITIALIZER;
    node->nxtnum = num;
    for(i=0; i<num; i++)
    {
        node_t_enlarge_nxt(node);
        node->nxt[i] = va_arg(ap, NODE_T*);
    }
    return node;
}

NODE_T * create_node_expression(int record, int num, ...)
{
    va_list ap;
    int i;
    NODE_T *node;
    va_start(ap, num);
    node = node_t_new();
    node->type = EXPRESSION;
    (node->record).integer = record;
    node->nxtnum = num;
    for(i=0; i<num; i++)
    {
        node_t_enlarge_nxt(node);
        node->nxt[i] = va_arg(ap, NODE_T*);
    }
    return node;
}

NODE_T * create_node_type_specifier(int record, int num, ...)
{
    va_list ap;
    int i;
    NODE_T *node;
    va_start(ap, num);
    node = node_t_new();
    node->type = TYPE_SPECIFIER;
    (node->record).integer = record;
    node->nxtnum = num;
    for(i=0; i<num; i++)
    {
        node_t_enlarge_nxt(node);
        node->nxt[i] = va_arg(ap, NODE_T*);
    }
    return node;
}

NODE_T * create_node_identifier(char *str, int num, ...)
{
    va_list ap;
    int i;
    NODE_T *node;
    va_start(ap, num);
    node = node_t_new();
    node->type = IDENTIFIER;
    (node->record).string = strdup(str);
    node->nxtnum = num;
    for(i=0; i<num; i++)
    {
        node_t_enlarge_nxt(node);
        node->nxt[i] = va_arg(ap, NODE_T*);
    }
    return node;
}

NODE_T * create_node_const(char *str, int num, ...)
{
    va_list ap;
    int i;
    NODE_T *node;
    va_start(ap, num);
    node = node_t_new();
    node->type = CONST;
    (node->record).string = strdup(str);
    node->nxtnum = num;
    for(i=0; i<num; i++)
    {
        node_t_enlarge_nxt(node);
        node->nxt[i] = va_arg(ap, NODE_T*);
    }
    return node;
}

NODE_T * create_node_typedef_name(char *str, int num, ...)
{
    va_list ap;
    int i;
    NODE_T *node;
    va_start(ap, num);
    node = node_t_new();
    node->type = TYPEDEFNAME;
    (node->record).string = strdup(str);
    node->nxtnum = num;
    for(i=0; i<num; i++)
    {
        node_t_enlarge_nxt(node);
        node->nxt[i] = va_arg(ap, NODE_T*);
    }
    return node;
}

NODE_T * create_node_postfix(int record, int num, ...)
{
    va_list ap;
    int i;
    NODE_T *node;
    va_start(ap, num);
    node = node_t_new();
    node->type = POSTFIX;
    (node->record).integer = record;
    node->nxtnum = num;
    for(i=0; i<num; i++)
    {
        node_t_enlarge_nxt(node);
        node->nxt[i] = va_arg(ap, NODE_T*);
    }
    return node;
}

NODE_T * create_node_plain_declarations(int num, ...)
{
    va_list ap;
    NODE_T *node;
    va_start(ap, num);
    if(num == 0)
    {
        node = node_t_new();
        node->type = PLAIN_DECLARATIONS;
        node->nxtnum = 0;
    }
    else if(num == 1)
    {
        node = node_t_new();
        node->type = PLAIN_DECLARATIONS;
        node_t_enlarge_nxt(node);
        node->nxt[0] = va_arg(ap, NODE_T*);
        node->nxtnum = 1;
    }
    else
    {
        node = va_arg(ap, NODE_T*);
        node_t_enlarge_nxt(node);
        node->nxt[node->nxtnum] = va_arg(ap, NODE_T*);
        node->nxtnum ++;
    }
    return node;
}

NODE_T * create_node_plain_declaration(int num, ...)
{
    va_list ap;
    int i;
    NODE_T *node;
    va_start(ap, num);
    node = node_t_new();
    node->type = PLAIN_DECLARATION;
    node->nxtnum = num;
    for(i=0; i<num; i++)
    {
        node_t_enlarge_nxt(node);
        node->nxt[i] = va_arg(ap, NODE_T*);
    }
    return node;
}

NODE_T * create_node_arguments(int num, ...)
{
    va_list ap;
    NODE_T *node;
    va_start(ap, num);
    if(num == 0)
    {
        node = node_t_new();
        node->type = ARGUMENTS;
        node->nxtnum = 0;
    }
    else if(num == 1)
    {
        node = node_t_new();
        node->type = ARGUMENTS;
        node_t_enlarge_nxt(node);
        node->nxt[0] = va_arg(ap, NODE_T*);
        node->nxtnum = 1;
    }
    else
    {
        node = va_arg(ap, NODE_T*);
        node_t_enlarge_nxt(node);
        node->nxt[node->nxtnum] = va_arg(ap, NODE_T*);
        node->nxtnum ++;
    }
    return node;
}

NODE_T * create_node_type_name(int record, int num, ...)
{
    va_list ap;
    int i;
    NODE_T *node;
    va_start(ap, num);
    node = node_t_new();
    node->type = TYPE_NAME;
    (node->record).integer = record;
    node->nxtnum = num;
    for(i=0; i<num; i++)
    {
        node_t_enlarge_nxt(node);
        node->nxt[i] = va_arg(ap, NODE_T*);
    }
    return node;
}

NODE_T * create_node_statements(int num, ...)
{
    va_list ap;
    NODE_T *node;
    va_start(ap, num);
    if(num == 0)
    {
        node = node_t_new();
        node->type = STATEMENTS;
        node->nxtnum = 0;
    }
    else if(num == 1)
    {
        node = node_t_new();
        node->type = STATEMENTS;
        node_t_enlarge_nxt(node);
        node->nxt[0] = va_arg(ap, NODE_T*);
        node->nxtnum = 1;
    }
    else
    {
        node = va_arg(ap, NODE_T*);
        node_t_enlarge_nxt(node);
        node->nxt[node->nxtnum] = va_arg(ap, NODE_T*);
        node->nxtnum ++;
    }
    return node;
}

NODE_T * create_node_statement(int record, int num, ...)
{
    va_list ap;
    int i;
    NODE_T *node;
    va_start(ap, num);
    node = node_t_new();
    node->type = STATEMENT;
    (node->record).integer = record;
    node->nxtnum = num;
    for(i=0; i<num; i++)
    {
        node_t_enlarge_nxt(node);
        node->nxt[i] = va_arg(ap, NODE_T*);
    }
    return node;
}

NODE_T * create_node_declarator(int record, int num, ...)
{
    va_list ap;
    NODE_T *node;
    va_start(ap, num);
    if(num == 0)
    {
        node = node_t_new();
        node->type = DECLARATOR;
        (node->record).integer = record;
        node->nxtnum = 0;
    }
    else if(num == 1)
    {
        node = node_t_new();
        node->type = DECLARATOR;
        (node->record).integer = record;
        node_t_enlarge_nxt(node);
        node->nxt[0] = va_arg(ap, NODE_T*);
        node->nxtnum = 1;
    }
    else
    {
        node = va_arg(ap, NODE_T*);
        (node->record).integer = record;
        node_t_enlarge_nxt(node);
        node->nxt[node->nxtnum] = va_arg(ap, NODE_T*);
        node->nxtnum ++;
        if(num == 2 && record == '[')
        {
            NODE_T *t;
            NODE_T *p;
            t = node->nxt[1];
            p = node;
            while(p->nxt[0] != NULL && p->nxt[0]->record.integer == '[')
            {
                p->nxt[1] = p->nxt[0]->nxt[1];
                p = p->nxt[0];
            }
            p->nxt[1] = t;
        }
    }
    return node;
}

NODE_T * create_node_ellipsis()
{
    NODE_T *node;
    node = node_t_new();
    node->type = ELLIPSIS;
    node->nxtnum = 0;
    return node;
}

NODE_T * create_node_typedef()
{
    NODE_T *node;
    node = node_t_new();
    node->type = TYPEDEF;
    node->nxtnum = 0;
    return node;
}

int tnhashsize;
TYPEDEF_NAME_HASH *tnhash;
int tnhashnum;
int tbscopelevel = 0;

void tnhash_init()
{
    tnhashsize = 128;
    tnhash = (TYPEDEF_NAME_HASH *)malloc(sizeof(TYPEDEF_NAME_HASH) * tnhashsize);
    tnhashnum = 0;
}

void tnhash_end()
{
    int i;
    for(i=0; i<tnhashnum; i++)
    {
        if(tnhash[i].string != NULL) free(tnhash[i].string);
    }
    free(tnhash);
}

void tnhash_enlarge()
{
    int i;
    TYPEDEF_NAME_HASH *t;
    if(tnhashnum < tnhashsize) return;
    t = (TYPEDEF_NAME_HASH *)malloc(sizeof(TYPEDEF_NAME_HASH) * tnhashsize * 2);
    for(i=0; i<tnhashsize; i++) t[i] = tnhash[i];
    tnhashsize *= 2;
    free(tnhash);
    tnhash = t;
}

void tnhash_add(char *string, int depth)
{
    tnhash_enlarge();
    tnhash[tnhashnum].string = strdup(string);
    tnhash[tnhashnum].depth = depth;
    tnhashnum ++;
}

int tnhash_query(char *string)
{
    int i;
    for(i=0; i<tnhashnum; i++)
    {
        if(tnhash[i].string != NULL && strcmp(string, tnhash[i].string) == 0) return 1;
    }
    return 0;
}

void tree_build_begin_scope()
{
    tbscopelevel ++;
}

void tree_build_end_scope()
{
    int i;
    for(i=0; i<tnhashnum; i++)
    {
        if(tnhash[i].string != NULL && tnhash[i].depth >= tbscopelevel)
        {
            free(tnhash[i].string);
            tnhash[i].string = NULL;
        }
    }
    tbscopelevel --;
}

void add_typedef_name(NODE_T *node)
{
    int i;
    if(node->nxtnum != 0)
    {
        for(i=0; i<node->nxtnum; i++)
        {
            add_typedef_name(node->nxt[i]);
        }
    }
    else
    {
        if(node->type == IDENTIFIER)
        {
            tnhash_add((node->record).string, tbscopelevel);
        }
    }
}

void tree_output_ast(NODE_T *node, int depth)
{
    int i;    
    for(i=0; i<depth*2; i++) fprintf(foutast, " ");
    switch(node->type)
    {
        case PROGRAM:
            fprintf(foutast, "program");
            break;
        case DECLARATION:
            fprintf(foutast, "declaration");
            break;
        case FUNCTION_DEFINITION:
            fprintf(foutast, "function_definition");
            break;
        case PARAMETER:
            fprintf(foutast, "parameter");
            break;
        case DECLARATORS:
            fprintf(foutast, "declarators");
            break;
        case INIT_DECLARATORS:
            fprintf(foutast, "init_declarators");
            break;
        case INIT_DECLARATOR:
            fprintf(foutast, "init_declarator");
            break;
        case INITIALIZER:
            fprintf(foutast, "initializer");
            break;
        case EXPRESSIONS:
            fprintf(foutast, "expressions");
            break;
        case EXPRESSION:
            switch((node->record).integer)
            {
                case SHL_ASSIGN: fprintf(foutast, "<<="); break;
                case SHR_ASSIGN: fprintf(foutast, ">>="); break;
                case MUL_ASSIGN: fprintf(foutast, "*="); break;
                case DIV_ASSIGN: fprintf(foutast, "/="); break;
                case MOD_ASSIGN: fprintf(foutast, "%%="); break;
                case ADD_ASSIGN: fprintf(foutast, "+="); break;
                case SUB_ASSIGN: fprintf(foutast, "-="); break;
                case AND_ASSIGN: fprintf(foutast, "&="); break;
                case XOR_ASSIGN: fprintf(foutast, "^="); break;
                case OR_ASSIGN: fprintf(foutast, "|="); break;
                case OR_OP: fprintf(foutast, "||"); break;
                case AND_OP: fprintf(foutast, "&&"); break;
                case EQ_OP: fprintf(foutast, "=="); break;
                case NE_OP: fprintf(foutast, "!="); break;
                case LE_OP: fprintf(foutast, "<="); break;
                case GE_OP: fprintf(foutast, ">="); break;
                case SHL_OP: fprintf(foutast, "<<"); break;
                case SHR_OP: fprintf(foutast, ">>"); break;
                case INC_OP: fprintf(foutast, "++"); break;
                case DEC_OP: fprintf(foutast, "--"); break;
                case PTR_OP: fprintf(foutast, "->"); break;
                case INC_OP_U: fprintf(foutast, "++U"); break;
                case DEC_OP_U: fprintf(foutast, "--U"); break;
                case CAST_EXP: fprintf(foutast, "cast_exp"); break;
                case BRACKET_EXP: fprintf(foutast, "bracket_exp"); break;
                case POSTFIX_EXP: fprintf(foutast, "postfix_exp"); break;
                default:
                    fprintf(foutast, "%c", (char)((node->record).integer)); break;
            }
            break;
        case TYPE_SPECIFIER:
            switch((node->record).integer)
            {
                case VOID_T: fprintf(foutast, "void"); break;
                case CHAR_T: fprintf(foutast, "char"); break;
                case INT_T: fprintf(foutast, "int"); break;
                case TYPEDEF_NAME: fprintf(foutast, "typedef_name"); break;
                case STRUCT_T: fprintf(foutast, "struct"); break;
                case UNION_T: fprintf(foutast, "union"); break;
            }
            break;
        case POSTFIX:
            switch((node->record).integer)
            {
                case INC_OP: fprintf(foutast, "++"); break;
                case DEC_OP: fprintf(foutast, "--"); break;
                case PTR_OP: fprintf(foutast, "->"); break;
                case '(': fprintf(foutast, "()"); break;
                case '[': fprintf(foutast, "[]"); break;
                case '.': fprintf(foutast, "."); break;
            }
            break;
        case PLAIN_DECLARATIONS:
            fprintf(foutast, "plain_declarations");
            break;
        case PLAIN_DECLARATION:
            fprintf(foutast, "plain_declaration");
            break;
        case ARGUMENTS:
            fprintf(foutast, "arguments");
            break;
        case TYPE_NAME:
            switch((node->record).integer)
            {
                case 0: fprintf(foutast, "type_specifier"); break;
                case '*': fprintf(foutast, "*"); break;
            }
            break;
        case STATEMENTS:
            fprintf(foutast, "statements");
            break;
        case STATEMENT:
            switch((node->record).integer)
            {
                case EXP_STMT: fprintf(foutast, "exp_stmt"); break;
                case '{': fprintf(foutast, "{"); break;
                case '}': fprintf(foutast, "}"); break;
                case IF_C: fprintf(foutast, "if"); break;
                case WHILE_C: fprintf(foutast, "while"); break;
                case FOR_C: fprintf(foutast, "for"); break;
                case BREAK_C: fprintf(foutast, "break"); break;
                case RETURN_C: fprintf(foutast, "return"); break;
                case CONTINUE_C: fprintf(foutast, "continue"); break;
            }
            break;
        case DECLARATOR:
            switch((node->record).integer)
            {
                case '(':
                    fprintf(foutast, "declarator()");
                    break;
                case '[':
                    fprintf(foutast, "declarator[]");
                    break;
                case 0:
                    fprintf(foutast, "declarator");
                    break;
                case '*':
                    fprintf(foutast, "declarator*");
                    break;
            }
            break;
        case TYPEDEFNAME:
            fprintf(foutast, "%s", (node->record).string);
            break;
        case IDENTIFIER:
            fprintf(foutast, "%s", (node->record).string);
            break;
        case CONST:
            fprintf(foutast, "%s", (node->record).string);
            break;
        case ELLIPSIS:
            fprintf(foutast, "...");
            break;
        case TYPEDEF:
            fprintf(foutast, "typedef");
            break;
    }    
    fflush(foutast);
    fprintf(foutast, " (\n");
    
    for(i=0; i<node->nxtnum; i++)
    {
        tree_output_ast(node->nxt[i], depth+1);
    }
    for(i=0; i<depth*2; i++) fprintf(foutast, " ");
    fprintf(foutast, ")\n");
}


