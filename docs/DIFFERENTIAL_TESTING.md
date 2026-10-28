# Offline upstream differential check

Run from the repository root with Python 3, Git and a GCC/Clang-compatible C11 compiler
on a little-endian Linux host (the script itself resolves sources relative to its location):

```sh
python3 scripts/check_upstream_differential.py /path/to/turing-upstream --cc cc
```

The checkout must have HEAD at `417f333263ff169e8f449c931b70db9dfd7b8ef4`. Before compiling, the script checks the SHA-256 of each original source listed in `UPSTREAM_LOCK.md` against the files in `kernel/turing_unlock`. A dirty checkout is accepted only if those locked files still match. A different revision, changed locked source, compiler error, failed driver, or differing result produces a nonzero exit status. `--cc` specifies one compiler executable, not a shell command or flags.

The script compiles the same driver separately with the current host `layout.c`, `prep.c`, `boot.c` and `vbios.c` and with their original upstream counterparts, in temporary directories. To build upstream `boot.c` on a host, it generates minimal `linux/firmware.h` and `linux/string.h` shims; the driver adapts its firmware-struct parser signature to the host byte-buffer signature. Upstream's booter parser reads native-endian fields, so this comparison assumes a little-endian host.

The deterministic synthetic corpus covers layout and radix boundaries, ELF section truncations and byte mutations, booter headers and lengths, v4 descriptors, framebuffer readiness values, and V2/V3 FWSEC ROM fixtures, truncations and mutations. It compares return codes and named output fields on success; failure output fields and struct padding are deliberately excluded. In particular, the host booter parser writes its output only on success, while upstream may write it before rejecting a header. Fixed successful layout, radix, ELF and FWSEC examples also check expected values independently of the comparison. No hardware unlock source is compiled, and no firmware is downloaded or used.

The corpus is finite and differential agreement is not proof of correctness, safety, complete firmware equality, or hardware compatibility. Both implementations may share defects, and these tests do not cover removed upstream helpers or device interaction. The ordinary host tests remain useful alongside this check.
