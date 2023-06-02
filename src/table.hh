//
// ALOX-CC
//

#pragma once

#include "common.hh"
#include "value.hh"

namespace alox {

struct Entry {
    ObjString *key;
    Value      value;
};

class Table {
  public:
    Table() = default;

    Table(const Table &) = delete;

    void free();

    bool        get(ObjString *key, Value *value);
    bool        set(ObjString *key, Value value);
    bool        del(ObjString *key);
    static void addAll(const Table &from, Table &to);

    void removeWhite();
    void mark();

  private:
    void adjustCapacity(size_t capacity);

    size_t count{0};
    size_t capacity{0};
    Entry *entries{nullptr};
};

} // namespace lox
