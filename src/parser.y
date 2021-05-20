%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "syntax_tree.h"

// external functions from lex
extern int yylex();

// external variables from lexical_analyzer module
extern int lines;
extern char *yytext;
extern int pos_end;
extern int pos_start;
FILE* yyin;
// Global syntax tree
syntax_tree *gt;

// Error reporting
void yyerror(const char *s);

// Helper functions written for you with love
syntax_tree_node *node(const char *node_name, int children_num, ...);
%}

/* TODO: Complete this definition. */
%union {
    syntax_tree_node* node;
}

/* TODO: Your tokens here. */

%start program
%token <node> INT_CONST
%token <node> ELSE IF INT RETURN VOID WHILE ARRAY EQ
    NEQ GTE LTE ADD SUB MUL DIV LT GT ASSIN SEMICOLON COMMA LPARENTHESE
    RPARENTHESE LBRACKET RBRACKET LBRACE RBRACE ID COMMENT BLANK EOL
    CONTINUE BREAK POS NEG NOT MOD AND OR CONST IDENT
%type <node> program comp_unit decl const_decl btype const_defs const_def const_init_val 
const_exp const_init_vals var_decl var_def init_val func_def func_type funcf_param funcf_params
block block_item stmt exp cond lval primary_exp number unary_exp unary_op assign_stmt if_stmt exp_stmt iter_stmt
break_stmt continue_stmt return_stmt
func_rparams mulexp addexp relexp eqexp landexp lorexp constexp const_pointer
%%
program: comp_unit {$$=node("program",1,$1); gt->root=$$;}

comp_unit:  comp_unit decl {
                $$=node("comp_unit",2,$1,$2);
            }|
            comp_unit func_def {
                $$=node("comp_unit",2,$1,$2);
            }|
            decl {
                $$=node("comp_unit",1,$1);
            }|
            func_def{
                $$=node("comp_unit",1,$1);
            }

decl:       const_decl {
                $$=node("decl",1,$1);
            }|
            var_decl {
                $$=node("decl",1,$1);
            }

const_decl: CONST btype const_defs SEMICOLON{
                $$=node("const_decl",4,$1,$2,$3,$4);
            }

btype:      INT {
                $$=node("btype",1,$1);
            }

const_defs: const_defs COMMA const_def{
                $$=node("const_defs",3,$1,$2,$3);
            }|
            const_def {
                $$=node("const_defs",1,$1);
            }

const_def:  IDENT ASSIN const_init_val{
                $$=node("const_def",3,$1,$2,$3);
            }|
            IDENT const_pointer ASSIN const_init_val{
                $$=node("const_def",4,$1,$2,$3);
            }

const_pointer:LBRACKET const_exp RBRACKET const_pointer{
                $$=node("const_pointer",4,$1,$2,$3,$4);
            }|
            LBRACKET const_exp RBRACKET{
                $$=node("const_pointer",3,$1,$2,$3);
            }
const_init_val: const_exp{
                $$=node("const_init_val",1,$1);
            }|
            LBRACE RBRACE{
                $$=node("const_init_val",2,$1,$2);
            }|
            LBRACE const_init_vals RBRACE{
                $$=node("const_init_val",$1,$2,$3);
            }
            
const_init_vals: const_init_val {
                $$=node("const_init_vals",1,$1);
            }|
            const_init_vals COMMA const_init_val{
                $$=node("const_init_vals",3,$1,$2,$3);
            }

var_decl :  btype var_defs SEMICOLON{
                $$=node("var_decl",3,$1,$2,$3);
            }

var_defs:   var_def {
                $$=node("var_defs",1,$1);
            }|
            var_defs COMMA var_def{
                $$=node("var_defs",3,$1,$2,$3);
            }

var_def:    IDENT {
                $$=node("var_def",1,$1);
            }|
            IDENT const_pointer {
                $$=node("var_def",2,$1,$2);
            }|
            IDENT const_pointer ASSIN init_val{
                $$=node("var_def",4,$1,$2,$3,$4);
            }

init_val:   exp{
                $$=node("init_val",1,$1);
            }|
            LBRACE RBRACE{
                $$=node("init_val",2,$1,$2);
            }|
            LBRACE init_vals RBRACE{
                $$=node("init_val",3,$1,$2,$3);
            }

init_vals:  init_val {
                $$=node("init_vals",1,$1);
            }|
            init_vals COMMA init_val{
                $$=node("const_init_vals",3,$1,$2,$3);
            }

func_def:   func_type IDENT LPARENTHESE RPARENTHESE block{
                $$=node("func_def",5,$1,$2,$3,$4,$5);
            }|
            func_type IDENT LPARENTHESE funcf_params RPARENTHESE block{
                $$=node("func_def",6,$1,$2,$3,$4,$5);
            }

func_type:  VOID{
                $$=node("func_type",1,$1);
            }|
            INT{
                $$=node("func_type",1,$1);
            }

funcf_params: funcf_param{
                $$=node("funcf_params",1,$1);
            }|
            funcf_param COMMA funcf_params{
                $$=node("funcf_params",3,$1,$2,$3);
            }

funcf_param:btype IDENT{
                $$=node("funcf_param",2,$1,$2);
            }|
            btype IDENT LBRACKET RBRACKET {
                $$=node("funcf_param",4,$1,$2,$3,$4);
            }|
            btype IDENT LBRACKET RBRACKET pointer{
                $$=node("funcf_param",5,$1,$2,$3,$4,$5);
            }

