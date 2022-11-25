//
// ALOX-CC
//
// Copyright © Alex Kowalenko 2022.
//

#pragma once

#include <string_view>
class Alox {
  public:
    Alox();
    ~Alox();

    void runFile(const char *path);
    void runString(const std::string_view &s);
    void repl();

  private:
    char *readFile(const char *path);
};
