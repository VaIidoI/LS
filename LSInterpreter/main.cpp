#include <unordered_set>
#include <functional>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>

using std::cout; using std::endl; using std::vector; using std::string; using std::to_string;

const std::unordered_set<char> delimiters = { ' ', '\t' };
const std::unordered_set<char> seperators = { ',', ':' };
const std::unordered_set<string> operators = { "==", "!=", "<" ,"<=" ,">" ,">=" };

string TrimWhitespace(const string& line)
{
    if (line.empty()) return line;

    const string tags = " \t\v\r\n";
    size_t start = line.find_first_not_of(tags);
    if (start == string::npos) return "";
    size_t end = line.find_last_not_of(tags);
    return (start == end) ? line.substr(start, 1) : line.substr(start, end - start + 1);
}

string RemoveWhitespace(const string& line) {
    string ret;
    
    for (const char& c : line)
        if (!isspace(c))
            ret += c;

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

        if (c == ';' && !isString && index != line.size()) {
            ret.back() += ';'; //wtf
            ret.push_back(string()); continue;
        }

        if (c == '"')
            isString = !isString;

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
    for (const char c : line) {
        ++index;

        //Edge case: No spaces between seperators
        if (seperators.find(ret.back().back()) != seperators.cend() && !isString)
            ret.push_back(string());

        //Edge case: Semicolons | Make sure semicolons are always the last token in a line
        if (c == ';' && !isString) {
            if (ret.back().empty())
                ret.back() += ";";
            else
                ret.push_back(";");

            if (index != line.size())
                ret.push_back(string());
            continue;
        }

        //Edge case: Quotes | Make sure strings are not affeced by the tokenizing process
        if (c == '"') {
            isString = !isString;
            //if (!ret.back().empty()) { ret.push_back("\""); continue; }
            ret.back() += "\""; continue;
        }

        if (delimiters.find(c) != delimiters.cend() && !isString) {
            if (ret.back().empty() || index == line.size()) continue;
            ret.push_back(""); continue;
        }

        if (seperators.find(c) != seperators.cend() && !isString) {
            if (!ret.back().empty()) ret.push_back(string()); 
            ret.back() += c; continue;
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

void ExitError(const string& error) noexcept {
    std::cerr << '\n' << error << endl;
    exit(-1);
}

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

void FormatString(string& s) {
    //First and last chars should be '"'.
    s = s.substr(1, s.size() - 2);
}

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

int main(int argc, char* argv[]) {
    if (argc < 1) {
        ExitError("Please specify a path to the file. ");
    }

    std::ifstream file("test.ls"); string line;

    if (!file.is_open()) {
        ExitError("Cannot locate or open file.");
    }

    //Firstly, get the lines and remove any whitespace, while ignoring empty lines. Also store the actual line.  
    vector<std::pair<int, string>> lines; int lineIndex = 0;
    while (getline(file, line)) {
        ++lineIndex;
        line = TrimWhitespace(line);
        if (line == string()) continue;
        lines.push_back({ lineIndex, line });
    }

    //Secondly, parse the lines and store them in a parsedLines vector.
    vector<std::pair<int, string>> parsedLines;
    for (const auto&[lineNum, l] : lines) {
        auto parsed = Parse(l);

        if (parsed.size() == 1 && parsed.back().empty())
            continue;

        //After seperating semicolons, trim them to get rid of any whitespace inbetween.
        for (const string& x : parsed)
            parsedLines.push_back({ lineNum, TrimWhitespace(x) });
    }
    lines.clear();

    //Predefine a map that stores all functions by their name, containing their implementation and number of arguments.
    std::map<string, std::pair<int, std::function<void(vector<string>)>>> funcMap;

    //Predefine the maps which store the variables based on their names.
    std::map<string, string> strings;
    std::map<string, double> doubles;
    std::map<string, int> ints;
    std::map<string, bool> bools;

    //Create a list of keywords, which cannot be the names of variables.
    vector<string> blacklist = { "string", "double", "int", "bool", "errorLevel" };

    //Create a map storing the label's location along with a vector storing the line "history"
    //Also create a lineIndex to keep track of the current line.
    std::map<string, int> labelMap; vector<int> callHistory; int parsedLineIndex = 0;

    //ErrorLevel is a flag that indicates if certain functions encountered any errors
    int errorLevel = 0;

    //Function that searches though every variable map and returns the value of a variable given its name.
    auto FindVar = [&](const string& varName) {
        if (varName == "errorLevel")
            return std::make_pair(to_string(errorLevel), (int)INT); //cursed

        string value = ""; int valueType = -1;

        if (strings.find(varName) != strings.end()) {
            value = '"' + strings.at(varName) + '"'; valueType = STRING;
        }
        if (doubles.find(varName) != doubles.end()) {
            value = to_string(doubles.at(varName)); valueType = DOUBLE;
        }
        if (ints.find(varName) != ints.end()) {
            value = to_string(ints.at(varName)); valueType = INT;
        }
        if (bools.find(varName) != bools.end()) {
            if (bools.at(varName))
                value = "true";
            else
                value = "false";
            valueType = BOOL;
        }

        return std::make_pair(value, valueType);
    };

    auto ResolveValue = [&](string& value, int& type) {
        type = GetDataType(value);

        if (type == ERROR) {
            auto found = FindVar(value);
            if (found.second == -1)
                throw std::exception(("Comapring undefined variable '" + value + "'").c_str());
            value = found.first; type = found.second;
        }
    };

    funcMap.emplace("printl", std::make_pair(1, [ResolveValue](vector<string> v) {
        string value = v[0]; int valueType = -1; ResolveValue(value, valueType);

        if (valueType == STRING)
            FormatString(value);

        cout << value << endl;
    }));

    funcMap.emplace("endl", std::make_pair(0, [ResolveValue](vector<string> v) {
        cout << endl;
    }));

    funcMap.emplace("cls", std::make_pair(0, [ResolveValue](vector<string> v) {
        //Istg this is the best way to do this
        cout << "\033[2J\033[1;1H" << endl;
    }));

    funcMap.emplace("print", std::make_pair(1, [ResolveValue](vector<string> v) {
        string value = v[0]; int valueType = -1; ResolveValue(value, valueType);

        if (valueType == STRING)
            FormatString(value);

        cout << value;
    }));

    funcMap.emplace("input", std::make_pair(1, [&](vector<string> v) {
        string name = v[0]; int nameType = -1;
        //Reset errorLevel to 0 
        errorLevel = 0;

        auto foundName = FindVar(name);
        if (foundName.second == -1)
            throw std::exception(("Input received undefined variable '" + name + "'").c_str());
        nameType = foundName.second;

        //if (nameType != STRING)
        //    throw std::exception(("Tried to read into variable of type '" + IntToType(nameType) + "'").c_str());

        //Get the line and its datatype. If it's errortype, it becomes a string, due to it not being anything else
        string s = ""; std::getline(std::cin, s); int lineType = GetDataType(s);

        if (lineType == ERROR)
            lineType = STRING;

        //Set errorLevel to 1, indicating a type mismatch, unless type is a string. 
        if (nameType != lineType && nameType != STRING)
            errorLevel = 1;

        //Save in the corresponding map
        if (errorLevel == 0) {
            switch (nameType) {
            case STRING:
                strings[name] = s;
            break;

            case DOUBLE:
                doubles[name] = stod(s);
            break;

            case INT:
                ints[name] = stoi(s);
            break;

            case BOOL:
                if (s == "true")
                    bools[name] = 1;
                else
                    bools[name] = 0;
                break;
            }
        }
    }));

    funcMap.emplace("exit", std::make_pair(1, [FindVar](vector<string> v) {
        string code = v[0]; int type = GetDataType(code);

        if (type == ERROR) {
            auto found = FindVar(code);

            if (found.second == -1)
                throw std::exception(("Exit received undefined variable '" + code + "'").c_str());

            code = found.first;
            type = found.second;
        }

        if (GetDataType(code) != INT) throw std::exception(("Exit requires argument type: 'int' got: '" + IntToType(type) + "'").c_str());
        cout << endl << "Program exited with code: " << to_string(stoi(code)) << endl;
        exit(stoi(code));
    }));

    funcMap.emplace("add", std::make_pair(2, [&](vector<string> v) {
        string name = v[0]; int nameType = -1;
        string value = v[1]; int valueType = GetDataType(value);

        auto foundName = FindVar(name);
        if (foundName.second == -1)
            throw std::exception(("Add received undefined variable '" + name + "'").c_str());
        nameType = foundName.second;

        //Value can also be a variable
        if (valueType == ERROR) {
            auto foundVar = FindVar(value);

            if (foundVar.second == -1)
                throw std::exception(("Add received undefined variable '" + value + "'").c_str());

            value = foundVar.first;
            valueType = foundVar.second;
        }

        if(nameType != DOUBLE && nameType != INT)
            throw std::exception(("Cannot perform arithmetic on variables with type'" + IntToType(nameType) + "'").c_str());
       
        if (valueType != DOUBLE && valueType != INT)
            throw std::exception(("Arithmetic operation received invalid type '" + IntToType(nameType) + "'").c_str());

        if (nameType == DOUBLE)
            doubles[name] += stod(value);
        if(nameType == INT) 
            ints[name] += stoi(value);
    }));

    funcMap.emplace("sub", std::make_pair(2, [&](vector<string> v) {
        string name = v[0]; int nameType = -1;
        string value = v[1]; int valueType = GetDataType(value);

        auto foundName = FindVar(name);
        if (foundName.second == -1)
            throw std::exception(("Subtract received undefined variable '" + name + "'").c_str());
        nameType = foundName.second;;

        //Value can also be a variable
        if (valueType == ERROR) {
            auto foundVar = FindVar(value);

            if (foundVar.second == -1)
                throw std::exception(("Subtract received undefined variable '" + value + "'").c_str());

            value = foundVar.first;
            valueType = foundVar.second;
        }

        if (nameType != DOUBLE && nameType != INT)
            throw std::exception(("Cannot perform arithmetic on variables with type'" + IntToType(nameType) + "'").c_str());

        if (valueType != DOUBLE && valueType != INT)
            throw std::exception(("Arithmetic operation received invalid type '" + IntToType(nameType) + "'").c_str());

        if (nameType == DOUBLE)
            doubles[name] -= stod(value);
        if (nameType == INT)
            ints[name] -= stoi(value);
    }));

    funcMap.emplace("multiply", std::make_pair(2, [&](vector<string> v) {
        string name = v[0]; int nameType = -1;
        string value = v[1]; int valueType = GetDataType(value);

        auto foundName = FindVar(name);
        if (foundName.second == -1)
            throw std::exception(("Multiply received undefined variable '" + name + "'").c_str());
        nameType = foundName.second;

        //Value can also be a variable
        if (valueType == ERROR) {
            auto foundVar = FindVar(value);

            if (foundVar.second == -1)
                throw std::exception(("Multiply received undefined variable '" + value + "'").c_str());

            value = foundVar.first;
            valueType = foundVar.second;
        }

        if (nameType != DOUBLE && nameType != INT)
            throw std::exception(("Cannot perform arithmetic on variables with type'" + IntToType(nameType) + "'").c_str());

        if (valueType != DOUBLE && valueType != INT)
            throw std::exception(("Arithmetic operation received invalid type '" + IntToType(nameType) + "'").c_str());

        if (nameType == DOUBLE)
            doubles[name] *= stod(value);
        if (nameType == INT)
            ints[name] *= stoi(value);
    }));

    funcMap.emplace("divide", std::make_pair(2, [&](vector<string> v) {
        string name = v[0]; int nameType = -1;
        string value = v[1]; int valueType = GetDataType(value);

        auto foundName = FindVar(name);
        if (foundName.second == -1)
            throw std::exception(("Divide received undefined variable '" + name + "'").c_str());
        nameType = foundName.second;

        //Value can also be a variable
        if (valueType == ERROR) {
            auto foundVar = FindVar(value);

            if (foundVar.second == -1)
                throw std::exception(("Divide received undefined variable '" + value + "'").c_str());

            value = foundVar.first;
            valueType = foundVar.second;
        }

        if (stod(value) == 0.0)
            throw std::exception("Division by 0 attempted");

        if (nameType != DOUBLE && nameType != INT)
            throw std::exception(("Cannot perform arithmetic on variables with type'" + IntToType(nameType) + "'").c_str());

        if (valueType != DOUBLE && valueType != INT)
            throw std::exception(("Arithmetic operation received invalid type '" + IntToType(nameType) + "'").c_str());

        if (nameType == DOUBLE)
            doubles[name] /= stod(value);
        if (nameType == INT)
            ints[name] /= stoi(value);
    }));

    funcMap.emplace("inc", std::make_pair(1, [&](vector<string> v) {
        string name = v[0]; string value = v[0]; int valueType = -1; ResolveValue(value, valueType);

        if (valueType != DOUBLE && valueType != INT)
            throw std::exception(("Cannot perform arithmetic on variables with type'" + IntToType(valueType) + "'").c_str());

        if (valueType == DOUBLE)
            doubles[name] += 1;
        if (valueType == INT)
            ints[name] += 1;
    }));

    funcMap.emplace("dec", std::make_pair(1, [&](vector<string> v) {
        string name = v[0]; string value = v[0]; int valueType = -1; ResolveValue(value, valueType);

        if (valueType != DOUBLE && valueType != INT)
            throw std::exception(("Cannot perform arithmetic on variables with type'" + IntToType(valueType) + "'").c_str());

        if (valueType == DOUBLE)
            doubles[name] -= 1;
        if (valueType == INT)
            ints[name] -= 1;
        }));

    funcMap.emplace("set", std::make_pair(2, [&](vector<string> v) {
        string name = v[0]; int nameType = -1;
        string value = v[1]; int valueType = GetDataType(value);

        auto foundName = FindVar(name);
        if (foundName.second == -1)
            throw std::exception(("Set received undefined variable '" + name + "'").c_str());
        nameType = foundName.second;

        //Value can also be a variable
        if (valueType == ERROR) {
            auto foundVar = FindVar(value);

            if (foundVar.second == -1)
                throw std::exception(("Set received undefined variable '" + value + "'").c_str());

            value = foundVar.first;
            valueType = foundVar.second;
        }

        if(nameType != valueType 
            && !(nameType == DOUBLE && valueType == INT)
            && !(nameType == INT && valueType == DOUBLE))
            throw std::exception(("Set received wrong type. Got:" + IntToType(valueType) + "' Expected: '" +  IntToType(nameType) + "'").c_str());

        switch (nameType) {
            case STRING:
                FormatString(value);
                strings[name] = value;
            break;

            case DOUBLE:
                doubles[name] = stod(value);
            break;

            case INT:
                ints[name] = stoi(value);
            break;

            case BOOL:
                if (value == "true")
                    bools[name] = 1;
                else
                    bools[name] = 0;
            break;
        }
    }));

    funcMap.emplace("delete", std::make_pair(1, [&](vector<string> v) {
        string name = v[0]; int nameType = -1;
        errorLevel = 0;

        auto foundName = FindVar(name);
        if (foundName.second == -1) {
            errorLevel = 1; return;
        }
            
        nameType = foundName.second;


        switch (nameType) {
            case STRING:
                strings.erase(name);
            break;

            case DOUBLE:
                doubles.erase(name);
            break;

            case INT:
                ints.erase(name);
            break;

            case BOOL:
                bools.erase(name);
            break;
        }
    }));

    funcMap.emplace("var", std::make_pair(3, [&](vector<string> v) {
        string type = v[0]; string name = v[1]; string value = v[2]; int valueType = GetDataType(value);

        //If the value is an errortype, perhaps it is a variable
        if (valueType == ERROR) {
            auto found = FindVar(value); 
            
            if(found.second == -1)
                throw std::exception(("Var received undefined variable '" + value + "'").c_str());

            value = found.first;
            valueType = found.second;
        }

        //Type must be one of these types. 
        if (type != "string" && type != "double" && type != "int" && type != "bool")
            throw std::exception(("Var received invalid type. Got: '" + type + "'").c_str());

        //Doubles and ints can be assigned interchangably. 
        //Type must match the actual type of the value
        if (type != IntToType(valueType)
            && !(type == "double" && valueType == INT)
            && !(type == "int" && valueType == DOUBLE))
            throw std::exception(("Var received mismatching types. Expected: '" + type + "' " + "Got: '" + IntToType(valueType) + "'").c_str());

        //Name should not contain any invalid characters or blacklisted names
        for (const char& c : name)
            if (!isalnum(c) && c != '_')
                throw std::exception("Var received a name with an invalid character");

        if(std::find(blacklist.cbegin(), blacklist.cend(), name) != blacklist.cend())
            throw std::exception(("Var received illegal variable name. " + string("Got: '") + name + "'").c_str());

        //Name should be unique
        auto foundName = FindVar(name);
        if (foundName.second != -1)
            throw std::exception(("Variable by the name of '" + name + "' already defined").c_str());

        switch (StringToType(type)) {
            case STRING:
                FormatString(value);
                strings[name] = value;
            break;

            case DOUBLE:
                doubles[name] = stod(value);
            break;

            case INT:
                ints[name] = stoi(value);
            break;

            case BOOL:
                if (value == "true")
                    bools[name] = 1;
                else
                    bools[name] = 0;
            break;
        }
    }));
    
    funcMap.emplace("if", std::make_pair(4, [&](vector<string> v) {
        string value1 = v[0]; int value1Type = 0;
        string op = v[1];
        string value2 = v[2]; int value2Type = 0;

        if (operators.find(op) == operators.cend())
            throw std::exception(("Invalid operator '" + op + "' used in if statement").c_str());

        //As you can compare both variables and literals, resolve them.
        ResolveValue(value1, value1Type); ResolveValue(value2, value2Type);

        //Make sure to not compare different types, unless double and int
        if (value1Type != value2Type
            && !(value1Type == DOUBLE && value2Type == INT)
            && !(value1Type == INT && value2Type == DOUBLE))
            throw std::exception(("Comparing different types. Type1: '" + IntToType(value1Type) + "' Type2: '" + IntToType(value2Type) + "'").c_str());

        //check for validity.
        if ((op == "==" && value1 == value2) ||
            (op == "!=" && value1 != value2)) {
            funcMap.at("jump").second(vector<string> { v[3] }); return;
        }
        //If operators are indeed that, but not true then return
        else if ((op == "==" || op == "!="))
            return;

        //greater than, etc cannot be used on non-number types
        if(value1Type == STRING || value1Type == BOOL)
            throw std::exception(("Cannot use relational operators on Type: '" + IntToType(value1Type) + "'").c_str());

        //Convert to doubles for comparison
        double value1d = stod(value1); double value2d = stod(value2);

        if ((op == "<" && value1d < value2d) ||
            (op == ">" && value1d > value2d) ||
            (op == ">=" && value1d >= value2d) ||
            (op == "<=" && value1d <= value2d)) {
            funcMap.at("jump").second(vector<string> { v[3] }); return;
        }
    }));

    funcMap.emplace("jump", std::make_pair(1, [&](vector<string> v) {
        string name = v[0]; int nameType = GetDataType(name);

        //As labels can only be ErrorTypes, check for that
        if (nameType != ERROR)
            throw std::exception(("Tried to jump to label of type '" + IntToType(nameType) + "'").c_str());

        if (labelMap.find(name) == labelMap.cend())
            throw std::exception(("Tried to jump to undefined label. Got: '" + name + "'").c_str());

        int newLine = labelMap[name];
        //Make sure we do not continiously push the same line.
        if (callHistory.size() == 0)
            callHistory.push_back(parsedLineIndex);
        if (newLine != callHistory.back())
            callHistory.push_back(parsedLineIndex);
        parsedLineIndex = newLine;
    }));

    funcMap.emplace("return", std::make_pair(0, [&](vector<string> v) {
        //Return is equivalent to exit if the callHistory is empty.
        if (callHistory.size() == 1)
            funcMap.at("exit").second(vector<string> { "0" });

        //Set current line to latest entry and remove the entry. 
        callHistory.pop_back(); parsedLineIndex = callHistory.back();
    }));
    
    //Append function names to the blacklist
    for (const auto& a : funcMap)
        blacklist.push_back(a.first);

    //Iterate through every line (for the 3rd time) and store the label line locations
    for (int i = 0; i < parsedLines.size(); i++) {
        if (parsedLines[i].second[0] != '=') continue;
        if(parsedLines[i].second.back() != ';') ExitError("Expected semicolon on label initialization. Got: '" + parsedLines[i].second + "'");
        string label = FormatLabel(parsedLines[i].second);

        if (label == string()) ExitError("Incorrect label initialization. Got: '" + parsedLines[i].second + "'");

        labelMap.emplace(label, i);
        //also push the label names to the blacklist
        blacklist.push_back(label);
    }

    //Thirdly, tokenize each line and go through the actual interpretation process. 
    for (parsedLineIndex; parsedLineIndex < parsedLines.size(); parsedLineIndex++) {
        int lineNum = parsedLines[parsedLineIndex].first; string l = parsedLines[parsedLineIndex].second;

        //Once the lineIndex reaches the latest call, remove that line from the history.
        if (callHistory.size() > 0 && callHistory.back() == parsedLineIndex)
            callHistory.pop_back();

        vector<string> tokens;

        //Skip labels
        if (l[0] == '=')
            continue;

        try {
            tokens = Tokenize(l);
        }
        catch (const std::exception& e) {
            ExitError(string(e.what()) + " on line " + to_string(lineNum));
        }

        //cout << "Line " << lineNum << " | " << l << endl;
        //for(int i = 0; i < tokens.size(); i++)
        //    cout << "\tToken " << i << ": " << tokens[i] << endl;

        //Check if semicolon is found
        if (l.back() != ';') {
            ExitError("Missing semicolon on line " + to_string(lineNum) + ".");
        }

        string funcName = tokens[0];
        //If token0 is not an function
        if (funcMap.find(funcName) == funcMap.cend())
            ExitError("Function expected, got: '" + funcName + "' on line " + to_string(lineNum));

        //Get the function implementation and argument count
        auto func = funcMap[funcName]; int argCount = func.first;

        //Token in 2nd place should always be a ':', except on functions with 0 arguments
        if (tokens[1] != ":" && argCount != 0)
            ExitError("Expected ':' got: '" + tokens[1] + "' on line " + to_string(lineNum));

        vector<string> args;

        //Parse the arguments, inbetween every argument a ',' is expected
        for (int i = 2; i <= argCount * 2; i++) {
            if (i % 2 == 0) {
                args.push_back(tokens[i]);
            }
            else if (tokens[i] != ",")
                ExitError("Expected ',' got '" + tokens[i] + "' on line " + to_string(lineNum));
        }

        //For every 1 argument, there is 1 seperator, adding the function name and 1 index gives us the theoretical position of the semicolon
        if (tokens[argCount * 2 + 1] != ";")
            ExitError("Expected ';' got '" + tokens[argCount * 2 + 1] + "' on line " + to_string(lineNum));

        //Call the function
        try {
            func.second(args);
        }
        catch (const std::exception& e) {

            ExitError(string(e.what()) + " on line " + to_string(lineNum));
        }
    }
   
    file.close();
    return 0;
}