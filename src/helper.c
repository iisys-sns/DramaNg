#include<stdio.h>
#include<errno.h>
#include<fcntl.h>
#include<string.h>
#include<unistd.h>
#include<sys/mman.h>
#include<sched.h>

#include "asm.h"
#include "helper.h"

void(*clflush)(volatile void *addr) = NULL;

void yield() {
  for(uint64_t i = 0; i < 20; i++) {
    sched_yield();
  }
}

int compareUInt64(const void *a1, const void *a2) {
	return *(uint64_t*)(a1) - *(uint64_t*)(a2);
}

void setClflush(void(*func)(volatile void *addr)) {
  clflush = func;
}

uint64_t readFileAtOffset(const char filePath[], uint64_t offset) {
  int fd = open(filePath, O_RDONLY);
  if(fd == -1) {
    dprintf(2, "Unable to open '%s'. Error: %s\n", filePath, strerror(errno));
    return 0;
  }

  off_t sOff = lseek(fd, offset, SEEK_SET);
  if(sOff == -1) {
    dprintf(2, "Unable to lseek offset %ld in file '%s'. Error: %s\n", offset, filePath, strerror(errno));
    close(fd);
    return 0;
  }

  uint64_t retVal = 0;
  ssize_t nBytesRead = read(fd, &retVal, sizeof(uint64_t));
  if(nBytesRead == -1) {
    dprintf(2, "Unable to read 8 bytes at offset %ld in file '%s'. Error: %s\n", offset, filePath, strerror(errno));
    close(fd);
    return 0;
  }

  close(fd);
  
  return retVal;
}

void *getPhysicalAddressForVirtualAddress(void *page) {
  uint64_t offset = ((uint64_t)(page) / sysconf(_SC_PAGESIZE)) * sizeof(uint64_t);
  uint64_t pagemap = readFileAtOffset("/proc/self/pagemap", offset);
  uint64_t pfn = pagemap & ((1L<<54) - 1);
  uint64_t physicalBaseAddress = pfn * sysconf(_SC_PAGESIZE);
  uint64_t physicalAddress = physicalBaseAddress | ((uint64_t)page & ((sysconf(_SC_PAGESIZE))-1));
  return (void *)(physicalAddress);
}

uint64_t getBankNumber(void *virtualAddress, uint64_t *linearAddressingFunctions, uint64_t nLinearAddressingFunctions) {
  uint64_t physicalAddress = (uint64_t)getPhysicalAddressForVirtualAddress(virtualAddress);
  uint64_t bank = 0;
  for(uint64_t i = 0; i < nLinearAddressingFunctions; i++) {
    bank *= 2;
    bank += xorBits(physicalAddress & linearAddressingFunctions[i]);
  }
  return bank;
}

uint64_t measureSingleAddressAccessTime(void *a1, uint64_t nMeasurements) {
	uint64_t start = rdtscp();

	for(uint64_t i = 0; i < nMeasurements; i++) {
    clflush(a1);
		//*(volatile char *)a1 = 0x2a;
		*(volatile char *)a1;
    mfence();
	}

	return (rdtscp() - start) / nMeasurements;
}

uint64_t measureAccessTime(void *a1, void *a2, uint64_t nMeasurements, int fenced) {
  void(*mFenceFunc)() = mfenceDummy;
  if(fenced) {
    mFenceFunc = mfence;
  }

	uint64_t start = rdtscp();

	for(uint64_t i = 0; i < nMeasurements; i++) {
    clflush(a1);
    clflush(a2);
		*(volatile char *)a1;
    //xchg(a1);
		*(volatile char *)a2;
    //xchg(a2);
    mFenceFunc();
	}

	return (rdtscp() - start) / nMeasurements;
}

void *getTHP() {
	void *mapping = NULL;
	if(posix_memalign(&mapping, 512 * sysconf(_SC_PAGESIZE), 512 * sysconf(_SC_PAGESIZE)) != 0) {
		dprintf(2, "Unable to map memory. Error: %s\n", strerror(errno));
		return NULL;
	}

	if(madvise(mapping, 512 * sysconf(_SC_PAGESIZE), MADV_HUGEPAGE) != 0) {
		dprintf(2, "Unable to madvise. Error: %s\n", strerror(errno));
		return NULL;
	}

	for(uint64_t i = 0; i < 512; i++) {
		*(volatile char *)((volatile char *)mapping + i * sysconf(_SC_PAGESIZE)) = 0x2a;
	}
  return mapping;
}

void freeTHP(void *thp) {
	free(thp);
}

uint64_t *getRandomIndices(uint64_t *number, uint64_t len) {
  uint64_t *indices = malloc(sizeof(uint64_t) * *number);

  if(len <= *number) {
    *number = len;
    for(uint64_t i = 0; i < len; i++) {
      indices[i] = i;
    }
    return indices;
  }

  uint64_t found = 0;
  char indexInList = 0;
  while(found < *number) {
    uint64_t candidate = rand() % len;
    indexInList = 0;
    for(uint64_t x = 0; x < found; x++) {
      if(indices[x] == candidate) {
        indexInList = 1;
        break;
      }
    }
    if(indexInList == 1) {
      continue;
    }

    indices[found++] = candidate;
  }

  return indices;
}

char isNumberPowerOfTwo(uint64_t number) {
  uint64_t nBitsSet = 0;
  for(uint64_t i = 0; i < sizeof(uint64_t) * 8; i++) {
    if(number>>i&1) {
      nBitsSet += 1;
    }
  }

  if(nBitsSet == 1) {
    return 1;
  }
  return 0;
}
