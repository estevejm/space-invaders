#include <stdint.h>

#ifndef MEMORY_H
#define MEMORY_H

// TODO: split in ROM, RAM, VRAM, stack, ...
#define MEMORY_BYTES (1 << 14)

typedef struct memory {
  uint8_t bytes[MEMORY_BYTES];
} Memory;

void memory_write(Memory *memory, uint8_t bytes[], int address, int size);
void memory_write_byte(Memory *memory, uint16_t address, uint8_t data);
uint8_t memory_read_byte(Memory *memory, uint16_t address);
void memory_write_word(Memory *memory, uint16_t address, uint16_t data);
uint16_t memory_read_word(Memory *memory, uint16_t address);
void memory_dump(Memory *memory);
void memory_peek(Memory *memory, int from, int to);

#endif //MEMORY_H
