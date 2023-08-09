#ifndef NODE_H_
#define NODE_H_

#define MAX_ID_LENGTH 100

typedef struct TYPE_ *TYPE; // 表示数据类型：int、float、char

typedef struct ID_ *ID;           // 变量
typedef struct FNUM_ *FNUM;       // 浮点数常量
typedef struct INUM_ *INUM;       // 整数常量
typedef struct CONST_C_ *CONST_C; // 字符常量
typedef struct CONST_S_ *CONST_S; // 字符串常量

typedef struct OP_ *OP; // 算术运算符、逻辑运算符

typedef struct MPTERM_ *MPTERM; // 中括号项，用于数字下标
typedef struct ARRAY_ *ARRAY;   // 数组

typedef struct EXP_ *EXP; // 表达式

typedef struct ADDR_ *ADDR; // 地址，包括变量和数组

typedef struct RC_ *RC; // 读内容
typedef struct WC_ *WC; // 写内容

typedef struct DEF_ *DEF;       // 变量、数组定义
typedef struct READ_ *READ;     // 读入变量
typedef struct WRITE_ *WRITE;   // 写变量
typedef struct ASSIGN_ *ASSIGN; // 变量赋值
typedef struct BRANCH_ *BRANCH; // if 条件分支语句
typedef struct LOOP_ *LOOP;     // while 循环语句
typedef struct JUMP_ *JUMP;     // continue、break 跳转语句

typedef struct STM_ *STM; // 语句

typedef struct FUNC_ *FUNC;           // 函数定义
typedef struct PARAM_ *PARAM;         // 函数定义参数
typedef struct FUNCALL_ *FUNCALL;     // 函数调用
typedef struct PARAMCALL_ *PARAMCALL; // 函数调用参数
typedef struct RETURN_ *RETURN;

typedef struct NODE_ *NODE; // 用于打印树测试

struct FUNC_
{
    TYPE funcType;
    ID funcName;
    PARAM params;
    STM funcStm;
};

struct PARAM_
{
    TYPE varType;
    ID id;
    PARAM next;
};

struct FUNCALL_
{
    ID id;
    PARAMCALL params;
};

struct PARAMCALL_
{
    EXP exp;
    PARAMCALL next;
};

struct TYPE_
{
    enum typekind
    {
        _int,
        _float,
        _char
    } typeKind;
};

struct INUM_
{
    int inum;
};

struct FNUM_
{
    float fnum;
};

struct CONST_C_
{
    char c;
};

struct CONST_S_
{
    char s[MAX_ID_LENGTH];
};

struct ID_
{
    char s[MAX_ID_LENGTH];
};

struct OP_
{
    enum opkind
    {
        _add,
        _minu,
        _times,
        _div,
        _eq,
        _ne,
        _gt,
        _ge,
        _lt,
        _le
    } opKind;
    EXP lexp, rexp; // 运算符左右的两个 exp
};

struct MPTERM_
{
    EXP exp;
    MPTERM next;
};

struct ARRAY_
{
    ID id;
    MPTERM mpTerm;
};

struct EXP_
{
    enum expkind
    {
        _addr,
        _fnum,
        _inum,
        _const_c,
        _const_s,
        _op,
        _funcall
    } expKind;
    union
    {
        ADDR addr;
        FNUM fnum;
        INUM inum;
        CONST_C c;
        CONST_S s;
        OP op;
        FUNCALL funcall;
    } exp;
};

struct ADDR_
{
    union
    {
        ID id;
        ARRAY arr;
    } addr;
    enum addrkind
    {
        _id,
        _array
    } addrKind;
};

struct DEF_
{
    TYPE type;
    ADDR addr;
};

struct RC_
{
    EXP exp;
    RC next;
};

struct READ_
{
    RC rc;
};

struct WC_
{
    EXP exp;
    WC next;
};

struct WRITE_
{
    enum writekind
    {
        _print,
        _printf,
        _println
    } writeKind;
    WC wc;
    INUM num;
    EXP exp;
};

struct ASSIGN_
{
    EXP addr; // 事实上它一定是ADDR，由yacc生成语法保证这一点，在这里写成EXP是为了后面IR使用方便
    EXP exp;
};

struct BRANCH_
{
    EXP condition;
    STM stm;
};

struct LOOP_
{
    EXP condition;
    STM stm;
};

struct JUMP_
{
    enum jumpkind
    {
        _continue,
        _break
    } jumpKind;
};

struct RETURN_
{
    EXP ret;
};

struct STM_
{
    /*
    wsx：
    对于 stm1;stm2 这种情况，处理的方法是将本身节点变为 stm1，
    next 节点变为 stm2
    */
    STM next;
    union
    {
        DEF def;
        READ read;
        WRITE write;
        ASSIGN assign;
        BRANCH branch;
        LOOP loop;
        JUMP jump;
        FUNC func;
        EXP ret;
        EXP exp;
    } stm;
    enum stmkind
    {
        _def,
        _read,
        _write,
        _assign,
        _branch,
        _loop,
        _jump,
        _func,
        _ret,
        _exp
    } stmKind;
};

#endif