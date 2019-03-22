#include "optimize_DAG.h"

extern int mid_code_top;                                                //中间代码表栈顶指针
extern struct mid_code midCodeTable[MID_CODE_TABLE_LENGTH];             //中间代码表
extern int globalTop;                                                   //记录最后一个全局变量/常量在符号表中的位置
extern struct myTab tab[TAB_LENGTH];    //符号表
extern int tab_top;          //符号表栈顶指针
extern int tempVarNum;          //临时变量编号

struct block block_division[MID_CODE_TABLE_LENGTH];         //记录中间代码中的基本块划分。与中间代码一一对应，从1开始
struct myNodeTable nodeTable[NODE_TABLE_LENGTH];            //DAG图优化中的节点表，从1存储
int node_table_top;                                         //节点表栈顶指针
struct myDAG dag[DAG_LENGTH];                               //DAG图，从1存储
int dag_top;                                                //DAG图栈顶指针
struct mid_code tempMidCodeTable[MID_CODE_TABLE_LENGTH];    //临时符号表
int temp_mid_code_top;                                      //临时符号表栈顶指针
int dag_queue[DAG_LENGTH];                                  //放置DAG图中间结点的队列
int dag_queue_top;                                          //队列栈顶指针

char tempFunctionInOptimizeDAG[TOKENMAXLENGTH];             //记录当前处理的函数



void optimize_DAG()
{
    temp_mid_code_top = 0;
    //构建DAG图消除局部公共子表达式
    divideBlock();          //先划分基本块
//    printBlockDivision();/////////////////////////////////////////
    int current_start;          //记录当前基本块的开始下标
    int next_start = 1;             //记录下一基本块的开始下标
    while(next_start != 0) {
        current_start = next_start;
        next_start = findNextStart(current_start);      //更新运算的基本块
        if(((next_start-current_start) == 1) || (current_start == mid_code_top)) {      //如果当前基本块只有一行中间代码
            temp_mid_code_top++;
            copyMidCodeTable(1, current_start, temp_mid_code_top);
            if(midCodeTable[current_start].op == FUNCINT || midCodeTable[current_start].op == FUNCCHAR || midCodeTable[current_start].op == FUNCVOID) {
                strcpy(tempFunctionInOptimizeDAG, midCodeTable[current_start].dst);       //记录当前函数名
            }
        }
        else {
            drawDAG(current_start, next_start);
            DAGToMidCode();
        }
    }
    //再把中间代码从temp表写回正式表
    int i;
    for(i = 1; i<=temp_mid_code_top; i++) {
        copyMidCodeTable(0, i, i);
    }
    mid_code_top = temp_mid_code_top;
}

void divideBlock()      //划分基本块
{
    int i;
    for(i = 0; i<MID_CODE_TABLE_LENGTH; i++) {          //初始化
        block_division[i].isStart = 0;                      //把所有的基本块开始标志初始化为0
        block_division[i].dst1 = i+1;                       //第一个跳转位置初始化为下一条指令
        block_division[i].dst2 = -1;                        //第二个跳转位置初始化为不存在
    }

    block_division[1].isStart = 1;

    for(i = 2; i<=mid_code_top; i++) {
        if(midCodeTable[i].op == FUNCINT ||
           midCodeTable[i].op == FUNCCHAR ||
           midCodeTable[i].op == FUNCVOID) {
            block_division[i].isStart = 1;
            block_division[i+1].isStart = 1;
        }
        else if(midCodeTable[i].op == RET ||
                midCodeTable[i].op == RET_EXTRA) {                       //如果是RET，则可以离开，是基本块的结束语句
            block_division[i].isStart = 1;                            //当前行设为基本块的开始
            block_division[i+1].isStart = 1;                            //把RET的下一行设为基本块的开始
            block_division[i].dst1 = -1;                                //RET直接指向出口
        }
        else if(midCodeTable[i].op == IF_LESS ||
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
                midCodeTable[i].op == CASE_OP) {                    //如果是BNZ跳转语句
            block_division[i].isStart = 1;
            block_division[i+1].isStart = 1;                            //把BNZ跳转语句的下一行设为基本块的开始
            block_division[i].dst1 = i+1;                               //第一个跳转位置为下一行，即不跳转
            int l = findLabel(midCodeTable[i].dst);                     //找到跳转的label对应的中间代码下标
            block_division[i].dst2 = l;                                 //第二个跳转位置设为跳转的label的位置
        }
        else if(midCodeTable[i].op == GOTO_OP) {                    //如果是GOTO跳转语句
            block_division[i].isStart = 1;
            block_division[i+1].isStart = 1;                            //把GOTO跳转语句的下一行设为基本块的开始
            int l = findLabel(midCodeTable[i].dst);                     //找到跳转的label对应的中间代码下标
            block_division[i].dst1 = l;                                 //第一个跳转位置设为跳转的label的位置
        }
        else if(midCodeTable[i].op == CALL) {                       //如果是CALL语句
            block_division[i].isStart = 1;
            block_division[i+1].isStart = 1;                            //把CALL语句的下一行设为基本块的开始
            //由于一个函数视为一个单独的整体，所以CALL语句的后继语句就是CALL语句下一行的语句
        }
        else if(midCodeTable[i].op == WHILE_START_OP) {
            block_division[i].isStart = 1;
            block_division[i+1].isStart = 1;
        }
        else if(midCodeTable[i].op != ASSIGN_OP &&
                midCodeTable[i].op != PLUS_OP &&
                midCodeTable[i].op != MINUS_OP &&
                midCodeTable[i].op != MULT_OP &&
                midCodeTable[i].op != DIV_OP &&
                midCodeTable[i].op != ARRAY_GET) {      //不是跳转语句，也不是运算语句的（因为优化只能发生在连续的运算语句中，所以只要遇到不是运算语句的，就开始一个新的基本块）
            block_division[i].isStart = 1;
            block_division[i+1].isStart = 1;
        }
    }
}

