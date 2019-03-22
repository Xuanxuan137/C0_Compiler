#ifndef COMPILER_GENERATE_MIPS_CODE_OPT_H
#define COMPILER_GENERATE_MIPS_CODE_OPT_H



#include <stdio.h>
#include "lexical_analysis.h"
#include "generate_mid_code.h"
#include "grammar_analysis.h"
#include "generate_mips_code.h"


struct myReg_MemCompTable {
    int index;      //当type为1时，index>0表示该变量在符号表中的下标，index<0表示临时变量的编号，//当type为2时，index为常量值  //index=0表示$ret?$ret就应该在$v0里
    int type;       //type为1表示变量，type为2表示常量，0表示未使用
    int lastUse;    //记录上一次使用的时间（中间代码下标）
};


void generate_mips_code_opt();


void generate_mips_code_opt_3_4(int i);
void generate_mips_code_opt_5_6(int i);
void generate_mips_code_opt_7_8_9(int i);
void generate_mips_code_opt_10_11(int i);
void generate_mips_code_opt_12(int i);
void generate_mips_code_opt_13(int i);
void generate_mips_code_opt_14(int i);
void generate_mips_code_opt_15(int i);
void generate_mips_code_opt_16(int i);
void generate_mips_code_opt_17(int i);
void generate_mips_code_opt_18(int i);
void generate_mips_code_opt_19(int i);
void generate_mips_code_opt_20(int i);
void generate_mips_code_opt_21(int i);
void generate_mips_code_opt_22(int i);
void generate_mips_code_opt_23(int i);
void generate_mips_code_opt_24(int i);
void generate_mips_code_opt_25(int i);
void generate_mips_code_opt_26(int i);
void generate_mips_code_opt_27(int i);
void generate_mips_code_opt_28(int i);
void generate_mips_code_opt_29(int i);
void generate_mips_code_opt_30(int i);
void generate_mips_code_opt_31(int i);
void generate_mips_code_opt_32(int i);
void generate_mips_code_opt_33(int i);
void generate_mips_code_opt_35(int i);
void generate_mips_code_opt_36(int i);
void generate_mips_code_opt_37(int i);
void generate_mips_code_opt_38(int i);
void generate_mips_code_opt_39(int i);
void generate_mips_code_opt_40(int i);
void generate_mips_code_opt_41(int i);

void cleanRegister();                                               //清空临时寄存器，把寄存器里的数据存回内存
void src1ToV0_opt(int i);                                           //把src1存入$v0
int src2ToA0_opt(int i);                                            //把src2存入$a0
int src1ToRegister_opt(int i);                                      //把src1存入寄存器
int src2ToRegister_opt(int i);                                      //把src2存入寄存器
void registerToDst_opt(int reg, int i);                             //把reg复制到i的dst占用的寄存器
int findInRegister(char name[], int i);                             //查找name是否已经加载到寄存器
int findARegisterToReplace(char name[]);                            //寻找一个寄存器来存储值
int findTempRegister(char name[]);                                  //寻找一个临时寄存器
int findGlobalRegister(char name[]);                                //寻找一个全局寄存器
int findAllRegister(char name[]);                                   //寻找一个寄存器
void loadSrc1(int dst, int i);                                      //把midCodeTable[i].src1加载到寄存器dst
void loadSrc2(int dst, int i);                                      //把midCodeTable[i].src2加载到寄存器dst
void loadDst(int src, int dst, int i);                              //把src复制到dst
void getDst(int dst, int i);                                        //把midCodeTable[i].dst加载到寄存器dst

void print_mips_code_opt(int o);
void printMipsDataCode_opt(FILE * file);
void printMipsTextCode_opt(FILE * file);

void getRegNameByNum(int regNum, char regName[]);       //把寄存器编号转成寄存器名   0-9表示$t0-$t9, 10-17表示$s0-$s7
int loc_mips_opt(char name[]);

#endif //COMPILER_GENERATE_MIPS_CODE_OPT_H
