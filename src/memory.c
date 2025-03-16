#include <stdio.h>
#include <string.h>
#include "memory.h"

void memory_write(Memory *memory, uint8_t bytes[], int address, int size) {
  // TODO: check if there's enough memory to write the desired size
  memcpy(memory->bytes + address, bytes, size);
}

void memory_write_byte(Memory *memory, uint16_t address, uint8_t data) {
  memory->bytes[address] = data;
}

uint8_t memory_read_byte(Memory *memory, uint16_t address) {
  return memory->bytes[address];
}

// little endian
uint16_t memory_read_word(Memory *memory, uint16_t address) {
  return memory->bytes[address] | memory->bytes[address + 1] << 8;
}

// little endian
void memory_write_word(Memory *memory, uint16_t address, uint16_t data) {
  memory->bytes[address] = data & 0xff;
  memory->bytes[address+1] = data >> 8;
}

void memory_peek(Memory *memory, int from, int to) {
  for (int i = from; i < to; i++) {
    if (i % 16 == 0) {
     printf("%04x", i);
    }
    if (i % 8 == 0) {
     printf(" ");
    }
    printf(" %02x", memory->bytes[i]);
    if (i % 16 == 15) {
     printf("\n");
    }
  }
}

void memory_dump(Memory *memory) {
  memory_peek(memory, 0, MEMORY_BYTES);
}
