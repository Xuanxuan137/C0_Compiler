#include "optimize_del.h"


extern struct block block_division[MID_CODE_TABLE_LENGTH];          //记录中间代码中的基本块划分。与中间代码一一对应，从1开始
extern struct mid_code midCodeTable[MID_CODE_TABLE_LENGTH];         //中间代码表
extern int mid_code_top;                                            //中间代码表栈顶指针
extern struct mid_code tempMidCodeTable[MID_CODE_TABLE_LENGTH];     //临时符号表
extern int temp_mid_code_top;                                       //临时符号表栈顶指针
extern int globalTop;                                                   //记录最后一个全局变量/常量在符号表中的位置
extern struct myTab tab[TAB_LENGTH];    //符号表
extern int tab_top;          //符号表栈顶指针

int global_changed;                     //标记优化是否修改过
char tempFunctionInOptimizeDel[TOKENMAXLENGTH];                     //记录当前处理的函数

void optimize_del()             //删除多余中间代码
{
    int i;
    for(i = 1; i<=mid_code_top; i++) {      //先把中间代码从中间代码表复制到临时中间代码表
        copyMidCodeTable(1, i, i);
    }
    temp_mid_code_top = mid_code_top;

    divide_block_del_temp();         //为处理临时变量划分块

    int current_start;          //记录当前基本块的开始下标
    int next_start = 1;             //记录下一基本块的开始下标
    while(next_start != 0) {
        current_start = next_start;
        next_start = findNextStart(current_start);      //更新运算的基本块
        if(tempMidCodeTable[current_start].op == FUNCINT || tempMidCodeTable[current_start].op == FUNCCHAR || tempMidCodeTable[current_start].op == FUNCVOID) {
            strcpy(tempFunctionInOptimizeDel, tempMidCodeTable[current_start].dst);       //记录当前函数名
        }
        del_mid_code_temp(current_start, next_start);        //删除多余中间代码
    }

    divide_block_del_var();         //为处理正经变量划分块

    next_start = 1;
    while(next_start != 0) {
        current_start = next_start;
        next_start = findNextStart(current_start);      //更新运算的基本块
        if(tempMidCodeTable[current_start].op == FUNCINT ||
           tempMidCodeTable[current_start].op == FUNCCHAR ||
           tempMidCodeTable[current_start].op == FUNCVOID) {
            strcpy(tempFunctionInOptimizeDel, tempMidCodeTable[current_start].dst);       //记录当前函数名
        }
        del_mid_code_var(current_start, next_start);        //删除多余中间代码
    }

//    updateType();       //在常量计算合并时，如果两个char常量计算结果超过了char范围，在把这个结果赋值给临时变量时，需要更新临时变量的类型

    //再把中间代码从temp表写回正式表
    int j = 1;
    for(i = 1; i<=temp_mid_code_top; i++) {
        if(tempMidCodeTable[i].effective == 1) {
            copyMidCodeTable(0, j, i);
            j++;
        }
    }
    mid_code_top = j - 1;
}

void divide_block_del_temp()         //划分基本块，仅划分，不标记跳转
{
    int i;
    for (i = 0; i < MID_CODE_TABLE_LENGTH; i++) {
        block_division[i].isStart = 0;
    }

    for (i = 1; i <= mid_code_top; i++) {
        if (midCodeTable[i].op == FUNCINT ||
            midCodeTable[i].op == FUNCCHAR ||
            midCodeTable[i].op == FUNCVOID) {               //如果是函数声明语句，设为基本块开始
            block_division[i].isStart = 1;
        }
        else if (midCodeTable[i].op == RET ||
                   midCodeTable[i].op == RET_EXTRA) {      //如果是返回语句，设下一句为基本块开始
            block_division[i+1].isStart = 1;
        }
        else if (midCodeTable[i].op == IF_LESS ||
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
                 midCodeTable[i].op == CASE_OP) {           //如果是bnz跳转语句，设下一句为基本块开始
            block_division[i+1].isStart = 1;
        }
        else if(midCodeTable[i].op == GOTO_OP) {            //如果是goto语句，设下一句为基本块开始
            block_division[i+1].isStart = 1;
        }
//        else if(midCodeTable[i].op == CALL) {               //如果是call语句，设下一句为基本块开始
//            block_division[i+1].isStart = 1;
//        }
        else if(midCodeTable[i].op == WHILE_START_OP) {
            block_division[i+1].isStart = 1;
        }
        if(midCodeTable[i].label) {
            block_division[i].isStart = 1;
        }
        //其他语句不作处理
    }
}

