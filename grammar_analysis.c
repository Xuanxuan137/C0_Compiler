#include "error.h"
#include "grammar_analysis.h"


extern int symbol;
extern FILE * file;
extern int totalCharCount;
extern int charCount;
extern int lineCount;
extern char token[TOKENMAXLENGTH];
extern int num;
extern int mid_code_top;
extern int labelNum;
extern struct mid_code midCodeTable[MID_CODE_TABLE_LENGTH];
extern int global_top_in_mid;



int tab_top = 0;        //符号表栈顶指针，符号表从1号位开始存储
int globalTop;      //记录最后一个全局变量/常量的位置

int tempVarNum = 0;     //记录临时变量编号 用$tempVarNum表示临时变量  $ret表示函数返回值

struct myTab tab[TAB_LENGTH];       //符号表
int tempTab[TEMP_TAB_LENGTH];      // 临时变量表，记录各临时变量的类型

int foundReturnInFunction = 0;      //记录是否在函数中找到了有返回内容的return语句

int caseIndex = 0;          //caseValue的下标
int caseValue[1000];        //记录每个case后面常量的值，以避免重复

int grammar_analysis()
{
    getSymbol();
    program();
    return 0;
}

void program()
{
    int foundMainFunction = 0;      //标记是否找到主函数
    int foundExtraSymbol = 0;       //标记是否在各大部分之间找到多余的代码

    while(symbol != CONSTSY && symbol != INTSY && symbol != CHARSY && symbol != VOIDSY && symbol != EOFSY) { //查找常量说明前的多余标识符
        foundExtraSymbol = 1;
        getSymbol();
    }
    if(foundExtraSymbol) {
        foundExtraSymbol = 0;
        error(10);
    }//////////////////////////////////////////////////////////////////

    if(symbol == CONSTSY) {                 //如果是const，处理常量说明
        constDeclaration();
        globalTop = tab_top;    //如果有全局变量，在全局变量后记录最后一个全局量的位置
        global_top_in_mid = mid_code_top;       //记录最后一个全局量声明在midCode中的位置
    }

    while(symbol != INTSY && symbol != CHARSY && symbol != VOIDSY && symbol != EOFSY) { //查找常量说明和变量说明之间的多余标识符
        foundExtraSymbol = 1;
        getSymbol();
    }
    if(foundExtraSymbol) {
        foundExtraSymbol = 0;
        error(10);
    }//////////////////////////////////////////////////////////////////

    if(symbol == INTSY || symbol == CHARSY) {           //如果是int或char，可能是变量说明或有返回值函数声明
        int tempSymbol = symbol;                                  //因为要再预读两个标识符才能确定是哪种语句，所以先记录一下读到的类型
        int tempTotalCharCount = totalCharCount;                  //因为要再预读两个标识符才能确定是哪种语句，所以先记录一下读文件的位置
        int tempCharCount = charCount;
        int tempLineCount = lineCount;
        getSymbol();                                //再读一个符号，不论是变量说明还是有返回值函数声明，这个符号都应该是identifier
        if(symbol == IDSY) {                    //如果是identifier，正确
            getSymbol();                //此时再读一个符号，如果是左小括号，则是有返回值函数声明，否则是变量说明
            if(symbol == LEFTPARENTHESESSY) {                   //如果是（，是有返回值函数声明
                globalTop = tab_top;   //如果没有全局变量，在全局常量后记录最后一个全局量的位置
                global_top_in_mid = mid_code_top;       //记录最后一个全局量声明在midCode中的位置
                fseek(file, -(totalCharCount - tempTotalCharCount), SEEK_CUR);          //把读文件指针退回到刚读完类型标识符的位置
                symbol = tempSymbol;                                                    //把symbol退回到刚读完类型标识符的状态，以便进行函数处理；
                totalCharCount = tempTotalCharCount;
                charCount = tempCharCount;
                lineCount = tempLineCount;
                functionWithReturnValueDeclaration();               //处理有返回值函数声明
            }
            else {                                              //如果不是（，则是变量说明
                fseek(file, -(totalCharCount - tempTotalCharCount), SEEK_CUR);          //把读文件指针退回到刚读完类型标识符的位置
                symbol = tempSymbol;                                                //把symbol退回到刚读完类型标识符的状态，以便进行变量说明处理；
                totalCharCount = tempTotalCharCount;
                charCount = tempCharCount;
                lineCount = tempLineCount;
                variableDeclaration();                      //处理变量说明
                globalTop = tab_top;    //如果有全局变量，在全局变量后记录最后一个全局量的位置
                global_top_in_mid = mid_code_top;       //记录最后一个全局量声明在midCode中的位置
            }
        }
        else {
            error(100);          //identifier required after int or char
            skip(14);     //在这里，类型标识符之后不是标识符，无法做容错处理     skip至下一个分号
        }
    }

    int lastSymbol = 0;
    while(symbol != INTSY && symbol != CHARSY && symbol != VOIDSY && symbol != EOFSY) { //查找变量说明和函数说明之间的多余标识符
        foundExtraSymbol = 1;
        lastSymbol = symbol;
        getSymbol();
    }
    if(foundExtraSymbol) {
        foundExtraSymbol = 0;
        error(10);
        if(symbol == VOIDSY) {  //如果是void，应该是函数声明，不跳读，进入函数声明
            //do nothing
        }
        else {
            if(lastSymbol == CONSTSY) {     //如果上一个是const，且这个是int/char
                skip(15);           //跳至分号
            }
            else {
                //do nothing
            }
        }
    }//////////////////////////////////////////////////////////////////

    while(symbol == INTSY || symbol == CHARSY || symbol == VOIDSY) {
        if(symbol == INTSY || symbol == CHARSY) {
            functionWithReturnValueDeclaration();               //如果是int或char，处理有返回值函数声明
        }
        else {                              //如果是void，可能是无返回值函数声明，也可能是main函数
            int tempSymbol = symbol;
            int tempTotalCharCount = totalCharCount;            //记录状态
            int tempCharCount = charCount;
            int tempLineCount = lineCount;
            getSymbol();
            if(symbol == IDSY) {                        //如果void后是标识符
                fseek(file, -(totalCharCount - tempTotalCharCount), SEEK_CUR);
                symbol = tempSymbol;                                //恢复至刚读完类型标识符的状态
                totalCharCount = tempTotalCharCount;
                charCount = tempCharCount;
                lineCount = tempLineCount;
                functionWithoutReturnValueDeclaration();            //处理无返回值函数声明
            }
            else {                                  //如果void后不是标识符
                fseek(file, -(totalCharCount - tempTotalCharCount), SEEK_CUR);
                symbol = tempSymbol;                                //恢复至刚读完类型标识符的状态
                totalCharCount = tempTotalCharCount;
                charCount = tempCharCount;
                lineCount = tempLineCount;
                mainFunction();
                foundMainFunction = 1;
                break;              //找到主函数后程序应该全部结束
            }
        }

        while(symbol != INTSY && symbol != CHARSY && symbol != VOIDSY && symbol != EOFSY) { //查找函数说明之间的多余标识符
            foundExtraSymbol = 1;
            getSymbol();
        }
        if(foundExtraSymbol) {
            foundExtraSymbol = 0;
            error(10);
        }//////////////////////////////////////////////////////////////////

    }

    if(foundMainFunction == 0) {
        error(108);
    }

    if(symbol != EOFSY) {
        error(101);                          //program should end after main function
        skip(2);            //直接跳到文件结尾      skip直至文件结尾
    }
}

void constDeclaration()
{
    while(symbol == CONSTSY) {
        getSymbol();
        constDefinition();              //常量定义
        if (symbol == SEMICOLONSY) {     //如果常量定义后面是分号
//            printf("Line %d: This is a const declaration\n", lineCount);
            getSymbol();
        } else {
            error(102);                  // semicolon required
            skip(3);                    //只缺一个分号，不跳
        }
    }
}

