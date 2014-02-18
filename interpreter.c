/*
暂时不考虑返回值是结构体的情形 to be continued
暂时不考虑结构体=结构体的情形 to be continued
*/
#define FINAL_TEST

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#define M 200000
#define N 100
#define PAR_NUM 32
#define TTIFNUM 37
typedef struct NODE_TRANS_RECORD
{
    enum
    {
        TRANS_CONST,
        TRANS_ADDRESS,
        TRANS_LABEL,
        TRANS_SYMBOL,
        TRANS_UNDEFINED
    } type;
    int record;
    int flag;
} NODE_TRANS_RECORD;
typedef struct
{
    char *op;
    NODE_TRANS_RECORD rec[3];
} NODE_TRANS_INSTR;
typedef struct
{
    char *op;
    int num;
} NODE_TRANS_INSTR_FORMAT;
NODE_TRANS_INSTR_FORMAT ttinstrformat[] =
{
    {"LABEL", 1}, {"GOTO", 1}, {"SHL", 3}, {"SHR", 3}, {"MUL", 3},
    {"DIV", 3}, {"MOD", 3}, {"ADD", 3}, {"SUB", 3}, {"AND", 3},
    {"XOR", 3}, {"OR", 3}, {"LAND", 3}, {"LOR", 3}, {"EQ", 3},
    {"NE", 3}, {"LE", 3}, {"GE", 3}, {"GT", 3}, {"LT", 3},
    {"NOT", 2}, {"LNOT", 2}, {"IF", 2}, {"CALL", 1}, {"RET", 0},
    {"PUSH", 1}, {"POP", 1}, {"TINT", 2}, {"TCHAR", 2}, {"MOVE", 2},
    {"ADS", 2}, {"STAR", 2}, {"NEW", 2}, {"FUNC", 1}, {"END", 0},
    {"AUG", 1}, {"GAUG", 1},
};

NODE_TRANS_INSTR code[M];
char tmpstr[100000];
int codenum = 0;
int globalvar[M];
int globalsymbol[M];
int globalstr[M];
int globallen = 0; //注意这里的len是变量+符号+字符串的个数
int staticvar[M];
int staticsymbol[M];
int staticlen = PAR_NUM*4; //假设至多有36个参数(每个参数大小为4字节) to be continued

int functionvar[M];
int functionsymbol[M];
int functionlen; //注意这里的len是空间大小(字节数)

void init(int argc, char *argv[])
{
    FILE *in;
    int i, j, k;
    memset(globalvar, -1, sizeof(globalvar));
    memset(globalsymbol, -1, sizeof(globalsymbol));
    memset(staticvar, -1, sizeof(staticvar));
    memset(staticsymbol, -1, sizeof(staticsymbol));
#ifdef FINAL_TEST
    in = stdin;
#else
    in = fopen(argv[1], "r");
#endif
    while(fscanf(in, "%s", tmpstr) != EOF)
    {
        code[codenum].op = strdup(tmpstr);
        for(i=0; i<TTIFNUM; i++)
            if(strcmp(code[codenum].op, ttinstrformat[i].op) == 0) break;
        k = ttinstrformat[i].num;
        for(i=0; i<k; i++)
        {
            char t;
            fscanf(in, " %c", &t);
            switch(t)
            {
                case 'd':
                    code[codenum].rec[i].type = TRANS_CONST;
                    code[codenum].rec[i].flag = 0;
                    fscanf(in, "%d", &code[codenum].rec[i].record);
                    break;
                case '.':
                    code[codenum].rec[i].type = TRANS_CONST;
                    code[codenum].rec[i].flag = 1;
                    //fscanf(in, "%s", tmpstr);
                    j = 0;
                    fscanf(in, "%c", &tmpstr[j]);
                    do
                    {
                        j ++;
                        fscanf(in, "%c", &tmpstr[j]);
                    } while(!(tmpstr[j] == '\"' && tmpstr[j-1] != '\\'));
                    tmpstr[j+1] = 0;
                    code[codenum].rec[i].record = (int)strdup(tmpstr);
                    break;
                case '$':
                    code[codenum].rec[i].type = TRANS_ADDRESS;
                    fscanf(in, "(%d)%d", &code[codenum].rec[i].flag, &code[codenum].rec[i].record);
                    break;
                case 's':
                    code[codenum].rec[i].type = TRANS_SYMBOL;
                    fscanf(in, "(%d)%d", &code[codenum].rec[i].flag, &code[codenum].rec[i].record);
                    break;
                case 'l':
                    code[codenum].rec[i].type = TRANS_LABEL;
                    fscanf(in, "%d", &code[codenum].rec[i].record);
                    break;
            }
        }
        for(i=k; i<3; i++)
        {
            code[codenum].rec[i].type = TRANS_UNDEFINED;
        }
        codenum ++;
    }
    fclose(in);
}

typedef struct MIPS_CODE
{
    char *label;
    char *op;
    int num;
    struct MIPS_CODE_REC
    {
        enum {MIPS_REG, MIPS_LABEL, MIPS_NUMBER, MIPS_STRING} type;
        union
        {
            int integer;
            char *string;
        } record;
    } rec[3];
} MIPS_CODE;
MIPS_CODE mipscode[M];
int mipscodenum = 0;
int mipscodemnum = M-1;
int mipscodeaddflag = 0;
int maininsertpoint = 0;
MIPS_CODE mipscodehead[M/2];
int mipscodeheadnum = 0;

