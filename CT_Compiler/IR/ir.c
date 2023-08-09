#include "ir.h"
#include "../Interpreter/interpreter.h"
#include "printir.h"
#include "../debug.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// 主函数，相当于所有代码都在主函数中
FuncBucket mainFunc;
// 开始执行的第一个 stm，前面是函数定义
irStm firstStm = NULL;

irStm jumpPatch = NULL, cjumpPatch = NULL, breakJumpPatch[100];
int jumpFlag = 0, cjumpFlag = 0, breakTop = 0, whileNum = 0, breakNum[100] = {0}, breakFlag = 0;

int backLabelStack[100], top = 0;

/*
若类型错误，返回 1；否则 0
*/
int checkTypeError(irExp exp, enum ExpType et)
{
    return (exp->valType != et);
}

enum IrOpKind OK_IOK(enum opkind ok)
{
    switch (ok)
    {
    case _add:
        return _irplus;
    case _minu:
        return _irminus;
    case _times:
        return _irmul;
    case _div:
        return _irdiv;
    case _eq:
        return _ireq;
    case _ne:
        return _irne;
    case _gt:
        return _irlt;
    case _ge:
        return _irle;
    case _lt:
        return _irgt;
    case _le:
        return _irge;
    default:
        break;
    }
}

enum ExpType VT_ET(enum VarType vt)
{
    switch (vt)
    {
    case _symint:
    case _symint_arr:
        return _numint;
    case _symfloat:
    case _symfloat_arr:
        return _numfloat;
    case _symchar:
    case _symchar_arr:
        return _character;
    case _symnull:
        return _null;
    default:
        ERROR("ERROR: Can't translate to ExpType\n");
        return _null;
    }
}

irStm newStm(enum irStmKind kind, irStm fa)
{
    irStm node = (irStm)calloc(1, sizeof(struct irStm_));
    switch (kind)
    {
    case _irseq:
        node->u.seq = (irSeq)calloc(1, sizeof(struct irSEQ_));
        break;
    case _irlabel:
        node->u.label = (irLabel)calloc(1, sizeof(struct irLabel_));
        break;
    case _irmove:
        node->u.move = (irMove)calloc(1, sizeof(struct irMove_));
        break;
    case _irjump:
        node->u.jump = (irJump)calloc(1, sizeof(struct irJump_));
        break;
    case _ircjump:
        node->u.cjump = (irCjump)calloc(1, sizeof(struct irCjump_));
        break;
    case _irexp:
        node->u.exp = (irExp)calloc(1, sizeof(struct irExp_));
        break;
    case _irprint:
        node->u.print = (irPrint)calloc(1, sizeof(struct irPrint_));
        break;
    case _irret:
        node->u.ret = (irRet)calloc(1, sizeof(struct irRet_));
    default:
        break;
    }
    node->kind = kind;
    node->fa = fa;
    return node;
}

irExp newExp(enum irExpKind kind, enum ExpType valtype)
{
    irExp node = (irExp)calloc(1, sizeof(struct irExp_));

    switch (kind)
    {
    case _irop:
        node->u.op = (irOp)calloc(1, sizeof(struct irOp_));
        break;
    case _irmem:
        node->u.mem = (irMem)calloc(1, sizeof(struct irMem_));
        break;
    case _irconstf:
        node->u.const_f = (irConstf)calloc(1, sizeof(struct irConstf_));
        break;
    case _irconsti:
        node->u.const_i = (irConsti)calloc(1, sizeof(struct irConsti_));
        break;
    case _irconst_c:
        node->u.const_c = (irConst_C)calloc(1, sizeof(struct irConst_C_));
        break;
    case _irconst_s:
        node->u.const_s = (irConst_S)calloc(1, sizeof(struct irConst_S_));
        break;
    case _ireseq:
        node->u.eseq = (irEseq)calloc(1, sizeof(struct irEseq_));
        break;
    case _irscan:
        node->u.scan = (irScan)calloc(1, sizeof(struct irScan_));
        break;

    default:
        break;
    }

    node->kind = kind;
    node->valType = valtype;
    return node;
}

/*
创建一下取 sp 内存的节点
*/
irExp newMemToSP()
{
    irExp mem = newExp(_irmem, _numint);
    mem->u.mem->offset = newExp(_irconsti, _numint);
    mem->u.mem->offset->u.const_i->n = SP_OFFSET;
    return mem;
}

/*
LabelList 构造函数
*/
LabelList newLabelList(int label, LabelList tail)
{
    LabelList ret = (LabelList)calloc(1, sizeof(struct LabelList_));
    ret->label = label;
    ret->tail = tail;
    return ret;
}

/*
ExpList 构造函数
*/
ExpList newExpList(irExp exp, ExpList tail)
{
    ExpList ret = (ExpList)calloc(1, sizeof(struct ExpList_));
    ret->exp = exp;
    ret->tail = tail;
    return ret;
}

int expTypeIsNumber(enum ExpType t)
{
    if (t == _numint || t == _numfloat || _character)
    {
        return 1;
    }
    return 0;
}

