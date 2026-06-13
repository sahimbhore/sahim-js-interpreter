#pragma once

#include "lexer.h"

#include <memory>
#include <string>
#include <vector>

struct Literal {
    enum class Type { Number, String, Bool, Undefined } type = Type::Undefined;
    double number = 0;
    std::string str;
    bool boolean = false;
};

struct Expr {
    explicit Expr(int line) : line(line) {}
    virtual ~Expr() = default;
    int line;
};

using ExprPtr = std::shared_ptr<Expr>;

struct LiteralExpr : Expr { Literal value; LiteralExpr(Literal v, int l) : Expr(l), value(std::move(v)) {} };
struct VariableExpr : Expr { std::string name; VariableExpr(std::string n, int l) : Expr(l), name(std::move(n)) {} };
struct AssignExpr : Expr { ExprPtr target; ExprPtr value; Token op; AssignExpr(ExprPtr t, ExprPtr v, Token o) : Expr(o.line), target(std::move(t)), value(std::move(v)), op(std::move(o)) {} };
struct BinaryExpr : Expr { ExprPtr left; Token op; ExprPtr right; BinaryExpr(ExprPtr a, Token o, ExprPtr b) : Expr(o.line), left(std::move(a)), op(std::move(o)), right(std::move(b)) {} };
struct UnaryExpr : Expr { Token op; ExprPtr right; UnaryExpr(Token o, ExprPtr r) : Expr(o.line), op(std::move(o)), right(std::move(r)) {} };
struct CallExpr : Expr { ExprPtr callee; std::vector<ExprPtr> args; CallExpr(ExprPtr c, std::vector<ExprPtr> a, int l) : Expr(l), callee(std::move(c)), args(std::move(a)) {} };
struct MemberExpr : Expr { ExprPtr object; std::string name; ExprPtr index; bool computed; MemberExpr(ExprPtr o, std::string n, int l) : Expr(l), object(std::move(o)), name(std::move(n)), computed(false) {} MemberExpr(ExprPtr o, ExprPtr i, int l) : Expr(l), object(std::move(o)), index(std::move(i)), computed(true) {} };
struct ArrayExpr : Expr { std::vector<ExprPtr> elements; std::vector<bool> spread; ArrayExpr(std::vector<ExprPtr> e, std::vector<bool> s, int l) : Expr(l), elements(std::move(e)), spread(std::move(s)) {} };

struct Stmt {
    explicit Stmt(int line) : line(line) {}
    virtual ~Stmt() = default;
    int line;
};

using StmtPtr = std::shared_ptr<Stmt>;

struct ExprStmt : Stmt { ExprPtr expr; ExprStmt(ExprPtr e, int l) : Stmt(l), expr(std::move(e)) {} };
struct VarStmt : Stmt { std::string kind; std::string name; ExprPtr init; VarStmt(std::string k, std::string n, ExprPtr i, int l) : Stmt(l), kind(std::move(k)), name(std::move(n)), init(std::move(i)) {} };
struct BlockStmt : Stmt { std::vector<StmtPtr> statements; BlockStmt(std::vector<StmtPtr> s, int l) : Stmt(l), statements(std::move(s)) {} };
struct IfStmt : Stmt { ExprPtr cond; StmtPtr thenBranch; StmtPtr elseBranch; IfStmt(ExprPtr c, StmtPtr t, StmtPtr e, int l) : Stmt(l), cond(std::move(c)), thenBranch(std::move(t)), elseBranch(std::move(e)) {} };
struct WhileStmt : Stmt { ExprPtr cond; StmtPtr body; WhileStmt(ExprPtr c, StmtPtr b, int l) : Stmt(l), cond(std::move(c)), body(std::move(b)) {} };
struct ForStmt : Stmt { StmtPtr init; ExprPtr cond; ExprPtr inc; StmtPtr body; ForStmt(StmtPtr i, ExprPtr c, ExprPtr in, StmtPtr b, int l) : Stmt(l), init(std::move(i)), cond(std::move(c)), inc(std::move(in)), body(std::move(b)) {} };
struct FunctionStmt : Stmt { std::string name; std::vector<std::string> params; std::vector<StmtPtr> body; FunctionStmt(std::string n, std::vector<std::string> p, std::vector<StmtPtr> b, int l) : Stmt(l), name(std::move(n)), params(std::move(p)), body(std::move(b)) {} };
struct ReturnStmt : Stmt { ExprPtr value; ReturnStmt(ExprPtr v, int l) : Stmt(l), value(std::move(v)) {} };
struct EmptyStmt : Stmt { explicit EmptyStmt(int l) : Stmt(l) {} };

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);
    std::vector<StmtPtr> parse();

private:
    std::vector<Token> tokens_;
    size_t current_ = 0;

    StmtPtr declaration();
    StmtPtr varDeclaration(const std::string& kind);
    StmtPtr functionDeclaration();
    StmtPtr statement();
    StmtPtr ifStatement();
    StmtPtr whileStatement();
    StmtPtr forStatement();
    StmtPtr returnStatement();
    StmtPtr blockStatement();
    StmtPtr expressionStatement();

    ExprPtr expression();
    ExprPtr assignment();
    ExprPtr logicalOr();
    ExprPtr logicalAnd();
    ExprPtr equality();
    ExprPtr comparison();
    ExprPtr term();
    ExprPtr factor();
    ExprPtr exponent();
    ExprPtr unary();
    ExprPtr postfix();
    ExprPtr call();
    ExprPtr primary();

    bool match(TokenType type);
    bool check(TokenType type) const;
    bool checkNext(TokenType type) const;
    Token advance();
    bool isAtEnd() const;
    Token peek() const;
    Token previous() const;
    Token consume(TokenType type, const std::string& message);
    void optionalSemicolon();
};
