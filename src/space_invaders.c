#include "space_invaders.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "raylib.h"

#define ROM_H_ADDRESS 0x0000
#define ROM_G_ADDRESS 0x0800
#define ROM_F_ADDRESS 0x1000
#define ROM_E_ADDRESS 0x1800
#define RAM_ADDRESS 0x2000
#define VRAM_ADDRESS 0x2400
#define VRAM_SIZE 0x1C00

const int window_width = 256;
const int window_height = 224;
const int scale = 3;
const int offset = 3;
const float rotation = -90.0f;
const Color color1 = BLACK;
const Color color2 = GREEN;

typedef struct spaceInvaders SpaceInvaders;
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
  memory_dump(&si->memory);
}

void program_test_rom(SpaceInvaders *si) {
  load_rom(si, 0, "roms/8080EXER.COM");
  memory_peek(&si->memory, 0, 0x2000);
}

void program_hardcoded(SpaceInvaders *si) {
  uint8_t program[] = {
      0x26, 0x0d,
      0x2e, 0xf0,
      0xe5,
      0x26, 0x0b,
      0x2e, 0x3c,
      0xe3,
      0x76,
  };
  size_t size = sizeof(program)/sizeof(program[0]);
  memory_write(&si->memory, program, 0, size);
  memory_peek(&si->memory, 0, 0x20);
}

SpaceInvaders *new() {
  SpaceInvaders *si = malloc(sizeof(SpaceInvaders));
  si->cpu.sp = MEMORY_BYTES & 0xffff;
  return si;
}

void peek_next_bytes(SpaceInvaders *si) {
  uint8_t first = memory_read_byte(&si->memory, si->cpu.pc);
  uint8_t second = memory_read_byte(&si->memory, si->cpu.pc+1);
  uint8_t third = memory_read_byte(&si->memory, si->cpu.pc+2);
  printf("%02x %02x %02x\n", first, second, third);
}

void print_instruction(SpaceInvaders *si, char *format, ...) {
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
  printf("\n····················\n");
}

void print_bus(SpaceInvaders *si) {
  printf("~ %c %04x %02x\n", si->write ? 'w' : 'r', si->address, si->data);
}

void print_stack(SpaceInvaders *si) {
  int stackPointerMemoryLineStart = si->cpu.sp & 0xfff0;
  if (si->cpu.sp % 0x10 == 0) {
    stackPointerMemoryLineStart -= 0x10;
    stackPointerMemoryLineStart = stackPointerMemoryLineStart & 0xffff;
  }
  memory_peek_highlight(&si->memory, stackPointerMemoryLineStart, 0x10, si->cpu.sp);
}

uint8_t read_byte(SpaceInvaders *si, uint16_t address) {
  si->write = false;
  si->address = address;
  si->data = memory_read_byte(&si->memory, si->address);
  print_bus(si);
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
  print_bus(si);
}