void divide_block_del_var()         //划分基本块，仅划分，不标记跳转
{
    int i;
    for (i = 0; i < MID_CODE_TABLE_LENGTH; i++) {
        block_division[i].isStart = 0;
    }

    for (i = 1; i <= mid_code_top; i++) {
        if (midCodeTable[i].op == FUNCINT ||
            midCodeTable[i].op == FUNCCHAR ||
            midCodeTable[i].op == FUNCVOID) {               //如果是函数声明语句，设为基本块开始
            block_division[i].isStart = 1;
        }
        else if (midCodeTable[i].op == RET ||
                 midCodeTable[i].op == RET_EXTRA) {      //如果是返回语句，设下一句为基本块开始
            block_division[i+1].isStart = 1;
        }
        else if (midCodeTable[i].op == IF_LESS ||
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
                 midCodeTable[i].op == CASE_OP) {           //如果是bnz跳转语句，设下一句为基本块开始
            block_division[i+1].isStart = 1;
        }
        else if(midCodeTable[i].op == GOTO_OP) {            //如果是goto语句，设下一句为基本块开始
            block_division[i+1].isStart = 1;
        }
        else if(midCodeTable[i].op == CALL) {               //如果是call语句，设下一句为基本块开始
            block_division[i+1].isStart = 1;
        }
        else if(midCodeTable[i].op == WHILE_START_OP) {
            block_division[i+1].isStart = 1;
        }
        if(midCodeTable[i].label) {
            block_division[i].isStart = 1;
        }
        //其他语句不作处理
    }
}

void del_mid_code_temp(int current_start, int next_start)
{
    const_replace(current_start, next_start);       //把const常量，直接替换成数值

    do {
        global_changed = 0;
        //第1步，计算常量
        del_calc_const(current_start, next_start);
        //第2步，用临时变量初始化时等号右面的值来替换这个临时变量（如果等号右面的值是单一值的话）
        del_tempVar(current_start, next_start);
        //第3步，删除没有用过的临时变量
        del_not_used(current_start, next_start);
    } while(global_changed);    //如果一遍中修改过，就再遍，直到不发生修改


}

void del_mid_code_var(int current_start, int next_start)
{
    do {
        global_changed = 0;
        //第1步，计算常量
        del_calc_const(current_start, next_start);
        //第2步，如果一个正经变量赋值时右边是常量，用这个常量替换正经变量重新赋值之前对于该正经变量的调用
        del_var_replace(current_start, next_start);
        //第3步，如果在基本块内，一个正经变量被连续两次赋值，在该基本块内在第二次次赋值之前该正经变量未被调用过，那么删去第一次赋值
        del_var_not_used(current_start, next_start);
    } while(global_changed);    //如果一遍中修改过，就再遍，直到不发生修改
}

