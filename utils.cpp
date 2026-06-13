#include "utils.h"

#include <cmath>
#include <iomanip>
#include <sstream>

JsError::JsError(std::string type, std::string message, int line)
    : std::runtime_error(message), type_(std::move(type)),
      message_(std::move(message)), line_(line) {}

const std::string& JsError::type() const { return type_; }

const std::string& JsError::message() const { return message_; }

int JsError::line() const { return line_; }

std::string JsError::format() const {
    std::ostringstream out;
    out << "Error: " << type_;
    if (line_ > 0) {
        out << " at line " << line_;
    }
    out << ": " << message_;
    return out.str();
}

void RuntimeErrorHandler::throwError(const std::string& type,
                                     const std::string& message,
                                     int line) {
    throw JsError(type, message, line);
}

std::string trimNumber(double value) {
    if (std::isnan(value)) return "NaN";
    if (std::isinf(value)) return value > 0 ? "Infinity" : "-Infinity";
    if (std::fabs(value) < 1e-12) value = 0;

    std::ostringstream out;
    out << std::setprecision(15) << value;
    std::string s = out.str();
    if (s.find('.') != std::string::npos) {
        while (!s.empty() && s.back() == '0') s.pop_back();
        if (!s.empty() && s.back() == '.') s.pop_back();
    }
    return s;
}
