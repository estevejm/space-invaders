#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifndef SPACE_INVADERS_H
#define SPACE_INVADERS_H

#define MEMORY_BYTES (1 << 14)

typedef struct memory {
  uint8_t bytes[MEMORY_BYTES];
} Memory;

void memory_write(Memory *memory, uint8_t bytes[], int address, int size);
void memory_peek(Memory *memory, int from, int to);
void memory_peek_highlight(Memory *memory, int address, int size, int highlight);
void memory_dump(Memory *memory);
void memory_write_byte(Memory *memory, uint16_t address, uint8_t data);
uint8_t memory_read_byte(Memory *memory, uint16_t address);

#define REGISTER_COUNT 8

enum Register {
  A, F, // SZ0A0P1C
  B, C,
  D, E,
  H, L,
};

static const char register_names[] = {
  'A', 'F',
  'B', 'C',
  'D', 'E',
  'H', 'L',
};

enum RegisterPair {
  PSW = 0,
  B_PAIR = 2,
  D_PAIR = 4,
  H_PAIR = 6,
  SP = 8 // not stored in registers array
};

static const char* register_pair_names[] = {
  "PSW", "",
  "B", "",
  "D", "",
  "H", "", // TODO: handle M when moving
  "SP", "",
};

typedef struct i8080 {
  uint8_t registers[REGISTER_COUNT];
  uint16_t pc;
  uint16_t sp;
  bool interrupt_enabled;
  bool stopped;
} I8080;

uint8_t get_register(I8080 *cpu, enum Register r);
void set_register(I8080 *cpu, enum Register r, uint8_t value);
void copy_register(I8080 *cpu, enum Register dst, enum Register src);
void increment_register(I8080 *cpu, enum Register r);
void decrement_register(I8080 *cpu, enum Register r);

void add_accumulator(I8080 *cpu, uint8_t value);
void add_with_carry_accumulator(I8080 *cpu, uint8_t value);
void subtract_accumulator(I8080 *cpu, uint8_t value);
void subtract_with_borrow_accumulator(I8080 *cpu, uint8_t value);
void add_register_accumulator(I8080 *cpu, enum Register r);
void add_register_accumulator_with_carry(I8080 *cpu, enum Register r);
void subtract_register_accumulator(I8080 *cpu, enum Register r);
void subtract_register_accumulator_with_borrow(I8080 *cpu, enum Register r);
void and_accumulator(I8080 *cpu, uint8_t value);
void and_register_accumulator(I8080 *cpu, enum Register r);
void exclusive_or_accumulator(I8080 *cpu, uint8_t value);
void exclusive_or_register_accumulator(I8080 *cpu, enum Register r);
void or_accumulator(I8080 *cpu, uint8_t value);
void or_register_accumulator(I8080 *cpu, enum Register r);
void compare_accumulator(I8080 *cpu, uint8_t value);
void compare_register_accumulator(I8080 *cpu, enum Register r);
void rotate_accumulator_left(I8080 *cpu);
void rotate_accumulator_right(I8080 *cpu);
void rotate_accumulator_left_through_carry(I8080 *cpu);
void rotate_accumulator_right_through_carry(I8080 *cpu);
void complement_accumulator(I8080 *cpu);

void complement_carry(I8080 *cpu);
void set_carry(I8080 *cpu);

uint16_t get_register_pair(I8080 *cpu, enum RegisterPair r);
void set_register_pair(I8080 *cpu, enum RegisterPair r, uint16_t value);
void increment_register_pair(I8080 *cpu, enum RegisterPair r);
void decrement_register_pair(I8080 *cpu, enum RegisterPair r);
void double_add(I8080 *cpu, enum RegisterPair r);
void exchange_registers(I8080 *cpu);
void load_pc(I8080 *cpu);
void load_sp(I8080 *cpu);

void decimal_adjust_accumulator(I8080 *cpu);

void jump(I8080 *cpu, uint16_t address);
void jump_if_carry(I8080 *cpu, uint16_t address);
void jump_if_no_carry(I8080 *cpu, uint16_t address);
void jump_if_zero(I8080 *cpu, uint16_t address);
void jump_if_not_zero(I8080 *cpu, uint16_t address);
void jump_if_minus(I8080 *cpu, uint16_t address);
void jump_if_positive(I8080 *cpu, uint16_t address);
void jump_if_parity_even(I8080 *cpu, uint16_t address);
void jump_if_parity_odd(I8080 *cpu, uint16_t address);

bool get_sign_flag(I8080 *cpu);
bool get_zero_flag(I8080 *cpu);
bool get_auxiliary_carry_flag(I8080 *cpu);
bool get_parity_flag(I8080 *cpu);
bool get_carry_flag(I8080 *cpu);

void stop(I8080 *cpu);
bool is_stopped(I8080 *cpu);
void enable_interrupt(I8080 *cpu);
void disable_interrupt(I8080 *cpu);

void print_state_8080(I8080 *cpu);

#endif //SPACE_INVADERS_H
