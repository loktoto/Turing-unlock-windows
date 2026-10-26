/* SPDX-License-Identifier: GPL-2.0-only */
#include "platform_model.h"
#include <string.h>

static bool valid_outcome(enum tu_pm_outcome outcome)
{
    return outcome == TU_PM_OK || outcome == TU_PM_FAIL_NO_EFFECT ||
           outcome == TU_PM_FAIL_UNCERTAIN;
}

static bool gop_clear(const struct tu_platform_model *m)
{
    return m->gop_graph_complete && m->gop == TU_PM_GOP_CLEAR;
}

static void uncertain(struct tu_platform_model *m)
{
    m->firmware = TU_PM_FW_UNKNOWN;
    if (m->dma != TU_PM_DMA_NONE)
        m->dma = TU_PM_DMA_RETAINED;
    m->gop = TU_PM_GOP_UNKNOWN;
    m->gop_graph_complete = false;
}

static int failed(struct tu_platform_model *m, enum tu_pm_outcome outcome)
{
    if (outcome == TU_PM_FAIL_UNCERTAIN)
        uncertain(m);
    m->phase = TU_PM_CONTAINED;
    return TU_PM_INJECTED_FAILURE;
}

void tu_pm_init(struct tu_platform_model *m)
{
    if (!m)
        return;
    memset(m, 0, sizeof(*m));
    m->firmware = TU_PM_FW_UNKNOWN;
    m->gop = TU_PM_GOP_UNKNOWN;
    m->phase = TU_PM_READY;
}

int tu_pm_record_gop(struct tu_platform_model *m, enum tu_pm_gop gop,
                     bool complete)
{
    if (!m || (gop != TU_PM_GOP_UNKNOWN && gop != TU_PM_GOP_TARGET_HELD &&
               gop != TU_PM_GOP_CLEAR))
        return TU_PM_INVALID;
    if (m->phase == TU_PM_HANDED_OFF)
        return TU_PM_BLOCKED;
    m->gop = gop;
    m->gop_graph_complete = complete;
    return TU_PM_ACCEPTED;
}

int tu_pm_record_quiescence(struct tu_platform_model *m,
                            enum tu_pm_outcome outcome)
{
    if (!m || !valid_outcome(outcome))
        return TU_PM_INVALID;
    if (m->phase != TU_PM_READY && m->phase != TU_PM_RUNNING &&
        m->phase != TU_PM_RECOVERY)
        return TU_PM_BLOCKED;
    if (outcome != TU_PM_OK)
        return failed(m, outcome);
    m->firmware = TU_PM_FW_QUIESCED;
    if (m->dma == TU_PM_DMA_RETAINED)
        m->dma = TU_PM_DMA_RELEASABLE;
    return TU_PM_ACCEPTED;
}

int tu_pm_map(struct tu_platform_model *m, uint64_t cpu, uint64_t device,
              size_t requested, size_t mapped, enum tu_pm_outcome outcome)
{
    if (!m || !valid_outcome(outcome) || !cpu || !requested || !mapped ||
        mapped > requested || mapped - 1 > UINT64_MAX - device)
        return TU_PM_INVALID;
    if (m->phase != TU_PM_READY || m->firmware != TU_PM_FW_QUIESCED ||
        m->dma != TU_PM_DMA_NONE || !gop_clear(m))
        return TU_PM_BLOCKED;
    if (outcome == TU_PM_FAIL_NO_EFFECT)
        return failed(m, outcome);
    m->cpu_token = cpu;
    m->device_address = device;
    m->requested_length = requested;
    m->mapped_length = mapped;
    m->dma = TU_PM_DMA_RELEASABLE;
    if (outcome != TU_PM_OK)
        return failed(m, outcome);
    return TU_PM_ACCEPTED;
}

