#include "printir.h"

void printIrOp(irOp op)
{
    printf("OP(");
    switch (op->op)
    {
    case _irplus:
        printf("+,");
        break;

    case _irminus:
        printf("-,");
        break;

    case _irmul:
        printf("*,");
        break;

    case _irdiv:
        printf("/,");
        break;

    case _ireq:
        printf("==,");
        break;

    case _irne:
        printf("!=,");
        break;

    case _irlt:
        printf("<,");
        break;

    case _irle:
        printf("<=,");
        break;

    case _irgt:
        printf(">,");
        break;

    case _irge:
        printf(">=,");
        break;

    case _true:
        printf("true,");
        break;

    case _false:
        printf("false,");
        break;

    default:
        break;
    }

    printIrExp(op->left);
    printf(",");
    printIrExp(op->right);
    printf(")");
}

void printIrMem(irMem mem)
{
    printf("MEM(");
    printIrExp(mem->offset);
    printf(")");
}

void printIrConsti(irConsti const_i)
{
    printf("CONST_I(%d)", const_i->n);
}

void printIrConstf(irConstf const_f)
{
    printf("CONST_F(%f)", const_f->n);
}

void printIrConstC(irConst_C const_c)
{
    printf("CONST_C(%c)", const_c->c);
}

void printIrConstS(irConst_S const_s)
{
    printf("CONST_S(%s)", const_s->str);
}

void printIrEseq(irEseq eseq)
{
    printf("ESEQ(");
    printIrStm(eseq->stm);
    printf(",");
    printIrExp(eseq->exp);
    printf(")");
}

void printIrScan(irScan scan)
{
    printf("SCAN()");
}

void printIrSeq(irSeq seq)
{
    printf("SEQ(");
    printIrStm(seq->left);
    printf(",");
    printIrStm(seq->right);
    printf(")");
}

void printIrLabel(irLabel label)
{
    printf("LABEL(%d)", label->label);
}

void printIrMove(irMove move)
{
    printf("MOVE(");
    printIrExp(move->dest);
    printf(",");
    printIrExp(move->src);
    printf(")");
}

void printIrLabelList(LabelList list)
{
    printf("LABELLIST[");
    while (list)
    {
        printf("%d", list->label);
        if (list->tail)
        {
            printf(",");
            list = list->tail;
        }
        else
            break;
    }
    printf("]");
}

void printIrJump(irJump jump)
{
    printf("JUMP(");
    printIrExp(jump->idx);
    printf(",");
    printIrLabelList(jump->labels);
    printf(")");
}

void printIrCjump(irCjump cjump)
{
    printf("CJUMP(");
    printIrExp(cjump->op);
    printf(",%d, %d)", cjump->trueL, cjump->falseL);
}

void printExpList(ExpList el)
{
    printf("ExpList[");
    while (el)
    {
        printIrExp(el->exp);
        if (el->tail)
        {
            printf(", ");
            el = el->tail;
        }
        else
            break;
    }
    printf("]");
}

void printIrPrint(irPrint print)
{
    switch (print->type)
    {
    case _line:
        printf("endl");
        break;
    case _printnormal:
        printf("PRINT(");
        printExpList(print->u.print.expList);
        printf(")");
        break;
    case _printformat:
        printf("PRINTF(");
        printf("%d: ", print->u.printf.width);
        printIrExp(print->u.printf.exp);
        printf(")");
        break;
    default:
        break;
    }
}

void printIrExp(irExp exp)
{
    if (exp == NULL)
        return;
    switch (exp->kind)
    {
    case _irop:
        printIrOp(exp->u.op);
        break;
    case _irmem:
        printIrMem(exp->u.mem);
        break;
    case _irconsti:
        printIrConsti(exp->u.const_i);
        break;
    case _irconstf:
        printIrConstf(exp->u.const_f);
        break;
    case _irconst_c:
        printIrConstC(exp->u.const_c);
        break;
    case _irconst_s:
        printIrConstS(exp->u.const_s);
        break;
    case _ireseq:
        printIrEseq(exp->u.eseq);
        break;
    case _irscan:
        printIrScan(exp->u.scan);
        break;
    default:
        break;
    }
}

void printIrRet(irRet ret)
{
    printf("RET(");
    printIrExp(ret->exp);
    printf(")");
}

void printIrStm(irStm stm)
{
    if (!stm)
    {
        printf("NULL");
        return;
    }

    switch (stm->kind)
    {
    case _irret:
        printIrRet(stm->u.ret);
        break;
    case _irseq:
        printIrSeq(stm->u.seq);
        break;
    case _irlabel:
        printIrLabel(stm->u.label);
        break;
    case _irmove:
        printIrMove(stm->u.move);
        break;
    case _irjump:
        printIrJump(stm->u.jump);
        break;
    case _ircjump:
        printIrCjump(stm->u.cjump);
        break;
    case _irexp:
        printIrExp(stm->u.exp);
        break;
    case _irprint:
        printIrPrint(stm->u.print);
        break;
    case _irfinish:
        printf("FINISH");
        break;
    default:
        break;
    }
}