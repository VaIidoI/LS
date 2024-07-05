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

const enum TokenType {
    ARG = 0,
    COLON = 100,
    SEMICOLON = 200,
    COMMA = 300,
    NEG = 400,
    O_PAREN = 500,
    C_PAREN = 600,
    O_CURLY = 700,
    C_CURLY = 800,
    LSHIFT = 900,
    RSHIFT = 1000,
    SET = 1100,
    MOD = 1200,
    LOGIC = 1300
};

// Brief explanation of token types
//  0.  Actual argument (ARG)
// 100. ':' character (COLON)
// 200. ';' character (SEMICOLON)
// 300. ',' character (COMMA)
// 400. Negation character '!' (NEG)
// 500. Open parenthesis '(' (O_PAREN)
// 600. Close parenthesis ')' (C_PAREN)
// 700. Open curly bracket '{' (O_CURLY)
// 800. Close curly bracket '}' (C_CURLY)
// 900. Left shift '<<' (LSHIFT)
// 1000. Right shift '>>' (RSHIFT)
// 1100. Set character '=' (SET)
// 1200. Modification characters like '++', '--', '+=', etc. (MOD)
// 1300. Logical comparators like '==' and '>=' (LOGIC)
using TokenTypes = std::vector<int>;

const std::unordered_map<std::string, int> separators = {
    {":", COLON}, {";", SEMICOLON}, {",", COMMA}, {"!", NEG},
    {"=", SET}, {"++", MOD}, {"--", MOD}, {"+=", MOD},
    {"-=", MOD}, {"*=", MOD}, {"/=", MOD}, {"%=", MOD},
    {"==", LOGIC}, {"!=", LOGIC}, {"<", LOGIC}, {">", LOGIC},
    {"<=", LOGIC}, {">=", LOGIC}, {"(", O_PAREN}, {")", C_PAREN},
    {"{", O_CURLY}, {"}", C_CURLY}, {">>", RSHIFT}, {"<<", LSHIFT}
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
    case O_PAREN:
        return "Open parenthesis";
    case C_PAREN:
        return "Close parenthesis";
    case O_CURLY:
        return "Open curly bracket";
    case C_CURLY:
        return "Close curly bracket";
    case LSHIFT:
        return "Left shift";
    case RSHIFT:
        return "Right shift";
    case SET:
        return "=";
    case MOD:
        return "Modification character";
    case LOGIC:
        return "Logical comparator";
    default:
        return "Unknown type";
    }
}

#endif // !GRAMMAR_H

