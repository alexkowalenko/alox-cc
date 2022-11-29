//
// ALOX-CC
//

#include "chunk.hh"
#include "memory.hh"

void Chunk::free() {
    gc.delete_array<uint8_t>(this->code, this->capacity);
}

void Chunk::write(uint8_t byte, int line) {
    if (this->capacity < this->count + 1) {
        size_t oldCapacity = this->capacity;
        this->capacity = grow_capacity(oldCapacity);
        this->code = gc.grow_array<uint8_t>(this->code, oldCapacity, this->capacity);
    }

    this->code[this->count] = byte;
    this->lines.push_back(line);
    this->count++;
}

const_index_t Chunk::add_constant(Value value) {
    return this->constants.write(value);
}
