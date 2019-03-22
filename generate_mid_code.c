#include "generate_mid_code.h"

extern int foundError;
extern int tab_top;                     //符号表栈顶指针
extern struct myTab tab[TAB_LENGTH];    //符号表

struct myTempTab offsetTempTab[OFFSET_TEMP_TAB_LENGTH]; //临时变量偏移表，记录各临时变量在运行栈中的偏移量
int offsetTempTabTop = 0;         //从1开始存储

struct mid_code midCodeTable[MID_CODE_TABLE_LENGTH];        //0号位不存储
int mid_code_top = 0;
int labelNum = 0;           //记录已经到第几个label了，从label1开始

int global_top_in_mid;      //记录最后一条全局量声明的中间代码位置

/*
 * label表示是否为这一行中间代码生成label
 * op记录操作符，具体数值见.h文件
 * dst记录结果名称
 * src1记录操作数一的名称(如果是常量声明，也可能存储常量)
 * src2记录操作数二的名称
 * effective不传入，在第一次生成中间代码的时候全部为1
 */
void generate_mid_code(int label, int op, char dst[], char src1[], char src2[])
{
    mid_code_top++;
    if(label) {         //如果生成label，记录label数值，在生成mips时，如果是函数，直接以函数名作为label，如果不是，以"label"加label数值作为label
        labelNum++;
        midCodeTable[mid_code_top].label = labelNum;
    }
    else {
        midCodeTable[mid_code_top].label = 0;
    }

    midCodeTable[mid_code_top].op = op;

    strcpy(midCodeTable[mid_code_top].dst, dst);

    strcpy(midCodeTable[mid_code_top].src1, src1);

    strcpy(midCodeTable[mid_code_top].src2, src2);

    midCodeTable[mid_code_top].effective = 1;
}

void fillTempTab()
{
    offsetTempTabTop = 0;
    //tempTab表从1开始存储，存储函数名，并在每个函数名后存储属于它们的临时变量
    int i;
    for(i = 1; i <= mid_code_top; i++) {        //遍历中间代码表
        if(midCodeTable[i].op == FUNCINT || midCodeTable[i].op == FUNCCHAR || midCodeTable[i].op == FUNCVOID) {     //如果是函数声明
            offsetTempTabTop++;
            strcpy(offsetTempTab[offsetTempTabTop].name, midCodeTable[i].dst);      //记录函数名
            int j = loc(midCodeTable[i].dst);   //在符号表中查找函数
            //从这个函数开始在符号表中查找，一直查找到下一个函数
            for(j++; j<=tab_top; j++) {
                if(tab[j].object == FUNC_TAB_VALUE) {
                    break;
                }
            }           //此时j指向下一个函数或符号表栈顶之后一个位置
            j--;        //符号表指针移向当前函数的最后一个 参数/变量/常量， 或如果该函数没有参数或变量常量的话，则直接指向该函数本身
            if(tab[j].object == FUNC_TAB_VALUE || tab[j].object == CONST_TAB_VALUE) {       //如果指向该函数本身或常量
                offsetTempTab[offsetTempTabTop].offset = 4;     //将偏移置为4，（这样下一个临时变量可以直接在4的基础上加4）
            }
            else if(tab[j].object == VAR_TAB_VALUE || tab[j].object == PARA_TAB_VALUE) {       //如果是变量或参数
                offsetTempTab[offsetTempTabTop].offset = tab[j].offset + 4;
            }
            else if(tab[j].object == ARRAY_TAB_VALUE) {     //如果是数组
                offsetTempTab[offsetTempTabTop].offset = tab[j].offset + 4 * tab[j].size;
            }
        }
        else if(midCodeTable[i].dst[0] == '$') {        //如果新申请了一个临时变量
            int j = locTemp(midCodeTable[i].dst);           //在tempTab中查找这个临时变量
            if(j == 0) {            //如果没找到
                offsetTempTabTop++;
                strcpy(offsetTempTab[offsetTempTabTop].name, midCodeTable[i].dst);
                offsetTempTab[offsetTempTabTop].offset = offsetTempTab[offsetTempTabTop-1].offset + 4;
            }
        }
    }
}

int locTemp(char name[])
{
    int i;
    strcpy(offsetTempTab[0].name, name);
    for(i = offsetTempTabTop; i>=0; i--) {
        if(strcmp(offsetTempTab[i].name, name) == 0) {
            return i;
        }
    }
    return 0;
}

void numToString(char numString[], int num)
{
    char temp[20];
    int tempNum;
    int p = 0;
    if(num < 0) {
        tempNum = -num;
        while(tempNum > 0) {
            int t = tempNum % 10;
            temp[p] = (char)('0' + t);
            p++;
            tempNum /= 10;
        }
        int p1 = 0;
        numString[p1] = '-';
        p1++;
        while(p > 0) {
            numString[p1] = temp[p-1];
            p1++;
            p--;
        }
        numString[p1] = 0;
    }
    else if(num > 0){
        tempNum = num;
        while(tempNum > 0) {
            int t = tempNum % 10;
            temp[p] = (char)('0' + t);
            p++;
            tempNum /= 10;
        }
        int p1 = 0;
        while(p > 0) {
            numString[p1] = temp[p-1];
            p1++;
            p--;
        }
        numString[p1] = 0;
    }
    else {
        numString[0] = '0';
        numString[1] = 0;
    }
}

