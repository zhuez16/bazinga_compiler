## 语法分析器输出中间处理

编写此层转换的原因：由于语法分析器的特性，Bison输出的抽象语法树会出现大量分支，故设计此层来将原始AST进行展平，去除因类似 `CompUnit $\to$ CompUnit DeclList | DeclList` 这样的结构导致的语法树难以与原输入语言进行对应的情况，简化下一步转换为IR的代码编写。

### 输出结果

```
+ASTProgram
|-+ASTFuncDecl
| |-API: getArguments -> List<Tuple<String, ASTType>>
| |-API: getReturnType -> ASTType
| |-API: getStatements -> List<ASTStatement/VarDecl>
| |-+ASTStatement
|   |-API: getType -> ASTStatementType
|   |-API: getIfCondition -> Cond  #  由于涉及到多条件而不像课程中最多一个条件，故此处将处理交由下层进行
|   |-API: getVarDecl -> ASTVarDecl
|   |-API: getIfStatements -> List<ASTStatement>
|   |-API: getElseStatements -> List<ASTStatement>
|   |-API: getWhileCondition -> Cond
|   |-API: getWhileStatements -> List<ASTStatement>
|   |-API: getExpression -> Exp
|   |-API: getBlock -> Block
|   |-API: getAssign -> ASTAssign
|   |-+ASTBlock
|   | |-API: getStatements -> List<ASTStatement>
|   |-+ASTAssign
|   | |-API: getLvalName -> String
|   | |-API: isArray -> bool
|   | |-API: getArrayDefine -> List<Exp>
|   | |-API: getRval -> Exp
|-+ASTVarDecl
  |-API: isConst -> bool
  |-API: getName -> String
  |-API: isArray -> bool
  |-API: getArrayDefine -> List<Exp>
  |-API: hasInitValue -> bool
  |-API: getInitValue -> List<List/Exp> # 注：将使用Struct进行包装

```