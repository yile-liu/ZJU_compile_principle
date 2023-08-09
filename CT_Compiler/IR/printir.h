#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ir.h"

void printIrOp(irOp op);

void printIrMem(irMem mem);

void printIrConsti(irConsti const_i);

void printIrConstf(irConstf const_f);

void printIrConstC(irConst_C const_c);

void printIrConstS(irConst_S const_s);

void printIrEseq(irEseq eseq);

void printIrScan(irScan scan);

void printIrSeq(irSeq seq);

void printIrLabel(irLabel label);

void printIrMove(irMove move);

void printIrLabelList(LabelList list);

void printIrJump(irJump jump);

void printIrCjump(irCjump cjump);

void printIrExp(irExp exp);

void printIrStm(irStm stm);