void constDefinition()
{
    if(symbol == INTSY || symbol == CHARSY) {           //如果常量定义开始的位置是int或char
        int type = symbol;          //记录常量类型
        do {    //常量类型后，是1个或多个 标识符=整数/字符 ，这之间用逗号分隔
            getSymbol();
            if (symbol == IDSY) {            //如果int或char后是标识符，正确
                char name[TOKENMAXLENGTH];
                strcpy(name, token);                //记录标识符名称
                getSymbol();
                if (symbol == ASSIGNSY) {               //如果标识符后是=，正确
                    getSymbol();
                    //这时，如果前面的类型是int，那么这里应该是一个整数。如果前面的类型是char，那么这里应该是一个字符
                    if(type == INTSY) {
                        if(symbol == INTEGERSY) {    //如果类型是int，且读到一个无符号整数
                            enterTab(name, tab_top, CONST_TAB_VALUE, INT_TAB_VALUE, 4, num);
                            char numString[20];
                            numToString(numString, num);            //把num转为string类型，以便传入generate_mid_code
                            generate_mid_code(0, CONSTINT, name, numString, "");        //成功声明一个常量，生成中间代码
                            getSymbol();
                        }
                        else if(symbol == PLUSSY || symbol == MINUSSY) { //如果类型是int，且读到加法运算符(有符号整数)
                            int op = symbol;        //记录运算符
                            getSymbol();
                            if(symbol == INTEGERSY) {       //如果加法运算符后是无符号整数，正确
                                enterTab(name, tab_top, CONST_TAB_VALUE, INT_TAB_VALUE, 4, (op == PLUSSY) ? num : (-num));
                                char numString[20];
                                numToString(numString, (op == PLUSSY) ? num : (-num));  //把num转为string类型，以便传入generate_mid_code
                                generate_mid_code(0, CONSTINT, name, numString, "");        //成功声明一个常量，生成中间代码
                                getSymbol();
                            }
                            else {
                                error(106);              //如果加法运算符后不是无符号整数，即构不成有符号整数
                                skip(1);                //跳至下一个分号
                            }
                        }
                        else {              //等号后不是整数，类型不匹配
                            error(105);          //n == 15, mismatched type
                            skip(1);                    //跳至下一个分号
                        }
                    }
                    else if(type == CHARSY) {
                        if(symbol == CHARACTERSY) {         //如果类型是char，且读到一个字符
                            enterTab(name, tab_top, CONST_TAB_VALUE, CHAR_TAB_VALUE, 4, token[1]);     //根据词法分析程序，token[0]和token[2]为单引号，token[1]为字符值
                            char charString[5];
                            charString[0] = '\'';
                            charString[1] = token[1];
                            charString[2] = '\'';
                            charString[3] = 0;          //把字符转为string类型，以便传入generate_mid_code
                            generate_mid_code(0, CONSTCHAR, name, charString, "");  //成功声明一个常量，生成中间代码
                            getSymbol();
                        }
                        else {              //等号后不是字符，类型不匹配
                            error(105);          //n == 15, mismatched type
                            skip(1);                    //跳至下一个分号
                        }
                    }
                }
                else {                      //标识符后不是等号
                    error(104);              //n == 14, = required
                    skip(1);                 //跳至下一个分号
                }
            } else {                    //类型标识符后不是标识符
                error(100);             //n == 10, identifier required after int or char
                skip(1);                    //跳至下一个分号
            }
        } while(symbol == COMMASY);
    }
    else {                                  //const之后不是int或char
        error(103);                      //there should be int or char at the beginning of constDefinition(type identifier required)
        skip(1);                    //跳至下一个分号
    }
}

void variableDeclaration()
{
    while(1) {
        variableDefinition();                   //此前已经读过int或char，进入变量定义处理
        if (symbol == SEMICOLONSY) {         //如果变量定义后面是分号
//            printf("Line %d: This is a variable declaration\n", lineCount);
            getSymbol();
        } else {
            error(102);                  // semicolon required
            skip(3);                //只缺一个分号，不跳
        }
        if(symbol == INTSY || symbol == CHARSY) {
            int tempSymbol = symbol;
            int tempTotalCharCount = totalCharCount;    //由于还需读两个符号才能确定是否还是变量声明，所以记录当前状态
            int tempCharCount = charCount;
            int tempLineCount = lineCount;
            getSymbol();                                //再读一个符号，此符号应为identifier
            if (symbol == IDSY) {
                getSymbol();                            //再读一个符号
                if (symbol == LEFTPARENTHESESSY) {          //如果是左小括号，则不再是变量声明。恢复状态，中断循环
                    fseek(file, -(totalCharCount - tempTotalCharCount), SEEK_CUR);
                    symbol = tempSymbol;
                    totalCharCount = tempTotalCharCount;
                    charCount = tempCharCount;
                    lineCount = tempLineCount;
                    break;
                } else {                                    //如果不是左小括号，继续变量声明处理。恢复状态，以便进入变量定义处理
                    fseek(file, -(totalCharCount - tempTotalCharCount), SEEK_CUR);
                    symbol = tempSymbol;
                    totalCharCount = tempTotalCharCount;
                    charCount = tempCharCount;
                    lineCount = tempLineCount;
                }
            }
        }
        else {
            break;
        }
    }
}

void variableDefinition()
{
    if(symbol == INTSY || symbol == CHARSY) {
        int type = symbol;          //记录类型
        do {
            getSymbol();
            if (symbol == IDSY) {
                char name[TOKENMAXLENGTH];
                strcpy(name, token);        //记录变量名字
                getSymbol();                    //在变量名字后再读一个符号，这个符号可能是逗号（进入同一行的下一个变量声明），也可能是左中括号（数组声明）
                if(symbol == COMMASY) {
                    enterTab(name, tab_top, VAR_TAB_VALUE, (type == INTSY)?INT_TAB_VALUE:CHAR_TAB_VALUE, 4, WAITING_OPERATION);           //如果是逗号,则当前变量声明已经结束，登录符号表
                    generate_mid_code(0, (type == INTSY) ? VARINT : VARCHAR, name, "", "");     //生成中间代码
                }
                else if(symbol == LEFTBRACKETSY) {      //如果是左中括号，进入数组声明
                    getSymbol();
                    if(symbol == INTEGERSY) {       //如果左中括号后是无符号整数
                        enterTab(name, tab_top, ARRAY_TAB_VALUE, (type == INTSY) ? INT_TAB_VALUE : CHAR_TAB_VALUE, num, WAITING_OPERATION);
                        char numString[20];
                        numToString(numString, num);        //把数组长度转化为字符串
                        generate_mid_code(0, (type == INTSY) ? ARRAYINT : ARRAYCHAR, name, numString, "");//生成中间代码
                        getSymbol();
                        if (symbol == RIGHTBRACKETSY) {      //如果无符号整数后是右中括号，正确
                            getSymbol();
                        }
                        else {              //声明数组时缺右中括号       如果后面是逗号或分号，可以视为只缺中括号，不跳    如果不是，则跳至下一个分号
                            error(107);
                            if (symbol == COMMASY || symbol == SEMICOLONSY) {
                                skip(3);
                            } else {
                                skip(1);
                            }
                        }
                    }
                    else {              //如果声明数组时左中括号后不是无符号整数
                        error(106);
                        skip(1);                //跳至下一个分号
                    }
                }
                else {
                    //如果标识符后既不是'['也不是','  那么将当前变量登录符号表，然后直接返回到variableDeclaration()
                    enterTab(name, tab_top, VAR_TAB_VALUE, (type == INTSY)?INT_TAB_VALUE:CHAR_TAB_VALUE, 4, WAITING_OPERATION);
                    generate_mid_code(0, (type == INTSY) ? VARINT : VARCHAR, name, "", "");     //生成中间代码
                }
            }
            else {
                error(100);             //n == 10, identifier required after int or char
                skip(1);                //类型标识符后不是标识符，跳至下一个分号
            }
        } while(symbol == COMMASY);  //在用逗号分隔，进行一行多个声明时，循环
    }
    else {
        error(103);                      //type identifier required
        skip(1);                //如果不出意外的话，程序应该在读到类型标识符后才会进入本函数，也就不会走到这里
    }
}

