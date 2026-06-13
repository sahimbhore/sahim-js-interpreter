#include "interpreter.h"
#include "lexer.h"
#include "parser.h"
#include "utils.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

static std::string readAll(std::istream& in) {
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

static void printUsage() {
    std::cerr << "Usage: ./myjs [file.js] [--debug]\n"
              << "       ./myjs --debug < input.js\n";
}

int main(int argc, char** argv) {
    bool debug = false;
    std::string file;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--debug") {
            debug = true;
        } else if (file.empty()) {
            file = arg;
        } else {
            printUsage();
            return 1;
        }
    }

    try {
        std::string source;
        if (!file.empty()) {
            std::ifstream input(file);
            if (!input) {
                std::cerr << "Error: RuntimeError: unable to open file '" << file << "'\n";
                return 1;
            }
            source = readAll(input);
        } else {
            source = readAll(std::cin);
        }

        Lexer lexer(source);
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        auto program = parser.parse();
        Interpreter interpreter(debug);
        interpreter.interpret(program);
        return 0;
    } catch (const JsError& error) {
        std::cerr << error.format() << '\n';
        if (debug) std::cerr << "[debug] execution stopped at reported error\n";
        return 1;
    } catch (const std::exception& ex) {
        std::cerr << "Error: RuntimeError: " << ex.what() << '\n';
        return 1;
    }
}
