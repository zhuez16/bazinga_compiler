%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "syntax_tree.h"
#include "ast_typedef.h"

// external functions from lex
extern int yylex();
extern void yyrestart(FILE* fp);

// external variables from lexical_analyzer module
extern int lines;
extern char *yytext;
extern int pos_end;
extern int pos_start;
extern FILE* yyin;
// Global syntax tree
SyntaxTree *syntax_tree;

// Error reporting
void yyerror(const char *s);

// Helper functions written for you with love
TreeNode *node(const char *node_name, LL_AST_TYPE type, int children_num, ...);
%}

/* TODO: Complete this definition. */
%union {
    TreeNode* node;
}

/* TODO: Your tokens here. */

%start program
%token <node> INT_CONST
%token <node> ELSE IF INT RETURN VOID WHILE ARRAY EQ
    NEQ GTE LTE ADD SUB MUL DIV LT GT ASSIN SEMICOLON COMMA LPARENTHESE
    RPARENTHESE LBRACKET RBRACKET LBRACE RBRACE ID COMMENT BLANK EOL
    CONTINUE BREAK POS NEG NOT MOD AND OR CONST IDENT
%type <node> program comp_unit decl const_decl const_defs const_def const_init_val 
const_exp const_init_vals var_decl var_def var_defs init_vals init_val func_def funcf_param funcf_params
block block_item block_items stmt exp cond lval primary_exp number unary_exp unary_op assign_stmt if_stmt exp_stmt iter_stmt
break_stmt continue_stmt return_stmt lval_addr func_call pointer
func_rparams mulexp addexp relexp eqexp landexp lorexp  const_pointer
%%
program: comp_unit {$$=node("program",AST_program,1,$1); syntax_tree->root=$$;}

comp_unit:  comp_unit decl {
                $$=node("comp_unit",AST_comp_unit,2,$1,$2);
            }|
            comp_unit func_def {
                $$=node("comp_unit",AST_comp_unit,2,$1,$2);
            }|
            decl {
                $$=node("comp_unit",AST_comp_unit,1,$1);
            }|
            func_def{
                $$=node("comp_unit",AST_comp_unit,1,$1);
            }

decl:       const_decl {
                $$=node("decl",AST_decl,1,$1);
            }|
            var_decl {
                $$=node("decl",AST_decl,1,$1);
            }

const_decl: CONST INT const_defs SEMICOLON{
                $$=node("const_decl",AST_const_decl,4,$1,$2,$3,$4);
            }


const_defs: const_defs COMMA const_def{
                $$=node("const_defs",AST_const_defs,3,$1,$2,$3);
            }|
            const_def {
                $$=node("const_defs",AST_const_defs,1,$1);
            }

const_def:  IDENT ASSIN const_init_val{
                $$=node("const_def",AST_const_def,3,$1,$2,$3);
            }|
            IDENT const_pointer ASSIN const_init_val{
                $$=node("const_def",AST_const_def,4,$1,$2,$3,$4);
            }

const_pointer:LBRACKET const_exp RBRACKET const_pointer{
                $$=node("const_pointer",AST_const_pointer,4,$1,$2,$3,$4);
            }|
            LBRACKET const_exp RBRACKET{
                $$=node("const_pointer",AST_const_pointer,3,$1,$2,$3);
            }
const_init_val: const_exp{
                $$=node("const_init_val",AST_const_init_val,1,$1);
            }|
            LBRACE RBRACE{
                $$=node("const_init_val",AST_const_init_val,2,$1,$2);
            }|
            LBRACE const_init_vals RBRACE{
                $$=node("const_init_val",AST_const_init_val,3,$1,$2,$3);
            }
            
const_init_vals: const_init_val {
                $$=node("const_init_vals",AST_const_init_vals,1,$1);
            }|
            const_init_vals COMMA const_init_val{
                $$=node("const_init_vals",AST_const_init_vals,3,$1,$2,$3);
            }

