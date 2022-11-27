//
// ALOX-CC
//

#include <iostream>

#include "alox.hh"
#include "options.hh"

int main(int argc, const char *argv[]) {

    Options options;
    getOptions(argc, argv, options);

    Alox alox(options);

    if (options.file_name == "") {
        alox.repl();
    } else {
        return alox.runFile(options.file_name);
    }
    return 0;
}