int stringToNum(char string[])
{
    int i;
    int l = (int)strlen(string);
    int sum = 0;
    int op = 0;
    if(string[0] == '+') {
        op = 1;
    }
    else if(string[0] == '-') {
        op = -1;
    }
    for(i = (op == 0) ? 0 : 1; i<l; i++) {
        sum *= 10;
        sum += string[i]-'0';
    }
    return (op == 0) ? sum : sum * op;
}

void printMidCodeTable()
{
    FILE * file = fopen("mid_code_table", "w");
    int i;
    for(i = 1; i<=mid_code_top; i++) {
        fprintf(file, "%d | %d | %s | %s | %s | %d\n"
                      , midCodeTable[i].label
                      , midCodeTable[i].op
                      , midCodeTable[i].dst
                      , midCodeTable[i].src1
                      , midCodeTable[i].src2
                      , midCodeTable[i].effective);
    }
    fclose(file);
}

void printMidCode(int o)
{
    FILE * file;
    if(o == 0) {
        file = fopen("mid_code.txt", "w");
    }
    else {   //(o == 1)
        file = fopen("mid_code_opt.txt", "w");
    }
//    if(foundError) {    //如果找到错误，不输出
//        exit(0);
//    }
    int i;
    for(i = 0; i<=mid_code_top; i++) {
        //第一步，输出label
        if(midCodeTable[i].label != 0) {
            if(midCodeTable[i].op == FUNCINT || midCodeTable[i].op == FUNCCHAR || midCodeTable[i].op == FUNCVOID) {
                fprintf(file, "%s:\n", midCodeTable[i].dst);   //如果是函数声明，以函数名作为label
            }
            else {
                fprintf(file, "label%d:\n", midCodeTable[i].label); //普通label以label加labelNum为label
            }
        }

        //第二步，判断op，根据op输出后面部分
        switch(midCodeTable[i].op) {
            case 0:     //NO_OP
                break;
            case 1:     //CONSTINT
                fprintf(file, "const int %s = %s\n", midCodeTable[i].dst, midCodeTable[i].src1);
                break;
            case 2:     //CONSTCHAR
                fprintf(file, "const char %s = %s\n", midCodeTable[i].dst, midCodeTable[i].src1);
                break;
            case 3:     //VARINT
                fprintf(file, "var int %s\n", midCodeTable[i].dst);
                break;
            case 4:     //VARCHAR
                fprintf(file, "var char %s\n", midCodeTable[i].dst);
                break;
            case 5:     //ARRAYINT
                fprintf(file, "var int %s[%s]\n", midCodeTable[i].dst, midCodeTable[i].src1);
                break;
            case 6:     //ARRAYCHAR
                fprintf(file, "var char %s[%s]\n", midCodeTable[i].dst, midCodeTable[i].src1);
                break;
            case 7:     //FUNCINT
                fprintf(file, "int %s()\n", midCodeTable[i].dst);
                break;
            case 8:     //FUNCCHAR
                fprintf(file, "char %s()\n", midCodeTable[i].dst);
                break;
            case 9:     //FUNCVOID
                fprintf(file, "void %s()\n", midCodeTable[i].dst);
                break;
            case 10:    //PARAINT
                fprintf(file, "para int %s\n", midCodeTable[i].dst);
                break;
            case 11:    //PARACHAR
                fprintf(file, "para char %s\n", midCodeTable[i].dst);
                break;
            case 12:    //RET
                fprintf(file, "return %s\n", midCodeTable[i].src1);
                break;
            case 13:    //IF_LESS
                fprintf(file, "%s < %s\n",  midCodeTable[i].src1, midCodeTable[i].src2);
                fprintf(file, "BNZ label%s\n", midCodeTable[i].dst);
                break;
            case 14:    //IF_LESSEQUAL
                fprintf(file, "%s <= %s\n",  midCodeTable[i].src1, midCodeTable[i].src2);
                fprintf(file, "BNZ label%s\n", midCodeTable[i].dst);
                break;
            case 15:    //IF_MORE
                fprintf(file, "%s > %s\n",  midCodeTable[i].src1, midCodeTable[i].src2);
                fprintf(file, "BNZ label%s\n", midCodeTable[i].dst);
                break;
            case 16:    //IF_MOREEQUAL
                fprintf(file, "%s >= %s\n",  midCodeTable[i].src1, midCodeTable[i].src2);
                fprintf(file, "BNZ label%s\n", midCodeTable[i].dst);
                break;
            case 17:    //IF_NOTEQUAL
                fprintf(file, "%s != %s\n",  midCodeTable[i].src1, midCodeTable[i].src2);
                fprintf(file, "BNZ label%s\n", midCodeTable[i].dst);
                break;
            case 18:    //IF_EQUAL
                fprintf(file, "%s == %s\n",  midCodeTable[i].src1, midCodeTable[i].src2);
                fprintf(file, "BNZ label%s\n", midCodeTable[i].dst);
                break;
            case 19:    //GOTO
                fprintf(file, "GOTO label%s\n", midCodeTable[i].dst);
                break;
            case 20:    //WHILE_LESS
                fprintf(file, "%s < %s\n",  midCodeTable[i].src1, midCodeTable[i].src2);
                fprintf(file, "BNZ label%s\n", midCodeTable[i].dst);
                break;
            case 21:    //WHILE_LESSEQUAL
                fprintf(file, "%s <= %s\n",  midCodeTable[i].src1, midCodeTable[i].src2);
                fprintf(file, "BNZ label%s\n", midCodeTable[i].dst);
                break;
            case 22:    //WHILE_MORE
                fprintf(file, "%s > %s\n",  midCodeTable[i].src1, midCodeTable[i].src2);
                fprintf(file, "BNZ label%s\n", midCodeTable[i].dst);
                break;
            case 23:    //WHILE_MOREEQUAL
                fprintf(file, "%s >= %s\n",  midCodeTable[i].src1, midCodeTable[i].src2);
                fprintf(file, "BNZ label%s\n", midCodeTable[i].dst);
                break;
            case 24:    //WHILE_NOTEQUAL
                fprintf(file, "%s != %s\n",  midCodeTable[i].src1, midCodeTable[i].src2);
                fprintf(file, "BNZ label%s\n", midCodeTable[i].dst);
                break;
            case 25:    //WHILE_EQUAL
                fprintf(file, "%s == %s\n",  midCodeTable[i].src1, midCodeTable[i].src2);
                fprintf(file, "BNZ label%s\n", midCodeTable[i].dst);
                break;
            case 26:    //SCANF
                fprintf(file, "scanf %s\n", midCodeTable[i].dst);
                break;
            case 27:    //PRINTF_OP1
                fprintf(file, "printf %s %s\n", midCodeTable[i].src1, midCodeTable[i].src2);
                break;
            case 28:    //PRINTF_OP2
                fprintf(file, "printf %s\n", midCodeTable[i].src1);
                break;
            case 29:    //PRINTF_OP3
                fprintf(file, "printf %s\n", midCodeTable[i].src2);
                break;
            case 30:    //ASSIGN
                fprintf(file, "%s = %s\n", midCodeTable[i].dst, midCodeTable[i].src1);
                break;
            case 31:    //ARRAY_ASSIGN
                fprintf(file, "%s[%s] = %s\n", midCodeTable[i].dst, midCodeTable[i].src1, midCodeTable[i].src2);
                break;
            case 32:    //CALL
                fprintf(file, "call %s\n", midCodeTable[i].src1);
                break;
            case 33:    //CASE
                fprintf(file, "%s == %s\n",  midCodeTable[i].src1, midCodeTable[i].src2);
                fprintf(file, "BNZ label%s\n", midCodeTable[i].dst);
                break;
            case 34:    //DEFAULT
                break;      //default行只输出一个label，无需跳转
            case 35:    //PUSH
                fprintf(file, "push %s\n", midCodeTable[i].src1);
                break;
            case 36:    //PLUS
                fprintf(file, "%s = %s + %s\n", midCodeTable[i].dst, midCodeTable[i].src1, midCodeTable[i].src2);
                break;
            case 37:    //MINUS
                fprintf(file, "%s = %s - %s\n", midCodeTable[i].dst, midCodeTable[i].src1, midCodeTable[i].src2);
                break;
            case 38:    //MULTIPLY
                fprintf(file, "%s = %s * %s\n", midCodeTable[i].dst, midCodeTable[i].src1, midCodeTable[i].src2);
                break;
            case 39:    //DIVIDE
                fprintf(file, "%s = %s / %s\n", midCodeTable[i].dst, midCodeTable[i].src1, midCodeTable[i].src2);
                break;
            case 40:    //ARRAY_GET
                fprintf(file, "%s = %s[%s]\n", midCodeTable[i].dst, midCodeTable[i].src1, midCodeTable[i].src2);
                break;
            case 41:    //RET
                break;      //return        仅用于生成mips时保证函数能够返回，不输出到中间代码文件里
            case 42:
                break;
            default:
                fprintf(file, "ERROR\n");
                break;
        }
    }
    fclose(file);
}

void printOffsetTempTab()
{
    printf("\n\nThis is temp tab\n");
    int i;
    for(i = 1; i<=offsetTempTabTop; i++) {
        printf("%s %d\n", offsetTempTab[i].name, offsetTempTab[i].offset);
    }
}