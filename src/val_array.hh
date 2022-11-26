//
// ALOX-CC
//

#pragma once

#include "value.hh"

class ValueArray {
  public:
    ValueArray() = default;
    ~ValueArray() { freeValueArray(); };

    void                        init();
    void                        writeValueArray(const Value &value);
    void                        freeValueArray();
    [[nodiscard]] constexpr int get_count() const { return count; };
    void                        markArray();
    constexpr Value            &get_value(size_t n) const { return values[n]; }

  private:
    int    capacity{0};
    int    count{0};
    Value *values{nullptr};
};