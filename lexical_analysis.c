#include "lexical_analysis.h"
#include "error.h"

FILE * file;

int tooLong = 0;                        //标记catToken时是否过长
int lineCount = 1;                      //存储读的行数
int charCount = 0;                      //存储当前行读的字符数
int lastLineCount;                      //存储当次getSymbol之前读的行数
int lastCharCount;                      //存储当次getSymbol之前读的字符数
int totalCharCount = 0;                 //记录读到的总的字符数
int symbol = 0;                         //存储读到的符号类型代码
char token[TOKENMAXLENGTH];             //存放读到的符号的字符串
int num;                                //存放当前读入的整型数值
char ch = ' ';                          //存放当前读入的字符
//int symbolCount = 0;                    //已读取的符号个数
char reservedWord[NUM_OF_RESERVED_WORD][20] = {           //存储所有保留字
        "const",
        "int",
        "char",
        "void",
        "main",
        "while",
        "switch",
        "case",
        "default",
        "scanf",
        "printf",
        "return",
        "if"
};



//如果在getSymbol中遇到了error，则symbol为0，此时在语法分析中会同时symbol与期望的不匹配而认为发生了语法错误（可以接受）
int getSymbol()                   // Lexical Analysis         return -1 when EOF, return 0 for normal, return greater than 0 for error code
{
    lastLineCount = lineCount;
    lastCharCount = charCount;
    tooLong = 0;
    clearToken();
    while(isSpace() || isTab() || isNewLine()) {
        getAChar();
    }
    if(isEOF()) {
        symbol = EOFSY;
        return -1;
    }
    if(isAlpha()) {                                     //如果读到了一个字母(标识符或保留字)
        while(isAlpha() || isDigit()) {                 //读字符直到不是字母或数字
            catToken();
            getAChar();
        }
        retract();
        if(tooLong) {
            error(1);
            symbol = ERRORSY;
            tooLong = 0;
            return 1;              //return error code 1
        }
        int reservedWordNumber = searchReservedWord();   //查找保留字编号，如果找到
        if(reservedWordNumber == 0) {                   //如果没找到保留字，则是自定义标识符
            symbol = IDSY;
        }
        else {
            symbol = reservedWordNumber;
        }
    }
    else if(isZero()) {                                 //  0 无符号整数
        catToken();
        num = transNum();
        symbol = INTEGERSY;
    }
    else if(isNotZeroDigit()) {                         //首个数字非零无符号整数
        while(isDigit()) {
            catToken();
            getAChar();
        }
        retract();
        if(tooLong) {
            error(1);
            symbol = ERRORSY;
            tooLong = 0;
            return 1;              //return error code 1
        }
        num = transNum();
        symbol = INTEGERSY;
    }
    else if(isApostrophe()) {                           //字符
        do {
            catToken();
            getAChar();
        } while (!isApostrophe() && !isNewLine() && !isEOF());
        catToken();                                 //把最后的单引号也填入token，(或者是导致读取结束的换行符或EOF)
        if (tooLong) {
            error(1);
            symbol = ERRORSY;
            tooLong = 0;
            return 1;              //return error code 1
        }
        if(isNewLine() || isEOF()) {                   //如果读字符时以newLine结尾(或已读到文件结尾)，即如果单引号在同一行内不成对
            token[strlen(token) - 1] = '\'';
            error(7);
            symbol = ERRORSY;
            return 7;
        }
        if (strlen(token) > 3) {                     //非法的字符型值，一对单引号中间超过了一个字符
            error(2);
            symbol = ERRORSY;
            return 2;              //return error code 2
        }
        if(strlen(token) < 3) {                 //非法的字符型值，一对单引号中间少于一个字符
            error(8);
            symbol = ERRORSY;
            return 8;
        }
        int i;
        int findIllegalCharacter = 0;               //标记字符中是否出现不应该出现的字符
        for (i = 1; i < strlen(token) - 1; i++) {        //在字符中查找不应该出现的字符
            ch = token[i];
            if (!isCharCanUsedInCharacter()) {
                findIllegalCharacter = 1;
                break;
            }
        }
        if (findIllegalCharacter) {                  //如果找到了
            error(6);
            symbol = ERRORSY;
            return 6;
        }
        symbol = CHARACTERSY;
    }
    else if(isDoubleQuotes()) {                         //字符串
        do {
            catToken();
            if(ch == '\\')
                catToken();
            getAChar();
        } while(!isDoubleQuotes() && !isNewLine() && !isEOF());
        catToken();                                 //把最后的双引号也填入token，(或者是导致读取结束的换行符或EOF)
        if(tooLong) {
            error(1);
            symbol = ERRORSY;
            tooLong = 0;
            return 1;              //return error code 1
        }
        if(isNewLine() || isEOF()) {                   //如果读字符时以newLine结尾(或已读到文件结尾)，即如果双引号在同一行内不成对
            token[strlen(token) - 1] = '\"';
            error(9);
            symbol = ERRORSY;
            return 9;
        }
        int i;
        int findIllegalCharacter = 0;               //标记字符串中是否出现不应该出现的字符
        for(i = 1; i<strlen(token)-1; i++) {        //在字符串中查找不应该出现的字符
            ch = token[i];
            if(!isCharCanUsedInString()) {
                findIllegalCharacter = 1;
                break;
            }
        }
        if(findIllegalCharacter) {                  //如果找到了
            error(5);
            symbol = ERRORSY;
            return 5;
        }
        symbol = STRINGSY;
    }
    else if(isPlus()) {                                 // +
        symbol = PLUSSY;
        catToken();
    }
    else if(isMinus()) {                                // -
        symbol = MINUSSY;
        catToken();
    }
    else if(isMultiply()) {                             // *
        symbol = MULTIPLYSY;
        catToken();
    }
    else if(isDivide()) {                               // /
        symbol = DIVIDESY;
        catToken();
    }
    else if(isLess()) {                                 // <
        catToken();
        getAChar();
        if(isEqual()) {                                         // <=
            symbol = LESSEQUALSY;
            catToken();
        }
        else {                                                  // <
            symbol = LESSSY;
            retract();
        }
    }
    else if(isMore()) {                                 // >
        catToken();
        getAChar();
        if(isEqual()) {                                         // >=
            symbol = MOREEQUALSY;
            catToken();
        }
        else {                                                  // >
            symbol = MORESY;
            retract();
        }
    }
    else if(isNot()) {                                  // !
        catToken();
        getAChar();
        if(isEqual()) {                                         // !=
            symbol = NOTEQUALSY;
            catToken();
        }
        else {                                          //only ! error
            error(3);
            symbol = ERRORSY;
            retract();
            return 3;
        }
    }
    else if(isEqual()) {                                // =
        catToken();
        getAChar();
        if(isEqual()) {                                         // ==
            symbol = EQUALSY;
            catToken();
        }
        else {                                                  // =
            symbol = ASSIGNSY;
            retract();
        }
    }
    else if(isSemicolon()) {                            // ;
        symbol = SEMICOLONSY;
        catToken();
    }
    else if(isComma()) {                                // ,
        symbol = COMMASY;
        catToken();
    }
    else if(isLeftBracket()) {                          // [
        symbol = LEFTBRACKETSY;
        catToken();
    }
    else if(isRightBracket()) {                         // ]
        symbol = RIGHTBRACKETSY;
        catToken();
    }
    else if(isLeftParentheses()) {                      // (
        symbol = LEFTPARENTHESESSY;
        catToken();
    }
    else if(isRightParentheses()) {                     // )
        symbol = RIGHTPARENTHESESSY;
        catToken();
    }
    else if(isLeftBigParentheses()) {                   // {
        symbol = LEFTBIGPARENTHESESSY;
        catToken();
    }
    else if(isRightBigParentheses()) {                  // }
        symbol = RIGHTBIGPARENTHESESSY;
        catToken();
    }
    else if(isColon()) {                                // :
        symbol = COLONSY;
        catToken();
    }
    else {
        error(4);               //Unrecognized character
        symbol = ERRORSY;
        return 4;
    }
    return 0;
}

