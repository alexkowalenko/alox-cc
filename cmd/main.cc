//
// ALOX-CC
//

#include <iostream>

#include "alox.hh"
#include "options.hh"

int main(int argc, const char *argv[]) {

    Options options(std::cout, std::cin, std::cerr);
    getOptions(argc, argv, options);

    Alox alox(options);

    if (options.file_name.empty()) {
        alox.repl();
    } else {
        return alox.runFile(options.file_name);
    }
    return 0;
}
