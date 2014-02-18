#ifndef TRANSLATE_OFF
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "def.h"
#include "translate.h"
#include "ast.h"
#include "y.tab.h"
#include "semantics.h"
#include "utility.h"

/*
规范：
LABEL   #n          //例:LABEL 1
GOTO    #n          //例:GOTO 2
SHL     $a  $b  $c  //a=b<<c
SHR     $a  $b  $c  //a=b>>c
MUL     $a  $b  $c  //a=b*c
DIV     $a  $b  $c  //a=b/c
MOD     $a  $b  $c  //a=b%c
ADD     $a  $b  $c  //a=b+c
SUB     $a  $b  $c  //a=b-c
AND     $a  $b  $c  //a=b&c
XOR     $a  $b  $c  //a=b^c
OR      $a  $b  $c  //a=b|c
LAND    $a  $b  $c  //a=b&&c
LOR     $a  $b  $c  //a=b||c
EQ      $a  $b  $c  //a=b==c
NE      $a  $b  $c  //a=b!=c
LE      $a  $b  $c  //a=b<=c
GE      $a  $b  $c  //a=b>=c
GT      $a  $b  $c  //a=b>c
LT      $a  $b  $c  //a=b<c
NOT     $a  $b      //a=~b
LNOT    $a  $b      //a=!b
IF      $a  $b      //if(a) goto b
CALL    $a          //goto a 函数调用, 可以隐含一些保存状态的操作
RET                 //return, 可隐含一些操作
PUSH    $a          //push a
POP     $a          //pop a
TINT    $a  $b      //a=(int)b;
TCHAR   $a  $b      //a=(char)b;
MOVE    $a  $b      //a=b
ADS     $a  $b      //a=&b
STAR    $a  $b      //a<-*b 到机器代码时将会用名a代替*b, a是一个符号
NEW     $a  $b      //a=malloc(b)
FUNC    #n          //同label, 特用于表示函数
END                 //用于表示函数结束, 一定程度上等价于RET
AUG     $a          //aug a
GAUG    $a          //gaug a  
*/

NODE_TRANS_RECORD ttmainlabel;
NODE_TRANS *ttglobaltrans;

NODE_TRANS_INSTR_FORMAT ttinstrformat[] =
{
    {"LABEL", 1}, {"GOTO", 1}, {"SHL", 3}, {"SHR", 3}, {"MUL", 3},
    {"DIV", 3}, {"MOD", 3}, {"ADD", 3}, {"SUB", 3}, {"AND", 3},
    {"XOR", 3}, {"OR", 3}, {"LAND", 3}, {"LOR", 3}, {"EQ", 3},
    {"NE", 3}, {"LE", 3}, {"GE", 3}, {"GT", 3}, {"LT", 3},
    {"NOT", 2}, {"LNOT", 2}, {"IF", 2}, {"CALL", 1}, {"RET", 0},
    {"PUSH", 1}, {"POP", 1}, {"TINT", 2}, {"TCHAR", 2}, {"MOVE", 2},
    {"ADS", 2}, {"STAR", 2}, {"NEW", 2}, {"FUNC", 1}, {"END", 0},
    {"AUG", 1}, {"GAUG", 1}
};

NODE_TRANS_VLA * tree_translate_new_vla(int len)
{
    NODE_TRANS_VLA *node;
    node = (NODE_TRANS_VLA *)malloc(sizeof(NODE_TRANS_VLA));
    node->pos = tree_translate_record_new_address(4);
    if(len != -1)
    {
        node->trans = tree_translate_new();
        node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MOVE", 2,
            node->pos, tree_translate_record_new_const(len)));
    }
    return node;
}

NODE_TRANS_RECORD tree_translate_record_new_const(int rec)
{
    NODE_TRANS_RECORD record;
    record.type = TRANS_CONST;
    record.record = rec;
    record.flag = 0;
    return record;
}

NODE_TRANS_RECORD tree_translate_record_new_string(int rec)
{
    NODE_TRANS_RECORD record;
    record.type = TRANS_CONST;
    record.record = rec;
    record.flag = 1;
    return record;
}

NODE_TRANS_RECORD tree_translate_record_new_address(int len)
{
    static int address = 0;
    NODE_TRANS_RECORD record;
    record.type = TRANS_ADDRESS;
    record.record = address ++;
    record.flag = len;
    return record;
}

NODE_TRANS_RECORD tree_translate_record_new_label()
{
    static int label = 0;
    NODE_TRANS_RECORD record;
    record.type = TRANS_LABEL;
    record.record = label ++;
    return record;
}

NODE_TRANS_RECORD tree_translate_record_new_symbol(int len)
{
    static int symbol = 0;
    NODE_TRANS_RECORD record;
    record.type = TRANS_SYMBOL;
    record.record = symbol ++;
    record.flag = len;
    return record;
}

NODE_TRANS * tree_translate_new()
{
    NODE_TRANS *node;
    node = (NODE_TRANS *)malloc(sizeof(NODE_TRANS));
    node->size = 1;
    node->num = 0;
    node->instr = (NODE_TRANS_INSTR *)malloc(sizeof(NODE_TRANS_INSTR));
    return node;
}

void tree_translate_enlarge(NODE_TRANS *node)
{
    NODE_TRANS_INSTR *p;
    int i;
    if(node->num < node->size) return;
    p = (NODE_TRANS_INSTR *)malloc(sizeof(NODE_TRANS_INSTR) * node->size * 2);
    for(i=0; i<node->size; i++)
    {
        p[i] = node->instr[i];
    }
    free(node->instr);
    node->instr = p;
    node->size *= 2;
}

NODE_TRANS * tree_translate_makelist(char *str, int num, ...)
{
    va_list ap;
    int i;
    NODE_TRANS *node;
    va_start(ap, num);
    node = tree_translate_new();
    node->num = 1;
    for(i=0; i<num; i++)
    {
        node->instr[0].rec[i] = va_arg(ap, NODE_TRANS_RECORD);
    }
    node->instr[0].op = str;
    return node;
}

NODE_TRANS * tree_translate_merge(int num, ...) //注意：merge后原节点的内容将被删除
{
    va_list ap;
    int i, j;
    NODE_TRANS *node;
    NODE_TRANS *p;
    va_start(ap, num);
    node = tree_translate_new();
    node->num = 0;
    for(i=0; i<num; i++)
    {
        p = va_arg(ap, NODE_TRANS *);
        for(j=0; j<p->num; j++)
        {
            node->num ++;
            tree_translate_enlarge(node);
            node->instr[node->num-1] = p->instr[j];
        }
        free(p->instr);
        p->instr = NULL;
        p->num = p->size = 0;
    }
    return node;
}

NODE_TRANS_RECORD tree_translate_check_symbol(char *id)
{
    int i, j;
    NODE_TRANS_RECORD r;
    for(i=tscenvnum-1; i>=0; i--)
    {
        for(j=0; j<tscenv[i].ventrynum; j++)
        {
            if(strcmp(tscenv[i].ventry[j].symbol, id) == 0)
            {
                return tscenv[i].ventry[j].rec;
            }
        }               
    }
    r.type = TRANS_ADDRESS;
    r.record = 0;
    return r;
}

void tree_translate_fetch_identifier(NODE_T *node)
{
    if(node->type != IDENTIFIER) return;
    node->trans = tree_translate_merge(2, node->trans,
        tree_translate_makelist("OR", 3,
            tree_translate_check_symbol((node->record).string),
            tree_translate_check_symbol((node->record).string),
            tree_translate_check_symbol((node->record).string)
        )
    );
}

