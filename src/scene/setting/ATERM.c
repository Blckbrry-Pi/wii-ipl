#include "scene/setting/ATERM.h"

#include <revolution/ncd.h>
#include <revolution/os.h>

#include <private/wd.h>
#include <revolution/wd.h>

#include <string.h>

typedef struct {
    NCDIpConfig ipConf;  // 0x000
    NCDIfConfig ifConf;  // 0x924
} ATERMiNCDIConf;

typedef struct {
    u16 flag;
    u16 len;
    u8 data[];
} ATERMiMessageChunk;

typedef struct {
    u16 size;        // 0x00
    u8 unk_0x02[6];  // 0x02, Padding I think
    union {
        u8 data[0x7f8];                // 0x08
        ATERMiMessageChunk dataChunk;  // 0x08
    };
} ATERMiBuildMessageBuf;

typedef enum {
    ATERM_MODE_NONE = 0,
    ATERM_MODE_WEP,  // Includes WEP40, WEP104, and WEP2 (WEP128)
    ATERM_MODE_WPA,
    ATERM_MODE_WPA2,
} ATERMiSecurityMode;

typedef union {
    u8 raw[0x28];  // 0x00
    struct {
        u32 isRawBinary;  // 0x00
        u32 unk_0x04;     // 0x04
        char hex[0x20];   // 0x08
    };
} ATERMiWorkingWepKeyInfo;

typedef struct {
    u8 unk_0x000[0x100];                 // 0x000
    char ssid[0x20];                     // 0x100
    u8 unk_0x120[0xc];                   // 0x120
    ATERMiSecurityMode mode;             // 0x12c
    u32 wepKeyID;                        // 0x130
    u8 unk_0x134[0x28];                  // 0x134
    ATERMiWorkingWepKeyInfo wepKeys[4];  // 0x15c
    u8 wpaKey[0x5C];                     // 0x1fc
} ATERMiRecvdSecurityData;

OSThread AtermThread;
u8 ATERMi_decryptionAesKey[0x20];
ATERMiBuildMessageBuf ATERMi_msgBuf;
u8 ATERMi_transmitMsgBuf[0x800];
u8 ATERMi_aesKeyA[0x10];
ATERMiApConfigResult ATERMi_configResult;
ATERMiRecvdSecurityData ATERMi_securityData;
char ATERMi_SSID[0x24];
NCDIfConfig ATERMi_ifConf;
NCDIpConfig ATERMi_ipConf;

BOOL ATERMi_cancelled;
u8 ATERMi_BSSID[6];
int ATERMi_configState;
u8 ATERMi_mac[6];
int ATERMi_81698CC4;
u8 ATERMi_81698CC0;
int ATERMi_msgLen;
u32 ATERMi_builtMsgLen;
BOOL ATERMi_ncdInterfaceDecided;
BOOL ATERMi_socketStarted;
u32 ATERMi_routerIdx;
u32 ATERMi_threadIsRunning;
void* ATERMi_threadStack;
ATERMiFreeFn ATERMi_free;
ATERMiAllocFn ATERMi_alloc;
void (*ATERMi_progressCb)(ATERMStateUpdate*);
s32 ATERMi_81698C94;
s32 ATERMi_state;

#define ADDR_32(A, B, C, D) (((u8)(A) << 0x18) | ((u8)(B) << 0x10) | ((u8)(C) << 0x08) | ((u8)(D) << 0x00))

u32 ATERMi_someTimeMs = -1;                        // .sdata, lbl_81697218
s32 ATERMi_maxAPs = 0x40;                          // .sdata, lbl_8169721C
u32 ATERMi_stackSize = 0x800;                      // .sdata, lbl_81697220
u32 ATERMi_ipAddr = ADDR_32(192, 168, 0, 176);     // .sdata, lbl_81697224
u32 ATERMi_ipNetmask = ADDR_32(255, 255, 255, 0);  // .sdata, lbl_81697228
u32 ATERMi_ipGateway = ADDR_32(192, 168, 0, 1);    // .sdata, lbl_8169722C
u32 ATERMi_ipDns1 = ADDR_32(192, 168, 0, 1);       // .sdata, lbl_81697230
u32 ATERMi_ipDns2 = ADDR_32(192, 168, 0, 1);       // .sdata, lbl_81697234
char ATERMi_obscuredPass[] = "******";
ATERMiMessageChunk* ATERMi_outMessageData = &ATERMi_msgBuf.dataChunk;  // .sdata, lbl_81697244

typedef struct {
    u8 w0[4];  // 0x00
    u8 w1[4];  // 0x04
    u8 w2[4];  // 0x08
    u8 w3[4];  // 0x0c
    u8 w4[4];  // 0x10
    u8 w5[4];  // 0x14
    u8 w6[4];  // 0x18
    u8 w7[4];  // 0x1c
} AESKey;

BOOL ATERMi_AESEncrypt(void* ciphertext, void* plaintext, u32 len, AESKey* key, u32 bytes);
BOOL ATERMi_AESDecrypt(void* plaintext, void* ciphertext, u32 len, AESKey* key, u32 bytes);

typedef struct {
    u32 a0;                 // 0x00
    u32 b0;                 // 0x04
    u32 c0;                 // 0x08
    u32 d0;                 // 0x0c
    u32 bitCntLo;           // 0x10
    u32 bitCntHi;           // 0x14
    u8 partialBlock[0x40];  // 0x18
} ATERMiMd5State;
void ATERMi_MD5(ATERMiMd5State* ctx, void* data, u32 dataLen, u8* digest);
void ATERMi_MD5Init(ATERMiMd5State* ctx);
void ATERMi_MD5Update(ATERMiMd5State* ctx, u8* newData, u32 newDataLen);
void ATERMi_MD5GetDigest(ATERMiMd5State* ctx, void* digest);
void ATERMi_MD5ProcessBlock(ATERMiMd5State* ctx, u8* chunk);
extern u8 ATERMi_md5Pad[0x40];

// Sleep milliseconds alarm handler
void ATERMi_WaitAlarmHandler(OSAlarm* alarm, OSContext* context);

inline void ATERMi_Wait(u32 ms) {
    OSMessage msgBufA[1];
    OSMessage msgDstA;
    OSMessageQueue queueA;
    OSAlarm alarmA;

    OSInitMessageQueue(&queueA, msgBufA, 1);
    OSCreateAlarm(&alarmA);
    OSSetAlarmTag(&alarmA, (u32)&queueA);
    OSSetAlarm(&alarmA, OSMillisecondsToTicks(ms), ATERMi_WaitAlarmHandler);
    OSReceiveMessage(&queueA, &msgDstA, 1);
}

inline void ATERMi_getStateInternal(ATERMStateUpdate* state) {
    state->state = ATERMi_state;
    if (ATERMi_someTimeMs == -1) {
        state->timeDiff = -1;
    } else {
        state->timeDiff = ATERMi_someTimeMs - (int)OSTicksToMilliseconds(OSGetTime());
    }
    state->isDone = ATERMi_81698C94;
}
inline void ATERMi_updateProgress(u32 newState, u32 newTime) {
    ATERMStateUpdate progress;

    ATERMi_state = newState;
    ATERMi_someTimeMs = newTime;

    progress.state = ATERMi_state;
    if (ATERMi_someTimeMs == -1) {
        progress.timeDiff = -1;
    } else {
        progress.timeDiff = ATERMi_someTimeMs - (int)OSTicksToMilliseconds(OSGetTime());
    }
    progress.isDone = ATERMi_81698C94;
    (ATERMi_progressCb)(&progress);
}

inline void ATERMi_SOSend(int s, void* buf, u32 len) {
    SOSockAddrIn sendDest;
    sendDest.len = 8;
    sendDest.family = 2;
    sendDest.addr.addr = 0xffffffff;
    sendDest.port = SOHtoNs(0xe601);
    SOSendTo(s, ATERMi_transmitMsgBuf, len, 0, (SOSockAddr*)&sendDest);
}

inline void ATERMi_SetupIpconf(NCDIpConfig* ipConf) {
    ipConf->adjust.maxTransferUnit = 0x514;
    ipConf->adjust.tcpRetransTimeout = 100;
    ipConf->adjust.dhcpRetransCount = 4;
    ipConf->useDhcp = FALSE;
    ipConf->useProxy = 0;
    memcpy(ipConf->ip.addr, &ATERMi_ipAddr, 4);
    memcpy(ipConf->ip.netmask, &ATERMi_ipNetmask, 4);
    memcpy(ipConf->ip.gateway, &ATERMi_ipGateway, 4);
    memcpy(ipConf->ip.dns1, &ATERMi_ipDns1, 4);
    memcpy(ipConf->ip.dns2, &ATERMi_ipDns2, 4);
    NCDSetIpConfig(ipConf);
}
inline NCDErr ATERMi_SetupIfconf(NCDIfConfig* ifConf) {
    ifConf->selectedMedia = 1;
    ifConf->netif.wireless.rateset = 0;
    ifConf->netif.wireless.configMethod = 0;
    ifConf->netif.wireless.config.manual.privacy.mode = 0;
    ifConf->netif.wireless.config.manual.ssidLength = strlen(ATERMi_SSID);
    memcpy(ifConf->netif.wireless.config.manual.ssid, &ATERMi_SSID, 0x20);
    return NCDSetIfConfig(ifConf);
}

inline NCDErr ATERMi_SetupIconf() {
    memset(&ATERMi_ipConf, 0, sizeof(ATERMi_ipConf));
    ATERMi_SetupIpconf(&ATERMi_ipConf);

    memset(&ATERMi_ifConf, 0, sizeof(ATERMi_ifConf));
    return ATERMi_SetupIfconf(&ATERMi_ifConf);
}

int ATERMi_SetupNCDInterface(void) {
    int ret;
    int isDecidedCnt;

    isDecidedCnt = 0;
    if (ATERMi_SetupIconf() != 0) {
        return -2;
    }
    while (NCDIsInterfaceDecided() == FALSE) {
        if (isDecidedCnt > 500)
            return -2;
        isDecidedCnt++;
        ATERMi_Wait(10);
    }
    ret = 1;
    ATERMi_ncdInterfaceDecided = 1;
    if (SOStartup() == 0) {
        ATERMi_socketStarted = 1;
    }
    if (ATERMi_socketStarted != 0) {
        while (SOGetHostID() == SOHtoNl(0)) {
            ATERMi_Wait(10);
        }
    } else {
        ret = -2;
    }
    return ret;
}

int ATERMi_WDScan(void* scanData, u32 scanDataLen) {
    int ncdLockId;
    int ret;

    u8* scanDataPtr = scanData;
    int startupCnt, scanCnt, cleanupCnt, unlockCnt;
    int scanResult;

    u8 macaddrUnused[6];
    WDScanParam scanParams ALIGN32;
    WD_Info wdInfo;

    ret = -2;

    startupCnt = 0;
    scanCnt = 0;
    cleanupCnt = 0;
    unlockCnt = 0;
    ncdLockId = NCDLockWirelessDriver();
    if (ncdLockId <= 0) {
        goto RETURN;
    }
    while (TRUE) {
        if (WD_Startup(3) == 0)  // open RW
            break;

        if (startupCnt > 10)
            goto UNLOCK;
        startupCnt++;
        ATERMi_Wait(10);
    }

    WD_GetInfo(&wdInfo);
    memcpy(macaddrUnused, wdInfo.MAC, 6);

    scanParams.channelBit = wdInfo.enableChannel;
    scanParams.maxChannelTime = 30;
    memset(scanParams.bssid, 0xff, 6);
    scanParams.type = 0;
    scanParams.ssidLength = 0;
    memset(scanParams.ssid, 0, 0x20);
    memset(scanParams.ssidMask, 0xff, 0x20);

    while (TRUE) {
        if (ATERMi_cancelled) {
            ret = -8;
            goto CLEANUP;
        }
        scanResult = WD_Scan(&scanParams, scanDataPtr, (u16)scanDataLen);
        if (scanResult != 0 && scanResult != -0x7fff7ffc)
            break;

        if (*(u16*)scanDataPtr != 0) {
            ret = *(u16*)scanDataPtr;
            goto CLEANUP;
        }

        scanCnt++;
        if (scanCnt > 10) {
            ret = 0;
            goto CLEANUP;
        }
        ATERMi_Wait(10);
    }
    if (scanResult == -0x7fff7ffc) {
        ret = -6;
    }

CLEANUP:
    while (WD_Cleanup() != 0) {
        if (cleanupCnt > 10) {
            ret = -2;
            break;
        }
        cleanupCnt++;
        ATERMi_Wait(10);
    }

UNLOCK:
    while (NCDUnlockWirelessDriver(ncdLockId)) {
        if (unlockCnt > 10) {
            ret = -2;
            goto RETURN;
        }
        unlockCnt++;
        ATERMi_Wait(10);
    }

RETURN:
    return ret;
}

typedef struct {
    u32 SSIDLen;         // 0x00
    u8 SSID[0x20];       // 0x04
    u32 unk_0x24;        // 0x24
    u8 BSSID[6];         // 0x28
    u16 unk_capability;  // 0x2e
} ATERMiRouterEntry;
typedef struct {
    u32 count;                 // 0x00
    ATERMiRouterEntry list[];  // 0x04
} ATERMiRouterEntryList;

inline void ATERMi_CrossCheck(ATERMiRouterEntry* newEntry, ATERMiRouterEntry* oldEntriesList, u32* count, BOOL* matchFound) {
    int j;
    char ssidToCheck[0x22] = "";
    BOOL crossMatchFound;

    memcpy(ssidToCheck, newEntry->SSID, 0x20);
    j = 0;
    ssidToCheck[newEntry->SSIDLen] = '\0';
    for (j = 0; j < *count; j++) {
        if (newEntry->SSIDLen == 0)
            break;
        if (newEntry->SSIDLen > 0x20)
            break;
        if (newEntry->SSIDLen == 1 && (newEntry->SSID[0] == '\0' || newEntry->SSID[0] == ' '))
            break;

        if (memcmp(ssidToCheck, oldEntriesList[j].SSID, strlen(ssidToCheck)) != 0)
            continue;
        if (memcmp(newEntry->BSSID, oldEntriesList[j].BSSID, 6) != 0)
            continue;

        if (newEntry->unk_capability == oldEntriesList[j].unk_capability)
            continue;
        if (newEntry->unk_capability != 0)
            continue;

        *matchFound = TRUE;
        break;
    }
}

