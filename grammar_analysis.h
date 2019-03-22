#ifndef _GRAMMAR_ANALYSIS_H_
#define _GRAMMAR_ANALYSIS_H_


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lexical_analysis.h"
#include "generate_mid_code.h"
#include "generate_mips_code.h"

#define TAB_LENGTH 1000
#define TEMP_TAB_LENGTH 100000

#define WAITING_OPERATION  (-1)  //这个值用于传递enterTab中的offset参数，在登录变量时使用，表示不直接传入，等待在enterTab中计算

//符号表中object部分的值
#define CONST_TAB_VALUE 1
#define VAR_TAB_VALUE 2
#define ARRAY_TAB_VALUE 3
#define FUNC_TAB_VALUE 4
#define PARA_TAB_VALUE 5

//符号表中type部分的值
#define INT_TAB_VALUE 1
#define CHAR_TAB_VALUE 2
#define VOID_TAB_VALUE 3

//item中type的值
#define INT_ITEM_TYPE 1
#define CHAR_ITEM_TYPE 2
#define VOID_ITEM_TYPE 3
#define ARRAY_ITEM_TYPE 4
#define FUNC_ITEM_TYPE 5


//定义存储符号表的结构
struct myTab {
    char name[TOKENMAXLENGTH];    //标识符名字，
    int link;       //指向同一函数中上一个标识符在tab表中的位置，每个函数在tab表中登录的第一个标识符（即函数名）的link值为上一个函数函数名的地址，第一个函数函数名的link值为最后一个全局变量或常量的位置
    int object;     //标识符种类，包括const, variable, array, function
    int type;       //标识符类型，包括int, char, void
    int size;       //如果是常量或变量，记录所占空间；如果是数组，记录数组元素个数；如果是函数，记录参数个数（为运算方便，int和char均为4字节）
    int offset;     //记录该标识符在运行时相对于abp的偏移量（单位：Byte）；如果是常量，记录常量值
};



struct conditionItem {      //用于存储condition返回的内容
    char src1[TOKENMAXLENGTH];
    int op;
    char src2[TOKENMAXLENGTH];
};

struct item {
    int type;       //1 for int, 2 for char, 3 for void, 4 for array(pointer)(illegal, should not be used), 5 for func
    char identifier[TOKENMAXLENGTH];
};


int  grammar_analysis();                                //语法分析

void program();                                         //处理程序
void constDeclaration();                                //常量说明
void constDefinition();                                 //常量定义
void variableDeclaration();                             //变量说明
void variableDefinition();                              //变量定义
void functionWithReturnValueDeclaration();              //有返回值函数定义
void functionWithoutReturnValueDeclaration();           //无返回值函数定义
void mainFunction();                                    //主函数
void parameterTable(char name[], int funcType);         //参数表处理，负责将函数名登录符号表
void compoundStatement();                               //复合语句
void statementColumn();                                 //语句列
void statement();                                       //语句
void ifStatement();                                     //条件语句
void whileStatement();                                  //循环语句
void scanfStatement();                                  //读语句
void printfStatement();                                 //写语句
void blankStatement();                                  //空语句
void switchStatement();                                 //情况语句
void returnStatement();                                 //返回语句
void assignStatement(char name[]);                      //赋值语句
void functionCall(char name[], int from);                         //函数调用
void situationTable(struct item*x, int*lastCaseIndex, int endCaseIndex[], int*caseNum);                 //情况表
void defaultSituation(struct item*x, int*lastCaseIndex, int endCaseIndex[], int*caseNum);               //缺省
void situationSubStatement(struct item*x, int*lastCaseIndex, int endCaseIndex[], int*caseNum);          //情况子语句
void constant(char constantX[]);                        //常量
void condition(struct conditionItem * ret);             //条件
void valueParameterTable(char name[]);                  //值参数表
void expression(struct item * x);                       //表达式
void term(struct item * x);                             //项
void factor(struct item * x);                           //因子
void recordTempVarType(struct item*x);

void recordCaseValue(char constantX[]);                 //记录case后常量的值以避免重复
void newTempVariable(char dst[]);                       //在处理表达式的过程中生成一个临时变量
int stringIsNum(char id[]);                             //判断string是否是纯数
int is_integer(char id[]);                              //判断string是否代表一个整数


int calculateIfOp(int op);                              //根据op计算中间代码中if语句使用哪一种op
int calculateWhileOp(int op);                           //根据op计算中间代码中while语句使用哪一种op


int symbolInStatementStartSet();                        //判断当前symbol是否在<语句>的first集里面
int symbolInExpression();                               //判断当前symbol是否在<表达式>的first集里面

int constToInt(char constantX[]);                       //把常量转为整数


/*
 * 在enterTab函数中
 * name表示登录符号表的 变量|常量|函数|数组 的名字
 * link指向同一函数中上一个标识符在tab表中的位置，每个函数在tab表中登录的第一个标识符（即函数名）的link值为上一个函数函数名的位置，
 *      第一个函数函数名的link值为最后一个全局变量或常量的位置（对每一个函数来说，可见的有函数内部声明的变量/常量，前面声明的函数，和全局变量/常量）
 *          在将函数登录符号表示，link值不直接传入，而是在enterTab内计算得出
 * object为标识符种类，其中const，variable，array，function, parameter依次为1，2，3，4, 5
 * type为标识符类型，其中int，char，void依次为1，2，3
 * size:如果是常量或变量，记录所占空间；如果是数组，记录数组元素个数；如果是函数，记录参数个数（为运算方便，int和char均为4字节）
 * offset:记录该标识符在运行时相对于abp的偏移量（单位：Byte）；如果是常量，记录常量值  (在将变量登录符号表时，不直接传入offset，而是在enterTab内计算得出)
 *          （计算方法：如果是function，offset为0
 *                    如果是变量（参数），向上寻找最近的上一个函数或变量（参数），
 *                          如果找到的是变量（参数），在上一个变量（参数）的offset的基础上加上上一个变量（参数）所占空间，作为自己的offset
 *                          如果找到的是function，offset置为8
 */
void enterTab(char name[], int link, int object, int type, int size, int offset);       //登录符号表
int loc(char name[]);                                                                   //在符号表中查找name的位置

/*
 * 用于声明时查找是否重复声明，
 * 对于函数声明，查找其前面的函数和全局变量/常量
 * 对于变量/常量(参数)声明，查找同一作用域的变量/常量(参数)/函数
 */
int loc1(char name[], int object);


void printTab();


#endif
