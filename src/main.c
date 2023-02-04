#include "include/common.h"
#include "include/core.h"
#include "include/params.h"
#include <doca_argp.h>
#include <doca_buf.h>
#include <doca_dev.h>
#include <doca_dma.h>
#include <doca_error.h>
#include <doca_log.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
DOCA_LOG_REGISTER(main.c);
const unsigned char all_char[] =
    "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
#define ALL_CHAR_LEN sizeof(all_char)
static doca_error_t dma_benchmark(struct doca_pci_bdf *pcie_dev,
                                  struct DMAConfig *config);
static doca_error_t do_dma_once(struct DocaCore *core,
                                struct doca_buf *dst_doca_buf,
                                struct doca_buf *src_doca_buf);
static doca_error_t do_dma_benchmark(struct DMAConfig *config,
                                     struct DocaCore *core,
                                     struct doca_buf *dst_doca_buf,
                                     struct doca_buf *src_doca_buf);
static doca_error_t generate_random_data(struct doca_buf *buf) {
  size_t len;
  doca_error_t result = DOCA_SUCCESS;
  result = doca_buf_get_data_len(buf, &len);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Fail in get data len: %s", doca_get_error_string(result));
    return result;
  }
  // DOCA_LOG_INFO("data len is %lu", len);
  uint8_t *data;
  result = doca_buf_get_data(buf, (void **)&data);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Fail in get data: %s", doca_get_error_string(result));
    return result;
  }
  for (int i = 0; i < len - 1; ++i) {
    data[i] = all_char[rand() % ALL_CHAR_LEN];
  }
  data[len - 1] = 0;
  return result;
}
static doca_error_t check_data(struct doca_buf *dst_buf,
                               struct doca_buf *src_buf, int *is_equal) {
  size_t src_len;
  doca_error_t result = DOCA_SUCCESS;
  result = doca_buf_get_data_len(src_buf, &src_len);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Fail in get data len: %s", doca_get_error_string(result));
    return result;
  }
  // DOCA_LOG_INFO("data len is %lu", src_len);
  uint8_t *src_data;
  result = doca_buf_get_data(src_buf, (void **)&src_data);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Fail in get data: %s", doca_get_error_string(result));
    return result;
  }
  size_t dst_len;
  result = doca_buf_get_data_len(dst_buf, &dst_len);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Fail in get data len: %s", doca_get_error_string(result));
    return result;
  }
  // DOCA_LOG_INFO("data len is %lu", dst_len);
  uint8_t *dst_data;
  result = doca_buf_get_data(dst_buf, (void **)&dst_data);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Fail in get data: %s", doca_get_error_string(result));
    return result;
  }
  if (dst_len != src_len) {
    DOCA_LOG_INFO("Src and dst len not equal");
    *is_equal = 0;
    return result;
  }
  DOCA_LOG_INFO("src data: %s, dst data: %s", src_data, dst_data);
  for (size_t i = 0; i < src_len; ++i) {
    if (dst_data[i] != src_data[i]) {
      *is_equal = 0;
      return result;
    }
  }
  *is_equal = 1;
  return result;
}
static doca_error_t do_dma_benchmark(struct DMAConfig *config,
                                     struct DocaCore *core,
                                     struct doca_buf *dst_doca_buf,
                                     struct doca_buf *src_doca_buf) {
  doca_error_t result = DOCA_SUCCESS;
  for (int i = 0; i < config->warm_up; ++i) {
    result = generate_random_data(src_doca_buf);
    if (result != DOCA_SUCCESS) {
      DOCA_LOG_ERR("Fail in generate random data");
      return result;
    }
    result = do_dma_once(core, dst_doca_buf, src_doca_buf);
    if (result != DOCA_SUCCESS) {
      DOCA_LOG_ERR("Fail in dma operation. Warm up %d.", i);
      return result;
    }
    int is_equal;
    result = check_data(dst_doca_buf, src_doca_buf, &is_equal);
    if (result != DOCA_SUCCESS) {
      DOCA_LOG_ERR("Fail in check data");
      return result;
    }
    if (!is_equal) {
      DOCA_LOG_ERR("src data and dst data not equal");
      return result;
    }
  }
  double avg_duration = 0;
  for (int i = 0; i < config->iterations; ++i) {
    struct timeval stv, etv;
    gettimeofday(&stv, NULL);
    result = do_dma_once(core, dst_doca_buf, src_doca_buf);
    if (result != DOCA_SUCCESS) {
      DOCA_LOG_ERR("Fail in dma operation. Iteration %d.", i);
    }
    gettimeofday(&etv, NULL);
    long duration =
        (etv.tv_sec - stv.tv_sec) * 1000000 + etv.tv_usec - stv.tv_usec;
    avg_duration += duration;
    DOCA_LOG_INFO("duration %ldus", duration);
  }
  avg_duration /= config->iterations;
  DOCA_LOG_INFO("average duration %lf us", avg_duration);
  return result;
}
static doca_error_t do_dma_once(struct DocaCore *core,
                                struct doca_buf *dst_doca_buf,
                                struct doca_buf *src_doca_buf) {
  struct doca_dma_job_memcpy dma_job = {0};
  struct timespec ts;
  doca_error_t result;
  struct doca_event event = {0};
  dma_job.base.type = DOCA_DMA_JOB_MEMCPY;
  dma_job.base.flags = DOCA_JOB_FLAGS_NONE;
  dma_job.base.ctx = core->ctx;
  dma_job.dst_buff = dst_doca_buf;
  dma_job.src_buff = src_doca_buf;

  /* Enqueue DMA job */
  result = doca_workq_submit(core->workq, &dma_job.base);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to submit DMA job: %s", doca_get_error_string(result));
    return result;
  }

  /* Wait for job completion */
  while ((result = doca_workq_progress_retrieve(
              core->workq, &event, DOCA_WORKQ_RETRIEVE_FLAGS_NONE)) ==
         DOCA_ERROR_AGAIN) {
    /* Wait for the job to complete */
    ts.tv_sec = 0;
    ts.tv_nsec = SLEEP_IN_NANOS;
    nanosleep(&ts, &ts);
  }
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to retrieve DMA job: %s",
                 doca_get_error_string(result));
    return result;
  }
  /* event result is valid */
  result = (doca_error_t)event.result.u64;
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("DMA job event returned unsuccessfully: %s",
                 doca_get_error_string(result));
    return result;
  }
  DOCA_LOG_INFO("Success, memory copied");
  return result;
}
int main(int argc, char **argv) {
  // Read params
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
  result = register_params();
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
  DOCA_LOG_INFO("pci_address: %s, byte_size: %d, warm up %d, iterations %d",
                config.pci_address, config.byte_size, config.warm_up,
                config.iterations);
  // parse PCIe address
  struct doca_pci_bdf pcie_dev;
  result = parse_pci_addr(config.pci_address, &pcie_dev);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to parse pci address: %s",
                 doca_get_error_string(result));
    doca_argp_destroy();
    return EXIT_FAILURE;
  }
  // dma benchmark
  dma_benchmark(&pcie_dev, &config);
  doca_argp_destroy();
  return EXIT_SUCCESS;
}

