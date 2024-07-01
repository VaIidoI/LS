#pragma once
#ifndef ARCHETYPES_H
#define ARCHETYPES_H

#include <functional>
#include <string>
#include <vector>

using std::vector; using std::string;

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

    void SetType(const int& type) {
        type_ = type;
    }

private:
    int type_; string data_;
};

//An instruction consists of arguments with specific types and an implementation
class Instruction {
public:
    Instruction() : types_(vector<int>{}), implementation_() {}

    Instruction(const vector<int>& types, const std::function<void(const vector<string>&)>& imp)
        : types_(types), implementation_(imp) {}

    //Getters

    vector<int> GetTypes() const {
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
    vector<int> types_;
    std::function<void(const vector<string>&)> implementation_;
};

class InstructionHandle {
public:
    InstructionHandle() = default;

    InstructionHandle(int line, const std::vector<std::string>& args, Instruction* instruction)
        : line_(line), args_(args), instruction_(instruction) {}

    // Getters
    int GetLine() const {
        return line_;
    }

    const std::vector<std::string>& GetArgs() const {
        return args_;
    }

    Instruction* GetInstruction() const {
        return instruction_;
    }

    // Setters
    void SetLine(int line) {
        line_ = line;
    }

    void SetArgs(const std::vector<std::string>& args) {
        args_ = args;
    }

private:
    int line_;
    std::vector<std::string> args_;
    Instruction* instruction_;
};

class ControlStructure {
public:
    ControlStructure() : types_(vector<int>{}), implementation_() {}

    ControlStructure(const vector<int>& types, const std::function<void(const vector<string>&, const int&)>& imp)
        : types_(types), implementation_(imp) {}

    //Getters
    vector<int> GetTypes() const {
        return types_;
    }

    //Function to execute the implementation
    void Execute(const vector<string>& args, const int& lineNum) const {
        implementation_(args, lineNum);
    }

private:
    vector<int> types_;
    std::function<void(const vector<string>&, const int&)> implementation_;
};

class ControlStructureData {
public:
    ControlStructureData() = default;

    ControlStructureData(const int& line, const int & type, const string& endStatement) :
        line_(line), type_(type), endStatement_(endStatement), jumpBegin_("") {}

    ControlStructureData(const int& line, const int& type, const string& jumpBegin, const string& jumpEnd, const string& endStatement) :
        line_(line), type_(type), jumpBegin_(jumpBegin), jumpEnd_(jumpEnd), endStatement_(endStatement) {}

    //Getters
    int GetLine() const {
        return line_;
    }

    int GetType() const {
        return type_;
    }

    string GetEndStatement() {
        return endStatement_;
    }

    string GetJumpBegin() const {
        return jumpBegin_;
    }

    string GetJumpEnd() const {
        return jumpEnd_;
    }

    //Setters
    void SetEndStatement(const string& endStatement) {
        endStatement_ = endStatement;
    }

private:
    int line_; int type_; string jumpBegin_, jumpEnd_, endStatement_;
};

class Function {
public:
    Function() : name_(""), args_() {}
    Function(const string& name, const vector<string>& args) : name_(name), args_(args) {}

    //Getters
    string GetName() const {
        return name_;
    }

    vector<string> GetArgs() const {
        return args_;
    }

    //Setters
    void AddArg(const string& arg) {
        args_.push_back(arg);
    }

private:
    string name_; vector<string> args_;
};
#endif // !ARCHETYPES_H

