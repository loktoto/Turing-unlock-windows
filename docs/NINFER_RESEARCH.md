# NInfer first-look review — 2026-10-28

These are inference-engine references, not TU102 firmware unlock implementations.
The review reads documentation and selected kernel source; no model was downloaded,
no engine was built or run, and no target-GPU performance was independently measured.
The original Strata objective remains unchanged; NInfer is an additional candidate
for a separately controlled stock baseline, not a replacement already selected.

## Source snapshots

| Project | Commit inspected | Scope |
|---|---|---|
| [mr-september/ninfer-2080ti-22g](https://github.com/mr-september/ninfer-2080ti-22g/tree/8dfa5cc96620b646be4608ffb1246cb89df9a444) | `8dfa5cc96620b646be4608ffb1246cb89df9a444` | Turing-only, dense Qwen3.8-27B on an already modified 22GB RTX 2080 Ti |
| [iamwavecut/ninfer-all](https://github.com/iamwavecut/ninfer-all/tree/71d3f8190772ef7089f8eda4e22371114fd5bbf9) | `71d3f8190772ef7089f8eda4e22371114fd5bbf9` | Wider model/architecture engine, including SM75 |

Key file Git blob IDs at these snapshots (not SHA256 file hashes):

- 2080 fork `README.md`: `56048ec9f4c3c5c4972357b52770c79ce613bbd6`
- 2080 fork `docs/turing-port.md`: `ed630414779f80f528d92f112a380cc2c4c68ed5`
- 2080 fork `src/ops/common/cuda/mma.cuh`: `975b26f41fd98652a87d022902d68d370f35cc87`
- all `docs/gpus.md`: `e6bfa6b18c52c2de6ba913e249430c7d5670c928`
- all `docs/build-windows.md`: `9e0ba5de751c137b615345de735b734e63b2734d`

## Why the 22GB fork is the first stock-baseline candidate

Its [README](https://github.com/mr-september/ninfer-2080ti-22g/blob/8dfa5cc96620b646be4608ffb1246cb89df9a444/README.md)
reports native Windows 11 measurements on one 22GB RTX 2080 Ti, driver 610.88,
CUDA 12.9 and Qwen3.8-27B groupwise-int. The documented installation does not
require an EFI unlocker. This assumes the board already exposes its physical
22GB; it does not modify memory capacity or establish selector/unlock state.

The [performance record](https://github.com/mr-september/ninfer-2080ti-22g/blob/8dfa5cc96620b646be4608ffb1246cb89df9a444/docs/turing-port.md#current-performance)
reports approximately 616 tok/s prefill for 512 tokens, 569 tok/s at 8K and
458 tok/s at 64K. Decode without MTP was about 27 tok/s in the short benchmark.
A real code prompt rose from 26.3 to 47.5 tok/s with MTP; a prose prompt rose
from 26.0 to 28.8. These are author's measurements, not a prediction for the
user's machine and not an unlock A/B. The random-token benchmark's MTP result
was only 29.6 tok/s; acceptance and prompt content materially change the result.

The fork is deliberately narrow: SM75 only, Qwen3.8-27B is the tested model,
BF16/INT8 KV, and MTP tested. Qwen3.6-27B is same-architecture but untested;
35B-A3B has no kernels in this tree; FP8/NVFP4 are not usable Turing paths.
At the documented defaults, device weights occupy 15.4 GiB with MTP and host
embedding/context allocations pin several GiB of RAM. The documented host had
32GB RAM. The user's prior 16GB assumption is unverified; available physical
RAM, desktop VRAM and actual context limits must be measured rather than assumed.

## What the kernel source actually does

The [MMA implementation](https://github.com/mr-september/ninfer-2080ti-22g/blob/8dfa5cc96620b646be4608ffb1246cb89df9a444/src/ops/common/cuda/mma.cuh#L38-L156)
uses native `mma.sync.aligned.m16n8k8...f32.f16.f16.f32`, splitting a larger tile
into two k8 operations. BF16 operands are converted to FP16 and saturated at
±65504; very small values can lose precision. TF32 is emulated with SIMT.
This enables a suitable inference implementation on Turing; it does not add
native BF16 instructions or alter the GPU's hardware issue-rate selectors.

The port notes describe FP32 SIMT decode at 1–4 tokens and tensor-core GEMMs at
5 or more. Therefore a successful FP16→FP32 issue-rate unlock might affect some
prefill/verification routes, but single-token decode may remain bandwidth/SIMT
limited. Neither source nor token rates establish any extra unlock benefit.
Profiling and same-engine stock/unlock A/B are required, with MTP independently
controlled. Tile/occupancy work, INT8 KV and host embeddings are separate changes.

## What ninfer-all adds, and what is not established

Its [GPU matrix](https://github.com/iamwavecut/ninfer-all/blob/71d3f8190772ef7089f8eda4e22371114fd5bbf9/docs/gpus.md#turing-75-build-75)
records an **11GB** RTX 2080 Ti running Bonsai 2 and a 24GB TITAN RTX running
several models. Neither is evidence that a particular 22GB modified 2080 Ti
has been validated. Broader formats, KV compression, per-device route selection
and CPU/disk expert strategies are worth studying, but their performance cannot
be transferred from larger/newer cards or different models.

The [Windows guide](https://github.com/iamwavecut/ninfer-all/blob/71d3f8190772ef7089f8eda4e22371114fd5bbf9/docs/build-windows.md)
reports contributor tests on TITAN RTX and RTX 3060. It distinguishes host compile
checks from native release device-code builds and says release CI has no GPU.
Windows packaging success is not model inference or performance qualification.
WDDM memory accounting and pinned-host-memory budgets are relevant lessons.
Multi-GPU features remain outside this project's single-card scope.

## Validation lessons to retain

- Interleave A/B in the same session and compare improvement with observed variance.
- Keep model hash, quantization, KV format, template, context, prompt and speculative
  decoding settings fixed for a hardware comparison. Comparing two engines or
  different artifacts is a separate experiment.
- Use operator oracles, perplexity and logit comparison: plausible text alone missed
  a documented incorrect kernel port that increased perplexity substantially.
- Pre-register numerical tolerances. The 22GB fork openly documents relaxing a
  first-token tolerance after measurement; that is a limitation of that particular
  acceptance record, not a justification to relax our gates after a failure.
- Validate packaged applications with their intended runtime dependencies and an
  otherwise clean PATH, separately from developer-machine builds.

## Integration decision for now

Both engines are Apache-2.0; this port's derived core is GPL-2.0-only. Do not copy
or link Apache-2.0 engine code into a GPL-2.0-only combined work without an
appropriate separate license grant/review. Independent executable benchmarking
and documenting ideas are the intended boundary. Model/artifact and bundled CUDA
runtime licenses are separate again. No engine code or binaries were imported.

Recommendation: first obtain a stock native-Windows baseline from the specialized
22GB fork, then evaluate ninfer-all as a separate engine if model coverage or memory
formats warrant it. Retain Strata's original G6 track. This is a research proposal,
not authorization to change firmware, boot settings, driver policy or security settings.
