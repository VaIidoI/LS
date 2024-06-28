#include "DataTypes.h"
#include <iostream>
#include <fstream>
#include "Parse.h"
#include <chrono>

using std::cout; using std::endl; using std::to_string; using std::pair; using std::make_pair;

void ExitError(const string& error) noexcept {
    std::cerr << '\n' << error << endl;
    exit(-1);
}

//A function consists of a name, arguments with specific types and an implementation
class Func {
public:
    Func() : name_(""), types_(OpTypes{}), implementation_() {}

    Func(const string& name, const OpTypes& types, const std::function<void(const vector<string>&)>& imp)
        : name_(name), types_(types), implementation_(imp) {}

    //Getters
    string GetName() const {
        return name_;
    }

    OpTypes GetTypes() const {
        return types_;
    }

    std::function<void(const vector<string>&)> GetImplementation() const {
        return implementation_;
    }

    //Function to execute the implementation
    void Execute(const vector<string>& args) const {
        implementation_(args);
    }

private:
    int returnType_; std::string name_;    OpTypes types_;
    std::function<void(const vector<string>&)> implementation_;
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
    int type_; string data_;
};

int main(int argc, char* argv[]) {
    if (argc < 1) {
        ExitError("Please specify a path to the file. ");
    }

    std::ifstream file("test.ls"); string line;

    if (!file.is_open()) {
        ExitError("Cannot locate or open file.");
    }

    auto start = std::chrono::high_resolution_clock::now();

    //Predefine a map storing all functions
    std::unordered_map<string, vector<Func>> funcMap;
    vector<Func> funcVec;

    //Predefine the maps which store the variables based on their names.
    std::unordered_map<string, Var> memory;

    //Create a list of keywords, which cannot be the names of variables.
    std::unordered_set<string> blacklist = { "string", "double", "int", "bool", "errorLevel" };

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
    vector<pair<int, string>> parsedLines; vector<pair<int, string>> statementVec;
    for (const auto&[lineNum, l] : lines) {
        auto parsed = Parse(l);

        if (parsed.size() == 1 && parsed.back().empty())
            continue;

        //After seperating semicolons, trim them to get rid of any whitespace inbetween.
        for (const string& x : parsed) {
            //Thirdly, resolve control flow statements
            string parsedLine = TrimWhitespace(x); auto tokens = Tokenize(l); string statementName = tokens[0];
            if (statementName == "for") {
                //A for loop always has at least 13 tokens
                if (tokens.size() < 12)
                    ExitError("Invalid args in for-loop initialization on line " + to_string(lineNum));

                if (tokens.back() != ":")
                    ExitError("Invalid for-loop initialization. Got '" + tokens.back() + "' Expected: ':' on line " + to_string(lineNum));

                //For each for-loop segment, parse it and also validate the syntax
                string initializer = "", condition = "", iteration = ""; int index = 1;
                for (int i = index; tokens[i] != ","; i++, index++) {
                    if (i == tokens.size() - 1)
                        ExitError("Invalid for-loop initialization. Expected: ',' on line " + to_string(lineNum));
                    initializer += tokens[i];
                }
                index++;
                for (int i = index; tokens[i] != ","; i++, index++) {
                    if (i == tokens.size() - 1)
                        ExitError("Invalid for-loop initialization. Expected: ',' on line " + to_string(lineNum));
                    condition += tokens[i];
                }
                index++;
                for (int i = index; tokens[i] != ":"; i++)
                    iteration += tokens[i];

                if(initializer.empty() || condition.empty() || iteration.empty())
                    ExitError("Invalid for-loop initialization. Segments not defined properly on line" + to_string(lineNum));
               
                int endIndex = lineNum;
                //Insert all of the necessary lines 
                parsedLines.push_back({ lineNum,  "var " + initializer + ";"});
                parsedLines.push_back({ lineNum,  "=FOR_" + to_string(endIndex) + ";"});
                parsedLines.push_back({ lineNum,  "if " + condition + ", END_" + to_string(endIndex) + ";" });
                statementVec.push_back({ lineNum, iteration + ";jump: FOR_" + to_string(endIndex)
                                                  + ";=END_" + to_string(endIndex) + ";delete: " + tokens[1] + ";" });
            }
            else if (statementName == "while") {
                if (tokens.size() < 5)
                    ExitError("Invalid args in while-loop initialization on line " + to_string(lineNum));

                if (tokens.back() != ":")
                    ExitError("Invalid while-loop initialization. Got '" + tokens.back() + "' Expected: ':' on line " + to_string(lineNum));
                
                string condition = "";
                for (int i = 1; tokens[i] != ":"; i++) {
                    condition += tokens[i];
                }

                if (condition.empty())
                    ExitError("Invalid while-loop initialization. Condition not defined properly on line" + to_string(lineNum));

                int endIndex = lineNum;
                parsedLines.push_back({ lineNum,  "=WHILE_" + to_string(endIndex) + ";" });
                parsedLines.push_back({ lineNum,  "if " + condition + ", END_" + to_string(endIndex) + ";" });
                statementVec.push_back({ lineNum, "jump: WHILE_" + to_string(endIndex) + ";=END_" + to_string(endIndex) + ";"});
            }
            else if (statementName == "if") {
                if (tokens.size() < 5)
                    ExitError("Invalid args in if-statement initialization on line " + to_string(lineNum));

                if (tokens.back() != ":")
                    ExitError("Invalid if-statement initialization. Got '" + tokens.back() + "' Expected: ':' on line " + to_string(lineNum));

                int endIndex = lineNum;
                //Remove colon
                parsedLine.pop_back();
                //Modify if-statement, in order to jump to the corresponding end if false.
                parsedLine += ", END_" + to_string(endIndex) + ";";
                parsedLines.push_back({ lineNum, parsedLine });
                statementVec.push_back({ lineNum, "=END_" + to_string(endIndex) + ";" });
            }
            else if (statementName == "end") {
                if(statementVec.size() == 0)
                    ExitError("Received hanging end-statement on line " + to_string(lineNum));
                //Modify line to the last entry in the statementVec
                parsedLine = statementVec.back().second;

                for(const auto& s : SplitString(parsedLine, ';'))
                    parsedLines.push_back({ lineNum, s + ";"});

                //Remove the entry in the statementVec
                statementVec.pop_back();
            }
            else {
                parsedLines.push_back({ lineNum, parsedLine });
            }
        }
    }

    //If any statements are still in vec, no end was received.
    if (statementVec.size() > 0)
        ExitError("If-statement did not receive end on line " + to_string(statementVec.front().first));

    lines.clear(); statementVec.clear();

    //Fouth, resolve label names
    for (int i = 0; i < parsedLines.size(); i++) {
        string l = parsedLines[i].second; int lineNum = parsedLines[i].first;

        if (l[0] != '=') continue;
        if (l.back() != ';') ExitError("Expected semicolon on label initialization. Got: '" + l + "'");
        string label = FormatLabel(l);

        if (label == string()) ExitError("Incorrect label initialization. Got: '" + l + "'");

        labelMap.emplace(label, i);
        //also push the label names to the blacklist
        blacklist.insert(label);
    }

    //Function that searches though memory and returns the value of a variable given its name.
    auto FindVar = [&memory, &errorLevel](const string& varName, string& value, int& valueType) {
        //Edge case: errorType.
        if (varName == "errorLevel") {
            value = to_string(errorLevel);
            valueType = INT;
            return true;
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
                throw std::exception(("Function received undefined identifier '" + value + "'").c_str());
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
            throw std::exception(("Variable initialization received illegal identifier. " + string("Got: '") + varName + "'").c_str());

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

    //Returns: Function implementation by name and opTypes
    auto FindFunc = [&funcMap](const string& funcName, const OpTypes& types) {
        //Find the function vector given the name
        auto found = funcMap.find(funcName);
        if(found == funcMap.cend())
            throw std::exception(("Function expected, got: '" + funcName + "'").c_str());
        auto funcSet = (*found).second;

        //Get the corresponding function implementation based on the types
        auto func = std::find_if(funcSet.cbegin(), funcSet.cend(), [types](const Func& f) {
            return f.GetTypes() == types;
        });

        //Build the error message
        string error = "";
        for (const int& a : types)
            error += "'" + IntToOpType(a) + "', ";
        //Remove the comma lol
        if (!error.empty()) {
            error.pop_back(); error.pop_back();
        }

        if (func == funcSet.cend())
            throw std::exception(("No overload for function '" + funcName + "' matches types: " + error).c_str());

        return (*func).GetImplementation();
    };

    funcMap.insert({ "print", vector<Func> {
        Func("print", OpTypes{ COLON, ARG }, [ResolveValue](vector<string> v) {
            string value = v[0]; int valueType = -1; ResolveValue(value, valueType);

            if (valueType == STRING)
                FormatString(value);

            cout << value;
        })
    } });

    funcMap.insert({ "printl", vector<Func> {
        Func("printl", OpTypes{ COLON, ARG }, [ResolveValue](vector<string> v) {
            string value = v[0]; int valueType = -1; ResolveValue(value, valueType);

            if (valueType == STRING)
                FormatString(value);

            cout << value << endl;
        })
    } });

    funcMap.insert({ "endl", vector<Func> {
        Func("endl", OpTypes{}, [ResolveValue](vector<string> v) {
            cout << endl;
        })
    } });

    funcMap.insert({ "cls", vector<Func> {
        Func("cls", OpTypes{}, [ResolveValue](vector<string> v) {
            // Istg this is the best way to do this
            cout << "\033[2J\033[1;1H" << endl;
        })
    } });

    funcMap.insert({ "input", vector<Func> {
        Func("input", OpTypes{ COLON, ARG }, [&](vector<string> v) {
            string name = v[0], value; int type = -1;
            // Reset errorLevel to 0 
            errorLevel = 0;

            if (!FindVar(name, value, type))
                throw std::exception(("Input received undefined identifier '" + name + "'").c_str());

            // Get the line and its datatype. If it's errortype, it becomes a string, due to it not being anything else
            string s = ""; std::getline(std::cin, s); int lineType = GetDataType(s);

            if (lineType == ERROR)
                lineType = STRING;

            // Uninitialized variable as target, proceed accordingly
            if (type == ERROR) {
                memory[name] = Var(s, lineType); return;
            }

            // Set errorLevel to 1, indicating a type mismatch, unless type is a string. 
            if (type != lineType && type != STRING)
                errorLevel = 1;

            // Save it to the corresponding variable
            if (errorLevel == 0) {
                memory[name].SetData(s);
            }
        }),
        // Overload: Print a string before inputting. 
        Func("input", OpTypes{ COLON, ARG, COMMA, ARG }, [&](vector<string> v) {
            cout << FormatString(v[0]); FindFunc("input", OpTypes{ COLON, ARG })(vector<string>{v[1]});
        })
    }});

    funcMap.insert({ "var", vector<Func> {
        Func("var", OpTypes{ ARG, SET, ARG }, [&](vector<string> v) {
            string name = v[0]; string value = v[1]; int valueType = 0;
            ResolveValue(value, valueType);

            ValidateVarName(name);
            if (valueType == STRING)
                FormatString(value);

            memory[name] = Var(value, valueType);
        }),
        // Overload: Define variable, but do not initialize it
        Func("var", OpTypes{ ARG }, [&](vector<string> v) {
            string name = v[0];
            ValidateVarName(name);

            memory[name] = Var("", ERROR);
        })
    }});

    funcMap.insert({ "[VarName]", vector<Func> {
        // Sets a variable to a value
        // the final line should look like [VarName] var1 = value, thus having an additional 0 prepended.
        Func("[VarName]", OpTypes{ ARG, SET, ARG }, [&](vector<string> v) {
            string name = v[0]; string nameValue = ""; int nameType = 0;
            string value = v[1]; int valueType = 0; ResolveValue(value, valueType);

            // if the name doesn't get found
            if (!FindVar(name, nameValue, nameType))
                throw std::exception(("Setter received a literal or undefined identifier. Got: '" + name + "'").c_str());

            // Uninitialized variable as target, proceed accordingly
            if (nameType == ERROR) {
                memory[name] = Var(value, valueType); return;
            }

            // If it isn't the same type, or number type.
            if (nameType != valueType && !(nameType == DOUBLE && valueType == INT) && !(nameType == INT && valueType == DOUBLE))
                throw std::exception(("Setter received wrong type. Got: '" + IntToType(valueType) + "' Expected: '" + IntToType(nameType) + "'").c_str());

            memory[name] = Var(value, valueType);
        }),
        // Modifying a variable
        Func("[VarName]", OpTypes{ ARG, MOD, ARG }, [&](vector<string> v) {
            string name = v[0]; string nameValue = ""; int nameType = 0;
            string value = v[2]; int valueType = 0; ResolveValue(value, valueType);
            string op = v[1];

            // if the name doesn't get found
            if (!FindVar(name, nameValue, nameType))
                throw std::exception(("Setter received a literal or undefined identifier. Got: '" + name + "'").c_str());

            // If it isn't the same type, or number type.
            if (nameType != valueType && !(nameType == DOUBLE && valueType == INT) && !(nameType == INT && valueType == DOUBLE))
                throw std::exception(("Arithmetic operation received wrong type. Got: '" + IntToType(valueType) + "' Expected: '" + IntToType(nameType) + "'").c_str());

            if (nameType == BOOL)
                throw std::exception("Cannot perform arithmetic operation on type 'bool'");

            if (nameType == STRING && op != "+=")
                throw std::exception(("Cannot use operator '" + op + "' on a string").c_str());
            else if (nameType == STRING) {
                string data = memory.at(name).GetData(); FormatString(value);
                memory[name].SetData(data + value); return;
            }

            // Get the data from memory
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
                    throw std::exception("Modulo by 0 attempted ");

                memory[name].SetData(to_string((int)data % (int)newData));
            }

            // Code efficient, albeit scuffed solution
            if (nameType == INT)
                memory[name].SetData(to_string((int)stod(memory.at(name).GetData())));
        }),
        // Incrementing or decrementing variable
        Func("[VarName]", OpTypes{ ARG, MOD }, [&](vector<string> v) {
            string name = v[0], value = ""; int type = 0;
            string op = v[1];

            // if the name doesn't get found
            if (!FindVar(name, value, type))
                throw std::exception(("Setter received a literal or undefined identifier. Got: '" + name + "'").c_str());

            // If type is string or bool
            if (type == STRING || type == BOOL)
                throw std::exception(("Cannot use operator '" + op + "' on type '" + IntToType(type) + "'").c_str());

            double data = stod(memory.at(name).GetData());

            if (op == "++")
                memory[name].SetData(to_string(data + 1.0));
            else if (op == "--")
                memory[name].SetData(to_string(data - 1.0));
            else
                throw std::exception("Wrong operator received. Expected '++' or '--'");

            // Code efficient, albeit scuffed solution
            if (type == INT)
                memory[name].SetData(to_string((int)stod(memory.at(name).GetData())));
        })
    }});

    funcMap.insert({ "sqrt", vector<Func> {
        // Modifying a variable
        Func("sqrt", OpTypes{ COLON, ARG }, [&](vector<string> v) {
            string name = v[0]; string nameValue = ""; int nameType = 0;

            // if the name doesn't get found
            if (!FindVar(name, nameValue, nameType))
                throw std::exception(("Sqrt received a literal or undefined identifier. Got: '" + name + "'").c_str());

            // If it isn't the same type, or number type.
            if (nameType != DOUBLE && nameType != INT)
                throw std::exception(("Square root operation received wrong type. Got: '" + IntToType(nameType) + "'").c_str());

            // Get the data from memory
            double data = stod(memory.at(name).GetData());

            memory[name].SetData(to_string(sqrt(data)));

            // Code efficient, albeit scuffed solution, converts double to int 
            if (nameType == INT)
                memory[name].SetData(to_string((int)stod(memory.at(name).GetData())));
        })
    }});

    funcMap.insert({ "delete", vector<Func> {
        Func("delete", OpTypes{ COLON, ARG }, [&](vector<string> v) {
            string name = v[0], value; int type = -1;
            errorLevel = 0;

            if (!FindVar(name, value, type)) {
                errorLevel = 1; return;
            }

            memory.erase(name);
        })
    }});

    funcMap.insert({ "exit", vector<Func> {
        Func("exit", OpTypes{ COLON, ARG }, [ResolveValue](vector<string> v) {
            string code = v[0]; int type = 0; ResolveValue(code, type);

            if (GetDataType(code) != INT) throw std::exception(("Exit requires argument type: 'int' got: '" + IntToType(type) + "'").c_str());
            cout << endl << "Program exited with code: " << code << endl;
            exit(stoi(code));
        })
    }});

    funcMap.insert({ "jump", vector<Func> {
        Func("jump", OpTypes{ COLON, ARG }, [&](vector<string> v) {
            string name = v[0]; int nameType = GetDataType(name);

            // As labels can only be ErrorTypes, check for that
            if (nameType != ERROR)
                throw std::exception(("Tried to jump to label of type '" + IntToType(nameType) + "'").c_str());

            if (labelMap.find(name) == labelMap.cend())
                throw std::exception(("Tried to jump to undefined label. Got: '" + name + "'").c_str());

            int newLine = labelMap[name];

            // Push current line to callHistory and set index to newLine
            callHistory.push_back(parsedLineIndex);
            parsedLineIndex = newLine;
        })
    }});

    funcMap.insert({ "return", vector<Func> {
        Func("return", OpTypes{}, [&](vector<string> v) {
            // Return is equivalent to exit if the callHistory is empty.
            if (callHistory.size() < 1)
                FindFunc("exit", OpTypes{ COLON, ARG })(vector<string> { "0" });

            // Set current line to latest entry and remove the entry. 
            parsedLineIndex = callHistory.back(); callHistory.pop_back();
        })
    }});

    funcMap.insert({ "if", vector<Func> {
        Func("if", OpTypes{ ARG, LOGIC, ARG, COMMA, ARG }, [&](vector<string> v) {
            string value1 = v[0]; int value1Type = 0;
            string op = v[1];
            string value2 = v[2]; int value2Type = 0;

            // As you can compare both variables and literals, resolve them.
            ResolveValue(value1, value1Type); ResolveValue(value2, value2Type);

            // Make sure to not compare different types, unless double and int
            if (value1Type != value2Type && !(value1Type == DOUBLE && value2Type == INT) && !(value1Type == INT && value2Type == DOUBLE))
                throw std::exception(("Comparing different types. Type1: '" + IntToType(value1Type) + "' Type2: '" + IntToType(value2Type) + "'").c_str());

            // check for validity.
            if ((op == "==" && value1 != value2) || (op == "!=" && value1 == value2)) {
                FindFunc("jump", OpTypes{ COLON, ARG })(vector<string> { v[3] }); return;
            }
            // If operators are indeed that, but not true then return
            else if ((op == "==" || op == "!="))
                return;

            // greater than, etc cannot be used on non-number types
            if (value1Type == STRING || value1Type == BOOL)
                throw std::exception(("Cannot use relational operators on Type: '" + IntToType(value1Type) + "'").c_str());

            // Convert to doubles for comparison
            double value1d = stod(value1); double value2d = stod(value2);

            // If the conditions aren't met, jump to the end_if
            if ((op == "<" && value1d >= value2d) || (op == ">" && value1d <= value2d) || (op == ">=" && value1d < value2d) || (op == "<=" && value1d > value2d)) {
                FindFunc("jump", OpTypes{ COLON, ARG })(vector<string> { v[3] });
            }
        })
    }});

    //Append function names to the blacklist
    for (const auto& a : funcVec)
        blacklist.insert(a.GetName());

    //Vector storing functions on each line. Int stores real line, vector<string> stores arg, function stores implementation
    vector<std::tuple<int, vector<string>, std::function<void(const vector<string>&)>>> functions;

    //Fifth, tokenize each line and go through the actual interpretation process. 
    for(const auto& [lineNum, l] : parsedLines) {
        //If callHistory has entries which are larger than parsedLineIndex, delete them, due to them being from past jumps
        auto onLine = std::find_if(callHistory.begin(), callHistory.end(), [parsedLineIndex](int index) { return parsedLineIndex <= index;  });
        if (onLine != callHistory.cend())
            callHistory.erase(onLine, callHistory.end());

        vector<string> tokens;

        //Skip labels
        if (l[0] == '=') {
            //Take into consideration that the location labels point to should be kept the same when actually running the function implementations
            functions.push_back({ -1, vector<string>{}, nullptr });
            continue;
        }

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
        //If funcName is not a function, perhaps it is an identifier. Prepend [VarName] and set it as the function name. 
        if (funcMap.find(funcName) == funcMap.cend()) {
            tokens.insert(tokens.begin(), "[VarName]"); funcName = "[VarName]";
        }

        vector<string> args; OpTypes argTypes;
        //For each token, check its opType and push it back to the vector
        for (int i = 1; i < tokens.size() - 1; i++) {
            string token = tokens[i];
            //Token is not a seperator, therefore it is an argument
            auto found = separators.find(token);
            if (found == separators.cend()) {
                args.push_back(token); argTypes.push_back(ARG);
            }
            //Token is a seperator and has a corresponding OpType
            else {
                int argType = (*found).second;
                argTypes.push_back(argType);
                //As opTypes 2 and below are unambiguous, do not push them
                if(argType > 3)
                    args.push_back(token);
            }
        }

        //Store the function in the function vector
        try {
            functions.push_back({ lineNum, args, FindFunc(funcName, argTypes) });
        }
        catch (const std::exception& e) {
            ExitError(string(e.what()) + " on line " + to_string(lineNum));
        }
    }

    //Execute the functions
    for (parsedLineIndex; parsedLineIndex < functions.size(); parsedLineIndex++) {
        int lineNum = std::get<0>(functions[parsedLineIndex]);
        //Label
        if (lineNum == -1)
            continue;
        auto args = std::get<1>(functions[parsedLineIndex]);
        auto func = std::get<2>(functions[parsedLineIndex]);

        try {
            //Get the function implementation and pass in the args. Index 2 and 1 respectively
            func(args);
        }
        catch (const std::exception& e) {
            cout << e.what() << " on line " << to_string(parsedLineIndex);
        }
    }

    auto end = std::chrono::high_resolution_clock::now();

    cout << endl << "Program sucessfully executed. Exited with code 0." <<  endl <<
        "Elapsed time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms" << std::endl;
   
    file.close();
    return 0;
}