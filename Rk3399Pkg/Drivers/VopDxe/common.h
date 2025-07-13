#ifndef COMMON_H
#define COMMON_H

#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Library/TimerLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>

#include "types.h"
#include "errno.h"

#define debug(x, ...) DEBUG((EFI_D_ERROR, x, ## __VA_ARGS__))
#define printf(x, ...) DEBUG((EFI_D_INFO, x, ## __VA_ARGS__))

#define clrbits_le32(where, val) MmioAnd32((UINTN) (where), ~(val))
#define setbits_le32(where, val) MmioOr32((UINTN) (where), (val))
#define clrsetbits_le32(where, clear, set) clrbits_le32(where, clear); setbits_le32(where, set)

#define writel(val, where) MmioWrite32((UINTN) (where), (val))
#define writeb(val, where) MmioWrite8((UINTN) (where), (val))
#define readb(where) MmioRead8((UINTN) (where))
#define readl(where) MmioRead32((UINTN) (where))

#define NS_TO_MS(x) ((x) / 1000000)

#define get_timer(base) (NS_TO_MS(GetTimeInNanoSecond (GetPerformanceCounter ())) - base)

#define udelay(x) MicroSecondDelay(x)

#define memcpy(dest, src, n) CopyMem(dest, src, n)
#define memcmp(a, b, n) CompareMem(a, b, n)
#define strcmp(a, b) AsciiStrCmp(a, b)
#define strlen(a) AsciiStrLen(a)

#define isspace(a) (a == ' ' || a == '\t' || a == '\n' || a == '\v' || a == '\f' || a == '\r')

#define offsetof __builtin_offsetof

#define check_member(structure, member, offset) _Static_assert( \
                                                               offsetof(struct structure, member) == offset, \
                                                               "`struct " #structure "` offset for `" #member "` is not " #offset)

#define STR_FMT "%a"

#define BIT(x) (1 << x)

#endif /* COMMON_H */
