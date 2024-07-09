#ifndef RECEIVER_H
#define RECEIVER_H

typedef struct {
  char *virtualAddress;
  uint64_t bankNumber;
  uint64_t nMeasurements;
  char debug;
  char *exportFile;
  uint64_t bufferSize;
  uint64_t threshold;
} ThreadData;

#define NEW  0
#define VALID_OFFSET_START  1
#define VALID_OFFSET_END  2
#define ZERO_PADDING  3

void initReceiver(char *exportFile);
void cleanupReceiver();
void sendInitSignal(char *address, uint64_t nMeasurements);
void receiveParallel(char **virtualAddresses, uint64_t nAddresses, uint64_t nMeasurements, char debug, char *exportFile, uint64_t *linearAddressingFunctions, uint64_t nLinearAddressingFunctions, uint64_t bufferSize, uint64_t threshold);
void receive(char *address, uint64_t nMeasurements, uint64_t threshold, uint64_t rate, char debug, uint64_t bufferSize, char vmMode);

#endif
