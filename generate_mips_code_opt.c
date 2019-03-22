#include "generate_mips_code_opt.h"


extern int global_top_in_mid;       //记录最后一个全局量在中间代码中的位置
extern struct mid_code midCodeTable[MID_CODE_TABLE_LENGTH];
extern int mid_code_top;            //中间代码栈顶指针
extern int tempVarNum;              //临时变量个数
extern struct myTab tab[TAB_LENGTH];    //符号表
extern int tempTab[TEMP_TAB_LENGTH];    //临时变量表
extern struct myTempTab offsetTempTab[OFFSET_TEMP_TAB_LENGTH];      //临时变量偏移表
extern int offsetTempTabTop;                                        //临时变量偏移表栈顶指针
extern int globalTop;      //记录最后一个全局变量/常量在符号表中的位置
extern int tab_top;          //符号表栈顶指针

struct mips_data_code mipsDataCodeTable_opt[MIPS_DATA_LENGTH];
struct mips_text_code mipsTextCodeTable_opt[MIPS_TEXT_LENGTH];


int data_top_opt = 0;       //data区指令栈顶指针
int text_top_opt = 0;       //text区指令栈顶指针

int stringNum_opt = 0;      //记录printf中string的编号

char tempFunction_opt[TOKENMAXLENGTH] = "";

struct myReg_MemCompTable reg_memCompTable[18];    //寄存器-内存对照表 //下标0-9表示$t0-$t9, 10-17表示$s0-$s7

int labelNeedCleanReg[MID_CODE_TABLE_LENGTH];

void generate_mips_code_opt()
{
    data_top_opt = 0;
    text_top_opt = 0;
    stringNum_opt = 0;
    int j;
    for(j = 0; j<18; j++) {     //初始化寄存器-内存对照表
        reg_memCompTable[j].type = 0;
        reg_memCompTable[j].lastUse = 0;
    }
    for(j = 0; j<MID_CODE_TABLE_LENGTH; j++) {
        labelNeedCleanReg[j] = 0;
    }

    //为全局变量申请空间
    int space_for_global = 0;
    if(tab[globalTop].object == CONST_TAB_VALUE || tab[globalTop].object == VAR_TAB_VALUE) {
        space_for_global = tab[globalTop].offset + 4;
    }
    else if(tab[globalTop].object == ARRAY_TAB_VALUE) {
        space_for_global = tab[globalTop].offset + 4*tab[globalTop].size;
    }
    data_top_opt++;
    strcpy(mipsDataCodeTable_opt[data_top_opt].ident, "$global");
    strcpy(mipsDataCodeTable_opt[data_top_opt].instr, ".space");
    sprintf(mipsDataCodeTable_opt[data_top_opt].content, "%d", space_for_global+100);           //$global: .space   space_for_global+100

    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "la");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$gp");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "$global");                     //la $gp, $global       //把全局变量起始地址存入$gp

    //为换行符申请一个空间，用于在每个printf后输出
    data_top_opt++;
    strcpy(mipsDataCodeTable_opt[data_top_opt].ident, "$newLine");
    strcpy(mipsDataCodeTable_opt[data_top_opt].instr, ".asciiz");
    strcpy(mipsDataCodeTable_opt[data_top_opt].content, "\"\\n\"");  //$newLine: .asciiz "\n"

    int i = global_top_in_mid + 1;

    //生成全局变量后面部分的指令
    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "move");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$fp");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "$sp");     //move $fp, $sp     //程序开始第一步，把fp置为sp
    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "jal");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "main");    //jal main            //第二步，跳到main
    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "li");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$v0");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "10");      //li $v0, 10
    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "syscall");   //syscall   结束程序

    for(; i<=mid_code_top; i++) {        //依次分析每一条中间代码
        if(midCodeTable[i].op == WHILE_START_OP ||
                midCodeTable[i].op == IF_LESS ||
                midCodeTable[i].op == IF_LESSEQUAL ||
                midCodeTable[i].op == IF_MORE ||
                midCodeTable[i].op == IF_MOREEQUAL ||
                midCodeTable[i].op == IF_NOTEQUAL ||
                midCodeTable[i].op == IF_EQUAL ||
                midCodeTable[i].op == WHILE_LESS ||
                midCodeTable[i].op == WHILE_LESSEQUAL ||
                midCodeTable[i].op == WHILE_MORE ||
                midCodeTable[i].op == WHILE_MOREEQUAL ||
                midCodeTable[i].op == WHILE_NOTEQUAL ||
                midCodeTable[i].op == WHILE_EQUAL ||
                midCodeTable[i].op == CASE_OP/* ||
                midCodeTable[i].op == SCANF_OP*/) {
            cleanRegister();
        }

        if(midCodeTable[i].op == IF_LESS ||
           midCodeTable[i].op == IF_LESSEQUAL ||
           midCodeTable[i].op == IF_MORE ||
           midCodeTable[i].op == IF_MOREEQUAL ||
           midCodeTable[i].op == IF_NOTEQUAL ||
           midCodeTable[i].op == IF_EQUAL) {
            labelNeedCleanReg[constToInt(midCodeTable[i].dst)] = 1;
        }

        if(midCodeTable[i].label != 0 && labelNeedCleanReg[midCodeTable[i].label] == 1) {
            cleanRegister();
        }

        //第一步，生成label
        if(midCodeTable[i].label != 0) {
            text_top_opt++;
            if(midCodeTable[i].op == FUNCINT || midCodeTable[i].op == FUNCCHAR || midCodeTable[i].op == FUNCVOID) {
                sprintf(mipsTextCodeTable_opt[text_top_opt].instr, "%s:", midCodeTable[i].dst); //如果是函数声明，以函数名作为label
                strcpy(tempFunction_opt, midCodeTable[i].dst);        //记录当前函数名，以便查找变量/常量
            }
            else {
                sprintf(mipsTextCodeTable_opt[text_top_opt].instr, "$label%d:", midCodeTable[i].label); //普通label以label加labelNum为label
            }
        }

        //第二步，判断op，根据op输出后面部分
        if(midCodeTable[i].op == 0) {       //NO_OP
            //无操作不生成指令
        }
        else if(midCodeTable[i].op == 1 || midCodeTable[i].op == 2) {   //CONSTINT CONSTCHAR
            //常量声明不生成指令
        }
        else if(midCodeTable[i].op == 3 || midCodeTable[i].op == 4) {   //VARINT VARCHAR
            generate_mips_code_opt_3_4(i);
        }
        else if(midCodeTable[i].op == 5 || midCodeTable[i].op == 6) {   //ARRAYINT ARRAYCHAR
            generate_mips_code_opt_5_6(i);
        }
        else if(midCodeTable[i].op == 7 || midCodeTable[i].op == 8 || midCodeTable[i].op == 9) { //FUNCINT FUNCCHAR FUNCVOID
            generate_mips_code_opt_7_8_9(i);
        }
        else if(midCodeTable[i].op == 10 || midCodeTable[i].op == 11) { //PARAINT PARACHAR
            generate_mips_code_opt_10_11(i);
        }
        else if(midCodeTable[i].op == 12) {     //RET
            generate_mips_code_opt_12(i);
        }
        else if(midCodeTable[i].op == 13) {     //IF_LESS
            generate_mips_code_opt_13(i);
        }
        else if(midCodeTable[i].op == 14) {     //IF_LESSEQUAL
            generate_mips_code_opt_14(i);
        }
        else if(midCodeTable[i].op == 15) {     //IF_MORE
            generate_mips_code_opt_15(i);
        }
        else if(midCodeTable[i].op == 16) {     //IF_MOREEQUAL
            generate_mips_code_opt_16(i);
        }
        else if(midCodeTable[i].op == 17) {     //IF_NOTEQUAL
            generate_mips_code_opt_17(i);
        }
        else if(midCodeTable[i].op == 18) {     //IF_EQUAL
            generate_mips_code_opt_18(i);
        }
        else if(midCodeTable[i].op == 19) {     //GOTO
            generate_mips_code_opt_19(i);
        }
        else if(midCodeTable[i].op == 20) {     //WHILE_LESS
            generate_mips_code_opt_20(i);
        }
        else if(midCodeTable[i].op == 21) {     //WHILE_LESSEQUAL
            generate_mips_code_opt_21(i);
        }
        else if(midCodeTable[i].op == 22) {     //WHILE_MORE
            generate_mips_code_opt_22(i);
        }
        else if(midCodeTable[i].op == 23) {     //WHILE_MOREEQUAL
            generate_mips_code_opt_23(i);
        }
        else if(midCodeTable[i].op == 24) {     //WHILE_NOTEQUAL
            generate_mips_code_opt_24(i);
        }
        else if(midCodeTable[i].op == 25) {     //WHILE_EQUAL
            generate_mips_code_opt_25(i);
        }
        else if(midCodeTable[i].op == 26) {     //SCANF
            generate_mips_code_opt_26(i);
        }
        else if(midCodeTable[i].op == 27) {     //PRINTF_OP1
            generate_mips_code_opt_27(i);
        }
        else if(midCodeTable[i].op == 28) {     //PRINTF_OP2
            generate_mips_code_opt_28(i);
        }
        else if(midCodeTable[i].op == 29) {     //PRINTF_OP3
            generate_mips_code_opt_29(i);
        }
        else if(midCodeTable[i].op == 30) {     //ASSIGN
            generate_mips_code_opt_30(i);
        }
        else if(midCodeTable[i].op == 31) {     //ARRAY_ASSIGN
            generate_mips_code_opt_31(i);
        }
        else if(midCodeTable[i].op == 32) {     //CALL
            generate_mips_code_opt_32(i);
        }
        else if(midCodeTable[i].op == 33) {     //CASE
            generate_mips_code_opt_33(i);
        }
        else if(midCodeTable[i].op == 34) {     //DEFAULT
            //default行不生成指令
        }
        else if(midCodeTable[i].op == 35) {     //PUSH
            generate_mips_code_opt_35(i);
        }
        else if(midCodeTable[i].op == 36) {     //PLUS
            generate_mips_code_opt_36(i);
        }
        else if(midCodeTable[i].op == 37) {     //MINUS
            generate_mips_code_opt_37(i);
        }
        else if(midCodeTable[i].op == 38) {     //MULTIPLY
            generate_mips_code_opt_38(i);
        }
        else if(midCodeTable[i].op == 39) {     //DIVIDE
            generate_mips_code_opt_39(i);
        }
        else if(midCodeTable[i].op == 40) {     //ARRAY_GET
            generate_mips_code_opt_40(i);
        }
        else if(midCodeTable[i].op == 41) {     //RET_EXTRA
            generate_mips_code_opt_41(i);
        }
    }

}

void generate_mips_code_opt_3_4(int i)
{
    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "addiu");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$fp");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "$fp");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "-4");      //add $fp, $fp, -4  为变量分配空间
}

void generate_mips_code_opt_5_6(int i)
{
    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "addiu");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$fp");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "$fp");
    int size = stringToNum(midCodeTable[i].src1);       //把中间代码中存储的字符串格式的数组长度转为整数
    sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "-%d", size*4);    //add $fp, $fp, size*4  为数组分配空间
}

void generate_mips_code_opt_7_8_9(int i)
{
//    cleanRegister();        //到了一个新的函数，说明上一个函数结束了，清空寄存器

    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "sw");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$ra");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "0");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "$fp");     //sw $ra, 0($fp)
    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "sw");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$sp");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "-4");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "$fp");     //sw $sp, -4($fp)
    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "move");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$sp");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "$fp");     //move $sp, $fp
    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "addiu");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$fp");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "$fp");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "-8");      //addi $fp, $fp, -8
    //提前为临时变量预留空间
    int j = locTemp(midCodeTable[i].dst);   //在临时变量偏移表中查找函数
    int num = 0;
    for(j++; j<=offsetTempTabTop; j++) {
        if(offsetTempTab[j].name[0] != '$') {       //从当前函数开始一直找到下一个函数或栈顶
            break;
        }
        num++;
    }
    //此时num为该函数的临时变量个数
    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "addiu");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$fp");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "$fp");
    sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "-%d", num*4+4);       //addi $fp, $fp, -(num*4+4)  //+4是为了多分一点保险
}

void generate_mips_code_opt_10_11(int i)
{
    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "addiu");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$fp");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "$fp");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "-4");      //add $fp, $fp, -4  为参数分配空间
}

void generate_mips_code_opt_12(int i)
{
    //首先把要返回的值存在$v0里
    src1ToV0_opt(i);
    //清理寄存器
    cleanRegister();
    //存好后，对sp和fp进行调整，以便返回
    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "lw");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$ra");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "0");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "$sp");     //lw $ra, 0($sp)
    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "move");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$fp");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "$sp");     //move, $fp, $sp
    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "lw");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$sp");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "-4");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "$sp");     //lw $sp, -4($sp)
    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "jr");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$ra");     //jr $ra
}