/*
根据 expType 获取数组元素大小
*/
int getVarSize(enum ExpType vt)
{
    if (vt == _numint || vt == _numfloat)
        return 4;
    else if (vt == _character)
        return 1;
    ERROR("ERROR: The type of element in Array must be number or char\n");
    return 0;
}

/*
中括号项生成的表达式将插到 root 的右边
*/
void MpTerm_IrExp(MPTERM term, irExp root, int dim, int *arrSize, int size, FuncBucket func)
{
    irExp node = root;
    int i = 0;
    if (!term)
        ERROR("ERROR: The mpterm is null\n");
    while (1)
    {
        if (term->next == NULL)
        {
            node->u.op->right = Exp_IrExp(term->exp, func);
            if (checkTypeError(node->u.op->right, _numint))
            { // 对中括号中的表达式进行类型检查
                ERROR("ERROR: The exp type in [] must be number\n");
            }
            break;
        }
        irExp right = newExp(_irop, _numint);
        right->u.op->op = _irplus;
        right->u.op->left = newExp(_irop, _numint);
        right->u.op->left->u.op->op = _irmul;
        right->u.op->left->u.op->left = Exp_IrExp(term->exp, func);
        right->u.op->left->u.op->right = newExp(_irconsti, _numint);
        size /= arrSize[i++];
        right->u.op->left->u.op->right->u.const_i->n = size;
        if (checkTypeError(right->u.op->left->u.op->left, _numint))
        { // 对中括号中的表达式进行类型检查
            ERROR("ERROR: The exp type in [] must be number\n");
        }
        node->u.op->right = right;
        node = right;
        term = term->next;
    }
}

