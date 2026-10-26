# Build direction

Decision: TU102-specific engine, portable C core, separate EDK II UEFI adapter.
Keep original Windows Boot Manager as an independent manually selectable path.
Do not implement unlock as a Windows application under an active NVIDIA driver.

Why: pinned Linux TU102 has the relevant parser/firmware sequencing evidence;
Windows Ampere provides platform design lessons but not matching TU102 firmware or
hardware profiles. Keeping the core independent makes bounds checking and lifecycle
failure tests possible without touching the display GPU.

## Deliverable sequence

1. **Host inspection core (current G1)**: ROM/ELF/Booter parsing, WPR layout,
   synthetic goldens and malformed input testing. Next implement mock-only ownership
   and DMA lifecycle contracts, seeded fuzzing, differential tests and Windows builds.
2. **Read-only EFI inspector (G2, after G1)**: EDK II application using PCI I/O and
   GOP enumeration; collect identity and ownership evidence. No bus mastering changes,
   register writes, firmware start, reset or boot installation. A PCI match alone is
   not a verified profile; unknown machine profiles remain unsupported.
3. **No-unlock handoff (G3)**: separately validate display restoration and loading
   the original Windows Boot Manager with documented recovery on the actual board.
4. **Profile-specific engine (G4)**: explicit pre-reset, firmware-active, quiesced,
   success and contained-failure states. No unconditional post-success FLR. Never
   release DMA or chainload while device ownership is uncertain.
5. **Verification (G5/G6)**: stock driver, controlled GEMM numerical checks and
   same-machine Strata Prefill/TTFT/Decode A/B. No estimated throughput claim.

EDK II is the proposed integration choice, not a dependency downloaded or a build
already validated. Pin its release/commit and toolchain when G2 work is admitted.
Do not import Ampere payloads or an executable MMIO adapter into the host library.

## Work that can proceed without target hardware

Synthetic fixtures, Windows compiler compatibility, fuzzing, parser differential
comparison, API/state-machine mock tests and license/provenance review.

## Inputs required for machine-specific work

Redacted inventory: PCI/subsystem, VBIOS, motherboard/BIOS, Windows/driver,
Secure Boot/BitLocker status, power limit; monitor wiring, PSU and recovery media.
No secret keys or public firmware upload requested. Missing inputs do not block
host development but do block claims of target compatibility.