int tu_pm_start(struct tu_platform_model *m, enum tu_pm_outcome outcome)
{
    if (!m || !valid_outcome(outcome))
        return TU_PM_INVALID;
    if (m->phase != TU_PM_READY || m->firmware != TU_PM_FW_QUIESCED ||
        m->dma != TU_PM_DMA_RELEASABLE || !gop_clear(m) ||
        m->mapped_length != m->requested_length)
        return TU_PM_BLOCKED;
    if (outcome != TU_PM_OK)
        return failed(m, outcome);
    m->firmware = TU_PM_FW_ACTIVE;
    m->dma = TU_PM_DMA_RETAINED;
    /* Starting firmware invalidates the prestart ownership observation. */
    m->gop = TU_PM_GOP_UNKNOWN;
    m->gop_graph_complete = false;
    m->phase = TU_PM_RUNNING;
    return TU_PM_ACCEPTED;
}

int tu_pm_release(struct tu_platform_model *m, enum tu_pm_outcome outcome)
{
    if (!m || !valid_outcome(outcome))
        return TU_PM_INVALID;
    if ((m->phase != TU_PM_READY && m->phase != TU_PM_RUNNING &&
         m->phase != TU_PM_RECOVERY) ||
        m->firmware != TU_PM_FW_QUIESCED || m->dma != TU_PM_DMA_RELEASABLE)
        return TU_PM_BLOCKED;
    if (outcome != TU_PM_OK)
        return failed(m, outcome);
    m->dma = TU_PM_DMA_NONE;
    m->cpu_token = 0;
    m->device_address = 0;
    m->requested_length = 0;
    m->mapped_length = 0;
    return TU_PM_ACCEPTED;
}

static bool resources_clear(const struct tu_platform_model *m)
{
    return m->firmware == TU_PM_FW_QUIESCED &&
           m->dma == TU_PM_DMA_NONE && gop_clear(m);
}

int tu_pm_mark_success(struct tu_platform_model *m)
{
    if (!m)
        return TU_PM_INVALID;
    if (m->phase != TU_PM_RUNNING || !resources_clear(m))
        return TU_PM_BLOCKED;
    m->phase = TU_PM_SUCCESS;
    return TU_PM_ACCEPTED;
}

bool tu_pm_can_handoff(const struct tu_platform_model *m)
{
    return m && (m->phase == TU_PM_READY || m->phase == TU_PM_SUCCESS) &&
           resources_clear(m);
}

int tu_pm_handoff(struct tu_platform_model *m)
{
    if (!m)
        return TU_PM_INVALID;
    if (!tu_pm_can_handoff(m))
        return TU_PM_BLOCKED;
    m->phase = TU_PM_HANDED_OFF;
    return TU_PM_ACCEPTED;
}

int tu_pm_begin_recovery(struct tu_platform_model *m)
{
    if (!m)
        return TU_PM_INVALID;
    if (m->phase != TU_PM_CONTAINED)
        return TU_PM_BLOCKED;
    m->phase = TU_PM_RECOVERY;
    /* Recovery must refresh ownership even after a no-effect failure. */
    m->gop = TU_PM_GOP_UNKNOWN;
    m->gop_graph_complete = false;
    return TU_PM_ACCEPTED;
}

int tu_pm_finish_recovery(struct tu_platform_model *m)
{
    if (!m)
        return TU_PM_INVALID;
    if (m->phase != TU_PM_RECOVERY || !resources_clear(m))
        return TU_PM_BLOCKED;
    m->phase = TU_PM_READY;
    return TU_PM_ACCEPTED;
}

int tu_pm_reset(struct tu_platform_model *m, enum tu_pm_reset_policy policy,
                enum tu_pm_outcome outcome)
{
    if (!m || !valid_outcome(outcome) ||
        (policy != TU_PM_RESET_PRESTART && policy != TU_PM_RESET_RECOVERY))
        return TU_PM_INVALID;
    if (!((policy == TU_PM_RESET_PRESTART && m->phase == TU_PM_READY &&
           m->firmware == TU_PM_FW_QUIESCED && m->dma == TU_PM_DMA_NONE) ||
          (policy == TU_PM_RESET_RECOVERY && m->phase == TU_PM_RECOVERY)))
        return TU_PM_BLOCKED;
    if (outcome != TU_PM_OK)
        return failed(m, outcome);
    uncertain(m);
    return TU_PM_ACCEPTED;
}