int newlabelnum = 0;

void mips_code_add(char *label, char *op, int num, ...)
{
    va_list ap;
    int i;
    va_start(ap, num);
    if(mipscodeaddflag == 0)
    {
        mipscode[mipscodenum].label = label;
        mipscode[mipscodenum].op = op;
        mipscode[mipscodenum].num = num;
        for(i=0; i<num; i++)
        {
            mipscode[mipscodenum].rec[i] = va_arg(ap, struct MIPS_CODE_REC);
        }
        mipscodenum ++;
    }
    else if(mipscodeaddflag == 1)
    {
        mipscode[mipscodemnum].label = label;
        mipscode[mipscodemnum].op = op;
        mipscode[mipscodemnum].num = num;
        for(i=0; i<num; i++)
        {
            mipscode[mipscodemnum].rec[i] = va_arg(ap, struct MIPS_CODE_REC);
        }
        mipscodemnum --;
    }
    else
    {
        mipscodehead[mipscodeheadnum].label = label;
        mipscodehead[mipscodeheadnum].op = op;
        mipscodehead[mipscodeheadnum].num = num;
        for(i=0; i<num; i++)
        {
            mipscodehead[mipscodeheadnum].rec[i] = va_arg(ap, struct MIPS_CODE_REC);
        }
        mipscodeheadnum ++;
    }
}

void interpret_gp_convert(int p)
{
    struct MIPS_CODE_REC rec[3];
    if(p < 32768)
    {
        sprintf(tmpstr, "%d($gp)", p);
        return;
    }
    else
    {
        rec[0].type = MIPS_REG;
        rec[0].record.string = "$t1";
        rec[1].type = MIPS_NUMBER;
        rec[1].record.integer = p>>16;
        /*
        读立即数方法:
        lui  $r1, 0x0123
        ori  $r1, $r1, 0xabcd
        */
        mips_code_add("", "lui", 2, rec[0], rec[1]);
        rec[1].type = MIPS_NUMBER;
        rec[1].record.integer = p & ((1<<16)-1);
        mips_code_add("", "ori", 3, rec[0], rec[0], rec[1]);
        
        rec[1].type = MIPS_REG;
        rec[1].record.string = "$gp";
        mips_code_add("", "addu", 3, rec[0], rec[0], rec[1]);
        sprintf(tmpstr, "0($t1)");
        return;
    }
}

void interpret_sp_convert(int p)
{
    struct MIPS_CODE_REC rec[3];
    if(p < 32768)
    {
        sprintf(tmpstr, "%d($sp)", p);
        return;
    }
    else
    {
        rec[0].type = MIPS_REG;
        rec[0].record.string = "$t1";
        rec[1].type = MIPS_NUMBER;
        rec[1].record.integer = p>>16;

        mips_code_add("", "lui", 2, rec[0], rec[1]);
        rec[1].type = MIPS_NUMBER;
        rec[1].record.integer = p & ((1<<16)-1);
        mips_code_add("", "ori", 3, rec[0], rec[0], rec[1]);
        
        rec[1].type = MIPS_REG;
        rec[1].record.string = "$sp";
        mips_code_add("", "addu", 3, rec[0], rec[0], rec[1]);
        sprintf(tmpstr, "0($t1)");
        return;
    }
}

void interpret_load_ads(NODE_TRANS_RECORD rec, char *pos) //将rec存入寄存器pos
{
    struct MIPS_CODE_REC trec[3];
    if(rec.type == TRANS_SYMBOL)
    {
        if(globalsymbol[rec.record] == -1) //可修改符号
        {
            if(staticsymbol[rec.record] == -1) //局部符号
            {
                trec[0].type = MIPS_REG;
                trec[0].record.string = pos;
                interpret_sp_convert(functionsymbol[rec.record]);
                trec[1].type = MIPS_REG;
                trec[1].record.string = strdup(tmpstr);
                mips_code_add("", "lw", 2, trec[0], trec[1]);
            }
            else
            {
                trec[0].type = MIPS_REG;
                trec[0].record.string = pos;
                interpret_gp_convert(staticsymbol[rec.record]);
                trec[1].type = MIPS_REG;
                trec[1].record.string = strdup(tmpstr);
                mips_code_add("", "lw", 2, trec[0], trec[1]);
            }
        }
        else
        {
            trec[0].type = MIPS_REG;
            trec[0].record.string = pos;
            sprintf(tmpstr, "g%d", globalsymbol[rec.record]);
            trec[1].type = MIPS_REG;
            trec[1].record.string = strdup(tmpstr);
            mips_code_add("", "lw", 2, trec[0], trec[1]);
        }
    }
    else if(rec.type == TRANS_ADDRESS)
    {
        if(globalvar[rec.record] == -1) //可修改变量
        {
            if(staticvar[rec.record] == -1) //局部变量
            {
                trec[0].type = MIPS_REG;
                trec[0].record.string = pos;
                interpret_sp_convert(functionvar[rec.record]);
                trec[1].type = MIPS_REG;
                trec[1].record.string = strdup(tmpstr);
                mips_code_add("", "la", 2, trec[0], trec[1]);
            }
            else
            {
                trec[0].type = MIPS_REG;
                trec[0].record.string = pos;
                interpret_gp_convert(staticvar[rec.record]);
                trec[1].type = MIPS_REG;
                trec[1].record.string = strdup(tmpstr);
                mips_code_add("", "la", 2, trec[0], trec[1]);
            }
        }
        else
        {
            trec[0].type = MIPS_REG;
            trec[0].record.string = pos;
            sprintf(tmpstr, "g%d", globalvar[rec.record]);
            trec[1].type = MIPS_REG;
            trec[1].record.string = strdup(tmpstr);
            mips_code_add("", "la", 2, trec[0], trec[1]);
        }
    }
    else
    {
        ;//不可能
    }
}