irExp Exp_IrExp(EXP exp, FuncBucket func)
{
    irExp irexp;

    if (exp->expKind == _fnum)
    {
        irexp = newExp(_irconstf, _numfloat);
        irexp->u.const_f->n = exp->exp.fnum->fnum;
    }
    else if (exp->expKind == _inum)
    {
        irexp = newExp(_irconsti, _numint);
        irexp->u.const_i->n = exp->exp.inum->inum;
    }
    else if (exp->expKind == _const_c)
    {
        irexp = newExp(_irconst_c, _character);
        irexp->u.const_c->c = exp->exp.c->c;
    }
    else if (exp->expKind == _const_s)
    {
        irexp = newExp(_irconst_s, _string);
        strcpy(irexp->u.const_s->str, exp->exp.s->s);
    }
    else if (exp->expKind == _addr)
    {
        if (exp->exp.addr->addrKind == _id)
        {                                         // 变量
            char *id = exp->exp.addr->addr.id->s; // 变量名

            enum VarType vt = lookUp(func->binding->symbolTable, id);
            enum ExpType et;
            if (vt == _symchar_arr)
            {
                et = _string;
            }
            else
            {
                et = VT_ET(vt);
            } 
            // 先在本地函数环境找该变量类型
            // 如果在本地找不到，且当前非 main 函数，则在全局找
            if (et == _null && func != mainFunc)
            {
                vt = lookUp(func->binding->symbolTable, id);
                if (vt == _symchar_arr)
                {
                    et = _string;
                }
                else
                    et = VT_ET(vt);
                if (et == _null)
                { // 在全局也找不到，报错
                    ERROR("ERROR: Not found id\n");
                }
                irexp = newExp(_irmem, et);
                irexp->u.mem->offset = newExp(_irconsti, _numint);
                // 若未得到偏移值，报错
                if ((irexp->u.mem->offset->u.const_i->n = getOffset(mainFunc->binding->symbolTable, id)) == -1)
                {
                    ERROR("ERROR: Not get id offset\n");
                }
            }
            else if (et != _null)
            { // 在本地找到了，则需要加上 sp，注意就算这里的本地是 main 也没关系，因为这时 sp=0
                irexp = newExp(_irmem, et);
                irexp->u.mem->offset = newExp(_irop, _numint); // sp + offset
                irexp->u.mem->offset->u.op->op = _irplus;
                irExp left = newMemToSP(), right = newExp(_irconsti, _numint);
                // 取偏移量：
                if ((right->u.const_i->n = getOffset(func->binding->symbolTable, id)) == -1)
                {
                    ERROR("ERROR: Not get id offset\n");
                }
                irexp->u.mem->offset->u.op->left = left;
                irexp->u.mem->offset->u.op->right = right;
            }
            else
            {
                ERROR("ERROR: Not found id1\n");
            }
        }
        else
        { // 数组

            ARRAY arr = exp->exp.addr->addr.arr;
            char *id = arr->id->s; // 数组名
            
            // 先在本地函数找数组
            enum ExpType et = VT_ET(lookUp(func->binding->symbolTable, id));

            irexp = newExp(_irmem, et);
            // 如果在本地找不到，就去全局找
            if (irexp->valType == _null && func != mainFunc)
            {
                irexp->valType = VT_ET(lookUp(mainFunc->binding->symbolTable, id));
                if (irexp->valType == _null)
                { // 在全局也找不到，报错
                    ERROR("ERROR: Not found array\n");
                }
                irexp->u.mem->offset = newExp(_irop, _numint); // 全局数组，数组基地址 + 中括号偏移量 * 元素大小
                irexp->u.mem->offset->u.op->op = _irplus;
                irExp left = newExp(_irconsti, _numint);
                int arrBaseAddr = getOffset(mainFunc->binding->symbolTable, id); // 数组基地址
                left->u.const_i->n = arrBaseAddr;
                irexp->u.mem->offset->u.op->left = left;

                irExp opMul = newExp(_irop, _numint); // 中括号偏移量 * 元素大小
                irexp->u.mem->offset->u.op->right = opMul;
                opMul->u.op->op = _irmul;
                opMul->u.op->left = newExp(_irconsti, _numint);
                opMul->u.op->left->u.const_i->n = getVarSize(irexp->valType);

                // 遍历数组中括号项
                int dim = getArrDim(mainFunc->binding->symbolTable, id);       // 数组维度
                int *arrSize = getArrSize(mainFunc->binding->symbolTable, id); // 数组每一维度大小
                if (!arrSize || !dim)
                    ERROR("ERROR: Can not find the arrSize and dim\n");
                MpTerm_IrExp(arr->mpTerm, opMul, dim, arrSize, getSize(mainFunc->binding->symbolTable, id), func);
            }
            else if (irexp->valType != _null)
            {                                                  // 在本地找到了，则需要加上 sp，注意就算这里的本地是 main 也没关系，因为这时 sp=0
                irexp->u.mem->offset = newExp(_irop, _numint); // 局部数组，sp + 数组基地址 + 中括号偏移量 * 元素大小
                irexp->u.mem->offset->u.op->op = _irplus;
                irexp->u.mem->offset->u.op->left = newMemToSP();

                irExp right = newExp(_irop, _numint); // 数组基地址 + 中括号偏移量 * 元素大小
                right->u.op->op = _irplus;
                irexp->u.mem->offset->u.op->right = right;

                irExp left1 = newExp(_irconsti, _numint);
                int arrBaseAddr = getOffset(func->binding->symbolTable, id); // 数组基地址
                left1->u.const_i->n = arrBaseAddr;
                right->u.op->left = left1;

                irExp opMul = newExp(_irop, _numint); // 中括号偏移量 * 元素大小
                right->u.op->right = opMul;
                opMul->u.op->op = _irmul;
                opMul->u.op->left = newExp(_irconsti, _numint);
                opMul->u.op->left->u.const_i->n = getVarSize(irexp->valType);

                // 遍历数组中括号项
                int dim = getArrDim(func->binding->symbolTable, id);       // 数组维度
                int *arrSize = getArrSize(func->binding->symbolTable, id); // 数组每一维度大小
                if (!arrSize || !dim)
                    ERROR("ERROR: Can not find the arrSize and dim\n");
                MpTerm_IrExp(arr->mpTerm, opMul, dim, arrSize, getSize(func->binding->symbolTable, id), func);
            }
        }
    }
    else if (exp->expKind == _op)
    {
        irExp left = Exp_IrExp(exp->exp.op->lexp, func), right = Exp_IrExp(exp->exp.op->rexp, func);

        if (!(expTypeIsNumber(left->valType) && expTypeIsNumber(right->valType)) && (left->valType != right->valType))
        { // op 运算符的两个操作表达式必须类型相同或者都是数字（数字包括整数、浮点数、字符）
            printf("%d %d\n", left->valType, right->valType);
            ERROR("The exptype of exps of Op must be the same\n");
        }
        switch (exp->exp.op->opKind)
        { // 运算符类型检查
        case _add:
        case _minu:
        case _times:
        case _div:
            if (!checkTypeError(left, _numint) && !checkTypeError(right, _numint)) // op 左右都是整数
            {
                irexp = newExp(_irop, _numint);
            }
            else if ((!checkTypeError(left, _numint) || !checkTypeError(left, _numfloat)) && (!checkTypeError(right, _numint) || !checkTypeError(right, _numfloat))) // op 两边都是整数或浮点数
            {
                irexp = newExp(_irop, _numfloat);
            }
            else if (expTypeIsNumber(left->valType) && expTypeIsNumber(right->valType)) // 存在字符，返回为整数
            {
                irexp = newExp(_irop, _numint);
            }
            else
            {
                ERROR("ERROR: The exptype of exps of Op(+-*/) must be the number\n");
            }
            break;
        case _eq:
        case _ne:
        case _gt:
        case _ge:
        case _lt:
        case _le:
            if ((checkTypeError(left, _numint) && checkTypeError(left, _numfloat) && checkTypeError(left, _character)) || (checkTypeError(right, _numint) && checkTypeError(right, _numfloat) && checkTypeError(right, _character)))
            {
                ERROR("The exptype of exps of Op(compare) must be the number or character or character\n");
            }
            irexp = newExp(_irop, _boolean);
            break;
        default:
            ERROR("ERROR: The kind of opExp is wrong\n");
            break;
        }

        switch (exp->exp.op->opKind)
        { // 运算符类型
        case _add:
            irexp->u.op->op = _irplus;
            break;
        case _minu:
            irexp->u.op->op = _irminus;
            break;
        case _times:
            irexp->u.op->op = _irmul;
            break;
        case _div:
            irexp->u.op->op = _irdiv;
            break;
        case _eq:
            irexp->u.op->op = _ireq;
            break;
        case _ne:
            irexp->u.op->op = _irne;
            break;
        case _gt:
            irexp->u.op->op = _irgt;
            break;
        case _ge:
            irexp->u.op->op = _irge;
            break;
        case _lt:
            irexp->u.op->op = _irlt;
            break;
        case _le:
            irexp->u.op->op = _irle;
            break;
        default:
            ERROR("ERROR: The kind of opExp is wrong\n");
            break;
        }
        irexp->u.op->left = left;
        irexp->u.op->right = right;
    }
    else if (exp->expKind == _funcall)
    {
        /*
        对于函数调用表达式，应该用 ESEQ 来完成，ESEQ 的 stm 部分首先完成函数参数的装载，然后调整 sp，
        以及跳转到函数代码部分，最后恢复 sp；ESEQ 的 exp 部分需要用 MEM 取出 ret 的值作为结果
        */
        char *funcName = exp->exp.funcall->id->s; // 函数名
        FuncBucket fb = lookUpFunc(funcName);     // 函数绑定

        irexp = newExp(_ireseq, VT_ET(fb->binding->retType)); // 函数调用转成的 irEseq 表达式节点的表达式类型取决于被调用函数的返回值类型

        /*
        首先装载函数参数
        */
        // 当前函数环境中最新定义的变量的 offset + size 即是在调用新函数时 sp 要移动到的位置
        int spMove = func->binding->latestBucket->offset + func->binding->latestBucket->size * getVarSize(VT_ET(func->binding->latestBucket->type));

        irExp destSp = newMemToSP(), srcSp = newMemToSP();

        irExp opPlus = newExp(_irop, _numint); // srcSp + spMove，即新 sp 的值
        opPlus->u.op->op = _irplus;
        opPlus->u.op->left = srcSp;
        opPlus->u.op->right = newExp(_irconsti, _numint);
        opPlus->u.op->right->u.const_i->n = spMove;

        irStm seq1 = newStm(_irseq, NULL); // 左边装载函数，右边继续
        irexp->u.eseq->stm = seq1;

        PARAMCALL pcalls = exp->exp.funcall->params; // 获取函数调用传入的参数列表
        irStm node = seq1;                           // 用于循环增加 move 参数的 stm 节点
        for (int i = 0; i < fb->binding->paramNum; i++)
        {
            char *pName = fb->binding->paramNames[i];                     // 函数参数名
            enum VarType pType = lookUp(fb->binding->symbolTable, pName); // 查找函数参数类型（函数参数相当于函数局部变量保存在函数的符号表中）
            EXP pcall = pcalls->exp;                                      // 获取参数
            irExp param = Exp_IrExp(pcall, func);                         // 将传入的 ast 参数表达式转成 ir 参数表达式
            pcalls = pcalls->next;
            if (checkTypeError(param, VT_ET(pType))) // 对参数进行类型检查
            {
                ERROR("ERROR: The type of param is wrong\n");
            }

            irStm moveParam = newStm(_irmove, NULL); // move(mem(mem(sp) + offset), param)
            moveParam->u.move->src = param;

            irExp pMem = newExp(_irmem, param->valType); // mem(mem(new sp) + offset)

            irExp plus = newExp(_irop, _numint); // mem(new sp) + offset
            plus->u.op->op = _irplus;
            plus->u.op->left = opPlus;

            int offset = getOffset(fb->binding->symbolTable, pName); // 函数参数在函数地址空间的偏移
            plus->u.op->right = newExp(_irconsti, _numint);
            plus->u.op->right->u.const_i->n = offset;

            pMem->u.mem->offset = plus;
            moveParam->u.move->dest = pMem;

            if (i == fb->binding->paramNum - 1) // 装载到最后一个参数
            {
                node->u.seq->left = moveParam;
                moveParam->fa = node;
            }
            else // 装载参数的 seq 的序列向左边增长，也就是每个 seq 的右边是move，左边是下一个 seq（装载参数的顺序不重要）
            {
                irStm seq = newStm(_irseq, node);
                seq->u.seq->right = moveParam;
                moveParam->fa = seq;
                node->u.seq->left = seq;
                node = seq;
            }
        }

        /*
        然后维护 sp
        */
        irStm seq2 = newStm(_irseq, seq1); // 左边维护 sp，右边 jump
        seq1->u.seq->right = seq2;

        irStm move = newStm(_irmove, seq2); // move（sp，sp + spMove）
        seq2->u.seq->left = move;
        move->u.move->dest = destSp;
        move->u.move->src = opPlus;

        /*
        jump 到函数代码部分
        */
        irStm jump = newStm(_irjump, seq2);
        seq2->u.seq->right = jump;
        jump->u.jump->idx = newExp(_irconsti, _numint);
        jump->u.jump->idx->u.const_i->n = 0; // 选择第一个 label 跳转
        jump->u.jump->labels = newLabelList(fb->binding->label, NULL);

        irStm move1 = newStm(_irmove, NULL); // move1（sp，sp - spMove）

        irExp destSp1 = newMemToSP(), srcSp1 = newMemToSP();

        irExp opMinus = newExp(_irop, _numint); // srcSp - spMove
        opMinus->u.op->op = _irminus;
        opMinus->u.op->left = srcSp1;
        opMinus->u.op->right = newExp(_irconsti, _numint);
        opMinus->u.op->right->u.const_i->n = spMove;

        move1->u.move->dest = destSp1;
        move1->u.move->src = opMinus;

        // 对于处理函数调用，第一个 eseq 的 exp 同样是 eseq，第二个 eseq 的左边是恢复 sp 的操作，右边是 ircall
        irexp->u.eseq->exp = newExp(_ireseq, irexp->valType);
        irexp->u.eseq->exp->u.eseq->stm = move1;
        // move1->fa =

        irexp->u.eseq->exp->u.eseq->exp = newExp(_ircall, VT_ET(fb->binding->retType));
    }

    return irexp;
}

