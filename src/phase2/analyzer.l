%option noyywrap
%{
/*****
此文件是lexical_analyzer的修改版本
编译Bison请使用该文件而不是Phase1中的lexical_analyzer.l
****/
/*****************声明和选项设置  begin*****************/
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include "syntax_tree.h"
#include "bs_parser.h"


int lines;
int pos_start;
int pos_end;
void send_to_parser(const char* name);
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
\+		{send_to_parser(yytext);return ADD;}
\-		{send_to_parser(yytext);return SUB;}
\*		{send_to_parser(yytext);return MUL;}
\/		{send_to_parser(yytext);return DIV;}
%       {send_to_parser(yytext);return MOD;}
\<		{send_to_parser(yytext);return LT;}
\<=		{send_to_parser(yytext);return LTE;}
\>		{send_to_parser(yytext);return GT;}
\>=		{send_to_parser(yytext);return GTE;}
&&      {send_to_parser(yytext);return AND;}
\|\|    {send_to_parser(yytext);return OR;}
!       {send_to_parser(yytext);return NOT;}
==		{send_to_parser(yytext);return EQ;}
!=		{send_to_parser(yytext);return NEQ;}
=		{send_to_parser(yytext);return ASSIN;}

 /*符号*/
;		{send_to_parser(yytext);return SEMICOLON;}
,		{send_to_parser(yytext);return COMMA;}
\(		{send_to_parser(yytext);return LPARENTHESE;}
\)		{send_to_parser(yytext);return RPARENTHESE;}
\[		{send_to_parser(yytext);return LBRACKET;}
\]		{send_to_parser(yytext);return RBRACKET;}
\{		{send_to_parser(yytext);return LBRACE;}
\}		{send_to_parser(yytext);return RBRACE;}

 /*关键字*/
const   {send_to_parser(yytext);return CONST;}
else	{send_to_parser(yytext);return ELSE;}
if	    {send_to_parser(yytext);return IF;}
int	    {send_to_parser(yytext);return INT;}
return	{send_to_parser(yytext);return RETURN;}
void	{send_to_parser(yytext);return VOID;}
while	{send_to_parser(yytext);return WHILE;}
continue {send_to_parser(yytext);return CONTINUE;}
break    {send_to_parser(yytext);return BREAK;}

 /*ID & NUM*/
{identifier_nondigit}({identifier_nondigit}|{identifier_digit})*		{send_to_parser(yytext);return IDENT;}
{decimal_const}|{octal_const}|{hexadecimal_const}		{send_to_parser(yytext);return INT_CONST;}
\[\]	{send_to_parser(yytext);return ARRAY;}

 /*其他*/
\n		{}
(\/\*([^\*]|(\*)*[^\*/])*(\*)*\*\/)|(\/\/.) 	{}
[ \t]		{}

. {}


 /****请在此补全所有flex的模式与动作  end******/
%%
 /****************C代码 start*************/

void send_to_parser(const char* name){
    yylval.node = new TreeNode(name);
}





/****************C代码 end*************/
