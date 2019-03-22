#ifndef _GENERATE_MIPS_CODE_H_
#define _GENERATE_MIPS_CODE_H_

#include <stdio.h>
#include "lexical_analysis.h"
#include "generate_mid_code.h"
#include "grammar_analysis.h"

#define MIPS_DATA_LENGTH 1000
#define MIPS_TEXT_LENGTH 100000

struct mips_data_code {
    char ident[TOKENMAXLENGTH];
    char instr[20];
    char content[TOKENMAXLENGTH];
};

struct mips_text_code {
    char instr[TOKENMAXLENGTH];
    char op1[TOKENMAXLENGTH];
    char op2[TOKENMAXLENGTH];
    char op3[TOKENMAXLENGTH];
};


void generate_mips_code();
int tempVarToNum(char string[]);    //把临时变量中数值部分转为整数，如$100，则返回100
int is_letter(char ch);
int is_digit(char ch);


void generate_mips_code_3_4(int i);
void generate_mips_code_5_6(int i);
void generate_mips_code_7_8_9(int i);
void generate_mips_code_10_11(int i);
void generate_mips_code_12(int i);
void generate_mips_code_13(int i);
void generate_mips_code_14(int i);
void generate_mips_code_15(int i);
void generate_mips_code_16(int i);
void generate_mips_code_17(int i);
void generate_mips_code_18(int i);
void generate_mips_code_19(int i);
void generate_mips_code_20(int i);
void generate_mips_code_21(int i);
void generate_mips_code_22(int i);
void generate_mips_code_23(int i);
void generate_mips_code_24(int i);
void generate_mips_code_25(int i);
void generate_mips_code_26(int i);
void generate_mips_code_27(int i);
void generate_mips_code_28(int i);
void generate_mips_code_29(int i);
void generate_mips_code_30(int i);
void generate_mips_code_31(int i);
void generate_mips_code_32(int i);
void generate_mips_code_33(int i);
void generate_mips_code_34(int i);
void generate_mips_code_35(int i);
void generate_mips_code_36(int i);
void generate_mips_code_37(int i);
void generate_mips_code_38(int i);
void generate_mips_code_39(int i);
void generate_mips_code_40(int i);
void generate_mips_code_41(int i);

void src1ToRegister(char reg[], int i);             //把midCodeTable[i].src1代表的量存入寄存器reg
void src2ToRegister(char reg[], int i);             //把midCodeTable[i].src2代表的量存入寄存器reg
void registerToDst(char reg[], int i);              //把reg中的值存入midCodeTable[i].dst


int loc_mips(char name[]);                          //在符号表中查找，目的与loc相同
int loc_function(char name[]);                      //在符号表中查找函数

void print_mips_code_table();       //直接输出临时的mips

void print_mips_code(int o);     //输出可执行的mips代码
void printMipsDataCode(FILE * file);
void printMipsTextCode(FILE * file);

#endif