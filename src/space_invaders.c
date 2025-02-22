#include <stdlib.h>
#include <stdio.h>
#include "space_invaders.h"
#include "i8080.h"
#include "memory.h"
#include "string.h"

#define ROM_H_ADDRESS 0x0000
#define ROM_G_ADDRESS 0x0800
#define ROM_F_ADDRESS 0x1000
#define ROM_E_ADDRESS 0x1800
#define RAM_ADDRESS 0x2000

struct spaceInvaders {
  I8080 cpu;
  Memory memory;
};

void load_rom(SpaceInvaders *si, int from_address, int to_address, char *filename) {
  int size = to_address - from_address;
  printf("going to read %d bytes from file %s\n", size, filename);
  FILE *f = fopen(filename, "r");

  uint8_t buffer[size];
  fread(buffer, size, 1, f);
  fclose(f);

  write_memory(&si->memory, buffer, from_address, size);
}

void program_rom(SpaceInvaders *si) {
  load_rom(si, ROM_H_ADDRESS, ROM_G_ADDRESS, "roms/INVADERS.H");
  load_rom(si, ROM_G_ADDRESS, ROM_F_ADDRESS, "roms/INVADERS.G");
  load_rom(si, ROM_F_ADDRESS, ROM_E_ADDRESS, "roms/INVADERS.F");
  load_rom(si, ROM_E_ADDRESS, RAM_ADDRESS, "roms/INVADERS.E");
}

SpaceInvaders *new() {
  SpaceInvaders *si = malloc(sizeof(SpaceInvaders));

  program_rom(si);

  return si;
}

void print(SpaceInvaders *si) {
  printf("CPU\n----------\n");
  print_state_8080(&si->cpu);
  printf("Memory\n----------\n");
  dump_memory(&si->memory);
}
