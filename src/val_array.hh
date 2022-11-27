//
// ALOX-CC
//

#pragma once

#include "value.hh"

#include <vector>

class ValueArray {
  public:
    ValueArray() = default;
    ~ValueArray() = default;

    ValueArray(const ValueArray &) = delete;

    void init();
    void free() { values = nullptr; };

    size_t write(const Value &value);
    void   mark();

    [[nodiscard]] constexpr Value &get_value(size_t n) { return (*values)[n]; }

  private:
    std::vector<Value> *values{nullptr};
};