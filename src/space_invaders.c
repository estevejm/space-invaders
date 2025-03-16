#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
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
  bool halted;
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

void print_instruction(SpaceInvaders *si, char *format, ...) {
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
  printf("\n··············\n");
}

void print_bus(SpaceInvaders *si) {
  printf("* %c %04x %02x\n", si->write ? 'w' : 'r', si->address, si->data);
}

void print_stack(SpaceInvaders *si) {
  int stackPointerMemoryLineStart = si->cpu.sp & 0xfff0;
  if (si->cpu.sp % 0x10 == 0) {
    stackPointerMemoryLineStart -= 0x10;
    stackPointerMemoryLineStart = stackPointerMemoryLineStart & 0xffff;
  }
  memory_peek(&si->memory, stackPointerMemoryLineStart, 0x10);
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

void subroutine_call_if_zero(SpaceInvaders *si, uint16_t address) {
  if (get_zero_flag(&si->cpu)) {
    subroutine_call(si, address);
  }
}

void subroutine_call_if_plus(SpaceInvaders *si, uint16_t address) {
  if (!get_sign_flag(&si->cpu)) {
    subroutine_call(si, address);
  }
}

void subroutine_return(SpaceInvaders *si) {
  si->cpu.pc = stack_pop_word(si);
}

void subroutine_return_if_plus(SpaceInvaders *si) {
  if (!get_sign_flag(&si->cpu)) {
    subroutine_return(si);
  }
}

void no_operation(SpaceInvaders *si) {
  print_instruction(si, "NOP");
}

void halt(SpaceInvaders *si) {
  print_instruction(si, "HLT");
  si->halted = true;
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
    case 0x10:
      no_operation(si);
      break;
    case 0x11: {
      uint16_t data = fetch_word(si);
      print_instruction(si, "LXI D,%04x", data);
      set_register_pair(&si->cpu, D_PAIR, data);
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
    case 0x18:
      no_operation(si);
      break;
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
    case 0x20:
      no_operation(si);
      break;
    case 0x21: {
      uint16_t data = fetch_word(si);
      print_instruction(si, "LXI H,%04x", data);
      set_register_pair(&si->cpu, H_PAIR, data);
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
    case 0x30:
      no_operation(si);
      break;
    case 0x31: {
      uint16_t data = fetch_word(si);
      print_instruction(si, "LXI SP,%04x", data);
      set_register_pair(&si->cpu, SP, data);
      break;
    }
    case 0x33:
      register_pair_increment(si, SP);
      break;
    case 0x38:
      no_operation(si);
      break;
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
    case 0x57: {
      register_move(si, D, A);
      break;
    }
    case 0x76:
      halt(si);
      break;
    case 0x77: {
      print_instruction(si, "MOV M,A");
      uint8_t data = get_register(&si->cpu, A);
      register_pair_write_byte(si, H_PAIR, data);
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
    case 0xc5: {
      print_instruction(si, "PUSH B");
      uint16_t data = get_register_pair(&si->cpu, B_PAIR);
      stack_push_word(si, data);
      break;
    }
    case 0xc7: {
      uint8_t exp = 0;
      print_instruction(si, "RST %d", exp);
      restart(&si->cpu, exp);
      break;
    }
    case 0xca: {
      uint16_t address = fetch_word(si);
      print_instruction(si, "JZ %04x", address);
      jump_if_zero(&si->cpu, address);
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
    case 0xd5: {
      print_instruction(si, "PUSH D");
      uint16_t data = get_register_pair(&si->cpu, D_PAIR);
      stack_push_word(si, data);
      break;
    }
    case 0xe1: {
      print_instruction(si, "POP H");
      uint16_t data = stack_pop_word(si);
      set_register_pair(&si->cpu, H_PAIR, data);
      break;
    }
    case 0xe5: {
      print_instruction(si, "PUSH H");
      uint16_t data = get_register_pair(&si->cpu, H_PAIR);
      stack_push_word(si, data);
      break;
    }
    case 0xea: {
      uint16_t address = fetch_word(si);
      print_instruction(si, "JPE %04x", address);
      jump_if_parity_even(&si->cpu, address);
      break;
    }
    case 0xf1: {
      print_instruction(si, "POP PSW");
      uint16_t data = stack_pop_word(si);
      set_register_pair(&si->cpu, PSW, data);
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
    case 0xfe: {
      uint8_t data = fetch_byte(si);
      print_instruction(si, "CPI %02x", data);
      compare_immediate_accumulator(&si->cpu, data);
      break;
    }
    default:
      print_instruction(si, "UNKNOWN OPCODE");
      exit(1);
  }
}

void run(SpaceInvaders *si) {
  // TODO: specify rom to load from program arg
  program_rom(si);
//  program_test_rom(si);
//  program_hardcoded(si);
  while (!si->halted) {
    print_state_8080(&si->cpu);
    printf("··············\n");
    print_stack(si);
    printf("~~~~~~~~~~~~~~\n");
    peek_next_bytes(si);
    cycle(si);
    // TODO: implement clock
    usleep(1 * 1000000);
  }
}