void functionWithReturnValueDeclaration()  //进入此函数时，symbol里存的是类型标识符
{
    foundReturnInFunction = 0;
    int funcType = symbol;   //记录类型
    getSymbol();
    if(symbol == IDSY) {        //如果类型标识符后是标识符，正确
        char name[TOKENMAXLENGTH];
        strcpy(name, token);        //记录函数名
        getSymbol();
        if(symbol == LEFTPARENTHESESSY) {   //如果标识符后是(,正确
            getSymbol();
//            printf("Line %d: This is a function with return value declaration\n", lineCount);
            parameterTable(name, funcType);     //参数表处理,传入函数名，在parameterTable内部登录符号表
            if(symbol == RIGHTPARENTHESESSY) {      //如果参数表后是),正确
                getSymbol();
                functionBody:
                if(symbol == LEFTBIGPARENTHESESSY) {          //如果)后是{，正确
                    getSymbol();
                    compoundStatement();        //处理复合语句
                    if(foundReturnInFunction == 0) {
                        error(1000);        //语义错误，无需跳读
                    }
                    if(symbol == RIGHTBIGPARENTHESESSY) {       //如果复合语句后是},正确
                        generate_mid_code(0, RET_EXTRA, "", "", "");      //在函数最后加一个返回，如果函数有返回语句，则不会到达这里，不产生影响。如果函数没有返回语句，则通过这一句返回
                        getSymbol();        //有返回值函数定义处理结束，此时再读一个符号，即可返回
                    }
                    else {
                        error(113);
                        skip(8);
                    }
                }
                else {                      //)后不是{
                    error(111);         //'{' required
                    skip(6);
                    goto functionBody;
                }
            }
            else {      //如果参数表后不是')'
                error(110);
                skip(4);
                goto functionBody;      //匹配')'继续进行函数体处理
            }
        }
        else {
            error(109);    // '(' required
            skip(5);
            goto functionBody;
        }
    }
    else {              //如果类型标识符后不是标识符
        error(100);          //identifier required after type identifier
        if(symbol == LEFTPARENTHESESSY) {   //如果类型标识符后直接是小括号，我们认为源代码漏掉了函数名
            skip(4);
            goto functionBody;      //匹配')'，继续进行函数体处理
        }
        else {
            skip(5);
            goto functionBody;
        }
    }
}

void functionWithoutReturnValueDeclaration()
{
    foundReturnInFunction = 0;
    int funcType = symbol;   //记录类型
    getSymbol();
    if(symbol == IDSY) {        //如果类型标识符后是标识符，正确
        char name[TOKENMAXLENGTH];
        strcpy(name, token);        //记录函数名
        getSymbol();
        if(symbol == LEFTPARENTHESESSY) {   //如果标识符后是(,正确
            getSymbol();
//            printf("Line %d: This is a function without return value declaration\n", lineCount);
            parameterTable(name, funcType);     //参数表处理,传入函数名，在parameterTable内部登录符号表
            if(symbol == RIGHTPARENTHESESSY) {      //如果参数表后是),正确
                getSymbol();
                functionBody:
                if(symbol == LEFTBIGPARENTHESESSY) {          //如果)后是{，正确
                    getSymbol();
                    compoundStatement();        //处理复合语句
                    if(foundReturnInFunction) {
                        error(1001);        //语义错误，无需跳读
                    }
                    if(symbol == RIGHTBIGPARENTHESESSY) {       //如果复合语句后是},正确
                        generate_mid_code(0, RET_EXTRA, "", "", "");      //在函数最后加一个返回，如果函数有返回语句，则不会到达这里，不产生影响。如果函数没有返回语句，则通过这一句返回
                        getSymbol();        //无返回值函数定义处理结束，此时再读一个符号，即可返回
                    }
                    else {
                        error(113);
                        skip(8);
                    }
                }
                else {                      //)后不是{
                    error(111);         //'{' required
                    skip(6);
                    goto functionBody;
                }
            }
            else {      //如果参数表后不是')'
                error(110);
                skip(4);
                goto functionBody;
            }
        }
        else {
            error(109);    // '(' required
            skip(5);
            goto functionBody;
        }
    }
    else {              //如果类型标识符后不是标识符
        error(100);          //identifier required after type identifier
        if(symbol == LEFTPARENTHESESSY) {   //如果类型标识符后直接是小括号，我们认为源代码漏掉了函数名
            skip(4);
            goto functionBody;      //匹配')'，继续进行函数体处理
        }
        else {
            skip(5);
            goto functionBody;
        }
    }
}

void mainFunction()
{
    foundReturnInFunction = 0;
    getSymbol();
    if(symbol == MAINSY) {
        char name[TOKENMAXLENGTH];
        strcpy(name, token);        //记录函数名
        getSymbol();
        if(symbol == LEFTPARENTHESESSY) {       //如果main后是(,正确
            getSymbol();
//            printf("Line %d: This is main function declaration\n", lineCount);
            enterTab(name, WAITING_OPERATION, FUNC_TAB_VALUE, VOID_TAB_VALUE, 0, WAITING_OPERATION);
            generate_mid_code(1, FUNCVOID, name, "", "");
            if(symbol == RIGHTPARENTHESESSY) {          //如果'('后是')'，正确
                getSymbol();
                functionBody:
                if(symbol == LEFTBIGPARENTHESESSY) {        //如果')'后是'{', 正确
                    getSymbol();
                    compoundStatement();
                    generate_mid_code(0, RET_EXTRA, "", "", "");
                    if(foundReturnInFunction) {
                        error(1002);    //语义错误，无需跳读
                    }
                    if(symbol == RIGHTBIGPARENTHESESSY) {       //如果复合语句后是'}',正确
                        getSymbol();        //主函数处理结束，此时再读一个符号，即可返回
                    }
                    else {
                        error(113);
                        skip(8);
                    }
                }
                else {
                    error(111);
                    skip(6);
                    goto functionBody;
                }
            }
            else {
                error(110);
                skip(4);
                goto functionBody;
            }
        }
        else {                                  //如果main后不是(，
            error(109);
            skip(5);
            goto functionBody;
        }
    }
    else {      // 如果void后不是main（能来到这里说明void后也不是函数名）
        error(112);
        skip(5);
        goto functionBody;
    }
}

void parameterTable(char name[], int funcType)
{
    int parameterNum = 0;                       //记录参数个数
    int type[100];                              //用来记录所有参数的类型
    char paraName[100][TOKENMAXLENGTH];         //用来记录所有参数的名字

    if(symbol == INTSY || symbol == CHARSY) {
        int tempType = symbol;
        getSymbol();
        if(symbol == IDSY) {        //如果类型标识符后是标识符，正确
            type[parameterNum] = tempType;
            strcpy(paraName[parameterNum], token);          //记录参数数据
            parameterNum++;
            getSymbol();
        }
        else {                      //如果类型标识符后不是标识符
            error(100);
            skip(16);
        }
    }
    else {          //如果第一个符号不是int或char，那么认为参数表为空，将函数名登录符号表后, 直接返回
        enterTab(name, WAITING_OPERATION, FUNC_TAB_VALUE, (funcType == INTSY) ? INT_TAB_VALUE : ((funcType == CHARSY)?CHAR_TAB_VALUE:VOID_TAB_VALUE), parameterNum, WAITING_OPERATION);       //将函数名登录符号表
        generate_mid_code(1, (funcType == INTSY) ? FUNCINT : ((funcType == CHARSY) ? FUNCCHAR : FUNCVOID), name, "", "");
        return;
    }

    //此时读完了第一个参数，并且读了第一个参数后的一个字符，则循环判断是否有后续参数
    while(symbol == COMMASY) {
        getSymbol();
        if(symbol == INTSY || symbol == CHARSY) {
            int tempType = symbol;
            getSymbol();
            if(symbol == IDSY) {        //如果类型标识符后是标识符，正确
                type[parameterNum] = tempType;
                strcpy(paraName[parameterNum], token);          //记录参数数据
                parameterNum++;
                getSymbol();
            }
            else {                      //如果类型标识符后不是标识符
                error(100);
                skip(16);
            }
        }
        else {      //逗号后不是类型标识符
            error(103);
            skip(16);
        }
    }

    enterTab(name, WAITING_OPERATION, FUNC_TAB_VALUE, (funcType == INTSY) ? INT_TAB_VALUE : ((funcType == CHARSY)?CHAR_TAB_VALUE:VOID_TAB_VALUE), parameterNum, WAITING_OPERATION);       //将函数名登录符号表
    generate_mid_code(1, (funcType == INTSY) ? FUNCINT : ((funcType == CHARSY) ? FUNCCHAR : FUNCVOID), name, "", "");
    int i;
    for(i = 0; i<parameterNum; i++) {
        enterTab(paraName[i], tab_top, PARA_TAB_VALUE, (type[i] == INTSY)?INT_TAB_VALUE:CHAR_TAB_VALUE, 4, WAITING_OPERATION);
        generate_mid_code(0, (type[i] == INTSY) ? PARAINT : PARACHAR, paraName[i], "", "");
    }

}

