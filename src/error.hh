//
// ALOX-CC
//

#pragma once

#include "scanner.hh"

class Error {
  public:
    Error(std::ostream &cerr) : cerr(cerr){};

    void errorAt(Token *token, std::string_view message);
    void errorAt(int line, std::string_view message);

    void reset() {
        panicMode = false;
        hadError = false;
    }

    bool panicMode{false};
    bool hadError{false};

  private:
    std::ostream &cerr;
};