/*
返回定义的变量、数组用于记录
*/
Bucket DEFTranslate(STM node, FuncBucket func)
{
    ADDR addr = node->stm.def->addr;
    TYPE type = node->stm.def->type;
    enum VarType vt = TK_VT(type->typeKind);
    if (addr->addrKind == _id)
    {
        return insert(func, addr->addr.id->s, vt, 1, NULL, 0);
    }
    else
    {
        vt = (vt == _symchar) ? _symchar_arr : vt;
        MPTERM term = addr->addr.arr->mpTerm;
        int size = 1; // 数组总元素数量
        int dim = 0;  // 数组维度
        while (term)
        {
            dim++;
            size *= term->exp->exp.inum->inum;
            term = term->next;
        }

        int *arrSize = (int *)calloc(dim, sizeof(int)); // 数组各个维度的 size
        int i;
        for (i = 0, term = addr->addr.arr->mpTerm; term; term = term->next, i++) // 重新遍历中括号项获取记录每个维度的大小
        {
            arrSize[i] = term->exp->exp.inum->inum;
        }
        return insert(func, addr->addr.arr->id->s, vt, size, arrSize, dim);
    }
}

irStm RETTranslate(STM node, FuncBucket func)
{
    irStm ret = newStm(_irret, NULL);
    ret->u.ret->exp = Exp_IrExp(node->stm.ret, func);

    enum ExpType retType = VT_ET(func->binding->retType);

    if (!(expTypeIsNumber(ret->u.ret->exp->valType) && expTypeIsNumber(retType)) && (ret->u.ret->exp->valType != retType))
    {
        ERROR("Return type do not match!");
    }

    return ret;
}

