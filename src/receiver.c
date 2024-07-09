#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<errno.h>
#include<ftw.h>
#include<pthread.h>

#include "helper.h"
#include "receiver.h"
#include "asm.h"

//int fd = 0;

void initReceiver(char *exportFile) {
  //fd = open(exportFile, O_WRONLY|O_TRUNC|O_CREAT, 0600);
  //if(fd == -1) {
  //  dprintf(2, "Unable to open %s. Error: %s\n", exportFile, strerror(errno));
  //  exit(-1);
  //}
}

void cleanupReceiver() {
  //fsync(fd);
  //close(fd);
}

void sendInitSignal(char *address, uint64_t nMeasurements) {
  uint64_t stop = (rdtscp() / (1UL << 34) + 2) * (1UL << 34);
  while(rdtscp() < stop) {
    measureSingleAddressAccessTime(address, nMeasurements);
  }
}

void *measureSingleBank(void *args) {
  ThreadData *threadData = (ThreadData *)args;
  uint64_t *timeBuffer = malloc(sizeof(uint64_t) * threadData->bufferSize);
  uint64_t *latencyBuffer = malloc(sizeof(uint64_t) * threadData->bufferSize);
  uint64_t cursor = 0;

  char *address = threadData->virtualAddress;
  uint64_t nMeasurements = threadData->nMeasurements;
  uint64_t bufferSize = threadData->bufferSize;
  uint64_t threshold = threadData -> threshold;
  uint64_t latency = 0;

  uint64_t start = rdtscp();

  for(uint64_t i = 0; i < bufferSize; i++) {
    latency = measureSingleAddressAccessTime(address, nMeasurements);
    if(latency > threshold) {
      timeBuffer[cursor] = rdtscp() - start;
      latencyBuffer[cursor] = latency;
      cursor++;
    }
  }

  int fd = open(threadData->exportFile, O_APPEND|O_WRONLY|O_CREAT, 0600);
  if(fd == -1) {
    dprintf(2, "Unable to open file %s. Error: %s\n", threadData->exportFile, strerror(errno));
    return NULL;
  }

  dprintf(fd, "\n");
  for(uint64_t i = 0; i < cursor; i++) {
    dprintf(fd, "%ld %ld\n", timeBuffer[i], latencyBuffer[i]);
  }

  fsync(fd);

  free(timeBuffer);
  free(latencyBuffer);
  close(fd);
  free(threadData->exportFile);
  free(threadData);

  return NULL;
}

/*int removeFiles(const char *fpath, const struct stat *sb, int typeflag) {
  if(typeflag != FTW_F) {
    return 0;
  }
  remove(fpath);
  return 0;
}*/


void receiveParallel(char **virtualAddresses, uint64_t nAddresses, uint64_t nMeasurements, char debug, char *exportFile, uint64_t *linearAddressingFunctions, uint64_t nLinearAddressingFunctions, uint64_t bufferSize, uint64_t threshold) {
  pthread_t *threads = malloc(sizeof(pthread_t) * nAddresses);

  // Don't cleanup automatically (allow multiple measurements)
  //if(exportFile[strlen(exportFile) - 1] == '/') {
  //  exportFile[strlen(exportFile) - 1] = '\0';
  //}
  //if(ftw(exportFile, removeFiles, 100) != 0) {
  //  dprintf(2, "Unable to cleanup directory %s. Error: %s\n", exportFile, strerror(errno));
  //}

  //rmdir(exportFile);
  if(mkdir(exportFile, 0700) == -1 && errno != EEXIST) {
    dprintf(2, "Unable to create directory %s. Error: %s\n", exportFile, strerror(errno));
    exit(-1);
  }

  for(uint64_t i = 0; i < nAddresses; i++) {
    ThreadData *threadData = malloc(sizeof(ThreadData));
    threadData->virtualAddress = virtualAddresses[i];
    threadData->bankNumber = getBankNumber(virtualAddresses[i], linearAddressingFunctions, nLinearAddressingFunctions);
    threadData->nMeasurements = nMeasurements;
    threadData->debug = debug;
    threadData->exportFile = malloc(sizeof(char) * 1024);
    snprintf(threadData->exportFile, 1024, "%s/%02ld.dat", exportFile, threadData->bankNumber);
    threadData->bufferSize = bufferSize;
    threadData->threshold = threshold;
    pthread_create(&threads[i], NULL, measureSingleBank, threadData);
  }

  for(uint64_t i = 0; i < nAddresses; i++) {
    pthread_join(threads[i], NULL);
  }

  free(threads);
}

