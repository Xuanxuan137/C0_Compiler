#include "generate_mips_code.h"



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

struct mips_data_code mipsDataCodeTable[MIPS_DATA_LENGTH];
struct mips_text_code mipsTextCodeTable[MIPS_TEXT_LENGTH];

int data_top = 0;       //data区指令栈顶指针
int text_top = 0;       //text区指令栈顶指针

int stringNum = 0;      //记录printf中string的编号


char tempFunction[TOKENMAXLENGTH] = "";


void generate_mips_code()
{
    data_top = 0;
    text_top = 0;
    stringNum = 0;
//    total_push = 0;
    //为全局变量申请空间
    int space_for_global = 0;
    if(tab[globalTop].object == CONST_TAB_VALUE || tab[globalTop].object == VAR_TAB_VALUE) {
        space_for_global = tab[globalTop].offset + 4;
    }
    else if(tab[globalTop].object == ARRAY_TAB_VALUE) {
        space_for_global = tab[globalTop].offset + 4*tab[globalTop].size;
    }

    data_top++;
    strcpy(mipsDataCodeTable[data_top].ident, "$global");
    strcpy(mipsDataCodeTable[data_top].instr, ".space");
    sprintf(mipsDataCodeTable[data_top].content, "%d", space_for_global+100);           //$global: .space   space_for_global+100

    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "la");
    strcpy(mipsTextCodeTable[text_top].op1, "$gp");
    strcpy(mipsTextCodeTable[text_top].op2, "$global");                     //la $gp, $global       //把全局变量起始地址存入$gp

    //为换行符申请一个空间，用于在每个printf后输出
    data_top++;
    strcpy(mipsDataCodeTable[data_top].ident, "$newLine");
    strcpy(mipsDataCodeTable[data_top].instr, ".asciiz");
    strcpy(mipsDataCodeTable[data_top].content, "\"\\n\"");  //$newLine: .asciiz "\n"


    int i = global_top_in_mid + 1;


    //生成全局变量后面部分的指令
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "move");
    strcpy(mipsTextCodeTable[text_top].op1, "$fp");
    strcpy(mipsTextCodeTable[text_top].op2, "$sp");     //move $fp, $sp     //程序开始第一步，把fp置为sp
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "jal");
    strcpy(mipsTextCodeTable[text_top].op1, "main");    //jal main            //第二步，跳到main
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "li");
    strcpy(mipsTextCodeTable[text_top].op1, "$v0");
    strcpy(mipsTextCodeTable[text_top].op2, "10");      //li $v0, 10
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "syscall");   //syscall   结束程序
    for(; i<=mid_code_top; i++) {
        //第一步，生成label
        if(midCodeTable[i].label != 0) {
            text_top++;
            if(midCodeTable[i].op == FUNCINT || midCodeTable[i].op == FUNCCHAR || midCodeTable[i].op == FUNCVOID) {
                sprintf(mipsTextCodeTable[text_top].instr, "%s:", midCodeTable[i].dst); //如果是函数声明，以函数名作为label
                strcpy(tempFunction, midCodeTable[i].dst);        //记录当前函数名，以便查找变量/常量
            }
            else {
                sprintf(mipsTextCodeTable[text_top].instr, "$label%d:", midCodeTable[i].label); //普通label以label加labelNum为label
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
            generate_mips_code_3_4(i);
        }
        else if(midCodeTable[i].op == 5 || midCodeTable[i].op == 6) {   //ARRAYINT ARRAYCHAR
            generate_mips_code_5_6(i);
        }
        else if(midCodeTable[i].op == 7 || midCodeTable[i].op == 8 || midCodeTable[i].op == 9) { //FUNCINT FUNCCHAR FUNCVOID
            generate_mips_code_7_8_9(i);
        }
        else if(midCodeTable[i].op == 10 || midCodeTable[i].op == 11) { //PARAINT PARACHAR
            generate_mips_code_10_11(i);
        }
        else if(midCodeTable[i].op == 12) {     //RET
            generate_mips_code_12(i);
        }
        else if(midCodeTable[i].op == 13) {     //IF_LESS
            generate_mips_code_13(i);
        }
        else if(midCodeTable[i].op == 14) {     //IF_LESSEQUAL
            generate_mips_code_14(i);
        }
        else if(midCodeTable[i].op == 15) {     //IF_MORE
            generate_mips_code_15(i);
        }
        else if(midCodeTable[i].op == 16) {     //IF_MOREEQUAL
            generate_mips_code_16(i);
        }
        else if(midCodeTable[i].op == 17) {     //IF_NOTEQUAL
            generate_mips_code_17(i);
        }
        else if(midCodeTable[i].op == 18) {     //IF_EQUAL
            generate_mips_code_18(i);
        }
        else if(midCodeTable[i].op == 19) {     //GOTO
            generate_mips_code_19(i);
        }
        else if(midCodeTable[i].op == 20) {     //WHILE_LESS
            generate_mips_code_20(i);
        }
        else if(midCodeTable[i].op == 21) {     //WHILE_LESSEQUAL
            generate_mips_code_21(i);
        }
        else if(midCodeTable[i].op == 22) {     //WHILE_MORE
            generate_mips_code_22(i);
        }
        else if(midCodeTable[i].op == 23) {     //WHILE_MOREEQUAL
            generate_mips_code_23(i);
        }
        else if(midCodeTable[i].op == 24) {     //WHILE_NOTEQUAL
            generate_mips_code_24(i);
        }
        else if(midCodeTable[i].op == 25) {     //WHILE_EQUAL
            generate_mips_code_25(i);
        }
        else if(midCodeTable[i].op == 26) {     //SCANF
            generate_mips_code_26(i);
        }
        else if(midCodeTable[i].op == 27) {     //PRINTF_OP1
            generate_mips_code_27(i);
        }
        else if(midCodeTable[i].op == 28) {     //PRINTF_OP2
            generate_mips_code_28(i);
        }
        else if(midCodeTable[i].op == 29) {     //PRINTF_OP3
            generate_mips_code_29(i);
        }
        else if(midCodeTable[i].op == 30) {     //ASSIGN
            generate_mips_code_30(i);
        }
        else if(midCodeTable[i].op == 31) {     //ARRAY_ASSIGN
            generate_mips_code_31(i);
        }
        else if(midCodeTable[i].op == 32) {     //CALL
            generate_mips_code_32(i);
        }
        else if(midCodeTable[i].op == 33) {     //CASE
            generate_mips_code_33(i);
        }
        else if(midCodeTable[i].op == 34) {     //DEFAULT
            //default行不生成指令
        }
        else if(midCodeTable[i].op == 35) {     //PUSH
            generate_mips_code_35(i);
        }
        else if(midCodeTable[i].op == 36) {     //PLUS
            generate_mips_code_36(i);
        }
        else if(midCodeTable[i].op == 37) {     //MINUS
            generate_mips_code_37(i);
        }
        else if(midCodeTable[i].op == 38) {     //MULTIPLY
            generate_mips_code_38(i);
        }
        else if(midCodeTable[i].op == 39) {     //DIVIDE
            generate_mips_code_39(i);
        }
        else if(midCodeTable[i].op == 40) {     //ARRAY_GET
            generate_mips_code_40(i);
        }
        else if(midCodeTable[i].op == 41) {     //RET_EXTRA
            generate_mips_code_41(i);
        }
        else if(midCodeTable[i].op == 42) {     //WHILE_START
            //无操作不生成指令
        }
    }
}

int tempVarToNum(char string[])
{
    int i;
    int l = (int)strlen(string);
    int sum = 0;
    for(i = 1; i<l; i++) {
        sum *= 10;
        sum += string[i]-'0';
    }
    return sum;
}

int is_letter(char ch)
{
    if(ch == '_' || (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'))
        return 1;
    return 0;
}

int is_digit(char ch)
{
    if(ch == '+' || ch == '-' || (ch >= '0' && ch <= '9'))
        return 1;
    return 0;
}

void generate_mips_code_3_4(int i)
{
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "addiu");
    strcpy(mipsTextCodeTable[text_top].op1, "$fp");
    strcpy(mipsTextCodeTable[text_top].op2, "$fp");
    strcpy(mipsTextCodeTable[text_top].op3, "-4");      //add $fp, $fp, -4  为变量分配空间
}

void generate_mips_code_5_6(int i)
{
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "addiu");
    strcpy(mipsTextCodeTable[text_top].op1, "$fp");
    strcpy(mipsTextCodeTable[text_top].op2, "$fp");
    int size = stringToNum(midCodeTable[i].src1);       //把中间代码中存储的字符串格式的数组长度转为整数
    sprintf(mipsTextCodeTable[text_top].op3, "-%d", size*4);    //add $fp, $fp, size*4  为数组分配空间
}

