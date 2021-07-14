## 语法分析器输出中间处理

编写此层转换的原因：由于语法分析器的特性，Bison输出的抽象语法树会出现大量分支，故设计此层来将原始AST进行展平，去除因类似 `CompUnit $\to$ CompUnit DeclList | DeclList` 这样的结构导致的语法树难以与原输入语言进行对应的情况，简化下一步转换为IR的代码编写。

API已完成编写，详情请查看`include/ast.h`文件