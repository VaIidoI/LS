#include "Archetypes.h"
#include <iostream>
#include <fstream>
#include "Parse.h"
#include <chrono>
#include <stack>
#include <thread>
#include <random>

using std::cout; using std::endl; using std::to_string; using namespace std::chrono;
using std::pair; using std::make_pair; using std::runtime_error;

void ExitError(const string& error) noexcept {
    std::cerr << '\n' << error << "." << endl;
    exit(-1);
}

void ExitError(const string& error, const int& lineNum) noexcept {
    std::cerr << '\n' << error << " on line " << to_string(lineNum) << "." << endl;
    exit(-1);
}

double GenerateRandomDouble() {
    //Random engine. 
    static std::mt19937 rng(static_cast<unsigned int>(steady_clock::now().time_since_epoch().count()));
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(rng);
}

double fast_stod(const string& str) {
    double value;
    std::from_chars(str.data(), str.data() + str.size(), value);
    return value;
}

int fast_stoi(const string& str) {
    int value;
    std::from_chars(str.data(), str.data() + str.size(), value);
    return value;
}

int main(int argc, char* argv[]) {
    if (argc < 1) {
        ExitError("Please specify a path to the file. ");
    }

    std::ifstream file("test.ls"); string line;

    if (!file.is_open()) {
        ExitError("Cannot locate or open file.");
    }
    //Start measuring time
    auto start = high_resolution_clock::now();

    //Predefine a map storing all instructions by name
    std::unordered_map<string, vector<Instruction>> instructions;
    // Also predefine a map storing statements by name
    std::unordered_map<string, vector<ControlStructure>> statements;

    //Predefine a map storing all functions. Stores function name and argument count.
    std::unordered_map<string, int> functions;

    //Predefine the maps which store the variables based on their names.
    std::unordered_map<string, Var> memory; memory.reserve(128);
    //Predefine a stack, storing variables
    vector<Var> stack; stack.reserve(128);

    //Create a list of keywords, which cannot be the names of variables or labels
    std::unordered_set<string> blacklist = { "string", "double", "int", "bool", "errorLevel" };

    //Create a map storing the label's location by name along with a vector storing the line "history"
    //Also create a lineIndex to keep track of the current line.
    std::unordered_map<string, int> labelMap; vector<int> callHistory; callHistory.reserve(128);

    //Create a vector storing all the parsed lines, along with the actual line number 
    vector<pair<int, string>> parsedLines; int parsedLineIndex = 0;

    //Create a vector storing all ControlFlow statement
    vector<ControlStructureData> statementVec;

    //ErrorLevel is a flag that indicates if certain functions encountered any errors
    int errorLevel = 0;

    // Implement all the statement functions here
    statements["for"] = vector<ControlStructure>{
        ControlStructure(TokenTypes{ O_PAREN, ARG, SET, ARG, COMMA, ARG, LOGIC, ARG, COMMA, ARG, MOD, ARG, C_PAREN, O_CURLY  }, [&parsedLines, &statementVec](const vector<string>& v, const int& lineNum) {
            //For each for-loop segment, parse it
            const string initializer = v[0] + "=" + v[1], condition = v[2] + v[3] + v[4], iteration = v[5] + v[6] + v[7];

            //Insert all of the necessary lines 
            string endStatement = iteration + ";jump: FOR_" + to_string(lineNum) + ";=END_" + to_string(lineNum) + ";delete: " + v[0] + ";";
            string jumpBegin = "FOR_" + to_string(lineNum); string jumpEnd = "END_" + to_string(lineNum);
            parsedLines.push_back({ lineNum,  "var " + initializer + ";"});
            parsedLines.push_back({ lineNum,   "=" + jumpBegin + ";"});
            parsedLines.push_back({ lineNum,  "if: " + condition + "," + jumpEnd + ";" });
            statementVec.push_back(ControlStructureData(lineNum, FOR, jumpBegin, jumpEnd, endStatement));
        }),
        //Override: Iteration is incremental
        ControlStructure(TokenTypes{ O_PAREN, ARG, SET, ARG, COMMA, ARG, LOGIC, ARG, COMMA, ARG, MOD, C_PAREN, O_CURLY  }, [&parsedLines, &statementVec](const vector<string>& v, const int& lineNum) {
            //For each for-loop segment, parse it
            string initializer = v[0] + "=" + v[1], condition = v[2] + v[3] + v[4], iteration = v[5] + v[6];

            //Insert all of the necessary lines 
            string endStatement = iteration + ";jump: FOR_" + to_string(lineNum) + ";=END_" + to_string(lineNum) + ";delete: " + v[0] + ";";
            string jumpBegin = "FOR_" + to_string(lineNum); string jumpEnd = "END_" + to_string(lineNum);
            parsedLines.push_back({ lineNum,  "var " + initializer + ";"});
            parsedLines.push_back({ lineNum,   "=" + jumpBegin + ";"});
            parsedLines.push_back({ lineNum,  "if: " + condition + "," + jumpEnd + ";" });
            statementVec.push_back(ControlStructureData(lineNum, FOR, jumpBegin, jumpEnd, endStatement));
        }),
        //Override:  no initializer provided. Iteration is incremental
        ControlStructure(TokenTypes{ O_PAREN, ARG, LOGIC, ARG, COMMA, ARG, MOD, ARG, C_PAREN, O_CURLY  }, [&parsedLines, &statementVec](const vector<string>& v, const int& lineNum) {
            //For each for-loop segment, parse it
            const string condition = v[0] + v[1] + v[2], iteration = v[3] + v[4] + v[5];

            //Insert all of the necessary lines 
            string endStatement = iteration + ";jump: FOR_" + to_string(lineNum) + ";=END_" + to_string(lineNum) + ";";
            string jumpBegin = "FOR_" + to_string(lineNum); string jumpEnd = "END_" + to_string(lineNum);
            parsedLines.push_back({ lineNum,   "=" + jumpBegin + ";"});
            parsedLines.push_back({ lineNum,  "if: " + condition + "," + jumpEnd + ";" });
            statementVec.push_back(ControlStructureData(lineNum, FOR, jumpBegin, jumpEnd, endStatement));
        }),
        //Override: no initializer provided. Iteration is incremental
        ControlStructure(TokenTypes{ O_PAREN, ARG, LOGIC, ARG, COMMA, ARG, MOD, C_PAREN, O_CURLY  }, [&parsedLines, &statementVec](const vector<string>& v, const int& lineNum) {
            //For each for-loop segment, parse it
            const string condition = v[0] + v[1] + v[2], iteration = v[3] + v[4];

            //Insert all of the necessary lines 
            string endStatement = iteration + ";jump: FOR_" + to_string(lineNum) + ";=END_" + to_string(lineNum) + ";";
            string jumpBegin = "FOR_" + to_string(lineNum); string jumpEnd = "END_" + to_string(lineNum);
            parsedLines.push_back({ lineNum,   "=" + jumpBegin + ";"});
            parsedLines.push_back({ lineNum,  "if: " + condition + "," + jumpEnd + ";" });
            statementVec.push_back(ControlStructureData(lineNum, FOR, jumpBegin, jumpEnd, endStatement));
        }),
    };

    statements["while"] = vector<ControlStructure>{
        ControlStructure(TokenTypes{ O_PAREN, ARG, LOGIC, ARG, C_PAREN, O_CURLY  }, [&parsedLines, &statementVec](const vector<string>& v, const int& lineNum) {
            //Parse the condition
            string condition = v[0] + v[1] + v[2];

            string jumpBegin = "WHILE_" + to_string(lineNum); string jumpEnd = "END_" + to_string(lineNum);
            parsedLines.push_back({ lineNum,  "=" + jumpBegin + ";" });
            parsedLines.push_back({ lineNum,  "if: " + condition + "," + jumpEnd + ";" });
            string endStatement = "jump: WHILE_" + to_string(lineNum) + ";=END_" + to_string(lineNum) + ";";
            statementVec.push_back(ControlStructureData(lineNum, WHILE, jumpBegin, jumpEnd, endStatement));
        }),
    };

    statements["if"] = vector<ControlStructure>{
        ControlStructure(TokenTypes{ O_PAREN, ARG, LOGIC, ARG, C_PAREN, O_CURLY  }, [&parsedLines, &statementVec](const vector<string>& v, const int& lineNum) {
            //Parse the condition
            string condition = v[0] + v[1] + v[2];
            //Modify if-statement, in order to jump to the corresponding end if false.
            string line = "if: " + condition + ", END_" + to_string(lineNum) + ";";
            parsedLines.push_back({ lineNum, line });
            statementVec.push_back(ControlStructureData(lineNum, IF, "=END_" + to_string(lineNum) + ";"));
        }),
        //Overload: Check if bool is true
        ControlStructure(TokenTypes{ O_PAREN, ARG, C_PAREN, O_CURLY  }, [&parsedLines, &statementVec](const vector<string>& v, const int& lineNum) {
            //Parse the condition
            string condition = v[0];
            //Modify if-statement, in order to jump to the corresponding end if false.
            string line = "if: " + condition + ", END_" + to_string(lineNum) + ";";
            parsedLines.push_back({ lineNum, line });
            statementVec.push_back(ControlStructureData(lineNum, IF, "=END_" + to_string(lineNum) + ";"));
        }),
        //Overload: Check if bool is false
        ControlStructure(TokenTypes{ O_PAREN, NEG, ARG, C_PAREN, O_CURLY  }, [&parsedLines, &statementVec](const vector<string>& v, const int& lineNum) {
            //Parse the condition
            string condition = "!" + v[0];
            //Modify if-statement, in order to jump to the corresponding end if false.
            string line = "if: " + condition + ", END_" + to_string(lineNum) + ";";
            parsedLines.push_back({ lineNum, line });
            statementVec.push_back(ControlStructureData(lineNum, IF, "=END_" + to_string(lineNum) + ";"));
        })
    };

    statements["else"] = vector<ControlStructure>{
        ControlStructure(TokenTypes{ O_CURLY  }, [&parsedLines, &statementVec](const vector<string>& v, const int& lineNum) {
            if (statementVec.size() == 0)
                throw runtime_error("Hanging else-statement received");
            if (statementVec.back().GetType() != IF)
                throw runtime_error("Else-statement can only be within an if-statement");

            //Mask else as the end jump, while adding a jump to else_end in order to not go into the else if condition is true
            parsedLines.push_back({ lineNum, "jump: ELSE_END_" + to_string(lineNum) + ";" });
            parsedLines.push_back({ lineNum, statementVec.back().GetEndStatement() });
            statementVec.pop_back();
            statementVec.push_back(ControlStructureData(lineNum, ELSE, "=ELSE_END_" + to_string(lineNum) + ";"));
        }),
    };

    statements["break"] = vector<ControlStructure>{
        ControlStructure(TokenTypes{ SEMICOLON }, [&parsedLines, &statementVec](const vector<string>& v, const int& lineNum) {
            auto found = std::find_if(statementVec.crbegin(), statementVec.crend(), [](const ControlStructureData& s) {
                return s.GetType() == FOR || s.GetType() == WHILE;
            });

            if (found == statementVec.crbegin())
                throw runtime_error("A break-statement can only be used within a loop");

            parsedLines.push_back({ lineNum, "jump: " + (*found).GetJumpEnd() + ";" });
        }),
    };

    statements["continue"] = vector<ControlStructure>{
        ControlStructure(TokenTypes{ SEMICOLON  }, [&parsedLines, &statementVec](const vector<string>& v, const int& lineNum) {
            auto found = std::find_if(statementVec.rbegin(), statementVec.rend(), [](const ControlStructureData& s) {
                return s.GetType() == FOR || s.GetType() == WHILE;
            });

            if (found == statementVec.rbegin())
                throw runtime_error("A continue-statement can only be used within a loop");

            //Modify the endStatement accordingly
            found->SetEndStatement("=CONT_" + to_string(lineNum) + ";" + found->GetEndStatement());
            parsedLines.push_back({ lineNum, "jump: CONT_" + to_string(lineNum) + ";" });
        }),
    };

    statements["return"] = vector<ControlStructure>{
        ControlStructure(TokenTypes{ SEMICOLON }, [&parsedLines, &statementVec](const vector<string>& v, const int& lineNum) {
            auto found = std::find_if(statementVec.cbegin(), statementVec.cend(), [](const ControlStructureData& s) {
                return s.GetType() == FUNC;
            });

            if (found == statementVec.cend())
                throw runtime_error("A return-statement can only be used within a function");

            parsedLines.push_back({ lineNum, "return;" });
        }),

        ControlStructure(TokenTypes{ COLON, ARG, SEMICOLON }, [&parsedLines, &statementVec](const vector<string>& v, const int& lineNum) {
            auto found = std::find_if(statementVec.cbegin(), statementVec.cend(), [](const ControlStructureData& s) {
                return s.GetType() == FUNC;
            });

            if (found == statementVec.cend())
                throw runtime_error("A return-statement can only be used within a function");

            parsedLines.push_back({ lineNum, "return: " + v[0] + ";"});
        })
    };

    statements["}"] = vector<ControlStructure>{
        ControlStructure(TokenTypes{ }, [&parsedLines, &statementVec](const vector<string>& v, const int& lineNum) {
            if (statementVec.size() == 0)
                throw runtime_error("Received hanging closing curly bracket");

            for (const auto& s : SplitString(statementVec.back().GetEndStatement(), ';'))
                parsedLines.push_back({ lineNum, s + ";"});

            //Remove the entry in the statementVec
            statementVec.pop_back();
        })
    };

    //Append statement names to the blacklist
    for (const auto& a : statements)
        blacklist.insert(a.first);

    //Firstly, get the lines and remove any whitespace, while ignoring empty lines. Also store the actual line.  
    vector<pair<int, string>> lines; int lineIndex = 0;
    while (getline(file, line)) {
        ++lineIndex;
        line = TrimWhitespace(line);
        if (line == string()) continue;
        lines.push_back({ lineIndex, line });
    }

    //an int value to keep track of the statement index
    int index = 0;
    //Secondly, parse the lines, check for statements and handle them accordingly
    for (const auto& [lineNum, l] : lines) {
        auto parsed = Parse(l);

        if (parsed.size() == 1 && parsed.back().empty())
            continue;

        //Thirdly, resolve control flow statements
        for (const string& x : parsed) {
            ++index;
            //After seperating semicolons, trim them to get rid of any whitespace inbetween.
            string parsedLine = TrimWhitespace(x);

            auto tokens = Tokenize(parsedLine);
            string statementName = tokens[0];
            if (statements.find(statementName) != statements.cend()) {
                vector<string> args; TokenTypes argTypes;
                //For each token, check its token type and push it back to the vector
                for (int i = 1; i < tokens.size(); i++) {
                    string token = tokens[i];
                    //Token is not a seperator, therefore it is an argument
                    auto found = separators.find(token);
                    if (found == separators.cend()) {
                        args.push_back(token); argTypes.push_back(ARG);
                    }
                    //Token is a seperator and has a corresponding token type
                    else {
                        int argType = (*found).second;
                        argTypes.push_back(argType);
                        //As TokenTypes SET and below are unambiguous, do not push them
                        if (argType > SET)
                            args.push_back(token);
                    }
                }

                //Call the function handling the corresponding control statement
                try {
                    auto found = std::find_if(statements[statementName].begin(), statements[statementName].end(), [argTypes](const ControlStructure& s) {
                        return (s.GetTypes() == argTypes);
                    });

                    if (found == statements[statementName].end())
                        ExitError(statementName + " received wrong implementation", lineNum);

                    found->Execute(args, index);
                }
                catch (const std::runtime_error& e) {
                    ExitError(string(e.what()), lineNum);
                }
            }
            else if (statementName == "func") {
                string funcName = tokens[1];

                //Function should always have at least 5 tokens
                if (tokens.size() < 5)
                    ExitError("Invalid args in function definition", lineNum);

                if (tokens.back() != "{")
                    ExitError("Expected '{' after function definition", lineNum);

                if (!statementVec.empty())
                    ExitError("Cannot define a function within another", lineNum);

                //Parse the function arguments
                //2nd index should always be a open bracket
                if (tokens[2] != "(")
                    ExitError("Expected '(' in function definition on line", lineNum);

                //2nd to last index should always be a closing bracket
                if (tokens[tokens.size() - 2] != ")")
                    ExitError("Expected ')' in function definition on line", lineNum);

                vector<string> args; string lastToken = "";

                for (int i = 3; tokens[i] != ")"; i++) {
                    string token = tokens[i]; lastToken = token;
                    auto found = separators.find(token);
                    //Token is an argument
                    if (separators.find(token) == separators.cend()) {
                        args.push_back(token);
                    }
                    //If it is a seperator, it should be a ','
                    else {
                        if (found->first != ",")
                            ExitError("Expected ',' in function argument definition", lineNum);
                        continue;
                    }
                }

                //Make sure the last token was indeed an argument
                if (lastToken == ",")
                    ExitError("Expected argument got ','", lineNum);

                functions.insert({ funcName, args.size() });

                //Jump to end in order to prevent getting into the function through normal line iteration
                parsedLines.push_back({ lineNum, "jump: END_" + to_string(index) + ";" });
                //Function jump
                parsedLines.push_back({ lineNum, "=" + funcName + ";" });
                //End statement consisting of the deletes for every created variable
                string endStatement = "";
                //Variable predefinitions and pops;
                for (const auto& arg : args) {
                    //parsedLines.push_back({ lineNum, "var " + arg + ";" });
                    parsedLines.push_back({ lineNum, "pop: " + arg + ";" });
                    endStatement += "delete:" + arg + ";";
                }
                //push the return back to the statementVec
                statementVec.push_back(ControlStructureData(lineNum, FUNC, endStatement + "return;" + "=END_" + to_string(index) + ";"));
            }
            //A function call has been found
            else if (functions.find(statementName) != functions.cend()) {
                if (tokens.size() < 4)
                    ExitError("Invalid args provided when calling function", lineNum);

                if (tokens.back() != ";")
                    ExitError("Expected ';' after calling function", lineNum);

                //Predefine 2 vectors. FuncArgs stores function calls and the arguments.
                //FuncHistory stores the hierarchy of function calls.
                std::list<Function> funcArgs; vector<Function*> funcHistory; int lastIndex = 0;
                //When funcHistory is empty, stop the for loop. Has to execute at least once
                for (int i = 0; !funcHistory.empty() || i < 1; i++, lastIndex++) {
                    string token = tokens[i];

                    //If the token is a function, act accordingly
                    if (functions.find(token) != functions.cend()) {
                        //If a nested function is within the current function, replace arg with a placeholder
                        if (!funcHistory.empty()) {
                            //add placeholder arg
                            funcHistory.back()->AddArg("func"); lastIndex++;
                        }

                        //Push a new function call and store function reference in the history
                        funcArgs.push_back({ token, vector<string>() });
                        funcHistory.push_back(&funcArgs.back());

                        //Make sure this isn't the last token
                        if (i >= tokens.size() - 2)
                            ExitError("Expected '(' when calling function", lineNum);

                        //If the next token isn't a '(', throw an error
                        if (tokens[i + 1] != "(")
                            ExitError("Expected '(' when calling function", lineNum);

                        //Skip over the next token, as we have confirmed it is a '('
                        i++;; continue;
                    }

                    //As we skip over valid '(', they should not be here
                    if (token == "(")
                        ExitError("Hanging '(' received in function call", lineNum);

                    if (token == ")") {
                        //Make sure functionHistory is not empty
                        if (funcHistory.empty())
                            ExitError("Hanging ')' received in function call", lineNum);

                        //Make sure function call doesn't end with ','
                        if (tokens[i - 1] == ",")
                            ExitError("Expected argument got ','", lineNum);
                        funcHistory.pop_back();
                        continue;
                    }

                    //If the token is not a seperator, it is an argument
                    if (separators.find(token) == separators.cend()) {
                        //Push the arg to the current function
                        funcHistory.back()->AddArg(token);
                    }
                    //if it is a seperator, make sure it is a ','
                    else if (token != ",")
                        ExitError("Expected: ',' Got '" + token + "' when calling function", lineNum);
                }

                //Make sure function call doesn't end with ','
                if (tokens[lastIndex - 1] == ",")
                    ExitError("Expected argument got ','", lineNum);

                //Make sure function call ends with ';' or '>>'
                if (tokens[lastIndex + 1] != ";" && tokens[lastIndex + 1] != ">>")
                    ExitError("Expected end of function call, got '" + tokens[lastIndex] + "'", lineNum);

                //Reverse functions, as we want to resolve the inner functions first
                reverse(funcArgs.begin(), funcArgs.end());

                for (const auto& func : funcArgs) {
                    auto function = *functions.find(func.GetName());
                    //Reverse function args too, as they get taken out of the stack backwards
                    auto args = func.GetArgs(); std::reverse(args.begin(), args.end());
                    //If function args do not match actual args
                    if (function.second != args.size())
                        ExitError("No instance of " + function.first + " takes " + to_string(args.size()) + " arguments", lineNum);
                    //Push each arg to the stack
                    for (const string& arg : args) {
                        //Ignore if arg is placeholder
                        if (arg == "func") continue;
                        parsedLines.push_back({ lineNum, "push: " + arg + ";" });
                    }
                    parsedLines.push_back({ lineNum, "call: " + func.GetName() + ";" }); //Call the function
                }

                //Function return is getting assigned to a variable 
                //the next 4 indices represent the variable assigning
                if (lastIndex + 4 == tokens.size()) {
                    if (tokens[lastIndex + 1] != ">>")
                        ExitError("Expected: '>>' Got: '" + tokens[lastIndex + 1] + "'", lineNum);
                    parsedLines.push_back({ lineNum, "pop: " + tokens[lastIndex + 2] + ";" });
                }
                //If the value is below that, clear the stack, since the pushed return value won't be used, causing a memory leak
                else if(lastIndex + 4 > tokens.size())
                    parsedLines.push_back({ lineNum, "pop;" });
                //Throw an error if the value is above that
                else if (lastIndex + 4 < tokens.size())
                    ExitError("Invalid args provided when calling function", lineNum);
            }
            else 
                parsedLines.push_back({ lineNum, parsedLine });
        }
    }

    //If any statements are still in vec, no end was received.
    if (statementVec.size() > 0)
        ExitError("If-statement did not receive end", statementVec.front().GetLine());

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

    // Function that searches through memory and returns iterator to a variable given a name
    auto FindVar = [&memory](const std::string& varName) {
        auto found = memory.find(varName);
        if (found != memory.cend()) {
            return found;
        }

        throw runtime_error("Instruction received undefined identifier '" + varName + "'");
    };

    // Function that returns a var object for a specific value. If value is a variable it returns that var 
    auto ResolveValue = [&memory](const std::string& value) {
        // Get the type based on data
        int type = GetDataType(value);

        // If the type is still nothing, perhaps it is a variable
        if (type == ERROR) {
            //In case of it being a variable, the value acts as a name
            auto found = memory.find(value);
            if (found == memory.cend())
                throw std::runtime_error("Instruction received undefined identifier '" + value + "'");
            type = found->second.GetType();
            // If the type is STILL nothing, it is an uninitialized variable
            if (type == ERROR)
                throw std::runtime_error("Instruction received uninitialized variable '" + value + "'");

            return found->second;
        }

        switch (type) {
            case STRING: {
                return Var(value);
            }
            case DOUBLE: {
                return Var(fast_stod(value));
            }
            case INT: {
                return Var(fast_stoi(value));
            }
            case BOOL: {
                if (value == "true")
                    return Var(true);
                else
                    return Var(false);
            }
            default: break;
        }
    };

    auto ValidateVarName = [&memory, &blacklist](const std::string& varName) {
        // Check for invalid characters and digit-only names
        bool isAllDigits = true;
        for (const char& c : varName) {
            if (!isalnum(c) && c != '_') {
                throw std::runtime_error("Variable initialization received a name with an invalid character. Got: '" + std::string(1, c) + "'");
            }
            if (!isdigit(c)) {
                isAllDigits = false;
            }
        }

        // Name should not be in blacklist
        if (blacklist.find(varName) != blacklist.cend()) {
            throw std::runtime_error("Variable initialization received illegal identifier. Got: '" + varName + "'");
        }

        // Name should not be all numbers
        if (isAllDigits) {
            throw std::runtime_error("Variable initialization received digit-only name. Got: '" + varName + "'");
        }

        // Name should be unique
        if (memory.find(varName) != memory.cend()) {
            throw std::runtime_error("Variable by the name of '" + varName + "' already defined");
        }
        };

    //Returns: Instruction implementation by name and TokenTypes
    auto FindInstruction = [&instructions](const std::string& funcName, const TokenTypes& types) {
        // Find the Instruction vector given the name
        const auto found = instructions.find(funcName);
        if (found == instructions.cend())
            throw std::runtime_error("Instruction expected, got: '" + funcName + "'");

        const auto funcSet = found->second;

        // Get the corresponding Instruction implementation based on the types
        const auto func = std::find_if(funcSet.cbegin(), funcSet.cend(), [&types](const Instruction& f) {
            return f.GetTypes() == types;
            });

        if (func == funcSet.cend()) {
            // Build the error message
            std::string error;
            for (const auto& a : types)
                error += "'" + IntToTokenType(a) + "', ";

            // Remove the trailing comma and space
            if (!error.empty()) {
                error.pop_back();
                error.pop_back();
            }

            throw std::runtime_error("No overload for Instruction '" + funcName + "' matches types: " + error);
        }

        return func->GetImplementation();
    };

    auto FindInstructionA = [&instructions](const std::string& funcName) {
        // Find the Instruction vector given the name
        const auto found = instructions.find(funcName);
        if (found == instructions.cend())
            throw std::runtime_error("Instruction expected, got: '" + funcName + "'");

        //Return the first implementation of the function with a given name
        return found->second.begin()->GetImplementation();
    };

    instructions["print"] = vector<Instruction>{
        Instruction(TokenTypes{ COLON, ARG }, [ResolveValue](const Arguments& v) {
            const Var& var1 = ResolveValue(v[0]); int type = var1.GetType();

            switch (type) {
                case STRING: {
                    auto val1 = get<string>(var1.GetData()); FormatString(val1);
                    cout << val1;
                    break;
                }
                case DOUBLE: {
                    auto val1 = get<double>(var1.GetData());
                    cout << (val1);
                    break;
                }
                case INT: {
                    auto val1 = get<int>(var1.GetData());
                    cout << to_string(val1);
                    break;
                }
                case BOOL: {
                    auto val1 = get<bool>(var1.GetData());
                    if (val1)
                        cout << "true";
                    else
                        cout << "false";
                    break;
                }
                default: break;
            }
        })
    };

    instructions["printl"] = vector<Instruction>{
        Instruction(TokenTypes{ COLON, ARG }, [FindInstructionA](const Arguments& v) {
            FindInstructionA("print")(vector<string> { v[0] });
            cout << "\n";
        })
    };

    instructions["endl"] = vector<Instruction>{
        Instruction(TokenTypes{}, [ResolveValue](const Arguments& v) {
            cout << endl;
        })
    };

    instructions["cls"] = vector<Instruction>{
        Instruction(TokenTypes{}, [ResolveValue](const Arguments& v) {
            // Istg this is the best way to do this
            cout << "\033[2J\033[1;1H" << endl;
        })
    };

    instructions["input"] = vector<Instruction>{
        Instruction(TokenTypes{ COLON, ARG }, [&](const Arguments& v) {
            Var& var1 = FindVar(v[0])->second; int type = var1.GetType();
            // Reset errorLevel to 0 
            errorLevel = 0;

            // Get the line and its datatype. If it's errortype, it becomes a string, due to it not being anything else
            string s = ""; std::getline(std::cin, s); int lineType = GetDataType(s);

            if (lineType == ERROR)
                lineType = STRING;

            // Uninitialized variable as target, set it to the lineType
            if (type == ERROR)
                type = lineType;

            // Set errorLevel to 1, indicating a type mismatch, unless type is a string, due to strings being everything theoretically.  
            if (type != lineType && type != STRING) {
                errorLevel = 1; return;
            }

            //Set the variable to the line
            switch (type) {
                case STRING: {
                    var1.SetData(line);
                    break;
                }
                case DOUBLE: {
                    var1.SetData(fast_stod(line));
                    break;
                }
                case INT: {
                    var1.SetData(fast_stoi(line));
                    break;
                }
                case BOOL: {
                    if (line == "true")
                        var1.SetData(true);
                    else
                        var1.SetData(false);
                    break;
                }
                default: break;
            }
        }),
        // Overload: Print a string before inputting. 
        Instruction(TokenTypes{ COLON, ARG, COMMA, ARG }, [&](const Arguments& v) {
            cout << FormatStringA(v[0]); FindInstruction("input", TokenTypes{ COLON, ARG })(Arguments { v[1] });
        })
    };

    instructions["push"] = vector<Instruction>{
        Instruction(TokenTypes{ COLON, ARG }, [&stack, ResolveValue](const Arguments& v) {
            const Var& var1 = ResolveValue(v[0]);

            if (var1.GetType() == ERROR)
                throw runtime_error("Tried pushing uninitialized variable '" + v[0] + "' onto stack");

            // Push it to the stack
            stack.emplace_back(var1);
        })
    };

    instructions["pop"] = vector<Instruction>{
        Instruction(TokenTypes{ COLON, ARG }, [&memory, FindVar, &stack, &errorLevel](const Arguments& v) {
            // Reset errorLevel
            errorLevel = 0;
            auto it = memory.find(v[0]);

            if (stack.empty()) {
                errorLevel = 1; return;
            }

            //Variable does not exist, initialize it. 
            if (it == memory.cend()) {
                memory[v[0]] = stack.back(); stack.pop_back();
                return;
            }

            //Get the top of the stack
            const Var& top = stack.back();
            stack.pop_back();


            const int& topType = top.GetType();
            const Data& topData = top.GetData();
            //Get the variable from the iterator.
            Var& var1 = it->second; int type = var1.GetType();

            // Uninitialized variable as target, set type to topType
            if (type == ERROR)
                type = topType;

            if (type != topType)
                throw runtime_error("Pop received wrong type Got: '" + IntToType(type) + "' Expected: '" + IntToType(topType));

            switch (type) {
                case STRING: {
                    std::string val1 = std::get<std::string>(topData);
                    FormatString(val1);
                    var1.SetData(std::move(val1));
                    break;
                }
                case DOUBLE: {
                    var1.SetData(std::get<double>(topData));
                    break;
                }
                case INT: {
                    var1.SetData(std::get<int>(topData));
                    break;
                }
                case BOOL: {
                    var1.SetData(std::get<bool>(topData));
                    break;
                }
                default: break;
            }
        }),
        Instruction(TokenTypes{ }, [&stack](const Arguments& v) {
            stack.clear();
        })
    };

    instructions["var"] = vector<Instruction>{
        Instruction(TokenTypes{ ARG, SET, ARG }, [&](const Arguments& v) {
            const string& name = v[0];
            const Var& var1 = ResolveValue(v[1]); int type = var1.GetType();

            ValidateVarName(name);
            switch (type) {
                case STRING: {
                    memory[name] = Var(FormatStringA(get<string>(var1.GetData())));
                    break;
                }
                case DOUBLE: {
                    memory[name] = Var(get<double>(var1.GetData()));
                    break;
                }
                case INT: {
                    memory[name] = Var(get<int>(var1.GetData()));
                    break;
                }
                case BOOL: {
                    memory[name] = Var(get<bool>(var1.GetData()));
                    break;
                }
                default: break;
            }
        }),
        // Overload: Define variable, but do not initialize it
        Instruction(TokenTypes{ ARG }, [&memory, ValidateVarName](const Arguments& v) {
            const string& name = v[0];
            ValidateVarName(name);

            memory[name] = Var();
        })
    };

    instructions["exit"] = vector<Instruction>{
        Instruction(TokenTypes{ COLON, ARG }, [ResolveValue, start](const Arguments& v) {
            const Var& var1 = ResolveValue(v[0]); int type = var1.GetType();

            if (type != INT) throw runtime_error(("Exit requires argument type: 'int' got: '" + IntToType(type) + "'").c_str());
            int code = get<int>(var1.GetData());

            cout << endl << "Program sucessfully executed. Exited with code " + to_string(code) + "." << endl <<
                "Elapsed time: " << duration_cast<milliseconds>(high_resolution_clock::now() - start).count() << " ms" << endl;

            exit(code);
        })
    };

    auto ModifyVar = [](Var& var1, const auto& val1, const auto& val2, const string& op) {
        if (op == "+=")
            var1.SetData(val1 + val2);
        else if (op == "-=")
            var1.SetData(val1 - val2);
        else if (op == "*=")
            var1.SetData(val1 * val2);
        else if (op == "/=") {
            if (val2 == 0.0)
                throw runtime_error("Division by 0 attempted");

            var1.SetData(val1 / val2);
        }
        else if (op == "%=") {
            // Make sure modulo is only used with integers
            if constexpr (std::is_integral_v<std::decay_t<decltype(val1)>> && std::is_integral_v<std::decay_t<decltype(val2)>>) {
                if (val2 == 0) {
                    throw runtime_error("Modulo by 0 attempted");
                }
                var1.SetData(val1 % val2);
            }
            else
                throw runtime_error("Modulo operation is only valid for integral types");
        }
    };

    instructions["[VarName]"] = vector<Instruction>{
        // Sets a variable to a value
        // the final line should look like [VarName] var1 = value, thus having an additional 0 prepended.
        Instruction(TokenTypes{ ARG, SET, ARG }, [&](const Arguments& v) {
            Var& var0 = FindVar(v[0])->second;
            const Var& var1 = ResolveValue(v[1]);
            int type = var1.GetType();

            switch (type) {
                case STRING: {
                    var0.SetData(FormatStringA(get<string>(var1.GetData())));
                    break;
                }
                case DOUBLE: {
                    var0.SetData(get<double>(var1.GetData()));
                    break;
                }
                case INT: {
                    var0.SetData(get<int>(var1.GetData()));
                    break;
                }
                case BOOL: {
                    var0.SetData(get<bool>(var1.GetData()));
                    break;
                }
                default: break;
            }
        }),
        //Modifying a variable
        Instruction(TokenTypes{ ARG, MOD, ARG }, [&](const Arguments& v) {
            Var& var1 = FindVar(v[0])->second; int nameType = var1.GetType();
            const Var& var2 = ResolveValue(v[2]); int valueType = var2.GetType();
            const string& op = v[1];


            // If it isn't the same type, or number type.
            if (nameType != valueType && !(nameType == DOUBLE && valueType == INT) && !(nameType == INT && valueType == DOUBLE))
                throw runtime_error(("Arithmetic operation received wrong type. Got: '" + IntToType(valueType) + "' Expected: '" + IntToType(nameType) + "'").c_str());

            if (nameType == BOOL)
                throw runtime_error("Cannot perform arithmetic operation on type 'bool'");

            if (nameType == STRING && op != "+=")
                throw runtime_error(("Cannot use operator '" + op + "' on a string").c_str());
            else if (nameType == STRING) {
                auto val1 = get<string>(var1.GetData());
                auto val2 = get<string>(var2.GetData());
                var1.SetData(val1 + FormatStringA(val2));
                return;
            }

            switch (nameType) {
                case DOUBLE: {
                    auto val1 = get<double>(var1.GetData());
                    if (valueType == DOUBLE) {
                        auto val2 = get<double>(var2.GetData());
                        ModifyVar(var1, val1, val2, op);
                    }
                    else {
                        auto val2 = get<int>(var2.GetData());
                        ModifyVar(var1, val1, val2, op);
                    }
                    break;
                }
                case INT: {
                    auto val1 = get<int>(var1.GetData());
                    if (valueType == DOUBLE) {
                        auto val2 = get<double>(var2.GetData());
                        ModifyVar(var1, val1, val2, op);
                    }
                    else {
                        auto val2 = get<int>(var2.GetData());
                        ModifyVar(var1, val1, val2, op);
                    }
                    break;
                }
                default: break;
            }
        }),
        // Incrementing or decrementing variable
        Instruction(TokenTypes{ ARG, MOD }, [&](const Arguments& v) {
            Var& var1 = FindVar(v[0])->second; int type = var1.GetType();
            const string& op = v[1];

            if (op != "++" && op != "--")
                throw runtime_error("Wrong operator received. Expected '++' or '--'");

            switch (type) {
                case DOUBLE: {
                    auto val1 = get<double>(var1.GetData());
                    if (op == "++")
                        var1.SetData(val1 + 1.0);
                    else if (op == "--")
                        var1.SetData(val1 - 1.0);
                    break;
                }
                case INT: {
                    auto val1 = get<int>(var1.GetData());
                    if (op == "++")
                        var1.SetData(val1 + 1);
                    else if (op == "--")
                        var1.SetData(val1 - 1);
                    break;
                }
                default: throw runtime_error("Cannot use operator '" + op + "' on type '" + IntToType(type) + "'"); break;
            }
        })
    };

    instructions["sqrt"] = vector<Instruction>{
        Instruction(TokenTypes{ COLON, ARG }, [&](const Arguments& v) {
            Var& var1 = FindVar(v[0])->second; int nameType = var1.GetType();

            // If it isn't the same type, or number type.
            if (nameType != DOUBLE && nameType != INT)
                throw runtime_error(("Square root operation received wrong type. Got: '" + IntToType(nameType) + "'").c_str());

            switch (nameType) {
                case DOUBLE: {
                    auto val1 = get<double>(var1.GetData());
                    var1.SetData(sqrt(val1));
                    break;
                }
                case INT: {
                    auto val1 = get<int>(var1.GetData());
                    var1.SetData((int)sqrt(val1));
                    break;
                }
            }
        })
    };

    instructions["abs"] = vector<Instruction>{
        Instruction(TokenTypes{ COLON, ARG }, [&](const Arguments& v) {
            Var& var1 = FindVar(v[0])->second; int nameType = var1.GetType();

            // If it isn't the same type, or number type.
            if (nameType != DOUBLE && nameType != INT)
                throw runtime_error(("Absolute operation received wrong type. Got: '" + IntToType(nameType) + "'").c_str());

            switch (nameType) {
                case DOUBLE: {
                    auto val1 = get<double>(var1.GetData());
                    var1.SetData(abs(val1));
                    break;
                }
                case INT: {
                    auto val1 = get<int>(var1.GetData());
                    var1.SetData(abs(val1));
                    break;
                }
            }
        })
    };

    //Gives random double between 0 and 1
    instructions["rand"] = vector<Instruction>{
        Instruction(TokenTypes{ COLON, ARG }, [&](const Arguments& v) {
            Var& var1 = FindVar(v[0])->second;
            var1.SetData(GenerateRandomDouble());
        })
    };

    //Function that gives the elapsed time in milliseconds since the program started
    instructions["millis"] = vector<Instruction>{
        Instruction(TokenTypes{ COLON, ARG }, [&](const Arguments& v) {
            Var& var1 = FindVar(v[0])->second;
            var1.SetData((int)duration_cast<milliseconds>(high_resolution_clock::now() - start).count());
        })
    };

    //Function that gives the elapsed time in seconds since the program started
    instructions["seconds"] = vector<Instruction>{
        Instruction(TokenTypes{ COLON, ARG }, [&](const Arguments& v) {
            Var& var1 = FindVar(v[0])->second;
            var1.SetData(duration<double>(high_resolution_clock::now() - start).count());
        })
    };

    instructions["delay"] = vector<Instruction>{
        Instruction(TokenTypes{ COLON, ARG }, [&](const Arguments& v) {
            const Var& var1 = ResolveValue(v[0]); int nameType = var1.GetType();

            // If it isn't the same type, or number type.
            if (nameType != DOUBLE && nameType != INT)
                throw runtime_error(("Delay received wrong type. Got: '" + IntToType(nameType) + "'").c_str());

            switch (nameType) {
                case DOUBLE: {
                    auto val1 = get<double>(var1.GetData());
                    std::this_thread::sleep_for(milliseconds((int)val1));
                    break;
                }
                case INT: {
                    auto val1 = get<int>(var1.GetData());
                    std::this_thread::sleep_for(milliseconds(val1));
                    break;
                }
            }
        })
    };

    instructions["delete"] = vector<Instruction>{
        Instruction(TokenTypes{ COLON, ARG }, [&memory, &errorLevel](const Arguments& v) {
            const string& name = v[0]; errorLevel = 0;
            const auto it = memory.find(name);
            if (it == memory.cend()) {
                errorLevel = 1; return;
            }

            memory.erase(it);
        })
    };

    instructions["jump"] = vector<Instruction>{
        Instruction(TokenTypes{ COLON, ARG }, [&labelMap, &parsedLineIndex](const Arguments& v) {
            const string& name = v[0];
            auto iter = labelMap.find(name);
            if (iter == labelMap.cend())
                throw runtime_error(("Tried to jump to undefined label. Got: '" + name + "'").c_str());

            // Jump to the new line
            parsedLineIndex = iter->second;
        })
    };

    instructions["call"] = std::vector<Instruction>{
        Instruction(TokenTypes{ COLON, ARG }, [&labelMap, &callHistory, &parsedLineIndex](const Arguments& v) {
            // Push current line to callHistory
            callHistory.emplace_back(parsedLineIndex);
            const string& name = v[0];
            const auto iter = labelMap.find(name);
            if (iter == labelMap.cend())
                throw runtime_error(("Tried to jump to undefined label. Got: '" + name + "'").c_str());

            // Jump to the new line
            parsedLineIndex = iter->second;
        })
    };

    instructions["return"] = vector<Instruction>{
        Instruction(TokenTypes{}, [&callHistory, FindInstructionA, &parsedLineIndex](const Arguments& v) {
            // Return is equivalent to exit if the callHistory is empty.
            if (callHistory.size() < 1)
                FindInstructionA("exit")(vector<string> { "0" });

            // Set current line to latest entry and remove the entry. 
            parsedLineIndex = callHistory.back(); callHistory.pop_back();
        }),
        // Override: Return a variable
        Instruction(TokenTypes{ COLON, ARG }, [&callHistory, FindInstructionA, &parsedLineIndex](const Arguments& v) {
            const string& name = v[0];

            // Push the variable to the stack
            FindInstructionA("push")(vector<string> { name });
            // Also delete the function
            FindInstructionA("delete")(vector<string> { name });

            // Set current line to latest entry and remove the entry. 
            parsedLineIndex = callHistory.back(); callHistory.pop_back();
        })
    };

    instructions["if"] = vector<Instruction>{
        Instruction(TokenTypes{ COLON, ARG, LOGIC, ARG, COMMA, ARG }, [&](const Arguments& v) {
            const Var& var1 = ResolveValue(v[0]), var2 = ResolveValue(v[2]);
            int value1Type = var1.GetType(), value2Type = var2.GetType();

            // First figure out which exact operator is being used
            int op = (v[1] == "==") ? 0 : (v[1] == "!=") ? 1 : (v[1] == "<") ? 2 : (v[1] == ">") ? 3 : (v[1] == "<=") ? 4 : (v[1] == ">=") ? 5 : -1;

            if (op == -1)
                throw std::runtime_error("Invalid operator: " + v[1]);
            
            // Check for types. Compare doubles and ints
            if (value1Type != value2Type && !(value1Type == DOUBLE && value2Type == INT) && !(value1Type == INT && value2Type == DOUBLE))
                throw std::runtime_error("Comparing different types. Type1: '" + IntToType(value1Type) + "' Type2: '" + IntToType(value2Type) + "'");
            

            // Handle equality and inequality first
            if (op == 0 || op == 1) {
                if (op == 0 && var1.GetData() != var2.GetData())
                    FindInstructionA("jump")({ v[3] });
                if (op == 1 && var1.GetData() == var2.GetData())
                    FindInstructionA("jump")({ v[3] });
                return;
            }

            // Make sure strings and bools cannot be compared relationally
            if (value1Type == STRING || value1Type == BOOL) {
                throw std::runtime_error("Cannot use relational operators on Type: '" + IntToType(value1Type) + "'");
            }

            bool jump = false;
            if (value1Type == DOUBLE || value1Type == INT) {
                //Get the values. Taking into account that doubles can be compared to ints.
                double val1 = (value1Type == DOUBLE) ? get<double>(var1.GetData()) : (double)(get<int>(var1.GetData()));
                double val2 = (value2Type == DOUBLE) ? get<double>(var2.GetData()) : (double)(get<int>(var2.GetData()));

                // Relational operators
                switch (op) {
                    case 2: // <
                        jump = (val1 >= val2);
                        break;
                    case 3: // >
                        jump = (val1 <= val2);
                        break;
                    case 4: // <=
                        jump = (val1 > val2);
                        break;
                    case 5: // >=
                        jump = (val1 < val2);
                        break;
                }
            }

            // Jump to line if condition isn't met
            if (jump) 
                FindInstructionA("jump")({ v[3] });
        }),
        // Override: If bool is true or variable is initialized
        Instruction(TokenTypes{ COLON, ARG, COMMA, ARG }, [&](const Arguments& v) {
            const Var& var1 = FindVar(v[0])->second; int type = var1.GetType();

            // If the type is bool and it isn't true or if the type is errorType, jump to end
            if(type == BOOL && !get<bool>(var1.GetData()))
                FindInstructionA("jump")(vector<string> { v[1] });
            if(type == ERROR)
                FindInstructionA("jump")(vector<string> { v[1] });
        }),
        // Override: If bool is true or variable is initialized
        Instruction(TokenTypes{ COLON, NEG, ARG, COMMA, ARG }, [&](const Arguments& v) {
            const Var& var1 = FindVar(v[0])->second; int type = var1.GetType();

            // If the type is bool and it is true or if the type isn't errorType, jump to end
            if (type == BOOL && get<bool>(var1.GetData()))
                FindInstructionA("jump")(vector<string> { v[1] });
            else if (type != ERROR && type != BOOL)
                FindInstructionA("jump")(vector<string> { v[1] });
        })
    };

    //Append instruction names to the blacklist
    for (const auto& a : instructions)
        blacklist.insert(a.first);

    //Vector storing functions on each line. Int stores real line, vector<string> stores arg, function stores implementation
    vector<InstructionHandle> instructionVec;

    //Fifth, tokenize each line and go through the actual interpretation process. 
    for (const auto& [lineNum, l] : parsedLines) {
        vector<string> tokens;

        //Skip labels
        if (l[0] == '=') {
            //Take into consideration that the location labels point to should be kept the same when actually running the function implementations
            instructionVec.push_back({ -1, Arguments{}, nullptr });
            continue;
        }
        try {
            tokens = Tokenize(l);
        }
        catch (const std::runtime_error& e) {
            ExitError(string(e.what()), lineNum);
        }

        //Check if semicolon is found
        if (l.back() != ';') {
            ExitError("Missing semicolon", lineNum);
        }

        string funcName = tokens[0];

        //If funcName is not a function, perhaps it is an identifier. Prepend [VarName] and set it as the function name. 
        if (instructions.find(funcName) == instructions.cend()) {
            tokens.insert(tokens.begin(), "[VarName]"); funcName = "[VarName]";
        }

        vector<string> args; TokenTypes argTypes;
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
                //As TokenTypes SET and below are unambiguous, do not push them
                if (argType > SET)
                    args.push_back(token);
            }
        }

        //Store the instruction in the instruction vector
        try {
            auto foundImplementation = FindInstruction(funcName, argTypes);
            instructionVec.push_back({ lineNum, args, foundImplementation });
        }
        catch (const std::runtime_error& e) {
            //Specialized error message for [VarName] as it indicates a non-instruction funcName
            if (funcName == "[VarName]")
                ExitError("No Instruction or identifier by the name '" + tokens[1] + "' found", lineNum);
            ExitError(string(e.what()), lineNum);
        }
    }

    //Execute the functions
    for (; parsedLineIndex < instructionVec.size(); parsedLineIndex++) {
        //If parsedLineIndex is on the latest call, delete it to avoid duplicate calls. 
        if (!callHistory.empty() && callHistory.back() == parsedLineIndex)
            callHistory.erase(callHistory.begin() + parsedLineIndex);

        int lineNum = instructionVec[parsedLineIndex].GetLine();

        //Label
        if (lineNum == -1)
            continue;

        try {
            //Get the function implementation and pass in the args. Index 2 and 1 respectively
            instructionVec[parsedLineIndex].Execute();
        }
        catch (const std::runtime_error& e) {
            ExitError(e.what(), lineNum);
        }
    }

    //Use the actual exit instruction to exit. Effectively saving 3 lines of code lol
    FindInstructionA("exit")( Arguments{ "0" });

    file.close();
    return 0;
}