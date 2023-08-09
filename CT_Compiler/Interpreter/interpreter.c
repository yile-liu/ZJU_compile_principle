#include "interpreter.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*
首先申请地址空间
遍历整个 IR 树，搜索所有的 label 并记录节点指针，由于 label 是从 0 开始自增生成的，所以可以直接用数组存。注意不能直接存 label 的 stm 节点，因为无法通过该节点向”上索引“，
因此要存 label 的父节点 seq。
*/

// 总体栈空间
void *STACK;

irStm seqWithLeftLabel[MAX_LABEL_NUM]; // IR 树中所有左节点是 label 的 seq 节点，下标即为 label

void RecordLabel(int label, irStm seq)
{
    if (seqWithLeftLabel[label]) // 一个 label 只能插入一次
    {
        ERROR("ERROR: Duplicate label\n");
    }
    seqWithLeftLabel[label] = seq;
}

/*
返回 label 对应的 stm，即父节点的右节点
*/
irStm JumpToLabel(int label)
{
    return seqWithLeftLabel[label]->u.seq->right;
}

void FindLabelFromExp(irExp exp)
{
    if (!exp)
        return;
    switch (exp->kind)
    {
    case _irop: // 递归继续寻找 op 的左右表达式
        FindLabelFromExp(exp->u.op->left);
        FindLabelFromExp(exp->u.op->right);
        break;
    case _irmem:
        FindLabelFromExp(exp->u.mem->offset);
        break;
    case _irconsti:
    case _irconstf:
    case _irconst_c:
    case _irconst_s:
    case _irscan:
    case _ircall:
        break;
    case _ireseq:
        FindLabelFromExp(exp->u.eseq->exp);
        FindLabelFromStm(exp->u.eseq->stm);
        break;
    default:
        ERROR("ERROR: The type of exp is wrong\n");
        break;
    }
}

void FindLabelFromStm(irStm stm)
{
    if (stm == NULL)
        return;
    switch (stm->kind)
    {
    case _irfinish:
        return;
    case _irseq:
        if (!stm->u.seq->left || !stm->u.seq->right)
            ERROR("ERROR: The left or right of seq is null\n");
        if (stm->u.seq->left->kind == _irlabel) // 检查左节点是否为 label，如果是则记录该 label 的父节点
            RecordLabel(stm->u.seq->left->u.label->label, stm);
        else
            FindLabelFromStm(stm->u.seq->left); // 递归继续寻找 label

        if (stm->u.seq->right->kind == _irlabel) // label 应该只出现在 seq 的左边
            ERROR("ERROR: Unexpected to find label in the right of seq\n");
        else
            FindLabelFromStm(stm->u.seq->right);

        break;
    case _irlabel: // 如果一个节点是 label，那么在找到他的父节点的时候就会停止继续深入
        ERROR("ERROR: Unexpected to find label\n");
        break;
    case _irmove:
        FindLabelFromExp(stm->u.move->dest);
        FindLabelFromExp(stm->u.move->src);
        break;
    case _irjump:
        FindLabelFromExp(stm->u.jump->idx);
        break;
    case _ircjump:
        FindLabelFromExp(stm->u.cjump->op);
        break;
    case _irexp:
        FindLabelFromExp(stm->u.exp);
        break;
    case _irprint:
        if (stm->u.print->type == _printformat)
            FindLabelFromExp(stm->u.print->u.printf.exp);
        else if (stm->u.print->type == _printnormal)
        {
            ExpList el = stm->u.print->u.print.expList;
            while (el)
            {
                FindLabelFromExp(el->exp);
                el = el->tail;
            }
        }
        break;
    case _irret:
        FindLabelFromExp(stm->u.ret->exp);
        break;
    default:
        ERROR("ERROR: The type of stm is wrong\n");
        break;
    }
}

