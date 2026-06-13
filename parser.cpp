#include "parser.h"
#include "utils.h"

#include <cstdlib>

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

std::vector<StmtPtr> Parser::parse() {
    std::vector<StmtPtr> statements;
    while (!isAtEnd()) statements.push_back(declaration());
    return statements;
}

StmtPtr Parser::declaration() {
    if (match(TokenType::Let)) return varDeclaration("let");
    if (match(TokenType::Const)) return varDeclaration("const");
    if (match(TokenType::Function)) return functionDeclaration();
    return statement();
}

StmtPtr Parser::varDeclaration(const std::string& kind) {
    Token name = consume(TokenType::Identifier, "expected variable name");
    ExprPtr init;
    if (match(TokenType::Equal)) init = expression();
    optionalSemicolon();
    return std::make_shared<VarStmt>(kind, name.lexeme, init, name.line);
}

StmtPtr Parser::functionDeclaration() {
    Token name = consume(TokenType::Identifier, "expected function name");
    consume(TokenType::LeftParen, "expected '(' after function name");
    std::vector<std::string> params;
    if (!check(TokenType::RightParen)) {
        do {
            params.push_back(consume(TokenType::Identifier, "expected parameter name").lexeme);
        } while (match(TokenType::Comma));
    }
    consume(TokenType::RightParen, "expected ')' after parameters");
    consume(TokenType::LeftBrace, "expected '{' before function body");
    auto body = std::dynamic_pointer_cast<BlockStmt>(blockStatement())->statements;
    return std::make_shared<FunctionStmt>(name.lexeme, params, body, name.line);
}

StmtPtr Parser::statement() {
    if (match(TokenType::Semicolon)) return std::make_shared<EmptyStmt>(previous().line);
    if (match(TokenType::If)) return ifStatement();
    if (match(TokenType::While)) return whileStatement();
    if (match(TokenType::For)) return forStatement();
    if (match(TokenType::Return)) return returnStatement();
    if (match(TokenType::LeftBrace)) return blockStatement();
    return expressionStatement();
}

StmtPtr Parser::ifStatement() {
    int line = previous().line;
    consume(TokenType::LeftParen, "expected '(' after if");
    auto cond = expression();
    consume(TokenType::RightParen, "expected ')' after if condition");
    auto thenBranch = statement();
    StmtPtr elseBranch;
    if (match(TokenType::Else)) elseBranch = statement();
    return std::make_shared<IfStmt>(cond, thenBranch, elseBranch, line);
}

StmtPtr Parser::whileStatement() {
    int line = previous().line;
    consume(TokenType::LeftParen, "expected '(' after while");
    auto cond = expression();
    consume(TokenType::RightParen, "expected ')' after while condition");
    return std::make_shared<WhileStmt>(cond, statement(), line);
}

StmtPtr Parser::forStatement() {
    int line = previous().line;
    consume(TokenType::LeftParen, "expected '(' after for");
    StmtPtr init;
    if (match(TokenType::Semicolon)) {
        init = nullptr;
    } else if (match(TokenType::Let)) {
        init = varDeclaration("let");
    } else if (match(TokenType::Const)) {
        init = varDeclaration("const");
    } else {
        auto e = expression();
        consume(TokenType::Semicolon, "expected ';' after for initializer");
        init = std::make_shared<ExprStmt>(e, e->line);
    }
    ExprPtr cond;
    if (!check(TokenType::Semicolon)) cond = expression();
    consume(TokenType::Semicolon, "expected ';' after for condition");
    ExprPtr inc;
    if (!check(TokenType::RightParen)) inc = expression();
    consume(TokenType::RightParen, "expected ')' after for clauses");
    return std::make_shared<ForStmt>(init, cond, inc, statement(), line);
}

StmtPtr Parser::returnStatement() {
    int line = previous().line;
    ExprPtr value;
    if (!check(TokenType::Semicolon) && !check(TokenType::RightBrace) && !isAtEnd()) value = expression();
    optionalSemicolon();
    return std::make_shared<ReturnStmt>(value, line);
}

StmtPtr Parser::blockStatement() {
    int line = previous().line;
    std::vector<StmtPtr> statements;
    while (!check(TokenType::RightBrace) && !isAtEnd()) statements.push_back(declaration());
    consume(TokenType::RightBrace, "expected '}' after block");
    return std::make_shared<BlockStmt>(statements, line);
}

StmtPtr Parser::expressionStatement() {
    auto expr = expression();
    optionalSemicolon();
    return std::make_shared<ExprStmt>(expr, expr->line);
}

ExprPtr Parser::expression() { return assignment(); }

ExprPtr Parser::assignment() {
    auto expr = logicalOr();
    if (match(TokenType::Equal) || match(TokenType::PlusEqual) || match(TokenType::MinusEqual) ||
        match(TokenType::StarEqual) || match(TokenType::SlashEqual) || match(TokenType::PercentEqual)) {
        Token op = previous();
        auto value = assignment();
        if (std::dynamic_pointer_cast<VariableExpr>(expr) || std::dynamic_pointer_cast<MemberExpr>(expr)) {
            return std::make_shared<AssignExpr>(expr, value, op);
        }
        RuntimeErrorHandler::throwError("SyntaxError", "invalid assignment target", op.line);
    }
    return expr;
}

ExprPtr Parser::logicalOr() {
    auto expr = logicalAnd();
    while (match(TokenType::OrOr)) {
        Token op = previous();
        auto right = logicalAnd();
        expr = std::make_shared<BinaryExpr>(expr, op, right);
    }
    return expr;
}

ExprPtr Parser::logicalAnd() {
    auto expr = equality();
    while (match(TokenType::AndAnd)) {
        Token op = previous();
        auto right = equality();
        expr = std::make_shared<BinaryExpr>(expr, op, right);
    }
    return expr;
}

