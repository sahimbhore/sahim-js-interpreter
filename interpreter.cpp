#include "interpreter.h"
#include "utils.h"

#include <algorithm>
#include <cmath>
#include <sstream>

class ReturnSignal {
public:
    ReturnSignal(Value value, int line) : value(std::move(value)), line(line) {}
    Value value;
    int line;
};

Value Value::undefined() { return {}; }
Value Value::numberValue(double n) { Value v; v.type = Type::Number; v.number = n; return v; }
Value Value::stringValue(std::string s) { Value v; v.type = Type::String; v.str = std::move(s); return v; }
Value Value::boolValue(bool b) { Value v; v.type = Type::Bool; v.boolean = b; return v; }
Value Value::arrayValue(std::vector<Value> values) { Value v; v.type = Type::Array; v.array = std::make_shared<std::vector<Value>>(std::move(values)); return v; }
Value Value::objectValue(std::string name) { Value v; v.type = Type::Object; v.str = std::move(name); return v; }
Value Value::functionValue(std::shared_ptr<FunctionValue> fn) { Value v; v.type = Type::Function; v.function = std::move(fn); return v; }

std::string Value::typeName() const {
    switch (type) {
        case Type::Undefined: return "undefined";
        case Type::Number: return "number";
        case Type::String: return "string";
        case Type::Bool: return "boolean";
        case Type::Array: return "array";
        case Type::Object: return "object";
        case Type::Function: return "function";
    }
    return "unknown";
}

std::string Value::toString() const {
    switch (type) {
        case Type::Undefined: return "undefined";
        case Type::Number: return trimNumber(number);
        case Type::String: return str;
        case Type::Bool: return boolean ? "true" : "false";
        case Type::Array: {
            std::string out = "[";
            for (size_t i = 0; i < array->size(); ++i) {
                if (i) out += ",";
                out += (*array)[i].toString();
            }
            out += "]";
            return out;
        }
        case Type::Object: return "[object " + str + "]";
        case Type::Function: return "[function]";
    }
    return "";
}

bool Value::isTruthy() const {
    switch (type) {
        case Type::Undefined: return false;
        case Type::Bool: return boolean;
        case Type::Number: return number != 0 && !std::isnan(number);
        case Type::String: return !str.empty();
        case Type::Array:
        case Type::Object:
        case Type::Function: return true;
    }
    return false;
}

Environment::Environment(std::shared_ptr<Environment> parent) : parent(std::move(parent)) {}

void Environment::define(const std::string& name, const Value& value, bool isConst) {
    values[name] = {value, isConst};
}

Value Environment::get(const std::string& name, int line) {
    auto it = values.find(name);
    if (it != values.end()) return it->second.value;
    if (parent) return parent->get(name, line);
    RuntimeErrorHandler::throwError("ReferenceError", "variable '" + name + "' is not defined", line);
}

void Environment::assign(const std::string& name, const Value& value, int line) {
    auto it = values.find(name);
    if (it != values.end()) {
        if (it->second.isConst) RuntimeErrorHandler::throwError("TypeError", "assignment to constant variable '" + name + "'", line);
        it->second.value = value;
        return;
    }
    if (parent) {
        parent->assign(name, value, line);
        return;
    }
    RuntimeErrorHandler::throwError("ReferenceError", "variable '" + name + "' is not defined", line);
}

Interpreter::Interpreter(bool debug, std::ostream& out, std::ostream& err)
    : globals_(std::make_shared<Environment>()), env_(globals_), debug_(debug), out_(out), err_(err) {
    installGlobals();
}

void Interpreter::interpret(const std::vector<StmtPtr>& statements) {
    try {
        for (const auto& stmt : statements) execute(stmt);
    } catch (const ReturnSignal& r) {
        RuntimeErrorHandler::throwError("SyntaxError", "return statement outside function", r.line);
    }
}

void Interpreter::installGlobals() {
    globals_->define("console", Value::objectValue("console"), true);
    globals_->define("Math", Value::objectValue("Math"), true);
}

