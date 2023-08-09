#include "ast.h"
#include "astNode.h"
#include "../debug.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../IR/ir.h"

void printParamcall(PARAMCALL p)
{
    printf("PARAMCALL(");
    printExp(p->exp);
    if (p->next)
    {
        printf(", ");
        printParamcall(p->next);
    }
    printf(")");
}

void printFuncall(FUNCALL f)
{
    printf("FUNCALL(");
    printId(f->id);
    printf(", ");
    printParamcall(f->params);
    printf(")");
}

void printParam(PARAM p)
{
    printf("PARAMS[");
    while (p)
    {
        printType(p->varType);
        printf(": ");
        printId(p->id);
        if (p->next)
        {
            printf(", ");
            p = p->next;
        }
        else
        {
            break;
        }
    }
    printf("]");
}

void printFunc(FUNC f)
{
    printf("FUNC(");
    printType(f->funcType);
    printf(": ");
    printId(f->funcName);
    printf(", ");
    printParam(f->params);
    printf(", ");
    printStm(f->funcStm);
    printf(")");
}

void printType(TYPE t)
{
    switch (t->typeKind)
    {
    case _int:
        printf("int");
        break;
    case _float:
        printf("float");
        break;
    case _char:
        printf("char");
        break;
    default:
        printf("Type ERROR");
        break;
    }
}

void printFNum(FNUM n)
{
    printf("%f", n->fnum);
}

void printINum(INUM n)
{
    printf("%d", n->inum);
}

void printConstC(CONST_C c)
{
    printf("CHAR(%d)", c->c);
}

void printConstS(CONST_S s)
{
    printf("STR(%s)", s->s);
}

void printId(ID i)
{
    printf("ID(%s)", i->s);
}

void printOp(OP o)
{
    switch (o->opKind)
    {
    case _add:
        printf("+");
        break;
    case _minu:
        printf("-");
        break;
    case _times:
        printf("*");
        break;
    case _div:
        printf("//");
        break;
    case _eq:
        printf("==");
        break;
    case _ne:
        printf("!=");
        break;
    case _gt:
        printf(">");
        break;
    case _ge:
        printf(">=");
        break;
    case _lt:
        printf("<");
        break;
    case _le:
        printf("<=");
        break;
    default:
        printf("OP ERROR");
        break;
    }
}

void printAddr(ADDR a)
{
    printf("ADDR(");
    switch (a->addrKind)
    {
    case _id:
        printId(a->addr.id);
        break;
    case _array:
        printArray(a->addr.arr);
        break;
    default:
        printf("ADDR ERROR");
        break;
    }
    printf(")");
}

void printExp(EXP e)
{
    printf("EXP(");
    switch (e->expKind)
    {
    case _addr:
        printAddr(e->exp.addr);
        break;
    case _fnum:
        printFNum(e->exp.fnum);
        break;
    case _inum:
        printINum(e->exp.inum);
        break;
    case _const_c:
        printConstC(e->exp.c);
        break;
    case _const_s:
        printConstS(e->exp.s);
        break;
    case _op:
        printOp(e->exp.op);
        printf(", ");
        printExp(e->exp.op->lexp);
        printf(", ");
        printExp(e->exp.op->rexp);
        break;
    case _funcall:
        printFuncall(e->exp.funcall);
        break;
    default:
        printf("EXP ERROR");
        break;
    }
    printf(")");
}

void printMpTerm(MPTERM m)
{
    printf("[");
    printExp(m->exp);
    printf("]");
    if (m->next != NULL)
    {
        printMpTerm(m->next);
    }
}

void printArray(ARRAY a)
{
    printf("Array(");
    printId(a->id);
    printf(", ");
    printMpTerm(a->mpTerm);
    printf(")");
}

void printDef(DEF d)
{
    printf("DEF(");
    printType(d->type);
    printf(", ");
    printAddr(d->addr);
    printf(")");
}

void printRc(RC r)
{
    printf("RC(");
    printExp(r->exp);
    if (r->next != NULL)
    {
        printf(", ");
        printRc(r->next);
    }
    printf(")");
}

void printRead(READ r)
{
    printf("READ(");
    printRc(r->rc);
    printf(")");
}

void printWc(WC w)
{
    printf("WC(");
    printExp(w->exp);
    if (w->next != NULL)
    {
        printf(", ");
        printWc(w->next);
    }
    printf(")");
}

void printWrite(WRITE w)
{
    printf("WRITE(");
    switch (w->writeKind)
    {
    case _print:
        printWc(w->wc);
        break;
    case _printf:
        printINum(w->num);
        printf(" : ");
        printExp(w->exp);
        break;
    case _println:
        printf("\\n");
        break;
    default:
        printf("WRITE ERROR");
        break;
    }
    printf(")");
}

void printAssign(ASSIGN a)
{
    printf("ASSIGN(");
    printExp(a->addr);
    printf(", ");
    printExp(a->exp);
    printf(")");
}

void printRet(EXP r)
{
    printf("RET(");
    printExp(r);
    printf(")");
}

void printStm(STM s)
{
    printf("STM(");
    switch (s->stmKind)
    {
    case _def:
        printDef(s->stm.def);
        break;
    case _read:
        printRead(s->stm.read);
        break;
    case _write:
        printWrite(s->stm.write);
        break;
    case _assign:
        printAssign(s->stm.assign);
        break;
    case _branch:
        printBranch(s->stm.branch);
        break;
    case _loop:
        printLoop(s->stm.loop);
        break;
    case _jump:
        printJump(s->stm.jump);
        break;
    case _func:
        printFunc(s->stm.func);
        break;
    case _ret:
        printRet(s->stm.ret);
        break;
    default:
        printf("STM ERROR");
        break;
    }
    if (s->next)
    {
        printf(", ");
        printStm(s->next);
    }
    printf(")");
}

void printBranch(BRANCH b)
{
    printf("BRANCH(");
    printExp(b->condition);
    printf(", ");
    printStm(b->stm);
    printf(")");
}

void printLoop(LOOP l)
{
    printf("LOOP(");
    printExp(l->condition);
    printf(", ");
    printStm(l->stm);
    printf(")");
}

void printJump(JUMP j)
{
    switch (j->jumpKind)
    {
    case _continue:
        printf("continue");
        break;
    case _break:
        printf("break");
        break;

    default:
        break;
    }
}

/*
这个函数主要的作用是“搭桥”，将运行代码引导到 IR 那边
*/
void translateToIR(STM root)
{
#ifdef SHOW_AST
    printStm(root);
    printf("\nTranslate to IR...\n");
#endif
    IRTranslation(root);
}