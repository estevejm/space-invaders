#include <stdlib.h>
#include <stdint.h>
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

void program_hardcoded(SpaceInvaders *si) {
  uint8_t program[] = {
      0x06, 0x00, // MVI B,d8
      0x0e, 0x01, // MVI C,d8
      0x26, 0xff, // MVI H,d8
      0x2e, 0xff, // MVI L,d8
      0x09,       // DAD B
      0x76        // HLT
  };
  size_t size = sizeof(program)/sizeof(program[0]);
  write_memory(&si->memory, program, 0, size);
}

SpaceInvaders *new() {
  SpaceInvaders *si = malloc(sizeof(SpaceInvaders));

  return si;
}

void peek_next_bytes(SpaceInvaders *si) {
  uint8_t first = read_byte_memory(&si->memory, si->cpu.pc);
  uint8_t second = read_byte_memory(&si->memory, si->cpu.pc+1);
  uint8_t third = read_byte_memory(&si->memory, si->cpu.pc+2);
  printf("%04x  %02x %02x %02x\n", si->cpu.pc, first, second, third);
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

// TODO: accurate cycle duration per instruction
void cycle(SpaceInvaders *si) {
  peek_next_bytes(si);
  uint8_t opcode = fetch_byte(si);
  switch (opcode) {
    case 0x00:
    case 0x08:
    case 0x10:
    case 0x18:
    case 0x20:
    case 0x28:
    case 0x30:
    case 0x38:
      printf("NOP");
      break;
    case 0x06: {
      uint8_t data = fetch_byte(si);
      printf("MVI B,%02x", data);
      set_register(&si->cpu, B, data);
      break;
    }
    case 0x09: {
      printf("DAD B");
      double_add(&si->cpu, B_PAIR);
      break;
    }
    case 0x0a: {
      printf("LDAX B");
      uint16_t address = get_register_pair(&si->cpu, B_PAIR);
      uint8_t data = read_byte_memory(&si->memory, address);
      set_register(&si->cpu, A, data);
      break;
    }
    case 0x0b: {
      printf("DCX B");
      decrement_register_pair(&si->cpu, B_PAIR);
      break;
    }
    case 0x0d: {
      printf("DCR C");
      decrement_register(&si->cpu, C);
      break;
    }
    case 0x0e: {
      uint8_t data = fetch_byte(si);
      printf("MVI C,%02x", data);
      set_register(&si->cpu, C, data);
      break;
    }
    case 0x16: {
      uint8_t data = fetch_byte(si);
      printf("MVI D,%02x", data);
      set_register(&si->cpu, D, data);
      break;
    }
    case 0x1e: {
      uint8_t data = fetch_byte(si);
      printf("MVI E,%02x", data);
      set_register(&si->cpu, E, data);
      break;
    }
    case 0x26: {
      uint8_t data = fetch_byte(si);
      printf("MVI H,%02x", data);
      set_register(&si->cpu, H, data);
      break;
    }
    case 0x2e: {
      uint8_t data = fetch_byte(si);
      printf("MVI L,%02x", data);
      set_register(&si->cpu, L, data);
      break;
    }
    case 0x27: {
      printf("DAA");
      decimal_adjust_accumulator(&si->cpu);
      break;
    }
    case 0x3c: {
      printf("INR A");
      increment_register(&si->cpu, A);
      break;
    }
    case 0x3e: {
      uint8_t data = fetch_byte(si);
      printf("MVI A,%02x", data);
      set_register(&si->cpu, A, data);
      break;
    }
    case 0x45: {
      printf("MOV B,L");
      copy_register(&si->cpu, B, L);
      break;
    }
    case 0x46: {
      printf("MOV B,M");
      uint16_t address = get_register_pair(&si->cpu, H_PAIR);
      uint8_t data = read_byte_memory(&si->memory, address);
      set_register(&si->cpu, B, data);
      break;
    }
    case 0x47: {
      printf("MOV B,A");
      copy_register(&si->cpu, B, A);
      break;
    }
    case 0x48: {
      printf("MOV C,B");
      copy_register(&si->cpu, C, B);
      break;
    }
    case 0x49: {
      printf("MOV C,C");
      break;
    }
    case 0x4e: {
      printf("MOV C,M");
      uint16_t address = get_register_pair(&si->cpu, H_PAIR);
      uint8_t data = read_byte_memory(&si->memory, address);
      set_register(&si->cpu, C, data);
      break;
    }
    case 0x4f: {
      printf("MOV C,A");
      copy_register(&si->cpu, C, A);
      break;
    }
    case 0x52: {
      printf("MOV D,D");
      break;
    }
    case 0x53: {
      printf("MOV D,E");
      copy_register(&si->cpu, D, E);
      break;
    }
    case 0x54: {
      printf("MOV D,H");
      copy_register(&si->cpu, D, H);
      break;
    }
    case 0x57: {
      printf("MOV D,A");
      copy_register(&si->cpu, D, A);
      break;
    }
    case 0x76: {
      printf("HLT");
      halt(&si->cpu);
      break;
    }
    case 0xc2: {
      uint16_t addr = fetch_word(si);
      printf("JNZ %04x", addr);
      if (!get_zero_flag(&si->cpu)) {
        si->cpu.pc = addr;
      }
      break;
    }
    case 0xc3: {
      uint16_t addr = fetch_word(si);
      printf("JMP %04x", addr);
      si->cpu.pc = addr;
      break;
    }
    case 0xc7: {
      uint8_t exp = 0;
      printf("RST %d", exp);
      restart(&si->cpu, exp);
      break;
    }
    case 0xcc: {
      uint16_t addr = fetch_word(si);
      printf("CZ %04x", addr);
      if (get_zero_flag(&si->cpu)) {
        // TODO: push current PC to stack to be used in return
        si->cpu.pc = addr;
      }
      break;
    }
    case 0xcd: {
      uint16_t addr = fetch_word(si);
      printf("CALL %04x", addr);
      // TODO: push current PC to stack to be used in return
      si->cpu.pc = addr;
      break;
    }
    case 0xea: {
      uint16_t addr = fetch_word(si);
      printf("JPE %04x", addr);
      if (get_parity_flag(&si->cpu)) {
        si->cpu.pc = addr;
      }
      break;
    }
    case 0xf4: {
      uint16_t addr = fetch_word(si);
      printf("CP %04x", addr);
      if (!get_sign_flag(&si->cpu)) {
        // TODO: push current PC to stack to be used in return
        si->cpu.pc = addr;
      }
      break;
    }
    case 0xfe: {
      uint8_t data = fetch_byte(si);
      printf("CPI %02x", data);
      compare_immediate_accumulator(&si->cpu, data);
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
//  program_test_rom(si);
  program_hardcoded(si);
//  dump_memory(&si->memory);
  while (!is_stopped(&si->cpu)) {
    printf("--------------\n");
    print_state_8080(&si->cpu);
    printf("~~~~~~~~~~~~~~\n");
    cycle(si);
    // TODO: implement clock
    usleep(1 * 1000000);
  }
}