void generate_mips_code_7_8_9(int i)
{
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "sw");
    strcpy(mipsTextCodeTable[text_top].op1, "$ra");
    strcpy(mipsTextCodeTable[text_top].op2, "0");
    strcpy(mipsTextCodeTable[text_top].op3, "$fp");     //sw $ra, 0($fp)
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "sw");
    strcpy(mipsTextCodeTable[text_top].op1, "$sp");
    strcpy(mipsTextCodeTable[text_top].op2, "-4");
    strcpy(mipsTextCodeTable[text_top].op3, "$fp");     //sw $sp, -4($fp)
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "move");
    strcpy(mipsTextCodeTable[text_top].op1, "$sp");
    strcpy(mipsTextCodeTable[text_top].op2, "$fp");     //move $sp, $fp
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "addiu");
    strcpy(mipsTextCodeTable[text_top].op1, "$fp");
    strcpy(mipsTextCodeTable[text_top].op2, "$fp");
    strcpy(mipsTextCodeTable[text_top].op3, "-8");      //addi $fp, $fp, -8
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
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "addiu");
    strcpy(mipsTextCodeTable[text_top].op1, "$fp");
    strcpy(mipsTextCodeTable[text_top].op2, "$fp");
    sprintf(mipsTextCodeTable[text_top].op3, "-%d", num*4+4);       //addi $fp, $fp, -(num*4+4)  //+4是为了多分一点保险
}

void generate_mips_code_10_11(int i)
{
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "addiu");
    strcpy(mipsTextCodeTable[text_top].op1, "$fp");
    strcpy(mipsTextCodeTable[text_top].op2, "$fp");
    strcpy(mipsTextCodeTable[text_top].op3, "-4");      //add $fp, $fp, -4  为参数分配空间
}

void generate_mips_code_12(int i)
{
    //首先把要返回的值存在$v0里
    if(midCodeTable[i].src1[0] == '$') {     //如果要返回的是临时变量
        if(midCodeTable[i].src1[1] == 'r') {        //如果是$ret
            //值已经在$v0里了
        }
        else {
            int j = locTemp(midCodeTable[i].src1);      //在临时变量表里查找该临时变量
            text_top++;
            strcpy(mipsTextCodeTable[text_top].instr, "lw");
            strcpy(mipsTextCodeTable[text_top].op1, "$v0");
            sprintf(mipsTextCodeTable[text_top].op2, "-%d", offsetTempTab[j].offset);
            strcpy(mipsTextCodeTable[text_top].op3, "$sp");              //lw reg, -offset($sp)  //把只存在reg里
        }
    }
    else if(is_letter(midCodeTable[i].src1[0])) {   //如果要返回的声明过的变量/常量
        int j = loc_mips(midCodeTable[i].src1);  //在符号表中查找
        if(tab[j].object == CONST_TAB_VALUE) {  //如果是常量
            text_top++;
            strcpy(mipsTextCodeTable[text_top].instr, "li");
            strcpy(mipsTextCodeTable[text_top].op1, "$v0");
            sprintf(mipsTextCodeTable[text_top].op2, "%d", tab[j].offset);      //li $v0, value
        }
        else if(tab[j].object == VAR_TAB_VALUE || tab[j].object == PARA_TAB_VALUE) {   //如果是变量
            if(j > globalTop) {                         //如果是局部变量
                text_top++;
                strcpy(mipsTextCodeTable[text_top].instr, "lw");
                strcpy(mipsTextCodeTable[text_top].op1, "$v0");
                sprintf(mipsTextCodeTable[text_top].op2, "-%d", tab[j].offset);
                strcpy(mipsTextCodeTable[text_top].op3, "$sp");     //lw $v0, -offset($sp)
            }
            else {                                      //如果是全局变量
                text_top++;
                strcpy(mipsTextCodeTable[text_top].instr, "lw");
                strcpy(mipsTextCodeTable[text_top].op1, "$v0");
                sprintf(mipsTextCodeTable[text_top].op2, "%d", tab[j].offset);
                strcpy(mipsTextCodeTable[text_top].op3, "$gp");                 //lw $v0, offset($gp)
            }
        }
    }
    else if(is_digit(midCodeTable[i].src1[0])) {        //如果要返回的是整数
        text_top++;
        strcpy(mipsTextCodeTable[text_top].instr, "li");
        strcpy(mipsTextCodeTable[text_top].op1, "$v0");
        strcpy(mipsTextCodeTable[text_top].op2, midCodeTable[i].src1);      //li $v0, integer
    }
    else if(midCodeTable[i].src1[0] == '\'') {          //如果要返回的是字符
        text_top++;
        strcpy(mipsTextCodeTable[text_top].instr, "li");
        strcpy(mipsTextCodeTable[text_top].op1, "$v0");
        strcpy(mipsTextCodeTable[text_top].op2, midCodeTable[i].src1);      //li $v0, character
    }
    //存好后，对sp和fp进行调整，以便返回
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "lw");
    strcpy(mipsTextCodeTable[text_top].op1, "$ra");
    strcpy(mipsTextCodeTable[text_top].op2, "0");
    strcpy(mipsTextCodeTable[text_top].op3, "$sp");     //lw $ra, 0($sp)
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "move");
    strcpy(mipsTextCodeTable[text_top].op1, "$fp");
    strcpy(mipsTextCodeTable[text_top].op2, "$sp");     //move, $fp, $sp
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "lw");
    strcpy(mipsTextCodeTable[text_top].op1, "$sp");
    strcpy(mipsTextCodeTable[text_top].op2, "-4");
    strcpy(mipsTextCodeTable[text_top].op3, "$sp");     //lw $sp, -4($sp)
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "jr");
    strcpy(mipsTextCodeTable[text_top].op1, "$ra");     //jr $ra
}

void generate_mips_code_13(int i)
{
    //先取src1的值,存在$t0里
    src1ToRegister("$t0", i);
    //再取src2的值,存在$t1里
    src2ToRegister("$t1", i);
    //然后用$t0-$t1，存在$t2里
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "subu");
    strcpy(mipsTextCodeTable[text_top].op1, "$t2");
    strcpy(mipsTextCodeTable[text_top].op2, "$t0");
    strcpy(mipsTextCodeTable[text_top].op3, "$t1");         //sub $t2, $t0, $t1
    //由于if中的条件符号为<
    //所以当t2小于0时不跳转，t2>=0时跳转
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "bgez");
    strcpy(mipsTextCodeTable[text_top].op1, "$t2");
    sprintf(mipsTextCodeTable[text_top].op2, "$label%s", midCodeTable[i].dst);   //bgez $t2, label
}

void generate_mips_code_14(int i)
{
    //先取src1的值,存在$t0里
    src1ToRegister("$t0", i);
    //再取src2的值,存在$t1里
    src2ToRegister("$t1", i);
    //然后用$t0-$t1，存在$t2里
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "subu");
    strcpy(mipsTextCodeTable[text_top].op1, "$t2");
    strcpy(mipsTextCodeTable[text_top].op2, "$t0");
    strcpy(mipsTextCodeTable[text_top].op3, "$t1");         //sub $t2, $t0, $t1
    //由于if中的条件符号为<=
    //所以当t2<=0时不跳转，t2>0时跳转
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "bgtz");
    strcpy(mipsTextCodeTable[text_top].op1, "$t2");
    sprintf(mipsTextCodeTable[text_top].op2, "$label%s", midCodeTable[i].dst);   //bgez $t2, label
}

void generate_mips_code_15(int i)
{
    //先取src1的值,存在$t0里
    src1ToRegister("$t0", i);
    //再取src2的值,存在$t1里
    src2ToRegister("$t1", i);
    //然后用$t0-$t1，存在$t2里
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "subu");
    strcpy(mipsTextCodeTable[text_top].op1, "$t2");
    strcpy(mipsTextCodeTable[text_top].op2, "$t0");
    strcpy(mipsTextCodeTable[text_top].op3, "$t1");         //sub $t2, $t0, $t1
    //由于if中的条件符号为>
    //所以当t2>0时不跳转，t2<=0时跳转
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "blez");
    strcpy(mipsTextCodeTable[text_top].op1, "$t2");
    sprintf(mipsTextCodeTable[text_top].op2, "$label%s", midCodeTable[i].dst);   //bgez $t2, label
}

void generate_mips_code_16(int i)
{
    //先取src1的值,存在$t0里
    src1ToRegister("$t0", i);
    //再取src2的值,存在$t1里
    src2ToRegister("$t1", i);
    //然后用$t0-$t1，存在$t2里
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "subu");
    strcpy(mipsTextCodeTable[text_top].op1, "$t2");
    strcpy(mipsTextCodeTable[text_top].op2, "$t0");
    strcpy(mipsTextCodeTable[text_top].op3, "$t1");         //sub $t2, $t0, $t1
    //由于if中的条件符号为>=
    //所以当t2>=0时不跳转，t2<0时跳转
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "bltz");
    strcpy(mipsTextCodeTable[text_top].op1, "$t2");
    sprintf(mipsTextCodeTable[text_top].op2, "$label%s", midCodeTable[i].dst);   //bgez $t2, label
}

void generate_mips_code_17(int i)
{
    //先取src1的值,存在$t0里
    src1ToRegister("$t0", i);
    //再取src2的值,存在$t1里
    src2ToRegister("$t1", i);
    //由于if中的条件符号为!=
    //所以当t0!=t1时不跳转，t0==t1时跳转
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "beq");
    strcpy(mipsTextCodeTable[text_top].op1, "$t0");
    strcpy(mipsTextCodeTable[text_top].op2, "$t1");
    sprintf(mipsTextCodeTable[text_top].op3, "$label%s", midCodeTable[i].dst);   //beq $t0, $t1, label
}