void Interpreter::execute(const StmtPtr& stmt) {
    debugStep("execute statement", stmt->line);
    if (auto s = std::dynamic_pointer_cast<EmptyStmt>(stmt)) {
        return;
    } else if (auto s = std::dynamic_pointer_cast<ExprStmt>(stmt)) {
        evaluate(s->expr);
    } else if (auto s = std::dynamic_pointer_cast<VarStmt>(stmt)) {
        Value value = s->init ? evaluate(s->init) : Value::undefined();
        env_->define(s->name, value, s->kind == "const");
    } else if (auto s = std::dynamic_pointer_cast<BlockStmt>(stmt)) {
        executeBlock(s->statements, std::make_shared<Environment>(env_));
    } else if (auto s = std::dynamic_pointer_cast<IfStmt>(stmt)) {
        if (evaluate(s->cond).isTruthy()) execute(s->thenBranch);
        else if (s->elseBranch) execute(s->elseBranch);
    } else if (auto s = std::dynamic_pointer_cast<WhileStmt>(stmt)) {
        long long guard = 0;
        while (evaluate(s->cond).isTruthy()) {
            if (++guard > LoopLimit) RuntimeErrorHandler::throwError("RuntimeError", "possible infinite loop detected", s->line);
            execute(s->body);
        }
    } else if (auto s = std::dynamic_pointer_cast<ForStmt>(stmt)) {
        auto loopEnv = std::make_shared<Environment>(env_);
        auto previous = env_;
        env_ = loopEnv;
        try {
            if (s->init) execute(s->init);
            long long guard = 0;
            while (!s->cond || evaluate(s->cond).isTruthy()) {
                if (++guard > LoopLimit) RuntimeErrorHandler::throwError("RuntimeError", "possible infinite loop detected", s->line);
                execute(s->body);
                if (s->inc) evaluate(s->inc);
            }
            env_ = previous;
        } catch (...) {
            env_ = previous;
            throw;
        }
    } else if (auto s = std::dynamic_pointer_cast<FunctionStmt>(stmt)) {
        auto fn = std::make_shared<FunctionValue>();
        fn->name = s->name;
        fn->params = s->params;
        fn->body = s->body;
        fn->closure = env_;
        env_->define(s->name, Value::functionValue(fn), true);
    } else if (auto s = std::dynamic_pointer_cast<ReturnStmt>(stmt)) {
        throw ReturnSignal(s->value ? evaluate(s->value) : Value::undefined(), s->line);
    }
}

void Interpreter::executeBlock(const std::vector<StmtPtr>& statements, std::shared_ptr<Environment> env) {
    auto previous = env_;
    env_ = std::move(env);
    try {
        for (const auto& stmt : statements) execute(stmt);
        env_ = previous;
    } catch (...) {
        env_ = previous;
        throw;
    }
}

