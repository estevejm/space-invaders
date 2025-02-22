#include <stdlib.h>

#ifndef MEMORY_H
#define MEMORY_H

#define BYTES_COUNT 1 << 14

typedef struct memory {
    uint8_t bytes[BYTES_COUNT];
} Memory;

void write_memory(Memory *memory, uint8_t bytes[], int address, int size);
void dump_memory(Memory *memory);

#endif //MEMORY_H
