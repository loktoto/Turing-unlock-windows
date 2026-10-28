# Host inputs, outputs and trust boundaries

The host library inspects caller-provided byte arrays and computes layout. It has
no approved target profile and accepts no executable firmware for deployment.
An input parsing successfully never means it is authentic or safe to execute.

## Current API rules

- Every buffer pointer must refer to at least the declared accessible byte length.
  A length parameter cannot make a false caller-owned pointer safe.
- Only consume output fields after return == 0. Layout validates before assigning;
  Booter parsing now publishes one complete result on success. ELF may modify its
  span before later rejecting a duplicate/bad section. Radix planning and VBIOS
  parsing may clear outputs before rejecting. Callers must not depend on failure
  outputs, except specifically tested transactional Booter behavior.
- VBIOS parses bounded ROM/BIT/descriptor structures; a checksum is not a signature.
- ELF selects a uniquely named bounded section; success does not authenticate an ELF
  image, resolve all non-selected sections or establish its suitability for firmware.
- Layout uses untrusted-by-default caller-supplied framebuffer bounds; synthetic
  success does not certify live BAR/VRAM/MMU state or the modified memory straps.
- Boot descriptors validate the retained upstream geometry, not instruction contents.
- `tu_platform_model` is a single-buffer synthetic model. Public fields and supplied
  quiescence/GOP observations are not capabilities or independent hardware evidence.

## Required future firmware-input policy

Before a future hardware prototype can accept real input, an exact profile must
bind GPU PCI/subsystem identity, VBIOS/ROM identity, board/BIOS and observable GOP
state to reviewed firmware input versions, exact lengths and cryptographic hashes.
Expected identities must come from independently reviewed provenance, not an
allowlist supplied together with the untrusted blob. The profile must identify the
upstream commit, original distribution URL/version, extraction recipe and applicable
license; hash agreement proves identity relative to that record, not publisher trust.
No real profile or approved firmware-input record is provided here.

The future loader must bound allocations/reads before parsing, verify expected
length and digest over the actual bytes it will parse/use, reject unknown/duplicate
roles and mismatches, and prevent a path replacement between verification and use.
A timestamp, model name, PCI ID, embedded checksum, successful parse or unlocked
log is insufficient. Signed provenance must be validated against an independently
trusted key if a signature-based policy is adopted; do not collect user private keys.

Current source-kit SHA256 verification checks archive consistency only. The manifest
inside that same ZIP is not a publisher signature and is not a firmware trust store.
The upstream differential tool verifies checked-out sources against the pinned
commit and file hashes before compiling them; it does not approve firmware inputs.

## Multi-buffer adapter requirements (not implemented)

A real DMA adapter must keep a per-allocation ledger of CPU allocation, device
address, requested/mapped length, mapping handle, live/unmapped/freed status and
possible device visibility. Track partial or uncertain mapping even if a call fails.
Starting firmware may expose multiple buffers; uncertainty retains every potentially
visible buffer. Cleanup of one allocation must not make others releasable. Failed
unmap/free must keep a record that prevents handoff; do not retry an operation whose
side effects are unknown as if it were guaranteed not to have happened.

Quiescence must be established with profile-specific evidence before releasing any
possibly visible DMA. Reset success alone is not evidence. Handoff requires the
entire ledger cleared and GOP ownership restored, not merely a zero counter or one
successful release. The current single-buffer model cannot satisfy this requirement;
a multi-buffer mock/adapter and failure-injection review remain acceptance work.