Value Interpreter::evaluate(const ExprPtr& expr) {
    if (auto e = std::dynamic_pointer_cast<LiteralExpr>(expr)) {
        switch (e->value.type) {
            case Literal::Type::Number: return Value::numberValue(e->value.number);
            case Literal::Type::String: return Value::stringValue(e->value.str);
            case Literal::Type::Bool: return Value::boolValue(e->value.boolean);
            case Literal::Type::Undefined: return Value::undefined();
        }
    } else if (auto e = std::dynamic_pointer_cast<VariableExpr>(expr)) {
        return env_->get(e->name, e->line);
    } else if (auto e = std::dynamic_pointer_cast<ArrayExpr>(expr)) {
        std::vector<Value> values;
        for (size_t i = 0; i < e->elements.size(); ++i) {
            Value v = evaluate(e->elements[i]);
            if (e->spread[i]) {
                if (v.type != Value::Type::Array) RuntimeErrorHandler::throwError("TypeError", "spread requires an array", e->line);
                values.insert(values.end(), v.array->begin(), v.array->end());
            } else {
                values.push_back(v);
            }
        }
        return Value::arrayValue(values);
    } else if (auto e = std::dynamic_pointer_cast<UnaryExpr>(expr)) {
        Value r = evaluate(e->right);
        if (e->op.type == TokenType::Bang) return Value::boolValue(!r.isTruthy());
        if (e->op.type == TokenType::Minus) return Value::numberValue(-requireNumber(r, e->line, "unary '-'"));
    } else if (auto e = std::dynamic_pointer_cast<BinaryExpr>(expr)) {
        if (e->op.type == TokenType::AndAnd) {
            Value left = evaluate(e->left);
            return left.isTruthy() ? evaluate(e->right) : left;
        }
        if (e->op.type == TokenType::OrOr) {
            Value left = evaluate(e->left);
            return left.isTruthy() ? left : evaluate(e->right);
        }
        return binary(evaluate(e->left), e->op, evaluate(e->right));
    } else if (auto e = std::dynamic_pointer_cast<AssignExpr>(expr)) {
        Value rhs = evaluate(e->value);
        auto compoundValue = [&](const Value& current) {
            if (e->op.type == TokenType::Equal) return rhs;
            Token binaryOp = e->op;
            if (e->op.type == TokenType::PlusEqual) binaryOp.type = TokenType::Plus;
            else if (e->op.type == TokenType::MinusEqual) binaryOp.type = TokenType::Minus;
            else if (e->op.type == TokenType::StarEqual) binaryOp.type = TokenType::Star;
            else if (e->op.type == TokenType::SlashEqual) binaryOp.type = TokenType::Slash;
            else if (e->op.type == TokenType::PercentEqual) binaryOp.type = TokenType::Percent;
            return binary(current, binaryOp, rhs);
        };
        if (auto v = std::dynamic_pointer_cast<VariableExpr>(e->target)) {
            Value value = compoundValue(env_->get(v->name, e->line));
            env_->assign(v->name, value, e->line);
            return value;
        }
        if (auto m = std::dynamic_pointer_cast<MemberExpr>(e->target)) {
            if (!m->computed) RuntimeErrorHandler::throwError("TypeError", "only indexed assignment is supported", e->line);
            Value object = evaluate(m->object);
            Value index = evaluate(m->index);
            Value value = compoundValue(getIndex(object, index, e->line));
            setIndex(object, index, value, e->line);
            return value;
        }
    } else if (auto e = std::dynamic_pointer_cast<MemberExpr>(expr)) {
        Value object = evaluate(e->object);
        return e->computed ? getIndex(object, evaluate(e->index), e->line) : getMember(object, e->name, e->line);
    } else if (auto e = std::dynamic_pointer_cast<CallExpr>(expr)) {
        std::vector<Value> args;
        for (auto& arg : e->args) args.push_back(evaluate(arg));
        if (auto m = std::dynamic_pointer_cast<MemberExpr>(e->callee)) return callMember(m, args, e->line);
        Value callee = evaluate(e->callee);
        if (callee.type != Value::Type::Function) RuntimeErrorHandler::throwError("TypeError", "attempted to call non-function", e->line);
        return callFunction(callee.function, args, e->line);
    }
    return Value::undefined();
}

Value Interpreter::callFunction(const std::shared_ptr<FunctionValue>& fn, const std::vector<Value>& args, int line) {
    if (fn->native) return fn->nativeCall(args, line);
    auto local = std::make_shared<Environment>(fn->closure);
    for (size_t i = 0; i < fn->params.size(); ++i) {
        local->define(fn->params[i], i < args.size() ? args[i] : Value::undefined(), false);
    }
    try {
        executeBlock(fn->body, local);
    } catch (const ReturnSignal& r) {
        return r.value;
    }
    return Value::undefined();
}

