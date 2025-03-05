#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "string.h"
#include "space_invaders.h"
#include "i8080.h"
#include "memory.h"

#define ROM_H_ADDRESS 0x0000
#define ROM_G_ADDRESS 0x0800
#define ROM_F_ADDRESS 0x1000
#define ROM_E_ADDRESS 0x1800
#define RAM_ADDRESS 0x2000

struct spaceInvaders {
  I8080 cpu;
  Memory memory;
};

void load_rom(SpaceInvaders *si, int address, char *filename) {
  FILE *f = fopen(filename, "r");
  fseek(f, 0L, SEEK_END);
  int size = ftell(f);
  fseek(f, 0L, SEEK_SET);
  printf("loading %d bytes from file %s to address 0x%04x\n", size, filename, address);
  uint8_t buffer[size];
  fread(buffer, size, 1, f);
  fclose(f);

  write_memory(&si->memory, buffer, address, size);
}

void program_rom(SpaceInvaders *si) {
  load_rom(si, ROM_H_ADDRESS, "roms/INVADERS.H");
  load_rom(si, ROM_G_ADDRESS, "roms/INVADERS.G");
  load_rom(si, ROM_F_ADDRESS, "roms/INVADERS.F");
  load_rom(si, ROM_E_ADDRESS, "roms/INVADERS.E");
}

void program_test_rom(SpaceInvaders *si) {
  load_rom(si, 0, "roms/CPUTEST.COM");
}

SpaceInvaders *new() {
  SpaceInvaders *si = malloc(sizeof(SpaceInvaders));

  return si;
}

void peek_next_bytes(SpaceInvaders *si) {
  uint8_t first = read_byte_memory(&si->memory, si->cpu.pc);
  uint8_t second = read_byte_memory(&si->memory, si->cpu.pc+1);
  uint8_t third = read_byte_memory(&si->memory, si->cpu.pc+2);
  printf("next: %02x %02x %02x\n", first, second, third);
}

uint8_t fetch_byte(SpaceInvaders *si) {
  uint8_t byte = read_byte_memory(&si->memory, si->cpu.pc);
  si->cpu.pc++;
  return byte;
}

uint16_t fetch_word(SpaceInvaders *si) {
  uint16_t word = read_word_memory(&si->memory, si->cpu.pc);
  si->cpu.pc+=2;
  return word;
}

void cycle(SpaceInvaders *si) {
  peek_next_bytes(si);
  uint8_t opcode = fetch_byte(si);
  printf("execute: ");
  switch (opcode) {
    case 0x00:
      printf("NOP");
      break;
    case 0x3c: {
      printf("INR A");
      increment_register(&si->cpu, A);
      // TODO: set flags
      break;
    }
    case 0x3e: {
      uint8_t data = fetch_byte(si);
      printf("MVI A,%02x", data);
      set_register(&si->cpu, A, data);
      break;
    }
    case 0x4e:
      printf("MOV C,M");
      uint16_t address = get_m(&si->cpu);
      uint8_t data = read_byte_memory(&si->memory, address);
      set_register(&si->cpu, C, data);
      break;
    case 0x4f:
      printf("MOV C,A");
      copy_register(&si->cpu, C, A);
      break;
    case 0x57:
      printf("MOV D,A");
      copy_register(&si->cpu, D, A);
      break;
	case 0xc3: {
      uint16_t addr = fetch_word(si);
      printf("JMP %04x", addr);
      si->cpu.pc = addr;
      break;
    }
    case 0xcc: {
      uint16_t addr = fetch_word(si);
      printf("CZ %04x", addr);
      if (is_zero_8080(&si->cpu)) {
        // TODO: push current PC to stack to be used in return
        si->cpu.pc = addr;
      }
      break;
    }
    case 0xea: {
      uint16_t addr = fetch_word(si);
      printf("JPE %04x", addr);
      if (is_parity_even_8080(&si->cpu)) {
        // TODO: push current PC to stack to be used in return
        si->cpu.pc = addr;
      }
      break;
    }
	case 0xf4: {
      uint16_t addr = fetch_word(si);
      printf("CP %04x", addr);
      if (is_plus_8080(&si->cpu)) {
        // TODO: push current PC to stack to be used in return
      	si->cpu.pc = addr;
      }
      break;
    }
    default:
      printf("UNKNOWN OPCODE\n");
      exit(1);
  }
  printf("\n");
}

void run(SpaceInvaders *si) {
  // TODO: specify rom to load from program arg
//  program_rom(si);
  program_test_rom(si);
  dump_memory(&si->memory);
  while (1) {
    printf("----------------\n");
    print_state_8080(&si->cpu);
    printf("~~~~~~~~~~~~~~~~\n");
    cycle(si);
    // TODO: implement clock
    sleep(1);
  }
}
