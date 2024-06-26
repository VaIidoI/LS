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

    Func(const string& name, const OpTypes& types, const std::function<void(vector<string>)>& imp)
        : name_(name), types_(types), implementation_(imp) {}

    //Getters
    string GetName() const {
        return name_;
    }

    OpTypes GetTypes() const {
        return types_;
    }

    std::function<void(vector<string>)> GetImplementation() const {
        return implementation_;
    }

    //Function to execute the implementation
    void Execute(const vector<string>& args) const {
        implementation_(args);
    }

private:
    std::string name_;
    OpTypes types_;
    std::function<void(std::vector<std::string>)> implementation_;
    int returnType_;
};

class Var {
public:
    Var() : data_(""), type_(0) {}
    Var(const string& data, const int& type) : data_(data), type_(type) {}

    string GetData() const {
        return data_;
    }

    void SetData(const string& data) {
        data_ = data;
    }

    int GetType() const {
        return type_;
    }

private:
    string data_; int type_;
};

int main(int argc, char* argv[]) {
    if (argc < 1) {
        ExitError("Please specify a path to the file. ");
    }

    std::ifstream file("test.ls"); string line;

    if (!file.is_open()) {
        ExitError("Cannot locate or open file.");
    }

    //Predefine a vector storing all functions
    vector<Func> funcVec;

    //Predefine the maps which store the variables based on their names.
    std::unordered_map<string, Var> memory;

    //Create a list of keywords, which cannot be the names of variables.
    vector<string> blacklist = { "string", "double", "int", "bool", "errorLevel" };

    //Create a map storing the label's location along with a vector storing the line "history"
    //Also create a lineIndex to keep track of the current line.
    std::map<string, int> labelMap; vector<int> callHistory; int parsedLineIndex = 0;

    //ErrorLevel is a flag that indicates if certain functions encountered any errors
    int errorLevel = 0;

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

    //Iterate through every line (for the 3rd time). Resolve ifs and store the label line locations
    vector<pair<int, int>> ifVec;
    for (int i = 0; i < parsedLines.size(); i++) {
        string l = parsedLines[i].second; int lineNum = parsedLines[i].first;

        //If the line starts with an if
        if (l.rfind("if", 0) == 0) {
            //push the line of the if-statement to the ifVec
            ifVec.push_back({ i, lineNum });
            //Remove semicolon
            parsedLines[i].second.pop_back();
            //Modify if-statement, in order to jump to the corresponding end if false.
            parsedLines[i].second += ", END" + to_string(i) + ";";
        }

        if (l == "end;") {
            //Modify line to the same jump-statement defined in the corresponding if. 
            parsedLines[i].second = "=END" + to_string(ifVec.back().first) + ";"; l = parsedLines[i].second;
            //Remove the entry in the ifVec
            ifVec.pop_back();
        }

        if (l[0] != '=') continue;
        if (l.back() != ';') ExitError("Expected semicolon on label initialization. Got: '" + l + "'");
        string label = FormatLabel(l);

        if (label == string()) ExitError("Incorrect label initialization. Got: '" + l + "'");

        labelMap.emplace(label, i);
        //also push the label names to the blacklist
        blacklist.push_back(label);
    }

    //If any if-statements are still in vec, no end was received.
    if (ifVec.size() > 0)
        ExitError("If-statement did not receive end on line " + to_string(ifVec.front().second));

    //Function that searches though memory and returns the value of a variable given its name.
    auto FindVar = [&memory, &errorLevel](const string& varName, string& value, int& valueType) {
        //Edge case: errorType.
        if (varName == "errorLevel") {
            value = to_string(errorLevel);
            valueType = INT;
        }

        auto found = memory.find(varName);
        if (found != memory.cend()) {
            const string data = (*found).second.GetData();
            const int type = (*found).second.GetType();
            valueType = type;

            if (type == STRING)
                value = '"' + data + '"';
            else
                value = data;

            return true;
        }

        return false;
    };

    auto ResolveValue = [FindVar](string& value, int& type) {
        //Get the type based on data
        type = GetDataType(value);

        //If the type is still nothing, perhaps it is a variable
        if (type == ERROR) {
            string varName = value;
            if (!FindVar(varName, value, type))
                throw std::exception(("Function received undefined variable '" + value + "'").c_str());
            //If the type is STILL nothing, it is an uninitialized variable
            if(type == ERROR)
                throw std::exception(("Function received uninitialized variable '" + value + "'").c_str());
        }
    };

    auto ValidateVarName = [&memory, blacklist](const string& varName) {
        //Name should not contain any invalid characters or blacklisted names
        for (const char& c : varName)
            if (!isalnum(c) && c != '_')
                throw std::exception(("Variable initialization received a name with an invalid character. " + string("Got: '") + string(1, c) + "'").c_str());

        //Name should not be in blacklist
        if (std::find(blacklist.cbegin(), blacklist.cend(), varName) != blacklist.cend())
            throw std::exception(("Variable initialization received illegal variable name. " + string("Got: '") + varName + "'").c_str());

        //Name should not be all numbers
        bool bNumber = true;
        for (const char& c : varName)
            if (!isdigit(c))
                bNumber = false;

        if(bNumber)
            throw std::exception(("Variable initialization received digit-only name. " + string("Got: '") + varName + "'").c_str());

        //Name should be unique
        if (memory.find(varName) != memory.cend())
            throw std::exception(("Variable by the name of '" + varName + "' already defined").c_str());
    };

    //Returns: Function implementation by name
    auto FindFunc = [&funcVec](const string& funcName) {
        return (*std::find_if(funcVec.cbegin(), funcVec.cend(), [funcName](const Func& f) {
            return f.GetName() == funcName;
        })).GetImplementation(); 
    };

    funcVec.push_back(Func("print", OpTypes{ COLON, ARG }, [ResolveValue](vector<string> v) {
        string value = v[0]; int valueType = -1; ResolveValue(value, valueType);

        if (valueType == STRING)
            FormatString(value);

        cout << value;
    }));

    funcVec.push_back(Func("printl", OpTypes{ COLON, ARG }, [ResolveValue](vector<string> v) {
        string value = v[0]; int valueType = -1; ResolveValue(value, valueType);

        if (valueType == STRING)
            FormatString(value);

        cout << value << endl;
    }));

    funcVec.push_back(Func("endl", OpTypes {}, [ResolveValue](vector<string> v) {
        cout << endl;
    }));

    funcVec.push_back(Func("cls", OpTypes{}, [ResolveValue](vector<string> v) {
        //Istg this is the best way to do this
        cout << "\033[2J\033[1;1H" << endl;
    }));

    funcVec.push_back(Func("input", OpTypes { COLON, ARG }, [&](vector<string> v) {
        string name = v[0], value; int type = -1;
        //Reset errorLevel to 0 
        errorLevel = 0;

        if(!FindVar(name, value, type))
            throw std::exception(("Input received undefined variable '" + name + "'").c_str());
        
        //Get the line and its datatype. If it's errortype, it becomes a string, due to it not being anything else
        string s = ""; std::getline(std::cin, s); int lineType = GetDataType(s);

        if (lineType == ERROR)
            lineType = STRING;

        //Uninitialized variable as target, proceed accordingly
        if (type == ERROR) {
            memory[name] = Var(s, lineType); return;
        }

        //Set errorLevel to 1, indicating a type mismatch, unless type is a string. 
        if (type != lineType && type != STRING)
            errorLevel = 1;

        //Save it to the corresponding variable
        if (errorLevel == 0) {
            memory[name].SetData(s);
        }
    }));

    //Overload: Print a string before inputting. 
    funcVec.push_back(Func("input", OpTypes{ COLON, ARG, COMMA, ARG }, [&](vector<string> v) {
        cout << FormatString(v[0]); FindFunc("input")(vector<string> { v[1] });
    }));

    funcVec.push_back(Func("var", OpTypes{ SPACE, ARG, SET, ARG }, [&](vector<string> v) {
        string name = v[0]; string value = v[1]; int valueType = 0;
        ResolveValue(value, valueType);

        ValidateVarName(name);
        if(valueType == STRING)
            FormatString(value);

        memory[name] = Var(value, valueType);
    }));

    //Overload: Define variable, but do not initialize it
    funcVec.push_back(Func("var", OpTypes{ SPACE, ARG }, [&](vector<string> v) {
        string name = v[0];
        ValidateVarName(name);

        memory[name] = Var("", ERROR);
    }));

    //Sets a variable to a value
    //the final line should look like [VarName] var1 = value, thus having an additional 0 prepended.
    funcVec.push_back(Func("[VarName]", OpTypes{ SPACE, ARG, SET, ARG }, [&](vector<string> v) {
        string name = v[0]; string nameValue = ""; int nameType = 0;
        string value = v[1]; int valueType = 0; ResolveValue(value, valueType);

        //if the name doesn't get found
        if(!FindVar(name, nameValue, nameType))
            throw std::exception(("Setter received a literal or undefined variable. Got: '" + name + "'").c_str());

        //Uninitialized variable as target, proceed accordingly
        if (nameType == ERROR) {
            memory[name] = Var(value, valueType); return;
        }

        //If it isn't the same type, or number type.
        if(nameType != valueType 
            && !(nameType == DOUBLE && valueType == INT)
            && !(nameType == INT && valueType == DOUBLE))
            throw std::exception(("Setter received wrong type. Got: '" + IntToType(valueType) + "' Expected: '" +  IntToType(nameType) + "'").c_str());

        memory[name] = Var(value, valueType);
    }));

    //Modifying a variable
    funcVec.push_back(Func("[VarName]", OpTypes{ SPACE, ARG, MOD, ARG }, [&](vector<string> v) {
        string name = v[0]; string nameValue = ""; int nameType = 0;
        string value = v[2]; int valueType = 0; ResolveValue(value, valueType);
        string op = v[1];

        //if the name doesn't get found
        if (!FindVar(name, nameValue, nameType))
            throw std::exception(("Setter received a literal or undefined variable. Got: '" + name + "'").c_str());

        //If it isn't the same type, or number type.
        if (nameType != valueType
            && !(nameType == DOUBLE && valueType == INT)
            && !(nameType == INT && valueType == DOUBLE))
            throw std::exception(("Arithmatic operation received wrong type. Got: '" + IntToType(valueType) + "' Expected: '" + IntToType(nameType) + "'").c_str());

        if (nameType == BOOL)
            throw std::exception("Cannot perform arithmatic operation on type 'bool'");

        if (nameType == STRING && op != "+=")
            throw std::exception(("Cannot use operator '" + op + "' on a string").c_str());
        else if (nameType == STRING) {
            string data = memory.at(name).GetData(); FormatString(value);
            memory[name].SetData(data + value); return;
        }

        //Get the data from memory
        double data = stod(memory.at(name).GetData());
        double newData = stod(value);

        if (op == "+=")
            memory[name].SetData(to_string(data + newData));
        else if (op == "-=")
            memory[name].SetData(to_string(data - newData));
        else if (op == "*=")
            memory[name].SetData(to_string(data * newData));
        else if (op == "/=") {
            if (stod(value) == 0.0)
                throw std::exception("Division by 0 attempted ");

            memory[name].SetData(to_string(data / newData));
        }
        else if (op == "%=") {
            if (stod(value) == 0.0)
                throw std::exception("Modolo by 0 attempted ");

            memory[name].SetData(to_string((int)data % (int)newData));
        }

        //Code efficient, albeit scuffed solution
        if (nameType == INT)
            memory[name].SetData(to_string((int)stod(memory.at(name).GetData())));
    }));

    //Incrementing or decrementing variable
    funcVec.push_back(Func("[VarName]", OpTypes{ SPACE, ARG, MOD }, [&](vector<string> v) {
        string name = v[0], value = ""; int type = 0;
        string op = v[1];

        //if the name doesn't get found
        if (!FindVar(name, value, type))
            throw std::exception(("Setter received a literal or undefined variable. Got: '" + name + "'").c_str());

        //If type is string or bool
        if (type == STRING || type == BOOL)
            throw std::exception(("Cannot use operator '" + op + "' on type '" + IntToType(type) + "'").c_str());

        double data = stod(memory.at(name).GetData());

        if (op == "++")
            memory[name].SetData(to_string(data + 1.0));
        else if (op == "--")
            memory[name].SetData(to_string(data - 1.0));
        else
            throw std::exception("Wrong operator received. Expected '++' or '--'");

        //Code efficient, albeit scuffed solution
        if (type == INT)
            memory[name].SetData(to_string((int)stod(memory.at(name).GetData())));
    }));

    funcVec.push_back(Func("delete", OpTypes{ COLON, ARG }, [&](vector<string> v) {
        string name = v[0], value; int type = -1;
        errorLevel = 0;

        if (!FindVar(name, value, type)) {
            errorLevel = 1; return;
        }
            
        memory.erase(name);
    }));

    funcVec.push_back(Func("exit", OpTypes{ COLON, ARG }, [ResolveValue](vector<string> v) {
        string code = v[0]; int type = 0; ResolveValue(code, type);

        if (GetDataType(code) != INT) throw std::exception(("Exit requires argument type: 'int' got: '" + IntToType(type) + "'").c_str());
        cout << endl << "Program exited with code: " << code << endl;
        exit(stoi(code));
    }));

    funcVec.push_back(Func("jump", OpTypes { COLON, ARG }, [&](vector<string> v) {
        string name = v[0]; int nameType = GetDataType(name);

        //As labels can only be ErrorTypes, check for that
        if (nameType != ERROR)
            throw std::exception(("Tried to jump to label of type '" + IntToType(nameType) + "'").c_str());

        if (labelMap.find(name) == labelMap.cend())
            throw std::exception(("Tried to jump to undefined label. Got: '" + name + "'").c_str());

        int newLine = labelMap[name];

        //Push current line to callHistory and set index to newLine
        callHistory.push_back(parsedLineIndex);
        parsedLineIndex = newLine;
    }));

    funcVec.push_back(Func("return", OpTypes{}, [&](vector<string> v) {
        //Return is equivalent to exit if the callHistory is empty.
        if (callHistory.size() < 1)
            FindFunc("exit")(vector<string> { "0" });

        //Set current line to latest entry and remove the entry. 
        parsedLineIndex = callHistory.back(); callHistory.pop_back();
    }));

    funcVec.push_back(Func("if", OpTypes{ COLON, ARG, LOGIC, ARG, COMMA, ARG }, [&](vector<string> v) {
        string value1 = v[0]; int value1Type = 0;
        string op = v[1];
        string value2 = v[2]; int value2Type = 0;

        //As you can compare both variables and literals, resolve them.
        ResolveValue(value1, value1Type); ResolveValue(value2, value2Type);

        //Make sure to not compare different types, unless double and int
        if (value1Type != value2Type
            && !(value1Type == DOUBLE && value2Type == INT)
            && !(value1Type == INT && value2Type == DOUBLE))
            throw std::exception(("Comparing different types. Type1: '" + IntToType(value1Type) + "' Type2: '" + IntToType(value2Type) + "'").c_str());

        //check for validity.
        if ((op == "==" && value1 != value2) ||
            (op == "!=" && value1 == value2)) {
            FindFunc("jump")(vector<string> { v[3] }); return;
        }
        //If operators are indeed that, but not true then return
        else if ((op == "==" || op == "!="))
            return;

        //greater than, etc cannot be used on non-number types
        if(value1Type == STRING || value1Type == BOOL)
            throw std::exception(("Cannot use relational operators on Type: '" + IntToType(value1Type) + "'").c_str());

        //Convert to doubles for comparison
        double value1d = stod(value1); double value2d = stod(value2);

        //If the conditions aren't met, jump to the end_if
        if ((op == "<" && value1d >= value2d) ||
            (op == ">" && value1d <= value2d) ||
            (op == ">=" && value1d < value2d) ||
            (op == "<=" && value1d > value2d)) {
            FindFunc("jump")(vector<string> { v[3] });
        }
    }));

    //Append function names to the blacklist
    for (const auto& a : funcVec)
        blacklist.push_back(a.GetName());

    //Thirdly, tokenize each line and go through the actual interpretation process. 
    for (parsedLineIndex; parsedLineIndex < parsedLines.size(); parsedLineIndex++) {
        int lineNum = parsedLines[parsedLineIndex].first; string l = parsedLines[parsedLineIndex].second;

        //If callHistory has entries which are larger than parsedLineIndex, delete them, due to them being from past jumps
        auto onLine = std::find_if(callHistory.begin(), callHistory.end(), [parsedLineIndex](int index) { return parsedLineIndex <= index;  });
        if (onLine != callHistory.cend()) {
            callHistory.erase(onLine, callHistory.end());
        }

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

        //Check if semicolon is found
        if (l.back() != ';') {
            ExitError("Missing semicolon on line " + to_string(lineNum));
        }

        string funcName = tokens[0];
        //If funcName is the name of a variable, prepend the function that handles variable changes [VarName], in order to pass the variable name in args
        if (memory.find(funcName) != memory.cend()) {
            tokens.insert(tokens.begin(), "[VarName]"); funcName = "[VarName]";
        }
        
        bool bFound = false, bFinished = false;
        //Get the function that fits best. (Has the correct number of types)
        for (const Func& f : funcVec) {
            if (f.GetName() != funcName) continue; bFound = true;
            OpTypes argTypes = f.GetTypes();  
            //Subtract the count of type-0 argTypes from the final total, in order to skip over whitespace
            int whitespaceArgs = std::count(argTypes.cbegin(), argTypes.cend(), 0);
            //Get the total amount of tokens the function should have
            int tokenCount = argTypes.size() - whitespaceArgs + 2;

            //Make sure the function has the correct amount of tokens, if not continue
            if (tokenCount != tokens.size())
                continue;

            vector<string> args; int argTypeIndex = 0;

            //Parse the arguments.
            for (int i = 1; i < tokens.size() - 1; i++) {
                string token = tokens[i]; int argType = argTypes[argTypeIndex];

                //As argType 0 is whitespace, which is not saved in the tokenizing process, skip to the next arg. 
                if (argType == 0)
                    argType = argTypes[++argTypeIndex];

                if (argType == ARG)
                    args.push_back(token);
                //If it is not an argument
                else {
                    //Token has to be a valid seperator, and of the same opType
                    auto found = separators.find(token);
                    if (found == separators.cend())
                        break;

                    if ((*found).second != argType)
                        break;

                    //As opTypes 2 and below are unambiguous, do not push them
                    if (argType > 3)
                        args.push_back(token);
                }
                argTypeIndex++;
            }
            
            //The total amount of args the function requires. Only take in opTypes above 3 or -1
            int argTotal = std::count_if(argTypes.cbegin(), argTypes.cend(), [](int value) {
                return (value > 3 || value == ARG) ? true : false;
            });

            //Check if all tokens were correct, leading to a satisfactory amount of arguments
            if (args.size() != argTotal)
                continue;

            //Call the function
            try {
                f.Execute(args);
            }
            catch (const std::exception& e) {
                ExitError(string(e.what()) + " on line " + to_string(lineNum));
            }
            bFinished = true; break;
        }

        //If token0 is not an function
        if(!bFound)
            ExitError("Function expected, got: '" + funcName + "' on line " + to_string(lineNum));
        if(bFound && !bFinished)
            ExitError("No function overload of function '" + funcName + "' matches argument list on line " + to_string(lineNum));
    }
   
    file.close();
    return 0;
}