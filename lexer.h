#pragma once

#include <string>
#include <vector>

enum class TokenType {
    LeftParen, RightParen, LeftBrace, RightBrace, LeftBracket, RightBracket,
    Comma, Dot, Semicolon,
    Plus, Minus, Star, Slash, Percent, Bang, Equal,
    Greater, Less,
    PlusEqual, MinusEqual, StarEqual, SlashEqual, PercentEqual, StarStar,
    BangEqualEqual, EqualEqualEqual, GreaterEqual, LessEqual,
    AndAnd, OrOr, PlusPlus, MinusMinus,
    Ellipsis,
    Identifier, String, Number,
    Let, Const, Function, Return, If, Else, For, While, True, False,
    EndOfFile
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
};

class Lexer {
public:
    explicit Lexer(std::string source);
    std::vector<Token> tokenize();

private:
    std::string source_;
    std::vector<Token> tokens_;
    size_t start_ = 0;
    size_t current_ = 0;
    int line_ = 1;

    bool isAtEnd() const;
    char advance();
    char peek() const;
    char peekNext() const;
    bool match(char expected);
    void add(TokenType type);
    void scanToken();
    void stringLiteral(char quote);
    void number();
    void identifier();
};
