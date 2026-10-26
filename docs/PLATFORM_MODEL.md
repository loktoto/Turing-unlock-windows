# Host-only platform state model

`host/platform_model.h` and `.c` define synthetic ownership bookkeeping. They do
not call PCI I/O, GOP, EFI, firmware, FLR or allocator APIs. No pointer is
dereferenced, DMA mapping is created, GPU register is written or boot option is
changed. The model is not a TU102 hardware adapter or evidence of hardware safety.

Initialization records **unknown firmware and GOP ownership**. A no-unlock mock
handoff requires explicit quiescence, a complete GOP ownership inventory summarized
as clear, and no outstanding DMA. `TU_PM_GOP_CLEAR` means the supplied mock graph
has no unresolved target ownership; `TARGET_HELD`, `UNKNOWN` or an incomplete graph
blocks handoff. The model consumes that summary; it does not enumerate or validate
an actual handle graph.

Synthetic DMA bookkeeping stores a CPU token, a separate device address, requested
length and mapped length. A device address of zero is valid. Zero lengths, lengths
larger than requested and device-address range overflow are rejected. A partial
mapping is tracked and releasable but cannot start firmware. Only one outstanding
buffer is modeled; this is not a complete future multiple-buffer allocator API.

The normal mock execution is:

1. Record quiescence and complete, clear GOP ownership in `READY`.
2. Record a complete mapping, then start: firmware becomes `ACTIVE`, DMA `RETAINED`,
   and the prestart GOP observation is invalidated.
3. Record successful quiescence: DMA becomes `RELEASABLE`.
4. Release DMA, refresh complete and clear GOP evidence, mark success, and
   explicitly hand off.

Release requires quiescent firmware and releasable DMA. Active or uncertain firmware
retains the mapping and blocks handoff. Marking success requires the running phase,
quiescence, released DMA and clear GOP evidence. Every mutation after handoff is
rejected. Queries and rejected transitions have no side effects.

Every simulated operation accepting an outcome supports success, failure known to
have no effect, and failure with uncertain effects. Failures enter `CONTAINED` and
block normal progress. An uncertain result invalidates firmware/GOP evidence and
retains any possibly visible DMA, including an uncertain mapping attempt. A known
no-effect failure preserves existing resource knowledge but still requires recovery.

Recovery must be entered explicitly. It requires quiescence before releasing
retained DMA, refreshed complete GOP evidence, and an explicit finish-recovery
transition before handoff. Another recovery failure returns to containment.
The caller must not use reinitialization as recovery for a live adapter; initialization
is for a new synthetic model and carries no containment proof.

Reset requires an explicit prestart or recovery policy. Prestart reset requires
quiescence and no DMA; recovery reset requires the recovery phase. A successful
mock reset deliberately leaves firmware/GOP evidence unknown, keeps existing DMA
retained, and requires separate quiescence and GOP evidence. Both policies reject
reset after success. There is no unconditional successful-post-unlock FLR path.

These are caller-supplied synthetic facts. The public struct is bookkeeping, not
an authenticated capability: direct field writes or false observations can bypass
the contract. A future adapter must independently establish hardware quiescence,
GOP ownership and mapped lengths; define reset effects for the precise profile;
and preserve containment across errors. Reset success alone is not that evidence.

`tests/test_platform_model.c` covers successful and no-unlock paths, unknown/held/
incomplete GOP evidence, active DMA retention, partial mappings, address overflow,
invalid transitions and arguments, both failure types at map/start/quiesce/release,
reset outcomes under both policies, and explicit recovery. All outcomes are
injected values; these tests cannot establish real firmware behavior, DMA safety,
display restoration, Windows bootability or target compatibility.
