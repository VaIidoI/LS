#include "DataTypes.h"
#include <iostream>
#include <fstream>
#include "Parse.h"

using std::cout; using std::endl; using std::to_string; using std::pair; using std::make_pair;

void ExitError(const string& error) noexcept {
    std::cerr << '\n' << error << endl;
    exit(-1);
}

//A function consists of a name, arguments with specific types and an implementation
class Func {
public:
    Func() : name_(""), types_(OpTypes{}), implementation_() {}

    Func(const std::string& name, const OpTypes& types, const std::function<void(std::vector<std::string>)>& imp)
        : name_(name), types_(types), implementation_(imp) {}

    //Getters
    std::string GetName() const {
        return name_;
    }

    OpTypes GetTypes() const {
        return types_;
    }

    std::function<void(std::vector<std::string>)> GetImplementation() const {
        return implementation_;
    }

    //Setters
    void SetName(const std::string& name) {
        name_ = name;
    }

    void SetTypes(const OpTypes& types) {
        types_ = types;
    }

    void SetImplementation(const std::function<void(std::vector<std::string>)>& imp) {
        implementation_ = imp;
    }

    //Function to execute the implementation
    void Execute(const std::vector<std::string>& args) const {
        if (implementation_) {
            implementation_(args);
        }
        else {
            std::cerr << "No implementation defined." << std::endl;
        }
    }

private:
    std::string name_;
    OpTypes types_;
    std::function<void(std::vector<std::string>)> implementation_;
};

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

    //Predefine a map that stores all functions by their name, containing their implementation and their argument types.
    std::map<string, std::pair<OpTypes, std::function<void(vector<string>)>>> funcMap;
    //Predefine a vector storing all functions
    vector<Func> funcVec;

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

    auto ValidateVarName = [&](const string& varName) {
        //Name should not contain any invalid characters or blacklisted names
        for (const char& c : varName)
            if (!isalnum(c) && c != '_')
                throw std::exception("Var received a name with an invalid character");

        if (std::find(blacklist.cbegin(), blacklist.cend(), varName) != blacklist.cend())
            throw std::exception(("Variable received illegal variable name. " + string("Got: '") + varName + "'").c_str());

        //Name should be unique
        auto foundName = FindVar(varName);
        if (foundName.second != -1)
            throw std::exception(("Variable by the name of '" + varName + "' already defined").c_str());
    };


    /*

    //funcMap.emplace("divide", std::make_pair(2, [&](vector<string> v) {
    //    string name = v[0]; int nameType = -1;
    //    string value = v[1]; int valueType = GetDataType(value);

    //    auto foundName = FindVar(name);
    //    if (foundName.second == -1)
    //        throw std::exception(("Divide received undefined variable '" + name + "'").c_str());
    //    nameType = foundName.second;

    //    //Value can also be a variable
    //    if (valueType == ERROR) {
    //        auto foundVar = FindVar(value);

    //        if (foundVar.second == -1)
    //            throw std::exception(("Divide received undefined variable '" + value + "'").c_str());

    //        value = foundVar.first;
    //        valueType = foundVar.second;
    //    }

    //    if (stod(value) == 0.0)
    //        throw std::exception("Division by 0 attempted");

    //    if (nameType != DOUBLE && nameType != INT)
    //        throw std::exception(("Cannot perform arithmetic on variables with type'" + IntToType(nameType) + "'").c_str());

    //    if (valueType != DOUBLE && valueType != INT)
    //        throw std::exception(("Arithmetic operation received invalid type '" + IntToType(nameType) + "'").c_str());

    //    if (nameType == DOUBLE)
    //        doubles[name] /= stod(value);
    //    if (nameType == INT)
    //        ints[name] /= stoi(value);
    //}));

    //funcMap.emplace("inc", std::make_pair(1, [&](vector<string> v) {
    //    string name = v[0]; string value = v[0]; int valueType = -1; ResolveValue(value, valueType);

    //    if (valueType != DOUBLE && valueType != INT)
    //        throw std::exception(("Cannot perform arithmetic on variables with type'" + IntToType(valueType) + "'").c_str());

    //    if (valueType == DOUBLE)
    //        doubles[name] += 1;
    //    if (valueType == INT)
    //        ints[name] += 1;
    //}));

    //funcMap.emplace("dec", std::make_pair(1, [&](vector<string> v) {
    //    string name = v[0]; string value = v[0]; int valueType = -1; ResolveValue(value, valueType);

    //    if (valueType != DOUBLE && valueType != INT)
    //        throw std::exception(("Cannot perform arithmetic on variables with type'" + IntToType(valueType) + "'").c_str());

    //    if (valueType == DOUBLE)
    //        doubles[name] -= 1;
    //    if (valueType == INT)
    //        ints[name] -= 1;
    //    }));

    //funcMap.emplace("set", std::make_pair(2, [&](vector<string> v) {
    //    string name = v[0]; int nameType = -1;
    //    string value = v[1]; int valueType = GetDataType(value);

    //    auto foundName = FindVar(name);
    //    if (foundName.second == -1)
    //        throw std::exception(("Set received undefined variable '" + name + "'").c_str());
    //    nameType = foundName.second;

    //    //Value can also be a variable
    //    if (valueType == ERROR) {
    //        auto foundVar = FindVar(value);

    //        if (foundVar.second == -1)
    //            throw std::exception(("Set received undefined variable '" + value + "'").c_str());

    //        value = foundVar.first;
    //        valueType = foundVar.second;
    //    }

    //    if(nameType != valueType 
    //        && !(nameType == DOUBLE && valueType == INT)
    //        && !(nameType == INT && valueType == DOUBLE))
    //        throw std::exception(("Set received wrong type. Got:" + IntToType(valueType) + "' Expected: '" +  IntToType(nameType) + "'").c_str());

    //    switch (nameType) {
    //        case STRING:
    //            FormatString(value);
    //            strings[name] = value;
    //        break;

    //        case DOUBLE:
    //            doubles[name] = stod(value);
    //        break;

    //        case INT:
    //            ints[name] = stoi(value);
    //        break;

    //        case BOOL:
    //            if (value == "true")
    //                bools[name] = 1;
    //            else
    //                bools[name] = 0;
    //        break;
    //    }
    //}));

    //funcMap.emplace("delete", std::make_pair(1, [&](vector<string> v) {
    //    string name = v[0]; int nameType = -1;
    //    errorLevel = 0;

    //    auto foundName = FindVar(name);
    //    if (foundName.second == -1) {
    //        errorLevel = 1; return;
    //    }
    //        
    //    nameType = foundName.second;


    //    switch (nameType) {
    //        case STRING:
    //            strings.erase(name);
    //        break;

    //        case DOUBLE:
    //            doubles.erase(name);
    //        break;

    //        case INT:
    //            ints.erase(name);
    //        break;

    //        case BOOL:
    //            bools.erase(name);
    //        break;
    //    }
    //}));*/

    funcMap.emplace("printl", std::make_pair(OpTypes { 1 }, [ResolveValue](vector<string> v) {
        string value = v[0]; int valueType = -1; ResolveValue(value, valueType);

        if (valueType == STRING)
            FormatString(value);

        cout << value << endl;
    }));

    funcMap.emplace("endl", std::make_pair(OpTypes {}, [ResolveValue](vector<string> v) {
        cout << endl;
    }));

    funcMap.emplace("cls", std::make_pair(OpTypes{}, [ResolveValue](vector<string> v) {
        //Istg this is the best way to do this
        cout << "\033[2J\033[1;1H" << endl;
    }));

    funcMap.emplace("print", std::make_pair(OpTypes{ 1 }, [ResolveValue](vector<string> v) {
        string value = v[0]; int valueType = -1; ResolveValue(value, valueType);

        if (valueType == STRING)
            FormatString(value);

        cout << value;
    }));

    funcMap.emplace("input", std::make_pair(OpTypes { 1 }, [&](vector<string> v) {
        string name = v[0]; int nameType = -1;
        //Reset errorLevel to 0 
        errorLevel = 0;

        auto foundName = FindVar(name);
        if (foundName.second == -1)
            throw std::exception(("Input received undefined variable '" + name + "'").c_str());
        nameType = foundName.second;

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

    funcMap.emplace("exit", std::make_pair(OpTypes{ 1 }, [FindVar](vector<string> v) {
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

    funcMap.emplace("string", std::make_pair(OpTypes{ 0, 3 }, [&](vector<string> v) {
        string name = v[0]; string value = v[1]; int valueType = GetDataType(value);

        //If the value is an errortype, perhaps it is a variable
        if (valueType == ERROR) {
            auto found = FindVar(value);

            if (found.second == -1)
                throw std::exception(("Variable received undefined variable '" + value + "'").c_str());

            value = found.first;
            valueType = found.second;
        }

        if(valueType != STRING)
            throw std::exception(("Variable received mismatching types. Expected: 'string' " + string("Got: '") + IntToType(valueType) + "'").c_str());

        ValidateVarName(name);

        FormatString(value); strings[name] = value;
    }));

    funcMap.emplace("double", std::make_pair(OpTypes{ 0, 3 }, [&](vector<string> v) {
        string name = v[0]; string value = v[1]; int valueType = GetDataType(value);

        //If the value is an errortype, perhaps it is a variable
        if (valueType == ERROR) {
            auto found = FindVar(value);

            if (found.second == -1)
                throw std::exception(("Variable received undefined variable '" + value + "'").c_str());

            value = found.first;
            valueType = found.second;
        }

        if (valueType != DOUBLE && valueType != INT)
            throw std::exception(("Variable received mismatching types. Expected: 'double' " + string("Got: '") + IntToType(valueType) + "'").c_str());

        ValidateVarName(name);

        doubles[name] = stod(value);
    }));

    funcMap.emplace("double", std::make_pair(OpTypes{ 0, 3 }, [&](vector<string> v) {
        string name = v[0]; string value = v[1]; int valueType = GetDataType(value);

        //If the value is an errortype, perhaps it is a variable
        if (valueType == ERROR) {
            auto found = FindVar(value);

            if (found.second == -1)
                throw std::exception(("Variable received undefined variable '" + value + "'").c_str());

            value = found.first;
            valueType = found.second;
        }

        if (valueType != INT && valueType != DOUBLE)
            throw std::exception(("Variable received mismatching types. Expected: 'int' " + string("Got: '") + IntToType(valueType) + "'").c_str());

        ValidateVarName(name);

        ints[name] = stoi(value);
    }));

    funcMap.emplace("bool", std::make_pair(OpTypes{ 0, 3 }, [&](vector<string> v) {
        string name = v[0]; string value = v[1]; int valueType = GetDataType(value);

        //If the value is an errortype, perhaps it is a variable
        if (valueType == ERROR) {
            auto found = FindVar(value);

            if (found.second == -1)
                throw std::exception(("Variable received undefined variable '" + value + "'").c_str());

            value = found.first;
            valueType = found.second;
        }

        if (valueType != BOOL)
            throw std::exception(("Variable received mismatching types. Expected: 'bool' " + string("Got: '") + IntToType(valueType) + "'").c_str());

        ValidateVarName(name);

        if (value == "true")
            bools[name] = 1;
        else
            bools[name] = 0;
    }));


    funcMap.emplace("if", std::make_pair(OpTypes{ 1, 5, 2 }, [&](vector<string> v) {
        
    }));
    
    /*funcMap.emplace("if", std::make_pair(4, [&](vector<string> v) {
    //    string value1 = v[0]; int value1Type = 0;
    //    string op = v[1];
    //    string value2 = v[2]; int value2Type = 0;

    //    if (operators.find(op) == operators.cend())
    //        throw std::exception(("Invalid operator '" + op + "' used in if statement").c_str());

    //    //As you can compare both variables and literals, resolve them.
    //    ResolveValue(value1, value1Type); ResolveValue(value2, value2Type);

    //    //Make sure to not compare different types, unless double and int
    //    if (value1Type != value2Type
    //        && !(value1Type == DOUBLE && value2Type == INT)
    //        && !(value1Type == INT && value2Type == DOUBLE))
    //        throw std::exception(("Comparing different types. Type1: '" + IntToType(value1Type) + "' Type2: '" + IntToType(value2Type) + "'").c_str());

    //    //check for validity.
    //    if ((op == "==" && value1 == value2) ||
    //        (op == "!=" && value1 != value2)) {
    //        funcMap.at("jump").second(vector<string> { v[3] }); return;
    //    }
    //    //If operators are indeed that, but not true then return
    //    else if ((op == "==" || op == "!="))
    //        return;

    //    //greater than, etc cannot be used on non-number types
    //    if(value1Type == STRING || value1Type == BOOL)
    //        throw std::exception(("Cannot use relational operators on Type: '" + IntToType(value1Type) + "'").c_str());

    //    //Convert to doubles for comparison
    //    double value1d = stod(value1); double value2d = stod(value2);

    //    if ((op == "<" && value1d < value2d) ||
    //        (op == ">" && value1d > value2d) ||
    //        (op == ">=" && value1d >= value2d) ||
    //        (op == "<=" && value1d <= value2d)) {
    //        funcMap.at("jump").second(vector<string> { v[3] }); return;
    //    }
    //}));

    //funcMap.emplace("jump", std::make_pair(1, [&](vector<string> v) {
    //    string name = v[0]; int nameType = GetDataType(name);

    //    //As labels can only be ErrorTypes, check for that
    //    if (nameType != ERROR)
    //        throw std::exception(("Tried to jump to label of type '" + IntToType(nameType) + "'").c_str());

    //    if (labelMap.find(name) == labelMap.cend())
    //        throw std::exception(("Tried to jump to undefined label. Got: '" + name + "'").c_str());

    //    int newLine = labelMap[name];
    //    //Make sure we do not continiously push the same line.
    //    if (callHistory.size() == 0)
    //        callHistory.push_back(parsedLineIndex);
    //    if (newLine != callHistory.back())
    //        callHistory.push_back(parsedLineIndex);
    //    parsedLineIndex = newLine;
    //}));

    //funcMap.emplace("return", std::make_pair(0, [&](vector<string> v) {
    //    //Return is equivalent to exit if the callHistory is empty.
    //    if (callHistory.size() == 1)
    //        funcMap.at("exit").second(vector<string> { "0" });

    //    //Set current line to latest entry and remove the entry. 
    //    callHistory.pop_back(); parsedLineIndex = callHistory.back();
    //}));
    */
    
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
            ExitError("Missing semicolon on line " + to_string(lineNum));
        }

        string funcName = tokens[0];
        
        //If token0 is not an function
        if (funcMap.find(funcName) == funcMap.cend())
            ExitError("Function expected, got: '" + funcName + "' on line " + to_string(lineNum));

        //Get the function implementation and argument count
        auto func = funcMap[funcName]; OpTypes argTypes = func.first; int argCount = argTypes.size();

        //Subtract the count of type-0 argTypes from the final total, in order to skip over whitespace
        int whitespaceArgs = std::count(argTypes.cbegin(), argTypes.cend(), 0);
        int tokenCount = argCount * 2 - whitespaceArgs + 2;


        //For every 1 argument, there is 1 seperator, adding the function name and 1 index gives us the theoretical position of the semicolon
        if(tokenCount < tokens.size())
            ExitError("Expected ';' got '" + tokens[tokenCount - 1] + "' on line " + to_string(lineNum));
        //Make sure the function has the correct amount of tokens
        if(tokenCount != tokens.size())
            ExitError("Function '" + funcName + "' expects " + to_string(argCount) + " arguments" + " on line " + to_string(lineNum));

        vector<string> args;
        int argTypeIndex = 0; int actualIndex = 1;
        //Parse the arguments.
        for (int i = 1; i <= argCount * 2 - whitespaceArgs; i++) {
            string token = tokens[i];

            //Check if token is whitespace
            //In case the argType is whitespace, we do not want to increment actualIndex, as the token itself does not exist.
            //Therefore, we continue.
            if (argTypes[argTypeIndex] == 0) {
                if (separators.find(token) == separators.cend()) {
                    ++argTypeIndex;
                    args.push_back(token);
                }
                else
                    ExitError("Expected whitespace got '" + token + "' on line " + to_string(lineNum));
                continue;
            }

            if (actualIndex % 2 == 0) {
                args.push_back(token);
            }
            else {
                int argType = argTypes[argTypeIndex];

                //If the separator is valid and has the same type as the required separator, save it in the list. 
                auto found = separators.find(token);
                if(found == separators.cend())
                    ExitError("Expected: " + IntToOpType(argType) + " got " + token + " on line " + to_string(lineNum));

                int foundType = (*found).second;

                if(foundType != argType)
                    ExitError("Expected: " + IntToOpType(argType) + " got " + IntToOpType(foundType) + " on line " + to_string(lineNum));

                //We do not need to save arg types below 3, as they are not ambiguous. 
                if(argType > 3)
                    args.push_back(token); 
                
                ++argTypeIndex;
            }
            ++actualIndex;
        }

        //for (const auto& a : args)
        //    cout << "Argument: " << a << endl;

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