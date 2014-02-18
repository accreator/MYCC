#ifndef FILE_TRANSLATE_H
#define FILE_TRANSLATE_H

typedef struct NODE_TRANS_RECORD
{
    enum
    {
        TRANS_CONST,
        TRANS_ADDRESS,
        TRANS_LABEL,
        TRANS_SYMBOL
    } type;
    int record;
    int flag;
} NODE_TRANS_RECORD;

typedef struct
{
    char *op;
    NODE_TRANS_RECORD rec[3];
} NODE_TRANS_INSTR;

typedef struct NODE_TRANS
{
    int num;
    int size;
    NODE_TRANS_INSTR *instr;
} NODE_TRANS;

typedef struct
{
    char *op;
    int num;
} NODE_TRANS_INSTR_FORMAT;

typedef struct
{
    NODE_TRANS_RECORD lstart;
    NODE_TRANS_RECORD lend;
    NODE_TRANS_RECORD lcontinue;
} NODE_TRANS_LOOP_LABEL;

typedef struct
{
    NODE_TRANS *trans;
    NODE_TRANS_RECORD pos;
} NODE_TRANS_VLA;

NODE_TRANS_VLA * tree_translate_new_vla(int len);
NODE_TRANS_RECORD tree_translate_record_new_const(int rec);
NODE_TRANS_RECORD tree_translate_record_new_address(int len);
NODE_TRANS_RECORD tree_translate_record_new_label();
NODE_TRANS * tree_translate_new();
void tree_translate_enlarge(NODE_TRANS *node);
NODE_TRANS * tree_translate_makelist(char *str, int num, ...);
NODE_TRANS * tree_translate_merge(int num, ...);
void tree_translate_main(NODE_T *node, int flag);
void tree_translate_output();
void tree_translate_init();
void tree_translate_end();

//extern NODE_TYPE *ttdeclarationtype;
//extern char *ttdeclarationid;

extern NODE_TRANS_INSTR_FORMAT ttinstrformat[];

extern FILE *fouttrans;

extern NODE_T *ttdeclarationcurrent;
extern struct NODE_TYPE *ttdelarationtype;
#endif
