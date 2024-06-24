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

int GetDataType(const string& line) {
    //Check for bool
    if (line == "true" || line == "false")
        return BOOL;

    //Check for int, if index is on last character, it means all characters are numbers
    for (int i = 0; i <= line.size(); i++) {
        if (i == line.size()) return INT;
        if (!isdigit(line[i]))  break;
    }

    //Check for double, if index is on last character, it means all characters are numbers or a dot
    int dotCount = 0;
    for (int i = 0; i <= line.size(); i++) {
        if (i == line.size() && dotCount == 1) return DOUBLE;
        if (line[i] == '.') { ++dotCount; continue; }
        if (!isdigit(line[i]))  break;
    }

    if (line[0] == '"' && line.back() == '"')
        return STRING;

    return 0;
}

#endif // !DATA_TYPES