void generate_mips_code_18(int i)
{
    //先取src1的值,存在$t0里
    src1ToRegister("$t0", i);
    //再取src2的值,存在$t1里
    src2ToRegister("$t1", i);
    //由于if中的条件符号为==
    //所以当t0==t1时不跳转，t0!=t1时跳转
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "bne");
    strcpy(mipsTextCodeTable[text_top].op1, "$t0");
    strcpy(mipsTextCodeTable[text_top].op2, "$t1");
    sprintf(mipsTextCodeTable[text_top].op3, "$label%s", midCodeTable[i].dst);   //beq $t0, $t1, label
}

void generate_mips_code_19(int i)
{
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "j");
    sprintf(mipsTextCodeTable[text_top].op1, "$label%s", midCodeTable[i].dst);      //j label
}

void generate_mips_code_20(int i)
{
    //先取src1的值,存在$t0里
    src1ToRegister("$t0", i);
    //再取src2的值,存在$t1里
    src2ToRegister("$t1", i);
    //然后用$t0-$t1，存在$t2里
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "subu");
    strcpy(mipsTextCodeTable[text_top].op1, "$t2");
    strcpy(mipsTextCodeTable[text_top].op2, "$t0");
    strcpy(mipsTextCodeTable[text_top].op3, "$t1");         //sub $t2, $t0, $t1
    //由于if中的条件符号为<
    //所以当t2小于0时不跳转，t2>=0时跳转
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "bgez");
    strcpy(mipsTextCodeTable[text_top].op1, "$t2");
    sprintf(mipsTextCodeTable[text_top].op2, "$label%s", midCodeTable[i].dst);   //bgez $t2, label
}

void generate_mips_code_21(int i)
{
    //先取src1的值,存在$t0里
    src1ToRegister("$t0", i);
    //再取src2的值,存在$t1里
    src2ToRegister("$t1", i);
    //然后用$t0-$t1，存在$t2里
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "subu");
    strcpy(mipsTextCodeTable[text_top].op1, "$t2");
    strcpy(mipsTextCodeTable[text_top].op2, "$t0");
    strcpy(mipsTextCodeTable[text_top].op3, "$t1");         //sub $t2, $t0, $t1
    //由于if中的条件符号为<=
    //所以当t2<=0时不跳转，t2>0时跳转
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "bgtz");
    strcpy(mipsTextCodeTable[text_top].op1, "$t2");
    sprintf(mipsTextCodeTable[text_top].op2, "$label%s", midCodeTable[i].dst);   //bgez $t2, label
}

void generate_mips_code_22(int i)
{
    //先取src1的值,存在$t0里
    src1ToRegister("$t0", i);
    //再取src2的值,存在$t1里
    src2ToRegister("$t1", i);
    //然后用$t0-$t1，存在$t2里
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "subu");
    strcpy(mipsTextCodeTable[text_top].op1, "$t2");
    strcpy(mipsTextCodeTable[text_top].op2, "$t0");
    strcpy(mipsTextCodeTable[text_top].op3, "$t1");         //sub $t2, $t0, $t1
    //由于if中的条件符号为>
    //所以当t2>0时不跳转，t2<=0时跳转
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "blez");
    strcpy(mipsTextCodeTable[text_top].op1, "$t2");
    sprintf(mipsTextCodeTable[text_top].op2, "$label%s", midCodeTable[i].dst);   //bgez $t2, label
}

void generate_mips_code_23(int i)
{
    //先取src1的值,存在$t0里
    src1ToRegister("$t0", i);
    //再取src2的值,存在$t1里
    src2ToRegister("$t1", i);
    //然后用$t0-$t1，存在$t2里
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "subu");
    strcpy(mipsTextCodeTable[text_top].op1, "$t2");
    strcpy(mipsTextCodeTable[text_top].op2, "$t0");
    strcpy(mipsTextCodeTable[text_top].op3, "$t1");         //sub $t2, $t0, $t1
    //由于if中的条件符号为>=
    //所以当t2>=0时不跳转，t2<0时跳转
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "bltz");
    strcpy(mipsTextCodeTable[text_top].op1, "$t2");
    sprintf(mipsTextCodeTable[text_top].op2, "$label%s", midCodeTable[i].dst);   //bgez $t2, label
}

void generate_mips_code_24(int i)
{
    //先取src1的值,存在$t0里
    src1ToRegister("$t0", i);
    //再取src2的值,存在$t1里
    src2ToRegister("$t1", i);
    //由于if中的条件符号为!=
    //所以当t0!=t1时不跳转，t0==t1时跳转
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "beq");
    strcpy(mipsTextCodeTable[text_top].op1, "$t0");
    strcpy(mipsTextCodeTable[text_top].op2, "$t1");
    sprintf(mipsTextCodeTable[text_top].op3, "$label%s", midCodeTable[i].dst);   //beq $t0, $t1, label
}

void generate_mips_code_25(int i)
{
    //先取src1的值,存在$t0里
    src1ToRegister("$t0", i);
    //再取src2的值,存在$t1里
    src2ToRegister("$t1", i);
    //由于if中的条件符号为==
    //所以当t0==t1时不跳转，t0!=t1时跳转
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "bne");
    strcpy(mipsTextCodeTable[text_top].op1, "$t0");
    strcpy(mipsTextCodeTable[text_top].op2, "$t1");
    sprintf(mipsTextCodeTable[text_top].op3, "$label%s", midCodeTable[i].dst);   //beq $t0, $t1, label
}

void generate_mips_code_26(int i)
{
    int j = loc_mips(midCodeTable[i].dst);      //找到待读取内容在符号表中的位置
    //从控制台读取一个 量 并存在$v0里
    if(tab[j].type == INT_TAB_VALUE) {      //如果是整数
        text_top++;
        strcpy(mipsTextCodeTable[text_top].instr, "li");
        strcpy(mipsTextCodeTable[text_top].op1, "$v0");
        strcpy(mipsTextCodeTable[text_top].op2, "5");   //li $v0, 5         //5 for read integer
        text_top++;
        strcpy(mipsTextCodeTable[text_top].instr, "syscall");   //syscall
    }
    else if(tab[j].type == CHAR_TAB_VALUE) {        //如果是字符
        text_top++;
        strcpy(mipsTextCodeTable[text_top].instr, "li");
        strcpy(mipsTextCodeTable[text_top].op1, "$v0");
        strcpy(mipsTextCodeTable[text_top].op2, "12");   //li $v0, 12       //12 for read character
        text_top++;
        strcpy(mipsTextCodeTable[text_top].instr, "syscall");   //syscall
    }
    //把量从$v0 存入内存
    if(j > globalTop) {     //如果是局部变量
        text_top++;
        strcpy(mipsTextCodeTable[text_top].instr, "sw");
        strcpy(mipsTextCodeTable[text_top].op1, "$v0");
        sprintf(mipsTextCodeTable[text_top].op2, "-%d", tab[j].offset);
        strcpy(mipsTextCodeTable[text_top].op3, "$sp");         //sw $v0, -offset($sp)
    }
    else {      //如果是全局变量
        text_top++;
        strcpy(mipsTextCodeTable[text_top].instr, "sw");
        strcpy(mipsTextCodeTable[text_top].op1, "$v0");
        sprintf(mipsTextCodeTable[text_top].op2, "%d", tab[j].offset);
        strcpy(mipsTextCodeTable[text_top].op3, "$gp");                 //sw $v0, offset($gp)
    }
}

