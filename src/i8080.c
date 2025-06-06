#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "space_invaders.h"

#define CARRY_FLAG_POS 0
#define PARITY_FLAG_POS 2
#define AUX_CARRY_FLAG_POS 4
#define ZERO_FLAG_POS 6
#define SIGN_FLAG_POS 7

const int bitcount[] = {
  0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
  4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8,
};

bool get_bit(uint8_t byte, uint8_t pos) {
  return byte >> pos & 1;
}

void set_bit(uint8_t *byte, uint8_t pos, bool value) {
  uint8_t mask = 1 << pos;
  if (value) {
    *byte |= mask;
  } else {
    *byte &= ~mask;
  }
}

// [S]Z0A0P1C
bool get_sign_flag(I8080 *cpu) {
  return get_bit(cpu->registers[F], SIGN_FLAG_POS);
}

void set_sign_flag(I8080 *cpu, uint8_t result) {
  set_bit(&cpu->registers[F], SIGN_FLAG_POS, result >> 7 & 1);
}

// S[Z]0A0P1C
bool get_zero_flag(I8080 *cpu) {
  return get_bit(cpu->registers[F], ZERO_FLAG_POS);
}

void set_zero_flag(I8080 *cpu, uint8_t result) {
  set_bit(&cpu->registers[F], ZERO_FLAG_POS, result == 0);
}

// SZ0[A]0P1C
bool get_auxiliary_carry_flag(I8080 *cpu) {
  return get_bit(cpu->registers[F], AUX_CARRY_FLAG_POS);
}

void set_auxiliary_carry_flag(I8080 *cpu, bool value) {
  set_bit(&cpu->registers[F], AUX_CARRY_FLAG_POS, value);
}

// SZ0A0[P]1C
bool get_parity_flag(I8080 *cpu) {
  return get_bit(cpu->registers[F], PARITY_FLAG_POS);
}

void set_parity_flag(I8080 *cpu, uint8_t result) {
  set_bit(&cpu->registers[F], PARITY_FLAG_POS, bitcount[result] % 2 == 0);
}

// SZ0A0P1[C]
bool get_carry_flag(I8080 *cpu) {
  return get_bit(cpu->registers[F], CARRY_FLAG_POS);
}

void set_carry_flag(I8080 *cpu, bool value) {
  set_bit(&cpu->registers[F], CARRY_FLAG_POS, value);
}

uint8_t get_register(I8080 *cpu, enum Register r) {
  return cpu->registers[r];
}

void set_register(I8080 *cpu, enum Register r, uint8_t value) {
  cpu->registers[r] = value;
}

void copy_register(I8080 *cpu, enum Register dst, enum Register src) {
  cpu->registers[dst] = cpu->registers[src];
}

bool nth_carry_occurs(uint32_t a, uint32_t b, uint32_t cin, uint32_t result, int n) {
  return (a ^ b ^ cin ^ result) >> n & 1;
}

bool half_carry_occurs(int8_t a, uint8_t b, uint8_t result) {
  return nth_carry_occurs(a, b, 0, result, 4);
}

bool half_carry_occurs_with_carry_in(int8_t a, int8_t b, int8_t cin, int8_t result) {
  return nth_carry_occurs(a, b, cin, result, 4);
}

bool carry_occurs(int16_t a, uint16_t b, uint16_t result) {
  return nth_carry_occurs(a, b, 0, result, 8);
}

bool carry_occurs_with_carry_in(int16_t a, uint16_t b, uint16_t cin, uint16_t result) {
  return nth_carry_occurs(a, b, cin, result, 8);
}

bool double_carry_occurs(uint32_t a, uint32_t b, uint32_t result) {
  return nth_carry_occurs(a, b, 0, result, 16);
}

void add_full_accumulator(I8080 *cpu, uint8_t value, uint8_t cin, bool sub) {
  uint8_t a = cpu->registers[A];
  uint8_t b = sub ? -value : value;
  uint16_t result = a + b + cin;

  set_sign_flag(cpu, result);
  set_zero_flag(cpu, result);
  set_parity_flag(cpu, result);

  bool hc = half_carry_occurs_with_carry_in(a, b, cin, result);
  set_auxiliary_carry_flag(cpu, sub ? !hc : hc);

  bool c = carry_occurs_with_carry_in(a, b, cin, result);
  set_carry_flag(cpu, sub ? !c : c);

  cpu->registers[A] = result;
}

void increment_register(I8080 *cpu, enum Register r) {
  uint8_t a = cpu->registers[r];
  uint8_t b = 1;
  uint8_t result = a + b;

  set_sign_flag(cpu, result);
  set_zero_flag(cpu, result);
  set_parity_flag(cpu, result);
  set_auxiliary_carry_flag(cpu, half_carry_occurs(a, b, result));

  cpu->registers[r] = result;
}

void decrement_register(I8080 *cpu, enum Register r) {
  uint8_t a = cpu->registers[r];
  uint8_t b = 1;
  uint8_t result = a - b; // TODO: use addition always? result = a + (-b) so we can have a uniform (aux) carry flag check?

  set_sign_flag(cpu, result);
  set_zero_flag(cpu, result);
  set_parity_flag(cpu, result);
  set_auxiliary_carry_flag(cpu, !half_carry_occurs(a, b, result));

  cpu->registers[r] = result;
}

