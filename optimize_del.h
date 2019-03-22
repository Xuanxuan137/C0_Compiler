#ifndef COMPILER_OPTIMIZE_DEL_H
#define COMPILER_OPTIMIZE_DEL_H

#include "optimize_DAG.h"


void optimize_del();                                                //删除中间代码中的无用代码并且重新生成中间代码
void divide_block_del_temp();                                            //划分基本块
void divide_block_del_var();
void del_mid_code_temp(int current_start, int next_start);               //删除多余中间代码
void del_mid_code_var(int current_start, int next_start);
void const_replace(int current_start, int next_start);              //把const常量直接替换成常量值
void del_calc_const(int current_start, int next_start);             //计算常量
void del_tempVar(int current_start, int next_start);                //尽可能删除临时变量
void del_not_used(int current_start, int next_start);               //删除赋值过但没用过的临时变量
void del_var_replace(int current_start, int next_start);            //正经变量替换为常量
void del_var_not_used(int current_start, int next_start);           //删去变量连续两次赋值中的前一次
int findEnd(int current_start);
int canAssign(int j);       //检测tempMidCodeTable[j]是否是一个对单独变量赋值的语句。单独变量指不是数组元素的变量
int canReplace(int j);      //检测tempMidCodeTable[j]是否是一个可以替换src1，src2的语句
int isAssign(int j);        //检测tempMidCodeTable[j]是否可以让dst获得一个新值
int loc_optimizeDel(char name[]);          //返回name在符号表中的下标      //在tab中查找name






#endif //COMPILER_OPTIMIZE_DEL_H
