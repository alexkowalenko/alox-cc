
//
// ALOX-CC
//

#include "val_array.hh"
#include "memory.hh"
#include <memory>
#include <vector>

size_t ValueArray::write(const Value &value) {
    auto result = std::find(values->begin(), values->end(), value);
    if (result != values->end()) {
        return std::distance(values->begin(), result);
    }
    values->push_back(value);
    return values->size() - 1;
}

void ValueArray::init() {
    values = new std::vector<Value>;
}

void ValueArray::mark() {
    if (values) {
        for (auto &v : *values) {
            gc.markValue(v);
        }
    }
}