void generate_mips_code_opt_13(int i)
{
    int src1_is_Const = 0;
    int src2_is_Const = 0;
    if(is_digit(midCodeTable[i].src1[0]) || midCodeTable[i].src1[0] == '\'') {
        src1_is_Const = 1;                      //如果src1是常量值
    }
    if(is_digit(midCodeTable[i].src2[0]) || midCodeTable[i].src2[0] == '\'') {
        src2_is_Const = 1;                      //如果src2是常量值
    }

    if(src1_is_Const == 1 && src2_is_Const == 0) {          //如果只有src1是常量
        int src2Reg = src2ToRegister_opt(i);
        char src2RegName[10];
        getRegNameByNum(src2Reg, src2RegName);
        //然后用src2-src1，存在$t8里
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "addiu");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src2RegName);
        sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "%d", -constToInt(midCodeTable[i].src1));     //addiu $t8, src2, -src1
        //由于if中的条件符号为<，且这里用src2-src1
        //所以当t8<=0时跳转，t8>0时不跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "blez");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "$label%s", midCodeTable[i].dst);   //blez $t8, label
    }
    else if(src1_is_Const == 0 && src2_is_Const == 1) {
        int src1Reg = src1ToRegister_opt(i);
        char src1RegName[10];
        getRegNameByNum(src1Reg, src1RegName);
        //然后用$src1Reg-$src2Reg，存在$t8里
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "addiu");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src1RegName);
        sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "%d", -constToInt(midCodeTable[i].src2));         //addiu $t8, src1, -src2
        //由于if中的条件符号为<
        //所以当t8小于0时不跳转，t8>=0时跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "bgez");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "$label%s", midCodeTable[i].dst);   //bgez $t8, label
    }
    else if(src1_is_Const == 1 && src2_is_Const == 1) {
        int src1 = constToInt(midCodeTable[i].src1);
        int src2 = constToInt(midCodeTable[i].src2);
        int result = src1 - src2;
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "li");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "%d", result);
        //由于if中的条件符号为<
        //所以当t8小于0时不跳转，t8>=0时跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "bgez");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "$label%s", midCodeTable[i].dst);   //bgez $t8, label
    }
    else {
        //先取src1的值,记录存储的寄存器
        int src1Reg = src1ToRegister_opt(i);
        //再取src2的值,记录存储的寄存器
        int src2Reg = src2ToRegister_opt(i);
        //把寄存器从编号转为名称
        char src1RegName[10];
        char src2RegName[10];
        getRegNameByNum(src1Reg, src1RegName);
        getRegNameByNum(src2Reg, src2RegName);
        //然后用$src1Reg-$src2Reg，存在$t8里
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "subu");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src1RegName);
        strcpy(mipsTextCodeTable_opt[text_top_opt].op3, src2RegName);         //sub $t8, src1RegName, src2RegName
        //由于if中的条件符号为<
        //所以当t8小于0时不跳转，t8>=0时跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "bgez");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "$label%s", midCodeTable[i].dst);   //bgez $t8, label
    }
}

void generate_mips_code_opt_14(int i)
{
    int src1_is_Const = 0;
    int src2_is_Const = 0;
    if(is_digit(midCodeTable[i].src1[0]) || midCodeTable[i].src1[0] == '\'') {
        src1_is_Const = 1;                      //如果src1是常量值
    }
    if(is_digit(midCodeTable[i].src2[0]) || midCodeTable[i].src2[0] == '\'') {
        src2_is_Const = 1;                      //如果src2是常量值
    }

    if(src1_is_Const == 1 && src2_is_Const == 0) {          //如果只有src1是常量
        int src2Reg = src2ToRegister_opt(i);
        char src2RegName[10];
        getRegNameByNum(src2Reg, src2RegName);
        //然后用src2-src1，存在$t8里
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "addiu");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src2RegName);
        sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "%d", -constToInt(midCodeTable[i].src1));     //addiu $t8, src2, -src1
        //由于if中的条件符号为<=，且src2-src1
        //所以当t8<0时跳转，t8>=0时不跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "bltz");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "$label%s", midCodeTable[i].dst);   //blez $t8, label
    }
    else if(src1_is_Const == 0 && src2_is_Const == 1) {
        int src1Reg = src1ToRegister_opt(i);
        char src1RegName[10];
        getRegNameByNum(src1Reg, src1RegName);
        //然后用$src1Reg-$src2Reg，存在$t8里
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "addiu");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src1RegName);
        sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "%d", -constToInt(midCodeTable[i].src2));         //addiu $t8, src1, -src2
        //由于if中的条件符号为<=
        //所以当t8<=0时不跳转，t8>0时跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "bgtz");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "$label%s", midCodeTable[i].dst);   //bgez $t8, label
    }
    else if(src1_is_Const == 1 && src2_is_Const == 1) {
        int src1 = constToInt(midCodeTable[i].src1);
        int src2 = constToInt(midCodeTable[i].src2);
        int result = src1 - src2;
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "li");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "%d", result);
        //由于if中的条件符号为<=
        //所以当t8<=0时不跳转，t8>0时跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "bgtz");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "$label%s", midCodeTable[i].dst);   //bgez $t8, label
    }
    else {
        //先取src1的值,记录存储的寄存器
        int src1Reg = src1ToRegister_opt(i);
        //再取src2的值,记录存储的寄存器
        int src2Reg = src2ToRegister_opt(i);
        //把寄存器从编号转为名称
        char src1RegName[10];
        char src2RegName[10];
        getRegNameByNum(src1Reg, src1RegName);
        getRegNameByNum(src2Reg, src2RegName);
        //然后用$src1Reg-$src2Reg，存在$t8里
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "subu");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src1RegName);
        strcpy(mipsTextCodeTable_opt[text_top_opt].op3, src2RegName);         //sub $t8, src1RegName, src2RegName
        //由于if中的条件符号为<=
        //所以当t8<=0时不跳转，t8>0时跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "bgtz");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "$label%s", midCodeTable[i].dst);   //bgez $t8, label
    }
}

void generate_mips_code_opt_15(int i)
{
    int src1_is_Const = 0;
    int src2_is_Const = 0;
    if(is_digit(midCodeTable[i].src1[0]) || midCodeTable[i].src1[0] == '\'') {
        src1_is_Const = 1;                      //如果src1是常量值
    }
    if(is_digit(midCodeTable[i].src2[0]) || midCodeTable[i].src2[0] == '\'') {
        src2_is_Const = 1;                      //如果src2是常量值
    }

    if(src1_is_Const == 1 && src2_is_Const == 0) {          //如果只有src1是常量
        int src2Reg = src2ToRegister_opt(i);
        char src2RegName[10];
        getRegNameByNum(src2Reg, src2RegName);
        //然后用src2-src1，存在$t8里
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "addiu");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src2RegName);
        sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "%d", -constToInt(midCodeTable[i].src1));     //addiu $t8, src2, -src1
        //由于if中的条件符号为>，且src2-src1
        //所以当t8>=0时跳转，t8<0时不跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "bgez");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "$label%s", midCodeTable[i].dst);   //blez $t8, label
    }
    else if(src1_is_Const == 0 && src2_is_Const == 1) {
        int src1Reg = src1ToRegister_opt(i);
        char src1RegName[10];
        getRegNameByNum(src1Reg, src1RegName);
        //然后用$src1Reg-$src2Reg，存在$t8里
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "addiu");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src1RegName);
        sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "%d", -constToInt(midCodeTable[i].src2));         //addiu $t8, src1, -src2
        //由于if中的条件符号为>
        //所以当t8>0时不跳转，t8<=0时跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "blez");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "$label%s", midCodeTable[i].dst);   //bgez $t8, label
    }
    else if(src1_is_Const == 1 && src2_is_Const == 1) {
        int src1 = constToInt(midCodeTable[i].src1);
        int src2 = constToInt(midCodeTable[i].src2);
        int result = src1 - src2;
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "li");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "%d", result);
        //由于if中的条件符号为>
        //所以当t8>0时不跳转，t8<=0时跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "blez");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "$label%s", midCodeTable[i].dst);   //bgez $t8, label
    }
    else {
        //先取src1的值,记录存储的寄存器
        int src1Reg = src1ToRegister_opt(i);
        //再取src2的值,记录存储的寄存器
        int src2Reg = src2ToRegister_opt(i);
        //把寄存器从编号转为名称
        char src1RegName[10];
        char src2RegName[10];
        getRegNameByNum(src1Reg, src1RegName);
        getRegNameByNum(src2Reg, src2RegName);
        //然后用$src1Reg-$src2Reg，存在$t8里
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "subu");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src1RegName);
        strcpy(mipsTextCodeTable_opt[text_top_opt].op3, src2RegName);         //sub $t8, src1RegName, src2RegName
        //由于if中的条件符号为>
        //所以当t8>0时不跳转，t8<=0时跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "blez");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "$label%s", midCodeTable[i].dst);   //bgez $t8, label
    }
}

void generate_mips_code_opt_16(int i)
{
    int src1_is_Const = 0;
    int src2_is_Const = 0;
    if(is_digit(midCodeTable[i].src1[0]) || midCodeTable[i].src1[0] == '\'') {
        src1_is_Const = 1;                      //如果src1是常量值
    }
    if(is_digit(midCodeTable[i].src2[0]) || midCodeTable[i].src2[0] == '\'') {
        src2_is_Const = 1;                      //如果src2是常量值
    }

    if(src1_is_Const == 1 && src2_is_Const == 0) {          //如果只有src1是常量
        int src2Reg = src2ToRegister_opt(i);
        char src2RegName[10];
        getRegNameByNum(src2Reg, src2RegName);
        //然后用src2-src1，存在$t8里
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "addiu");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src2RegName);
        sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "%d", -constToInt(midCodeTable[i].src1));     //addiu $t8, src2, -src1
        //由于if中的条件符号为>=，且src2-src1
        //所以当t8>0时跳转，t8<=0时不跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "bgtz");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "$label%s", midCodeTable[i].dst);   //blez $t8, label
    }
    else if(src1_is_Const == 0 && src2_is_Const == 1) {
        int src1Reg = src1ToRegister_opt(i);
        char src1RegName[10];
        getRegNameByNum(src1Reg, src1RegName);
        //然后用$src1Reg-$src2Reg，存在$t8里
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "addiu");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src1RegName);
        sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "%d", -constToInt(midCodeTable[i].src2));         //addiu $t8, src1, -src2
        //由于if中的条件符号为>=
        //所以当t8>=0时不跳转，t8<0时跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "bltz");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "$label%s", midCodeTable[i].dst);   //bgez $t8, label
    }
    else if(src1_is_Const == 1 && src2_is_Const == 1) {
        int src1 = constToInt(midCodeTable[i].src1);
        int src2 = constToInt(midCodeTable[i].src2);
        int result = src1 - src2;
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "li");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "%d", result);
        //由于if中的条件符号为>=
        //所以当t8>=0时不跳转，t8<0时跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "bltz");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "$label%s", midCodeTable[i].dst);   //bgez $t8, label
    }
    else {
        //先取src1的值,记录存储的寄存器
        int src1Reg = src1ToRegister_opt(i);
        //再取src2的值,记录存储的寄存器
        int src2Reg = src2ToRegister_opt(i);
        //把寄存器从编号转为名称
        char src1RegName[10];
        char src2RegName[10];
        getRegNameByNum(src1Reg, src1RegName);
        getRegNameByNum(src2Reg, src2RegName);
        //然后用$src1Reg-$src2Reg，存在$t8里
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "subu");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src1RegName);
        strcpy(mipsTextCodeTable_opt[text_top_opt].op3, src2RegName);         //sub $t8, src1RegName, src2RegName
        //由于if中的条件符号为>=
        //所以当t8>=0时不跳转，t8<0时跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "bltz");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "$label%s", midCodeTable[i].dst);   //bgez $t8, label
    }
}

void generate_mips_code_opt_17(int i)
{
    int src1_is_Const = 0;
    int src2_is_Const = 0;
    if(is_digit(midCodeTable[i].src1[0]) || midCodeTable[i].src1[0] == '\'') {
        src1_is_Const = 1;                      //如果src1是常量值
    }
    if(is_digit(midCodeTable[i].src2[0]) || midCodeTable[i].src2[0] == '\'') {
        src2_is_Const = 1;                      //如果src2是常量值
    }

    if(src1_is_Const == 1 && src2_is_Const == 0) {          //如果只有src1是常量
        int src2Reg = src2ToRegister_opt(i);
        char src2RegName[10];
        getRegNameByNum(src2Reg, src2RegName);
        //然后用src2-src1，存在$t8里
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "addiu");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src2RegName);
        sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "%d", -constToInt(midCodeTable[i].src1));     //addiu $t8, src2, -src1
        //由于if中的条件符号为!=，且$t8=src2-src1
        //所以当$t8!=0时不跳转，$t8==0时跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "beq");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "$0");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "$label%s", midCodeTable[i].dst);   //bgez $t8, label
    }
    else if(src1_is_Const == 0 && src2_is_Const == 1) {
        int src1Reg = src1ToRegister_opt(i);
        char src1RegName[10];
        getRegNameByNum(src1Reg, src1RegName);
        //然后用$src1Reg-$src2Reg，存在$t8里
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "addiu");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src1RegName);
        sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "%d", -constToInt(midCodeTable[i].src2));         //addiu $t8, src1, -src2
        //由于if中的条件符号为!=，且$t8=src1-src2
        //所以当$t8!=0时不跳转，$t8==0时跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "beq");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "$0");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "$label%s", midCodeTable[i].dst);   //bgez $t8, label
    }
    else if(src1_is_Const == 1 && src2_is_Const == 1) {
        int src1 = constToInt(midCodeTable[i].src1);
        int src2 = constToInt(midCodeTable[i].src2);
        int result = src1 - src2;
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "li");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "%d", result);
        //由于if中的条件符号为!=，且$t8=src1-src2
        //所以当$t8!=0时不跳转，$t8==0时跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "beq");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "$0");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "$label%s", midCodeTable[i].dst);   //bgez $t8, label
    }
    else {
        //先取src1的值,记录存储的寄存器
        int src1Reg = src1ToRegister_opt(i);
        //再取src2的值,记录存储的寄存器
        int src2Reg = src2ToRegister_opt(i);
        //把寄存器从编号转为名称
        char src1RegName[10];
        char src2RegName[10];
        getRegNameByNum(src1Reg, src1RegName);
        getRegNameByNum(src2Reg, src2RegName);
        //由于if中的条件符号为!=
        //所以当src1Reg!=src2Reg时不跳转，src1Reg==src2Reg时跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "beq");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, src1RegName);
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src2RegName);
        sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "$label%s", midCodeTable[i].dst);   //beq src1Reg, src2Reg, label
    }
}

