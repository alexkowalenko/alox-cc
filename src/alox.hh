//
// ALOX-CC
//
// Copyright © Alex Kowalenko 2022.
//

#pragma once

#include <string_view>

#include "vm.hh"

class Alox {
  public:
    Alox();
    ~Alox();

    int  runFile(const std::string_view &path);
    void runString(const std::string_view &s);
    void repl();

  private:
    std::string readFile(const std::string_view &path);
    VM          vm;
};
