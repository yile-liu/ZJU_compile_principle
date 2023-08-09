%{
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "ast.h"
#include "../debug.h"

int yydebug = 1;

int yylex(void);
void yyerror(char *);

void yyerrok() {
    // 禁用默认的换行符输出行为
}

ID newID(char *name) {
    ID i = (ID)calloc(1, sizeof(struct ID_));
    strcpy(i->s, name);
    return i;
}

CONST_C newConst_C(char c) {
    CONST_C cc = (CONST_C)calloc(1, sizeof(struct CONST_C_));
    cc->c = c;
    return cc;
}

CONST_S newConst_S(char *s) {
    CONST_S cs = (CONST_S)calloc(1, sizeof(struct CONST_S_));
    strcpy(cs->s, s);
    return cs;
}

FNUM newFNUM(float f) {
    FNUM n = (FNUM)calloc(1, sizeof(struct FNUM_));
    n->fnum = f;
    return n;
}

INUM newINUM(int i) {
    INUM n = (INUM)calloc(1, sizeof(struct INUM_));
    n->inum = i;
    return n;
}

STM root = NULL;

%}

%union{
    char * s;
	float fnum;
    int inum;
    char c;
    struct TYPE_ * type;
    struct OP_ * op;
    struct ARRAY_ * array;
    struct EXP_ *exp;
    struct MPTERM_ *mpterm;
    struct ADDR_ * addr;
    struct DEF_ * def;
    struct READ_ * read;
    struct RC_ * rc;
    struct WC_ * wc;
    struct WRITE_ * write;
    struct ASSIGN_ * assign;
    struct BRANCH_ * branch;
    struct STM_ * stm;
    struct LOOP_ * loop;
    struct JUMP_ * jump;
    struct FUNC_ * func;
    struct PARAM_ * param;
    struct FUNCALL_ * funcall;
    struct PARAMCALL_ * paramcall;
}

%token  INT FLOAT CHAR END RET
%token  ADD SUB MUL DIV EQ NE GT GE LT LE _ASSIGN_
%token  SCAN PRINT PRINTF IF WHILE CONTINUE BREAK PRINTLN
%token  LP RP LMP RMP LLP RLP COLON SEMICOLON COMMA

%token <fnum> _FNUM_
%token <inum> _INUM_
%token <s> _ID_
%token <c> _CONST_C_
%token <s> _CONST_S_

%type <type> type
%type <addr> addr
%type <def> def
%type <assign> assign
%type <exp> exp
%type <branch> branch
%type <stm> stm
%type <array> array
%type <mpterm> mpTerm
%type <read> read
%type <rc> rc
%type <write> write
%type <wc> wc
%type <op> op
%type <funcall> funcall
%type <paramcall> paramcall
%type <jump> jump
%type <param> param
%type <func> func
%type <loop> loop

