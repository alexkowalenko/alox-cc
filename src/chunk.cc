//
// ALOX-CC
//

#include "chunk.hh"
#include "memory.hh"

void Chunk::init() {
    this->count = 0;
    this->capacity = 0;
    this->code = nullptr;
    this->constants.init();

    lines = new std::vector<size_t>;
}

void Chunk::free() {
    free_array<uint8_t>(this->code, this->capacity);
    this->constants.free();
    delete lines;

    init();
}

void Chunk::write(uint8_t byte, int line) {
    if (this->capacity < this->count + 1) {
        size_t oldCapacity = this->capacity;
        this->capacity = grow_capacity(oldCapacity);
        this->code = grow_array<uint8_t>(this->code, oldCapacity, this->capacity);
    }

    this->code[this->count] = byte;
    this->lines->push_back(line);
    this->count++;
}

int Chunk::addConstant(Value value) {
    this->constants.write(value);
    return this->constants.get_count() - 1;
}