void generate_mips_code_27(int i)
{
    //为字符串创建空间
    stringNum++;
    data_top++;
    sprintf(mipsDataCodeTable[data_top].ident, "$string%d", stringNum);
    strcpy(mipsDataCodeTable[data_top].instr, ".asciiz");
    strcpy(mipsDataCodeTable[data_top].content, midCodeTable[i].src1);  //$stringNum: .asciiz "string"

    //输出字符串
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "la");
    strcpy(mipsTextCodeTable[text_top].op1, "$a0");
    sprintf(mipsTextCodeTable[text_top].op2, "$string%d", stringNum);    //la $a0, $stringNum
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "li");
    strcpy(mipsTextCodeTable[text_top].op1, "$v0");
    strcpy(mipsTextCodeTable[text_top].op2, "4");        //li $v0, 4    //4 for print string
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "syscall");       //syscall

    //取src2的值，存在$a0里
    if(midCodeTable[i].src2[0] == '$') {     //如果是临时变量
        if(midCodeTable[i].src2[1] == 'r') {          //如果是$ret
            //把$ret的值从$v0移至$a0
            text_top++;
            strcpy(mipsTextCodeTable[text_top].instr, "move");
            strcpy(mipsTextCodeTable[text_top].op1, "$a0");
            strcpy(mipsTextCodeTable[text_top].op2, "$v0");         //move $a0, $v0
            //如果printf后面是ret,那么向上查找最近的call语句
            int j;
            for(j = i; j > 0; j--) {
                if(midCodeTable[j].op == CALL) {
                    break;
                }
            }
            if(j == 0) {    //如果没找到call语句，那么一定出错了
                printf("ERROR: found no call statement in mid code\n");
                exit(0);
            }
            int k = loc_function(midCodeTable[i].src1);     //在符号表中查找这个函数
            if(tab[k].type == INT_TAB_VALUE) {      //如果这个函数返回的是int
                text_top++;
                strcpy(mipsTextCodeTable[text_top].instr, "li");
                strcpy(mipsTextCodeTable[text_top].op1, "$v0");
                strcpy(mipsTextCodeTable[text_top].op2, "1");       //li $v0, 1
                text_top++;
                strcpy(mipsTextCodeTable[text_top].instr, "syscall");   //syscall
            }
            else if(tab[k].type == CHAR_TAB_VALUE) {    //如果这个函数返回的是char
                text_top++;
                strcpy(mipsTextCodeTable[text_top].instr, "li");
                strcpy(mipsTextCodeTable[text_top].op1, "$v0");
                strcpy(mipsTextCodeTable[text_top].op2, "11");       //li $v0, 11
                text_top++;
                strcpy(mipsTextCodeTable[text_top].instr, "syscall");   //syscall
            }
        }
        else {
            int m = locTemp(midCodeTable[i].src2);      //在临时变量表里查找该临时变量
            text_top++;
            strcpy(mipsTextCodeTable[text_top].instr, "lw");
            strcpy(mipsTextCodeTable[text_top].op1, "$a0");
            sprintf(mipsTextCodeTable[text_top].op2, "-%d", offsetTempTab[m].offset);
            strcpy(mipsTextCodeTable[text_top].op3, "$sp");              //lw $a0, -offset($sp)  //把值存在$a0里

            //查找 midCodeTable[i].src2 的类型
            int j;
            int l = (int) strlen(midCodeTable[i].src2);
            char numString[20];
            for (j = 1; j <= l; j++) {
                numString[j - 1] = midCodeTable[i].src2[j];  //把midCodeTable[i].src2中$后面的部分（即数字部分）复制到numString中
            }
            int k = stringToNum(numString);     //计算midCodeTable[i].src2在tempTab表中的编号
            if (tempTab[k] == INT_TAB_VALUE) {   //如果是int
                text_top++;
                strcpy(mipsTextCodeTable[text_top].instr, "li");
                strcpy(mipsTextCodeTable[text_top].op1, "$v0");
                strcpy(mipsTextCodeTable[text_top].op2, "1");       //li $v0, 1
                text_top++;
                strcpy(mipsTextCodeTable[text_top].instr, "syscall");   //syscall
            } else if (tempTab[k] == CHAR_TAB_VALUE) { //如果是char
                text_top++;
                strcpy(mipsTextCodeTable[text_top].instr, "li");
                strcpy(mipsTextCodeTable[text_top].op1, "$v0");
                strcpy(mipsTextCodeTable[text_top].op2, "11");       //li $v0, 11
                text_top++;
                strcpy(mipsTextCodeTable[text_top].instr, "syscall");   //syscall
            }
        }
    }
    else if(is_letter(midCodeTable[i].src2[0])) {   //如果是声明过的变量/常量
        int j = loc_mips(midCodeTable[i].src2);  //在符号表中查找
        if(tab[j].object == CONST_TAB_VALUE) {  //如果是常量
            text_top++;
            strcpy(mipsTextCodeTable[text_top].instr, "li");
            strcpy(mipsTextCodeTable[text_top].op1, "$a0");
            sprintf(mipsTextCodeTable[text_top].op2, "%d", tab[j].offset);      //li $a0, value
        }
        else if(tab[j].object == VAR_TAB_VALUE || tab[j].object == PARA_TAB_VALUE) {   //如果是变量
            if(j > globalTop) {                         //如果是局部变量
                text_top++;
                strcpy(mipsTextCodeTable[text_top].instr, "lw");
                strcpy(mipsTextCodeTable[text_top].op1, "$a0");
                sprintf(mipsTextCodeTable[text_top].op2, "-%d", tab[j].offset);
                strcpy(mipsTextCodeTable[text_top].op3, "$sp");     //lw $a0, -offset($sp)
            }
            else {                                      //如果是全局变量
                text_top++;
                strcpy(mipsTextCodeTable[text_top].instr, "lw");
                strcpy(mipsTextCodeTable[text_top].op1, "$a0");
                sprintf(mipsTextCodeTable[text_top].op2, "%d", tab[j].offset);
                strcpy(mipsTextCodeTable[text_top].op3, "$gp");                 //lw $a0, offset($gp)
            }
        }
        //根据类型输出
        if(tab[j].type == INT_TAB_VALUE) {//如果是int
            text_top++;
            strcpy(mipsTextCodeTable[text_top].instr, "li");
            strcpy(mipsTextCodeTable[text_top].op1, "$v0");
            strcpy(mipsTextCodeTable[text_top].op2, "1");       //li $v0, 1
            text_top++;
            strcpy(mipsTextCodeTable[text_top].instr, "syscall");   //syscall
        }
        else if(tab[j].type == CHAR_TAB_VALUE) { //如果是char
            text_top++;
            strcpy(mipsTextCodeTable[text_top].instr, "li");
            strcpy(mipsTextCodeTable[text_top].op1, "$v0");
            strcpy(mipsTextCodeTable[text_top].op2, "11");       //li $v0, 11
            text_top++;
            strcpy(mipsTextCodeTable[text_top].instr, "syscall");   //syscall
        }
    }
    else if(is_digit(midCodeTable[i].src2[0])) {        //如果要返回的是整数
        text_top++;
        strcpy(mipsTextCodeTable[text_top].instr, "li");
        strcpy(mipsTextCodeTable[text_top].op1, "$a0");
        strcpy(mipsTextCodeTable[text_top].op2, midCodeTable[i].src2);      //li $a0, integer
        text_top++;
        strcpy(mipsTextCodeTable[text_top].instr, "li");
        strcpy(mipsTextCodeTable[text_top].op1, "$v0");
        strcpy(mipsTextCodeTable[text_top].op2, "1");       //li $v0, 1
        text_top++;
        strcpy(mipsTextCodeTable[text_top].instr, "syscall");   //syscall
    }
    else if(midCodeTable[i].src2[0] == '\'') {          //如果要返回的是字符
        text_top++;
        strcpy(mipsTextCodeTable[text_top].instr, "li");
        strcpy(mipsTextCodeTable[text_top].op1, "$a0");
        strcpy(mipsTextCodeTable[text_top].op2, midCodeTable[i].src2);      //li $a0, character
        text_top++;
        strcpy(mipsTextCodeTable[text_top].instr, "li");
        strcpy(mipsTextCodeTable[text_top].op1, "$v0");
        strcpy(mipsTextCodeTable[text_top].op2, "11");       //li $v0, 11
        text_top++;
        strcpy(mipsTextCodeTable[text_top].instr, "syscall");   //syscall
    }

    //输出换行符
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "la");
    strcpy(mipsTextCodeTable[text_top].op1, "$a0");
    sprintf(mipsTextCodeTable[text_top].op2, "$newLine");    //la $a0, $newLine
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "li");
    strcpy(mipsTextCodeTable[text_top].op1, "$v0");
    strcpy(mipsTextCodeTable[text_top].op2, "4");        //li $v0, 4    //4 for print string
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "syscall");       //syscall
}

void generate_mips_code_28(int i)
{
    //为字符串创建空间
    stringNum++;
    data_top++;
    sprintf(mipsDataCodeTable[data_top].ident, "$string%d", stringNum);
    strcpy(mipsDataCodeTable[data_top].instr, ".asciiz");
    strcpy(mipsDataCodeTable[data_top].content, midCodeTable[i].src1);  //$stringNum: .asciiz "string"

    //输出字符串
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "la");
    strcpy(mipsTextCodeTable[text_top].op1, "$a0");
    sprintf(mipsTextCodeTable[text_top].op2, "$string%d", stringNum);    //la $a0, $stringNum
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "li");
    strcpy(mipsTextCodeTable[text_top].op1, "$v0");
    strcpy(mipsTextCodeTable[text_top].op2, "4");        //li $v0, 4    //4 for print string
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "syscall");       //syscall

    //输出换行符
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "la");
    strcpy(mipsTextCodeTable[text_top].op1, "$a0");
    sprintf(mipsTextCodeTable[text_top].op2, "$newLine");    //la $a0, $newLine
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "li");
    strcpy(mipsTextCodeTable[text_top].op1, "$v0");
    strcpy(mipsTextCodeTable[text_top].op2, "4");        //li $v0, 4    //4 for print string
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "syscall");       //syscall
}

