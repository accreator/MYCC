#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include "y.tab.h"
#include "def.h"
#include "utility.h"
#include "translate.h"
#include "semantics.h"
#include "ast.h"



NODE_TYPE * node_type_name_new(char *name, struct NODE_ENV *env)
{
    NODE_TYPE *node;
    node = (NODE_TYPE *)malloc(sizeof(NODE_TYPE));
    node->type = NAME;
    node->size = 4;
    (node->record).name.name = strdup(name);
    (node->record).name.env = env;
    
    node->isconst = 0;
    node->id = "";
#ifdef USE_VLA
    node->info = (int)tree_translate_new_vla(4);
#endif
    return node;
}

NODE_TYPE * node_type_copy_new(NODE_TYPE *x)
{
    NODE_TYPE *node;
    if(x == NULL) return NULL;
    node = (NODE_TYPE *)malloc(sizeof(NODE_TYPE));
    node->type = x->type;
    node->size = x->size;
    node->info = x->info;
    node->record = x->record;
    node->isconst = x->isconst;
    node->id = x->id;
    return node;
}

NODE_TYPE * node_type_copy_r_new(NODE_TYPE *x)
{
    int i;
    NODE_TYPE *node;
    
    if(x == NULL) return NULL;
    
    node = (NODE_TYPE *)malloc(sizeof(NODE_TYPE));
    node->type = x->type;
    node->size = x->size;
    node->info = x->info;
    node->isconst = x->isconst;
    node->id = strdup(x->id);
    
    switch(node->type)
    {
        case TYPE: break;
        case VOID: break;
        case CHAR: break;
        case INT: break;
        case NAME:
            (node->record).name.name = strdup((x->record).name.name);
            (node->record).name.env = (x->record).name.env;
            break;
        case POINTER:
            (node->record).pointer = node_type_copy_r_new((x->record).pointer);
            break;
        case ARRAY:
            (node->record).array.capacity = (x->record).array.capacity;
            (node->record).array.pointer = node_type_copy_r_new((x->record).array.pointer);
            break;
        case STRUCT:
        case UNION:
            (node->record).record.size = (x->record).record.size;
            (node->record).record.field = (NODE_TYPE_RECORD_FIELD *)malloc(sizeof(NODE_TYPE_RECORD_FIELD) * (node->record).record.size);
            for(i=0; i<(node->record).record.size; i++)
            {
                (node->record).record.field[i].name = strdup((x->record).record.field[i].name);
                (node->record).record.field[i].type = node_type_copy_r_new((x->record).record.field[i].type);
            }
            break;
        case FUNCTION:
            (node->record).function.size = (x->record).function.size;
            if((x->record).function.size == 0)
                (node->record).function.argumenttype = NULL;
            else
                (node->record).function.argumenttype = (NODE_TYPE *)malloc(sizeof(NODE_TYPE) * (x->record).function.size);
            for(i=0; i<(x->record).function.size; i++)
                (node->record).function.argumenttype[i] = *(node_type_copy_r_new(&((x->record).function.argumenttype[i])));
            (node->record).function.returntype = node_type_copy_r_new((x->record).function.returntype);
            break;
    }
    return node;
}

void node_type_cal_size_r(NODE_TYPE *x)
{
    int i;
    if(x == NULL) return;
    switch(x->type)
    {
        case TYPE: x->size = 4; break;
        case VOID: x->size = 4; break;
        case CHAR: x->size = 1; break;
        case INT: x->size = 4; break;
        case NAME: x->size = 4; break;
        case POINTER:
            node_type_cal_size_r((x->record).pointer);
            x->size = 4;
            break;
        case ARRAY:
            node_type_cal_size_r((x->record).array.pointer);
            x->size = (x->record).array.capacity * (x->record).array.pointer->size;
            while(x->size % 4) x->size ++; //对齐
            break;
        case UNION:
            x->size = 0;
            for(i=0; i<(x->record).record.size; i++)
            {
                node_type_cal_size_r((x->record).record.field[i].type);
                if(x->size < x->record.record.field[i].type->size)
                    x->size = x->record.record.field[i].type->size;
                while(x->size % 4) x->size ++; //对齐
            }
            break;
        case STRUCT:
            x->size = 0;
            for(i=0; i<(x->record).record.size; i++)
            {
                node_type_cal_size_r((x->record).record.field[i].type);
                x->size += x->record.record.field[i].type->size;
                while(x->size % 4) x->size ++; //对齐
            }
            break;
        case FUNCTION:
            x->size = 4;
            for(i=0; i<(x->record).function.size; i++)
                node_type_cal_size_r(&((x->record).function.argumenttype[i]));
            node_type_cal_size_r((x->record).function.returntype);
            break;
    }
}

void node_type_fix_size_vla(NODE_TYPE *x, int flag)
{
    int i;
#ifndef USE_VLA
    return;
#endif
    if(x == NULL) return;
    switch(x->type)
    {
        case TYPE: x->size = 4; break;
        case VOID: x->size = 4; break;
        case CHAR: x->size = 1; break;
        case INT: x->size = 4; break;
        case NAME: x->size = 4; break;
        case POINTER:
            node_type_fix_size_vla((x->record).pointer, 0);
            x->size = 4;
            break;
        case ARRAY:
            node_type_fix_size_vla((x->record).array.pointer, flag);
            if(flag)
            {
                x->size = -1;
                x->record.array.capacity = -1;
            }
            else
                x->size = (x->record).array.capacity * (x->record).array.pointer->size;
            break;
        case UNION:
            x->size = 0;
            for(i=0; i<(x->record).record.size; i++)
            {
                node_type_fix_size_vla((x->record).record.field[i].type, 0);
                if(x->size < x->record.record.field[i].type->size)
                    x->size = x->record.record.field[i].type->size;
            }
            break;
        case STRUCT:
            x->size = 0;
            for(i=0; i<(x->record).record.size; i++)
            {
                node_type_fix_size_vla((x->record).record.field[i].type, 0);
                x->size += x->record.record.field[i].type->size;
            }
            break;
        case FUNCTION:
            x->size = 4;
            for(i=0; i<(x->record).function.size; i++)
                node_type_fix_size_vla(&((x->record).function.argumenttype[i]), 0);
            node_type_fix_size_vla((x->record).function.returntype, 0);
            break;
    }
}

int node_type_cal_size_r_v(NODE_TYPE *x)
{
    int i;
    int size = 0;
    if(x == NULL) return 0;
    switch(x->type)
    {
        case TYPE: return 4; break;
        case VOID: return 4; break;
        case CHAR: return 1; break;
        case INT: return 4; break;
        case NAME: return 4; break;
        case POINTER: return 4; break;
        case ARRAY:
            return (x->record).array.capacity * node_type_cal_size_r_v((x->record).array.pointer);
            break;
        case UNION:
            size = 0;
            for(i=0; i<(x->record).record.size; i++)
            {
                int t;
                t = node_type_cal_size_r_v((x->record).record.field[i].type);
                if(size < t)
                    size = t;
            }
            return size;
            break;
        case STRUCT:
            size = 0;
            for(i=0; i<(x->record).record.size; i++)
            {
                size += node_type_cal_size_r_v((x->record).record.field[i].type);
            }
            return size;
            break;
        case FUNCTION:
            return size;
            break;
    }
    return size; //实际不可能到这里
}

NODE_TYPE * node_type_type_new()
{
    NODE_TYPE *node;
    node = (NODE_TYPE *)malloc(sizeof(NODE_TYPE));
    node->type = TYPE;
    node->size = 4;
    
    node->isconst = 0;
    node->id = "";
#ifdef USE_VLA
    node->info = (int)tree_translate_new_vla(4);
#endif    
    return node;
}

NODE_TYPE * node_type_void_new()
{
    NODE_TYPE *node;
    node = (NODE_TYPE *)malloc(sizeof(NODE_TYPE));
    node->type = VOID;
    node->size = 4;
    
    node->isconst = 0;
    node->id = "";
#ifdef USE_VLA
    node->info = (int)tree_translate_new_vla(4);
#endif    
    return node;
}

NODE_TYPE * node_type_int_new()
{
    NODE_TYPE *node;
    node = (NODE_TYPE *)malloc(sizeof(NODE_TYPE));
    node->type = INT;
    node->size = 4;
    
    node->isconst = 0;
    node->id = "";
#ifdef USE_VLA
    node->info = (int)tree_translate_new_vla(4);
#endif    
    return node;
}

NODE_TYPE * node_type_char_new()
{
    NODE_TYPE *node;
    node = (NODE_TYPE *)malloc(sizeof(NODE_TYPE));
    node->type = CHAR;
    node->size = 1;
    
    node->isconst = 0;
    node->id = "";
#ifdef USE_VLA
    node->info = (int)tree_translate_new_vla(1);
#endif    
    return node;
}

NODE_TYPE * node_type_pointer_new(NODE_TYPE *p)
{
    NODE_TYPE *node;
    node = (NODE_TYPE *)malloc(sizeof(NODE_TYPE));
    node->type = POINTER;
    node->size = 4;
    (node->record).pointer = p;
    
    node->isconst = 0;
    node->id = "";
#ifdef USE_VLA
    node->info = (int)tree_translate_new_vla(4);
#endif    
    return node;
}

NODE_TYPE * node_type_array_new(int capacity, NODE_TYPE *p)
{
    NODE_TYPE *node;
    node = (NODE_TYPE *)malloc(sizeof(NODE_TYPE));
    node->type = ARRAY;
    
    if(capacity >= 1)
        node->size = p->size * capacity;
    else
        node->size = 4;
    (node->record).array.capacity = capacity;
    (node->record).array.pointer = p;
    
    node->isconst = 0;
    node->id = "";
#ifdef USE_VLA
    node->info = (int)tree_translate_new_vla(-1);
#endif    
    return node;
}

NODE_TYPE * node_type_struct_new(int size, NODE_TYPE_RECORD_FIELD *p)
{
    NODE_TYPE *node;
    int i;
    node = (NODE_TYPE *)malloc(sizeof(NODE_TYPE));
    node->type = STRUCT;
    node->size = 0;
    for(i=0; i<size; i++)
    {
        node->size += p[i].type->size;
    }
    (node->record).record.size = size;
    (node->record).record.field = p;
    
    node->isconst = 0;
    node->id = "";
#ifdef USE_VLA
    node->info = (int)tree_translate_new_vla(-1);
#endif    
    return node;
}

NODE_TYPE * node_type_union_new(int size, NODE_TYPE_RECORD_FIELD *p)
{
    NODE_TYPE *node;
    int i;
    node = (NODE_TYPE *)malloc(sizeof(NODE_TYPE));
    node->type = UNION;
    node->size = 0;
    for(i=0; i<size; i++)
    {
        if(node->size < p[i].type->size)
            node->size = p[i].type->size;
    }
    (node->record).record.size = size;
    (node->record).record.field = p;
    
    node->isconst = 0;
    node->id = "";
#ifdef USE_VLA
    node->info = (int)tree_translate_new_vla(-1);
#endif    
    return node;
}

NODE_TYPE * node_type_function_new(int size, NODE_TYPE *argumenttype, NODE_TYPE *returntype)
{
    NODE_TYPE *node;
    node = (NODE_TYPE *)malloc(sizeof(NODE_TYPE));
    node->type = FUNCTION;
    node->size = 4;
    (node->record).function.argumenttype = argumenttype;
    (node->record).function.returntype = returntype;
    (node->record).function.size = size;
    
    node->isconst = 0;
    node->id = "";
#ifdef USE_VLA
    node->info = (int)tree_translate_new_vla(4);
#endif    
    return node;
}