var_decl :  INT var_defs SEMICOLON{
                $$=node("var_decl",AST_var_decl,3,$1,$2,$3);
            }

var_defs:   var_def {
                $$=node("var_defs",AST_var_defs,1,$1);
            }|
            var_defs COMMA var_def{
                $$=node("var_defs",AST_var_defs,3,$1,$2,$3);
            }

var_def:    IDENT {
                $$=node("var_def",AST_var_def,1,$1);
            }|
            IDENT ASSIN init_val {
            	$$=node("var_def",AST_var_def,3,$1,$2,$3);
            }|
            IDENT const_pointer {
                $$=node("var_def",AST_var_def,2,$1,$2);
            }|
            IDENT const_pointer ASSIN init_val{
                $$=node("var_def",AST_var_def,4,$1,$2,$3,$4);
            }

init_val:   exp{
                $$=node("init_val",AST_init_val,1,$1);
            }|
            LBRACE RBRACE{
                $$=node("init_val",AST_init_val,2,$1,$2);
            }|
            LBRACE init_vals RBRACE{
                $$=node("init_val",AST_init_val,3,$1,$2,$3);
            }

init_vals:  init_val {
                $$=node("init_vals",AST_init_vals,1,$1);
            }|
            init_vals COMMA init_val{
                $$=node("const_init_vals",AST_const_init_vals,3,$1,$2,$3);
            }

func_def:   VOID IDENT LPARENTHESE RPARENTHESE block{
                $$=node("func_def",AST_func_def,5,$1,$2,$3,$4,$5);
            }|
            INT IDENT LPARENTHESE RPARENTHESE block{
                $$=node("func_def",AST_func_def,5,$1,$2,$3,$4,$5);
            }|
            VOID IDENT LPARENTHESE funcf_params RPARENTHESE block{
                $$=node("func_def",AST_func_def,6,$1,$2,$3,$4,$5,$6);
            }|
            INT IDENT LPARENTHESE funcf_params RPARENTHESE block{
                $$=node("func_def",AST_func_def,6,$1,$2,$3,$4,$5,$6);
            }

funcf_params: funcf_param{
                $$=node("funcf_params",AST_funcf_params,1,$1);
            }|
            funcf_params COMMA funcf_param{
                $$=node("funcf_params",AST_funcf_params,3,$1,$2,$3);
            }

funcf_param:INT IDENT{
                $$=node("funcf_param",AST_funcf_param,2,$1,$2);
            }|
            INT IDENT ARRAY {
                $$=node("funcf_param",AST_funcf_param,3,$1,$2,$3);
            }|
            INT IDENT ARRAY pointer{
                $$=node("funcf_param",AST_funcf_param,4,$1,$2,$3,$4);
            }

block:      LBRACE RBRACE{
                $$=node("block",AST_block,2,$1,$2);
            }|
            LBRACE block_items RBRACE{
                $$=node("block",AST_block,3,$1,$2,$3);
            }

block_items:block_item{
                $$=node("block_items",AST_block_items,1,$1);
            }|
            block_items block_item{
                $$=node("block_items",AST_block_items,2,$1,$2);
            }

block_item: decl{
                $$=node("block_item",AST_block_item,1,$1);
            }|
            stmt{
                $$=node("block_item",AST_block_item,1,$1);
            }

stmt:       assign_stmt{
                $$=node("stmt",AST_stmt,1,$1);
            }|
            exp_stmt{
                $$=node("stmt",AST_stmt,1,$1);
            }|
            block{
                $$=node("stmt",AST_stmt,1,$1);
            }|
            if_stmt{
                $$=node("stmt",AST_stmt,1,$1);
            }|
            iter_stmt{
                $$=node("stmt",AST_stmt,1,$1);
            }|
            break_stmt{
                $$=node("stmt",AST_stmt,1,$1);
            }|
            continue_stmt{
                $$=node("stmt",AST_stmt,1,$1);
            }|
            return_stmt{
                $$=node("stmt",AST_stmt,1,$1);
            }
