#include "include/core.h"
#include <doca_log.h>
DOCA_LOG_REGISTER(core.c);
doca_error_t init_core_object(struct DocaCore *state, uint32_t extensions,
                               uint32_t workq_depth, uint32_t max_chunks) {
  doca_error_t res;
  struct doca_workq *workq;

  res = doca_mmap_create(NULL, &state->mmap);
  if (res != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Unable to create mmap: %s", doca_get_error_string(res));
    return res;
  }

  res =
      doca_buf_inventory_create(NULL, max_chunks, extensions, &state->buf_inv);
  if (res != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Unable to create buffer inventory: %s",
                 doca_get_error_string(res));
    return res;
  }

  res = doca_mmap_set_max_num_chunks(state->mmap, max_chunks);
  if (res != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Unable to set memory map nb chunks: %s",
                 doca_get_error_string(res));
    return res;
  }

  res = doca_mmap_start(state->mmap);
  if (res != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Unable to start memory map: %s", doca_get_error_string(res));
    doca_mmap_destroy(state->mmap);
    state->mmap = NULL;
    return res;
  }

  res = doca_mmap_dev_add(state->mmap, state->dev);
  if (res != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Unable to add device to mmap: %s",
                 doca_get_error_string(res));
    doca_mmap_destroy(state->mmap);
    state->mmap = NULL;
    return res;
  }

  res = doca_buf_inventory_start(state->buf_inv);
  if (res != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Unable to start buffer inventory: %s",
                 doca_get_error_string(res));
    return res;
  }

  res = doca_ctx_dev_add(state->ctx, state->dev);
  if (res != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Unable to register device with lib context: %s",
                 doca_get_error_string(res));
    state->ctx = NULL;
    return res;
  }

  res = doca_ctx_start(state->ctx);
  if (res != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Unable to start lib context: %s", doca_get_error_string(res));
    doca_ctx_dev_rm(state->ctx, state->dev);
    state->ctx = NULL;
    return res;
  }

  res = doca_workq_create(workq_depth, &workq);
  if (res != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Unable to create work queue: %s", doca_get_error_string(res));
    return res;
  }

  res = doca_ctx_workq_add(state->ctx, workq);
  if (res != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Unable to register work queue with context: %s",
                 doca_get_error_string(res));
    doca_workq_destroy(state->workq);
    state->workq = NULL;
  } else
    state->workq = workq;

  return res;
}

doca_error_t destroy_core_object(struct DocaCore *state) {
  doca_error_t tmp_result, result = DOCA_SUCCESS;

  if (state->workq != NULL) {
    tmp_result = doca_ctx_workq_rm(state->ctx, state->workq);
    if (tmp_result != DOCA_SUCCESS) {
      DOCA_ERROR_PROPAGATE(result, tmp_result);
      DOCA_LOG_ERR("Failed to remove work queue from ctx: %s",
                   doca_get_error_string(tmp_result));
    }

    tmp_result = doca_workq_destroy(state->workq);
    if (tmp_result != DOCA_SUCCESS) {
      DOCA_ERROR_PROPAGATE(result, tmp_result);
      DOCA_LOG_ERR("Failed to destroy work queue: %s",
                   doca_get_error_string(tmp_result));
    }
    state->workq = NULL;
  }

  if (state->buf_inv != NULL) {
    tmp_result = doca_buf_inventory_destroy(state->buf_inv);
    if (tmp_result != DOCA_SUCCESS) {
      DOCA_ERROR_PROPAGATE(result, tmp_result);
      DOCA_LOG_ERR("Failed to destroy buf inventory: %s",
                   doca_get_error_string(tmp_result));
    }
    state->buf_inv = NULL;
  }

  if (state->mmap != NULL) {
    tmp_result = doca_mmap_dev_rm(state->mmap, state->dev);
    if (tmp_result != DOCA_SUCCESS) {
      DOCA_ERROR_PROPAGATE(result, tmp_result);
      DOCA_LOG_ERR("Failed to remove device from mmap: %s",
                   doca_get_error_string(tmp_result));
    }

    tmp_result = doca_mmap_destroy(state->mmap);
    if (tmp_result != DOCA_SUCCESS) {
      DOCA_ERROR_PROPAGATE(result, tmp_result);
      DOCA_LOG_ERR("Failed to destroy mmap: %s",
                   doca_get_error_string(tmp_result));
    }
    state->mmap = NULL;
  }

  if (state->ctx != NULL) {
    tmp_result = doca_ctx_stop(state->ctx);
    if (tmp_result != DOCA_SUCCESS) {
      DOCA_ERROR_PROPAGATE(result, tmp_result);
      DOCA_LOG_ERR("Unable to stop context: %s",
                   doca_get_error_string(tmp_result));
    }

    tmp_result = doca_ctx_dev_rm(state->ctx, state->dev);
    if (tmp_result != DOCA_SUCCESS) {
      DOCA_ERROR_PROPAGATE(result, tmp_result);
      DOCA_LOG_ERR("Failed to remove device from ctx: %s",
                   doca_get_error_string(tmp_result));
    }
  }

  if (state->dev != NULL) {
    tmp_result = doca_dev_close(state->dev);
    if (tmp_result != DOCA_SUCCESS) {
      DOCA_ERROR_PROPAGATE(result, tmp_result);
      DOCA_LOG_ERR("Failed to close device: %s",
                   doca_get_error_string(tmp_result));
    }
    state->dev = NULL;
  }

  return result;
}