void generate_mips_code_opt_18(int i)
{
    int src1_is_Const = 0;
    int src2_is_Const = 0;
    if(is_digit(midCodeTable[i].src1[0]) || midCodeTable[i].src1[0] == '\'') {
        src1_is_Const = 1;                      //如果src1是常量值
    }
    if(is_digit(midCodeTable[i].src2[0]) || midCodeTable[i].src2[0] == '\'') {
        src2_is_Const = 1;                      //如果src2是常量值
    }

    if(src1_is_Const == 1 && src2_is_Const == 0) {          //如果只有src1是常量
        int src2Reg = src2ToRegister_opt(i);
        char src2RegName[10];
        getRegNameByNum(src2Reg, src2RegName);
        //然后用src2-src1，存在$t8里
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "addiu");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src2RegName);
        sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "%d", -constToInt(midCodeTable[i].src1));     //addiu $t8, src2, -src1
        //由于if中的条件符号为==，且$t8=src2-src1
        //所以当$t8==0时不跳转，$t8!=0时跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "bne");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "$0");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "$label%s", midCodeTable[i].dst);   //bgez $t8, label
    }
    else if(src1_is_Const == 0 && src2_is_Const == 1) {
        int src1Reg = src1ToRegister_opt(i);
        char src1RegName[10];
        getRegNameByNum(src1Reg, src1RegName);
        //然后用$src1Reg-$src2Reg，存在$t8里
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "addiu");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src1RegName);
        sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "%d", -constToInt(midCodeTable[i].src2));         //addiu $t8, src1, -src2
        //由于if中的条件符号为==，且$t8=src1-src2
        //所以当$t8==0时不跳转，$t8!=0时跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "bne");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "$0");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "$label%s", midCodeTable[i].dst);   //bgez $t8, label
    }
    else if(src1_is_Const == 1 && src2_is_Const == 1) {
        int src1 = constToInt(midCodeTable[i].src1);
        int src2 = constToInt(midCodeTable[i].src2);
        int result = src1 - src2;
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "li");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "%d", result);
        //由于if中的条件符号为==，且$t8=src1-src2
        //所以当$t8==0时不跳转，$t8!=0时跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "bne");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "$0");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "$label%s", midCodeTable[i].dst);   //bgez $t8, label
    }
    else {
        //先取src1的值,记录存储的寄存器
        int src1Reg = src1ToRegister_opt(i);
        //再取src2的值,记录存储的寄存器
        int src2Reg = src2ToRegister_opt(i);
        //把寄存器从编号转为名称
        char src1RegName[10];
        char src2RegName[10];
        getRegNameByNum(src1Reg, src1RegName);
        getRegNameByNum(src2Reg, src2RegName);
        //由于if中的条件符号为==
        //所以当src1Reg==src2Reg时不跳转，src1Reg!=src2Reg时跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "bne");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, src1RegName);
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src2RegName);
        sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "$label%s", midCodeTable[i].dst);   //beq src1Reg, src2Reg, label
    }
}

void generate_mips_code_opt_19(int i)
{
    cleanRegister();
    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "j");
    sprintf(mipsTextCodeTable_opt[text_top_opt].op1, "$label%s", midCodeTable[i].dst);      //j label
}

void generate_mips_code_opt_20(int i)
{
    int src1_is_Const = 0;
    int src2_is_Const = 0;
    if(is_digit(midCodeTable[i].src1[0]) || midCodeTable[i].src1[0] == '\'') {
        src1_is_Const = 1;                      //如果src1是常量值
    }
    if(is_digit(midCodeTable[i].src2[0]) || midCodeTable[i].src2[0] == '\'') {
        src2_is_Const = 1;                      //如果src2是常量值
    }

    if(src1_is_Const == 1 && src2_is_Const == 0) {          //如果只有src1是常量
        int src2Reg = src2ToRegister_opt(i);
        char src2RegName[10];
        getRegNameByNum(src2Reg, src2RegName);
        //然后用src2-src1，存在$t8里
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "addiu");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src2RegName);
        sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "%d", -constToInt(midCodeTable[i].src1));     //addiu $t8, src2, -src1
        //由于if中的条件符号为<，且这里用src2-src1
        //所以当t8<=0时跳转，t8>0时不跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "blez");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "$label%s", midCodeTable[i].dst);   //blez $t8, label
    }
    else if(src1_is_Const == 0 && src2_is_Const == 1) {
        int src1Reg = src1ToRegister_opt(i);
        char src1RegName[10];
        getRegNameByNum(src1Reg, src1RegName);
        //然后用$src1Reg-$src2Reg，存在$t8里
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "addiu");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src1RegName);
        sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "%d", -constToInt(midCodeTable[i].src2));         //addiu $t8, src1, -src2
        //由于if中的条件符号为<
        //所以当t8小于0时不跳转，t8>=0时跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "bgez");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "$label%s", midCodeTable[i].dst);   //bgez $t8, label
    }
    else if(src1_is_Const == 1 && src2_is_Const == 1) {
        int src1 = constToInt(midCodeTable[i].src1);
        int src2 = constToInt(midCodeTable[i].src2);
        int result = src1 - src2;
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "li");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "%d", result);
        //由于if中的条件符号为<
        //所以当t8小于0时不跳转，t8>=0时跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "bgez");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "$label%s", midCodeTable[i].dst);   //bgez $t8, label
    }
    else {
        //先取src1的值,记录存储的寄存器
        int src1Reg = src1ToRegister_opt(i);
        //再取src2的值,记录存储的寄存器
        int src2Reg = src2ToRegister_opt(i);
        //把寄存器从编号转为名称
        char src1RegName[10];
        char src2RegName[10];
        getRegNameByNum(src1Reg, src1RegName);
        getRegNameByNum(src2Reg, src2RegName);
        //然后用$src1Reg-$src2Reg，存在$t8里
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "subu");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src1RegName);
        strcpy(mipsTextCodeTable_opt[text_top_opt].op3, src2RegName);         //sub $t8, src1RegName, src2RegName
        //由于if中的条件符号为<
        //所以当t8小于0时不跳转，t8>=0时跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "bgez");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "$label%s", midCodeTable[i].dst);   //bgez $t8, label
    }
}

void generate_mips_code_opt_21(int i)
{
    int src1_is_Const = 0;
    int src2_is_Const = 0;
    if(is_digit(midCodeTable[i].src1[0]) || midCodeTable[i].src1[0] == '\'') {
        src1_is_Const = 1;                      //如果src1是常量值
    }
    if(is_digit(midCodeTable[i].src2[0]) || midCodeTable[i].src2[0] == '\'') {
        src2_is_Const = 1;                      //如果src2是常量值
    }

    if(src1_is_Const == 1 && src2_is_Const == 0) {          //如果只有src1是常量
        int src2Reg = src2ToRegister_opt(i);
        char src2RegName[10];
        getRegNameByNum(src2Reg, src2RegName);
        //然后用src2-src1，存在$t8里
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "addiu");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src2RegName);
        sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "%d", -constToInt(midCodeTable[i].src1));     //addiu $t8, src2, -src1
        //由于if中的条件符号为<=，且src2-src1
        //所以当t8<0时跳转，t8>=0时不跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "bltz");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "$label%s", midCodeTable[i].dst);   //blez $t8, label
    }
    else if(src1_is_Const == 0 && src2_is_Const == 1) {
        int src1Reg = src1ToRegister_opt(i);
        char src1RegName[10];
        getRegNameByNum(src1Reg, src1RegName);
        //然后用$src1Reg-$src2Reg，存在$t8里
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "addiu");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src1RegName);
        sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "%d", -constToInt(midCodeTable[i].src2));         //addiu $t8, src1, -src2
        //由于if中的条件符号为<=
        //所以当t8<=0时不跳转，t8>0时跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "bgtz");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "$label%s", midCodeTable[i].dst);   //bgez $t8, label
    }
    else if(src1_is_Const == 1 && src2_is_Const == 1) {
        int src1 = constToInt(midCodeTable[i].src1);
        int src2 = constToInt(midCodeTable[i].src2);
        int result = src1 - src2;
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "li");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "%d", result);
        //由于if中的条件符号为<=
        //所以当t8<=0时不跳转，t8>0时跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "bgtz");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "$label%s", midCodeTable[i].dst);   //bgez $t8, label
    }
    else {
        //先取src1的值,记录存储的寄存器
        int src1Reg = src1ToRegister_opt(i);
        //再取src2的值,记录存储的寄存器
        int src2Reg = src2ToRegister_opt(i);
        //把寄存器从编号转为名称
        char src1RegName[10];
        char src2RegName[10];
        getRegNameByNum(src1Reg, src1RegName);
        getRegNameByNum(src2Reg, src2RegName);
        //然后用$src1Reg-$src2Reg，存在$t8里
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "subu");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src1RegName);
        strcpy(mipsTextCodeTable_opt[text_top_opt].op3, src2RegName);         //sub $t8, src1RegName, src2RegName
        //由于if中的条件符号为<=
        //所以当t8<=0时不跳转，t8>0时跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "bgtz");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "$label%s", midCodeTable[i].dst);   //bgez $t8, label
    }
}

void generate_mips_code_opt_22(int i)
{
    int src1_is_Const = 0;
    int src2_is_Const = 0;
    if(is_digit(midCodeTable[i].src1[0]) || midCodeTable[i].src1[0] == '\'') {
        src1_is_Const = 1;                      //如果src1是常量值
    }
    if(is_digit(midCodeTable[i].src2[0]) || midCodeTable[i].src2[0] == '\'') {
        src2_is_Const = 1;                      //如果src2是常量值
    }

    if(src1_is_Const == 1 && src2_is_Const == 0) {          //如果只有src1是常量
        int src2Reg = src2ToRegister_opt(i);
        char src2RegName[10];
        getRegNameByNum(src2Reg, src2RegName);
        //然后用src2-src1，存在$t8里
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "addiu");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src2RegName);
        sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "%d", -constToInt(midCodeTable[i].src1));     //addiu $t8, src2, -src1
        //由于if中的条件符号为>，且src2-src1
        //所以当t8>=0时跳转，t8<0时不跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "bgez");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "$label%s", midCodeTable[i].dst);   //blez $t8, label
    }
    else if(src1_is_Const == 0 && src2_is_Const == 1) {
        int src1Reg = src1ToRegister_opt(i);
        char src1RegName[10];
        getRegNameByNum(src1Reg, src1RegName);
        //然后用$src1Reg-$src2Reg，存在$t8里
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "addiu");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src1RegName);
        sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "%d", -constToInt(midCodeTable[i].src2));         //addiu $t8, src1, -src2
        //由于if中的条件符号为>
        //所以当t8>0时不跳转，t8<=0时跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "blez");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "$label%s", midCodeTable[i].dst);   //bgez $t8, label
    }
    else if(src1_is_Const == 1 && src2_is_Const == 1) {
        int src1 = constToInt(midCodeTable[i].src1);
        int src2 = constToInt(midCodeTable[i].src2);
        int result = src1 - src2;
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "li");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "%d", result);
        //由于if中的条件符号为>
        //所以当t8>0时不跳转，t8<=0时跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "blez");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "$label%s", midCodeTable[i].dst);   //bgez $t8, label
    }
    else {
        //先取src1的值,记录存储的寄存器
        int src1Reg = src1ToRegister_opt(i);
        //再取src2的值,记录存储的寄存器
        int src2Reg = src2ToRegister_opt(i);
        //把寄存器从编号转为名称
        char src1RegName[10];
        char src2RegName[10];
        getRegNameByNum(src1Reg, src1RegName);
        getRegNameByNum(src2Reg, src2RegName);
        //然后用$src1Reg-$src2Reg，存在$t8里
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "subu");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src1RegName);
        strcpy(mipsTextCodeTable_opt[text_top_opt].op3, src2RegName);         //sub $t8, src1RegName, src2RegName
        //由于if中的条件符号为>
        //所以当t8>0时不跳转，t8<=0时跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "blez");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "$label%s", midCodeTable[i].dst);   //bgez $t8, label
    }
}

void generate_mips_code_opt_23(int i)
{
    int src1_is_Const = 0;
    int src2_is_Const = 0;
    if(is_digit(midCodeTable[i].src1[0]) || midCodeTable[i].src1[0] == '\'') {
        src1_is_Const = 1;                      //如果src1是常量值
    }
    if(is_digit(midCodeTable[i].src2[0]) || midCodeTable[i].src2[0] == '\'') {
        src2_is_Const = 1;                      //如果src2是常量值
    }

    if(src1_is_Const == 1 && src2_is_Const == 0) {          //如果只有src1是常量
        int src2Reg = src2ToRegister_opt(i);
        char src2RegName[10];
        getRegNameByNum(src2Reg, src2RegName);
        //然后用src2-src1，存在$t8里
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "addiu");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src2RegName);
        sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "%d", -constToInt(midCodeTable[i].src1));     //addiu $t8, src2, -src1
        //由于if中的条件符号为>=，且src2-src1
        //所以当t8>0时跳转，t8<=0时不跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "bgtz");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "$label%s", midCodeTable[i].dst);   //blez $t8, label
    }
    else if(src1_is_Const == 0 && src2_is_Const == 1) {
        int src1Reg = src1ToRegister_opt(i);
        char src1RegName[10];
        getRegNameByNum(src1Reg, src1RegName);
        //然后用$src1Reg-$src2Reg，存在$t8里
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "addiu");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src1RegName);
        sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "%d", -constToInt(midCodeTable[i].src2));         //addiu $t8, src1, -src2
        //由于if中的条件符号为>=
        //所以当t8>=0时不跳转，t8<0时跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "bltz");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "$label%s", midCodeTable[i].dst);   //bgez $t8, label
    }
    else if(src1_is_Const == 1 && src2_is_Const == 1) {
        int src1 = constToInt(midCodeTable[i].src1);
        int src2 = constToInt(midCodeTable[i].src2);
        int result = src1 - src2;
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "li");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "%d", result);
        //由于if中的条件符号为>=
        //所以当t8>=0时不跳转，t8<0时跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "bltz");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "$label%s", midCodeTable[i].dst);   //bgez $t8, label
    }
    else {
        //先取src1的值,记录存储的寄存器
        int src1Reg = src1ToRegister_opt(i);
        //再取src2的值,记录存储的寄存器
        int src2Reg = src2ToRegister_opt(i);
        //把寄存器从编号转为名称
        char src1RegName[10];
        char src2RegName[10];
        getRegNameByNum(src1Reg, src1RegName);
        getRegNameByNum(src2Reg, src2RegName);
        //然后用$src1Reg-$src2Reg，存在$t8里
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "subu");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src1RegName);
        strcpy(mipsTextCodeTable_opt[text_top_opt].op3, src2RegName);         //sub $t8, src1RegName, src2RegName
        //由于if中的条件符号为>=
        //所以当t8>=0时不跳转，t8<0时跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "bltz");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "$label%s", midCodeTable[i].dst);   //bgez $t8, label
    }
}

