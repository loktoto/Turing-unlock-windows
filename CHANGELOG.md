# Changelog

## Unreleased

Initial experimental host foundation:

- Pinned TU102 upstream source provenance and GPL-2.0-only license.
- Portable VBIOS/FWSEC, ELF and Booter parsers; explicit little-endian Booter decoding.
- Framebuffer layout/radix calculations and synthetic lifecycle failure model.
- Four host test suites, seeded fuzz harness and Windows PowerShell test runner.
- Linux GCC/Clang and Windows MSVC/clang-cl CI definitions.

No EFI unlocker, target hardware validation or Strata performance claim is included.