void generate_mips_code_29(int i)
{
    //取src2的值，存在$a0里
    if(midCodeTable[i].src2[0] == '$') {     //如果是临时变量
        if(midCodeTable[i].src2[1] == 'r') {          //如果是$ret
            //把$ret的值从$v0移至$a0
            text_top++;
            strcpy(mipsTextCodeTable[text_top].instr, "move");
            strcpy(mipsTextCodeTable[text_top].op1, "$a0");
            strcpy(mipsTextCodeTable[text_top].op2, "$v0");         //move $a0, $v0
            //如果printf后面是ret,那么向上查找最近的call语句
            int j;
            for(j = i; j > 0; j--) {
                if(midCodeTable[j].op == CALL) {
                    break;
                }
            }
            if(j == 0) {    //如果没找到call语句，那么一定出错了
                printf("ERROR: found no call statement in mid code\n");
                exit(0);
            }
            int k = loc_function(midCodeTable[i].src1);     //在符号表中查找这个函数
            if(tab[k].type == INT_TAB_VALUE) {      //如果这个函数返回的是int
                text_top++;
                strcpy(mipsTextCodeTable[text_top].instr, "li");
                strcpy(mipsTextCodeTable[text_top].op1, "$v0");
                strcpy(mipsTextCodeTable[text_top].op2, "1");       //li $v0, 1
                text_top++;
                strcpy(mipsTextCodeTable[text_top].instr, "syscall");   //syscall
            }
            else if(tab[k].type == CHAR_TAB_VALUE) {    //如果这个函数返回的是char
                text_top++;
                strcpy(mipsTextCodeTable[text_top].instr, "li");
                strcpy(mipsTextCodeTable[text_top].op1, "$v0");
                strcpy(mipsTextCodeTable[text_top].op2, "11");       //li $v0, 11
                text_top++;
                strcpy(mipsTextCodeTable[text_top].instr, "syscall");   //syscall
            }
        }
        else {
            int m = locTemp(midCodeTable[i].src2);      //在临时变量表里查找该临时变量
            text_top++;
            strcpy(mipsTextCodeTable[text_top].instr, "lw");
            strcpy(mipsTextCodeTable[text_top].op1, "$a0");
            sprintf(mipsTextCodeTable[text_top].op2, "-%d", offsetTempTab[m].offset);
            strcpy(mipsTextCodeTable[text_top].op3, "$sp");              //lw reg, -offset($sp)  //把只存在reg里

            //查找 midCodeTable[i].src2 的类型
            int j;
            int l = (int) strlen(midCodeTable[i].src2);
            char numString[20];
            for (j = 1; j <= l; j++) {
                numString[j - 1] = midCodeTable[i].src2[j];  //把midCodeTable[i].src2中$后面的部分（即数字部分）复制到numString中
            }
            int k = stringToNum(numString);     //计算midCodeTable[i].src2在tempTab表中的编号
            if (tempTab[k] == INT_TAB_VALUE) {   //如果是int
                text_top++;
                strcpy(mipsTextCodeTable[text_top].instr, "li");
                strcpy(mipsTextCodeTable[text_top].op1, "$v0");
                strcpy(mipsTextCodeTable[text_top].op2, "1");       //li $v0, 1
                text_top++;
                strcpy(mipsTextCodeTable[text_top].instr, "syscall");   //syscall
            } else if (tempTab[k] == CHAR_TAB_VALUE) { //如果是char
                text_top++;
                strcpy(mipsTextCodeTable[text_top].instr, "li");
                strcpy(mipsTextCodeTable[text_top].op1, "$v0");
                strcpy(mipsTextCodeTable[text_top].op2, "11");       //li $v0, 11
                text_top++;
                strcpy(mipsTextCodeTable[text_top].instr, "syscall");   //syscall
            }
        }
    }
    else if(is_letter(midCodeTable[i].src2[0])) {   //如果是声明过的变量/常量
        int j = loc_mips(midCodeTable[i].src2);  //在符号表中查找
        if(tab[j].object == CONST_TAB_VALUE) {  //如果是常量
            text_top++;
            strcpy(mipsTextCodeTable[text_top].instr, "li");
            strcpy(mipsTextCodeTable[text_top].op1, "$a0");
            sprintf(mipsTextCodeTable[text_top].op2, "%d", tab[j].offset);      //li $a0, value
        }
        else if(tab[j].object == VAR_TAB_VALUE || tab[j].object == PARA_TAB_VALUE) {   //如果是变量
            if(j > globalTop) {                         //如果是局部变量
                text_top++;
                strcpy(mipsTextCodeTable[text_top].instr, "lw");
                strcpy(mipsTextCodeTable[text_top].op1, "$a0");
                sprintf(mipsTextCodeTable[text_top].op2, "-%d", tab[j].offset);
                strcpy(mipsTextCodeTable[text_top].op3, "$sp");     //lw $a0, -offset($sp)
            }
            else {                                      //如果是全局变量
                text_top++;
                strcpy(mipsTextCodeTable[text_top].instr, "lw");
                strcpy(mipsTextCodeTable[text_top].op1, "$a0");
                sprintf(mipsTextCodeTable[text_top].op2, "%d", tab[j].offset);
                strcpy(mipsTextCodeTable[text_top].op3, "$gp");                 //lw $a0, offset($gp)
            }
        }
        //根据类型输出
        if(tab[j].type == INT_TAB_VALUE) {//如果是int
            text_top++;
            strcpy(mipsTextCodeTable[text_top].instr, "li");
            strcpy(mipsTextCodeTable[text_top].op1, "$v0");
            strcpy(mipsTextCodeTable[text_top].op2, "1");       //li $v0, 1
            text_top++;
            strcpy(mipsTextCodeTable[text_top].instr, "syscall");   //syscall
        }
        else if(tab[j].type == CHAR_TAB_VALUE) { //如果是char
            text_top++;
            strcpy(mipsTextCodeTable[text_top].instr, "li");
            strcpy(mipsTextCodeTable[text_top].op1, "$v0");
            strcpy(mipsTextCodeTable[text_top].op2, "11");       //li $v0, 11
            text_top++;
            strcpy(mipsTextCodeTable[text_top].instr, "syscall");   //syscall
        }
    }
    else if(is_digit(midCodeTable[i].src2[0])) {        //如果要返回的是整数
        text_top++;
        strcpy(mipsTextCodeTable[text_top].instr, "li");
        strcpy(mipsTextCodeTable[text_top].op1, "$a0");
        strcpy(mipsTextCodeTable[text_top].op2, midCodeTable[i].src2);      //li $a0, integer
        text_top++;
        strcpy(mipsTextCodeTable[text_top].instr, "li");
        strcpy(mipsTextCodeTable[text_top].op1, "$v0");
        strcpy(mipsTextCodeTable[text_top].op2, "1");       //li $v0, 1
        text_top++;
        strcpy(mipsTextCodeTable[text_top].instr, "syscall");   //syscall
    }
    else if(midCodeTable[i].src2[0] == '\'') {          //如果要返回的是字符
        text_top++;
        strcpy(mipsTextCodeTable[text_top].instr, "li");
        strcpy(mipsTextCodeTable[text_top].op1, "$a0");
        strcpy(mipsTextCodeTable[text_top].op2, midCodeTable[i].src2);      //li $a0, character
        text_top++;
        strcpy(mipsTextCodeTable[text_top].instr, "li");
        strcpy(mipsTextCodeTable[text_top].op1, "$v0");
        strcpy(mipsTextCodeTable[text_top].op2, "11");       //li $v0, 11
        text_top++;
        strcpy(mipsTextCodeTable[text_top].instr, "syscall");   //syscall
    }

    //输出换行符
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "la");
    strcpy(mipsTextCodeTable[text_top].op1, "$a0");
    sprintf(mipsTextCodeTable[text_top].op2, "$newLine");    //la $a0, $newLine
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "li");
    strcpy(mipsTextCodeTable[text_top].op1, "$v0");
    strcpy(mipsTextCodeTable[text_top].op2, "4");        //li $v0, 4    //4 for print string
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "syscall");       //syscall
}

void generate_mips_code_30(int i)
{
    //先取src1的值,存在$t0里
    src1ToRegister("$t0", i);
    //再找到dst
    registerToDst("$t0", i);
}

