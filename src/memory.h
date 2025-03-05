#include <stdlib.h>

#ifndef MEMORY_H
#define MEMORY_H

#define BYTES_COUNT 1 << 15 // needed for 8080 test rom, but space invaders only has 1 << 14

typedef struct memory {
    uint8_t bytes[BYTES_COUNT];
} Memory;

void write_memory(Memory *memory, uint8_t bytes[], int address, int size);
uint8_t read_byte_memory(Memory *memory, uint16_t address);
uint16_t read_word_memory(Memory *memory, uint16_t address);
void dump_memory(Memory *memory);

#endif //MEMORY_H
