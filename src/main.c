#include "include/common.h"
#include "include/params.h"
#include <doca_argp.h>
#include <doca_error.h>
#include <doca_log.h>
#include <stdlib.h>
DOCA_LOG_REGISTER(DPU_LOCAL_DMA_COPY::MAIN);

int main(int argc, char **argv) {
  struct DMAConfig config = {
      .byte_size = 1024,
      .iterations = 1,
      .pci_address = "03:00.0",
      .warm_up = 1,
  };
  doca_error_t result = DOCA_SUCCESS;
  result = doca_argp_init("dma_bench", &config);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to init ARGP resources: %s",
                 doca_get_error_string(result));
    return EXIT_FAILURE;
  }
  result = register_dma_params();
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to register DMA sample parameters: %s",
                 doca_get_error_string(result));
    return EXIT_FAILURE;
  }
  result = doca_argp_start(argc, argv);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to parse sample input: %s",
                 doca_get_error_string(result));
    return EXIT_FAILURE;
  }
}