void add_accumulator(I8080 *cpu, uint8_t value) {
  add_full_accumulator(cpu, value, 0, false);
}

void subtract_accumulator(I8080 *cpu, uint8_t value) {
  add_full_accumulator(cpu, value, 0, true);
}

void add_with_carry_accumulator(I8080 *cpu, uint8_t value) {
  add_full_accumulator(cpu, value, get_carry_flag(cpu), false);
}

void subtract_with_borrow_accumulator(I8080 *cpu, uint8_t value) {
  add_full_accumulator(cpu, value, get_carry_flag(cpu), true);
}

void add_register_accumulator(I8080 *cpu, enum Register r) {
  add_accumulator(cpu, cpu->registers[r]);
}

void add_register_accumulator_with_carry(I8080 *cpu, enum Register r) {
  add_with_carry_accumulator(cpu, cpu->registers[r]);
}

void subtract_register_accumulator(I8080 *cpu, enum Register r) {
  subtract_accumulator(cpu, cpu->registers[r]);
}

void subtract_register_accumulator_with_borrow(I8080 *cpu, enum Register r) {
  subtract_with_borrow_accumulator(cpu, cpu->registers[r]);
}

void and_accumulator(I8080 *cpu, uint8_t value) {
  cpu->registers[A] &= value;

  set_sign_flag(cpu, cpu->registers[A]);
  set_zero_flag(cpu, cpu->registers[A]);
  set_parity_flag(cpu, cpu->registers[A]);
  set_carry_flag(cpu, 0);
}

void and_register_accumulator(I8080 *cpu, enum Register r) {
  and_accumulator(cpu, cpu->registers[r]);
}

void or_accumulator(I8080 *cpu, uint8_t value) {
  cpu->registers[A] |= value;

  set_sign_flag(cpu, cpu->registers[A]);
  set_zero_flag(cpu, cpu->registers[A]);
  set_parity_flag(cpu, cpu->registers[A]);
  set_carry_flag(cpu, 0);
}

void or_register_accumulator(I8080 *cpu, enum Register r) {
  or_accumulator(cpu, cpu->registers[r]);
}

void exclusive_or_accumulator(I8080 *cpu, uint8_t value) {
  cpu->registers[A] ^= value;

  set_sign_flag(cpu, cpu->registers[A]);
  set_zero_flag(cpu, cpu->registers[A]);
  set_parity_flag(cpu, cpu->registers[A]);
  set_carry_flag(cpu, 0);
}

void exclusive_or_register_accumulator(I8080 *cpu, enum Register r) {
  exclusive_or_accumulator(cpu, cpu->registers[r]);
}

// TODO: reuse subtract logic
void compare_accumulator(I8080 *cpu, uint8_t value) {
  uint8_t a = cpu->registers[A];
  uint8_t b = -value;
  uint16_t result = a + b;

  set_sign_flag(cpu, result);
  set_zero_flag(cpu, result);
  set_parity_flag(cpu, result);
  set_auxiliary_carry_flag(cpu, !half_carry_occurs(a, b, result));
  set_carry_flag(cpu, !carry_occurs(a, b, result));
}

void compare_register_accumulator(I8080 *cpu, enum Register r) {
  compare_accumulator(cpu, cpu->registers[r]);
}

void rotate_accumulator_left(I8080 *cpu) {
  uint8_t msb = cpu->registers[A] >> 7 & 1;
  cpu->registers[A] = cpu->registers[A] << 1 | msb;
  set_carry_flag(cpu, msb);
}

void rotate_accumulator_right(I8080 *cpu) {
  uint8_t lsb = cpu->registers[A] & 1;
  cpu->registers[A] = lsb << 7 | cpu->registers[A] >> 1;
  set_carry_flag(cpu, lsb);
}

void rotate_accumulator_left_through_carry(I8080 *cpu) {
  uint8_t msb = cpu->registers[A] >> 7 & 1;
  cpu->registers[A] = cpu->registers[A] << 1 | get_carry_flag(cpu);
  set_carry_flag(cpu, msb);
}

void rotate_accumulator_right_through_carry(I8080 *cpu) {
  uint8_t lsb = cpu->registers[A] & 1;
  cpu->registers[A] = get_carry_flag(cpu) << 7 | cpu->registers[A] >> 1;
  set_carry_flag(cpu, lsb);
}

void complement_accumulator(I8080 *cpu) {
  cpu->registers[A] = ~cpu->registers[A];
}

void complement_carry(I8080 *cpu) {
  set_carry_flag(cpu, !get_carry_flag(cpu));
}

void set_carry(I8080 *cpu) {
  set_carry_flag(cpu, 1);
}

uint16_t get_register_pair(I8080 *cpu, enum RegisterPair r) {
  if (r == SP) {
    return cpu->sp;
  }

  return cpu->registers[r] << 8 | cpu->registers[r + 1];
}

