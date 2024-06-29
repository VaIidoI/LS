#pragma once
#ifndef ARCHETYPES_H
#define ARCHETYPES_H

#include <functional>
#include <string>
#include <vector>

using std::vector; using std::string;

//A function consists of a name, arguments with specific types and an implementation
class Instruction {
public:
    Instruction() : name_(""), types_(vector<int>{}), implementation_() {}

    Instruction(const string& name, const vector<int>& types, const std::function<void(const vector<string>&)>& imp)
        : name_(name), types_(types), implementation_(imp) {}

    Instruction(const vector<int>& types, const std::function<void(const vector<string>&)>& imp)
        : name_(""), types_(types), implementation_(imp) {}

    //Getters
    string GetName() const {
        return name_;
    }

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
    std::string name_; vector<int> types_;
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

class ControlFlow {
public:
    ControlFlow() = default;

    ControlFlow(const int& line, const string& type, const string& endStatement) :
        line_(line), type_(type), endStatement_(endStatement), jumpBegin_("") {}

    ControlFlow(const int& line, const string& type, const string& jumpBegin, const string& jumpEnd, const string& endStatement) :
        line_(line), type_(type), jumpBegin_(jumpBegin), jumpEnd_(jumpEnd), endStatement_(endStatement) {}

    //Getters
    int GetLine() const {
        return line_;
    }

    string GetType() const {
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
    int line_; string type_, jumpBegin_, jumpEnd_, endStatement_;
};
#endif // !ARCHETYPES_H