inline int ATERMi_IndividualCheck(ATERMiRouterEntry* newEntries, ATERMiRouterEntry* oldEntries, u32* countNew, u32* countOld, BOOL* matchFound) {
    int i;
    char ssidBuf[0x22] = "";
    BOOL matchInNewEntries, matchInOldEntries;
    matchInOldEntries = FALSE;
    matchInNewEntries = FALSE;
    for (i = 0; i < *countNew; i++) {
        memcpy(ssidBuf, newEntries[i].SSID, 0x20);
        ssidBuf[newEntries[i].SSIDLen] = '\0';
        if (memcmp(ssidBuf, ATERMi_obscuredPass, strlen(ATERMi_obscuredPass)) == 0 && newEntries[i].unk_capability == 0) {
            matchInNewEntries = TRUE;
            break;
        }
    }
    for (i = 0; i < *countOld; i++) {
        memcpy(ssidBuf, oldEntries[i].SSID, 0x20);
        ssidBuf[oldEntries[i].SSIDLen] = '\0';
        if (strlen(ssidBuf) == strlen(ATERMi_obscuredPass)) {
            if (memcmp(ssidBuf, ATERMi_obscuredPass, strlen(ATERMi_obscuredPass)) == 0 && oldEntries[i].unk_capability == 0) {
                matchInOldEntries = TRUE;
                break;
            }
        }
    }
    if (matchInOldEntries && !matchInNewEntries) {
        *matchFound = TRUE;
    }
    return i;
}

BOOL ATERMi_GetATERMRouterIdx(ATERMiRouterEntryList* newEntries, ATERMiRouterEntryList* oldEntries, u32* atermRouterIdx) {
    ATERMiRouterEntry* oldEntryList;
    BOOL ret;
    u32 i;
    ATERMiRouterEntry* newEntryPtr;
    BOOL matchFound;

    ret = FALSE;
    matchFound = FALSE;
    newEntryPtr = newEntries->list;

    i = 0;
    oldEntryList = oldEntries->list;
    while (i < newEntries->count) {
        ATERMi_CrossCheck(newEntryPtr, oldEntryList, &oldEntries->count, &matchFound);
        if (matchFound)
            break;

        newEntryPtr++;
        oldEntryList = oldEntries->list;
        i++;
    }
    if (!matchFound) {
        oldEntryList = newEntries->list;
        newEntryPtr = oldEntries->list;
        i = ATERMi_IndividualCheck(newEntryPtr, oldEntryList, &oldEntries->count, &newEntries->count, &matchFound);
    }
    if (matchFound) {
        *atermRouterIdx = i;
        ret = TRUE;
    }
    return ret;
}

u32 ATERMi_toHex(u8 byte, char* str) {
    int j;
    char* curr;

    int currHexDigit;

    curr = str;

    currHexDigit = (byte >> 4) & 0xf;
    for (j = 0; j < 2; j++) {
        if (currHexDigit <= 9) {
            *curr++ = '0' + currHexDigit;
        } else {
            *curr++ = currHexDigit - 10 + 'A';
        }
        currHexDigit = (byte >> 0) & 0xf;
    }
    *curr = '\0';
    return curr - str;
}
void ATERMi_formatMacAddr(u8* bytesIn, char* hexOut) {
    u8 byte;
    char* currOut;
    int i;

    currOut = hexOut;
    for (i = 0; i < 6; i++) {
        currOut += ATERMi_toHex(*bytesIn++, currOut);
        if (i < 5)
            *currOut++ = ':';
    }
    *currOut = '\0';
}

typedef struct {
    u16 count;
    WDBssDesc bssDescs[];
} ATERMiAAAAAAAA;

int ATERMi_ScanForATERM() {
    WDBssDesc* firstBssDesc;
    int i;
    s32 res;
    s32 count;
    int j;
    u32 size;
    ATERMiRouterEntryList* newEntries;
    ATERMiRouterEntryList* oldEntries;
    WDBssDesc* currBssDesc;
    ATERMiAAAAAAAA* scanData;
    int scanSize;
    u32 routerIdx;
    ATERMStateUpdate progressState;
    char bssidHexStr[32];
    void* scanAllocation;

    ATERMiRouterEntry* atermEntry;

    res = -1;
    routerIdx = 0;
    oldEntries = NULL;
    size = sizeof(ATERMiRouterEntryList) + sizeof(ATERMiRouterEntry) + sizeof(ATERMiRouterEntry) * ATERMi_maxAPs;
    scanAllocation = NULL;

    newEntries = (ATERMiRouterEntryList*)(*ATERMi_alloc)(size);
    if (newEntries != NULL)
        memset(newEntries, 0, size);

    if (newEntries == NULL)
        goto CLEANUP;

    oldEntries = (ATERMiRouterEntryList*)(*ATERMi_alloc)(size);
    if (oldEntries != NULL)
        memset(oldEntries, 0, size);
    if (oldEntries == NULL)
        goto CLEANUP;

    scanSize = ATERMi_maxAPs * 0x100;
    scanAllocation = (void*)(*ATERMi_alloc)(scanSize + 0x40);
    scanData = (ATERMiAAAAAAAA*)ROUNDUP((u64)scanAllocation, 0x20);
    firstBssDesc = scanData->bssDescs;
    i = 0;
    while (i < 300 && !ATERMi_cancelled) {
        if ((u32)OSTicksToMilliseconds(OSGetTime()) >= ATERMi_someTimeMs)
            break;
        count = ATERMi_WDScan(scanData, scanSize);
        if (count < 0) {
            res = count;
            goto CLEANUP;
        }
        if (ATERMi_cancelled)
            break;
        if (count >= ATERMi_maxAPs) {
            res = -6;
            goto CLEANUP;
        }

        currBssDesc = firstBssDesc;
        for (j = 0; j < count; j++) {
            memcpy(newEntries->list[j].SSID, currBssDesc->ssid, 0x20);
            if (currBssDesc->ssidLength > 0x20) {
                newEntries->list[j].SSIDLen = 0;
            } else {
                newEntries->list[j].SSIDLen = (u32)currBssDesc->ssidLength;
            }
            newEntries->list[j].SSID[newEntries->list[j].SSIDLen] = '\0';
            newEntries->list[j].unk_capability = (currBssDesc->capabilities & 0x10) == 0x10;
            memcpy(newEntries->list[j].BSSID, currBssDesc->bssid, 6);
            currBssDesc = (WDBssDesc*)((u16*)currBssDesc + currBssDesc->length);
        }
        newEntries->count = count;

        if (ATERMi_state != 1 && ATERMi_GetATERMRouterIdx(newEntries, oldEntries, &routerIdx)) {
            ATERMi_routerIdx = routerIdx;
            atermEntry = &newEntries->list[ATERMi_routerIdx];
            strcpy(ATERMi_SSID, (char*)atermEntry->SSID);
            memcpy(&ATERMi_BSSID, atermEntry->BSSID, 6);

            ATERMi_formatMacAddr(ATERMi_BSSID, bssidHexStr);
            break;
        }
        memcpy(oldEntries, newEntries, size);

        ATERMi_state = 2;
        ATERMi_getStateInternal(&progressState);
        (*ATERMi_progressCb)(&progressState);
        i++;
    }

    if (i >= 300 || (u32)OSTicksToMilliseconds(OSGetTime()) > ATERMi_someTimeMs) {
        res = -3;
    } else {
        res = 1;
        if (ATERMi_cancelled) {
            res = -8;
        }
    }

CLEANUP:
    if (scanAllocation != NULL) {
        (*ATERMi_free)(scanAllocation);
    }
    if (newEntries != NULL) {
        (*ATERMi_free)(newEntries);
    }
    if (oldEntries != NULL) {
        (*ATERMi_free)(oldEntries);
    }
    return res;
}

typedef struct {
    u16 msgType;
    u16 len;
    u16 unk_0x4;
    u8 dataBuf[];
} ATERMiMessage;

u32 ATERMi_EncryptMessage(ATERMiMessage* output, ATERMiBuildMessageBuf* plaintext, u32 len, AESKey* key) {
    memset(plaintext, 0, 8);
    plaintext->size = SOHtoNs(len - 8);
    if (key != NULL) {
        ATERMi_AESEncrypt(output->dataBuf, plaintext, len, key, 0x10);
        len += 8;
    } else {
        memcpy(output->dataBuf, plaintext, len);
    }
    return len;
}

u32 ATERMi_BuildMessage(ATERMiMessage* output, u32 msgType, ATERMiBuildMessageBuf* plaintext, u32 len, AESKey* key) {
    u8* startPtr;
    u8* endPtr;
    u32 encLen;
    u32 totalLen;
    short checksum;

    checksum = 0;

    encLen = ATERMi_EncryptMessage(output, plaintext, len, key);

    memset(output, 0, offsetof(ATERMiMessage, dataBuf));
    output->msgType = SOHtoNs(msgType);
    output->len = SOHtoNs(encLen);

    endPtr = output->dataBuf;
    endPtr += encLen;
    startPtr = (u8*)output;

    while (startPtr < endPtr) {
        checksum += *startPtr;
        startPtr++;
    }
    *(u16*)endPtr = SOHtoNs(checksum);

    endPtr += 2;
    totalLen = endPtr - (u8*)output;
    return totalLen;
}

inline void ATERMi_Process(u8* dataPtr, u32 len, int* valA, int* valB, int* valC) {
    u8* endPtr;
    ATERMiMessageChunk* startPtr;

    // Weird hack to assign regs
    int chunkType = len;
    int newLen;

    endPtr = dataPtr + len;
    startPtr = (ATERMiMessageChunk*)(dataPtr + 8);

    goto ADVANCE;
    while (TRUE) {
        switch (chunkType) {
            case 1:
                *valA = SONtoHs(*(u16*)dataPtr);
                break;
            case 2:
                *valB = SONtoHs(*(u16*)dataPtr);
                break;
            case 5:
                *valC = SONtoHs(*(u16*)dataPtr);
                break;
        }

    ADVANCE:
        if ((u8*)startPtr >= endPtr) {
            dataPtr = NULL;
        } else {
            chunkType = SONtoHs(startPtr->flag);
            newLen = SONtoHs(startPtr->len);
            dataPtr = startPtr->data;
            startPtr = (ATERMiMessageChunk*)(ROUNDUP(newLen + 4, 8) + (u32)startPtr);
        }
        if (dataPtr == NULL)
            break;
    }
}

BOOL ATERMi_HandleAPSupportInfo(ATERMiMessage* msg, BOOL* param_2) {
    u16 calcedSum;
    int unkFlag;
    u32 len;

    u8* startPtr;
    u8* endPtr;
    u8* dataPtr;

    int valA, valB, valC;

    calcedSum = 0;
    unkFlag = SONtoHs(msg->msgType);
    len = SONtoHs(msg->len);
    endPtr = (u8*)msg + len + 6;

    startPtr = (u8*)msg;
    endPtr = msg->dataBuf + len;

    while (startPtr < endPtr) {
        calcedSum += *startPtr;
        startPtr++;
    }

    if (SONtoHs(*(u16*)endPtr) != calcedSum) {
        dataPtr = NULL;
    } else {
        dataPtr = msg->dataBuf;
    }
    valA = 0;
    valB = 0;
    valC = 0;
    if (dataPtr == NULL) {
        return FALSE;
    }
    if (unkFlag != 1) {
        return FALSE;
    }

    ATERMi_Process(dataPtr, len, &valA, &valB, &valC);
    if (valA != 1 || valB != 1) {
        return FALSE;
    }
    if (valC >= 1) {
        *param_2 = 1;
    } else {
        *param_2 = 0;
    }
    return TRUE;
}

static BOOL ATERMi_81697244 = TRUE;
BOOL ATERMi_GetInitialAESKey(u8* key, SOSockAddr* addr) {
    int i;

    char* currStr;
    u8* currPtr;

    u8 bssid[6];
    u8 macAddr[6];
    char macStr[0x20];
    char bssidStr[0x20];

    memcpy(key + 12, "WARP", 4);
    memcpy(bssid, &ATERMi_BSSID, 6);
    bssid[0] = bssid[0] & 0b11111101;  // Mask out U/L bit
    NCDGetWirelessMacAddress(macAddr);
    memcpy(ATERMi_mac, macAddr, 6);
    i = memcmp(bssid, macAddr, 6);
    if (i <= 0) {
        memcpy(key, macAddr, 6);
        memcpy(key + 6, bssid, 6);
    } else {
        memcpy(key, bssid, 6);
        memcpy(key + 6, macAddr, 6);
    }
    if (ATERMi_81697244) {
        ATERMi_formatMacAddr(macAddr, macStr);
        ATERMi_formatMacAddr(bssid, bssidStr);
    }
    return TRUE;
}

typedef enum {
    ATERM_CHUNK_SSID = 0x201,
    ATERM_CHUNK_SECURITY_MODE = 0x202,
    ATERM_CHUNK_WEP_IS_RAW = 0x203,
    ATERM_CHUNK_UNK_ATTRIB_0x04 = 0x204,
    ATERM_CHUNK_WEP_KEY_ID = 0x205,
    ATERM_CHUNK_WEP_KEY_0_VALUE = 0x206,
    ATERM_CHUNK_WEP_KEY_1_VALUE = 0x207,
    ATERM_CHUNK_WEP_KEY_2_VALUE = 0x208,
    ATERM_CHUNK_WEP_KEY_3_VALUE = 0x209,
    ATERM_CHUNK_WPA_KEY_VALUE = 0x20a,
} ATERMiKeyTransferChunkType;