int findLabel(char labelNumber[])       //根据label查找label对应的中间代码下标（这里label虽然是字符串，但实际上只是一个整数）
{
    int lbn = stringToNum(labelNumber);
    int i;
    for(i = 1; i<=mid_code_top; i++) {
        if(midCodeTable[i].label == lbn) {
            return i;
        }
    }
    return 0;
}

void printBlockDivision()       //输出基本块划分结果
{
    printf("This is block division\n");
    int i;
    for(i = 1; i<=mid_code_top; i++) {
        printf("%-5d %5d %5d %5d\n", i, block_division[i].isStart, block_division[i].dst1, block_division[i].dst2);
    }
}

void printDAG()        //输出DAG图
{
    printf("This is DAG\n");
    int i;
    for(i = 1; i<=dag_top; i++) {
        printf("%5d %5d %5d %5d\n", dag[i].index, dag[i].op, dag[i].left, dag[i].right);
    }
}

int findNextStart(int current_start)    //根据当前基本块的开始下标查找下一基本块的开始下标
{
    int i;
    for(i = current_start+1; i<=mid_code_top; i++) {
        if(block_division[i].isStart == 1) {
            return i;                                   //如果找到了下一基本块的下标，返回这个下标
        }
    }
    return 0;          //如果没找到，则这已经是最后一个基本块，返回-1
}

void drawDAG(int current_start, int next_start)    //构建DAG图消除局部公共子表达式
{
    int i;
    node_table_top = 0;
    dag_top = 0;
    int end = (next_start == 0) ? (mid_code_top+1) : next_start;        //如果next_start为0，即当前块是最后一个块，则把块结尾置为中间代码表栈顶指针加一
    for(i = current_start; i<end; i++) {         //遍历基本块中的每一行中间代码
        if(midCodeTable[i].op == FUNCINT || midCodeTable[i].op == FUNCCHAR || midCodeTable[i].op == FUNCVOID) {
            strcpy(tempFunctionInOptimizeDAG, midCodeTable[i].dst);       //记录当前函数名
        }
        else if(midCodeTable[i].op == ASSIGN_OP) {                  //dst = src1
            dag_assign(i);
        }
        else if(midCodeTable[i].op == ARRAY_ASSIGN) {               //dst = src1 []= src2
            dag_array_assign(i);
        }
        else if(midCodeTable[i].op == PLUS_OP) {                    //dst = src1 + src
            dag_plus(i);
        }
        else if(midCodeTable[i].op == MINUS_OP) {                   //dst = src1 - src2
            dag_minus(i);
        }
        else if(midCodeTable[i].op == MULT_OP) {                    //dst = src1 * src2
            dag_mult(i);
        }
        else if(midCodeTable[i].op == DIV_OP) {                     //dst = src1 / src2
            dag_div(i);
        }
        else if(midCodeTable[i].op == ARRAY_GET) {                  //dst = src1 [] src2
            dag_array_get(i);
        }
    }
//    printDAG();
}

