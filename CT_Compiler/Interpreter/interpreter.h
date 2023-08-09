/*
IR 树可以视作 irStm 的链表，每个原程序语句对应链表中的一个节点。
所有的函数定义被移动到链表前端，需要跳过这一部分开始执行。
*/
#ifndef INTERPRETER_H_
#define INTERPRETER_H_

#include "../IR/ir.h"

#define MAX_LABEL_NUM 1000
#define STACK_SIZE ((1 << 30)) // 200M 内存空间

typedef struct ExpValue_
{
    enum ExpValueType
    {
        _expint,
        _expfloat,
        _expchar,
        _expstring,
        _expboolean,
        _expnull
    } type;
    union
    {
        int i;
        float f;
        char c;
        char str[MAX_STR_LENGTH];
        int boolean;
    } u;
} ExpValue;

/*
完成执行 IR 树的任务
startStm：IR 树开始执行的 stm 节点
root：IR 树的根节点
*/
extern void Interpret(irStm startStm, irStm root);

/*
从 stm 节点开始遍历 IR 树，找到所有 label
*/
void FindLabelFromStm(irStm stm);

/*
从 exp 节点开始遍历 IR 树，找到所有 label
*/
void FindLabelFromExp(irExp exp);

/*
执行 irStm 节点
*/
ExpValue ExecStm(irStm stm);

/*
执行 irExp 节点，返回值
*/
ExpValue ExecExp(irExp exp);

#endif