//
// ALOX-CC
//

#pragma once

#include "scanner.hh"

namespace alox {

class ErrorManager {
  public:
    ErrorManager(std::ostream &cerr) : cerr(cerr){};

    void errorAt(Token *token, std::string_view message);
    void errorAt(size_t line, std::string_view message);

    void reset() {
        panicMode = false;
        hadError = false;
    }

    bool panicMode{false};
    bool hadError{false};

  private:
    std::ostream &cerr;
};

} // namespace alox