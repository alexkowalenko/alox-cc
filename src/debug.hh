//
// ALOX-CC
//

#pragma once

#include "chunk.hh"

void disassembleChunk(Chunk *chunk, const char *name);
int  disassembleInstruction(Chunk *chunk, int offset);
