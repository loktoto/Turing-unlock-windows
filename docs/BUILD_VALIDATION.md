# Build validation

Current executed results are recorded in [G1_TEST_REPORT.md](G1_TEST_REPORT.md).
Windows instructions and result collection: [WINDOWS_TESTING.md](WINDOWS_TESTING.md).

Linux:
```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DSANITIZE=ON
cmake --build build
UBSAN_OPTIONS=halt_on_error=1 ctest --test-dir build --output-on-failure
```

Clang fuzz smoke:
```sh
CC=clang cmake -S . -B build-fuzz -DFUZZ=ON
cmake --build build-fuzz
python3 scripts/make_fuzz_corpus.py build-corpus
./build-fuzz/fuzz_parsers build-corpus -seed=1 -runs=10000 -max_total_time=30 -max_len=1048576
```

Seeds are generated from synthetic headers with zero-filled bodies; they are not
firmware. Fuzz mutates the output directory, so do not check it into source control.
Boot parsing now uses explicit little-endian loads and only publishes validated
outputs. Windows compiler/PowerShell execution is still unverified in this Linux environment.
CI configuration alone is not evidence of a successful remote run.

## CI evidence update — 2026-10-28

Windows MSVC and clang-cl, Linux GCC/Clang sanitizers and fuzz smoke all passed
at commit `4a587da33ef3b07b556f1375e1a17c888768bbd4`: [run 40397802686](https://github.com/loktoto/Turing-unlock-windows/actions/runs/40397802686).
This supersedes earlier statements that Windows compilation had not run. It
validates host code, not GPU/UEFI compatibility or the target-machine profile.
