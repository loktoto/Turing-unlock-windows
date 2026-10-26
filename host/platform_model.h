/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef TU_PLATFORM_MODEL_H
#define TU_PLATFORM_MODEL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Synthetic bookkeeping only. No value here is hardware evidence or a mapping. */
enum tu_pm_firmware { TU_PM_FW_UNKNOWN, TU_PM_FW_QUIESCED, TU_PM_FW_ACTIVE };
enum tu_pm_dma { TU_PM_DMA_NONE, TU_PM_DMA_RELEASABLE, TU_PM_DMA_RETAINED };
enum tu_pm_gop { TU_PM_GOP_UNKNOWN, TU_PM_GOP_TARGET_HELD, TU_PM_GOP_CLEAR };
enum tu_pm_phase { TU_PM_READY, TU_PM_RUNNING, TU_PM_SUCCESS,
                   TU_PM_CONTAINED, TU_PM_RECOVERY, TU_PM_HANDED_OFF };
enum tu_pm_outcome { TU_PM_OK, TU_PM_FAIL_NO_EFFECT, TU_PM_FAIL_UNCERTAIN };
enum tu_pm_reset_policy { TU_PM_RESET_PRESTART, TU_PM_RESET_RECOVERY };
enum tu_pm_status { TU_PM_ACCEPTED = 0, TU_PM_INVALID = -1,
                    TU_PM_BLOCKED = -2, TU_PM_INJECTED_FAILURE = -3 };

struct tu_platform_model {
    enum tu_pm_firmware firmware;
    enum tu_pm_dma dma;
    enum tu_pm_gop gop;
    enum tu_pm_phase phase;
    bool gop_graph_complete;
    /* Opaque, independently supplied numbers: never dereferenced or mapped. */
    uint64_t cpu_token;
    uint64_t device_address;
    size_t requested_length;
    size_t mapped_length;
};

void tu_pm_init(struct tu_platform_model *model);
int tu_pm_record_gop(struct tu_platform_model *model, enum tu_pm_gop ownership,
                     bool graph_complete);
/* Caller supplies synthetic evidence; reset success alone is insufficient. */
int tu_pm_record_quiescence(struct tu_platform_model *model,
                            enum tu_pm_outcome outcome);
int tu_pm_map(struct tu_platform_model *model, uint64_t cpu_token,
              uint64_t device_address, size_t requested_length,
              size_t mapped_length, enum tu_pm_outcome outcome);
int tu_pm_start(struct tu_platform_model *model, enum tu_pm_outcome outcome);
int tu_pm_release(struct tu_platform_model *model, enum tu_pm_outcome outcome);
int tu_pm_mark_success(struct tu_platform_model *model);
bool tu_pm_can_handoff(const struct tu_platform_model *model);
int tu_pm_handoff(struct tu_platform_model *model);
int tu_pm_begin_recovery(struct tu_platform_model *model);
int tu_pm_finish_recovery(struct tu_platform_model *model);
/* Explicit policy; successful execution invalidates prior quiescence/GOP evidence. */
int tu_pm_reset(struct tu_platform_model *model, enum tu_pm_reset_policy policy,
                enum tu_pm_outcome outcome);

#endif