assign_stmt:lval_addr ASSIN exp SEMICOLON{
                $$=node("assign_stmt",AST_assign_stmt,4,$1,$2,$3,$4);
            }

lval_addr:  IDENT{
                $$=node("lval",AST_lval,1,$1);
            }|
            IDENT pointer{
                $$=node("lval",AST_lval,2,$1,$2);
            }
exp_stmt:   SEMICOLON{
                $$=node("exp_stmt",AST_exp_stmt,1,$1);
            }|
            exp SEMICOLON{
                $$=node("exp_stmt",AST_exp_stmt,2,$1,$2);
            }
if_stmt:    IF LPARENTHESE cond RPARENTHESE stmt{
                $$=node("if_stmt",AST_if_stmt,5,$1,$2,$3,$4,$5);
            }|
            IF LPARENTHESE cond RPARENTHESE stmt ELSE stmt{
                $$=node("if_else_stmt",AST_if_stmt,7,$1,$2,$3,$4,$5,$6,$7);
            }
iter_stmt:  WHILE LPARENTHESE cond RPARENTHESE stmt{
                $$=node("iter_stmt",AST_iter_stmt,5,$1,$2,$3,$4,$5);
            }
break_stmt: BREAK SEMICOLON{
                $$=node("break_stmt",AST_break_stmt,2,$1,$2);
            }
continue_stmt:CONTINUE SEMICOLON{
                $$=node("continue_stmt",AST_continue_stmt,2,$1,$2);
            }
return_stmt:RETURN SEMICOLON{
                $$=node("return_stmt",AST_return_stmt,2,$1,$2);
            }|
            RETURN exp SEMICOLON{
                $$=node("return_stmt",AST_return_stmt,3,$1,$2,$3);
            }
exp:        addexp{
                $$=node("exp",AST_exp,1,$1);
            }

cond:       lorexp{
                $$=node("cond",AST_cond,1,$1);
            }

lval:       IDENT{
                $$=node("lval",AST_lval,1,$1);
            }|
            IDENT pointer{
                $$=node("lval",AST_lval,2,$1,$2);
            }

pointer:    LBRACKET exp RBRACKET{
                $$=node("pointer",AST_pointer,3,$1,$2,$3);
            }|
            LBRACKET exp RBRACKET pointer{
                $$=node("pointer",AST_pointer,4,$1,$2,$3,$4);
            }

primary_exp:LPARENTHESE exp RPARENTHESE{
                $$=node("primary_exp",AST_primary_exp,3,$1,$2,$3);
            }|
            lval{
                $$=node("primary_exp",AST_primary_exp,1,$1);
            }|
            number{
                $$=node("primary_exp",AST_primary_exp,1,$1);
            }

number:     INT_CONST{
                $$=node("number",AST_number,1,$1);
            }

unary_op:   ADD { $$=node("unary_op", AST_unary_op, 1, $1); }|
            SUB { $$=node("unary_op", AST_unary_op, 1, $1); }|
            NOT { $$=node("unary_op", AST_unary_op, 1, $1); }

unary_exp:  primary_exp{
                $$=node("unary_exp",AST_unary_exp,1,$1);
            }|
            unary_op unary_exp{
                $$=node("unary_exp",AST_unary_exp,2,$1,$2);
            }|
            func_call{
                $$=node("unary_exp", AST_unary_exp, 1, $1);
            }
func_call:  IDENT LPARENTHESE RPARENTHESE{
                $$=node("func_call",AST_func_call,3,$1,$2,$3);
            }|
            IDENT LPARENTHESE func_rparams RPARENTHESE{
                $$=node("func_call",AST_func_call,4,$1,$2,$3,$4);
            }