void DAGToMidCode()      //把DAG图重新转换为中间代码
{
    dag_queue_top = 0;
    while(haveNodeNotInQueue()) {
        int n = chooseANode();      //选取一个结点
        dag_queue_top++;
        dag_queue[dag_queue_top] = n;       //加入队列
        dag[n].isInQueue = 1;               //在DAG图中标记为已加入队列

        while((n = dag[n].left) != 0) {     //遍历n的左子结点
            if(canAddToQueue(n)) {
                dag_queue_top++;
                dag_queue[dag_queue_top] = n;       //加入队列
                dag[n].isInQueue = 1;               //在DAG图中标记为已加入队列
            }
            else {
                break;
            }
        }
    }
    //为dag图中所有添0变量创建临时变量
    int i;
    for(i = 1; i<=dag_top; i++) {
        if(dag[i].op == 1 && tab[dag[i].index].object != ARRAY_TAB_VALUE) {
            char tempVar[20];
            newTempVariable(tempVar);
            temp_mid_code_top++;
            tempMidCodeTable[temp_mid_code_top].label = 0;
            tempMidCodeTable[temp_mid_code_top].op = ASSIGN_OP;
            strcpy(tempMidCodeTable[temp_mid_code_top].dst, tempVar);
            strcpy(tempMidCodeTable[temp_mid_code_top].src1, tab[dag[i].index].name);
            strcpy(tempMidCodeTable[temp_mid_code_top].src2, "");
            tempMidCodeTable[temp_mid_code_top].effective = 1;
            node_table_top++;
            nodeTable[node_table_top].index = -tempVarToNum(tempVar);
            nodeTable[node_table_top].number = i;
            nodeTable[node_table_top].type = 1;
            nodeTable[node_table_top].assigned = 1;

            struct item * tempItem = (struct item*)malloc(sizeof(struct item));
            tempItem->type = tab[dag[i].index].type;
            strcpy(tempItem->identifier, tempVar);
            recordTempVarType(tempItem);                        //记录新申请的临时变量类型
        }
    }
    //此时已将所有结点加入队列
    for(i = dag_queue_top; i>=1; i--) {         //逆序计算各个结点
        char src1[TOKENMAXLENGTH];
        char src2[TOKENMAXLENGTH];
        int op;
        char dst[TOKENMAXLENGTH];

        int left = dag[dag_queue[i]].left;
        int right = dag[dag_queue[i]].right;
        //先处理左子树src1
        if(left != 0) {
            if(dag[left].op == 8) {                         //如果是常量
                sprintf(src1, "%d", dag[left].index);
            }
            else if(dag[left].op == 1) {                    //如果是添0变量
                if(tab[dag[left].index].object == ARRAY_TAB_VALUE) {        //如果是数组
                    strcpy(src1, tab[dag[left].index].name);
                }
                else {
                    //从结点表中找到一个结果在这里的变量
                    int j;
                    for(j = 1; j<=node_table_top; j++) {
                        if(nodeTable[j].number == left) {
                            if(nodeTable[j].index > 0) {
                                strcpy(src1, tab[nodeTable[j].index].name);
                            }
                            else if(nodeTable[j].index < 0){
                                sprintf(src1, "$%d", -nodeTable[j].index);
                            }
                            break;
                        }
                    }
                }
            }
            else if(dag[left].op == 0) {                    //如果是非添0变量
                if(dag[left].index > 0) {
                    strcpy(src1, tab[dag[left].index].name);        //如果是声明变量，在符号表中找到变量名，并存入src1
                }
                else if(dag[left].index < 0) {
                    sprintf(src1, "$%d", -dag[left].index);         //如果是临时变量，转为$形式存入src1
                }
                else {
                    strcpy(src1, "$ret");                           //如果是$ret
                }
            }
            else {                                          //如果是中间结点
                //从结点表中找到一个结果在这里的变量
                int j;
                for(j = 1; j<=node_table_top; j++) {
                    if(nodeTable[j].number == left) {
                        if(nodeTable[j].index > 0) {
                            strcpy(src1, tab[nodeTable[j].index].name);
                        }
                        else if(nodeTable[j].index < 0){
                            sprintf(src1, "$%d", -nodeTable[j].index);
                        }
                        break;
                    }
                }
            }
        }
        //再处理右子树src2
        if(right != 0) {
            if(dag[right].op == 8) {                         //如果是常量
                sprintf(src2, "%d", dag[right].index);
            }
            else if(dag[right].op == 1) {                    //如果是添0变量
                if(tab[dag[right].index].object == ARRAY_TAB_VALUE) {        //如果是数组
                    strcpy(src2, tab[dag[left].index].name);
                }
                else {
                    //从结点表中找到一个结果在这里的变量
                    int j;
                    for(j = 1; j<=node_table_top; j++) {
                        if(nodeTable[j].number == right) {
                            if(nodeTable[j].index > 0) {
                                strcpy(src2, tab[nodeTable[j].index].name);
                            }
                            else if(nodeTable[j].index < 0){
                                sprintf(src2, "$%d", -nodeTable[j].index);
                            }
                            break;
                        }
                    }
                }
            }
            else if(dag[right].op == 0) {                    //如果是非添0变量
                if(dag[right].index > 0) {
                    strcpy(src2, tab[dag[right].index].name);        //如果是声明变量，在符号表中找到变量名，并存入src1
                }
                else if(dag[right].index < 0) {
                    sprintf(src2, "$%d", -dag[right].index);         //如果是临时变量，转为$形式存入src1
                }
                else {
                    strcpy(src2, "$ret");                           //如果是$ret
                }
            }
            else {                                          //如果是中间结点
                //从结点表中找到一个结果在这里的变量
                int j;
                for(j = 1; j<=node_table_top; j++) {
                    if(nodeTable[j].number == right) {
                        if(nodeTable[j].index > 0) {
                            strcpy(src2, tab[nodeTable[j].index].name);
                        }
                        else if(nodeTable[j].index < 0){
                            sprintf(src2, "$%d", -nodeTable[j].index);
                        }
                        break;
                    }
                }
            }
        }
        //再处理op
        switch(dag[dag_queue[i]].op) {
            case 2:
                op = PLUS_OP;
                break;
            case 3:
                op = MINUS_OP;
                break;
            case 4:
                op = MULT_OP;
                break;
            case 5:
                op = DIV_OP;
                break;
            case 6:
                op = ARRAY_GET;
                break;
            case 7:
                op = ARRAY_ASSIGN;
                break;
            default:
                op = NO_OP;
                printf("ERROR in DAGToMidCode\n");
        }
        //再处理dst
        int j;          //j是节点表中第一个指向当前结点的变量
        for(j = 1; j<=node_table_top; j++) {
            if(nodeTable[j].number == dag_queue[i]) {
                if(nodeTable[j].index > 0) {
                    strcpy(dst, tab[nodeTable[j].index].name);
                }
                else if(nodeTable[j].index < 0) {
                    sprintf(dst, "$%d", -nodeTable[j].index);
                }
                break;
            }
        }
        nodeTable[j].assigned = 1;  //已经为nodeTable[j]赋值过
        temp_mid_code_top++;
        tempMidCodeTable[temp_mid_code_top].label = 0;
        tempMidCodeTable[temp_mid_code_top].op = op;
        strcpy(tempMidCodeTable[temp_mid_code_top].dst, dst);
        strcpy(tempMidCodeTable[temp_mid_code_top].src1, src1);
        strcpy(tempMidCodeTable[temp_mid_code_top].src2, src2);
        tempMidCodeTable[temp_mid_code_top].effective = 1;                  //生成中间代码
        //为j之后的指向当前结点的变量生成中间代码
        for(j++; j<=node_table_top; j++) {
            if(nodeTable[j].number == dag_queue[i]) {
                if(nodeTable[j].index > 0) {
                    temp_mid_code_top++;
                    tempMidCodeTable[temp_mid_code_top].label = 0;
                    tempMidCodeTable[temp_mid_code_top].op = ASSIGN_OP;
                    strcpy(tempMidCodeTable[temp_mid_code_top].dst, tab[nodeTable[j].index].name);
                    strcpy(tempMidCodeTable[temp_mid_code_top].src1, dst);
                    strcpy(tempMidCodeTable[temp_mid_code_top].src2, "");
                    tempMidCodeTable[temp_mid_code_top].effective = 1;
                }
                else if(nodeTable[j].index < 0) {
                    temp_mid_code_top++;
                    tempMidCodeTable[temp_mid_code_top].label = 0;
                    tempMidCodeTable[temp_mid_code_top].op = ASSIGN_OP;
                    sprintf(tempMidCodeTable[temp_mid_code_top].dst, "$%d", -nodeTable[j].index);
                    strcpy(tempMidCodeTable[temp_mid_code_top].src1, dst);
                    strcpy(tempMidCodeTable[temp_mid_code_top].src2, "");
                    tempMidCodeTable[temp_mid_code_top].effective = 1;
                }
                nodeTable[j].assigned = 1;  //已经为nodeTable[j]赋值过
            }
        }
    }
    //为nodeTable中没赋值过的变量赋值
    for(i = 1; i<=node_table_top; i++) {
        if(nodeTable[i].assigned == 0 && nodeTable[i].index != 0 && nodeTable[i].type == 1) {      //如果nodeTable[i]没有赋值过
            if(nodeTable[i].index>0 && nodeTable[i].type == 1 && tab[nodeTable[i].index].object == CONST_TAB_VALUE) {
                //如果nodeTable[i]是符号表中的一个常量
                nodeTable[i].assigned = 1;
                continue;
            }
            //检查dag[nodeTable[i].number]
            if(dag[nodeTable[i].number].op == 0) {      //如果i指向的结点是普通结点
                char src1[TOKENMAXLENGTH];
                char dst[TOKENMAXLENGTH];
                if(dag[nodeTable[i].number].index > 0) {
                    strcpy(src1, tab[dag[nodeTable[i].number].index].name);
                }
                else if(dag[nodeTable[i].number].index < 0) {
                    sprintf(src1, "$%d", -dag[nodeTable[i].number].index);
                }
                else {
                    strcpy(src1, "$ret");
                }
                if(nodeTable[i].index > 0) {
                    strcpy(dst, tab[nodeTable[i].index].name);
                }
                else {
                    sprintf(dst, "$%d", -nodeTable[i].index);
                }
                temp_mid_code_top++;
                tempMidCodeTable[temp_mid_code_top].label = 0;
                tempMidCodeTable[temp_mid_code_top].op = ASSIGN_OP;
                strcpy(tempMidCodeTable[temp_mid_code_top].dst, dst);
                strcpy(tempMidCodeTable[temp_mid_code_top].src1, src1);
                strcpy(tempMidCodeTable[temp_mid_code_top].src2, "");
                tempMidCodeTable[temp_mid_code_top].effective = 1;
            }
            else if(dag[nodeTable[i].number].op == 1) {             //如果i指向的结点是添0结点
                //一定已经为这个添0结点创建过临时变量
                //检查这个添0结点是否是数组
                //如果是数组，那么这不是赋值语句，而是数组调用后再没有使用过，因此数组名还指向原来的位置，也不需要给数组名重新赋值
                if (tab[dag[nodeTable[i].number].index].object != ARRAY_TAB_VALUE) {
                    int j;
                    for(j = 1; j<=node_table_top; j++) {
                        if(nodeTable[j].number == nodeTable[i].number && nodeTable[j].index < 0 && nodeTable[j].assigned == 1) {
                            char dst[TOKENMAXLENGTH];
                            if(nodeTable[i].index > 0) {
                                strcpy(dst, tab[nodeTable[i].index].name);
                            }
                            else {
                                sprintf(dst, "$%d", -nodeTable[i].index);
                            }
                            temp_mid_code_top++;
                            tempMidCodeTable[temp_mid_code_top].label = 0;
                            tempMidCodeTable[temp_mid_code_top].op = ASSIGN_OP;
                            strcpy(tempMidCodeTable[temp_mid_code_top].dst, dst);
                            sprintf(tempMidCodeTable[temp_mid_code_top].src1, "$%d", -nodeTable[j].index);
                            strcpy(tempMidCodeTable[temp_mid_code_top].src2, "");
                            tempMidCodeTable[temp_mid_code_top].effective = 1;
                            break;
                        }
                    }
                }
            }
            else if(dag[nodeTable[i].number].op == 8) {             //如果i指向的结点是常量
                char dst[TOKENMAXLENGTH];
                if(nodeTable[i].index > 0) {
                    strcpy(dst, tab[nodeTable[i].index].name);
                }
                else {
                    sprintf(dst, "$%d", -nodeTable[i].index);
                }
                temp_mid_code_top++;
                tempMidCodeTable[temp_mid_code_top].label = 0;
                tempMidCodeTable[temp_mid_code_top].op = ASSIGN_OP;
                strcpy(tempMidCodeTable[temp_mid_code_top].dst, dst);
                sprintf(tempMidCodeTable[temp_mid_code_top].src1, "%d", dag[nodeTable[i].number].index);
                strcpy(tempMidCodeTable[temp_mid_code_top].src2, "");
                tempMidCodeTable[temp_mid_code_top].effective = 1;
            }
            else {          //如果i指向的结点是中间结点，那么一定有运算结果在这里(如果i指向中间结点，那么i一定赋过值了,程序无法到达这里)
                int j;
                for(j = 1; j<=node_table_top; j++) {
                    if(nodeTable[j].number == nodeTable[i].number && j != i) {
                        char dst[TOKENMAXLENGTH];
                        if(nodeTable[i].index > 0) {
                            strcpy(dst, tab[nodeTable[i].index].name);
                        }
                        else {
                            sprintf(dst, "$%d", -nodeTable[i].index);
                        }
                        if(nodeTable[j].index > 0) {
                            temp_mid_code_top++;
                            tempMidCodeTable[temp_mid_code_top].label = 0;
                            tempMidCodeTable[temp_mid_code_top].op = ASSIGN_OP;
                            strcpy(tempMidCodeTable[temp_mid_code_top].dst, dst);
                            strcpy(tempMidCodeTable[temp_mid_code_top].src1, tab[nodeTable[j].index].name);
                            strcpy(tempMidCodeTable[temp_mid_code_top].src2, "");
                            tempMidCodeTable[temp_mid_code_top].effective = 1;
                        }
                        else if(nodeTable[j].index < 0) {
                            temp_mid_code_top++;
                            tempMidCodeTable[temp_mid_code_top].label = 0;
                            tempMidCodeTable[temp_mid_code_top].op = ASSIGN_OP;
                            strcpy(tempMidCodeTable[temp_mid_code_top].dst, dst);
                            sprintf(tempMidCodeTable[temp_mid_code_top].src1, "$%d", -nodeTable[j].index);
                            strcpy(tempMidCodeTable[temp_mid_code_top].src2, "");
                            tempMidCodeTable[temp_mid_code_top].effective = 1;
                        }
                        else {
                            temp_mid_code_top++;
                            tempMidCodeTable[temp_mid_code_top].label = 0;
                            tempMidCodeTable[temp_mid_code_top].op = ASSIGN_OP;
                            strcpy(tempMidCodeTable[temp_mid_code_top].dst, dst);
                            strcpy(tempMidCodeTable[temp_mid_code_top].src1, "$ret");
                            strcpy(tempMidCodeTable[temp_mid_code_top].src2, "");
                            tempMidCodeTable[temp_mid_code_top].effective = 1;
                        }
                        break;
                    }
                }
            }
            nodeTable[i].assigned = 1;
        }
    }
}

