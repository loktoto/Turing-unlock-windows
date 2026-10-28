# G1 / Windows Host readiness — 2026-10-28

**Ready for Windows Host testing; full G1 remains BLOCKED.** No EFI unlocker is supplied.

## Executed evidence

- Linux x64 GCC 14.2 and Clang 19.1.7 Debug + ASan/UBSan: each CTest 5/5 passed
  after the Layout/Prep boundary suite was added.
- The Clang libFuzzer build's five CTest targets passed; a fresh synthetic-seeded
  campaign completed 100,000 runs in 24 seconds with no sanitizer finding.
  This bounded campaign is not exhaustive and is not a performance benchmark.
- The repeatable [offline upstream comparison](DIFFERENTIAL_TESTING.md) passed
  7,663 cases with both GCC and Clang against the locked upstream. It compares
  return codes and successful named fields across retained Layout, Radix, ELF,
  Booter, v4 boot descriptor, framebuffer readiness and FWSEC functions. The
  upstream Booter build uses documented host shims; failure outputs intentionally
  differ. Differential agreement is not an independent proof of correctness.
- Earlier independent FWSEC contract tests also passed against the pinned original
  parser (12,368 parser calls). Current [coverage](HOST_COVERAGE.md) records 98.00%
  aggregate executed lines; some branch outcomes remain unvisited.
- The prior five-source GCC -fanalyzer run emitted no diagnostics. This is not
  a complete formal or security review.
- Ten source-kit verification tests pass. The generated ZIP verifies against its
  embedded SHA256 manifest; this proves internal consistency, not publisher identity.

Test targets: host_contracts, fwsec_contracts, platform_model_contracts,
boot_contracts and layout_prep_contracts. Synthetic data covers V2/V3 ROM/BIT,
NPDE/multiple-image adjustment, truncation/overflow, malformed descriptors, ELF
ambiguity and bounds, layout/radix boundaries, explicit boot byte ordering and
mocked ownership/failure/recovery transitions. No executable firmware is supplied.

## Windows evidence

[Run 40637956001](https://github.com/loktoto/Turing-unlock-windows/actions/runs/40637956001)
at `d052ec2` passed the delivered PowerShell runner under Windows PowerShell with
MSVC and clang-cl, plus Linux GCC/Clang sanitizers, fuzz smoke and source-kit jobs.
It predates the new fifth suite/differential harness; later commits require fresh
CI. Remote Windows compilation is not a target-GPU test. Windows result JSON/logs
and the source ZIP are uploaded as workflow artifacts.

## Remaining acceptance evidence

The [input/API contract](INPUT_AND_API_CONTRACTS.md) specifies future firmware
provenance and integrity requirements, but no real firmware profile is approved
and no firmware loader admission implementation is supplied. The single-buffer
platform model does not fulfill the documented multi-buffer DMA contract.
Broader structure-aware testing, meaningful remaining branches, a longer fuzz
campaign and independent acceptance review remain necessary before admitting G2.

G0 target inventory/baselines are absent. G2 remains a design; G3–G6 require the
exact target and recovery evidence. See [milestone readiness](MILESTONE_READINESS.md).
Source-kit verification and successful mocks must not be interpreted as permission
to deploy a GPU unlock EFI or as evidence of target compatibility.
