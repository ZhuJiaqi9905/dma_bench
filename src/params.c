#include "include/params.h"
#include "include/common.h"
#include <doca_argp.h>
#include <doca_log.h>
#include <string.h>
DOCA_LOG_REGISTER(params.c);
static doca_error_t pci_callback(void *param, void *config) {
  struct DMAConfig *conf = (struct DMAConfig *)config;
  const char *addr = (char *)param;
  int addr_len = strlen(addr);
  if (addr_len == MAX_ARG_SIZE) {
    DOCA_LOG_ERR("Entered pci address exceeded buffer size of: %d",
                 MAX_ARG_SIZE - 1);
    return DOCA_ERROR_INVALID_VALUE;
  }
  strcpy(conf->pci_address, addr);
  return DOCA_SUCCESS;
}
static doca_error_t iteration_callback(void *param, void *config) {
  struct DMAConfig *conf = (struct DMAConfig *)config;
  int iteration = *(int *)param;
  if (iteration <= 0) {
    DOCA_LOG_ERR("Iteration needs to be positive, but got %d", iteration);
    return DOCA_ERROR_INVALID_VALUE;
  }
  conf->iterations = iteration;
  return DOCA_SUCCESS;
}
static doca_error_t warm_up_callback(void *param, void *config) {
  struct DMAConfig *conf = (struct DMAConfig *)config;
  int warm_up = *(int *)param;
  if (warm_up <= 0) {
    DOCA_LOG_ERR("Iteration needs to be positive, but got %d", warm_up);
    return DOCA_ERROR_INVALID_VALUE;
  }
  conf->warm_up = warm_up;
  return DOCA_SUCCESS;
}
static doca_error_t byte_size_callback(void *param, void *config) {
  struct DMAConfig *conf = (struct DMAConfig *)config;
  int byte_size = *(int *)param;
  if (byte_size <= 0) {
    DOCA_LOG_ERR("Data size to be positive, but got %d", byte_size);
    return DOCA_ERROR_INVALID_VALUE;
  }
  conf->byte_size = byte_size;
  return DOCA_SUCCESS;
}
doca_error_t register_params() {

  doca_error_t result = DOCA_SUCCESS;
  // Create and register PCI address param
  struct doca_argp_param *pci_address_param;
  result = doca_argp_param_create(&pci_address_param);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to create ARGP param: %s",
                 doca_get_error_string(result));
    return result;
  }
  doca_argp_param_set_short_name(pci_address_param, "p");
  doca_argp_param_set_long_name(pci_address_param, "pci");
  doca_argp_param_set_description(pci_address_param,
                                  "DOCA DMA device PCI address");
  doca_argp_param_set_callback(pci_address_param, pci_callback);
  doca_argp_param_set_type(pci_address_param, DOCA_ARGP_TYPE_STRING);
  result = doca_argp_register_param(pci_address_param);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to register program param: %s",
                 doca_get_error_string(result));
    return result;
  }

  // Create and register iteration param
  struct doca_argp_param *iteration_param;
  result = doca_argp_param_create(&iteration_param);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to create ARGP param: %s",
                 doca_get_error_string(result));
    return result;
  }
  doca_argp_param_set_short_name(iteration_param, "i");
  doca_argp_param_set_long_name(iteration_param, "iteration");
  doca_argp_param_set_description(iteration_param,
                                  "DMA iteration times for benchmark");
  doca_argp_param_set_callback(iteration_param, iteration_callback);
  doca_argp_param_set_type(iteration_param, DOCA_ARGP_TYPE_INT);
  result = doca_argp_register_param(iteration_param);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to register program param: %s",
                 doca_get_error_string(result));
    return result;
  }

  // Create and register warm_up param
  struct doca_argp_param *warm_up_param;
  result = doca_argp_param_create(&warm_up_param);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to create ARGP param: %s",
                 doca_get_error_string(result));
    return result;
  }
  doca_argp_param_set_short_name(warm_up_param, "w");
  doca_argp_param_set_long_name(warm_up_param, "warm-up");
  doca_argp_param_set_description(warm_up_param, "Warm up times for benchmark");
  doca_argp_param_set_callback(warm_up_param, warm_up_callback);
  doca_argp_param_set_type(warm_up_param, DOCA_ARGP_TYPE_INT);
  result = doca_argp_register_param(warm_up_param);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to register program param: %s",
                 doca_get_error_string(result));
    return result;
  }
  // Create and register byte_size param
  struct doca_argp_param *byte_size_param;
  result = doca_argp_param_create(&byte_size_param);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to create ARGP param: %s",
                 doca_get_error_string(result));
    return result;
  }
  doca_argp_param_set_short_name(byte_size_param, "s");
  doca_argp_param_set_long_name(byte_size_param, "size");
  doca_argp_param_set_description(byte_size_param, "DMA data size");
  doca_argp_param_set_callback(byte_size_param, byte_size_callback);
  doca_argp_param_set_type(byte_size_param, DOCA_ARGP_TYPE_INT);
  result = doca_argp_register_param(byte_size_param);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to register program param: %s",
                 doca_get_error_string(result));
    return result;
  }
  return DOCA_SUCCESS;
}