void node_type_delete(NODE_TYPE *node) //将不影响内部构造
{
    switch(node->type)
    {
        case NAME: free((node->record).name.name); break;
        case TYPE:
        case VOID:
        case INT:
        case CHAR:
        case POINTER:
        case STRUCT:
        case UNION:
        case FUNCTION:
            break;
        case ARRAY:
            node_type_delete((node->record).array.pointer);
            break;
    }
    free(node);
    free((void *)(node->info));
}

/*
需要垃圾回收，或者建立树形结构时均新建节点
一个简单的方法是#define malloc...
未完成

void node_type_delete_r(NODE_TYPE *node) //delete recursively
{
    int i;
    switch(node->type)
    {
        case NAME: free((node->record->name).name); break;
        case VOID: break;
        case INT: break;
        case CHAR: break;
        case POINTER: node_type_delete_r(node->record->pointer); break;
        case STRUCT: 
        case UNION:
            for(i=0; i<(node->record->record).size; i++)
            {
                ;
            }
    }
    free(node);
}
*/

NODE_ENV * node_env_new()
{
    NODE_ENV *node;
    node = (NODE_ENV *)malloc(sizeof(NODE_ENV));
    
    node->ventrysize = 1;
    node->ventry = (NODE_ENTRY *)malloc(sizeof(NODE_ENTRY));
    node->ventrynum = 0;
    
    node->tentrysize = 1;
    node->tentry = (NODE_ENTRY *)malloc(sizeof(NODE_ENTRY));
    node->tentrynum = 0;
    return node;
}

void node_env_enlarge(NODE_ENV *node)
{
    int i;
    NODE_ENTRY *t;
    if(node->ventrynum+1 >= node->ventrysize)
    {
        if(node->ventrysize == 0)
        {
            node->ventrysize = 1;
        }
        else
        {
            node->ventrysize *= 2;
        }
        t = (NODE_ENTRY *)malloc(sizeof(NODE_ENTRY) * node->ventrysize);
        for(i=0; i<node->ventrynum; i++)
        {
            t[i] = node->ventry[i];
        }
        if(node->ventry != NULL) free(node->ventry);
        node->ventry = t;
    }
    if(node->tentrynum+1 >= node->tentrysize)
    {
        if(node->tentrysize == 0)
        {
            node->tentrysize = 1;
        }
        else
        {
            node->tentrysize *= 2;
        }
        t = (NODE_ENTRY *)malloc(sizeof(NODE_ENTRY) * node->tentrysize);
        for(i=0; i<node->tentrynum; i++)
        {
            t[i] = node->tentry[i];
        }
        if(node->tentry != NULL) free(node->tentry);
        node->tentry = t;
    }
    fflush(stdout);
}

void node_env_delete(NODE_ENV *node)
{
    free(node->ventry);
    free(node->tentry);
    free(node);
}

NODE_ENV *tscenv = NULL;
int tscenvnum = 0;
int tscenvsize = 0;

void tscenv_enlarge()
{
    int i;
    NODE_ENV *t;
    if(tscenvnum+1 >= tscenvsize)
    {
        if(tscenvsize == 0) tscenvsize = 1;
        while(tscenvnum+1 >= tscenvsize) tscenvsize *= 2;
        t = (NODE_ENV *)malloc(sizeof(NODE_ENV) * tscenvsize);
        for(i=0; i<tscenvnum; i++)
        {
            t[i] = tscenv[i];
        }
        for(i=tscenvnum; i<tscenvsize; i++)
        {
            t[i].ventrynum = t[i].ventrysize = 0;
            t[i].tentrynum = t[i].tentrysize = 0;
            t[i].ventry = t[i].tentry = NULL;
        }
        if(tscenv != NULL) free(tscenv);
        tscenv = t;
    }
}

int tscloopcount = 0;
int tscscopelevel = 0;

void tree_semantic_check_begin_scope()
{
    tscscopelevel ++;
    if(tscenvnum < tscscopelevel)
    {
        tscenv_enlarge();
        tscenvnum ++;
    }
    tscenv[tscenvnum-1].ventrynum = 0;
    tscenv[tscenvnum-1].tentrynum = 0;
    if(tscenvnum>1)
        tscenv[tscenvnum-1].selftype = tscenv[tscenvnum-2].selftype;
    else
        tscenv[tscenvnum-1].selftype = NULL;
}

void tree_semantic_check_end_scope()
{
    tscscopelevel --;
    tscenvnum --;
}

void tree_semantic_check_hold_begin_scope()
{
    tscscopelevel ++;
    tscenvnum ++;
}

void tree_semantic_check_hold_end_scope()
{
    tscscopelevel --;
    tscenvnum --;
}

void tree_semantic_check_type_specifier_record(NODE_T *node)
{
    int i;
    NODE_TYPE_RECORD_FIELD *field;
    if(node->nxtnum == 1 && node->nxt[0]->type == IDENTIFIER) //struct_or_union IDENTIFIER_O 声明
    {
        node->selftype = node_type_name_new((node->nxt[0]->record).string, &tscenv[tscenvnum-1]);
        node->selftype->isconst = 1;
        node->selftype->id = strdup((node->nxt[0]->record).string);
        
        tree_semantic_check_add_typeentry_n(strdup(node->selftype->id), node_type_copy_r_new(node->selftype));
    }
    else if(node->nxtnum == 1) //struct_or_union '{' plain_declarations '}'
    {
        field = (NODE_TYPE_RECORD_FIELD *)malloc(sizeof(NODE_TYPE_RECORD_FIELD) * node->nxt[0]->nxtnum);
        for(i=0; i<node->nxt[0]->nxtnum; i++)
        {
            field[i].type = node->nxt[0]->nxt[i]->selftype;
            field[i].name = strdup(node->nxt[0]->nxt[i]->selftype->id);
        }
        if((node->record).integer == STRUCT_T)
            node->selftype = node_type_struct_new(node->nxt[0]->nxtnum, field);
        else
            node->selftype = node_type_union_new(node->nxt[0]->nxtnum, field);
        node->selftype->isconst = 1;
        node->selftype->id = random_string_gen();
        
        tree_semantic_check_add_typeentry(strdup(node->selftype->id), node_type_copy_r_new(node->selftype));
        //to be continued
        //以上没有考虑在struct/union内声明struct/union
    }
    else //struct_or_union IDENTIFIER_O '{' plain_declarations '}' 
    {
        field = (NODE_TYPE_RECORD_FIELD *)malloc(sizeof(NODE_TYPE_RECORD_FIELD) * node->nxt[1]->nxtnum);
        for(i=0; i<node->nxt[1]->nxtnum; i++)
        {
            field[i].type = node->nxt[1]->nxt[i]->selftype;
            field[i].name = strdup(node->nxt[1]->nxt[i]->selftype->id);
        }
        if((node->record).integer == STRUCT_T)
            node->selftype = node_type_struct_new(node->nxt[1]->nxtnum, field);
        else
            node->selftype = node_type_union_new(node->nxt[1]->nxtnum, field);
        node->selftype->isconst = 1;
        node->selftype->id = strdup((node->nxt[0]->record).string);
        
        tree_semantic_check_add_typeentry(strdup(node->selftype->id), node_type_copy_r_new(node->selftype));
        //to be continued
        //以上没有考虑在struct/union内声明struct/union
    }
}

int tree_semantic_check_same_type(NODE_TYPE *x, NODE_TYPE *y) 
{
////注意所有str function需判断NULL
    int i;
    if(x->type != y->type) return 0;
    if(x == y) return 1;
    switch(x->type)
    {
        case TYPE: break;
        case VOID: break;
        case CHAR: break;
        case INT: break;
        case NAME:
            if(strcmp((x->record).name.name, (y->record).name.name) != 0) return 0;
            //目前不检查evn是否相同
            break;
        case POINTER:
            if(tree_semantic_check_same_type((x->record).pointer, (y->record).pointer) == 0) return 0;
            break;
        case ARRAY:
            if((x->record).array.capacity != (y->record).array.capacity) return 0;
            if(tree_semantic_check_same_type((x->record).array.pointer, (y->record).array.pointer) == 0) return 0;
            if(x->record.array.capacity == -1 && x->info != y->info) return 0; 
            break;
        case STRUCT:
        case UNION:
            if((x->record).record.size != (y->record).record.size) return 0;
            for(i=0; i<(x->record).record.size; i++)
            {
                if(strcmp((x->record).record.field[i].name, (y->record).record.field[i].name) != 0) return 0;
                if(tree_semantic_check_same_type((x->record).record.field[i].type, (y->record).record.field[i].type) == 0) return 0;
            }
            break;
        case FUNCTION:
            if((x->record).function.size != (y->record).function.size) return 0;
            for(i=0; i<(x->record).function.size; i++)
            {
                if(tree_semantic_check_same_type(&((x->record).function.argumenttype[i]), &((y->record).function.argumenttype[i])) == 0) return 0;
            }
            if(tree_semantic_check_same_type((x->record).function.returntype, (y->record).function.returntype) == 0) return 0;
            if((x->record).function.size != (y->record).function.size) return 0;
            break;
    }
    return 1;
}

NODE_TYPE * tree_semantic_check_symbol(char *id) //找不到返回NULL
{
    int i, j;
#ifdef NOT_USE_FUNC_1
    return NULL;
#endif
    for(i=tscenvnum-1; i>=0; i--)
    {
        for(j=0; j<tscenv[i].ventrynum; j++)
        {
            if(strcmp(tscenv[i].ventry[j].symbol, id) == 0)
            {
                return tscenv[i].ventry[j].type;
            }
        }
        for(j=0; j<tscenv[i].tentrynum; j++)
        {
            if(tscenv[i].tentry[j].type->type == NAME) continue;
            if(strcmp(tscenv[i].tentry[j].symbol, id) == 0)
            {
                return tscenv[i].tentry[j].type;
            }
        }                
    }
    for(i=tscenvnum-1; i>=0; i--)
    {
        for(j=0; j<tscenv[i].tentrynum; j++)
        {
            if(strcmp(tscenv[i].tentry[j].symbol, id) == 0)
            {
                return tscenv[i].tentry[j].type;
            }
        }                
    }
    //暂时无法处理好只有声明情况下的->/.操作
    //to be continued
    return NULL;
}

NODE_TYPE * tree_semantic_check_symbol_ventry(char *id) //找不到返回NULL
{
    int i, j;
    for(i=tscenvnum-1; i>=0; i--)
    {
        for(j=0; j<tscenv[i].ventrynum; j++)
        {
            if(strcmp(tscenv[i].ventry[j].symbol, id) == 0)
            {
                return tscenv[i].ventry[j].type;
            }
        }               
    }
    return NULL;
}

NODE_TYPE * tree_semantic_check_symbol_fun(char *id) //找不到返回NULL
{
    int i, j;
    for(i=tscenvnum-1; i>=0; i--)
    {
        for(j=0; j<tscenv[i].ventrynum; j++)
        {
            if(tscenv[i].ventry[j].entrytype == FUNENTRY)
            {
                if(strcmp(tscenv[i].ventry[j].symbol, id) == 0)
                {
                    return tscenv[i].ventry[j].type;
                }
            }
        }         
    }
    return NULL;
}

NODE_TYPE * tree_semantic_check_symbol_top(char *id) //找不到返回NULL
{
    int i, j;
#ifdef NOT_USE_FUNC_2
    return NULL;
#endif
    i = tscenvnum - 1;
    for(j=0; j<tscenv[i].ventrynum; j++)
    {
        if(strcmp(tscenv[i].ventry[j].symbol, id) == 0)
        {
            return tscenv[i].ventry[j].type;
        }
    }
    for(j=0; j<tscenv[i].tentrynum; j++)
    {
        if(strcmp(tscenv[i].tentry[j].symbol, id) == 0)
        {
            return tscenv[i].tentry[j].type;
        }
    } 
    return NULL;
}

