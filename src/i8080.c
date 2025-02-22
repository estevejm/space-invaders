#include <stdio.h>
#include <string.h>
#include "i8080.h"

void print_state_8080(I8080 *cpu) {
    printf("data: %02x\n", cpu->data);
    printf("addr: %04x\n", cpu->address);
    printf("pc  : %04x\n", cpu->pc);
    printf("sp  : %04x\n", cpu->sp);

    printf("regs:");
    for (int i = 0; i < REGISTER_COUNT; i++) {
        printf(" %d=%02x", i, cpu->registers[i]);
    }
    printf("\n");
}