int tree_translate_expression_precal(NODE_T *node)
{
    switch((node->record).integer)
    {
        case SHL_ASSIGN:
        case SHR_ASSIGN:
        case MUL_ASSIGN:
        case DIV_ASSIGN:
        case MOD_ASSIGN:
        case ADD_ASSIGN:
        case SUB_ASSIGN:
        case AND_ASSIGN:
        case XOR_ASSIGN:
        case OR_ASSIGN:
            break;
        case OR_OP:
            if(node->nxt[0]->type == CONST && node->nxt[1]->type == CONST)
            {
                node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MOVE", 2, tree_translate_record_new_address(4),
                    tree_translate_record_new_const(node->nxt[0]->trans->instr[node->nxt[0]->trans->num-1].rec[1].record ||
                        node->nxt[1]->trans->instr[node->nxt[1]->trans->num-1].rec[1].record)
                    )
                );
                return 1;
            }
            break;
        case AND_OP:
            if(node->nxt[0]->type == CONST && node->nxt[1]->type == CONST)
            {
                node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MOVE", 2, tree_translate_record_new_address(4),
                    tree_translate_record_new_const(node->nxt[0]->trans->instr[node->nxt[0]->trans->num-1].rec[1].record &&
                        node->nxt[1]->trans->instr[node->nxt[1]->trans->num-1].rec[1].record)
                    )
                );
                return 1;
            }
            break;
        case EQ_OP:
            if(node->nxt[0]->type == CONST && node->nxt[1]->type == CONST)
            {
                node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MOVE", 2, tree_translate_record_new_address(4),
                    tree_translate_record_new_const(node->nxt[0]->trans->instr[node->nxt[0]->trans->num-1].rec[1].record ==
                        node->nxt[1]->trans->instr[node->nxt[1]->trans->num-1].rec[1].record)
                    )
                );
                return 1;
            }
            break;
        case NE_OP:
            if(node->nxt[0]->type == CONST && node->nxt[1]->type == CONST)
            {
                node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MOVE", 2, tree_translate_record_new_address(4),
                    tree_translate_record_new_const(node->nxt[0]->trans->instr[node->nxt[0]->trans->num-1].rec[1].record !=
                        node->nxt[1]->trans->instr[node->nxt[1]->trans->num-1].rec[1].record)
                    )
                );
                return 1;
            }
            break;
        case LE_OP:
            if(node->nxt[0]->type == CONST && node->nxt[1]->type == CONST)
            {
                node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MOVE", 2, tree_translate_record_new_address(4),
                    tree_translate_record_new_const(node->nxt[0]->trans->instr[node->nxt[0]->trans->num-1].rec[1].record <=
                        node->nxt[1]->trans->instr[node->nxt[1]->trans->num-1].rec[1].record)
                    )
                );
                return 1;
            }
            break;
        case GE_OP:
            if(node->nxt[0]->type == CONST && node->nxt[1]->type == CONST)
            {
                node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MOVE", 2, tree_translate_record_new_address(4),
                    tree_translate_record_new_const(node->nxt[0]->trans->instr[node->nxt[0]->trans->num-1].rec[1].record >=
                        node->nxt[1]->trans->instr[node->nxt[1]->trans->num-1].rec[1].record)
                    )
                );
                return 1;
            }
            break;
        case '>':
            if(node->nxt[0]->type == CONST && node->nxt[1]->type == CONST)
            {
                node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MOVE", 2, tree_translate_record_new_address(4),
                    tree_translate_record_new_const(node->nxt[0]->trans->instr[node->nxt[0]->trans->num-1].rec[1].record >
                        node->nxt[1]->trans->instr[node->nxt[1]->trans->num-1].rec[1].record)
                    )
                );
                return 1;
            }
            break;
        case '<':
            if(node->nxt[0]->type == CONST && node->nxt[1]->type == CONST)
            {
                node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MOVE", 2, tree_translate_record_new_address(4),
                    tree_translate_record_new_const(node->nxt[0]->trans->instr[node->nxt[0]->trans->num-1].rec[1].record <
                        node->nxt[1]->trans->instr[node->nxt[1]->trans->num-1].rec[1].record)
                    )
                );
                return 1;
            }
            break;
        case SHL_OP:
            if(node->nxt[0]->type == CONST && node->nxt[1]->type == CONST)
            {
                node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MOVE", 2, tree_translate_record_new_address(4),
                    tree_translate_record_new_const(node->nxt[0]->trans->instr[node->nxt[0]->trans->num-1].rec[1].record <<
                        node->nxt[1]->trans->instr[node->nxt[1]->trans->num-1].rec[1].record)
                    )
                );
                return 1;
            }
            break;
        case SHR_OP:
            if(node->nxt[0]->type == CONST && node->nxt[1]->type == CONST)
            {
                node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MOVE", 2, tree_translate_record_new_address(4),
                    tree_translate_record_new_const(node->nxt[0]->trans->instr[node->nxt[0]->trans->num-1].rec[1].record >>
                        node->nxt[1]->trans->instr[node->nxt[1]->trans->num-1].rec[1].record)
                    )
                );
                return 1;
            }
            break;
        case SIZEOF_OP:
            //注意对于变长数组还需重新考虑
            //warning
            tree_translate_fetch_identifier(node->nxt[0]);
            node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MOVE", 2,
                tree_translate_record_new_address(4),
                tree_translate_record_new_const(node->nxt[0]->selftype->size)));
            return 1;
            break;
        case INC_OP: //不可能遇到
        case DEC_OP: //不可能遇到
        case PTR_OP: //不可能遇到
            break;
        case INC_OP_U:
        case DEC_OP_U:
            break;
        case CAST_EXP:
            if(node->nxt[1]->type == CONST)
            {
                if(node->nxt[0]->selftype->type == CHAR)
                {
                    node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MOVE", 2, tree_translate_record_new_address(4),
                        tree_translate_record_new_const((char)(node->nxt[1]->trans->instr[node->nxt[1]->trans->num-1].rec[1].record)))
                    );
                }
                else //==INT
                {
                    node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MOVE", 2, tree_translate_record_new_address(4),
                        tree_translate_record_new_const((int)(node->nxt[1]->trans->instr[node->nxt[1]->trans->num-1].rec[1].record)))
                    );
                }
                return 1;
            }
            break;
        case BRACKET_EXP:
            if(node->nxt[0]->type == CONST)
            {
                node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MOVE", 2, tree_translate_record_new_address(4),
                    tree_translate_record_new_const(node->nxt[0]->trans->instr[node->nxt[0]->trans->num-1].rec[1].record))
                );
                return 1;
            }
            break;
        case POSTFIX_EXP:
            break;
        case ',':
            if(node->nxt[0]->type == CONST && node->nxt[1]->type == CONST)
            {
                node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MOVE", 2, tree_translate_record_new_address(4),
                    tree_translate_record_new_const(node->nxt[1]->trans->instr[node->nxt[1]->trans->num-1].rec[1].record))
                );
                return 1;
            }
            break;
        case '=':
            break;
        case '+':
            if(node->nxtnum == 1)
            {
                if(node->nxt[0]->type == CONST)
                {
                    node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MOVE", 2, tree_translate_record_new_address(4),
                        tree_translate_record_new_const(node->nxt[0]->trans->instr[node->nxt[0]->trans->num-1].rec[1].record))
                    );
                    return 1;
                }
            }
            else //==2
            {
                if(node->nxt[0]->type == CONST && node->nxt[1]->type == CONST)
                {
                    node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MOVE", 2, tree_translate_record_new_address(4),
                        tree_translate_record_new_const(node->nxt[0]->trans->instr[node->nxt[0]->trans->num-1].rec[1].record +
                            node->nxt[1]->trans->instr[node->nxt[1]->trans->num-1].rec[1].record)
                        )
                    );
                    return 1;
                }
            }
            break;
        case '-':
            if(node->nxtnum == 1)
            {
                if(node->nxt[0]->type == CONST)
                {
                    node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MOVE", 2, tree_translate_record_new_address(4),
                        tree_translate_record_new_const(-node->nxt[0]->trans->instr[node->nxt[0]->trans->num-1].rec[1].record))
                    );
                    return 1;
                }
            }
            else //==2
            {
                if(node->nxt[0]->type == CONST && node->nxt[1]->type == CONST)
                {
                    node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MOVE", 2, tree_translate_record_new_address(4),
                        tree_translate_record_new_const(node->nxt[0]->trans->instr[node->nxt[0]->trans->num-1].rec[1].record -
                            node->nxt[1]->trans->instr[node->nxt[1]->trans->num-1].rec[1].record)
                        )
                    );
                    return 1;
                }
            }
            break;
        case '~':
            if(node->nxt[0]->type == CONST)
            {
                node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MOVE", 2, tree_translate_record_new_address(4),
                    tree_translate_record_new_const(~node->nxt[0]->trans->instr[node->nxt[0]->trans->num-1].rec[1].record))
                );
                return 1;
            }
            break;
        case '!':
            if(node->nxt[0]->type == CONST)
            {
                node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MOVE", 2, tree_translate_record_new_address(4),
                    tree_translate_record_new_const(!node->nxt[0]->trans->instr[node->nxt[0]->trans->num-1].rec[1].record))
                );
                return 1;
            }
            break;
        case '*':
            if(node->nxtnum == 1)
            {
                break;
            }
            else //==2
            {
                if(node->nxt[0]->type == CONST && node->nxt[1]->type == CONST)
                {
                    node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MOVE", 2, tree_translate_record_new_address(4),
                        tree_translate_record_new_const(node->nxt[0]->trans->instr[node->nxt[0]->trans->num-1].rec[1].record *
                            node->nxt[1]->trans->instr[node->nxt[1]->trans->num-1].rec[1].record)
                        )
                    );
                    return 1;
                }
            }
            break;
        case '&':
            if(node->nxtnum == 1)
            {
                break;
            }
            else //==2
            {
                if(node->nxt[0]->type == CONST && node->nxt[1]->type == CONST)
                {
                    node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MOVE", 2, tree_translate_record_new_address(4),
                        tree_translate_record_new_const(node->nxt[0]->trans->instr[node->nxt[0]->trans->num-1].rec[1].record &
                            node->nxt[1]->trans->instr[node->nxt[1]->trans->num-1].rec[1].record)
                        )
                    );
                    return 1;
                }
            }
            break;
        case '/':
            if(node->nxt[0]->type == CONST && node->nxt[1]->type == CONST)
            {
                node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MOVE", 2, tree_translate_record_new_address(4),
                    tree_translate_record_new_const(node->nxt[0]->trans->instr[node->nxt[0]->trans->num-1].rec[1].record /
                        node->nxt[1]->trans->instr[node->nxt[1]->trans->num-1].rec[1].record)
                    )
                );
                return 1;
            }
            break;
        case '%':
            if(node->nxt[0]->type == CONST && node->nxt[1]->type == CONST)
            {
                node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MOVE", 2, tree_translate_record_new_address(4),
                    tree_translate_record_new_const(node->nxt[0]->trans->instr[node->nxt[0]->trans->num-1].rec[1].record %
                        node->nxt[1]->trans->instr[node->nxt[1]->trans->num-1].rec[1].record)
                    )
                );
                return 1;
            }
            break;
        case '^':
            if(node->nxt[0]->type == CONST && node->nxt[1]->type == CONST)
            {
                node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MOVE", 2, tree_translate_record_new_address(4),
                    tree_translate_record_new_const(node->nxt[0]->trans->instr[node->nxt[0]->trans->num-1].rec[1].record ^
                        node->nxt[1]->trans->instr[node->nxt[1]->trans->num-1].rec[1].record)
                    )
                );
                return 1;
            }
            break;
        case '|':
            if(node->nxt[0]->type == CONST && node->nxt[1]->type == CONST)
            {
                node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MOVE", 2, tree_translate_record_new_address(4),
                    tree_translate_record_new_const(node->nxt[0]->trans->instr[node->nxt[0]->trans->num-1].rec[1].record |
                        node->nxt[1]->trans->instr[node->nxt[1]->trans->num-1].rec[1].record)
                    )
                );
                return 1;
            }
            break;

    }
    return 0;
}