NODE_TYPE * tree_semantic_check_symbol_top_ventry(char *id) //找不到返回NULL
{
    int i, j;
    i = tscenvnum - 1;
    for(j=0; j<tscenv[i].ventrynum; j++)
    {
        if(strcmp(tscenv[i].ventry[j].symbol, id) == 0)
        {
            return tscenv[i].ventry[j].type;
        }
    }
    return NULL;
}

NODE_TYPE * tree_semantic_check_symbol_top_def(char *id) //找不到返回NULL
{
    int i, j;
#ifdef NOT_USE_FUNC_2
    return NULL;
#endif
    i = tscenvnum - 1;
    for(j=0; j<tscenv[i].ventrynum; j++)
    {
        if(strcmp(tscenv[i].ventry[j].symbol, id) == 0)
        {
            if(tscenv[i].ventry[j].def == 0)
                return tscenv[i].ventry[j].type;
        }
    }
    for(j=0; j<tscenv[i].tentrynum; j++)
    {
        if(strcmp(tscenv[i].tentry[j].symbol, id) == 0)
        {
            return tscenv[i].tentry[j].type;
        }
    } 
    return NULL;
}

int tscaddfunentryflag = 0;
char *tscaddfunentryid = NULL;

void tree_semantic_check_add_funentry(char *id, NODE_TYPE *type, int flag) //flag=0 定义 flag=1声明
{
    NODE_ENV *env;
#ifdef NOT_USE_FUNC_3
    return;
#endif
    tscaddfunentryflag = 0;
    if(tscaddfunentryid != NULL) free(tscaddfunentryid);
    tscaddfunentryid = strdup(id);
    if(!flag && tree_semantic_check_symbol_top_def(id) != NULL)
    {
        report_error(-1, "semantic error", "names conflict");
    }
    if(tree_semantic_check_symbol_top(id) != NULL) return;
    tscaddfunentryflag = 1;
    
    env = &tscenv[tscenvnum-1];
    node_env_enlarge(env);
    (env->ventry[env->ventrynum]).entrytype = FUNENTRY;
    (env->ventry[env->ventrynum]).symbol = strdup(id);
    (env->ventry[env->ventrynum]).type = type;
    (env->ventry[env->ventrynum]).def = flag;
    env->ventrynum ++;
}

void tree_semantic_check_add_typeentry(char *id, NODE_TYPE *type)
{
    NODE_ENV *env;
#ifdef NOT_USE_FUNC_4
    return;
#endif
    NODE_TYPE *t;
    if((t=tree_semantic_check_symbol_top(id)) != NULL)
    {
        if(t->type != NAME)
        {
            if(tree_semantic_check_same_type(type, t)) return;
            report_error(-1, "semantic error", "names conflict(type)");
        }
    }
    env = &tscenv[tscenvnum-1];
    node_env_enlarge(env);
    (env->tentry[env->tentrynum]).entrytype = TYPEENTRY;
    (env->tentry[env->tentrynum]).symbol = strdup(id);
    (env->tentry[env->tentrynum]).type = type;
    (env->ventry[env->tentrynum]).def = 0;
    env->tentrynum ++;
}

void tree_semantic_check_add_typeentry_n(char *id, NODE_TYPE *type)
{
    NODE_ENV *env;
    NODE_TYPE *t;
    if((t=tree_semantic_check_symbol_top(id)) != NULL)
    {
        return;
    }
    env = &tscenv[tscenvnum-1];
    node_env_enlarge(env);
    (env->tentry[env->tentrynum]).entrytype = TYPEENTRY;
    (env->tentry[env->tentrynum]).symbol = strdup(id);
    (env->tentry[env->tentrynum]).type = type;
    (env->ventry[env->tentrynum]).def = 0;
    env->tentrynum ++;
}

void tree_semantic_check_add_varentry(char *id, NODE_TYPE *type)
{
    NODE_ENV *env;
#ifdef NOT_USE_FUNC_5
    return;
#endif
    if(tree_semantic_check_symbol_top(id) != NULL)
    {
        report_error(-1, "semantic error", "names conflict");
    }
    env = &tscenv[tscenvnum-1];
    node_env_enlarge(env);
    (env->ventry[env->ventrynum]).entrytype = VARENTRY;
    (env->ventry[env->ventrynum]).symbol = strdup(id);
    (env->ventry[env->ventrynum]).type = type;
    (env->ventry[env->ventrynum]).def = 0;
    env->ventrynum ++;
}

void tree_semantic_check_fetch_identifier_type(NODE_T *node)
{
#ifdef NOT_USE_FUNC_6
    return;
#endif
    if(node->type != IDENTIFIER) return;
    node->selftype = node_type_copy_r_new(tree_semantic_check_symbol((node->record).string));
    if(node->selftype == NULL)
    {
        report_error(-1, "semantic error", "undefined name");
    }
    node->selftype->isconst = 0;
    node->selftype->id = strdup((node->record).string);
}

void tree_semantic_check_fetch_function_type(NODE_T *node)
{
    if(node->type != IDENTIFIER) return;
    node->selftype = node_type_copy_r_new(tree_semantic_check_symbol_fun((node->record).string));
    if(node->selftype == NULL)
    {
        report_error(-1, "semantic error", "undefined function");
    }
    node->selftype->isconst = 0;
    node->selftype->id = strdup((node->record).string);
}

void tree_semantic_check_plain_declaration(NODE_T *node) //由myc.y可见其只能出现在参数表和struct的内部
{
    int j;
    NODE_TYPE **t, *p;
    char *id;
    NODE_TYPE *argumenttype, *returntype;
#ifdef NOT_USE_FUNC_7
    return;
#endif
    if(node->nxt[0]->selftype->type == TYPE)
    {
        node->nxt[0]->selftype = tree_semantic_check_symbol(node->nxt[0]->selftype->id);
    }
    if(node->nxt[1]->selftype->type == FUNCTION) //函数
    {
        p = node_type_copy_r_new(node->nxt[1]->nxt[0]->selftype);
        t = &p;
        while((*t)->type != TYPE)
        {
            if((*t)->type == ARRAY)
            {
                report_error(-1, "semantic error", "f[]( )");
            }
            t = &(((*t)->record).pointer);
        }
        id = (*t)->id;
        *t = node_type_copy_r_new(node->nxt[0]->selftype);
        (*t)->isconst = 1;
        (*t)->id = "";    
        node_type_cal_size_r(p);
        
        if(node->nxt[1]->nxtnum == 2) //若有参数
            argumenttype = (NODE_TYPE *)malloc(sizeof(NODE_TYPE) * node->nxt[1]->nxt[1]->nxtnum);
        else
            argumenttype = NULL;
        returntype = p;
        if(node->nxt[1]->nxtnum == 2) //若有参数
        {
            for(j=0; j<node->nxt[1]->nxt[1]->nxtnum; j++)
            {
                if(node->nxt[1]->nxt[1]->nxt[j]->type == ELLIPSIS)
                    argumenttype[j] = *(node_type_type_new());
                else
                    argumenttype[j] = *(node_type_copy_r_new(node->nxt[1]->nxt[1]->nxt[j]->selftype));
            }
        }
        tree_semantic_check_add_funentry(id, node_type_function_new(node->nxt[1]->nxt[1]->nxtnum, argumenttype, returntype), 1);
        tree_translate_main(node, 1);
    }
    else //变量
    {
        p = node_type_copy_r_new(node->nxt[1]->selftype); 
        t = &p;
        while((*t)->type != TYPE)
        {
            if((*t)->type == ARRAY)
            {
                t = &(((*t)->record).array.pointer);
            }
            else //((*t)->type == POINTER
            {
                t = &(((*t)->record).pointer);
            }
        }
        id = (*t)->id;
        *t = node_type_copy_r_new(node->nxt[0]->selftype);
        (*t)->isconst = 1;
        (*t)->id = "";
        if(p->type == ARRAY && p->size == -1) //变长数组
        {
            ;//to be continued
        }
        else
        {
            node_type_cal_size_r(p);
        }
        
        tree_semantic_check_add_varentry(id, p);
        
        node->selftype = node_type_copy_new(p);
        node->selftype->isconst = 0;
        node->selftype->id = id;
    }
}

void tree_semantic_check_declaration_2(NODE_T *node)
{
    int i;
#ifdef NOT_USE_FUNC_8
    return;
#endif
    //node->nxt[0]是type_specifier
    //node->nxt[1]是init_declarators
    if(node->nxt[0]->selftype->type == TYPE)
    {
        node->nxt[0]->selftype = tree_semantic_check_symbol(node->nxt[0]->selftype->id);
    }
    for(i=0; i<node->nxt[1]->nxtnum; i++)
    {
        NODE_T *d;
        d = node->nxt[1]->nxt[i]; //init_declarator等级
        tree_semantic_check_init_declarator(d, 1, node->nxt[0]->selftype, node);
    }
}

void tree_semantic_check_declaration_3(NODE_T *node)
{
    int i, j;
#ifdef NOT_USE_FUNC_9
    return;
#endif
    //node->nxt[0]是TYPEDEF_T
    //node->nxt[1]是type_specifier
    //node->nxt[2]是declarators
    if(node->nxt[1]->selftype->type == TYPE)
    {
        node->nxt[1]->selftype = tree_semantic_check_symbol(node->nxt[1]->selftype->id);
    }
    for(i=0; i<node->nxt[2]->nxtnum; i++)
    {
        NODE_T *d;
        NODE_TYPE **t, *p;
        char *id;
        d = node->nxt[2]->nxt[i]; //declarator等级
        if(d->selftype->type == FUNCTION)
        {
            NODE_TYPE *argumenttype, *returntype;
            id = d->nxt[0]->selftype->id;
            //函数声明
            if(d->nxtnum == 1) //无参数
            {
                argumenttype = NULL;
                p = node_type_copy_r_new(d->nxt[0]->selftype);
                t = &p;
                while((*t)->type != TYPE)
                {
                    t = &(((*t)->record).pointer);
                }
                *t = node_type_copy_r_new(node->nxt[1]->selftype);
                (*t)->isconst = 1;
                (*t)->id = id;
                node_type_cal_size_r(p);
                returntype = p;
                tree_semantic_check_add_varentry(id, node_type_function_new(0, argumenttype, returntype));
            }
            else //==2, 有参数
            {
                argumenttype = (NODE_TYPE *)malloc(sizeof(NODE_TYPE) * d->nxt[1]->nxtnum);
                p = node_type_copy_r_new(d->nxt[0]->selftype);
                t = &p;
                while((*t)->type != TYPE)
                {
                    t = &(((*t)->record).pointer);
                }
                *t = node_type_copy_r_new(node->nxt[1]->selftype);
                (*t)->isconst = 1;
                (*t)->id = id;
                node_type_cal_size_r(p);
                returntype = p;
                for(j=0; j<d->nxt[1]->nxtnum; j++) //d->nxt[1]是paramaters
                {
                    if(d->nxt[1]->nxt[j]->type == ELLIPSIS)
                        argumenttype[j] = *(node_type_type_new());
                    else
                        argumenttype[j] = *(node_type_copy_r_new(d->nxt[1]->nxt[j]->selftype));
                }
                tree_semantic_check_add_varentry(id, node_type_function_new(d->nxt[1]->nxtnum, argumenttype, returntype));
            }
        }
        else
        {
            p = node_type_copy_r_new(d->selftype); //declarator的selftype
            t = &p;
            while((*t)->type != TYPE)
            {
                if((*t)->type == ARRAY)
                {
                    t = &(((*t)->record).array.pointer);
                }
                else //((*t)->type == POINTER
                {
                    t = &(((*t)->record).pointer);
                }
            }
            id = (*t)->id;
            *t = node_type_copy_r_new(node->nxt[1]->selftype);
            (*t)->isconst = 1;
            (*t)->id = "";
            node_type_cal_size_r(p);
            
            tree_semantic_check_add_typeentry(id, p);
        }
    }
}

