#pragma once
#ifndef PARSE_H
#define PARSE_H

#include <unordered_set>
#include <functional>
#include <algorithm>
#include <string>
#include <vector>
#include <map>

using std::vector; using std::string; using std::to_string;

// Brief explanation of operator types
// -1. Actual argument
//  0. Whitespace
//  1. ':' character
//  2. ',' character
//  3. Set character '='
//  4. Modification characters like '=' and '+=' character
//  5. Logical comparators like '==' and '>='
using OpTypes = vector<int>;
const enum OpType {
    ARG = 0,
    COLON = 1,
    COMMA = 2,
    NEG = 3,
    SET = 4,
    MOD = 5,
    LOGIC = 6,
    BRACK = 7,
    SHIFT = 8
};

const std::unordered_map<std::string, int> separators = {
    {":", 1}, {",", 2}, {"!", 3}, {"=", 4}, {"++", 5}, {"--", 5}, {"+=", 5},
    {"-=", 5}, {"*=", 5}, {"/=", 5}, {"%=", 5}, {"==", 6}, {"!=", 6}, {"<", 6},
    {">", 6}, {"<=", 6}, {">=", 6}, {"(", 7}, {")", 7}, {">>", 8}, {"<<", 8}
};

string IntToOpType(const int& type) {
    switch (type) {
    case 0:
        return "Argument";
    case 1:
        return ":";
    case 2:
        return ",";
    case 3:
        return "!";
    case 4:
        return "=";
    case 5:
        return "Modification character";
    case 6:
        return "Logical comparator";
    case 7:
        return "Brackets";
    case 8:
        return "Shifts";
    default:
        return "Unknown type";
    }
}


//Returns: Trimmed string | Trims trailing and leading whitespace from string
string TrimWhitespace(const string& line)
{
    if (line.empty()) return line;

    const string tags = " \t\v\r\n";
    size_t start = line.find_first_not_of(tags);
    if (start == string::npos) return "";
    size_t end = line.find_last_not_of(tags);
    return (start == end) ? line.substr(start, 1) : line.substr(start, end - start + 1);
}

vector<string> SplitString(const string& s, const char& delimiter) {
    vector<string> ret(1, string());
    for (const char& c : s) {
        if (c == delimiter) {
            ret.push_back(string()); continue;
        }
        ret.back() += c;
    }

    //Remove any lines that are whitespace
    for (int i = 0; i < ret.size(); i++)
        if (TrimWhitespace(ret[i]).empty())
            ret.erase(ret.begin() + i);
    return ret;
}

//Returns: Parsed line vector | Parse a line, making sure ; equals a new line, while checking if it isn't within quotes or in a comment. 
vector<string> Parse(const string& line) {
    vector<string> ret(1, string()); bool isString = false, isComment = false; int index = 0;
    for (const char& c : line) {
        ++index;

        //Anything after comment tag is ignored
        if (c == '#' && !isString)
            isComment = true;

        if (isComment)
            continue;

        if (c == '"')
            isString = !isString;

        if (c == ';' && !isString && index != line.size()) {
            ret.back() += ';'; //wtf
            ret.push_back(string()); continue;
        }

        ret.back() += c;
    }

    //Edge case where whitespace or a comment follows a semicolon
    if (std::all_of(ret.back().begin(), ret.back().end(), isspace))
        ret.pop_back();

    return ret;
}

//Returns: Tokens in line | Split the line based on an arbitrary amount of tokens. 
vector<string> Tokenize(const string& line) {
    vector<string> ret(1, string());
    int index = 0; bool isString = false, isComment = false;

    //Kinda scuffed, as adding a 3-wide seperator would break the algorithm but whatever
    for (int i = 0; i < line.size(); i++) {
        char c = line[i]; char c1 = line[i + 1];

        //Edge case: Semicolons | Make sure semicolons are always the last token in a line
        if (c == ';' && !isString) {
            if (ret.back().empty())
                ret.back() += ";";
            else
                ret.push_back(";");
            continue;
        }

        //Edge case: No spaces between seperators
        if (separators.find(ret.back()) != separators.cend() && !isString)
            ret.push_back(string());

        //Check for string start and end
        if (c == '"') {
            isString = !isString; ret.back() += "\""; continue;
        }

        //If any whitespace is found, not within a string and the last token is not empty, push a new token
        if (isspace(c) && !isString) {
            if (!ret.back().empty()) ret.push_back(string()); continue;
        }

        //If the current and next character form a seperator, push that back, also skip the next char
        if (separators.find(string(1, c) + c1) != separators.end() && !isString) {
            if (!ret.back().empty()) ret.push_back(string());
            ret.back() += (string(1, c) + c1); ++i; continue;
        }

        //If only the current character is a seperator, do the same
        if (separators.find(string(1, c)) != separators.end() && !isString) {
            if (!ret.back().empty()) ret.push_back(string());
        }

        ret.back() += c;
    }

    //In case a string was started but never ended with a quote.
    if (isString)
        throw std::exception("Missing closing quote");

    if (ret.back() == string())
        ret.pop_back();

    return ret;
}

//Removes the quotes from a string
string FormatString(string& s) {
    //First and last chars should be '"'.
    s = s.substr(1, s.size() - 2);
    return s;
}

//Removes a labels formatting
string FormatLabel(const string& label) {
    //First char should be '=', last char should be ';'. 
    //Everything in between follows the same ruleset as variable names
    string s = label.substr(1, label.size() - 2);
    for (const char& c : s) {
        if (!isalnum(c) && c != '_')
            return string();
    }

    return s;
}

#endif // !PARSE_H

