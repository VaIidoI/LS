#include "Archetypes.h"
#include "Grammar.h"
#include <iostream>
#include <fstream>
#include "Parse.h"
#include <chrono>
#include <stack>

using std::cout; using std::endl; using std::to_string; using std::pair; using std::make_pair; using std::runtime_error;

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

    //Predefine a map storing all instructions by name
    std::unordered_map<string, vector<Instruction>> instructions;
    // Also predefine a map storing statements by name
    std::unordered_map<string, vector<ControlStructure>> statements;

    //Predefine a map storing all functions. Stores function name and argument count.
    std::unordered_map<string, int> functions;

    //Predefine the maps which store the variables based on their names.
    std::unordered_map<string, Var> memory;
    //Predefine a stack, storing variables
    std::deque<Var> stack;

    //Create a list of keywords, which cannot be the names of variables or labels
    std::unordered_set<string> blacklist = { "string", "double", "int", "bool", "errorLevel" };

    //Create a map storing the label's location by name along with a vector storing the line "history"
    //Also create a lineIndex to keep track of the current line.
    std::unordered_map<string, int> labelMap; vector<int> callHistory; 
    
    //Create a vector storing all the parsed lines, along with the actual line number 
    vector<pair<int, string>> parsedLines; int parsedLineIndex = 0;

    //Create a vector storing all ControlFlow statement
    vector<ControlStructureData> statementVec;

    //ErrorLevel is a flag that indicates if certain functions encountered any errors
    int errorLevel = 0;

    // Implement all the statement functions here
    statements["for"] = vector<ControlStructure>{
        ControlStructure(TokenTypes{ O_PAREN, ARG, SET, ARG, COMMA, ARG, LOGIC, ARG, COMMA, ARG, MOD, ARG, C_PAREN, COLON  }, [&parsedLines, &statementVec](const vector<string>& v, const int& lineNum) {
            //For each for-loop segment, parse it
            string initializer = v[1] + "=" + v[2], condition = v[3] + v[4] + v[5], iteration = v[6] + v[7] + v[8];

            //Insert all of the necessary lines 
            string endStatement = iteration + ";jump: FOR_" + to_string(lineNum) + ";=END_" + to_string(lineNum) + ";delete: " + v[1] + ";";
            string jumpBegin = "FOR_" + to_string(lineNum); string jumpEnd = "END_" + to_string(lineNum);
            parsedLines.push_back({ lineNum,  "var " + initializer + ";"});
            parsedLines.push_back({ lineNum,   "=" + jumpBegin + ";"});
            parsedLines.push_back({ lineNum,  "if: " + condition + "," + jumpEnd + ";" });
            statementVec.push_back(ControlStructureData(lineNum, FOR, jumpBegin, jumpEnd, endStatement));
        }),
        //Override: Iteration is incremental
        ControlStructure(TokenTypes{ O_PAREN, ARG, SET, ARG, COMMA, ARG, LOGIC, ARG, COMMA, ARG, MOD, C_PAREN, COLON  }, [&parsedLines, &statementVec](const vector<string>& v, const int& lineNum) {
            //For each for-loop segment, parse it
            string initializer = v[1] + "=" + v[2], condition = v[3] + v[4] + v[5], iteration = v[6] + v[7] ;

            //Insert all of the necessary lines 
            string endStatement = iteration + ";jump: FOR_" + to_string(lineNum) + ";=END_" + to_string(lineNum) + ";delete: " + v[1] + ";";
            string jumpBegin = "FOR_" + to_string(lineNum); string jumpEnd = "END_" + to_string(lineNum);
            parsedLines.push_back({ lineNum,  "var " + initializer + ";"});
            parsedLines.push_back({ lineNum,   "=" + jumpBegin + ";"});
            parsedLines.push_back({ lineNum,  "if: " + condition + "," + jumpEnd + ";" });
            statementVec.push_back(ControlStructureData(lineNum, FOR, jumpBegin, jumpEnd, endStatement));
        }),
        //Override:  no initializer provided. Iteration is incremental
        ControlStructure(TokenTypes{ O_PAREN, ARG, LOGIC, ARG, COMMA, ARG, MOD, ARG, C_PAREN, COLON  }, [&parsedLines, &statementVec](const vector<string>& v, const int& lineNum) {
            //For each for-loop segment, parse it
            string condition = v[1] + v[2] + v[3], iteration = v[4] + v[5] + v[6];

            //Insert all of the necessary lines 
            string endStatement = iteration + ";jump: FOR_" + to_string(lineNum) + ";=END_" + to_string(lineNum) + ";";
            string jumpBegin = "FOR_" + to_string(lineNum); string jumpEnd = "END_" + to_string(lineNum);
            parsedLines.push_back({ lineNum,   "=" + jumpBegin + ";"});
            parsedLines.push_back({ lineNum,  "if: " + condition + "," + jumpEnd + ";" });
            statementVec.push_back(ControlStructureData(lineNum, FOR, jumpBegin, jumpEnd, endStatement));
        }),
        //Override: no initializer provided. Iteration is incremental
        ControlStructure(TokenTypes{ O_PAREN, ARG, LOGIC, ARG, COMMA, ARG, MOD, C_PAREN, COLON  }, [&parsedLines, &statementVec](const vector<string>& v, const int& lineNum) {
            //For each for-loop segment, parse it
            string condition = v[1] + v[2] + v[3], iteration = v[4] + v[5];

            //Insert all of the necessary lines 
            string endStatement = iteration + ";jump: FOR_" + to_string(lineNum) + ";=END_" + to_string(lineNum) + ";";
            string jumpBegin = "FOR_" + to_string(lineNum); string jumpEnd = "END_" + to_string(lineNum);
            parsedLines.push_back({ lineNum,   "=" + jumpBegin + ";"});
            parsedLines.push_back({ lineNum,  "if: " + condition + "," + jumpEnd + ";" });
            statementVec.push_back(ControlStructureData(lineNum, FOR, jumpBegin, jumpEnd, endStatement));
        }),
    };

    statements["while"] = vector<ControlStructure>{
        ControlStructure(TokenTypes{ O_PAREN, ARG, LOGIC, ARG, C_PAREN, COLON  }, [&parsedLines, &statementVec](const vector<string>& v, const int& lineNum) {
            //Parse the condition
            string condition = v[1] + v[2] + v[3];

            string jumpBegin = "WHILE_" + to_string(lineNum); string jumpEnd = "END_" + to_string(lineNum);
            parsedLines.push_back({ lineNum,  "=" + jumpBegin + ";" });
            parsedLines.push_back({ lineNum,  "if: " + condition + "," + jumpEnd + ";" });
            string endStatement = "jump: WHILE_" + to_string(lineNum) + ";=END_" + to_string(lineNum) + ";";
            statementVec.push_back(ControlStructureData(lineNum, WHILE, jumpBegin, jumpEnd, endStatement));
        }),
    };

    statements["if"] = vector<ControlStructure>{
        ControlStructure(TokenTypes{ O_PAREN, ARG, LOGIC, ARG, C_PAREN, COLON  }, [&parsedLines, &statementVec](const vector<string>& v, const int& lineNum) {
            //Parse the condition
            string condition = v[1] + v[2] + v[3];
            //Modify if-statement, in order to jump to the corresponding end if false.
            string line = "if: " + condition + ", END_" + to_string(lineNum) + ";";
            parsedLines.push_back({ lineNum, line });
            statementVec.push_back(ControlStructureData(lineNum, IF, "=END_" + to_string(lineNum) + ";"));
        }),
        //Overload: Check if bool is true
        ControlStructure(TokenTypes{ O_PAREN, ARG, C_PAREN, COLON  }, [&parsedLines, &statementVec](const vector<string>& v, const int& lineNum) {
            //Parse the condition
            string condition = v[1];
            //Modify if-statement, in order to jump to the corresponding end if false.
            string line = "if: " + condition + ", END_" + to_string(lineNum) + ";";
            parsedLines.push_back({ lineNum, line });
            statementVec.push_back(ControlStructureData(lineNum, IF, "=END_" + to_string(lineNum) + ";"));
        }),
        //Overload: Check if bool is false
        ControlStructure(TokenTypes{ O_PAREN, NEG, ARG, C_PAREN, COLON  }, [&parsedLines, &statementVec](const vector<string>& v, const int& lineNum) {
            //Parse the condition
            string condition = "!" + v[1];
            //Modify if-statement, in order to jump to the corresponding end if false.
            string line = "if: " + condition + ", END_" + to_string(lineNum) + ";";
            parsedLines.push_back({ lineNum, line });
            statementVec.push_back(ControlStructureData(lineNum, IF, "=END_" + to_string(lineNum) + ";"));
        })
    };

    statements["else"] = vector<ControlStructure>{
        ControlStructure(TokenTypes{ COLON  }, [&parsedLines, &statementVec](const vector<string>& v, const int& lineNum) {
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

    statements["end"] = vector<ControlStructure>{
        ControlStructure(TokenTypes{ SEMICOLON }, [&parsedLines, &statementVec](const vector<string>& v, const int& lineNum) {
            if (statementVec.size() == 0)
                throw runtime_error("Received hanging end-statement");

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

    //Secondly, parse the lines, check for statements and handle them accordingly
    for (const auto&[lineNum, l] : lines) {
        auto parsed = Parse(l);

        if (parsed.size() == 1 && parsed.back().empty())
            continue;

        //Thirdly, resolve control flow statements
        for (const string& x : parsed) {
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
                        //As TokenTypes 5 and below are unambiguous, do not push them
                        if (argType > 5)
                            args.push_back(token);
                    }
                }

                //Call the function handling the corresponding control statement
                try {
                    auto found = std::find_if(statements[statementName].begin(), statements[statementName].end(), [argTypes](const ControlStructure& s) {
                        return (s.GetTypes() == argTypes);
                    });

                    if (found == statements[statementName].end())
                        ExitError(statementName + " received wrong implementation on line " + to_string(lineNum));

                    found->Execute(args, lineNum);
                }
                catch (const std::runtime_error& e) {
                    ExitError(string(e.what()) + " on line " + to_string(lineNum));
                }
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
                //2nd index should always be a open bracket
                if(tokens[2] != "(")
                    ExitError("Expected '(' in function definition on line " + to_string(lineNum));

                //2nd to last index should always be a closing bracket
                if (tokens[tokens.size() - 2] != ")")
                    ExitError("Expected ')' in function definition on line " + to_string(lineNum));

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
                            ExitError("Expected ',' in function argument definition on line " + to_string(lineNum));
                        continue;
                    }
                }

                //Make sure the last token was indeed an argument
                if(lastToken == ",")
                    ExitError("Expected argument got ',' on line " + to_string(lineNum));

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
                statementVec.push_back(ControlStructureData(lineNum, FUNC, endStatement + "return;"));
            }
            //A function call has been found
            else if (functions.find(statementName) != functions.cend()) {
                if (tokens.size() < 4)
                    ExitError("Invalid args provided when calling function on line " + to_string(lineNum));

                if (tokens.back() != ";")
                    ExitError("Expected ';' after calling function on line " + to_string(lineNum));

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
                            ExitError("Expected '(' when calling function on line " + to_string(lineNum));

                        //If the next token isn't a '(', throw an error
                        if (tokens[i + 1] != "(")
                            ExitError("Expected '(' when calling function on line " + to_string(lineNum));

                        //Skip over the next token, as we have confirmed it is a '('
                        i++;; continue;
                    }

                    //As we skip over valid '(', they should not be here
                    if (token == "(")
                        ExitError("Hanging '(' received in function call on line " + to_string(lineNum));

                    if (token == ")") {
                        //Make sure functionHistory is not empty
                        if (funcHistory.empty())
                            ExitError("Hanging ')' received in function call on line " + to_string(lineNum));

                        //Make sure function call doesn't end with ','
                        if (tokens[i - 1] == ",")
                            ExitError("Expected argument got ',' on line " + to_string(lineNum));
                        funcHistory.pop_back();
                        continue;
                    }

                    //If the token is not a seperator, it is an argument
                    if (separators.find(token) == separators.cend()) {
                        //Push the arg to the current function
                        funcHistory.back()->AddArg(token);
                    }
                    //if it is a seperator, make sure it is a ','
                    else if(token != ",")
                        ExitError("Expected: ',' Got '" + token + "' when calling function on line " + to_string(lineNum));
                }

                //Make sure function call doesn't end with ','
                if(tokens[lastIndex - 1] == ",")
                    ExitError("Expected argument got ',' on line " + to_string(lineNum));

                //Make sure function call ends with ';' or '>>'
                if (tokens[lastIndex + 1] != ";" && tokens[lastIndex + 1] != ">>")
                    ExitError("Expected end of function call, got '" + tokens[lastIndex] + "' on line " + to_string(lineNum));

                //Reverse functions, as we want to resolve the inner functions first
                reverse(funcArgs.begin(), funcArgs.end());

                for (const auto& func : funcArgs) {
                    auto function = *functions.find(func.GetName());
                    //Reverse function args too, as they get taken out of the stack backwards
                    auto args = func.GetArgs(); std::reverse(args.begin(), args.end());
                    //If function args do not match actual args
                    if(function.second != args.size())
                        ExitError("No instance of " + function.first + " takes " + to_string(args.size()) + " arguments on line " + to_string(lineNum));
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
                    if(tokens[lastIndex + 1] != ">>")
                        ExitError("Expected: '>>' Got: '" + tokens[lastIndex + 1]  + "' on line " + to_string(lineNum));
                    parsedLines.push_back({ lineNum, "pop: " + tokens[lastIndex + 2] + ";" });
                }
                //Throw an error if the value is above that
                else if (lastIndex + 4 < tokens.size()) {
                    ExitError("Invalid args provided when calling function on line " + to_string(lineNum));
                }
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


    // Function that searches through memory and returns the value of a variable given its name.
    auto FindVar = [&memory, &errorLevel](const std::string& varName, std::string& value, int& valueType) {
        // Edge case: errorType.
        if (varName == "errorLevel") {
            value = to_string(errorLevel);
            valueType = INT;
            return true;
        }

        const auto found = memory.find(varName);
        if (found != memory.cend()) {
            const auto& data = found->second.GetData();
            valueType = found->second.GetType();

            if (valueType == STRING)
                value = '"' + data + '"';
            else
                value = data;

            return true;
        }

        return false;
    };

    auto ResolveValue = [&FindVar](std::string& value, int& type) {
        // Get the type based on data
        type = GetDataType(value);

        // If the type is still nothing, perhaps it is a variable
        if (type == ERROR) {
            const std::string varName = value;
            if (!FindVar(varName, value, type))
                throw std::runtime_error("Instruction received undefined identifier '" + varName + "'");
            // If the type is STILL nothing, it is an uninitialized variable
            if (type == ERROR)
                throw std::runtime_error("Instruction received uninitialized variable '" + varName + "'");
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

        const auto& funcSet = found->second;

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

    instructions["print"] = vector<Instruction>{
        Instruction(TokenTypes{ COLON, ARG }, [ResolveValue](const vector<string>& v) {
            string value = v[0]; int valueType = -1; ResolveValue(value, valueType);

            if (valueType == STRING)
                FormatString(value);

            cout << value;
        })
    };

    instructions["printl"] = vector<Instruction>{
        Instruction(TokenTypes{ COLON, ARG }, [ResolveValue](const vector<string>& v) {
            string value = v[0]; int valueType = -1; ResolveValue(value, valueType);

            if (valueType == STRING)
                FormatString(value);

            cout << value << endl;
        })
    };

    instructions["endl"] = vector<Instruction>{
        Instruction(TokenTypes{}, [ResolveValue](const vector<string>& v) {
            cout << endl;
        })
    };

    instructions["cls"] = vector<Instruction>{
        Instruction(TokenTypes{}, [ResolveValue](const vector<string>& v) {
            // Istg this is the best way to do this
            cout << "\033[2J\033[1;1H" << endl;
        })
    };

    instructions["input"] = vector<Instruction>{
        Instruction(TokenTypes{ COLON, ARG }, [&](const vector<string>& v) {
            string name = v[0], value; int type = -1;
            // Reset errorLevel to 0 
            errorLevel = 0;

            if (!FindVar(name, value, type))
                throw runtime_error(("Input received undefined identifier '" + name + "'").c_str());

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
        Instruction(TokenTypes{ COLON, ARG, COMMA, ARG }, [&](const vector<string>& v) {
            cout << FormatString(v[0]); FindInstruction("input", TokenTypes{ COLON, ARG })(vector<string>{v[1]});
        })
    };

    instructions["push"] = vector<Instruction>{
        Instruction(TokenTypes{ COLON, ARG }, [&stack, ResolveValue](const vector<string>& v) {
            string value = v[0]; int type = -1; ResolveValue(value, type);

            // Push it to the stack
            stack.emplace_back(Var(value, type));
        })
    };

    instructions["pop"] = vector<Instruction>{
        Instruction(TokenTypes{ COLON, ARG }, [&errorLevel, &memory, &stack](const vector<string>& v) {
            string name = v[0], value; int type = -1;
            // Reset errorLevel
            errorLevel = 0;

            // Check if variable exists
            auto it = memory.find(name);
            if (it == memory.end()) {
                throw runtime_error(("Pop received undefined identifier '" + name + "'").c_str());
            }

            value = it->second.GetData();
            type = it->second.GetType();

            if (stack.empty()) {
                errorLevel = 1; return;
            }

            int topType = stack.back().GetType();
            string topData = stack.back().GetData();
            stack.pop_back();

            if (topType == STRING)
                FormatString(topData);

            // Uninitialized variable as target, proceed accordingly
            if (type == ERROR) {
                memory[name] = Var(topData, topType);
                return;
            }

            if (type != topType)
                throw runtime_error(("Pop received wrong type Got: '" + IntToType(type) + "' Expected: '" + IntToType(topType)).c_str());
            // Pop it to the variable
            memory[name].SetData(topData);
        })
    };

    instructions["var"] = vector<Instruction>{
        Instruction(TokenTypes{ ARG, SET, ARG }, [&](const vector<string>& v) {
            string name = v[0]; string value = v[1]; int valueType = 0;
            ResolveValue(value, valueType);

            ValidateVarName(name);
            if (valueType == STRING)
                FormatString(value);

            memory[name] = Var(value, valueType);
        }),
        // Overload: Define variable, but do not initialize it
        Instruction(TokenTypes{ ARG }, [&memory, ValidateVarName](const vector<string>& v) {
            string name = v[0];
            ValidateVarName(name);

            memory[name] = Var("", ERROR);
        })
    };

    instructions["[VarName]"] = vector<Instruction>{
        // Sets a variable to a value
        // the final line should look like [VarName] var1 = value, thus having an additional 0 prepended.
        Instruction(TokenTypes{ ARG, SET, ARG }, [&](const vector<string>& v) {
            string name = v[0]; string nameValue = ""; int nameType = 0;
            string value = v[1]; int valueType = 0; ResolveValue(value, valueType);

            // if the name doesn't get found
            if (!FindVar(name, nameValue, nameType))
                throw runtime_error(("Setter received a literal or undefined identifier. Got: '" + name + "'").c_str());

            // Uninitialized variable as target, proceed accordingly
            if (nameType == ERROR) {
                memory[name] = Var(value, valueType); return;
            }

            // If it isn't the same type, or number type.
            if (nameType != valueType && !(nameType == DOUBLE && valueType == INT) && !(nameType == INT && valueType == DOUBLE))
                throw runtime_error(("Setter received wrong type. Got: '" + IntToType(valueType) + "' Expected: '" + IntToType(nameType) + "'").c_str());

            memory[name] = Var(value, valueType);
        }),
        // Modifying a variable
        Instruction(TokenTypes{ ARG, MOD, ARG }, [&](const vector<string>& v) {
            string name = v[0]; string nameValue = ""; int nameType = 0;
            string value = v[2]; int valueType = 0; ResolveValue(value, valueType);
            string op = v[1];

            // if the name doesn't get found
            if (!FindVar(name, nameValue, nameType))
                throw runtime_error(("Setter received a literal or undefined identifier. Got: '" + name + "'").c_str());

            // If it isn't the same type, or number type.
            if (nameType != valueType && !(nameType == DOUBLE && valueType == INT) && !(nameType == INT && valueType == DOUBLE))
                throw runtime_error(("Arithmetic operation received wrong type. Got: '" + IntToType(valueType) + "' Expected: '" + IntToType(nameType) + "'").c_str());

            if (nameType == BOOL)
                throw runtime_error("Cannot perform arithmetic operation on type 'bool'");

            if (nameType == STRING && op != "+=")
                throw runtime_error(("Cannot use operator '" + op + "' on a string").c_str());
            else if (nameType == STRING) {
                string data = memory.at(name).GetData(); FormatString(value);
                memory[name].SetData(data + value); return;
            }

            // Get the data from memory
            auto& var = memory.at(name);
            double data = stod(var.GetData());
            double newData = stod(value);

            if (op == "+=")
                data += newData;
            else if (op == "-=")
                data -= newData;
            else if (op == "*=")
                data *= newData;
            else if (op == "/=") {
                if (newData == 0.0)
                    throw runtime_error("Division by 0 attempted");

                data /= newData;
            }
            else if (op == "%=") {
                if (newData == 0.0)
                    throw runtime_error("Modulo by 0 attempted");

                data = (int)data % (int)newData;
            }
            else
                throw runtime_error("Wrong operator received. Expected '+=', '-=', '*=', '/=' or '%='");

            if (nameType == INT)
                var.SetData(to_string((int)(data)));
            else
                var.SetData(to_string(data));
        }),
        // Incrementing or decrementing variable
        Instruction(TokenTypes{ ARG, MOD }, [&](const vector<string>& v) {
            string name = v[0], value = ""; int type = 0;
            string op = v[1];

            // if the name doesn't get found
            if (!FindVar(name, value, type))
                throw runtime_error(("Setter received a literal or undefined identifier. Got: '" + name + "'").c_str());

            // If type is string or bool
            if (type == STRING || type == BOOL)
                throw runtime_error(("Cannot use operator '" + op + "' on type '" + IntToType(type) + "'").c_str());

            auto& var = memory.at(name);
            double data = stod(var.GetData());

            if (op == "++")
                data += 1.0;
            else if (op == "--")
                data -= 1.0;
            else
                throw runtime_error("Wrong operator received. Expected '++' or '--'");

            if (type == INT)
                var.SetData(to_string((int)(data)));
            else
                var.SetData(to_string(data));
        })
    };

    instructions["sqrt"] = vector<Instruction>{
        Instruction(TokenTypes{ COLON, ARG }, [&](const vector<string>& v) {
            string name = v[0]; string nameValue = ""; int nameType = 0;

            // if the name doesn't get found
            if (!FindVar(name, nameValue, nameType))
                throw runtime_error(("Sqrt received a literal or undefined identifier. Got: '" + name + "'").c_str());

            // If it isn't the same type, or number type.
            if (nameType != DOUBLE && nameType != INT)
                throw runtime_error(("Square root operation received wrong type. Got: '" + IntToType(nameType) + "'").c_str());

            // Get the data from memory
            auto& var = memory.at(name);
            double data = stod(var.GetData());

            if (nameType == INT)
                var.SetData(to_string((int)(sqrt(data))));
            else
                var.SetData(to_string(sqrt(data)));
        })
    };

    instructions["delete"] = vector<Instruction>{
        Instruction(TokenTypes{ COLON, ARG }, [&memory, &errorLevel](const vector<string>& v) {
            string name = v[0]; errorLevel = 0;

            if (memory.find(name) == memory.cend()) {
                errorLevel = 1; return;
            }

            memory.erase(name);
        })
    };

    instructions["exit"] = vector<Instruction>{
        Instruction(TokenTypes{ COLON, ARG }, [ResolveValue, start](const vector<string>& v) {
            string code = v[0]; int type = 0; ResolveValue(code, type);

            if (GetDataType(code) != INT) throw runtime_error(("Exit requires argument type: 'int' got: '" + IntToType(type) + "'").c_str());

            auto end = std::chrono::high_resolution_clock::now();
            cout << endl << "Program sucessfully executed. Exited with code 0." << endl <<
                "Elapsed time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms" << endl;
            exit(stoi(code));
        })
    };

    instructions["jump"] = vector<Instruction>{
        Instruction(TokenTypes{ COLON, ARG }, [&labelMap, &parsedLineIndex](const vector<string>& v) {
            string name = v[0];

            if (labelMap.find(name) == labelMap.cend())
                throw runtime_error(("Tried to jump to undefined label. Got: '" + name + "'").c_str());

            int newLine = labelMap[name];

            // Jump to the new line
            parsedLineIndex = newLine;
        })
    };

    instructions["call"] = std::vector<Instruction>{
        Instruction(TokenTypes{ COLON, ARG }, [&labelMap, &callHistory, &parsedLineIndex](std::vector<std::string> v) {
            std::string name = v[0];

            if (labelMap.find(name) == labelMap.cend())
                throw std::runtime_error(("Tried to call undefined function. Got: '" + name + "'").c_str());

            int newLine = labelMap.at(name);

            // Push current line to callHistory and set index to newLine
            callHistory.push_back(parsedLineIndex);
            parsedLineIndex = newLine;
        })
    };

    instructions["return"] = vector<Instruction>{
        Instruction(TokenTypes{}, [&callHistory, FindInstruction, &parsedLineIndex](const vector<string>& v) {
            // Return is equivalent to exit if the callHistory is empty.
            if (callHistory.size() < 1)
                FindInstruction("exit", TokenTypes{ COLON, ARG })(vector<string> { "0" });

            // Set current line to latest entry and remove the entry. 
            parsedLineIndex = callHistory.back(); callHistory.pop_back();
        }),
        // Override: Return a variable
        Instruction(TokenTypes{ COLON, ARG }, [&callHistory, FindInstruction, &parsedLineIndex](const vector<string>& v) {
            string name = v[0];

            // Push the variable to the stack
            FindInstruction("push", TokenTypes{ COLON, ARG })(vector<string> { name });
            // Also delete the function
            FindInstruction("delete", TokenTypes{ COLON, ARG })(vector<string> { name });

            // Set current line to latest entry and remove the entry. 
            parsedLineIndex = callHistory.back(); callHistory.pop_back();
        })
    };

    instructions["if"] = vector<Instruction>{
         Instruction(TokenTypes{ COLON, ARG, LOGIC, ARG, COMMA, ARG }, [&](const vector<string>& v) {
            string value1 = v[0]; string op = v[1]; string value2 = v[2];
            int value1Type = 0, value2Type = 0;

            //Resolve values and types
            ResolveValue(value1, value1Type);
            ResolveValue(value2, value2Type);

            //Check for types. Compare doubles and ints
            if (value1Type != value2Type && !(value1Type == DOUBLE && value2Type == INT) && !(value1Type == INT && value2Type == DOUBLE)) {
                throw runtime_error(("Comparing different types. Type1: '" + IntToType(value1Type) + "' Type2: '" + IntToType(value2Type) + "'").c_str());
            }

            //Check for equality and inequality
            if (op == "==" || op == "!=") {
                bool condition = (op == "==") ? (value1 == value2) : (value1 != value2);
                if (!condition) {
                    FindInstruction("jump", TokenTypes{ COLON, ARG })(vector<string> { v[3] });
                }
                return;
            }

            //Make sure strigns and bools cannot be compared relationally
            if (value1Type == STRING || value1Type == BOOL) {
                throw runtime_error(("Cannot use relational operators on Type: '" + IntToType(value1Type) + "'").c_str());
            }

            double value1d = stod(value1);
            double value2d = stod(value2);

            //Relational operators
            bool jump = false;
            if ((op == "<" && value1d >= value2d) ||
                (op == ">" && value1d <= value2d) ||
                (op == ">=" && value1d < value2d) ||
                (op == "<=" && value1d > value2d)) {
                jump = true;
            }

            //Jump to line if condition isn't met
            if (jump) {
                FindInstruction("jump", TokenTypes{ COLON, ARG })(vector<string> { v[3] });
            }
        }),
        // Override: If bool is true or variable is initialized
        Instruction(TokenTypes{ COLON, ARG, COMMA, ARG }, [&](const vector<string>& v) {
            string name = v[0], value = name; int type = GetDataType(name);
            if (type == ERROR && !FindVar(name, value, type))
                throw runtime_error(("If statement received undefined identifier '" + name + "'").c_str());

            // If the type is bool and it isn't true or if the type is errorType, jump to end
            if(value == "false")
                FindInstruction("jump", TokenTypes{ COLON, ARG })(vector<string> { v[1] });
            if(type == ERROR)
                FindInstruction("jump", TokenTypes{ COLON, ARG })(vector<string> { v[1] });
        }),
        // Override: If bool is true or variable is initialized
        Instruction(TokenTypes{ COLON, NEG, ARG, COMMA, ARG }, [&](const vector<string>& v) {
            string name = v[0], value = name; int type = GetDataType(name);
            if (type == ERROR && !FindVar(name, value, type))
                throw runtime_error(("If statement received undefined identifier '" + name + "'").c_str());

            // If the type is bool and it is true or if the type isn't errorType, jump to end
            if (value == "true")
                FindInstruction("jump", TokenTypes{ COLON, ARG })(vector<string> { v[1] });
            else if (type != ERROR && type != BOOL)
                FindInstruction("jump", TokenTypes{ COLON, ARG })(vector<string> { v[1] });
        })
    };

    //Append instruction names to the blacklist
    for (const auto& a : instructions)
        blacklist.insert(a.first);

    //Vector storing functions on each line. Int stores real line, vector<string> stores arg, function stores implementation
    vector<InstructionHandle> instructionVec;

    //Fifth, tokenize each line and go through the actual interpretation process. 
    for(const auto& [lineNum, l] : parsedLines) {
        vector<string> tokens;
        //Skip labels
        if (l[0] == '=') {
            //Take into consideration that the location labels point to should be kept the same when actually running the function implementations
            instructionVec.push_back({ -1, vector<string>{}, nullptr });
            continue;
        }
        try {
            tokens = Tokenize(l);
        }
        catch (const std::runtime_error& e) {
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
                //As TokenTypes 2 and below are unambiguous, do not push them
                if(argType > 5)
                    args.push_back(token);
            }
        }

        //for (const auto& a : argTypes)
        //    cout << IntToTokenType(a) << endl;

        //Store the instruction in the instruction vector
        try {
            auto found = std::find_if(instructions[funcName].begin(), instructions[funcName].end(), [argTypes](const Instruction& f) {
                return (f.GetTypes() == argTypes);
                });

            if (found == instructions[funcName].end())
                ExitError(funcName + " received wrong implementation on line " + to_string(lineNum));
            instructionVec.push_back({ lineNum, args, &(*(found)) });
        }
        catch (const std::runtime_error& e) {
            //Specialized error message for [VarName] as it indicates a non-instruction funcName
            if (funcName == "[VarName]")
                ExitError("No Instruction or identifier by the name '" + tokens[1] + "' found on line " + to_string(lineNum));
            ExitError(string(e.what()) + " on line " + to_string(lineNum));
        }
    }
    start = std::chrono::high_resolution_clock::now();
    //Execute the functions
    for (; parsedLineIndex < instructionVec.size(); parsedLineIndex++) {
        //If parsedLineIndex is on the latest call, delete it to avoid duplicate calls. 
        if (!callHistory.empty() && callHistory.back() == parsedLineIndex)
            callHistory.erase(callHistory.begin() + parsedLineIndex);

        int lineNum = instructionVec[parsedLineIndex].GetLine();
        const auto args = instructionVec[parsedLineIndex].GetArgs();

        //Label
        if (lineNum == -1)
            continue;

        try {
            //Get the function implementation and pass in the args. Index 2 and 1 respectively
            instructionVec[parsedLineIndex].GetInstruction()->Execute(args);
        }
        catch (const std::runtime_error& e) {
            ExitError(e.what() + string(" on line ") + to_string(lineNum));
        }
    }

    //Use the actual exit instruction to exit. Effectively saving 3 lines of code lol
    FindInstruction("exit", TokenTypes{ COLON, ARG })(vector<string> { "0" });
   
    file.close();
    return 0;
}