void compoundStatement()
{
    if(symbol == CONSTSY) {
        constDeclaration();
    }

    if(symbol == INTSY || symbol == CHARSY) {
        variableDeclaration();
    }

    statementColumn();
}

void statementColumn()
{
    while(symbolInStatementStartSet()) {          //当当前symbol是<语句>的可能的开始symbol时，循环
        statement();
    }
}

void statement()
{
    if(symbol == IFSY) {
        ifStatement();
    }
    else if(symbol == WHILESY) {
        whileStatement();
    }
    else if(symbol == LEFTBIGPARENTHESESSY) {
        getSymbol();
        statementColumn();
        if(symbol == RIGHTBIGPARENTHESESSY) {
            getSymbol();
        }
        else {      //<语句>中语句列分支缺少最后的'}'
            error(113);
            skip(8);
        }
    }
    else if(symbol == SCANFSY) {
        scanfStatement();
        if(symbol == SEMICOLONSY) {
            getSymbol();
        }
        else {
            error(102);
            skip(3);
        }
    }
    else if(symbol == PRINTFSY) {
        printfStatement();
        if(symbol == SEMICOLONSY) {
            getSymbol();
        }
        else {
            error(102);
            skip(3);
        }
    }
    else if(symbol == SEMICOLONSY) {
        blankStatement();
    }
    else if(symbol == SWITCHSY) {
        switchStatement();
    }
    else if(symbol == RETURNSY) {
        returnStatement();
        if(symbol == SEMICOLONSY) {
            getSymbol();
        }
        else {
            error(102);
            skip(3);
        }
    }
    else if(symbol == IDSY) {       //赋值语句或函数调用语句
        char name[TOKENMAXLENGTH];
        int tempSymbol = symbol;
        int tempTotalCharCount = totalCharCount;
        int tempCharCount = charCount;
        int tempLineCount = lineCount;
        strcpy(name, token);            //记录变量名或函数名
        getSymbol();
        if(symbol == ASSIGNSY || symbol == LEFTBRACKETSY) {
            fseek(file, -(totalCharCount-tempTotalCharCount), SEEK_CUR);
            symbol = tempSymbol;
            totalCharCount = tempTotalCharCount;
            charCount = tempCharCount;
            lineCount = tempLineCount;
            assignStatement(name);
            if(symbol == SEMICOLONSY) {
                getSymbol();
            }
            else {
                error(102);
                skip(3);
            }
        }
        else if(symbol == LEFTPARENTHESESSY) {
            fseek(file, -(totalCharCount-tempTotalCharCount), SEEK_CUR);
            symbol = tempSymbol;
            totalCharCount = tempTotalCharCount;
            charCount = tempCharCount;
            lineCount = tempLineCount;
            functionCall(name, 0);
            if(symbol == SEMICOLONSY) {     //函数调用后是分号，正确
                getSymbol();
            }
            else {
                error(102);
                skip(3);
            }
        }
        else {
            error(117);
            skip(1);
        }
    }
    else {
        error(114);
        if(symbol == LEFTPARENTHESESSY) {
            skip(7);
        }
        else {
            skip(1);        //跳读至下一个分号
        }
    }
}

void ifStatement()
{
    struct conditionItem * x = (struct conditionItem*)malloc(sizeof(struct conditionItem));
    int tempMidIndex = 0;       //生成if语句的中间代码时暂时无法确定跳转位置，所以记录if语句中间代码的下标，留待后面补全
    getSymbol();
    if(symbol == LEFTPARENTHESESSY) {       //if后是'('，正确
        getSymbol();
        condition(x);
        generate_mid_code(0, calculateIfOp(x->op), "", x->src1, x->src2);       //生成if语句的中间代码，其中dst为跳转目标，留待后面补全
        tempMidIndex = mid_code_top;
        if(symbol == RIGHTPARENTHESESSY) {      //条件后是')'，正确
//            printf("Line %d: This is a if statement\n", lineCount);
            getSymbol();
            ifBody:
            statement();
            generate_mid_code(1, NO_OP, "", "", "");        //在if语句的语句部分后面生成一个label
            char numString[20];
            numToString(numString, labelNum);       //把labelNum转成数字
            strcpy(midCodeTable[tempMidIndex].dst, numString);      //把if语句要跳转到的位置填入中间代码表中
        }
        else {      //条件后没有')'
            error(110);
            skip(7);
        }
    }
    else {      //if后不是'('
        error(109);
        skip(10);
        if(symbol == LEFTBIGPARENTHESESSY) {
            goto ifBody;
        }
    }
}

int calculateIfOp(int op)
{
    if(op == LESSSY) {
        return IF_LESS;
    }
    else if(op == LESSEQUALSY) {
        return IF_LESSEQUAL;
    }
    else if(op == MORESY) {
        return IF_MORE;
    }
    else if(op == MOREEQUALSY) {
        return IF_MOREEQUAL;
    }
    else if(op == NOTEQUALSY) {
        return IF_NOTEQUAL;
    }
    else if(op == EQUALSY) {
        return IF_EQUAL;
    }
    return -1;
}

void whileStatement()
{

    struct conditionItem * x = (struct conditionItem*)malloc(sizeof(struct conditionItem));
    int tempMidIndex;        //生成while语句的中间代码时暂时无法确定跳转位置，所以记录if语句中间代码的下标，留待后面补全

    generate_mid_code(1, WHILE_START_OP, "", "", "");    //因为while需要多次判断条件，所以最后的goto语句需要跳转到计算条件之前
    int whileLabelNum;      //while最后需要无条件跳转回while一行，所以记录while一行的label
    whileLabelNum = labelNum;

    getSymbol();
    if(symbol == LEFTPARENTHESESSY) {       //if后是'('，正确
        getSymbol();
        condition(x);
        generate_mid_code(0, calculateWhileOp(x->op), "", x->src1, x->src2);
        tempMidIndex = mid_code_top;        //记录while这一行的中间代码下标
//        whileLabelNum = labelNum;           //记录while这一行的label
        if(symbol == RIGHTPARENTHESESSY) {      //条件后是')',正确
//            printf("Line %d: This is a while statement\n", lineCount);
            getSymbol();
            statement();
            char numString[20];
            numToString(numString, whileLabelNum);
            generate_mid_code(0, GOTO_OP, numString, "", "");       //在while结束处无条件跳转至while一行
            generate_mid_code(1, NO_OP, "", "", "");             //在while语句的语句部分后面生成一个label
            numToString(numString, labelNum);
            strcpy(midCodeTable[tempMidIndex].dst, numString);      //把while语句要跳转的位置填入中间代码表中
        }
        else {      //条件后没有')'
            error(110);
            skip(7);
        }
    }
    else {      //while后没有'('
        error(109);
        skip(10);
    }
}

int calculateWhileOp(int op)
{
    if(op == LESSSY) {
        return WHILE_LESS;
    }
    else if(op == LESSEQUALSY) {
        return WHILE_LESSEQUAL;
    }
    else if(op == MORESY) {
        return WHILE_MORE;
    }
    else if(op == MOREEQUALSY) {
        return WHILE_MOREEQUAL;
    }
    else if(op == NOTEQUALSY) {
        return WHILE_NOTEQUAL;
    }
    else if(op == EQUALSY) {
        return WHILE_EQUAL;
    }
    return -1;
}

