#include "Archetypes.h"
#include "DataTypes.h"
#include <iostream>
#include <fstream>
#include "Parse.h"
#include <chrono>
#include <stack>

using std::cout; using std::endl; using std::to_string; using std::pair; using std::make_pair;

void ExitError(const string& error) noexcept {
    std::cerr << '\n' << error << "." << endl;
    exit(-1);
}

int main(int argc, char* argv[]) {
    if (argc < 1) {
        ExitError("Please specify a path to the file. ");
    }

    std::ifstream file(argv[1]); string line;

    if (!file.is_open()) {
        ExitError("Cannot locate or open file.");
    }

    auto start = std::chrono::high_resolution_clock::now();

    //Predefine a map storing all instructions
    std::unordered_map<string, vector<Instruction>> instructions;

    //Predefine a map storing all functions. Stores function name and argument count.
    std::unordered_map<string, int> functions;

    //Predefine the maps which store the variables based on their names.
    std::unordered_map<string, Var> memory;
    //Predefine a stack, storing variables
    std::stack<Var> stack;

    //Create a list of keywords, which cannot be the names of variables.
    std::unordered_set<string> blacklist = { "string", "double", "int", "bool", "errorLevel" };

    //Create a map storing the label's location along with a vector storing the line "history"
    //Also create a lineIndex to keep track of the current line.
    std::unordered_map<string, int> labelMap; vector<int> callHistory; int parsedLineIndex = 0;

    //ErrorLevel is a flag that indicates if certain functions encountered any errors
    int errorLevel = 0;

    //Firstly, get the lines and remove any whitespace, while ignoring empty lines. Also store the actual line.  
    vector<pair<int, string>> lines; int lineIndex = 0;
    while (getline(file, line)) {
        ++lineIndex;
        line = TrimWhitespace(line);
        if (line == string()) continue;
        lines.push_back({ lineIndex, line });
    }

    //Secondly, parse the lines and store them in a parsedLines vector.
    vector<pair<int, string>> parsedLines; vector<ControlFlow> statementVec;
    for (const auto&[lineNum, l] : lines) {
        auto parsed = Parse(l);

        if (parsed.size() == 1 && parsed.back().empty())
            continue;

        //After seperating semicolons, trim them to get rid of any whitespace inbetween.
        int index = 0;
        for (const string& x : parsed) {
            ++index;
            //Thirdly, resolve control flow statements
            string parsedLine = TrimWhitespace(x); auto tokens = Tokenize(parsedLine); string statementName = tokens[0];
            if (statementName == "for") {
                //A for loop always has at least 10 tokens
                if (tokens.size() < 12)
                    ExitError("Invalid args in for-loop initialization on line " + to_string(lineNum));

                if (tokens.back() != ":")
                    ExitError("Invalid for-loop initialization. Got '" + tokens.back() + "' Expected: ':' on line " + to_string(lineNum));

                //For each for-loop segment, parse it and also validate the syntax
                string initializer = "", condition = "", iteration = ""; int index = 1;
                for (int i = index; tokens[i] != ","; i++, index++) {
                    if (i == tokens.size() - 1)
                        ExitError("Invalid for-loop initialization. Expected: ',' on line " + to_string(lineNum));
                    initializer += tokens[i] + " ";
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
                string endStatement = iteration + ";jump: FOR_" + to_string(endIndex) + ";=END_" + to_string(endIndex) + ";delete: " + tokens[1] + ";";
                string jumpBegin = "FOR_" + to_string(lineNum); string jumpEnd = "END_" + to_string(lineNum);
                parsedLines.push_back({ lineNum,  "var "+ initializer + ";"});
                parsedLines.push_back({ lineNum,   "=" + jumpBegin + ";"});
                parsedLines.push_back({ lineNum,  "if " + condition + "," + jumpEnd + ";" });
                statementVec.push_back(ControlFlow(lineNum, "for", jumpBegin, jumpEnd, endStatement));
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
                string jumpBegin = "WHILE_" + to_string(lineNum); string jumpEnd = "END_" + to_string(lineNum);
                parsedLines.push_back({ lineNum,  "="  + jumpBegin + ";" });
                parsedLines.push_back({ lineNum,  "if " + condition + "," + jumpEnd + ";" });
                string endStatement = "jump: WHILE_" + to_string(endIndex) + ";=END_" + to_string(endIndex) + ";";
                statementVec.push_back(ControlFlow(lineNum, "while", jumpBegin, jumpEnd, endStatement));
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
                statementVec.push_back(ControlFlow(lineNum, "if", "=END_" + to_string(endIndex) + "; "));
            }
            else if (statementName == "break") {
                if(tokens[1] != ";")
                    ExitError("Expected ';' after break-statement on line " + to_string(lineNum));

                auto found = std::find_if(statementVec.crbegin(), statementVec.crend(), [](const ControlFlow& s) {
                    return s.GetType() != "if" && s.GetType() != "func";
                });

                if(found == statementVec.crbegin())
                    ExitError("A break-statement can only be used within a loop on line " + to_string(lineNum));

                parsedLines.push_back({ lineNum, "jump: " + (*found).GetJumpEnd() + ";" });
            }
            else if (statementName == "continue") {
                if (tokens[1] != ";")
                    ExitError("Expected ';' after continue-statement on line " + to_string(lineNum));

                auto found = std::find_if(statementVec.rbegin(), statementVec.rend(), [](const ControlFlow& s) {
                    return s.GetType() != "if" && s.GetType() != "func";
                });

                if (found == statementVec.rbegin())
                    ExitError("A continue-statement can only be used within a loop on line " + to_string(lineNum));

                //Modify the endStatement accordingly
                found->SetEndStatement("=CONT_" + to_string(lineNum) + ";" + found->GetEndStatement());
                parsedLines.push_back({ lineNum, "jump: CONT_" +  to_string(lineNum) + ";" });
            }
            else if (statementName == "func") {
                string funcName = tokens[1];

                //Function should always have at least 5 tokens
                if (tokens.size() < 5)
                    ExitError("Invalid args in function definition on line " + to_string(lineNum));
                
                if (tokens.back() != ":")
                    ExitError("Expected ':' after function definition on line " + to_string(lineNum));

                if(!statementVec.empty())
                    ExitError("Cannot define a function within another statement on line " + to_string(lineNum));

                //Parse the function arguments
                //3rd index should always be a open bracket
                if(tokens[2] != "(")
                    ExitError("Expected '(' in function definition on line " + to_string(lineNum));

                //2nd to last index should always be a closing bracket
                if (tokens[tokens.size() - 2] != ")")
                    ExitError("Expected ')' in function definition on line " + to_string(lineNum));

                vector<string> args; 

                for (int i = 3; tokens[i] != ")"; i++) {
                    string token = tokens[i]; 
                    auto found = separators.find(token);
                    //Token is an argument
                    if (separators.find(token) == separators.cend()) {
                        args.push_back(token);
                    }
                    //If it is a seperator, it should be a ','
                    else {
                        if (found->first != ",")
                            ExitError("Expected ',' in function argument definition on line " + to_string(lineNum));
                        continue;
                    }
                }

                functions.insert({ funcName, args.size() });

                //Function jump
                parsedLines.push_back({ lineNum, "=" + funcName + ";" });
                //End statement consisting of the deletes for every created variable
                string endStatement = "";
                //Variable predefinitions and pops;
                for (const auto& arg : args) {
                    parsedLines.push_back({ lineNum, "var " + arg + ";" });
                    parsedLines.push_back({ lineNum, "pop: " + arg + ";" });
                    endStatement += "delete:" + arg + ";";
                }
                statementVec.push_back(ControlFlow(lineNum, "func", endStatement + "return;"));
            }
            //A function call has been found
            else if (functions.find(statementName) != functions.cend()) {
                auto function = *functions.find(statementName);

                if (tokens.size() < 4)
                    ExitError("Invalid args provided when calling function on line " + to_string(lineNum));

                if (tokens.back() != ";")
                    ExitError("Expected ';' after calling function on line " + to_string(lineNum));

                //2rd index should always be a open bracket
                if (tokens[1] != "(")
                    ExitError("Expected '(' when calling function on line " + to_string(lineNum));

                //2nd to last index should always be a closing bracket
                if (tokens[tokens.size() - 2] != ")")
                    ExitError("Expected ')' when calling function on line " + to_string(lineNum));

                vector<string> args;

                for (int i = 2; tokens[i] != ")"; i++) {
                    string token = tokens[i];
                    auto found = separators.find(token);
                    //Token is an argument
                    if (separators.find(token) == separators.cend()) {
                        args.push_back(token);
                    }
                    //If it is a seperator, it should be a ','
                    else {
                        if (found->first != ",")
                            ExitError("Expected ',' when calling function on line " + to_string(lineNum));
                        continue;
                    }
                }

                //If function args do not match actual args
                if(function.second != args.size())
                    ExitError("No instance of " + statementName + " takes " + to_string(args.size()) + " arguments on line " + to_string(lineNum));

                //Push each arg to the stack
                for(int i = args.size() - 1; i >= 0; i--)
                    parsedLines.push_back({ lineNum, "push: " + args[i] + ";"});
                //Function calling
                parsedLines.push_back({ lineNum, "call:" + statementName + ";" });
            }
            else if (statementName == "end") {
                if(statementVec.size() == 0)
                    ExitError("Received hanging end-statement on line " + to_string(lineNum));
                //Modify line to the last entry in the statementVec
                parsedLine = statementVec.back().GetEndStatement();

                for (const auto& s : SplitString(statementVec.back().GetEndStatement(), ';'))
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
        ExitError("If-statement did not receive end on line " + to_string(statementVec.front().GetLine()));

    lines.clear();

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
                throw std::exception(("Instruction received undefined identifier '" + value + "'").c_str());
            //If the type is STILL nothing, it is an uninitialized variable
            if(type == ERROR)
                throw std::exception(("Instruction received uninitialized variable '" + value + "'").c_str());
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

    //Returns: Instruction implementation by name and opTypes
    auto FindInstruction = [&instructions](const string& funcName, const OpTypes& types) {
        //Find the Instruction vector given the name
        auto found = instructions.find(funcName);
        if(found == instructions.cend())
            throw std::exception(("Instruction expected, got: '" + funcName + "'").c_str());
        auto funcSet = (*found).second;

        //Get the corresponding Instruction implementation based on the types
        auto func = std::find_if(funcSet.cbegin(), funcSet.cend(), [types](const Instruction& f) {
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
            throw std::exception(("No overload for Instruction '" + funcName + "' matches types: " + error).c_str());
        
        return (*func).GetImplementation();
    };

    instructions.insert({ "print", vector<Instruction> {
        Instruction("print", OpTypes{ COLON, ARG }, [ResolveValue](vector<string> v) {
            string value = v[0]; int valueType = -1; ResolveValue(value, valueType);

            if (valueType == STRING)
                FormatString(value);

            cout << value;
        })
    } });

    instructions.insert({ "printl", vector<Instruction> {
        Instruction("printl", OpTypes{ COLON, ARG }, [ResolveValue](vector<string> v) {
            string value = v[0]; int valueType = -1; ResolveValue(value, valueType);

            if (valueType == STRING)
                FormatString(value);

            cout << value << endl;
        })
    } });

    instructions.insert({ "endl", vector<Instruction> {
        Instruction("endl", OpTypes{}, [ResolveValue](vector<string> v) {
            cout << endl;
        })
    } });

    instructions.insert({ "cls", vector<Instruction> {
        Instruction("cls", OpTypes{}, [ResolveValue](vector<string> v) {
            // Istg this is the best way to do this
            cout << "\033[2J\033[1;1H" << endl;
        })
    } });

    instructions.insert({ "input", vector<Instruction> {
        Instruction("input", OpTypes{ COLON, ARG }, [&](vector<string> v) {
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
        Instruction("input", OpTypes{ COLON, ARG, COMMA, ARG }, [&](vector<string> v) {
            cout << FormatString(v[0]); FindInstruction("input", OpTypes{ COLON, ARG })(vector<string>{v[1]});
        })
    }});

    instructions.insert({ "push", vector<Instruction> {
        Instruction(OpTypes{ COLON, ARG }, [&](vector<string> v) {
            string value = v[0]; int type = -1; ResolveValue(value, type);

            //Push it to the stack
            stack.push(Var(value, type));        
        })
    }});

    instructions.insert({ "pop", vector<Instruction> {
        Instruction(OpTypes{ COLON, ARG }, [&](vector<string> v) {
            string name = v[0], value; int type = -1; 

            if (!FindVar(name, value, type))
                throw std::exception(("Pop received undefined identifier '" + name + "'").c_str());

            if(stack.empty())
                throw std::exception("Tried popping while stack is empty");

            Var top = stack.top(); stack.pop();
            // Uninitialized variable as target, proceed accordingly
            if (type == ERROR) {
                memory[name] = top;
                return;
            }

            if(type != top.GetType())
                throw std::exception(("Pop received wrong type Got: '" + IntToType(type) + "' Expected: '" + IntToType(top.GetType())).c_str());
            //Pop it to the variable
            memory[name].SetData(top.GetData());
        })
    }});

    instructions.insert({ "var", vector<Instruction> {
        Instruction("var", OpTypes{ ARG, SET, ARG }, [&](vector<string> v) {
            string name = v[0]; string value = v[1]; int valueType = 0;
            ResolveValue(value, valueType);

            ValidateVarName(name);
            if (valueType == STRING)
                FormatString(value);

            memory[name] = Var(value, valueType);
        }),
        // Overload: Define variable, but do not initialize it
        Instruction("var", OpTypes{ ARG }, [&](vector<string> v) {
            string name = v[0];
            ValidateVarName(name);

            memory[name] = Var("", ERROR);
        })
    }});

    instructions.insert({ "[VarName]", vector<Instruction> {
        // Sets a variable to a value
        // the final line should look like [VarName] var1 = value, thus having an additional 0 prepended.
        Instruction("[VarName]", OpTypes{ ARG, SET, ARG }, [&](vector<string> v) {
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
        Instruction("[VarName]", OpTypes{ ARG, MOD, ARG }, [&](vector<string> v) {
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
        Instruction("[VarName]", OpTypes{ ARG, MOD }, [&](vector<string> v) {
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
        }),

    }});

    instructions.insert({ "sqrt", vector<Instruction> {
        // Modifying a variable
        Instruction("sqrt", OpTypes{ COLON, ARG }, [&](vector<string> v) {
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

    instructions.insert({ "delete", vector<Instruction> {
        Instruction("delete", OpTypes{ COLON, ARG }, [&](vector<string> v) {
            string name = v[0], value; int type = -1;
            errorLevel = 0;

            if (!FindVar(name, value, type)) {
                errorLevel = 1; return;
            }

            memory.erase(name);
        })
    }});

    instructions.insert({ "exit", vector<Instruction> {
        Instruction("exit", OpTypes{ COLON, ARG }, [ResolveValue, start](vector<string> v) {
            string code = v[0]; int type = 0; ResolveValue(code, type);

            if (GetDataType(code) != INT) throw std::exception(("Exit requires argument type: 'int' got: '" + IntToType(type) + "'").c_str());

            auto end = std::chrono::high_resolution_clock::now();
            cout << endl << "Program sucessfully executed. Exited with code 0." << endl <<
                "Elapsed time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms" << std::endl;
            exit(stoi(code));
        })
    }});

    instructions.insert({ "jump", vector<Instruction> {
        Instruction("jump", OpTypes{ COLON, ARG }, [&](vector<string> v) {
            string name = v[0]; int nameType = GetDataType(name);

            // As labels can only be ErrorTypes, check for that
            if (nameType != ERROR)
                throw std::exception(("Tried to jump to label of type '" + IntToType(nameType) + "'").c_str());

            if (labelMap.find(name) == labelMap.cend())
                throw std::exception(("Tried to jump to undefined label. Got: '" + name + "'").c_str());

            int newLine = labelMap[name];

            //Jump to the new line
            parsedLineIndex = newLine;
        })
    }});

    instructions.insert({ "call", vector<Instruction> {
        Instruction("call", OpTypes{ COLON, ARG }, [&](vector<string> v) {
            string name = v[0]; int nameType = GetDataType(name);

            // As labels can only be ErrorTypes, check for that
            if (nameType != ERROR)
                throw std::exception(("Tried to jump to literal or identifier of type '" + IntToType(nameType) + "'").c_str());

            if (labelMap.find(name) == labelMap.cend())
                throw std::exception(("Tried to jump to undefined label. Got: '" + name + "'").c_str());

            int newLine = labelMap[name];

            // Push current line to callHistory and set index to newLine
            callHistory.push_back(parsedLineIndex);
            parsedLineIndex = newLine;
        })
} });

    instructions.insert({ "return", vector<Instruction> {
        Instruction("return", OpTypes{}, [&](vector<string> v) {
            // Return is equivalent to exit if the callHistory is empty.
            if (callHistory.size() < 1)
                FindInstruction("exit", OpTypes{ COLON, ARG })(vector<string> { "0" });

            // Set current line to latest entry and remove the entry. 
            parsedLineIndex = callHistory.back(); callHistory.pop_back();
        })
    }});

    instructions.insert({ "if", vector<Instruction> {
        Instruction("if", OpTypes{ ARG, LOGIC, ARG, COMMA, ARG }, [&](vector<string> v) {
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
                FindInstruction("jump", OpTypes{ COLON, ARG })(vector<string> { v[3] }); return;
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
                FindInstruction("jump", OpTypes{ COLON, ARG })(vector<string> { v[3] });
            }
        })
    }});

    //Append function names to the blacklist
    for (const auto& a : instructions)
        blacklist.insert(a.first);

    //Vector storing functions on each line. Int stores real line, vector<string> stores arg, function stores implementation
    vector<std::tuple<int, vector<string>, std::function<void(const vector<string>&)>>> instructionVec;

    //Fifth, tokenize each line and go through the actual interpretation process. 
    for(const auto& [lineNum, l] : parsedLines) {
        vector<string> tokens;
        // cout << l << endl;
        //Skip labels
        if (l[0] == '=') {
            //Take into consideration that the location labels point to should be kept the same when actually running the function implementations
            instructionVec.push_back({ -1, vector<string>{}, nullptr });
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
        if (instructions.find(funcName) == instructions.cend()) {
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
            instructionVec.push_back({ lineNum, args, FindInstruction(funcName, argTypes) });
        }
        catch (const std::exception& e) {
            //Specialized error message for [VarName] as it indicates a non-function funcName
            if (funcName == "[VarName]")
                ExitError("No Instruction or identifier by the name '" + tokens[1] + "' found on line " + to_string(lineNum));
            ExitError(string(e.what()) + " on line " + to_string(lineNum));
        }
    }

    //Execute the functions
    for (; parsedLineIndex < instructionVec.size(); parsedLineIndex++) {
        //If parsedLineIndex is on the latest call, delete it to avoid duplicate calls. 
        if (!callHistory.empty() && callHistory.back() == parsedLineIndex)
            callHistory.erase(callHistory.begin() + parsedLineIndex);

        int lineNum = std::get<0>(instructionVec[parsedLineIndex]);
        auto args = std::get<1>(instructionVec[parsedLineIndex]);
        auto func = std::get<2>(instructionVec[parsedLineIndex]);

        //Label
        if (lineNum == -1)
            continue;

        try {
            //Get the function implementation and pass in the args. Index 2 and 1 respectively
            func(args);
        }
        catch (const std::exception& e) {
            ExitError(e.what() + string(" on line ") + to_string(parsedLineIndex));
        }
    }

    //Use the actual exit instruction to exit. Effectively saving 3 lines of code lol
    FindInstruction("exit", OpTypes{ COLON, ARG })(vector<string> { "0" });
   
    file.close();
    return 0;
}