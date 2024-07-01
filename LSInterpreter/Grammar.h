#pragma once
#include <unordered_map>
#include <string>
#ifndef GRAMMAR_H
#define GRAMMAR_H

enum DataTypes {
    ERROR = 0,
    STRING = 1000,
    DOUBLE = 2000,
    INT = 3000,
    BOOL = 4000
};

const enum TokenType {
    ARG = 0,
    COLON = 1,
    SEMICOLON = 2,
    COMMA = 3,
    NEG = 4,
    SET = 5,
    MOD = 6,
    LOGIC = 7,
    O_PAREN = 8,
    C_PAREN = 9,
    LSHIFT = 10,
    RSHIFT = 11
};


const enum ControlStatements {
    IF = 0,
    ELSE,
    FOR,
    WHILE,
    BREAK,
    CONTINUE,
    FUNC,
    RETURN,
    END
};

// Brief explanation of token types
// -1. Actual argument
//  0. Whitespace
//  1. ':' character
//  2. ';' character
//  3. ',' character
//  4. Negation character '!' 
//  5. Set character '='
//  6. Modification characters like '++', '--', '+=', etc.
//  7. Logical comparators like '==' and '>='
//  8. Open parenthesis '('
//  9. Close parenthesis ')'
// 10. Left shift '<<'
// 11. Right shift '>>'
using TokenTypes = std::vector<int>;

const std::unordered_map<std::string, int> separators = {
    {":", 1}, {";", 2}, {",", 3}, {"!", 4}, {"=", 5}, {"++", 6}, {"--", 6}, {"+=", 6},
    {"-=", 6}, {"*=", 6}, {"%=", 6}, {"==", 7}, {"!=", 7}, {"<", 7},
    {">", 7}, {"<=", 7}, {">=", 7}, {"(", 8}, {")", 9}, {">>", 11}, {"<<", 10}
};

// Helper functions for conversions and such

std::string IntToType(const int& type) {
    switch (type) {
        case 0:
            return "ErrorType";
        case 1000:
            return "string";
        case 2000:
            return "double";
        case 3000:
            return "int";
        case 4000:
            return "bool";
        default:
            return "???";
    }
}

int GetDataType(const std::string& line) {
    if (line.empty())
        return ERROR;

    // Check for bool
    if (line == "true" || line == "false")
        return BOOL;

    // Check for string
    if (line.front() == '"' && line.back() == '"')
        return STRING;

    int minusCount = 0;
    int dotCount = 0;

    for (const char& c : line) {
        if (c == '-') {
            ++minusCount;
        }
        else if (c == '.') {
            ++dotCount;
        }
        else if (!isdigit(c)) {
            return ERROR;
        }
    }

    // Check for int
    if (dotCount == 0 && minusCount <= 1)
        return INT;

    // Check for double
    if (dotCount == 1 && minusCount <= 1)
        return DOUBLE;

    return ERROR;
}

std::string IntToTokenType(const int& type) {
    switch (type) {
    case ARG:
        return "Argument";
    case COLON:
        return ":";
    case SEMICOLON:
        return ";";
    case COMMA:
        return ",";
    case NEG:
        return "!";
    case SET:
        return "=";
    case MOD:
        return "Modification character";
    case LOGIC:
        return "Logical comparator";
    case O_PAREN:
        return "Open parenthesis";
    case C_PAREN:
        return "Close parenthesis";
    case LSHIFT:
        return "Left shift";
    case RSHIFT:
        return "Right shift";
    default:
        return "Unknown type";
    }
}




#endif // !GRAMMAR_H