void generate_mips_code_opt_24(int i)
{
    int src1_is_Const = 0;
    int src2_is_Const = 0;
    if(is_digit(midCodeTable[i].src1[0]) || midCodeTable[i].src1[0] == '\'') {
        src1_is_Const = 1;                      //如果src1是常量值
    }
    if(is_digit(midCodeTable[i].src2[0]) || midCodeTable[i].src2[0] == '\'') {
        src2_is_Const = 1;                      //如果src2是常量值
    }

    if(src1_is_Const == 1 && src2_is_Const == 0) {          //如果只有src1是常量
        int src2Reg = src2ToRegister_opt(i);
        char src2RegName[10];
        getRegNameByNum(src2Reg, src2RegName);
        //然后用src2-src1，存在$t8里
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "addiu");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src2RegName);
        sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "%d", -constToInt(midCodeTable[i].src1));     //addiu $t8, src2, -src1
        //由于if中的条件符号为!=，且$t8=src2-src1
        //所以当$t8!=0时不跳转，$t8==0时跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "beq");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "$0");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "$label%s", midCodeTable[i].dst);   //bgez $t8, label
    }
    else if(src1_is_Const == 0 && src2_is_Const == 1) {
        int src1Reg = src1ToRegister_opt(i);
        char src1RegName[10];
        getRegNameByNum(src1Reg, src1RegName);
        //然后用$src1Reg-$src2Reg，存在$t8里
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "addiu");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src1RegName);
        sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "%d", -constToInt(midCodeTable[i].src2));         //addiu $t8, src1, -src2
        //由于if中的条件符号为!=，且$t8=src1-src2
        //所以当$t8!=0时不跳转，$t8==0时跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "beq");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "$0");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "$label%s", midCodeTable[i].dst);   //bgez $t8, label
    }
    else if(src1_is_Const == 1 && src2_is_Const == 1) {
        int src1 = constToInt(midCodeTable[i].src1);
        int src2 = constToInt(midCodeTable[i].src2);
        int result = src1 - src2;
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "li");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "%d", result);
        //由于if中的条件符号为!=，且$t8=src1-src2
        //所以当$t8!=0时不跳转，$t8==0时跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "beq");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "$0");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "$label%s", midCodeTable[i].dst);   //bgez $t8, label
    }
    else {
        //先取src1的值,记录存储的寄存器
        int src1Reg = src1ToRegister_opt(i);
        //再取src2的值,记录存储的寄存器
        int src2Reg = src2ToRegister_opt(i);
        //把寄存器从编号转为名称
        char src1RegName[10];
        char src2RegName[10];
        getRegNameByNum(src1Reg, src1RegName);
        getRegNameByNum(src2Reg, src2RegName);
        //由于if中的条件符号为!=
        //所以当src1Reg!=src2Reg时不跳转，src1Reg==src2Reg时跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "beq");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, src1RegName);
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src2RegName);
        sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "$label%s", midCodeTable[i].dst);   //beq src1Reg, src2Reg, label
    }
}

void generate_mips_code_opt_25(int i)
{
    int src1_is_Const = 0;
    int src2_is_Const = 0;
    if(is_digit(midCodeTable[i].src1[0]) || midCodeTable[i].src1[0] == '\'') {
        src1_is_Const = 1;                      //如果src1是常量值
    }
    if(is_digit(midCodeTable[i].src2[0]) || midCodeTable[i].src2[0] == '\'') {
        src2_is_Const = 1;                      //如果src2是常量值
    }

    if(src1_is_Const == 1 && src2_is_Const == 0) {          //如果只有src1是常量
        int src2Reg = src2ToRegister_opt(i);
        char src2RegName[10];
        getRegNameByNum(src2Reg, src2RegName);
        //然后用src2-src1，存在$t8里
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "addiu");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src2RegName);
        sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "%d", -constToInt(midCodeTable[i].src1));     //addiu $t8, src2, -src1
        //由于if中的条件符号为==，且$t8=src2-src1
        //所以当$t8==0时不跳转，$t8!=0时跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "bne");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "$0");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "$label%s", midCodeTable[i].dst);   //bgez $t8, label
    }
    else if(src1_is_Const == 0 && src2_is_Const == 1) {
        int src1Reg = src1ToRegister_opt(i);
        char src1RegName[10];
        getRegNameByNum(src1Reg, src1RegName);
        //然后用$src1Reg-$src2Reg，存在$t8里
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "addiu");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src1RegName);
        sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "%d", -constToInt(midCodeTable[i].src2));         //addiu $t8, src1, -src2
        //由于if中的条件符号为==，且$t8=src1-src2
        //所以当$t8==0时不跳转，$t8!=0时跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "bne");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "$0");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "$label%s", midCodeTable[i].dst);   //bgez $t8, label
    }
    else if(src1_is_Const == 1 && src2_is_Const == 1) {
        int src1 = constToInt(midCodeTable[i].src1);
        int src2 = constToInt(midCodeTable[i].src2);
        int result = src1 - src2;
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "li");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "%d", result);
        //由于if中的条件符号为==，且$t8=src1-src2
        //所以当$t8==0时不跳转，$t8!=0时跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "bne");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "$0");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "$label%s", midCodeTable[i].dst);   //bgez $t8, label
    }
    else {
        //先取src1的值,记录存储的寄存器
        int src1Reg = src1ToRegister_opt(i);
        //再取src2的值,记录存储的寄存器
        int src2Reg = src2ToRegister_opt(i);
        //把寄存器从编号转为名称
        char src1RegName[10];
        char src2RegName[10];
        getRegNameByNum(src1Reg, src1RegName);
        getRegNameByNum(src2Reg, src2RegName);
        //由于if中的条件符号为==
        //所以当src1Reg==src2Reg时不跳转，src1Reg!=src2Reg时跳转
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "bne");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, src1RegName);
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src2RegName);
        sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "$label%s", midCodeTable[i].dst);   //beq src1Reg, src2Reg, label
    }
}

void generate_mips_code_opt_26(int i)
{
    int j = loc_mips_opt(midCodeTable[i].dst);      //找到待读取内容在符号表中的位置
    //从控制台读取一个 量 并存在$v0里
    if(tab[j].type == INT_TAB_VALUE) {      //如果是整数
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "li");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$v0");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "5");   //li $v0, 5         //5 for read integer
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "syscall");   //syscall
    }
    else if(tab[j].type == CHAR_TAB_VALUE) {        //如果是字符
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "li");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$v0");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "12");   //li $v0, 12       //12 for read character
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "syscall");   //syscall
    }
    //把读取的内容读入$v0后，如果这个变量已经在寄存器中了，更新寄存器的值
    //如果不在寄存器中，写入内存
    int k = findInRegister(midCodeTable[i].dst, i);
    if(k < 0) {     //如果在寄存器中没找到
        //把量从$v0 存入内存
        if(j > globalTop) {     //如果是局部变量
            text_top_opt++;
            strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "sw");
            strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$v0");
            sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "-%d", tab[j].offset);
            strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "$sp");         //sw $v0, -offset($sp)
        }
        else {      //如果是全局变量
            text_top_opt++;
            strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "sw");
            strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$v0");
            sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "%d", tab[j].offset);
            strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "$gp");                 //sw $v0, offset($gp)
        }
    }
    else {
        if(k < 10) {
            text_top_opt++;
            strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "move");
            sprintf(mipsTextCodeTable_opt[text_top_opt].op1, "$t%d", k);
            strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "$v0");
        }
        else {
            text_top_opt++;
            strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "move");
            sprintf(mipsTextCodeTable_opt[text_top_opt].op1, "$s%d", k-10);
            strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "$v0");
        }
    }
}

void generate_mips_code_opt_27(int i)
{
    //为字符串创建空间
    stringNum_opt++;
    data_top_opt++;
    sprintf(mipsDataCodeTable_opt[data_top_opt].ident, "$string%d", stringNum_opt);
    strcpy(mipsDataCodeTable_opt[data_top_opt].instr, ".asciiz");
    strcpy(mipsDataCodeTable_opt[data_top_opt].content, midCodeTable[i].src1);  //$stringNum: .asciiz "string"

    //输出字符串
    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "la");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$a0");
    sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "$string%d", stringNum_opt);    //la $a0, $stringNum
    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "li");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$v0");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "4");        //li $v0, 4    //4 for print string
    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "syscall");       //syscall

    //输出变量
    int syscallNum = src2ToA0_opt(i);       //返回1为输出int，返回11为输出char

    if(syscallNum == -1) {
        printf("syscall num is -1 when printf\n");
    }

    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "li");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$v0");
    sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "%d", syscallNum);       //li $v0, syscallNum
    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "syscall");   //syscall

    //输出换行符
    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "la");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$a0");
    sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "$newLine");    //la $a0, $newLine
    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "li");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$v0");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "4");        //li $v0, 4    //4 for print string
    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "syscall");       //syscall
}

void generate_mips_code_opt_28(int i)
{
    //为字符串创建空间
    stringNum_opt++;
    data_top_opt++;
    sprintf(mipsDataCodeTable_opt[data_top_opt].ident, "$string%d", stringNum_opt);
    strcpy(mipsDataCodeTable_opt[data_top_opt].instr, ".asciiz");
    strcpy(mipsDataCodeTable_opt[data_top_opt].content, midCodeTable[i].src1);  //$stringNum: .asciiz "string"

    //输出字符串
    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "la");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$a0");
    sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "$string%d", stringNum_opt);    //la $a0, $stringNum
    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "li");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$v0");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "4");        //li $v0, 4    //4 for print string
    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "syscall");       //syscall

    //输出换行符
    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "la");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$a0");
    sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "$newLine");    //la $a0, $newLine
    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "li");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$v0");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "4");        //li $v0, 4    //4 for print string
    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "syscall");       //syscall
}

void generate_mips_code_opt_29(int i)
{
    //输出变量
    int syscallNum = src2ToA0_opt(i);       //返回1为输出int，返回11为输出char

    if(syscallNum == -1) {
        printf("syscall num is -1 when printf\n");
    }

    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "li");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$v0");
    sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "%d", syscallNum);       //li $v0, syscallNum
    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "syscall");   //syscall

    //输出换行符
    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "la");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$a0");
    sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "$newLine");    //la $a0, $newLine
    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "li");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$v0");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "4");        //li $v0, 4    //4 for print string
    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "syscall");       //syscall
}

void generate_mips_code_opt_30(int i)
{
    //先取src1的值
    int src1Reg = src1ToRegister_opt(i);
    //再找到dst
    registerToDst_opt(src1Reg, i);
}

void generate_mips_code_opt_31(int i)
{
    //先取src1的值,记录存储的寄存器
    int src1Reg = src1ToRegister_opt(i);
    //再取src2的值,记录存储的寄存器
    int src2Reg = src2ToRegister_opt(i);
    //把寄存器从编号转为名称
    char src1RegName[10];
    char src2RegName[10];
    getRegNameByNum(src1Reg, src1RegName);
    getRegNameByNum(src2Reg, src2RegName);
    //再找到dst
    int j = loc_mips_opt(midCodeTable[i].dst);  //在符号表中查找
    if(j > globalTop) {                         //如果是局部变量
        //数组名offset+数组下标src1*4求得地址
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "sll");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t9");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src1RegName);
        strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "2");       //sll $t9, src1Reg, 2  //src1Reg << 2 ($t9 = src1Reg * 4)
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "addiu");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t9");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "$t9");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "%d", tab[j].offset);  //addi $t9, $t9, offset  ($t9 = src1 * 4 + offset)
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "subu");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t9");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "$sp");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "$t9");     //sub $t9, $sp, $t9 (内存地址)
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "sw");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, src2RegName);
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "%d", 0);
        strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "$t9");     //sw src2Reg, 0($t9)  //$t9 is address, src2Reg is the value going to assign
    }
    else {                                      //如果是全局变量
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "sll");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src1RegName);
        strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "2");       //sll $t8, src1Reg, 2  //src1 << 2 ($t8 = src1 * 4)//下标*4,偏移量
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "addiu");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t9");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "$gp");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "%d", tab[j].offset);      //addi $t9, $gp, offset 数组基址存入$t9
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "addu");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t9");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "$t9");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "$t8");     //add $t9, $t9, $t8 //基地址加上偏移量，存入$t9(内存地址)
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "sw");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, src2RegName);
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "0");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "$t9");                 //sw src1, 0($t9)
    }
}

void generate_mips_code_opt_32(int i)
{
    cleanRegister();                //call，清理寄存器
    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "jal");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, midCodeTable[i].src1);
}

void generate_mips_code_opt_33(int i)
{
    //先取src1的值,记录存储的寄存器
    int src1Reg = src1ToRegister_opt(i);
    //再取src2的值,记录存储的寄存器
    int src2Reg = src2ToRegister_opt(i);
    //把寄存器从编号转为名称
    char src1RegName[10];
    char src2RegName[10];
    getRegNameByNum(src1Reg, src1RegName);
    getRegNameByNum(src2Reg, src2RegName);
    //由于if中的条件符号为==
    //所以当src1Reg==src2Reg时不跳转，src1Reg!=src2Reg时跳转
    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "bne");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, src1RegName);
    strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src2RegName);
    sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "$label%s", midCodeTable[i].dst);   //beq src1Reg, src2Reg, label
}

void generate_mips_code_opt_35(int i)
{
    //先取src1的值,记录存储的寄存器
    int src1Reg = src1ToRegister_opt(i);
    //把寄存器从编号转为名称
    char src1RegName[10];
    getRegNameByNum(src1Reg, src1RegName);

    //在中间代码中向上找直到找到不是PUSH的指令，以便直到这是第几个push，以对应到函数的第几个参数
    int j;
    for(j = i; j>0; j--) {
        if(midCodeTable[j].op != 35)
            break;          //向上找直到操作符不是push
    }
    int number = i-j;       //求得这是函数调用中的第number个push
    //由于此时是函数调用，则$fp指向call后的函数栈底，则该参数应存储的位置为$fp+number*4+4  ($fp+8为第一个参数)
    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "sw");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, src1RegName);
    sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "-%d", number*4+4);
    strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "$fp");     //sw src1, -(number*4+4)($fp)
}

