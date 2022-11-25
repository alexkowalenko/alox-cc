//
// ALOX-CC
//
// Copyright © Alex Kowalenko 2022.
//

#include <cstdio>
#include <cstdlib>

#include "linenoise.h"

#include "alox.hh"
#include "vm.hh"

constexpr auto history_file = "./alox-cc";

Alox::Alox() {
    initVM();
}

Alox::~Alox() {
    freeVM();
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
        interpret(result);
        linenoiseHistoryAdd(result);
        free(result);
    }
    linenoiseHistorySave(history_file);
    linenoiseHistoryFree();
}

char *Alox::readFile(const char *path) {
    FILE *file = fopen(path, "rb");
    if (file == nullptr) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }

    fseek(file, 0L, SEEK_END);
    auto fileSize = size_t(ftell(file));
    rewind(file);

    char *buffer = (char *)malloc(fileSize + 1);
    if (buffer == nullptr) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }

    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize) {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }

    buffer[bytesRead] = '\0';

    fclose(file);
    return buffer;
}

void Alox::runFile(const char *path) {
    char           *source = readFile(path);
    InterpretResult result = interpret(source);
    free(source); // [owner]

    if (result == INTERPRET_COMPILE_ERROR)
        exit(65);
    if (result == INTERPRET_RUNTIME_ERROR)
        exit(70);
};

void Alox::runString(const std::string_view &s) {
    InterpretResult result = interpret(s.data());
}