BOOL ATERMi_HandleRecvSecurityKeys(ATERMiBuildMessageBuf* input) {
    ATERMiMessageChunk* startPtr;
    u8* dataPtr = (u8*)input;
    u8* endPtr;

    int chunkType;
    int chunkLen;

    BOOL hasSSID;

    int i;
    char* wepHex;

    u8 hexByte;
    u32 advanced;
    int hiHexChar, loHexChar;

    hasSSID = FALSE;
    startPtr = &input->dataChunk;
    endPtr = (u8*)startPtr + SONtoHs(input->size);
    goto ADVANCE;

    while (TRUE) {
        switch (chunkType) {
            case ATERM_CHUNK_SSID:
                memset(ATERMi_securityData.ssid, 0, 0x20);
                memcpy(ATERMi_securityData.ssid, dataPtr, chunkLen);
                hasSSID = 1;
                break;
            case ATERM_CHUNK_SECURITY_MODE:
                ATERMi_securityData.mode = (s32)SONtoHs(*(u16*)dataPtr);
                break;
            case ATERM_CHUNK_WEP_IS_RAW:
                ATERMi_securityData.wepKeys[0].isRawBinary = SONtoHs(*(u16*)dataPtr);
                ATERMi_securityData.wepKeys[1].isRawBinary = ATERMi_securityData.wepKeys[0].isRawBinary;
                ATERMi_securityData.wepKeys[2].isRawBinary = ATERMi_securityData.wepKeys[0].isRawBinary;
                ATERMi_securityData.wepKeys[3].isRawBinary = ATERMi_securityData.wepKeys[0].isRawBinary;
                break;
            case ATERM_CHUNK_UNK_ATTRIB_0x04:
                ATERMi_securityData.wepKeys[0].unk_0x04 = SONtoHs(*(u16*)dataPtr);
                ATERMi_securityData.wepKeys[1].unk_0x04 = ATERMi_securityData.wepKeys[0].unk_0x04;
                ATERMi_securityData.wepKeys[2].unk_0x04 = ATERMi_securityData.wepKeys[0].unk_0x04;
                ATERMi_securityData.wepKeys[3].unk_0x04 = ATERMi_securityData.wepKeys[0].unk_0x04;
                break;
            case ATERM_CHUNK_WEP_KEY_ID:
                ATERMi_securityData.wepKeyID = SONtoHs(*(u16*)dataPtr);
                break;
            case ATERM_CHUNK_WEP_KEY_0_VALUE:
            case ATERM_CHUNK_WEP_KEY_1_VALUE:
            case ATERM_CHUNK_WEP_KEY_2_VALUE:
            case ATERM_CHUNK_WEP_KEY_3_VALUE:
                wepHex = ATERMi_securityData.wepKeys[chunkType - ATERM_CHUNK_WEP_KEY_0_VALUE].hex;
                memset(wepHex, 0, 0x20);
                if (ATERMi_securityData.wepKeys[0].isRawBinary == 1) {
                    for (i = 0; i < chunkLen; i++) {
                        wepHex += ATERMi_toHex(*dataPtr++, wepHex);
                    }
                } else {
                    memcpy(wepHex, dataPtr, chunkLen);
                }
                break;
            case ATERM_CHUNK_WPA_KEY_VALUE:
                memset(ATERMi_securityData.wpaKey, 0, 0x48);
                memcpy(ATERMi_securityData.wpaKey, dataPtr, chunkLen);
        }

    ADVANCE:
        if ((u8*)startPtr >= endPtr) {
            dataPtr = NULL;
        } else {
            chunkType = SONtoHs(startPtr->flag);
            chunkLen = SONtoHs(startPtr->len);
            dataPtr = startPtr->data;
            startPtr = (ATERMiMessageChunk*)(ROUNDUP(chunkLen + 4, 8) + (u32)startPtr);
        }
        if (dataPtr == NULL)
            break;
    }
    return hasSSID;
}

BOOL ATERMi_ParseHex(u8* val, char* str, int strLen) {
    int inChr;
    int outByte;
    int i;

    outByte = 0;
    for (i = 0; i < strLen; i++, str++) {
        inChr = *str;
        switch (inChr) {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                outByte += inChr - '0';
                break;
            case 'a':
            case 'b':
            case 'c':
            case 'd':
            case 'e':
            case 'f':
                outByte += inChr - 'a' + 10;
                break;
            case 'A':
            case 'B':
            case 'C':
            case 'D':
            case 'E':
            case 'F':
                outByte += inChr - 'A' + 10;
                break;
            default:
                return FALSE;
        }

        if (i % 2 == 0) {
            outByte = outByte << 4;
        } else {
            val[i / 2] = outByte;
            outByte = 0;
        }
    }
    return TRUE;
}

int ATERMi_CreateConfigResult() {
    char* scratchPtr;
    int i;
    int ret;
    char scratch[0x21];

    ret = 1;
    strcpy(ATERMi_configResult.ssid, ATERMi_securityData.ssid);
    switch (ATERMi_securityData.mode) {
        case ATERM_MODE_NONE:
            ATERMi_configResult.mode = NCD_MODE_NONE;
            break;
        case ATERM_MODE_WEP:
            if (ATERMi_securityData.wepKeyID == 0) {
                ret = -7;
                break;
            }
            scratchPtr = scratch;
            ATERMi_configResult.wepKeyID = ATERMi_securityData.wepKeyID;
            i = 0;
            do {
                memcpy(scratch, ATERMi_securityData.wepKeys[i].hex, 0x20);
                scratch[0x20] = '\0';
                switch (strlen(scratch)) {
                    case 0:
                        break;

                    case 0x05:
                        ATERMi_configResult.mode = NCD_MODE_WEP40;
                        memcpy(ATERMi_configResult.wepKeys[i], scratchPtr, 5);
                        break;
                    case 0x05 * 2:
                        ATERMi_configResult.mode = NCD_MODE_WEP40;
                        ATERMi_ParseHex(ATERMi_configResult.wepKeys[i], scratchPtr, 10);
                        break;

                    case 0x0d:
                        ATERMi_configResult.mode = NCD_MODE_WEP104;
                        memcpy(ATERMi_configResult.wepKeys[i], scratchPtr, 0xd);
                        break;
                    case 0x0d * 2:
                        ATERMi_configResult.mode = NCD_MODE_WEP104;
                        ATERMi_ParseHex(ATERMi_configResult.wepKeys[i], scratchPtr, 0xd * 2);
                        break;

                    // Why does it literally only support it here wtf
                    case 0x10:
                        ATERMi_configResult.mode = NCD_MODE_WEP2;
                        memcpy(ATERMi_configResult.wepKeys[i], scratchPtr, 0x10);
                        break;
                    case 0x10 * 2:
                        ATERMi_configResult.mode = NCD_MODE_WEP2;
                        ATERMi_ParseHex(ATERMi_configResult.wepKeys[i], scratchPtr, 0x10 * 2);
                        break;

                    default:
                        ret = -7;
                        break;
                }
                i++;
            } while (i < 4);
            break;
        case ATERM_MODE_WPA:
            ATERMi_configResult.mode = NCD_MODE_WPA_PSK_TKIP;
            memcpy(ATERMi_configResult.textKeys, ATERMi_securityData.wpaKey, 0x40);
            break;
        case ATERM_MODE_WPA2:
            ATERMi_configResult.mode = NCD_MODE_WPA2_PSK_TKIP;
            memcpy(ATERMi_configResult.textKeys, ATERMi_securityData.wpaKey, 0x40);
            break;
        default:
            ret = -7;
            break;
    }
    return ret;
}

inline void* ATERMi_GetDataValidateSum(void* msg, int* flagOut, u32* lenOut) {
    u16 calcedSum;
    u32 expectedSum;
    u8* endPtr;
    u8* startPtr;
    u32 flag;
    u32 len;
    u8* out;

    calcedSum = 0;
    flag = SONtoHs(((u16*)msg)[0]);
    *flagOut = flag;
    len = SONtoHs(((u16*)msg)[1]);
    *lenOut = len;

    endPtr = (len + (u8*)msg) + 6;
    // endPtr += 6;
    startPtr = (u8*)msg;

    while (startPtr < endPtr) {
        calcedSum += *startPtr;
        startPtr++;
    }
    expectedSum = SONtoHs(*(u16*)endPtr);

    out = (u8*)msg;
    out += 6;
    if (calcedSum != (u16)expectedSum)
        out = NULL;
    return out;
}

inline u32 ATERMi_ValidateAndDecrypt(void* outBuf, void* inBuf, int* flagOut, int expectedFlag) {
    void* msgData;
    u32 len;
    void* decryptionKey;
    msgData = ATERMi_GetDataValidateSum(inBuf, flagOut, &len);
    if (msgData == NULL) {
        return 0;
    } else if (*flagOut != expectedFlag) {
        return 0;
    }

    decryptionKey = ATERMi_decryptionAesKey;
    if (decryptionKey != NULL) {
        ATERMi_AESDecrypt(&ATERMi_msgBuf, msgData, len, decryptionKey, 0x10);
        len -= 8;
    } else {
        memcpy(&ATERMi_msgBuf, msgData, len);
    }
    return len;
}

inline ATERMiMessageChunk* ATERMi_AppendChunk(ATERMiMessageChunk* chunk, u16 flag, u16 len, void* data) {
    u32 alignedLen = ROUNDUP(len + 4, 8);
    u32 alignedDataLen = alignedLen - 4;

    memset(chunk, 0, 4);
    chunk->flag = SOHtoNs(flag);
    chunk->len = SOHtoNs(len);
    memset(chunk->data, 0, alignedDataLen);
    memcpy(chunk->data, data, len);
    // return (ATERMiMessageChunk*)((u8*)chunk + alignedLen);
    return (ATERMiMessageChunk*)((u8*)chunk + alignedLen);
}

inline void* ATERMi_BuildMessage5(ATERMiMessageChunk* chunk) {
    ATERMiMessageChunk *curr2, *curr3, *curr4;
    u16 someData = SOHtoNs(1);
    u8 someOtherData[] = {6, 0, 1, 2, 3, 4, 5};

    chunk = ATERMi_AppendChunk(chunk, 1, 2, &someData);
    chunk = ATERMi_AppendChunk(chunk, 2, 2, &someData);
    if (ATERMi_81698CC4 != 0) {
        chunk = ATERMi_AppendChunk(chunk, 5, 2, &someData);
    }
    chunk = ATERMi_AppendChunk(chunk, 3, 7, someOtherData);
    if (ATERMi_81698CC4 != 0) {
        chunk = ATERMi_AppendChunk(chunk, 4, 6, ATERMi_mac);
    }
    return chunk;
}

