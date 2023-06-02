//
// ALOX-CC
//

#include "chunk.hh"
#include "memory.hh"

namespace alox {

void Chunk::free() {
    delete[] code;
}

void Chunk::write(uint8_t byte, size_t line) {
    if (capacity < count + 1) {
        const size_t oldCapacity = this->capacity;
        capacity = grow_capacity(oldCapacity);
        code = grow_array<uint8_t>(this->code, oldCapacity, this->capacity);
    }

    code[count] = byte;
    lines.push_back(line);
    count++;
}

const_index_t Chunk::add_constant(Value value) {
    return this->constants.write(value);
}

} // namespace alox
