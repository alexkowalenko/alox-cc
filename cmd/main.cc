//
// ALOX-CC
//

#include <iostream>

#include "alox.hh"

int main(int argc, const char *argv[]) {

    Alox alox;

    if (argc == 1) {
        alox.repl();
    } else if (argc == 2) {
        return alox.runFile(argv[1]);
    } else {
        std::cerr << "Usage: alox [path]\n";
        return 64;
    }
    return 0;
}
