#ifndef CONFIG_H
#define CONFIG_H

#include<stdint.h>

#define STYLE_RESET "\e[0m"
#define STYLE_BOLD "\e[1m"
#define STYLE_UNDERLINE "\e[4m"

typedef struct {
  char debug;
  void(*clflush)(volatile void *);
  uint64_t threshold;
  uint64_t groupingThreshold;
  uint64_t thps;
  char *sendStr;
  uint64_t sendLen;
  char isSender;
  uint64_t rate;
  uint64_t initialBlockSize;
  uint64_t maxRetriesForGrouping;
  uint64_t nMeasurements;
  char fenced;
  char *parallel;
  uint64_t *linearAddressingFunctions;
  uint64_t nLinearAddressingFunctions;
  uint64_t bufferSize;
  char vmMode;
} Config;

uint64_t handleNumericValue(char *value , const char *name);
Config *getDefaultConfig();
Config *getConfig(int argc, char *argv[]);
void printHelpPage(int exitCode);

#endif