irStm ASSIGNTranslate(STM node, FuncBucket func)
{
    irExp src = Exp_IrExp(node->stm.assign->exp, func);
    irExp dst = Exp_IrExp(node->stm.assign->addr, func);

    if (!(expTypeIsNumber(src->valType) && expTypeIsNumber(dst->valType)) && (src->valType != dst->valType))
    {
        printf("%d %d\n", src->valType, dst->valType);
        ERROR("Assign type do not match\n");
    }

    irStm assign = newStm(_irmove, NULL);
    assign->u.move->src = src;
    assign->u.move->dest = dst;

    return assign;
}

FuncBucket funcBucketFromSTM(STM node)
{
    if (node->stmKind != _func)
    {
        ERROR("Cannot create FuncBucket from node with stmKind != _func");
    }

    // 1. funcName as key
    char *key = node->stm.func->funcName->s;

    // 2.1. get param number
    int paramNum = 0;
    int i;
    PARAM p;
    for (p = node->stm.func->params; p != NULL; p = p->next)
    {
        paramNum++;
    }
    // 2.2. paramNames as pts
    char **paramNames = (char **)calloc(1, sizeof(char *) * paramNum);
    for (i = 0, p = node->stm.func->params; p != NULL; i++, p = p->next)
    {
        paramNames[i] = (char *)calloc(1, sizeof(char) * strlen(p->id->s));
        strcpy(paramNames[i], p->id->s);
    }

    // 3. return type as retType
    RetType retType = TK_VT(node->stm.func->funcType->typeKind);

    // 4. insert global funcTable
    FuncBucket newBucket = insertFunc(key, paramNum, paramNames, retType, newLabel());

    // 5. insert params to the symbolTable of newBucket
    for (p = node->stm.func->params; p != NULL; p = p->next)
    {
        insert(newBucket, p->id->s, TK_VT(p->varType->typeKind), 1, NULL, 0);
    }

    return newBucket;
}

