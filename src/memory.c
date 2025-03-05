#include <stdio.h>
#include <string.h>
#include "memory.h"

void write_memory(Memory *memory, uint8_t bytes[], int address, int size) {
  // TODO: check if there's enough memory to write the desired size
  memcpy(memory->bytes + address, bytes, size);
}

uint8_t read_byte_memory(Memory *memory, uint16_t address) {
  return memory->bytes[address];
}

// little endian
uint16_t read_word_memory(Memory *memory, uint16_t address) {
  return memory->bytes[address] | memory->bytes[address + 1] << 8;
}

void dump_memory(Memory *memory) {
  for (int i = 0; i < BYTES_COUNT; i++) {
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
