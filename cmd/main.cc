//
// ALOX-CC
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "alox.hh"

int main(int argc, const char *argv[]) {

    Alox alox;

    if (argc == 1) {
        alox.repl();
    } else if (argc == 2) {
        alox.runFile(argv[1]);
    } else {
        fprintf(stderr, "Usage: alox [path]\n");
        exit(64);
    }
    return 0;
}
