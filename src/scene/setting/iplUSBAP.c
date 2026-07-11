#include "scene/setting/iplUSBAP.h"

#include "revolution/os/OSInterrupt.h"
#include "revolution/os/OSMessage.h"
#include "revolution/os/OSThread.h"

#include <revolution/wd.h>

#include <string.h>

static OSThread usbapThread;
static u8 usbapStack[0x1000];
static OSMessageQueue usbapMessageQ;
static wchar_t usbapNickname[10];

static OSMessage usbapMessageQBuf[1];
static USBAPRegisterCallbackFn usbapRegisterCb;
static BOOL usbapSomeConnectFlag;
static u16 usbapChannelBitmap;
static u8* usbapResultBuf;
static u8* usbapBSSIDBuf;
u8* usbapState;

typedef struct {
    u8 counter;
    u8 bssids[20][6];
} USBAPBSSIDBuf;

#ifdef __cplusplus
extern "C" {
#endif

static void* DoRegistration(void*);

#ifdef __cplusplus
}
#endif

BOOL USBAPStartRegistration(u64 __unused, u32 priority, BOOL someFlag, const wchar_t* nickname, USBAPRegisterCallbackFn registerCb, u16* resPtr,
                            u8* statePtr) {
    BOOL level;
    int createThreadRes;

    level = OSDisableInterrupts();
    usbapResultBuf = (u8*)resPtr;
    usbapBSSIDBuf = (u8*)statePtr;
    if (usbapThread.state != OS_THREAD_STATE_UNINITIALIZED && !OSIsThreadTerminated(&usbapThread)) {
        OSRestoreInterrupts(level);
        return FALSE;
    }

    if (priority > 31 || nickname == NULL || registerCb == NULL) {
        OSRestoreInterrupts(level);
        return FALSE;
    }

    usbapSomeConnectFlag = someFlag;
    usbapRegisterCb = registerCb;
    memcpy(usbapNickname, nickname, 0x14);
    OSInitMessageQueue(&usbapMessageQ, usbapMessageQBuf, ARRAY_LENGTH(usbapMessageQBuf));
    createThreadRes = OSCreateThread(&usbapThread, DoRegistration, NULL, usbapStack + sizeof(usbapStack), sizeof(usbapStack), priority, 1);
    if (createThreadRes == TRUE) {
        OSRestoreInterrupts(level);
        while (OSResumeThread(&usbapThread) > 1)
            ;
        return TRUE;
    } else {
        OSRestoreInterrupts(level);
        return FALSE;
    }
}

BOOL USBAPCancelRegistration() {
    // Message value doesn't matter, thread is stopped as soon as ANY message is received
    return OSSendMessage(&usbapMessageQ, (OSMessage)0, 0);
}

BOOL USBAPIsThreadTerminated() {
    return OSIsThreadTerminated(&usbapThread);
}

typedef enum {
    USBAP_REGISTRATION_STATE_WORKING = 0,
    USBAP_REGISTRATION_STATE_DONE_OK = 1,
    USBAP_REGISTRATION_STATE_DONE_ERR = 2,
} USBAPRegistrationResult;

static void* DoRegistration(void* __unused_args) {
    WDScanParam scanParams;
    OSMessage msgBuf;
    BOOL isBad;
    BOOL foundMatchingSSID;
    int i;
    int totalCount;
    WDBssDesc* next;
    int j;
    int state;
    BOOL ret;
    if (WDCheckEnableChannel(&usbapChannelBitmap) != 0) {
        ret = FALSE;
        if (usbapRegisterCb != NULL) {
            usbapRegisterCb(0);
        }
        goto RETURN;
    }

    do {
        OSYieldThread();

        // Check for termination message (any message is a termination message)
        if (OSReceiveMessage(&usbapMessageQ, &msgBuf, 0) == 1) {
            ret = FALSE;
            if (usbapRegisterCb != NULL) {
                usbapRegisterCb(0);
            }
            goto RETURN;
        }

        scanParams.maxChannelTime = 100;
        state = USBAP_REGISTRATION_STATE_WORKING;
        scanParams.channelBit = usbapChannelBitmap;
        memset(scanParams.bssid, 0xff, 6);
        scanParams.type = 0;
        scanParams.ssidLength = 0x20;
        memcpy(scanParams.ssid, "NWCUSBAP", 8);
        scanParams.ssid[8] = 0x00;
        scanParams.ssid[9] = 0x01;
        if (usbapSomeConnectFlag == 1)
            scanParams.ssid[9] |= 0x02;

        scanParams.ssid[10] = 0x01;
        scanParams.ssid[11] = 0x00;

        memcpy(scanParams.ssid + 0xc, &usbapNickname, 0x14);

        memset(scanParams.ssidMask, 0, 8);
        memset(scanParams.ssidMask + 8, 0xff, 0x18);

        if (WDScanOnce((u8*)usbapResultBuf, 0x800, &scanParams) != 0)
            continue;

        totalCount = *(u16*)usbapResultBuf;
        foundMatchingSSID = FALSE;
        isBad = FALSE;
        next = (WDBssDesc*)(usbapResultBuf + 2);
        for (i = 0; i < totalCount; next = (WDBssDesc*)((u16*)next + next->length), i++) {
            if (next->ssidLength != 0x20)
                continue;
            if (strncmp((const char*)next->ssid, "NWCUSBAP", 8) != 0)
                continue;

            for (j = 0; j < 20; j++) {
                if (memcmp(next->bssid, offsetof(USBAPBSSIDBuf, bssids[j]) + usbapBSSIDBuf, 6) == 0)
                    break;
            }
            if (j != 20)
                continue;

            if (next->ssid[9] == 0x01) {
                memcpy(usbapBSSIDBuf + offsetof(USBAPBSSIDBuf, bssids[*usbapBSSIDBuf]), next->bssid, 6);
                foundMatchingSSID = TRUE;
                (*usbapBSSIDBuf)++;
            } else if (next->ssid[9] == 0x00) {
                isBad = TRUE;
            }
        }
        if (foundMatchingSSID) {
            state = !!isBad + USBAP_REGISTRATION_STATE_DONE_OK;
        }
    } while (state == USBAP_REGISTRATION_STATE_WORKING);

    ret = state == USBAP_REGISTRATION_STATE_DONE_OK;
    if (usbapRegisterCb != NULL) {
        usbapRegisterCb(ret);
    }
RETURN:
    return (void*)ret;
}