block:      LBRACE RBRACE{
                $$=node("block",2,$1,$2);
            }|
            LBRACE block_items RBRACE{
                $$=node("block",3,$1,$2,$3);
            }

block_items:block_item{
                $$=node("block_items",1,$1);
            }|
            block_items block_item{
                $$=node("block_items",2,$1,$2);
            }

block_item: decl{
                $$=node("block_item",1,$1);
            }|
            stmt{
                $$=node("block_item",1,$1);
            }

stmt:       assign_stmt{
                $$=node("stmt",1,$1);
            }|
            exp_stmt{
                $$=node("stmt",1,$1);
            }|
            block{
                $$=node("stmt",1,$1);
            }|
            if_stmt{
                $$=node("stmt",1,$1);
            }|
            iter_stmt{
                $$=node("stmt",1,$1);
            }|
            break_stmt{
                $$=node("stmt",1,$1);
            }|
            continue_stmt{
                $$=node("stmt",1,$1);
            }|
            return_stmt{
                $$=node("stmt",1,$1);
            }
assign_stmt:lval ASSIN exp SEMICOLON{
                $$=node("assign_stmt",4,$1,$2,$3,$4);
            }
exp_stmt:   SEMICOLON{
                $$=node("exp_stmt",1,$1);
            }
            exp SEMICOLON{
                $$=node("exp_stmt",2,$1,$2);
            }
if_stmt:    IF LPARENTHESE cond RPARENTHESE stmt{
                $$=node("if_stmt",5,$1,$2,$3,$4,$5);
            }|
            IF LPARENTHESE cond RPARENTHESE stmt ELSE stmt{
                $$=node("if_else_stmt"7,$1,$2,$3,$4,$5,$6,$7);
            }
iter_stmt:  WHILE LPARENTHESE cond RPARENTHESE stmt{
                $$=node("iter_stmt",5,$1,$2,$3,$4,$5);
            }
break_stmt: BREAK SEMICOLON{
                $$=node("break_stmt",2,$1,$2);
            }
continue_stmt:CONTINUE SEMICOLON{
                $$=node("continue_stmt",2,$1,$2);
            }
return_stmt:RETURN SEMICOLON{
                $$=node("return_stmt",2,$1,$2);
            }|
            RETURN exp SEMICOLON{
                $$=node("return_stmt",3,$1,$2,$3);
            }
exp:        addexp{
                $$=node("exp",1,$1);
            }

cond:       lorexp{
                $$=node("cond",1,$1);
            }

lval:       IDENT{
                $$=node("lval",1,$1);
            }|
            IDENT pointer{
                $$=node("lval",2,$1,$2);
            }

pointer:    LBRACKET exp RBRACKET{
                $$=node("pointer",3,$1,$2,$3);
            }|
            LBRACKET exp RBRACKET pointer{
                $$=node("pointer",4,$1,$2,$3,$4);
            }

primary_exp:LPARENTHESE exp RPARENTHESE{
                $$=node("primary_exp",3,$1,$2,$3);
            }|
            lval{
                $$=node("primary_exp",1,$1);
            }|
            number{
                $$=node("primary_exp",1,$1);
            }

number:     INT_CONST{
                $$=node("number",1,$1);
            }

unary_exp:  primary_exp{
                $$=node("unary_exp",1,$1);
            }|
            IDENT LPARENTHESE RPARENTHESE{
                $$=node("unary_exp",3,$1,$2,$3);
            }|
            IDENT LPARENTHESE func_rparams RPARENTHESE{
                $$=node("unary_exp",4,$1,$2,$3,$4);
            }


%%
/// The error reporting function.
void yyerror(const char *s)
{
    // TO STUDENTS: This is just an example.
    // You can customize it as you like.
    fprintf(stderr, "error at line %d column %d: %s, yytext = %s\n", lines, pos_start, s, yytext);
}

/// Parse input from file `input_path`, and prints the parsing results
/// to stdout.  If input_path is NULL, read from stdin.
///
/// This function initializes essential states before running yyparse().
syntax_tree *parse(const char *input_path)
{
    if (input_path != NULL) {
        if (!(yyin = fopen(input_path, "r"))) {
            fprintf(stderr, "[ERR] Open input file %s failed.\n", input_path);
            exit(1);
        }
    } else {
        yyin = stdin;
    }

    lines = pos_start = pos_end = 1;
    gt = new_syntax_tree();
    yyrestart(yyin);
    yyparse();
    return gt;
}

/// A helper function to quickly construct a tree node.
///
/// e.g.
///     $$ = node("program", 1, $1);
///     $$ = node("local-declarations", 0);
syntax_tree_node *node(const char *name, int children_num, ...)
{
    syntax_tree_node *p = new_syntax_tree_node(name);
    syntax_tree_node *child;
    if (children_num == 0) {
        child = new_syntax_tree_node("epsilon");
        syntax_tree_add_child(p, child);
    } else {
        va_list ap;
        va_start(ap, children_num);
        for (int i = 0; i < children_num; ++i) {
            child = va_arg(ap, syntax_tree_node *);
            syntax_tree_add_child(p, child);
        }
        va_end(ap);
    }
    return p;
}