void const_replace(int current_start, int next_start)
{
    /*
     * 这个函数的作用是，把const常量直接替换成常量值
     */
    int i;
    int end = (next_start == 0) ? temp_mid_code_top+1 : next_start;
    for(i = current_start; i<end; i++) {
        if(tempMidCodeTable[i].effective == 0) {
            continue;
        }

        if(tempMidCodeTable[i].op == RET ||
                tempMidCodeTable[i].op == ASSIGN_OP ||
                tempMidCodeTable[i].op == PUSH_OP) {                                                //只检查src1
            if(is_letter(tempMidCodeTable[i].src1[0])) {                //如果是声明过的变量/常量
                int j = loc_optimizeDel(tempMidCodeTable[i].src1);      //在符号表中查找
                if(tab[j].object == CONST_TAB_VALUE) {                      //如果是常量
                    if(tab[j].type == INT_TAB_VALUE) {                          //如果该常量是int
                        sprintf(tempMidCodeTable[i].src1, "%d", tab[j].offset);
                    }
                    else if(tab[j].type == CHAR_TAB_VALUE) {                    //如果该常量是char
                        sprintf(tempMidCodeTable[i].src1, "\'%c\'", tab[j].offset);
                    }
                }
            }
        }
        else if(tempMidCodeTable[i].op == PRINTF_OP1 ||
                tempMidCodeTable[i].op == PRINTF_OP3 ||
                tempMidCodeTable[i].op == ARRAY_GET) {                                              //只检查src2
            if(is_letter(tempMidCodeTable[i].src2[0])) {                //如果是声明过的变量/常量
                int j = loc_optimizeDel(tempMidCodeTable[i].src2);      //在符号表中查找
                if(tab[j].object == CONST_TAB_VALUE) {                      //如果是常量
                    if(tab[j].type == INT_TAB_VALUE) {                          //如果该常量是int
                        sprintf(tempMidCodeTable[i].src2, "%d", tab[j].offset);
                    }
                    else if(tab[j].type == CHAR_TAB_VALUE) {                    //如果该常量是char
                        sprintf(tempMidCodeTable[i].src2, "\'%c\'", tab[j].offset);
                    }
                }
            }
        }
        else if(tempMidCodeTable[i].op == IF_LESS ||
                tempMidCodeTable[i].op == IF_LESSEQUAL ||
                tempMidCodeTable[i].op == IF_MORE ||
                tempMidCodeTable[i].op == IF_MOREEQUAL ||
                tempMidCodeTable[i].op == IF_NOTEQUAL ||
                tempMidCodeTable[i].op == IF_EQUAL ||
                tempMidCodeTable[i].op == WHILE_LESS ||
                tempMidCodeTable[i].op == WHILE_LESSEQUAL ||
                tempMidCodeTable[i].op == WHILE_MORE ||
                tempMidCodeTable[i].op == WHILE_MOREEQUAL ||
                tempMidCodeTable[i].op == WHILE_NOTEQUAL ||
                tempMidCodeTable[i].op == WHILE_EQUAL ||
                tempMidCodeTable[i].op == ARRAY_ASSIGN ||
                tempMidCodeTable[i].op == CASE_OP ||
                tempMidCodeTable[i].op == PLUS_OP ||
                tempMidCodeTable[i].op == MINUS_OP ||
                tempMidCodeTable[i].op == MULT_OP ||
                tempMidCodeTable[i].op == DIV_OP) {                                                 //检查src1和src2
            if(is_letter(tempMidCodeTable[i].src1[0])) {                //如果是声明过的变量/常量
                int j = loc_optimizeDel(tempMidCodeTable[i].src1);      //在符号表中查找
                if(tab[j].object == CONST_TAB_VALUE) {                      //如果是常量
                    if(tab[j].type == INT_TAB_VALUE) {                          //如果该常量是int
                        sprintf(tempMidCodeTable[i].src1, "%d", tab[j].offset);
                    }
                    else if(tab[j].type == CHAR_TAB_VALUE) {                    //如果该常量是char
                        sprintf(tempMidCodeTable[i].src1, "\'%c\'", tab[j].offset);
                    }
                }
            }
            if(is_letter(tempMidCodeTable[i].src2[0])) {                //如果是声明过的变量/常量
                int j = loc_optimizeDel(tempMidCodeTable[i].src2);      //在符号表中查找
                if(tab[j].object == CONST_TAB_VALUE) {                      //如果是常量
                    if(tab[j].type == INT_TAB_VALUE) {                          //如果该常量是int
                        sprintf(tempMidCodeTable[i].src2, "%d", tab[j].offset);
                    }
                    else if(tab[j].type == CHAR_TAB_VALUE) {                    //如果该常量是char
                        sprintf(tempMidCodeTable[i].src2, "\'%c\'", tab[j].offset);
                    }
                }
            }
        }
    }
}

