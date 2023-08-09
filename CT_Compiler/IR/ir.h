#ifndef IR_H_
#define IR_H_

#include "../AST/astNode.h"
#include "symbol.h"

#define MAX_STR_LENGTH 200
#define MAX_LABEL_LENGTH 5
#define MAX_BUCKET_NUM 100

/* EXP 部分 */
typedef struct irOp_ *irOp;
typedef struct irMem_ *irMem;
typedef struct irConstf_ *irConstf;
typedef struct irConsti_ *irConsti;
typedef struct irConst_C_ *irConst_C;
typedef struct irConst_S_ *irConst_S;
typedef struct irEseq_ *irEseq;
typedef struct irScan_ *irScan;

/* STM 部分 */
typedef struct irSEQ_ *irSeq;
typedef struct irLabel_ *irLabel;
typedef struct irMove_ *irMove;
typedef struct irJump_ *irJump;
typedef struct irCjump_ *irCjump;
typedef struct irPrint_ *irPrint;
typedef struct irRet_ *irRet;

/* 基本类型 */
typedef struct irExp_ *irExp;
typedef struct irStm_ *irStm;

/* 链表容器 */
typedef struct ExpList_ *ExpList;
typedef struct StmList_ *StmList;
typedef struct LabelList_ *LabelList;

struct irOp_
{
    enum IrOpKind
    {
        _irplus,
        _irminus,
        _irmul,
        _irdiv,
        _ireq,
        _irne,
        _irlt,
        _irle,
        _irgt,
        _irge,
        _true,
        _false // 可以直接为 true 或 false，即默认跳转
    } op;
    irExp left, right;
};

struct irMem_
{
    irExp offset;
    int upperLimit; // offset 能够去到的上限，用于防止数组访问越界
};

struct irConstf_
{
    float n;
};

struct irConsti_
{
    int n;
};

struct irConst_C_
{
    char c;
};

struct irConst_S_
{
    char str[MAX_STR_LENGTH];
};

struct irEseq_
{
    irStm stm;
    irExp exp;
};

struct irSEQ_
{
    irStm left, right;
};

struct irLabel_
{
    int label; // 从 0 开始自增
};

struct irMove_
{
    irExp dest, src;
};

struct irJump_
{
    irExp idx;
    LabelList labels;
};

struct irCjump_
{
    irExp op;
    int trueL, falseL;
};

struct irPrint_
{
    enum IrPrintType
    {
        _line,
        _printnormal,
        _printformat
    } type;
    union
    {
        struct print_
        {
            ExpList expList;
        } print;
        struct printf_
        {
            irExp exp;
            int width;
        } printf;
    } u;
};

struct irScan_
{
};

struct irExp_
{
    enum irExpKind
    {
        _irop,
        _irmem,
        _irconstf,
        _irconsti,
        _irconst_c,
        _irconst_s,
        _ireseq,
        _irscan,
        _ircall
    } kind;
    union
    {
        irOp op;
        irMem mem;
        irConsti const_i;
        irConstf const_f;
        irConst_C const_c;
        irConst_S const_s;
        irEseq eseq;
        irScan scan;
    } u;
    enum ExpType
    {
        _numint,
        _numfloat,
        _character,
        _string,
        _boolean,
        _null
    } valType; // 真实值的类型，用于类型检查
};

struct irRet_
{
    irExp exp;
};

struct irStm_
{
    enum irStmKind
    {
        _irseq,
        _irlabel,
        _irmove,
        _irjump,
        _ircjump,
        _irexp,
        _irprint,
        _irret,
        _irfinish // 标记程序运行结束
    } kind;
    union
    {
        irSeq seq;
        irLabel label;
        irMove move;
        irJump jump;
        irCjump cjump;
        irExp exp; // 表达式同时可以是一个语句
        irPrint print;
        irRet ret;
    } u;
    irStm fa;
};

struct ExpList_
{
    irExp exp;
    ExpList tail;
};

struct LabelList_
{
    int label;
    LabelList tail;
};

extern void IRTranslation(STM root);

irStm StmTranslate(STM startNode, FuncBucket func, int popFlag, irStm tail);
irExp Exp_IrExp(EXP exp, FuncBucket func);

#endif