void tree_semantic_check_funtion_definition(NODE_T *node)
{
    int j;
    NODE_TYPE **t, *p;
    char *id;
    NODE_TYPE *argumenttype, *returntype;
#ifdef NOT_USE_FUNC_10
    return;
#endif
    if(node->nxt[0]->selftype->type == TYPE)
    {
        node->nxt[0]->selftype = tree_semantic_check_symbol(node->nxt[0]->selftype->id);
    }
    p = node_type_copy_r_new(node->nxt[1]->selftype);
    t = &p;
    while((*t)->type != TYPE)
    {
        t = &(((*t)->record).pointer);
    }
    id = (*t)->id;
    *t = node_type_copy_r_new(node->nxt[0]->selftype);
    (*t)->isconst = 1;
    (*t)->id = "";
    node_type_cal_size_r(p);
    
    if(node->nxtnum == 4) //若有参数
        argumenttype = (NODE_TYPE *)malloc(sizeof(NODE_TYPE) * node->nxt[2]->nxtnum);
    else
        argumenttype = NULL;
    returntype = p;
    if(node->nxtnum == 4) //若有参数
    {
        for(j=0; j<node->nxt[2]->nxtnum; j++)
        {
            if(node->nxt[2]->nxt[j]->type == ELLIPSIS)
                argumenttype[j] = *(node_type_type_new());
            else
            {
                argumenttype[j] = *(node_type_copy_r_new(node->nxt[2]->nxt[j]->selftype));
            }
        }
    }
    tree_semantic_check_add_funentry(id, node_type_function_new(node->nxtnum==4?node->nxt[2]->nxtnum:0, argumenttype, returntype), 0);
    //必须紧跟tree_semantic_check_add_funentry
    tree_translate_main(node, 1);
    tree_semantic_check_hold_begin_scope();
    tscenv[tscenvnum-1].selftype = node_type_function_new(node->nxtnum==4?node->nxt[2]->nxtnum:0, argumenttype, returntype);
    tree_semantic_check_hold_end_scope();
}

void tree_semantic_check_init_declarator(NODE_T *node, int flag, NODE_TYPE *type, NODE_T *root)
{
    int j;
    NODE_TYPE *p;
    NODE_TYPE **t;
    if(node->nxtnum == 1) //没有赋初值
    {
        if(flag) tree_semantic_check(node->nxt[0]);
        node->selftype = node_type_copy_new(node->nxt[0]->selftype);
        node->selftype->isconst = 1;
        node->selftype->id = strdup(node->nxt[0]->selftype->id);
    }
    else //==2 赋初值
    {
        if(flag) tree_semantic_check(node->nxt[0]);

        if(node->nxt[0]->selftype->type == FUNCTION)
        {
            report_error(-1, "semantic error", "fun( , ) = xxx");
        }
        node->selftype = node_type_copy_new(node->nxt[0]->selftype);
        node->selftype->isconst = 1;
        node->selftype->id = strdup(node->nxt[0]->selftype->id);
    }
    if(flag)
    {
        ttdeclarationcurrent = NULL;
        if(node->nxtnum == 1) //无初始化，即包含declarator
        {
            p = node_type_copy_r_new(node->nxt[0]->selftype); //declarator的selftype
            if(p->type == FUNCTION)
            {
                char *id;
                NODE_TYPE *argumenttype, *returntype;
                id = node->nxt[0]->nxt[0]->selftype->id;
                //函数声明
                if(node->nxt[0]->nxtnum == 1) //无参数
                {
                    argumenttype = NULL;
                    p = node_type_copy_r_new(node->nxt[0]->nxt[0]->selftype);
                    t = &p;
                    while((*t)->type != TYPE)
                    {
                        t = &(((*t)->record).pointer);
                    }
                    *t = node_type_copy_r_new(type);
                    (*t)->isconst = 1;
                    (*t)->id = id;
                    node_type_cal_size_r(p);
                    returntype = p;
                    tree_semantic_check_add_funentry(id, node_type_function_new(0, argumenttype, returntype), 1);
                    //注意必须紧接tree_semantic_check_add_funentry
                    tree_translate_main(root, 2);
                }
                else //==2, 有参数
                {
                    argumenttype = (NODE_TYPE *)malloc(sizeof(NODE_TYPE) * node->nxt[0]->nxt[1]->nxtnum);
                    p = node_type_copy_r_new(node->nxt[0]->nxt[0]->selftype);
                    t = &p;
                    while((*t)->type != TYPE)
                    {
                        t = &(((*t)->record).pointer);
                    }
                    *t = node_type_copy_r_new(type);
                    (*t)->isconst = 1;
                    (*t)->id = id;
                    node_type_cal_size_r(p);
                    returntype = p;
                    for(j=0; j<node->nxt[0]->nxt[1]->nxtnum; j++) //node->nxt[0]->nxt[1]是paramaters
                    {
                        if(node->nxt[0]->nxt[1]->nxt[j]->type == ELLIPSIS)
                            argumenttype[j] = *(node_type_type_new());
                        else
                            argumenttype[j] = *(node_type_copy_r_new(node->nxt[0]->nxt[1]->nxt[j]->selftype));
                    }
                    tree_semantic_check_add_funentry(id, node_type_function_new(node->nxt[0]->nxt[1]->nxtnum, argumenttype, returntype), 1);
                    //注意必须紧接tree_semantic_check_add_funentry
                    tree_translate_main(root, 2);
                }
            }
            else
            {
                char *id;
                t = &p;
                while((*t)->type != TYPE)
                {
                    if((*t)->type == ARRAY)
                    {
                        t = &(((*t)->record).array.pointer);
                    }
                    else //((*t)->type == POINTER
                    {
                        t = &(((*t)->record).pointer);
                    }
                }
                id = (*t)->id;
                *t = node_type_copy_r_new(type);
                (*t)->isconst = 1;
                (*t)->id = id;
                if(p->type == ARRAY && p->size == -1) //变长数组
                {
#ifdef USE_VLA
                    t = &p;
                    while((*t)->type == ARRAY)
                    {
                        ttdelarationtype = *t;
                        tree_translate_main(root, 3);
                        t = (((*t)->record).array.pointer);
                    }
#endif
                }
                else
                {
                    node_type_cal_size_r(p);
                }
                
                tree_semantic_check_add_varentry(id, p);
                //注意必须紧接tree_semantic_check_add_varentry
                tree_translate_main(root, 4);
                tree_translate_main(root, 1);
            }
        }
        else //==2 有初始化, 包含declarator = initializer
        {
            p = node_type_copy_r_new(node->nxt[0]->selftype); //declarator的selftype
            if(p->type == FUNCTION)
            {
                ;//函数声明 不能初始化, 已于儿子节点处理, ok
            }
            else
            {
                char *id;
                t = &p;
                while((*t)->type != TYPE)
                {
                    if((*t)->type == ARRAY)
                    {
                        t = &(((*t)->record).array.pointer);
                    }
                    else //((*t)->type == POINTER
                    {
                        t = &(((*t)->record).pointer);
                    }
                }
                id = (*t)->id;
                *t = node_type_copy_r_new(type);
                (*t)->isconst = 1;
                (*t)->id = id;
                node_type_cal_size_r(p);
                
                //to be continued
                //需要一段代码检查初始值和类型是否匹配
                
                tree_semantic_check_add_varentry(id, p);
                tree_translate_main(root, 4);
                
                tree_semantic_check(node->nxt[1]);
                //注意必须紧接tree_semantic_check_add_varentry
                ttdeclarationcurrent = node;
                tree_translate_main(root, 1);
            }
        }
        //if(node->nxtnum == 2) tree_semantic_check(node->nxt[1]);
    }
}