void del_calc_const(int current_start, int next_start)
{
    /*
     * 这个函数的作用是，如果一个等式右端是可以直接计算的常量，那么将它计算出来(不论左边是临时变量还是正经变量)
     */
    int i;
    int end = (next_start == 0) ? temp_mid_code_top+1 : next_start;
    for(i = current_start; i<end; i++) {
        if(tempMidCodeTable[i].effective == 0) {
            continue;       //如果这一句已经被删除，直接跳过
        }

        if(tempMidCodeTable[i].op == PLUS_OP ||
           tempMidCodeTable[i].op == MINUS_OP ||
           tempMidCodeTable[i].op == MULT_OP ||
           tempMidCodeTable[i].op == DIV_OP) {      //如果是+-*/
            int src1 = 0;
            int src1Type = 0;   //记录src1的类型，1为int，2为char
            int src2 = 0;
            int src2Type = 0;   //记录src2的类型，1为int，2为char
            int op = tempMidCodeTable[i].op;
            int result = 0;
            int canCalc = 0;
            if(isConstant(tempMidCodeTable[i].src1)) {      //如果src1是常量
                src1 = constToInt(tempMidCodeTable[i].src1);        //把src1转为整数
                if(tempMidCodeTable[i].src1[0] == '\'') {           //记录src1的类型
                    src1Type = 2;
                }
                else {
                    src1Type = 1;
                }
                canCalc++;          //记录src1是可以计算的类型
            }
            if(isConstant(tempMidCodeTable[i].src2)) {      //如果src2是常量
                src2 = constToInt(tempMidCodeTable[i].src2);        //把src2转为整数
                if(tempMidCodeTable[i].src2[0] == '\'') {           //记录src2的类型
                    src2Type = 2;
                }
                else {
                    src2Type = 1;
                }
                canCalc++;          //记录src2是可以计算的类型
            }
            if(canCalc == 2) {      //如果src1和src2都是常量，可以计算
                switch (op) {
                    case PLUS_OP:
                        result = src1 + src2;
                        break;
                    case MINUS_OP:
                        result = src1 - src2;
                        break;
                    case MULT_OP:
                        result = src1 * src2;
                        break;
                    case DIV_OP:
                        result = src1 / src2;
                        break;
                    default:
                        break;  //不可能到达default
                }
                //修改第i个中间代码
                tempMidCodeTable[i].op = ASSIGN_OP;
                if(src1Type == 2 && src2Type == 2 && (result >= -128 && result <= 127)) { //如果src1和src2都是字符，且结果在char范围内，那么把result以char形式存入i的src1
                    sprintf(tempMidCodeTable[i].src1, "\'%c\'", result);
                }
                else {
                    sprintf(tempMidCodeTable[i].src1, "%d", result);    //否则以int形式存入i的src1
                }
                global_changed = 1;
            }
        }
    }
}

