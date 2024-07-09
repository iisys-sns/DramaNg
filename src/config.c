#include<stdio.h>
#include<stdlib.h>
#include<getopt.h>
#include<string.h>
#include<stdint.h>
#include<unistd.h>
#include<errno.h>
#include<fcntl.h>

#include "config.h"
#include "asm.h"

uint64_t handleNumericValue(char *value , const char *name) {
  uint64_t v = atoi(value);
  if(v == 0) {
    dprintf(2, "Value %s is invalid for parameter %s\n\n", value, name);
    printHelpPage(EXIT_FAILURE);
  }
  return v;
}

Config *getDefaultConfig() {
  Config *config = malloc(sizeof(Config));
  config->debug = 0;
  config->clflush = clflushOpt;
  config->groupingThreshold = 100;
  config->threshold = 100;
  config->thps = 1;
  config->sendStr = NULL;
  config->sendLen = 0;
  config->isSender = 0;
  config->rate = 32;
  config->initialBlockSize = 4096;
  config->maxRetriesForGrouping = 1;
  config->nMeasurements = 3;
  config->fenced = 1;
  config->parallel = NULL;
  config->nLinearAddressingFunctions = 0;
  config->linearAddressingFunctions = NULL;
  config->bufferSize = 1000;
  config->vmMode = 0;

  return config;
}

uint64_t parseHexNumber(char *number) {
  uint64_t value = 0;
  if(number[0] != '0' && number[1] != 'x') {
    dprintf(2, "Error: Hex number has to start with '0x' (was %s).\n", number);
  }
  for (uint64_t i = 2; number[i] != '\0'; i++) {
    value *= 16;
    if(number[i] >= '0' && number[i] <= '9') {
      value += number[i] - '0';
    }
    if(number[i] >= 'a' && number[i] <= 'f') {
      value += number[i] - 'a' + 10;
    }
    if(number[i] >= 'A' && number[i] <= 'F') {
      value += number[i] - 'A' + 10;
    }
  }

  return value;
}

void addLinearAddressingFunction(Config *config, char *function) {
  config->nLinearAddressingFunctions += 1;
  config->linearAddressingFunctions = realloc(config->linearAddressingFunctions, config->nLinearAddressingFunctions * sizeof(uint64_t));
  config->linearAddressingFunctions[config->nLinearAddressingFunctions - 1] = parseHexNumber(function);
}

Config *getConfig(int argc, char *argv[]) {
  Config *config = getDefaultConfig();

  opterr = 0;
  static struct option long_options[] = {
    {"help", no_argument, 0, 'h'},
    {"debug", no_argument, 0, 'd'},
    {"memory-type", required_argument, 0, 'g'},
    {"threshold", required_argument, 0, 't'},
    {"grouping-threshold", required_argument, 0, 'T'},
    {"thps", required_argument, 0, 'p'},
    {"sender", required_argument, 0, 's'},
    {"sender-file", required_argument, 0, 'S'},
    {"rate", required_argument, 0, 'r'},
    {"exp-rate", required_argument, 0, 'R'},
    {"initial-block-size", required_argument, 0, 'b'},
    {"max-grouping-retries", required_argument, 0, 'e'},
    {"measurements", required_argument, 0, 'm'},
    {"no-fences", no_argument, 0, 'f'},
    {"parallel", required_argument, 0, 'a'},
    {"linear-addressing-functions", required_argument, 0, 'l'},
    {"buffer-size", required_argument, 0, 'z'},
    {"vm", no_argument, 0, 'v'},
    {0, 0, 0, 0}
  };

  int c = 0;
  int option_index = 0;
  while(1) {
    c = getopt_long(argc, argv, "dg:t:T:p:s:S:r:R:a:b:e:m:fa:l:z:v", long_options, &option_index);
    if(c == -1) {
      break;
    }

    switch(c) {
      case 'h':
        printHelpPage(EXIT_SUCCESS);
        break;
      case 'd':
        config->debug = 1;
        break;
      case 'g':
        uint64_t len = strlen(optarg) < strlen("ddr3") ? strlen(optarg) : strlen("ddr3");
        if(strncmp(optarg, "ddr3", len) == 0) {
          config->clflush = clflushOrig;
        } else if(strncmp(optarg, "ddr4", len) == 0) {
          config->clflush = clflushOpt;
        } else if(strncmp(optarg, "ddr5", len) == 0) {
          config->clflush = clflushOpt;
        } else {
          dprintf(2, "DRAM type '%s' not supported.", optarg);
          exit(-1);
        }
        break;
      case 't':
        config->threshold = handleNumericValue(optarg, long_options[option_index].name);
        break;
      case 'T':
        config->groupingThreshold = handleNumericValue(optarg, long_options[option_index].name);
        break;
      case 'p':
        config->thps = handleNumericValue(optarg, long_options[option_index].name);
        break;
      case 's':
        config->sendStr = optarg;
        config->sendLen = strlen(config->sendStr);
        config->isSender = 1;
        break;
      case 'S':
        int fd = open(optarg, O_RDONLY, 0600);
        if(fd == -1) {
          dprintf(2, "Unable to open file %s. Error: %s\n", optarg, strerror(errno));
          break;
        }
        uint64_t bufIncrease = 1000;
        uint64_t bufSize = bufIncrease;
        char *buffer = malloc(sizeof(char) * bufSize);
        uint64_t bufCur = 0;
        uint64_t bytesRead = 0;
        while((bytesRead = read(fd, buffer + bufCur, bufSize - bufCur)) > 0) {
          bufSize += bufIncrease;
          bufCur += bytesRead;
          buffer = realloc(buffer, sizeof(char) * bufSize);
        }
        config->sendStr = buffer;
        config->sendLen = bufCur;
        config->isSender = 1;
        break;
      case 'r':
        config->rate = handleNumericValue(optarg, long_options[option_index].name);
        break;
      case 'R':
        config->rate = (1UL<<handleNumericValue(optarg, long_options[option_index].name));
        break;
      case 'b':
        config->initialBlockSize = handleNumericValue(optarg, long_options[option_index].name);
        break;
      case 'e':
        config->maxRetriesForGrouping = handleNumericValue(optarg, long_options[option_index].name);
        break;
      case 'm':
        config->nMeasurements = handleNumericValue(optarg, long_options[option_index].name);
        break;
      case 'f':
        config->fenced = 0;
        break;
      case 'a':
        config->parallel = optarg;
        break;
      case 'l':
        char *function = strtok(optarg, ",");
        addLinearAddressingFunction(config, function);
        while((function = strtok(NULL, ",")) != NULL) {
          addLinearAddressingFunction(config, function);
        }
        break;
      case 'z':
        config->bufferSize = handleNumericValue(optarg, long_options[option_index].name);
        break;
      case 'v':
        config->vmMode = 1;
        break;
      case '?':
      default:
        dprintf(2, "Invalid option %d\n", c);
        printHelpPage(EXIT_FAILURE);
    }
  }

  return config;
}

