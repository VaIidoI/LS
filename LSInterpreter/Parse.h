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
        throw std::runtime_error("Missing closing quote");

    if (ret.back() == string())
        ret.pop_back();

    return ret;
}

// Removes the quotes from a string
void FormatString(string& s) {
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"') {
        s = s.substr(1, s.size() - 2);
    }
}

// Removes the quotes from a string (returns a new string)
string FormatString(std::string_view s) {
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"') {
        return string(s.substr(1, s.size() - 2));
    }
    return string(s); // Return as is if not properly quoted
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

