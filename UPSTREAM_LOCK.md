# Upstream lock

Source: https://github.com/nlzy/turing-unlock

Commit: `417f333263ff169e8f449c931b70db9dfd7b8ef4` (checked out successfully). GPL-2.0-only.

Original source SHA256 (before local edits):

- `layout.c`: `498922d0f1d5f9fa7c1e981436b6480f23f00a8b6aaae5ed29924af7bd16744f`
- `layout.h`: `e90def31eb95b6efd086f0a3126fe7bbe9d1bb53d03ee67bef1dd2c5b219ba3d`
- `vbios.c`: `1b8bb2efab9fbe55bdc64994d38bee1e380a1b88f9c4ba16b4eded6e12ca3402`
- `vbios.h`: `0ecd2d5b88fc6edcffe45d7b85cd9d22d54ae1f7a4089f5ed86f65a12e1ced35`
- `prep.c`: `35e95aff51e17dee4db42ac0b444688d9a2d2aa202d1fb54a5ce915dfe62da40`
- `prep.h`: `3977ae3d02f5e4a978d4cecdec29990fffe6a8649e5800c4218e0ef1e34669f6`
- `boot.c`: `db317f58100759c94d0532cb65466259f08ae10093135c9d4762b2d5e94334bb`
- `boot.h`: `fc5525966a12ee34233e19de0c43edcd95ab784b8222886ab2f954ffc316bf6a`
- `diagnostic.h`: `f711f36dab46764aff6763b547161c28b23f0fca033bf35b6dcff206a28acf93`

Platform reference only (no code copied): https://github.com/impacifist/Windows-GPU-UNLOCK
Commit: `5c98f11eaafdcb7a2a06fb14fcdcde7297eb409e`.

- `unlock/port.c`: `0b3b65248c6d1f2d95d11c862fc285e7a1d6e7ed3949bdbd9dd78f109111f612`
- `unlock/app.c`: `21e78a732064149242cb1e07b9b5545e2f4fa19635fcfe52ac8b67cadd529ef9`

Build dependencies: C11 compiler, CMake >=3.16, CTest. No firmware downloads or binary dependencies. Other handover repositories are not implementation inputs in this revision.
