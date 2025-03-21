#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "memory.h"

void memory_write(Memory *memory, uint8_t bytes[], int address, int size) {
  if (address + size > MEMORY_BYTES) {
    printf("Error: address out of bounds\n");
    exit(1);
  }
  memcpy(memory->bytes + address, bytes, size);
}

void memory_peek(Memory *memory, int address, int size) {
  for (int i = address; i < address + size; i++) {
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

void memory_peek_highlight(Memory *memory, int address, int size, int highlight) {
  int to = address + size;
  for (int i = address; i < address + size; i++) {
    if (i % 16 == 0) {
      printf("%04x", i);
    }
    if (i % 8 == 0) {
      printf(" ");
    }
    printf(i == highlight ? "|" : " ");
    printf("%02x", memory->bytes[i]);
    if (i % 16 == 15) {
      if (i + 1 == highlight) {
        printf("|");
      }
      printf("\n");
    }
  }
}

void memory_dump(Memory *memory) {
  memory_peek(memory, 0, MEMORY_BYTES);
}

void memory_write_byte(Memory *memory, uint16_t address, uint8_t data) {
  memory->bytes[address] = data;
}

uint8_t memory_read_byte(Memory *memory, uint16_t address) {
  return memory->bytes[address];
}
