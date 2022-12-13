//
// ALOX-CC
//
// Copyright © Alex Kowalenko 2022.
//

#pragma once

#include <iostream>
#include <string>

struct Options {

    Options(std::ostream &out, std::istream &in, std::ostream &err)
        : out(out), in(in), err(err) {}
    bool parse{false};
    bool debug_code{false};
    bool trace{false};
    bool silent{false};

    std::string file_name;

    std::ostream &out;
    std::istream &in;
    std::ostream &err;
};

int getOptions(int argc, const char *argv[], Options &options);
