# API review and port boundary

| Pinned TU102 source | Host API / decision | Future platform dependency |
|---|---|---|
| vbios.c | turing_unlock_vbios_fwsec retained; parses bounded bytes, does not authenticate firmware | ROM acquisition and hash/profile validation |
| layout.c | turing_unlock_layout retained unchanged | Supply independently verified framebuffer limits |
| prep.c | ELF section parser and radix size planner retained | No allocation or device addresses generated here |
| prep.c | booter patch, radix page and WPR metadata packing excluded | Deferred audited firmware preparation |
| vbios.c | FRTS image patch excluded | Deferred firmware preparation |
| boot.c | firmware object replaced by explicit byte pointer + size; geometry validation retained | No request_firmware dependency |
| boot.c | descriptor v4 and framebuffer readiness predicate retained | Predicate alone cannot establish safe state |

Local edits: unified fixed-width types and errno compatibility for MSVC; removed payload
patch/packing routines; no Linux headers in compiled paths. Booter parser now decodes explicit little-endian fields and commits output only on success.
The intended tested platform remains x64.
Output values must only be consumed after return == 0; some upstream parsers partially
populate outputs before rejecting. Geometry validation is not signature/hash verification.

Windows reference `port.c` supplies GuDmaAllocate/Free (PCI I/O mapped common buffers),
GuConfigRead/Write, GuEnableMaster/ClearMaster, GuSavePci/RestorePci and GuFlr.
`app.c` checks identity, BARs, PROM and GOP before arming, and tracks started/quiesced,
retained DMA and containment before handoff. These are architecture references, not TU102
compatibility evidence. No Windows reference implementation is linked into this library.

Proposed platform contract (not implemented): PCI identity/config read and ROM snapshot
return bounded data plus status; GOP inventory returns complete ownership graph; DMA mapping
tracks CPU address, device address, mapped length and ownership independently. Release is
legal only after proven firmware quiescence. Reset requires explicit state/policy, never
unconditional successful-post-unlock FLR. Unknown ownership or retained DMA prevents chainload.
A pure lifecycle model and failure injection now exist in platform_model.c; real PCI/DMA/GOP adapters remain deferred.