void getAChar()          //读取一个字符
{
    ch = (char)fgetc(file);
    totalCharCount++;
    if(ch == '\n') {
        lineCount++;
        charCount = 0;
    }
    else {
        if(ch == '\t') {
            charCount += 4;
        }
        else {
            charCount++;
        }
    }
}

void clearToken()                  //清空token
{
    int i;
    for(i = 0; i<256; i++) {
        token[i] = 0;
    }
    ch = ' ';
}

void catToken()                    //将ch置于token末尾
{
    int l = (int)strlen(token);
    if(l == TOKENMAXLENGTH-1) {
        tooLong = 1;
        do {                                        //如果token已满，跳过当前一连串的字符（过长的标识符 || 字符串 || 非法的拥有多个字符的字符类型值）
            getAChar();
        } while(isAlpha() || isDigit() /* || isDoubleQuotes() || isApostrophe() */);
        /* 这里不读取 字符串最后的双引号 和 字符最后的单引号，用于留作在getSymbol中跳出循环的出口。
         * 而在标识符分支中，直接通过非字母数字即可跳出循环。
         * 跳出循环后，进行一次是否过长的判断，以决定是否进行正常的symbol赋值过程
         */
        retract();
        return;
    }
    token[l] = ch;
    token[l+1] = 0;
}