void interpret_load(NODE_TRANS_RECORD rec, char *pos) //将rec存入寄存器pos
{
    struct MIPS_CODE_REC trec[3];
    char *op;
    if(rec.flag == 1)
        op = "lb";
    else if(rec.flag == 4)
        op = "lw";
    else
        op = "lb";
    if(rec.type == TRANS_SYMBOL)
    {
        if(globalsymbol[rec.record] == -1) //可修改符号
        {
            if(staticsymbol[rec.record] == -1) //局部符号
            {
                trec[0].type = MIPS_REG;
                trec[0].record.string = pos;
                interpret_sp_convert(functionsymbol[rec.record]);
                trec[1].type = MIPS_REG;
                trec[1].record.string = strdup(tmpstr);
                mips_code_add("", "lw", 2, trec[0], trec[1]);
                sprintf(tmpstr, "0(%s)", pos);
                trec[1].type = MIPS_REG;
                trec[1].record.string = strdup(tmpstr);
                mips_code_add("", op, 2, trec[0], trec[1]);
            }
            else //全局符号
            {
                trec[0].type = MIPS_REG;
                trec[0].record.string = pos;
                interpret_gp_convert(staticsymbol[rec.record]);
                trec[1].type = MIPS_REG;
                trec[1].record.string = strdup(tmpstr);
                mips_code_add("", "lw", 2, trec[0], trec[1]);
                sprintf(tmpstr, "0(%s)", pos);
                trec[1].type = MIPS_REG;
                trec[1].record.string = strdup(tmpstr);
                mips_code_add("", op, 2, trec[0], trec[1]);
            }
        }
        else //不可修改符号
        {
            trec[0].type = MIPS_REG;
            trec[0].record.string = pos;
            sprintf(tmpstr, "g%d", globalsymbol[rec.record]);
            trec[1].type = MIPS_REG;
            trec[1].record.string = strdup(tmpstr);
            mips_code_add("", "lw", 2, trec[0], trec[1]);
            sprintf(tmpstr, "0(%s)", pos);
            trec[1].type = MIPS_REG;
            trec[1].record.string = strdup(tmpstr);
            mips_code_add("", op, 2, trec[0], trec[1]);
        }
    }
    else if(rec.type == TRANS_ADDRESS)
    {
        if(globalvar[rec.record] == -1) //可修改变量
        {
            if(staticvar[rec.record] == -1) //局部变量
            {
                trec[0].type = MIPS_REG;
                trec[0].record.string = pos;
                interpret_sp_convert(functionvar[rec.record]);
                trec[1].type = MIPS_REG;
                trec[1].record.string = strdup(tmpstr);
                mips_code_add("", op, 2, trec[0], trec[1]);
            }
            else //全局变量
            {
                trec[0].type = MIPS_REG;
                trec[0].record.string = pos;
                interpret_gp_convert(staticvar[rec.record]);
                trec[1].type = MIPS_REG;
                trec[1].record.string = strdup(tmpstr);
                mips_code_add("", op, 2, trec[0], trec[1]);
            }
        }
        else //不可修改变量
        {
            trec[0].type = MIPS_REG;
            trec[0].record.string = pos;
            sprintf(tmpstr, "g%d", globalvar[rec.record]);
            trec[1].type = MIPS_REG;
            trec[1].record.string = strdup(tmpstr);
            mips_code_add("", op, 2, trec[0], trec[1]);
        }
    }
    else if(rec.type == TRANS_CONST)
    {
        if(rec.flag == 1) //string
        {
            ; //不可能            
        }
        else
        {
            trec[0].type = MIPS_REG;
            trec[0].record.string = pos;
            if(rec.record < 32768 && rec.record >= -32768)
            {
                trec[1].type = MIPS_NUMBER;
                trec[1].record.integer = rec.record;
                mips_code_add("", "li", 2, trec[0], trec[1]);
            }
            else
            {
                trec[1].type = MIPS_NUMBER;
                trec[1].record.integer = rec.record >> 16;
                mips_code_add("", "lui", 2, trec[0], trec[1]);
                trec[1].type = MIPS_NUMBER;
                trec[1].record.integer = rec.record & ((1<<16)-1);
                mips_code_add("", "ori", 3, trec[0], trec[0], trec[1]);
            }
        }
    }
    else
    {
        ;//不可能
    }
}

