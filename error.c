
#include <stdlib.h>
#include "error.h"
#include "lexical_analysis.h"

extern int lineCount;
extern int charCount;
extern int lastLineCount;
extern int lastCharCount;
extern int symbol;

int foundError = 0;

void error(int n)                       //错误处理
{
    foundError = 1;
    //n == 1, token is full
    if(n == 1) {
        printf("ERROR: Line %d, Column %d:\t\t\ttoo long identifier\n", lineCount, charCount);
    }

    //n == 2, illegal char, too much characters between the Apostrophes(')
    else if(n == 2) {
        printf("ERROR: Line %d, Column %d:\t\t\tIllegal char, too much characters between the Apostrophes\n", lineCount, charCount);
    }

    //n == 3, only ! without =
    else if(n == 3) {
        printf("ERROR: Line %d, Column %d:\t\t\t= required\n", lineCount, charCount);
    }

    //n == 4, unrecognized character
    else if(n == 4) {
        printf("ERROR: Line %d, Column %d:\t\t\tunrecognized character\n", lineCount, charCount);
    }

    //n == 5, illegal character in string
    else if(n == 5) {
        printf("ERROR: Line %d, Column %d:\t\t\tillegal character in string\n", lineCount, charCount);
    }

    //n == 6, illegal character in char
    else if(n == 6) {
        printf("ERROR: Line %d, Column %d:\t\t\tillegal character in char\n", lineCount, charCount);
    }

    //n == 7, unpaired apostrophe
    else if(n == 7) {
        printf("ERROR: Line %d, Column %d:\t\t\tunpaired apostrophe\n", lineCount, charCount);
    }

    //n == 8, too little characters between the Apostrophes(')
    else if(n == 8) {
        printf("ERROR: Line %d, Column %d:\t\t\tIllegal char, too little characters between the Apostrophes\n", lineCount, charCount);
    }

    //n == 9, unpaired double quotes
    else if(n == 9) {
        printf("ERROR: Line %d, Column %d:\t\t\tunpaired double quotes\n", lineCount, charCount);
    }

    //n == 10, extra symbol
    else if(n == 10) {
        printf("ERROR: Line %d, Column %d:\t\t\textra symbol\n", lastLineCount, lastCharCount);
    }


//对于语法分析之后的错误，要注意报错的位置，（读完一个符号之后发现不和语法，可能需要在这个符号之前报错）
    // n == 100, identifier required
    else if(n == 100) {
        printf("ERROR: Line %d, Column %d:\t\t\tidentifier required\n", lastLineCount, lastCharCount);
    }

    //n == 101, program should end after main function
    else if(n == 101) {
        printf("ERROR: Line %d, Column %d:\t\t\tToo much symbols after main function\n", lineCount, charCount);
    }

    //n == 102, ';' required
    else if(n == 102) {
        printf("ERROR: Line %d, Column %d:\t\t\t';' required\n", lastLineCount, lastCharCount);
    }

    //n == 103, type identifier required
    else if(n == 103) {
        printf("ERROR: Line %d, Column %d:\t\t\ttype identifier required\n", lastLineCount, lastCharCount);
    }

    //n == 104, '=' required
    else if(n == 104) {
        printf("ERROR: Line %d, Column %d:\t\t\t'=' required\n", lastLineCount, lastCharCount);
    }

    //n == 105, mismatched type
    else if(n == 105) {
        printf("ERROR: Line %d, Column %d:\t\t\tmismatched type\n", lineCount, charCount);
    }

    //n == 106, unsigned integer required;
    else if(n == 106) {
        printf("ERROR: Line %d, Column %d:\t\t\tunsigned integer required\n", lastLineCount, lastCharCount);
    }

    //n == 107, ']' required
    else if(n == 107) {
        printf("ERROR: Line %d, Column %d:\t\t\t']' required\n", lastLineCount, lastCharCount);
    }

    //n == 108, no main function
    else if(n == 108) {
        printf("ERROR:\t\t\tno main function\n");
    }

    //n == 109, '(' required
    else if(n == 109) {
        printf("ERROR: Line %d, Column %d:\t\t\t'(' required\n", lastLineCount, lastCharCount);
    }

    //n == 110, ')' required
    else if(n == 110) {
        printf("ERROR: Line %d, Column %d:\t\t\t')' required\n", lastLineCount, lastCharCount);
    }

    //n == 111, '{' required
    else if(n == 111) {
        printf("ERROR: Line %d, Column %d:\t\t\t'{' required\n", lastLineCount, lastCharCount);
    }

    //n == 112, function name required
    else if(n == 112) {
        printf("ERROR: Line %d, Column %d:\t\t\tfunction name required\n", lastLineCount, lastCharCount);
    }

    //n == 113, '}' required
    else if(n == 113) {
        printf("ERROR: Line %d, Column %d:\t\t\t'}' required\n", lastLineCount, lastCharCount);
    }

    //n == 114, symbol do not belong to the FIRST set of statement
    else if(n == 114) {
        printf("ERROR: Line %d, Column %d:\t\t\tsymbol do not belong to the FIRST set of statement\n", lineCount, charCount);
    }

    //n == 115, ':' required
    else if(n == 115) {
        printf("ERROR: Line %d, Column %d:\t\t\t':' required\n", lastLineCount, lastCharCount);
    }

    //n == 116, constant required
    else if(n == 116) {
        printf("ERROR: Line %d, Column %d:\t\t\tconstant required\n", lastLineCount, lastCharCount);
    }

    //n == 117, illegal symbol
    else if(n == 117) {
        printf("ERROR: Line %d, Column %d:\t\t\tillegal symbol\n", lineCount, charCount);
    }

    //n == 118, integer required
    else if(n == 118) {
        printf("ERROR: Line %d, Column %d:\t\t\tinteger required\n", lastLineCount, lastCharCount);
    }

    //n == 119, tab full
    else if(n == 119) {
        printf("Tab full\n");
    }

    //n == 120, identifier redefined
    else if(n == 120) {
        printf("ERROR: Line %d, Column %d:\t\t\tidentifier redefined\n", lineCount, charCount);
    }

    //n == 121, The number of parameters exceeds the range allowed by the compiler
    else if(n == 121) {
        printf("ERROR: Line %d, Column %d:\t\t\tThe number of parameters exceeds the range allowed by the compiler\n", lineCount, charCount);
    }

//1000之后为语义分析错误
    //n == 1000, found no return in function with return value
    else if(n == 1000) {
        printf("ERROR: Line %d, Column %d:\t\t\tfound no return in function with return value\n", lineCount, charCount);
    }

    //n == 1001, found return in function without return value
    else if(n == 1001) {
        printf("ERROR: Line %d, Column %d:\t\t\tfound return in function without return value\n", lineCount, charCount);
    }

    //n == 1002, found return in main function
    else if(n == 1002) {
        printf("ERROR: Line %d, Column %d:\t\t\tfound return in main function\n", lineCount, charCount);
    }

    //n == 1003, mismatched type
    else if(n == 1003) {
        printf("ERROR: Line %d, Column %d:\t\t\tmismatched type\n", lineCount, charCount);
    }

    //n == 1004, identifier not declared
    else if(n == 1004) {
        printf("ERROR: Line %d, Column %d:\t\t\tidentifier not declared\n", lineCount, charCount);
    }

    //n == 1005, too much parameters
    else if(n == 1005) {
        printf("ERROR: Line %d, Column %d:\t\t\ttoo much parameters\n", lineCount, charCount);
    }

    //n == 1006, too little parameters
    else if(n == 1006) {
        printf("ERROR: Line %d, Column %d:\t\t\ttoo little parameters\n", lineCount, charCount);
    }

    //n == 1007, array index out of bounds
    else if(n == 1007) {
        printf("ERROR: Line %d, Column %d:\t\t\tarray index out of bounds\n", lineCount, charCount);
    }

    //n == 1008, grammar is function call but the identifier is not a function
    else if(n == 1008) {
        printf("ERROR: Line %d, Column %d:\t\t\tgrammar is function call but the identifier is not a function\n", lineCount, charCount);
    }

    //n == 1009, grammar is array use but the identifier is not an array
    else if(n == 1009) {
        printf("ERROR: Line %d, Column %d:\t\t\tgrammar is array use but the identifier is not an array\n", lineCount, charCount);
    }

    //n == 1010, grammar is constant or variable use but the identifier is not a constant or variable
    else if(n == 1010) {
        printf("ERROR: Line %d, Column %d:\t\t\tgrammar is constant or variable use but the identifier is not a constant or variable\n", lineCount, charCount);
    }

    //n == 1011, the identifier going to scanf is not a variable
    else if(n == 1011) {
        printf("ERROR: Line %d, Column %d:\t\t\tthe identifier going to scanf is not a variable\n", lineCount, charCount);
    }

    //n == 1012, repeated case value
    else if(n == 1012) {
        printf("ERROR: Line %d, Column %d:\t\t\trepeated case value\n", lineCount, charCount);
    }

    //n == 1013, assign to const
    else if(n == 1013) {
        printf("ERROR: Line %d, Column %d:\t\t\tassign to const\n", lineCount, charCount);
    }
}