void generate_mips_code_31(int i)
{
    //先取src1的值,存在$t0里
    src1ToRegister("$t0", i);
    //再取src2的值,存在$t1里
    src2ToRegister("$t1", i);
    //再找到dst
    int j = loc_mips(midCodeTable[i].dst);  //在符号表中查找
    if(j > globalTop) {                         //如果是局部变量
        //数组名offset+数组下标src1*4求得地址
        text_top++;
        strcpy(mipsTextCodeTable[text_top].instr, "sll");
        strcpy(mipsTextCodeTable[text_top].op1, "$t2");
        strcpy(mipsTextCodeTable[text_top].op2, "$t0");
        strcpy(mipsTextCodeTable[text_top].op3, "2");       //sll $t2, $t0, 2  //$t0 << 2 ($t2 = $t0 * 4)
        text_top++;
        strcpy(mipsTextCodeTable[text_top].instr, "addiu");
        strcpy(mipsTextCodeTable[text_top].op1, "$t3");
        strcpy(mipsTextCodeTable[text_top].op2, "$t2");
        sprintf(mipsTextCodeTable[text_top].op3, "%d", tab[j].offset);  //addi $t3, $t2, offset  ($t3 = src1 * 4 + offset)
        text_top++;
        strcpy(mipsTextCodeTable[text_top].instr, "subu");
        strcpy(mipsTextCodeTable[text_top].op1, "$t4");
        strcpy(mipsTextCodeTable[text_top].op2, "$sp");
        strcpy(mipsTextCodeTable[text_top].op3, "$t3");     //sub $t4, $sp, $t3  (内存地址)
        text_top++;
        strcpy(mipsTextCodeTable[text_top].instr, "sw");
        strcpy(mipsTextCodeTable[text_top].op1, "$t1");
        sprintf(mipsTextCodeTable[text_top].op2, "%d", 0);
        strcpy(mipsTextCodeTable[text_top].op3, "$t4");     //sw $t1, 0($t4)  //$t4 is address, $t1 is the value going to assign
    }
    else {                                      //如果是全局变量
        text_top++;
        strcpy(mipsTextCodeTable[text_top].instr, "sll");
        strcpy(mipsTextCodeTable[text_top].op1, "$t2");
        strcpy(mipsTextCodeTable[text_top].op2, "$t0");
        strcpy(mipsTextCodeTable[text_top].op3, "2");       //sll $t2, $t0, 2  //$t0 << 2 ($t2 = $t0 * 4)//下标*4,偏移量
        text_top++;
        strcpy(mipsTextCodeTable[text_top].instr, "addiu");
        strcpy(mipsTextCodeTable[text_top].op1, "$t3");
        strcpy(mipsTextCodeTable[text_top].op2, "$gp");
        sprintf(mipsTextCodeTable[text_top].op3, "%d", tab[j].offset);      //addi $t3, $gp, offset 数组基址存入$t3
        text_top++;
        strcpy(mipsTextCodeTable[text_top].instr, "addu");
        strcpy(mipsTextCodeTable[text_top].op1, "$t4");
        strcpy(mipsTextCodeTable[text_top].op2, "$t3");
        strcpy(mipsTextCodeTable[text_top].op3, "$t2");     //add $t4, $t3, $t2 //基地址加上偏移量，存入$t4(内存地址)
        text_top++;
        strcpy(mipsTextCodeTable[text_top].instr, "sw");
        strcpy(mipsTextCodeTable[text_top].op1, "$t1");
        strcpy(mipsTextCodeTable[text_top].op2, "0");
        strcpy(mipsTextCodeTable[text_top].op3, "$t4");                 //sw $t1, 0($t4)
    }
}

void generate_mips_code_32(int i)
{
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "jal");
    strcpy(mipsTextCodeTable[text_top].op1, midCodeTable[i].src1);
}

void generate_mips_code_33(int i)
{
    //先取src1的值,存在$t0里
    src1ToRegister("$t0", i);
    //再取src2的值,存在$t1里
    src2ToRegister("$t1", i);
    //由于if中的条件符号为==
    //所以当t0==t1时不跳转，t0!=t1时跳转
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "bne");
    strcpy(mipsTextCodeTable[text_top].op1, "$t0");
    strcpy(mipsTextCodeTable[text_top].op2, "$t1");
    sprintf(mipsTextCodeTable[text_top].op3, "$label%s", midCodeTable[i].dst);   //beq $t0, $t1, label
}

void generate_mips_code_34(int i)
{

}

void generate_mips_code_35(int i)
{
    //先取src1的值,存在$t0里
    src1ToRegister("$t0", i);

    //在中间代码中向上找直到找到不是PUSH的指令，以便直到这是第几个push，以对应到函数的第几个参数
    int j;
    for(j = i; j>0; j--) {
        if(midCodeTable[j].op != 35)
            break;          //向上找直到操作符不是push
    }
    int number = i-j;       //求得这是函数调用中的第number个push
    //由于此时是函数调用，则$fp指向call后的函数栈底，则该参数应存储的位置为$fp+number*4+4  ($fp+8为第一个参数)
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "sw");
    strcpy(mipsTextCodeTable[text_top].op1, "$t0");
    sprintf(mipsTextCodeTable[text_top].op2, "-%d", number*4+4);
    strcpy(mipsTextCodeTable[text_top].op3, "$fp");     //sw $t0, -(number*4+4)($fp)
}

void generate_mips_code_36(int i)
{
    //先取src1的值,存在$t0里
    src1ToRegister("$t0", i);
    //再取src2的值,存在$t1里
    src2ToRegister("$t1", i);
    //然后用$t0+$t1，存在$t2里     //这个是最终要存回内存的结果
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "addu");
    strcpy(mipsTextCodeTable[text_top].op1, "$t2");
    strcpy(mipsTextCodeTable[text_top].op2, "$t0");
    strcpy(mipsTextCodeTable[text_top].op3, "$t1");         //add $t2, $t0, $t1
    //然后存回dst
    registerToDst("$t2", i);
}

void generate_mips_code_37(int i)
{
    //先取src1的值,存在$t0里
    src1ToRegister("$t0", i);
    //再取src2的值,存在$t1里
    src2ToRegister("$t1", i);
    //然后用$t0-$t1，存在$t2里     //这个是最终要存回内存的结果
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "subu");
    strcpy(mipsTextCodeTable[text_top].op1, "$t2");
    strcpy(mipsTextCodeTable[text_top].op2, "$t0");
    strcpy(mipsTextCodeTable[text_top].op3, "$t1");         //sub $t2, $t0, $t1
    //然后存回dst
    registerToDst("$t2", i);
}

void generate_mips_code_38(int i)
{
    //先取src1的值,存在$t0里
    src1ToRegister("$t0", i);
    //再取src2的值,存在$t1里
    src2ToRegister("$t1", i);
    //然后用$t0*$t1，存在$t2里     //这个是最终要存回内存的结果
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "mult");
    strcpy(mipsTextCodeTable[text_top].op1, "$t0");
    strcpy(mipsTextCodeTable[text_top].op2, "$t1");         //mult $t0, $t1
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "mflo");
    strcpy(mipsTextCodeTable[text_top].op1, "$t2");         //mflo $t2
    //然后存回dst
    registerToDst("$t2", i);
}

void generate_mips_code_39(int i)
{
    //先取src1的值,存在$t0里
    src1ToRegister("$t0", i);
    //再取src2的值,存在$t1里
    src2ToRegister("$t1", i);
    //然后用$t0 / $t1，存在$t2里     //这个是最终要存回内存的结果
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "div");
    strcpy(mipsTextCodeTable[text_top].op1, "$t0");
    strcpy(mipsTextCodeTable[text_top].op2, "$t1");         //div $t0, $t1
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "mflo");
    strcpy(mipsTextCodeTable[text_top].op1, "$t2");         //mflo $t2
    //然后存回dst
    registerToDst("$t2", i);
}

void generate_mips_code_40(int i)
{
    //先取src2的值,存在$t1里
    src2ToRegister("$t1", i);
    //已经把数组下标的值存在$t1里
    int j = loc_mips(midCodeTable[i].src1);     //查找数组名
    if(j > globalTop) {     //如果是局部变量数组
        text_top++;
        strcpy(mipsTextCodeTable[text_top].instr, "sll");
        strcpy(mipsTextCodeTable[text_top].op1, "$t1");
        strcpy(mipsTextCodeTable[text_top].op2, "$t1");
        strcpy(mipsTextCodeTable[text_top].op3, "2");     //sll $t1, $t1, 2   //t1 = t1 * 4  (下标偏移量)
        text_top++;
        strcpy(mipsTextCodeTable[text_top].instr, "addiu");
        strcpy(mipsTextCodeTable[text_top].op1, "$t0");
        strcpy(mipsTextCodeTable[text_top].op2, "$t1");
        sprintf(mipsTextCodeTable[text_top].op3, "%d", tab[j].offset);  //addi $t0, $t1, offset  (求出相对于sp的偏移量)
        text_top++;
        strcpy(mipsTextCodeTable[text_top].instr, "subu");
        strcpy(mipsTextCodeTable[text_top].op1, "$t4");
        strcpy(mipsTextCodeTable[text_top].op2, "$sp");
        strcpy(mipsTextCodeTable[text_top].op3, "$t0");         //sub $t4, $sp, $t0  求出数组元素内存地址
        text_top++;
        strcpy(mipsTextCodeTable[text_top].instr, "lw");
        strcpy(mipsTextCodeTable[text_top].op1, "$t2");
        sprintf(mipsTextCodeTable[text_top].op2, "0");
        strcpy(mipsTextCodeTable[text_top].op3, "$t4");     //lw $t2, 0($t4)  把数组元素的值存入$t2
    }
    else {                  //如果是全局变量数组
        text_top++;
        strcpy(mipsTextCodeTable[text_top].instr, "sll");
        strcpy(mipsTextCodeTable[text_top].op1, "$t1");
        strcpy(mipsTextCodeTable[text_top].op2, "$t1");
        strcpy(mipsTextCodeTable[text_top].op3, "2");     //sll $t1, $t1, 2   //t1 = t1 * 4  (下标偏移量)
        text_top++;
        strcpy(mipsTextCodeTable[text_top].instr, "addiu");
        strcpy(mipsTextCodeTable[text_top].op1, "$t0");
        strcpy(mipsTextCodeTable[text_top].op2, "$gp");
        sprintf(mipsTextCodeTable[text_top].op3, "%d", tab[j].offset);      //addi $t0, $gp, offset
        text_top++;
        strcpy(mipsTextCodeTable[text_top].instr, "addu");
        strcpy(mipsTextCodeTable[text_top].op1, "$t4");
        strcpy(mipsTextCodeTable[text_top].op2, "$t0");
        strcpy(mipsTextCodeTable[text_top].op3, "$t1");     //add $t4, $t0, $t1  数组元素内存地址
        text_top++;
        strcpy(mipsTextCodeTable[text_top].instr, "lw");
        strcpy(mipsTextCodeTable[text_top].op1, "$t2");
        sprintf(mipsTextCodeTable[text_top].op2, "0");
        strcpy(mipsTextCodeTable[text_top].op3, "$t4");     //lw $t2, 0($t4)  把数组元素的值存入$t2
    }
    //已经把要存的值存在$t2里
    //然后存回dst
    registerToDst("$t2", i);
}