void interpret_store(NODE_TRANS_RECORD rec, char *pos) //将寄存器pos存入rec
{
    struct MIPS_CODE_REC trec[3];
    char *op;
    int i, k = 1;
    if(rec.flag == 1)
        op = "sb";
    else if(rec.flag == 4)
        op = "sw";
    else
    {
        op = "sb";
        k = rec.flag;
    }
    for(i=0; i<k && i<4; i++) //warning & to be continued 这里全局变量若空间大于4，则只会将前4位清零，但由于spim默认的$gp空间是0，所以这样做没有问题
    {
    if(rec.type == TRANS_SYMBOL)
    {
        if(globalsymbol[rec.record] == -1) //可修改符号
        {
            if(staticsymbol[rec.record] == -1) //局部符号
            {
                trec[0].type = MIPS_REG;
                trec[0].record.string = "$t0";
                interpret_sp_convert(functionsymbol[rec.record]);
                trec[1].type = MIPS_REG;
                trec[1].record.string = strdup(tmpstr);
                mips_code_add("", "lw", 2, trec[0], trec[1]);
                trec[0].type = MIPS_REG;
                trec[0].record.string = pos;
                trec[1].type = MIPS_REG;
                sprintf(tmpstr, "%d($t0)", i);
                trec[1].record.string = strdup(tmpstr);
                mips_code_add("", op, 2, trec[0], trec[1]);
            }
            else //全局符号
            {
                trec[0].type = MIPS_REG;
                trec[0].record.string = "$t0";
                interpret_gp_convert(staticsymbol[rec.record]);
                trec[1].type = MIPS_REG;
                trec[1].record.string = strdup(tmpstr);
                mips_code_add("", "lw", 2, trec[0], trec[1]);
                trec[0].type = MIPS_REG;
                trec[0].record.string = pos;
                trec[1].type = MIPS_REG;
                sprintf(tmpstr, "%d($t0)", i);
                trec[1].record.string = strdup(tmpstr);
                mips_code_add("", op, 2, trec[0], trec[1]);
            }
        }
        else //不可修改符号
        {
            trec[0].type = MIPS_REG;
            trec[0].record.string = "$t0";
            sprintf(tmpstr, "g%d", globalsymbol[rec.record]);
            trec[1].type = MIPS_REG;
            trec[1].record.string = strdup(tmpstr);
            mips_code_add("", "lw", 2, trec[0], trec[1]);
            trec[0].type = MIPS_REG;
            trec[0].record.string = pos;
            trec[1].type = MIPS_REG;
            sprintf(tmpstr, "%d($t0)", i);
            trec[1].record.string = strdup(tmpstr);
            mips_code_add("", op, 2, trec[0], trec[1]);
        }
    }
    else if(rec.type == TRANS_ADDRESS)
    {
        if(globalvar[rec.record] == -1) //可修改变量
        {
            if(staticvar[rec.record] == -1) //局部变量
            {
                trec[0].type = MIPS_REG;
                trec[0].record.string = pos;
                interpret_sp_convert(functionvar[rec.record] + i);
                trec[1].type = MIPS_REG;
                trec[1].record.string = strdup(tmpstr); 
                mips_code_add("", op, 2, trec[0], trec[1]);
            }
            else
            {
                trec[0].type = MIPS_REG;
                trec[0].record.string = pos;
                interpret_gp_convert(staticvar[rec.record] + i);
                trec[1].type = MIPS_REG;
                trec[1].record.string = strdup(tmpstr); 
                mips_code_add("", op, 2, trec[0], trec[1]);
            }
        }
        else //不可修改变量
        {
            trec[0].type = MIPS_REG;
            trec[0].record.string = pos;
            sprintf(tmpstr, "g%d+%d", globalvar[rec.record], i);
            trec[1].type = MIPS_REG;
            trec[1].record.string = strdup(tmpstr);
            mips_code_add("", op, 2, trec[0], trec[1]);
        }
    }
    else
    {
        ;//不可能
    }
    }
}