void skip(int n)
{
    switch (n) {
        case 1:
            skip1(n);
            break;
        case 2:
            skip2(n);
            break;
        case 3:
            skip3(n);
            break;
        case 4:
            skip4(n);
            break;
        case 5:
            skip5(n);
            break;
        case 6:
            skip6(n);
            break;
        case 7:
            skip7(n);
            break;
        case 8:
            skip8(n);
            break;
        case 9:
            skip9(n);
            break;
        case 10:
            skip10(n);
            break;
        case 11:
            skip11(n);
            break;
        case 12:
            skip12(n);
            break;
        case 13:
            skip13(n);
            break;
        case 14:
            skip14(n);
            break;
        case 15:
            skip15(n);
            break;
        case 16:
            skip16(n);
            break;
        case 17:
            skip17(n);
            break;
        case 18:
            skip18(n);
            break;
        default:
            break;
    }
}

void skip1(int n)
{
    while(symbol != SEMICOLONSY) {
        if(symbol == EOFSY) {
            break;
        }
        getSymbol();
    }
}

void skip2(int n)
{
    while(symbol != EOFSY) {
        getSymbol();
    }
}

void skip3(int n)
{

}

void skip4(int n)
{
    while(symbol != RIGHTPARENTHESESSY) {
        if(symbol == EOFSY) {
            break;
        }
        getSymbol();
    }
    getSymbol();
}