int chooseANode()     //在DAG图中选取一个结点来加入队列
{
    int i;
    for(i = dag_top; i>=1; i--) {       //遍历DAG图中的结点  （一般来讲，编号大的结点先加入队列，所以从上向下遍历）
        if(dag[i].isInQueue) {
            continue;           //如果结点i已经加入队列，跳过i
        }
        if(dag[i].left == 0 && dag[i].right == 0) {
            continue;           //如果结点i是叶结点，跳过i
        }
        if(canAddToQueue(i)) {
            return i;
        }
    }
    return 0;
}

int canAddToQueue(int i)        //判断结点i是否可以加入队列
{
    if(dag[i].left == 0 && dag[i].right == 0) {
        return 0;           //如果结点i是叶结点，不可加入队列
    }
    int j;
    for(j = 1; j<=dag_top; j++) {       //遍历DAG图
        if(dag[j].left == i || dag[j].right == i) { //如果dag[j]的左子树或右子树是i，即如果j是i的父结点
            if(dag[j].isInQueue == 0) {     //如果j尚未加入队列，即i有父结点尚未加入队列
                return 0;   //i不能加入队列
            }
        }
    }
    //如果i的所有父结点都已加入队列，则i可以加入队列
    return 1;
}

int haveNodeNotInQueue()     //查找DAG图中是否还有结点未加入队列
{
    int i;
    for(i = 1; i<=dag_top; i++) {
        if(dag[i].isInQueue == 0 && !(dag[i].left == 0 && dag[i].right == 0)) {     //如果i不是叶结点且i尚未加入队列
            return 1;
        }
    }
    return 0;
}