void interpret()
{
    int i, j, k;
    int pmain;
    int level = 0;
    int flag = 0;
    int augcount = 0;
    int gaugcount = 0;
    
    for(i=0; i<PAR_NUM; i++)
    {
        staticvar[M-i-1] = i * 4;
    }
    
    mipscodeaddflag = 2;
    for(i=0; i<codenum; i++)
    {
        int s;
        struct MIPS_CODE_REC rec[3];
        
        for(j=0; j<TTIFNUM; j++) if(strcmp(code[i].op, ttinstrformat[j].op) == 0) break;
        s = j;
        if(s == 29)
        {
            if(code[i].rec[0].type == TRANS_ADDRESS &&
               code[i].rec[1].type == TRANS_CONST && 
               code[i].rec[1].flag == 1)
            {
                rec[0].type = MIPS_STRING;
                rec[0].record.string = strdup((char *)code[i].rec[1].record);
                sprintf(tmpstr, "g%d", globallen);
                mips_code_add("", ".data", 0);
                mips_code_add(strdup(tmpstr), ".asciiz", 1, rec[0]);
                
                globalvar[code[i].rec[0].record] = globallen;
                globallen ++;
                continue;
            }
        }
        if(level == 0 && s != 33 && ttinstrformat[s].num >= 1)
        {
            if(code[i].rec[0].type == TRANS_ADDRESS)
            {
                /*
                mips_code_add("", ".data", 0);
                sprintf(tmpstr, "g%d", globallen);
                rec[0].type = MIPS_NUMBER;
                rec[0].record.integer = code[i].rec[0].flag;
                label = strdup(tmpstr);
                mips_code_add(label, ".space", 1, rec[0]);
                globalvar[code[i].rec[0].record] = globallen;
                globallen ++;
                */
                staticvar[code[i].rec[0].record] = staticlen;
                staticlen += code[i].rec[0].flag;
                while(staticlen % 4) staticlen ++;
            }
            else if(code[i].rec[0].type == TRANS_SYMBOL)
            {
                /*
                mips_code_add("", ".data", 0);
                sprintf(tmpstr, "g%d", globallen);
                rec[0].type = MIPS_NUMBER;
                rec[0].record.integer = 4;
                label = strdup(tmpstr);
                mips_code_add(label, ".space", 1, rec[0]);
                globalsymbol[code[i].rec[0].record] = globallen;
                globallen ++;
                */
                staticsymbol[code[i].rec[0].record] = staticlen;
                staticlen += 4;
            }
        }
        if(s == 33)
        {
            level ++;
        }
        if(s ==  34)
        {
            level --;
        }
    }
    
    for(i=0; i<codenum; i++)
    {
        int s;
        char *label;
        char *op;
        struct MIPS_CODE_REC rec[3];
        for(j=0; j<TTIFNUM; j++) if(strcmp(code[i].op, ttinstrformat[j].op) == 0) break;
        s = j;
        mipscodeaddflag = 0;
        if(s == 29 && code[i].rec[0].type == TRANS_ADDRESS &&
           code[i].rec[1].type == TRANS_CONST && 
           code[i].rec[1].flag == 1)
        {
            continue;
        }
        if(level == 0 && s != 33) //处理全局变量
        {
            if(s == 23) //CALL 后面对应的是main
            {
                pmain = i;
                continue;
            }
            mipscodeaddflag = 1;
        }
        if(mipscodeaddflag == 0 && flag == 0)
        {
            flag = 1;
            mips_code_add("", ".text", 0);
        }
        op = NULL;
        switch(s)
        {
            case 0: //LABEL
                sprintf(tmpstr, "l%d", code[i].rec[0].record);
                label = strdup(tmpstr);
                mips_code_add(label, "", 0);
                break;
            case 1: //GOTO
                rec[0].type = MIPS_LABEL;
                if(code[i].rec[0].record == code[pmain].rec[0].record)
                {
                    sprintf(tmpstr, "main");
                }
                else
                {
                    sprintf(tmpstr, "l%d", code[i].rec[0].record);
                }
                rec[0].record.string = strdup(tmpstr);
                mips_code_add("", "b", 1, rec[0]);
                break;
            case 2: //SHL
                if(!op) op = "sllv";
            case 3: //SHR
                if(!op) op = "srlv";
            case 4: //MUL
                if(!op) op = "mul";
            case 5: //DIV
                if(!op) op = "div"; //to be continued 注意这里需要做正负判断, 或者将这个判断放到translate中
            case 6: //MOD
                if(!op) op = "rem"; //to be continued 注意这里需要做正负判断, 或者将这个判断放到translate中
            case 7: //ADD
                if(!op)
                {
                /*
                    if(code[i].rec[2].type == TRANS_CONST)
                        op = "addi";
                    else
                */
                        op = "add";
                }
            case 8: //SUB
                if(!op) op = "sub";
            case 9: //AND
                if(!op) op = "and";
            case 10: //XOR
                if(!op) op = "xor";
            case 11: //OR
                if(!op) op = "or";
            case 12: //LAND
                if(!op) op = "and"; 
            case 13: //LOR
                if(!op) op = "or";
            case 14: //EQ
                if(!op) op = "seq";
            case 15: //NE
                if(!op) op = "sne";
            case 16: //LE
                if(!op) op = "sle";
            case 17: //GE
                if(!op) op = "sge";
            case 18: //GT
                if(!op) op = "sgt";
            case 19: //LT
                if(!op) op = "slt";
                interpret_load(code[i].rec[1], "$s0");
                interpret_load(code[i].rec[2], "$s1");
                rec[0].type = MIPS_REG;
                rec[0].record.string = "$s0";
                rec[1].type = MIPS_REG;
                rec[1].record.string = "$s0";
                rec[2].type = MIPS_REG;
                rec[2].record.string = "$s1";
                mips_code_add("", op, 3, rec[0], rec[1], rec[2]);
                interpret_store(code[i].rec[0], "$s0");
                break;
            case 20: //NOT
                interpret_load(code[i].rec[1], "$s0");
                rec[0].type = MIPS_REG;
                rec[0].record.string = "$s0";
                rec[1].type = MIPS_REG;
                rec[1].record.string = "$s0";
                mips_code_add("", "not", 2, rec[0], rec[1]);
                interpret_store(code[i].rec[0], "$s0");
                break;
            case 21: //LNOT
                interpret_load(code[i].rec[1], "$s0");
                sprintf(tmpstr, "n%d", newlabelnum);
                newlabelnum ++;
                rec[0].type = MIPS_REG;
                rec[0].record.string = "$s0";
                rec[1].type = MIPS_LABEL;
                rec[1].record.string = strdup(tmpstr);
                mips_code_add("", "beqz", 2, rec[0], rec[1]);
                rec[1].type = MIPS_NUMBER;
                rec[1].record.integer = 1;
                mips_code_add("", "li", 2, rec[0], rec[1]);
                mips_code_add(strdup(tmpstr), "", 0);
                mips_code_add("", "xori", 3, rec[0], rec[0], rec[1]);
                interpret_store(code[i].rec[0], "$s0");
                break;
            case 22: //IF
                interpret_load(code[i].rec[0], "$s0");
                rec[0].type = MIPS_REG;
                rec[0].record.string = "$s0";
                rec[1].type = MIPS_LABEL;
                sprintf(tmpstr, "l%d", code[i].rec[1].record);
                rec[1].record.string = strdup(tmpstr);
                mips_code_add("", "bnez", 2, rec[0], rec[1]);
                break;
            case 23: //CALL
                if(code[i].rec[0].record == code[pmain].rec[0].record)
                {
                    sprintf(tmpstr, "main");
                }
                else if(code[i].rec[0].record == 0 && augcount==1) //printf("...");
				{
					sprintf(tmpstr, "ls0");
				}
				else
                {
                    sprintf(tmpstr, "l%d", code[i].rec[0].record);
                }
                rec[0].type = MIPS_LABEL;
                rec[0].record.string = strdup(tmpstr);
                mips_code_add("", "jal", 1, rec[0]);
                augcount = 0;
                break;
            case 24: //RET
            case 34: //END
                interpret_sp_convert(functionlen - 4);
                rec[0].type = MIPS_REG;
                rec[0].record.string = "$s0";
                rec[1].type = MIPS_REG;
                rec[1].record.string = strdup(tmpstr);
                mips_code_add("", "lw", 2, rec[0], rec[1]);
                
                interpret_sp_convert(functionlen - 8);
                rec[0].type = MIPS_REG;
                rec[0].record.string = "$s1";
                rec[1].type = MIPS_REG;
                rec[1].record.string = strdup(tmpstr);
                mips_code_add("", "lw", 2, rec[0], rec[1]);
                
                interpret_sp_convert(functionlen - 12);
                rec[0].type = MIPS_REG;
                rec[0].record.string = "$ra";
                rec[1].type = MIPS_REG;
                rec[1].record.string = strdup(tmpstr);
                mips_code_add("", "lw", 2, rec[0], rec[1]);
                
                for(j=0; j<functionlen/32767; j++)
                {
                    sprintf(tmpstr, "%d", 32767);
                    rec[0].type = MIPS_REG;
                    rec[0].record.string = "$sp";
                    rec[1].type = MIPS_REG;
                    rec[1].record.string = "$sp";
                    rec[2].type = MIPS_REG;
                    rec[2].record.string = strdup(tmpstr);
                    mips_code_add("", "addiu", 3, rec[0], rec[1], rec[2]);
                }
                sprintf(tmpstr, "%d", functionlen % 32767);
                rec[0].type = MIPS_REG;
                rec[0].record.string = "$sp";
                rec[1].type = MIPS_REG;
                rec[1].record.string = "$sp";
                rec[2].type = MIPS_REG;
                rec[2].record.string = strdup(tmpstr);
                mips_code_add("", "addiu", 3, rec[0], rec[1], rec[2]);
                
                rec[0].type = MIPS_REG;
                rec[0].record.string = "$ra";
                mips_code_add("", "jr", 1, rec[0]);
                
                if(s == 34) level --;
                break;
            case 25: //PUSH
                interpret_load(code[i].rec[0], "$v0");
                break;
            case 26: //POP
                interpret_store(code[i].rec[0], "$v0");
                break;
            case 27: //TINT
                interpret_load(code[i].rec[1], "$s0");
                interpret_store(code[i].rec[0], "$s0");
                break;
            case 28: //TCHAR
                interpret_load(code[i].rec[1], "$s0");
                interpret_store(code[i].rec[0], "$s0");
                break;
            case 29: //MOVE
                interpret_load(code[i].rec[1], "$s0");
                interpret_store(code[i].rec[0], "$s0");
                break;
            case 30: //ADS
                interpret_load_ads(code[i].rec[1], "$s0");
                interpret_store(code[i].rec[0], "$s0");
                break;
            case 31: //STAR
                interpret_load(code[i].rec[1], "$s0");
                rec[0].type = MIPS_REG;
                if(globalsymbol[code[i].rec[0].record] == -1)
                {
                    if(staticsymbol[code[i].rec[0].record] == -1)
                    {
                        interpret_sp_convert(functionsymbol[code[i].rec[0].record]);
                        rec[0].record.string = strdup(tmpstr);
                    }
                    else
                    {
                        interpret_gp_convert(staticsymbol[code[i].rec[0].record]);
                        rec[0].record.string = strdup(tmpstr);
                    }
                }
                else
                {
                    sprintf(tmpstr, "g%d", globalsymbol[code[i].rec[0].record]);
                    rec[0].record.string = strdup(tmpstr);
                }
                rec[1].type = MIPS_REG;
                rec[1].record.string = "$s0";
                mips_code_add("", "sw", 2, rec[1], rec[0]);
                break;
            case 32: //NEW 不再考虑
                break;
            case 33: //FUNC
                gaugcount = 0;
                if(code[i].rec[0].record == code[pmain].rec[0].record)
                {
                    sprintf(tmpstr, "main");
                }
                else
                {
                    sprintf(tmpstr, "l%d", code[i].rec[0].record);
                }
                label = strdup(tmpstr);
                mips_code_add(label, "", 0);
                functionlen = 0;
                for(j=i+1; j<codenum; j++)
                {
                    if(strcmp(code[j].op, "END") == 0) break;
                    for(k=0; k<TTIFNUM; k++) if(strcmp(code[j].op, ttinstrformat[k].op) == 0) break;
                    for(k=ttinstrformat[k].num-1; k>=0; k--)
                    {
                        if(code[j].rec[k].type == TRANS_ADDRESS)
                        {
                            if(globalvar[code[j].rec[k].record] == -1 &&
                              staticvar[code[j].rec[k].record] == -1)
                            {
                                functionvar[code[j].rec[k].record] = functionlen;
                                functionlen += code[j].rec[k].flag;
                            }
                        }
                        else if(code[j].rec[k].type == TRANS_SYMBOL)
                        {
                            if(globalsymbol[code[j].rec[k].record] == -1 &&
                              staticsymbol[code[j].rec[k].record] == -1)
                            {
                                functionsymbol[code[j].rec[k].record] = functionlen;
                                functionlen += 4;
                            }
                        }
                        while(functionlen % 4) functionlen ++;
                    }
                }
                functionlen += 4*2 + 4; //s0,s1,ra
                
                for(j=0; j<functionlen/32767; j++)
                {
                    sprintf(tmpstr, "%d", 32767);
                    rec[0].type = MIPS_REG;
                    rec[0].record.string = "$sp";
                    rec[1].type = MIPS_REG;
                    rec[1].record.string = "$sp";
                    rec[2].type = MIPS_REG;
                    rec[2].record.string = strdup(tmpstr);
                    mips_code_add("", "subu", 3, rec[0], rec[1], rec[2]);
                }
                sprintf(tmpstr, "%d", functionlen%32767);
                rec[0].type = MIPS_REG;
                rec[0].record.string = "$sp";
                rec[1].type = MIPS_REG;
                rec[1].record.string = "$sp";
                rec[2].type = MIPS_REG;
                rec[2].record.string = strdup(tmpstr);
                mips_code_add("", "subu", 3, rec[0], rec[1], rec[2]);
                
                interpret_sp_convert(functionlen - 4);
                rec[0].type = MIPS_REG;
                rec[0].record.string = "$s0";
                rec[1].type = MIPS_REG;
                rec[1].record.string = strdup(tmpstr);
                mips_code_add("", "sw", 2, rec[0], rec[1]);
                
                interpret_sp_convert(functionlen - 8);
                rec[0].type = MIPS_REG;
                rec[0].record.string = "$s1";
                rec[1].type = MIPS_REG;
                rec[1].record.string = strdup(tmpstr);
                mips_code_add("", "sw", 2, rec[0], rec[1]);
                
                interpret_sp_convert(functionlen - 12);
                rec[0].type = MIPS_REG;
                rec[0].record.string = "$ra";
                rec[1].type = MIPS_REG;
                rec[1].record.string = strdup(tmpstr);
                mips_code_add("", "sw", 2, rec[0], rec[1]);
                
                if(code[i].rec[0].record == code[pmain].rec[0].record)
                {
                    maininsertpoint = mipscodenum - 1;
                }
                
                level ++;
                break;
            case 35: //AUG
                interpret_load(code[i].rec[0], "$s0");
                if(augcount < 4)
                {
                    sprintf(tmpstr, "$a%d", augcount);
                    rec[0].type = MIPS_REG;
                    rec[0].record.string = strdup(tmpstr);
                    rec[1].type = MIPS_REG;
                    rec[1].record.string = "$s0";
                    mips_code_add("", "move", 2, rec[0], rec[1]);
                }
                else
                {
                    NODE_TRANS_RECORD r;
                    r.type = TRANS_ADDRESS;
                    r.record = M-1-(augcount-4);
                    r.flag = 4;
                    interpret_store(r, "$s0");
                }
                augcount ++;
                break;
            case 36: //GAUG
                if(gaugcount < 4)
                {
                    sprintf(tmpstr, "$a%d", gaugcount);
                    interpret_store(code[i].rec[0], strdup(tmpstr));
                }
                else
                {
                    NODE_TRANS_RECORD r;
                    r.type = TRANS_ADDRESS;
                    r.record = M-1-(gaugcount-4);
                    r.flag = 4;
                    interpret_load(r, "$s0");
                    interpret_store(code[i].rec[0], "$s0");
                }
                gaugcount ++;
                break;
        }
    }
}
void output(int argc, char *argv[])
{
    FILE *out;
    FILE *in;
    int i, j, k;
#ifdef FINAL_TEST
    out = stdout;
#else
    sprintf(tmpstr, "%s.mips", argv[1]);
    out = fopen(tmpstr, "w");
#endif
    for(i=0; i<mipscodenum; i++)
    {
        if(i > 0 && strcmp(mipscode[i].op, "lw") == 0)
        {
            if(strcmp(mipscode[i-1].op, "sw") == 0 &&
               mipscode[i-1].rec[0].type == MIPS_REG &&
               mipscode[i].rec[0].type == MIPS_REG &&
               strcmp(mipscode[i-1].rec[0].record.string, mipscode[i].rec[0].record.string) == 0 &&
               strcmp(mipscode[i-1].rec[1].record.string, mipscode[i].rec[1].record.string) == 0)
            {
                continue;
            }
        }
        if(mipscode[i].label[0]) fprintf(out, "%s:\t", mipscode[i].label); else fprintf(out, "\t");
        if(mipscode[i].op[0])
        {
            fprintf(out, "%s\t", mipscode[i].op);
            for(j=0; j<mipscode[i].num; j++)
            {
                switch(mipscode[i].rec[j].type)
                {
                    case MIPS_REG:
                        fprintf(out, "%s", mipscode[i].rec[j].record.string);
                        break;
                    case MIPS_LABEL:
                        fprintf(out, "%s", mipscode[i].rec[j].record.string);
                        break;
                    case MIPS_NUMBER:
                        fprintf(out, "%d", mipscode[i].rec[j].record.integer);
                        break;
                    case MIPS_STRING:
                        fprintf(out, "%s", mipscode[i].rec[j].record.string);
                        break;
                }
                if(j+1<mipscode[i].num) fprintf(out, ",\t");
            }
        }
        fprintf(out, "\n");
        if(i == maininsertpoint)
        {
            for(j=M-1; j>mipscodemnum; j--)
            {
                if(j!=M-1 &&
                   strcmp(mipscode[j].op, "lw") == 0 &&
                   strcmp(mipscode[j+1].op, "sw") == 0 &&
                   mipscode[j+1].rec[0].type == MIPS_REG &&
                   mipscode[j].rec[0].type == MIPS_REG &&
                   strcmp(mipscode[j+1].rec[0].record.string, mipscode[j].rec[0].record.string) == 0 &&
                   strcmp(mipscode[j+1].rec[1].record.string, mipscode[j].rec[1].record.string) == 0)
                {
                    continue;
                }
                if(mipscode[j].label[0]) fprintf(out, "%s:\t", mipscode[j].label); else fprintf(out, "\t");
                if(mipscode[j].op[0])
                {
                    fprintf(out, "%s\t", mipscode[j].op);
                    for(k=0; k<mipscode[j].num; k++)
                    {
                        switch(mipscode[j].rec[k].type)
                        {
                            case MIPS_REG:
                                fprintf(out, "%s", mipscode[j].rec[k].record.string);
                                break;
                            case MIPS_LABEL:
                                fprintf(out, "%s", mipscode[j].rec[k].record.string);
                                break;
                            case MIPS_NUMBER:
                                fprintf(out, "%d", mipscode[j].rec[k].record.integer);
                                break;
                            case MIPS_STRING:
                                fprintf(out, "%s", mipscode[j].rec[k].record.string);
                                break;
                        }
                        if(k+1<mipscode[j].num) fprintf(out, ",\t");
                    }
                }
                fprintf(out, "\n");
            }
        }
    }
    
    fprintf(out, "ls0:\n");
    fprintf(out, "\tli\t$v0,\t4\n");
    fprintf(out, "\tsyscall\n");
	fprintf(out, "\tjr\t$ra\n");
	
	fprintf(out, "l2:\n");
	fprintf(out, "\tli\t$v0,\t9\n");
	fprintf(out, "\tsyscall\n");
	fprintf(out, "\tjr\t$ra\n");
	
    in = fopen("myc_printf.mips", "r");
    while(fgets(tmpstr, sizeof(tmpstr), in))
    {
        fprintf(out, "%s", tmpstr);
    }
    fclose(in);

    //fclose(out);
    
    for(i=0; i<mipscodeheadnum; i++)
    {
        if(mipscodehead[i].label[0]) fprintf(out, "%s:\t", mipscodehead[i].label); else fprintf(out, "\t");
        if(mipscodehead[i].op[0])
        {
            fprintf(out, "%s\t", mipscodehead[i].op);
            for(j=0; j<mipscodehead[i].num; j++)
            {
                switch(mipscodehead[i].rec[j].type)
                {
                    case MIPS_REG:
                        fprintf(out, "%s", mipscodehead[i].rec[j].record.string);
                        break;
                    case MIPS_LABEL:
                        fprintf(out, "%s", mipscodehead[i].rec[j].record.string);
                        break;
                    case MIPS_NUMBER:
                        fprintf(out, "%d", mipscodehead[i].rec[j].record.integer);
                        break;
                    case MIPS_STRING:
                        fprintf(out, "%s", mipscodehead[i].rec[j].record.string);
                        break;
                }
                if(j+1<mipscodehead[i].num) fprintf(out, ",\t");
            }
        }
        fprintf(out, "\n");
    }
    
    fclose(out);
}
int main(int argc, char *argv[])
{
    init(argc, argv);
    interpret();
    output(argc, argv);
    return 0;
}
