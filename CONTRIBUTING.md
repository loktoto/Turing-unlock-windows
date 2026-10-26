# Contributing

This project is an experimental Windows-first host foundation for TU102 research.
G1 is incomplete: passing parser and model tests does not demonstrate GPU unlock,
Windows boot, stock NVIDIA driver, CUDA, or Strata compatibility. Read the
[README](README.md), [development direction](docs/BUILD_DIRECTION.md), and
[upstream lock](UPSTREAM_LOCK.md) before changing behavior.

## Scope and licensing

Contribute focused fixes, synthetic test cases, Windows build results, and
reproducible documentation improvements. Discuss changes to stage boundaries or
hardware-facing behavior in an issue before implementing them. Keep Windows
host validation distinct from later EFI and hardware work.

Contributions are accepted under **GPL-2.0-only**, as recorded in [LICENSE](LICENSE).
Submit only work you have the right to contribute. Preserve upstream copyright,
license notices, attribution, and provenance. Identify imported code and its
source revision; do not silently replace the source lock or treat platform
references as interchangeable implementations.

Do not submit firmware dumps, proprietary binaries, credentials, private keys,
recovery keys, or unredacted hardware inventories. Use synthetic bytes for test
fixtures. Review logs for usernames, absolute paths, serial numbers, and other
personal information before posting them.

## Build and check

For Windows prerequisites, the host runner, and its result format, follow
[Windows testing](docs/WINDOWS_TESTING.md):

```powershell
.\scripts\Test-WindowsHost.ps1
```

Record the commit, compiler version, build configuration, commands, exit status,
and CTest results. Optional clang-cl results must be reported separately from
MSVC results. If Windows was unavailable, state that explicitly.

For the Linux GCC/Clang host checks:

```sh
cmake -S . -B build -DSANITIZE=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Use a fresh build directory when changing compilers or configurations. A Linux
pass provides host evidence only; it does not verify Windows execution. Changes
to parser bounds, layout rules, or error handling should include a focused
synthetic regression case. Documentation-only changes need link and command
review rather than a new test.

## Issues and pull requests

Search existing issues before opening a report. Include reproducible steps,
expected and actual results, and relevant redacted logs. Do not describe a
planned capability as working without the corresponding execution evidence.

Keep pull requests small and explain the user-visible effect, relevant gate,
and validation performed. Document untested environments and remaining gaps.
Use the pull request checklist and update affected documentation when behavior
or prerequisites change. Keep discussion respectful and specific to the work.
