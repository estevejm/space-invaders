#include <stdint.h>
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
  PSW = 0,
  B_PAIR = 2,
  D_PAIR = 4,
  H_PAIR = 6,
  SP = 8 // not stored in registers array
};

typedef struct i8080 {
  uint8_t registers[REGISTER_COUNT];
  uint16_t pc;
  uint16_t sp;
  bool halted;
} I8080;

uint8_t get_register(I8080 *cpu, enum Register r);
void set_register(I8080 *cpu, enum Register r, uint8_t value);
void copy_register(I8080 *cpu, enum Register dst, enum Register src);
void increment_register(I8080 *cpu, enum Register r);
void decrement_register(I8080 *cpu, enum Register r);

uint16_t get_register_pair(I8080 *cpu, enum RegisterPair r);
void set_register_pair(I8080 *cpu, enum RegisterPair r, uint16_t value);
void increment_register_pair(I8080 *cpu, enum RegisterPair r);
void decrement_register_pair(I8080 *cpu, enum RegisterPair r);
void double_add(I8080 *cpu, enum RegisterPair r);

void compare_immediate_accumulator(I8080 *cpu, uint8_t value);

void decimal_adjust_accumulator(I8080 *cpu);

void restart(I8080 *cpu, uint8_t value);
void no_operation(I8080 *cpu);
void halt(I8080 *cpu);
bool is_halted(I8080 *cpu);

void jump(I8080 *cpu, uint16_t address);
void jump_if_zero(I8080 *cpu, uint16_t address);
void jump_if_not_zero(I8080 *cpu, uint16_t address);
void jump_if_parity_even(I8080 *cpu, uint16_t address);
void jump_if_no_carry(I8080 *cpu, uint16_t address);

void subroutine_call(I8080 *cpu, uint16_t address);
void subroutine_call_if_zero(I8080 *cpu, uint16_t address);
void subroutine_call_if_plus(I8080 *cpu, uint16_t address);

void subroutine_return(I8080 *cpu);
void subroutine_return_if_plus(I8080 *cpu);

bool get_sign_flag(I8080 *cpu);
bool get_zero_flag(I8080 *cpu);
bool get_auxiliary_carry_flag(I8080 *cpu);
bool get_parity_flag(I8080 *cpu);
bool get_carry_flag(I8080 *cpu);

void print_state_8080(I8080 *cpu);

#endif //I8080_H
