//
// ALOX-CC
//
// Copyright © Alex Kowalenko 2022.
//

#pragma once

#include <string_view>

#include "options.hh"
#include "vm.hh"

class Alox {
  public:
    Alox(const Options &opt);
    ~Alox();

    int  runFile(const std::string_view &path);
    void runString(const std::string_view &s);
    void repl();

  private:
    static std::string readFile(const std::string_view &path);

    const Options &options;
    VM             vm;
};
