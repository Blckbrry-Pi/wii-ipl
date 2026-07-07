#ifndef ATERM_H
#define ATERM_H

#include <revolution/types.h>

#include <revolution/soex.h>

#ifdef __cplusplus
extern "C" {
#endif

// NOTE: Unsure on the name of this struct, may update later
typedef struct {
    int state;     // 0x00
    u32 timeDiff;  // 0x04
    u32 unk_0x08;  // 0x08
} ATERM_TimestampedState;

typedef struct ATERMiApConfigResult {
    char ssid[0x20];        // 0x00
    s32 mode;               // 0x20
    u32 wepKeyID;           // 0x24
    u8 wepKeys[0x4][0x20];  // 0x28
    char textKeys[0x40];    // 0xa8
} ATERMiApConfigResult;

typedef void* (*ATERMiAllocFn)(u32 size);
typedef void (*ATERMiFreeFn)(void* buf);

void* ATERMi_AutoConfigThread(void*);

int ATERMi_ApConfigStart(u32, u32, void (*)(ATERM_TimestampedState*), ATERMiAllocFn alloc, ATERMiFreeFn free, u32 stackSize);
int ATERMi_ApConfigEnd();

int ATERMi_ApConfigGetState(ATERM_TimestampedState* buf);
int ATERMi_ApConfigGetResult(ATERMiApConfigResult* result);
u32 ATERMi_ApConfigGetVersion();

#ifdef __cplusplus
}
#endif

#endif  // ATERM_H