void retract()          //读指针后退一个字符
{
    fseek(file, -1, SEEK_CUR);
    charCount--;
    totalCharCount--;
}

int searchReservedWord()           //查找保留字，如果token是保留字，返回保留字编号。如果不是，返回0
{
    int i;
    for(i = 0; i<NUM_OF_RESERVED_WORD; i++) {
        if(strcmp(token, reservedWord[i]) ==0) {
            return i+1;
        }
    }
    return 0;
}

int transNum()                     //将token转为整型值
{
    int l = (int)strlen(token);
    int i;
    int sum = 0;
    for(i = 0; i<l; i++) {
        sum *= 10;
        sum += token[i] - '0';
    }
    num = sum;
    return sum;
}


//void getOutputString(char symbolString[])    //计算要输出哪一个代号
//{
//    if(symbol == CONSTSY) {
//        strcpy(symbolString, "CONSTSY");
//    } else if(symbol == INTSY) {
//        strcpy(symbolString, "INTSY");
//    } else if(symbol == CHARSY) {
//        strcpy(symbolString, "CHARSY");
//    } else if(symbol == VOIDSY) {
//        strcpy(symbolString, "VOIDSY");
//    } else if(symbol == MAINSY) {
//        strcpy(symbolString, "MAINSY");
//    } else if(symbol == WHILESY) {
//        strcpy(symbolString, "WHILESY");
//    } else if(symbol == SWITCHSY) {
//        strcpy(symbolString, "SWITCHSY");
//    } else if(symbol == CASESY) {
//        strcpy(symbolString, "CASESY");
//    } else if(symbol == DEFAULTSY) {
//        strcpy(symbolString, "DEFAULTSY");
//    } else if(symbol == SCANFSY) {
//        strcpy(symbolString, "SCANFSY");
//    } else if(symbol == PRINTFSY) {
//        strcpy(symbolString, "PRINTFSY");
//    } else if(symbol == RETURNSY) {
//        strcpy(symbolString, "RETURNSY");
//    } else if(symbol == IDSY) {
//        strcpy(symbolString, "IDSY");
//    } else if(symbol == INTEGERSY) {
//        strcpy(symbolString, "INTEGERSY");
//    } else if(symbol == CHARACTERSY) {
//        strcpy(symbolString, "CHARACTERSY");
//    } else if(symbol == STRINGSY) {
//        strcpy(symbolString, "STRINGSY");
//    } else if(symbol == PLUSSY) {
//        strcpy(symbolString, "PLUSSY");
//    } else if(symbol == MINUSSY) {
//        strcpy(symbolString, "MINUSSY");
//    } else if(symbol == MULTIPLYSY) {
//        strcpy(symbolString, "MULTIPLYSY");
//    } else if(symbol == DIVIDESY) {
//        strcpy(symbolString, "DIVIDESY");
//    } else if(symbol == LESSSY) {
//        strcpy(symbolString, "LESSSY");
//    } else if(symbol == LESSEQUALSY) {
//        strcpy(symbolString, "LESSEQUALSY");
//    } else if(symbol == MORESY) {
//        strcpy(symbolString, "MORESY");
//    } else if(symbol == MOREEQUALSY) {
//        strcpy(symbolString, "MOREEQUALSY");
//    } else if(symbol == NOTEQUALSY) {
//        strcpy(symbolString, "NOTEQUALSY");
//    } else if(symbol == EQUALSY) {
//        strcpy(symbolString, "EQUALSY");
//    } else if(symbol == SEMICOLONSY) {
//        strcpy(symbolString, "SEMICOLONSY");
//    } else if(symbol == ASSIGNSY) {
//        strcpy(symbolString, "ASSIGNSY");
//    } else if(symbol == COMMASY) {
//        strcpy(symbolString, "COMMASY");
//    } else if(symbol == LEFTBRACKETSY) {
//        strcpy(symbolString, "LEFTBRACKETSY");
//    } else if(symbol == RIGHTBRACKETSY) {
//        strcpy(symbolString, "RIGHTBRACKETSY");
//    } else if(symbol == LEFTPARENTHESESSY) {
//        strcpy(symbolString, "LEFTPARENTHESESSY");
//    } else if(symbol == RIGHTPARENTHESESSY) {
//        strcpy(symbolString, "RIGHTPARENTHESESSY");
//    } else if(symbol == LEFTBIGPARENTHESESSY) {
//        strcpy(symbolString, "LEFTBIGPARENTHESESSY");
//    } else if(symbol == RIGHTBIGPARENTHESESSY) {
//        strcpy(symbolString, "RIGHTBIGPARENTHESESSY");
//    } else if(symbol == COLONSY) {
//        strcpy(symbolString, "COLONSY");
//    }
//}


