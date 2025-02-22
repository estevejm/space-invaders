#include <stdlib.h>

#ifndef I8080_H
#define I8080_H

#define REGISTER_COUNT 8

typedef struct i8080 {
    uint8_t data;
    uint16_t address;
    uint8_t registers[REGISTER_COUNT];
    uint16_t pc;
    uint16_t sp;
} I8080;

void print_state_8080(I8080 *cpu);

#endif //I8080_H
