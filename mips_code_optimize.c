#include "mips_code_optimize.h"


extern struct mips_text_code mipsTextCodeTable_opt[MIPS_TEXT_LENGTH];

extern int text_top_opt;       //text区指令栈顶指针

struct temp_mips_text_code tempMipsTextCodeTable[MIPS_TEXT_LENGTH];     //临时mips text指令表
int temp_text_top;                                                      //临时mips text指令表栈顶指针

void optimize_mips()
{
    //第一步，把mips指令从mipsTextCodeTable_opt复制到tempMipsTextCodeTable，并把effective初始化为1
    int i;
    for(i = 1; i<=text_top_opt; i++) {
        copyMipsTextCodeTable(1, i, i);
    }
    temp_text_top = text_top_opt;
    for(i = 1; i<=temp_text_top; i++) {
        tempMipsTextCodeTable[i].effective = 1;
    }

    //第二步，删除，合并无用代码
    del_mips_code();

    //第三步，把优化后的代码写回mipsTextCodeTable_opt
    int j = 0;
    for(i = 1; i<=temp_text_top; i++) {
        if(tempMipsTextCodeTable[i].effective == 1) {
            j++;
            copyMipsTextCodeTable(0, j, i);
        }
    }
    text_top_opt = j;
}

void del_mips_code()
{
    //删除连续的$fp递减操作
    int i;
    for(i = 1; i<=temp_text_top; i++) {
        if(tempMipsTextCodeTable[i].effective == 0) {
            continue;
        }
        if(strcmp(tempMipsTextCodeTable[i].instr, "addiu") == 0 &&
                strcmp(tempMipsTextCodeTable[i].op1, "$fp") == 0 &&
                strcmp(tempMipsTextCodeTable[i].op2, "$fp") == 0 &&
                is_digit(tempMipsTextCodeTable[i].op3[0])) {                //如果指令i是addi $fp, $fp, num
            int sum = constToInt(tempMipsTextCodeTable[i].op3);             //记录指令i的num
            int j;
            for(j = i+1; j<=temp_text_top; j++) {       //遍历i之后的指令
                if(strcmp(tempMipsTextCodeTable[j].instr, "addiu") == 0 &&
                   strcmp(tempMipsTextCodeTable[j].op1, "$fp") == 0 &&
                   strcmp(tempMipsTextCodeTable[j].op2, "$fp") == 0 &&
                   is_digit(tempMipsTextCodeTable[j].op3[0])) {             //如果指令j是addi $fp, $fp, num
                    sum += constToInt(tempMipsTextCodeTable[j].op3);        //在原来的sum的基础上加上指令j的num
                    tempMipsTextCodeTable[j].effective = 0;                 //删除指令j
                }
                else {
                    break;
                }
            }
            sprintf(tempMipsTextCodeTable[i].op3, "%d", sum);       //把求得的和写入指令i的num
        }
    }
}

void copyMipsTextCodeTable(int dst, int i, int j)
{
    //dst=0为从临时表到正式表
    //dst=1为从正式表到临时表
    //i是正式表下标
    //j是临时表下标
    if(dst == 0) {
        sprintf(mipsTextCodeTable_opt[i].instr, "%s", tempMipsTextCodeTable[j].instr);
        sprintf(mipsTextCodeTable_opt[i].op1, "%s", tempMipsTextCodeTable[j].op1);
        sprintf(mipsTextCodeTable_opt[i].op2, "%s", tempMipsTextCodeTable[j].op2);
        sprintf(mipsTextCodeTable_opt[i].op3, "%s", tempMipsTextCodeTable[j].op3);
    }
    else {
        sprintf(tempMipsTextCodeTable[j].instr, "%s", mipsTextCodeTable_opt[i].instr);
        sprintf(tempMipsTextCodeTable[j].op1, "%s", mipsTextCodeTable_opt[i].op1);
        sprintf(tempMipsTextCodeTable[j].op2, "%s", mipsTextCodeTable_opt[i].op2);
        sprintf(tempMipsTextCodeTable[j].op3, "%s", mipsTextCodeTable_opt[i].op3);
    }
}