int tree_translate_cal_record_offset(NODE_TYPE *node, char *id, NODE_TYPE **p)
{
    int i;
    int offset = 0;
    if(node->type == UNION) return offset;
    for(i=0; i<node->record.record.size; i++)
    {
        if(strcmp(node->record.record.field[i].name, id) == 0)
        {
            *p = node->record.record.field[i].type;
            return offset;
        }
        offset += node->record.record.field[i].type->size;
        while(offset % 4) offset ++;
    }
    return offset; //实际上不可能到达这里
}

void tree_translate_fix_array(NODE_T *node)
{
    if(node->selftype->type != ARRAY) return;
    if(node->type == IDENTIFIER)
    {
        node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("ADS", 2,
            tree_translate_record_new_address(4),
            node->trans->instr[node->trans->num-1].rec[0]));
    }
}

void tree_translate_expression_1(NODE_T *node) //第一类表达式, 2目运算的+,-,*,/,%,&,^,|,<<,>>,!=,==,>=,<=,>,<
{
    char *op;
    int size;
    int offset;
    switch((node->record).integer)
    {
        case '+': op = "ADD"; break;
        case '-': op = "SUB"; break;
        case '*': op = "MUL"; break;
        case '/': op = "DIV"; break;
        case '%': op = "MOD"; break;
        case '&': op = "AND"; break;
        case '^': op = "XOR"; break;
        case '|': op = "OR"; break;
        case SHL_OP: op = "SHL"; break;
        case SHR_OP: op = "SHR"; break;
        case NE_OP: op = "NE"; break;
        case LE_OP: op = "LE"; break;
        case GE_OP: op = "GE"; break;
        case EQ_OP: op = "EQ"; break;
        case '>': op = "GT"; break;
        case '<': op = "LT"; break;
        default: op = ""; break;
    }
    tree_translate_fetch_identifier(node->nxt[0]);
    tree_translate_fetch_identifier(node->nxt[1]);
    tree_translate_fix_array(node->nxt[0]);
    tree_translate_fix_array(node->nxt[1]);
    //warning: 可能需要考虑typedef的情况
    //类型转换
    size = 1;
    if(node->nxt[0]->selftype->type != CHAR || node->nxt[1]->selftype->type != CHAR)
    {
        size = 4;
        if(node->nxt[0]->selftype->type == CHAR)
        {
            node->nxt[0]->trans = tree_translate_merge(2, node->nxt[0]->trans, tree_translate_makelist("TINT", 2, tree_translate_record_new_address(4),
                node->nxt[0]->trans->instr[node->nxt[0]->trans->num-1].rec[0]));
        }
        if(node->nxt[1]->selftype->type == CHAR)
        {
            node->nxt[1]->trans = tree_translate_merge(2, node->nxt[1]->trans, tree_translate_makelist("TINT", 2, tree_translate_record_new_address(4),
                node->nxt[1]->trans->instr[node->nxt[1]->trans->num-1].rec[0]));
        }
    }
    //处理指针的加减
    if(((node->record).integer == '+' || (node->record).integer == '-') &&
       (node->nxt[0]->selftype->type == POINTER || node->nxt[0]->selftype->type == ARRAY) &&
       node->nxt[1]->selftype->type != POINTER &&
       node->nxt[1]->selftype->type != ARRAY)
    {
        if(node->nxt[0]->selftype->type == POINTER)
            offset = node->nxt[0]->selftype->record.pointer->size;
        else
            offset = node_type_cal_size_r_v(node->nxt[0]->selftype->record.array.pointer) * node->nxt[0]->selftype->record.array.capacity;
    }
    else
    {
        offset = 1;
    }
    node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MUL", 3,
        tree_translate_record_new_address(4),
        node->nxt[1]->trans->instr[node->nxt[1]->trans->num-1].rec[0],
        tree_translate_record_new_const(offset)));
    node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist(op, 3, tree_translate_record_new_address(size),
        node->nxt[0]->trans->instr[node->nxt[0]->trans->num-1].rec[0],
        node->trans->instr[node->trans->num-1].rec[0]
        //node->nxt[1]->trans->instr[node->trans->num-1].rec[0])
        ));
    node->trans = tree_translate_merge(2, node->nxt[1]->trans, node->trans);
    node->trans = tree_translate_merge(2, node->nxt[0]->trans, node->trans);
}

void tree_translate_expression_2(NODE_T *node) //第二类表达式, =, <<=, >>=, *=, /=, %=, +=, -=, |=, &=, ^=
{
    char *op;
    int size;
    int offset;
    switch((node->record).integer)
    {
        case SHL_ASSIGN: op = "SHL"; break;
        case SHR_ASSIGN: op = "SHR"; break;
        case MUL_ASSIGN: op = "MUL"; break;
        case DIV_ASSIGN: op = "DIV"; break;
        case MOD_ASSIGN: op = "MOD"; break;
        case ADD_ASSIGN: op = "ADD"; break;
        case SUB_ASSIGN: op = "SUB"; break;
        case AND_ASSIGN: op = "AND"; break;
        case XOR_ASSIGN: op = "XOR"; break;
        case OR_ASSIGN: op = "OR"; break;        
        default: op = ""; break;
    }
    if((node->record).integer != '=')
    {
        tree_translate_fetch_identifier(node->nxt[0]);
        tree_translate_fetch_identifier(node->nxt[1]);
        tree_translate_fix_array(node->nxt[0]);
        tree_translate_fix_array(node->nxt[1]);
        //warning: 可能需要考虑typedef的情况
        //类型转换
        size = 1;
        if(node->nxt[0]->selftype->type != CHAR || node->nxt[1]->selftype->type != CHAR)
        {
            size = 4;
            if(node->nxt[0]->selftype->type == CHAR)
            {
                node->nxt[0]->trans = tree_translate_merge(2, node->nxt[0]->trans, tree_translate_makelist("TINT", 2, tree_translate_record_new_address(4),
                    node->nxt[0]->trans->instr[node->nxt[0]->trans->num-1].rec[0]));
            }
            if(node->nxt[1]->selftype->type == CHAR)
            {
                node->nxt[1]->trans = tree_translate_merge(2, node->nxt[1]->trans, tree_translate_makelist("TINT", 2, tree_translate_record_new_address(4),
                    node->nxt[1]->trans->instr[node->nxt[1]->trans->num-1].rec[0]));
            }
        }
        //处理指针的加减
        if(((node->record).integer==ADD_ASSIGN || (node->record).integer==SUB_ASSIGN) &&
           node->nxt[0]->selftype->type == POINTER &&
           node->nxt[1]->selftype->type != POINTER &&
           node->nxt[1]->selftype->type != ARRAY
          )
        {
            offset = node->nxt[0]->selftype->record.pointer->size;
        }
        else
        {
            offset = 1;
        }
        node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MUL", 3,
            tree_translate_record_new_address(4),
            node->nxt[1]->trans->instr[node->nxt[1]->trans->num-1].rec[0],
            tree_translate_record_new_const(offset)));
        node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist(op, 3, tree_translate_record_new_address(size),
            node->nxt[0]->trans->instr[node->nxt[0]->trans->num-1].rec[0],
            node->trans->instr[node->trans->num-1].rec[0]));
        if(node->nxt[0]->selftype->type == CHAR && node->nxt[1]->selftype->type != CHAR)
        {
            node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("TCHAR", 2, tree_translate_record_new_address(1),
                node->trans->instr[node->trans->num-1].rec[0]));
        }
        node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MOVE", 2, tree_translate_check_symbol((node->nxt[0]->record).string), 
            node->trans->instr[node->trans->num-1].rec[0]));
    }
    else
    {
        tree_translate_fetch_identifier(node->nxt[0]);
        tree_translate_fetch_identifier(node->nxt[1]);
        tree_translate_fix_array(node->nxt[0]);
        tree_translate_fix_array(node->nxt[1]);
        //类型转换
        if(node->nxt[0]->selftype->type == CHAR && node->nxt[1]->selftype->type != CHAR)
        {
            node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("TCHAR", 2, tree_translate_record_new_address(1),
                node->nxt[1]->trans->instr[node->nxt[1]->trans->num-1].rec[0]));
            node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MOVE", 2, node->nxt[0]->trans->instr[node->nxt[0]->trans->num-1].rec[0], 
                node->trans->instr[node->trans->num-1].rec[0]));
        }
        else
        {
            node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MOVE", 2, node->nxt[0]->trans->instr[node->nxt[0]->trans->num-1].rec[0], 
                node->nxt[1]->trans->instr[node->nxt[1]->trans->num-1].rec[0]));
        }
    }
    node->trans = tree_translate_merge(2, node->nxt[1]->trans, node->trans);
    node->trans = tree_translate_merge(2, node->nxt[0]->trans, node->trans);
}

