%{
#include <stdio.h>
#include "y.tab.h"
void yyerror(char *);
%}
INUM    [1-9]+[0-9]*|0
FNUM	[0-9]+\.[0-9]+
%%


{INUM}		                {
                            sscanf(yytext,"%lf",&yylval.dnum);
                            // printf("int: %lf\n", yylval.dnum);
                            return NUM;
                            }
{FNUM}		                {
                            sscanf(yytext,"%lf",&yylval.dnum);
                            // printf("float: %lf\n", yylval.dnum);
                            return NUM;
                            }


"+"							return ADD;
"-"							return SUB;
"*"							return MUL;
"/"							return DIV;
"^"                         return POWER;
"("                         return LEFT_P;
")"                         return RIGHT_P;
\n                          return CR;
[ \t]+                     /* ignore whitespace */;
.
%%
