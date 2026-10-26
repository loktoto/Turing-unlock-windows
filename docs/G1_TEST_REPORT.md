# G1 / Windows Host readiness — 2026-10-26

**Ready for Windows Host testing; full G1 remains BLOCKED.** No EFI unlocker is supplied.

## Executed evidence

- GCC 14.2 Linux x64 Debug + ASan/UBSan: CTest 4/4 passed.
- Clang 19.1.7 Linux x64 Debug + ASan/UBSan: CTest 4/4 passed.
- Clang libFuzzer build: CTest 4/4 passed; final updated-parser campaign completed
  10,000 runs with five synthetic format seeds plus evolved corpus, no sanitizer finding.
  An earlier 10,000-run campaign also completed. This is bounded smoke, not exhaustive fuzzing.
- GCC -fanalyzer over boot, layout, prep, vbios and platform_model: no diagnostics.
- Same FWSEC test executable compiled against pinned original upstream vbios.c:
  passed all 12,368 parser calls, including expected field values and rejection codes.
  This compares shared synthetic expectations, not all output bytes or real firmware.

Test targets: host_contracts, fwsec_contracts, platform_model_contracts, boot_contracts.
Coverage includes V2/V3 ROM/BIT descriptors, NPDE/multiple-image adjustment, truncation,
overflow, malformed descriptors, ELF spans, layout/radix sizes, boot endian decoding,
transactional boot outputs, and mocked ownership/failure/recovery transitions.
All data is synthetic; no NVIDIA firmware, signatures, GPU or EFI operations are included.

## Remaining acceptance evidence

Windows MSVC/clang-cl and PowerShell runner execution require a Windows machine.
The source kit and workflow are prepared, but remote CI has not run. Full G1 also
needs wider differential/golden validation, measured coverage, longer fuzz campaigns,
firmware-input authenticity design and review of platform model assumptions.
Mocked success is not proof of hardware quiescence or safe boot handoff.

G0 target inventory/baselines are absent. G2–G6 remain unstarted. A Windows Host pass
must not be interpreted as permission to deploy a GPU unlock EFI or claim compatibility.
Follow WINDOWS_TESTING.md for the next test, preserving the original Windows boot setup.

PowerShell 7 runner syntax and mocked native-process success/failure paths were
checked on Linux by the Windows-kit worker. This does not verify VS2022 or PS5.1.
