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
#define VRAM_ADDRESS 0x2400

struct spaceInvaders {
  I8080 cpu;
  Memory memory;
  // TODO: extract buses
  bool write;
  uint8_t data;
  uint16_t address;
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

  memory_write(&si->memory, buffer, address, size);
}

void program_rom(SpaceInvaders *si) {
  load_rom(si, ROM_H_ADDRESS, "roms/INVADERS.H");
  load_rom(si, ROM_G_ADDRESS, "roms/INVADERS.G");
  load_rom(si, ROM_F_ADDRESS, "roms/INVADERS.F");
  load_rom(si, ROM_E_ADDRESS, "roms/INVADERS.E");
  memory_peek(&si->memory, 0, 0x2000);
}

void program_test_rom(SpaceInvaders *si) {
  load_rom(si, 0, "roms/8080PRE.COM");
  memory_peek(&si->memory, 0, 0x200);
  si->cpu.sp = MEMORY_BYTES;
}

void program_hardcoded(SpaceInvaders *si) {
  uint8_t program[] = {
      0x06, 0x0b,
      0x0e, 0x33,
      0xc5,
      0xe1,
      0x76,
  };
  size_t size = sizeof(program)/sizeof(program[0]);
  memory_write(&si->memory, program, 0, size);
  memory_peek(&si->memory, 0, 0x20);
  si->cpu.sp = MEMORY_BYTES;
}

SpaceInvaders *new() {
  SpaceInvaders *si = malloc(sizeof(SpaceInvaders));

  return si;
}

void peek_next_bytes(SpaceInvaders *si) {
  uint8_t first = memory_read_byte(&si->memory, si->cpu.pc);
  uint8_t second = memory_read_byte(&si->memory, si->cpu.pc+1);
  uint8_t third = memory_read_byte(&si->memory, si->cpu.pc+2);
  printf("%02x %02x %02x\n", first, second, third);
}

uint8_t read_byte(SpaceInvaders *si, uint16_t address) {
  si->write = false;
  si->address = address;
  si->data = memory_read_byte(&si->memory, si->address);
  return si->data;
}

uint16_t read_word(SpaceInvaders *si, uint16_t address) {
  uint8_t lsb = read_byte(si, address);
  uint8_t msb = read_byte(si, address+1);

  // little endian
  return (msb << 8) | lsb;
}

void write_byte(SpaceInvaders *si, uint16_t address, uint8_t data) {
  si->write = true;
  si->address = address;
  si->data = data;
  memory_write_byte(&si->memory, si->address, si->data);
}

void write_word(SpaceInvaders *si, uint16_t address, uint16_t data) {
  // little endian
  uint8_t lsb = data & 0xff;
  uint8_t msb = data >> 8;

  write_byte(si, address, lsb);
  write_byte(si, address+1, msb);
}

uint8_t fetch_byte(SpaceInvaders *si) {
  uint8_t data = read_byte(si, si->cpu.pc);
  si->cpu.pc++;
  return data;
}

uint16_t fetch_word(SpaceInvaders *si) {
  uint16_t data = read_word(si, si->cpu.pc);
  si->cpu.pc+=2;
  return data;
}

uint8_t register_pair_read_byte(SpaceInvaders *si, enum RegisterPair r) {
  uint16_t address = get_register_pair(&si->cpu, r);
  return read_byte(si, address);
}

void register_pair_write_byte(SpaceInvaders *si, enum RegisterPair r, uint8_t data) {
  uint16_t address = get_register_pair(&si->cpu, r);
  write_byte(si, address, data);
}

void stack_push_word(SpaceInvaders *si, uint16_t data) {
  si->cpu.sp-=2;
  write_word(si, si->cpu.sp, data);
}

uint16_t stack_pop_word(SpaceInvaders *si) {
  uint16_t data = read_word(si, si->cpu.sp);
  si->cpu.sp+=2;
  return data;
}

