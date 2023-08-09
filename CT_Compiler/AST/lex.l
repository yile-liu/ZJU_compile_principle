%{
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "y.tab.h"


void yyerror(char *);

// TODO: 带空格的字符、字符串, 以及\n等特殊逃逸字符
%}

_INUM_              0|[1-9]+[0-9]*
_FNUM_              [0-9]+\.[0-9]+
CHARACTER           \'(.)\'
STRING              \"(\\.|[^\\\"])*\"
_ID_                [a-zA-Z_][a-zA-Z0-9_]*

%%
{_INUM_}                    {
                                sscanf(yytext,"%d",&yylval.inum);
                                return _INUM_;
                            }
{_FNUM_}                    {
                                sscanf(yytext,"%f",&yylval.fnum);
                                return _FNUM_;
                            }

{STRING}                    {
                                char temp[100];
                                strcpy(temp, yytext);
                                yylval.s = (char *)calloc(1, sizeof(char) * 100);
                                strncpy(yylval.s, temp + 1, strlen(temp) - 2);
                                return _CONST_S_;
                            }
{CHARACTER}                 {
                                char temp[100];
                                strcpy(temp, yytext);
                                // sscanf(yytext, "%s", temp);
                                yylval.c = temp[1];
                                return _CONST_C_;
                            }

"int"                       return INT;
"float"                     return FLOAT;
"char"                      return CHAR;
"continue"                  return CONTINUE;
"break"                     return BREAK;
"scan"                      return SCAN;
"print"                     return PRINT;
"printf"                    return PRINTF;
"println"                   return PRINTLN;
"if"                        return IF;
"while"                     return WHILE;
"end"                       return END;
"return"                    return RET;

{_ID_}                      {
                                yylval.s = (char *)calloc(1, sizeof(char) * 100);
                                strcpy(yylval.s, yytext);
                                return _ID_;
                            }


"+"							return ADD;
"-"							return SUB;
"*"							return MUL;
"/"							return DIV;
"=="                        return EQ;
"!="                        return NE;
">"                         return GT;
">="                        return GE;
"<"                         return LT;
"<="                        return LE;

"="                         return _ASSIGN_;

"("                         return LP;
")"                         return RP;
"["                         return LMP;
"]"                         return RMP;
"{"                         return LLP;
"}"                         return RLP;

":"                         return COLON;
";"                         return SEMICOLON;
","                         return COMMA;
"\n"
.
%%