u32 ATERMi_DoAutoConfig() {
    ATERMiBuildMessageBuf* msgBuf;
    int sockFd;
    int retVal;        // Return Value
    int triesStartMs;  // Some timestamp
    int tries;         // Some counter?

    BOOL done;  // Is Done

    u32 tempLen;
    ATERMiMessageChunk* puVar3;  // Curr message chunk
    ATERMiMessageChunk* curr2;   // Curr message chunk
    ATERMiMessageChunk* curr3;   // Curr message chunk
    ATERMiMessageChunk* curr4;   // Curr message chunk
    void* puVar15;
    void* __src;
    u8* somePtr;
    ATERMiMessage* transmitMsgBuf;
    AESKey* decryptionKey;
    int uVar5;  // Build Message result

    u32 someByte;
    int local_60;

    int nowInMs;

    ATERMiMd5State local_b8;
    ATERMStateUpdate local_118;  // Timestamped state report
    ATERMStateUpdate local_124;  // Timestamped state report
    ATERMStateUpdate local_130;  // Timestamped state report
    SOSockAddrIn local_138;      // Socket startup address
    SOSockAddrIn local_140;      // Socket recv address
    SOSockAddrIn local_150;      // Socket send address
    SOSockAddrIn local_160;
    SOSockAddrIn local_168;
    u32 local_16c;  // Timestamp for md5
    int flag8;
    u32 len8;
    u8 local_148[7];
    u16 local_178;

    ATERMi_configState = 1;
    msgBuf = &ATERMi_msgBuf;
    transmitMsgBuf = (ATERMiMessage*)ATERMi_transmitMsgBuf;
    decryptionKey = (AESKey*)ATERMi_decryptionAesKey;
    sockFd = 0;
    retVal = -5;

    triesStartMs = 0;
    tries = 0;
    done = FALSE;

    while (!done && !ATERMi_cancelled) {
        ATERMi_Wait(500);

        switch (ATERMi_configState) {
            case 1:
                retVal = ATERMi_ScanForATERM();
                if (retVal != 1) {
                    done = TRUE;
                    break;
                }

                ATERMi_state = 3;
                local_118.state = 3;
                if (ATERMi_someTimeMs == -1) {
                    local_118.timeDiff = -1;
                } else {
                    local_118.timeDiff = ATERMi_someTimeMs - (int)OSTicksToMilliseconds(OSGetTime());
                }
                local_118.isDone = ATERMi_81698C94;
                (*ATERMi_progressCb)(&local_118);
                ATERMi_configState = 2;
                break;
            case 2:
                retVal = ATERMi_SetupNCDInterface();
                if (retVal != 1) {
                    done = TRUE;
                    break;
                }

                nowInMs = (u32)OSTicksToMilliseconds(OSGetTime());
                if (ATERMi_someTimeMs < nowInMs + 10000U) {
                    ATERMi_someTimeMs = (int)OSTicksToMilliseconds(OSGetTime()) + 10000;
                }
                ATERMi_configState = 3;
                break;
            case 3:
                sockFd = SOSocket(2, 2, 0);
                if (sockFd < 0) {
                    retVal = -2;
                    done = TRUE;
                } else {
                    memset(&local_138, 0, 8);
                    local_138.len = 8;
                    local_138.family = 2;
                    local_138.port = SOHtoNs(0xe601);
                    local_138.addr.addr = 0;
                    retVal = SOBind(sockFd, (SOSockAddr*)&local_138);
                    if ((int)retVal < 0) {
                        retVal = -2;
                        done = TRUE;
                    } else {
                        ATERMi_configState = 4;
                    }
                }
                break;
            case 4:
                if ((u32)OSTicksToMilliseconds(OSGetTime()) >= ATERMi_someTimeMs) {
                    SOClose(sockFd);
                    retVal = -3;
                    done = TRUE;
                    break;
                }

                local_140.len = 8;
                ATERMi_GetInitialAESKey(ATERMi_aesKeyA, (SOSockAddr*)&local_140);
                if (SORecvFrom(sockFd, ATERMi_transmitMsgBuf, 0x800, 4, (SOSockAddr*)&local_140) <= 0)
                    break;
                if (ATERMi_HandleAPSupportInfo((ATERMiMessage*)ATERMi_transmitMsgBuf, &ATERMi_81698CC4) != FALSE) {
                    ATERMi_someTimeMs = (int)OSTicksToMilliseconds(OSGetTime()) + 30000;
                    ATERMi_configState = 5;
                    ATERMi_state = 4;
                    ATERMi_getStateInternal(&local_124);
                    (*ATERMi_progressCb)(&local_124);
                }
                break;
            case 5:
                // puVar3 = ATERMi_outMessageData;
                uVar5 = ((u8*)ATERMi_BuildMessage5(ATERMi_outMessageData) - (u8*)(ATERMi_outMessageData) + 0x08);
                uVar5 = ATERMi_BuildMessage((ATERMiMessage*)ATERMi_transmitMsgBuf, 2, &ATERMi_msgBuf, uVar5, NULL);
                ATERMi_builtMsgLen = uVar5;
                ATERMi_SOSend(sockFd, ATERMi_transmitMsgBuf, uVar5);

                triesStartMs = OSTicksToMilliseconds(OSGetTime());
                ATERMi_configState = 6;
                break;
            case 6:
                if ((u32)OSTicksToMilliseconds(OSGetTime()) >= ATERMi_someTimeMs) {
                    SOClose(sockFd);
                    retVal = -4;
                    done = TRUE;
                    break;
                }
                if (SORecvFrom(sockFd, ATERMi_transmitMsgBuf, 0x800, 4, (SOSockAddr*)&local_140) > 0) {
                    tempLen = ATERMi_ValidateAndDecrypt(&ATERMi_msgBuf, transmitMsgBuf, &flag8, 3);
                    // puVar15 = ATERMi_GetDataValidateSum(transmitMsgBuf, &flag8, &tempLen);
                    // if (puVar15 == NULL) {
                    //     tempLen = 0;
                    // } else if (flag8 != 3) {
                    //     tempLen = 0;
                    // } else if (ATERMi_decryptionAesKey != NULL) {
                    //     ATERMi_AESDecrypt(&ATERMi_buildMsgBuf, puVar15, tempLen, (AESKey*)ATERMi_decryptionAesKey, 0x10);
                    //     tempLen = tempLen - 8;
                    // } else {
                    //     memcpy(&ATERMi_buildMsgBuf, puVar15, tempLen);
                    // }
                    if (tempLen != 0) {
                        somePtr = ATERMi_msgBuf.data;
                        tempLen = SONtoHs(ATERMi_msgBuf.size);
                        if (somePtr + tempLen <= somePtr) {
                            __src = NULL;
                        } else {
                            local_60 = SONtoHs(ATERMi_msgBuf.dataChunk.flag);
                            SONtoHs(ATERMi_msgBuf.dataChunk.len);
                            __src = ATERMi_msgBuf.dataChunk.data;
                        }
                        if (local_60 == 0x101) {
                            local_16c = OSTicksToMilliseconds(OSGetTime());
                            memcpy(ATERMi_decryptionAesKey, __src, 8);
                            ATERMi_MD5(&local_b8, &local_16c, 4, ATERMi_decryptionAesKey + 8);
                            ATERMi_configState = 7;
                            ATERMi_state = 5;
                            tries = 0;
                            ATERMi_someTimeMs = 0xffffffff;
                            ATERMi_getStateInternal(&local_130);
                            (*ATERMi_progressCb)(&local_130);
                            // local_130.state = 5;
                            // local_130.timeDiff = -1;
                            // local_130.unk_0x08 = ATERMi_81698C94;
                            // (*ATERMi_progressCb)(&local_130);
                        }
                        break;
                    }
                }
                if ((u32)OSTicksToMilliseconds(OSGetTime()) >= triesStartMs + 2000U) {
                    ATERMi_configState = 5;
                }
                break;
            case 7:

                memset(msgBuf, 0, 8);
                curr2 = &msgBuf->dataChunk;
                curr2 = ATERMi_AppendChunk(curr2, 0x102, 8, decryptionKey->w2);
                ATERMi_msgLen = (u8*)curr2 - (u8*)msgBuf;
                msgBuf->size = ATERMi_msgLen - 8;

                uVar5 = ATERMi_BuildMessage((ATERMiMessage*)ATERMi_transmitMsgBuf, 4, msgBuf, ATERMi_msgLen, (AESKey*)ATERMi_aesKeyA);
                ATERMi_builtMsgLen = uVar5;
                ATERMi_SOSend(sockFd, ATERMi_transmitMsgBuf, uVar5);

                triesStartMs = OSTicksToMilliseconds(OSGetTime());
                memset(&ATERMi_securityData, 0, 0x254);
                ATERMi_configState = 8;
                break;
            case 8:
                if (SORecvFrom(sockFd, ATERMi_transmitMsgBuf, 0x800, 4, (SOSockAddr*)&local_140) > 0) {
                    tempLen = ATERMi_ValidateAndDecrypt(&ATERMi_msgBuf, transmitMsgBuf, &flag8, 5);
                    ATERMi_msgLen = tempLen;
                    if ((ATERMi_msgLen != 0) && ATERMi_HandleRecvSecurityKeys(&ATERMi_msgBuf)) {
                        tries = 0;
                        ATERMi_configState = 9;
                        ATERMi_81698CC0 = ATERMi_securityData.ssid[0] != '\0';
                        break;
                    }
                }
                if ((u32)OSTicksToMilliseconds(OSGetTime()) >= triesStartMs + 1000U) {
                    tries++;
                    if (tries >= 10) {
                        SOClose(sockFd);
                        retVal = -2;
                        done = TRUE;
                    } else {
                        ATERMi_configState = 7;
                    }
                }
                break;
            case 9:
                memset(msgBuf, 0, 8);
                curr2 = &msgBuf->dataChunk;
                ATERMi_msgLen = (u8*)ATERMi_AppendChunk(curr2, 0x301, 1, &ATERMi_81698CC0) - (u8*)msgBuf;
                msgBuf->size = ATERMi_msgLen - 8;
                ATERMi_builtMsgLen =
                    ATERMi_BuildMessage((ATERMiMessage*)ATERMi_transmitMsgBuf, 6, msgBuf, ATERMi_msgLen, (AESKey*)ATERMi_decryptionAesKey);

                if (NCDGetLinkStatus() != 5) {
                    triesStartMs = (int)OSTicksToMilliseconds(OSGetTime()) + 1000;
                    tries = 10;
                    ATERMi_configState = 10;
                    break;
                }
                ATERMi_SOSend(sockFd, ATERMi_transmitMsgBuf, ATERMi_builtMsgLen);
                triesStartMs = OSTicksToMilliseconds(OSGetTime());
                ATERMi_configState = 10;
                break;
            case 10:
                if ((u32)OSTicksToMilliseconds(OSGetTime()) >= triesStartMs + 1000U) {
                    tries++;
                    if (tries >= 10) {
                        done = TRUE;
                        retVal = ATERMi_CreateConfigResult();
                    } else {
                        ATERMi_configState = 9;
                    }
                }
        }
    }

    if (sockFd != 0) {
        SOClose(sockFd);
    }
    if (ATERMi_cancelled) {
        retVal = -8;
    }
    return retVal;
}

u32 ATERMi_DoAutoConfig();
void* ATERMi_AutoConfigThread(void* _) {
    u32 newState;
    u32 configureRes;
    ATERMStateUpdate state;

    configureRes = ATERMi_DoAutoConfig();
    ATERMi_81698C94 = configureRes;
    if (ATERMi_ncdInterfaceDecided != 0) {
        ATERMi_ncdInterfaceDecided = 0;
    }
    if (ATERMi_socketStarted != 0) {
        ATERMi_socketStarted = 0;
        SOCleanup();
    }
    configureRes = configureRes == 1 ? 6 : 7;

    ATERMi_someTimeMs = -1;
    ATERMi_state = configureRes;
    ATERMi_getStateInternal(&state);
    (*ATERMi_progressCb)(&state);
    return NULL;
}

// I would just calculate these table values from scratch but I think clangd would murder me
#define FT(V)                                                                                                                                        \
    {                                                                                                                                                \
        V(0x63), V(0x7C), V(0x77), V(0x7B), V(0xF2), V(0x6B), V(0x6F), V(0xC5), V(0x30), V(0x01), V(0x67), V(0x2B), V(0xFE), V(0xD7), V(0xAB),       \
        V(0x76), V(0xCA), V(0x82), V(0xC9), V(0x7D), V(0xFA), V(0x59), V(0x47), V(0xF0), V(0xAD), V(0xD4), V(0xA2), V(0xAF), V(0x9C), V(0xA4),       \
        V(0x72), V(0xC0), V(0xB7), V(0xFD), V(0x93), V(0x26), V(0x36), V(0x3F), V(0xF7), V(0xCC), V(0x34), V(0xA5), V(0xE5), V(0xF1), V(0x71),       \
        V(0xD8), V(0x31), V(0x15), V(0x04), V(0xC7), V(0x23), V(0xC3), V(0x18), V(0x96), V(0x05), V(0x9A), V(0x07), V(0x12), V(0x80), V(0xE2),       \
        V(0xEB), V(0x27), V(0xB2), V(0x75), V(0x09), V(0x83), V(0x2C), V(0x1A), V(0x1B), V(0x6E), V(0x5A), V(0xA0), V(0x52), V(0x3B), V(0xD6),       \
        V(0xB3), V(0x29), V(0xE3), V(0x2F), V(0x84), V(0x53), V(0xD1), V(0x00), V(0xED), V(0x20), V(0xFC), V(0xB1), V(0x5B), V(0x6A), V(0xCB),       \
        V(0xBE), V(0x39), V(0x4A), V(0x4C), V(0x58), V(0xCF), V(0xD0), V(0xEF), V(0xAA), V(0xFB), V(0x43), V(0x4D), V(0x33), V(0x85), V(0x45),       \
        V(0xF9), V(0x02), V(0x7F), V(0x50), V(0x3C), V(0x9F), V(0xA8), V(0x51), V(0xA3), V(0x40), V(0x8F), V(0x92), V(0x9D), V(0x38), V(0xF5),       \
        V(0xBC), V(0xB6), V(0xDA), V(0x21), V(0x10), V(0xFF), V(0xF3), V(0xD2), V(0xCD), V(0x0C), V(0x13), V(0xEC), V(0x5F), V(0x97), V(0x44),       \
        V(0x17), V(0xC4), V(0xA7), V(0x7E), V(0x3D), V(0x64), V(0x5D), V(0x19), V(0x73), V(0x60), V(0x81), V(0x4F), V(0xDC), V(0x22), V(0x2A),       \
        V(0x90), V(0x88), V(0x46), V(0xEE), V(0xB8), V(0x14), V(0xDE), V(0x5E), V(0x0B), V(0xDB), V(0xE0), V(0x32), V(0x3A), V(0x0A), V(0x49),       \
        V(0x06), V(0x24), V(0x5C), V(0xC2), V(0xD3), V(0xAC), V(0x62), V(0x91), V(0x95), V(0xE4), V(0x79), V(0xE7), V(0xC8), V(0x37), V(0x6D),       \
        V(0x8D), V(0xD5), V(0x4E), V(0xA9), V(0x6C), V(0x56), V(0xF4), V(0xEA), V(0x65), V(0x7A), V(0xAE), V(0x08), V(0xBA), V(0x78), V(0x25),       \
        V(0x2E), V(0x1C), V(0xA6), V(0xB4), V(0xC6), V(0xE8), V(0xDD), V(0x74), V(0x1F), V(0x4B), V(0xBD), V(0x8B), V(0x8A), V(0x70), V(0x3E),       \
        V(0xB5), V(0x66), V(0x48), V(0x03), V(0xF6), V(0x0E), V(0x61), V(0x35), V(0x57), V(0xB9), V(0x86), V(0xC1), V(0x1D), V(0x9E), V(0xE1),       \
        V(0xF8), V(0x98), V(0x11), V(0x69), V(0xD9), V(0x8E), V(0x94), V(0x9B), V(0x1E), V(0x87), V(0xE9), V(0xCE), V(0x55), V(0x28), V(0xDF),       \
        V(0x8C), V(0xA1), V(0x89), V(0x0D), V(0xBF), V(0xE6), V(0x42), V(0x68), V(0x41), V(0x99), V(0x2D), V(0x0F), V(0xB0), V(0x54), V(0xBB),       \
        V(0x16),                                                                                                                                     \
    }

#define RT(V)                                                                                                                                        \
    {                                                                                                                                                \
        V(0x52), V(0x09), V(0x6A), V(0xD5), V(0x30), V(0x36), V(0xA5), V(0x38), V(0xBF), V(0x40), V(0xA3), V(0x9E), V(0x81), V(0xF3), V(0xD7),       \
        V(0xFB), V(0x7C), V(0xE3), V(0x39), V(0x82), V(0x9B), V(0x2F), V(0xFF), V(0x87), V(0x34), V(0x8E), V(0x43), V(0x44), V(0xC4), V(0xDE),       \
        V(0xE9), V(0xCB), V(0x54), V(0x7B), V(0x94), V(0x32), V(0xA6), V(0xC2), V(0x23), V(0x3D), V(0xEE), V(0x4C), V(0x95), V(0x0B), V(0x42),       \
        V(0xFA), V(0xC3), V(0x4E), V(0x08), V(0x2E), V(0xA1), V(0x66), V(0x28), V(0xD9), V(0x24), V(0xB2), V(0x76), V(0x5B), V(0xA2), V(0x49),       \
        V(0x6D), V(0x8B), V(0xD1), V(0x25), V(0x72), V(0xF8), V(0xF6), V(0x64), V(0x86), V(0x68), V(0x98), V(0x16), V(0xD4), V(0xA4), V(0x5C),       \
        V(0xCC), V(0x5D), V(0x65), V(0xB6), V(0x92), V(0x6C), V(0x70), V(0x48), V(0x50), V(0xFD), V(0xED), V(0xB9), V(0xDA), V(0x5E), V(0x15),       \
        V(0x46), V(0x57), V(0xA7), V(0x8D), V(0x9D), V(0x84), V(0x90), V(0xD8), V(0xAB), V(0x00), V(0x8C), V(0xBC), V(0xD3), V(0x0A), V(0xF7),       \
        V(0xE4), V(0x58), V(0x05), V(0xB8), V(0xB3), V(0x45), V(0x06), V(0xD0), V(0x2C), V(0x1E), V(0x8F), V(0xCA), V(0x3F), V(0x0F), V(0x02),       \
        V(0xC1), V(0xAF), V(0xBD), V(0x03), V(0x01), V(0x13), V(0x8A), V(0x6B), V(0x3A), V(0x91), V(0x11), V(0x41), V(0x4F), V(0x67), V(0xDC),       \
        V(0xEA), V(0x97), V(0xF2), V(0xCF), V(0xCE), V(0xF0), V(0xB4), V(0xE6), V(0x73), V(0x96), V(0xAC), V(0x74), V(0x22), V(0xE7), V(0xAD),       \
        V(0x35), V(0x85), V(0xE2), V(0xF9), V(0x37), V(0xE8), V(0x1C), V(0x75), V(0xDF), V(0x6E), V(0x47), V(0xF1), V(0x1A), V(0x71), V(0x1D),       \
        V(0x29), V(0xC5), V(0x89), V(0x6F), V(0xB7), V(0x62), V(0x0E), V(0xAA), V(0x18), V(0xBE), V(0x1B), V(0xFC), V(0x56), V(0x3E), V(0x4B),       \
        V(0xC6), V(0xD2), V(0x79), V(0x20), V(0x9A), V(0xDB), V(0xC0), V(0xFE), V(0x78), V(0xCD), V(0x5A), V(0xF4), V(0x1F), V(0xDD), V(0xA8),       \
        V(0x33), V(0x88), V(0x07), V(0xC7), V(0x31), V(0xB1), V(0x12), V(0x10), V(0x59), V(0x27), V(0x80), V(0xEC), V(0x5F), V(0x60), V(0x51),       \
        V(0x7F), V(0xA9), V(0x19), V(0xB5), V(0x4A), V(0x0D), V(0x2D), V(0xE5), V(0x7A), V(0x9F), V(0x93), V(0xC9), V(0x9C), V(0xEF), V(0xA0),       \
        V(0xE0), V(0x3B), V(0x4D), V(0xAE), V(0x2A), V(0xF5), V(0xB0), V(0xC8), V(0xEB), V(0xBB), V(0x3C), V(0x83), V(0x53), V(0x99), V(0x61),       \
        V(0x17), V(0x2B), V(0x04), V(0x7E), V(0xBA), V(0x77), V(0xD6), V(0x26), V(0xE1), V(0x69), V(0x14), V(0x63), V(0x55), V(0x21), V(0x0C),       \
        V(0x7D),                                                                                                                                     \
    }

