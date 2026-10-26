/* SPDX-License-Identifier: GPL-2.0-only */
#include "platform_model.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "Failed line %d: %s\n", __LINE__, #x); exit(1); \
} } while (0)

static void ready(struct tu_platform_model *m)
{
    tu_pm_init(m);
    CHECK(!tu_pm_can_handoff(m));
    CHECK(tu_pm_record_quiescence(m, TU_PM_OK) == TU_PM_ACCEPTED);
    CHECK(tu_pm_record_gop(m, TU_PM_GOP_CLEAR, true) == TU_PM_ACCEPTED);
    CHECK(tu_pm_can_handoff(m));
}

static void mapped(struct tu_platform_model *m)
{
    ready(m);
    CHECK(tu_pm_map(m, 0x1000, 0x80000000, 4096, 4096, TU_PM_OK) == 0);
    CHECK(m->cpu_token == 0x1000 && m->device_address == 0x80000000);
    CHECK(m->mapped_length == 4096 && m->requested_length == 4096);
    CHECK(!tu_pm_can_handoff(m));
}

static void running(struct tu_platform_model *m)
{
    mapped(m);
    CHECK(tu_pm_start(m, TU_PM_OK) == 0);
    CHECK(m->firmware == TU_PM_FW_ACTIVE && m->dma == TU_PM_DMA_RETAINED);
    CHECK(tu_pm_release(m, TU_PM_OK) == TU_PM_BLOCKED);
    CHECK(tu_pm_handoff(m) == TU_PM_BLOCKED);
}

static void recover(struct tu_platform_model *m)
{
    CHECK(m->phase == TU_PM_CONTAINED);
    CHECK(!tu_pm_can_handoff(m));
    CHECK(tu_pm_finish_recovery(m) == TU_PM_BLOCKED);
    CHECK(tu_pm_release(m, TU_PM_OK) == TU_PM_BLOCKED);
    CHECK(tu_pm_begin_recovery(m) == 0);
    CHECK(tu_pm_finish_recovery(m) == TU_PM_BLOCKED);
    CHECK(tu_pm_record_quiescence(m, TU_PM_OK) == 0);
    if (m->dma != TU_PM_DMA_NONE)
        CHECK(tu_pm_release(m, TU_PM_OK) == 0);
    CHECK(tu_pm_finish_recovery(m) == TU_PM_BLOCKED);
    CHECK(tu_pm_record_gop(m, TU_PM_GOP_CLEAR, true) == 0);
    CHECK(tu_pm_finish_recovery(m) == 0);
    CHECK(tu_pm_can_handoff(m));
}

static void test_success_and_gop(void)
{
    struct tu_platform_model m;
    enum tu_pm_gop gop;
    unsigned complete;
    running(&m);
    CHECK(tu_pm_mark_success(&m) == TU_PM_BLOCKED);
    CHECK(tu_pm_reset(&m, TU_PM_RESET_PRESTART, TU_PM_OK) == TU_PM_BLOCKED);
    CHECK(tu_pm_record_quiescence(&m, TU_PM_OK) == 0);
    CHECK(m.dma == TU_PM_DMA_RELEASABLE);
    CHECK(tu_pm_mark_success(&m) == TU_PM_BLOCKED);
    CHECK(tu_pm_release(&m, TU_PM_OK) == 0);
    CHECK(tu_pm_mark_success(&m) == TU_PM_BLOCKED);
    CHECK(tu_pm_record_gop(&m, TU_PM_GOP_CLEAR, true) == 0);
    CHECK(tu_pm_mark_success(&m) == 0);
    CHECK(tu_pm_reset(&m, TU_PM_RESET_PRESTART, TU_PM_OK) == TU_PM_BLOCKED);
    CHECK(tu_pm_reset(&m, TU_PM_RESET_RECOVERY, TU_PM_OK) == TU_PM_BLOCKED);
    CHECK(m.phase == TU_PM_SUCCESS && m.firmware == TU_PM_FW_QUIESCED);
    for (gop = TU_PM_GOP_UNKNOWN; gop <= TU_PM_GOP_CLEAR; ++gop) {
        for (complete = 0; complete < 2; ++complete) {
            CHECK(tu_pm_record_gop(&m, gop, complete != 0) == 0);
            CHECK(tu_pm_can_handoff(&m) ==
                  (gop == TU_PM_GOP_CLEAR && complete != 0));
            if (!tu_pm_can_handoff(&m))
                CHECK(tu_pm_handoff(&m) == TU_PM_BLOCKED);
        }
    }
    CHECK(tu_pm_handoff(&m) == 0);
    CHECK(tu_pm_handoff(&m) == TU_PM_BLOCKED);
    CHECK(tu_pm_record_gop(&m, TU_PM_GOP_CLEAR, true) == TU_PM_BLOCKED);
    CHECK(tu_pm_record_quiescence(&m, TU_PM_OK) == TU_PM_BLOCKED);
    CHECK(tu_pm_map(&m, 1, 0, 1, 1, TU_PM_OK) == TU_PM_BLOCKED);
    CHECK(tu_pm_start(&m, TU_PM_OK) == TU_PM_BLOCKED);
    CHECK(tu_pm_release(&m, TU_PM_OK) == TU_PM_BLOCKED);
    CHECK(tu_pm_begin_recovery(&m) == TU_PM_BLOCKED);
    ready(&m);
    CHECK(tu_pm_handoff(&m) == 0); /* No-unlock path is separately represented. */
}

