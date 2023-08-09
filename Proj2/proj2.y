%{
#include <stdio.h>
#include <string.h>
#include <math.h>
int yylex(void);
void yyerror(char *);
%}

%union{
  int inum;
  double dnum;
}

%token ADD SUB MUL DIV POWER LEFT_P RIGHT_P CR
%token <dnum> NUM
%type  <dnum> expression term single power

%%
       line_list: line
                | line_list line
                ;
				
	       line : expression CR  {printf("expression = %f\n",$1);}

      expression: term 
                | SUB power             {$$=-$2;}
                | expression ADD term   {$$=$1+$3;}
				| expression SUB term   {$$=$1-$3;}
                ;

            term: power
				| term MUL power		{$$=$1*$3;}
				| term DIV power		{$$=$1/$3;}
				;

           power: single
                | power POWER power     {$$=pow($1, $3);}
                | LEFT_P expression RIGHT_P     {$$=$2;}
				;

		  single: NUM
				;
%%
void yyerror(char *str){
    fprintf(stderr,"error: %s\n",str);
}

int yywrap(){
    return 1;
}
int main()
{
    yyparse();
}