void printHelpPage(int exitCode) {
  Config *defaultConfig = getDefaultConfig();

  printf("DRAMA-NG(1)\n");
  printf("%sNAME%s\n", STYLE_BOLD, STYLE_RESET);
  printf("  drama-ng - New implementation of the Drama Sidechannel originally published by Pessl et al.\n");
  printf("%sSYNOPSIS%s\n", STYLE_BOLD, STYLE_RESET);
  printf("  drama-ng [%sOPTION%s]...\n\n", STYLE_UNDERLINE, STYLE_RESET);
  printf("%sDESCRIPTION%s\n", STYLE_BOLD, STYLE_RESET);
  printf("  %s-h%s, %s--help%s\n", STYLE_BOLD, STYLE_RESET, STYLE_BOLD, STYLE_RESET);
  printf("    Show this help message and exit\n");
  printf("  %s-d%s, %s--debug%s\n", STYLE_BOLD, STYLE_RESET, STYLE_BOLD, STYLE_RESET);
  printf("    Enable additional debugging output\n");
  printf("  %s-g%s, %s--memory-type%s=%sTYPE%s\n", STYLE_BOLD, STYLE_RESET, STYLE_BOLD, STYLE_RESET, STYLE_UNDERLINE, STYLE_RESET);
  printf("    TYPE of the memory that is used; this specifies if clflush() or clflushopt()\n");
  printf("    is used; can be set to 'ddr3', 'ddr4', 'ddr5' (default: 'ddr4')\n");
  printf("  %s-t%s, %s--threshold%s=%sNUMBER%s\n", STYLE_BOLD, STYLE_RESET, STYLE_BOLD, STYLE_RESET, STYLE_UNDERLINE, STYLE_RESET);
  printf("    NUMBER of rdtsc ticks to distinguish between row hit and row conflict during measurement\n");
  printf("  %s-T%s, %s--grouping-threshold%s=%sNUMBER%s\n", STYLE_BOLD, STYLE_RESET, STYLE_BOLD, STYLE_RESET, STYLE_UNDERLINE, STYLE_RESET);
  printf("    NUMBER of rdtsc ticks to distinguish between row hit and row conflict during grouping\n");
  printf("    (use drama-verify to measure the threshold)\n");
  printf("  %s-p%s, %s--thps%s=%sNUMBER%s\n", STYLE_BOLD, STYLE_RESET, STYLE_BOLD, STYLE_RESET, STYLE_UNDERLINE, STYLE_RESET);
  printf("    NUMBER of 2M transparent hugepages that is allocated for the receiver\n");
  printf("    (default: %ld).\n", defaultConfig->thps);
  printf("  %s-s%s, %s--sender%s=%sSTRING%s\n", STYLE_BOLD, STYLE_RESET, STYLE_BOLD, STYLE_RESET, STYLE_UNDERLINE, STYLE_RESET);
  printf("    STRING that should be transmitted. If this option is not specified, drama-ng\n");
  printf("    runs in 'receive' mode (default: not set and drama-verify runs in receive mode).\n");
  printf("  %s-S%s, %s--sender-file%s=%sFILE%s\n", STYLE_BOLD, STYLE_RESET, STYLE_BOLD, STYLE_RESET, STYLE_UNDERLINE, STYLE_RESET);
  printf("    FILE to read content that should be transmitted. This option can be used instead of '-s'\n");
  printf("    to send the content of a file rather than specifying the string as command-line parameter..\n");
  printf("  %s-r%s, %s--rate%s=%sNUMBER%s\n", STYLE_BOLD, STYLE_RESET, STYLE_BOLD, STYLE_RESET, STYLE_UNDERLINE, STYLE_RESET);
  printf("    NUMBER of rdtsc ticks that sending or receiving a single byte takes (default: %ld).\n", defaultConfig->rate);
  printf("  %s-R%s, %s--exp-rate%s=%sNUMBER%s\n", STYLE_BOLD, STYLE_RESET, STYLE_BOLD, STYLE_RESET, STYLE_UNDERLINE, STYLE_RESET);
  printf("    NUMBER of rdtsc ticks that sending or receiving a single byte takes. The number\n");
  printf("    is the power of two, so '32' would mean 2^32 cycles. You can use this instead of\n");
  printf("    the '-r' option for bigger and less fine-grained numbers\n");
  printf("  %s-b%s, %s--initial-block-size%s=%sNUMBER%s\n", STYLE_BOLD, STYLE_RESET, STYLE_BOLD, STYLE_RESET, STYLE_UNDERLINE, STYLE_RESET);
  printf("    NUMBER of bytes that are considered to be in the same block initially (default: %ld)\n", defaultConfig->initialBlockSize);
  printf("  %s-e%s, %s--max-grouping-retries%s=%sNUMBER%s\n", STYLE_BOLD, STYLE_RESET, STYLE_BOLD, STYLE_RESET, STYLE_UNDERLINE, STYLE_RESET);
  printf("    NUMBER retries when an address can not be grouped (default: %ld)\n", defaultConfig->maxRetriesForGrouping);
  printf("  %s-m%s, %s--measurements%s=%sNUMBER%s\n", STYLE_BOLD, STYLE_RESET, STYLE_BOLD, STYLE_RESET, STYLE_UNDERLINE, STYLE_RESET);
  printf("    NUMBER of measurements when a single address is accessed (default: %ld)\n", defaultConfig->nMeasurements);
  printf("  %s-f%s, %s--no-fences%s\n", STYLE_BOLD, STYLE_RESET, STYLE_BOLD, STYLE_RESET);
  printf("    Disable memory fence instructions (mfence) between memory accesses (enabled by default)\n");
  printf("  %s-a%s, %s--parallel%s=%sPATH%s\n", STYLE_BOLD, STYLE_RESET, STYLE_BOLD, STYLE_RESET, STYLE_UNDERLINE, STYLE_RESET);
  printf("    If this option is used, the access times of each bank are stored in PATH/<bank_nr>_<cnt>.dat.\n");
  printf("  %s-l%s, %s--linear-addressing-functions%s=%sFUNCTIONS%s\n", STYLE_BOLD, STYLE_RESET, STYLE_BOLD, STYLE_RESET, STYLE_UNDERLINE, STYLE_RESET);
  printf("    List of addressing functions in hexadecimal format delimited by ',' (e.g. 0x22000,0x44000,0x88000,0x110000)\n");
  printf("  %s-z%s, %s--buffer-size%s=%sNUMBER%s\n", STYLE_BOLD, STYLE_RESET, STYLE_BOLD, STYLE_RESET, STYLE_UNDERLINE, STYLE_RESET);
  printf("    NUMBER of characters that are received in 'normal' receiver mode. NUMBER of measurements that\n");
  printf("    are performed within each thread before data is written to file in 'parallel' receiver mode (default: %ld)\n", defaultConfig->bufferSize);
  printf("  %s-v%s, %s--vm%s\n", STYLE_BOLD, STYLE_RESET, STYLE_BOLD, STYLE_RESET);
  printf("    Enable 'VM' mode which performs additional synchronization steps to work cross-vm\n");
  exit(exitCode);
}