irStm FUNCTranslate(STM node, FuncBucket func)
{
    if (func != mainFunc)
    {
        ERROR("The definition of function must be in the main\n");
    }

    // 从AST节点中提取相关信息生成FuncBucket
    FuncBucket funcBuk = funcBucketFromSTM(node);

    // 建立函数定义节点
    irStm funcDef = newStm(_irseq, NULL);
    // 左子树是Label，后入将作为这个函数的入口
    funcDef->u.seq->left = newStm(_irlabel, funcDef);
    funcDef->u.seq->left->u.label->label = funcBuk->binding->label;
    // 右子树是函数体，通过递归调用StmTranslate()生成
    funcDef->u.seq->right = StmTranslate(node->stm.func->funcStm, funcBuk, 0, newStm(_irfinish, NULL));
    return funcDef;
}

/*
将 branch 或 loop 的 condition 转换成 cjump
主打一个代码复用
*/
irStm Condition_Cjump(EXP condition, FuncBucket func, int t)
{
    irStm cjump = newStm(_ircjump, NULL);
    cjumpPatch = cjump;
    cjumpFlag = 1;
    cjump->u.cjump->trueL = t;
    irExp op = Exp_IrExp(condition, func);
    if (!checkTypeError(op, _boolean)) // 若类型为 boolean，即正常情况，则正常处理
    {
        cjump->u.cjump->op = op;
    }
    else if (!checkTypeError(op, _numint) || !checkTypeError(op, _numfloat)) // 若类型为整数或浮点数，则转换成 condition ne 0
    {
        irExp newop = newExp(_irop, _boolean);
        newop->u.op->op = _irne;
        newop->u.op->right = newExp(_irconsti, _numint);
        newop->u.op->right->u.const_i->n = 0;
        newop->u.op->left = op;
        cjump->u.cjump->op = newop;
    }
    else // 类型检查
    {
        ERROR("ERROR: The type of condition in branch must be boolean or number\n");
    }
    return cjump;
}

irStm BRANCHTranslate(STM node, FuncBucket func)
{
    EXP condition = node->stm.branch->condition;
    STM stm = node->stm.branch->stm;

    irStm irRoot = newStm(_irseq, NULL); // 根节点，左边是 cjump，右边是 seq1

    irStm seq1 = newStm(_irseq, irRoot); // 左边是 truelabel，右边是执行主体
    irRoot->u.seq->right = seq1;

    irStm trueLabel = newStm(_irlabel, seq1);
    trueLabel->u.label->label = newLabel();
    seq1->u.seq->left = trueLabel;

    irStm jump = newStm(_irjump, NULL);
    jump->u.jump->idx = newExp(_irconsti, _numint);
    jump->u.jump->idx->u.const_i->n = 0;

    seq1->u.seq->right = StmTranslate(stm, func, 1, jump); // 将 AST branch 的 stm 部分转成 seq3 的 IR 执行部分，while 中的语句执行完后，需要 pop 所有局部变量

    // 条件分支判断跳转 cjump：
    irRoot->u.seq->left = Condition_Cjump(condition, func, trueLabel->u.label->label);
    irRoot->u.seq->left->fa = irRoot;

    if (jumpFlag)
        ERROR("ERROR: jumpflag should be 0\n");
    jumpPatch = jump; // jump 的 labels 部分在之后 doPatch 添加
    jumpFlag = 1;

    return irRoot;
}

irStm LOOPTranslate(STM node, FuncBucket func)
{
    irStm root = newStm(_irseq, NULL); // 左边是 backlabel，右边是 seq1

    irStm backLabel = newStm(_irlabel, root);
    backLabel->u.label->label = newLabel();
    root->u.seq->left = backLabel;
    // update
    backLabelStack[top++] = backLabel->u.label->label;

    irStm seq1 = newStm(_irseq, root); // 在 root 的右边；左边是 cjump，右边是 seq2
    root->u.seq->right = seq1;

    irStm seq2 = newStm(_irseq, seq1); // 在 seq1 的右边；左边是 truelabel，右边是 seq3
    seq1->u.seq->right = seq2;

    irStm trueLabel = newStm(_irlabel, seq2);
    trueLabel->u.label->label = newLabel();
    seq2->u.seq->left = trueLabel;

    irStm jump = newStm(_irjump, NULL);
    irExp idx = newExp(_irconsti, _numint);
    jump->u.jump->idx = idx;
    idx->u.const_i->n = 0; // 直接去 labellist 的第一个 label 跳转
    jump->u.jump->labels = newLabelList(backLabel->u.label->label, NULL);

    breakNum[whileNum++] = 0;
    seq2->u.seq->right = StmTranslate(node->stm.loop->stm, func, 1, jump); // loop 主体
    breakFlag = 1;
    top--;

    // 条件分支判断跳转 cjump：
    seq1->u.seq->left = Condition_Cjump(node->stm.loop->condition, func, trueLabel->u.label->label);
    seq1->u.seq->left->fa = seq1;

    return root;
}