/*
将 expvalue 存进内存，内存位置由 mem 的 exp 节点指示
*/
void SaveExpValueInMem(ExpValue ev, irExp mem)
{
    if (mem->kind != _irmem) // 首先要检查 mem exp 是否是 irmem
        ERROR("ERROR: mem is not irmem type\n");

    ExpValue offset = ExecExp(mem->u.mem->offset);
    if (offset.type != _expint)
        ERROR("ERROR: The type of offset in mem must be int\n");

    int off = offset.u.i;

    switch (mem->valType)
    {
    case _numint:
        if (ev.type == _expint)
        {
            *((int *)(STACK + off)) = ev.u.i;
        }
        else if (ev.type == _expfloat)
        {
            *((int *)(STACK + off)) = (int)(ev.u.f);
        }
        else if (ev.type == _expchar)
        {
            *((int *)(STACK + off)) = (int)(ev.u.c);
        }
        else
        {
            ERROR("ERROR: Can only save int or float to int\n");
        }
        break;
    case _numfloat:
        if (ev.type == _expint)
        {
            *((float *)(STACK + off)) = (float)(ev.u.i);
        }
        else if (ev.type == _expfloat)
        {
            *((float *)(STACK + off)) = ev.u.f;
        }
        else
        {
            ERROR("ERROR: Can only save int or float to float\n");
        }
        break;
    case _expchar:
        if (ev.type == _expchar)
            *((char *)(STACK + off)) = ev.u.c;
        else if (ev.type == _expint)
        {
            if (ev.u.i > 255)
                ERROR("If want to save int in char, then int <= 255\n");
            *((char *)(STACK + off)) = (char)(ev.u.i);
        }
        else
        {
            ERROR("ERROR: Can only save int or char to char\n");
        }
        break;
    case _expstring:
        if (ev.type == _expstring)
            strcpy((char *)(STACK + off), ev.u.str);
        else
        {
            ERROR("Can only save str to str\n");
        }
        break;
    default:
        ERROR("ERROR: The type of ev in mem is wrong\n");
        break;
    }
}

