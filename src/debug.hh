//
// ALOX-CC
//

#pragma once

#include "chunk.hh"
#include <string_view>

namespace alox {

void disassembleChunk(Chunk *chunk, const std::string_view &name);
int  disassembleInstruction(Chunk *chunk, int offset);

} // namespace lox