// TODO: accurate cycle duration per instruction
void cycle(SpaceInvaders *si) {
  peek_next_bytes(si);
  uint8_t opcode = fetch_byte(si);
  switch (opcode) {
    case 0x00:
      no_operation(&si->cpu);
      break;
    case 0x01: {
      uint16_t data = fetch_word(si);
      printf("LXI B,%04x", data);
      set_register_pair(&si->cpu, B_PAIR, data);
      break;
    }
    case 0x03: {
      printf("INX B");
      increment_register_pair(&si->cpu, B_PAIR);
      break;
    }
    case 0x04: {
      increment_register(&si->cpu, B);
      break;
    }
    case 0x05: {
      decrement_register(&si->cpu, B);
      break;
    }
    case 0x06: {
      uint8_t data = fetch_byte(si);
      printf("MVI B,%02x", data);
      set_register(&si->cpu, B, data);
      break;
    }
    case 0x08:
      no_operation(&si->cpu);
      break;
    case 0x09: {
      printf("DAD B");
      double_add(&si->cpu, B_PAIR);
      break;
    }
    case 0x0a: {
      printf("LDAX B");
      uint8_t data = register_pair_read_byte(si, B_PAIR);
      set_register(&si->cpu, A, data);
      break;
    }
    case 0x0b: {
      printf("DCX B");
      decrement_register_pair(&si->cpu, B_PAIR);
      break;
    }
    case 0x0c: {
      increment_register(&si->cpu, C);
      break;
    }
    case 0x0d: {
      decrement_register(&si->cpu, C);
      break;
    }
    case 0x0e: {
      uint8_t data = fetch_byte(si);
      printf("MVI C,%02x", data);
      set_register(&si->cpu, C, data);
      break;
    }
    case 0x10:
      no_operation(&si->cpu);
      break;
    case 0x11: {
      uint16_t data = fetch_word(si);
      printf("LXI D,%04x", data);
      set_register_pair(&si->cpu, D_PAIR, data);
      break;
    }
    case 0x13: {
      printf("INX D");
      increment_register_pair(&si->cpu, D_PAIR);
      break;
    }
    case 0x14: {
      increment_register(&si->cpu, D);
      break;
    }
    case 0x15: {
      decrement_register(&si->cpu, D);
      break;
    }
    case 0x16: {
      uint8_t data = fetch_byte(si);
      printf("MVI D,%02x", data);
      set_register(&si->cpu, D, data);
      break;
    }
    case 0x18:
      no_operation(&si->cpu);
      break;
    case 0x1a: {
      printf("LDAX D");
      uint8_t data = register_pair_read_byte(si, D_PAIR);
      set_register(&si->cpu, A, data);
      break;
    }
    case 0x1b: {
      printf("DCX D");
      decrement_register_pair(&si->cpu, D_PAIR);
      break;
    }
    case 0x1c: {
      increment_register(&si->cpu, E);
      break;
    }
    case 0x1d: {
      decrement_register(&si->cpu, E);
      break;
    }
    case 0x1e: {
      uint8_t data = fetch_byte(si);
      printf("MVI E,%02x", data);
      set_register(&si->cpu, E, data);
      break;
    }
    case 0x20:
      no_operation(&si->cpu);
      break;
    case 0x21: {
      uint16_t data = fetch_word(si);
      printf("LXI H,%04x", data);
      set_register_pair(&si->cpu, H_PAIR, data);
      break;
    }
    case 0x23: {
      printf("INX H");
      increment_register_pair(&si->cpu, H_PAIR);
      break;
    }
    case 0x24: {
      increment_register(&si->cpu, H);
      break;
    }
    case 0x25: {
      decrement_register(&si->cpu, H);
      break;
    }
    case 0x26: {
      uint8_t data = fetch_byte(si);
      printf("MVI H,%02x", data);
      set_register(&si->cpu, H, data);
      break;
    }
    case 0x27: {
      decimal_adjust_accumulator(&si->cpu);
      break;
    }
    case 0x28:
      no_operation(&si->cpu);
      break;
    case 0x2b: {
      printf("DCX H");
      decrement_register_pair(&si->cpu, H_PAIR);
      break;
    }
    case 0x2c: {
      increment_register(&si->cpu, L);
      break;
    }
    case 0x2d: {
      decrement_register(&si->cpu, L);
      break;
    }
    case 0x2e: {
      uint8_t data = fetch_byte(si);
      printf("MVI L,%02x", data);
      set_register(&si->cpu, L, data);
      break;
    }
    case 0x30:
      no_operation(&si->cpu);
      break;
    case 0x31: {
      uint16_t data = fetch_word(si);
      printf("LXI SP,%04x", data);
      set_register_pair(&si->cpu, SP, data);
      break;
    }
    case 0x33: {
      printf("INX SP");
      increment_register_pair(&si->cpu, SP);
      break;
    }
    case 0x38:
      no_operation(&si->cpu);
      break;
    case 0x3a: {
      uint16_t address = fetch_word(si);
      printf("LDA %04x", address);
      uint8_t data = read_byte(si, address);
      set_register(&si->cpu, A, data);
      break;
    }
    case 0x3b: {
      printf("DCX SP");
      decrement_register_pair(&si->cpu, SP);
      break;
    }
    case 0x3c: {
      increment_register(&si->cpu, A);
      break;
    }
    case 0x3d: {
      decrement_register(&si->cpu, A);
      break;
    }
    case 0x3e: {
      uint8_t data = fetch_byte(si);
      printf("MVI A,%02x", data);
      set_register(&si->cpu, A, data);
      break;
    }
    case 0x45: {
      copy_register(&si->cpu, B, L);
      break;
    }
    case 0x46: {
      printf("MOV B,M");
      uint8_t data = register_pair_read_byte(si, H_PAIR);
      set_register(&si->cpu, B, data);
      break;
    }
    case 0x47: {
      copy_register(&si->cpu, B, A);
      break;
    }
    case 0x48: {
      copy_register(&si->cpu, C, B);
      break;
    }
    case 0x49: {
      copy_register(&si->cpu, C, C);
      break;
    }
    case 0x4e: {
      printf("MOV C,M");
      uint8_t data = register_pair_read_byte(si, H_PAIR);
      set_register(&si->cpu, C, data);
      break;
    }
    case 0x4f: {
      copy_register(&si->cpu, C, A);
      break;
    }
    case 0x52: {
      copy_register(&si->cpu, D, D);
      break;
    }
    case 0x53: {
      copy_register(&si->cpu, D, E);
      break;
    }
    case 0x54: {
      copy_register(&si->cpu, D, H);
      break;
    }
    case 0x57: {
      copy_register(&si->cpu, D, A);
      break;
    }
    case 0x76: {
      halt(&si->cpu);
      break;
    }
    case 0x77: {
      printf("MOV M,A");
      uint8_t data = get_register(&si->cpu, A);
      register_pair_write_byte(si, H_PAIR, data);
      break;
    }
    case 0xc1: {
      printf("POP B");
      uint16_t data = stack_pop_word(si);
      set_register_pair(&si->cpu, B_PAIR, data);
      break;
    }
    case 0xc2: {
      uint16_t address = fetch_word(si);
      printf("JNZ %04x", address);
      jump_if_not_zero(&si->cpu, address);
      break;
    }
    case 0xc3: {
      uint16_t address = fetch_word(si);
      printf("JMP %04x", address);
      jump(&si->cpu, address);
      break;
    }
    case 0xc5: {
      printf("PUSH B");
      uint16_t data = get_register_pair(&si->cpu, B_PAIR);
      stack_push_word(si, data);
      break;
    }
    case 0xc7: {
      uint8_t exp = 0;
      printf("RST %d", exp);
      restart(&si->cpu, exp);
      break;
    }
    case 0xca: {
      uint16_t address = fetch_word(si);
      printf("JZ %04x", address);
      jump_if_zero(&si->cpu, address);
      break;
    }
    case 0xcc: {
      uint16_t address = fetch_word(si);
      printf("CZ %04x", address);
      subroutine_call_if_zero(&si->cpu, address);
      break;
    }
    case 0xcd: {
      uint16_t address = fetch_word(si);
      printf("CALL %04x", address);
      subroutine_call(&si->cpu, address);
      break;
    }
    case 0xd1: {
      printf("POP D");
      uint16_t data = stack_pop_word(si);
      set_register_pair(&si->cpu, D_PAIR, data);
      break;
    }
    case 0xd2: {
      uint16_t address = fetch_word(si);
      printf("JNC %04x", address);
      jump_if_no_carry(&si->cpu, address);
      break;
    }
    case 0xd5: {
      printf("PUSH D");
      uint16_t data = get_register_pair(&si->cpu, D_PAIR);
      stack_push_word(si, data);
      break;
    }
    case 0xe1: {
      printf("POP H");
      uint16_t data = stack_pop_word(si);
      set_register_pair(&si->cpu, H_PAIR, data);
      break;
    }
    case 0xe5: {
      printf("PUSH H");
      uint16_t data = get_register_pair(&si->cpu, H_PAIR);
      stack_push_word(si, data);
      break;
    }
    case 0xea: {
      uint16_t address = fetch_word(si);
      printf("JPE %04x", address);
      jump_if_parity_even(&si->cpu, address);
      break;
    }
    case 0xf1: {
      printf("POP PSW");
      uint16_t data = stack_pop_word(si);
      set_register_pair(&si->cpu, PSW, data);
      break;
    }
    case 0xf4: {
      uint16_t address = fetch_word(si);
      printf("CP %04x", address);
      subroutine_call_if_plus(&si->cpu, address);
      break;
    }
    case 0xf5: {
      printf("PUSH PSW");
      uint16_t data = get_register_pair(&si->cpu, PSW);
      stack_push_word(si, data);
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

void print_bus(SpaceInvaders *si) {
  printf(si->write ? "w" : "r");
  printf(" %04x %02x\n", si->address, si->data);
}

void print_stack(SpaceInvaders *si) {
  // TODO: print from SP and print always entire "lines" (16 bytes)
  int size = 0x10;
  int address = VRAM_ADDRESS - size; // last section of RAM
  memory_peek(&si->memory, address, size);
}

void run(SpaceInvaders *si) {
  // TODO: specify rom to load from program arg
  program_rom(si);
//  program_test_rom(si);
//  program_hardcoded(si);
  while (!is_halted(&si->cpu)) {
    printf("--------------\n");
    print_state_8080(&si->cpu);
    printf("``````````````\n");
    print_bus(si);
//    print_stack(si);
    printf("~~~~~~~~~~~~~~\n");
    cycle(si);
    // TODO: implement clock
    usleep(0.1 * 1000000);
  }
}
