#pragma once
#ifndef DATATYPES_H
#define DATATYPES_H

#include <string>

using std::string;

enum DataTypes {
    ERROR = 0,
    STRING = 1000,
    DOUBLE = 2000,
    INT = 3000,
    BOOL = 4000
};

string IntToType(int type) {
    if (type == 0) return "ErrorType";
    if (type == 1000) return "string";
    if (type == 2000) return "double";
    if (type == 3000) return "int";
    if (type == 4000) return "bool";
    return "???";
}

int StringToType(const string& s) {
    if (s == "ErrorType") return 0;
    if (s == "string") return 1000;
    if (s == "double") return 2000;
    if (s == "int") return 3000;
    if (s == "bool") return 4000;
    return -1; // Indicating an unknown type
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


#endif // !DATA_TYPES
