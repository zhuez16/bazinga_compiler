# 编译器2021

### 测试说明
* lexer的测试
本CMakeLists为`FLEX`与`BISON`的测试文件
先正确安装flex与bison并添加至系统环境变量（推荐使用apt一键安装），然后使用执行如下命令：
```bash
mkdir build && cd build && cmake .. && make
```
此时build文件夹下应生成可执行文件`lexer`，执行`./lexer xxx.sysy`即可输出词法分析器结果


### ChangeLog
- 2021/5/25
  * 添加测试工具链
  * 使用Vector结构替代了原来的列表，现在TOKEN数量不再受到限制
  * 注：parser文件有问题，很多token无对应生成规则