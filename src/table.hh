//
// ALOX-CC
//

#pragma once

#include "common.hh"
#include "value.hh"

struct Entry {
    ObjString *key;
    Value      value;
};

class Table {
  public:
    Table() = default;

    Table(const Table &) = delete;

    void init();
    void free();

    bool        get(ObjString *key, Value *value);
    bool        set(ObjString *key, Value value);
    bool        del(ObjString *key);
    static void addAll(Table *from, Table *to);
    ObjString  *findString(const char *chars, int length, uint32_t hash);

    void removeWhite();
    void mark();

  private:
    void adjustCapacity(int capacity);

    int    count{0};
    int    capacity{0};
    Entry *entries{nullptr};
};
