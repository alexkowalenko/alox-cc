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
    void        init();
    void        freeTable();
    
    bool        tableGet(ObjString *key, Value *value);
    bool        tableSet(ObjString *key, Value value);
    bool        tableDelete(ObjString *key);
    static void tableAddAll(Table *from, Table *to);
    ObjString  *tableFindString(const char *chars, int length, uint32_t hash);

    void tableRemoveWhite();
    void markTable();

  private:
    void adjustCapacity(int capacity);

    int    count{0};
    int    capacity{0};
    Entry *entries{nullptr};
};