static void test_failures(void)
{
    struct tu_platform_model m;
    enum tu_pm_outcome failure;
    unsigned operation;
    for (failure = TU_PM_FAIL_NO_EFFECT; failure <= TU_PM_FAIL_UNCERTAIN;
         ++failure) {
        for (operation = 0; operation < 4; ++operation) {
            int rc;
            switch (operation) {
            case 0:
                ready(&m);
                rc = tu_pm_map(&m, 0x1000, 0x80000000, 4096, 4096, failure);
                CHECK(m.dma == (failure == TU_PM_FAIL_NO_EFFECT ?
                      TU_PM_DMA_NONE : TU_PM_DMA_RETAINED));
                break;
            case 1:
                mapped(&m);
                rc = tu_pm_start(&m, failure);
                CHECK(m.dma == (failure == TU_PM_FAIL_NO_EFFECT ?
                      TU_PM_DMA_RELEASABLE : TU_PM_DMA_RETAINED));
                break;
            case 2:
                running(&m);
                rc = tu_pm_record_quiescence(&m, failure);
                CHECK(m.dma == TU_PM_DMA_RETAINED);
                CHECK(m.firmware == (failure == TU_PM_FAIL_NO_EFFECT ?
                      TU_PM_FW_ACTIVE : TU_PM_FW_UNKNOWN));
                break;
            default:
                mapped(&m);
                rc = tu_pm_release(&m, failure);
                CHECK(m.cpu_token == 0x1000 && m.mapped_length == 4096);
                break;
            }
            CHECK(rc == TU_PM_INJECTED_FAILURE);
            CHECK(m.phase == TU_PM_CONTAINED);
            if (failure == TU_PM_FAIL_UNCERTAIN) {
                CHECK(m.firmware == TU_PM_FW_UNKNOWN);
                CHECK(m.gop == TU_PM_GOP_UNKNOWN && !m.gop_graph_complete);
            }
            recover(&m);
        }
    }
}

static void test_reset_and_recovery(void)
{
    struct tu_platform_model m;
    enum tu_pm_outcome outcome;
    for (outcome = TU_PM_OK; outcome <= TU_PM_FAIL_UNCERTAIN; ++outcome) {
        ready(&m);
        CHECK(tu_pm_reset(&m, TU_PM_RESET_RECOVERY, outcome) == TU_PM_BLOCKED);
        CHECK(tu_pm_reset(&m, TU_PM_RESET_PRESTART, outcome) ==
              (outcome == TU_PM_OK ? TU_PM_ACCEPTED : TU_PM_INJECTED_FAILURE));
        CHECK(!tu_pm_can_handoff(&m));
        if (outcome == TU_PM_OK) {
            CHECK(m.firmware == TU_PM_FW_UNKNOWN);
            CHECK(tu_pm_record_quiescence(&m, TU_PM_OK) == 0);
            CHECK(!tu_pm_can_handoff(&m));
            CHECK(tu_pm_record_gop(&m, TU_PM_GOP_CLEAR, true) == 0);
            CHECK(tu_pm_can_handoff(&m));
        } else {
            recover(&m);
        }
        running(&m);
        CHECK(tu_pm_record_quiescence(&m, TU_PM_FAIL_UNCERTAIN) < 0);
        CHECK(tu_pm_begin_recovery(&m) == 0);
        CHECK(tu_pm_reset(&m, TU_PM_RESET_PRESTART, outcome) == TU_PM_BLOCKED);
        CHECK(tu_pm_reset(&m, TU_PM_RESET_RECOVERY, outcome) ==
              (outcome == TU_PM_OK ? TU_PM_ACCEPTED : TU_PM_INJECTED_FAILURE));
        CHECK(m.dma == TU_PM_DMA_RETAINED);
        CHECK(tu_pm_release(&m, TU_PM_OK) == TU_PM_BLOCKED);
        CHECK(!tu_pm_can_handoff(&m));
        if (outcome != TU_PM_OK)
            CHECK(tu_pm_begin_recovery(&m) == 0);
        CHECK(tu_pm_record_quiescence(&m, TU_PM_OK) == 0);
        CHECK(tu_pm_release(&m, TU_PM_OK) == 0);
        CHECK(tu_pm_record_gop(&m, TU_PM_GOP_CLEAR, true) == 0);
        CHECK(tu_pm_finish_recovery(&m) == 0);
    }
}