ExprPtr Parser::equality() {
    auto expr = comparison();
    while (match(TokenType::EqualEqualEqual) || match(TokenType::BangEqualEqual)) {
        Token op = previous();
        auto right = comparison();
        expr = std::make_shared<BinaryExpr>(expr, op, right);
    }
    return expr;
}

ExprPtr Parser::comparison() {
    auto expr = term();
    while (match(TokenType::Greater) || match(TokenType::GreaterEqual) ||
           match(TokenType::Less) || match(TokenType::LessEqual)) {
        Token op = previous();
        auto right = term();
        expr = std::make_shared<BinaryExpr>(expr, op, right);
    }
    return expr;
}

ExprPtr Parser::term() {
    auto expr = factor();
    while (match(TokenType::Plus) || match(TokenType::Minus)) {
        Token op = previous();
        auto right = factor();
        expr = std::make_shared<BinaryExpr>(expr, op, right);
    }
    return expr;
}

ExprPtr Parser::factor() {
    auto expr = exponent();
    while (match(TokenType::Star) || match(TokenType::Slash) || match(TokenType::Percent)) {
        Token op = previous();
        auto right = exponent();
        expr = std::make_shared<BinaryExpr>(expr, op, right);
    }
    return expr;
}

ExprPtr Parser::exponent() {
    auto expr = unary();
    if (match(TokenType::StarStar)) {
        Token op = previous();
        auto right = exponent();
        expr = std::make_shared<BinaryExpr>(expr, op, right);
    }
    return expr;
}

ExprPtr Parser::unary() {
    if (match(TokenType::Bang) || match(TokenType::Minus)) return std::make_shared<UnaryExpr>(previous(), unary());
    return postfix();
}

ExprPtr Parser::postfix() {
    auto expr = call();
    if (match(TokenType::PlusPlus) || match(TokenType::MinusMinus)) {
        Token op = previous();
        Literal one;
        one.type = Literal::Type::Number;
        one.number = 1;
        auto plusOne = std::make_shared<BinaryExpr>(expr, Token{op.type == TokenType::PlusPlus ? TokenType::Plus : TokenType::Minus, op.lexeme, op.line}, std::make_shared<LiteralExpr>(one, op.line));
        return std::make_shared<AssignExpr>(expr, plusOne, Token{TokenType::Equal, "=", op.line});
    }
    return expr;
}

ExprPtr Parser::call() {
    auto expr = primary();
    while (true) {
        if (match(TokenType::LeftParen)) {
            std::vector<ExprPtr> args;
            if (!check(TokenType::RightParen)) {
                do { args.push_back(expression()); } while (match(TokenType::Comma));
            }
            Token paren = consume(TokenType::RightParen, "expected ')' after arguments");
            expr = std::make_shared<CallExpr>(expr, args, paren.line);
        } else if (match(TokenType::Dot)) {
            Token name = consume(TokenType::Identifier, "expected property name after '.'");
            expr = std::make_shared<MemberExpr>(expr, name.lexeme, name.line);
        } else if (match(TokenType::LeftBracket)) {
            auto idx = expression();
            Token bracket = consume(TokenType::RightBracket, "expected ']' after index");
            expr = std::make_shared<MemberExpr>(expr, idx, bracket.line);
        } else break;
    }
    return expr;
}

ExprPtr Parser::primary() {
    if (match(TokenType::False)) { Literal v; v.type = Literal::Type::Bool; v.boolean = false; return std::make_shared<LiteralExpr>(v, previous().line); }
    if (match(TokenType::True)) { Literal v; v.type = Literal::Type::Bool; v.boolean = true; return std::make_shared<LiteralExpr>(v, previous().line); }
    if (match(TokenType::Number)) { Literal v; v.type = Literal::Type::Number; v.number = std::stod(previous().lexeme); return std::make_shared<LiteralExpr>(v, previous().line); }
    if (match(TokenType::String)) { Literal v; v.type = Literal::Type::String; v.str = previous().lexeme; return std::make_shared<LiteralExpr>(v, previous().line); }
    if (match(TokenType::Identifier)) return std::make_shared<VariableExpr>(previous().lexeme, previous().line);
    if (match(TokenType::LeftParen)) {
        auto expr = expression();
        consume(TokenType::RightParen, "expected ')' after expression");
        return expr;
    }
    if (match(TokenType::LeftBracket)) {
        int line = previous().line;
        std::vector<ExprPtr> elements;
        std::vector<bool> spread;
        if (!check(TokenType::RightBracket)) {
            do {
                bool isSpread = match(TokenType::Ellipsis);
                elements.push_back(expression());
                spread.push_back(isSpread);
            } while (match(TokenType::Comma));
        }
        consume(TokenType::RightBracket, "expected ']' after array literal");
        return std::make_shared<ArrayExpr>(elements, spread, line);
    }
    RuntimeErrorHandler::throwError("SyntaxError", "expected expression", peek().line);
}

bool Parser::match(TokenType type) {
    if (!check(type)) return false;
    advance();
    return true;
}

bool Parser::check(TokenType type) const {
    return !isAtEnd() && peek().type == type;
}

bool Parser::checkNext(TokenType type) const {
    return current_ + 1 < tokens_.size() && tokens_[current_ + 1].type == type;
}

Token Parser::advance() {
    if (!isAtEnd()) current_++;
    return previous();
}

bool Parser::isAtEnd() const { return peek().type == TokenType::EndOfFile; }

Token Parser::peek() const { return tokens_[current_]; }

Token Parser::previous() const { return tokens_[current_ - 1]; }

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    RuntimeErrorHandler::throwError("SyntaxError", message, peek().line);
}

void Parser::optionalSemicolon() {
    match(TokenType::Semicolon);
}
