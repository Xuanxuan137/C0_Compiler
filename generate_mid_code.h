#ifndef _GENERATE_MID_CODE_H_
#define _GENERATE_MID_CODE_H_

#include "lexical_analysis.h"
#include "grammar_analysis.h"
#include <stdlib.h>
#define MID_CODE_TABLE_LENGTH 100000
#define OFFSET_TEMP_TAB_LENGTH 100000

//为op定义一些值
#define NO_OP 0                             //不做操作(用作单纯生成一个label用)
#define CONSTINT 1                          //const int
#define CONSTCHAR 2                         //const char
#define VARINT 3                            //int
#define VARCHAR 4                           //char
#define ARRAYINT 5                          //int[]
#define ARRAYCHAR 6                         //char[]
#define FUNCINT 7                           //int func
#define FUNCCHAR 8                          //char func
#define FUNCVOID 9                          //void func
#define PARAINT 10                          //parameter int
#define PARACHAR 11                         //parameter char
#define RET 12                              //return
#define IF_LESS 13                          //if(* < *)
#define IF_LESSEQUAL 14                     //if(* <= *)
#define IF_MORE 15                          //if(* > *)
#define IF_MOREEQUAL 16                     //if(* >= *)
#define IF_NOTEQUAL 17                      //if(* != *)
#define IF_EQUAL 18                         //if(* == *)
#define GOTO_OP 19                          //goto
#define WHILE_LESS 20                       //while(* < *)
#define WHILE_LESSEQUAL 21                  //while(* <= *)
#define WHILE_MORE 22                       //while(* > *)
#define WHILE_MOREEQUAL 23                  //while(* >= *)
#define WHILE_NOTEQUAL 24                   //while(* != *)
#define WHILE_EQUAL 25                      //while(* == *)
#define SCANF_OP 26                         //scanf
#define PRINTF_OP1 27                       //printf("string", id);
#define PRINTF_OP2 28                       //printf("string");
#define PRINTF_OP3 29                       //printf(id);
#define ASSIGN_OP 30                        //a=
#define ARRAY_ASSIGN 31                     //a[]=
#define CALL 32                             //function call
#define CASE_OP 33                          //case constant:
#define DEFAULT_OP 34                       //default:
#define PUSH_OP 35                          //push parameter when function
#define PLUS_OP 36                          //+
#define MINUS_OP 37                         //-
#define MULT_OP 38                          //*
#define DIV_OP 39                           // /
#define ARRAY_GET 40                        //a[]
#define RET_EXTRA 41                        //return        仅用于生成mips时保证函数能够返回，不输出到中间代码文件里
#define WHILE_START_OP 42                   //while开始时的label，用于跳转

struct mid_code {
    int label;
    int op;
    char dst[TOKENMAXLENGTH];
    char src1[TOKENMAXLENGTH];
    char src2[TOKENMAXLENGTH];
    int effective;
};

//定义临时变量偏移符号表的结构
struct myTempTab {
    char name[TOKENMAXLENGTH];      //函数名或临时变量名
    int offset;                     //记录临时变量在运行栈中的偏移量，如果是函数名，则为函数内声明的最后一个变量之后的位置
};

void generate_mid_code(int label, int op, char dst[], char src1[], char src2[]);

void fillTempTab();                 //填充tempTab
int locTemp(char name[]);           //在tempTab中查找name

void numToString(char numString[], int num);
int stringToNum(char string[]);


void printMidCodeTable();       //输出自定义格式中间代码
void printMidCode(int o);            //输出规定格式中间代码  o为0表示未优化，o为1表示优化过了

void printOffsetTempTab();            //输出临时变量表


#endif