/// @brief do dma benchmark.
/// @param pcie_dev [in]: The PCIe address of a device.
/// @param config [in]: config of DMA benchmark.
/// @return
static doca_error_t dma_benchmark(struct doca_pci_bdf *pcie_dev,
                                  struct DMAConfig *config) {
  doca_error_t result = DOCA_SUCCESS;
  // allocate src_buf
  uint8_t *src_buf;
  src_buf = (uint8_t *)malloc(config->byte_size);
  if (src_buf == NULL) {
    DOCA_LOG_ERR("Source buffer allocation failed");
    result = DOCA_ERROR_NO_MEMORY;
    goto fail_alloc_src_buf;
  }
  // allocate dst_buf
  uint8_t *dst_buf;
  dst_buf = (uint8_t *)malloc(config->byte_size);
  if (dst_buf == NULL) {
    DOCA_LOG_ERR("Source buffer allocation failed");
    result = DOCA_ERROR_NO_MEMORY;
    goto fail_alloc_dst_buf;
  }
  // create DMA engine
  struct doca_dma *dma_ctx;
  result = doca_dma_create(&dma_ctx);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Unable to create DMA engine: %s",
                 doca_get_error_string(result));
    goto fail_create_dma;
  }
  // create core object
  struct DocaCore doca_core = {0};
  doca_core.ctx = doca_dma_as_ctx(dma_ctx);
  result = open_doca_device_with_pci(pcie_dev, dma_jobs_is_supported,
                                     &doca_core.dev);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Unable to open DOCA device: %s",
                 doca_get_error_string(result));
    goto fail_open_doca_dev;
  }
  uint32_t max_chunks = 2; /* Two chunks for source and dest buffers */
  result = init_core_object(&doca_core, DOCA_BUF_EXTENSION_NONE, WORKQ_DEPTH,
                            max_chunks);
  if (result != DOCA_SUCCESS) {
    goto fail_init_core_obj;
  }
  // 把DMA的源地址和目的地址都加入到DOCA memory map中。
  if (doca_mmap_populate(doca_core.mmap, dst_buf, config->byte_size, PAGE_SIZE,
                         NULL, NULL) != DOCA_SUCCESS ||
      doca_mmap_populate(doca_core.mmap, src_buf, config->byte_size, PAGE_SIZE,
                         NULL, NULL) != DOCA_SUCCESS) {
    goto fail_mmap;
  }
  struct doca_buf *src_doca_buf;
  struct doca_buf *dst_doca_buf;
  // Clear destination memory buffer
  memset(dst_buf, 0, config->byte_size);
  // Construct DOCA buffer for each address range
  result =
      doca_buf_inventory_buf_by_addr(doca_core.buf_inv, doca_core.mmap, src_buf,
                                     config->byte_size, &src_doca_buf);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Unable to acquire DOCA buffer representing source buffer: %s",
                 doca_get_error_string(result));
    goto fail_construct_src_buf;
  }
  // DOCA_LOG_INFO("Before set data");
  // DOCA_LOG_INFO("src_buffer: %p", src_buffer);
  // print_doca_buf_info(src_doca_buf);

  /* Construct DOCA buffer for each address range */
  result =
      doca_buf_inventory_buf_by_addr(doca_core.buf_inv, doca_core.mmap, dst_buf,
                                     config->byte_size, &dst_doca_buf);

  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR(
        "Unable to acquire DOCA buffer representing destination buffer: %s",
        doca_get_error_string(result));
    goto fail_construct_dst_buf;
  }

  // Set data position in src_buff
  result = doca_buf_set_data(src_doca_buf, src_buf, config->byte_size);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to set data for DOCA buffer: %s",
                 doca_get_error_string(result));
    goto fail_set_src_buff_data;
  }
  // do benchmark
  do_dma_benchmark(config, &doca_core, dst_doca_buf, src_doca_buf);
fail_set_src_buff_data:
  doca_buf_refcount_rm(dst_doca_buf, NULL);
fail_construct_dst_buf:

  doca_buf_refcount_rm(src_doca_buf, NULL);
fail_construct_src_buf:
fail_mmap:

  destroy_core_object(&doca_core);
fail_init_core_obj:
  if (doca_core.dev != NULL) {
    result = doca_dev_close(doca_core.dev);
    if (result != DOCA_SUCCESS) {
      DOCA_LOG_ERR("Failed to close dev: %s", doca_get_error_string(result));
    }
  }

fail_open_doca_dev:
  result = doca_dma_destroy(dma_ctx);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to destroy dma: %s", doca_get_error_string(result));
  }
fail_create_dma:
  free(dst_buf);
fail_alloc_dst_buf:
  free(src_buf);
fail_alloc_src_buf:
  return result;
}