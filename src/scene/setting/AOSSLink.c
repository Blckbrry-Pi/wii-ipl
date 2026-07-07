#include "scene/setting/AOSS/AOSSLink.h"

// #include <revolution/os/OSAlarm.h>
// #include <revolution/os/OSInterrupt.h>
// #include <revolution/os/OSMessage.h>
// #include <revolution/os/OSTime.h>
#include <revolution/ncd.h>
#include <revolution/os.h>

#include <private/wd.h>
#include <revolution/wd.h>

#include <string.h>

static NCDIfConfig AOSSi_ifconfig;  // AOSS_810BD4A8
static NCDIpConfig AOSSi_ipconfig;  // AOSS_810BD608

static AOSSiCallback AOSSi_cb;  // lbl_81698C58
static SOFree AOSSi_free;       // lbl_81698C54
static SOAlloc AOSSi_alloc;     // lbl_81698C50
static u8 AOSSi_ip_dns2[4];     // lbl_81698C4C
static u8 AOSSi_ip_dns1[4];     // lbl_81698C48
static u8 AOSSi_ip_gateway[4];  // lbl_81698C44
static u8 AOSSi_ip_netmask[4];  // lbl_81698C40
static u8 AOSSi_ip_addr[4];     // lbl_81698C3C
BOOL AOSSi_cancel_flag;         // AOSSi_cancel_flag

void AOSSi_Cancel() {
    AOSSi_cancel_flag = TRUE;
}

// Sleep milliseconds alarm handler (super internal)
void AOSSi_SleepAlarmHandler(OSAlarm* alarm, OSContext* context) {
    OSSendMessage((OSMessageQueue*)alarm->tag, 0, 0);
    return;
}
// Sleep milliseconds (internal)
void AOSSi_SleepInternal(u32 ms) {
    OSMessage msgBuf[1];
    OSMessage msgRecv;
    OSMessageQueue msgQ;
    OSAlarm alarm;
    OSInitMessageQueue(&msgQ, msgBuf, 1);
    OSCreateAlarm(&alarm);
    OSSetAlarmTag(&alarm, (u32)&msgQ);
    OSSetAlarm(&alarm, OSMillisecondsToTicks(ms), AOSSi_SleepAlarmHandler);
    OSReceiveMessage(&msgQ, &msgRecv, 1);
}

void* AOSSi_Alloc(u32 size) {
    if (AOSSi_alloc != NULL)
        return (*AOSSi_alloc)(0, size);
    else
        return NULL;
}
void AOSSi_Free(void* buf) {
    if (AOSSi_free != NULL)
        (*AOSSi_free)(0, buf, 0);
}

// Copy to u8 array IP addr from u32 IP addr (internal)
void AOSSi_CopyIP(u8* ip8Ptr, u32 ip32) {
    int i;
    for (i = 0; i < 4; i++) {
        *ip8Ptr = ip32 >> ((3 - i) * 8);
        ip8Ptr++;
    }
}
// #define AOSSi_CopyIP(IP8_PTR, IP32) AOSS_813FD25C(IP8_PTR, IP32);

int AOSSi_SetNCDIPAddr(u32 addr, u32 netmask, u32 gateway, u32 dns1, u32 dns2) {
    AOSSi_CopyIP(AOSSi_ip_addr, addr);
    AOSSi_CopyIP(AOSSi_ip_netmask, netmask);
    AOSSi_CopyIP(AOSSi_ip_gateway, gateway);
    AOSSi_CopyIP(AOSSi_ip_dns1, dns1);
    AOSSi_CopyIP(AOSSi_ip_dns2, dns2);
    return 0;
}

int AOSSi_InitLocal(SOAlloc alloc, SOFree free) {
    BOOL level;

    if (alloc == NULL || free == NULL)
        return -1;

    level = OSDisableInterrupts();
    AOSSi_alloc = alloc;
    AOSSi_free = free;
    AOSSi_cancel_flag = 0;
    OSRestoreInterrupts(level);
    return 0;
}
int AOSSi_EndLocal() {
    BOOL level;
    level = OSDisableInterrupts();
    AOSSi_alloc = NULL;
    AOSSi_free = NULL;
    OSRestoreInterrupts(level);
    return 0;
}

typedef struct {
    u16 idx;   // 0x00
    u8 value;  // 0x02
} SupportedRateEntry;
SupportedRateEntry AOSSi_supportedRates[12] = {
    {0x0001, 0x02}, {0x0002, 0x04}, {0x0004, 0x0b}, {0x0008, 0x0c}, {0x0010, 0x12}, {0x0020, 0x16},
    {0x0040, 0x18}, {0x0080, 0x24}, {0x0100, 0x30}, {0x0200, 0x48}, {0x0400, 0x60}, {0x0800, 0x6c},
};

