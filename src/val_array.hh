//
// ALOX-CC
//

#pragma once

#include "value.hh"

#include <vector>

class ValueArray {
  public:
    ValueArray() = default;
    ~ValueArray() { free(); };

    ValueArray(const ValueArray &) = delete;

    void init();
    void free();

    void write(const Value &value);
    void mark();

    [[nodiscard]] constexpr size_t get_count() const { return values->size(); };
    [[nodiscard]] constexpr Value &get_value(size_t n) { return (*values)[n]; }

  private:
    std::vector<Value> *values{nullptr};
};