/*
a && b
a
if(!a) goto l1;
b
goto l2;
l1: exp = 0
    goto l3;
l2: exp = b
    goto l3;
l3:
*/
void tree_translate_expression_3(NODE_T *node, int flag) //第三类表达式, ||, &&
{
    NODE_TRANS_RECORD l1, l2, l3, l4;
    NODE_TRANS_RECORD a;
    int p;
    
    tree_translate_fetch_identifier(node->nxt[0]);
    tree_translate_fetch_identifier(node->nxt[1]);
    tree_translate_fix_array(node->nxt[0]);
    tree_translate_fix_array(node->nxt[1]);
    l1 = tree_translate_record_new_label(); //短路
    l2 = tree_translate_record_new_label();
    l3 = tree_translate_record_new_label();
    l4 = tree_translate_record_new_label();
    a = tree_translate_record_new_address(4);
    //目前这里假定L类运算支持char和int，否则，以下部分需要增加类型转换
    //warning
    if(flag == 1) //&&
    {
        node->nxt[0]->trans = tree_translate_merge(2, node->nxt[0]->trans, tree_translate_makelist("LNOT", 2,
            tree_translate_record_new_address(4), node->nxt[0]->trans->instr[node->nxt[0]->trans->num-1].rec[0]));
    }
    node->trans = tree_translate_merge(2, node->trans, node->nxt[0]->trans);
    node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("IF", 2,
        node->trans->instr[node->trans->num-1].rec[0], l1));
    node->trans = tree_translate_merge(2, node->trans, node->nxt[1]->trans);
    p = node->trans->num - 1;
    node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("GOTO", 1, l2));
    node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("LABEL", 1, l1));
    if(flag == 1) //&&
        node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MOVE", 2, a, tree_translate_record_new_const(0)));
    else //||
        node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MOVE", 2, a, tree_translate_record_new_const(1)));
    node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("GOTO", 1, l4));
    node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("LABEL", 1, l2));
    if(flag == 1) //&&
        node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MOVE", 2, a, node->trans->instr[p].rec[0]/*, tree_translate_record_new_const(1)*/)); //LAND
    else //||
        node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MOVE", 2, a, node->trans->instr[p].rec[0]/*, tree_translate_record_new_const(0)*/)); //LOR
    node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("LABEL", 1, l3));
    node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("LNOT", 2, tree_translate_record_new_address(4), a));
    node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("IF", 2,
        node->trans->instr[node->trans->num-1].rec[0], l4));
    node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MOVE", 2, a, tree_translate_record_new_const(1)));
    node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("LABEL", 1, l4));
    node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("OR", 3, a, a, a));
}

void tree_translate_expression_4(NODE_T *node) //第四类表达式, !, ~
{
    char *op;
    int size;
    switch((node->record).integer)
    {
        case '!': op = "LNOT"; break;
        case '~': op = "NOT"; break;        
        default: op = ""; break;
    }
    tree_translate_fetch_identifier(node->nxt[0]);
    tree_translate_fix_array(node->nxt[0]);
    node->trans = tree_translate_merge(2, node->trans, node->nxt[0]->trans);
    size = node->nxt[0]->selftype->size;
    if((node->record).integer == '!') //类型转换
    {
        size = 4;
        if(node->nxt[0]->selftype->type == CHAR)
        {
            node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("TINT", 2, tree_translate_record_new_address(4),
                node->trans->instr[node->trans->num-1].rec[0]));
        }
    }
    node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist(op, 2, tree_translate_record_new_address(size),
        node->trans->instr[node->trans->num-1].rec[0]));
}

void tree_translate_expression_5(NODE_T *node) //第五类表达式, ++x, --x
{
    char *op;
    int offset;
    switch((node->record).integer)
    {
        case INC_OP_U: op = "ADD"; break;
        case DEC_OP_U: op = "SUB"; break;        
        default: op = ""; break;
    }
    tree_translate_fetch_identifier(node->nxt[0]);
    node->trans = tree_translate_merge(2, node->trans, node->nxt[0]->trans);
    //处理指针的加减
    if(node->nxt[0]->selftype->type == POINTER)
    {
        offset = node->nxt[0]->selftype->record.pointer->size;
    }
    else
    {
        offset = 1;
    }
    node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist(op, 3,
        node->trans->instr[node->trans->num-1].rec[0],
        node->trans->instr[node->trans->num-1].rec[0],
        tree_translate_record_new_const(offset)));
}

void tree_translate_expression_6(NODE_T *node) //第六类表达式, 强制类型转换 cast
{
    char *op;
    if(node->nxt[0]->selftype->type == CHAR)
        op = "TCHAR";
    else
        op = "TINT";
    tree_translate_fetch_identifier(node->nxt[1]);
    tree_translate_fix_array(node->nxt[1]);
    node->trans = tree_translate_merge(2, node->trans, node->nxt[1]->trans);
    node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist(op, 2,
        tree_translate_record_new_address(node->nxt[0]->selftype->size),
        node->trans->instr[node->trans->num-1].rec[0]));
}

void tree_translate_expression_7(NODE_T *node) //第七类表达式，postfix
{
    char *op;
    NODE_TRANS_RECORD a;
    int i;
    int offset;
    
    switch((node->nxt[1]->record).integer)
    {
        case INC_OP:
        case DEC_OP:
            tree_translate_fetch_identifier(node->nxt[0]);
            tree_translate_fix_array(node->nxt[0]);
            op = (node->nxt[1]->record).integer == INC_OP ? "ADD" : "SUB";
            node->trans = tree_translate_merge(2, node->trans, node->nxt[0]->trans);
            a = tree_translate_record_new_address(node->nxt[0]->selftype->size);
            node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MOVE", 2,
                a, node->trans->instr[node->trans->num-1].rec[0]));
            if(node->nxt[0]->selftype->type == POINTER) //处理指针的加减
            {
                offset = node->nxt[0]->selftype->record.pointer->size;
            }
            else
            {
                offset = 1;
            }
            node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist(op, 3,
                node->trans->instr[node->trans->num-2].rec[0],
                node->trans->instr[node->trans->num-2].rec[0],
                tree_translate_record_new_const(offset)));
            node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("OR", 3, a, a, a));
            break;
        case PTR_OP:
        {
            NODE_TYPE *p;
            tree_translate_fetch_identifier(node->nxt[0]);
            tree_translate_fix_array(node->nxt[0]);
            node->trans = tree_translate_merge(2, node->trans, node->nxt[0]->trans);
            node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("ADD", 3,
                tree_translate_record_new_address(4),
                node->trans->instr[node->trans->num-1].rec[0],
                tree_translate_record_new_const(tree_translate_cal_record_offset(node->nxt[0]->selftype->record.pointer, node->nxt[1]->selftype->id, &p))));
            node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("STAR", 2,
                tree_translate_record_new_symbol(p->size),
                node->trans->instr[node->trans->num-1].rec[0]));
            break;
        }
        case '.':
        {
            NODE_TYPE *p;
            tree_translate_fetch_identifier(node->nxt[0]);
            tree_translate_fix_array(node->nxt[0]);
            node->trans = tree_translate_merge(2, node->trans, node->nxt[0]->trans);
            node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("ADS", 2,
                tree_translate_record_new_address(4),
                node->trans->instr[node->trans->num-1].rec[0]));
            node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("ADD", 3,
                tree_translate_record_new_address(4),
                node->trans->instr[node->trans->num-1].rec[0],
                tree_translate_record_new_const(tree_translate_cal_record_offset(node->nxt[0]->selftype, node->nxt[1]->selftype->id, &p))));
            node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("STAR", 2,
                tree_translate_record_new_symbol(p->size),
                node->trans->instr[node->trans->num-1].rec[0]));
            break;
        }
        case '(':
        {
            node->trans = tree_translate_merge(2, node->trans, node->nxt[0]->trans);
            if(node->nxt[1]->nxtnum == 1) //有参数
            {
                int *p;
                p = (int *)malloc((1+node->nxt[1]->nxt[0]->nxtnum) * sizeof(int));
                for(i=node->nxt[1]->nxt[0]->nxtnum-1; i>=0; i--)
                {
                    NODE_TYPE *x, *y;
                    tree_translate_fetch_identifier(node->nxt[1]->nxt[0]->nxt[i]);
                    node->trans = tree_translate_merge(2, node->trans, node->nxt[1]->nxt[0]->nxt[i]->trans);
                    x = &((node->nxt[0]->selftype->record).function.argumenttype[i]);
                    y = node->nxt[1]->nxt[0]->nxt[i]->selftype;
                    if(x->type == CHAR && y->type != CHAR)
                    {
                        node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("TCHAR", 2,
                            tree_translate_record_new_address(1),
                            node->trans->instr[node->trans->num-1].rec[0]));
                    }
                    else if(x->type != CHAR && y->type == CHAR)
                    {
                        node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("TINT", 2,
                            tree_translate_record_new_address(4),
                            node->trans->instr[node->trans->num-1].rec[0]));
                    }
                    p[i] = node->trans->num-1;
                }
                for(i=0; i<node->nxt[1]->nxt[0]->nxtnum; i++)
                {
                    node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("AUG", 1,
                        node->trans->instr[p[i]].rec[0]));
                }
                free(p);
            }
            node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("CALL", 1, tree_translate_check_symbol(node->nxt[0]->selftype->id)));
            if((node->nxt[0]->selftype->record).function.returntype->type != VOID)
            {
                node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("POP", 1, tree_translate_record_new_address((node->nxt[0]->selftype->record).function.returntype->size)));
            }
            break;
        }
        case '[':
        {
            int p1, p2;
            int size;
            tree_translate_fetch_identifier(node->nxt[0]);
            node->trans = tree_translate_merge(2, node->trans, node->nxt[0]->trans);
            /*
            printf("POSTFIX_EXP %d\n", POSTFIX_EXP);
            printf("POINTER %d\n", POINTER);
            printf("ARRAY %d\n", ARRAY);
            printf("%d\n", node->nxt[0]->selftype->type);
            printf("%d\n", node->nxt[0]->type);
            */
            
            if(node->nxt[0]->selftype->type == POINTER ||
               (node->nxt[0]->type == EXPRESSION && node->nxt[0]->record.integer == POSTFIX_EXP && node->nxt[0]->nxt[1]->record.integer == '['))
            {
                ;//do nothing
            }
            else
            {
                node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("ADS", 2,
                    tree_translate_record_new_address(4),
                    node->trans->instr[node->trans->num-1].rec[0]));
            }
            p1 = node->trans->num-1;
            node->trans = tree_translate_merge(2, node->trans, node->nxt[1]->trans);
            p2 = node->trans->num-1;
            if(node->nxt[0]->selftype->type == ARRAY)
            {
                size = node_type_cal_size_r_v(node->nxt[0]->selftype->record.array.pointer);
            }
            else
            {
                size = node->nxt[0]->selftype->record.pointer->size;
            }
            node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MUL", 3,
                tree_translate_record_new_address(4),
                node->trans->instr[p2].rec[0],
                tree_translate_record_new_const(size)));
            node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("ADD", 3,
                tree_translate_record_new_address(4),
                node->trans->instr[node->trans->num-1].rec[0],
                node->trans->instr[p1].rec[0]));
            
            if((node->nxt[0]->selftype->type == POINTER /*&& node->nxt[0]->selftype->record.pointer->type != POINTER*/  && node->nxt[0]->selftype->record.pointer->type != ARRAY) ||
               (node->nxt[0]->selftype->type == ARRAY && node->nxt[0]->selftype->record.array.pointer->type != POINTER && node->nxt[0]->selftype->record.array.pointer->type != ARRAY))
            {
                node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("STAR", 2,
                    tree_translate_record_new_symbol(size),
                    node->trans->instr[node->trans->num-1].rec[0]));
            }
            break;
        }
    }
}