%%
        program : stm END SEMICOLON     {
                                            translateToIR(root);
                                            exit(0);
                                        }
        stm     : stm stm       		{
                                            // printf("stm stm -> stm\n");
                                            STM s = $1; // 本身节点变为第一个 stm
                                            STM tail = s;
                                            while (tail->next) tail = tail->next;
                                            tail->next = $2;   // next 连接第二个 stm
                                            $$ = s;
                                            // printStm($1);
                                            // printStm($2);

                                        }

                | def SEMICOLON			{
                                            STM s = (STM)calloc(1, sizeof(struct STM_));
                					        s->stmKind = _def;
                                            s->stm.def = $1;

                                            $$ = s;

                                            if(root == NULL){
                                                root = s;
                                            }
                                        }
                | read SEMICOLON        {
                                            STM s = (STM)calloc(1, sizeof(struct STM_));
                					        s->stmKind = _read;
                                            s->stm.read = $1;

                                            $$ = s;

                                            if(root == NULL){
                                                root = s;
                                            }
                				        }
                | write SEMICOLON       {
                                            STM s = (STM)calloc(1, sizeof(struct STM_));
                					        s->stmKind = _write;
                                            s->stm.write = $1;

                                            $$ = s;

                                            if(root == NULL){
                                                root = s;
                                            }
                				        }
                | assign SEMICOLON      {
                                            STM s = (STM)calloc(1, sizeof(struct STM_));
                					        s->stmKind = _assign;
                                            s->stm.assign = $1;

                                            $$ = s;

                                            if(root == NULL){
                                                root = s;
                                            }
                				        }
                | branch SEMICOLON      {
                                            STM s = (STM)calloc(1, sizeof(struct STM_));
                					        s->stmKind = _branch;
                                            s->stm.branch = $1;

                                            $$ = s;

                                            if(root == NULL || s->stm.branch->stm == root){
                                                root = s;
                                            }
                				        }
                | loop SEMICOLON        {
                                            STM s = (STM)calloc(1, sizeof(struct STM_));
                					        s->stmKind = _loop;
                                            s->stm.loop = $1;

                                            $$ = s;

                                            if(root == NULL || s->stm.loop->stm == root) {
                                                root = s;
                                            }
                				        }
                | jump SEMICOLON        {
                                            STM s = (STM)calloc(1, sizeof(struct STM_));
                					        s->stmKind = _jump;
                                            s->stm.jump = $1;

                                            $$ = s;

                                            if(root == NULL){
                                                root = s;
                                            }
                				        }
                | func SEMICOLON        {
                                            STM s = (STM)calloc(1, sizeof(struct STM_));
                					        s->stmKind = _func;
                                            s->stm.func = $1;

                                            $$ = s;

                                            if(root == NULL || s->stm.func->funcStm == root){
                                                root = s;
                                            }
                                        }
                | RET exp SEMICOLON     {
                                            STM s = (STM)calloc(1, sizeof(struct STM_));
                					        s->stmKind = _ret;
                                            s->stm.ret = $2;

                                            $$ = s;

                                            if(root == NULL || s->stm.func->funcStm == root){
                                                root = s;
                                            }
                                        }
                | exp SEMICOLON         {
                                            STM s = (STM)calloc(1, sizeof(struct STM_));
                					        s->stmKind = _exp;
                                            s->stm.exp = $1;

                                            $$ = s;

                                            if(root == NULL || s->stm.func->funcStm == root){
                                                root = s;
                                            }
                                        }
                ;



        type    : INT				{
        						        TYPE t = (TYPE) calloc(1, sizeof(struct TYPE_));
                                        t->typeKind = _int;
                                        $$ = t;
        					        }
        	    | CHAR              {
        						        TYPE t = (TYPE) calloc(1, sizeof(struct TYPE_));
                                        t->typeKind = _char;
                                        $$ = t;
        					        }
        	    | FLOAT             {
        						        TYPE t = (TYPE) calloc(1, sizeof(struct TYPE_));
                                        t->typeKind = _float;
                                        $$ = t;
        					        }
                ;

        def     : type addr		{
									DEF d = (DEF)calloc(1, sizeof(struct DEF_));
									d->type = $1;
									d->addr = $2;
									$$ = d;
								}
                ;

        array   : _ID_ mpTerm           {
                                                ARRAY a = (ARRAY)calloc(1, sizeof(struct ARRAY_));
                                                a->id = newID($1);
                                                a->mpTerm = $2;
                                                $$ = a;
                                        }
                ;

        mpTerm  : LMP exp RMP mpTerm    {
                                                MPTERM m = (MPTERM)calloc(1, sizeof(struct MPTERM_));
                                                m->exp = $2;
                                                m->next = $4;
                                                $$ = m;
                                        }
                | LMP exp RMP           {
                                                MPTERM m = (MPTERM)calloc(1, sizeof(struct MPTERM_));
                                                m->exp = $2;
                                                $$ = m;
                                        }
                ;

        read    : SCAN LP rc RP         {
                                                READ r = (READ)calloc(1, sizeof(struct READ_));
                                                r->rc = $3;
                                                $$ = r;
                                        }
                ;

        addr    : _ID_ 		{
								ADDR a = (ADDR)calloc(1, sizeof(struct ADDR_));
								a->addrKind = _id;
								a->addr.id = newID($1);
								$$ = a;
							}
				| array		{
								ADDR a = (ADDR)calloc(1, sizeof(struct ADDR_));
								a->addrKind = _array;
								a->addr.arr = $1;
								$$ = a;
							}
                ;

        rc      : addr COMMA rc                     {
                                                        RC r = (RC)calloc(1, sizeof(struct RC_));
                                                        EXP exp = (EXP)calloc(1, sizeof(struct EXP_));
                                                        exp->expKind = _addr;
                                                        exp->exp.addr = $1;
                                                        r->exp = exp;
                                                        r->next = $3;
                                                        $$ = r;
                                                    }
                | addr               {
                                                        RC r = (RC)calloc(1, sizeof(struct RC_));
                                                        EXP exp = (EXP)calloc(1, sizeof(struct EXP_));
                                                        exp->expKind = _addr;
                                                        exp->exp.addr = $1;
                                                        r->exp = exp;
                                                        $$ = r;
                                                }
                ;

        write   : PRINT LP wc RP                {
                                                        WRITE w = (WRITE)calloc(1, sizeof(struct WRITE_));
                                                        w->writeKind = _print;
                                                        w->wc = $3;
                                                        $$ = w;
                                                }
                | PRINTF LP _INUM_ COMMA exp RP     {
                                                        WRITE w = (WRITE)calloc(1, sizeof(struct WRITE_));
                                                        w->writeKind = _printf;
                                                        w->num = newINUM($3);
                                                        w->exp = $5;
                                                        $$ = w;
                                                    }
                | PRINTLN                       {
                                                        WRITE w = (WRITE)calloc(1, sizeof(struct WRITE_));
                                                        w->writeKind = _println;
                                                        $$ = w;
                                                }
                ;

        wc      : wc COMMA exp                  {
                                                        WC w = (WC)calloc(1, sizeof(struct WC_));
                                                        w->exp = $3;
                                                        w->next = $1;
                                                        $$ = w;
                                                }
                | exp                           {
                                                        WC w = (WC)calloc(1, sizeof(struct WC_));
                                                        w->exp = $1;
                                                        $$ = w;
                                                }
                ;

        op      : ADD                   {
                                            OP o = (OP)calloc(1, sizeof(struct OP_));
                                            o->opKind = _add;
                                            $$ = o;
                                        }
                | SUB                   {
                                            OP o = (OP)calloc(1, sizeof(struct OP_));
                                            o->opKind = _minu;
                                            $$ = o;
                                        }
                | MUL                   {
                                            OP o = (OP)calloc(1, sizeof(struct OP_));
                                            o->opKind = _times;
                                            $$ = o;
                                        }
                | DIV                   {
                                            OP o = (OP)calloc(1, sizeof(struct OP_));
                                            o->opKind = _div;
                                            $$ = o;
                                        }
                | EQ                    {
                                            OP o = (OP)calloc(1, sizeof(struct OP_));
                                            o->opKind = _eq;
                                            $$ = o;
                                        }
                | NE                    {
                                            OP o = (OP)calloc(1, sizeof(struct OP_));
                                            o->opKind = _ne;
                                            $$ = o;
                                        }
                | GT                    {
                                            OP o = (OP)calloc(1, sizeof(struct OP_));
                                            o->opKind = _gt;
                                            $$ = o;
                                        }
                | GE                    {
                                            OP o = (OP)calloc(1, sizeof(struct OP_));
                                            o->opKind = _ge;
                                            $$ = o;
                                        }
                | LT                    {
                                            OP o = (OP)calloc(1, sizeof(struct OP_));
                                            o->opKind = _lt;
                                            $$ = o;
                                        }
                | LE                    {
                                            OP o = (OP)calloc(1, sizeof(struct OP_));
                                            o->opKind = _le;
                                            $$ = o;
                                        }
                ;

        exp     : addr					{
											EXP e = (EXP)calloc(1, sizeof(struct EXP_));
											e->expKind = _addr;
											e->exp.addr = $1;
											$$ = e;
										}
                | _FNUM_		    	{
											EXP e = (EXP)calloc(1, sizeof(struct EXP_));
											e->expKind = _fnum;
											e->exp.fnum = newFNUM($1);
											$$ = e;
										}
                | _INUM_		    	{
											EXP e = (EXP)calloc(1, sizeof(struct EXP_));
											e->expKind = _inum;
											e->exp.inum = newINUM($1);
											$$ = e;
										}
                | SUB _FNUM_			{
											EXP e = (EXP)calloc(1, sizeof(struct EXP_));
											e->expKind = _fnum;
											e->exp.fnum = newFNUM(-$2);
											$$ = e;
										}
                | SUB _INUM_			{
											EXP e = (EXP)calloc(1, sizeof(struct EXP_));
											e->expKind = _inum;
											e->exp.inum = newINUM(-$2);
											$$ = e;
										}
                | _CONST_C_				{
											EXP e = (EXP)calloc(1, sizeof(struct EXP_));
											e->expKind = _const_c;
											e->exp.c = newConst_C($1);
											$$ = e;
										}
                | _CONST_S_				{
											EXP e = (EXP)calloc(1, sizeof(struct EXP_));
											e->expKind = _const_s;
											e->exp.s = newConst_S($1);
											$$ = e;
										}
                | funcall               {
											EXP e = (EXP)calloc(1, sizeof(struct EXP_));
											e->expKind = _funcall;
											e->exp.funcall = $1;
											$$ = e;
										}
                | exp op exp			{
											EXP e = (EXP)calloc(1, sizeof(struct EXP_));
											e->expKind = _op;
											e->exp.op = $2;
											($2)->lexp = $1;
											($2)->rexp = $3;
											$$ = e;
										}
                | LP exp RP				{
											$$ = $2;
										}
                ;

        assign  : addr _ASSIGN_ exp		{
											ASSIGN a = (ASSIGN)calloc(1, sizeof(struct ASSIGN_));
                                            a->addr = (EXP)calloc(1, sizeof(struct EXP_));
                                            a->addr->expKind = _addr;
                                            a->addr->exp.addr = $1;
											a->exp = $3;
											$$ = a;
										}
                ;

        branch  : IF LP exp RP LLP stm RLP		{
													BRANCH b = (BRANCH)calloc(1, sizeof(struct BRANCH_));
													b->condition = $3;
													b->stm = $6;
													$$ = b;
												}
                ;

        loop    : WHILE LP exp RP LLP stm RLP   {
                                                        LOOP l = (LOOP)calloc(1, sizeof(struct LOOP_));
                                                        l->condition = $3;
                                                        l->stm = $6;
                                                        $$ = l;
                                                }
                ;

        jump    : CONTINUE                      {
                                                    JUMP j = (JUMP)calloc(1, sizeof(struct JUMP_));
                                                    j->jumpKind = _continue;
                                                    $$ = j;
                                                }
                | BREAK                         {
                                                    JUMP j = (JUMP)calloc(1, sizeof(struct JUMP_));
                                                    j->jumpKind = _break;
                                                    $$ = j;
                                                }
                ;

        func    : type _ID_ LP param RP LLP stm RLP     {
                                                        FUNC f = (FUNC)calloc(1, sizeof(struct FUNC_));
                                                        f->funcType = $1;
                                                        f->funcName = newID($2);
                                                        f->params = $4;
                                                        f->funcStm = $7;
                                                        $$ = f;
                                                        }
                | type _ID_ LP RP LLP stm RLP           {
                                                        FUNC f = (FUNC)calloc(1, sizeof(struct FUNC_));
                                                        f->funcType = $1;
                                                        f->funcName = newID($2);
                                                        f->params = NULL;
                                                        f->funcStm = $6;
                                                        $$ = f;
                                                        }
                ;

        param   : param COMMA type _ID_           {
                                                    PARAM p = (PARAM)calloc(1, sizeof(struct PARAM_));
                                                    p->varType = $3;
                                                    p->id = newID($4);
                                                    p->next = $1;
                                                    $$ = p;
                                                }
                | type _ID_                     {
                                                    PARAM p = (PARAM)calloc(1, sizeof(struct PARAM_));
                                                    p->varType = $1;
                                                    p->id = newID($2);
                                                    $$ = p;
                                                }
                ;

        funcall : _ID_ LP paramcall RP            {
                                                    FUNCALL f = (FUNCALL)calloc(1, sizeof(struct FUNCALL_));
                                                    f->id = newID($1);
                                                    f->params = $3;
                                                    $$ = f;
                                                }
                ;

        paramcall   : paramcall COMMA exp       {
                                                    PARAMCALL p = (PARAMCALL)calloc(1, sizeof(struct PARAMCALL_));
                                                    p->exp = $3;
                                                    p->next = $1;
                                                    $$ = p;
                                                }
                    | exp                       {
                                                    PARAMCALL p = (PARAMCALL)calloc(1, sizeof(struct PARAMCALL_));
                                                    p->exp = $1;
                                                    $$ = p;
                                                }
                    ;

%%
void yyerror(char *str){
	fprintf(stderr,"error: %s\n",str);
}

int yywrap(){
    return 1;
}
int main(int argc, char *argv[])
{
#ifdef FILE_IN
    extern FILE *yyin;
#ifdef MATRIX
    yyin = fopen("./test_code/matrix.txt", "r");
#endif
#ifdef QUICK
    yyin = fopen("./test_code/quicksort_modify.txt", "r");
#endif
#ifdef ADVISOR
    yyin = fopen("./test_code/auto-adviser_modify.txt", "r");
#endif
    if (yyin == NULL)
    {
        printf("Can not open file\n");
    }
    #endif
    yyparse();

    return 0;
}
