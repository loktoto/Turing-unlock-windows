/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef TU_HOST_TYPES_H
#define TU_HOST_TYPES_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <errno.h>
#ifndef EOPNOTSUPP
#define EOPNOTSUPP 95
#endif
#ifndef EOVERFLOW
#define EOVERFLOW 75
#endif
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
#endif
