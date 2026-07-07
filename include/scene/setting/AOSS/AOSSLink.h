#ifndef AOSSLINK_H
#define AOSSLINK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "scene/setting/AOSS/AOSS.h"

#include <revolution/types.h>

void AOSSi_Cancel();

void* AOSSi_Alloc(u32 size);
void AOSSi_Free(void* buf);

int AOSSi_SetNCDIPAddr(u32 addr, u32 netmask, u32 gateway, u32 dns1, u32 dns2);

int AOSSi_InitLocal(SOAlloc, SOFree);
int AOSSi_EndLocal();

int AOSSi_WLANGetBSSList(AOSSiBSSList** bssListOut);
int AOSSi_WLANConnect(AOSSInfoSSID* in, AOSSiWLANConnectResult* out);

void AOSSi_Sleep(u32 ms);
int AOSSi_Status(int status);

void AOSS_SetCallback(AOSSiCallback cb);

extern BOOL AOSSi_cancel_flag;
#ifdef __cplusplus
}
#endif

#endif  // AOSSLINK_H