void tree_translate_expression_8(NODE_T *node) //第八类表达式，单目&,*
{
    tree_translate_fetch_identifier(node->nxt[0]);
    tree_translate_fix_array(node->nxt[0]);
    node->trans = tree_translate_merge(2, node->trans, node->nxt[0]->trans);
    if((node->record).integer == '*')
    {
        node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("STAR", 2,
            tree_translate_record_new_symbol(node->nxt[0]->selftype->record.pointer->size), node->trans->instr[node->trans->num-1].rec[0]));
    }
    else
    {
        node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("ADS", 2,
            tree_translate_record_new_address(4), node->trans->instr[node->trans->num-1].rec[0]));
    }
}

/*
注意sizeof(exp)中 exp对外无影响
*/
void tree_translate_expression_9(NODE_T *node) //第九类表达式，sizeof
{
//若考虑变长数组，则需要增加代码
//warning
    tree_translate_fetch_identifier(node->nxt[0]);
    node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MOVE", 2,
        tree_translate_record_new_address(4),
        tree_translate_record_new_const(node->nxt[0]->selftype->size)));
}

void tree_translate_begin_scope(NODE_T *node)
{
    ;
}

void tree_translate_end_scope(NODE_T *node)
{
    ;
}

/*
if(x) A else B

if(!x) goto l1;
A
goto l2;
l1: B
l2:

if(x) A

if(!x) goto l1;
A
l1:
*/
void tree_translate_statement_if(NODE_T *node)
{
    NODE_TRANS_RECORD l1;
    tree_translate_fetch_identifier(node->nxt[0]);
    tree_translate_fix_array(node->nxt[0]);
    node->trans = tree_translate_merge(2, node->trans, node->nxt[0]->trans);
    node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("LNOT", 2,
        tree_translate_record_new_address(4), node->trans->instr[node->trans->num-1].rec[0]));
    l1 = tree_translate_record_new_label();
    node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("IF", 2,
        node->trans->instr[node->trans->num-1].rec[0], l1));
    if(node->nxtnum == 3)
    {
        NODE_TRANS_RECORD l2;
        l2 = tree_translate_record_new_label();
        node->trans = tree_translate_merge(2, node->trans, node->nxt[1]->trans); //A
        node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("GOTO", 1, l2));
        node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("LABEL", 1, l1));
        node->trans = tree_translate_merge(2, node->trans, node->nxt[2]->trans); //B
        node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("LABEL", 1, l2));
    }
    else //==2
    {
        node->trans = tree_translate_merge(2, node->trans, node->nxt[1]->trans); //A
        node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("LABEL", 1, l1));
    }
}

NODE_TRANS_LOOP_LABEL *ttlooplabel = NULL;
int ttlooplabelnum = 0;
int ttlooplabelsize = 0;
void ttlooplabel_enlarge()
{
    NODE_TRANS_LOOP_LABEL *p;
    int i;
    if(ttlooplabel == NULL)
    {
        ttlooplabel = (NODE_TRANS_LOOP_LABEL *)malloc(sizeof(NODE_TRANS_LOOP_LABEL));
        ttlooplabelsize = 1;
    }
    while(ttlooplabelnum+1 >= ttlooplabelsize) ttlooplabelsize *= 2;
    p = (NODE_TRANS_LOOP_LABEL *)malloc(sizeof(NODE_TRANS_LOOP_LABEL)*ttlooplabelsize);
    for(i=0; i<ttlooplabelnum; i++) p[i] = ttlooplabel[i];
    free(ttlooplabel);
    ttlooplabel = p;
}

void tree_translate_statement_while(NODE_T *node, int flag) //flag=0 生成代码 flag=1生成标记
{
    if(flag == 1)
    {
        ttlooplabelnum ++;
        ttlooplabel_enlarge();
        ttlooplabel[ttlooplabelnum-1].lstart = tree_translate_record_new_label();
        ttlooplabel[ttlooplabelnum-1].lend = tree_translate_record_new_label();
        ttlooplabel[ttlooplabelnum-1].lcontinue = tree_translate_record_new_label();
    }
    else //flag == 0
    {
        node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("LABEL", 1, ttlooplabel[ttlooplabelnum-1].lcontinue));
        node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("LABEL", 1, ttlooplabel[ttlooplabelnum-1].lstart));
        tree_translate_fetch_identifier(node->nxt[0]);
        tree_translate_fix_array(node->nxt[0]);
        node->trans = tree_translate_merge(2, node->trans, node->nxt[0]->trans);
        node->trans = tree_translate_merge(2, node->trans,  tree_translate_makelist("LNOT", 2,
            tree_translate_record_new_address(4), node->trans->instr[node->trans->num-1].rec[0]));
        node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("IF", 2,
            node->trans->instr[node->trans->num-1].rec[0], ttlooplabel[ttlooplabelnum-1].lend));
        node->trans = tree_translate_merge(2, node->trans, node->nxt[1]->trans);
        node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("GOTO", 1, ttlooplabel[ttlooplabelnum-1].lstart));
        node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("LABEL", 1, ttlooplabel[ttlooplabelnum-1].lend));
        
        ttlooplabelnum --;
    }
}