void scanfStatement()
{
    getSymbol();
    if(symbol == LEFTPARENTHESESSY) {       //scanf后是'('，正确
        do {
            getSymbol();
            if(symbol == IDSY) {
                int i = loc(token);
                if(i == 0) {
                    error(1004);    //语义错误不跳
                }
                else if(tab[i].object != VAR_TAB_VALUE) {       //如果不是变量
                    error(1011);
                }
                generate_mid_code(0, SCANF_OP, token, "", "");      //为每一个输入生成一行中间代码
                getSymbol();
            }
            else {
                error(100);
                skip(7);
            }
        } while(symbol == COMMASY);
        if(symbol == RIGHTPARENTHESESSY) {
//            printf("Line %d: This is scanf statement\n", lineCount);
            getSymbol();
        }
        else {
            error(110);
            skip(17);
        }
    }
    else {              //scanf后不是'('
        error(109);
        skip(1);
    }
}

void printfStatement()
{
    char string[TOKENMAXLENGTH];
    char identifier[TOKENMAXLENGTH];
    struct item * x = (struct item*)malloc(sizeof(struct item));
    getSymbol();
    if(symbol == LEFTPARENTHESESSY) {       //printf后是'(',正确
        getSymbol();
        if(symbol == STRINGSY) {        //如果'('后是字符串
            strcpy(string, token);          //记录要输出的字符串
            getSymbol();
            if(symbol == COMMASY) {         //如果字符串后是','
                getSymbol();
                expression(x);
                strcpy(identifier, x->identifier);
                if(symbol == RIGHTPARENTHESESSY) {      //如果表达式后是')'
//                    printf("Line %d: This is a printf statement\n", lineCount);
                    generate_mid_code(0, PRINTF_OP1, "", string, identifier);
                    getSymbol();
                }
                else {                                  //如果表达式后不是')'
                    error(110);
                    skip(17);
                }
            }
            else if(symbol == RIGHTPARENTHESESSY) {     //如果字符串后是')'
//                printf("Line %d: This is a printf statement\n", lineCount);
                generate_mid_code(0, PRINTF_OP2, "", string, "");
                getSymbol();
            }
            else {          //如果字符串后不是','也不是')'
                error(110);
                skip(17);
            }
        }
        else {      //如果'('后不是字符串
            expression(x);
            strcpy(identifier, x->identifier);
            if(symbol == RIGHTPARENTHESESSY) {
//                printf("Line %d: This is a printf statement\n", lineCount);
                generate_mid_code(0, PRINTF_OP3, "", "", identifier);
                getSymbol();
            }
            else {
                error(110);
                skip(17);
            }
        }
    }
    else {              //printf后不是'('
        error(109);
        skip(1);
    }
}

void blankStatement()
{
//    printf("Line %d: This is a blank statement\n", lineCount);
    //空语句不生成中间代码
    getSymbol();
}

void switchStatement()
{
    struct item * x = (struct item*)malloc(sizeof(struct item));
    int endCaseIndex[1000];     //记录每个case结束时的中间代码下标
    int caseNum = 0;            //记录case的个数
    int lastCaseIndex = 0;      //记录上一个case的index
    getSymbol();
    if(symbol == LEFTPARENTHESESSY) {   //switch后是'('
        getSymbol();
        expression(x);
        if(symbol == RIGHTPARENTHESESSY) {      //表达式后是')'
            getSymbol();
            if(symbol == LEFTBIGPARENTHESESSY) {    //')'后是'{'
//                printf("Line %d: This is a switch statement\n", lineCount);
                getSymbol();
                situationTable(x, &lastCaseIndex, endCaseIndex, &caseNum);
                defaultSituation(x, &lastCaseIndex, endCaseIndex, &caseNum);
                if(symbol == RIGHTBIGPARENTHESESSY) {       //缺省后是'}'
                    //此时switch语句结束，生成一个label，用于每个case结束后跳转
                    generate_mid_code(1, NO_OP, "", "", "");
                    //并把这个label填回每个case最后的跳转语句
                    int i = 0;
                    char numString[20];
                    numToString(numString, labelNum);
                    while(i < caseNum) {
                        strcpy(midCodeTable[endCaseIndex[i]].dst, numString);
                        i++;
                    }
                    getSymbol();
                }
                else {          //缺省后不是 '}'
                    error(113);
                    skip(8);
                }
            }
            else {   //')'后不是'{'
                error(111);
                skip(11);
            }
        }
        else {      //表达式后不是')'
            error(110);
            skip(17);
        }
    }
    else {      //switch后不是'('
        error(109);
        skip(10);
    }
}

void returnStatement()
{
    struct item * x = (struct item*)malloc(sizeof(struct item));
    getSymbol();
    if(symbol == LEFTPARENTHESESSY) {
        getSymbol();
        expression(x);
        if(symbol == RIGHTPARENTHESESSY) {
//            printf("Line %d: This is a return statement\n", lineCount);
            foundReturnInFunction = 1;
            generate_mid_code(0, RET, "", x->identifier, "");
            getSymbol();
        }
        else {
            error(110);
            skip(17);
        }
    }
    else {  //如果return后不是'('
        generate_mid_code(0, RET, "", "", "");
//        printf("Line %d: This is a return statement\n", lineCount);
    }
}

void assignStatement(char name[])
{
    struct item * x = (struct item*)malloc(sizeof(struct item));
    struct item * y = (struct item*)malloc(sizeof(struct item));
    getSymbol();
    if(symbol == ASSIGNSY) {
        getSymbol();
        expression(x);
//        printf("Line %d: This is a assign statement\n", lineCount);
        int i = loc(name);      //在符号表中查找name
        if(i != 0) {
            if (tab[i].type == x->type &&
                (tab[i].object == VAR_TAB_VALUE || tab[i].object == PARA_TAB_VALUE)) {        //如果name和x的类型相同
                generate_mid_code(0, ASSIGN_OP, name, x->identifier, "");
            }
            else if (tab[i].object == CONST_TAB_VALUE) {
                error(1013);    //赋值给const
            }
            else {
                error(1003);    //类型不匹配， 语义错误不跳读
            }
        }
        else {
            error(1004);    //变量未声明，语义错误不跳读
        }
    }
    else if(symbol == LEFTBRACKETSY) {
        getSymbol();
        expression(x);      //x为数组下标
        int isPureNum = is_integer(x->identifier);     //检测数组下标是否是纯数字
        if(isPureNum) {     //如果x是纯数字
            int i = loc(name);
            int arrayLength = tab[i].size;      //查找数组长度
            if(arrayLength <= constToInt(x->identifier) || 0 > constToInt(x->identifier)) {     //检测数组下标是否越界
                error(1007);
            }
        }
        if(symbol == RIGHTBRACKETSY) {  //表达式后是']'
            getSymbol();
            if(symbol == ASSIGNSY) {
                getSymbol();
                expression(y);      //y为等号右边的内容
//                printf("Line %d: This is a assign statement\n", lineCount);
                int i = loc(name);
                if(tab[i].type == y->type && tab[i].object == ARRAY_TAB_VALUE) {
                    generate_mid_code(0, ARRAY_ASSIGN, name, x->identifier, y->identifier);
                }
                else {
                    error(1003);  //类型不匹配， 语义错误不跳读
                }
            }
            else {
                error(104);
                skip(1);
            }
        }
        else {      //表达式后不是']'
            error(107);
            skip(1);
        }
    }
    else {
        error(104);
        skip(1);
    }
}

void functionCall(char name[], int from)
{
    int i = loc(name);      //在tab表中查找该函数的位置
    if(i == 0 && from == 0) {   //from=0，从statment函数中过来，需要报未声明的错   from=1，从factor中过来，已经报过未声明的错
        error(1004);        //语义错误不跳
    }
//    int funcType = tab[i].type;     //记录该函数的返回值类型
    getSymbol();
    if(symbol == LEFTPARENTHESESSY) {       //如果标识符后是'('
        getSymbol();
        valueParameterTable(name);
        generate_mid_code(0, CALL, "", name, "");
        if(symbol == RIGHTPARENTHESESSY) {
//            if(funcType == INT_TAB_VALUE || funcType == CHAR_TAB_VALUE) {
//                printf("Line %d: This is a function with return value call\n", lineCount);
//            }
//            else if(funcType == VOID_TAB_VALUE) {
//                printf("Line %d: This is a function without return value call\n", lineCount);
//            }
//            else {
//                printf("Line %d: This may be a function call which is not declared, but we will not report the error in grammar analysis\n", lineCount);
//            }
            getSymbol();
        }
        else {
            error(110);
            skip(17);
        }
    }
    else {      //标识符后不是'(',,正常情况下读到'('才会进入functionCall，所以不会走到这
        error(109);
        skip(1);
    }
}

