bison --yacc -dv proj2.y
flex proj2.l
gcc -o test y.tab.c lex.yy.c 