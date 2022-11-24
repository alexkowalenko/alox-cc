//
// ALOX-CC
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "linenoise.h"

#include "chunk.hh"
#include "common.hh"
#include "debug.hh"
#include "vm.hh"

constexpr auto history_file = "./alox-cc";

static void repl() {
    for (;;) {
        auto *result = linenoise("> ");
        if (result == nullptr) {
            break;
        }
        interpret(result);
        linenoiseHistoryAdd(result);
        free(result);
    }
}

static char *readFile(const char *path) {
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

static void runFile(const char *path) {
    char           *source = readFile(path);
    InterpretResult result = interpret(source);
    free(source); // [owner]

    if (result == INTERPRET_COMPILE_ERROR)
        exit(65);
    if (result == INTERPRET_RUNTIME_ERROR)
        exit(70);
}

int main(int argc, const char *argv[]) {
    linenoiseInstallWindowChangeHandler();

    initVM();

    if (argc == 1) {
        linenoiseInstallWindowChangeHandler();
        linenoiseHistoryLoad(history_file);
        repl();
        linenoiseHistorySave(history_file);
        linenoiseHistoryFree();
    } else if (argc == 2) {
        runFile(argv[1]);
    } else {
        fprintf(stderr, "Usage: clox [path]\n");
        exit(64);
    }

    freeVM();
    return 0;
}