void generate_mips_code_opt_36(int i)
{
    int src1_is_Const = 0;
    int src2_is_Const = 0;
    if(is_digit(midCodeTable[i].src1[0]) || midCodeTable[i].src1[0] == '\'') {
        src1_is_Const = 1;                      //如果src1是常量值
    }
    if(is_digit(midCodeTable[i].src2[0]) || midCodeTable[i].src2[0] == '\'') {
        src2_is_Const = 1;                      //如果src2是常量值
    }

    if((src1_is_Const == 1 && src2_is_Const == 0) || (src1_is_Const == 0 && src2_is_Const == 1)) {  //如果src1是常量或src2是常量
        char srcRegName[10];
        if(src1_is_Const == 1) {   //如果src1是常量，src2是变量
            //再取src2的值,记录存储的寄存器
            int src2Reg = src2ToRegister_opt(i);
            //把寄存器从编号转为名称
            getRegNameByNum(src2Reg, srcRegName);
        }
        else {                      //如果src1是变量，src2是常量
            int src1Reg = src1ToRegister_opt(i);
            getRegNameByNum(src1Reg, srcRegName);
        }
        //然后找到结果要存的寄存器
        int j = findInRegister(midCodeTable[i].dst, i);
        int dstReg;
        if (j < 0) {
            int k = findARegisterToReplace(midCodeTable[i].dst);
            dstReg = k;
            //把k标记为dst，并更新最后使用
            getDst(k, i);
        } else {
            dstReg = j;
            //把j标记为dst，并更新最后使用
            getDst(j, i);
        }
        char dstRegName[10];
        getRegNameByNum(dstReg, dstRegName);
        //计算
        if(src1_is_Const == 1) {        //如果src1是常量
            text_top_opt++;
            strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "addiu");
            strcpy(mipsTextCodeTable_opt[text_top_opt].op1, dstRegName);
            strcpy(mipsTextCodeTable_opt[text_top_opt].op2, srcRegName);
            sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "%d", constToInt(midCodeTable[i].src1));     //addiu dst, src1/2, const
        }
        else {                      //如果src2是常量
            text_top_opt++;
            strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "addiu");
            strcpy(mipsTextCodeTable_opt[text_top_opt].op1, dstRegName);
            strcpy(mipsTextCodeTable_opt[text_top_opt].op2, srcRegName);
            sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "%d", constToInt(midCodeTable[i].src2));     //addiu dst, src1/2, const
        }
    }
    else {      //如果src1和src2都是变量       //不可能都是常量
        //先取src1的值,记录存储的寄存器
        int src1Reg = src1ToRegister_opt(i);
        //再取src2的值,记录存储的寄存器
        int src2Reg = src2ToRegister_opt(i);
        //把寄存器从编号转为名称
        char src1RegName[10];
        char src2RegName[10];
        getRegNameByNum(src1Reg, src1RegName);
        getRegNameByNum(src2Reg, src2RegName);
        //然后找到结果要存的寄存器
        int j = findInRegister(midCodeTable[i].dst, i);
        int dstReg;
        if (j < 0) {
            int k = findARegisterToReplace(midCodeTable[i].dst);
            dstReg = k;
            //把k标记为dst，并更新最后使用
            getDst(k, i);
        } else {
            dstReg = j;
            //把j标记为dst，并更新最后使用
            getDst(j, i);
        }
        char dstRegName[10];
        getRegNameByNum(dstReg, dstRegName);
        //计算
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "addu");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, dstRegName);
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src1RegName);
        strcpy(mipsTextCodeTable_opt[text_top_opt].op3, src2RegName);         //addu dst, src1, src2
    }
}

void generate_mips_code_opt_37(int i)
{
    int src2_is_Const = 0;
    if(is_digit(midCodeTable[i].src2[0]) || midCodeTable[i].src2[0] == '\'') {
        src2_is_Const = 1;                      //如果src2是常量值
    }
    if(src2_is_Const) {     //如果src2是常量
        //先取src1的值,记录存储的寄存器
        int src1Reg = src1ToRegister_opt(i);
        //把寄存器从编号转为名称
        char src1RegName[10];
        getRegNameByNum(src1Reg, src1RegName);
        //然后找到结果要存的寄存器
        int j = findInRegister(midCodeTable[i].dst, i);
        int dstReg;
        if (j < 0) {
            int k = findARegisterToReplace(midCodeTable[i].dst);
            dstReg = k;
            //把k标记为dst，并更新最后使用
            getDst(k, i);
        } else {
            dstReg = j;
            //把j标记为dst，并更新最后使用
            getDst(j, i);
        }
        char dstRegName[10];
        getRegNameByNum(dstReg, dstRegName);
        //计算
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "addiu");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, dstRegName);
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src1RegName);
        sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "%d", -constToInt(midCodeTable[i].src2));         //addiu dst, src1, -src2
    }
    else {
        //先取src1的值,记录存储的寄存器
        int src1Reg = src1ToRegister_opt(i);
        //再取src2的值,记录存储的寄存器
        int src2Reg = src2ToRegister_opt(i);
        //把寄存器从编号转为名称
        char src1RegName[10];
        char src2RegName[10];
        getRegNameByNum(src1Reg, src1RegName);
        getRegNameByNum(src2Reg, src2RegName);
        //然后找到结果要存的寄存器
        int j = findInRegister(midCodeTable[i].dst, i);
        int dstReg;
        if (j < 0) {
            int k = findARegisterToReplace(midCodeTable[i].dst);
            dstReg = k;
            //把k标记为dst，并更新最后使用
            getDst(k, i);
        } else {
            dstReg = j;
            //把j标记为dst，并更新最后使用
            getDst(j, i);
        }
        char dstRegName[10];
        getRegNameByNum(dstReg, dstRegName);
        //计算
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "subu");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, dstRegName);
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src1RegName);
        strcpy(mipsTextCodeTable_opt[text_top_opt].op3, src2RegName);         //subu dst, src1, src2
    }
}

void generate_mips_code_opt_38(int i)
{
    //先取src1的值,记录存储的寄存器
    int src1Reg = src1ToRegister_opt(i);
    //再取src2的值,记录存储的寄存器
    int src2Reg = src2ToRegister_opt(i);
    //把寄存器从编号转为名称
    char src1RegName[10];
    char src2RegName[10];
    getRegNameByNum(src1Reg, src1RegName);
    getRegNameByNum(src2Reg, src2RegName);
    //然后找到结果要存的寄存器
    int j = findInRegister(midCodeTable[i].dst, i);
    int dstReg;
    if(j < 0) {
        int k = findARegisterToReplace(midCodeTable[i].dst);
        dstReg = k;
        //把k标记为dst，并更新最后使用
        getDst(k, i);
    }
    else {
        dstReg = j;
        //把j标记为dst，并更新最后使用
        getDst(j, i);
    }
    char dstRegName[10];
    getRegNameByNum(dstReg, dstRegName);
    //计算
    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "mult");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, src1RegName);
    strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src2RegName);         //mult src1, src2
    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "mflo");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, dstRegName);                 //mflo dst

//    //然后用src1*src2，存在$t8里     //这个是最终要存回内存的结果
//    text_top_opt++;
//    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "mult");
//    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, src1RegName);
//    strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src2RegName);         //mult src1, src2
//    text_top_opt++;
//    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "mflo");
//    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");                 //mflo $t8
//    //然后存回dst
//    registerToDst_opt(8, i);            //$t8存回dst
}

void generate_mips_code_opt_39(int i)
{
    //先取src1的值,记录存储的寄存器
    int src1Reg = src1ToRegister_opt(i);
    //再取src2的值,记录存储的寄存器
    int src2Reg = src2ToRegister_opt(i);
    //把寄存器从编号转为名称
    char src1RegName[10];
    char src2RegName[10];
    getRegNameByNum(src1Reg, src1RegName);
    getRegNameByNum(src2Reg, src2RegName);
    //然后找到结果要存的寄存器
    int j = findInRegister(midCodeTable[i].dst, i);
    int dstReg;
    if(j < 0) {
        int k = findARegisterToReplace(midCodeTable[i].dst);
        dstReg = k;
        //把k标记为dst，并更新最后使用
        getDst(k, i);
    }
    else {
        dstReg = j;
        //把j标记为dst，并更新最后使用
        getDst(j, i);
    }
    char dstRegName[10];
    getRegNameByNum(dstReg, dstRegName);
    //计算
    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "div");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, src1RegName);
    strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src2RegName);         //div src1, src2
    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "mflo");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, dstRegName);                 //mflo dst

//    //然后用src1/src2，存在$t8里     //这个是最终要存回内存的结果
//    text_top_opt++;
//    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "div");
//    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, src1RegName);
//    strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src2RegName);         //div src1, src2
//    text_top_opt++;
//    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "mflo");
//    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");                 //mflo $t8
//    //然后存回dst
//    registerToDst_opt(8, i);            //$t8存回dst
}

void generate_mips_code_opt_40(int i)
{
    //先取src2的值,记录存储的寄存器
    int src2Reg = src2ToRegister_opt(i);
    //把寄存器从编号转为名称
    char src2RegName[10];
    getRegNameByNum(src2Reg, src2RegName);
    //已经把数组下标的值存在src2Reg里
    int j = loc_mips_opt(midCodeTable[i].src1);     //查找数组名
    if(j > globalTop) {     //如果是局部变量数组
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "sll");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src2RegName);
        strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "2");     //sll $t8, src2, 2   //t8 = src2 * 4  (下标偏移量)
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "addiu");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "%d", tab[j].offset);  //addi $t8, $t8, offset  (求出相对于sp的偏移量)
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "subu");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "$sp");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "$t8");         //sub $t8, $sp, $t8  求出数组元素内存地址
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "lw");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "0");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "$t8");     //lw $t8, 0($t8)  把数组元素的值存入$t8
    }
    else {                  //如果是全局变量数组
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "sll");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, src2RegName);
        strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "2");     //sll $t8, src2, 2   //t8 = src2 * 4  (下标偏移量)
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "addiu");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t9");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "$gp");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op3, "%d", tab[j].offset);      //addi $t9, $gp, offset
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "addu");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "$t9");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "$t8");     //add $t8, $t9, $t8  数组元素内存地址
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "lw");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$t8");
        sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "0");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "$t8");     //lw $t8, 0($t8)  把数组元素的值存入$t8
    }
    //已经把要存的值存在$t8里
    //然后存回dst
    registerToDst_opt(8, i);            //$t8存回dst
}

void generate_mips_code_opt_41(int i)
{
    //清理寄存器
    cleanRegister();
    //对sp和fp进行调整，以便返回
    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "lw");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$ra");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "0");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "$sp");     //lw $ra, 0($sp)
    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "move");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$fp");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "$sp");     //move, $fp, $sp
    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "lw");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$sp");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "-4");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "$sp");     //lw $sp, -4($sp)
    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "jr");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$ra");     //jr $ra
}

void cleanRegister()
{
    /*
     * 清空$t0-$t7，把$t0-$t7里的数据放回内存
     */
    int i;
    for(i = 0; i<=7; i++) {                 //遍历8个寄存器
        if(reg_memCompTable[i].type == 0) {     //如果第i个是空着的，跳过
            continue;
        }
        if(reg_memCompTable[i].type == 2) {     //如果第i个存的是常量
            reg_memCompTable[i].type = 0;           //把它清空
            continue;
        }
        //如果第i个存着的是变量
        if(reg_memCompTable[i].index > 0) {     //如果是声明过的变量/常量
            if(tab[reg_memCompTable[i].index].object == CONST_TAB_VALUE) {      //如果是const(实际上不可能是const，const在中间代码优化的时候就消去了)
                reg_memCompTable[i].type = 0;
            }
            else {                                      //如果不是const，就只能是var和para，不可能是array和func
                if(reg_memCompTable[i].index <= globalTop) {
                    printf("ERROR in cleanRegister: store global var in temp register\n");
                }
                else {
                    text_top_opt++;
                    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "sw");
                    sprintf(mipsTextCodeTable_opt[text_top_opt].op1, "$t%d", i);
                    sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "-%d", tab[reg_memCompTable[i].index].offset);
                    strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "$sp");
                    reg_memCompTable[i].type = 0;
                }
            }
        }
        else if(reg_memCompTable[i].index < 0) {        //如果是临时变量
            char tempVar[20];
            sprintf(tempVar, "$%d", -reg_memCompTable[i].index);
            int j = locTemp(tempVar);
            text_top_opt++;
            strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "sw");
            sprintf(mipsTextCodeTable_opt[text_top_opt].op1, "$t%d", i);
            sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "-%d", offsetTempTab[j].offset);
            strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "$sp");
            reg_memCompTable[i].type = 0;
        }
        else {
            reg_memCompTable[i].type = 0;
        }
    }
    for(i = 10; i<=17; i++) {
        if(reg_memCompTable[i].type == 0) {     //如果第i个是空着的，跳过
            continue;
        }
        if(reg_memCompTable[i].type == 2) {     //如果第i个存的是常量
            reg_memCompTable[i].type = 0;           //把它清空
            continue;
        }
        //如果第i个存着的是变量
        if(reg_memCompTable[i].index > 0) {     //如果是声明过的变量/常量
            if(tab[reg_memCompTable[i].index].object == CONST_TAB_VALUE) {      //如果是const(实际上不可能是const，const在中间代码优化的时候就消去了)
                reg_memCompTable[i].type = 0;
            }
            else {                                      //如果不是const，就只能是var和para，不可能是array和func
                if(reg_memCompTable[i].index <= globalTop) {
                    text_top_opt++;
                    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "sw");
                    sprintf(mipsTextCodeTable_opt[text_top_opt].op1, "$s%d", i-10);
                    sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "%d", tab[reg_memCompTable[i].index].offset);
                    strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "$gp");                 //sw reg, offset($gp)
                    reg_memCompTable[i].type = 0;
                }
                else {
                    printf("ERROR in cleanRegister: store local var in global register\n");
                }
            }
        }
        else if(reg_memCompTable[i].index < 0) {        //如果是临时变量
            printf("ERROR in cleanRegister: store temp var in global register\n");
        }
        else {
            reg_memCompTable[i].type = 0;
        }
    }
}