void condition(struct conditionItem * ret)
{
    struct item * x = (struct item*)malloc(sizeof(struct item));
    struct item * y = (struct item*)malloc(sizeof(struct item));
    expression(x);
    strcpy(ret->src1, x->identifier);
    if(symbol == LESSSY || symbol == LESSEQUALSY || symbol == MORESY || symbol == MOREEQUALSY || symbol == NOTEQUALSY || symbol == EQUALSY) {       //如果表达式后是关系运算符，那么还有一个表达式
        ret->op = symbol;
        getSymbol();
        expression(y);
        strcpy(ret->src2, y->identifier);
        if(x->type != y->type) {
            error(105);
        }
    }
    else {      //如果条件中没有第二部分，则视为src1 != 0
        ret->op = NOTEQUALSY;
        strcpy(ret->src2, "0");
        if(x->type != INT_ITEM_TYPE) {
            error(105);
        }
    }
//    printf("Line %d: This is a condition\n", lineCount);
    //如果表达式后不是关系运算符，直接返回
}

void situationTable(struct item*x, int*lastCaseIndex, int endCaseIndex[], int*caseNum)
{
    caseIndex = 0;
    while(symbol == CASESY) {
        situationSubStatement(x, lastCaseIndex, endCaseIndex, caseNum);
        generate_mid_code(0, GOTO_OP, "", "", "");
        endCaseIndex[*caseNum] = mid_code_top;      //记录每一个case最后的goto语句下标，用于后面补全跳转地址
        (*caseNum)++;   //case数目加一
    }
}

void situationSubStatement(struct item*x, int*lastCaseIndex, int endCaseIndex[], int*caseNum)
{
    char constantX[20];
    constant(constantX);
    recordCaseValue(constantX);     //记录case后常量的值以避免重复
    int constantType;                   //记录case后常量的类型
    if(is_digit(constantX[0])) {            //如果constant是int
        constantType = INT_ITEM_TYPE;
    }
    else {                                  //如果constant是char
        constantType = CHAR_ITEM_TYPE;
    }
    if(x->type != constantType) {
        error(105);
    }
    if(symbol == COLONSY) {
//        printf("Line %d: This is a situation sub statement\n", lineCount);
        generate_mid_code(1, CASE_OP, "", x->identifier, constantX);        //为case行生成中间代码(src1!=src2时跳转到dst,dst后面补全)
        if(*lastCaseIndex != 0) {
            char numString[20];
            numToString(numString, labelNum);       //把当前case的label转为string
            strcpy(midCodeTable[*lastCaseIndex].dst, numString);     //把当前case的label存入上一条case的跳转地址
        }
        *lastCaseIndex = mid_code_top;      //记录当前case下标（在后面作为上一条case下标使用）
        getSymbol();
        statement();
    }
    else {
        error(115);
        skip(15);
    }
}

void defaultSituation(struct item*x, int*lastCaseIndex, int endCaseIndex[], int*caseNum)
{
    if(symbol == DEFAULTSY) {
        getSymbol();
        if (symbol == COLONSY) {
//            printf("Line %d: This is a default situation\n", lineCount);
            generate_mid_code(1, DEFAULT_OP, "", "", "");       //为default行生成中间代码
            if(*lastCaseIndex != 0) {
                char numString[20];
                numToString(numString, labelNum);
                strcpy(midCodeTable[*lastCaseIndex].dst, numString);        //把default的label存入上一条case的跳转地址
            }
            getSymbol();
            statement();
            /////////////////////////////////////////
            generate_mid_code(0, GOTO_OP, "", "", "");
            endCaseIndex[*caseNum] = mid_code_top;      //记录每一个case最后的goto语句下标，用于后面补全跳转地址
            (*caseNum)++;   //case数目加一
            /////////////////////////////////////////
        } else {      //default后不是':'
            error(115);
            skip(15);
        }
    }
    else {
        generate_mid_code(1, DEFAULT_OP, "", "", "");   //没有default也为default行输出一个label，以便跳转
        if(*lastCaseIndex != 0) {
            char numString[20];
            numToString(numString, labelNum);
            strcpy(midCodeTable[*lastCaseIndex].dst, numString);        //把default的label存入上一条case的跳转地址
        }
        return;
    }
}

void constant(char constantX[])
{
    getSymbol();
    if(symbol == PLUSSY || symbol == MINUSSY) { //有符号整数
        int op = symbol;
        getSymbol();
        if(symbol == INTEGERSY) {
            numToString(constantX, (op == PLUSSY) ? num : (-num));  //把常量转成字符串存入constantX
            getSymbol();
        }
        else {
            error(106);
            skip(1);
        }
    }
    else if(symbol == INTEGERSY) {      //无符号整数
        numToString(constantX, num);    //把常量转成字符串存入constantX
        getSymbol();
    }
    else if(symbol == CHARACTERSY) {        //字符
        constantX[0] = '\'';
        constantX[1] = token[1];
        constantX[2] = '\'';
        constantX[3] = 0;           //把字符常量存入constantX
        getSymbol();
    }
    else {  //都不是
        error(116);
        skip(1);
    }
}

void valueParameterTable(char name[])   //把name传进来检查参数数量和类型是否冲突
{
    int i = loc(name);
    int paraNum = 0;
    struct item * x = (struct item*)malloc(sizeof(struct item));
    char tempVar[200][20];      //临时存储要push的参数
    if(symbolInExpression()) {
        expression(x);
        paraNum++;
        if(x->type != tab[i+paraNum].type) {        //如果表达式类型和函数的第paraNum个参数类型不符，报错
            error(1003);
        }
//        generate_mid_code(0, PUSH_OP, "", x->identifier, "");
        newTempVariable(tempVar[0]);
        generate_mid_code(0, ASSIGN_OP, tempVar[0], x->identifier, "");     //把要push的参数存在临时变量里
        struct item * tempItem = (struct item*)malloc(sizeof(struct item));
        tempItem->type = x->type;
        strcpy(tempItem->identifier, tempVar[0]);
        recordTempVarType(tempItem);                        //记录新申请的临时变量类型
        int j = 1;
        while(symbol == COMMASY) {
            getSymbol();
            expression(x);
            paraNum++;
            if(x->type != tab[i+paraNum].type) {
                error(1003);
            }
//            generate_mid_code(0, PUSH_OP, "", x->identifier, "");
            newTempVariable(tempVar[j]);
            generate_mid_code(0, ASSIGN_OP, tempVar[j], x->identifier, "");     //把要push的参数存在临时变量里
            tempItem->type = x->type;
            strcpy(tempItem->identifier, tempVar[j]);
            recordTempVarType(tempItem);                        //记录新申请的临时变量类型
            j++;
            if(j == 199) {
                error(121);     //参数过多
                skip(18);
            }
        }
        //把所有参数一次性全部push    （把存储着要push的参数的临时变量push出去）
        int k;
        for(k = 0; k<j; k++) {
            generate_mid_code(0, PUSH_OP, "", tempVar[k], "");
        }
    }
    else {
        //如果左括号后不是表达式，则视为无传入参数
    }
    if(paraNum > tab[i].size) {
        error(1005);    //参数过多
    }
    else if(paraNum < tab[i].size) {
        error(1006);    //参数过少
    }
}

