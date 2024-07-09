#include<stdio.h>
#include<time.h>

#include "helper.h"
#include "sender.h"
#include "asm.h"

char *detectInitSignal(char *baseAddress, uint64_t len, uint64_t nMeasurements, uint64_t threshold) {
    uint64_t stop = (rdtscp() / (1UL << 34) + 2) * (1UL << 34);
    uint64_t time = 0;
    uint64_t *conflicts = malloc(sizeof(uint64_t) * len);
    uint64_t *hits = malloc(sizeof(uint64_t) * len);

    uint64_t idx = 0;
    while(rdtscp() < stop) {
      time = measureSingleAddressAccessTime(baseAddress + idx, nMeasurements);
      if (time > threshold) {
        conflicts[idx] += 1;
      } else {
        hits[idx] += 1;
      }
      idx += 64;
      if(idx == len) {
        idx = 0;
      }
    }

    uint64_t maxConflictIdx = -1;
    uint64_t maxConflicts = 0;

    for(uint64_t i = 0; i < len; i++) {
      if(conflicts[i] > hits[i] && conflicts[i] > maxConflicts) {
        dprintf(2, "Setting maxConflicts to index %07ld (value: %ld)\n", i, conflicts[i]);
        maxConflicts = conflicts[i];
        maxConflictIdx = i;
      }
    }

    if(maxConflictIdx == -1) {
      return NULL;
    }

    dprintf(2, "Using offset %ld\n", maxConflictIdx);

    return baseAddress + maxConflictIdx;
}

void synchronizeClockSender(uint64_t len, char *address, uint64_t nMeasurements, uint64_t rate, char debug) {
  // Wait a little bit to assure that receiver is reading and not still sending
  // init signal
  uint64_t stop = (rdtscp() / (1UL << 34) + 2) * (1UL << 34);
  while(rdtscp() < stop) {
    asm volatile("nop" : : : );
  }

  // Send synchronization sequence of len bytes 10101010 (0xaa)
  stop = 0;
  for(uint64_t i = 0; i  < len * sizeof(char) * 8; i++) {
    stop = (rdtscp() / (rate) + 1) * (rate);
    while(rdtscp() < stop) {
      if(i % 2 == 1) {
          measureSingleAddressAccessTime(address, nMeasurements);
      } else {
        asm volatile("nop" : : : );
      }
    }
  }

  // Send 2 bytes zero afterwards to sync
  for(uint64_t i = 0; i  < 2 * sizeof(char) * 8; i++) {
    stop = (rdtscp() / (rate) + 1) * (rate);
    while(rdtscp() < stop) {
      asm volatile("nop" : : : );
    }
  }

  // Send final sequence to synchronize bit border
  for(uint64_t i = 0; i  < 1 * sizeof(char) * 8; i++) {
    stop = (rdtscp() / (rate) + 1) * (rate);
    while(rdtscp() < stop) {
      if(i % 2 == 1) {
          measureSingleAddressAccessTime(address, nMeasurements);
      } else {
        asm volatile("nop" : : : );
      }
    }
  }

  // Send an additional 0 (required for the sender to complete synchronization
  // during testing with R=32. TODO: Remove or adjust dynamically to transmission
  // rate
  for(uint64_t i = 0; i  < 1; i++) {
    stop = (rdtscp() / (rate) + 1) * (rate);
    while(rdtscp() < stop) {
      asm volatile("nop" : : : );
    }
  }
}

void sendChars(char transmit[], uint64_t len, char *address, uint64_t nMeasurements, uint64_t rate, char debug, char vmMode) {
  uint64_t stop = 0;
  uint64_t bit = 0;

  struct timespec start_time, stop_time;
  uint64_t start = (rdtscp() / (1UL << 32) + 2) * (1UL << 32);

  if (vmMode) {
    synchronizeClockSender(25, address, nMeasurements, rate, debug);
  } else {
    while(rdtscp() < start) {
      asm volatile("nop" : : : );
    }
  }


  dprintf(2, "Starting to transmit...\n");

  clock_gettime(CLOCK_MONOTONIC, &start_time);

  for(uint64_t i = 0; i  < len * sizeof(char) * 8; i++) {
    if(i % 8 == 0) {
      if(debug) {
        dprintf(2, "Sending byte: '%c' (0x%02x)\n", transmit[i/8], transmit[i/8]);
      }
    }
    bit = (transmit[i / 8] >> ((7 - i) % 8)) % 2;
    if(debug) {
      dprintf(2, "  Bit %1ld\n", bit);
    }
    //stop = (rdtscp() / (1UL << rate) + 1) * (1UL << rate);
    stop = (rdtscp() / (rate) + 1) * (rate);
    while(rdtscp() < stop) {
      if (bit == 1) {
        measureSingleAddressAccessTime(address, nMeasurements);
      } else {
        asm volatile("nop" : : : );
      }
    }
  }

  clock_gettime(CLOCK_MONOTONIC, &stop_time);

  uint64_t difference = (stop_time.tv_sec - start_time.tv_sec) * 1000000000 + stop_time.tv_nsec - start_time.tv_nsec;
  uint64_t bitrate = (8UL * 1000000000)/(difference / len);

  // Get millisecond precision for print
  uint64_t print_difference = difference / 1000000;

  dprintf(2, "Send %ld bytes in %ld.%03ld seconds (transmission rate: %ld bps).\n", len, print_difference / 1000, print_difference % 1000, bitrate);
}