void write_word(SpaceInvaders *si, uint16_t address, uint16_t data) {
  // little endian
  uint8_t msb = data >> 8;
  uint8_t lsb = data & 0xff;

  write_byte(si, address+1, msb);
  write_byte(si, address, lsb);
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

void subroutine_call(SpaceInvaders *si, uint16_t address) {
  stack_push_word(si, si->cpu.pc);
  jump(&si->cpu, address);
}

void subroutine_call_if_carry(SpaceInvaders *si, uint16_t address) {
  if (get_carry_flag(&si->cpu)) {
    subroutine_call(si, address);
  }
}

void subroutine_call_if_no_carry(SpaceInvaders *si, uint16_t address) {
  if (!get_carry_flag(&si->cpu)) {
    subroutine_call(si, address);
  }
}

void subroutine_call_if_zero(SpaceInvaders *si, uint16_t address) {
  if (get_zero_flag(&si->cpu)) {
    subroutine_call(si, address);
  }
}

void subroutine_call_if_not_zero(SpaceInvaders *si, uint16_t address) {
  if (!get_zero_flag(&si->cpu)) {
    subroutine_call(si, address);
  }
}

void subroutine_call_if_minus(SpaceInvaders *si, uint16_t address) {
  if (get_sign_flag(&si->cpu)) {
    subroutine_call(si, address);
  }
}

void subroutine_call_if_plus(SpaceInvaders *si, uint16_t address) {
  if (!get_sign_flag(&si->cpu)) {
    subroutine_call(si, address);
  }
}

void subroutine_call_if_parity_even(SpaceInvaders *si, uint16_t address) {
  if (get_parity_flag(&si->cpu)) {
    subroutine_call(si, address);
  }
}

void subroutine_call_if_parity_odd(SpaceInvaders *si, uint16_t address) {
  if (!get_parity_flag(&si->cpu)) {
    subroutine_call(si, address);
  }
}

void subroutine_return(SpaceInvaders *si) {
  si->cpu.pc = stack_pop_word(si);
}

void subroutine_return_if_carry(SpaceInvaders *si) {
  if (get_carry_flag(&si->cpu)) {
    subroutine_return(si);
  }
}

void subroutine_return_if_no_carry(SpaceInvaders *si) {
  if (!get_carry_flag(&si->cpu)) {
    subroutine_return(si);
  }
}

void subroutine_return_if_zero(SpaceInvaders *si) {
  if (get_zero_flag(&si->cpu)) {
    subroutine_return(si);
  }
}

void subroutine_return_if_not_zero(SpaceInvaders *si) {
  if (!get_zero_flag(&si->cpu)) {
    subroutine_return(si);
  }
}

void subroutine_return_if_minus(SpaceInvaders *si) {
  if (get_sign_flag(&si->cpu)) {
    subroutine_return(si);
  }
}

void subroutine_return_if_plus(SpaceInvaders *si) {
  if (!get_sign_flag(&si->cpu)) {
    subroutine_return(si);
  }
}

void subroutine_return_if_parity_even(SpaceInvaders *si) {
  if (get_carry_flag(&si->cpu)) {
    subroutine_return(si);
  }
}

void subroutine_return_if_parity_odd(SpaceInvaders *si) {
  if (!get_carry_flag(&si->cpu)) {
    subroutine_return(si);
  }
}

void no_operation(SpaceInvaders *si) {
  print_instruction(si, "NOP");
}

void register_increment(SpaceInvaders *si, enum Register r) {
  print_instruction(si, "INR %c", register_names[r]);
  increment_register(&si->cpu, r);
}

void register_decrement(SpaceInvaders *si, enum Register r) {
  print_instruction(si, "DCR %c", register_names[r]);
  decrement_register(&si->cpu, r);
}

void register_pair_increment(SpaceInvaders *si, enum RegisterPair r) {
  print_instruction(si, "INX %s", register_pair_names[r]);
  increment_register_pair(&si->cpu, r);
}

void register_pair_decrement(SpaceInvaders *si, enum RegisterPair r) {
  print_instruction(si, "DCX %s", register_pair_names[r]);
  decrement_register_pair(&si->cpu, r);
}

void register_move(SpaceInvaders *si, enum Register dst, enum Register src) {
  print_instruction(si, "MOV %c,%c", register_names[dst], register_names[src]);
  copy_register(&si->cpu, dst, src);
}

void restart(SpaceInvaders *si, uint8_t exp) {
  // TODO: check interrupt enabled?
  uint8_t bounded = exp % 8;
  print_instruction(si, "RST %d", bounded);
  stack_push_word(si, si->cpu.pc);
  si->cpu.pc = bounded << 3;
}

void register_add(SpaceInvaders *si, enum Register r) {
  print_instruction(si, "ADD %c", register_names[r]);
  add_register_accumulator(&si->cpu, r);
}

void register_add_with_carry(SpaceInvaders *si, enum Register r) {
  print_instruction(si, "ADC %c", register_names[r]);
  add_register_accumulator_with_carry(&si->cpu, r);
}

void register_subtract(SpaceInvaders *si, enum Register r) {
  print_instruction(si, "SUB %c", register_names[r]);
  subtract_register_accumulator(&si->cpu, r);
}

void register_subtract_with_borrow(SpaceInvaders *si, enum Register r) {
  print_instruction(si, "SBB %c", register_names[r]);
  subtract_register_accumulator_with_borrow(&si->cpu, r);
}

void register_and(SpaceInvaders *si, enum Register r) {
  print_instruction(si, "ANA %c", register_names[r]);
  and_register_accumulator(&si->cpu, r);
}

void register_or(SpaceInvaders *si, enum Register r) {
  print_instruction(si, "ORA %c", register_names[r]);
  or_register_accumulator(&si->cpu, r);
}

void register_exclusive_or(SpaceInvaders *si, enum Register r) {
  print_instruction(si, "XRA %c", register_names[r]);
  exclusive_or_register_accumulator(&si->cpu, r);
}

void register_compare(SpaceInvaders *si, enum Register r) {
  print_instruction(si, "CMP %c", register_names[r]);
  compare_register_accumulator(&si->cpu, r);
}

// TODO: accurate cycle duration per instruction
void cycle(SpaceInvaders *si) {
  uint8_t opcode = fetch_byte(si);
  switch (opcode) {
    case 0x00:
      no_operation(si);
      break;
    case 0x01: {
      uint16_t data = fetch_word(si);
      print_instruction(si, "LXI B,%04x", data);
      set_register_pair(&si->cpu, B_PAIR, data);
      break;
    }
    case 0x02: {
      print_instruction(si, "STAX B");
      uint8_t data = get_register(&si->cpu, A);
      register_pair_write_byte(si, B_PAIR, data);
      break;
    }
    case 0x03:
      register_pair_increment(si, B_PAIR);
      break;
    case 0x04:
      register_increment(si, B);
      break;
    case 0x05:
      register_decrement(si, B);
      break;
    case 0x06: {
      uint8_t data = fetch_byte(si);
      print_instruction(si, "MVI B,%02x", data);
      set_register(&si->cpu, B, data);
      break;
    }
    case 0x07: {
      print_instruction(si, "RLC");
      rotate_accumulator_left(&si->cpu);
      break;
    }
    case 0x08:
      no_operation(si);
      break;
    case 0x09: {
      print_instruction(si, "DAD B");
      double_add(&si->cpu, B_PAIR);
      break;
    }
    case 0x0a: {
      print_instruction(si, "LDAX B");
      uint8_t data = register_pair_read_byte(si, B_PAIR);
      set_register(&si->cpu, A, data);
      break;
    }
    case 0x0b:
      register_pair_decrement(si, B_PAIR);
      break;
    case 0x0c:
      register_increment(si, C);
      break;
    case 0x0d:
      decrement_register(&si->cpu, C);
      break;
    case 0x0e: {
      uint8_t data = fetch_byte(si);
      print_instruction(si, "MVI C,%02x", data);
      set_register(&si->cpu, C, data);
      break;
    }
    case 0x0f: {
      print_instruction(si, "RRC");
      rotate_accumulator_right(&si->cpu);
      break;
    }
    case 0x10:
      no_operation(si);
      break;
    case 0x11: {
      uint16_t data = fetch_word(si);
      print_instruction(si, "LXI D,%04x", data);
      set_register_pair(&si->cpu, D_PAIR, data);
      break;
    }
    case 0x12: {
      print_instruction(si, "STAX D");
      uint8_t data = get_register(&si->cpu, A);
      register_pair_write_byte(si, D_PAIR, data);
      break;
    }
    case 0x13:
      register_pair_increment(si, D_PAIR);
      break;
    case 0x14:
      register_increment(si, D);
      break;
    case 0x15:
      decrement_register(&si->cpu, D);
      break;
    case 0x16: {
      uint8_t data = fetch_byte(si);
      print_instruction(si, "MVI D,%02x", data);
      set_register(&si->cpu, D, data);
      break;
    }
    case 0x17: {
      print_instruction(si, "RAL");
      rotate_accumulator_left_through_carry(&si->cpu);
      break;
    }
    case 0x18:
      no_operation(si);
      break;
    case 0x19: {
      print_instruction(si, "DAD D");
      double_add(&si->cpu, D_PAIR);
      break;
    }
    case 0x1a: {
      print_instruction(si, "LDAX D");
      uint8_t data = register_pair_read_byte(si, D_PAIR);
      set_register(&si->cpu, A, data);
      break;
    }
    case 0x1b:
      register_pair_decrement(si, D_PAIR);
      break;
    case 0x1c:
      register_increment(si, E);
      break;
    case 0x1d:
      decrement_register(&si->cpu, E);
      break;
    case 0x1e: {
      uint8_t data = fetch_byte(si);
      print_instruction(si, "MVI E,%02x", data);
      set_register(&si->cpu, E, data);
      break;
    }
    case 0x1f: {
      print_instruction(si, "RAR");
      rotate_accumulator_right_through_carry(&si->cpu);
      break;
    }
    case 0x20:
      no_operation(si);
      break;
    case 0x21: {
      uint16_t data = fetch_word(si);
      print_instruction(si, "LXI H,%04x", data);
      set_register_pair(&si->cpu, H_PAIR, data);
      break;
    }
    case 0x22: {
      uint16_t address = fetch_word(si);
      print_instruction(si, "SHLD %04x", address);
      uint16_t data = get_register_pair(&si->cpu, H_PAIR);
      write_word(si, address, data);
      break;
    }
    case 0x23:
      register_pair_increment(si, H_PAIR);
      break;
    case 0x24:
      register_increment(si, H);
      break;
    case 0x25:
      register_decrement(si, H);
      break;
    case 0x26: {
      uint8_t data = fetch_byte(si);
      print_instruction(si, "MVI H,%02x", data);
      set_register(&si->cpu, H, data);
      break;
    }
    case 0x27: {
      print_instruction(si, "DAA");
      decimal_adjust_accumulator(&si->cpu);
      break;
    }
    case 0x28:
      no_operation(si);
      break;
    case 0x29: {
      print_instruction(si, "DAD H");
      double_add(&si->cpu, H_PAIR);
      break;
    }
    case 0x2a: {
      uint16_t address = fetch_word(si);
      print_instruction(si, "LHLD %04x", address);
      uint16_t data = read_word(si, address);
      set_register_pair(&si->cpu, H_PAIR, data);
      break;
    }
    case 0x2b:
      register_pair_decrement(si, H_PAIR);
      break;
    case 0x2c:
      increment_register(&si->cpu, L);
      break;
    case 0x2d:
      register_decrement(si, L);
      break;
    case 0x2e: {
      uint8_t data = fetch_byte(si);
      print_instruction(si, "MVI L,%02x", data);
      set_register(&si->cpu, L, data);
      break;
    }
    case 0x2f: {
      print_instruction(si, "CMA");
      complement_accumulator(&si->cpu);
      break;
    }
    case 0x30:
      no_operation(si);
      break;
    case 0x31: {
      uint16_t data = fetch_word(si);
      print_instruction(si, "LXI SP,%04x", data);
      set_register_pair(&si->cpu, SP, data);
      break;
    }
    case 0x32: {
      uint16_t address = fetch_word(si);
      print_instruction(si, "STA %04x", address);
      uint8_t data = get_register(&si->cpu, A);
      write_byte(si, address, data);
      break;
    }
    case 0x33:
      register_pair_increment(si, SP);
      break;
    case 0x34: {
      print_instruction(si, "INR M");
      uint8_t data = register_pair_read_byte(si, H_PAIR);
      register_pair_write_byte(si, H_PAIR, data + 1);
      break;
    }
    case 0x35: {
      print_instruction(si, "DCR M");
      uint8_t data = register_pair_read_byte(si, H_PAIR);
      register_pair_write_byte(si, H_PAIR, data - 1);
      break;
    }
    case 0x36: {
      uint8_t data = fetch_byte(si);
      print_instruction(si, "MVI M,%02x", data);
      register_pair_write_byte(si, H_PAIR, data);
      break;
    }
    case 0x37: {
      print_instruction(si, "STC");
      set_carry(&si->cpu);
      break;
    }
    case 0x38:
      no_operation(si);
      break;
    case 0x39: {
      print_instruction(si, "DAD SP");
      double_add(&si->cpu, SP);
      break;
    }
    case 0x3a: {
      uint16_t address = fetch_word(si);
      print_instruction(si, "LDA %04x", address);
      uint8_t data = read_byte(si, address);
      set_register(&si->cpu, A, data);
      break;
    }
    case 0x3b:
      register_pair_decrement(si, SP);
      break;
    case 0x3c:
      register_increment(si, A);
      break;
    case 0x3d:
      register_decrement(si, A);
      break;
    case 0x3e: {
      uint8_t data = fetch_byte(si);
      print_instruction(si, "MVI A,%02x", data);
      set_register(&si->cpu, A, data);
      break;
    }
    case 0x3f: {
      print_instruction(si, "CMC");
      complement_carry(&si->cpu);
      break;
    }
    case 0x40: {
      register_move(si, B, B);
      break;
    }
    case 0x41: {
      register_move(si, B, C);
      break;
    }
    case 0x42: {
      register_move(si, B, D);
      break;
    }
    case 0x43: {
      register_move(si, B, E);
      break;
    }
    case 0x44: {
      register_move(si, B, H);
      break;
    }
    case 0x45: {
      register_move(si, B, L);
      break;
    }
    case 0x46: {
      print_instruction(si, "MOV B,M");
      uint8_t data = register_pair_read_byte(si, H_PAIR);
      set_register(&si->cpu, B, data);
      break;
    }
    case 0x47: {
      register_move(si, B, A);
      break;
    }
    case 0x48: {
      register_move(si, C, B);
      break;
    }
    case 0x49: {
      register_move(si, C, C);
      break;
    }
    case 0x4a: {
      register_move(si, C, D);
      break;
    }
    case 0x4b: {
      register_move(si, C, E);
      break;
    }
    case 0x4c: {
      register_move(si, C, H);
      break;
    }
    case 0x4d: {
      register_move(si, C, L);
      break;
    }
    case 0x4e: {
      print_instruction(si, "MOV C,M");
      uint8_t data = register_pair_read_byte(si, H_PAIR);
      set_register(&si->cpu, C, data);
      break;
    }
    case 0x4f: {
      register_move(si, C, A);
      break;
    }
    case 0x50: {
      register_move(si, D, B);
      break;
    }
    case 0x51: {
      register_move(si, D, C);
      break;
    }
    case 0x52: {
      register_move(si, D, D);
      break;
    }
    case 0x53: {
      register_move(si, D, E);
      break;
    }
    case 0x54: {
      register_move(si, D, H);
      break;
    }
    case 0x55: {
      register_move(si, D, L);
      break;
    }
    case 0x56: {
      print_instruction(si, "MOV D,M");
      uint8_t data = register_pair_read_byte(si, H_PAIR);
      set_register(&si->cpu, D, data);
      break;
    }
    case 0x57: {
      register_move(si, D, A);
      break;
    }
    case 0x58: {
      register_move(si, E, B);
      break;
    }
    case 0x59: {
      register_move(si, E, C);
      break;
    }
    case 0x5a: {
      register_move(si, E, D);
      break;
    }
    case 0x5b: {
      register_move(si, E, E);
      break;
    }
    case 0x5c: {
      register_move(si, E, H);
      break;
    }
    case 0x5d: {
      register_move(si, E, L);
      break;
    }
    case 0x5e: {
      print_instruction(si, "MOV E,M");
      uint8_t data = register_pair_read_byte(si, H_PAIR);
      set_register(&si->cpu, E, data);
      break;
    }
    case 0x5f: {
      register_move(si, E, A);
      break;
    }
    case 0x60: {
      register_move(si, H, B);
      break;
    }
    case 0x61: {
      register_move(si, H, C);
      break;
    }
    case 0x62: {
      register_move(si, H, D);
      break;
    }
    case 0x63: {
      register_move(si, H, E);
      break;
    }
    case 0x64: {
      register_move(si, H, H);
      break;
    }
    case 0x65: {
      register_move(si, H, L);
      break;
    }
    case 0x66: {
      print_instruction(si, "MOV H,M");
      uint8_t data = register_pair_read_byte(si, H_PAIR);
      set_register(&si->cpu, H, data);
      break;
    }
    case 0x67: {
      register_move(si, H, A);
      break;
    }
    case 0x68: {
      register_move(si, L, B);
      break;
    }
    case 0x69: {
      register_move(si, L, C);
      break;
    }
    case 0x6a: {
      register_move(si, L, D);
      break;
    }
    case 0x6b: {
      register_move(si, L, E);
      break;
    }
    case 0x6c: {
      register_move(si, L, H);
      break;
    }
    case 0x6d: {
      register_move(si, L, L);
      break;
    }
    case 0x6e: {
      print_instruction(si, "MOV L,M");
      uint8_t data = register_pair_read_byte(si, H_PAIR);
      set_register(&si->cpu, L, data);
      break;
    }
    case 0x6f: {
      register_move(si, L, A);
      break;
    }
    case 0x70: {
      print_instruction(si, "MOV M,B");
      uint8_t data = get_register(&si->cpu, B);
      register_pair_write_byte(si, H_PAIR, data);
      break;
    }
    case 0x71: {
      print_instruction(si, "MOV M,C");
      uint8_t data = get_register(&si->cpu, C);
      register_pair_write_byte(si, H_PAIR, data);
      break;
    }
    case 0x72: {
      print_instruction(si, "MOV M,D");
      uint8_t data = get_register(&si->cpu, D);
      register_pair_write_byte(si, H_PAIR, data);
      break;
    }
    case 0x73: {
      print_instruction(si, "MOV M,E");
      uint8_t data = get_register(&si->cpu, E);
      register_pair_write_byte(si, H_PAIR, data);
      break;
    }
    case 0x74: {
      print_instruction(si, "MOV M,H");
      uint8_t data = get_register(&si->cpu, H);
      register_pair_write_byte(si, H_PAIR, data);
      break;
    }
    case 0x75: {
      print_instruction(si, "MOV M,L");
      uint8_t data = get_register(&si->cpu, L);
      register_pair_write_byte(si, H_PAIR, data);
      break;
    }
    case 0x76:
      print_instruction(si, "HLT");
      stop(&si->cpu);
      break;
    case 0x77: {
      print_instruction(si, "MOV M,A");
      uint8_t data = get_register(&si->cpu, A);
      register_pair_write_byte(si, H_PAIR, data);
      break;
    }
    case 0x78: {
      register_move(si, A, B);
      break;
    }
    case 0x79: {
      register_move(si, A, C);
      break;
    }
    case 0x7a: {
      register_move(si, A, D);
      break;
    }
    case 0x7b: {
      register_move(si, A, E);
      break;
    }
    case 0x7c: {
      register_move(si, A, H);
      break;
    }
    case 0x7d: {
      register_move(si, A, L);
      break;
    }
    case 0x7e: {
      print_instruction(si, "MOV A,M");
      uint8_t data = register_pair_read_byte(si, H_PAIR);
      set_register(&si->cpu, A, data);
      break;
    }
    case 0x7f: {
      register_move(si, A, A);
      break;
    }
    case 0x80: {
      register_add(si, B);
      break;
    }
    case 0x81: {
      register_add(si, C);
      break;
    }
    case 0x82: {
      register_add(si, D);
      break;
    }
    case 0x83: {
      register_add(si, E);
      break;
    }
    case 0x84: {
      register_add(si, H);
      break;
    }
    case 0x85: {
      register_add(si, L);
      break;
    }
    case 0x86: {
      print_instruction(si, "ADD M");
      uint8_t data = register_pair_read_byte(si, H_PAIR);
      add_accumulator(&si->cpu, data);
      break;
    }
    case 0x87: {
      register_add(si, A);
      break;
    }
    case 0x88: {
      register_add_with_carry(si, B);
      break;
    }
    case 0x89: {
      register_add_with_carry(si, C);
      break;
    }
    case 0x8a: {
      register_add_with_carry(si, D);
      break;
    }
    case 0x8b: {
      register_add_with_carry(si, E);
      break;
    }
    case 0x8c: {
      register_add_with_carry(si, H);
      break;
    }
    case 0x8d: {
      register_add_with_carry(si, L);
      break;
    }
    case 0x8e: {
      print_instruction(si, "ADC M");
      uint8_t data = register_pair_read_byte(si, H_PAIR);
      add_with_carry_accumulator(&si->cpu, data);
      break;
    }
    case 0x8f: {
      register_add_with_carry(si, A);
      break;
    }
    case 0x90: {
      register_subtract(si, B);
      break;
    }
    case 0x91: {
      register_subtract(si, C);
      break;
    }
    case 0x92: {
      register_subtract(si, D);
      break;
    }
    case 0x93: {
      register_subtract(si, E);
      break;
    }
    case 0x94: {
      register_subtract(si, H);
      break;
    }
    case 0x95: {
      register_subtract(si, L);
      break;
    }
    case 0x96: {
      print_instruction(si, "SUB M");
      uint8_t data = register_pair_read_byte(si, H_PAIR);
      subtract_accumulator(&si->cpu, data);
      break;
    }
    case 0x97: {
      register_subtract(si, A);
      break;
    }
    case 0x98: {
      register_subtract_with_borrow(si, B);
      break;
    }
    case 0x99: {
      register_subtract_with_borrow(si, C);
      break;
    }
    case 0x9a: {
      register_subtract_with_borrow(si, D);
      break;
    }
    case 0x9b: {
      register_subtract_with_borrow(si, E);
      break;
    }
    case 0x9c: {
      register_subtract_with_borrow(si, H);
      break;
    }
    case 0x9d: {
      register_subtract_with_borrow(si, L);
      break;
    }
    case 0x9e: {
      print_instruction(si, "SBB M");
      uint8_t data = register_pair_read_byte(si, H_PAIR);
      subtract_with_borrow_accumulator(&si->cpu, data);
      break;
    }
    case 0x9f: {
      register_subtract_with_borrow(si, A);
      break;
    }
    case 0xa0: {
      register_and(si, B);
      break;
    }
    case 0xa1: {
      register_and(si, C);
      break;
    }
    case 0xa2: {
      register_and(si, D);
      break;
    }
    case 0xa3: {
      register_and(si, E);
      break;
    }
    case 0xa4: {
      register_and(si, H);
      break;
    }
    case 0xa5: {
      register_and(si, L);
      break;
    }
    case 0xa6: {
      print_instruction(si, "ANA M");
      uint8_t data = register_pair_read_byte(si, H_PAIR);
      and_accumulator(&si->cpu, data);
      break;
    }
    case 0xa7: {
      register_and(si, A);
      break;
    }
    case 0xa8: {
      register_exclusive_or(si, B);
      break;
    }
    case 0xa9: {
      register_exclusive_or(si, C);
      break;
    }
    case 0xaa: {
      register_exclusive_or(si, D);
      break;
    }
    case 0xab: {
      register_exclusive_or(si, E);
      break;
    }
    case 0xac: {
      register_exclusive_or(si, H);
      break;
    }
    case 0xad: {
      register_exclusive_or(si, L);
      break;
    }
    case 0xae: {
      print_instruction(si, "XRA M");
      uint8_t data = register_pair_read_byte(si, H_PAIR);
      exclusive_or_accumulator(&si->cpu, data);
      break;
    }
    case 0xaf: {
      register_exclusive_or(si, A);
      break;
    }
    case 0xb0: {
      register_or(si, B);
      break;
    }
    case 0xb1: {
      register_or(si, C);
      break;
    }
    case 0xb2: {
      register_or(si, D);
      break;
    }
    case 0xb3: {
      register_or(si, E);
      break;
    }
    case 0xb4: {
      register_or(si, H);
      break;
    }
    case 0xb5: {
      register_or(si, L);
      break;
    }
    case 0xb6: {
      print_instruction(si, "ORA M");
      uint8_t data = register_pair_read_byte(si, H_PAIR);
      or_accumulator(&si->cpu, data);
      break;
    }
    case 0xb7: {
      register_or(si, A);
      break;
    }
    case 0xb8: {
      register_compare(si, B);
      break;
    }
    case 0xb9: {
      register_compare(si, C);
      break;
    }
    case 0xba: {
      register_compare(si, D);
      break;
    }
    case 0xbb: {
      register_compare(si, E);
      break;
    }
    case 0xbc: {
      register_compare(si, H);
      break;
    }
    case 0xbd: {
      register_compare(si, L);
      break;
    }
    case 0xbe: {
      print_instruction(si, "CMP M");
      uint8_t data = register_pair_read_byte(si, H_PAIR);
      compare_accumulator(&si->cpu, data);
      break;
    }
    case 0xbf: {
      register_compare(si, A);
      break;
    }
    case 0xc0: {
      print_instruction(si, "RNZ");
      subroutine_return_if_not_zero(si);
      break;
    }
    case 0xc1: {
      print_instruction(si, "POP B");
      uint16_t data = stack_pop_word(si);
      set_register_pair(&si->cpu, B_PAIR, data);
      break;
    }
    case 0xc2: {
      uint16_t address = fetch_word(si);
      print_instruction(si, "JNZ %04x", address);
      jump_if_not_zero(&si->cpu, address);
      break;
    }
    case 0xc3: {
      uint16_t address = fetch_word(si);
      print_instruction(si, "JMP %04x", address);
      jump(&si->cpu, address);
      break;
    }
    case 0xc4: {
      uint16_t address = fetch_word(si);
      print_instruction(si, "CNZ %04x", address);
      subroutine_call_if_not_zero(si, address);
      break;
    }
    case 0xc5: {
      print_instruction(si, "PUSH B");
      uint16_t data = get_register_pair(&si->cpu, B_PAIR);
      stack_push_word(si, data);
      break;
    }
    case 0xc6: {
      uint8_t data = fetch_byte(si);
      print_instruction(si, "ADI %02x", data);
      add_accumulator(&si->cpu, data);
      break;
    }
    case 0xc7: {
      restart(si, 0);
      break;
    }
    case 0xc8: {
      print_instruction(si, "RZ");
      subroutine_return_if_zero(si);
      break;
    }
    case 0xc9: {
      print_instruction(si, "RET");
      subroutine_return(si);
      break;
    }
    case 0xca: {
      uint16_t address = fetch_word(si);
      print_instruction(si, "JZ %04x", address);
      jump_if_zero(&si->cpu, address);
      break;
    }
    case 0xcb: {
      uint16_t address = fetch_word(si);
      print_instruction(si, "JMP %04x", address);
      jump(&si->cpu, address);
      break;
    }
    case 0xcc: {
      uint16_t address = fetch_word(si);
      print_instruction(si, "CZ %04x", address);
      subroutine_call_if_zero(si, address);
      break;
    }
    case 0xcd: {
      uint16_t address = fetch_word(si);
      print_instruction(si, "CALL %04x", address);
      subroutine_call(si, address);
      break;
    }
    case 0xce: {
      uint8_t data = fetch_byte(si);
      print_instruction(si, "ACI %02x", data);
      add_register_accumulator_with_carry(&si->cpu, data);
      break;
    }
    case 0xcf: {
      restart(si, 1);
      break;
    }
    case 0xd0: {
      print_instruction(si, "RNC");
      subroutine_return_if_no_carry(si);
      break;
    }
    case 0xd1: {
      print_instruction(si, "POP D");
      uint16_t data = stack_pop_word(si);
      set_register_pair(&si->cpu, D_PAIR, data);
      break;
    }
    case 0xd2: {
      uint16_t address = fetch_word(si);
      print_instruction(si, "JNC %04x", address);
      jump_if_no_carry(&si->cpu, address);
      break;
    }
    case 0xd3: {
      uint8_t device = fetch_byte(si);
      print_instruction(si, "OUT %02x", device);
      uint8_t data = get_register(&si->cpu, A);
      printf("* A register value %02x sent to output device %02x -> ", data, device);
      switch (device) {
        case 2:
          printf("SHFTAMNT");
          break;
        case 3:
          printf("SOUND1");
          break;
        case 4:
          printf("SHFT_DATA");
          break;
        case 5:
          printf("SOUND2");
          break;
        case 6:
          printf("WATCHDOG");
          break;
        default:
          printf("UNKNOWN INPUT");
      }
      printf("\n");
      break;
    }
    case 0xd4: {
      uint16_t address = fetch_word(si);
      print_instruction(si, "CNC %04x", address);
      subroutine_call_if_no_carry(si, address);
      break;
    }
    case 0xd5: {
      print_instruction(si, "PUSH D");
      uint16_t data = get_register_pair(&si->cpu, D_PAIR);
      stack_push_word(si, data);
      break;
    }
    case 0xd6: {
      uint8_t data = fetch_byte(si);
      print_instruction(si, "SUI %02x", data);
      subtract_accumulator(&si->cpu, data);
      break;
    }
    case 0xd7: {
      restart(si, 2);
      break;
    }
    case 0xd8: {
      print_instruction(si, "RC");
      subroutine_return_if_carry(si);
      break;
    }
    case 0xd9: {
      print_instruction(si, "RET");
      subroutine_return(si);
      break;
    }
    case 0xda: {
      uint16_t address = fetch_word(si);
      print_instruction(si, "JC %04x", address);
      jump_if_carry(&si->cpu, address);
      break;
    }
    case 0xdb: {
      uint8_t device = fetch_byte(si);
      print_instruction(si, "IN %02x", device);

      uint8_t data = 0; // TODO: read from input
      set_register(&si->cpu, A, data);
      printf("* A register value set to %02x, received from input device %02x -> ", data, device);
      switch (device) {
        case 0:
        case 1:
        case 2:
          printf("INP%d", device);
          break;
        case 3:
          printf("SHFT_IN");
          break;
        default:
          printf("UNKNOWN INPUT");
      }
      printf("\n");
      break;
    }
    case 0xdc: {
      uint16_t address = fetch_word(si);
      print_instruction(si, "CC %04x", address);
      subroutine_call_if_carry(si, address);
      break;
    }
    case 0xdd: {
      uint16_t address = fetch_word(si);
      print_instruction(si, "CALL %04x", address);
      subroutine_call(si, address);
      break;
    }
    case 0xde: {
      uint8_t data = fetch_byte(si);
      print_instruction(si, "SBI %02x", data);
      subtract_with_borrow_accumulator(&si->cpu, data);
      break;
    }
    case 0xdf: {
      restart(si, 3);
      break;
    }
    case 0xe0: {
      print_instruction(si, "RPO");
      subroutine_return_if_parity_odd(si);
      break;
    }
    case 0xe1: {
      print_instruction(si, "POP H");
      uint16_t data = stack_pop_word(si);
      set_register_pair(&si->cpu, H_PAIR, data);
      break;
    }
    case 0xe2: {
      uint16_t address = fetch_word(si);
      print_instruction(si, "JPO %04x", address);
      jump_if_parity_odd(&si->cpu, address);
      break;
    }
    case 0xe3: {
      print_instruction(si, "XTHL");
      uint16_t sp_data = read_word(si, si->cpu.sp);
      uint16_t hl_data = get_register_pair(&si->cpu, H_PAIR);
      set_register_pair(&si->cpu, H_PAIR, sp_data);
      write_word(si, si->cpu.sp, hl_data);
      break;
    }
    case 0xe4: {
      uint16_t address = fetch_word(si);
      print_instruction(si, "CPO %04x", address);
      subroutine_call_if_parity_odd(si, address);
      break;
    }
    case 0xe5: {
      print_instruction(si, "PUSH H");
      uint16_t data = get_register_pair(&si->cpu, H_PAIR);
      stack_push_word(si, data);
      break;
    }
    case 0xe6: {
      uint8_t data = fetch_byte(si);
      print_instruction(si, "ANI %02x", data);
      and_accumulator(&si->cpu, data);
      break;
    }
    case 0xe7: {
      restart(si, 4);
      break;
    }
    case 0xe8: {
      print_instruction(si, "RPE");
      subroutine_return_if_parity_even(si);
      break;
    }
    case 0xe9: {
      print_instruction(si, "PCHL");
      load_pc(&si->cpu);
      break;
    }
    case 0xea: {
      uint16_t address = fetch_word(si);
      print_instruction(si, "JPE %04x", address);
      jump_if_parity_even(&si->cpu, address);
      break;
    }
    case 0xeb: {
      print_instruction(si, "XCHG");
      exchange_registers(&si->cpu);
      break;
    }
    case 0xec: {
      uint16_t address = fetch_word(si);
      print_instruction(si, "CPE %04x", address);
      subroutine_call_if_parity_even(si, address);
      break;
    }
    case 0xed: {
      uint16_t address = fetch_word(si);
      print_instruction(si, "CALL %04x", address);
      subroutine_call(si, address);
      break;
    }
    case 0xee: {
      uint8_t data = fetch_byte(si);
      print_instruction(si, "XRI %02x", data);
      exclusive_or_accumulator(&si->cpu, data);
      break;
    }
    case 0xef: {
      restart(si, 5);
      break;
    }
    case 0xf0: {
      print_instruction(si, "RP");
      subroutine_return_if_plus(si);
      break;
    }
    case 0xf1: {
      print_instruction(si, "POP PSW");
      uint16_t data = stack_pop_word(si);
      set_register_pair(&si->cpu, PSW, data);
      break;
    }
    case 0xf2: {
      uint16_t address = fetch_word(si);
      print_instruction(si, "JP %04x", address);
      jump_if_positive(&si->cpu, address);
      break;
    }
    case 0xf3: {
      print_instruction(si, "DI");
      disable_interrupt(&si->cpu);
      break;
    }
    case 0xf4: {
      uint16_t address = fetch_word(si);
      print_instruction(si, "CP %04x", address);
      subroutine_call_if_plus(si, address);
      break;
    }
    case 0xf5: {
      print_instruction(si, "PUSH PSW");
      uint16_t data = get_register_pair(&si->cpu, PSW);
      stack_push_word(si, data);
      break;
    }
    case 0xf6: {
      uint8_t data = fetch_byte(si);
      print_instruction(si, "ORI %02x", data);
      or_accumulator(&si->cpu, data);
      break;
    }
    case 0xf7: {
      restart(si, 6);
      break;
    }
    case 0xf8: {
      print_instruction(si, "RM");
      subroutine_return_if_minus(si);
      break;
    }
    case 0xf9: {
      print_instruction(si, "SPHL");
      load_sp(&si->cpu);
      break;
    }
    case 0xfa: {
      uint16_t address = fetch_word(si);
      print_instruction(si, "JM %04x", address);
      jump_if_minus(&si->cpu, address);
      break;
    }
    case 0xfb: {
      print_instruction(si, "EI");
      enable_interrupt(&si->cpu);
      break;
    }
    case 0xfc: {
      uint16_t address = fetch_word(si);
      print_instruction(si, "CM %04x", address);
      subroutine_call_if_minus(si, address);
      break;
    }
    case 0xfd: {
      uint16_t address = fetch_word(si);
      print_instruction(si, "CALL %04x", address);
      subroutine_call(si, address);
      break;
    }
    case 0xfe: {
      uint8_t data = fetch_byte(si);
      print_instruction(si, "CPI %02x", data);
      compare_accumulator(&si->cpu, data);
      break;
    }
    case 0xff: {
      restart(si, 7);
      break;
    }
  }
}

void draw_screen(Memory *memory) {
  const int line_bytes = 0x20;

  ClearBackground(color1);

  for (int i = 0; i < VRAM_SIZE; i++) {
    const uint8_t data = memory_read_byte(memory, VRAM_ADDRESS + i);
    if (data == 0) {
      continue;
    }
    const int x = 8 * (i % line_bytes);
    const int y = i / line_bytes;
    for (int j = 0; j < 8; j++) {
      if (data >> j & 1) {
        DrawRectangle(x + j, y, 1, 1, color2);
      }
    }
  }
}

void run(SpaceInvaders *si) {
  InitWindow(window_height * scale + offset * 2, window_width * scale + offset * 2, "Space Invaders");
  // SetTargetFPS(60); // TODO: implement clock (separate update loop from drawing one)

  RenderTexture2D target = LoadRenderTexture(window_width, window_height);

  // TODO: specify rom to load from program arg
  program_rom(si);
  //  program_test_rom(si);
  //  program_hardcoded(si);

  while (!WindowShouldClose() && !is_stopped(&si->cpu))
  {
    cycle(si);
    print_state_8080(&si->cpu);
    printf("····················\n");
    print_stack(si);
    printf("~~~~~~~~~~~~~~~~~~~~\n");

    BeginTextureMode(target);
      draw_screen(&si->memory);
    EndTextureMode();

    BeginDrawing();
      ClearBackground(color2);
      Rectangle source = {
        0.0f,
        0.0f,
        (float) target.texture.width,
        -(float) target.texture.height
      };
      Rectangle dest = {
        (float) offset,
        (float) offset + (float) window_width * (float) scale,
        (float) target.texture.width * (float) scale,
        (float) target.texture.height * (float) scale
      };
      Vector2 origin = { 0.0f, 0.0f };

      DrawTexturePro(target.texture, source, dest, origin, rotation, WHITE);
    EndDrawing();
  }

  UnloadRenderTexture(target);

  CloseWindow();
}

int main(void) {
  SpaceInvaders *si = new();
  run(si);

  return 0;
}
