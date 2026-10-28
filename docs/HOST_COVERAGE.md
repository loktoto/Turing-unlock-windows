# Host coverage snapshot — 2026-10-28

GCC 14.2, Debug, `--coverage`, five CTest suites passed. Measured with
`gcov -b CMakeFiles/tu102_host.dir/host/*.gcno CMakeFiles/tu102_platform_model.dir/host/*.gcno`.
This measures deterministic unit tests, not cumulative fuzz coverage.

| Source | Lines executed | Branches taken at least once |
|---|---:|---:|
| boot.c | 96.08% | 90.32% |
| layout.c | 96.25% | 86.11% |
| prep.c | 100.00% | 95.83% |
| vbios.c | 96.88% | 86.54% |
| platform_model.c | 100.00% | 91.56% |

Aggregate reported line coverage: 98.00% of 551 lines, including diagnostic inline
code. High coverage does not prove firmware authenticity, correctness of untested
layouts, device quiescence, or target Windows boot safety. Additional Layout/Prep boundary tests improved branch coverage; remaining unvisited
branches still need case-by-case review, and model coverage is not coverage of a real adapter.

Reproduce:

```sh
cmake -S . -B build-coverage -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS=--coverage -DCMAKE_EXE_LINKER_FLAGS=--coverage
cmake --build build-coverage
ctest --test-dir build-coverage --output-on-failure
cd build-coverage
gcov -b CMakeFiles/tu102_host.dir/host/*.gcno CMakeFiles/tu102_platform_model.dir/host/*.gcno
```
