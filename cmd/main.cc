//
// ALOX-CC
//

#include <iostream>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-align"
#include <gc_cpp.h>
#pragma clang diagnostic pop

#include "alox.hh"
#include "options.hh"

int main(int argc, const char *argv[]) {

    GC_INIT();

    alox::Options options(std::cout, std::cin, std::cerr);
    getOptions(argc, argv, options);

    alox::Alox alox(options);

    if (options.file_name.empty()) {
        alox.repl();
    } else {
        return alox.runFile(options.file_name);
    }
    return 0;
}