static void test_arguments_and_lengths(void)
{
    struct tu_platform_model m;
    tu_pm_init(NULL);
    CHECK(!tu_pm_can_handoff(NULL));
    CHECK(tu_pm_record_gop(NULL, TU_PM_GOP_CLEAR, true) == TU_PM_INVALID);
    CHECK(tu_pm_record_quiescence(NULL, TU_PM_OK) == TU_PM_INVALID);
    CHECK(tu_pm_map(NULL, 1, 0, 1, 1, TU_PM_OK) == TU_PM_INVALID);
    CHECK(tu_pm_start(NULL, TU_PM_OK) == TU_PM_INVALID);
    CHECK(tu_pm_release(NULL, TU_PM_OK) == TU_PM_INVALID);
    CHECK(tu_pm_mark_success(NULL) == TU_PM_INVALID);
    CHECK(tu_pm_handoff(NULL) == TU_PM_INVALID);
    CHECK(tu_pm_begin_recovery(NULL) == TU_PM_INVALID);
    CHECK(tu_pm_finish_recovery(NULL) == TU_PM_INVALID);
    CHECK(tu_pm_reset(NULL, TU_PM_RESET_PRESTART, TU_PM_OK) == TU_PM_INVALID);
    ready(&m);
    CHECK(tu_pm_record_gop(&m, (enum tu_pm_gop)99, true) == TU_PM_INVALID);
    CHECK(tu_pm_start(&m, (enum tu_pm_outcome)99) == TU_PM_INVALID);
    CHECK(tu_pm_reset(&m, (enum tu_pm_reset_policy)99, TU_PM_OK) == TU_PM_INVALID);
    CHECK(tu_pm_map(&m, 0, 0, 1, 1, TU_PM_OK) == TU_PM_INVALID);
    CHECK(tu_pm_map(&m, 1, 0, 0, 0, TU_PM_OK) == TU_PM_INVALID);
    CHECK(tu_pm_map(&m, 1, 0, 1, 2, TU_PM_OK) == TU_PM_INVALID);
    CHECK(tu_pm_map(&m, 1, UINT64_MAX, 2, 2, TU_PM_OK) == TU_PM_INVALID);
    CHECK(tu_pm_can_handoff(&m));
    CHECK(tu_pm_map(&m, 1, 0, 4096, 2048, TU_PM_OK) == 0);
    CHECK(tu_pm_start(&m, TU_PM_OK) == TU_PM_BLOCKED);
    CHECK(tu_pm_map(&m, 2, 0, 4096, 4096, TU_PM_OK) == TU_PM_BLOCKED);
    CHECK(tu_pm_release(&m, TU_PM_OK) == 0);
    CHECK(m.cpu_token == 0 && m.device_address == 0 && m.mapped_length == 0);
    CHECK(tu_pm_release(&m, TU_PM_OK) == TU_PM_BLOCKED);
    CHECK(tu_pm_map(&m, 1, UINT64_MAX, 1, 1, TU_PM_OK) == 0);
    CHECK(tu_pm_release(&m, TU_PM_OK) == 0);
}

int main(void)
{
    test_success_and_gop();
    test_failures();
    test_reset_and_recovery();
    test_arguments_and_lengths();
    puts("Platform model contracts passed (synthetic state and failures only)");
    return 0;
}