void src1ToV0_opt(int i)
{
    //查找src1是否已经在寄存器里
    int j = findInRegister(midCodeTable[i].src1, i);

    if(j < 0) {     //如果没找到
        //把src1存入$v0寄存器
        if(midCodeTable[i].src1[0] == '$') {     //如果要返回的是临时变量
            if(midCodeTable[i].src1[1] == 'r') {        //如果是$ret
                //值已经在$v0里了
            }
            else {
                int k = locTemp(midCodeTable[i].src1);      //在临时变量表里查找该临时变量
                text_top_opt++;
                strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "lw");
                strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$v0");
                sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "-%d", offsetTempTab[k].offset);
                strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "$sp");              //lw reg, -offset($sp)  //把只存在reg里
            }
        }
        else if(is_letter(midCodeTable[i].src1[0])) {   //如果要返回的声明过的变量/常量
            int k = loc_mips_opt(midCodeTable[i].src1);  //在符号表中查找
            if(tab[k].object == CONST_TAB_VALUE) {  //如果是常量
                text_top_opt++;
                strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "li");
                strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$v0");
                sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "%d", tab[k].offset);      //li $v0, value
            }
            else if(tab[k].object == VAR_TAB_VALUE || tab[k].object == PARA_TAB_VALUE) {   //如果是变量
                if(k > globalTop) {                         //如果是局部变量
                    text_top_opt++;
                    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "lw");
                    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$v0");
                    sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "-%d", tab[k].offset);
                    strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "$sp");     //lw $v0, -offset($sp)
                }
                else {                                      //如果是全局变量
                    text_top_opt++;
                    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "lw");
                    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$v0");
                    sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "%d", tab[k].offset);
                    strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "$gp");                 //lw $v0, offset($gp)
                }
            }
        }
        else if(is_digit(midCodeTable[i].src1[0])) {        //如果要返回的是整数
            text_top_opt++;
            strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "li");
            strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$v0");
            strcpy(mipsTextCodeTable_opt[text_top_opt].op2, midCodeTable[i].src1);      //li $v0, integer
        }
        else if(midCodeTable[i].src1[0] == '\'') {          //如果要返回的是字符
            text_top_opt++;
            strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "li");
            strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$v0");
            strcpy(mipsTextCodeTable_opt[text_top_opt].op2, midCodeTable[i].src1);      //li $v0, character
        }
    }
    else {          //如果找到了
        if(j < 10) {        //如果是t寄存器
            text_top_opt++;
            strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "move");
            strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$v0");
            sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "$t%d", j);
        }
        else {              //如果是s寄存器
            text_top_opt++;
            strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "move");
            strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$v0");
            sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "$s%d", j-10);
        }
    }
}

int src2ToA0_opt(int i)
{
    //把src2存入$a0,并返回src2的类型，int为1，char为11
    int j = findInRegister(midCodeTable[i].src2, i);           //查找src2是否已经在寄存器里

    if(j < 0) {     //如果没找到
        if(midCodeTable[i].src2[0] == '$') {     //如果是临时变量
            if(midCodeTable[i].src2[1] == 'r') {          //如果是$ret
                //把$ret的值从$v0移至$a0
                text_top_opt++;
                strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "move");
                strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$a0");
                strcpy(mipsTextCodeTable_opt[text_top_opt].op2, "$v0");         //move $a0, $v0
                //如果printf后面是ret,那么向上查找最近的call语句
                int l;
                for(l = i; l > 0; l--) {
                    if(midCodeTable[l].op == CALL) {
                        break;
                    }
                }
                if(l == 0) {    //如果没找到call语句，那么一定出错了
                    printf("ERROR: found no call statement in mid code\n");
                    exit(0);
                }
                int k = loc_function(midCodeTable[i].src1);     //在符号表中查找这个函数
                if(tab[k].type == INT_TAB_VALUE) {      //如果这个函数返回的是int
                    return 1;
                }
                else if(tab[k].type == CHAR_TAB_VALUE) {    //如果这个函数返回的是char
                    return 11;
                }
            }
            else {
                int m = locTemp(midCodeTable[i].src2);      //在临时变量表里查找该临时变量
                text_top_opt++;
                strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "lw");
                strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$a0");
                sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "-%d", offsetTempTab[m].offset);
                strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "$sp");              //lw reg, -offset($sp)  //把只存在reg里

                //查找 midCodeTable[i].src2 的类型
                int n;
                int l = (int) strlen(midCodeTable[i].src2);
                char numString[20];
                for (n = 1; n <= l; n++) {
                    numString[n - 1] = midCodeTable[i].src2[n];  //把midCodeTable[i].src2中$后面的部分（即数字部分）复制到numString中
                }
                int k = stringToNum(numString);     //计算midCodeTable[i].src2在tempTab表中的编号
                if (tempTab[k] == INT_TAB_VALUE) {   //如果是int
                    return 1;
                } else if (tempTab[k] == CHAR_TAB_VALUE) { //如果是char
                    return 11;
                }
            }
        }
        else if(is_letter(midCodeTable[i].src2[0])) {   //如果是声明过的变量/常量
            int l = loc_mips_opt(midCodeTable[i].src2);  //在符号表中查找
            if(tab[l].object == CONST_TAB_VALUE) {  //如果是常量
                text_top_opt++;
                strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "li");
                strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$a0");
                sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "%d", tab[l].offset);      //li $a0, value
            }
            else if(tab[l].object == VAR_TAB_VALUE || tab[l].object == PARA_TAB_VALUE) {   //如果是变量
                if(l > globalTop) {                         //如果是局部变量
                    text_top_opt++;
                    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "lw");
                    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$a0");
                    sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "-%d", tab[l].offset);
                    strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "$sp");     //lw $a0, -offset($sp)
                }
                else {                                      //如果是全局变量
                    text_top_opt++;
                    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "lw");
                    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$a0");
                    sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "%d", tab[l].offset);
                    strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "$gp");                 //lw $a0, offset($gp)
                }
            }
            //根据类型输出
            if(tab[l].type == INT_TAB_VALUE) {//如果是int
                return 1;
            }
            else if(tab[l].type == CHAR_TAB_VALUE) { //如果是char
                return 11;
            }
        }
        else if(is_digit(midCodeTable[i].src2[0])) {        //如果要返回的是整数
            text_top_opt++;
            strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "li");
            strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$a0");
            strcpy(mipsTextCodeTable_opt[text_top_opt].op2, midCodeTable[i].src2);      //li $a0, integer
            return 1;
        }
        else if(midCodeTable[i].src2[0] == '\'') {          //如果要返回的是字符
            text_top_opt++;
            strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "li");
            strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$a0");
            strcpy(mipsTextCodeTable_opt[text_top_opt].op2, midCodeTable[i].src2);      //li $a0, character
            return 11;
        }
    }
    else {          //如果找到了
        if(j < 10) {
            text_top_opt++;
            strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "move");
            strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$a0");
            sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "$t%d", j);
        }
        else {
            text_top_opt++;
            strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "move");
            strcpy(mipsTextCodeTable_opt[text_top_opt].op1, "$a0");
            sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "$s%d", j-10);
        }
        if(reg_memCompTable[j].type == 1) {     //如果是变量
            if (reg_memCompTable[j].index < 0) {     //如果src2是临时变量
                //根据类型返回
                if (tempTab[-reg_memCompTable[j].index] == INT_TAB_VALUE) {   //如果是int
                    return 1;
                } else if (tempTab[-reg_memCompTable[j].index] == CHAR_TAB_VALUE) { //如果是char
                    return 11;
                }
            }
            else {                                  //如果src2是声明过的量
                //根据类型返回
                if (tab[reg_memCompTable[j].index].type == INT_TAB_VALUE) {//如果是int
                    return 1;
                } else if (tab[reg_memCompTable[j].index].type == CHAR_TAB_VALUE) { //如果是char
                    return 11;
                }
            }
        }
        else {          //如果是常量
            if(is_digit(midCodeTable[i].src2[0])) {
                return 1;
            }
            else if(midCodeTable[i].src2[0] == '\'') {
                return 11;
            }
        }
    }
    return -1;
}

int src1ToRegister_opt(int i)
{
    int j = findInRegister(midCodeTable[i].src1, i);
    
    if(j < 0) {
        int k = findARegisterToReplace(midCodeTable[i].src1);
        loadSrc1(k, i);
        return k;
    }
    else {
        reg_memCompTable[j].lastUse = i;
        return j;
    }
}

int src2ToRegister_opt(int i)
{
    int j = findInRegister(midCodeTable[i].src2, i);

    if(j < 0) {
        int k = findARegisterToReplace(midCodeTable[i].src2);
        loadSrc2(k, i);
        return k;
    }
    else {
        reg_memCompTable[j].lastUse = i;
        return j;
    }
}

void registerToDst_opt(int reg, int i)
{
    int j = findInRegister(midCodeTable[i].dst, i);

    if(j < 0) {
        int k = findARegisterToReplace(midCodeTable[i].dst);
        loadDst(reg, k, i);      //表面上把dst读取到寄存器k里，实际上不读内存，只是reg复制到k，并标记k为dst
    }
    else {
        loadDst(reg, j, i);     //把reg复制到寄存器j，并标记j为dst
    }
}

int findInRegister(char name[], int i)
{
    /*
     * 查找name是否已经在寄存器里，如果在，返回寄存器编号0-7 10-17 -1
     * 并更新寄存器最后使用
     */
    if(name[0] == '$') {        //如果是临时变量或$ret
        if(name[1] == 'r') {        //如果是$ret
            return -1;
        }
        else {
            int t = -tempVarToNum(name);
            int j;
            for(j = 0; j<=7; j++) {
                if(reg_memCompTable[j].index == t && reg_memCompTable[j].type == 1) {   //如果该临时变量在寄存器中找到了
                    reg_memCompTable[j].lastUse = i;    //更新最后使用
                    return j;                           //返回该寄存器
                }
            }
            return -1;
        }
    }
    else if(is_letter(name[0])) {        //如果是声明过的变量/常量
        int j = loc_mips_opt(name);         //在符号表中查找
        if(tab[j].object == CONST_TAB_VALUE) {      //如果是常量
            int value = tab[j].offset;                  //取常量值
            int k;
            //遍历16个寄存器
            for(k = 0; k<=7; k++) {
                if(reg_memCompTable[k].index == value && reg_memCompTable[k].type == 2) {   //如果该常量在寄存器中找到了
                    reg_memCompTable[k].lastUse = i;      //更新最后使用
                    return k;                           //返回该寄存器
                }
            }
            for(k = 10; k<=17; k++) {
                if(reg_memCompTable[k].index == value && reg_memCompTable[k].type == 2) {   //如果该常量在寄存器中找到了
                    reg_memCompTable[k].lastUse = i;      //更新最后使用
                    return k;                           //返回该寄存器
                }
            }
            return -1;  //如果在寄存器中没找到
        }
        else if(tab[j].object == VAR_TAB_VALUE || tab[j].object == PARA_TAB_VALUE) {   //如果是变量
            if(j > globalTop) {         //如果是局部变量
                int k;
                for(k = 0; k<=7; k++) { //遍历8个寄存器
                    if(reg_memCompTable[k].index == j && reg_memCompTable[k].type == 1) {
                        reg_memCompTable[k].lastUse = i;
                        return k;
                    }
                }
                return -1;
            }
            else {                      //如果是全局变量
                int k;
                for(k = 10; k<=17; k++) {
                    if(reg_memCompTable[k].index == j && reg_memCompTable[k].type == 1) {
                        reg_memCompTable[k].lastUse = i;
                        return k;
                    }
                }
                return -1;
            }
        }
    }
    else if(is_digit(name[0])) {        //如果是整数
        int value = constToInt(name);
        int k;
        //遍历16个寄存器
        for(k = 0; k<=7; k++) {
            if(reg_memCompTable[k].index == value && reg_memCompTable[k].type == 2) {   //如果该常量在寄存器中找到了
                reg_memCompTable[k].lastUse = i;      //更新最后使用
                return k;                           //返回该寄存器
            }
        }
        for(k = 10; k<=17; k++) {
            if(reg_memCompTable[k].index == value && reg_memCompTable[k].type == 2) {   //如果该常量在寄存器中找到了
                reg_memCompTable[k].lastUse = i;      //更新最后使用
                return k;                           //返回该寄存器
            }
        }
        return -1;  //如果在寄存器中没找到
    }
    else if(name[0] == '\'') {          //如果是字符
        int value = constToInt(name);
        int k;
        //遍历16个寄存器
        for(k = 0; k<=7; k++) {
            if(reg_memCompTable[k].index == value && reg_memCompTable[k].type == 2) {   //如果该常量在寄存器中找到了
                reg_memCompTable[k].lastUse = i;      //更新最后使用
                return k;                           //返回该寄存器
            }
        }
        for(k = 10; k<=17; k++) {
            if(reg_memCompTable[k].index == value && reg_memCompTable[k].type == 2) {   //如果该常量在寄存器中找到了
                reg_memCompTable[k].lastUse = i;      //更新最后使用
                return k;                           //返回该寄存器
            }
        }
        return -1;  //如果在寄存器中没找到
    }
    return -1;
}

int findARegisterToReplace(char name[])
{
    /*
     * 根据name类型分配寄存器,并把该寄存器里原来的内容存回内存
     * 可能的返回值 0-7  10-17  20
     */
    if(name[0] == '$') {
        if(name[1] == 'r') {
            return 20;
        }
        else {
            return findTempRegister(name);
        }
    }
    else if(is_letter(name[0])) {
        int j = loc_mips_opt(name);
        if(j > globalTop) {
            return findTempRegister(name);
        }
        else {
            return findGlobalRegister(name);
        }
    }
    else if(is_digit(name[0])) {
        return findAllRegister(name);
    }
    else if(name[0] == '\'') {
        return findAllRegister(name);
    }
    //不可能到达这里
    printf("ERROR: find -1 register\n");
    return -1;
}