uint64_t synchronizeClockReceiver(char *address, uint64_t nMeasurements, uint64_t threshold, uint64_t rate, char debug) {
  uint64_t stop = 0;
  uint64_t conflicts = 0;
  uint64_t hits = 0;

  uint64_t bit = 0;
  uint64_t time = 0;

  uint8_t byte = 0;
  uint8_t nBitsInByte = 0;

  uint64_t offset = 0;
  uint64_t validOffsetStart = 0;
  uint64_t validOffsetEnd = 0;
  uint64_t n_zero = 0;
  uint8_t state = NEW;

  while(1) {
    stop = ((rdtscp() - offset) / (rate) + 1) * (rate);
    conflicts = 0;
    hits = 0;

    while((rdtscp() - offset) < stop) {
      time = measureSingleAddressAccessTime(address, nMeasurements);
      if (time > threshold) {
        conflicts += 1;
      } else {
        hits += 1;
      }
    }

    bit = 0;
    if (conflicts > (75 * (conflicts + hits) / 100)) {
      bit = 1;
      n_zero = 0;
      if(debug) {
        dprintf(2, "C: h=%4ld c=%4ld (%ld)\n", hits, conflicts, bit);
      }
    } else {
      n_zero += 1;
      if (debug){
        dprintf(2, "H: h=%4ld c=%4ld (%ld); n_zero=%ld\n", hits, conflicts, bit, n_zero);
      }
    }

    byte *= 2;
    byte += bit;
    nBitsInByte += 1;

    if(state == NEW && nBitsInByte == 8) {
      if(byte == 0xaa || byte == 0x55) {
        state = VALID_OFFSET_START;
        validOffsetStart = offset;
        if(debug) {
          dprintf(2, "Changed state to VALID_OFFSET_START.\n");
        }
      } else {
        // Shift rdtsc offset 5% of the rate
        offset += rate * 5 / 100;
        if (offset >= rate) {
          offset = 0;
        }
      }

      byte = 0;
      nBitsInByte = 0;
    }

    if(state == VALID_OFFSET_START && nBitsInByte == 8) {
      if(!(byte == 0xaa || byte == 0x55) || offset >= rate) {
        state = VALID_OFFSET_END;
        validOffsetEnd = offset;
        offset = (validOffsetEnd + validOffsetStart) / 2;
        if(debug) {
          dprintf(2, "Changed state to VALID_OFFSET_END.\n");
        }
      } else {
        // Shift rdtsc offset 5% of the rate
        offset += rate * 5 / 100;
      }

      byte = 0;
      nBitsInByte = 0;
    }

    if (state == VALID_OFFSET_END && n_zero == 17) {
      nBitsInByte = 0;
      byte = 0;
      if(debug) {
        dprintf(2, "Changed state to ZERO_PADDING.\n");
      }
      state = ZERO_PADDING;
    }


    if (state == ZERO_PADDING && nBitsInByte == 8) {
      if(debug) {
        dprintf(2, "[DEBUG]: state = %d, nBitsInByte = %d, byte = 0x%hhx\n", state, nBitsInByte, byte);
      }
      if(byte == 0xaa) {
        if(debug) {
          dprintf(2, "Using offset: %ld\n", offset);
        }
        return offset;
      }
    }

    if(nBitsInByte == 8) {
      nBitsInByte = 0;
    }
  }
}

void receive(char *address, uint64_t nMeasurements, uint64_t threshold, uint64_t rate, char debug, uint64_t bufferSize, char vmMode) {
  uint64_t start = (rdtscp() / (1UL << 32) + 2) * (1UL << 32);

  char *buffer = malloc(sizeof(char) * bufferSize);
  uint64_t bufCur = 0;

  uint64_t conflicts = 0;
  uint64_t hits = 0;
  //uint64_t cnt = 0;

  uint64_t stop = 0;
  uint64_t bit = 0;
  uint64_t time = 0;
  uint64_t clockOffset = 0;

  char byte = 0;
  char nBitsInByte = 0;

  if (vmMode) {
    clockOffset = synchronizeClockReceiver(address, nMeasurements, threshold, rate, debug);
  } else {
    while((rdtscp()) < start) {
      asm volatile("nop" : : : );
    }
  }

  dprintf(2, "Starting to receive...\n");

  while(bufCur < bufferSize) {
    //stop = ((rdtscp() - clockOffset) / (1UL << rate) + 1) * (1UL << rate);
    stop = ((rdtscp() - clockOffset) / (rate) + 1) * (rate);

    conflicts = 0;
    hits = 0;
    //dprintf(fd, "\n");
    
    while((rdtscp() - clockOffset) < stop) {
      time = measureSingleAddressAccessTime(address, nMeasurements);
      if (time > threshold) {
        conflicts += 1;
      } else {
        hits += 1;
      }

      //dprintf(fd, "%ld %ld\n", cnt++, time);
    }

    bit = 0;
    if (conflicts > (75 * (conflicts + hits) / 100)) {
      bit = 1;
      if(debug) {
        dprintf(2, "C: h=%4ld c=%4ld (%ld)\n", hits, conflicts, bit);
      }
    } else if (debug){
      dprintf(2, "H: h=%4ld c=%4ld (%ld)\n", hits, conflicts, bit);
    }

    byte *= 2;
    byte += bit;
    nBitsInByte += 1;

    if(nBitsInByte == 8) {
      buffer[bufCur++] = byte;
      if(debug) {
        dprintf(2, "0x%x (%c)\n", byte, byte);
      }
      byte = 0;
      nBitsInByte = 0;
    }
  }
  for(uint64_t i = 0; i < bufferSize; i++) {
    printf("%c", buffer[i]);
  }
  fflush(stdout);
  dprintf(2, "Done.\n");
  fflush(stderr);
}
