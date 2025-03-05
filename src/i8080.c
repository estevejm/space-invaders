#include <stdio.h>
#include <string.h>
#include "i8080.h"

const char register_names[] = {
  'A', 'F',
  'B', 'C',
  'D', 'E',
  'H', 'L',
};

void increment_register(I8080 *cpu, enum Register r) {
  cpu->registers[r]++;
}

void set_register(I8080 *cpu, enum Register r, uint8_t data) {
  cpu->registers[r] = data;
}

void copy_register(I8080 *cpu, enum Register dst, enum Register src) {
  cpu->registers[dst] = cpu->registers[src];
}

uint16_t get_m(I8080 *cpu) {
	return cpu->registers[H] << 8 | cpu->registers[L];
}

bool is_plus_8080(I8080 *cpu) {
  return ~(cpu->registers[F] >> 7 & 1);
}

bool is_zero_8080(I8080 *cpu) {
  return ~(cpu->registers[F] >> 6 & 1);
}

bool is_parity_even_8080(I8080 *cpu) {
  return cpu->registers[F] >> 2 & 1;
}

void print_state_8080(I8080 *cpu) {
    printf("data: %02x\n", cpu->data);
    printf("addr: %04x\n", cpu->address);
    printf("pc  : %04x\n", cpu->pc);
    printf("sp  : %04x\n", cpu->sp);

    printf("ra:");
    for (int i = 0; i < REGISTER_COUNT; i++) {
        printf(" %c=%02x", register_names[i], cpu->registers[i]);
    }
    printf("\n");
}
