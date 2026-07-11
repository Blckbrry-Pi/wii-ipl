#ifndef ATERM_H
#define ATERM_H

#include <revolution/types.h>

#include <revolution/soex.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ATERMConfigState {
    ATERM_STATE_IDLE = 1,                 // 1
    ATERM_STATE_FOUND_ROUTER,             // 2
    ATERM_STATE_CONFIRMED_ROUTER,         // 3
    ATERM_STATE_ESTABLISHING_ENCRYPTION,  // 4
    ATERM_STATE_TRANSFERRING_KEYS,        // 5
    ATERM_STATE_DONE_OK,                  // 6
    ATERM_STATE_DONE_ERR,                 // 7
} ATERMConfigState;

typedef struct {
    ATERMConfigState state;  // 0x00
    u32 timeDiff;            // 0x04
    u32 isDone;              // 0x08
} ATERMStateUpdate;

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

int ATERMi_ApConfigStart(u32, u32, void (*)(ATERMStateUpdate*), ATERMiAllocFn alloc, ATERMiFreeFn free, u32 stackSize);
int ATERMi_ApConfigEnd();

int ATERMi_ApConfigGetState(ATERMStateUpdate* buf);
int ATERMi_ApConfigGetResult(ATERMiApConfigResult* result);
u32 ATERMi_ApConfigGetVersion();

#ifdef __cplusplus
}
#endif

#endif  // ATERM_H
