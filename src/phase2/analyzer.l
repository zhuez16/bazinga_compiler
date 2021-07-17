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
#include "ast_typedef.h"


int lines;
int pos_start;
int pos_end;
void send_to_parser(const char* name, LL_AST_TYPE t);
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
\+		{send_to_parser(yytext, AST_ADD);return ADD;}
\-		{send_to_parser(yytext, AST_SUB);return SUB;}
\*		{send_to_parser(yytext, AST_MUL);return MUL;}
\/		{send_to_parser(yytext, AST_DIV);return DIV;}
%       {send_to_parser(yytext, AST_MOD);return MOD;}
\<		{send_to_parser(yytext, AST_LT);return LT;}
\<=		{send_to_parser(yytext, AST_LTE);return LTE;}
\>		{send_to_parser(yytext, AST_GT);return GT;}
\>=		{send_to_parser(yytext, AST_GTE);return GTE;}
&&      {send_to_parser(yytext, AST_AND);return AND;}
\|\|    {send_to_parser(yytext, AST_OR);return OR;}
!       {send_to_parser(yytext, AST_NOT);return NOT;}
==		{send_to_parser(yytext, AST_EQ);return EQ;}
!=		{send_to_parser(yytext, AST_NEQ);return NEQ;}
=		{send_to_parser(yytext, AST_ASSIGN);return ASSIN;}

 /*符号*/
;		{send_to_parser(yytext, AST_SEMICOLON);return SEMICOLON;}
,		{send_to_parser(yytext, AST_COMMA);return COMMA;}
\(		{send_to_parser(yytext, AST_LPARENTHESE);return LPARENTHESE;}
\)		{send_to_parser(yytext, AST_RPARENTHESE);return RPARENTHESE;}
\[		{send_to_parser(yytext, AST_LBRACKET);return LBRACKET;}
\]		{send_to_parser(yytext, AST_RBRACKET);return RBRACKET;}
\{		{send_to_parser(yytext, AST_LBRACE);return LBRACE;}
\}		{send_to_parser(yytext, AST_RBRACE);return RBRACE;}

 /*关键字*/
const   {send_to_parser(yytext, AST_CONST);return CONST;}
else	{send_to_parser(yytext, AST_ELSE);return ELSE;}
if	    {send_to_parser(yytext, AST_IF);return IF;}
int	    {send_to_parser(yytext, AST_INT);return INT;}
return	{send_to_parser(yytext, AST_RETURN);return RETURN;}
void	{send_to_parser(yytext, AST_VOID);return VOID;}
while	{send_to_parser(yytext, AST_WHILE);return WHILE;}
continue {send_to_parser(yytext, AST_CONTINUE);return CONTINUE;}
break    {send_to_parser(yytext, AST_BREAK);return BREAK;}

 /*ID & NUM*/
{identifier_nondigit}({identifier_nondigit}|{identifier_digit})*		{send_to_parser(yytext, AST_IDENT);return IDENT;}
{decimal_const}|{octal_const}|{hexadecimal_const}		{send_to_parser(yytext, AST_INT_CONST);return INT_CONST;}
\[\]	{send_to_parser(yytext, AST_ARRAY);return ARRAY;}

 /*其他*/
\n		{}
(\/\*([^\*]|(\*)*[^\*/])*(\*)*\*\/)|(\/\/.) 	{}
"//".* { }
[ \t]		{}

. {}


 /****请在此补全所有flex的模式与动作  end******/
%%
 /****************C代码 start*************/

void send_to_parser(const char* name, LL_AST_TYPE t){
    yylval.node = new TreeNode(name, t);
}





/****************C代码 end*************/
