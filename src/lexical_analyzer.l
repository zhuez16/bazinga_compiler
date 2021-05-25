%option noyywrap
%{
/*****************声明和选项设置  begin*****************/
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include "include/lexical_analyzer.h"

int lines;
int pos_start;
int pos_end;

/*****************声明和选项设置  end*****************/

%}
identifier_nondigit [A-Za-z_] 
identifier_digit [0-9]
hexadecimal_prefix 0x|0X
nonzero_digit [1-9]
octal_digit [0-7]
hexadecimal_digit [0-9a-fA-F]
decimal_const {nonzero_digit}{identifier_digit}*
octal_const 0{octal_digit}*
hexadecimal_const {hexadecimal_prefix}{hexadecimal_digit}+
%%

 /******************TODO*********************/
 /****请在此补全所有flex的模式与动作  start******/

 /*运算*/
\+		{return ADD;}
\-		{return SUB;}
\*		{return MUL;}
\/		{return DIV;}
%       {return MOD;}
\<		{return LT;}
\<=		{return LTE;}
\>		{return GT;}
\>=		{return GTE;}
&&      {return AND;}
\|\|    {return OR;}
!       {return NOT;}
==		{return EQ;}
!=		{return NEQ;}
=		{return ASSIN;}

 /*符号*/
;		{return SEMICOLON;}
,		{return COMMA;}
\(		{return LPARENTHESE;}
\)		{return RPARENTHESE;}
\[		{return LBRACKET;}
\]		{return RBRACKET;}
\{		{return LBRACE;}
\}		{return RBRACE;}

 /*关键字*/
const   {return CONST;}
else	{return ELSE;}
if	    {return IF;}
int	    {return INT;}
return	{return RETURN;}
void	{return VOID;}
while	{return WHILE;}
continue {return CONTINUE;}
break    {return BREAK;}

 /*ID & NUM*/
{identifier_nondigit}({identifier_nondigit}|{identifier_digit})*		{return IDENT;}
{decimal_const}|{octal_const}|{hexadecimal_const}		{return INT_CONST;}
\[\]	{return ARRAY;}

 /*其他*/
\n		{return EOL;}
(\/\*([^\*]|(\*)*[^\*/])*(\*)*\*\/)|(\/\/.) 	{return COMMENT;}
[ \t]		{return BLANK;}

. {return ERROR;}


 /****请在此补全所有flex的模式与动作  end******/
%%
 /****************C代码 start*************/

/// \brief analysize a *.cminus file
///
/// \param input_file, 需要分析的文件路径
/// \param token stream, Token_Node结构体数组，用于存储分析结果，具体定义参考lexical_analyer.h

void analyzer(char* input_file, std::vector<Token_Node>& token_stream){
    lines = 1;
    pos_start = 1;
    pos_end = 1;
    if(!(yyin = fopen(input_file,"r"))){
        printf("[ERR] No input file\n");
        exit(1);
    }
    printf("[START]: Read from: %s\n", input_file);

    int token;
    // int index = 0;

    while(token = yylex()){
        switch(token){
            case COMMENT:
                pos_start = pos_end;
                if(yytext[1] == '/'){  //single line comment
                    for (int i = 0; i < strlen(yytext); i++) {
                        pos_end ++;
                    }
                }
                else{ // multi line comment
                    for (int i = 0; i < strlen(yytext); i++) {
                        if (yytext[i] == '\n') { // 在注释中读到换行符
                            lines++;
                            pos_end = 1; 
                        }
                        else {
                            pos_end++;
                        }
                    }
                }
				break;
            case BLANK:
                pos_start++;
                pos_end++;
                break;
            case EOL:
                lines++;
                pos_start = 1;
                pos_end = 1;
                break;
            case ERROR:
                printf("[ERR]: unable to analysize %s at %d line, from %d to %d\n", yytext, lines, pos_start, pos_end);
				break;
			default:
            	pos_start = pos_end;
            	pos_end += strlen(yytext);
                Token_Node tk;
                if (token == ERROR) {
                    sprintf(tk.text, "[ERR]: unable to analysize %s at %d line, from %d to %d", yytext, lines, pos_start, pos_end);
                } else {
                    strcpy(tk.text, yytext);
                }
                tk.token = token;
                tk.lines = lines;
                tk.pos_start = pos_start;
                tk.pos_end = pos_end;
                /*
                index++;
                if (index >= MAX_NUM_TOKEN_NODE) {
                    printf("%s has too many tokens (> %d)", input_file, MAX_NUM_TOKEN_NODE);
                    exit(1);
                }
                */
                token_stream.push_back(tk);
        }
    }
    printf("[END]: Analysis completed.\n");
    return;
}






/****************C代码 end*************/