int findTempRegister(char name[])
{
    int index;
    if(name[0] == '$') {        //如果是临时变量
        index = -tempVarToNum(name);
    }
    else {                      //如果是局部变量/常量
        index = loc_mips_opt(name);
        if(tab[index].object == CONST_TAB_VALUE) {
            index = tab[index].offset;
        }
    }
    int j;
    //遍历7个临时寄存器
    for(j = 0; j<=7; j++) {
        if(reg_memCompTable[j].type == 0) { //如果有空着的
            reg_memCompTable[j].index = index;
            return j;
        }
    }
    //如果没找到空着的，再遍历一次，找到最久没使用的
    int minLastUse = 100000;
    int minLastUseNum = -1;
    for(j = 0; j<=7; j++) {
        if(reg_memCompTable[j].lastUse < minLastUse) {      //如果寄存器j比已经找到的最久没使用的更久没使用过
            minLastUse = reg_memCompTable[j].lastUse;
            minLastUseNum = j;
        }
    }
    //把寄存器内容写回内存
    if(reg_memCompTable[minLastUseNum].type == 1) {
        if(reg_memCompTable[minLastUseNum].index > 0) {
            if(reg_memCompTable[minLastUseNum].index > globalTop) {                         //如果是局部变量
                text_top_opt++;
                strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "sw");
                sprintf(mipsTextCodeTable_opt[text_top_opt].op1, "$t%d", minLastUseNum);
                sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "-%d", tab[reg_memCompTable[minLastUseNum].index].offset);
                strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "$sp");     //sw reg, -offset($sp)
            }
            else {                                      //如果是全局变量
                text_top_opt++;
                strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "sw");
                sprintf(mipsTextCodeTable_opt[text_top_opt].op1, "$t%d", minLastUseNum);
                sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "%d", tab[reg_memCompTable[minLastUseNum].index].offset);
                strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "$gp");                 //sw reg, offset($gp)
            }
        }
        else if(reg_memCompTable[minLastUseNum].index < 0) {
            char tempVar[20];
            sprintf(tempVar, "$%d", -reg_memCompTable[minLastUseNum].index);
            int k = locTemp(tempVar);      //在临时变量表里查找该临时变量
            text_top_opt++;
            strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "sw");
            sprintf(mipsTextCodeTable_opt[text_top_opt].op1, "$t%d", minLastUseNum);
            sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "-%d", offsetTempTab[k].offset);
            strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "$sp");              //sw reg, -offset($sp)
        }
    }
    //修改寄存器
    reg_memCompTable[minLastUseNum].index = index;
    return minLastUseNum;
}

int findGlobalRegister(char name[])
{
    //name一定是全局变量/常量
    int k = loc_mips_opt(name);
    int index;
    if(tab[k].object == CONST_TAB_VALUE) {
        index = tab[k].offset;
    }
    else {
        index = k;
    }
    //遍历7个全局寄存器
    int j;
    for(j = 10; j<=17; j++) {
        if(reg_memCompTable[j].type == 0) { //如果有空着的
            reg_memCompTable[j].index = index;
            return j;
        }
    }
    //如果没找到空着的，再遍历一次，找到最久没使用的
    int minLastUse = 100000;
    int minLastUseNum = -1;
    for(j = 10; j<=17; j++) {
        if(reg_memCompTable[j].lastUse < minLastUse) {      //如果寄存器j比已经找到的最久没使用的更久没使用过
            minLastUse = reg_memCompTable[j].lastUse;
            minLastUseNum = j;
        }
    }
    //把寄存器内容写回内存
    if(reg_memCompTable[minLastUseNum].type == 1) {
        if(reg_memCompTable[minLastUseNum].index > 0) {
            if(reg_memCompTable[minLastUseNum].index > globalTop) {                         //如果是局部变量
                text_top_opt++;
                strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "sw");
                sprintf(mipsTextCodeTable_opt[text_top_opt].op1, "$s%d", minLastUseNum-10);
                sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "-%d", tab[reg_memCompTable[minLastUseNum].index].offset);
                strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "$sp");     //sw reg, -offset($sp)
            }
            else {                                      //如果是全局变量
                text_top_opt++;
                strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "sw");
                sprintf(mipsTextCodeTable_opt[text_top_opt].op1, "$s%d", minLastUseNum-10);
                sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "%d", tab[reg_memCompTable[minLastUseNum].index].offset);
                strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "$gp");                 //sw reg, offset($gp)
            }
        }
        else if(reg_memCompTable[minLastUseNum].index < 0) {
            char tempVar[20];
            sprintf(tempVar, "$%d", -reg_memCompTable[minLastUseNum].index);
            int l = locTemp(tempVar);      //在临时变量表里查找该临时变量
            text_top_opt++;
            strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "sw");
            sprintf(mipsTextCodeTable_opt[text_top_opt].op1, "$s%d", minLastUseNum-10);
            sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "-%d", offsetTempTab[l].offset);
            strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "$sp");              //sw reg, -offset($sp)
        }
    }
    //修改寄存器
    reg_memCompTable[minLastUseNum].index = index;
    return minLastUseNum;
}

int findAllRegister(char name[])
{
    //name一定是常量
    int index = constToInt(name);
    //遍历7个临时寄存器
    int j;
    for(j = 0; j<=7; j++) {
        if(reg_memCompTable[j].type == 0) { //如果有空着的
            reg_memCompTable[j].index = index;
            return j;
        }
    }
    //遍历7个全局寄存器
    for(j = 10; j<=17; j++) {
        if(reg_memCompTable[j].type == 0) { //如果有空着的
            reg_memCompTable[j].index = index;
            return j;
        }
    }
    //如果没找到空着的，再遍历一次，找到最久没使用的
    int minLastUse = 100000;
    int minLastUseNum = -1;
    for(j = 0; j<=7; j++) {
        if(reg_memCompTable[j].lastUse < minLastUse) {      //如果寄存器j比已经找到的最久没使用的更久没使用过
            minLastUse = reg_memCompTable[j].lastUse;
            minLastUseNum = j;
        }
    }
    for(j = 10; j<=17; j++) {
        if(reg_memCompTable[j].lastUse < minLastUse) {      //如果寄存器j比已经找到的最久没使用的更久没使用过
            minLastUse = reg_memCompTable[j].lastUse;
            minLastUseNum = j;
        }
    }
    //把寄存器内容写回内存
    if(minLastUseNum < 10) {        //如果要写回的是t寄存器
        if(reg_memCompTable[minLastUseNum].type == 1) {
            if(reg_memCompTable[minLastUseNum].index > 0) {
                if(reg_memCompTable[minLastUseNum].index > globalTop) {                         //如果是局部变量
                    text_top_opt++;
                    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "sw");
                    sprintf(mipsTextCodeTable_opt[text_top_opt].op1, "$t%d", minLastUseNum);
                    sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "-%d", tab[reg_memCompTable[minLastUseNum].index].offset);
                    strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "$sp");     //sw reg, -offset($sp)
                }
                else {                                      //如果是全局变量
                    text_top_opt++;
                    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "sw");
                    sprintf(mipsTextCodeTable_opt[text_top_opt].op1, "$t%d", minLastUseNum);
                    sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "%d", tab[reg_memCompTable[minLastUseNum].index].offset);
                    strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "$gp");                 //sw reg, offset($gp)
                }
            }
            else if(reg_memCompTable[minLastUseNum].index < 0) {
                char tempVar[20];
                sprintf(tempVar, "$%d", -reg_memCompTable[minLastUseNum].index);
                int k = locTemp(tempVar);      //在临时变量表里查找该临时变量
                text_top_opt++;
                strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "sw");
                sprintf(mipsTextCodeTable_opt[text_top_opt].op1, "$t%d", minLastUseNum);
                sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "-%d", offsetTempTab[k].offset);
                strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "$sp");              //sw reg, -offset($sp)
            }
        }
    }
    else {                          //如果要写回的是s寄存器
        if(reg_memCompTable[minLastUseNum].type == 1) {
            if(reg_memCompTable[minLastUseNum].index > 0) {
                if(reg_memCompTable[minLastUseNum].index > globalTop) {                         //如果是局部变量
                    text_top_opt++;
                    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "sw");
                    sprintf(mipsTextCodeTable_opt[text_top_opt].op1, "$s%d", minLastUseNum-10);
                    sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "-%d", tab[reg_memCompTable[minLastUseNum].index].offset);
                    strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "$sp");     //sw reg, -offset($sp)
                }
                else {                                      //如果是全局变量
                    text_top_opt++;
                    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "sw");
                    sprintf(mipsTextCodeTable_opt[text_top_opt].op1, "$s%d", minLastUseNum-10);
                    sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "%d", tab[reg_memCompTable[minLastUseNum].index].offset);
                    strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "$gp");                 //sw reg, offset($gp)
                }
            }
            else if(reg_memCompTable[minLastUseNum].index < 0) {
                char tempVar[20];
                sprintf(tempVar, "$%d", -reg_memCompTable[minLastUseNum].index);
                int l = locTemp(tempVar);      //在临时变量表里查找该临时变量
                text_top_opt++;
                strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "sw");
                sprintf(mipsTextCodeTable_opt[text_top_opt].op1, "$s%d", minLastUseNum-10);
                sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "-%d", offsetTempTab[l].offset);
                strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "$sp");              //sw reg, -offset($sp)
            }
        }
    }
    //修改寄存器
    reg_memCompTable[minLastUseNum].index = index;
    return minLastUseNum;
}

void loadSrc1(int dst, int i)
{
    if(dst == 20) {
        return;
    }
    char reg[10];
    if(dst < 10) {
        sprintf(reg, "$t%d", dst);
    }
    else {
        sprintf(reg, "$s%d", dst-10);
    }
    if(midCodeTable[i].src1[0] == '$') {     //如果是临时变量
        if(midCodeTable[i].src1[1] == 'r') {    //如果是$ret
            printf("ERROR: cannot be $ret in loadSrc1\n");
        }
        else {                                  //如果不是$ret
            int j = locTemp(midCodeTable[i].src1);      //在临时变量表里查找该临时变量
            text_top_opt++;
            strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "lw");
            strcpy(mipsTextCodeTable_opt[text_top_opt].op1, reg);
            sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "-%d", offsetTempTab[j].offset);
            strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "$sp");              //lw reg, -offset($sp)  //把只存在reg里
            reg_memCompTable[dst].lastUse = i;
            reg_memCompTable[dst].type = 1;
        }
    }
    else if(is_letter(midCodeTable[i].src1[0])) {   //如果是声明过的变量/常量
        int j = loc_mips_opt(midCodeTable[i].src1);  //在符号表中查找
        if(tab[j].object == CONST_TAB_VALUE) {  //如果是常量
            text_top_opt++;
            strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "li");
            strcpy(mipsTextCodeTable_opt[text_top_opt].op1, reg);
            sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "%d", tab[j].offset);      //li reg, value
            reg_memCompTable[dst].lastUse = i;
            reg_memCompTable[dst].type = 2;
        }
        else if(tab[j].object == VAR_TAB_VALUE || tab[j].object == PARA_TAB_VALUE) {   //如果是变量
            if(j > globalTop) {                         //如果是局部变量
                text_top_opt++;
                strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "lw");
                strcpy(mipsTextCodeTable_opt[text_top_opt].op1, reg);
                sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "-%d", tab[j].offset);
                strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "$sp");     //lw reg, -offset($sp)
                reg_memCompTable[dst].lastUse = i;
                reg_memCompTable[dst].type = 1;
            }
            else {                                      //如果是全局变量
                text_top_opt++;
                strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "lw");
                strcpy(mipsTextCodeTable_opt[text_top_opt].op1, reg);
                sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "%d", tab[j].offset);
                strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "$gp");                 //lw reg, offset($gp)
                reg_memCompTable[dst].lastUse = i;
                reg_memCompTable[dst].type = 1;
            }
        }
    }
    else if(is_digit(midCodeTable[i].src1[0])) {        //如果要返回的是整数
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "li");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, reg);
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, midCodeTable[i].src1);      //li reg, integer
        reg_memCompTable[dst].lastUse = i;
        reg_memCompTable[dst].type = 2;
    }
    else if(midCodeTable[i].src1[0] == '\'') {          //如果要返回的是字符
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "li");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, reg);
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, midCodeTable[i].src1);      //li reg, character
        reg_memCompTable[dst].lastUse = i;
        reg_memCompTable[dst].type = 2;
    }
}

void loadSrc2(int dst ,int i)
{
    if(dst == 20) {
        return;
    }
    char reg[10];
    if(dst < 10) {
        sprintf(reg, "$t%d", dst);
    }
    else {
        sprintf(reg, "$s%d", dst-10);
    }
    if(midCodeTable[i].src2[0] == '$') {     //如果是临时变量
        if(midCodeTable[i].src2[1] == 'r') {    //如果是$ret
            printf("ERROR: cannot be $ret in loadSrc2\n");
        }
        else {
            int j = locTemp(midCodeTable[i].src2);      //在临时变量表里查找该临时变量
            text_top_opt++;
            strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "lw");
            strcpy(mipsTextCodeTable_opt[text_top_opt].op1, reg);
            sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "-%d", offsetTempTab[j].offset);
            strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "$sp");              //lw reg, -offset($sp)  //把只存在reg里
            reg_memCompTable[dst].lastUse = i;
            reg_memCompTable[dst].type = 1;
        }
    }
    else if(is_letter(midCodeTable[i].src2[0])) {   //如果是声明过的变量/常量
        int j = loc_mips_opt(midCodeTable[i].src2);  //在符号表中查找
        if(tab[j].object == CONST_TAB_VALUE) {  //如果是常量
            text_top_opt++;
            strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "li");
            strcpy(mipsTextCodeTable_opt[text_top_opt].op1, reg);
            sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "%d", tab[j].offset);      //li reg, value
            reg_memCompTable[dst].lastUse = i;
            reg_memCompTable[dst].type = 2;
        }
        else if(tab[j].object == VAR_TAB_VALUE || tab[j].object == PARA_TAB_VALUE) {   //如果是变量
            if(j > globalTop) {                         //如果是局部变量
                text_top_opt++;
                strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "lw");
                strcpy(mipsTextCodeTable_opt[text_top_opt].op1, reg);
                sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "-%d", tab[j].offset);
                strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "$sp");     //lw reg, -offset($sp)
                reg_memCompTable[dst].lastUse = i;
                reg_memCompTable[dst].type = 1;
            }
            else {                                      //如果是全局变量
                text_top_opt++;
                strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "lw");
                strcpy(mipsTextCodeTable_opt[text_top_opt].op1, reg);
                sprintf(mipsTextCodeTable_opt[text_top_opt].op2, "%d", tab[j].offset);
                strcpy(mipsTextCodeTable_opt[text_top_opt].op3, "$gp");                 //lw reg, offset($gp)
                reg_memCompTable[dst].lastUse = i;
                reg_memCompTable[dst].type = 1;
            }
        }
    }
    else if(is_digit(midCodeTable[i].src2[0])) {        //如果要返回的是整数
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "li");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, reg);
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, midCodeTable[i].src2);      //li reg, integer
        reg_memCompTable[dst].lastUse = i;
        reg_memCompTable[dst].type = 2;
    }
    else if(midCodeTable[i].src2[0] == '\'') {          //如果要返回的是字符
        text_top_opt++;
        strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "li");
        strcpy(mipsTextCodeTable_opt[text_top_opt].op1, reg);
        strcpy(mipsTextCodeTable_opt[text_top_opt].op2, midCodeTable[i].src2);      //li reg, character
        reg_memCompTable[dst].lastUse = i;
        reg_memCompTable[dst].type = 2;
    }
}

