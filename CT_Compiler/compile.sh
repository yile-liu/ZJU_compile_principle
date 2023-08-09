cd AST
bison --yacc -dv yacc.y
flex lex.l
cd ..
gcc -g -o out ./AST/y.tab.c ./AST/lex.yy.c ./AST/ast.c ./IR/ir.c ./IR/symbol.c ./IR/printir.c  ./Interpreter/interpreter.c
rm ./AST/y.tab.c ./AST/lex.yy.c ./AST/y.output ./AST/y.tab.h