void set_register_pair(I8080 *cpu, enum RegisterPair r, uint16_t value) {
  if (r == SP) {
    cpu->sp = value;
    return;
  }
  cpu->registers[r] = value >> 8;
  cpu->registers[r+1] = value & 0xff;
}

void increment_register_pair(I8080 *cpu, enum RegisterPair r) {
  uint16_t value = get_register_pair(cpu, r);
  value++;
  set_register_pair(cpu, r, value);
}

void decrement_register_pair(I8080 *cpu, enum RegisterPair r) {
  uint16_t value = get_register_pair(cpu, r);
  value--;
  set_register_pair(cpu, r, value);
}

void double_add(I8080 *cpu, enum RegisterPair r) {
  // TODO: optimize H_PAIR use case? (just shift left 1 position)
  uint32_t rp = get_register_pair(cpu, r);
  uint32_t hl = get_register_pair(cpu, H_PAIR);
  uint32_t result = rp + hl;
  set_register_pair(cpu, H_PAIR, result);
  set_carry_flag(cpu, double_carry_occurs(rp, hl, result));
}

void exchange_registers(I8080 *cpu) {
  uint32_t de = get_register_pair(cpu, D_PAIR);
  uint32_t hl = get_register_pair(cpu, H_PAIR);
  set_register_pair(cpu, D_PAIR, hl);
  set_register_pair(cpu, H_PAIR, de);
}

void load_pc(I8080 *cpu) {
  cpu->pc = get_register_pair(cpu, H_PAIR);
}

void load_sp(I8080 *cpu) {
  cpu->sp = get_register_pair(cpu, H_PAIR);
}

//   6 = 0b0110
//   9 = 0b1001 (max BCD value)
// 9+6 = 0b1111
void decimal_adjust_accumulator(I8080 *cpu) {
  uint8_t acc = get_register(cpu, A);

  uint8_t lsb = acc & 0xf;
  bool half_carry = lsb > 9 || get_auxiliary_carry_flag(cpu);
  if (half_carry) {
    acc += 0x06;
  }

  uint8_t msb = acc >> 4;
  bool carry = msb > 9 || get_carry_flag(cpu);
  if (carry) {
    acc += 0x60;
  }

  set_register(cpu, A, acc);

  set_sign_flag(cpu, acc);
  set_zero_flag(cpu, acc);
  set_parity_flag(cpu, acc);
  set_auxiliary_carry_flag(cpu, half_carry);
  set_carry_flag(cpu, carry);
}

void jump(I8080 *cpu, uint16_t address) {
  cpu->pc = address;
}

void jump_if_carry(I8080 *cpu, uint16_t address) {
  if (get_carry_flag(cpu)) {
    jump(cpu, address);
  }
}

void jump_if_no_carry(I8080 *cpu, uint16_t address) {
  if (!get_carry_flag(cpu)) {
    jump(cpu, address);
  }
}

void jump_if_zero(I8080 *cpu, uint16_t address) {
  if (get_zero_flag(cpu)) {
    jump(cpu, address);
  }
}

void jump_if_not_zero(I8080 *cpu, uint16_t address) {
  if (!get_zero_flag(cpu)) {
    jump(cpu, address);
  }
}

void jump_if_minus(I8080 *cpu, uint16_t address) {
  if (get_sign_flag(cpu)) {
    jump(cpu, address);
  }
}

void jump_if_positive(I8080 *cpu, uint16_t address) {
  if (!get_sign_flag(cpu)) {
    jump(cpu, address);
  }
}

void jump_if_parity_even(I8080 *cpu, uint16_t address) {
  if (get_parity_flag(cpu)) {
    jump(cpu, address);
  }
}

void jump_if_parity_odd(I8080 *cpu, uint16_t address) {
  if (get_parity_flag(cpu)) {
    jump(cpu, address);
  }
}

void stop(I8080 *cpu) {
  cpu->stopped = true;
}

bool is_stopped(I8080 *cpu) {
  return cpu->stopped;
}

void enable_interrupt(I8080 *cpu) {
  cpu->interrupt_enabled = true;
}

void disable_interrupt(I8080 *cpu) {
  cpu->interrupt_enabled = false;
}

void print_state_8080(I8080 *cpu) {
  for (int i = 0; i < REGISTER_COUNT; i++) {
    printf("%c|%02x", register_names[i], cpu->registers[i]);
    if (i % 2 == 0) {
       printf(" ");
    } else {
      printf("  ");
      if (i == 1) {
        printf("S Z - A - P - C");
      } else if (i == 3) {
        printf(
          "%d %d * %d * %d * %d",
          (int)get_sign_flag(cpu),
          (int)get_zero_flag(cpu),
          (int)get_auxiliary_carry_flag(cpu),
          (int)get_parity_flag(cpu),
          (int)get_carry_flag(cpu)
        );
      } else if (i == 5) {
        printf("SP|%04x  INTE|%d", cpu->sp, cpu->interrupt_enabled);
      } else if (i == 7) {
        printf("PC|%04x  HALT|%d", cpu->pc, cpu->stopped);
      }
      printf("\n");
    }
  }
}