void tree_translate_statement_for(NODE_T *node, int flag) //flag=0 生成代码 flag=1生成标记
{
    if(flag == 1)
    {
        ttlooplabelnum ++;
        ttlooplabel_enlarge();
        ttlooplabel[ttlooplabelnum-1].lstart = tree_translate_record_new_label();
        ttlooplabel[ttlooplabelnum-1].lend = tree_translate_record_new_label();
        ttlooplabel[ttlooplabelnum-1].lcontinue = tree_translate_record_new_label();
    }
    else
    {
        node->trans = tree_translate_merge(2, node->trans, node->nxt[0]->trans);
        node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("LABEL", 1, ttlooplabel[ttlooplabelnum-1].lstart));
        node->trans = tree_translate_merge(2, node->trans, node->nxt[1]->trans);
        node->trans = tree_translate_merge(2, node->trans,  tree_translate_makelist("LNOT", 2,
            tree_translate_record_new_address(4), node->trans->instr[node->trans->num-1].rec[0]));
        node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("IF", 2,
            node->trans->instr[node->trans->num-1].rec[0], ttlooplabel[ttlooplabelnum-1].lend));
        if(node->nxtnum == 4)
        {
            node->trans = tree_translate_merge(2, node->trans, node->nxt[3]->trans);
            node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("LABEL", 1, ttlooplabel[ttlooplabelnum-1].lcontinue));
            node->trans = tree_translate_merge(2, node->trans, node->nxt[2]->trans);
        }
        else //==3
        {
            node->trans = tree_translate_merge(2, node->trans, node->nxt[2]->trans);
            node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("LABEL", 1, ttlooplabel[ttlooplabelnum-1].lcontinue));
        }
        node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("GOTO", 1, ttlooplabel[ttlooplabelnum-1].lstart));
        node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("LABEL", 1, ttlooplabel[ttlooplabelnum-1].lend));
        
        ttlooplabelnum --;
    }
}

void tree_translate_statement_break(NODE_T *node)
{
    node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("GOTO", 1, ttlooplabel[ttlooplabelnum-1].lend));
}

void tree_translate_statement_continue(NODE_T *node)
{
    node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("GOTO", 1, ttlooplabel[ttlooplabelnum-1].lcontinue));
}

void tree_translate_statement_return(NODE_T *node)
{
    //对于结构体的返回
    //to be continued
    if(node->nxtnum == 1)
    {
        tree_translate_fetch_identifier(node->nxt[0]);
        tree_translate_fix_array(node->nxt[0]);
        node->trans = tree_translate_merge(2, node->trans, node->nxt[0]->trans);
        node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("PUSH", 1, node->trans->instr[node->trans->num-1].rec[0]));
    }
    node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("RET", 0));
}

//NODE_TYPE *ttdeclarationtype;
//char *ttdeclarationid;

void tree_translate_declaration_initializer(NODE_T *node, NODE_TYPE *type, int pos, NODE_TRANS_RECORD record)
{
    int i;
    switch(node->type)
    {
        case INITIALIZER:
            if(node->nxt[0]->type == EXPRESSIONS) //initializers
            {
                tree_translate_declaration_initializer(node->nxt[0], type, pos, record);
                node->trans = tree_translate_merge(2, node->trans, node->nxt[0]->trans);
            }
            else //assignment expression / identifier / const etc
            {
                tree_translate_declaration_initializer(node->nxt[0], type, pos, record);
                node->trans = tree_translate_merge(2, node->trans, node->nxt[0]->trans);
            }
            break;
        case EXPRESSIONS:
        {
            int offset = 0;
            for(i=0; i<node->nxtnum; i++)
            {
                if(type->type == ARRAY)
                {
                    tree_translate_declaration_initializer(node->nxt[i], type->record.array.pointer, pos+offset, record);
                    offset += type->record.array.pointer->size;
                }
                else if(type->type == STRUCT)
                {
                    tree_translate_declaration_initializer(node->nxt[i], type->record.record.field[i].type, pos+offset, record);
                    offset += type->record.record.field[i].type->size;
                }
                else if(type->type == UNION)
                {
                    tree_translate_declaration_initializer(node->nxt[i], type->record.record.field[i].type, pos, record);
                }
                node->trans = tree_translate_merge(2, node->trans, node->nxt[i]->trans);
            }
            break;
        }
        case EXPRESSION:
        case IDENTIFIER:
        case CONST:
        {
            int p;
            tree_translate_fetch_identifier(node);
            tree_translate_fix_array(node);
            p = node->trans->num - 1;
            node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("ADS", 2,
                tree_translate_record_new_address(4), record));
            node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("ADD", 3,
                tree_translate_record_new_address(4),
                node->trans->instr[node->trans->num-1].rec[0],
                tree_translate_record_new_const(pos)));
            node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("STAR", 2,
                tree_translate_record_new_symbol(type->size),
                node->trans->instr[node->trans->num-1].rec[0]));
            node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MOVE", 2,
                node->trans->instr[node->trans->num-1].rec[0],
                node->trans->instr[p].rec[0]));
            break;
        }
        default:
            //实际不可能发生
            break;
    }
}

NODE_T *ttdeclarationcurrent;
NODE_TYPE *ttdelarationtype;

void tree_translate_declaration(NODE_T *node, int flag)
{
    if(flag == 0) return; //flag == 0: 无工作
    if(flag == 1) //变量
    {
        NODE_ENV *env;
        env = &tscenv[tscenvnum-1];
        //(env->ventry[env->ventrynum-1]).rec = tree_translate_record_new_address((env->ventry[env->ventrynum-1]).type->size);
        
        if(env->ventry[env->ventrynum-1].type->type == ARRAY &&
           env->ventry[env->ventrynum-1].type->size == -1) //变长数组
        {
#ifdef USE_VLA
            //to be continued
            //node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("
#endif
        }
        else //The type of the entity to be initialized shall be an array of unknown size or an object type that is not a variable length array type.
        {
            //初始化处理
            if(ttdeclarationcurrent != NULL && ttdeclarationcurrent->nxtnum == 2) //有初始化
            {
                node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MOVE", 2,
                    (env->ventry[env->ventrynum-1]).rec,
                    tree_translate_record_new_const(0)));
                tree_translate_declaration_initializer(ttdeclarationcurrent->nxt[1], (env->ventry[env->ventrynum-1]).type, 0, (env->ventry[env->ventrynum-1]).rec);
                node->trans = tree_translate_merge(2, node->trans, ttdeclarationcurrent->nxt[1]->trans);
                if(tscenvnum == 1)
                {
                    ttglobaltrans = tree_translate_merge(2, ttglobaltrans, node->trans);
                    node->trans = tree_translate_new();
                }
            }
            else
            {
                //若是全局变量，则初始化为0
                if(tscenvnum == 1)
                {
                    ttglobaltrans = tree_translate_merge(2, ttglobaltrans, tree_translate_makelist("MOVE", 2,
                        (env->ventry[env->ventrynum-1]).rec,
                        tree_translate_record_new_const(0)));
                }
            }
        }
        return;
    }
    if(flag == 2) //函数
    {
        if(tscaddfunentryflag)
        {
            NODE_ENV *env;
            env = &tscenv[tscenvnum-1];
            (env->ventry[env->ventrynum-1]).rec = tree_translate_record_new_label();
        }
        return;
    }
    if(flag == 3) //变长数组
    {
        node->trans = tree_translate_merge(2, node->trans, (NODE_TRANS*)(ttdelarationtype->info));
        return;
    }
    if(flag == 4)
    {
        NODE_ENV *env;
        env = &tscenv[tscenvnum-1];
        (env->ventry[env->ventrynum-1]).rec = tree_translate_record_new_address((env->ventry[env->ventrynum-1]).type->size);
        return;
    }
}

void tree_translate_function_definition(NODE_T *node, int flag)
{
    if(flag == 0) return; //flag == 0: 无工作
    if(flag == 1) //声明部分
    {
        NODE_ENV *env;
        int i, j;
        if(tscaddfunentryflag)
        {
            env = &tscenv[tscenvnum-1];
            (env->ventry[env->ventrynum-1]).rec = tree_translate_record_new_label();
            node->trans = tree_translate_merge(2, tree_translate_makelist("FUNC", 1, (env->ventry[env->ventrynum-1]).rec), node->trans);
            
            //记录入口main的位置
            if(strcmp("main", (env->ventry[env->ventrynum-1]).symbol) == 0)
            {
                ttmainlabel = (env->ventry[env->ventrynum-1]).rec;
            }
        }
        else
        {
            i = tscenvnum - 1;
            for(j=0; j<tscenv[i].ventrynum; j++)
            {
                if(strcmp(tscenv[i].ventry[j].symbol, tscaddfunentryid) == 0)
                {
                    break;
                }
            }
            tscenv[i].ventry[j].def = 0;
            node->trans = tree_translate_merge(2, tree_translate_makelist("LABEL", 1, tscenv[i].ventry[j].rec), node->trans);
        }
        return;
    }
    if(flag == 2) //函数体
    {
        node->trans = tree_translate_merge(2, node->trans, node->nxt[node->nxtnum-1]->trans);
        //printf("%d %d ", node->trans->num, node->trans->size);
        node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("END", 0));
        //printf("%d %d ", node->trans->num, node->trans->size);
        //if(node->trans->num == 2) printf("%s ", node->trans->instr[0].op);
    }
    if(flag == 3) //参数
    {
        int i, j;
        i = tscenvnum - 1;
        for(j=0; j<tscenv[i].ventrynum; j++)
        {
            NODE_TYPE *t;
            t = tscenv[i].ventry[j].type;
            while(t!=NULL && t->type == ARRAY)
            {
                t->size = 4; //形参中的"type name[size]"
                t = t->record.array.pointer;
            }
            tscenv[i].ventry[j].rec = tree_translate_record_new_address(tscenv[i].ventry[j].type->size);
            node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("GAUG", 1, tscenv[i].ventry[j].rec));
        }
    }
}