void skip5(int n)
{
    while(1) {
        if(symbol == EOFSY) {
            break;
        }
        getSymbol();
        if(symbol == LEFTPARENTHESESSY) {
            skip(4);
            return;
        }
        else if(symbol == LEFTBIGPARENTHESESSY) {
            return;
        }
    }
}

void skip6(int n)
{
    while(1) {
        if(symbol == EOFSY) {
            break;
        }
        getSymbol();
        if(symbol == LEFTBIGPARENTHESESSY) {
            return;
        }
    }
}

void skip7(int n)
{
    while(symbol != RIGHTPARENTHESESSY) {
        if(symbol == EOFSY) {
            break;
        }
        getSymbol();
    }
}

void skip8(int n)
{
    while(symbol != RIGHTBIGPARENTHESESSY) {
        if(symbol == EOFSY) {
            break;
        }
        getSymbol();
    }
    getSymbol();
}

void skip9(int n)
{
    while(symbol !=RIGHTBRACKETSY) {
        if(symbol == EOFSY) {
            break;
        }
        getSymbol();
    }
}

void skip10(int n)
{
    while(1) {
        if(symbol == EOFSY) {
            break;
        }
        getSymbol();
        if(symbol == LEFTPARENTHESESSY) {
            skip(17);
            break;
        }
        else if(symbol == LEFTBIGPARENTHESESSY) {
            skip(8);
            break;
        }
        else if(symbol == SEMICOLONSY) {
            getSymbol();
            break;
        }
    }
}

void skip11(int n)
{
    while(1) {
        if(symbol == EOFSY) {
            break;
        }
        getSymbol();
        if(symbol == SEMICOLONSY) {
            getSymbol();
            break;
        }
        else if(symbol == LEFTBIGPARENTHESESSY) {
            skip(8);
            break;
        }
    }
}

void skip12(int n)
{

}

void skip13(int n)
{

}

void skip14(int n)
{
    while (symbol != INTSY && symbol != CHARSY && symbol != VOIDSY) {
        if(symbol == EOFSY) {
            break;
        }
        getSymbol();
    }
}

void skip15(int n)
{
    while(symbol != SEMICOLONSY) {
        if(symbol == EOFSY) {
            break;
        }
        getSymbol();
    }
    getSymbol();
}

void skip16(int n)
{
    while(symbol != COMMASY && symbol != RIGHTPARENTHESESSY) {
        if(symbol == EOFSY) {
            break;
        }
        getSymbol();
    }
}

void skip17(int n)
{
    while(symbol != RIGHTPARENTHESESSY) {
        if(symbol == EOFSY) {
            break;
        }
        getSymbol();
    }
    getSymbol();
}

void skip18(int n)
{
    while(symbol != RIGHTPARENTHESESSY && symbol != SEMICOLONSY) {
        if(symbol == EOFSY) {
            break;
        }
        getSymbol();
    }
}