#include <stdio.h>
#include <string.h>
#include "i8080.h"

const char register_names[] = {
  'A', 'F',
  'B', 'C',
  'D', 'E',
  'H', 'L',
};

uint8_t get_register(I8080 *cpu, enum Register r) {
  return cpu->registers[r];
}

void set_register(I8080 *cpu, enum Register r, uint8_t data) {
  cpu->registers[r] = data;
}

void copy_register(I8080 *cpu, enum Register dst, enum Register src) {
  cpu->registers[dst] = cpu->registers[src];
}

void increment_register(I8080 *cpu, enum Register r) {
  cpu->registers[r]++;
}

void decrement_register(I8080 *cpu, enum Register r) {
  cpu->registers[r]--;
}

uint16_t get_register_pair(I8080 *cpu, enum RegisterPair r) {
  return cpu->registers[r] << 8 | cpu->registers[r + 1];
}

void set_register_pair(I8080 *cpu, enum RegisterPair r, uint16_t value) {
  cpu->registers[r] = value >> 8;
  cpu->registers[r+1] = value & 0xff;
}

void decrement_register_pair(I8080 *cpu, enum RegisterPair r) {
  uint16_t value = get_register_pair(cpu, r);
  value--;
  set_register_pair(cpu, r, value);
}

bool is_plus_8080(I8080 *cpu) {
  return ~(cpu->registers[F] >> 7 & 1);
}

bool is_zero_8080(I8080 *cpu) {
  return ~(cpu->registers[F] >> 6 & 1);
}

bool is_carry_8080(I8080 *cpu) {
  return cpu->registers[F] & 1;
}

bool is_auxiliary_carry_8080(I8080 *cpu) {
  return cpu->registers[F] >> 4 & 1;
}

bool is_parity_even_8080(I8080 *cpu) {
  return cpu->registers[F] >> 2 & 1;
}

void print_state_8080(I8080 *cpu) {
  for (int i = 0; i < REGISTER_COUNT; i++) {
    printf("%c|%02x", register_names[i], cpu->registers[i]);
    if (i % 2 == 0) {
       printf(" ");
    } else {
      printf("\n");
    }
  }

  printf("SP | %04x\n", cpu->sp);
  printf("PC | %04x\n", cpu->pc);
}
