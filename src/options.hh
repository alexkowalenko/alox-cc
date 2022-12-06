//
// ALOX-CC
//
// Copyright © Alex Kowalenko 2022.
//

#pragma once

#include <string>

struct Options {
    bool parse{false};
    bool debug_code{false};
    bool trace{false};
    bool silent{false};

    std::string file_name;
};

int getOptions(int argc, const char *argv[], Options &options);
