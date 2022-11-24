
//
// ALOX-CC
//

#include "val_array.hh"
#include "memory.hh"

void ValueArray::writeValueArray(Value value) {
    if (this->capacity < this->count + 1) {
        int oldCapacity = this->capacity;
        this->capacity = GROW_CAPACITY(oldCapacity);
        this->values = GROW_ARRAY(Value, this->values, oldCapacity, this->capacity);
    }

    this->values[this->count] = value;
    this->count++;
}

void ValueArray::init() {
    capacity = 0;
    count = 0;
    values = nullptr;
}

void ValueArray::freeValueArray() {
    FREE_ARRAY(Value, this->values, this->capacity);
    init();
}

void ValueArray::markArray() {
    for (int i = 0; i < count; i++) {
        markValue(values[i]);
    }
}