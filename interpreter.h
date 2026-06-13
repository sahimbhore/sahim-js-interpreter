#pragma once

#include "parser.h"

#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct Environment;
struct FunctionValue;

struct Value {
    enum class Type { Undefined, Number, String, Bool, Array, Object, Function } type = Type::Undefined;
    double number = 0;
    std::string str;
    bool boolean = false;
    std::shared_ptr<std::vector<Value>> array;
    std::shared_ptr<FunctionValue> function;

    static Value undefined();
    static Value numberValue(double n);
    static Value stringValue(std::string s);
    static Value boolValue(bool b);
    static Value arrayValue(std::vector<Value> values);
    static Value objectValue(std::string name);
    static Value functionValue(std::shared_ptr<FunctionValue> fn);

    std::string typeName() const;
    std::string toString() const;
    bool isTruthy() const;
};

struct VariableSlot {
    Value value;
    bool isConst = false;
};

struct Environment : std::enable_shared_from_this<Environment> {
    explicit Environment(std::shared_ptr<Environment> parent = nullptr);
    void define(const std::string& name, const Value& value, bool isConst);
    Value get(const std::string& name, int line);
    void assign(const std::string& name, const Value& value, int line);

    std::shared_ptr<Environment> parent;
    std::unordered_map<std::string, VariableSlot> values;
};

struct FunctionValue {
    std::string name;
    std::vector<std::string> params;
    std::vector<StmtPtr> body;
    std::shared_ptr<Environment> closure;
    bool native = false;
    std::function<Value(const std::vector<Value>&, int)> nativeCall;
};

class Interpreter {
public:
    explicit Interpreter(bool debug = false, std::ostream& out = std::cout, std::ostream& err = std::cerr);
    void interpret(const std::vector<StmtPtr>& statements);

private:
    std::shared_ptr<Environment> globals_;
    std::shared_ptr<Environment> env_;
    bool debug_;
    std::ostream& out_;
    std::ostream& err_;
    static constexpr long long LoopLimit = 1000000;

    void installGlobals();
    void execute(const StmtPtr& stmt);
    void executeBlock(const std::vector<StmtPtr>& statements, std::shared_ptr<Environment> env);
    Value evaluate(const ExprPtr& expr);
    Value callFunction(const std::shared_ptr<FunctionValue>& fn, const std::vector<Value>& args, int line);
    Value callMember(const std::shared_ptr<MemberExpr>& member, const std::vector<Value>& args, int line);
    Value getMember(const Value& object, const std::string& name, int line);
    Value getIndex(const Value& object, const Value& index, int line);
    void setIndex(Value& object, const Value& index, const Value& value, int line);
    Value binary(const Value& left, const Token& op, const Value& right);
    bool strictEqual(const Value& a, const Value& b) const;
    double requireNumber(const Value& value, int line, const std::string& context) const;
    int toIndex(const Value& value, int line) const;
    void debugStep(const std::string& text, int line);
};