void dag_assign(int i)    //构建DAG图时处理赋值语句
{
    //对于形如 z = x 的表达式
    //处理x
    int x_index = dealX(i);
    //处理z
    int f3 = findInNodeTable(midCodeTable[i].dst);
    if(f3 == 0) {               //如果没找到
        node_table_top++;               //在结点表中新建一项
        if(midCodeTable[i].dst[0] == '$') {     //如果 z 是临时变量
            if(midCodeTable[i].dst[1] == 'r') {      //如果 z 是$ret，虽然这不可能，因为$ret不可能出现在左边
                nodeTable[node_table_top].index = 0;                    //记录z
                nodeTable[node_table_top].type = 1;                         //记录类型
                nodeTable[node_table_top].number = x_index;             //记录结点号
                nodeTable[node_table_top].assigned = 0;
            }
            else {                                  //如果z不是$ret
                int t = -tempVarToNum(midCodeTable[i].dst);     //把临时变量转为整数，并取负值
                nodeTable[node_table_top].index = t;                    //记录z
                nodeTable[node_table_top].type = 1;                         //记录类型
                nodeTable[node_table_top].number = x_index;             //记录结点号
                nodeTable[node_table_top].assigned = 0;
            }
        }
        else {                                  //如果z不是临时变量
            int t = loc_optimizeDAG(midCodeTable[i].dst);         //在符号表中查找z
            nodeTable[node_table_top].index = t;                        //记录z
            nodeTable[node_table_top].type = 1;                         //记录类型
            nodeTable[node_table_top].number = x_index;                 //记录结点号
            nodeTable[node_table_top].assigned = 0;
        }
    }
    else {
        nodeTable[f3].number = x_index;
    }
}

