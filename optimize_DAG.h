#ifndef OPTIMIZER_DAG_H
#define OPTIMIZER_DAG_H

#include "generate_mid_code.h"
#include "generate_mips_code.h"

#define NODE_TABLE_LENGTH 100000
#define DAG_LENGTH 100000


struct block {                  //基本块划分的结构
    int isStart;                            //标记某一条中间代码是否是一个基本块的开始
    int dst1;                               //记录某一条中间代码可能跳转到的位置，-1表示出口
    int dst2;                               //记录某一条中间代码可能跳转到的另一个位置，-1表示不存在第二条跳转位置
};

struct myNodeTable {            //DAG图结点表结构
    int index;                      //记录变量，正值表示局部/全局变量在符号表中的下标，负值表示临时变量的编号，0表示$ret
    int type;                       //类型，1表示变量，2表示常量
    int number;                     //结点号
    int assigned;                   //标记导出时是否赋值过
};

struct myDAG {                  //DAG图结构
    int index;                      //记录变量，正值表示局部/全局变量在符号表中的下标，负值表示临时变量的编号，0表示$ret
    int op;                         //0表示没有操作，1表示对变量右下角加0，2表示+，3表示-，4表示*，5表示/，6表示[]，7表示[]=，8表示这是一个常量
    int left;                       //左子树
    int right;                      //右子树
    int isInQueue;                  //在从DAG图导出中间代码时标记该结点是否已加入队列
};


void optimize_DAG();

void divideBlock();                                     //划分基本块

int findLabel(char labelNumber[]);                      //根据label查找label对应的中间代码下标（这里label虽然是字符串，但实际上只是一个整数）
void printBlockDivision();                              //输出基本块划分结果
void printDAG();                                        //输出DAG图

int findNextStart(int current_start);                   //根据当前基本块的开始下标查找下一基本块的开始下标
void drawDAG(int current_start, int next_start);        //构建DAG图消除局部公共子表达式
void DAGToMidCode();                                    //把DAG图重新转换为中间代码
int haveNodeNotInQueue();                               //查找DAG图中是否还有结点未加入队列
int chooseANode();                                      //在DAG图中选取一个结点来加入队列
int canAddToQueue(int i);                               //判断结点i是否可以加入队列
void dag_assign(int i);                                 //构建DAG图时处理赋值语句
void dag_array_assign(int i);                           //构建DAG图时处理数组赋值语句
void dag_plus(int i);                                   //构建DAG图时处理加法运算语句
void dag_minus(int i);                                  //构建DAG图时处理减法运算语句
void dag_mult(int i);                                   //构建DAG图时处理乘法运算语句
void dag_div(int i);                                    //构建DAG图时处理除法运算语句
void dag_array_get(int i);                              //构建DAG图时处理数组取值语句
int dealX(int i);                                       //在构建DAG图时处理z = x op y中的x，并返回x的结点号
int dealY(int i);                                       //在构建DAG图时处理z = x op y中的y，并返回y的结点号
int dealOP(int i, int op, int x_index, int y_index);    //在构建DAG图时处理z = x op y中的op，并返回op的结点号
void dealZ(int i, int op_index);                        //在构建DAG图时处理z = x op y中的z
int findInNodeTable(char name[]);                       //在节点表中查找name
int findInDAG(int op, int x_index, int y_index);        //在DAG图中查找op
int findArrayInDAG(int i, int op, int x_index, int y_index);    //在DAG图中查找6或7两种op
int loc_optimizeDAG(char name[]);                         //在tab中查找name
void copyMidCodeTable(int dst, int i, int j);           //复制符号表的某一项，在符号表和temp符号表之间复制
int isConstant(char name[]);                            //判断一个字符串是否代表一个常量(整数或字符)

#endif //COMPILER_OPTIMIZER_H