/*
根据 op 类型对两个 expvalue 进行对应 op 操作
*/
ExpValue OpForExpValue(enum IrOpKind op, ExpValue e1, ExpValue e2)
{
    ExpValue ret;
    if (op == _irplus)
    {
        if (e1.type == _expint && e2.type == _expint) // 二者都是 int，返回为 int
        {
            ret.type = _expint;
            ret.u.i = e1.u.i + e2.u.i;
        }
        else if ((e1.type == _expint || e1.type == _expfloat) && (e2.type == _expint || e2.type == _expfloat)) // 二者都是数字且有浮点数，返回 float
        {
            ret.type = _expfloat;
            float f1 = (e1.type == _expint) ? e1.u.i : e1.u.f;
            float f2 = (e2.type == _expint) ? e2.u.i : e2.u.f;
            ret.u.f = f1 + f2;
        }
        else // 包含字符，返回为 int
        {
            ret.type = _expint;
            int i1 = (e1.type == _expint) ? e1.u.i : (e1.type == _expfloat ? e1.u.f : (e1.type == _expchar ? e1.u.c : 0));
            int i2 = (e2.type == _expint) ? e2.u.i : (e2.type == _expfloat ? e2.u.f : (e2.type == _expchar ? e2.u.c : 0));
            ret.u.i = i1 + i2;
        }
    }
    else if (op == _irminus)
    {
        if (e1.type == _expint && e2.type == _expint) // 二者都是 int，返回为 int
        {
            ret.type = _expint;
            ret.u.i = e1.u.i - e2.u.i;
        }
        else if ((e1.type == _expint || e1.type == _expfloat) && (e2.type == _expint || e2.type == _expfloat)) // 二者都是数字且有浮点数，返回 float
        {
            ret.type = _expfloat;
            float f1 = (e1.type == _expint) ? e1.u.i : e1.u.f;
            float f2 = (e2.type == _expint) ? e2.u.i : e2.u.f;
            ret.u.f = f1 - f2;
        }
        else
        {
            ret.type = _expint;
            int i1 = (e1.type == _expint) ? e1.u.i : (e1.type == _expfloat ? e1.u.f : (e1.type == _expchar ? e1.u.c : 0));
            int i2 = (e2.type == _expint) ? e2.u.i : (e2.type == _expfloat ? e2.u.f : (e2.type == _expchar ? e2.u.c : 0));
            ret.u.i = i1 - i2;
        }
    }
    else if (op == _irmul)
    {
        if (e1.type == _expint && e2.type == _expint) // 二者都是 int，返回为 int
        {
            ret.type = _expint;
            ret.u.i = e1.u.i * e2.u.i;
        }
        else if ((e1.type == _expint || e1.type == _expfloat) && (e2.type == _expint || e2.type == _expfloat)) // 二者都是数字且有浮点数，返回 float
        {
            ret.type = _expfloat;
            float f1 = (e1.type == _expint) ? e1.u.i : e1.u.f;
            float f2 = (e2.type == _expint) ? e2.u.i : e2.u.f;
            ret.u.f = f1 * f2;
        }
        else
        {
            ERROR("ERROR: The exps of mul must be number\n");
        }
    }
    else if (op == _irdiv)
    {
        if (e1.type == _expint && e2.type == _expint) // 二者都是 int，返回为 int
        {
            if (e2.u.i == 0)
                ERROR("ERROR: Can not div 0\n");
            ret.type = _expint;
            ret.u.i = e1.u.i / e2.u.i;
        }
        else if ((e1.type == _expint || e1.type == _expfloat) && (e2.type == _expint || e2.type == _expfloat)) // 二者都是数字且有浮点数，返回 float
        {
            ret.type = _expfloat;
            float f1 = (e1.type == _expint) ? e1.u.i : e1.u.f;
            float f2 = (e2.type == _expint) ? e2.u.i : e2.u.f;
            if (f2 == 0)
                ERROR("ERROR: Can not div 0.0\n");
            ret.u.f = f1 / f2;
        }
        else
        {
            ERROR("ERROR: The exps of minus must be number\n");
        }
    }
    else if (op == _ireq)
    {
        ret.type = _expboolean;
        if ((e1.type == _expint || e1.type == _expfloat || e1.type == _expchar) && (e2.type == _expint || e2.type == _expfloat || e2.type == _expchar)) // 二者都是数字且有浮点数，返回 float
        {
            float f1 = (e1.type == _expint) ? e1.u.i : (e1.type == _expfloat ? e1.u.f : (int)e1.u.c);
            float f2 = (e2.type == _expint) ? e2.u.i : (e2.type == _expfloat ? e2.u.f : (int)e2.u.c);
            ret.u.boolean = (f1 == f2);
        }
        else if (e1.type == _expchar && e2.type == _expchar)
        {
            ret.u.boolean = (e1.u.c == e2.u.c);
        }
        else if (e1.type == _expboolean && e2.type == _expboolean)
        {
            ret.u.boolean = (e1.u.boolean == e2.u.boolean);
        }
    }
    else if (op == _irne)
    {
        ret = OpForExpValue(_ireq, e1, e2);
        ret.u.boolean = 1 - ret.u.boolean;
    }
    else if (op == _irlt)
    {
        ret.type = _expboolean;
        if ((e1.type == _expint || e1.type == _expfloat) && (e2.type == _expint || e2.type == _expfloat)) // 二者都是数字且有浮点数，返回 float
        {
            float f1 = (e1.type == _expint) ? e1.u.i : e1.u.f;
            float f2 = (e2.type == _expint) ? e2.u.i : e2.u.f;
            ret.u.boolean = (f1 < f2);
        }
        else if (e1.type == _expchar && e2.type == _expchar)
        {
            ret.u.boolean = (e1.u.c < e2.u.c);
        }
    }
    else if (op == _irle)
    {
        ret.type = _expboolean;
        if ((e1.type == _expint || e1.type == _expfloat) && (e2.type == _expint || e2.type == _expfloat)) // 二者都是数字且有浮点数，返回 float
        {
            float f1 = (e1.type == _expint) ? e1.u.i : e1.u.f;
            float f2 = (e2.type == _expint) ? e2.u.i : e2.u.f;
            ret.u.boolean = (f1 <= f2);
        }
        else if (e1.type == _expchar && e2.type == _expchar)
        {
            ret.u.boolean = (e1.u.c <= e2.u.c);
        }
    }
    else if (op == _irgt)
    {
        ret = OpForExpValue(_irle, e1, e2);
        ret.u.boolean = 1 - ret.u.boolean;
    }
    else if (op == _irge)
    {
        ret = OpForExpValue(_irlt, e1, e2);
        ret.u.boolean = 1 - ret.u.boolean;
    }
    else if (op == _true)
    {
        ret.type = _expboolean;
        ret.u.boolean = 1;
    }
    else if (op == _false)
    {
        ret.type = _expboolean;
        ret.u.boolean = 0;
    }
    return ret;
}

