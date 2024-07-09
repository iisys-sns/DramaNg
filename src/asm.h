#ifndef ASM_H
#define ASM_H

#include<stdlib.h>
#include<stdint.h>

static inline void clflushOrig(volatile void *addr) {
    asm volatile("clflush (%0)" : : "r" (addr) : "memory");
}

static inline void clflushOpt(volatile void *addr) {
    asm volatile("clflushopt (%0)" : : "r" (addr) : "memory");
}

static inline void mfence() {
    asm volatile("mfence" : : : "memory");
}

static inline void mfenceDummy() {
    asm volatile("nop" : : : );
}


static inline void lfence() {
    asm volatile("lfence" : : : "memory");
}

static inline uint64_t rdtscp() {
    int64_t a, c, d;
    asm volatile("rdtscp" : "=a"(a), "=d"(d), "=c"(c) : : );
    return (d<<32)|a;
}

#endif