void tree_semantic_check(NODE_T *node)
{
    int i;
    int ac = 0; //already check
    int at = 0; //already translate
#ifdef NOT_USE_FUNC_11
    return;
#endif
    node->trans = tree_translate_new();
    switch(node->type)
    {
        case PROGRAM: //ok
            break;
        case DECLARATION:
            switch(node->nxtnum)
            {
                case 1:
                    tree_semantic_check(node->nxt[0]);
                    break;
                case 2:
                    tree_semantic_check(node->nxt[0]);
                    //tree_semantic_check(node->nxt[1]);
                    tree_semantic_check_declaration_2(node);
                    break;
                case 3: //typedef
                    tree_semantic_check(node->nxt[0]);
                    tree_semantic_check(node->nxt[1]);
                    tree_semantic_check(node->nxt[2]);
                    tree_semantic_check_declaration_3(node);
                    break;
            }
            ac = 1;
            
            tree_translate_main(node, 0);
            at = 1;
            break;
        case FUNCTION_DEFINITION:
            if(node->nxtnum == 4)
            {
                tree_semantic_check(node->nxt[0]);
                tree_semantic_check(node->nxt[1]);
                tree_semantic_check_begin_scope();
                tree_semantic_check(node->nxt[2]);
                tree_translate_main(node, 3);
                tree_semantic_check_hold_end_scope();
                tree_semantic_check_funtion_definition(node);
                tree_semantic_check_hold_begin_scope();
                tree_semantic_check(node->nxt[3]);
                tree_translate_main(node, 2);
                tree_semantic_check_end_scope();
            }
            else //==3
            {
                tree_semantic_check(node->nxt[0]);
                tree_semantic_check(node->nxt[1]);
                tree_semantic_check_begin_scope();
                tree_semantic_check_hold_end_scope();
                tree_semantic_check_funtion_definition(node);
                tree_semantic_check_hold_begin_scope();
                tree_semantic_check(node->nxt[2]);
                tree_translate_main(node, 2);
                tree_semantic_check_end_scope();
            }
            ac = 1;
            tree_translate_main(node, 0);
            at = 1;
            break;
        case PARAMETER: //ok
            break;
        case DECLARATORS: //ok
            break;
        case INIT_DECLARATORS: //ok
            tree_translate_main(node, 0);
            at = 1;
            break;
        case INIT_DECLARATOR: //只检查函数不能被赋予初值，其它部分在高层处理, ok
            if(node->nxtnum == 1) //没有赋初值
            {
                tree_semantic_check(node->nxt[0]);
            }
            else //==2 赋初值
            {
                tree_semantic_check(node->nxt[0]);
                tree_semantic_check(node->nxt[1]);
            }
            tree_semantic_check_init_declarator(node, 0, NULL, NULL);
            ac = 1;
            
            tree_translate_main(node, 0);
            at = 1;
            break;
        case INITIALIZER: //ok
            tree_translate_main(node, 0);
            at = 1;
            break;
        case EXPRESSIONS: //ok
            tree_translate_main(node, 0);
            at = 1;
            break;
        case EXPRESSION:
            switch((node->record).integer)
            {
                case SHL_ASSIGN:
                case SHR_ASSIGN:
                case MUL_ASSIGN:
                case DIV_ASSIGN:
                case MOD_ASSIGN:
                case AND_ASSIGN:
                case XOR_ASSIGN:
                case OR_ASSIGN:
                    tree_semantic_check(node->nxt[0]);
                    tree_semantic_check(node->nxt[1]);
                    tree_semantic_check_fetch_identifier_type(node->nxt[0]);
                    tree_semantic_check_fetch_identifier_type(node->nxt[1]);
                    
                    if(node->nxt[1]->selftype->type != INT && node->nxt[1]->selftype->type != CHAR)
                    {
                        report_error(-1, "semantic error", "SHL/SHR/MUL/DIV/MOD/AND/XOR/OR ASSIGN");
                    }
                    if(node->nxt[0]->selftype->isconst)
                    {
                        report_error(-1, "semantic error", "SHL/SHR/MUL/DIV/MOD/AND/XOR/OR ASSIGN");
                    }
                    if(node->nxt[0]->selftype->type != INT &&
                       node->nxt[0]->selftype->type != CHAR)
                    {
                        report_error(-1, "semantic error", "SHL/SHR/MUL/DIV/MOD/AND/XOR/OR ASSIGN");
                    }
                    node->selftype = node_type_copy_new(node->nxt[0]->selftype);
                    node->selftype->isconst = 1;
                    node->selftype->id = "";
                    ac = 1;
                    
                    tree_translate_main(node, 0);
                    at = 1;
                    break;
                case ADD_ASSIGN:
                    tree_semantic_check(node->nxt[0]);
                    tree_semantic_check(node->nxt[1]);
                    tree_semantic_check_fetch_identifier_type(node->nxt[0]);
                    tree_semantic_check_fetch_identifier_type(node->nxt[1]);
                    
                    if(node->nxt[0]->selftype->isconst)
                    {
                        report_error(-1, "semantic error", "ADD ASSIGN");
                    }
                    /* to be continued */
                    node->selftype = node_type_copy_new(node->nxt[0]->selftype);
                    node->selftype->isconst = 1;
                    node->selftype->id = "";
                    ac = 1;
                    
                    tree_translate_main(node, 0);
                    at = 1;
                    break;
                case SUB_ASSIGN:
                    tree_semantic_check(node->nxt[0]);
                    tree_semantic_check(node->nxt[1]);
                    tree_semantic_check_fetch_identifier_type(node->nxt[0]);
                    tree_semantic_check_fetch_identifier_type(node->nxt[1]);
                    
                    if(node->nxt[0]->selftype->isconst)
                    {
                        report_error(-1, "semantic error", "SUB ASSIGN");
                    }
                    /* to be continued */
                    /* 两指针可相减，但不可相加 */
                    node->selftype = node_type_copy_new(node->nxt[0]->selftype);
                    node->selftype->isconst = 1;
                    node->selftype->id = "";
                    ac = 1;
                    
                    tree_translate_main(node, 0);
                    at = 1;
                    break;
                case OR_OP:
                case AND_OP:
                    tree_semantic_check(node->nxt[0]);
                    tree_semantic_check(node->nxt[1]);
                    tree_semantic_check_fetch_identifier_type(node->nxt[0]);
                    tree_semantic_check_fetch_identifier_type(node->nxt[1]);
                    
                    if((node->nxt[0]->selftype->type != INT && node->nxt[0]->selftype->type != CHAR && node->nxt[0]->selftype->type != POINTER && node->nxt[0]->selftype->type != ARRAY) ||
                       (node->nxt[1]->selftype->type != INT && node->nxt[1]->selftype->type != CHAR && node->nxt[1]->selftype->type != POINTER && node->nxt[1]->selftype->type != ARRAY))
                    {
                        report_error(-1, "semantic error", "OR/AND OP");
                    }
                    node->selftype = node_type_int_new();
                    node->selftype->isconst = 1;
                    node->selftype->id = "";
                    ac = 1;
                    
                    tree_translate_main(node, 0);
                    at = 1;
                    break;
                case EQ_OP:
                case NE_OP:
                case LE_OP:
                case GE_OP:
                case '<':
                case '>':
                    tree_semantic_check(node->nxt[0]);
                    tree_semantic_check(node->nxt[1]);
                    tree_semantic_check_fetch_identifier_type(node->nxt[0]);
                    tree_semantic_check_fetch_identifier_type(node->nxt[1]);
                    
                    if(node->nxt[0]->selftype->type != INT &&
                       node->nxt[0]->selftype->type != CHAR &&
                       node->nxt[0]->selftype->type != POINTER)
                    {
                        report_error(-1, "semantic error", "EQ/NE/LE/GE/>/< OP");
                    }
                    if(node->nxt[1]->selftype->type != INT &&
                       node->nxt[1]->selftype->type != CHAR &&
                       node->nxt[1]->selftype->type != POINTER)
                    {
                        report_error(-1, "semantic error", "EQ/NE/LE/GE/>/< OP");
                    }
                    if(node->nxt[0]->selftype->type == POINTER &&
                       node->nxt[1]->selftype->type != POINTER)
                    {
                        report_error(-1, "semantic error", "EQ/NE/LE/GE/>/< OP");
                    }
                    if(node->nxt[0]->selftype->type != POINTER &&
                       node->nxt[1]->selftype->type == POINTER)
                    {
                        report_error(-1, "semantic error", "EQ/NE/LE/GE/>/< OP");
                    }                    
                    node->selftype = node_type_int_new();
                    node->selftype->isconst = 1;
                    node->selftype->id = "";
                    ac = 1;
                    
                    tree_translate_main(node, 0);
                    at = 1;
                    break;
                case SHL_OP:
                case SHR_OP:
                    tree_semantic_check(node->nxt[0]);
                    tree_semantic_check(node->nxt[1]);
                    tree_semantic_check_fetch_identifier_type(node->nxt[0]);
                    tree_semantic_check_fetch_identifier_type(node->nxt[1]);
                    
                    if(node->nxt[1]->selftype->type != INT && node->nxt[1]->selftype->type != CHAR)
                    {
                        report_error(-1, "semantic error", "SHL/SHR OP");
                    }
                    if(node->nxt[0]->selftype->type != INT && node->nxt[0]->selftype->type != CHAR)
                    {
                        report_error(-1, "semantic error", "SHL/SHR OP");
                    }
                    node->selftype = node_type_copy_new(node->nxt[0]->selftype);
                    node->selftype->isconst = 1;
                    node->selftype->id = "";
                    ac = 1;
                    
                    tree_translate_main(node, 0);
                    at = 1;
                    break;
                case SIZEOF_OP:
                    tree_semantic_check(node->nxt[0]);
                    node->selftype = node_type_int_new();
                    node->selftype->isconst = 1;
                    node->selftype->id = "";
                    ac = 1;
                    
                    tree_translate_main(node, 0);
                    at = 1;
                    break;
                case INC_OP: //不可能遇到, ok
                case DEC_OP: //不可能遇到, ok
                    break;
                case PTR_OP: //不可能遇到, ok
                    break;
                case INC_OP_U:
                case DEC_OP_U:
                    tree_semantic_check(node->nxt[0]);
                    tree_semantic_check_fetch_identifier_type(node->nxt[0]);
                    
                    if(node->nxt[0]->selftype->isconst)
                    {
                        report_error(-1, "semantic error", "INC/DEC OP_U");
                    }
                    if(node->nxt[0]->selftype->type != INT &&
                       node->nxt[0]->selftype->type != CHAR &&
                       node->nxt[0]->selftype->type != POINTER)
                    {
                        report_error(-1, "semantic error", "INC/DEC OP_U");
                    }
                    node->selftype = node_type_copy_new(node->nxt[0]->selftype);
                    node->selftype->isconst = 1;
                    node->selftype->id = strdup(node->nxt[0]->selftype->id);
                    ac = 1;
                    
                    tree_translate_main(node, 0);
                    at = 1;
                    break;
                case CAST_EXP:
                    tree_semantic_check(node->nxt[0]);
                    tree_semantic_check(node->nxt[1]);
                    tree_semantic_check_fetch_identifier_type(node->nxt[0]);
                    tree_semantic_check_fetch_identifier_type(node->nxt[1]);
                    
                    if((node->nxt[0]->selftype->type == INT || node->nxt[0]->selftype->type == CHAR || node->nxt[0]->selftype->type == POINTER) &&
                       node->nxt[1]->selftype->type != INT &&
                       node->nxt[1]->selftype->type != CHAR &&
                       node->nxt[1]->selftype->type != POINTER)
                    {
                        report_error(-1, "semantic error", "CAST_EXP");
                    }
                    if((node->nxt[1]->selftype->type == INT || node->nxt[1]->selftype->type == CHAR || node->nxt[1]->selftype->type == POINTER) &&
                       node->nxt[0]->selftype->type != INT &&
                       node->nxt[0]->selftype->type != CHAR &&
                       node->nxt[0]->selftype->type != POINTER)
                    {
                        report_error(-1, "semantic error", "CAST_EXP");
                    }
                    node->selftype = node_type_copy_new(node->nxt[0]->selftype);
                    node->selftype->isconst = 1;
                    node->selftype->id = strdup(node->nxt[0]->selftype->id);
                    /* to be continued (typedef和struct强转) */
                    ac = 1;
                    
                    tree_translate_main(node, 0);
                    at = 1;
                    break;
                case BRACKET_EXP:
                    tree_semantic_check(node->nxt[0]);
                    tree_semantic_check_fetch_identifier_type(node->nxt[0]);
                    
                    node->selftype = node_type_copy_new(node->nxt[0]->selftype);
                    node->selftype->isconst = node->nxt[0]->selftype->isconst;
                    node->selftype->id = strdup(node->nxt[0]->selftype->id);
                    ac = 1;
                    
                    tree_translate_main(node, 0);
                    at = 1;
                    break;
                case POSTFIX_EXP:
                    tree_semantic_check(node->nxt[0]);
                    tree_semantic_check(node->nxt[1]);
                    tree_semantic_check_fetch_identifier_type(node->nxt[0]);
                    switch((node->nxt[1]->record).integer)
                    {
                        case INC_OP:
                        case DEC_OP:
                            if(node->nxt[0]->selftype->type != CHAR &&
                               node->nxt[0]->selftype->type != INT &&
                               node->nxt[0]->selftype->type != POINTER)
                            {
                                report_error(-1, "semantic error", "INC/DEC OP");
                            }
                            node->selftype = node_type_copy_new(node->nxt[0]->selftype);
                            node->selftype->isconst = 0;
                            node->selftype->id = strdup(node->nxt[0]->selftype->id);
                            break;
                        case PTR_OP:
                            if(node->nxt[0]->selftype->type != POINTER)
                            {
                                report_error(-1, "semantic error", "-> A OP");
                            }
                            if((node->nxt[0]->selftype->record).pointer->type == NAME)
                            {
                                (node->nxt[0]->selftype->record).pointer = tree_semantic_check_symbol(((node->nxt[0]->selftype->record).pointer->record).name.name);
                                //只能暂时解决部分问题，对NAME的处理需要全局地修改
                            }
                            if((node->nxt[0]->selftype->record).pointer->type != STRUCT &&
                               (node->nxt[0]->selftype->record).pointer->type != UNION)
                            {
                                report_error(-1, "semantic error", "-> B OP");
                            }
                            for(i=0; i<((node->nxt[0]->selftype->record).pointer->record).record.size; i++)
                            {
                                if(strcmp(((node->nxt[0]->selftype->record).pointer->record).record.field[i].name,
                                   node->nxt[1]->selftype->id) == 0)
                                {
                                    break;
                                }
                            }
                            if(i >= ((node->nxt[0]->selftype->record).pointer->record).record.size)
                            {
                                report_error(-1, "semantic error", "-> C OP");
                            }
                            node->selftype = node_type_copy_new(((node->nxt[0]->selftype->record).pointer->record).record.field[i].type);
                            node->selftype->isconst = 0;
                            node->selftype->id = strdup(node->nxt[0]->selftype->id);
                            break;
                        case '.':
                            if(node->nxt[0]->selftype->type != UNION &&
                               node->nxt[0]->selftype->type != STRUCT)
                            {
                                report_error(-1, "semantic error", ". OP A");
                            }
                            for(i=0; i<(node->nxt[0]->selftype->record).record.size; i++)
                            {
                                if(strcmp((node->nxt[0]->selftype->record).record.field[i].name, node->nxt[1]->selftype->id) == 0)
                                    break;
                            }
                            if(i >= (node->nxt[0]->selftype->record).record.size)
                            {
                                report_error(-1, "semantic error", ". OP B");
                            }
                            node->selftype = node_type_copy_new((node->nxt[0]->selftype->record).record.field[i].type);
                            node->selftype->isconst = 0;
                            node->selftype->id = strdup(node->nxt[0]->selftype->id);
                            //注意这里还没有考虑typedef的影响
                            break;
                        case '(':
                            if(node->nxt[0]->selftype->type != FUNCTION)
                            {
                                report_error(-1, "semantic error", "undefined funtion");
                            }
                            if(node->nxt[1]->nxtnum == 0) //无参数
                            {
                                if((node->nxt[0]->selftype->record).function.size != 0)
                                {
                                    report_error(-1, "semantic error", "arguments not match");
                                }
                            }
                            else //有参数
                            {
                                //node->nxt[0]是函数
                                //node->nxt[1]是参数
                                
                                for(i=0; i<node->nxt[1]->nxt[0]->nxtnum; i++) //每个参数
                                {
                                    NODE_TYPE *x, *y;
                                
                                    if(i >= (node->nxt[0]->selftype->record).function.size)
                                    {
                                        report_error(-1, "semantic error", "arguments not match");
                                    }
                                    x = &((node->nxt[0]->selftype->record).function.argumenttype[i]);
                                    tree_semantic_check_fetch_identifier_type(node->nxt[1]->nxt[0]->nxt[i]);
                                    y = node->nxt[1]->nxt[0]->nxt[i]->selftype;
                                    if(x->type == TYPE) //不定参
                                    {
                                        break;
                                    }
                                    if(i==node->nxt[1]->nxt[0]->nxtnum-1 && //已经是最后一个参数
                                       node->nxt[1]->nxt[0]->nxtnum < (node->nxt[0]->selftype->record).function.size && //参数数比函数定义得少
                                       (node->nxt[0]->selftype->record).function.argumenttype[i+1].type != TYPE //函数不是不定参
                                      )
                                    {
                                        report_error(-1, "semantic error", "arguments not match");
                                    }
                                    if((x->type == POINTER || x->type == ARRAY) && y->type != POINTER && y->type != ARRAY)
                                    {
                                        report_error(-1, "semantic error", "argument's type not match");
                                    }
                                    if((y->type == POINTER || y->type == ARRAY) && x->type != POINTER && x->type != ARRAY)
                                    {
                                        report_error(-1, "semantic error", "argument's type not match");
                                    }
                                    if((x->type == INT || x->type == CHAR) && y->type != INT && y->type != CHAR)
                                    {
                                        report_error(-1, "semantic error", "argument's type not match");
                                    }
                                    if((y->type == INT || y->type == CHAR) && x->type != INT && x->type != CHAR)
                                    {
                                        report_error(-1, "semantic error", "argument's type not match");
                                    }
                                    if(!tree_semantic_check_same_type(x, y) &&
                                       x->type != POINTER && x->type != ARRAY &&
                                       x->type != CHAR && x->type != INT)
                                    {
                                        report_error(-1, "semantic error", "argument's type not match");
                                    }
                                    //以上还没有考虑typedef造成的影响
                                }
                            }
                            node->selftype = node_type_copy_new((node->nxt[0]->selftype->record).function.returntype);
                            node->selftype->isconst = 1;
                            node->selftype->id = strdup(node->nxt[0]->selftype->id);
                            break;
                        case '[':
                            if(node->nxt[0]->selftype->type != ARRAY && node->nxt[0]->selftype->type != POINTER)
                            {
                                report_error(-1, "semantic error", "[] ARRAY/POINTER does not match");
                            }
                            if(node->nxt[0]->selftype->type == ARRAY)
                            {
                                node->selftype = node_type_copy_new((node->nxt[0]->selftype->record).array.pointer);
                            }
                            else
                            {
                                //... tree_semantic_check_fetch_identifier_type();
                                node->selftype = node_type_copy_new((node->nxt[0]->selftype->record).pointer);
                            }
                            node->selftype->isconst = 0;
                            node->selftype->id = strdup(node->nxt[0]->selftype->id);
                            break;
                    }
                    /* to be continued */
                    ac = 1;
                    
                    tree_translate_main(node, 0);
                    at = 1;
                    break;
                case ',':
                    tree_semantic_check(node->nxt[0]);
                    tree_semantic_check(node->nxt[1]);
                    tree_semantic_check_fetch_identifier_type(node->nxt[0]);
                    tree_semantic_check_fetch_identifier_type(node->nxt[1]);
                    
                    node->selftype = node_type_copy_new(node->nxt[1]->selftype);
                    node->selftype->isconst = 1;
                    node->selftype->id = "";
                    ac = 1;
                    
                    tree_translate_main(node, 0);
                    at = 1;
                    break;
                case '=':
                    tree_semantic_check(node->nxt[0]);
                    tree_semantic_check(node->nxt[1]);
                    tree_semantic_check_fetch_identifier_type(node->nxt[0]);
                    tree_semantic_check_fetch_identifier_type(node->nxt[1]);
                    
                    if(node->nxt[0]->selftype->isconst)
                    {
                        report_error(-1, "semantic error", "=");
                    }
                    if(node->nxt[0]->selftype->type == POINTER &&
                       (node->nxt[1]->selftype->type != POINTER && node->nxt[1]->selftype->type != ARRAY &&
                        node->nxt[1]->selftype->type != INT && node->nxt[1]->selftype->type != CHAR))
                    {
                        report_error(-1, "semantic error", "=");
                    }
                    if(node->nxt[0]->selftype->type != POINTER &&
                       (node->nxt[1]->selftype->type == POINTER || node->nxt[1]->selftype->type == ARRAY))
                    {
                        report_error(-1, "semantic error", "=");
                    }
                    if((node->nxt[0]->selftype->type == INT || node->nxt[0]->selftype->type == CHAR) &&
                       (node->nxt[1]->selftype->type != INT && node->nxt[1]->selftype->type != CHAR))
                    {
                        report_error(-1, "semantic error", "=");
                    }
                    if((node->nxt[0]->selftype->type != INT && node->nxt[0]->selftype->type != CHAR && node->nxt[0]->selftype->type != POINTER) &&
                       (node->nxt[1]->selftype->type == INT || node->nxt[1]->selftype->type == CHAR))
                    {
                        report_error(-1, "semantic error", "=");
                    }
                    if(node->nxt[0]->selftype->type != INT &&
                       node->nxt[0]->selftype->type != CHAR &&
                       node->nxt[0]->selftype->type != POINTER &&
                       !tree_semantic_check_same_type(node->nxt[0]->selftype, node->nxt[1]->selftype))
                    {
                        report_error(-1, "semantic error", "=");
                    }
                    /* 注意以上没有考虑到typedef造成的影响, 没考虑到不同struct名但构造相同的struct造成的影响 */
                    //to be continued
                    node->selftype = node_type_copy_new(node->nxt[0]->selftype);
                    node->selftype->isconst = 1;
                    node->selftype->id = strdup(node->nxt[0]->selftype->id);
                    ac = 1;
                    
                    tree_translate_main(node, 0);
                    at = 1;
                    break;
                case '+':
                    if(node->nxtnum == 1)
                    {
                        tree_semantic_check(node->nxt[0]);
                        tree_semantic_check_fetch_identifier_type(node->nxt[0]);
                        
                        if(node->nxt[0]->selftype->type != INT && node->nxt[0]->selftype->type != CHAR)
                        {
                            report_error(-1, "semantic error", "+_U");
                        }
                        node->selftype = node_type_copy_new(node->nxt[0]->selftype);
                        node->selftype->isconst = 1;
                        node->selftype->id = "";
                    }
                    else
                    {
                        tree_semantic_check(node->nxt[0]);
                        tree_semantic_check(node->nxt[1]);
                        tree_semantic_check_fetch_identifier_type(node->nxt[0]);
                        tree_semantic_check_fetch_identifier_type(node->nxt[1]);
                        
                        if(node->nxt[1]->selftype->type != INT &&
                           node->nxt[1]->selftype->type != CHAR &&
                           node->nxt[1]->selftype->type != POINTER &&
                           node->nxt[1]->selftype->type != ARRAY)
                        {
                            report_error(-1, "semantic error", "+");
                        }
                        switch(node->nxt[0]->selftype->type)
                        {
                            case INT:
                                if(node->nxt[1]->selftype->type == POINTER || node->nxt[1]->selftype->type == ARRAY)
                                {
                                    node->selftype = node_type_copy_new(node->nxt[1]->selftype);
                                    node->selftype->isconst = 1;
                                    node->selftype->id = strdup(node->nxt[1]->selftype->id);
                                }
                                else
                                {
                                    node->selftype = node_type_copy_new(node->nxt[0]->selftype);
                                    node->selftype->isconst = 1;
                                    node->selftype->id = "";
                                }
                                break;
                            case CHAR:
                                node->selftype = node_type_copy_new(node->nxt[1]->selftype);
                                node->selftype->isconst = 1;
                                if(node->nxt[1]->selftype->type == POINTER || node->nxt[1]->selftype->type == ARRAY)
                                    node->selftype->id = strdup(node->nxt[1]->selftype->id);
                                else
                                    node->selftype->id = "";
                                break;
                            case POINTER:
                            case ARRAY:
                                if(node->nxt[1]->selftype->type == POINTER || node->nxt[1]->selftype->type == ARRAY)
                                {
                                    report_error(-1, "semantic error", "+");
                                }
                                else
                                {
                                    node->selftype = node_type_copy_new(node->nxt[0]->selftype);
                                    node->selftype->isconst = 1;
                                    node->selftype->id = strdup(node->nxt[0]->selftype->id);
                                }
                                break;
                            default:
                                report_error(-1, "semantic error", "+");
                        }
                    }
                    ac = 1;
                    
                    tree_translate_main(node, 0);
                    at = 1;
                    break;
                case '-':
                    if(node->nxtnum == 1)
                    {
                        tree_semantic_check(node->nxt[0]);
                        tree_semantic_check_fetch_identifier_type(node->nxt[0]);
                        
                        if(node->nxt[0]->selftype->type != INT && node->nxt[0]->selftype->type != CHAR)
                        {
                            report_error(-1, "semantic error", "-_U");
                        }
                        node->selftype = node_type_copy_new(node->nxt[0]->selftype);
                        node->selftype->isconst = 1;
                        node->selftype->id = "";
                    }
                    else
                    {
                        tree_semantic_check(node->nxt[0]);
                        tree_semantic_check(node->nxt[1]);
                        tree_semantic_check_fetch_identifier_type(node->nxt[0]);
                        tree_semantic_check_fetch_identifier_type(node->nxt[1]);
                        
                        if(node->nxt[1]->selftype->type != INT &&
                           node->nxt[1]->selftype->type != CHAR &&
                           node->nxt[1]->selftype->type != POINTER &&
                           node->nxt[1]->selftype->type != ARRAY)
                        {
                            report_error(-1, "semantic error", "-");
                        }
                        switch(node->nxt[0]->selftype->type)
                        {
                            case INT:
                                if(node->nxt[1]->selftype->type == POINTER || node->nxt[1]->selftype->type == ARRAY)
                                {
                                    report_error(-1, "semantic error", "-");
                                }
                                node->selftype = node_type_copy_new(node->nxt[0]->selftype);
                                node->selftype->isconst = 1;
                                node->selftype->id = "";
                                break;
                            case CHAR:
                                if(node->nxt[1]->selftype->type == POINTER || node->nxt[1]->selftype->type == ARRAY)
                                {
                                    report_error(-1, "semantic error", "-");
                                }
                                node->selftype = node_type_copy_new(node->nxt[1]->selftype);
                                node->selftype->isconst = 1;
                                node->selftype->id = "";
                                break;
                            case POINTER:
                            case ARRAY:
                                if(node->nxt[1]->selftype->type == POINTER || node->nxt[1]->selftype->type == ARRAY)
                                {
                                    /* to be continued */
                                    /* 判断指针类型是否相同 */
                                    node->selftype = node_type_int_new();
                                    node->selftype->isconst = 1;
                                    node->selftype->id = "";
                                }
                                else
                                {
                                    node->selftype = node_type_copy_new(node->nxt[0]->selftype);
                                    node->selftype->isconst = 1;
                                    node->selftype->id = strdup(node->nxt[0]->selftype->id);
                                }
                                break;
                            default:
                                report_error(-1, "semantic error", "-");
                        }
                    }
                    ac = 1;
                    
                    tree_translate_main(node, 0);
                    at = 1;
                    break;
                case '~':
                    tree_semantic_check(node->nxt[0]);
                    tree_semantic_check_fetch_identifier_type(node->nxt[0]);
                    
                    if(node->nxt[0]->selftype->type != INT &&
                       node->nxt[0]->selftype->type != CHAR)
                    {
                        report_error(-1, "semantic error", "~ OP");
                    }
                    node->selftype = node_type_copy_new(node->nxt[0]->selftype);
                    node->selftype->isconst = 1;
                    node->selftype->id = "";
                    ac = 1;
                    
                    tree_translate_main(node, 0);
                    at = 1;
                    break;
                case '!':
                    tree_semantic_check(node->nxt[0]);
                    tree_semantic_check_fetch_identifier_type(node->nxt[0]);
                    
                    if(node->nxt[0]->selftype->type != INT &&
                       node->nxt[0]->selftype->type != CHAR)
                    {
                        report_error(-1, "semantic error", "! OP");
                    }
                    node->selftype = node_type_int_new();
                    node->selftype->isconst = 1;
                    node->selftype->id = "";
                    ac = 1;
                    
                    tree_translate_main(node, 0);
                    at = 1;
                    break;
                case '*':
                    if(node->nxtnum == 1)
                    {
                        tree_semantic_check(node->nxt[0]);
                        tree_semantic_check_fetch_identifier_type(node->nxt[0]);
                        
                        if(node->nxt[0]->selftype->type != POINTER) //是否需要考虑typedef?
                        {
                            report_error(-1, "semantic error", "*_U");
                        }
                        node->selftype = node_type_copy_new((node->nxt[0]->selftype->record).pointer);
                        node->selftype->isconst = 0;
                        node->selftype->id = strdup(node->nxt[0]->selftype->id);
                    }
                    else
                    {
                        tree_semantic_check(node->nxt[0]);
                        tree_semantic_check(node->nxt[1]);
                        tree_semantic_check_fetch_identifier_type(node->nxt[0]);
                        tree_semantic_check_fetch_identifier_type(node->nxt[1]);
                        
                        if(node->nxt[1]->selftype->type != INT && node->nxt[1]->selftype->type != CHAR)
                        {
                            report_error(-1, "semantic error", "*/%&^| OP");
                        }
                        switch(node->nxt[0]->selftype->type)
                        {
                            case INT:
                                node->selftype = node_type_int_new();
                                node->selftype->isconst = 1;
                                node->selftype->id = "";
                                break;
                            case CHAR:
                                if(node->nxt[1]->selftype->type == CHAR)
                                {
                                    node->selftype = node_type_char_new();
                                }
                                else
                                {
                                    node->selftype = node_type_int_new();
                                }
                                node->selftype->isconst = 1;
                                node->selftype->id = "";
                                break;
                            default:
                                report_error(-1, "semantic error", "*/%&^| OP");
                                break;
                        }
                    }
                    ac = 1;
                    
                    tree_translate_main(node, 0);
                    at = 1;
                    break;
                case '&':
                    if(node->nxtnum == 1)
                    {
                        tree_semantic_check(node->nxt[0]);
                        tree_semantic_check_fetch_identifier_type(node->nxt[0]);
                        
                        node->selftype = node_type_pointer_new(node->nxt[0]->selftype);
                        node->selftype->isconst = 1;
                        node->selftype->id = strdup(node->nxt[0]->selftype->id);
                    }
                    else
                    {
                        tree_semantic_check(node->nxt[0]);
                        tree_semantic_check(node->nxt[1]);
                        tree_semantic_check_fetch_identifier_type(node->nxt[0]);
                        tree_semantic_check_fetch_identifier_type(node->nxt[1]);
                    
                        if(node->nxt[1]->selftype->type != INT && node->nxt[1]->selftype->type != CHAR)
                        {
                            report_error(-1, "semantic error", "*/%&^| OP");
                        }
                        switch(node->nxt[0]->selftype->type)
                        {
                            case INT:
                                node->selftype = node_type_int_new();
                                node->selftype->isconst = 1;
                                node->selftype->id = "";
                                break;
                            case CHAR:
                                if(node->nxt[1]->selftype->type == CHAR)
                                {
                                    node->selftype = node_type_char_new();
                                }
                                else
                                {
                                    node->selftype = node_type_int_new();
                                }
                                node->selftype->isconst = 1;
                                node->selftype->id = "";
                                break;
                            default:
                                report_error(-1, "semantic error", "*/%&^| OP");
                                break;
                        }
                    }
                    ac = 1;
                    
                    tree_translate_main(node, 0);
                    at = 1;
                    break;
                case '/':
                case '%':
                case '^':
                case '|':
                    tree_semantic_check(node->nxt[0]);
                    tree_semantic_check(node->nxt[1]);
                    tree_semantic_check_fetch_identifier_type(node->nxt[0]);
                    tree_semantic_check_fetch_identifier_type(node->nxt[1]);
                    
                    if(node->nxt[1]->selftype->type != INT && node->nxt[1]->selftype->type != CHAR)
                    {
                        report_error(-1, "semantic error", "*/%&^| OP");
                    }
                    switch(node->nxt[0]->selftype->type)
                    {
                        case INT:
                            node->selftype = node_type_int_new();
                            node->selftype->isconst = 1;
                            node->selftype->id = "";
                            break;
                        case CHAR:
                            if(node->nxt[1]->selftype->type == CHAR)
                            {
                                node->selftype = node_type_char_new();
                            }
                            else
                            {
                                node->selftype = node_type_int_new();
                            }
                            node->selftype->isconst = 1;
                            node->selftype->id = "";
                            break;
                        default:
                            report_error(-1, "semantic error", "*/%&^| OP");
                            break;
                    }
                    ac = 1;

                    tree_translate_main(node, 0);
                    at = 1;
                    break;
            }
            break;
        case TYPE_SPECIFIER:
            switch((node->record).integer)
            {
                case VOID_T:
                    node->selftype = node_type_void_new();
                    node->selftype->isconst = 1;
                    node->selftype->id = "";
                    break;
                case CHAR_T:
                    node->selftype = node_type_char_new();
                    node->selftype->isconst = 1;
                    node->selftype->id = "";
                    break;
                case INT_T:
                    node->selftype = node_type_int_new();
                    node->selftype->isconst = 1;
                    node->selftype->id = "";
                    break;
                case TYPEDEF_NAME:
                    tree_semantic_check(node->nxt[0]);
                    node->selftype = node_type_type_new();
                    node->selftype->isconst = 1;
                    node->selftype->id = strdup(node->nxt[0]->selftype->id);
                    break;
                case STRUCT_T:
                    if(node->nxtnum == 1)
                    {
                        if(node->nxt[0]->type == IDENTIFIER)
                        {
                            tree_semantic_check(node->nxt[0]);
                        }
                        else
                        {
                            tree_semantic_check_begin_scope();
                            tree_semantic_check(node->nxt[0]);
                            tree_semantic_check_end_scope();
                        }
                    }
                    else //==2
                    {
                        tree_semantic_check(node->nxt[0]);
                        tree_semantic_check_begin_scope();
                        tree_semantic_check(node->nxt[1]);
                        tree_semantic_check_end_scope();
                    }
                    tree_semantic_check_type_specifier_record(node);
                    break;
                case UNION_T:
                    if(node->nxtnum == 1)
                    {
                        if(node->nxt[0]->type == IDENTIFIER)
                        {
                            tree_semantic_check(node->nxt[0]);
                        }
                        else
                        {
                            tree_semantic_check_begin_scope();
                            tree_semantic_check(node->nxt[0]);
                            tree_semantic_check_end_scope();
                        }
                    }
                    else //==2
                    {
                        tree_semantic_check(node->nxt[0]);
                        tree_semantic_check_begin_scope();
                        tree_semantic_check(node->nxt[1]);
                        tree_semantic_check_end_scope();
                    }
                    tree_semantic_check_type_specifier_record(node);
                    break;
            }
            ac = 1;
            break;
        case POSTFIX: //ok
            switch((node->record).integer)
            {
                case INC_OP: //在高层处理, ok
                    break;
                case DEC_OP: //在高层处理, ok
                    break;
                case PTR_OP: //其它部分在高层处理, ok
                    tree_semantic_check(node->nxt[0]);
                    node->selftype = node_type_copy_new(node->nxt[0]->selftype);
                    node->selftype->isconst = 0;
                    node->selftype->id = node->nxt[0]->selftype->id;
                    ac = 1;
                    break;
                case '(': //在高层处理, ok
                    /*
                    if(node->nxtnum == 1)
                    {
                        ;
                    }
                    else
                    {
                        ;
                    }
                    */
                    break;
                case '[':
                    tree_semantic_check(node->nxt[0]);
                    tree_semantic_check_fetch_identifier_type(node->nxt[0]);
                    if(node->nxt[0]->selftype->type != INT && node->nxt[0]->selftype->type != CHAR)
                    {
                        report_error(-1, "semantic error", "[exp] exp's type");
                    }
                    node->selftype = node_type_type_new();
                    node->selftype->isconst = 1;
                    node->selftype->id = "";
                    ac = 1;
                    
                    tree_translate_main(node, 0);
                    at = 1;
                    break;
                case '.': //部分在高层处理, ok
                    tree_semantic_check(node->nxt[0]);
                    node->selftype = node_type_copy_new(node->nxt[0]->selftype);
                    node->selftype->isconst = 0;
                    node->selftype->id = node->nxt[0]->selftype->id;
                    ac = 1;
                    break;
            }
            break;
        case PLAIN_DECLARATIONS: //在高层处理, ok
            break;
        case PLAIN_DECLARATION: //ok
            tree_semantic_check(node->nxt[0]);
            tree_semantic_check(node->nxt[1]);
            tree_semantic_check_plain_declaration(node);
            ac = 1;
            
            tree_translate_main(node, 0);
            at = 1;
            break;
        case ARGUMENTS: //在高层处理, ok
            at = 1;
            break;
        case TYPE_NAME: //ok
            switch((node->record).integer)
            {
                case 0:
                    tree_semantic_check(node->nxt[0]);
                    node->selftype = node_type_copy_new(node->nxt[0]->selftype);
                    node->selftype->isconst = 1;
                    node->selftype->id = strdup(node->nxt[0]->selftype->id);
                    break;
                case '*':
                    tree_semantic_check(node->nxt[0]);
                    node->selftype = node_type_pointer_new(node->nxt[0]->selftype);
                    node->selftype->isconst = 1;
                    node->selftype->id = strdup(node->nxt[0]->selftype->id);
                    break;
            }
            ac = 1;
            break;
        case STATEMENTS: //ok
            break;
        case STATEMENT: //ok
            switch((node->record).integer)
            {
                case EXP_STMT: //ok
                    if(node->nxtnum == 1)
                    {
                        tree_semantic_check(node->nxt[0]);
                    }
                    ac = 1;
                    tree_translate_main(node, 0);
                    at = 1;
                    break;
                case '{':
                    //tree_semantic_check_begin_scope();
                    break;
                case '}':
                    //tree_semantic_check_end_scope();
                    break;
                case IF_C:
                    if(node->nxtnum == 3)
                    {
                        //expression
                        tree_semantic_check(node->nxt[0]);
                        tree_semantic_check_fetch_identifier_type(node->nxt[0]);
                        if(node->nxt[0]->selftype->type != CHAR &&
                           node->nxt[0]->selftype->type != INT &&
                           node->nxt[0]->selftype->type != POINTER &&
                           node->nxt[0]->selftype->type != ARRAY)
                        {
                            report_error(-1, "semantic error", "if(exp) exp's type");
                        }
                        //这里还没有考虑typedef的影响
                        //statement
                        tree_semantic_check_begin_scope();
                        tree_semantic_check(node->nxt[1]);
                        tree_semantic_check_end_scope();
                        //statement
                        tree_semantic_check_begin_scope();
                        tree_semantic_check(node->nxt[2]);
                        tree_semantic_check_end_scope();
                    }
                    else //==2
                    {
                        //expression
                        tree_semantic_check(node->nxt[0]);
                        tree_semantic_check_fetch_identifier_type(node->nxt[0]);
                        if(node->nxt[0]->selftype->type != CHAR &&
                           node->nxt[0]->selftype->type != INT &&
                           node->nxt[0]->selftype->type != POINTER &&
                           node->nxt[0]->selftype->type != ARRAY)
                        {
                            report_error(-1, "semantic error", "if(exp) exp's type");
                        }
                        //statement
                        tree_semantic_check_begin_scope();
                        tree_semantic_check(node->nxt[1]);
                        tree_semantic_check_end_scope();
                    }
                    node->selftype = node_type_type_new();
                    node->selftype->isconst = 1;
                    node->selftype->id = "";
                    ac = 1;
                    
                    tree_translate_main(node, 0);
                    at = 1;
                    break;
                case WHILE_C:
                    tree_translate_main(node, 1);
                    //expression
                    tree_semantic_check(node->nxt[0]);
                    tree_semantic_check_fetch_identifier_type(node->nxt[0]);
                    if(node->nxt[0]->selftype->type != CHAR &&
                       node->nxt[0]->selftype->type != INT &&
                       node->nxt[0]->selftype->type != POINTER &&
                       node->nxt[0]->selftype->type != ARRAY)
                    {
                        report_error(-1, "semantic error", "if(exp) exp's type");
                    }
                    //statement
                    tscloopcount ++;
                    tree_semantic_check(node->nxt[1]);
                    tscloopcount --;
                    node->selftype = node_type_type_new();
                    node->selftype->isconst = 1;
                    node->selftype->id = "";
                    ac = 1;
                    
                    tree_translate_main(node, 0);
                    at = 1;
                    break;
                case FOR_C:
                    tree_translate_main(node, 1);
                    if(node->nxtnum == 4)
                    {
                        //expression statement
                        tree_semantic_check(node->nxt[0]);
                        //expression statement
                        tree_semantic_check(node->nxt[1]);
                        //expression
                        tree_semantic_check(node->nxt[2]);
                        //statement
                        tscloopcount ++;
                        tree_semantic_check_begin_scope();
                        tree_semantic_check(node->nxt[3]);
                        tree_semantic_check_end_scope();
                        tscloopcount --;
                    }
                    else //==3
                    {
                        //expression statement
                        tree_semantic_check(node->nxt[0]);
                        //expression statement
                        tree_semantic_check(node->nxt[1]);
                        //statement
                        tscloopcount ++;
                        tree_semantic_check_begin_scope();
                        tree_semantic_check(node->nxt[2]);
                        tree_semantic_check_end_scope();
                        tscloopcount --;
                    }
                    node->selftype = node_type_type_new();
                    node->selftype->isconst = 1;
                    node->selftype->id = "";
                    ac = 1;
                    
                    tree_translate_main(node, 0);
                    at = 1;
                    break;
                case BREAK_C:
                    if(tscloopcount < 1) report_error(-1, "semantic error", "break");
                    
                    tree_translate_main(node, 0);
                    at = 1;
                    break;
                case RETURN_C:
                    if(tscenv[tscenvnum-1].selftype == NULL)
                    {
                        report_error(-1, "semantic error", "return");
                    }
                    if(node->nxtnum == 0) //ok
                    {
                        ;
                    }
                    else
                    {
                        tree_semantic_check(node->nxt[0]);
                        tree_semantic_check_fetch_identifier_type(node->nxt[0]);
                        if(tscenv[tscenvnum-1].selftype->record.function.returntype->type == POINTER &&
                           (node->nxt[0]->selftype->type != POINTER && node->nxt[0]->selftype->type != INT && node->nxt[0]->selftype->type != CHAR))
                        {
                            report_error(-1, "semantic error", "types not matched in return");
                        }
                        if((tscenv[tscenvnum-1].selftype->record.function.returntype->type == INT ||
                           tscenv[tscenvnum-1].selftype->record.function.returntype->type == CHAR) &&
                           (node->nxt[0]->selftype->type != INT && node->nxt[0]->selftype->type != CHAR))
                        {
                            report_error(-1, "semantic error", "types not matched in return");
                        }
                        if(tscenv[tscenvnum-1].selftype->record.function.returntype->type != INT &&
                           tscenv[tscenvnum-1].selftype->record.function.returntype->type != CHAR &&
                           tscenv[tscenvnum-1].selftype->record.function.returntype->type != POINTER &&
                           !tree_semantic_check_same_type(tscenv[tscenvnum-1].selftype->record.function.returntype, node->nxt[0]->selftype))
                        {
                            report_error(-1, "semantic error", "types not matched in return");
                        }
                        node->selftype = node_type_type_new();
                        node->selftype->isconst = 1;
                        node->selftype->id = "";
                        ac = 1;
                    }
                    
                    tree_translate_main(node, 0);
                    at = 1;
                    break;
                case CONTINUE_C:
                    if(tscloopcount < 1) report_error(-1, "semantic error", "continue");
                    
                    tree_translate_main(node, 0);
                    at = 1;
                    break;
            }
            break;
        case DECLARATOR: //ok
            switch((node->record).integer)
            {
                case '(': //函数声明,不在这一层做处理,而是在更高层做处理
                    if(node->nxtnum == 1)
                    {
                        tree_semantic_check(node->nxt[0]);
                        node->selftype = node_type_function_new(0, NULL, NULL);
                        node->selftype->isconst = 1;
                        node->selftype->id = strdup(node->nxt[0]->selftype->id);
                    }
                    else //==2
                    {
                        tree_semantic_check(node->nxt[0]);
                        tree_semantic_check(node->nxt[1]);
                        node->selftype = node_type_function_new(0, NULL, NULL);
                        node->selftype->isconst = 1;
                        node->selftype->id = strdup(node->nxt[0]->selftype->id);
                    }
                    ac = 1;
                    break;
                case '[':
                    tree_semantic_check(node->nxt[0]);
                    tree_semantic_check(node->nxt[1]);
                    if(node->nxt[1]->selftype->type != INT && node->nxt[1]->selftype->type != CHAR)
                    {
                        report_error(-1, "semantic error", "[exp] exp's type");
                    }
                    if(node->nxt[1]->type == CONST) //普通数组
                        node->selftype = node_type_array_new(node->nxt[1]->trans->instr[node->nxt[1]->trans->num-1].rec[1].record, node->nxt[0]->selftype);
                    else //变长数组
                    {
                        node->selftype = node_type_array_new(-1, node->nxt[0]->selftype);
                        node_type_fix_size_vla(node->selftype, 1);
                    }
                    node->selftype->isconst = 0;
                    node->selftype->id = strdup(node->nxt[0]->selftype->id);
                    ac = 1;
                    tree_translate_main(node, node->nxt[1]->type!=CONST);
                    break;
                case 0:
                    tree_semantic_check(node->nxt[0]);
                    if(node->nxt[0]->type == IDENTIFIER)
                    {
                        node->selftype = node_type_type_new();
                        node->selftype->isconst = 0;
                        node->selftype->id = strdup(node->nxt[0]->selftype->id);
                    }
                    else
                    {
                        node->selftype = node_type_copy_new(node->nxt[0]->selftype);
                        node->selftype->isconst = 0;
                        node->selftype->id = strdup(node->nxt[0]->selftype->id);
                    }
                    ac = 1;
                    break;
                case '*':
                    tree_semantic_check(node->nxt[0]);
                    node->selftype = node_type_pointer_new(node->nxt[0]->selftype);
                    node->selftype->isconst = 0;
                    node->selftype->id = strdup(node->nxt[0]->selftype->id);
                    ac = 1;
                    break;
            }
            //at=1可能有问题, warning
            at = 1;
            break;
        case TYPEDEFNAME: //ok, 以后可能需要修改
            node->selftype = node_type_type_new();
            node->selftype->isconst = 1;
            node->selftype->id = strdup((node->record).string);
            ac = 1;
            break;
        case IDENTIFIER: //相关涉及到identifier的check到高层检查, ok
            node->selftype = node_type_type_new();
            node->selftype->isconst = 0;
            node->selftype->id = strdup((node->record).string);
            ac = 1;
            break;
        case CONST: //ok
            switch((node->record).string[0])
            {
                case '\'':
                    node->selftype = node_type_int_new();
                    node->selftype->isconst = 1;
                    node->selftype->id = "";
                    break;
                case '\"':
                    node->selftype = node_type_pointer_new(node_type_char_new());
                    node->selftype->isconst = 1;
                    node->selftype->id = "";
                    break;
                default:
                    node->selftype = node_type_int_new();
                    node->selftype->isconst = 1;
                    node->selftype->id = "";
                    break;
            }
            ac = 1; 
            
            tree_translate_main(node, 0);
            at = 1;
            break;
        case ELLIPSIS: //ok
            break;
        case TYPEDEF: //ok
            break;
    }
    if(!ac)
    {
        for(i=0; i<node->nxtnum; i++)
        {
            tree_semantic_check(node->nxt[i]);
        }
        node->selftype = node_type_type_new();
        node->selftype->isconst = 1;
        node->selftype->id = "";
    }
    if(!at)
    {
        for(i=0; i<node->nxtnum; i++)
        {
            node->trans = tree_translate_merge(2, node->trans, node->nxt[i]->trans);
        }
    }
}

