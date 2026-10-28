# Changelog

## Unreleased

Initial experimental host foundation:

- Pinned TU102 upstream source provenance and GPL-2.0-only license.
- Portable VBIOS/FWSEC, ELF and Booter parsers; explicit little-endian Booter decoding.
- Framebuffer layout/radix calculations and synthetic lifecycle failure model.
- Four host test suites, seeded fuzz harness and Windows PowerShell test runner.
- Linux GCC/Clang and Windows MSVC/clang-cl CI definitions.

No EFI unlocker, target hardware validation or Strata performance claim is included.

### Validation and research follow-up

- Confirmed Windows PowerShell/MSVC/clang-cl host CI; preserved physical-hardware gates.
- Added source ZIP integrity verification, Layout/Prep boundaries and pinned upstream
  differential checks; measured branch coverage and extended seeded fuzz smoke.
- Reviewed NInfer's 22GB Turing fork and ninfer-all at pinned commits. No engine code
  was imported and their inference results are not treated as hardware-unlock evidence.