#define GF_x1(V) (u8)(V)
#define GF_x2(V) (u8)((V << 1) ^ (0x1B * (V >> 7)))
#define GF_x3(V) (u8)((V << 1) ^ V ^ (0x1B * (V >> 7)))
#define GF_x9(V) (u8)((V << 3) ^ V ^ (0x6C * (V >> 7)) ^ (0x36 * ((V >> 6) & 1)) ^ (0x1B * ((V >> 5) & 1)))
#define GF_x11(V) (u8)((V << 3) ^ (V << 1) ^ V ^ (0x77 * (V >> 7)) ^ (0x36 * ((V >> 6) & 1)) ^ (0x1B * ((V >> 5) & 1)))
#define GF_x13(V) (u8)((V << 3) ^ (V << 2) ^ V ^ (0x5A * (V >> 7)) ^ (0x2D * ((V >> 6) & 1)) ^ (0x1B * ((V >> 5) & 1)))
#define GF_x14(V) (u8)((V << 3) ^ (V << 2) ^ (V << 1) ^ (0x41 * (V >> 7)) ^ (0x2D * ((V >> 6) & 1)) ^ (0x1B * ((V >> 5) & 1)))

#define GF_COMBINE(S, a, b, c, d) (((u32)GF_x##a(S) << 0x18) | ((u32)GF_x##b(S) << 0x10) | ((u32)GF_x##c(S) << 0x08) | ((u32)GF_x##d(S) << 0x00))

// Forward tables
#define VFWD0(S) GF_COMBINE(S, 2, 1, 1, 3)
#define VFWD1(S) GF_COMBINE(S, 3, 2, 1, 1)
#define VFWD2(S) GF_COMBINE(S, 1, 3, 2, 1)
#define VFWD3(S) GF_COMBINE(S, 1, 1, 3, 2)

#define VREV0(S) GF_COMBINE(S, 14, 9, 13, 11)
#define VREV1(S) GF_COMBINE(S, 11, 14, 9, 13)
#define VREV2(S) GF_COMBINE(S, 13, 11, 14, 9)
#define VREV3(S) GF_COMBINE(S, 9, 13, 11, 14)

#define VDIRECT(S) GF_COMBINE(S, 1, 1, 1, 1)

const u32 ATERMi_ForwardTable0SBox[0x100] = FT(VFWD0);
const u32 ATERMi_ForwardTable1SBox[0x100] = FT(VFWD1);
const u32 ATERMi_ForwardTable2SBox[0x100] = FT(VFWD2);
const u32 ATERMi_ForwardTable3SBox[0x100] = FT(VFWD3);
const u32 ATERMi_ForwardSbox[0x100] = FT(VDIRECT);

// Reverse tables
const u32 ATERMi_ReverseTable0SBox[0x100] = RT(VREV0);
const u32 ATERMi_ReverseTable1SBox[0x100] = RT(VREV1);
const u32 ATERMi_ReverseTable2SBox[0x100] = RT(VREV2);
const u32 ATERMi_ReverseTable3SBox[0x100] = RT(VREV3);
const u32 ATERMi_ReverseSbox[0x100] = RT(VDIRECT);

#undef VFWD0
#undef VFWD1
#undef VFWD2
#undef VFWD3
#undef VREV0
#undef VREV1
#undef VREV2
#undef VREV3
#undef VDIRECT

#undef GF_COMBINE

#undef GF_x1
#undef GF_x2
#undef GF_x3
#undef GF_x9
#undef GF_x11
#undef GF_x13
#undef GF_x14

#undef FT
#undef RT

const u32 ATERMi_RoundConstants[] = {
    0x01000000U, 0x02000000U, 0x04000000U, 0x08000000U, 0x10000000U, 0x20000000U, 0x40000000U, 0x80000000U, 0x1B000000U, 0x36000000U,
};

typedef struct {
    u32 w0;  // 0x00
    u32 w1;  // 0x04
    u32 w2;  // 0x08
    u32 w3;  // 0x0c
} AES128RoundKey;
typedef struct {
    u32 w0;  // 0x00
    u32 w1;  // 0x04
    u32 w2;  // 0x08
    u32 w3;  // 0x0c
    u32 w4;  // 0x10
    u32 w5;  // 0x14
} AES192RoundKey;
typedef struct {
    u32 w0;  // 0x00
    u32 w1;  // 0x04
    u32 w2;  // 0x08
    u32 w3;  // 0x0c
    u32 w4;  // 0x10
    u32 w5;  // 0x14
    u32 w6;  // 0x18
    u32 w7;  // 0x1c
} AES256RoundKey;

int ATERMi_AESForwardKeySched(AES128RoundKey* roundKeys, AESKey* key, int bits);
int ATERMi_AESReverseKeySched(AES128RoundKey* roundKeys, AESKey* key, int bits);
void ATERMi_AESEncryptBlock(AES128RoundKey* data, int param_2, u8* stateIn, u8* stateOut);
void ATERMi_AESDecryptBlock(AES128RoundKey* data, int param_2, u8* stateIn, u8* stateOut);

BOOL ATERMi_AESEncrypt(void* ciphertext, void* plaintext, u32 len, AESKey* key, u32 bytes) {
    int blockI;
    int encI;
    int blockCnt;

    int rounds;
    u8* ct = ciphertext;
    u32 offset;

    u64 counterBuf;
    u32 upperiv[2];
    u8 aesState[16];
    AES128RoundKey roundKeys[20];

    upperiv[1] = 0xa6a6a6a6;
    upperiv[0] = 0xa6a6a6a6;
    if (((len & 7) != 0) || ((bytes & 7) != 0))
        return 0;

    blockCnt = len >> 3;
    if (blockCnt < 2) {
        return 0;
    }
    rounds = ATERMi_AESForwardKeySched(roundKeys, key, bytes << 3);
    memcpy(ct + 8, plaintext, len);
    memcpy(aesState, upperiv, 8);
    encI = 0;
    do {
        for (blockI = 1, offset = 8; blockI <= blockCnt; blockI++, offset += 8) {
            memcpy(aesState + 8, ct + offset, 8);
            ATERMi_AESEncryptBlock(roundKeys, rounds, aesState, aesState);
            counterBuf = blockI + (u64)blockCnt * (u64)encI;
            aesState[0] ^= ((u8*)&counterBuf)[0];
            aesState[1] ^= ((u8*)&counterBuf)[1];
            aesState[2] ^= ((u8*)&counterBuf)[2];
            aesState[3] ^= ((u8*)&counterBuf)[3];
            aesState[4] ^= ((u8*)&counterBuf)[4];
            aesState[5] ^= ((u8*)&counterBuf)[5];
            aesState[6] ^= ((u8*)&counterBuf)[6];
            aesState[7] ^= ((u8*)&counterBuf)[7];
            memcpy(ct + offset, aesState + 8, 8);
        }
        encI = encI + 1;
    } while (encI < 6);
    memcpy(ct, aesState, 8);
    return 1;
}

BOOL ATERMi_AESDecrypt(void* plaintext, void* ciphertext, u32 len, AESKey* key, u32 bytes) {
    int blockI;
    int encI;
    int blockCnt;

    int rounds;
    BOOL matches = TRUE;
    u8* pt = plaintext;

    u64 counterBuf;
    u32 upperiv[2];
    u8 aesState[16];
    AES128RoundKey roundKeys[20];

    upperiv[1] = 0xa6a6a6a6;
    upperiv[0] = 0xa6a6a6a6;
    if (((len & 7) != 0) || ((bytes & 7) != 0))
        return 0;

    blockCnt = (len - 1) >> 3;
    if (blockCnt < 2) {
        return 0;
    }
    rounds = ATERMi_AESReverseKeySched(roundKeys, key, bytes << 3);
    memcpy(aesState, ciphertext, 8);
    memcpy(pt, (u8*)ciphertext + 8, len - 1);
    encI = 5;
    do {
        for (blockI = blockCnt; blockI > 0; blockI--) {
            counterBuf = blockI + (u64)blockCnt * (u64)encI;
            aesState[0] ^= ((u8*)&counterBuf)[0];
            aesState[1] ^= ((u8*)&counterBuf)[1];
            aesState[2] ^= ((u8*)&counterBuf)[2];
            aesState[3] ^= ((u8*)&counterBuf)[3];
            aesState[4] ^= ((u8*)&counterBuf)[4];
            aesState[5] ^= ((u8*)&counterBuf)[5];
            aesState[6] ^= ((u8*)&counterBuf)[6];
            aesState[7] ^= ((u8*)&counterBuf)[7];

            memcpy(aesState + 8, pt + (blockI - 1) * 8, 8);
            ATERMi_AESDecryptBlock(roundKeys, rounds, aesState, aesState);
            memcpy(pt + (blockI - 1) * 8, aesState + 8, 8);
        }
        encI--;
    } while (encI >= 0);

    if (memcmp(upperiv, aesState, 8) != 0)
        matches = FALSE;
    return matches;
}

