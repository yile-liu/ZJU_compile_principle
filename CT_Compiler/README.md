# CT_Compiler

## 语言

```c
// 基本定义：
int x1;
float f1;
char c1;
int a[100];
char c[100][100];

// 读入
scan(i, f,...);

// 输出
print("sdvfsfvsd",x1,c1);
printf(10,x1);
println;	//换行

// 赋值：
i = 0;

// 循环：
while(condition) {...};

// 条件：
if (condition) {...};

// 运算符
+ - * / =
< > != <= >= ==

// 字符常量
'c'
' '

// 字符串常量
"sd fs d"

// 其他：
continue;
break;

// 函数定义：
type func_name(type a, type b, ...) {};

// 函数调用：
func_name(a, b, c, ...)
```

## Grammar

定义：
$$
def \rightarrow type~addr\\
array\rightarrow id~mpTerm\\
mpTerm\rightarrow mpTerm~[~exp~]|[~exp~]\\
type\rightarrow int|float|char\\
$$
读入：
$$
read\rightarrow scan~(~rc~)\\
rc\rightarrow rc~,~type~:~addr|type~:~addr\\
addr\rightarrow id|array
$$
输出：
$$
write\rightarrow print~(~wc~)\\
wc\rightarrow wc~,~exp|exp\\
write\rightarrow printf~(~num~,~exp~)\\
exp\rightarrow id|array|num|const\_c|const\_s|funcall|exp~op~exp|(~exp~)\\
write\rightarrow println\\
$$
赋值：
$$
assign\rightarrow addr~=~exp\\
$$
条件：
$$
branch\rightarrow if~(~exp~)~\{~stm~\}\\
stm\rightarrow stm;stm|e|def|read|write|assign|branch|loop|jump|func\\
$$
循环：
$$
loop\rightarrow while~(exp)\{stm\}
$$

运算：
$$
op\rightarrow +|-|*|/|==|!=|>|<|>=|<=
$$
跳转：
$$
jump\rightarrow continue|break
$$

函数定义：
$$
func\rightarrow type~id~(param)\{stm\}\\
param\rightarrow param,type~id|type~id\\
$$
函数调用：
$$
funcall\rightarrow id(paramcall)\\
paramcall\rightarrow paramcall, exp|exp\\
$$
