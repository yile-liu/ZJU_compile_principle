# 第一个参数为flex文件名
# 第二个参数为输出可执行文件名（可选 默认为a.out）


if [ $# == 0 ]
then
    echo "Flex源文件名不能为空"
elif [ $# == 1 ]
then
    flex $1
    gcc lex.yy.c
    # ./a.out
else
    flex $1
    gcc lex.yy.c -o $2
    # ./$2
fi

rm lex.yy.c
