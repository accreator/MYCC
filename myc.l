%option yylineno
%option yymore
ALP [a-zA-Z]
DIG [0-9]

%{
/* Please read README for more information */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "y.tab.h"
#include "def.h"
#include "utility.h"

void ignore_a();
void ignore_b();
void common(int, char *);
int is_type_name();

%}

%%

"/*"        { ignore_a(); }
"//"        { ignore_b(); }
^#.*$       { }

"typedef"   { common(0, "typedef"); return TYPEDEF_T; }
"void"      { common(0, "void"); return VOID_T; }
"char"      { common(0, "char"); return CHAR_T; }
"int"       { common(0, "int"); return INT_T; }
"struct"    { common(0, "struct"); return STRUCT_T; }
"union"     { common(0, "union"); return UNION_T; }
"if"        { common(1, "if"); return IF_C; }
"else"      { common(1, "else"); return ELSE_C; }
"while"     { common(1, "while"); return WHILE_C; }
"for"       { common(1, "for"); return FOR_C; }
"continue"  { common(1, "continue"); return CONTINUE_C; }
"break"     { common(1, "break"); return BREAK_C; }
"return"    { common(1, "return"); return RETURN_C; }
"sizeof"    { common(2, "sizeof"); return SIZEOF_OP; }

({ALP}|_)({ALP}|_|{DIG})*   { yylval.string = strdup(yytext); if(is_type_name()) { common(3, "TYPEDEF_NAME"); return TYPEDEF_NAME; } else {common(3, "IDENTIFIER"); return IDENTIFIER_O;} }
0[xX]({DIG}|[a-fA-F])+      { yylval.string = strdup(yytext); /*sscanf(yytext, "0%*[Xx]%x", &yylval.integer);*/ common(4, "INT_CONSTANT"); return INT_CONSTANT; }
0{DIG}+                     { yylval.string = strdup(yytext); /*sscanf(yytext, "0%o", &yylval.integer);*/ common(4, "INT_CONSTANT"); return INT_CONSTANT; }
{DIG}+                      { yylval.string = strdup(yytext); /*yylval.integer = atoi(yytext);*/ common(4, "INT_CONSTANT"); return INT_CONSTANT; }
'([^\\']|\\.)+'             { yylval.string = strdup(yytext); /*int i=0; yylval.character = 0; while(yytext[i]) { yylval.character <<= 8; yylval.character |= yytext[i]; i ++; }*/ common(5, "CHAR_CONSTANT"); return CHAR_CONSTANT; }
\"([^\\\"]|\\.)*\"          { yylval.string = strdup(yytext); common(6, "STRING_CONSTANT"); return STRING_CONSTANT; }

"<<="       { common(7, "<<="); return SHL_ASSIGN; }
">>="       { common(7, ">>="); return SHR_ASSIGN; }
"*="        { common(7, "*="); return MUL_ASSIGN; }
"/="        { common(7, "/="); return DIV_ASSIGN; }
"%="        { common(7, "%="); return MOD_ASSIGN; }
"+="        { common(7, "+="); return ADD_ASSIGN; }
"-="        { common(7, "-="); return SUB_ASSIGN; }
"&="        { common(7, "&="); return AND_ASSIGN; }
"^="        { common(7, "^="); return XOR_ASSIGN; }
"|="        { common(7, "|="); return OR_ASSIGN; }

"||"        { common(8, "||"); return OR_OP; }
"&&"        { common(8, "&&"); return AND_OP; }
"=="        { common(9, "=="); return EQ_OP; }
"!="        { common(9, "!="); return NE_OP; }
"<="        { common(9, "<="); return LE_OP; }
">="        { common(9, ">="); return GE_OP; }
"<<"        { common(10, "<<"); return SHL_OP; }
">>"        { common(10, ">>"); return SHR_OP; }
"++"        { common(11, "++"); return INC_OP; }
"--"        { common(11, "--"); return DEC_OP; }
"->"        { common(12, "->"); return PTR_OP; }
"..."       { common(13, "..."); return ELLIPSIS_O; }

"("         { common(14, "("); return '('; }
")"         { common(14, ")"); return ')'; }
";"         { common(14, ";"); return ';'; }
","         { common(14, ","); return ','; }
"="         { common(14, "="); return '='; }
"{"         { common(14, "{"); return '{'; }
"}"         { common(14, "}"); return '}'; }
"["         { common(14, "["); return '['; }
"]"         { common(14, "]"); return ']'; }
"|"         { common(14, "|"); return '|'; }
"^"         { common(14, "^"); return '^'; }
"&"         { common(14, "&"); return '&'; }
"<"         { common(14, "<"); return '<'; }
">"         { common(14, ">"); return '>'; }
"+"         { common(14, "+"); return '+'; }
"-"         { common(14, "-"); return '-'; }
"*"         { common(14, "*"); return '*'; }
"/"         { common(14, "/"); return '/'; }
"%"         { common(14, "%"); return '%'; }
"~"         { common(14, "~"); return '~'; }
"!"         { common(14, "!"); return '!'; }
"."         { common(14, "."); return '.'; }

[ \t\f\v\r\n]   { common(15, ""); /* */ }
.           { report_error(yylineno, "syntax error (l)", yytext); }

%%

int is_type_name()
{
    int s, l, r;
    int i;
    int f;
    
    s = yyleng;
    yytext[s] = input();
    for(i=s; yytext[i]; i++) if(!isspace(yytext[i])) break;
    l = i;
    for(; yytext[i]; i++)
    {
        if(!isalpha(yytext[i]) && !isdigit(yytext[i]) && yytext[i]!='_' && yytext[i]!='*')
        {
            break;
        }
    }
    r = i;
    f = yytext[s];
    unput(yytext[s]);
    yytext[s] = 0;
    
    if(r-l > 0 && f != ';')
        return tnhash_query(yytext);
    else
        return 0;
}

void ignore_a()
{
    char t;
    do {
        while((t=input()) && t!='*');
        if(t != 0) t = input();
    } while(t!=0 && t!='/');
}

void ignore_b()
{
    char t;
    while((t=input()) && t!='\n');
}

void common(int typenum, char *str)
{
#ifdef OUTPUT_TOKENS
    switch(typenum)
    {
        case 3:
            fprintf(yyout, "<%s, ", str[0]=='T'?"type":"id");
            fwrite(yytext, yyleng, 1, yyout);
            fprintf(yyout, ">\n");
            break;
        case 4:
            fprintf(yyout, "<number, ");
            fwrite(yytext, yyleng, 1, yyout);
            fprintf(yyout, ">\n");
            break;
        case 5:
            fprintf(yyout, "<char, "); 
            fwrite(yytext, yyleng, 1, yyout);
            fprintf(yyout, ">\n");
            break;
        case 6:
            fprintf(yyout, "<string, ");
            fwrite(yytext, yyleng, 1, yyout);
            fprintf(yyout, ">\n");
            break;
        default:
            if(typenum <= 14)
            {
                fwrite(yytext, yyleng, 1, yyout);
                fprintf(yyout, "\n");
            }
            break;
    }
    //printf("((-----[%d,%s]-----))", typenum, str);
#endif
}