void tree_semantic_check_init()
{
    NODE_TYPE *argumenttype;
    
    tree_semantic_check_begin_scope();
    
    argumenttype = (NODE_TYPE *)malloc(sizeof(NODE_TYPE) * 2);
    argumenttype[0] = *(node_type_pointer_new(node_type_char_new()));
    argumenttype[1] = *(node_type_type_new());
    tree_semantic_check_add_funentry("printf", node_type_function_new(2, argumenttype, node_type_int_new()), 0);
    
    argumenttype = (NODE_TYPE *)malloc(sizeof(NODE_TYPE) * 2);
    argumenttype[0] = *(node_type_pointer_new(node_type_char_new()));
    argumenttype[1] = *(node_type_type_new());
    tree_semantic_check_add_funentry("scanf", node_type_function_new(2, argumenttype, node_type_int_new()), 0);
    
    argumenttype = (NODE_TYPE *)malloc(sizeof(NODE_TYPE) * 1);
    argumenttype[0] = *(node_type_int_new());
    tree_semantic_check_add_funentry("malloc", node_type_function_new(1, argumenttype, node_type_pointer_new(node_type_void_new())), 0);
    
    tree_translate_init();
}

void tree_semantic_check_end()
{
    tree_translate_end();
    
    tree_semantic_check_end_scope();
}

