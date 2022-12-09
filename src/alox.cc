//
// ALOX-CC
//
// Copyright © Alex Kowalenko 2022.
//

#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>

#include <fmt/core.h>
#include <linenoise.h>
#include <sstream>

#include "alox.hh"
#include "compiler.hh"
#include "error.hh"
#include "memory.hh"
#include "parser.hh"
#include "printer.hh"
#include "scanner.hh"
#include "vm.hh"

constexpr auto history_file = "./alox-cc";

Alox::Alox(const Options &opt) : options(opt), vm(options) {
    gc.init(&vm);
    vm.init();
}

Alox::~Alox() {
    vm.free();
    gc.freeObjects(); // and the last enemy to be destroyed is death.
}

void Alox::repl() {
    linenoiseInstallWindowChangeHandler();
    linenoiseInstallWindowChangeHandler();
    linenoiseHistoryLoad(history_file);
    for (;;) {
        auto *result = linenoise("> ");
        if (result == nullptr) {
            break;
        }
        runString(result);
        linenoiseHistoryAdd(result);
        free(result);
    }
    linenoiseHistorySave(history_file);
    linenoiseHistoryFree();
}

std::string Alox::readFile(const std::string_view &path) {
    const std::filesystem::path file{path};
    if (!std::filesystem::exists(file)) {
        throw std::runtime_error(fmt::format("file {} doesn't exist.", path));
    }
    std::ifstream ifs(path);
    std::string   buffer((std::istreambuf_iterator<char>(ifs)),
                         std::istreambuf_iterator<char>());
    return buffer;
}

int Alox::runFile(const std::string_view &path) {
    InterpretResult result{INTERPRET_OK};
    try {
        auto source = readFile(path);
        result = runString(source);
    } catch (std::exception &e) {
        std::cerr << e.what() << '\n';
        return 74;
    }

    if (result == INTERPRET_COMPILE_ERROR) {
        return 65;
    }
    if (result == INTERPRET_RUNTIME_ERROR) {
        return 70;
    }
    return 0;
};

InterpretResult Alox::runString(const std::string &source) {

    auto scanner = Scanner(source);
    auto errors = Error(options.err);
    auto parser = Parser(scanner, errors);

    auto ast = parser.parse();
    if (errors.hadError) {
        return INTERPRET_PARSE_ERROR;
    }
    if (options.parse) {
        std::stringstream os;
        AST_Printer       printer(os);
        printer.print(ast);
        std::cout << os.str();
    }

    Compiler compiler(options, errors);
    gc.set_compiler(&compiler);
    ObjFunction *function = compiler.compile(ast);
    if (function == nullptr) {
        return INTERPRET_COMPILE_ERROR;
    }
    InterpretResult result = vm.run(function);
    return result;
}