irStm JUMPTranslate(STM node, FuncBucket func)
{
    /*
    如果是 continue 语句，就跳转到最近的 while 的 backlabel；
    如果是 break 语句，就跳转到最近的 while 的 falselabel。
    */
    irStm jump = newStm(_irjump, NULL);
    if (node->stm.jump->jumpKind == _continue)
    {
        jump->u.jump->labels = newLabelList(backLabelStack[top - 1], NULL);
    }
    else
    {
        breakNum[whileNum - 1]++;
        breakJumpPatch[breakTop++] = jump;
    }
    irExp idx = newExp(_irconsti, _numint);
    jump->u.jump->idx = idx;
    idx->u.const_i->n = 0; // 直接去 labellist 的第一个 label 跳转

    return jump;
}

irStm WRITETranslate(STM node, FuncBucket func)
{
    irStm root = newStm(_irprint, NULL);
    if (node->stm.write->writeKind == _printf) // 格式化输出
    {
        root->u.print->type = _printformat;
        root->u.print->u.printf.width = node->stm.write->num->inum;
        root->u.print->u.printf.exp = Exp_IrExp(node->stm.write->exp, func);
    }
    else if (node->stm.write->writeKind == _print)
    {
        WC wc = node->stm.write->wc;
        while (wc)
        {
            root->u.print->type = _printnormal;
            root->u.print->u.print.expList = newExpList(Exp_IrExp(wc->exp, func), root->u.print->u.print.expList);
            wc = wc->next;
        }
    }
    else if (node->stm.write->writeKind == _println)
    {
        root->u.print->type = _line;
    }
    return root;
}

irStm RC_IrMove(RC rc, FuncBucket func)
{
    irStm move = newStm(_irmove, NULL);
    irExp mem = Exp_IrExp(rc->exp, func);
    if (mem->kind != _irmem)
    {
        ERROR("ERROR: The mem was generated incorrectly\n");
    }

    move->u.move->dest = mem;
    move->u.move->src = newExp(_irscan, mem->valType);
    return move;
}

irStm READTranslate(STM node, FuncBucket func)
{
    irStm root, newSeq;
    RC rc = node->stm.read->rc;
    if (rc->next == NULL)
    {
        root = RC_IrMove(rc, func);
    }
    else
    {
        root = newStm(_irseq, NULL);
        root->u.seq->left = RC_IrMove(rc, func);
        root->u.seq->left->fa = root;
        rc = rc->next;
        newSeq = root;
        while (rc)
        {
            if (rc->next)
            {
                newSeq->u.seq->right = newStm(_irseq, newSeq);
                newSeq = newSeq->u.seq->right;
                newSeq->u.seq->left = RC_IrMove(rc, func);
                newSeq->u.seq->left->fa = newSeq;
            }
            else
            {
                newSeq->u.seq->right = RC_IrMove(rc, func);
                newSeq->u.seq->right->fa = newSeq;
            }
            rc = rc->next;
        }
    }
    return root;
}

/*
根据 bucketlist 回退环境，即当前函数的符号表；用于从 branch、loop 主体中退出时 pop 出其中定义的局部变量
st：symbolTable，即要回退的环境
bl：bucketlist，所有要回退的局部变量
size：bucketlist 的 size
*/
void EnvRollBack(Bucket *st, Bucket *bl, int size)
{
    for (int i = 0; i < size; i++)
    {
        Bucket var = bl[i];
        pop(st, var->key);
    }
}

irStm DoPatch(STM stmNode, irStm irOldNode)
{
    irStm irNowNode;

    if ((jumpFlag || cjumpFlag || breakFlag) && (stmNode == NULL || (stmNode->stmKind != _def && stmNode->stmKind != _func)))
    {
        // 1. 创建label节点
        irNowNode = newStm(_irseq, irOldNode);
        irOldNode->u.seq->right = irNowNode;
        irNowNode->u.seq->left = newStm(_irlabel, irNowNode);

        // 2. 获取label值并填写到新节点和patch
        int label = newLabel();
        irNowNode->u.seq->left->u.label->label = label;
        if (jumpFlag)
        {
            jumpPatch->u.jump->labels = newLabelList(label, NULL);
            jumpPatch = NULL;
            jumpFlag = 0;
        }
        if (cjumpFlag)
        {
            cjumpPatch->u.cjump->falseL = label;
            cjumpPatch = NULL;
            cjumpFlag = 0;
        }
        if (breakFlag)
        {
            int t = breakNum[--whileNum];
            while (t--)
            {
                if (--breakTop < 0)
                    ERROR("ERROR: breakTop < 0\n");
                breakJumpPatch[breakTop]->u.jump->labels = newLabelList(label, NULL);
            }
            breakFlag = 0;
        }

        return irNowNode;
    }
    return irOldNode;
}