void loadDst(int src, int dst, int i)
{
    if(dst == 20) {
        printf("ERROR: $ret cannot be dst\n");
    }
    char srcReg[10];
    if(src == 20) {
        strcpy(srcReg, "$v0");
    }
    else if(src < 10) {
        sprintf(srcReg, "$t%d", src);
    }
    else {
        sprintf(srcReg, "$s%d", src-10);
    }
    char dstReg[10];
    if(dst < 10) {
        sprintf(dstReg, "$t%d", dst);
    }
    else {
        sprintf(dstReg, "$s%d", dst-10);
    }
    //把src复制到dst
    text_top_opt++;
    strcpy(mipsTextCodeTable_opt[text_top_opt].instr, "move");
    strcpy(mipsTextCodeTable_opt[text_top_opt].op1, dstReg);
    strcpy(mipsTextCodeTable_opt[text_top_opt].op2, srcReg);
    //修改寄存器-内存对照表
    if(midCodeTable[i].dst[0] == '$') {
        reg_memCompTable[dst].lastUse = i;
        reg_memCompTable[dst].index = -tempVarToNum(midCodeTable[i].dst);
        reg_memCompTable[dst].type = 1;
    }
    else if(is_letter(midCodeTable[i].dst[0])) {
        int j = loc_mips_opt(midCodeTable[i].dst);
        reg_memCompTable[dst].lastUse = i;
        reg_memCompTable[dst].index = j;
        reg_memCompTable[dst].type = 1;
    }
}

void getDst(int dst, int i)
{
    //把寄存器dst标记为midCodeTable[i].dst，并更新最后使用
    if(dst == 20) {
        printf("ERROR: $ret cannot be dst\n");
    }
    //修改寄存器-内存对照表
    if(midCodeTable[i].dst[0] == '$') {
        reg_memCompTable[dst].lastUse = i;
        reg_memCompTable[dst].index = -tempVarToNum(midCodeTable[i].dst);
        reg_memCompTable[dst].type = 1;
    }
    else if(is_letter(midCodeTable[i].dst[0])) {
        int j = loc_mips_opt(midCodeTable[i].dst);
        reg_memCompTable[dst].lastUse = i;
        reg_memCompTable[dst].index = j;
        reg_memCompTable[dst].type = 1;
    }
}

void print_mips_code_opt(int o)
{
    FILE * file;
    if(o == 0) {
        file = fopen("mips.asm", "w");
    }
    else if(o == 1){
        file = fopen("mips_opt.asm", "w");
    }
    else {
        file = fopen("mips_opt2.asm", "w");
    }
    printMipsDataCode_opt(file);
    printMipsTextCode_opt(file);
    fclose(file);
}

void printMipsDataCode_opt(FILE * file)
{
    fprintf(file, ".data\n");
    int i;
    for(i = 1; i<=data_top_opt; i++) {
        fprintf(file, "\t%s: %s %s\n", mipsDataCodeTable_opt[i].ident, mipsDataCodeTable_opt[i].instr, mipsDataCodeTable_opt[i].content);
    }
}

void printMipsTextCode_opt(FILE * file)
{
    fprintf(file, ".text\n");
    int i;
    for(i = 1; i<=text_top_opt; i++) {
        if(strcmp(mipsTextCodeTable_opt[i].instr, "add") == 0) {
            fprintf(file, "\t%s %s, %s, %s\n", mipsTextCodeTable_opt[i].instr, mipsTextCodeTable_opt[i].op1, mipsTextCodeTable_opt[i].op2, mipsTextCodeTable_opt[i].op3);
        }
        else if(strcmp(mipsTextCodeTable_opt[i].instr, "addi") == 0) {
            fprintf(file, "\t%s %s, %s, %s\n", mipsTextCodeTable_opt[i].instr, mipsTextCodeTable_opt[i].op1, mipsTextCodeTable_opt[i].op2, mipsTextCodeTable_opt[i].op3);
        }
        else if(strcmp(mipsTextCodeTable_opt[i].instr, "addiu") == 0) {
            fprintf(file, "\t%s %s, %s, %s\n", mipsTextCodeTable_opt[i].instr, mipsTextCodeTable_opt[i].op1, mipsTextCodeTable_opt[i].op2, mipsTextCodeTable_opt[i].op3);
        }
        else if(strcmp(mipsTextCodeTable_opt[i].instr, "addu") == 0) {
            fprintf(file, "\t%s %s, %s, %s\n", mipsTextCodeTable_opt[i].instr, mipsTextCodeTable_opt[i].op1, mipsTextCodeTable_opt[i].op2, mipsTextCodeTable_opt[i].op3);
        }
        else if(strcmp(mipsTextCodeTable_opt[i].instr, "beq") == 0) {
            fprintf(file, "\t%s %s, %s, %s\n", mipsTextCodeTable_opt[i].instr, mipsTextCodeTable_opt[i].op1, mipsTextCodeTable_opt[i].op2, mipsTextCodeTable_opt[i].op3);
        }
        else if(strcmp(mipsTextCodeTable_opt[i].instr, "bgez") == 0) {
            fprintf(file, "\t%s %s, %s\n", mipsTextCodeTable_opt[i].instr, mipsTextCodeTable_opt[i].op1, mipsTextCodeTable_opt[i].op2);
        }
        else if(strcmp(mipsTextCodeTable_opt[i].instr, "bgtz") == 0) {
            fprintf(file, "\t%s %s, %s\n", mipsTextCodeTable_opt[i].instr, mipsTextCodeTable_opt[i].op1, mipsTextCodeTable_opt[i].op2);
        }
        else if(strcmp(mipsTextCodeTable_opt[i].instr, "blez") == 0) {
            fprintf(file, "\t%s %s, %s\n", mipsTextCodeTable_opt[i].instr, mipsTextCodeTable_opt[i].op1, mipsTextCodeTable_opt[i].op2);
        }
        else if(strcmp(mipsTextCodeTable_opt[i].instr, "bltz") == 0) {
            fprintf(file, "\t%s %s, %s\n", mipsTextCodeTable_opt[i].instr, mipsTextCodeTable_opt[i].op1, mipsTextCodeTable_opt[i].op2);
        }
        else if(strcmp(mipsTextCodeTable_opt[i].instr, "bne") == 0) {
            fprintf(file, "\t%s %s, %s, %s\n", mipsTextCodeTable_opt[i].instr, mipsTextCodeTable_opt[i].op1, mipsTextCodeTable_opt[i].op2, mipsTextCodeTable_opt[i].op3);
        }
        else if(strcmp(mipsTextCodeTable_opt[i].instr, "div") == 0) {
            fprintf(file, "\t%s %s, %s\n", mipsTextCodeTable_opt[i].instr, mipsTextCodeTable_opt[i].op1, mipsTextCodeTable_opt[i].op2);
        }
        else if(strcmp(mipsTextCodeTable_opt[i].instr, "j") == 0) {
            fprintf(file, "\t%s %s\n", mipsTextCodeTable_opt[i].instr, mipsTextCodeTable_opt[i].op1);
        }
        else if(strcmp(mipsTextCodeTable_opt[i].instr, "jal") == 0) {
            fprintf(file, "\t%s %s\n", mipsTextCodeTable_opt[i].instr, mipsTextCodeTable_opt[i].op1);
        }
        else if(strcmp(mipsTextCodeTable_opt[i].instr, "jr") == 0) {
            fprintf(file, "\t%s %s\n", mipsTextCodeTable_opt[i].instr, mipsTextCodeTable_opt[i].op1);
        }
        else if(strcmp(mipsTextCodeTable_opt[i].instr, "la") == 0) {
            fprintf(file, "\t%s %s, %s\n", mipsTextCodeTable_opt[i].instr, mipsTextCodeTable_opt[i].op1, mipsTextCodeTable_opt[i].op2);
        }
        else if(strcmp(mipsTextCodeTable_opt[i].instr, "li") == 0) {
            fprintf(file, "\t%s %s, %s\n", mipsTextCodeTable_opt[i].instr, mipsTextCodeTable_opt[i].op1, mipsTextCodeTable_opt[i].op2);
        }
        else if(strcmp(mipsTextCodeTable_opt[i].instr, "lw") == 0) {
            fprintf(file, "\t%s %s, %s(%s)\n", mipsTextCodeTable_opt[i].instr, mipsTextCodeTable_opt[i].op1, mipsTextCodeTable_opt[i].op2, mipsTextCodeTable_opt[i].op3);
        }
        else if(strcmp(mipsTextCodeTable_opt[i].instr, "mflo") == 0) {
            fprintf(file, "\t%s %s\n", mipsTextCodeTable_opt[i].instr, mipsTextCodeTable_opt[i].op1);
        }
        else if(strcmp(mipsTextCodeTable_opt[i].instr, "move") == 0) {
            fprintf(file, "\t%s %s, %s\n", mipsTextCodeTable_opt[i].instr, mipsTextCodeTable_opt[i].op1, mipsTextCodeTable_opt[i].op2);
        }
        else if(strcmp(mipsTextCodeTable_opt[i].instr, "mult") == 0) {
            fprintf(file, "\t%s %s, %s\n", mipsTextCodeTable_opt[i].instr, mipsTextCodeTable_opt[i].op1, mipsTextCodeTable_opt[i].op2);
        }
        else if(strcmp(mipsTextCodeTable_opt[i].instr, "sll") == 0) {
            fprintf(file, "\t%s %s, %s, %s\n", mipsTextCodeTable_opt[i].instr, mipsTextCodeTable_opt[i].op1, mipsTextCodeTable_opt[i].op2, mipsTextCodeTable_opt[i].op3);
        }
        else if(strcmp(mipsTextCodeTable_opt[i].instr, "sub") == 0) {
            fprintf(file, "\t%s %s, %s, %s\n", mipsTextCodeTable_opt[i].instr, mipsTextCodeTable_opt[i].op1, mipsTextCodeTable_opt[i].op2, mipsTextCodeTable_opt[i].op3);
        }
        else if(strcmp(mipsTextCodeTable_opt[i].instr, "subu") == 0) {
            fprintf(file, "\t%s %s, %s, %s\n", mipsTextCodeTable_opt[i].instr, mipsTextCodeTable_opt[i].op1, mipsTextCodeTable_opt[i].op2, mipsTextCodeTable_opt[i].op3);
        }
        else if(strcmp(mipsTextCodeTable_opt[i].instr, "sw") == 0) {
            fprintf(file, "\t%s %s, %s(%s)\n", mipsTextCodeTable_opt[i].instr, mipsTextCodeTable_opt[i].op1, mipsTextCodeTable_opt[i].op2, mipsTextCodeTable_opt[i].op3);
        }
        else if(strcmp(mipsTextCodeTable_opt[i].instr, "syscall") == 0) {
            fprintf(file, "\t%s\n", mipsTextCodeTable_opt[i].instr);
        }
        else {
            fprintf(file, "%s\n", mipsTextCodeTable_opt[i].instr);
        }
    }
}

/*
 * 对寄存器分配进行优化
 * 当把一个变量读入寄存器时，记录寄存器存储的是哪个变量
 * 当调用一个变量时，先检查该变量是否已经在寄存器里，如果是，直接从寄存器操作，如果不是，读入寄存器
 * 当call时，把寄存器里的变量全部写回内存，然后把寄存器-变量对照表清空
 * $t0-$t7留给局部变量和临时变量
 * $t8 $t9留给if while switch array_assign +-* /
 * $s0-$s7留给全局变量
 */

void getRegNameByNum(int regNum, char regName[])
{
    //0-9表示$t0-$t9, 10-17表示$s0-$s7, 20表示$v0
    if(regNum == 20) {
        strcpy(regName, "$v0");
    }
    else if(regNum < 10) {          //把src1Reg,src2Reg转成寄存器名
        sprintf(regName, "$t%d", regNum);
    }
    else {
        sprintf(regName, "$s%d", regNum-10);
    }
}

int loc_mips_opt(char name[])
{
    /*
     * 因为在生成mips时，符号表已经全部构建完成，所以不能再用原来的loc函数查找变量/常量
     * 在这里采用新的方法：
     * 在生成mips时，每当遇到函数声明时，就将tempFunction置为函数名
     * 在查找变量/常量时，根据当前位置的函数进行查找。具体查找方法：
     * 1.在符号表中查找到当前函数的位置
     * 2.在此位置的基础山向后找，直到找到下一个函数（或符号表结尾）
     * 3.在下一个函数（或符号表结尾）的基础上，将指针前移一个值（即指向当前函数的最后一个变量/常量）
     * 4.接下来按照loc的算法进行查找
     */
    int seek_top = 0;
    if(strcmp(tempFunction_opt, "") == 0) {     //如果在全局变量区
        seek_top = globalTop;
    }
    else {      //1.在符号表中查找到当前函数的位置
        int i;
        for (i = tab_top; i > 0; i--) {
            if(strcmp(tab[i].name, tempFunction_opt) == 0 && tab[i].object == FUNC_TAB_VALUE) {
                seek_top = i;
                break;
            }
        }
    }
    //2.在此位置的基础山向后找，直到找到下一个函数（或符号表结尾）
    for(seek_top++ ; seek_top <= tab_top; seek_top++) {
        if(tab[seek_top].object == FUNC_TAB_VALUE) {
            break;
        }
    }
    seek_top--;     //3.在下一个函数（或符号表结尾）的基础上，将指针前移一个值（即指向当前函数的最后一个变量/常量）
    int ptr = seek_top;
    strcpy(tab[0].name, name);
    while(ptr >= 0) {
        if(strcmp(name, tab[ptr].name) == 0) {
            break;
        }
        ptr = tab[ptr].link;
    }
    return ptr;
}