void generate_mips_code_41(int i)
{
    //对sp和fp进行调整，以便返回
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "lw");
    strcpy(mipsTextCodeTable[text_top].op1, "$ra");
    strcpy(mipsTextCodeTable[text_top].op2, "0");
    strcpy(mipsTextCodeTable[text_top].op3, "$sp");     //lw $ra, 0($sp)
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "move");
    strcpy(mipsTextCodeTable[text_top].op1, "$fp");
    strcpy(mipsTextCodeTable[text_top].op2, "$sp");     //move, $fp, $sp
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "lw");
    strcpy(mipsTextCodeTable[text_top].op1, "$sp");
    strcpy(mipsTextCodeTable[text_top].op2, "-4");
    strcpy(mipsTextCodeTable[text_top].op3, "$sp");     //lw $sp, -4($sp)
    text_top++;
    strcpy(mipsTextCodeTable[text_top].instr, "jr");
    strcpy(mipsTextCodeTable[text_top].op1, "$ra");     //jr $ra
}


void src1ToRegister(char reg[], int i)
{
    if(midCodeTable[i].src1[0] == '$') {     //如果是临时变量
        if(midCodeTable[i].src1[1] == 'r') {    //如果是$ret
            text_top++;
            strcpy(mipsTextCodeTable[text_top].instr, "move");
            strcpy(mipsTextCodeTable[text_top].op1, reg);
            strcpy(mipsTextCodeTable[text_top].op2, "$v0");    //move reg, $v0
        }
        else {                                  //如果不是$ret
            int j = locTemp(midCodeTable[i].src1);      //在临时变量表里查找该临时变量
            text_top++;
            strcpy(mipsTextCodeTable[text_top].instr, "lw");
            strcpy(mipsTextCodeTable[text_top].op1, reg);
            sprintf(mipsTextCodeTable[text_top].op2, "-%d", offsetTempTab[j].offset);
            strcpy(mipsTextCodeTable[text_top].op3, "$sp");              //lw reg, -offset($sp)  //把只存在reg里
        }
    }
    else if(is_letter(midCodeTable[i].src1[0])) {   //如果是声明过的变量/常量
        int j = loc_mips(midCodeTable[i].src1);  //在符号表中查找
        if(tab[j].object == CONST_TAB_VALUE) {  //如果是常量
            text_top++;
            strcpy(mipsTextCodeTable[text_top].instr, "li");
            strcpy(mipsTextCodeTable[text_top].op1, reg);
            sprintf(mipsTextCodeTable[text_top].op2, "%d", tab[j].offset);      //li reg, value
        }
        else if(tab[j].object == VAR_TAB_VALUE || tab[j].object == PARA_TAB_VALUE) {   //如果是变量
            if(j > globalTop) {                         //如果是局部变量
                text_top++;
                strcpy(mipsTextCodeTable[text_top].instr, "lw");
                strcpy(mipsTextCodeTable[text_top].op1, reg);
                sprintf(mipsTextCodeTable[text_top].op2, "-%d", tab[j].offset);
                strcpy(mipsTextCodeTable[text_top].op3, "$sp");     //lw reg, -offset($sp)
            }
            else {                                      //如果是全局变量
                text_top++;
                strcpy(mipsTextCodeTable[text_top].instr, "lw");
                strcpy(mipsTextCodeTable[text_top].op1, reg);
                sprintf(mipsTextCodeTable[text_top].op2, "%d", tab[j].offset);
                strcpy(mipsTextCodeTable[text_top].op3, "$gp");                 //lw reg, offset($gp)
            }
        }
    }
    else if(is_digit(midCodeTable[i].src1[0])) {        //如果要返回的是整数
        text_top++;
        strcpy(mipsTextCodeTable[text_top].instr, "li");
        strcpy(mipsTextCodeTable[text_top].op1, reg);
        strcpy(mipsTextCodeTable[text_top].op2, midCodeTable[i].src1);      //li reg, integer
    }
    else if(midCodeTable[i].src1[0] == '\'') {          //如果要返回的是字符
        text_top++;
        strcpy(mipsTextCodeTable[text_top].instr, "li");
        strcpy(mipsTextCodeTable[text_top].op1, reg);
        strcpy(mipsTextCodeTable[text_top].op2, midCodeTable[i].src1);      //li reg, character
    }
}

void src2ToRegister(char reg[], int i)
{
    if(midCodeTable[i].src2[0] == '$') {     //如果是临时变量
        if(midCodeTable[i].src2[1] == 'r') {    //如果是$ret
            text_top++;
            strcpy(mipsTextCodeTable[text_top].instr, "move");
            strcpy(mipsTextCodeTable[text_top].op1, reg);
            strcpy(mipsTextCodeTable[text_top].op2, "$v0");    //move reg, $v0
        }
        else {
            int j = locTemp(midCodeTable[i].src2);      //在临时变量表里查找该临时变量
            text_top++;
            strcpy(mipsTextCodeTable[text_top].instr, "lw");
            strcpy(mipsTextCodeTable[text_top].op1, reg);
            sprintf(mipsTextCodeTable[text_top].op2, "-%d", offsetTempTab[j].offset);
            strcpy(mipsTextCodeTable[text_top].op3, "$sp");              //lw reg, -offset($sp)  //把只存在reg里
        }
    }
    else if(is_letter(midCodeTable[i].src2[0])) {   //如果是声明过的变量/常量
        int j = loc_mips(midCodeTable[i].src2);  //在符号表中查找
        if(tab[j].object == CONST_TAB_VALUE) {  //如果是常量
            text_top++;
            strcpy(mipsTextCodeTable[text_top].instr, "li");
            strcpy(mipsTextCodeTable[text_top].op1, reg);
            sprintf(mipsTextCodeTable[text_top].op2, "%d", tab[j].offset);      //li reg, value
        }
        else if(tab[j].object == VAR_TAB_VALUE || tab[j].object == PARA_TAB_VALUE) {   //如果是变量
            if(j > globalTop) {                         //如果是局部变量
                text_top++;
                strcpy(mipsTextCodeTable[text_top].instr, "lw");
                strcpy(mipsTextCodeTable[text_top].op1, reg);
                sprintf(mipsTextCodeTable[text_top].op2, "-%d", tab[j].offset);
                strcpy(mipsTextCodeTable[text_top].op3, "$sp");     //lw reg, -offset($sp)
            }
            else {                                      //如果是全局变量
                text_top++;
                strcpy(mipsTextCodeTable[text_top].instr, "lw");
                strcpy(mipsTextCodeTable[text_top].op1, reg);
                sprintf(mipsTextCodeTable[text_top].op2, "%d", tab[j].offset);
                strcpy(mipsTextCodeTable[text_top].op3, "$gp");                 //lw reg, offset($gp)
            }
        }
    }
    else if(is_digit(midCodeTable[i].src2[0])) {        //如果要返回的是整数
        text_top++;
        strcpy(mipsTextCodeTable[text_top].instr, "li");
        strcpy(mipsTextCodeTable[text_top].op1, reg);
        strcpy(mipsTextCodeTable[text_top].op2, midCodeTable[i].src2);      //li reg, integer
    }
    else if(midCodeTable[i].src2[0] == '\'') {          //如果要返回的是字符
        text_top++;
        strcpy(mipsTextCodeTable[text_top].instr, "li");
        strcpy(mipsTextCodeTable[text_top].op1, reg);
        strcpy(mipsTextCodeTable[text_top].op2, midCodeTable[i].src2);      //li reg, character
    }
}

void registerToDst(char reg[], int i)
{
    if(midCodeTable[i].dst[0] == '$') {     //如果是临时变量
        int j = locTemp(midCodeTable[i].dst);      //在临时变量表里查找该临时变量
        text_top++;
        strcpy(mipsTextCodeTable[text_top].instr, "sw");
        strcpy(mipsTextCodeTable[text_top].op1, reg);
        sprintf(mipsTextCodeTable[text_top].op2, "-%d", offsetTempTab[j].offset);
        strcpy(mipsTextCodeTable[text_top].op3, "$sp");              //sw reg, -offset($sp)  //把只存在reg里
    }
    else if(is_letter(midCodeTable[i].dst[0])) {   //如果是声明过的变量/常量
        int j = loc_mips(midCodeTable[i].dst);  //在符号表中查找
        if(j > globalTop) {                         //如果是局部变量
            text_top++;
            strcpy(mipsTextCodeTable[text_top].instr, "sw");
            strcpy(mipsTextCodeTable[text_top].op1, reg);
            sprintf(mipsTextCodeTable[text_top].op2, "-%d", tab[j].offset);
            strcpy(mipsTextCodeTable[text_top].op3, "$sp");     //sw reg, -offset($sp)
        }
        else {                                      //如果是全局变量
            text_top++;
            strcpy(mipsTextCodeTable[text_top].instr, "sw");
            strcpy(mipsTextCodeTable[text_top].op1, reg);
            sprintf(mipsTextCodeTable[text_top].op2, "%d", tab[j].offset);
            strcpy(mipsTextCodeTable[text_top].op3, "$gp");                 //sw reg, offset($gp)
        }
    }
}