void expression(struct item*x)
{
    struct item * y = (struct item*)malloc(sizeof(struct item));
    if(symbol == PLUSSY) {  //如果是+
        getSymbol();
        term(y);
        if(stringIsNum(y->identifier)) {        //如果term分析到的是纯数字
            strcpy(x->identifier, y->identifier);    //x = y
            x->type = INT_ITEM_TYPE;
        }
        else {
            char tempVar[20];
            newTempVariable(tempVar);       //生成一个临时变量
            generate_mid_code(0, PLUS_OP, tempVar, "0", y->identifier);  //tempVar = 0 + y;
            strcpy(x->identifier, tempVar);
            x->type = INT_ITEM_TYPE;
            recordTempVarType(x);
        }
    }
    else if(symbol == MINUSSY) {        //如果是-
        getSymbol();
        term(y);
        if(stringIsNum(y->identifier)) {        //如果term分析到的是纯数字
            sprintf(x->identifier, "-%s", y->identifier);   //x = -y
            x->type = INT_ITEM_TYPE;
        }
        else {
            char tempVar[20];
            newTempVariable(tempVar);       //生成一个临时变量
            generate_mid_code(0, MINUS_OP, tempVar, "0", y->identifier);  //tempVar = 0 + y;
            strcpy(x->identifier, tempVar);
            x->type = INT_ITEM_TYPE;
            recordTempVarType(x);
        }
    }
    else {
        term(y);
        strcpy(x->identifier, y->identifier);   //如果前面没有前置+-，则不必再运算，直接存入
        x->type = y->type;
    }
    //当term后是加法运算符时循环
    while(symbol == PLUSSY || symbol == MINUSSY) {
        int tempSymbol = symbol;
        getSymbol();
        term(y);
        char tempVar[20];
        newTempVariable(tempVar);
        if(tempSymbol == PLUSSY) {
            generate_mid_code(0, PLUS_OP, tempVar, x->identifier, y->identifier);
        }
        else if(tempSymbol == MINUSSY) {
            generate_mid_code(0, MINUS_OP, tempVar, x->identifier, y->identifier);
        }
        strcpy(x->identifier, tempVar);
        if((x->type == INT_ITEM_TYPE || y->type == INT_ITEM_TYPE)
           && (x->type != VOID_ITEM_TYPE) && (x->type != ARRAY_ITEM_TYPE)
           && (y->type != VOID_ITEM_TYPE) && (y->type != ARRAY_ITEM_TYPE)) { //如果xy中至少有一个是int且都不是void或array
            x->type = INT_ITEM_TYPE;                                         //则把结果置为int
        }
        else if(x->type == CHAR_ITEM_TYPE && y->type == CHAR_ITEM_TYPE) {   //如果xy都是char
            x->type = INT_ITEM_TYPE;                                       //则把结果置为int
        }
        else {
            //error,给type赋值的时候不报错，留待使用的时候报错
        }
        recordTempVarType(x);
    }
    recordTempVarType(x);       //记录表达式返回的（如果是临时变量的话）临时变量的类型//在mips中printf的时候要用到
//    printf("Line %d: This is a expression\n", lineCount);
}

void term(struct item*x)
{
    struct item * y = (struct item*)malloc(sizeof(struct item));
    factor(y);
    strcpy(x->identifier, y->identifier);
    x->type = y->type;
    recordTempVarType(x);
    while(symbol == MULTIPLYSY || symbol == DIVIDESY) {
        int tempSymbol = symbol;
        getSymbol();
        factor(y);
        char tempVar[20];
        newTempVariable(tempVar);
        if(tempSymbol == MULTIPLYSY) {
            generate_mid_code(0, MULT_OP, tempVar, x->identifier, y->identifier);
        }
        else if(tempSymbol == DIVIDESY) {
            generate_mid_code(0, DIV_OP, tempVar, x->identifier, y->identifier);
        }
        strcpy(x->identifier, tempVar);
        if((x->type == INT_ITEM_TYPE || y->type == INT_ITEM_TYPE)
           && (x->type != VOID_ITEM_TYPE) && (x->type != ARRAY_ITEM_TYPE)
           && (y->type != VOID_ITEM_TYPE) && (y->type != ARRAY_ITEM_TYPE)) { //如果xy中至少有一个是int且都不是void或array
            x->type = INT_ITEM_TYPE;                                         //则把结果置为int
        }
        else if(x->type == CHAR_ITEM_TYPE && y->type == CHAR_ITEM_TYPE) {   //如果xy都是char
            x->type = INT_ITEM_TYPE;                                       //则把结果置为int
        }
        else {
            //error,给type赋值的时候不报错，留待使用的时候报错
        }
        recordTempVarType(x);
    }
//    printf("Line %d: This is a term\n", lineCount);
}

void factor(struct item*x)
{
    struct item * y = (struct item*)malloc(sizeof(struct item));
    if(symbol == IDSY) {            //标识符（数组），或有返回值函数调用语句
        char name[TOKENMAXLENGTH];
        strcpy(name, token);
        int i = loc(name);
        if(i == 0) {
            error(1004);    //identifier没有声明
        }
        int tempSymbol = symbol;
        int tempTotalCharCount = totalCharCount;
        int tempCharCount = charCount;
        int tempLineCount = lineCount;
        getSymbol();            //标识符后的符号，可能是'['(数组), 或'('(有返回值函数调用语句),或其他
        if(symbol == LEFTPARENTHESESSY) {       //如果标识符后是'('，有返回值函数调用
            fseek(file, -(totalCharCount - tempTotalCharCount), SEEK_CUR);
            symbol = tempSymbol;
            totalCharCount = tempTotalCharCount;
            charCount = tempCharCount;
            lineCount = tempLineCount;      //退回至刚读完标识符的位置
            if(tab[i].object != FUNC_TAB_VALUE && i != 0) { //检查标识符是否是函数名(如果标识符未定义，在其他地方报错)
                error(1008);
            }
            functionCall(name, 1);             //进入函数调用
            char tempVar[20];
            newTempVariable(tempVar);
            generate_mid_code(0, ASSIGN_OP, tempVar, "$ret", "");     //存储函数返回值
            strcpy(x->identifier, tempVar);
            x->type = tab[i].type;  // 把返回的item的类型置为函数返回值类型
//            printf("Line %d: This is a factor\n", lineCount);
            recordTempVarType(x);
        }
        else if(symbol == LEFTBRACKETSY) {      //如果标识符后是'['，数组
            getSymbol();
            expression(y);
            int isPureNum = is_integer(y->identifier);     //检测数组下标是否是纯数字
            if(isPureNum) {     //如果y是纯数字
//                int i = loc(name);
                int arrayLength = tab[i].size;      //查找数组长度
                if(arrayLength <= constToInt(y->identifier) || 0 > constToInt(y->identifier)) {     //检测数组下标是否越界
                    error(1007);
                }
            }
            if(tab[i].object != ARRAY_TAB_VALUE && i != 0) { //检查标识符是否是数组名(如果标识符未定义，在其他地方报错)
                error(1009);
            }
            char tempVar[20];
            newTempVariable(tempVar);
            generate_mid_code(0, ARRAY_GET, tempVar, name, y->identifier);    //数组取值
            strcpy(x->identifier, tempVar);             //把取到的值存入x中
            x->type = tab[i].type;      //把返回的item的类型置为数组类型
            recordTempVarType(x);
            if(symbol == RIGHTBRACKETSY) {          //如果表达式后是']',正确
                getSymbol();
//                printf("Line %d: This is a factor\n", lineCount);
            }
            else {          //如果表达式后不是']',错误
                error(107);
                skip(1);
            }
        }
        else {      //如果标识符后是其他
//            printf("Line %d: This is a factor\n", lineCount);
            strcpy(x->identifier, name);    //把调用的变量存入x
            if(i != 0) {
                if (tab[i].object != CONST_TAB_VALUE && tab[i].object != VAR_TAB_VALUE && tab[i].object != PARA_TAB_VALUE) {//如果不是const和var
                    error(1010);
                    if (tab[i].object == ARRAY_TAB_VALUE) {
                        x->type = ARRAY_ITEM_TYPE;
                    } else if (tab[i].object == FUNC_TAB_VALUE) {
                        x->type = FUNC_ITEM_TYPE;
                    }
                }
                else{
                    x->type = tab[i].type;
                }
            }
        }
    }
    else if(symbol == LEFTPARENTHESESSY) {      // '('表达式')'
        getSymbol();
        expression(y);
        strcpy(x->identifier, y->identifier);   //把表达式结果存入x
        x->type = INT_ITEM_TYPE;
        if(symbol == RIGHTPARENTHESESSY) {      //如果表达式后是')',正确
//            printf("Line %d: This is a factor\n", lineCount);
            getSymbol();
        }
        else {          //如果表达式后不是')'
            error(110);
            skip(17);
        }
    }
    else if(symbol == PLUSSY || symbol == MINUSSY) {        //有符号整数
        int op = symbol;
        getSymbol();
        if(symbol == INTEGERSY) {
//            printf("Line %d: This is a factor\n", lineCount);
            char numString[20];
            numToString(numString, (op == PLUSSY) ? num : -num);        //把有符号整数转为字符串
            strcpy(x->identifier, numString);                           //并存入x
            x->type = INT_ITEM_TYPE;
            getSymbol();
        }
        else {
            error(118);
            skip(1);
        }
    }
    else if(symbol == INTEGERSY) {      //无符号整数
//        printf("Line %d: This is a factor\n", lineCount);
        char numString[20];
        numToString(numString, num);        //把无符号整数转为字符串
        strcpy(x->identifier, numString);   //并存入x
        x->type = INT_ITEM_TYPE;
        getSymbol();
    }
    else if(symbol == CHARACTERSY) {        //字符
//        printf("Line %d: This is a factor\n", lineCount);
        char charString[5];
        charString[0] = '\'';
        charString[1] = token[1];
        charString[2] = '\'';
        charString[3] = 0;
        strcpy(x->identifier, charString);  //把字符转为字符串并存入x
        x->type = CHAR_ITEM_TYPE;
        getSymbol();
    }
    else {      //error
        error(117);
        skip(1);
    }
}