int AOSSi_WLANGetBSSList(AOSSiBSSList** bssListOut) {
    int rateCount;
    int rateIdx;
    int bssIdx;
    undefined4 retVal;
    AOSSiBSSList* bssList;
    int scanRet;
    int lockID;
    u16* scanBuf;
    WDBssDesc* currBss;
    u32 bssCount;
    int startupAttempts;
    int scanAttempts;
    int cleanupAttempts;
    int unlockAttempts;

    u16 enableChannel;
    void* setDst;
    u8 setFill;
    u32 setSize;

    u8 macaddr[6];
    WDScanParam scanParams ALIGN32;
    WD_Info wdinfo ALIGN32;

    retVal = -1;
    startupAttempts = 0;
    scanAttempts = 0;
    bssIdx = 0;
    cleanupAttempts = 0;
    unlockAttempts = 0;
    if (!AOSSi_alloc || !AOSSi_free) {
        return -1;
    }

    lockID = NCDLockWirelessDriver();
    if (lockID <= 0) {
        return -1;
    }
    while (TRUE) {
        if (WD_Startup(3) == 0)
            break;
        if (startupAttempts > 10)
            goto UNLOCK_AND_CLEANUP;
        startupAttempts++;
        AOSSi_SleepInternal(10);
    }
    if (WD_GetInfo(&wdinfo) == 0) {
        memcpy(macaddr, wdinfo.MAC, 6);
    }
    scanBuf = AOSSi_Alloc(0x3200);
    if (scanBuf) {
        memset(scanBuf, 0, 0x3200);

        scanParams.channelBit = wdinfo.enableChannel;
        scanParams.maxChannelTime = 40;

        memset(scanParams.bssid, 0xFF, sizeof(scanParams.bssid));
        scanParams.type = 0;
        scanParams.ssidLength = 0;
        memset(scanParams.ssid, 0, 0x20);
        memset(scanParams.ssidMask, 0xff, 0x20);

        while (TRUE) {
            scanRet = WD_Scan(&scanParams, (u8*)scanBuf, 0x3200);
            if (scanRet != 0 && scanRet != 0x80008004)
                break;

            bssCount = *scanBuf;
            if (bssCount != 0) {
                bssList = (AOSSiBSSList*)AOSSi_Alloc((bssCount - 1) * sizeof(AOSSiBSSListEntry) + sizeof(AOSSiBSSListEntry) + 4);
                if (!bssList) {
                    retVal = -1;
                    break;
                }

                bssList->count = bssCount;
                currBss = (WDBssDesc*)(scanBuf + 1);
                for (bssIdx = 0; bssIdx < (int)bssCount; bssIdx++) {
                    bssList->bss[bssIdx].SSIDLength = currBss->ssidLength;
                    memcpy(bssList->bss[bssIdx].SSID, currBss->ssid, 0x20);
                    bssList->bss[bssIdx].channel = currBss->channel;
                    memcpy(bssList->bss[bssIdx].BSSID, currBss->bssid, 6);

                    rateCount = 0;
                    for (rateIdx = 0; rateIdx < 0xc; rateIdx++) {
                        if (currBss->rateSet.support & AOSSi_supportedRates[rateIdx].idx) {
                            bssList->bss[bssIdx].ratesBuf[rateCount] = AOSSi_supportedRates[rateIdx].value;
                            if (currBss->rateSet.basic & AOSSi_supportedRates[rateIdx].idx) {
                                bssList->bss[bssIdx].ratesBuf[rateCount] |= 0x80;
                            }
                            rateCount++;
                        }
                    }

                    bssList->bss[bssIdx].rateCount = rateCount;
                    bssList->bss[bssIdx].beaconPeriod = currBss->beaconPeriod;
                    if ((currBss->capabilities & 3) == 1) {
                        bssList->bss[bssIdx].capabilities = 1;
                    } else if ((currBss->capabilities & 3) == 2) {
                        bssList->bss[bssIdx].capabilities = 2;
                    } else {
                        bssList->bss[bssIdx].capabilities = 0;
                    }
                    currBss = (WDBssDesc*)((u8*)currBss + currBss->length * 2);
                }
                *bssListOut = bssList;
                retVal = 0;
                break;
            }
            if (AOSSi_cancel_flag == 1) {
                retVal = -1;
                break;
            }
            scanAttempts++;
            if (scanAttempts > 10) {
                bssList = (AOSSiBSSList*)AOSSi_Alloc(offsetof(AOSSiBSSList, bss) + sizeof(AOSSiBSSListEntry));
                if (!bssList) {
                    retVal = -1;
                } else {
                    bssList->count = 0;
                    retVal = 0;
                    *bssListOut = bssList;
                }
                break;
            }
            AOSSi_SleepInternal(100);
        }
        if (AOSSi_free) {
            AOSSi_free(0, scanBuf, 0);
        }
    }
    while (WD_Cleanup() != 0) {
        if (cleanupAttempts > 10) {
            retVal = -1;
            break;
        }
        cleanupAttempts = cleanupAttempts + 1;
        AOSSi_SleepInternal(10);
    }
UNLOCK_AND_CLEANUP:
    while (NCDUnlockWirelessDriver(lockID) != 0) {
        if (unlockAttempts > 10) {
            retVal = -1;
            break;
        }
        unlockAttempts++;
        AOSSi_SleepInternal(10);
    }
    return retVal;
}

