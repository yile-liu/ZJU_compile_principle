#ifndef SYMBOL_H
#define SYMBOL_H

#include "../AST/astNode.h"

/*
该部分需要完成符号表的功能
*/

#define MAX_KEY_LENGTH 100 // 变量、函数名最长长度
#define MAX_TABLE_SIZE 100 // 符号表容量
#define MAX_PARAM_NUM 10
#define SP_SIZE 4
#define SP_OFFSET 0
#define MAX_ARR_DIM 4

/*绑定符号类型*/
enum VarType
{
    _symint,
    _symfloat,
    _symchar, // 基本变量类型
    _symint_arr,
    _symfloat_arr,
    _symchar_arr, // 数组类型
    _symnull
};

typedef enum VarType ParamType; // 函数参数类型
typedef enum VarType RetType;   // 函数返回类型

typedef struct FuncBucket_ *FuncBucket;
typedef struct Bucket_ *Bucket;

/*
变量符号项：
key：变量名
type：变量绑定类型
*/
struct Bucket_
{
    char key[MAX_KEY_LENGTH];
    enum VarType type;
    Bucket next;
    int size;     // 变量、数组大小，单位：数量
    int offset;   // 变量、数组在地址空间所处位置
    int *arrSize; // 记录数组每一维度的大小
    int dim;      // 数组维度
};

/*
向符号表中插入新项，用于定义变量、数组时使用
size：变量、数组大小
arrSize：数组每一维度的大小
dim：数组维度
*/
extern Bucket insert(FuncBucket func, char *key, enum VarType t, int size, int *arrSize, int dim);

/*
在符号标中查找变量符号，返回变量绑定类型
*/
extern enum VarType lookUp(Bucket *symbolTable, char *key);

/*
查找然后获取数组的长度
*/
extern int getSize(Bucket *symbolTable, char *key);

/*
获取变量、数组在地址空间的起始地址
*/
extern int getOffset(Bucket *symbolTable, char *key);

/*
获取数组每一维度大小
*/
extern int *getArrSize(Bucket *symbolTable, char *key);

/*
获取数组维度
*/
extern int getArrDim(Bucket *symbolTable, char *key);

/*
从符号表中删除变量符号，传入符号变量名
注意：这里实际上是直接删除了符号表中符号变量所在链表的头部，
相当于是默认该符号处于链表头部，这种操作是基于环境的维护来定义的，
因为环境的改变总是一个符号先进后出、后进先出的
*/
extern void pop(Bucket *symbolTable, char *key);

/*
函数符号绑定，包括了函数的参数类型数组和返回类型
*/
typedef struct FuncBinding_ *FuncBinding;
struct FuncBinding_
{
    char **paramNames;
    int paramNum; // 参数数量

    RetType retType;

    Bucket symbolTable[MAX_TABLE_SIZE]; // 函数局部变量符号表
    Bucket latestBucket;                // 最新定义的变量，用于 sp 的移动
    int stackFrameSize;                 // 当前栈帧的大小，用于 sp 的移动

    int label;
};

/*
函数符号项，包括函数名和函数绑定
*/
struct FuncBucket_
{
    char key[MAX_KEY_LENGTH];
    FuncBinding binding;
    FuncBucket next;
};

/*
key: 函数名
pts: 函数参数类型数组指针
paramNum：参数数量
label：函数定义在 IR 中的 label 位置标记
*/
FuncBucket newFuncBucket(char *key, int paramNum, char **paramNames, RetType rt, int label, FuncBucket next);

/*
传入 AST 中函数定义语句的节点
向 funcTable 中插入这个函数相关的记录，并向其对应的 symbolTable 中插入参数作为变量
因此在调用该函数的地方，不要通过创建数组的形式记录函数类型
而应该从 funcBucket 的 funcBinding 取出 ParamType 指针数组
*/
extern FuncBucket insertFunc(char *key, int paramNum, char **paramNames, RetType rt, int label);

/*
查找函数符号项，返回函数绑定
*/
extern FuncBucket lookUpFunc(char *key);

/*
从enum typekind转变为enum VarType
*/
extern enum VarType TK_VT(enum typekind tk);

/*
从 1 开始递增，生成 label
*/
extern int newLabel();

extern void ERROR(char *meg); // 输出 msg 并 exit

#endif