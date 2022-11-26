//
// ALOX-CC
//

#include "chunk.hh"
#include "memory.hh"
#include "vm.hh"

void Chunk::initChunk() {
    this->count = 0;
    this->capacity = 0;
    this->code = nullptr;
    this->lines = nullptr;
    this->constants.init();
}

void Chunk::freeChunk() {
    FREE_ARRAY(uint8_t, this->code, this->capacity);
    FREE_ARRAY(int, this->lines, this->capacity);
    this->constants.freeValueArray();
    initChunk();
}

void Chunk::writeChunk(uint8_t byte, int line) {
    if (this->capacity < this->count + 1) {
        int oldCapacity = this->capacity;
        this->capacity = GROW_CAPACITY(oldCapacity);
        this->code = GROW_ARRAY(uint8_t, this->code, oldCapacity, this->capacity);
        this->lines = GROW_ARRAY(int, this->lines, oldCapacity, this->capacity);
    }

    this->code[this->count] = byte;
    this->lines[this->count] = line;
    this->count++;
}

int Chunk::addConstant(Value value) {
    push(value);
    this->constants.writeValueArray(value);
    pop();
    return this->constants.get_count() - 1;
}