Value Interpreter::callMember(const std::shared_ptr<MemberExpr>& member, const std::vector<Value>& args, int line) {
    Value object = evaluate(member->object);
    if (member->computed) RuntimeErrorHandler::throwError("TypeError", "indexed value is not callable", line);
    const std::string& name = member->name;

    if (object.type == Value::Type::Object && object.str == "console" && name == "log") {
        for (size_t i = 0; i < args.size(); ++i) {
            if (i) out_ << ' ';
            out_ << args[i].toString();
        }
        out_ << '\n';
        return Value::undefined();
    }

    if (object.type == Value::Type::Object && object.str == "Math") {
        auto arg = [&](size_t i) -> double {
            if (i >= args.size()) return 0;
            return requireNumber(args[i], line, "Math." + name);
        };
        if (name == "floor") return Value::numberValue(std::floor(arg(0)));
        if (name == "ceil") return Value::numberValue(std::ceil(arg(0)));
        if (name == "round") return Value::numberValue(std::round(arg(0)));
        if (name == "abs") return Value::numberValue(std::fabs(arg(0)));
        if (name == "max") { double m = args.empty() ? -INFINITY : arg(0); for (size_t i = 1; i < args.size(); ++i) m = std::max(m, arg(i)); return Value::numberValue(m); }
        if (name == "min") { double m = args.empty() ? INFINITY : arg(0); for (size_t i = 1; i < args.size(); ++i) m = std::min(m, arg(i)); return Value::numberValue(m); }
        if (name == "pow") return Value::numberValue(std::pow(arg(0), arg(1)));
        RuntimeErrorHandler::throwError("ReferenceError", "function 'Math." + name + "' is not defined", line);
    }

    if (object.type == Value::Type::Array) {
        if (name == "push") {
            for (const auto& a : args) object.array->push_back(a);
            return Value::numberValue(static_cast<double>(object.array->size()));
        }
        if (name == "pop") {
            if (object.array->empty()) return Value::undefined();
            Value v = object.array->back();
            object.array->pop_back();
            return v;
        }
        if (name == "reverse") {
            std::reverse(object.array->begin(), object.array->end());
            return object;
        }
        if (name == "join") {
            std::string sep = args.empty() ? "," : args[0].toString();
            std::string s;
            for (size_t i = 0; i < object.array->size(); ++i) {
                if (i) s += sep;
                s += (*object.array)[i].toString();
            }
            return Value::stringValue(s);
        }
        if (name == "includes") {
            if (args.empty()) return Value::boolValue(false);
            return Value::boolValue(std::any_of(object.array->begin(), object.array->end(), [&](const Value& v) { return strictEqual(v, args[0]); }));
        }
    }

    if (object.type == Value::Type::String) {
        if (name == "split") {
            std::string sep = args.empty() ? "" : args[0].toString();
            std::vector<Value> parts;
            if (sep.empty()) {
                for (char c : object.str) parts.push_back(Value::stringValue(std::string(1, c)));
            } else {
                size_t pos = 0;
                while (true) {
                    size_t found = object.str.find(sep, pos);
                    if (found == std::string::npos) {
                        parts.push_back(Value::stringValue(object.str.substr(pos)));
                        break;
                    }
                    parts.push_back(Value::stringValue(object.str.substr(pos, found - pos)));
                    pos = found + sep.size();
                }
            }
            return Value::arrayValue(parts);
        }
        if (name == "substring") {
            int start = args.empty() ? 0 : toIndex(args[0], line);
            int end = args.size() < 2 ? static_cast<int>(object.str.size()) : toIndex(args[1], line);
            start = std::max(0, std::min(start, static_cast<int>(object.str.size())));
            end = std::max(0, std::min(end, static_cast<int>(object.str.size())));
            if (start > end) std::swap(start, end);
            return Value::stringValue(object.str.substr(start, end - start));
        }
        if (name == "includes") {
            std::string needle = args.empty() ? "undefined" : args[0].toString();
            return Value::boolValue(object.str.find(needle) != std::string::npos);
        }
    }

    RuntimeErrorHandler::throwError("ReferenceError", "function '" + name + "' is not defined", line);
}

Value Interpreter::getMember(const Value& object, const std::string& name, int line) {
    if ((object.type == Value::Type::Array || object.type == Value::Type::String) && name == "length") {
        return Value::numberValue(object.type == Value::Type::Array ? static_cast<double>(object.array->size()) : static_cast<double>(object.str.size()));
    }
    if (object.type == Value::Type::Object && (object.str == "console" || object.str == "Math")) return object;
    RuntimeErrorHandler::throwError("ReferenceError", "property '" + name + "' is not defined", line);
}

Value Interpreter::getIndex(const Value& object, const Value& index, int line) {
    int i = toIndex(index, line);
    if (object.type == Value::Type::Array) {
        if (i < 0 || i >= static_cast<int>(object.array->size())) RuntimeErrorHandler::throwError("RuntimeError", "array index out of bounds", line);
        return (*object.array)[i];
    }
    if (object.type == Value::Type::String) {
        if (i < 0 || i >= static_cast<int>(object.str.size())) RuntimeErrorHandler::throwError("RuntimeError", "string index out of bounds", line);
        return Value::stringValue(std::string(1, object.str[i]));
    }
    RuntimeErrorHandler::throwError("TypeError", "indexed access requires array or string", line);
}