void dag_array_assign(int i)     //构建DAG图时处理数组赋值语句
{
    //对于形如 z = x op y 的形式
    //处理x
    int x_index = dealX(i);
    //处理y
    int y_index = dealY(i);
    //处理op
    int op_index = dealOP(i, 7, x_index, y_index);
    //处理z
    dealZ(i, op_index);
}

void dag_plus(int i)   //构建DAG图时处理加法运算语句
{
    //对于形如 z = x op y 的形式
    //处理x
    int x_index = dealX(i);
    //处理y
    int y_index = dealY(i);
    //处理op
    int op_index = dealOP(i, 2, x_index, y_index);
    //处理z
    dealZ(i, op_index);
}

void dag_minus(int i)      //构建DAG图时处理减法运算语句
{
    //对于形如 z = x op y 的形式
    //处理x
    int x_index = dealX(i);
    //处理y
    int y_index = dealY(i);
    //处理op
    int op_index = dealOP(i, 3, x_index, y_index);
    //处理z
    dealZ(i, op_index);
}

void dag_mult(int i)      //构建DAG图时处理乘法运算语句
{
    //对于形如 z = x op y 的形式
    //处理x
    int x_index = dealX(i);
    //处理y
    int y_index = dealY(i);
    //处理op
    int op_index = dealOP(i, 4, x_index, y_index);
    //处理z
    dealZ(i, op_index);
}

void dag_div(int i)      //构建DAG图时处理除法运算语句
{
    //对于形如 z = x op y 的形式
    //处理x
    int x_index = dealX(i);
    //处理y
    int y_index = dealY(i);
    //处理op
    int op_index = dealOP(i, 5, x_index, y_index);
    //处理z
    dealZ(i, op_index);
}

void dag_array_get(int i)        //构建DAG图时处理数组取值语句
{
    //对于形如 z = x op y 的形式
    //处理x
    int x_index = dealX(i);
    //处理y
    int y_index = dealY(i);
    //处理op
    int op_index = dealOP(i, 6, x_index, y_index);
    //处理z
    dealZ(i, op_index);
}

int dealX(int i)      //在构建DAG图时处理z = x op y中的x，并返回x的结点号
{
    int f1 = findInNodeTable(midCodeTable[i].src1);
    int x_index;
    if (f1 == 0) {               //如果没找到
        if (midCodeTable[i].src1[0] == '$') {                   //如果是临时变量
            if (midCodeTable[i].src1[1] == 'r') {                    //如果是$ret
                dag_top++;                          //在DAG中创建新结点
                dag[dag_top].index = 0;                                     //记录x
                dag[dag_top].op = 0;
                dag[dag_top].left = 0;                                      //左子树空
                dag[dag_top].right = 0;                                     //右子树空
                dag[dag_top].isInQueue = 0;                                 //尚未进入队列
                x_index = dag_top;                                          //记录结点号
                node_table_top++;                   //在结点表中新建一项
                nodeTable[node_table_top].index = 0;                        //记录x
                nodeTable[node_table_top].type = 1;                         //记录类型
                nodeTable[node_table_top].number = x_index;                 //记录结点号
                nodeTable[node_table_top].assigned = 0;
            } else {
                int t = -tempVarToNum(midCodeTable[i].src1);       //把临时变量转为整数，并置为负值
                dag_top++;                          //在DAG中创建新结点
                dag[dag_top].index = t;                                     //记录x
                dag[dag_top].op = 0;
                dag[dag_top].left = 0;                                      //左子树空
                dag[dag_top].right = 0;                                     //右子树空
                dag[dag_top].isInQueue = 0;                                 //尚未进入队列
                x_index = dag_top;                                          //记录结点号
                node_table_top++;                   //在结点表中新建一项
                nodeTable[node_table_top].index = t;                        //记录x
                nodeTable[node_table_top].type = 1;                         //记录类型
                nodeTable[node_table_top].number = x_index;                 //记录结点号
                nodeTable[node_table_top].assigned = 0;
            }
        }
        else if(isConstant(midCodeTable[i].src1)) {             //如果是常量
            dag_top++;
            dag[dag_top].index = constToInt(midCodeTable[i].src1);
            dag[dag_top].op = 8;
            dag[dag_top].left = 0;
            dag[dag_top].right = 0;
            dag[dag_top].isInQueue = 0;                                 //尚未进入队列
            x_index = dag_top;
            node_table_top++;
            nodeTable[node_table_top].index = constToInt(midCodeTable[i].src1);
            nodeTable[node_table_top].type = 2;                         //记录类型
            nodeTable[node_table_top].number = x_index;
            nodeTable[node_table_top].assigned = 0;
        }
        else {
            int t = loc_optimizeDAG(midCodeTable[i].src1);            //查找x在符号表中的下标
            dag_top++;                              //在DAG中创建新结点
            dag[dag_top].index = t;                                         //记录x
            dag[dag_top].op = 1;                                            //右下角添0
            dag[dag_top].left = 0;                                          //左子树空
            dag[dag_top].right = 0;                                         //右子树空
            dag[dag_top].isInQueue = 0;                                     //尚未进入队列
            x_index = dag_top;                                              //记录结点号
            node_table_top++;                       //在结点表中新建一项
            nodeTable[node_table_top].index = t;                            //记录x
            nodeTable[node_table_top].type = 1;                             //记录类型
            nodeTable[node_table_top].number = x_index;                     //记录结点号
            nodeTable[node_table_top].assigned = 0;
        }
    } else {                      //如果找到了
        x_index = nodeTable[f1].number;                 //记录结点号
    }

    return x_index;
}