/*
根据表达式类型，从内存中安装偏移量取出值
*/
ExpValue MemFromSTACK(int offset, enum ExpType et)
{
    ExpValue ret;
    switch (et)
    {
    case _numint:
        ret.type = _expint;
        ret.u.i = *((int *)(STACK + offset));
        break;
    case _numfloat:
        ret.type = _expfloat;
        ret.u.f = *((float *)(STACK + offset));
        break;
    case _character:
        ret.type = _expchar;
        ret.u.c = *((char *)(STACK + offset));
        break;
    default:
        ERROR("ERROR: The type of mem var must be number or char\n");
        break;
    }
    return ret;
}

void safe_flush(FILE *fp) // 定义一个清除缓存的函数
{
    int ch;
    while ((ch = fgetc(fp)) != EOF && ch != '\n')
        ;
}

/*
根据 exptype 读取用户输入
*/
ExpValue Scan(enum ExpType et)
{
    static int i = 1;
    ExpValue ret;

    switch (et)
    {
    case _numint:
        ret.type = _expint;
        scanf("%d", &ret.u.i);
        break;
    case _numfloat:
        ret.type = _expfloat;
        scanf("%f", &ret.u.f);
        break;
    case _character:
        ret.type = _expchar;
        scanf("%c", &ret.u.c);
        break;
    case _string:
        ret.type = _expstring;
        int scanfRet = scanf("%s", ret.u.str);
        if (scanfRet == -1)
            ret.u.str[0] = 0;
        break;
    default:
        ERROR("ERROR: The type of scan var must be number or char\n");
        break;
    }

    return ret;
}

ExpValue ExecExp(irExp exp)
{
    ExpValue ret;
    if (exp->kind == _irop)
    {
        ExpValue left = ExecExp(exp->u.op->left), right = ExecExp(exp->u.op->right);
        ret = OpForExpValue(exp->u.op->op, left, right);
    }
    else if (exp->kind == _irmem)
    {
        ExpValue offset = ExecExp(exp->u.mem->offset);
        if (offset.type != _expint) // 取地址的 offset 表达式必须是 int
        {
            ERROR("ERROR: The type of offset in mem must be int\n");
        }
        ret = MemFromSTACK(offset.u.i, exp->valType);
    }
    else if (exp->kind == _irconsti)
    {
        ret.type = _expint;
        ret.u.i = exp->u.const_i->n;
    }
    else if (exp->kind == _irconstf)
    {
        ret.type = _expfloat;
        ret.u.f = exp->u.const_f->n;
    }
    else if (exp->kind == _irconst_c)
    {
        ret.type = _expchar;
        ret.u.c = exp->u.const_c->c;
    }
    else if (exp->kind == _irconst_s)
    {
        ret.type = _expstring;
        strcpy(ret.u.str, exp->u.const_s->str);
    }
    else if (exp->kind == _irscan)
    {
        ret = Scan(exp->valType);
    }
    else if (exp->kind == _ireseq)
    {
        ret = ExecStm(exp->u.eseq->stm);
        ExecStm(exp->u.eseq->exp->u.eseq->stm); // 恢复 sp
    }
    else
    {
        ERROR("ERROR: The type of exec exp is wrong\n");
    }

    return ret;
}

void PrintfEv(ExpValue ev, int width)
{
    char format[64] = "%";
    switch (ev.type)
    {
    case _expint:
        sprintf(format + 1, "%dd", width);
        printf(format, ev.u.i);
        break;
    case _expfloat:
        sprintf(format + 1, ".%df", width);
        printf(format, ev.u.f);
        break;
    default:
        ERROR("ERROR: can only printf number\n");
        break;
    }
}

