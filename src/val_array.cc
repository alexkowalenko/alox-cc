
//
// ALOX-CC
//

#include "val_array.hh"
#include "memory.hh"
#include <vector>

void ValueArray::write(const Value &value) {
    values->push_back(value);
}

void ValueArray::init() {
    values = new std::vector<Value>;
}

void ValueArray::free() {
    delete values;
    init();
}

void ValueArray::mark() {
    for (auto &v : *values) {
        gc.markValue(v);
    }
}