#include <stdlib.h>
#include <stdbool.h>

#ifndef I8080_H
#define I8080_H

#define REGISTER_COUNT 8

enum Register {
  A, F, // SZ0A0P1C
  B, C,
  D, E,
  H, L,
};

enum RegisterPair {
  B_PAIR = 2,
  D_PAIR = 4,
  H_PAIR = 6
};

typedef struct i8080 {
  uint8_t data; // TODO: implement in a way this is used?
  uint16_t address; // TODO: implement in a way this is used?
  uint8_t registers[REGISTER_COUNT];
  uint16_t pc;
  uint16_t sp;
} I8080;

uint8_t get_register(I8080 *cpu, enum Register r);
void set_register(I8080 *cpu, enum Register r, uint8_t data);
void copy_register(I8080 *cpu, enum Register dst, enum Register src);
void increment_register(I8080 *cpu, enum Register r);
void decrement_register(I8080 *cpu, enum Register r);

uint16_t get_register_pair(I8080 *cpu, enum RegisterPair r);
void set_register_pair(I8080 *cpu, enum RegisterPair r, uint16_t value);
void decrement_register_pair(I8080 *cpu, enum RegisterPair r);

bool is_plus_8080(I8080 *cpu);
bool is_zero_8080(I8080 *cpu);
bool is_carry_8080(I8080 *cpu);
bool is_auxiliary_carry_8080(I8080 *cpu);
bool is_parity_even_8080(I8080 *cpu);

void print_state_8080(I8080 *cpu);

#endif //I8080_H