void PrintEv(ExpValue ev)
{
    switch (ev.type)
    {
    case _expint:
        printf("%d", ev.u.i);
        break;
    case _expfloat:
        printf("%f", ev.u.f);
        break;
    case _expchar:
        printf("%c", ev.u.c);
        break;
    case _expstring:
        printf("%s", ev.u.str);
        break;
    case _expboolean:
        if (ev.u.boolean)
            printf("True");
        else
            printf("False");
        break;
    default:
        ERROR("ERROR: Ev type in PrintEv is wrong\n");
        break;
    }
}

/*
从 stm 节点开始，回溯父节点，当发现自己是父节点的左节点时停止，并返回要跳转到的父节点的右节点。
可以理解为该函数的作用是游走到下一个要执行的节点，即树的左序优先遍历。
*/
irStm StmToNext(irStm stm)
{
    if (stm == NULL || stm->fa == NULL)
    {
        return NULL;
        // ERROR("ERROR: StmToNext\n");
    }

    while (stm->fa->u.seq->left != stm) // 当发现自己是父节点的左节点时停止
    {
        stm = stm->fa;
        if (stm->fa == NULL)
            ERROR("ERROR: Can not find the next stm\n");
    }
    return stm->fa->u.seq->right;
}

/*
顶层的执行 stm 的函数。
采用在 IR 树上游走的方式进行（在 IR 生成时维护了 stm 的父节点）。
在一开始执行的时候，以及对每个函数调用的 ESEQ 中的 stm 部分会调用该函数；
对于后者，返回值为函数调用的返回结果；否则为 _expnull。
*/
ExpValue ExecStm(irStm stm)
{
    if (stm == NULL)
        ERROR("ERROR: The stm can not be null\n");

    ExpValue ret;
    ExpValue opRes;

    irStm nowStm = stm; // 当前正在执行的 irStm，即游走的指针
    while (1)
    {
        if (!nowStm)
        {
            ret.type = _expnull;
            return ret;
        }
        switch (nowStm->kind)
        {
        case _irfinish: // 执行结束
            ret.type = _expnull;
            return ret;
        case _irseq: // 指针切换到左节点继续执行
            nowStm = nowStm->u.seq->left;
            break;
        case _irlabel:
            nowStm = StmToNext(nowStm);
            break;
        case _irmove:
            SaveExpValueInMem(ExecExp(nowStm->u.move->src), nowStm->u.move->dest); // 内存保存操作
            nowStm = StmToNext(nowStm);
            if (!nowStm)
            {
                ret.type = _expnull;
                return ret;
            }
            break;
        case _irjump:
            /* 先直接默认跳第一个，后面有需求再加 */
            nowStm = JumpToLabel(nowStm->u.jump->labels->label); // 跳到对应 label 的父亲 seq 节点的右节点执行
            break;
        case _ircjump:
            opRes = ExecExp(nowStm->u.cjump->op);
            if (opRes.type != _expboolean)
            {
                ERROR("ERROR: The type of op in cjump must be boolean\n");
            }

            // 根据 op 的结果跳转到各自 label：
            if (opRes.u.boolean)
            {
                nowStm = JumpToLabel(nowStm->u.cjump->trueL);
            }
            else
            {
                nowStm = JumpToLabel(nowStm->u.cjump->falseL);
            }
            break;
        case _irexp:
            ExecExp(nowStm->u.exp);
            nowStm = StmToNext(nowStm);
            break;
        case _irprint:
            if (nowStm->u.print->type == _line)
                printf("\n");
            else if (nowStm->u.print->type == _printformat)
            {
                ExpValue printEv = ExecExp(nowStm->u.print->u.printf.exp);
                PrintfEv(printEv, nowStm->u.print->u.printf.width);
            }
            else if (nowStm->u.print->type == _printnormal)
            {
                ExpList el = nowStm->u.print->u.print.expList;
                while (el)
                {
                    PrintEv(ExecExp(el->exp));
                    el = el->tail;
                }
            }
            nowStm = StmToNext(nowStm);
            break;
        case _irret:
            return ExecExp(nowStm->u.ret->exp);
        default:
            ERROR("ERROR: The type of exec stm is wrong\n");
            break;
        }
    }
}

void Interpret(irStm startStm, irStm root)
{
    STACK = calloc(1, STACK_SIZE); // 分配总体栈空间

    FindLabelFromStm(root);

    ExecStm(startStm);
    // printf("Interpret end\n");

    exit(0);
}