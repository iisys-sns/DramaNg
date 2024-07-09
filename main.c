#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<signal.h>
#include<unistd.h>

#include"src/config.h"
#include"src/helper.h"
#include"src/sender.h"
#include"src/receiver.h"

void handleSigInt(int signal) {
  cleanupReceiver();
  exit(-1);
}

int main(int argc, char * argv[]) {
  struct sigaction action;
  action.sa_handler = handleSigInt;
  sigaction(SIGINT, &action, NULL);

  Config *config = getConfig(argc, argv);
  setClflush(config->clflush);

  void *thp = getTHP();
  void *address = NULL;
  if(config->isSender) {
    address = detectInitSignal(thp, 512 * sysconf(_SC_PAGESIZE), config->nMeasurements, config->threshold);
    if(address == NULL) {
      dprintf(2, "Unable to synchronize. Is the receiver running and in init mode?\n");
      exit(EXIT_FAILURE);
    }
  } else {
    address = thp;
    sendInitSignal(address, config->nMeasurements);
    initReceiver("timings.dat");
  }

  //dprintf(2, "Initialization done. Press any key to continue.\n");
  //getchar();

  if(config->isSender) {
    sendChars(config->sendStr, strlen(config->sendStr), address, config->nMeasurements, config->rate, config->debug, config->vmMode);
  } else {
    if(config->parallel != NULL) {
      char **addresses = malloc(sizeof(char *) * (1UL<<config->nLinearAddressingFunctions));
      for(uint64_t i = 0;i < (1UL<<config->nLinearAddressingFunctions); i++) {
        addresses[i] = NULL;
      }

      void **thps = malloc(sizeof(void *) * config->thps);
      for(uint64_t i = 0; i < config->thps; i++) {
        thps[i] = getTHP();
        for(uint64_t x = 0; x < 512 * sysconf(_SC_PAGESIZE); x++) {
          uint64_t bank = getBankNumber(thps[i] + x, config->linearAddressingFunctions, config->nLinearAddressingFunctions);
          if(addresses[bank] != NULL) {
            continue;
          }
          addresses[bank] = thps[i] + x;
        }
      }

      for(uint64_t i = 0;i < (1UL<<config->nLinearAddressingFunctions); i++) {
        if(addresses[i] == NULL) {
          dprintf(2, "Error: There is no address for bank %ld. Try increasing the number of thps ('-p' command-line flag)\n", i);
          exit(EXIT_FAILURE);
        }
      }

      dprintf(2, "Initialization done. Press RETURN to continue...\n");
      getchar();
      receiveParallel(addresses, (1UL<<config->nLinearAddressingFunctions), config->nMeasurements, config->debug, config->parallel, config->linearAddressingFunctions, config->nLinearAddressingFunctions, config->bufferSize, config->threshold);
      free(addresses);
    } else {
      receive(address, config->nMeasurements, config->threshold, config->rate, config->debug, config->bufferSize, config->vmMode);
    }
  }

  freeTHP(thp);

  exit(EXIT_SUCCESS);
}
