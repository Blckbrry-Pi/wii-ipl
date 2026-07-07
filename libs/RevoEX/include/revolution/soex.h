#ifndef REVOLUTION_SO_EX_H
#define REVOLUTION_SO_EX_H

#include <revolution/types.h>

#include <revolution/so/SOBasic.h>
#include <revolution/so/SOOption.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

typedef void* (*SOAlloc)(u32, s32);
typedef void (*SOFree)(u32, void*, s32);

typedef struct SOWork {
    SOAlloc alloc;  // 0x00
    SOFree free;    // 0x04
    int unk_0x08;   // 0x08
    s32 fd;         // 0x0c
    void* buffer;   // 0x10
    int allocCount;
} SOWork;

typedef struct SOLibraryConfig {
    SOAlloc alloc;  // 0x00
    SOFree free;    // 0x04
} SOLibraryConfig;

int SOInit(SOLibraryConfig* config);

int SOFinish();

int SOStartup();
int SOStartupEx(int timeOut);

int SOCleanup();

SOWork* SOiGetSysWork();

void* SOiAlloc(u32, s32 size);
void* SOiFree(u32, void* buf, s32 size);

int SOiPrepare(u32, int* fd);
int SOiConclude(u32, int err);

int SOiPrepareTempRm(u32, int* fd, BOOL* startupSucceeded);
int SOiConcludeTempRm(u32, int err, BOOL someFlag);

int SOiWaitForDHCPEx(int timeoutMs);

int SOGetLastError();

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // REVOLUTION_SO_EX_H
