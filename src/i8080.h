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

typedef struct i8080 {
    uint8_t data;
    uint16_t address;
    uint8_t registers[REGISTER_COUNT];
    uint16_t pc;
    uint16_t sp;
} I8080;

void increment_register(I8080 *cpu, enum Register r);
void set_register(I8080 *cpu, enum Register r, uint8_t data);
void copy_register(I8080 *cpu, enum Register dst, enum Register src);
uint16_t get_m(I8080 *cpu);
bool is_plus_8080(I8080 *cpu);
bool is_zero_8080(I8080 *cpu);
bool is_parity_even_8080(I8080 *cpu);
void print_state_8080(I8080 *cpu);

#endif //I8080_H