void del_tempVar(int current_start, int next_start)     //尽可能删除临时变量
{
    /*
     * 这个函数的作用是
     * 如果一个临时变量在初始化的时候，等号右面是一个单一的值
     * 那么在以后调用这个临时变量的时候，用初始化时等号右面的值去替换这个临时变量，以减少临时变量的使用
     * （如果一个临时变量只被赋值而没有被使用，就可以删除）
     */
    int i;
    int end = (next_start == 0) ? temp_mid_code_top+1 : next_start;
    for(i = current_start; i<end; i++) {
        if(tempMidCodeTable[i].effective == 0) {
            continue;       //如果这一句已经被删除，直接跳过
        }

        int changed = 0;            //标记在一遍中是否对中间代码作出了修改

        if(tempMidCodeTable[i].op == ASSIGN_OP) {       //如果是赋值语句
            if(strcmp(tempMidCodeTable[i].dst, tempMidCodeTable[i].src1) == 0) {    //如果等号左右相同，删掉这一句
                tempMidCodeTable[i].effective = 0;
                changed = 1;
            }
            else if(tempMidCodeTable[i].dst[0] == '$') {        //如果dst是临时变量
                char temp_dst[TOKENMAXLENGTH];
                char temp_src1[TOKENMAXLENGTH];
                strcpy(temp_dst, tempMidCodeTable[i].dst);          //在temp_dst中记录左侧临时变量名
                strcpy(temp_src1, tempMidCodeTable[i].src1);        //在temp_src1中记录右侧临时变量名
                int j;
                for(j = i+1; j<end; j++) {      //遍历i之后的中间代码，直到temp_src1被赋值，或temp_dst被赋值
                    if(strcmp(tempMidCodeTable[j].dst, temp_dst) == 0 && canAssign(j)) {
                        //如果j是一个对单独变量赋值的语句，且temp_dst出现在了等号左侧，即如果temp_dst被赋值了
                        break;
                    }
                    if(strcmp(tempMidCodeTable[j].dst, temp_src1) == 0 && canAssign(j)) {
                        //如果j是一个对单独变量赋值的语句，且temp_src1出现在了等号左侧，即如果temp_src1被赋值了
                        if(strcmp(tempMidCodeTable[j].src1, temp_dst) == 0) {//如果这个赋值语句右侧的src1和temp_dst相同
                            strcpy(tempMidCodeTable[j].src1, temp_src1);    //用temp_src1替代j的src1
                            changed = 1;
                        }
                        if(strcmp(tempMidCodeTable[j].src2, temp_dst) == 0) {//如果这个赋值语句右侧的src2和temp_dst相同
                            strcpy(tempMidCodeTable[j].src2, temp_src1);    //用temp_src1替代j的src2
                            changed = 1;
                        }
                        break;
                    }
                    if(tempMidCodeTable[j].op == CALL) {
                        //如果j是一个call语句，那么$ret会被重新赋值
                        if(strcmp(temp_src1, "$ret") == 0) {    //如果temp_src1是$ret，那么j过后$ret不能再使用
                            break;
                        }
                    }
                    if(canReplace(j)) {  //如果j是一个可以替换src1,src2的语句
                        if((tempMidCodeTable[j].op == PRINTF_OP1 || tempMidCodeTable[j].op == PRINTF_OP3) &&
                           (strcmp(temp_src1, "$ret") == 0)) {
                            //如果j是printf语句，且temp_src1是$ret，则不替换
                            //由于printf语句在生成mips时需要向$v0写入，而$ret就存在$v0里，会发生冲突，所以不替换
                            //do nothing
                        }
                        else {
                            if (strcmp(tempMidCodeTable[j].src1, temp_dst) == 0) {//如果这个语句右侧的src1和temp_dst相同
                                strcpy(tempMidCodeTable[j].src1, temp_src1);    //用temp_src1替代j的src1
                                changed = 1;
                            }
                            if (strcmp(tempMidCodeTable[j].src2, temp_dst) == 0) {//如果这个语句右侧的src2和temp_dst相同
                                strcpy(tempMidCodeTable[j].src2, temp_src1);    //用temp_src1替代j的src2
                                changed = 1;
                            }
                        }
                    }
                }
            }
        }

        if(changed) {       //多遍直到最优
            i = current_start-1;
            global_changed = 1;
        }
    }
}

void del_not_used(int current_start, int next_start)
{
    /*
     * 这个函数的作用是，如果一个临时变量只被赋值过但没有被使用过，那么把它删除
     */
    int i;
    int end = (next_start == 0) ? temp_mid_code_top+1 : next_start;
    for(i = current_start; i<end; i++) {
        if(tempMidCodeTable[i].effective == 0) {
            continue;           //如果这一句已经被删除，直接跳过
        }
        if(tempMidCodeTable[i].dst[0] == '$' && isAssign(i)) {  //如果i是一个能让dst获得新值的语句，且dst是一个临时变量（即i对临时变量赋值）
            //在后面查找dst是否被使用过，如果没有，把这句删除
            int used = 0;
            char temp_dst[TOKENMAXLENGTH];
            strcpy(temp_dst, tempMidCodeTable[i].dst);
            int j;
            int seek_end = findEnd(current_start);
            for(j = i+1; j<seek_end; j++) {
                if(strcmp(tempMidCodeTable[j].src1, temp_dst) == 0) {
                    used = 1;
                    break;
                }
                if(strcmp(tempMidCodeTable[j].src2, temp_dst) == 0) {
                    used = 1;
                    break;
                }
            }
            if(used == 0) {
                tempMidCodeTable[i].effective = 0;
                global_changed = 1;
            }
        }
    }
}

