#include <stdio.h>
#include <string.h>


#define TOKENMAXLENGTH 256       //token允许的最大长度
#define NUM_OF_RESERVED_WORD 13

#define EOFSY (-1)
#define ERRORSY 0
#define CONSTSY 1               //保留字
#define INTSY 2
#define CHARSY 3
#define VOIDSY 4
#define MAINSY 5
#define WHILESY 6
#define SWITCHSY 7
#define CASESY 8
#define DEFAULTSY 9
#define SCANFSY 10
#define PRINTFSY 11
#define RETURNSY 12
#define IFSY 13
#define IDSY 21                 //标识符
#define INTEGERSY 22            //无符号整数
#define CHARACTERSY 23          //字符
#define STRINGSY 24             //字符串
#define PLUSSY 31               // +
#define MINUSSY 32              // -
#define MULTIPLYSY 33           // *
#define DIVIDESY 34             // /
#define LESSSY 35               // <
#define LESSEQUALSY 36          // <=
#define MORESY 37               // >
#define MOREEQUALSY 38          // >=
#define NOTEQUALSY 39           // !=
#define EQUALSY 40              // ==
#define SEMICOLONSY 41          // ;
#define ASSIGNSY 42             // =
#define COMMASY 43              // ,
#define LEFTBRACKETSY 44        // [
#define RIGHTBRACKETSY 45       // ]
#define LEFTPARENTHESESSY 46    // (
#define RIGHTPARENTHESESSY 47   // )
#define LEFTBIGPARENTHESESSY 48 // {
#define RIGHTBIGPARENTHESESSY 49// }
#define COLONSY 50              // :



void getAChar();         //读取一个字符
void clearToken();                  //清空token
void catToken();                    //将ch置于token末尾
void retract();          //读指针后退一个字符
int searchReservedWord();           //查找保留字，如果token是保留字，返回保留字编号。如果不是，返回0
int transNum();                     //将token转为整型值
void error(int n);                       //错误处理
//void getOutputString(char symbolString[]);    //计算要输出哪一个代号

int isPlus();                   //检测ch是否是+
int isMinus();                  // -
int isMultiply();               // *
int isDivide();                 // /
int isLess();                   // <
int isMore();                   // >
int isNot();                    // !
int isEqual();                  // =
int isAlpha();                  // _ a..z A..Z
int isZero();                   // 0
int isNotZeroDigit();           // 1..9
int isDigit();                  // 0..9
int isApostrophe();             // '
int isDoubleQuotes();           // "
int isCharCanUsedInCharacter(); // +-*/ 字母 数字
int isCharCanUsedInString();    // 32,33,35..126 ascii character
int isSemicolon();              // ;
int isComma();                  // ,
int isLeftBracket();            // [
int isRightBracket();           // ]
int isLeftParentheses();        // (
int isRightParentheses();       // )
int isLeftBigParentheses();     // {
int isRightBigParentheses();    // }
int isColon();                  // :
int isSpace();                  // ' '
int isTab();                    // '\t'
int isNewLine();                // '\n'
int isEOF();                    // EOF

int getSymbol();                   // Lexical Analysis         return -1 when EOF, return 0 for normal, return greater than 0 for error code
