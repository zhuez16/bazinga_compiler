Program $\to$ CompUnit

CompUnit $\to$ CompUnit DeclList | DeclList

DeclList $\to$ Decl | FuncDecl

Decl $\to$ ConstDecl | VarDecl

ConstDecl $\to$ 'const' Btype ConstDefs ';'

Btype $\to$ 'int'

ConstDef $\to $ 'ident' | 'ident' constpointer '=' constinitialval;

ConstInitialVal $\to$ constexp|'{''}'|'{' ConstInitialVals '}'

ConstInitialVals$\to$ ConstInitialVal|ConstInitialVals,ConstInitialVal

VarDecl$\to$BType VarDefs;

VarDefs$\to$Vardef | VarDefs,Vardef

VarDef$\to$Ident|Ident pointer|Ident pointer = InitVal

InitVal$\to$ exp|{}|{InitVals}

InitVals$\to$InitVal|InitVals,InitVal

FuncDef$\to$FuncType Ident () Block|FuncType Ident (FuncFParams) Block

FuncType$\to$int|void

FuncFParams$\to$FuncFParam|FuncFParams,FuncFParam

FuncFParam$\to$BTYPE Ident|BTYPE Ident []|BTYPE Ident [] pointer

Block$\to$ {} | {BlockItems}

BlockItems$\to$ BlockItem|BlockItems BlockItem

BlockItem$\to$Decl|Stmt

Stmt$\to$AssignStmt|ExpressionStmt|BlockStmt|IfStmt|IterStmt|

​			BreakStmt|ContinueStmt|ReturnStmt|

AssignStmt$\to$LVal=Exp;

ExpressionStmt$\to$;|Exp;

BlockStmt$\to$Block

IfStmt$\to$if (Cond) Stmt|If (Cond) Stmt Else Stmt

IterStmt$\to$while (Cond) Stmt

BreakStmt$\to$ break;

ContinueStmt$\to$ continue;

ReturnStmt$\to$ return;|return Exp;

Exp$\to$ AddExp

Cond$\to$LOrExp

LVal$\to$Ident|Ident pointer

PrimaryExp$\to$(Exp)|LVal|Number

Number$\to$IntConst

UnaryExp$\to$PrimaryExp|Ident()|Ident(FuncRParams)|UnaryOp UnaryExp

UnaryOp$\to$+|-|!

FuncRParams$\to$Exp|FuncRParams,Exp

MulExp$\to$UnaryExp|MulExp\*UnaryExp|MulExp/UnaryExp|MulExp%UnaryExp

AddExp$\to$MulExp|AddExp+MulExp|AddExp-MulExp

RelExp$\to$AddExp|RelExp<AddExp|RelExp>AddExp|RelExp<=AddExp|RelExp>=AddExp

EqExp$\to$RelExp|EqExp==RelExp|EqExp!=RelExp

LAndExp$\to$EqExp|LAndExp && EqExp

LOrExp$\to$LAndExp|LOrExp||LAndExp

ConstExp$\to$AddExp(Ident is Const)