func_rparams: exp{
                $$=node("func_rparams",AST_func_rparams,1,$1);
            }|
            func_rparams COMMA exp{
                $$=node("func_rparams",AST_func_rparams,3,$1,$2,$3);
            }

mulexp:     unary_exp{
                $$=node("mulexp",AST_mulexp,1,$1);
            }|
            mulexp MUL unary_exp{
                $$=node("mulexp",AST_mulexp,3,$1,$2,$3);
            }|
            mulexp DIV unary_exp{
                $$=node("mulexp",AST_mulexp,3,$1,$2,$3);
            }|
            mulexp MOD unary_exp{
                $$=node("mulexp",AST_mulexp,3,$1,$2,$3);
            }

addexp:     mulexp{
                $$=node("addexp",AST_addexp,1,$1);
            }|
            addexp ADD mulexp{
                $$=node("addexp",AST_addexp,3,$1,$2,$3);
            }|
            addexp SUB unary_exp{
                $$=node("addexp",AST_addexp,3,$1,$2,$3);
            }

relexp:     addexp{
                $$=node("relexp",AST_relexp,1,$1);
            }|
            relexp GT addexp{
                $$=node("relexp",AST_relexp,3,$1,$2,$3);
            }|
            relexp GTE addexp{
                $$=node("relexp",AST_relexp,3,$1,$2,$3);
            }|
            relexp LT addexp{
                $$=node("relexp",AST_relexp,3,$1,$2,$3);
            }|
            relexp LTE addexp{
                $$=node("relexp",AST_relexp,3,$1,$2,$3);
            }

eqexp:      relexp{
                $$=node("eqexp",AST_eqexp,1,$1);
            }|
            eqexp EQ relexp{
                $$=node("eqexp",AST_eqexp,2,$1,$2);
            }|
            eqexp NEQ relexp{
                $$=node("eqexp",AST_eqexp,3,$1,$2,$3);
            }

landexp:    eqexp{
                $$=node("landexp",AST_landexp,1,$1);
            }|
            landexp AND eqexp{
                $$=node("landexp",AST_landexp,3,$1,$2,$3);
            }

lorexp:     landexp{
                $$=node("lorexp",AST_lorexp,1,$1);
            }|
            lorexp OR landexp{
                $$=node("lorexp",AST_lorexp,1,$1);
            }

const_exp:  addexp{
                $$=node("const_exp",AST_const_exp,1,$1);
            }
%%
/// The error reporting function.
void yyerror(const char *s)
{
    fprintf(stderr, "error at line %d column %d: %s, yytext = %s\n", lines, pos_start, s, yytext);
}

/// Parse input from file `input_path`, and prints the parsing results
/// to stdout.  If input_path is NULL, read from stdin.
///
/// This function initializes essential states before running yyparse().
SyntaxTree *parse(const char *input_path)
{
    yydebug = 1;
    if (input_path != NULL) {
        if (!(yyin = fopen(input_path, "r"))) {
            fprintf(stderr, "[ERR] Open input file %s failed.\n", input_path);
            exit(1);
        }
    } else {
        yyin = stdin;
    }

    lines = pos_start = pos_end = 1;
    syntax_tree = new SyntaxTree();
    yyrestart(yyin);
    yyparse();
    return syntax_tree;
}

/// A helper function to quickly construct a tree node.
///
/// e.g.
///     $$ = node("program", 1, $1);
///     $$ = node("local-declarations", 0);
TreeNode *node(const char *name, LL_AST_TYPE type, int children_num, ...)
{
    TreeNode *p = new TreeNode(name, type);
    TreeNode *child;
    if (children_num == 0) {
        child = new TreeNode("epsilon", AST_EMPTY);
        p->add_child(child);
    } else {
        va_list ap;
        va_start(ap, children_num);
        for (int i = 0; i < children_num; ++i) {
            child = va_arg(ap, TreeNode *);
            p->add_child(child);
        }
        va_end(ap);
    }
    return p;
}