void del_var_replace(int current_start, int next_start)
{
    /*
     * 如果一个正经变量赋值时右边是常量，用这个常量替换正经变量重新赋值之前对于该正经变量的调用
     */
    int i;
    int end = (next_start == 0) ? temp_mid_code_top+1 : next_start;
    for(i = current_start; i<end; i++) {
        if(tempMidCodeTable[i].effective == 0) {
            continue;       //如果这一句已经被删除，直接跳过
        }

        if(tempMidCodeTable[i].op == ASSIGN_OP) {       //如果是赋值语句
            if(strcmp(tempMidCodeTable[i].dst, tempMidCodeTable[i].src1) == 0) {    //如果等号左右相同，删掉这一句
                tempMidCodeTable[i].effective = 0;
                global_changed = 1;
            }
            else if(is_letter(tempMidCodeTable[i].dst[0]) && isConstant(tempMidCodeTable[i].src1)) {  //如果dst是声明过的变量,并且src1是常量
                char temp_dst[TOKENMAXLENGTH];      //记录dst
                char temp_src1[TOKENMAXLENGTH];     //记录src1的值
                strcpy(temp_dst, tempMidCodeTable[i].dst);
                strcpy(temp_src1, tempMidCodeTable[i].src1);
                int j;
                for(j = i+1; j<end; j++) {      //遍历i之后的中间代码，直到temp_dst被赋值
                    if(strcmp(tempMidCodeTable[j].dst, temp_dst) == 0 && canAssign(j)) {    //如果temp_dst被重新赋值了
                        if(strcmp(tempMidCodeTable[j].src1, temp_dst) == 0) {   //如果这个赋值语句的src1和temp_dst相同
                            strcpy(tempMidCodeTable[j].src1, temp_src1);            //用temp_src1替代j的src1
                            global_changed = 1;
                        }
                        if(strcmp(tempMidCodeTable[j].src2, temp_dst) == 0) {   //如果这个赋值语句的src2和temp_dst相同
                            strcpy(tempMidCodeTable[j].src2, temp_src1);            //用temp_src1替代j的src2
                            global_changed = 1;
                        }
                        break;
                    }
                    if(canReplace(j)) {
                        if (strcmp(tempMidCodeTable[j].src1, temp_dst) == 0) {  //如果这个语句右侧的src1和temp_dst相同
                            strcpy(tempMidCodeTable[j].src1, temp_src1);            //用temp_src1替代j的src1
                            global_changed = 1;
                        }
                        if (strcmp(tempMidCodeTable[j].src2, temp_dst) == 0) {  //如果这个语句右侧的src2和temp_dst相同
                            strcpy(tempMidCodeTable[j].src2, temp_src1);            //用temp_src1替代j的src2
                            global_changed = 1;
                        }
                    }
                }
            }
        }
    }
}

void del_var_not_used(int current_start, int next_start)
{
    /*
     * 如果在基本块内，一个正经变量被连续两次赋值，在该基本块内在第二次次赋值之前该正经变量未被调用过，那么删去第一次赋值
     */
    int i;
    int end = (next_start == 0) ? temp_mid_code_top+1 : next_start;
    for(i = current_start; i<end; i++) {
        if(tempMidCodeTable[i].effective == 0) {
            continue;       //如果这一句已经被删除，直接跳过
        }

        if(canAssign(i)) {       //如果i是对左侧赋值的语句
            if(strcmp(tempMidCodeTable[i].dst, tempMidCodeTable[i].src1) == 0 && tempMidCodeTable[i].op == ASSIGN_OP) {    //如果等号左右相同，删掉这一句
                tempMidCodeTable[i].effective = 0;
                global_changed = 1;
            }
            else if(is_letter(tempMidCodeTable[i].dst[0])) {        //如果dst是声明过的变量
                char temp_dst[TOKENMAXLENGTH];
                strcpy(temp_dst, tempMidCodeTable[i].dst);
                int j;
                for(j = i+1; j<end; j++) {
                    if(canAssign(j) && (strcmp(tempMidCodeTable[j].dst, temp_dst) == 0)) {  //如果对temp_dst重新赋值，且temp_dst没用过
                        tempMidCodeTable[i].effective = 0;  //把上一个对temp_dst的赋值删去
                        break;                              //停止查找
                    }
                    if(strcmp(tempMidCodeTable[j].src1, temp_dst) == 0) {
                        break;  //如果temp_dst用过了，停止查找
                    }
                    if(strcmp(tempMidCodeTable[j].src2, temp_dst) == 0) {
                        break;  //如果temp_dst用过了，停止查找
                    }
                }
            }
        }
    }
}

