//
// ALOX-CC
//
// Copyright © Alex Kowalenko 2022.
//

#include "options.hh"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"
#include <CLI/CLI.hpp>
#pragma clang diagnostic pop

int getOptions(int argc, const char *argv[], Options &options) {

    CLI::App app{"alox: Alox"};

    app.add_flag("-s,--silent", options.silent, "silent, don't print the prompt");
    app.add_option("file", options.file_name, "file to run");

    app.add_flag("-p,--parse", options.parse, "print the parsing");
    app.add_flag("-d,--debug", options.debug_code, "print the bytecode and exit");
    app.add_flag("-x,--trace", options.trace, "trace execution");

    CLI11_PARSE(app, argc, argv);
    return 0;
}