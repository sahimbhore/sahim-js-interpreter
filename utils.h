#pragma once

#include <stdexcept>
#include <string>

class JsError : public std::runtime_error {
public:
    JsError(std::string type, std::string message, int line);

    const std::string& type() const;
    const std::string& message() const;
    int line() const;
    std::string format() const;

private:
    std::string type_;
    std::string message_;
    int line_;
};

class RuntimeErrorHandler {
public:
    [[noreturn]] static void throwError(const std::string& type,
                                        const std::string& message,
                                        int line);
};

std::string trimNumber(double value);