irStm StmTranslate(STM startNode, FuncBucket func, int popFlag, irStm tail)
{
    irStm irDummyRoot = newStm(_irseq, NULL), irNowNode = irDummyRoot;
    Bucket bucketList[MAX_BUCKET_NUM]; // 记录当前作用域定义的变量，用于 pop
    int idx = 0;
    // 遍历 AST 树：
    for (STM node = startNode; node; node = node->next)
    {
        // 1. 如果需要patch, 在生成下一个节点之前先生成用于跳出的label
        irNowNode = DoPatch(node, irNowNode);

        // 2. 分类讨论节点类型并递归向下生成
        if (node->stmKind == _def)
        {
            bucketList[idx++] = DEFTranslate(node, func);
        }
        else if (node->stmKind == _read)
        {
            irNowNode->u.seq->right = newStm(_irseq, irNowNode);
            irNowNode->u.seq->right->u.seq->left = READTranslate(node, func);
            irNowNode->u.seq->right->u.seq->left->fa = irNowNode->u.seq->right;
        }
        else if (node->stmKind == _write)
        {
            irNowNode->u.seq->right = newStm(_irseq, irNowNode);
            irNowNode->u.seq->right->u.seq->left = WRITETranslate(node, func);
            irNowNode->u.seq->right->u.seq->left->fa = irNowNode->u.seq->right;
        }
        else if (node->stmKind == _assign)
        {
            irNowNode->u.seq->right = newStm(_irseq, irNowNode);
            irNowNode->u.seq->right->u.seq->left = ASSIGNTranslate(node, func);
            irNowNode->u.seq->right->u.seq->left->fa = irNowNode->u.seq->right;
        }
        else if (node->stmKind == _branch)
        {
            irNowNode->u.seq->right = newStm(_irseq, irNowNode);
            irNowNode->u.seq->right->u.seq->left = BRANCHTranslate(node, func);
            irNowNode->u.seq->right->u.seq->left->fa = irNowNode->u.seq->right;
        }
        else if (node->stmKind == _loop)
        {
            irNowNode->u.seq->right = newStm(_irseq, irNowNode);
            irNowNode->u.seq->right->u.seq->left = LOOPTranslate(node, func);
            irNowNode->u.seq->right->u.seq->left->fa = irNowNode->u.seq->right;
        }
        else if (node->stmKind == _jump)
        {
            irNowNode->u.seq->right = newStm(_irseq, irNowNode);
            irNowNode->u.seq->right->u.seq->left = JUMPTranslate(node, func);
            irNowNode->u.seq->right->u.seq->left->fa = irNowNode->u.seq->right;
        }
        else if (node->stmKind == _func)
        {
            irStm funcNode = newStm(_irseq, NULL);
            funcNode->u.seq->left = FUNCTranslate(node, func);
            funcNode->u.seq->left->fa = funcNode;

            // 为了后续实现简便，将函数定义节点插在链表头而不是尾
            funcNode->u.seq->right = irDummyRoot->u.seq->right;
            irDummyRoot->u.seq->right = funcNode;

            // TODO: irNowNode改名成tail可能更合适
            // irNowNode应该始终为主链表的末尾节点
            // 当且仅当主链表为空（irNowNode==irDummyHead）时进行头插，会使末尾节点会改变
            // 此时需要更新irNowNode
            if (irNowNode == irDummyRoot)
            {
                irNowNode = funcNode;
            }
        }
        else if (node->stmKind == _ret)
        {
            irNowNode->u.seq->right = newStm(_irseq, irNowNode);
            irNowNode->u.seq->right->u.seq->left = RETTranslate(node, func);
            irNowNode->u.seq->right->u.seq->left->fa = irNowNode->u.seq->right;
        }
        else if (node->stmKind == _exp)
        {
            irNowNode->u.seq->right = newStm(_irseq, irNowNode);
            irNowNode->u.seq->right->u.seq->left = newStm(_irexp, irNowNode->u.seq->right);
            irNowNode->u.seq->right->u.seq->left->u.exp = Exp_IrExp(node->stm.exp, func);
        }

        // 3.
        // 变量定义不产生IR节点，函数定义节点插在链表头
        // 除这两种情况外，都有新的节点插在链表末尾，需要移动irNowNode
        if (node->stmKind != _def && node->stmKind != _func)
        {
            irNowNode = irNowNode->u.seq->right;
            if (firstStm == NULL && func == mainFunc) // 维护开始执行的第一个 stm 节点
            {
                firstStm = irNowNode;
            }
        }
    }

    if (popFlag)
    {
        EnvRollBack(func->binding->symbolTable, bucketList, idx);
    }

    irNowNode = DoPatch(NULL, irNowNode);
    irNowNode->u.seq->right = tail;
    tail->fa = irNowNode;

    return irDummyRoot->u.seq->right;
}

void IRTranslation(STM root)
{
    // 初始化“主函数”：
    mainFunc = insertFunc("main", 0, NULL, _symnull, 0);
    insert(mainFunc, "sp", _symint, 1, NULL, 0); // 将函数栈指针视作全局变量

    irStm irRoot = StmTranslate(root, mainFunc, 0, newStm(_irfinish, NULL));
#ifdef SHOW_IR
    printf("Finish generate IR\n");

    printIrStm(irRoot);
    printf("\n");
#endif
    Interpret(firstStm, irRoot);
}