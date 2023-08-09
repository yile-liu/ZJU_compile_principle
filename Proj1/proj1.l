%{
    #include<string.h>

   int char_cnt = 0;
   int word_cnt = 0;
   int line_cnt = 0;
   char word_buff[200];
%}

WORD    [a-zA-Z0-9`·"'_-]*
KEY     if|else|int|double|return

%%
{WORD}				{
                  word_cnt++;
                  // Make a copy from yytext, instead of using yytext as a ptr directly.
                  strcpy(word_buff, yytext);
                  char_cnt += strlen(word_buff);
                  // printf("Word %s, strlen %lu, char_cnt %d\n", word_buff, strlen(word_buff), char_cnt);
              }

\n            {
                  // NOTICE that newline and punctuations are also counted as characters.
                  line_cnt++;
                  char_cnt++; 
              }
.             char_cnt++;

%%
int main(int argc, char **argv)
{
  yylex();
  printf("\n******Statistics******\n");
  printf("Char count: %d\n", char_cnt);
  printf("Word count: %d\n", word_cnt);
  printf("Line count: %d\n", line_cnt);
  yywrap();
}

int yywrap() // 可以读多文件（？
{
	return 1;
}