int isPlus()                   //检测ch是否是+
{
    if(ch == '+')
        return 1;
    return 0;
}

int isMinus()                  // -
{
    if(ch == '-')
        return 1;
    return 0;
}

int isMultiply()               // *
{
    if(ch == '*')
        return 1;
    return 0;
}

int isDivide()                 // /
{
    if(ch == '/')
        return 1;
    return 0;
}

int isLess()                   // <
{
    if(ch == '<')
        return 1;
    return 0;
}

int isMore()                   // >
{
    if(ch == '>')
        return 1;
    return 0;
}

int isNot()                    // !
{
    if(ch == '!')
        return 1;
    return 0;
}

int isEqual()                  // =
{
    if(ch == '=')
        return 1;
    return 0;
}

int isAlpha()                  // _ a..z A..Z
{
    if((ch == '_') || ((ch >= 'a') && (ch <= 'z')) || ((ch >= 'A') && (ch <= 'Z')))
        return 1;
    return 0;
}

int isZero()                   // 0
{
    if(ch == '0')
        return 1;
    return 0;
}

int isNotZeroDigit()           // 1..9
{
    if((ch >= '1') && (ch <= '9'))
        return 1;
    return 0;
}

int isDigit()                  // 0..9
{
    if(isZero() || isNotZeroDigit())
        return 1;
    return 0;
}

int isApostrophe()             // '
{
    if(ch == '\'')
        return 1;
    return 0;
}

int isDoubleQuotes()           // "
{
    if(ch == '\"')
        return 1;
    return 0;
}

int isCharCanUsedInCharacter() //+-*/ 字母 数字
{
    if(isPlus() || isMinus() || isMultiply() || isDivide() || isAlpha() || isZero() || isNotZeroDigit())
        return 1;
    return 0;
}

int isCharCanUsedInString()    // 32,33,35..126 ascii character
{
    if((ch == 32) || (ch == 33) || ((ch >= 35) && (ch <= 126)))
        return 1;
    return 0;
}

int isSemicolon()              // ;
{
    if(ch == ';')
        return 1;
    return 0;
}

int isComma()                  // ,
{
    if(ch == ',')
        return 1;
    return 0;
}

int isLeftBracket()            // [
{
    if(ch == '[')
        return 1;
    return 0;
}

int isRightBracket()           // ]
{
    if(ch == ']')
        return 1;
    return 0;
}

int isLeftParentheses()        // (
{
    if(ch == '(')
        return 1;
    return 0;
}

int isRightParentheses()       // )
{
    if(ch == ')')
        return 1;
    return 0;
}

int isLeftBigParentheses()     // {
{
    if(ch == '{')
        return 1;
    return 0;
}

int isRightBigParentheses()    // }
{
    if(ch == '}')
        return 1;
    return 0;
}

int isColon()                  // :
{
    if(ch == ':')
        return 1;
    return 0;
}

int isSpace()                  // ' '
{
    if(ch == ' ')
        return 1;
    return 0;
}

int isTab()                    // '\t'
{
    if(ch == '\t')
        return 1;
    return 0;
}

int isNewLine()                // '\n'
{
    if(ch == '\n' || ch == '\r')
        return 1;
    return 0;
}

int isEOF() {                  // EOF
    if(ch == EOF)
        return 1;
    return 0;
}