inline void AOSSi_ClearIfCfg(NCDIfConfig* ifcfg) {
    memset(ifcfg, 0, sizeof(NCDIfConfig));
    ifcfg->selectedMedia = 1;
    ifcfg->netif.wireless.rateset = 0;
    ifcfg->netif.wireless.configMethod = 0;
}

inline void AOSSi_WLANSetupForNone(NCDIfConfig* ifcfg, u32* in) {
    if (in[10] == 0) {
        ifcfg->netif.wireless.config.manual.privacy.mode = NCD_MODE_NONE;
    }
}

inline void AOSSi_SetupIpCfg(NCDIpConfig* ipcfg) {
    memset(ipcfg, 0, sizeof(NCDIpConfig));
    ipcfg->adjust.maxTransferUnit = 1300;
    ipcfg->adjust.tcpRetransTimeout = 100;
    ipcfg->adjust.dhcpRetransCount = 4;
    ipcfg->useDhcp = FALSE;
    ipcfg->useProxy = FALSE;
    memcpy(ipcfg->ip.addr, AOSSi_ip_addr, sizeof(AOSSi_ip_addr));
    memcpy(ipcfg->ip.netmask, AOSSi_ip_netmask, sizeof(AOSSi_ip_netmask));
    memcpy(ipcfg->ip.gateway, AOSSi_ip_gateway, sizeof(AOSSi_ip_gateway));
    memcpy(ipcfg->ip.dns1, AOSSi_ip_dns1, sizeof(AOSSi_ip_dns1));
    memcpy(ipcfg->ip.dns2, AOSSi_ip_dns2, sizeof(AOSSi_ip_dns2));
}

int AOSSi_WLANConnect(AOSSInfoSSID* in, AOSSiWLANConnectResult* out) {
    int ret;
    int ifDecidedCheckCnt;
    NCDIfConfig* ifcfg;
    NCDIpConfig* ipcfg;
    WD_Info wdinfo ALIGN32;
    u32 ipCfgSize;
    int ipCfgFill;

    ifcfg = &AOSSi_ifconfig;
    ifDecidedCheckCnt = 0;

    AOSSi_ClearIfCfg(ifcfg);
    if (in->keyLen == 0) {
        ifcfg->netif.wireless.config.manual.privacy.mode = NCD_MODE_NONE;
    }
    if (in->keyLen == 5) {
        // WEP40
        ifcfg->netif.wireless.config.manual.privacy.mode = NCD_MODE_WEP40;
        ifcfg->netif.wireless.config.manual.privacy.wep40.keyId = 0;
        memcpy(ifcfg->netif.wireless.config.manual.privacy.wep40.key, in->key, in->keyLen);
    } else if (in->keyLen == 13) {
        // WEP104
        ifcfg->netif.wireless.config.manual.privacy.mode = NCD_MODE_WEP104;
        ifcfg->netif.wireless.config.manual.privacy.wep104.keyId = 0;
        memcpy(ifcfg->netif.wireless.config.manual.privacy.wep104.key, in->key, in->keyLen);
    } else if (in->keyLen == 0x10) {
        // TKIP?
        return -1;
    } else {
        // AES?
        return -1;
    }

    ifcfg->netif.wireless.config.manual.ssidLength = (u8)in->ssidLen;
    memcpy(ifcfg->netif.wireless.config.manual.ssid, in->ssid, in->ssidLen);

    ret = 0;
    ipcfg = (NCDIpConfig*)&AOSSi_ipconfig;

    AOSSi_SetupIpCfg(ipcfg);

    if (NCDSetIpConfig(ipcfg) != 0)
        ret = -1;

    if (ret == 0)
        if (NCDSetIfConfig(&AOSSi_ifconfig) != 0)
            ret = -1;

    if (ret == 0) {
        while (NCDIsInterfaceDecided() == FALSE) {
            if (AOSSi_cancel_flag == TRUE) {
                ret = -1;
                break;
            }
            if (ifDecidedCheckCnt > 600) {
                ret = -1;
                break;
            }
            ifDecidedCheckCnt++;
            AOSSi_SleepInternal(10);
        }
    }
    if (ret == 0) {
        out->connected = TRUE;
        if (WD_GetInfo(&wdinfo) == 0) {
            out->channel = wdinfo.channel;
        }
        out->ssidLen = ifcfg->netif.wireless.config.aoss.wep40.ssidLength;
        memcpy(out->ssid, ifcfg->netif.wireless.config.aoss.wep40.ssid, out->ssidLen);
    } else {
        out->connected = 0;
    }
    return ret;
}

void AOSSi_Sleep(u32 ms) {
    AOSSi_SleepInternal(ms);
}
int AOSSi_Status(int status) {
    if (AOSSi_cb != NULL)
        (*AOSSi_cb)(status);
    return 0;
}

void AOSS_SetCallback(AOSSiCallback cb) {
    BOOL level;
    level = OSDisableInterrupts();
    AOSSi_cb = cb;
    OSRestoreInterrupts(level);
}