int loc_mips(char name[])
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
    if(strcmp(tempFunction, "") == 0) {     //如果在全局变量区
        seek_top = globalTop;
    }
    else {      //1.在符号表中查找到当前函数的位置
        int i;
        for (i = tab_top; i > 0; i--) {
            if(strcmp(tab[i].name, tempFunction) == 0 && tab[i].object == FUNC_TAB_VALUE) {
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

int loc_function(char name[])
{
    int i;
    for(i = 1; i<=tab_top; i++) {
        if(strcmp(tab[i].name, name) == 0 && tab[i].object == FUNC_TAB_VALUE) {
            return i;
        }
    }
    return 0;
}

void print_mips_code_table()
{
    int i;
    for(i = 0; i<=data_top; i++) {
        printf("%s: %s %s\n", mipsDataCodeTable[i].ident, mipsDataCodeTable[i].instr, mipsDataCodeTable[i].content);
    }
    for(i = 0; i<=text_top; i++) {
        printf("%s %s %s %s\n", mipsTextCodeTable[i].instr, mipsTextCodeTable[i].op1, mipsTextCodeTable[i].op2, mipsTextCodeTable[i].op3);
    }
}

void print_mips_code(int o)
{
    FILE * file;
    if(o == 0) {
        file = fopen("mips.asm", "w");
    }
    else {
        file = fopen("mips_opt.asm", "w");
    }
    printMipsDataCode(file);
    printMipsTextCode(file);
    fclose(file);
}

void printMipsDataCode(FILE * file)
{
    fprintf(file, ".data\n");
    int i;
    for(i = 1; i<=data_top; i++) {
        fprintf(file, "\t%s: %s %s\n", mipsDataCodeTable[i].ident, mipsDataCodeTable[i].instr, mipsDataCodeTable[i].content);
    }
}

void printMipsTextCode(FILE * file)
{
    fprintf(file, ".text\n");
    int i;
    for(i = 1; i<=text_top; i++) {
        if(strcmp(mipsTextCodeTable[i].instr, "add") == 0) {
            fprintf(file, "\t%s %s, %s, %s\n", mipsTextCodeTable[i].instr, mipsTextCodeTable[i].op1, mipsTextCodeTable[i].op2, mipsTextCodeTable[i].op3);
        }
        else if(strcmp(mipsTextCodeTable[i].instr, "addi") == 0) {
            fprintf(file, "\t%s %s, %s, %s\n", mipsTextCodeTable[i].instr, mipsTextCodeTable[i].op1, mipsTextCodeTable[i].op2, mipsTextCodeTable[i].op3);
        }
        else if(strcmp(mipsTextCodeTable[i].instr, "addiu") == 0) {
            fprintf(file, "\t%s %s, %s, %s\n", mipsTextCodeTable[i].instr, mipsTextCodeTable[i].op1, mipsTextCodeTable[i].op2, mipsTextCodeTable[i].op3);
        }
        else if(strcmp(mipsTextCodeTable[i].instr, "addu") == 0) {
            fprintf(file, "\t%s %s, %s, %s\n", mipsTextCodeTable[i].instr, mipsTextCodeTable[i].op1, mipsTextCodeTable[i].op2, mipsTextCodeTable[i].op3);
        }
        else if(strcmp(mipsTextCodeTable[i].instr, "beq") == 0) {
            fprintf(file, "\t%s %s, %s, %s\n", mipsTextCodeTable[i].instr, mipsTextCodeTable[i].op1, mipsTextCodeTable[i].op2, mipsTextCodeTable[i].op3);
        }
        else if(strcmp(mipsTextCodeTable[i].instr, "bgez") == 0) {
            fprintf(file, "\t%s %s, %s\n", mipsTextCodeTable[i].instr, mipsTextCodeTable[i].op1, mipsTextCodeTable[i].op2);
        }
        else if(strcmp(mipsTextCodeTable[i].instr, "bgtz") == 0) {
            fprintf(file, "\t%s %s, %s\n", mipsTextCodeTable[i].instr, mipsTextCodeTable[i].op1, mipsTextCodeTable[i].op2);
        }
        else if(strcmp(mipsTextCodeTable[i].instr, "blez") == 0) {
            fprintf(file, "\t%s %s, %s\n", mipsTextCodeTable[i].instr, mipsTextCodeTable[i].op1, mipsTextCodeTable[i].op2);
        }
        else if(strcmp(mipsTextCodeTable[i].instr, "bltz") == 0) {
            fprintf(file, "\t%s %s, %s\n", mipsTextCodeTable[i].instr, mipsTextCodeTable[i].op1, mipsTextCodeTable[i].op2);
        }
        else if(strcmp(mipsTextCodeTable[i].instr, "bne") == 0) {
            fprintf(file, "\t%s %s, %s, %s\n", mipsTextCodeTable[i].instr, mipsTextCodeTable[i].op1, mipsTextCodeTable[i].op2, mipsTextCodeTable[i].op3);
        }
        else if(strcmp(mipsTextCodeTable[i].instr, "div") == 0) {
            fprintf(file, "\t%s %s, %s\n", mipsTextCodeTable[i].instr, mipsTextCodeTable[i].op1, mipsTextCodeTable[i].op2);
        }
        else if(strcmp(mipsTextCodeTable[i].instr, "j") == 0) {
            fprintf(file, "\t%s %s\n", mipsTextCodeTable[i].instr, mipsTextCodeTable[i].op1);
        }
        else if(strcmp(mipsTextCodeTable[i].instr, "jal") == 0) {
            fprintf(file, "\t%s %s\n", mipsTextCodeTable[i].instr, mipsTextCodeTable[i].op1);
        }
        else if(strcmp(mipsTextCodeTable[i].instr, "jr") == 0) {
            fprintf(file, "\t%s %s\n", mipsTextCodeTable[i].instr, mipsTextCodeTable[i].op1);
        }
        else if(strcmp(mipsTextCodeTable[i].instr, "la") == 0) {
            fprintf(file, "\t%s %s, %s\n", mipsTextCodeTable[i].instr, mipsTextCodeTable[i].op1, mipsTextCodeTable[i].op2);
        }
        else if(strcmp(mipsTextCodeTable[i].instr, "li") == 0) {
            fprintf(file, "\t%s %s, %s\n", mipsTextCodeTable[i].instr, mipsTextCodeTable[i].op1, mipsTextCodeTable[i].op2);
        }
        else if(strcmp(mipsTextCodeTable[i].instr, "lw") == 0) {
            fprintf(file, "\t%s %s, %s(%s)\n", mipsTextCodeTable[i].instr, mipsTextCodeTable[i].op1, mipsTextCodeTable[i].op2, mipsTextCodeTable[i].op3);
        }
        else if(strcmp(mipsTextCodeTable[i].instr, "mflo") == 0) {
            fprintf(file, "\t%s %s\n", mipsTextCodeTable[i].instr, mipsTextCodeTable[i].op1);
        }
        else if(strcmp(mipsTextCodeTable[i].instr, "move") == 0) {
            fprintf(file, "\t%s %s, %s\n", mipsTextCodeTable[i].instr, mipsTextCodeTable[i].op1, mipsTextCodeTable[i].op2);
        }
        else if(strcmp(mipsTextCodeTable[i].instr, "mult") == 0) {
            fprintf(file, "\t%s %s, %s\n", mipsTextCodeTable[i].instr, mipsTextCodeTable[i].op1, mipsTextCodeTable[i].op2);
        }
        else if(strcmp(mipsTextCodeTable[i].instr, "sll") == 0) {
            fprintf(file, "\t%s %s, %s, %s\n", mipsTextCodeTable[i].instr, mipsTextCodeTable[i].op1, mipsTextCodeTable[i].op2, mipsTextCodeTable[i].op3);
        }
        else if(strcmp(mipsTextCodeTable[i].instr, "sub") == 0) {
            fprintf(file, "\t%s %s, %s, %s\n", mipsTextCodeTable[i].instr, mipsTextCodeTable[i].op1, mipsTextCodeTable[i].op2, mipsTextCodeTable[i].op3);
        }
        else if(strcmp(mipsTextCodeTable[i].instr, "subu") == 0) {
            fprintf(file, "\t%s %s, %s, %s\n", mipsTextCodeTable[i].instr, mipsTextCodeTable[i].op1, mipsTextCodeTable[i].op2, mipsTextCodeTable[i].op3);
        }
        else if(strcmp(mipsTextCodeTable[i].instr, "sw") == 0) {
            fprintf(file, "\t%s %s, %s(%s)\n", mipsTextCodeTable[i].instr, mipsTextCodeTable[i].op1, mipsTextCodeTable[i].op2, mipsTextCodeTable[i].op3);
        }
        else if(strcmp(mipsTextCodeTable[i].instr, "syscall") == 0) {
            fprintf(file, "\t%s\n", mipsTextCodeTable[i].instr);
        }
        else {
            fprintf(file, "%s\n", mipsTextCodeTable[i].instr);
        }
    }
}