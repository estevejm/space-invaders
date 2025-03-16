#include <stdio.h>
#include "i8080.h"

#define CARRY_FLAG_POS 1
#define PARITY_FLAG_POS 2
#define AUX_CARRY_FLAG_POS 4
#define ZERO_FLAG_POS 6
#define SIGN_FLAG_POS 7

const char register_names[] = {
  'A', 'F',
  'B', 'C',
  'D', 'E',
  'H', 'L',
};

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
  printf("MOV %c,%c", register_names[dst], register_names[src]);
  cpu->registers[dst] = cpu->registers[src];
}

bool nth_carry_occurs(uint32_t a, uint32_t b, uint32_t result, int n) {
  return (a ^ b ^ result) >> n & 1;
}

bool half_carry_occurs(int8_t a, uint8_t b, uint8_t result) {
  return nth_carry_occurs(a, b, result, 4);
}

bool carry_occurs(int16_t a, uint16_t b, uint16_t result) {
  return nth_carry_occurs(a, b, result, 8);
}

bool double_carry_occurs(uint32_t a, uint32_t b, uint32_t result) {
  return nth_carry_occurs(a, b, result, 16);
}

void increment_register(I8080 *cpu, enum Register r) {
  printf("INR %c", register_names[r]);

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
  printf("DCR %c", register_names[r]);

  uint8_t a = cpu->registers[r];
  uint8_t b = 1;
  uint8_t result = a - b; // TODO: use addition always? result = a + (-b) so we can have a uniform (aux) carry flag check?

  set_sign_flag(cpu, result);
  set_zero_flag(cpu, result);
  set_parity_flag(cpu, result);
  set_auxiliary_carry_flag(cpu, !half_carry_occurs(a, b, result));

  cpu->registers[r] = result;
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
  uint32_t bc = get_register_pair(cpu, r);
  uint32_t hl = get_register_pair(cpu, H_PAIR);
  uint32_t result = bc + hl;
  set_register_pair(cpu, H_PAIR, result);
  set_carry_flag(cpu, double_carry_occurs(bc, hl, result));
}

void compare_immediate_accumulator(I8080 *cpu, uint8_t value) {
  uint8_t a = cpu->registers[A];
  uint8_t b = -value;
  uint16_t result = a + b;

  set_sign_flag(cpu, result);
  set_zero_flag(cpu, result);
  set_parity_flag(cpu, result);
  set_auxiliary_carry_flag(cpu, !half_carry_occurs(a, b, result));
  set_carry_flag(cpu, !carry_occurs(a, b, result));
}

//   6 = 0b0110
//   9 = 0b1001 (max BCD value)
// 9+6 = 0b1111
void decimal_adjust_accumulator(I8080 *cpu) {
  printf("DAA");

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

void restart(I8080 *cpu, uint8_t value) {
  uint8_t bounded = value % 8;
  // TODO: push current PC to stack to be used by return
  cpu->pc = bounded << 3;
}

void no_operation(I8080 *cpu) {
  printf("NOP");
}

void halt(I8080 *cpu) {
  printf("HLT");
  cpu->halted = true;
}

bool is_halted(I8080 *cpu) {
  return cpu->halted;
}

void jump(I8080 *cpu, uint16_t address) {
  cpu->pc = address;
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

void jump_if_parity_even(I8080 *cpu, uint16_t address) {
  if (get_parity_flag(cpu)) {
    jump(cpu, address);
  }
}

void jump_if_no_carry(I8080 *cpu, uint16_t address) {
  if (!get_carry_flag(cpu)) {
    jump(cpu, address);
  }
}

void subroutine_call(I8080 *cpu, uint16_t address) {
  // TODO: push current PC to stack to be used in return
  jump(cpu, address);
}

void subroutine_call_if_zero(I8080 *cpu, uint16_t address) {
  if (get_zero_flag(cpu)) {
    subroutine_call(cpu, address);
  }
}

void subroutine_call_if_plus(I8080 *cpu, uint16_t address) {
  if (!get_sign_flag(cpu)) {
    subroutine_call(cpu, address);
  }
}

void subroutine_return(I8080 *cpu) {
  // TODO: pop PC from stack
  cpu->pc = 0;
}

void subroutine_return_if_plus(I8080 *cpu) {
  if (!get_sign_flag(cpu)) {
    subroutine_return(cpu);
  }
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

  printf("S Z A P C\n");
  printf(
      "%d %d %d %d %d\n",
      (int)get_sign_flag(cpu),
      (int)get_zero_flag(cpu),
      (int)get_auxiliary_carry_flag(cpu),
      (int)get_parity_flag(cpu),
      (int)get_carry_flag(cpu)
  );
}
