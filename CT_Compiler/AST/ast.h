#ifndef AST_AST_H
#define AST_AST_H

#include "astNode.h"

/*
分工：
1. stm、jump、op、id、num、type......................lsr
2. def、assign、branch、addr、const_c、const_s、exp...wsx
3. read、write、rc、wc、loop、array、mpterm...........czl
*/

void printType(TYPE t);
void printINum(INUM n);
void printFNum(FNUM n);
void printConstC(CONST_C c);
void printConstS(CONST_S s);
void printId(ID i);
void printOp(OP o);
void printAddr(ADDR a);
void printExp(EXP e);
void printMpTerm(MPTERM m);
void printArray(ARRAY a);
void printDef(DEF d);
void printRc(RC r);
void printRead(READ r);
void printWc(WC w);
void printWrite(WRITE w);
void printAssign(ASSIGN a);
void printStm(STM s);
void printBranch(BRANCH b);
void printLoop(LOOP l);
void printJump(JUMP j);
void printParamcall(PARAMCALL p);
void printFuncall(FUNCALL f);
void printParam(PARAM p);
void printFunc(FUNC f);
void printRet(EXP r);

extern void translateToIR(STM root);

#endif // AST_AST_H