int dealY(int i)      //在构建DAG图时处理z = x op y中的y，并返回y的结点号
{
    int f2 = findInNodeTable(midCodeTable[i].src2);
    int y_index;
    if(f2 == 0) {               //如果没找到
        if(midCodeTable[i].src2[0] == '$') {
            if(midCodeTable[i].src2[1] == 'r') {                    //如果是$ret
                dag_top++;                          //在DAG中创建新结点
                dag[dag_top].index = 0;                                     //记录y
                dag[dag_top].op = 0;
                dag[dag_top].left = 0;                                      //左子树空
                dag[dag_top].right = 0;                                     //右子树空
                dag[dag_top].isInQueue = 0;                                 //尚未进入队列
                y_index = dag_top;                                          //记录结点号
                node_table_top++;                   //在结点表中新建一项
                nodeTable[node_table_top].index = 0;                        //记录y
                nodeTable[node_table_top].type = 1;                         //记录类型
                nodeTable[node_table_top].number = y_index;                 //记录结点号
                nodeTable[node_table_top].assigned = 0;
            }
            else {
                int t = -tempVarToNum(midCodeTable[i].src2);       //把临时变量转为整数，并置为负值
                dag_top++;                          //在DAG中创建新结点
                dag[dag_top].index = t;                                     //记录y
                dag[dag_top].op = 0;
                dag[dag_top].left = 0;                                      //左子树空
                dag[dag_top].right = 0;                                     //右子树空
                dag[dag_top].isInQueue = 0;                                 //尚未进入队列
                y_index = dag_top;                                          //记录结点号
                node_table_top++;                   //在结点表中新建一项
                nodeTable[node_table_top].index = t;                        //记录y
                nodeTable[node_table_top].type = 1;                         //记录类型
                nodeTable[node_table_top].number = y_index;                 //记录结点号
                nodeTable[node_table_top].assigned = 0;
            }
        }
        else if(isConstant(midCodeTable[i].src2)) {             //如果是常量
            dag_top++;
            dag[dag_top].index = constToInt(midCodeTable[i].src2);
            dag[dag_top].op = 8;
            dag[dag_top].left = 0;
            dag[dag_top].right = 0;
            dag[dag_top].isInQueue = 0;                                 //尚未进入队列
            y_index = dag_top;
            node_table_top++;
            nodeTable[node_table_top].index = constToInt(midCodeTable[i].src2);
            nodeTable[node_table_top].type = 2;                         //记录类型
            nodeTable[node_table_top].number = y_index;
            nodeTable[node_table_top].assigned = 0;
        }
        else {
            int t = loc_optimizeDAG(midCodeTable[i].src2);            //查找x在符号表中的下标
            dag_top++;                              //在DAG中创建新结点
            dag[dag_top].index = t;                                         //记录y
            dag[dag_top].op = 1;                                            //右下角添0
            dag[dag_top].left = 0;                                          //左子树空
            dag[dag_top].right = 0;                                         //右子树空
            dag[dag_top].isInQueue = 0;                                     //尚未进入队列
            y_index = dag_top;                                              //记录结点号
            node_table_top++;                       //在结点表中新建一项
            nodeTable[node_table_top].index = t;                            //记录y
            nodeTable[node_table_top].type = 1;                             //记录类型
            nodeTable[node_table_top].number = y_index;                     //记录结点号
            nodeTable[node_table_top].assigned = 0;
        }
    }
    else {                      //如果找到了
        y_index = nodeTable[f2].number;                 //记录结点号
    }
    return y_index;
}

int dealOP(int i, int op, int x_index, int y_index)    //在构建DAG图时处理z = x op y中的op，并返回op的结点号
{
    int op_index;
    int fop = findInDAG(op, x_index, y_index);     //在DAG图中查找op
    if (fop == 0) {              //如果没找到
        dag_top++;                              //在DAG图中创建结点
        dag[dag_top].op = op;                   //记录op
        dag[dag_top].left = x_index;            //记录左子树
        dag[dag_top].right = y_index;           //记录右子树
        dag[dag_top].isInQueue = 0;             //尚未进入队列
        op_index = dag_top;                     //记录op的结点号
    } else {                      //如果找到了
        op_index = fop;                         //记录op的结点号
    }

    return op_index;
}

