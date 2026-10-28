# Milestone readiness

This is an evidence ledger, not authorization to perform device writes. No target inventory or target-hardware results were supplied. [Upstream research](UPSTREAM_RESEARCH.md) distinguishes source observations from verified behavior. The [host CI run 40637956001](https://github.com/loktoto/Turing-unlock-windows/actions/runs/40637956001) succeeded for commit `d052ec2`, including the delivered Windows PowerShell runner with MSVC and clang-cl; it is host evidence, not target-hardware evidence. Changes after that commit require their own validation.

| Gate | Status | Evidence and remaining acceptance |
|---|---|---|
| G0: inventory and baseline | Blocked | Obtain redacted exact PCI/subsystem and VBIOS identity, board/BIOS, Windows/driver, display wiring, Secure Boot and BitLocker state, PSU/power limit, recovery path and baseline measurements. A PCI ID alone is not a validated profile. |
| G1: host inspection core | In progress | Synthetic tests and bounded fuzz/differential results are in [G1_TEST_REPORT.md](G1_TEST_REPORT.md); CI evidence is separately identified below. Complete the acceptance work listed below. No hardware safety follows from mock tests. |
| G2: read-only EFI inspector | Design only | Pin EDK II and its toolchain, implement and validate the read-only contract below; no build or target run is claimed. |
| G3: no-unlock handoff | Blocked | On the exact machine, separately demonstrate complete ownership inventory, display/console recovery, original Windows Boot Manager handoff and independent recovery, including failure paths. |
| G4: profile-specific engine | Blocked | Requires G0–G3 evidence, trusted firmware-input policy, reviewed profile-specific sequencing, DMA/ownership and quiescence evidence and contained failures. Successful TU102 execution must not unconditionally perform post-success FLR. |
| G5: correctness | Blocked | Verify stock driver behavior and controlled GEMM numerical correctness on the target, with reproducible settings and raw results. |
| G6: workload comparison | Blocked | Repeat same-machine baseline and treatment with documented settings for Strata prefill, TTFT and decode, reporting variability and failures. No speedup is inferred. |

## Remaining G1 acceptance

1. Preserve and review the Windows MSVC and clang-cl build and test results, runner versions and logs from the successful CI run, and repeat them for the final reviewed commit. Distinguish a remote CI pass from a physical target test.
2. Review the [input/output contracts](INPUT_AND_API_CONTRACTS.md) and the [7,663-case differential corpus](DIFFERENTIAL_TESTING.md), which now covers all retained function families. Expand structured cases where meaningful and retain independent golden expectations; shared upstream agreement alone is not correctness.
3. [Branch coverage](HOST_COVERAGE.md) is measured; close remaining meaningful gaps in truncation, arithmetic, format boundaries and lifecycle failure/recovery paths; preserve reproducible commands and reports. Continue seeded longer fuzzing under sanitizers, retain any reproducer and document limits rather than treating a finite run as proof.
4. Review the [firmware input and API contract](INPUT_AND_API_CONTRACTS.md) and implement its future-loader admission checks separately from parsing. Do not distribute proprietary payloads. Review the state model against an explicit multi-buffer DMA and ownership contract; test uncertain side effects, partial mappings, cleanup failure, reset and handoff without asserting hardware quiescence. Independent review must sign off the chosen acceptance criteria and residual limitations.

## G2 read-only design for review

An EDK II application should enumerate PCI I/O and GOP handles, snapshot bounded PCI configuration and device paths with statuses, and report segment/bus/device/function, identity, subsystem, BAR descriptors and current command/attributes. Enumerate GOP handles, console relationships, parent/device paths, mode/framebuffer metadata and protocol ownership relationships as observable; mark missing, failed, ambiguous or incomplete observations unknown. Do not infer exclusive ownership from a single handle or assume a platform topology. Output a versioned, redacted report with individual failure statuses and a clear unsupported/unknown verdict. Match inventory against an independently reviewed exact profile only; a match is evidence for further review, not permission to unlock.

Inspection must perform no GPU register or PCI configuration writes, bus-master or decode changes, reset, DMA allocation/map, driver disconnect, firmware start, EFI installation or boot-variable modification. Do not silently fall back to more invasive ROM acquisition. Keep the original boot path independently selectable. Test with mocked PCI/GOP graphs, multiple/absent targets, inconsistent paths, protocol failures, bounded output and explicit assertions that no write-capable operation is called. Build reproducibly with pinned toolchain and inspect source/binary before any separately authorized target observation. A report alone cannot establish safe display restoration, device quiescence or Windows handoff.
