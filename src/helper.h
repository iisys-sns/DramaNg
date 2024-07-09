#ifndef HELPER_H
#define HELPER_H

#include<stdint.h>


void yield();
int compareUInt64(const void *a1, const void *a2);
void setClflush(void(*func)(volatile void *addr));
uint64_t getBankNumber(void *virtualAddress, uint64_t *linearAddressingFunctions, uint64_t nLinearAddressingFunctions);
uint64_t measureSingleAddressAccessTime(void *a1, uint64_t nMeasurements);
uint64_t measureAccessTime(void *a1, void *a2, uint64_t nMeasurements, int fenced);
void *getMemory(uint64_t nBytes);
void freeMemory(void *memory);
void *getTHP();
void freeTHP(void *thp);
uint64_t *getRandomIndices(uint64_t *number, uint64_t len);
char isNumberPowerOfTwo(uint64_t number);

static inline uint64_t xorBits(uint64_t x) {
    int sum = 0;
    while(x != 0) {
        //x&-x has a binary one where the lest significant one
        //in x was before. By applying XOR to this, the last
        //one becomes a zero.
        //
        //So, this overwrites all ones in x and toggles sum
        //every time until there are no ones left.
        sum^=1;
        x ^= (x&-x);
    }
    return sum;
}

#endif