void Interpreter::setIndex(Value& object, const Value& index, const Value& value, int line) {
    int i = toIndex(index, line);
    if (object.type != Value::Type::Array) RuntimeErrorHandler::throwError("TypeError", "indexed assignment requires array", line);
    if (i < 0 || i >= static_cast<int>(object.array->size())) RuntimeErrorHandler::throwError("RuntimeError", "array index out of bounds", line);
    (*object.array)[i] = value;
}

Value Interpreter::binary(const Value& left, const Token& op, const Value& right) {
    switch (op.type) {
        case TokenType::Plus:
            if (left.type == Value::Type::Number && right.type == Value::Type::Number) return Value::numberValue(left.number + right.number);
            if ((left.type == Value::Type::String || right.type == Value::Type::String) &&
                left.type != Value::Type::Array && right.type != Value::Type::Array &&
                left.type != Value::Type::Object && right.type != Value::Type::Object &&
                left.type != Value::Type::Function && right.type != Value::Type::Function) {
                return Value::stringValue(left.toString() + right.toString());
            }
            RuntimeErrorHandler::throwError("TypeError", "invalid operation between " + left.typeName() + " and " + right.typeName(), op.line);
        case TokenType::Minus:
        case TokenType::Star:
        case TokenType::StarStar:
        case TokenType::Slash:
        case TokenType::Percent: {
            if (left.type != Value::Type::Number || right.type != Value::Type::Number) {
                RuntimeErrorHandler::throwError("TypeError", "invalid operation between " + left.typeName() + " and " + right.typeName(), op.line);
            }
            if ((op.type == TokenType::Slash || op.type == TokenType::Percent) && right.number == 0) {
                RuntimeErrorHandler::throwError("RuntimeError", "division by zero", op.line);
            }
            if (op.type == TokenType::Minus) return Value::numberValue(left.number - right.number);
            if (op.type == TokenType::Star) return Value::numberValue(left.number * right.number);
            if (op.type == TokenType::StarStar) return Value::numberValue(std::pow(left.number, right.number));
            if (op.type == TokenType::Slash) return Value::numberValue(left.number / right.number);
            return Value::numberValue(std::fmod(left.number, right.number));
        }
        case TokenType::Greater:
        case TokenType::GreaterEqual:
        case TokenType::Less:
        case TokenType::LessEqual: {
            double a = requireNumber(left, op.line, "comparison");
            double b = requireNumber(right, op.line, "comparison");
            if (op.type == TokenType::Greater) return Value::boolValue(a > b);
            if (op.type == TokenType::GreaterEqual) return Value::boolValue(a >= b);
            if (op.type == TokenType::Less) return Value::boolValue(a < b);
            return Value::boolValue(a <= b);
        }
        case TokenType::EqualEqualEqual: return Value::boolValue(strictEqual(left, right));
        case TokenType::BangEqualEqual: return Value::boolValue(!strictEqual(left, right));
        default: break;
    }
    RuntimeErrorHandler::throwError("RuntimeError", "unknown binary operator '" + op.lexeme + "'", op.line);
}

bool Interpreter::strictEqual(const Value& a, const Value& b) const {
    if (a.type != b.type) return false;
    switch (a.type) {
        case Value::Type::Undefined: return true;
        case Value::Type::Number: return a.number == b.number;
        case Value::Type::String: return a.str == b.str;
        case Value::Type::Bool: return a.boolean == b.boolean;
        case Value::Type::Array: return a.array == b.array;
        case Value::Type::Object: return a.str == b.str;
        case Value::Type::Function: return a.function == b.function;
    }
    return false;
}

double Interpreter::requireNumber(const Value& value, int line, const std::string& context) const {
    if (value.type != Value::Type::Number) {
        RuntimeErrorHandler::throwError("TypeError", context + " requires number, got " + value.typeName(), line);
    }
    return value.number;
}

int Interpreter::toIndex(const Value& value, int line) const {
    double n = requireNumber(value, line, "index");
    return static_cast<int>(n);
}

void Interpreter::debugStep(const std::string& text, int line) {
    if (debug_) err_ << "[debug] line " << line << ": " << text << '\n';
}
