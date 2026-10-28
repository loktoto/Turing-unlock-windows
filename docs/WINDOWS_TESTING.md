# Windows host test handoff

This is the Windows compiler and synthetic test check for the G1 host library.
It is **not an EFI unlocker** and does not establish RTX 2080 Ti 22GB, firmware,
stock NVIDIA driver, CUDA or Strata compatibility. No GPU is needed. Run as a
normal user; this procedure does not change firmware, boot entries, drivers,
Secure Boot, BitLocker or PowerShell execution policy.

## Prerequisites

- Windows x64 with Windows PowerShell 5.1 or PowerShell 7.
- Visual Studio 2022 or its Build Tools, with **Desktop development with C++**,
  the MSVC v143 x64/x86 build tools and a Windows SDK.
- CMake **3.21 or later** and its matching CTest on `PATH`. CMake 3.21 adds the
  VS 2022 generator and the CTest JUnit output used by the script. The project's
  CMake minimum alone is insufficient for this Windows runner.
- For the optional second compiler: install **C++ Clang tools for Windows** and
  **MSBuild support for LLVM (clang-cl)** through Visual Studio Installer.
- A local checkout of this repository in a writable directory. Git is only
  needed to obtain/update that checkout.

Open Developer PowerShell for VS 2022 in the checkout. The script selects the
VS 2022 generator and x64 architecture explicitly, so CMake can also discover
the installed toolchain from an ordinary PowerShell with CMake on `PATH`.
Use your organization's existing script execution/signing policy. If that
policy blocks this local script, use the explicit commands below or the
organization's approved signing workflow; no policy change is required here.

## Run and inspect

From the repository root:

```powershell
# Default: MSVC v143, x64, Debug.
.\scripts\Test-WindowsHost.ps1
$LASTEXITCODE

# MSVC plus the optional clang-cl check, with independent build directories.
.\scripts\Test-WindowsHost.ps1 -IncludeClangCL
$LASTEXITCODE

# Optional: choose an artifact parent and parallel build count.
.\scripts\Test-WindowsHost.ps1 -IncludeClangCL -OutputRoot C:\Temp\tu102-host-results -Jobs 4
$LASTEXITCODE
```

Each invocation creates a unique directory under `artifacts/windows-host` (or
`-OutputRoot`). Never reuse an older result as evidence for a newer source tree.
Debug is the reference configuration for consistent diagnostics;
`-Configuration Release` and `RelWithDebInfo` are also available. The suites use
custom checks that remain active in those configurations. Record the selected
configuration with each result.

The script returns **0 only if every requested toolchain configures, builds and
passes CTest**; it returns **1 on failure**, including a missing prerequisite.
It checks native process exit codes explicitly. When one toolchain fails, the
other requested toolchain is still attempted. A failed configure or build leaves
later stages as `not_run`, rather than reporting their absence as a test failure.
CTest treats zero discovered tests as an error.

Retain these artifacts for the Windows handoff:

- `result.json`: schema version, UTC times, overall status, requested toolchains,
  separate configure/build/test statuses, errors and stage exit codes/log paths.
- `cmake-version.log`, `ctest-version.log`: tool versions.
- Per toolchain: `compiler.log` with CMake's compiler path, identity and version;
  `configure.log`, `build.log`, `test.log`; and `ctest.xml` JUnit results after
  CTest runs. CMake/CTest's own detailed logs remain in the corresponding `build`
  directory.

The runner does not gather machine inventory, account names, serial numbers,
PCI IDs, ROMs or firmware. Build tools can print absolute source/compiler paths
that contain a user name. Review logs before sharing; share the summary and
relevant logs rather than the entire build tree. Record the tested Git commit
alongside the result when handing it off.

## Explicit commands

These commands are useful for investigating a failed stage. In interactive
PowerShell, check `$LASTEXITCODE` after **each** command and stop on a nonzero
value. Use fresh build directories for a new run or compiler selection.

```powershell
cmake -S . -B build-windows-msvc -G "Visual Studio 17 2022" -A x64 -T v143 -DSANITIZE=OFF -DFUZZ=OFF
cmake --build build-windows-msvc --config Debug --parallel 2
ctest --test-dir build-windows-msvc -C Debug --output-on-failure --no-tests=error --output-junit msvc-ctest.xml

# Optional: requires the VS LLVM/Clang components listed above.
cmake -S . -B build-windows-clangcl -G "Visual Studio 17 2022" -A x64 -T ClangCL -DSANITIZE=OFF -DFUZZ=OFF
cmake --build build-windows-clangcl --config Debug --parallel 2
ctest --test-dir build-windows-clangcl -C Debug --output-on-failure --no-tests=error --output-junit clangcl-ctest.xml
```

If configuration cannot find VS, install/repair the C++ workload and v143
components. If only ClangCL fails to configure, verify both LLVM components in
VS Installer. If compilation fails, inspect `compiler.log` and `build.log`. If
CTest fails, inspect `test.log`, `ctest.xml` and `build/Testing/Temporary`.
Keep failures visible in `result.json`; an MSVC pass does not imply a ClangCL
pass, and a successful build does not imply successful tests.

## Gate boundaries and current evidence

This handoff supplies a runner, not a Windows execution result. Actual Windows
MSVC/clang-cl results still need to be produced on Windows. Linux validation
cannot certify either toolchain.

Passing Windows host tests contributes only compiler and synthetic contract
evidence to G1. Follow [BUILD_DIRECTION.md](BUILD_DIRECTION.md) and
[G1_TEST_REPORT.md](G1_TEST_REPORT.md) for the remaining gate requirements.
G2 is the later read-only EFI inspector; G3 separately verifies no-unlock
handoff to the original Windows Boot Manager and recovery. Profile-specific
unlock and stock driver/CUDA/Strata verification belong to later gates. Do not
install boot components or attempt GPU/EFI work as part of this host check.

## CI evidence update — 2026-10-28

Windows MSVC and clang-cl, Linux GCC/Clang sanitizers and fuzz smoke all passed
at commit `4a587da33ef3b07b556f1375e1a17c888768bbd4`: [run 40397802686](https://github.com/loktoto/Turing-unlock-windows/actions/runs/40397802686).
This supersedes earlier statements that Windows compilation had not run. It
validates host code, not GPU/UEFI compatibility or the target-machine profile.
