# Turing Unlock for Windows — NVIDIA RTX 2080 Ti / TU102

[![Host checks](https://github.com/loktoto/Turing-unlock-windows/actions/workflows/host.yml/badge.svg)](https://github.com/loktoto/Turing-unlock-windows/actions/workflows/host.yml)
[![License: GPL-2.0-only](https://img.shields.io/badge/license-GPL--2.0--only-blue)](LICENSE)

**NVIDIA Turing GPU unlock research for Windows (TU102 / RTX 2080 Ti 22GB):** an experimental, community-maintained port and portable C host-validation toolkit hosted by **loktoto**.
The long-term goal is RTX 2080 Ti 22GB → UEFI pre-boot unlock → stock NVIDIA
Windows driver → CUDA → measurable Strata inference improvements.

**Current release scope: a Windows-testable host C library and synthetic tests.**
There is no bootable EFI unlocker, verified Windows GPU unlock, or measured
Strata speedup in this repository. No GPU is required to run the current tests.

繁體中文：本專案正建立 TU102 Windows 移植基礎，目前只可測試 Host 程式，
未能用來開機解鎖顯示卡。Windows 編譯結果須以實際測試／CI 為準。

## Start on Windows

Install Visual Studio 2022 Build Tools with **Desktop development with C++**,
MSVC v143, a Windows SDK, and **CMake 3.21+** on PATH. Clone or extract the source,
then run from Developer PowerShell:

```powershell
git clone https://github.com/loktoto/Turing-unlock-windows.git
cd Turing-unlock-windows
.\scripts\Test-WindowsHost.ps1
```

The script writes `result.json`, build logs and test results under
`artifacts/windows-host/`. Optional: add `-IncludeClangCL` after installing the
VS LLVM tools. Follow [Windows testing](docs/WINDOWS_TESTING.md) for prerequisites,
manual commands and troubleshooting. No administrator access, GPU access,
firmware execution, driver installation or boot configuration changes are needed.

## Linux development

Requires a C11 compiler and CMake 3.16+:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DSANITIZE=ON
cmake --build build
UBSAN_OPTIONS=halt_on_error=1 ctest --test-dir build --output-on-failure
```

See [build validation](docs/BUILD_VALIDATION.md) for synthetic-seeded fuzzing.
Local Linux GCC and Clang sanitizer runs passed all four test suites; see the
[recorded evidence](docs/G1_TEST_REPORT.md). A workflow badge describes CI, not
hardware support. Windows execution is not claimed by the local evidence.

## What is included

| Directory | Purpose |
|---|---|
| `host/` | Bounded ROM/ELF/Booter parsing, layout calculations and pure lifecycle model |
| `tests/` | Synthetic golden/negative cases, mock failure injection and fuzz entry point |
| `scripts/` | Windows test runner, synthetic corpus generation and source packaging |
| `docs/` | Architecture, provenance, test evidence and gate requirements |
| `.github/` | Cross-platform CI and contribution templates |

## Roadmap

| Gate | Scope | Status |
|---|---|---|
| G0 | Target inventory, stock baseline and recovery evidence | Awaiting target-machine evidence |
| G1 | Portable parsers, layout, mock lifecycle and host verification | In progress; Windows host test kit available |
| G2 | Read-only UEFI diagnostic | Not started |
| G3 | No-unlock Windows handoff and recovery | Not started |
| G4 | Profile-specific TU102 unlock prototype | Not started |
| G5 | Stock Windows driver + CUDA/GEMM validation | Not started |
| G6 | Repeated Strata Prefill/TTFT/Decode A/B | Not started |

[Build direction](docs/BUILD_DIRECTION.md) defines the sequencing. G1 is not
complete merely because host tests pass. A separate RTX 3080 research line is
outside the initial TU102 scope; this is not a multi-GPU inference project.

## Attribution and license

The portable core derives from [nlzy/turing-unlock](https://github.com/nlzy/turing-unlock),
under **GPL-2.0-only**. This repository preserves that license and upstream attribution;
it does not claim original authorship of the upstream implementation.
[Windows-GPU-UNLOCK](https://github.com/impacifist/Windows-GPU-UNLOCK) is a platform
architecture reference, not proof of TU102 support. Exact commits, original file
hashes and local adaptations are documented in [UPSTREAM_LOCK.md](UPSTREAM_LOCK.md)
and [license notes](docs/LICENSE_NOTES.md). NVIDIA firmware is not included and
must not be assumed redistributable under this project's license.

## Contributing

Read [CONTRIBUTING.md](CONTRIBUTING.md), then open an issue or pull request.
Useful contributions include Windows test results, synthetic parser cases,
portability fixes and reproducible test infrastructure. Reports must distinguish
synthetic tests, compiler results and real hardware observations.

Do not submit recovery keys, passwords, private keys, raw firmware dumps or
unreviewed hardware-write payloads. Review logs for personal paths before sharing.

To build a source-only ZIP with a SHA256 manifest:

```sh
python scripts/package_host_kit.py TU102_Windows_Host_Test_Kit.zip
```

## CI evidence update — 2026-10-28

Windows MSVC and clang-cl, Linux GCC/Clang sanitizers and fuzz smoke all passed
at commit `4a587da33ef3b07b556f1375e1a17c888768bbd4`: [run 40397802686](https://github.com/loktoto/Turing-unlock-windows/actions/runs/40397802686).
This supersedes earlier statements that Windows compilation had not run. It
validates host code, not GPU/UEFI compatibility or the target-machine profile.
