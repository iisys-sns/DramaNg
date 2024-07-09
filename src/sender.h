#ifndef SENDER_H
#define SENDER_H

#include<stdint.h>

char *detectInitSignal(char *baseAddress, uint64_t len, uint64_t nMeasurements, uint64_t threshold);
void sendChars(char transmit[], uint64_t len, char *address, uint64_t nMeasurements, uint64_t rate, char debug, char vmMode);

#endif
