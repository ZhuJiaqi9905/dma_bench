#pragma once
#include <doca_error.h>
#define MAX_ARG_SIZE 256 /* Maximum size of input argument */
struct DMAConfig {
  char pci_address[MAX_ARG_SIZE];
  int iterations;
  int byte_size;
  int warm_up;
};