int ATERMi_AESForwardKeySched(AES128RoundKey* roundKeys, AESKey* key, int bits) {
    u32 kw0, kw1, kw2, kw3, kw4, kw5, kw6, kw7;
    u32 kw00, kw01, kw02, kw03;
    u32 kw10, kw11, kw12, kw13;
    u32 kw20, kw21, kw22, kw23;
    u32 kw30, kw31, kw32, kw33;
    u32 kw40, kw41, kw42, kw43;
    u32 kw50, kw51, kw52, kw53;
    u32 kw60, kw61, kw62, kw63;
    u32 kw70, kw71, kw72, kw73;

    u32 uVar10;

    int roundCnt;
    u32* puVar11;
    int i;
    int j;
    int wordShift;
    int maskShift;
    u32 uVar12;
    u32 k0;

    u32 subwordA, subwordB, subwordC, subwordD;

    AES128RoundKey* roundKeys128;
    AES192RoundKey* roundKeys192;
    AES256RoundKey* roundKeys256;
    const u32* roundConstPtr;

    kw00 = key->w0[0] << 0x18;
    kw01 = key->w0[1] << 0x10;
    kw02 = key->w0[2] << 0x08;
    kw03 = key->w0[3] << 0x00;
    kw0 = kw00 ^ kw01 ^ kw02 ^ kw03;

    kw10 = key->w1[0] << 0x18;
    kw11 = key->w1[1] << 0x10;
    kw12 = key->w1[2] << 0x08;
    kw13 = key->w1[3] << 0x00;
    kw1 = kw10 ^ kw11 ^ kw12 ^ kw13;

    kw30 = key->w3[0] << 0x18;
    kw31 = key->w3[1] << 0x10;
    kw32 = key->w3[2] << 0x08;
    kw33 = key->w3[3] << 0x00;
    kw3 = kw30 ^ kw31 ^ kw32 ^ kw33;

    kw20 = key->w2[0] << 0x18;
    kw21 = key->w2[1] << 0x10;
    kw22 = key->w2[2] << 0x08;
    kw23 = key->w2[3] << 0x00;
    kw2 = kw20 ^ kw21 ^ kw22 ^ kw23;

    roundKeys128 = (AES128RoundKey*)roundKeys;
    roundKeys128[0].w1 = kw1;
    roundKeys128[0].w0 = kw0;
    roundKeys128[0].w2 = kw2;
    roundKeys128[0].w3 = kw3;

    i = 0;
    if (bits == 128) {
        roundConstPtr = ATERMi_RoundConstants;
        while (TRUE) {
            // uVar10 = 0;
            // subwordA = (ATERMi_ForwardSbox[(u8)(roundKeys128[0].w3 >> 0x18)] & (0x000000ff << 0x00));
            // subwordB = (ATERMi_ForwardSbox[(u8)(roundKeys128[0].w3 >> 0x00)] & (0x000000ff << 0x08));
            // subwordC = (ATERMi_ForwardSbox[(u8)(roundKeys128[0].w3 >> 0x08)] & (0x000000ff << 0x10));
            // subwordD = (ATERMi_ForwardSbox[(u8)(roundKeys128[0].w3 >> 0x10)] & (0x000000ff << 0x18));
            // uVar10 = subwordA ^ subwordB ^ subwordD ^ subwordC ^ (roundKeys128[0].w0 ^ *roundConstPtr++);

            uVar10 = 0;
            uVar10 ^= (ATERMi_ForwardSbox[(u8)(roundKeys128[0].w3 >> 0x18)] & (0x000000ff << 0x00));
            uVar10 ^= (ATERMi_ForwardSbox[(u8)(roundKeys128[0].w3 >> 0x00)] & (0x000000ff << 0x08));
            uVar10 ^= (ATERMi_ForwardSbox[(u8)(roundKeys128[0].w3 >> 0x10)] & (0x000000ff << 0x18));
            uVar10 ^= (ATERMi_ForwardSbox[(u8)(roundKeys128[0].w3 >> 0x08)] & (0x000000ff << 0x10));
            uVar10 ^= *roundConstPtr++;

            roundKeys128[1].w0 = uVar10;
            roundKeys128[1].w1 = roundKeys128[0].w1 ^ roundKeys128[1].w0;
            roundKeys128[1].w2 = roundKeys128[0].w2 ^ roundKeys128[1].w1;
            roundKeys128[1].w3 = roundKeys128[0].w3 ^ roundKeys128[1].w2;
            if (++i == 10) {
                return 10;
            }
            roundKeys128++;
        }
        roundCnt = 10;
    }

    kw40 = key->w4[0] << 0x18;
    kw41 = key->w4[1] << 0x10;
    kw42 = key->w4[2] << 0x08;
    kw43 = key->w4[3] << 0x00;
    kw4 = kw40 ^ kw41 ^ kw42 ^ kw43;

    kw50 = key->w5[0] << 0x18;
    kw51 = key->w5[1] << 0x10;
    kw52 = key->w5[2] << 0x08;
    kw53 = key->w5[3] << 0x00;
    kw5 = kw50 ^ kw51 ^ kw52 ^ kw53;

    roundKeys192 = (AES192RoundKey*)roundKeys;
    roundKeys192[0].w5 = kw5;
    roundKeys192[0].w4 = kw4;
    if (bits == 192) {
        // puVar11 = ATERMi_RoundConstants;
        while (TRUE) {
            uVar10 = 0;
            uVar10 ^= (ATERMi_ForwardSbox[(u8)(roundKeys192[0].w5 >> 0x18)] & 0x000000ff);
            uVar10 ^= (ATERMi_ForwardSbox[(u8)(roundKeys192[0].w5 >> 0x00)] & 0x0000ff00);
            uVar10 ^= (ATERMi_ForwardSbox[(u8)(roundKeys192[0].w5 >> 0x08)] & 0x00ff0000);
            uVar10 ^= (ATERMi_ForwardSbox[(u8)(roundKeys192[0].w5 >> 0x10)] & 0xff000000);
            // uVar10 ^= roundKeys128[0].w0;
            uVar10 ^= ATERMi_RoundConstants[i];

            roundKeys192[1].w0 = roundKeys192[0].w0;
            roundKeys192[1].w0 ^= uVar10;
            roundKeys192[1].w1 = roundKeys192[0].w1 ^ roundKeys192[1].w0;
            roundKeys192[1].w2 = roundKeys192[0].w2 ^ roundKeys192[1].w1;
            roundKeys192[1].w3 = roundKeys192[0].w3 ^ roundKeys192[1].w2;
            if (++i == 8) {
                return 12;
            }
            roundKeys192[1].w4 = roundKeys192[0].w4 ^ roundKeys192[1].w3;
            roundKeys192[1].w5 = roundKeys192[0].w5 ^ roundKeys192[1].w4;
            roundKeys192++;
            // roundKeys = roundKeys + 4;
        }
        // roundCnt = 0xc;
    }

    kw60 = key->w6[0] << 0x18;
    kw61 = key->w6[1] << 0x10;
    kw62 = key->w6[2] << 0x08;
    kw63 = key->w6[3] << 0x00;
    kw6 = kw60 ^ kw61 ^ kw62 ^ kw63;

    kw70 = key->w7[0] << 0x18;
    kw71 = key->w7[1] << 0x10;
    kw72 = key->w7[2] << 0x08;
    kw73 = key->w7[3] << 0x00;
    kw7 = kw70 ^ kw71 ^ kw72 ^ kw73;

    roundKeys256 = (AES256RoundKey*)roundKeys;
    roundKeys256[0].w7 = kw7;
    roundKeys256[0].w6 = kw6;
    if (bits == 256) {
        roundConstPtr = ATERMi_RoundConstants;
        while (TRUE) {
            roundKeys256[1].w0 = ((ATERMi_ForwardSbox[(u8)(roundKeys256[0].w7 >> 0x10)] & 0xff000000) ^
                                  (ATERMi_ForwardSbox[(u8)(roundKeys256[0].w7 >> 0x08)] & 0x00ff0000) ^
                                  (ATERMi_ForwardSbox[(u8)(roundKeys256[0].w7 >> 0x00)] & 0x0000ff00) ^
                                  (ATERMi_ForwardSbox[(u8)(roundKeys256[0].w7 >> 0x18)] & 0x000000ff)) ^
                                 roundKeys256[0].w0 ^ ATERMi_RoundConstants[i];
            // uVar10 ^= roundKeys128[0].w0;
            // uVar10 ^= ;

            // roundKeys256[1].w0 = ;
            // roundKeys256[1].w0 ^= uVar10;
            roundKeys256[1].w1 = roundKeys256[0].w1 ^ roundKeys256[1].w0;
            roundKeys256[1].w2 = roundKeys256[0].w2 ^ roundKeys256[1].w1;
            roundKeys256[1].w3 = roundKeys256[0].w3 ^ roundKeys256[1].w2;
            if (++i == 7) {
                return 14;
            }

            roundKeys256[1].w4 = roundKeys256[0].w4 ^ (ATERMi_ForwardSbox[(u8)(roundKeys256[1].w3 >> 0x18)] & 0xff000000) ^
                                 (ATERMi_ForwardSbox[(u8)(roundKeys256[1].w3 >> 0x10)] & 0x00ff0000) ^
                                 (ATERMi_ForwardSbox[(u8)(roundKeys256[1].w3 >> 0x08)] & 0x0000ff00) ^
                                 (ATERMi_ForwardSbox[(u8)(roundKeys256[1].w3 >> 0x00)] & 0x000000ff);
            roundKeys256[1].w5 = roundKeys256[0].w5 ^ roundKeys256[1].w4;
            roundKeys256[1].w6 = roundKeys256[0].w6 ^ roundKeys256[1].w5;
            roundKeys256[1].w7 = roundKeys256[0].w7 ^ roundKeys256[1].w6;
            roundKeys256++;
            // roundKeys = roundKeys + 4;
        }
    }
    return 0;
}

void ATERMi_SwapWords(u32* a, u32* b) {
    u32 tmp = *a;
    *a = *b;
    *b = tmp;
}

int ATERMi_AESReverseKeySched(AES128RoundKey* roundKeys, AESKey* key, int bits) {
    int numRounds;

    u32* wordPtr;
    u32 tmp;
    int startOffset;
    int endOffset;
    int i;

    numRounds = ATERMi_AESForwardKeySched(roundKeys, key, bits);

    // Reverse the order of the round keys so the decrypt block method can
    // iterate through them forward (Better for cache I think)
    startOffset = 0;
    endOffset = numRounds * 4;
    wordPtr = (u32*)roundKeys;
    for (; endOffset > startOffset;) {
        for (i = 0; i < 4; i++) {
            tmp = *(wordPtr + startOffset + i);
            *(wordPtr + startOffset + i) = *(wordPtr + endOffset + i);
            *(wordPtr + endOffset + i) = tmp;
        }

        startOffset += 4;
        endOffset -= 4;
    }

    // Do a transformation on the round keys to make it so they're easier to use
    // repeatedly for decryption (upfront cost to reduce the cost of each block)
    //
    // Because the last round of encryption/first round of decryption is
    // different, (there's no MixColumns step,) we don't need to apply this to
    // the first round key.
    for (i = 1; i < numRounds; i++) {
        tmp = roundKeys[1].w0;
        roundKeys[1].w0 = ATERMi_ReverseTable0SBox[(u8)ATERMi_ForwardSbox[(u8)(roundKeys[1].w0 >> 0x18)]] ^
                          ATERMi_ReverseTable1SBox[(u8)ATERMi_ForwardSbox[(u8)(roundKeys[1].w0 >> 0x10)]] ^
                          ATERMi_ReverseTable2SBox[(u8)ATERMi_ForwardSbox[(u8)(roundKeys[1].w0 >> 0x08)]] ^
                          ATERMi_ReverseTable3SBox[(u8)ATERMi_ForwardSbox[(u8)(roundKeys[1].w0 >> 0x00)]];

        tmp = roundKeys[1].w1;
        roundKeys[1].w1 = ATERMi_ReverseTable0SBox[(u8)ATERMi_ForwardSbox[(u8)(roundKeys[1].w1 >> 0x18)]] ^
                          ATERMi_ReverseTable1SBox[(u8)ATERMi_ForwardSbox[(u8)(roundKeys[1].w1 >> 0x10)]] ^
                          ATERMi_ReverseTable2SBox[(u8)ATERMi_ForwardSbox[(u8)(roundKeys[1].w1 >> 0x08)]] ^
                          ATERMi_ReverseTable3SBox[(u8)ATERMi_ForwardSbox[(u8)(roundKeys[1].w1 >> 0x00)]];

        tmp = roundKeys[1].w2;
        roundKeys[1].w2 = ATERMi_ReverseTable0SBox[(u8)ATERMi_ForwardSbox[(u8)(roundKeys[1].w2 >> 0x18)]] ^
                          ATERMi_ReverseTable1SBox[(u8)ATERMi_ForwardSbox[(u8)(roundKeys[1].w2 >> 0x10)]] ^
                          ATERMi_ReverseTable2SBox[(u8)ATERMi_ForwardSbox[(u8)(roundKeys[1].w2 >> 0x08)]] ^
                          ATERMi_ReverseTable3SBox[(u8)ATERMi_ForwardSbox[(u8)(roundKeys[1].w2 >> 0x00)]];

        tmp = roundKeys[1].w3;
        roundKeys[1].w3 = ATERMi_ReverseTable0SBox[(u8)ATERMi_ForwardSbox[(u8)(roundKeys[1].w3 >> 0x18)]] ^
                          ATERMi_ReverseTable1SBox[(u8)ATERMi_ForwardSbox[(u8)(roundKeys[1].w3 >> 0x10)]] ^
                          ATERMi_ReverseTable2SBox[(u8)ATERMi_ForwardSbox[(u8)(roundKeys[1].w3 >> 0x08)]] ^
                          ATERMi_ReverseTable3SBox[(u8)ATERMi_ForwardSbox[(u8)(roundKeys[1].w3 >> 0x00)]];

        roundKeys++;
    }

    return numRounds;
}

u32 ATERMi_getU32(u8* data, u32 offset) {
    int i;
    u32 val = 0;
    for (i = 0; i < 4; i++) {
        val <<= 8;
        val ^= data[i + offset];
    }
    return val;
}