void recordTempVarType(struct item*x)
{
    if(x->identifier[0] == '$') {   //如果是临时变量
        int i;
        int l = (int)strlen(x->identifier);
        char numString[20];
        for(i = 1; i<=l; i++) {
            numString[i-1] = x->identifier[i];  //把x->identifier中$后面的部分（即数字部分）复制到numString中
        }
        int j = stringToNum(numString);     //计算x->identifier的编号
        tempTab[j] = x->type;       //记录x的类型
    }
    //如果不是临时变量就不必记录了
}

void recordCaseValue(char constantX[])
{
    int i;
    int c = constToInt(constantX);
    for(i = 1; i<=caseIndex; i++) {
        if(caseValue[i] == c) {     //如果找到了一样的
            error(1012);
            return;
        }
    }
    //如果没找到一样的
    caseIndex++;
    caseValue[caseIndex] = c;
}

int symbolInStatementStartSet()
{
    if(symbol == IFSY ||
       symbol == WHILESY ||
       symbol == LEFTBIGPARENTHESESSY ||
       symbol == IDSY ||
       symbol == PRINTFSY ||
       symbol == SCANFSY ||
       symbol == SEMICOLONSY ||
       symbol == SWITCHSY ||
       symbol == RETURNSY
            ) {
        return 1;
    }
    return 0;
}

int symbolInExpression()
{
    if(symbol == PLUSSY ||
       symbol == MINUSSY ||
       symbol == IDSY ||
       symbol == LEFTPARENTHESESSY ||
       symbol == INTEGERSY ||
       symbol == CHARACTERSY
            ) {
        return 1;
    }
    else {
        return 0;
    }
}

void enterTab(char name[], int link, int object, int type, int size, int offset)
{
    if(tab_top == TAB_LENGTH - 1) {     //如果符号表已满，报错
        error(119);
        skip(2);
        return;
    }

    int i = loc1(name, object);
    if(i > 0) {         //在符号表中查找是否重复定义，如果找到了
        error(120);
        //重复声明，没有语法错误，不需跳读
    }
    else {                 //如果没找到
        tab_top++;      //首先把top加一，达到一个空位置
        strcpy(tab[tab_top].name, name);        //登录name
        tab[tab_top].object = object;           //登录object
        tab[tab_top].type = type;               //登录type
        tab[tab_top].size = size;               //登录size
        //登录link
        if(object == CONST_TAB_VALUE || object == VAR_TAB_VALUE || object == ARRAY_TAB_VALUE || object == PARA_TAB_VALUE) {
            tab[tab_top].link = tab_top - 1;
        }
        else {                      //如果要登录的是函数
            for(i = tab_top-1; i>globalTop; i--) {
                if(tab[i].object == FUNC_TAB_VALUE) {       //找到上一个函数，或找到最后一个全局量
                    break;
                }
            }
            tab[tab_top].link = i;
        }
        //登录offset
        if(object == CONST_TAB_VALUE) {
            tab[tab_top].offset = offset;
        }
        else if(object == FUNC_TAB_VALUE) {
            tab[tab_top].offset = 0;
        }
        else {          //如果要登录的是变量/参数/数组
            if(tab[tab_top-1].object == ARRAY_TAB_VALUE) {      //如果上一个是数组
                tab[tab_top].offset = tab[tab_top-1].offset + tab[tab_top-1].size * 4;
            }
            else if(tab[tab_top-1].object == CONST_TAB_VALUE || tab[tab_top-1].object == FUNC_TAB_VALUE) {     //如果上一个是常量或函数，那么在这之前直到上一个偏移为0的成分之前(函数/程序起始)没有声明过变量，则直接将偏移设为8
                tab[tab_top].offset = 8;        //函数起始时，返回地址占运行栈第一格，prevabp占运行栈第二格，变量从第三格开始存，所以offset为8
            }
            else {
                tab[tab_top].offset = tab[tab_top-1].offset + 4;
            }
        }
    }
}

int loc(char name[])
{
    int ptr = tab_top;
    strcpy(tab[0].name, name);
    while(ptr >= 0) {
        if(strcmp(name, tab[ptr].name) == 0) {
            break;
        }
        ptr = tab[ptr].link;
    }
    return ptr;
}

int loc1(char name[], int object)
{
    int ptr = tab_top;
    strcpy(tab[0].name, name);
    if(object == FUNC_TAB_VALUE) {      //如果要查找的是function
        for(ptr = tab_top; ptr > globalTop; ptr--) {
            if(tab[ptr].object == FUNC_TAB_VALUE) {     //先从上到下找到第一个function
                break;
            }
        }
        while(ptr >= 0) {
            if(strcmp(tab[ptr].name, name) == 0) {
                break;
            }
            ptr = tab[ptr].link;
        }
        return ptr;
    }
    else {      //如果不是function
        for(ptr = tab_top; ptr >= 0; ptr--) {
            if(tab[ptr].object == FUNC_TAB_VALUE) {     //如果找到了函数还没找到重名变量
                if(strcmp(name, tab[ptr].name) == 0) {      //如果变量名和它所属的函数名相同
                    return ptr;                                     //视为重复声明，返回ptr
                }
                return 0;                           //否则返回0，视为无重复声明
            }
            if(strcmp(tab[ptr].name, name) == 0) {      //如果找到了，返回ptr
                break;
            }
        }
        return ptr;
    }
}

void printTab()
{
    printf("\n\nThis is tab\n");
    int i;
    for(i = 1; i<=tab_top; i++) {
        printf("%-5d%-20s%-5d%-5d%-5d%-5d%-5d\n", i, tab[i].name, tab[i].link, tab[i].object, tab[i].type, tab[i].size, tab[i].offset);
    }
}




int constToInt(char constantX[])
{
    if(constantX[0] == '\'') {
        return (int)constantX[1];
    }
    else {
        return stringToNum(constantX);
    }
}

void newTempVariable(char dst[])
{
    tempVarNum++;
    char temp[20];
    numToString(temp, tempVarNum);
    dst[0] = '$';
    dst[1] = 0;
    strcat(dst, temp);
}

int stringIsNum(char id[])      //判断字符串是不是纯数字
{
    int i;
    int l = (int)strlen(id);
    for(i = 0; i<l; i++) {
        if(!(id[i]>='0' && id[i]<='9')) //如果id中有一位不是数字，返回0
            return 0;
    }
    return 1;       //如果id是纯数字，返回1
}

int is_integer(char id[])           //判断字符串是不是表示一个整数
{

    int i;
    int l = (int)strlen(id);
    if(id[0] == '+' || id[0] == '-') {
        i = 1;
    }
    else {
        i = 0;
    }
    for(; i<l; i++) {
        if(!(id[i]>='0' && id[i]<='9')) //如果id中有一位不是数字，返回0
            return 0;
    }
    return 1;       //如果id是纯数字，返回1
}

