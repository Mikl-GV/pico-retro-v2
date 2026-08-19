#ifndef CPU6502_H
#define CPU6502_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t a, x, y, sp, flags;
    uint16_t pc;
    uint32_t cycles;
    bool irq_pending;
    bool nmi_pending;
    bool (*nmi_check)(void);
    uint8_t (*read)(uint16_t addr);
    void (*write)(uint16_t addr, uint8_t val);
} cpu6502_t;

enum {
    FLAG_C = 1 << 0,
    FLAG_Z = 1 << 1,
    FLAG_I = 1 << 2,
    FLAG_D = 1 << 3,
    FLAG_B = 1 << 4,
    FLAG_V = 1 << 6,
    FLAG_N = 1 << 7,
};

void cpu6502_init(cpu6502_t *cpu, uint8_t (*read)(uint16_t), void (*write)(uint16_t, uint8_t));
void cpu6502_reset(cpu6502_t *cpu);
uint32_t cpu6502_step(cpu6502_t *cpu);
void cpu6502_nmi(cpu6502_t *cpu);

extern const uint8_t cpu6502_cycles[256];

#endif