int findEnd(int current_start)
{
    int i;
    for(i = current_start+1; i<=temp_mid_code_top; i++) {
        if(tempMidCodeTable[i].op == FUNCINT ||
                tempMidCodeTable[i].op == FUNCCHAR ||
                tempMidCodeTable[i].op == FUNCVOID) {
            break;
        }
    }
    return i;
}

int canAssign(int j)    //检测tempMidCodeTable[j]是否是一个对单独变量赋值的语句。单独变量指不是数组元素的变量
{
    if(tempMidCodeTable[j].op == ASSIGN_OP ||
       tempMidCodeTable[j].op == ARRAY_GET ||
       tempMidCodeTable[j].op == PLUS_OP ||
       tempMidCodeTable[j].op == MINUS_OP ||
       tempMidCodeTable[j].op == MULT_OP ||
       tempMidCodeTable[j].op == DIV_OP)
        return 1;
    return 0;
}

int canReplace(int j)   //检测tempMidCodeTable[j]是否是一个可以替换src1，src2的语句
{
    if(tempMidCodeTable[j].op == RET ||
       tempMidCodeTable[j].op == IF_LESS ||
       tempMidCodeTable[j].op == IF_LESSEQUAL ||
       tempMidCodeTable[j].op == IF_MORE ||
       tempMidCodeTable[j].op == IF_MOREEQUAL ||
       tempMidCodeTable[j].op == IF_NOTEQUAL ||
       tempMidCodeTable[j].op == IF_EQUAL ||
       tempMidCodeTable[j].op == WHILE_LESS ||
       tempMidCodeTable[j].op == WHILE_LESSEQUAL ||
       tempMidCodeTable[j].op == WHILE_MORE ||
       tempMidCodeTable[j].op == WHILE_MOREEQUAL ||
       tempMidCodeTable[j].op == WHILE_NOTEQUAL ||
       tempMidCodeTable[j].op == WHILE_EQUAL ||
       tempMidCodeTable[j].op == PRINTF_OP1 ||
       tempMidCodeTable[j].op == PRINTF_OP3 ||
       tempMidCodeTable[j].op == ASSIGN_OP ||
       tempMidCodeTable[j].op == CASE_OP ||
       tempMidCodeTable[j].op == PUSH_OP ||
       tempMidCodeTable[j].op == PLUS_OP ||
       tempMidCodeTable[j].op == MINUS_OP ||
       tempMidCodeTable[j].op == MULT_OP ||
       tempMidCodeTable[j].op == DIV_OP)        //printf_op2是纯字符串输出，不必考虑
        return 1;
    return 0;
}

int isAssign(int j)     //检测tempMidCodeTable[j]是否可以让dst获得一个新值
{
    if(tempMidCodeTable[j].op == ASSIGN_OP ||
       tempMidCodeTable[j].op == PLUS_OP ||
       tempMidCodeTable[j].op == MINUS_OP ||
       tempMidCodeTable[j].op == MULT_OP ||
       tempMidCodeTable[j].op == DIV_OP ||
       tempMidCodeTable[j].op == ARRAY_GET)
        return 1;
    return 0;
}

int loc_optimizeDel(char name[])          //返回name在符号表中的下标      //在tab中查找name
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
    if(strcmp(tempFunctionInOptimizeDel, "") == 0) {     //如果在全局变量区
        seek_top = globalTop;
    }
    else {      //1.在符号表中查找到当前函数的位置
        int i;
        for (i = tab_top; i > 0; i--) {
            if(strcmp(tab[i].name, tempFunctionInOptimizeDel) == 0 && tab[i].object == FUNC_TAB_VALUE) {
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