void ATERMi_AESEncryptBlock(AES128RoundKey* roundKeys, int param_2, u8* stateIn, u8* stateOut) {
    u32 a;
    u32 a2;
    AES128RoundKey* puVar1;
    u32 b;
    u32 b2;
    u32 d;
    u32 d2;
    u32 c;
    u32 c2;

    u8 a_b0, a_b1, a_b2, a_b3;
    u8 b_b0, b_b1, b_b2, b_b3;
    u8 c_b0, c_b1, c_b2, c_b3;
    u8 d_b0, d_b1, d_b2, d_b3;

    u32 uVar2;
    u32 uVar3;
    u32 uVar4;
    int iVar5;
    u32 uVar6;

    // u32 kw0, kw1, kw2, kw3, kw4, kw5, kw6, kw7;
    u32 kw00, kw01, kw02, kw03;
    u32 kw10, kw11, kw12, kw13;
    u32 kw20, kw21, kw22, kw23;
    u32 kw30, kw31, kw32, kw33;
    // u32 kw40, kw41, kw42, kw43;
    // u32 kw50, kw51, kw52, kw53;
    // u32 kw60, kw61, kw62, kw63;
    // u32 kw70, kw71, kw72, kw73;

    iVar5 = param_2 >> 1;
    // u32 uVar10;

    // int roundCnt;
    // u32* puVar11;
    // int i;
    // int j;
    // int wordShift;
    // int maskShift;
    // u32 uVar12;
    // u32 k0;

    // u32 subwordA, subwordB, subwordC, subwordD;

    // AES128RoundKey* roundKeys128;
    // AES192RoundKey* roundKeys192;
    // AES256RoundKey* roundKeys256;
    // const u32* roundConstPtr;

    kw03 = stateIn[0x3] << 0x00;
    kw02 = stateIn[0x2] << 0x08;
    kw01 = stateIn[0x1] << 0x10;
    kw00 = stateIn[0x0] << 0x18;

    kw13 = stateIn[0x7] << 0x00;
    kw12 = stateIn[0x6] << 0x08;
    kw11 = stateIn[0x5] << 0x10;
    kw10 = stateIn[0x4] << 0x18;

    kw23 = stateIn[0xb] << 0x00;
    kw22 = stateIn[0xa] << 0x08;
    kw21 = stateIn[0x9] << 0x10;
    kw20 = stateIn[0x8] << 0x18;

    kw33 = stateIn[0xf] << 0x00;
    kw32 = stateIn[0xe] << 0x08;
    kw31 = stateIn[0xd] << 0x10;
    kw30 = stateIn[0xc] << 0x18;

    a = kw00 ^ kw01 ^ kw02 ^ kw03;
    b = kw10 ^ kw11 ^ kw12 ^ kw13;
    c = kw20 ^ kw21 ^ kw22 ^ kw23;
    d = kw30 ^ kw31 ^ kw32 ^ kw33;

    // roundKeys128 = (AES128RoundKey*)roundKeys;
    // a = kw0;
    // b = kw1;
    // c = kw2;
    // d = kw3;

    // a = ((u32)stateIn[0x3] << 0x00) ^ ((u32)stateIn[0x2] << 0x08) ^ ((u32)stateIn[0x1] << 0x10) ^ ((u32)stateIn[0x0] << 0x18);
    // b = ((u32)stateIn[0x7] << 0x00) ^ ((u32)stateIn[0x6] << 0x08) ^ ((u32)stateIn[0x5] << 0x10) ^ ((u32)stateIn[0x4] << 0x18);
    // c = ((u32)stateIn[0xb] << 0x00) ^ ((u32)stateIn[0xa] << 0x08) ^ ((u32)stateIn[0x9] << 0x10) ^ ((u32)stateIn[0x8] << 0x18);
    // d = ((u32)stateIn[0xf] << 0x00) ^ ((u32)stateIn[0xe] << 0x08) ^ ((u32)stateIn[0xd] << 0x10) ^ ((u32)stateIn[0xc] << 0x18);

    // AddRoundKey
    a ^= roundKeys[0].w0;
    b ^= roundKeys[0].w1;
    c ^= roundKeys[0].w2;
    d ^= roundKeys[0].w3;
    while (TRUE) {
        iVar5--;

        a_b0 = a >> 24, a_b1 = a >> 16, a_b2 = a >> 8, a_b3 = a;
        b_b0 = b >> 24, b_b1 = b >> 16, b_b2 = b >> 8, b_b3 = b;
        c_b0 = c >> 24, c_b1 = c >> 16, c_b2 = c >> 8, c_b3 = c;
        d_b0 = d >> 24, d_b1 = d >> 16, d_b2 = d >> 8, d_b3 = d;

        a2 = ATERMi_ForwardTable3SBox[d_b3] ^ ATERMi_ForwardTable2SBox[c_b2] ^ ATERMi_ForwardTable1SBox[b_b1] ^ ATERMi_ForwardTable0SBox[a_b0] ^
             roundKeys[1].w0;
        b2 = ATERMi_ForwardTable3SBox[a_b3] ^ ATERMi_ForwardTable2SBox[d_b2] ^ ATERMi_ForwardTable1SBox[c_b1] ^ ATERMi_ForwardTable0SBox[b_b0] ^
             roundKeys[1].w1;
        c2 = ATERMi_ForwardTable3SBox[b_b3] ^ ATERMi_ForwardTable2SBox[a_b2] ^ ATERMi_ForwardTable1SBox[d_b1] ^ ATERMi_ForwardTable0SBox[c_b0] ^
             roundKeys[1].w2;
        d2 = ATERMi_ForwardTable3SBox[c_b3] ^ ATERMi_ForwardTable2SBox[b_b2] ^ ATERMi_ForwardTable1SBox[a_b1] ^ ATERMi_ForwardTable0SBox[d_b0] ^
             roundKeys[1].w3;
        // a2 = ATERMi_ForwardTable3SBox[(u8)d] ^ ATERMi_ForwardTable2SBox[(u8)(c >> 8)] ^ ATERMi_ForwardTable1SBox[(u8)(b >> 16)] ^
        //      ATERMi_ForwardTable0SBox[(u8)(a >> 24)] ^ data[1].w0;
        // b2 = ATERMi_ForwardTable3SBox[(u8)a] ^ ATERMi_ForwardTable2SBox[(u8)(d >> 8)] ^ ATERMi_ForwardTable1SBox[(u8)(c >> 16)] ^
        //      ATERMi_ForwardTable0SBox[(u8)(b >> 24)] ^ data[1].w1;
        // c2 = ATERMi_ForwardTable3SBox[(u8)b] ^ ATERMi_ForwardTable2SBox[(u8)(a >> 8)] ^ ATERMi_ForwardTable1SBox[(u8)(d >> 16)] ^
        //      ATERMi_ForwardTable0SBox[(u8)(c >> 24)] ^ data[1].w2;
        // d2 = ATERMi_ForwardTable3SBox[(u8)c] ^ ATERMi_ForwardTable2SBox[(u8)(b >> 8)] ^ ATERMi_ForwardTable1SBox[(u8)(a >> 16)] ^
        //      ATERMi_ForwardTable0SBox[(u8)(d >> 24)] ^ data[1].w3;

        puVar1 = roundKeys + 2;
        if (iVar5 == 0)
            break;
        a = roundKeys[2].w0 ^ ATERMi_ForwardTable0SBox[(u8)(a2 >> 24)] ^ ATERMi_ForwardTable1SBox[(u8)(c2 >> 16)] ^
            ATERMi_ForwardTable2SBox[(u8)(b2 >> 8)] ^ ATERMi_ForwardTable3SBox[(u8)d2];
        b = roundKeys[2].w1 ^ ATERMi_ForwardTable0SBox[(u8)(c2 >> 24)] ^ ATERMi_ForwardTable1SBox[(u8)(b2 >> 16)] ^
            ATERMi_ForwardTable2SBox[(u8)(d2 >> 8)] ^ ATERMi_ForwardTable3SBox[(u8)a2];
        c = roundKeys[2].w2 ^ ATERMi_ForwardTable0SBox[(u8)(b2 >> 24)] ^ ATERMi_ForwardTable1SBox[(u8)(d2 >> 16)] ^
            ATERMi_ForwardTable2SBox[(u8)(a2 >> 8)] ^ ATERMi_ForwardTable3SBox[(u8)c2];
        d = roundKeys[2].w3 ^ ATERMi_ForwardTable0SBox[(u8)(d2 >> 24)] ^ ATERMi_ForwardTable1SBox[(u8)(a2 >> 16)] ^
            ATERMi_ForwardTable2SBox[(u8)(c2 >> 8)] ^ ATERMi_ForwardTable3SBox[(u8)b2];
        roundKeys = puVar1;
        // if (iVar5 == 0)
        //     break;
    }
    uVar2 = ATERMi_ForwardSbox[(u8)(c2 >> 0x00)];
    uVar3 = ATERMi_ForwardSbox[(u8)(b2 >> 0x08)];
    uVar4 = ATERMi_ForwardSbox[(u8)(d2 >> 0x18)];
    uVar6 = ATERMi_ForwardSbox[(u8)(a2 >> 0x10)];

    a = (uVar2 & 0xff) ^ (uVar3 & 0xff00) ^ (uVar4 & 0xff000000) ^ (uVar6 & 0xff0000);
    c = (ATERMi_ForwardSbox[(u8)(b2 >> 24)] & 0xff000000) ^ (ATERMi_ForwardSbox[(u8)(c2 >> 16)] & 0x00ff0000) ^
        (ATERMi_ForwardSbox[(u8)(d2 >> 8)] & 0x0000ff00) ^ (ATERMi_ForwardSbox[(u8)a2] & 0x000000ff);
    b = (ATERMi_ForwardSbox[(u8)(a2 >> 24)] & 0xff000000) ^ (ATERMi_ForwardSbox[(u8)(b2 >> 16)] & 0x00ff0000) ^
        (ATERMi_ForwardSbox[(u8)(c2 >> 8)] & 0x0000ff00) ^ (ATERMi_ForwardSbox[(u8)d2] & 0x000000ff);
    d = (ATERMi_ForwardSbox[(u8)(c2 >> 24)] & 0xff000000) ^ (ATERMi_ForwardSbox[(u8)(d2 >> 16)] & 0x00ff0000) ^
        (ATERMi_ForwardSbox[(u8)(a2 >> 8)] & 0x0000ff00) ^ (ATERMi_ForwardSbox[(u8)b2] & 0x000000ff);
    a ^= roundKeys[2].w3;
    b ^= roundKeys[2].w0;
    c ^= roundKeys[2].w1;
    d ^= roundKeys[2].w2;

    stateOut[0x0] = (u8)(c >> 0x18);
    stateOut[0x1] = (u8)(c >> 0x10);
    stateOut[0x2] = (u8)(c >> 0x08);
    stateOut[0x3] = (u8)(c >> 0x00);
    stateOut[0x4] = (u8)(b >> 0x18);
    stateOut[0x5] = (u8)(b >> 0x10);
    stateOut[0x6] = (u8)(b >> 0x08);
    stateOut[0x7] = (u8)(b >> 0x00);
    stateOut[0x8] = (u8)(d >> 0x18);
    stateOut[0x9] = (u8)(d >> 0x10);
    stateOut[0xa] = (u8)(d >> 0x08);
    stateOut[0xb] = (u8)(d >> 0x00);
    stateOut[0xc] = (u8)(a >> 0x18);
    stateOut[0xd] = (u8)(a >> 0x10);
    stateOut[0xe] = (u8)(a >> 0x08);
    stateOut[0xf] = (u8)(a >> 0x00);
    return;
}

#define ATERM_MD5FnF(b, c, d) ((b & c) | (d & ~b))
#define ATERM_MD5FnG(b, c, d) ((d & b) | (~d & c))
#define ATERM_MD5FnH(b, c, d) (b ^ c ^ d)
#define ATERM_MD5FnI(b, c, d) (c ^ (b | ~d))
// inline u32 ATERM_MD5FnF(u32 b, u32 c, u32 d) {
//     return (b & c) | (d & ~b);
// }
// inline u32 ATERM_MD5FnG(u32 b, u32 c, u32 d) {
//     return (d & b) | (~d & c);
// }
// inline u32 ATERM_MD5FnH(u32 b, u32 c, u32 d) {
//     return b ^ c ^ d;
// }
// inline u32 ATERM_MD5FnI(u32 b, u32 c, u32 d) {
//     return c ^ (b | ~d);
// }

inline void ATERMi_cpy(ATERMiMd5State* state, u32 partialOff, u8* data, u32 dataOff, u32 len) {
    u32 i;
    u8* dst;
    u8* src;
    for (i = 0; i < len; i++) {
        dst = (u8*)state->partialBlock + partialOff;
        src = (u8*)data + dataOff;
        *(dst + i) = *(src + i);
    }
}

// A 1 followed by a bunch of zeros
u8 ATERMi_md5Pad[0x40] = {0x80};

void ATERMi_MD5(ATERMiMd5State* ctx, void* data, u32 dataLen, u8* digest) {
    ATERMi_MD5Init(ctx);
    ATERMi_MD5Update(ctx, data, dataLen);
    ATERMi_MD5GetDigest(ctx, digest);
}

void ATERMi_MD5Init(ATERMiMd5State* ctx) {
    ctx->bitCntHi = 0;
    ctx->bitCntLo = 0;

    // MD5 initialization constants
    // Fun fact: It's palindromic! (only in hex tho, not in binary)
    ctx->a0 = 0x67452301;
    ctx->b0 = 0xefcdab89;
    ctx->c0 = 0x98badcfe;
    ctx->d0 = 0x10325476;
}
inline void ATERMi_MD5Pad(ATERMiMd5State* ctx, u8* basePad, u8* sizeData) {
    u32 byteCnt;
    u32 paddingBytes;

    // This does not handle non-full bytes (as this MD5 implementation doesn't
    // support that)
    // If we're past the 8-bytes-until-the-end-of-the-chunk threshold, we need
    // another whole block of padding to be able to put in those last 8 bytes.
    byteCnt = (ctx->bitCntLo >> 3) % 64;
    paddingBytes = byteCnt < (64 - 8) ? (64 - 8) - byteCnt : (128 - 8) - byteCnt;
    ATERMi_MD5Update(ctx, basePad, paddingBytes);
    ATERMi_MD5Update(ctx, sizeData, 8);
}
inline void ATERMi_MD5GetDigest(ATERMiMd5State* ctx, void* digest) {
    u8* pDigest = digest;
    u8 sizeData[8];

    u32 byteCnt;
    u32 paddingBytes;

    int i, j;

    for (i = 0; i < 2; i += 1) {
        for (j = 0; j < 4; j++) {
            sizeData[i * 4 + j] = (u8)(((u32*)&ctx->bitCntLo)[i] >> (j * 8));
        }
    }

    ATERMi_MD5Pad(ctx, ATERMi_md5Pad, sizeData);
    // pad = ATERMi_md5Pad;

    // // This does not handle non-full bytes (as this MD5 implementation doesn't
    // // support that)
    // byteCnt = (ctx->bitCntLo >> 3) % 64;
    // paddingBytes = (128 - 8) - byteCnt;
    // if (byteCnt < (64 - 8)) {
    //     paddingBytes = (64 - 8) - byteCnt;
    // }
    // ATERMi_MD5Update(ctx, pad, paddingBytes);
    // ATERMi_MD5Update(ctx, sizeData, 8);

    for (i = 0; i < 4; i += 1) {
        for (j = 0; j < 4; j++) {
            pDigest[i * 4 + j] = (u8)(((u32*)ctx)[i] >> (j * 8));
        }
    }

    for (i = 0; i < sizeof(ATERMiMd5State); i++) {
        ((u8*)ctx)[i] = 0;
    }
}

void ATERMi_MD5Update(ATERMiMd5State* ctx, u8* newData, u32 newDataLen) {
    u32 i;
    u32 bytesRemaining;
    u32 partialByteOffset;
    u32 amtUsed;

    u32 bitCountLo;
    u32 numNewBits;

    u8* src;
    u8* dst;

    numNewBits = newDataLen * 8;
    bitCountLo = ctx->bitCntLo + numNewBits;
    partialByteOffset = bitCountLo / 8 % 0x40;
    // ctx->bitCntLo += newDataLen * 8;
    ctx->bitCntLo = bitCountLo;
    if (bitCountLo < numNewBits) {
        ctx->bitCntHi++;
    }
    ctx->bitCntHi += (newDataLen >> 0x1d);

    amtUsed = 0x40 - partialByteOffset;
    if (newDataLen >= amtUsed) {
        ATERMi_cpy(ctx, partialByteOffset, newData, 0, amtUsed);

        ATERMi_MD5ProcessBlock(ctx, ctx->partialBlock);
        while (amtUsed + 0x3f < newDataLen) {
            ATERMi_MD5ProcessBlock(ctx, newData + amtUsed);
            amtUsed = amtUsed + 0x40;
        }
        partialByteOffset = 0;
    } else {
        amtUsed = 0;
    }
    bytesRemaining = newDataLen - amtUsed;

    ATERMi_cpy(ctx, partialByteOffset, newData, amtUsed, bytesRemaining);
}

u32 ATERMi_RotL(u32 value, int rotation) {
    return (value << rotation) | (value >> (32 - rotation));
}

u32 ATERMi_MD5ConvertWord(u8* wordPtr) {
    return wordPtr[0] << (0 * 8) | wordPtr[1] << (1 * 8) | wordPtr[2] << (2 * 8) | wordPtr[3] << (3 * 8);
}

