//
// ALOX-CC
//

#include "chunk.hh"
#include "memory.hh"
#include "vm.hh"

void Chunk::init() {
    this->count = 0;
    this->capacity = 0;
    this->code = nullptr;
    this->lines = nullptr;
    this->constants.init();
}

void Chunk::free() {
    free_array<uint8_t>(this->code, this->capacity);
    free_array<int>(this->lines, this->capacity);
    this->constants.free();
    init();
}

void Chunk::write(uint8_t byte, int line) {
    if (this->capacity < this->count + 1) {
        size_t oldCapacity = this->capacity;
        this->capacity = grow_capacity(oldCapacity);
        this->code = grow_array<uint8_t>(this->code, oldCapacity, this->capacity);
        this->lines = grow_array<int>(this->lines, oldCapacity, this->capacity);
    }

    this->code[this->count] = byte;
    this->lines[this->count] = line;
    this->count++;
}

int Chunk::addConstant(Value value) {
    push(value);
    this->constants.write(value);
    pop();
    return this->constants.get_count() - 1;
}
