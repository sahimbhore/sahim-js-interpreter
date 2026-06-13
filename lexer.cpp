#include "lexer.h"
#include "utils.h"

#include <cctype>
#include <unordered_map>

Lexer::Lexer(std::string source) : source_(std::move(source)) {}

std::vector<Token> Lexer::tokenize() {
    while (!isAtEnd()) {
        start_ = current_;
        scanToken();
    }
    tokens_.push_back({TokenType::EndOfFile, "", line_});
    return tokens_;
}

bool Lexer::isAtEnd() const { return current_ >= source_.size(); }

char Lexer::advance() { return source_[current_++]; }

char Lexer::peek() const { return isAtEnd() ? '\0' : source_[current_]; }

char Lexer::peekNext() const {
    return current_ + 1 >= source_.size() ? '\0' : source_[current_ + 1];
}

bool Lexer::match(char expected) {
    if (isAtEnd() || source_[current_] != expected) return false;
    current_++;
    return true;
}

void Lexer::add(TokenType type) {
    tokens_.push_back({type, source_.substr(start_, current_ - start_), line_});
}

void Lexer::scanToken() {
    char c = advance();
    switch (c) {
        case '(': add(TokenType::LeftParen); break;
        case ')': add(TokenType::RightParen); break;
        case '{': add(TokenType::LeftBrace); break;
        case '}': add(TokenType::RightBrace); break;
        case '[': add(TokenType::LeftBracket); break;
        case ']': add(TokenType::RightBracket); break;
        case ',': add(TokenType::Comma); break;
        case ';': add(TokenType::Semicolon); break;
        case '+':
            if (match('+')) add(TokenType::PlusPlus);
            else if (match('=')) add(TokenType::PlusEqual);
            else add(TokenType::Plus);
            break;
        case '-':
            if (match('-')) add(TokenType::MinusMinus);
            else if (match('=')) add(TokenType::MinusEqual);
            else add(TokenType::Minus);
            break;
        case '*':
            if (match('*')) add(TokenType::StarStar);
            else if (match('=')) add(TokenType::StarEqual);
            else add(TokenType::Star);
            break;
        case '%': add(match('=') ? TokenType::PercentEqual : TokenType::Percent); break;
        case '.':
            if (match('.') && match('.')) add(TokenType::Ellipsis);
            else add(TokenType::Dot);
            break;
        case '!':
            if (match('=') && match('=')) add(TokenType::BangEqualEqual);
            else add(TokenType::Bang);
            break;
        case '=':
            if (match('=') && match('=')) add(TokenType::EqualEqualEqual);
            else add(TokenType::Equal);
            break;
        case '>': add(match('=') ? TokenType::GreaterEqual : TokenType::Greater); break;
        case '<': add(match('=') ? TokenType::LessEqual : TokenType::Less); break;
        case '&':
            if (match('&')) add(TokenType::AndAnd);
            else RuntimeErrorHandler::throwError("SyntaxError", "unexpected character '&'", line_);
            break;
        case '|':
            if (match('|')) add(TokenType::OrOr);
            else RuntimeErrorHandler::throwError("SyntaxError", "unexpected character '|'", line_);
            break;
        case '/':
            if (match('/')) {
                while (peek() != '\n' && !isAtEnd()) advance();
            } else if (match('*')) {
                while (!(peek() == '*' && peekNext() == '/') && !isAtEnd()) {
                    if (advance() == '\n') line_++;
                }
                if (isAtEnd()) RuntimeErrorHandler::throwError("SyntaxError", "unterminated block comment", line_);
                advance();
                advance();
            } else {
                add(match('=') ? TokenType::SlashEqual : TokenType::Slash);
            }
            break;
        case '"':
        case '\'':
            stringLiteral(c);
            break;
        case ' ':
        case '\r':
        case '\t':
            break;
        case '\n':
            line_++;
            break;
        default:
            if (std::isdigit(static_cast<unsigned char>(c))) number();
            else if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') identifier();
            else RuntimeErrorHandler::throwError("SyntaxError", std::string("unexpected character '") + c + "'", line_);
            break;
    }
}

void Lexer::stringLiteral(char quote) {
    std::string value;
    while (peek() != quote && !isAtEnd()) {
        char c = advance();
        if (c == '\n') line_++;
        if (c == '\\' && !isAtEnd()) {
            char n = advance();
            if (n == 'n') value.push_back('\n');
            else if (n == 't') value.push_back('\t');
            else value.push_back(n);
        } else {
            value.push_back(c);
        }
    }
    if (isAtEnd()) RuntimeErrorHandler::throwError("SyntaxError", "unterminated string", line_);
    advance();
    tokens_.push_back({TokenType::String, value, line_});
}

void Lexer::number() {
    while (std::isdigit(static_cast<unsigned char>(peek()))) advance();
    if (peek() == '.' && std::isdigit(static_cast<unsigned char>(peekNext()))) {
        advance();
        while (std::isdigit(static_cast<unsigned char>(peek()))) advance();
    }
    add(TokenType::Number);
}

void Lexer::identifier() {
    while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_') advance();
    static const std::unordered_map<std::string, TokenType> keywords = {
        {"let", TokenType::Let}, {"const", TokenType::Const},
        {"function", TokenType::Function}, {"return", TokenType::Return},
        {"if", TokenType::If}, {"else", TokenType::Else},
        {"for", TokenType::For}, {"while", TokenType::While},
        {"true", TokenType::True}, {"false", TokenType::False},
    };
    std::string text = source_.substr(start_, current_ - start_);
    auto it = keywords.find(text);
    tokens_.push_back({it == keywords.end() ? TokenType::Identifier : it->second, text, line_});
}