void tree_translate_plain_declaration(NODE_T *node, int flag)
{
    if(flag == 0) return; //flag == 0: 无工作
    if(flag == 1)
    {
        if(tscaddfunentryflag)
        {
            NODE_ENV *env;
            env = &tscenv[tscenvnum-1];
            (env->ventry[env->ventrynum-1]).rec = tree_translate_record_new_label();
        }
        return;
    }
}

void tree_translate_declarator(NODE_T *node, int flag)
{
    switch((node->record).integer)
    {
        case '(': break;
        case '[':
#ifdef USE_VLA
            if(flag == 1 || flag == 0) //flag==1:变长数组  flag==0:普通数组
            {
                NODE_TRANS_VLA *vla;
                int p;
                vla = (NODE_TRANS_VLA *)malloc(sizeof(NODE_TRANS_VLA));
                vla->pos = tree_translate_record_new_address(4);
                vla->trans = node->nxt[1]->trans;
                p = vla->trans->num - 1;
                node->nxt[1]->trans = tree_translate_new();
                vla->trans = tree_translate_merge(2, vla->trans, tree_translate_makelist("OR", 3,
                    ((NODE_TRANS_VLA *)(node->nxt[0]->selftype->info))->pos,
                    ((NODE_TRANS_VLA *)(node->nxt[0]->selftype->info))->pos,
                    ((NODE_TRANS_VLA *)(node->nxt[0]->selftype->info))->pos);
                vla->trans = tree_translate_merge(2, vla->trans, tree_translate_makelist("MUL", 2, 
                    vla->pos, vla->trans->instr[p].rec[0], vla->trans->instr[vla->trans->num-1].rec[0]));
                node->selftype->info = (int)vla;
            }
#endif
            break;
        case 0: break;
        case '*': break;
    }
}

void tree_translate_main(NODE_T *node, int flag)
{
    int i;
    switch(node->type)
    {
        case PROGRAM: //ok
            break;
        case DECLARATION:
            tree_translate_declaration(node, flag);
            break;
        case FUNCTION_DEFINITION:
            tree_translate_function_definition(node, flag);
            break;
        case PARAMETER:
            break;
        case DECLARATORS:
            break;
        case INIT_DECLARATORS: //ok
            break;
        case INIT_DECLARATOR: //ok
            break;
        case INITIALIZER: //ok
            break;
        case EXPRESSIONS: //ok
            break;
        case EXPRESSION:
            if(tree_translate_expression_precal(node)) //如果表达式已经可以预先计算
            {
                char str[20];
                node->type = CONST;
                sprintf(str, "%d", node->trans->instr[node->trans->num-1].rec[1].record);
                (node->record).string = strdup(str);
                break;
            }
            switch((node->record).integer)
            {
                case SHL_ASSIGN:
                    tree_translate_expression_2(node);
                    break;
                case SHR_ASSIGN:
                    tree_translate_expression_2(node);
                    break;
                case MUL_ASSIGN:
                    tree_translate_expression_2(node);
                    break;
                case DIV_ASSIGN:
                    tree_translate_expression_2(node);
                    break;
                case MOD_ASSIGN:
                    tree_translate_expression_2(node);
                    break;
                case ADD_ASSIGN:
                    tree_translate_expression_2(node);
                    break;
                case SUB_ASSIGN:
                    tree_translate_expression_2(node);
                    break;
                case AND_ASSIGN:
                    tree_translate_expression_2(node);
                    break;
                case XOR_ASSIGN:
                    tree_translate_expression_2(node);
                    break;
                case OR_ASSIGN:
                    tree_translate_expression_2(node);
                    break;
                case OR_OP:
                    tree_translate_expression_3(node, 0);
                    break;
                case AND_OP:
                    tree_translate_expression_3(node, 1);
                    break;
                case EQ_OP:
                    tree_translate_expression_1(node);
                    break;
                case NE_OP:
                    tree_translate_expression_1(node);
                    break;
                case LE_OP:
                    tree_translate_expression_1(node);
                    break;
                case GE_OP:
                    tree_translate_expression_1(node);
                    break;
                case '>':
                    tree_translate_expression_1(node);
                    break;
                case '<':
                    tree_translate_expression_1(node);
                    break;
                case SHL_OP:
                    tree_translate_expression_1(node);
                    break;
                case SHR_OP:
                    tree_translate_expression_1(node);
                    break;
                case SIZEOF_OP:
                    tree_translate_expression_9(node);
                    break;
                case INC_OP: //不可能遇到
                    break;
                case DEC_OP: //不可能遇到
                    break;
                case PTR_OP: //不可能遇到
                    break;
                case INC_OP_U:
                    tree_translate_expression_5(node);
                    break;
                case DEC_OP_U:
                    tree_translate_expression_5(node);
                    break;
                case CAST_EXP:
                    tree_translate_expression_6(node);
                    break;
                case BRACKET_EXP:
                    tree_translate_fetch_identifier(node->nxt[0]);
                    tree_translate_fix_array(node->nxt[0]);
                    node->trans = tree_translate_merge(2, node->trans, node->nxt[0]->trans);
                    break;
                case POSTFIX_EXP:
                    tree_translate_expression_7(node);
                    break;
                case ',':
                    tree_translate_fetch_identifier(node->nxt[0]);
                    tree_translate_fetch_identifier(node->nxt[1]);
                    tree_translate_fix_array(node->nxt[0]);
                    tree_translate_fix_array(node->nxt[1]);
                    node->trans = tree_translate_merge(3, node->trans, node->nxt[0]->trans, node->nxt[1]->trans);
                    break;
                case '=':
                    tree_translate_expression_2(node);
                    break;
                case '+':
                    tree_translate_expression_1(node);
                    break;
                case '-':
                    tree_translate_expression_1(node);
                    break;
                case '~':
                    tree_translate_expression_4(node);
                    break;
                case '!':
                    tree_translate_expression_4(node);
                    break;
                case '*':
                    if(node->nxtnum == 1)
                    {
                        tree_translate_expression_8(node);
                    }
                    else //==2
                    {
                        tree_translate_expression_1(node);
                    }
                    break;
                case '&':
                    if(node->nxtnum == 1)
                    {
                        tree_translate_expression_8(node);
                    }
                    else //==2
                    {
                        tree_translate_expression_1(node);
                    }
                    break;
                case '/':
                    tree_translate_expression_1(node);
                    break;
                case '%':
                    tree_translate_expression_1(node);
                    break;
                case '^':
                    tree_translate_expression_1(node);
                    break;
                case '|':
                    tree_translate_expression_1(node);
                    break;
            }
            break;
        case TYPE_SPECIFIER:
            switch((node->record).integer)
            {
                case VOID_T:
                    break;
                case CHAR_T:
                    break;
                case INT_T:
                    break;
                case TYPEDEF_NAME:
                    break;
                case STRUCT_T:
                    break;
                case UNION_T:
                    break;
            }
            break;
        case POSTFIX:
            switch((node->record).integer)
            {
                case INC_OP:
                    break;
                case DEC_OP:
                    break;
                case PTR_OP:
                    break;
                case '(':
                    break;
                case '[':
                    tree_translate_fetch_identifier(node->nxt[0]);
                    node->trans = tree_translate_merge(2, node->trans, node->nxt[0]->trans);
                    break;
                case '.':
                    break;
            }
            break;
        case PLAIN_DECLARATIONS:
            break;
        case PLAIN_DECLARATION:
            tree_translate_plain_declaration(node, flag);
            break;
        case ARGUMENTS:
            break;
        case TYPE_NAME:
            switch((node->record).integer)
            {
                case 0:
                    break;
                case '*':
                    break;
            }
            break;
        case STATEMENTS:
            break;
        case STATEMENT:
            switch((node->record).integer)
            {
                case EXP_STMT:
                    if(node->nxtnum == 1)
                    {
                        tree_translate_fetch_identifier(node->nxt[0]);
                        tree_translate_fix_array(node->nxt[0]);
                        node->trans = tree_translate_merge(2, node->trans, node->nxt[0]->trans);
                    }
                    break;
                case '{':
                    tree_translate_begin_scope(node);
                    break;
                case '}':
                    tree_translate_end_scope(node);
                    break;
                case IF_C:
                    tree_translate_statement_if(node);
                    break;
                case WHILE_C:
                    tree_translate_statement_while(node, flag);
                    break;
                case FOR_C:
                    tree_translate_statement_for(node, flag);
                    break;
                case BREAK_C:
                    tree_translate_statement_break(node);
                    break;
                case RETURN_C:
                    tree_translate_statement_return(node);
                    break;
                case CONTINUE_C:
                    tree_translate_statement_continue(node);
                    break;
            }
            break;
        case DECLARATOR:
            switch((node->record).integer)
            {
                case '(':
                    break;
                case '[':
                    break;
                case 0:
                    break;
                case '*':
                    break;
            }
            break;
        case TYPEDEFNAME:
            break;
        case IDENTIFIER:
            break;
        case CONST:
            switch((node->record).string[0])
            {
                case '\'':
                {
                    int x = 0;
                    for(i=1; (node->record).string[i] != '\''; i++)
                    {
                        x <<= 8;
                        if((node->record).string[i] == '\\')
                        {
                            i ++;
                            if((node->record).string[i] == 'n')
                            {
                                x |= '\n';
                            }
                            else
                            {
                                x |= (node->record).string[i];
                            }
                        }
                        else
                        {
                            x |= (node->record).string[i];
                        }
                    }
                    node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MOVE", 2, tree_translate_record_new_address(4), tree_translate_record_new_const(x)));
                    break;
                }
                case '\"':
                {
                    int size;
                    size = -1;
                    for(i=0; node->record.string[i]; i++)
                    {
                        if(node->record.string[i] == '\\') i++;
                        size ++;
                    }
                    node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MOVE", 2,
                        tree_translate_record_new_address(size),
                        tree_translate_record_new_string((int)(strdup(node->record.string)))));
                    node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("ADS", 2,
                        tree_translate_record_new_address(4),
                        node->trans->instr[node->trans->num-1].rec[0]));
                    break;
                }
                default: //integer
                    if((node->record).string[0] == '0' && ((node->record).string[1] == 'X' || (node->record).string[1] == 'x'))
                    {
                        int x;
                        sscanf((node->record).string, "0%*[Xx]%x", &x);
                        node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MOVE", 2, tree_translate_record_new_address(4), tree_translate_record_new_const(x)));
                    }
                    else if((node->record).string[0] == '0' && (node->record).string[1] != 0)
                    {
                        int x;
                        sscanf((node->record).string, "0%o", &x);
                        node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MOVE", 2, tree_translate_record_new_address(4), tree_translate_record_new_const(x)));
                    }
                    else
                    {
                        int x;
                        sscanf((node->record).string, "%d", &x);
                        node->trans = tree_translate_merge(2, node->trans, tree_translate_makelist("MOVE", 2, tree_translate_record_new_address(4), tree_translate_record_new_const(x)));
                    }
                    break;
            }
            break;
        case ELLIPSIS:
            break;
        case TYPEDEF:
            break;
    }
}

