//
// ALOX-CC
//

#pragma once

#include "value.hh"

#include <vector>

namespace alox {

class ValueArray {
  public:
    ValueArray() = default;
    ~ValueArray() = default;

    ValueArray(const ValueArray &) = delete;

    size_t write(const Value &value);
    void   mark();

    [[nodiscard]] constexpr Value &get_value(size_t n) { return values[n]; }

  private:
    std::vector<Value> values{};
};

} // namespace lox