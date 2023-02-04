#pragma once
#include <doca_buf_inventory.h>
#include <doca_ctx.h>
#include <doca_dev.h>
#include <doca_error.h>
#include <doca_mmap.h>
#include <doca_types.h>

struct DocaCore {
  struct doca_dev *dev;
  struct doca_mmap *mmap;
  struct doca_buf_inventory *buf_inv;
  struct doca_ctx *ctx;
  struct doca_workq *workq;
};

doca_error_t destroy_core_object(struct DocaCore *state);
doca_error_t init_core_object(struct DocaCore *state, uint32_t extensions,
                               uint32_t workq_depth, uint32_t max_chunks);