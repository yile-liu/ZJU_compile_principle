#include "symbol.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

FuncBucket funcTable[MAX_TABLE_SIZE];

int newLabel()
{
    static int count = 0;
    return count++;
}

void ERROR(char *meg)
{
    printf("%s", meg);
    exit(-1);
}

enum VarType TK_VT(enum typekind tk)
{
    switch (tk)
    {
    case _int:
        return _symint;
    case _float:
        return _symfloat;
    case _char:
        return _symchar;
    default:
        ERROR("ERROR: TK_VT\n");
        break;
    }
}

int hash(char *s)
{
    int ret = 0;
    for (char *p = s; *p; p++)
    {
        ret = (ret * 65599 + *p) % MAX_TABLE_SIZE;
    }
    return ret;
}

int getTypeSize(enum VarType vt)
{
    switch (vt)
    {
    case _symchar:
    case _symchar_arr:
        return sizeof(char);
        break;
    case _symfloat:
        return sizeof(float);
        break;
    case _symint:
        return sizeof(int);
    default:
        ERROR("ERROR: getTypeSize\n");
        return 0;
        break;
    }
}

/*
创建新符号项
size：变量/数组所占空间大小
arrSize：数组每一维度的大小
dim：数组维度
*/
Bucket newBucket(FuncBucket func, char *key, enum VarType t, int size, int *arrSize, int dim, Bucket next)
{
    Bucket nb = (Bucket)calloc(1, sizeof(struct Bucket_));
    strcpy(nb->key, key);
    nb->type = t;
    nb->next = next;
    nb->size = size;
    nb->arrSize = arrSize;
    nb->dim = dim;

    nb->offset = func->binding->stackFrameSize;
    func->binding->stackFrameSize += nb->size * getTypeSize(t); // 更新未被使用的空间起始地址

    return nb;
}

Bucket insert(FuncBucket func, char *key, enum VarType t, int size, int *arrSize, int dim)
{
    int index = hash(key);
    // 将新 bucket 插入函数的符号表中，同时更新最近定义的 bucket
    func->binding->latestBucket = func->binding->symbolTable[index] = newBucket(func, key, t, size, arrSize, dim, func->binding->symbolTable[index]);

    return func->binding->symbolTable[index];
}

/*从符号表中查找符号*/
enum VarType lookUp(Bucket *symbolTable, char *key)
{
    int index = hash(key);
    for (Bucket bp = symbolTable[index]; bp; bp = bp->next)
    {
        if (0 == strcmp(bp->key, key))
            return bp->type;
    }
    return _symnull; // 未找到
}

/*
获取数组元素数量
*/
int getSize(Bucket *symbolTable, char *key)
{
    int index = hash(key);
    for (Bucket bp = symbolTable[index]; bp; bp = bp->next)
    {
        // 需要检查符号是否匹配，以及是否是数组
        if (0 == strcmp(bp->key, key))
        {
            return bp->size;
        }
    }
    return 0; // 未找到
}

int getOffset(Bucket *symbolTable, char *key)
{
    int index = hash(key);
    for (Bucket bp = symbolTable[index]; bp; bp = bp->next)
    {
        if (0 == strcmp(bp->key, key))
            return bp->offset;
    }
    return -1; // 未找到
}

int *getArrSize(Bucket *symbolTable, char *key)
{
    int index = hash(key);
    for (Bucket bp = symbolTable[index]; bp; bp = bp->next)
    {
        if (0 == strcmp(bp->key, key))
        {
            return bp->arrSize;
        }
    }
    return NULL; // 未找到
}

int getArrDim(Bucket *symbolTable, char *key)
{
    int index = hash(key);
    for (Bucket bp = symbolTable[index]; bp; bp = bp->next)
    {
        if (0 == strcmp(bp->key, key))
            return bp->dim;
    }
    return -1; // 未找到
}

/*从符号表中删除符号*/
void pop(Bucket *symbolTable, char *key)
{
    int index = hash(key);
    symbolTable[index] = symbolTable[index]->next;
}

FuncBucket newFuncBucket(char *key, int paramNum, char **paramNames, RetType rt, int label, FuncBucket next)
{
    FuncBucket nfb = (FuncBucket)calloc(1, sizeof(struct FuncBucket_));
    nfb->binding = (FuncBinding)calloc(1, sizeof(struct FuncBinding_));

    strcpy(nfb->key, key);
    nfb->binding->paramNames = paramNames;
    nfb->binding->paramNum = paramNum;
    nfb->binding->retType = rt;
    nfb->next = next;
    nfb->binding->label = label;
    nfb->binding->stackFrameSize = 0;

    return nfb;
}

FuncBucket insertFunc(char *key, int paramNum, char **paramNames, RetType rt, int label)
{
    int index = hash(key);
    funcTable[index] = newFuncBucket(key, paramNum, paramNames, rt, label, funcTable[index]);
    return funcTable[index];
}

FuncBucket lookUpFunc(char *key)
{
    int index = hash(key);
    for (FuncBucket fbp = funcTable[index]; fbp; fbp = fbp->next)
    {
        if (0 == strcmp(fbp->key, key))
            return fbp;
    }
    return NULL; // 未找到
}