void dealZ(int i, int op_index)      //在构建DAG图时处理z = x op y中的z
{
    int f3 = findInNodeTable(midCodeTable[i].dst);
    if(f3 == 0) {               //如果没找到
        node_table_top++;               //在结点表中新建一项
        if(midCodeTable[i].dst[0] == '$') {     //如果 z 是临时变量
            if(midCodeTable[i].dst[1] == 'r') {      //如果 z 是$ret，虽然这不可能，因为$ret不可能出现在左边
                nodeTable[node_table_top].index = 0;                    //记录z
                nodeTable[node_table_top].type = 1;                         //记录类型
                nodeTable[node_table_top].number = op_index;            //记录结点号
                nodeTable[node_table_top].assigned = 0;
            }
            else {                                  //如果z不是$ret
                int t = -tempVarToNum(midCodeTable[i].dst);     //把临时变量转为整数，并取负值
                nodeTable[node_table_top].index = t;                    //记录z
                nodeTable[node_table_top].type = 1;                         //记录类型
                nodeTable[node_table_top].number = op_index;            //记录结点号
                nodeTable[node_table_top].assigned = 0;
            }
        }
        else {                                  //如果z不是临时变量
            int t = loc_optimizeDAG(midCodeTable[i].dst);         //在符号表中查找z
            nodeTable[node_table_top].index = t;                        //记录z
            nodeTable[node_table_top].type = 1;                         //记录类型
            nodeTable[node_table_top].number = op_index;                //记录结点号
            nodeTable[node_table_top].assigned = 0;
        }
    }
    else {
        nodeTable[f3].number = op_index;        //记录结点号
    }
}

int findInNodeTable(char name[])            //返回name在结点表中的下标      //在节点表中查找name
{
    if(name[0] == '$') {                //如果是临时变量
        if(name[1] == 'r') {                //如果是$ret
            int i;
            for (i = 1; i <= node_table_top; i++) {
                if (nodeTable[i].index == 0 && nodeTable[i].type == 1) {
                    return i;                       //如果找到了，返回下标
                }
            }
            return 0;                               //没找到，返回0
        }
        else {                              //如果不是$ret
            int t = -tempVarToNum(name);         //提取临时变量后面的数字部分，并置为负值
            int i;
            for (i = 1; i <= node_table_top; i++) {
                if (nodeTable[i].index == t && nodeTable[i].type == 1) {
                    return i;                       //如果找到了，返回下标
                }
            }
            return 0;                               //没找到，返回0
        }
    }
    else if(isConstant(name)) {         //如果是常量
        int i;
        int t = constToInt(name);
        for(i = 1; i<=node_table_top; i++) {
            if(nodeTable[i].index == t && nodeTable[i].type == 2) {
                return i;
            }
        }
        return 0;
    }
    else {                              //如果不是临时变量
        int t = loc_optimizeDAG(name);
        int i;
        for(i = 1; i<= node_table_top; i++) {
            if(nodeTable[i].index == t && nodeTable[i].type == 1) {
                return i;
            }
        }
        return 0;
    }
}

int findInDAG(int op, int x_index, int y_index)     //在DAG图中查找op
{
    int i;
    for(i = 1; i<=dag_top; i++) {
        if(dag[i].op == op && dag[i].left == x_index && dag[i].right == y_index) {
            return i;
        }
    }
    return 0;
}

int loc_optimizeDAG(char name[])          //返回name在符号表中的下标      //在tab中查找name
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
    if(strcmp(tempFunctionInOptimizeDAG, "") == 0) {     //如果在全局变量区
        seek_top = globalTop;
    }
    else {      //1.在符号表中查找到当前函数的位置
        int i;
        for (i = tab_top; i > 0; i--) {
            if(strcmp(tab[i].name, tempFunctionInOptimizeDAG) == 0 && tab[i].object == FUNC_TAB_VALUE) {
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

void copyMidCodeTable(int dst, int i, int j)     //复制符号表的某一项，在符号表和temp符号表之间复制
{
    //复制符号表，dst为0表示从temp表到正式表，为1表示从正式表到temp表
    //i为正式表下标，j为temp表下标
    if(dst == 0) {
        midCodeTable[i].label = tempMidCodeTable[j].label;
        midCodeTable[i].op = tempMidCodeTable[j].op;
//        strcpy(midCodeTable[i].dst, tempMidCodeTable[j].dst);
//        strcpy(midCodeTable[i].src1, tempMidCodeTable[j].src1);
//        strcpy(midCodeTable[i].src2, tempMidCodeTable[j].src2);
        sprintf(midCodeTable[i].dst, "%s", tempMidCodeTable[j].dst);
        sprintf(midCodeTable[i].src1, "%s", tempMidCodeTable[j].src1);
        sprintf(midCodeTable[i].src2, "%s", tempMidCodeTable[j].src2);
        midCodeTable[i].effective = tempMidCodeTable[j].effective;
    }
    else if(dst == 1) {
        tempMidCodeTable[j].label = midCodeTable[i].label;
        tempMidCodeTable[j].op = midCodeTable[i].op;
//        strcpy(tempMidCodeTable[j].dst, midCodeTable[i].dst);
//        strcpy(tempMidCodeTable[j].src1, midCodeTable[i].src1);
//        strcpy(tempMidCodeTable[j].src2, midCodeTable[i].src2);
        sprintf(tempMidCodeTable[j].dst, "%s", midCodeTable[i].dst);
        sprintf(tempMidCodeTable[j].src1, "%s", midCodeTable[i].src1);
        sprintf(tempMidCodeTable[j].src2, "%s", midCodeTable[i].src2);
        tempMidCodeTable[j].effective = midCodeTable[i].effective;
    }
}

int isConstant(char name[])     //判断一个字符串是否代表一个常量(整数或字符)
{
    if(name[0] == '\'' || (name[0] >= '0' && name[0] <= '9') || name[0] == '+' || name[0] == '-')
        return 1;
    return 0;
}