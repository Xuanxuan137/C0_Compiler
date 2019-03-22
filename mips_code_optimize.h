#ifndef COMPILER_MIPS_CODE_OPTIMIZE_H
#define COMPILER_MIPS_CODE_OPTIMIZE_H

#include "generate_mips_code_opt.h"



struct temp_mips_text_code {
    char instr[TOKENMAXLENGTH];
    char op1[TOKENMAXLENGTH];
    char op2[TOKENMAXLENGTH];
    char op3[TOKENMAXLENGTH];
    int effective;
};


void optimize_mips();           //优化mips指令
void del_mips_code();           //删除无用指令
void copyMipsTextCodeTable(int dst, int i, int j);

#endif //COMPILER_MIPS_CODE_OPTIMIZE_H