/*
将以下情形
STAR  sA    $B
ADS   $C    sA
变为
STAR  sA    $B
MOVE  $C    $B
*/
void tree_translate_opt_0()
{
    int i;
    for(i=1; i<treeroot->trans->num; i++)
    {
        if(strcmp(treeroot->trans->instr[i].op, "ADS") != 0) continue;
        if(strcmp(treeroot->trans->instr[i-1].op, "STAR") != 0) continue;
        if(memcmp(&(treeroot->trans->instr[i].rec[1]), &(treeroot->trans->instr[i-1].rec[0]), sizeof(NODE_TRANS_RECORD)) == 0)
        {
            treeroot->trans->instr[i].op = "MOVE";
            treeroot->trans->instr[i].rec[1] = treeroot->trans->instr[i-1].rec[1];
        }
    }
}

void tree_translate_opt_1()
{
    int i, j;
    for(i=0; i<treeroot->trans->num; i++)
    {
        if((strcmp(treeroot->trans->instr[i].op, "OR") == 0 || strcmp(treeroot->trans->instr[i].op, "AND") == 0)&&
           memcmp(&(treeroot->trans->instr[i].rec[0]), &(treeroot->trans->instr[i].rec[1]), sizeof(NODE_TRANS_RECORD)) == 0 &&
           memcmp(&(treeroot->trans->instr[i].rec[0]), &(treeroot->trans->instr[i].rec[2]), sizeof(NODE_TRANS_RECORD)) == 0)
        {
            treeroot->trans->instr[i].op = "";
            continue;
        }
        if((strcmp(treeroot->trans->instr[i].op, "ADD") == 0 || strcmp(treeroot->trans->instr[i].op, "SUB") == 0)&&
           memcmp(&(treeroot->trans->instr[i].rec[0]), &(treeroot->trans->instr[i].rec[1]), sizeof(NODE_TRANS_RECORD)) == 0 &&
           treeroot->trans->instr[i].rec[2].type == CONST && treeroot->trans->instr[i].rec[2].record == 0)
        {
            treeroot->trans->instr[i].op = "";
            continue;
        }
        if(strcmp(treeroot->trans->instr[i].op, "ADD") == 0 &&
           memcmp(&(treeroot->trans->instr[i].rec[0]), &(treeroot->trans->instr[i].rec[2]), sizeof(NODE_TRANS_RECORD)) == 0 &&
           treeroot->trans->instr[i].rec[1].type == CONST && treeroot->trans->instr[i].rec[1].record == 0)
        {
            treeroot->trans->instr[i].op = "";
            continue;
        }
        if((strcmp(treeroot->trans->instr[i].op, "MUL") == 0 || strcmp(treeroot->trans->instr[i].op, "DIV") == 0)&&
           memcmp(&(treeroot->trans->instr[i].rec[0]), &(treeroot->trans->instr[i].rec[1]), sizeof(NODE_TRANS_RECORD)) == 0 &&
           treeroot->trans->instr[i].rec[2].type == CONST && treeroot->trans->instr[i].rec[2].record == 1)
        {
            treeroot->trans->instr[i].op = "";
            continue;
        }
        if(strcmp(treeroot->trans->instr[i].op, "MUL") == 0 &&
           memcmp(&(treeroot->trans->instr[i].rec[0]), &(treeroot->trans->instr[i].rec[2]), sizeof(NODE_TRANS_RECORD)) == 0 &&
           treeroot->trans->instr[i].rec[1].type == CONST && treeroot->trans->instr[i].rec[1].record == 1)
        {
            treeroot->trans->instr[i].op = "";
            continue;
        }
        if(strcmp(treeroot->trans->instr[i].op, "MOVE") == 0 &&
           memcmp(&(treeroot->trans->instr[i].rec[0]), &(treeroot->trans->instr[i].rec[1]), sizeof(NODE_TRANS_RECORD)) == 0)
        {
            treeroot->trans->instr[i].op = "";
            continue;
        }
    }
    for(i=0,j=0; j<treeroot->trans->num; j++)
    {
        if(treeroot->trans->instr[j].op[0] != 0)
        {
            treeroot->trans->instr[i] = treeroot->trans->instr[j];
            i ++;
        }
    }
    treeroot->trans->num = i;
}

FILE *fouttrans;

void tree_translate_output()
{
    int i, j, k;
    for(i=0; i<treeroot->trans->num; i++)
    {
        fprintf(fouttrans, "%6s", treeroot->trans->instr[i].op);
        for(j=0; j<37; j++)
            if(strcmp(treeroot->trans->instr[i].op, ttinstrformat[j].op) == 0) break;
        k = ttinstrformat[j].num;
        for(j=0; j<k; j++)
        {
            switch(treeroot->trans->instr[i].rec[j].type)
            {
                case TRANS_CONST:
#ifdef EASY_INTERPRETER
                    if(treeroot->trans->instr[i].rec[j].flag == 0)
                        fprintf(fouttrans, "\td%d", treeroot->trans->instr[i].rec[j].record);
                    else
                        fprintf(fouttrans, "\t.%s", (char *)(treeroot->trans->instr[i].rec[j].record));
#else
                    if(treeroot->trans->instr[i].rec[j].flag == 0)
                        fprintf(fouttrans, "\t%d", treeroot->trans->instr[i].rec[j].record);
                    else
                        fprintf(fouttrans, "\t%s", (char *)(treeroot->trans->instr[i].rec[j].record));
#endif                        
                    break;
                case TRANS_ADDRESS:
#ifndef EASY_INTERPRETER
                    if(treeroot->trans->instr[i].rec[j].flag == 4)
                        fprintf(fouttrans, "\t$%d", treeroot->trans->instr[i].rec[j].record);
                    else
#endif                    
                        fprintf(fouttrans, "\t$(%d)%d", treeroot->trans->instr[i].rec[j].flag, treeroot->trans->instr[i].rec[j].record);
                    break;
                case TRANS_LABEL:
                    fprintf(fouttrans, "\tl%d", treeroot->trans->instr[i].rec[j].record);
                    break;
                case TRANS_SYMBOL:
#ifndef EASY_INTERPRETER                
                    if(treeroot->trans->instr[i].rec[j].flag == 4)
                        fprintf(fouttrans, "\ts%d", treeroot->trans->instr[i].rec[j].record);
                    else
#endif                    
                        fprintf(fouttrans, "\ts(%d)%d", treeroot->trans->instr[i].rec[j].flag, treeroot->trans->instr[i].rec[j].record);
            }
        }
        fprintf(fouttrans, "\n");
    }
}

void tree_translate_init()
{
    NODE_ENV *env;
    int i;
    env = &tscenv[tscenvnum-1];
    for(i=0; i<env->ventrynum; i++)
    {
        (env->ventry[i]).rec = tree_translate_record_new_label();
    }
    ttglobaltrans = tree_translate_new();
}

void tree_translate_end()
{
    treeroot->trans = tree_translate_merge(2, tree_translate_makelist("CALL", 1, ttmainlabel), treeroot->trans);
    treeroot->trans = tree_translate_merge(2, ttglobaltrans, treeroot->trans);
    
    tree_translate_opt_0(); //必须做，不做会影响到最终生成机器代码
    tree_translate_opt_1();
#ifdef OUTPUT_TRANS
    if(!reporterrorflag)
        tree_translate_output();
#endif
}
#endif