void ATERMi_MD5ProcessBlock(ATERMiMd5State* ctx, u8* chunk) {
    int i, chunkI, chunkIShifted;
    int j;
    u32 off;
    u32 word;
    u8* chunkBytePtr;

    u32 byte0, byte1, byte2, byte3;
    u32 F, g;
    u32 data[0x10];
    u8* dataBytePtr;
    u32* dataWordPtr;

    u32 a, b, c, d;
    a = ctx->a0;
    b = ctx->b0;
    c = ctx->c0;
    d = ctx->d0;

// Copy chunk
#define SHA1_CONVERT_TO_WORD(I) data[I] = (chunk[I * 4 + 3] << 0) | (chunk[I * 4 + 3] << 8) | (chunk[I * 4 + 3] << 16) | (chunk[I * 4 + 3] << 24);
    // off = 0;

    // for (i = 0, chunkI = 0; i < 0x10; i++, chunkI++) {
    //     // chunkBytePtr = chunk + off;
    //     chunkIShifted = chunkI * 4;
    //     byte0 = chunk[chunkIShifted + 3];
    //     byte1 = chunk[chunkIShifted + 2];
    //     byte2 = chunk[chunkIShifted + 1];
    //     byte3 = chunk[chunkIShifted + 0];
    //     dataWordPtr[i] = ((((byte2 | (byte3 << 8)) << 8) | byte1) << 8) | byte0;
    // }

    dataWordPtr = data;
    dataBytePtr = (u8*)data;
    off = 0;
    for (i = 0; i < 0x40; i += 4) {
        // Convert to little-endian
        // The local variable `input` our 512-bit chunk separated into 32-bit words
        // we can use in calculations
        // chunkI = i * 4;
        // data[i] = 0;
        // for (j = 0; j < 4; j++) {
        // }
        // chunkBytePtr = chunk + chunkI;
        // word = chunk[i + 0] << (0 * 8) | chunk[i + 1] << (1 * 8) | chunk[i + 2] << (2 * 8) | chunk[i + 3] << (3 * 8);
        // *dataWordPtr++ = word;
        *dataWordPtr++ = chunk[i + 0] << (0 * 8) | chunk[i + 1] << (1 * 8) | chunk[i + 2] << (2 * 8) | chunk[i + 3] << (3 * 8);
        // word = ATERMi_MD5ConvertWord(chunk + i * 4);
        // off++;
        // data[i] = (u32)(chunk[chunkI + 3]) << 24 | (u32)(chunk[chunkI + 2]) << 16 | (u32)(chunk[chunkI + 1]) << 8 | (u32)(chunk[chunkI]);
    }
    // OSReport("", data);

#define SHA1_STEP(IDX, IDXMUL, IDXADD, BASE_EXPR, CONST, SHIFT, VA, VB, VC, VD)                                                                      \
    F = BASE_EXPR(VB, VC, VD);                                                                                                                       \
    g = (IDX * IDXMUL + IDXADD) % 16;                                                                                                                \
    VA = ATERMi_RotL(F + VA + (CONST) + data[g], SHIFT) + VB;

    SHA1_STEP(0x00, 1, 0, ATERM_MD5FnF, 0xd76aa478U, 0x07, a, b, c, d);
    SHA1_STEP(0x01, 1, 0, ATERM_MD5FnF, 0xe8c7b756U, 0x0c, d, a, b, c);
    SHA1_STEP(0x02, 1, 0, ATERM_MD5FnF, 0x242070dbU, 0x11, c, d, a, b);
    SHA1_STEP(0x03, 1, 0, ATERM_MD5FnF, 0xc1bdceeeU, 0x16, b, c, d, a);
    SHA1_STEP(0x04, 1, 0, ATERM_MD5FnF, 0xf57c0fafU, 0x07, a, b, c, d);
    SHA1_STEP(0x05, 1, 0, ATERM_MD5FnF, 0x4787c62aU, 0x0c, d, a, b, c);
    SHA1_STEP(0x06, 1, 0, ATERM_MD5FnF, 0xa8304613U, 0x11, c, d, a, b);
    SHA1_STEP(0x07, 1, 0, ATERM_MD5FnF, 0xfd469501U, 0x16, b, c, d, a);
    SHA1_STEP(0x08, 1, 0, ATERM_MD5FnF, 0x698098d8U, 0x07, a, b, c, d);
    SHA1_STEP(0x09, 1, 0, ATERM_MD5FnF, 0x8b44f7afU, 0x0c, d, a, b, c);
    SHA1_STEP(0x0a, 1, 0, ATERM_MD5FnF, 0xffff5bb1U, 0x11, c, d, a, b);
    SHA1_STEP(0x0b, 1, 0, ATERM_MD5FnF, 0x895cd7beU, 0x16, b, c, d, a);
    SHA1_STEP(0x0c, 1, 0, ATERM_MD5FnF, 0x6b901122U, 0x07, a, b, c, d);
    SHA1_STEP(0x0d, 1, 0, ATERM_MD5FnF, 0xfd987193U, 0x0c, d, a, b, c);
    SHA1_STEP(0x0e, 1, 0, ATERM_MD5FnF, 0xa679438eU, 0x11, c, d, a, b);
    SHA1_STEP(0x0f, 1, 0, ATERM_MD5FnF, 0x49b40821U, 0x16, b, c, d, a);
    SHA1_STEP(0x10, 5, 1, ATERM_MD5FnG, 0xf61e2562U, 0x05, a, b, c, d);
    SHA1_STEP(0x11, 5, 1, ATERM_MD5FnG, 0xc040b340U, 0x09, d, a, b, c);
    SHA1_STEP(0x12, 5, 1, ATERM_MD5FnG, 0x265e5a51U, 0x0e, c, d, a, b);
    SHA1_STEP(0x13, 5, 1, ATERM_MD5FnG, 0xe9b6c7aaU, 0x14, b, c, d, a);
    SHA1_STEP(0x14, 5, 1, ATERM_MD5FnG, 0xd62f105dU, 0x05, a, b, c, d);
    SHA1_STEP(0x15, 5, 1, ATERM_MD5FnG, 0x02441453U, 0x09, d, a, b, c);
    SHA1_STEP(0x16, 5, 1, ATERM_MD5FnG, 0xd8a1e681U, 0x0e, c, d, a, b);
    SHA1_STEP(0x17, 5, 1, ATERM_MD5FnG, 0xe7d3fbc8U, 0x14, b, c, d, a);
    SHA1_STEP(0x18, 5, 1, ATERM_MD5FnG, 0x21e1cde6U, 0x05, a, b, c, d);
    SHA1_STEP(0x19, 5, 1, ATERM_MD5FnG, 0xc33707d6U, 0x09, d, a, b, c);
    SHA1_STEP(0x1a, 5, 1, ATERM_MD5FnG, 0xf4d50d87U, 0x0e, c, d, a, b);
    SHA1_STEP(0x1b, 5, 1, ATERM_MD5FnG, 0x455a14edU, 0x14, b, c, d, a);
    SHA1_STEP(0x1c, 5, 1, ATERM_MD5FnG, 0xa9e3e905U, 0x05, a, b, c, d);
    SHA1_STEP(0x1d, 5, 1, ATERM_MD5FnG, 0xfcefa3f8U, 0x09, d, a, b, c);
    SHA1_STEP(0x1e, 5, 1, ATERM_MD5FnG, 0x676f02d9U, 0x0e, c, d, a, b);
    SHA1_STEP(0x1f, 5, 1, ATERM_MD5FnG, 0x8d2a4c8aU, 0x14, b, c, d, a);
    SHA1_STEP(0x20, 3, 5, ATERM_MD5FnH, 0xfffa3942U, 0x04, a, b, c, d);
    SHA1_STEP(0x21, 3, 5, ATERM_MD5FnH, 0x8771f681U, 0x0b, d, a, b, c);
    SHA1_STEP(0x22, 3, 5, ATERM_MD5FnH, 0x6d9d6122U, 0x10, c, d, a, b);
    SHA1_STEP(0x23, 3, 5, ATERM_MD5FnH, 0xfde5380cU, 0x17, b, c, d, a);
    SHA1_STEP(0x24, 3, 5, ATERM_MD5FnH, 0xa4beea44U, 0x04, a, b, c, d);
    SHA1_STEP(0x25, 3, 5, ATERM_MD5FnH, 0x4bdecfa9U, 0x0b, d, a, b, c);
    SHA1_STEP(0x26, 3, 5, ATERM_MD5FnH, 0xf6bb4b60U, 0x10, c, d, a, b);
    SHA1_STEP(0x27, 3, 5, ATERM_MD5FnH, 0xbebfbc70U, 0x17, b, c, d, a);
    SHA1_STEP(0x28, 3, 5, ATERM_MD5FnH, 0x289b7ec6U, 0x04, a, b, c, d);
    SHA1_STEP(0x29, 3, 5, ATERM_MD5FnH, 0xeaa127faU, 0x0b, d, a, b, c);
    SHA1_STEP(0x2a, 3, 5, ATERM_MD5FnH, 0xd4ef3085U, 0x10, c, d, a, b);
    SHA1_STEP(0x2b, 3, 5, ATERM_MD5FnH, 0x04881d05U, 0x17, b, c, d, a);
    SHA1_STEP(0x2c, 3, 5, ATERM_MD5FnH, 0xd9d4d039U, 0x04, a, b, c, d);
    SHA1_STEP(0x2d, 3, 5, ATERM_MD5FnH, 0xe6db99e5U, 0x0b, d, a, b, c);
    SHA1_STEP(0x2e, 3, 5, ATERM_MD5FnH, 0x1fa27cf8U, 0x10, c, d, a, b);
    SHA1_STEP(0x2f, 3, 5, ATERM_MD5FnH, 0xc4ac5665U, 0x17, b, c, d, a);
    SHA1_STEP(0x30, 7, 0, ATERM_MD5FnI, 0xf4292244U, 0x06, a, b, c, d);
    SHA1_STEP(0x31, 7, 0, ATERM_MD5FnI, 0x432aff97U, 0x0a, d, a, b, c);
    SHA1_STEP(0x32, 7, 0, ATERM_MD5FnI, 0xab9423a7U, 0x0f, c, d, a, b);
    SHA1_STEP(0x33, 7, 0, ATERM_MD5FnI, 0xfc93a039U, 0x15, b, c, d, a);
    SHA1_STEP(0x34, 7, 0, ATERM_MD5FnI, 0x655b59c3U, 0x06, a, b, c, d);
    SHA1_STEP(0x35, 7, 0, ATERM_MD5FnI, 0x8f0ccc92U, 0x0a, d, a, b, c);
    SHA1_STEP(0x36, 7, 0, ATERM_MD5FnI, 0xffeff47dU, 0x0f, c, d, a, b);
    SHA1_STEP(0x37, 7, 0, ATERM_MD5FnI, 0x85845dd1U, 0x15, b, c, d, a);
    SHA1_STEP(0x38, 7, 0, ATERM_MD5FnI, 0x6fa87e4fU, 0x06, a, b, c, d);
    SHA1_STEP(0x39, 7, 0, ATERM_MD5FnI, 0xfe2ce6e0U, 0x0a, d, a, b, c);
    SHA1_STEP(0x3a, 7, 0, ATERM_MD5FnI, 0xa3014314U, 0x0f, c, d, a, b);
    SHA1_STEP(0x3b, 7, 0, ATERM_MD5FnI, 0x4e0811a1U, 0x15, b, c, d, a);
    SHA1_STEP(0x3c, 7, 0, ATERM_MD5FnI, 0xf7537e82U, 0x06, a, b, c, d);
    SHA1_STEP(0x3d, 7, 0, ATERM_MD5FnI, 0xbd3af235U, 0x0a, d, a, b, c);
    SHA1_STEP(0x3e, 7, 0, ATERM_MD5FnI, 0x2ad7d2bbU, 0x0f, c, d, a, b);
    SHA1_STEP(0x3f, 7, 0, ATERM_MD5FnI, 0xeb86d391U, 0x15, b, c, d, a);

    ctx->a0 += a;
    ctx->b0 += b;
    ctx->c0 += c;
    ctx->d0 += d;

    dataBytePtr = (u8*)data;
    for (i = 0; i < 0x40; i++) {
        *dataBytePtr++ = 0;
    }
}

// Sleep milliseconds alarm handler
void ATERMi_WaitAlarmHandler(OSAlarm* alarm, OSContext* context) {
    OSSendMessage((OSMessageQueue*)alarm->tag, 0, 0);
}

int ATERMi_ApConfigStart(u32 priority, u32 param_2, void (*timeFn)(ATERMStateUpdate*), ATERMiAllocFn alloc, ATERMiFreeFn free, u32 stackSize) {
    int iVar1;
    s64 sVar2;
    ATERMStateUpdate tsState;

    if ((1 <= ATERMi_state) && (ATERMi_state <= 5)) {
        return -10;
    } else {
        ATERMi_maxAPs = param_2;
        ATERMi_state = 7;
        ATERMi_progressCb = timeFn;
        ATERMi_alloc = alloc;
        ATERMi_free = free;
        ATERMi_stackSize = stackSize;
        ATERMi_threadStack = (*alloc)(stackSize);
        if (ATERMi_threadStack == NULL) {
            ATERMi_81698C94 = -1;
            return ATERMi_81698C94;
        } else {
            OSCreateThread(&AtermThread, ATERMi_AutoConfigThread, NULL, (u8*)ATERMi_threadStack + (ATERMi_stackSize & ~0b111), ATERMi_stackSize,
                           priority, 1);
            ATERMi_state = 1;
            ATERMi_someTimeMs = (u32)OSTicksToMilliseconds(OSGetTime()) + 60000;
            ATERMi_cancelled = FALSE;
            memset(&ATERMi_configResult, 0, 0xe8);
            ATERMi_getStateInternal(&tsState);
            (*ATERMi_progressCb)(&tsState);
            OSResumeThread(&AtermThread);
            ATERMi_threadIsRunning = 1;
            return ATERMi_threadIsRunning;
        }
    }
}

int ATERMi_ApConfigEnd(void) {
    int oldState;
    int iVar2;
    undefined4 tick0;
    s64 sVar3;
    OSMessage msgBufB;
    OSMessage msgDstB;
    OSMessage msgBufA;
    OSMessage msgDstA;

    ATERMStateUpdate tsState;

    OSMessageQueue queueB;
    OSMessageQueue queueA;
    OSAlarm alarmB;
    OSAlarm alarmA;

    if (ATERMi_threadIsRunning != 0) {
        oldState = ATERMi_state;
        ATERMi_cancelled = TRUE;

        while (1 <= ATERMi_state && ATERMi_state <= 5)
            ATERMi_Wait(100);

        ATERMi_Wait(500);
        while (!OSIsThreadTerminated(&AtermThread)) {
            OSJoinThread(&AtermThread, NULL);
        }
        if (ATERMi_threadStack != NULL) {
            (*ATERMi_free)(ATERMi_threadStack);
            ATERMi_threadStack = NULL;
        }
        ATERMi_threadIsRunning = 0;
        if (oldState != ATERMi_state) {
            ATERMi_getStateInternal(&tsState);
            (*ATERMi_progressCb)(&tsState);
        }
    }
    return 1;
}

int ATERMi_ApConfigGetState(ATERMStateUpdate* state) {
    ATERMi_getStateInternal(state);
    return 1;
}
int ATERMi_ApConfigGetResult(ATERMiApConfigResult* result) {
    memcpy(result, &ATERMi_configResult, sizeof(ATERMiApConfigResult));
    return 1;
}
u32 ATERMi_ApConfigGetVersion() {
    u32 major = 1;
    u32 minor = 6;
    return (major << 8) | minor;
}
