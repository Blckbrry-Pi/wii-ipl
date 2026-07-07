#ifndef IPL_USBAP_H
#define IPL_USBAP_H

#include <revolution/types.h>
#include <wchar.h>
// #include <revolution/soex.h>

typedef void (*USBAPRegisterCallbackFn)(int);

typedef struct {
    u8 counter;
    u8 bssids[20][6];
} USBAP_UnkBuffer;

#ifdef __cplusplus
extern "C" {
#endif

BOOL USBAPStartRegistration(u64, u32 priority, BOOL someFlag, const wchar_t* nickname, USBAPRegisterCallbackFn registerCb, u16* resPtr, u8* statePtr);
BOOL USBAPCancelRegistration();
BOOL USBAPIsThreadTerminated();

#ifdef __cplusplus
}
#endif

extern u8* usbapState;

#endif  // IPL_USBAP_H
