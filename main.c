#include <stdio.h>
#include "grammar_analysis.h"
#include "generate_mips_code.h"
#include "generate_mid_code.h"
#include "optimizer.h"
#include "generate_mips_code_opt.h"
#include "mips_code_optimize.h"


extern FILE * file;

int main() {
    char source[200];
    printf("Please input source code path:");
    fgets(source, 199, stdin);
    source[strlen(source) - 1] = 0;
    file = fopen(source, "r");

    grammar_analysis();             //语法分析，语义分析并生成中间代码
    fillTempTab();                  //填充临时变量表
    printTab();                     //输出符号表
    printMidCode(0);                //在文件中输出中间代码
    generate_mips_code();           //生成mips指令
    print_mips_code(0);             //输出mips指令，在mips.asm里




    optimize();                     //中间代码优化
    fillTempTab();                  //填充临时变量表
    printMidCode(1);                //在文件中输出优化后的midcode

    generate_mips_code_opt();       //使用优化方法生成mips指令
    print_mips_code_opt(1);         //输出优化后的mips指令，在mips_opt.asm里

    optimize_mips();                //优化mips指令
    print_mips_code_opt(2);         //输出再次优化后的mips指令，在mips_opt2.asm里

    return 0;
}

//后面记录所有声明的全局变量
/*
 * lexical_analysis.c
FILE * file;                            //待编译的源代码文件

int tooLong = 0;                        //标记catToken时是否过长
int lineCount = 1;                      //存储读的行数
int charCount = 0;                      //存储当前行读的字符数
int lastLineCount;                      //存储当次getSymbol之前读的行数
int lastCharCount;                      //存储当次getSymbol之前读的字符数
int totalCharCount = 0;                 //记录读到的总的字符数
int symbol = 0;                         //存储读到的符号类型代码
char token[TOKENMAXLENGTH];             //存放读到的符号的字符串
int num;                                //存放当前读入的整型数值
char ch = ' ';                          //存放当前读入的字符
//int symbolCount = 0;                  //已读取的符号个数
char reservedWord[NUM_OF_RESERVED_WORD][20] = {           //存储所有保留字
        "const",
        "int",
        "char",
        "void",
        "main",
        "while",
        "switch",
        "case",
        "default",
        "scanf",
        "printf",
        "return",
        "if"
};
 */

/*
 * error.c
 * int foundError = 0;                  //标记是否找到错误
 */

/*
 * grammar_analysis.c
int tab_top = 0;                        //符号表栈顶指针，符号表从1号位开始存储
int globalTop;                          //记录最后一个全局变量/常量在符号表中的位置

int tempVarNum = 0;                     //记录临时变量编号 用$tempVarNum表示临时变量  $ret表示函数返回值

struct myTab tab[TAB_LENGTH];           //符号表
int tempTab[TEMP_TAB_LENGTH];           //临时变量表，记录各临时变量的类型

int foundReturnInFunction = 0;          //记录是否在函数中找到了有返回内容的return语句
 */

/*
 * generate_mid_code.c
struct myTempTab offsetTempTab[OFFSET_TEMP_TAB_LENGTH];             //临时变量偏移表，记录各临时变量在运行栈中的偏移量
int offsetTempTabTop = 0;                                           //临时变量偏移表栈顶指针   从1开始存储

struct mid_code midCodeTable[MID_CODE_TABLE_LENGTH];                //中间代码表，0号位不存储
int mid_code_top = 0;                                               //中间代码表栈顶指针
int labelNum = 0;                                                   //label编号。记录已经到第几个label了，从label1开始

int global_top_in_mid;                                              //记录最后一条全局量声明的中间代码位置
 */

/*
 * generate_mips_code.c
struct mips_data_code mipsDataCodeTable[MIPS_DATA_LENGTH];          //mips指令data部分表
struct mips_text_code mipsTextCodeTable[MIPS_TEXT_LENGTH];          //mips指令text部分表

int data_top = 0;                                                   //data区指令栈顶指针
int text_top = 0;                                                   //text区指令栈顶指针

int stringNum = 0;                                                  //记录printf中string的编号

char tempFunction[TOKENMAXLENGTH] = "";                             //记录在分析中间代码生成mips指令时，当前处于哪个函数
 */