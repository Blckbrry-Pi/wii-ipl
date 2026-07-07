#include "scene/setting/AOSS/AOSS.h"
#include "scene/setting/AOSS/AOSSPrivate.h"

#include "scene/setting/AOSS/AOSSLink.h"

#include <stdlib.h>

// #include <revolution/os/OSAlarm.h>
// #include <revolution/os/OSInterrupt.h>
// #include <revolution/os/OSMessage.h>
// #include <revolution/os/OSTime.h>
#include <revolution/ncd.h>
#include <revolution/os.h>
#include <revolution/soex.h>
#include <revolution/wd.h>

#include <string.h>

#define ADDR_32(A, B, C, D) (((u8)(A) << 0x18) | ((u8)(B) << 0x10) | ((u8)(C) << 0x08) | ((u8)(D) << 0x00))

AOSSiInternalState AOSSi_internalState;
AOSSiSparseWirelessSettings AOSSi_sparseWireless[2];
AOSSi_Struct0x5b8 AOSSi_extraConfig;
u8 AOSSi_wepNegotiationKey[104];  // they made it 104 bytes instead of 104 bits lol
u32 AOSSi_CRC32Table[0x100];

AOSS_Struct8C* AOSSi_packetBuf;                // lbl_81698C8C
int AOSSi_soStarted;                           // lbl_81698C88
u32 AOSSi_error;                               // lbl_81698C84
int AOSSi_allowEnc;                            // lbl_81698C80
AOSSi_MessageIdent AOSSi_sentMessageIdent;     // lbl_81698C78
AOSSiWLANConnectResult* AOSSi_socketStartRes;  // lbl_81698C74
AOSSiBSSList* AOSSi_bssList;                   // lbl_81698C74

int AOSSi_sockfd = -1;  // lbl_81697200
char AOSSi_MELCO[6] = "MELCO";
int AOSSi_status = -1;

void AOSSi_InitCRC32Table(undefined4 _, u32* randBuf);
u32 AOSSi_CRC32(s32 dataSize, u8* data);

inline void AOSS_EnsureSOClose() {
    if (AOSSi_sockfd != -1) {
        SOClose(AOSSi_sockfd);
    }
}
inline int AOSS_EnsureSOCleanup() {
    if (AOSSi_soStarted == 1) {
        AOSSi_soStarted = 0;
        if (SOCleanup() < 0) {
            return -1;
        }
    }
    return 0;
}
inline void AOSS_CleanupAllocations() {
    if (AOSSi_socketStartRes != NULL) {
        AOSSi_Free(AOSSi_socketStartRes);
        AOSSi_socketStartRes = NULL;
    }
    if (AOSSi_bssList != NULL) {
        AOSSi_Free(AOSSi_bssList);
        AOSSi_bssList = NULL;
    }
}

void AOSSi_KeySchedRC4(AOSSi_RC4InternalState* state, u8* seedData, u32 seedDataSize, int stateSize);

// https://en.wikipedia.org/wiki/RC4#Pseudo-random_generation_algorithm_(PRGA)
// This is slightly different than the one described, it uses a custom
// permutation size and index modulus, rather than always being 256
inline void AOSSi_RNGXorRC4(void* in, u8* out, u32 dataSize, AOSSi_RC4InternalState* rc4State) {
    u8 newOffsA, newOffsB;
    u8 valA, valB;
    int i;
    u8* buf;

    for (i = 0; i < dataSize; i++) {
        buf = rc4State->perm;
        newOffsA = ((1 + rc4State->offsA) % rc4State->permLen);
        valA = buf[newOffsA];

        newOffsB = ((valA + rc4State->offsB) % rc4State->permLen);
        valB = buf[newOffsB];

        rc4State->offsA = newOffsA;
        rc4State->offsB = newOffsB;

        buf[newOffsB] = valA;
        buf[newOffsA] = valB;

        out[i] = buf[(valA + valB) % rc4State->permLen] ^ ((u8*)in)[i];
    }
}

// void AOSSi_PRNGXorRC4Step(AOSSi_RC4InternalState* rc4State, u8* in, u8* out) {
//     u8* buf;
//     u8 valA, valB;
//     u8 newOffsA, newOffsB;
//     // u8* srcbuf;
//     // u8* dstbuf;

//     // u8* buf;
//     // u8 newOffsA, newOffsB;
//     // u8 valA, valB;

//     buf = rc4State->perm;
//     newOffsA = ((1 + rc4State->offsA) % rc4State->permLen);
//     valA = buf[newOffsA];

//     newOffsB = ((valA + rc4State->offsB) % rc4State->permLen);
//     valB = buf[newOffsB];

//     rc4State->offsA = newOffsA;
//     rc4State->offsB = newOffsB;

//     buf[newOffsB] = valA;
//     buf[newOffsA] = valB;

//     *out = buf[(valA + valB) % rc4State->permLen] ^ *in;
// }

inline void AOSSi_Crypt(void* in, u8* out, u32 dataSize, AOSSi_RC4InternalState* rc4State, u8* randSeed) {
    int i;
    u8 newOffsA, newOffsB;
    u8* buf;
    u8 valA, valB;

    memcpy(AOSSi_wepNegotiationKey, randSeed, 2);
    memcpy(AOSSi_wepNegotiationKey + 2, &AOSSi_sentMessageIdent, 8);

    AOSSi_KeySchedRC4(rc4State, AOSSi_wepNegotiationKey, 10, dataSize);

    for (i = 0; i < dataSize; i++) {
        buf = rc4State->perm;
        newOffsA = ((1 + rc4State->offsA) % rc4State->permLen);
        valA = buf[newOffsA];

        newOffsB = ((valA + rc4State->offsB) % rc4State->permLen);
        valB = buf[newOffsB];

        rc4State->offsA = newOffsA;
        rc4State->offsB = newOffsB;

        buf[newOffsB] = valA;
        buf[newOffsA] = valB;

        out[i] = buf[(valA + valB) % rc4State->permLen] ^ ((u8*)in)[i];
    }
}

inline u32 AOSS_min(u32 a, u32 b) {
    if (a < b)
        return a;
    else if (a == b)
        return b;
    else if (a > b)
        return b;
    else if (a != b)
        return b;
    return b;
}

inline int AOSSi_Poll(u16 pollDelay) {
    SOPollFD pollFds[2];

    // volatile s64 pollDelaySecs64;
    s32 pollDelaySecs;
    s32 pollDelayMicros;
    s64 pollDelaySecsTicks, pollDelayMicrosTicks;

    // pollDelaySecs64 = pollDelay / 1000;
    // pollDelaySecs64 = (u32)pollDelaySecs64 / 1000;
    pollDelaySecs = pollDelay / 1000;
    pollDelayMicros = (pollDelay - (pollDelaySecs * 1000)) * 1000;
    // pollDelayParts.micros = pollDelayMicros;
    pollDelaySecsTicks = OSSecondsToTicks(pollDelaySecs);
    pollDelayMicrosTicks = OSMicrosecondsToTicks(pollDelayMicros);

    pollFds[0].fd = AOSSi_sockfd;
    pollFds[0].events = SO_POLLRDNORM;
    pollFds[0].revents = 0;

    pollFds[1].fd = AOSSi_sockfd;
    pollFds[1].revents = 0;
    pollFds[1].events = SO_POLLRDNORM;

    return SOPoll(pollFds, 1, pollDelaySecsTicks + pollDelayMicrosTicks);
}

u32 AOSS_GetAddrAfterGateway(u32 subnetBase, u32 gatewaySubaddr, u32 netmask) {
    gatewaySubaddr = subnetBase | (gatewaySubaddr + 1);
    if (gatewaySubaddr >= (subnetBase | ~netmask))
        return subnetBase | 1;
    else
        return gatewaySubaddr;
}
u32 AOSS_GetIPAddr(u32 gateway, u32 netmask) {
    u32 subnetBase = gateway & netmask;
    u32 maxAddr = subnetBase | ~netmask;
    u32 gatewaySubaddr = gateway & ~netmask;
    gatewaySubaddr = subnetBase | (gatewaySubaddr + 1);
    if (gatewaySubaddr >= maxAddr)
        return subnetBase | 1;
    else
        return gatewaySubaddr;
}
int AOSS_SetIPAddr(u32 ipAddr) {
    u32 netmask = *(u32*)&AOSSi_internalState.netmask;
    u32 gateway = *(u32*)&AOSSi_internalState.gateway;
    return AOSSi_SetNCDIPAddr(ipAddr, netmask, gateway, 0, 0);
}

#define AOSS_InitFail(CFG, CFG_VALUE)                                                                                                                \
    {                                                                                                                                                \
        (CFG)->aossResult = CFG_VALUE;                                                                                                               \
        AOSS_CleanupAllocations();                                                                                                                   \
        return -1;                                                                                                                                   \
    }

#define TIMEOUT(CFG, MS)                                                                                                                             \
    for (remMs = MS; remMs != 0; remMs = (u16)(remMs - sleepStep)) {                                                                                 \
        if (AOSSi_cancel_flag == 1) {                                                                                                                \
            (CFG)->aossResult = 0x0f;                                                                                                                \
            AOSS_CleanupAllocations();                                                                                                               \
            return -1;                                                                                                                               \
        }                                                                                                                                            \
        AOSSi_Sleep(AOSS_min(100, remMs));                                                                                                           \
        sleepStep = AOSS_min(100, remMs);                                                                                                            \
    }

int AOSS_Init_old(AOSSiConfig* cfg);

int AOSSi_Init(AOSSiConfig* cfg) {
    int retVal;

    if (cfg->retryCount_Scan == 0 || cfg->retryCount_Scan < -1 || cfg->retryInterval_Scan < -1 || cfg->retryCount_Comu == 0 ||
        cfg->retryCount_Comu < -1 || cfg->retryInterval_Comu < -1 || cfg->selectInterval < -1 || cfg->productInfo.dataLen == 0 ||
        cfg->productInfo.dataLen > 0x100 || cfg->productInfo.deviceName[cfg->productInfo.dataLen - 1] != '\0') {
        retVal = -1;
    } else {
        retVal = 0;
    }
    if (retVal == -1)
        AOSS_InitFail(cfg, 0x0f);

    AOSSi_packetBuf = (AOSS_Struct8C*)AOSSi_Alloc(0x5f8);
    if (AOSSi_packetBuf == NULL)
        AOSS_InitFail(cfg, 0x0f);

    retVal = AOSS_Init_old(cfg);

    AOSSi_Free(AOSSi_packetBuf);
    AOSS_CleanupAllocations();
    AOSS_EnsureSOClose();

    if (AOSS_EnsureSOCleanup() != 0)
        AOSS_InitFail(cfg, 0x0f);

    if (retVal == -1 && AOSSi_cancel_flag == TRUE) {
        retVal = -2;
    }
    return retVal;
}

int AOSSi_CopyCheckSparseWireless(AOSSiConfig* cfg);

int AOSSi_ConnectToSSID(AOSSInfoSSID* ssid, AOSSiWLANConnectResult* res);

int AOSSi_SendNegotiationMsg(int negotiationState, void* unk, AOSSi_MessageIdent* msgIdents, int s);
int AOSSi_SendNegotiationMsg0(void*, AOSSi_MessageIdent* msgIdents, int s);
int AOSSi_SendKeyTransferMsg(void*, AOSSi_MessageIdent* msgIdents, int s);
int AOSSi_SendFinishNegotiationMsg(void*, AOSSi_MessageIdent* msgIdents, int s);

int AOSSi_NameXor(u8* buf, int bufSize, char* name, int nameBytes);

int AOSSi_HandleNegotiationPacket(int param_1, AOSS_Struct8C* param_2, int* count, AOSSi_MessageIdent* msgIdents, int fd);

inline int AOSS_TryConnectToSSID(s16 tries, AOSSInfoSSID* ssidInfo, AOSSiConfig* cfg, s16 timeoutMs) {
    u32 sleepStep;
    u32 remMs;

    u32 i;
    int result;
    for (i = 0; i < tries; i++) {
        result = AOSSi_ConnectToSSID(ssidInfo, AOSSi_socketStartRes);
        if (result == -1)
            AOSS_InitFail(cfg, 0x0f);
        if (result == 0 && AOSSi_socketStartRes->connected == TRUE)
            break;
        TIMEOUT(cfg, timeoutMs);
        if (AOSSi_cancel_flag == 1)
            AOSS_InitFail(cfg, 0x0f);
    }
    return 0;
}

inline void AOSSi_SetRetriesExceededErr(int negotiationState) {
    if (negotiationState == NEGOTIATION_STATE_UNK0) {
        AOSSi_error = 0xf;
    } else if (negotiationState == NEGOTIATION_STATE_KEY_TRANSFER) {
        AOSSi_error = 0x10;
    } else if (negotiationState != NEGOTIATION_STATE_KEY_TRANSFER) {
        AOSSi_error = 0x11;
    } else if (negotiationState == NEGOTIATION_STATE_REBOOTING_STACK) {
        AOSSi_error = 0x11;
    } else if (negotiationState == NEGOTIATION_STATE_DONE) {
        AOSSi_error = 0x11;
    } else {
        AOSSi_error = 0x11;
    }
}

inline int AOSSi_Recv(int soFd, AOSS_Struct8C* packet) {
    SOSockAddr recvFrom;
    int soRecvRes;
    recvFrom.len = 8;
    soRecvRes = SORecvFrom(AOSSi_sockfd, &packet->recvPacket, 1500, 0, &recvFrom);
    packet->sockFd = AOSSi_sockfd;
    packet->soRecvRes = SONtoHs(soRecvRes);
    return soRecvRes;
}

inline void AOSSi_SetupMessageIds(AOSSiConfig* cfg, void* msgIds) {
    int i;
    void* base = msgIds;

    int offset;
    int randVal;
    for (i = 0; i < 3; i++) {
        offset = i * sizeof(AOSSi_MessageIdent);

        memcpy(((AOSSi_MessageIdent*)((u8*)msgIds + offset))->mac, cfg->macAddr, 6);
        randVal = rand();

        ((AOSSi_MessageIdent*)((u8*)base + offset))->idx = randVal;
        ((AOSSi_MessageIdent*)((u8*)base + offset))->idx = SOHtoNs(randVal);
    }
}

inline int AOSSi_Init_old_Cleanup(AOSSiConfig* cfg, int res) {
    u8 statusValue;

    AOSS_EnsureSOClose();
    AOSSi_sockfd = -1;
    if (AOSS_EnsureSOCleanup() != 0)
        AOSS_InitFail(cfg, 0x0f);

    if (res != 0) {
        switch (AOSSi_error) {
            case 0x0f:
                statusValue = 0x03;
                break;
            case 0x10:
                statusValue = 0x04;
                break;
            case 0x11:
                statusValue = 0x05;
                break;
            case 0x14:
                statusValue = 0x07;
                break;
            case 0x15:
                statusValue = 0x08;
                break;
            default:
                statusValue = 0x0f;
                break;
        }
        AOSS_InitFail(cfg, statusValue);
    } else {
        if (AOSSi_CopyCheckSparseWireless(cfg) != 0)
            AOSS_InitFail(cfg, 0x06);
        return 0;
    }
}

void AOSSi_UpdateStatus(int newStatus) {
    if (AOSSi_status != newStatus) {
        AOSSi_status = newStatus;
        AOSSi_Status(AOSSi_status);
    }
}

void AOSSi_loadDefault(u16* out, s16 in, s16 defaultVal) {
    *out = in;
    if (in == -1)
        *out = defaultVal;
}

// typedef struct {
//     s16 a;
//     s16 b;
// } StructAAAAAAAAAAAAA;
int AOSS_Init_old(AOSSiConfig* cfg) {
    AOSS_Struct8C* piVar1;
    AOSSiSOPacket* packet;
    u8 uVar2;
    int connectRet;
    int negotiationState;
    s16 uVar7;
    int soRecvRes;
    u32 maskedGateway;
    u32 sleepStep;
    int selectInterval;
    int unaff_r14;
    s16 sVar12;
    int connectAttempts;
    s16 uVar13;
    int iVar11;
    int res;
    s64 pollDelaySecsTicks;
    s64 pollDelayMicrosTicks;
    u32 uVar15;
    u16 i16;
    int i;
    u32 remMs;

    int randVal;
    int checkApRes;
    int setNcdIpAddrRes;
    int connectToSsidRes;
    int newNegotiationState;

    SOSockAddrIn bindLoc;

    undefined4 auStack_d8[5];
    AOSSi_MessageIdent negotiationMsgIdents[3];

    // struct {
    //     int count;  // 0x0
    //     s16 retryMax_Comu;
    //     s16 retryInterval_Comu;
    //     s16 retryMax_Scan;       // 0x8
    //     s16 retryInterval_Scan;  // 0xa
    // } ALIGN32 state;
    AOSSiTimeouts state;
    struct {
        u32 defaultAddr;
        u32 defaultGateway;
    } defaults;

    AOSSInfoSSID ssidInfo;

    state.retryCfgScan = (AOSSiRetryCfg){-1, -1};
    state.retryCfgComu = (AOSSiRetryCfg){0, 0};
    state.count = 0;
    negotiationState = NEGOTIATION_STATE_UNK0;
    memset(negotiationMsgIdents, 0, 0x18);

    AOSSi_loadDefault(&state.retryCfgScan.count, cfg->retryCount_Scan, 10);
    AOSSi_loadDefault(&state.retryCfgComu.count, cfg->retryCount_Comu, 10);
    AOSSi_loadDefault(&state.retryCfgScan.interval, cfg->retryInterval_Scan, 100);
    AOSSi_loadDefault(&state.retryCfgComu.interval, cfg->retryInterval_Comu, 100);

    selectInterval = cfg->selectInterval;
    if (selectInterval == -1)
        selectInterval = 2000;

    memset(&AOSSi_sentMessageIdent, 0, 8);
    AOSSi_error = 1;
    memset(&AOSSi_internalState, 0, 0x1c);
    AOSSi_internalState.deviceName = cfg->productInfo.deviceName;
    AOSSi_internalState.prodInfoDataLen = cfg->productInfo.dataLen;
    AOSSi_internalState.allowedModes = cfg->mode & AOSS_MODE_ALL;
    AOSSi_internalState.prodInfoDataType = cfg->productInfo.dataType;
    AOSSi_internalState.negotiatedModes = 0;
    AOSSi_internalState.gateway = ADDR_32(192, 168, 11, 1);
    AOSSi_internalState.unk_0x18 = 0;
    if ((AOSSi_internalState.allowedModes & AOSS_MODE_WEP40) != AOSS_MODE_WEP40) {
        AOSSi_error = 0x13;
        AOSS_InitFail(cfg, 0x0f);
    }

    sVar12 = 0;
    // uVar7 = state.uStack_138;
    // if (AOSS_status != 0) {
    //     AOSS_status = 0;
    //     AOSSi_Status(AOSS_status);
    // }
    AOSSi_UpdateStatus(0);
    uVar7 = state.retryCfgScan.count;
    while (TRUE) {
        if (AOSSi_bssList != NULL) {
            AOSSi_Free(AOSSi_bssList);
            AOSSi_bssList = NULL;
        }
        if (AOSSi_WLANGetBSSList(&AOSSi_bssList) == -1)
            AOSS_InitFail(cfg, 0x0f);
        if (AOSSi_cancel_flag == 1)
            AOSS_InitFail(cfg, 0x0f);
        checkApRes = AOSS_CheckAP(AOSSi_bssList);
        if (checkApRes == 4)
            AOSS_InitFail(cfg, 0x02);
        if (checkApRes == 0)
            break;
        if (sVar12 >= uVar7)
            AOSS_InitFail(cfg, 0x01);
        TIMEOUT(cfg, state.retryCfgScan.interval);
        if (AOSSi_cancel_flag == 1)
            AOSS_InitFail(cfg, 0x0f);
        sVar12++;
    }
    if (AOSSi_status != 1) {
        AOSSi_status = 1;
        AOSSi_Status(AOSSi_status);
    }
    memset(&ssidInfo, 0, 0x3c);
    ssidInfo.ssidLen = strlen("ESSID-AOSS");
    memcpy(ssidInfo.ssid, "ESSID-AOSS", ssidInfo.ssidLen);
    ssidInfo.unk_0x24 = 1;

    ssidInfo.keyLen = strlen(AOSSi_MELCO);
    if (ssidInfo.keyLen > 0xd) {
        if (ssidInfo.keyLen > 0x10) {
        } else if (ssidInfo.keyLen > 0x40) {
        } else {
        }
    } else {
        memcpy(ssidInfo.key, AOSSi_MELCO, ssidInfo.keyLen);
    }
    setNcdIpAddrRes =
        AOSSi_SetNCDIPAddr(ADDR_32(192, 168, 11, 101), ADDR_32(255, 255, 255, 0), ADDR_32(192, 168, 11, 1), ADDR_32(0, 0, 0, 0), ADDR_32(0, 0, 0, 0));
    if (setNcdIpAddrRes != 0) {
        AOSSi_error = 0xc;
        AOSS_InitFail(cfg, 0x0f);
    }
    if (setNcdIpAddrRes != 0)
        AOSS_InitFail(cfg, 0x0f);

    AOSSi_socketStartRes = (void*)AOSSi_Alloc(0x58);
    if (AOSSi_socketStartRes == NULL)
        AOSS_InitFail(cfg, 0x0f);

    memset(AOSSi_socketStartRes, 0, 0x58);

    uVar7 = state.retryCfgScan.count;
    for (uVar13 = 0; (short)uVar13 < (short)uVar7; uVar13++) {
        connectToSsidRes = AOSSi_ConnectToSSID(&ssidInfo, AOSSi_socketStartRes);
        if (connectToSsidRes == -1)
            AOSS_InitFail(cfg, 0x0f);
        if (connectToSsidRes == 0) {
            if (connectToSsidRes != 0 || AOSSi_socketStartRes->connected == TRUE)
                break;
        }

        TIMEOUT(cfg, state.retryCfgScan.interval);
        if (AOSSi_cancel_flag == 1)
            AOSS_InitFail(cfg, 0x0f);
    }
    if (uVar13 == state.retryCfgScan.count)
        AOSS_InitFail(cfg, 0x0f);

    AOSS_CleanupAllocations();

    AOSSi_SetupMessageIds(cfg, negotiationMsgIdents);

    AOSSi_sockfd = SOSocket(2, 2, 0);
    if (AOSSi_sockfd < 0) {
        AOSS_InitFail(cfg, 0x0f);
    } else if (unaff_r14 < 0) {
        AOSSi_error = 0xb;
        AOSS_InitFail(cfg, 0x0f);
    }
    memset(&bindLoc, 0, 8);
    bindLoc.family = 2;
    bindLoc.addr.addr = SOGetHostID();
    bindLoc.port = SOHtoNs(0x5790);
    bindLoc.len = 8;
    if (SOBind(AOSSi_sockfd, (SOSockAddr*)&bindLoc) < 0)
        AOSS_InitFail(cfg, 0x0f);

    iVar11 = state.retryCfgComu.count;
    defaults.defaultAddr = ADDR_32(192, 168, 11, 101);
    defaults.defaultGateway = ADDR_32(192, 168, 11, 1);

    while (TRUE) {
        piVar1 = AOSSi_packetBuf;
        memset(auStack_d8, 0, 0x14);
        auStack_d8[4] = defaults.defaultAddr;
        auStack_d8[0] = defaults.defaultGateway;
        do {
            if (negotiationState == NEGOTIATION_STATE_KEY_TRANSFER && (AOSSi_internalState.unk_0x18 != 1)) {
                AOSS_EnsureSOClose();
                AOSSi_sockfd = -1;

                if (AOSS_EnsureSOCleanup() != 0)
                    AOSS_InitFail(cfg, 0x0f);

                if (AOSS_SetIPAddr(AOSS_GetIPAddr(AOSSi_internalState.gateway, AOSSi_internalState.netmask)) != 0) {
                    AOSSi_error = 0xc;
                    AOSS_InitFail(cfg, 0x0f);
                }
                AOSSi_internalState.unk_0x18 = 1;
                AOSSi_socketStartRes = (void*)AOSSi_Alloc(0x58);
                if (AOSSi_socketStartRes == NULL)
                    AOSS_InitFail(cfg, 0x0f);
                memset(AOSSi_socketStartRes, 0, 0x58);

                // Try to connect ESSID-AOSS
                uVar7 = state.retryCfgScan.count;
                for (connectAttempts = 0; (s16)connectAttempts < uVar7; connectAttempts++) {
                    connectRet = AOSSi_ConnectToSSID(&ssidInfo, AOSSi_socketStartRes);
                    if (connectRet == -1)
                        AOSS_InitFail(cfg, 0x0f);
                    if (connectRet == 0) {
                        if (connectRet != 0 || AOSSi_socketStartRes->connected == TRUE)
                            break;
                    }
                    TIMEOUT(cfg, state.retryCfgScan.interval);
                    if (AOSSi_cancel_flag == 1)
                        AOSS_InitFail(cfg, 0x0f);
                }

                AOSSi_sockfd = SOSocket(2, 2, 0);
                if (AOSSi_sockfd < 0)
                    AOSS_InitFail(cfg, 0x0f);
                memset(&bindLoc, 0, 8);
                bindLoc.family = 2;
                bindLoc.addr.addr = SOGetHostID();
                bindLoc.port = SOHtoNs(0x5790);
                bindLoc.len = 8;
                if (SOBind(AOSSi_sockfd, (SOSockAddr*)&bindLoc) < 0)
                    AOSS_InitFail(cfg, 0x0f);
            }

            if (AOSSi_SendNegotiationMsg(negotiationState, auStack_d8, negotiationMsgIdents, AOSSi_sockfd) == -1) {
                AOSSi_error = negotiationState + 0x1000;
                AOSS_InitFail(cfg, 0x0f);
            }
            memset(piVar1, 0, 0x5f8);

            if (AOSSi_Poll(selectInterval) > 0)
                break;
            state.count++;
            if (state.count > iVar11) {
                AOSSi_SetRetriesExceededErr(negotiationState);
                res = -1;
                goto CLEANUP;
            }
            TIMEOUT(cfg, state.retryCfgComu.interval);
            if (AOSSi_cancel_flag == TRUE)
                AOSS_InitFail(cfg, 0x0f);
        } while (TRUE);

        AOSSi_Recv(AOSSi_sockfd, piVar1);

        newNegotiationState = AOSSi_HandleNegotiationPacket(negotiationState, piVar1, &state.count, negotiationMsgIdents, AOSSi_sockfd);
        if (newNegotiationState == NEGOTIATION_STATE_DONE) {
            res = 0;
            break;
        }
        if (newNegotiationState == NEGOTIATION_STATE_ERR) {
            res = -1;
            break;
        }

        if (negotiationState != newNegotiationState) {
            if (newNegotiationState != NEGOTIATION_STATE_REBOOTING_STACK)
                continue;

            AOSS_EnsureSOClose();
            AOSSi_sockfd = -1;
            if (AOSS_EnsureSOCleanup() != 0)
                AOSS_InitFail(cfg, 0x0f);
            AOSSi_UpdateStatus(4);
            if (AOSSi_bssList != NULL) {
                AOSSi_Free(AOSSi_bssList);
                AOSSi_bssList = NULL;
            }
            if (AOSSi_WLANGetBSSList(&AOSSi_bssList) == -1)
                AOSS_InitFail(cfg, 0x0f);
            if (AOSSi_cancel_flag == 1)
                AOSS_InitFail(cfg, 0x0f);
            negotiationState = AOSS_CheckAP(AOSSi_bssList);
            if (negotiationState == 4)
                AOSS_InitFail(cfg, 0x02);
            if (negotiationState != 0)
                AOSS_InitFail(cfg, 0x01);
            AOSSi_socketStartRes = (void*)AOSSi_Alloc(0x58);
            if (AOSSi_socketStartRes == NULL)
                AOSS_InitFail(cfg, 0x0f);
            memset(AOSSi_socketStartRes, 0, 0x58);

            uVar7 = state.retryCfgScan.count;
            for (connectAttempts = 0; (s16)connectAttempts < uVar7; connectAttempts++) {
                connectRet = AOSSi_ConnectToSSID(&ssidInfo, AOSSi_socketStartRes);
                if (connectRet == -1)
                    AOSS_InitFail(cfg, 0x0f);
                if (connectRet == 0)
                    if (connectRet != 0 || AOSSi_socketStartRes->connected == TRUE)
                        break;
                TIMEOUT(cfg, state.retryCfgScan.interval);
                if (AOSSi_cancel_flag == 1)
                    AOSS_InitFail(cfg, 0x0f);
            }
            AOSS_CleanupAllocations();
            AOSSi_sockfd = SOSocket(2, 2, 0);
            if (AOSSi_sockfd < 0)
                AOSS_InitFail(cfg, 0x0f);
            memset(&bindLoc, 0, 8);
            bindLoc.family = 2;
            bindLoc.addr.addr = SOGetHostID();
            bindLoc.port = SOHtoNs(0x5790);
            bindLoc.len = 8;
            if (SOBind(AOSSi_sockfd, (SOSockAddr*)&bindLoc) < 0)
                AOSS_InitFail(cfg, 0x0f);

            negotiationState = newNegotiationState;
        } else {
            negotiationState = newNegotiationState;

            if (state.count > state.retryCfgComu.count) {
                AOSSi_SetRetriesExceededErr(negotiationState);
                res = -1;
                break;
            }

            TIMEOUT(cfg, state.retryCfgComu.interval);
            if (AOSSi_cancel_flag == 1)
                AOSS_InitFail(cfg, 0x0f);
        }
    }

CLEANUP:
    return AOSSi_Init_old_Cleanup(cfg, res);
}

int AOSSi_checkWEP40(AOSSiWirelessSettings* cfgSettings, AOSSiSparseWirelessSettings* sparseSettings) {
    int sizeLeft;
    char* keyPtr;
    u8 ch;
    memcpy(cfgSettings->wep40Key[0], sparseSettings->wep40.key[0], sparseSettings->wep40.keyChunkSize);
    memcpy(cfgSettings->wep40Key[1], sparseSettings->wep40.key[1], sparseSettings->wep40.keyChunkSize);
    memcpy(cfgSettings->wep40Key[2], sparseSettings->wep40.key[2], sparseSettings->wep40.keyChunkSize);
    memcpy(cfgSettings->wep40Key[3], sparseSettings->wep40.key[3], sparseSettings->wep40.keyChunkSize);
    sizeLeft = strlen(sparseSettings->wep40.ssid);
    keyPtr = sparseSettings->wep40.ssid;
    while (sizeLeft-- > 0) {
        ch = *(keyPtr++);
        if (' ' > ch || ch > '\x7f')
            return -1;
    }
    return 0;
}
int AOSSi_checkWEP104(AOSSiWirelessSettings* cfgSettings, AOSSiSparseWirelessSettings* sparseSettings) {
    int sizeLeft;
    char* keyPtr;
    u8 ch;
    memcpy(cfgSettings->wep104Key[0], sparseSettings->wep104.key[0], sparseSettings->wep104.keyChunkSize);
    memcpy(cfgSettings->wep104Key[1], sparseSettings->wep104.key[1], sparseSettings->wep104.keyChunkSize);
    memcpy(cfgSettings->wep104Key[2], sparseSettings->wep104.key[2], sparseSettings->wep104.keyChunkSize);
    memcpy(cfgSettings->wep104Key[3], sparseSettings->wep104.key[3], sparseSettings->wep104.keyChunkSize);
    sizeLeft = strlen(sparseSettings->wep104.ssid);
    keyPtr = sparseSettings->wep104.ssid;
    while (sizeLeft-- > 0) {
        ch = *(keyPtr++);
        if (' ' > ch || ch > '\x7f')
            return -1;
    }
    return 0;
}

int AOSSi_checkKeyTKIP(AOSSiWirelessSettings* cfgSettings, AOSSiSparseWirelessSettings* sparseSettings) {
    int sizeLeft;
    char* keyPtr;
    u8 ch;
    keyPtr = sparseSettings->tkip.key;
    sizeLeft = sparseSettings->tkip.keySize + -1;
    while (sizeLeft-- > 0) {
        ch = *(keyPtr++);
        if (' ' > ch || ch > '\x7f')
            return -1;
    }
    return 0;
}
int AOSSi_checkSsidTKIP(AOSSiWirelessSettings* cfgSettings, AOSSiSparseWirelessSettings* sparseSettings) {
    int sizeLeft;
    char* ssidPtr;
    u8 ch;

    sizeLeft = strlen(sparseSettings->tkip.ssid);
    ssidPtr = sparseSettings->tkip.ssid;
    while (sizeLeft-- > 0) {
        ch = *(ssidPtr++);
        if (' ' > ch || ch > '\x7f')
            return -1;
    }
    return 0;
}

int AOSSi_checkKeyAES(AOSSiWirelessSettings* cfgSettings, AOSSiSparseWirelessSettings* sparseSettings) {
    int sizeLeft;
    char* keyPtr;
    u8 ch;
    keyPtr = sparseSettings->aes.key;
    sizeLeft = sparseSettings->aes.keySize + -1;
    while (sizeLeft-- > 0) {
        ch = *(keyPtr++);
        if (' ' > ch || ch > '\x7f')
            return -1;
    }
    return 0;
}
int AOSSi_checkSsidAES(AOSSiWirelessSettings* cfgSettings, AOSSiSparseWirelessSettings* sparseSettings) {
    int sizeLeft;
    char* ssidPtr;
    u8 ch;

    sizeLeft = strlen(sparseSettings->aes.ssid);
    ssidPtr = sparseSettings->aes.ssid;
    while (sizeLeft-- > 0) {
        ch = *(ssidPtr++);
        if (' ' > ch || ch > '\x7f')
            return -1;
    }
    return 0;
}

int AOSSi_CopyCheckSparseWireless(AOSSiConfig* cfg) {
    AOSSiSparseWirelessSettings* sparseSettings = &AOSSi_sparseWireless[0];
    AOSSiWirelessSettings* cfgSettings = &cfg->wirelessSettings;
    if (cfgSettings == NULL) {
        return 0xffffffff;
    }
    cfg->mode = AOSSi_internalState.allowedModes & AOSSi_internalState.negotiatedModes;
    memset(cfgSettings, 0, sizeof(AOSSiWirelessSettings));

    if ((cfg->mode & AOSS_MODE_WEP40) != 0) {
        if (AOSSi_checkWEP40(cfgSettings, sparseSettings) != 0)
            goto ASCII_ERROR;
        memcpy(cfgSettings->wep40SSID, sparseSettings->wep40.ssid, strlen(sparseSettings->wep40.ssid));
    }
    if ((cfg->mode & AOSS_MODE_WEP104) != 0) {
        if (AOSSi_checkWEP104(cfgSettings, sparseSettings) != 0)
            goto ASCII_ERROR;
        memcpy(cfgSettings->wep104SSID, sparseSettings->wep104.ssid, strlen(sparseSettings->wep104.ssid));
    }
    if ((cfg->mode & AOSS_MODE_TKIP) != 0) {
        if (AOSSi_checkKeyTKIP(cfgSettings, sparseSettings) != 0)
            goto ASCII_ERROR;
        memcpy(cfgSettings->tkipKey, sparseSettings->tkip.key, sparseSettings->tkip.keySize);

        if (AOSSi_checkSsidTKIP(cfgSettings, sparseSettings) != 0)
            goto ASCII_ERROR;
        memcpy(cfgSettings->tkipSSID, sparseSettings->tkip.ssid, strlen(sparseSettings->tkip.ssid));
    }
    if ((cfg->mode & AOSS_MODE_AES) != 0) {
        if (AOSSi_checkKeyAES(cfgSettings, sparseSettings) != 0)
            goto ASCII_ERROR;
        memcpy(cfgSettings->aesKey, sparseSettings->aes.key, sparseSettings->aes.keySize);

        if (AOSSi_checkSsidAES(cfgSettings, sparseSettings) != 0)
            goto ASCII_ERROR;
        memcpy(cfgSettings->aesSSID, sparseSettings->aes.ssid, strlen(sparseSettings->aes.ssid));
    }
    cfg->aossResult = 0;
    return 0;
ASCII_ERROR:
    memset(cfgSettings, 0, sizeof(AOSSiWirelessSettings));
    return -1;
}

int AOSS_CheckAP(AOSSiBSSList* bssList) {
    u32 bssCount;
    size_t num;
    AOSSiBSSList* currList;
    u8* ssid;
    int i;
    int retVal;
    int countAOSS;

    bssCount = bssList->count;
    retVal = 0;
    countAOSS = 0;
    if (bssCount == 0) {
        return 5;
    } else {
        bssCount = bssCount > 0x40 ? 0x40 : bssCount;
        ssid = bssList->bss->SSID;
        currList = bssList;
        for (i = 0; i < (int)bssCount; i++) {
            if ((currList->bss[0].capabilities & 1) != 0 && currList->bss[0].SSIDLength == strlen("ESSID-AOSS")) {
                if (memcmp(ssid, "ESSID-AOSS", strlen("ESSID-AOSS")) == 0) {
                    countAOSS++;
                }
            }
            currList = (AOSSiBSSList*)((u8*)currList + sizeof(AOSSiBSSListEntry));
            ssid += sizeof(AOSSiBSSListEntry);
        }
        if (countAOSS > 1)
            retVal = 4;
        if (countAOSS == 0)
            retVal = 5;
    }
    return retVal;
}

int AOSSi_DecryptNegotiationMsg(AOSSiSOPacket*);

int AOSSi_HandleNegotiation0Packet(AOSSi_ChunkedPacketData* data, AOSSi_Struct0x5b8* somethingElse);
int AOSSi_HandleKeyTransferPacket(int, AOSSi_ChunkedPacketData*, int, AOSSiSparseWirelessSettings*, AOSSi_Struct0x5b8*);

int AOSSi_HandleNegotiation0Phase(int state, AOSS_Struct8C* packet, int* count, AOSSi_MessageIdent* msgIdents);
int AOSSi_HandleKeyTransferPhase(int state, AOSS_Struct8C* packet, int* count, AOSSi_MessageIdent* msgIdents);
int AOSSi_HandleFinishNegotiation(int state, AOSS_Struct8C* packet, int* count, AOSSi_MessageIdent* msgIdents);

int AOSSi_HandleNegotiationPacket(int state, AOSS_Struct8C* packet, int* count, AOSSi_MessageIdent* msgIdents, int fd) {
    if (SONtoHs(packet->recvPacket.unk_0x00) < 1) {
        (*count)++;
        return state;
    }
    if (packet->recvPacket.unk_0x0f != 0x11) {
        (*count)++;
        return state;
    }
    if (AOSSi_DecryptNegotiationMsg(&packet->recvPacket) > 0) {
        (*count)++;
        return state;
    }
    switch (SONtoHs(packet->recvPacket.negotiationPhase)) {
        case NEGOTIATION_MSG_UNK0_RECV:
            state = AOSSi_HandleNegotiation0Phase(state, packet, count, msgIdents);
            break;
        case NEGOTIATION_MSG_KEY_TRANSFER_RECV:
            state = AOSSi_HandleKeyTransferPhase(state, packet, count, msgIdents);
            break;
        case NEGOTIATION_MSG_FINISH_RECV:
            state = AOSSi_HandleFinishNegotiation(state, packet, count, msgIdents);
            break;
    }

    return state;
}

inline int AOSSi_CheckMsgIdent(AOSS_Struct8C* packet, AOSSi_MessageIdent* msgIdent) {
    int res;
    u16 receivedIdx, expectedIdx;
    res = 0;
    AOSSi_NameXor((u8*)&packet->recvPacket.msgIdent, 8, AOSSi_MELCO, strlen(AOSSi_MELCO));
    if (memcmp(msgIdent->mac, packet->recvPacket.msgIdent.mac, 6) != 0) {
        res = -1;
    } else {
        receivedIdx = SONtoHs(packet->recvPacket.msgIdent.idx);
        expectedIdx = SONtoHs(msgIdent->idx);
        if (expectedIdx + 1 != receivedIdx) {
            res = -2;
        }
    }
    return res;
}

int AOSSi_HandleNegotiation0Phase(int state, AOSS_Struct8C* packet, int* count, AOSSi_MessageIdent* msgIdents) {
    int useEncryption;
    u16 mode;
    int handleRes;

    u32* finishData;
    AOSSi_ChunkedPacketData* packetData;

    // Check that we're in the right state to handle this type of message
    if (state != NEGOTIATION_STATE_UNK0) {
        (*count)++;
        return state;
    }

    packetData = (AOSSi_ChunkedPacketData*)&packet->recvPacket.packetData;

    // Check that we're actually the intended target of the message
    if (AOSSi_CheckMsgIdent(packet, &msgIdents[0]) < 0) {
        (*count)++;
        return state;
    }

    // Check that the packet isn't empty
    if (SONtoHs(packetData->dataSize) == 0) {
        (*count)++;
        return state;
    }

    // Fail if we received a finish command already
    if (packetData->cmd == NEGOTIATION_CMD_FINISH) {
        finishData = (u32*)packetData->data;
        if (SONtoHl(*finishData) == -2) {
            AOSSi_error = 0x14;
        } else if (SONtoHl(*finishData) == -3) {
            AOSSi_error = 0x15;
        } else {
            AOSSi_error = 0x18;
        }
        return NEGOTIATION_STATE_ERR;
    }

    // Check 5
    if (packetData->cmd != NEGOTIATION_CMD_PHASE0_INFO_RECV) {
        (*count)++;
        return state;
    }

    handleRes = AOSSi_HandleNegotiation0Packet((AOSSi_ChunkedPacketData*)packetData->data, &AOSSi_extraConfig);
    if (handleRes < 0) {
        if (handleRes == -2) {
            AOSSi_error = 0x16;
            return NEGOTIATION_STATE_ERR;
        } else {
            (*count)++;
            return state;
        }
    }

    mode = SONtoHs(packet->recvPacket.encMode);
    useEncryption = FALSE;
    if ((mode & 0x10) != 0) {
        useEncryption = TRUE;
    }
    AOSSi_allowEnc = useEncryption;
    *count = 0;
    return NEGOTIATION_STATE_KEY_TRANSFER;
}

int AOSSi_HandleKeyTransferPhase(int state, AOSS_Struct8C* packet, int* count, AOSSi_MessageIdent* msgIdents) {
    int handleRes;

    AOSSi_ChunkedPacketData* packetData;

    // Check that we're in the right state to handle this type of message
    if (state != NEGOTIATION_STATE_KEY_TRANSFER) {
        (*count)++;
        return state;
    }

    packetData = (AOSSi_ChunkedPacketData*)&packet->recvPacket.packetData;

    // Check that we're actually the intended target of the message
    if (AOSSi_CheckMsgIdent(packet, &msgIdents[1]) < 0) {
        (*count)++;
        return state;
    }

    // Check that the packet isn't empty
    if (SONtoHs(packetData->dataSize) == 0) {
        (*count)++;
        return state;
    }

    // Fail if we received a finish command already
    if (packetData->cmd == NEGOTIATION_CMD_FINISH) {
        if (SONtoHl(*(u32*)(packetData->data)) == -2) {
            AOSSi_error = 0x14;
        } else {
            AOSSi_error = SONtoHl(*(u32*)(packetData->data)) == -3 ? 0x15 : 0x18;
        }
        return NEGOTIATION_STATE_ERR;
    }

    // Logic
    memset(&AOSSi_sparseWireless, 0, sizeof(AOSSi_sparseWireless));
    handleRes = AOSSi_HandleKeyTransferPacket(0, packetData, SONtoHs(packet->recvPacket.dataSize), AOSSi_sparseWireless, &AOSSi_extraConfig);
    if (handleRes < 0) {
        (*count)++;
        return state;
    }

    if ((AOSSi_internalState.negotiatedModes & AOSSi_internalState.allowedModes) == AOSS_MODE_NONE) {
        return state;
    }

    *count = 0;
    return NEGOTIATION_STATE_REBOOTING_STACK;
}

int AOSSi_HandleFinishNegotiation(int state, AOSS_Struct8C* packet, int* count, AOSSi_MessageIdent* msgIdents) {
    int iVar6;

    AOSSi_ChunkedPacketData* packetData;

    // Check that we're in the right state to handle this type of message
    if (state != NEGOTIATION_STATE_REBOOTING_STACK) {
        (*count)++;
        return state;
    }

    packetData = (AOSSi_ChunkedPacketData*)&packet->recvPacket.packetData;

    // Check that we're actually the intended target of the message
    if (AOSSi_CheckMsgIdent(packet, &msgIdents[2]) < 0) {
        (*count)++;
        return state;
    }

    // Check 4
    if (packetData->cmd != NEGOTIATION_CMD_FINISH) {
        (*count)++;
        return state;
    }

    // Check that the packet isn't empty
    if (SONtoHs(packetData->dataSize) == 0) {
        (*count)++;
        return state;
    }

    // Logic
    if (SONtoHl(*(u32*)packetData->data) == 0) {
        return NEGOTIATION_STATE_DONE;
    } else {
        if (SONtoHl(*(u32*)packetData->data) == -2) {
            AOSSi_error = 0x14;
            return NEGOTIATION_STATE_ERR;
        } else if (SONtoHl(*(u32*)packetData->data) == -3) {
            AOSSi_error = 0x15;
            return NEGOTIATION_STATE_ERR;
        } else {
            AOSSi_error = 0x18;
            return NEGOTIATION_STATE_ERR;
        }
    }
}

int AOSSi_CheckMsgIdentValid(u16 negotiationPhase, AOSSi_MessageIdent* msgIdent);

int AOSSi_DecryptNegotiationMsg(AOSSiSOPacket* packet) {
    int ret;
    u8 expectedCrc32;

    AOSSi_PacketData* packetData;
    u8* unencPacketData;
    u8 calculatedCrc32;

    int dataSize;

    AOSSi_MessageIdent msgIdent;
    AOSSi_RC4InternalState rc4State;

    int i;
    packetData = &packet->packetData;

    memcpy(&msgIdent, &packet->msgIdent, 8);
    if (AOSSi_NameXor((u8*)&msgIdent, 8, AOSSi_MELCO, strlen(AOSSi_MELCO)) == -1) {
        AOSSi_error = 2;
        return -100;
    }

    ret = AOSSi_CheckMsgIdentValid(SONtoHs(packet->negotiationPhase), &msgIdent);
    if (ret != 0) {
        return ret;
    }
    if (SONtoHs(packet->negotiationPhase) == NEGOTIATION_MSG_LOAD_ENCKEY) {
        memcpy(&AOSSi_sentMessageIdent, &msgIdent, 8);
    }
    if ((SONtoHs(packet->encMode) & AOSS_MODE_ALL) == AOSS_MODE_NONE) {
        return 0;
    }

    dataSize = SONtoHs(packetData->enc.dataSize);
    unencPacketData = (u8*)AOSSi_Alloc(dataSize);
    if (unencPacketData == NULL) {
        AOSSi_error = 2;
        return 100;
    }
    expectedCrc32 = packet->crc32;
    rc4State.perm = AOSSi_Alloc(dataSize);
    if (rc4State.perm == NULL) {
        ret = -1;
        AOSSi_error = 2;
        goto FINISH;
    }

    AOSSi_Crypt(packetData->enc.data, unencPacketData, dataSize, &rc4State, packetData->enc.randSeed);
    // memcpy(AOSSi_wepNegotiationKey, packetData->enc.randSeed, 2);
    // memcpy(AOSSi_wepNegotiationKey + 2, &AOSSi_sentMessageIdent, 8);

    // AOSSi_KeySchedRC4(&rc4State, AOSSi_wepNegotiationKey, 10, dataSize);
    // AOSSi_RNGXorRC4(packetData->enc.data, unencPacketData, dataSize, &rc4State);

    calculatedCrc32 = AOSSi_CRC32(dataSize, unencPacketData);
    if (calculatedCrc32 != expectedCrc32) {
        AOSSi_error = 0x12;
        AOSSi_Free(rc4State.perm);
        ret = -1;
    } else {
        AOSSi_Free(rc4State.perm);
        ret = 0;
    }

FINISH:
    if (ret < 0) {
        AOSSi_Free(unencPacketData);
        ret = 200;
        if (AOSSi_error == 2) {
            ret = 100;
        }
    } else {
        memcpy(packetData, unencPacketData, dataSize);
        packet->dataSize = SOHtoNs(dataSize);
        AOSSi_Free(unencPacketData);
        ret = 0;
    }
    return ret;
}

int AOSSi_CheckMsgIdentValid(u16 negotiationPhase, AOSSi_MessageIdent* msgIdent) {
    u8* macAddr;
    BOOL someNonzero;
    int retVal;

    macAddr = AOSSi_sentMessageIdent.mac;

    someNonzero = FALSE;
    retVal = 0;
    if (macAddr[0] != '\0') {
        someNonzero = TRUE;
    } else if (macAddr[1] != '\0') {
        someNonzero = TRUE;
    } else if (macAddr[2] != '\0') {
        someNonzero = TRUE;
    } else if (macAddr[3] != '\0') {
        someNonzero = TRUE;
    } else if (macAddr[4] != '\0') {
        someNonzero = TRUE;
    } else if (macAddr[5] != '\0') {
        someNonzero = TRUE;
    }
    if (someNonzero) {
        if (memcmp(AOSSi_sentMessageIdent.mac, msgIdent->mac, 6) != 0) {
            retVal = 1;
        }
    } else if (negotiationPhase != NEGOTIATION_MSG_LOAD_ENCKEY) {
        retVal = 2;
    }
    return retVal;
}

int AOSSi_HandleNegotiation0Packet(AOSSi_ChunkedPacketData* data, AOSSi_Struct0x5b8* param_2) {
    int chunksize;
    u8* currInner;
    int i;
    u32 num;
    AOSSi_ExtendedChunkedPacketData* curr;

    memset(param_2, 0, 0x104);
    curr = (AOSSi_ExtendedChunkedPacketData*)data;
    while (TRUE) {
        chunksize = SONtoHs(curr->size);
        if (chunksize <= 0)
            return -1;
        switch (curr->cmd) {
            case 0:
                memcpy(param_2->data[0], curr->data, chunksize);
                break;
            case 1:
                memcpy(param_2->data[1], curr->data, chunksize);
                break;
            case 2:
                memcpy(param_2->data[2], curr->data, chunksize);
                break;
            case 3:
            case 4:
                if ((int)SONtoHs(curr->data[0]) <= 0)
                    return -2;
                break;
            case 5:
                currInner = curr->data;
                num = 0;
                for (i = 0; i < chunksize; i++) {
                    num <<= 8;
                    num += *currInner++;
                }
                AOSSi_internalState.gateway = SONtoHl(num);
                break;
            case 6:
                currInner = curr->data;
                num = 0;
                for (i = 0; i < chunksize; i++) {
                    num <<= 8;
                    num += *currInner++;
                }
                AOSSi_internalState.netmask = SONtoHl(num);
                break;
            default:
                return -1;
        }
        if (curr->nextOffset == 0)
            break;
        curr = (AOSSi_ExtendedChunkedPacketData*)((u8*)data + SONtoHs(curr->nextOffset));
    }
    return 0;
}

enum {
    KEYTRANS_CMD_WEP40_KEY0 = 0x10,
    KEYTRANS_CMD_WEP40_KEY1 = 0x11,
    KEYTRANS_CMD_WEP40_KEY2 = 0x12,
    KEYTRANS_CMD_WEP40_KEY3 = 0x13,
    KEYTRANS_CMD_WEP40_SSID = 0x15,

    KEYTRANS_CMD_WEP104_KEY0 = 0x20,
    KEYTRANS_CMD_WEP104_KEY1 = 0x21,
    KEYTRANS_CMD_WEP104_KEY2 = 0x22,
    KEYTRANS_CMD_WEP104_KEY3 = 0x23,
    KEYTRANS_CMD_WEP104_SSID = 0x25,

    KEYTRANS_CMD_TKIP_KEY = 0x30,
    KEYTRANS_CMD_TKIP_SSID = 0x35,

    KEYTRANS_CMD_AES_KEY = 0x40,
    KEYTRANS_CMD_AES_SSID = 0x45,

    KEYTRANS_CMD_UNKPROTOCOL_DATA = 0x70,
};

int AOSSi_HandleKeyTransferWEP(AOSSi_KeyTransferPacketChunk* baseChunk, AOSSiSparseWirelessWEP* wep) {
    AOSSi_KeyTransferPacketChunk* currChunk;
    AOSSi_KeyTransferPacketSubChunk* subChunk;
    u8 cmd;
    u32 subchunkSize;

    currChunk = baseChunk;
    subChunk = baseChunk->subchunks;
    do {
        subchunkSize = SONtoHs(subChunk->dataSize);
        switch (subChunk->cmd) {
            case KEYTRANS_CMD_WEP40_KEY0:
            case KEYTRANS_CMD_WEP40_KEY1:
            case KEYTRANS_CMD_WEP40_KEY2:
            case KEYTRANS_CMD_WEP40_KEY3:
                if (subchunkSize > 5)
                    return -1;
                break;
            case KEYTRANS_CMD_WEP104_KEY0:
            case KEYTRANS_CMD_WEP104_KEY1:
            case KEYTRANS_CMD_WEP104_KEY2:
            case KEYTRANS_CMD_WEP104_KEY3:
                if (subchunkSize > 0xd)
                    return -1;
                break;
            case KEYTRANS_CMD_WEP40_SSID:
            case KEYTRANS_CMD_WEP104_SSID:
                if (subchunkSize > 0x21)
                    return -1;
                break;
        }
        switch (subChunk->cmd) {
            case KEYTRANS_CMD_WEP40_KEY0:
            case KEYTRANS_CMD_WEP104_KEY0:
                memcpy(wep->key[0], subChunk->data, subchunkSize);
                wep->keyChunkSize = subchunkSize;
                break;
            case KEYTRANS_CMD_WEP40_KEY1:
            case KEYTRANS_CMD_WEP104_KEY1:
                memcpy(wep->key[1], subChunk->data, subchunkSize);
                wep->keyChunkSize = subchunkSize;
                break;
            case KEYTRANS_CMD_WEP40_KEY2:
            case KEYTRANS_CMD_WEP104_KEY2:
                memcpy(wep->key[2], subChunk->data, subchunkSize);
                wep->keyChunkSize = subchunkSize;
                break;
            case KEYTRANS_CMD_WEP40_KEY3:
            case KEYTRANS_CMD_WEP104_KEY3:
                memcpy(wep->key[3], subChunk->data, subchunkSize);
                wep->keyChunkSize = subchunkSize;
                break;
            case KEYTRANS_CMD_WEP40_SSID:
            case KEYTRANS_CMD_WEP104_SSID:
                if (subchunkSize != 0 && (subChunk->data[subchunkSize - 1] != '\0'))
                    return -1;
                memcpy(wep->ssid, subChunk->data, subchunkSize);
                break;
            default:
                return -1;
        }
        if (subChunk->nextOffset == 0)
            break;

        currChunk = (AOSSi_KeyTransferPacketChunk*)((u8*)baseChunk + SONtoHs(subChunk->nextOffset));
        subChunk = (AOSSi_KeyTransferPacketSubChunk*)(currChunk->subchunks);
    } while (TRUE);
    return 0;
}

int AOSSi_HandleKeyTransferWPA(AOSSi_KeyTransferPacketChunk* baseChunk, AOSSiSparseWirelessTKIPAES* keyDst) {
    AOSSi_KeyTransferPacketChunk* currChunk;
    AOSSi_KeyTransferPacketSubChunk* subChunk;
    u8 cmd;
    u32 subchunkSize;

    currChunk = baseChunk;
    subChunk = baseChunk->subchunks;
    do {
        subchunkSize = SONtoHs(subChunk->dataSize);
        switch (subChunk->cmd) {
            case KEYTRANS_CMD_TKIP_KEY:
            case KEYTRANS_CMD_AES_KEY:
                if (subchunkSize > 0x40)
                    return -1;
                break;
            case KEYTRANS_CMD_TKIP_SSID:
            case KEYTRANS_CMD_AES_SSID:
                if (subchunkSize > 0x21)
                    return -1;
                break;
        }
        switch (subChunk->cmd) {
            case KEYTRANS_CMD_TKIP_KEY:
            case KEYTRANS_CMD_AES_KEY:
                memcpy(keyDst->key, subChunk->data, subchunkSize);
                keyDst->keySize = subchunkSize;
                break;

            case KEYTRANS_CMD_TKIP_SSID:
            case KEYTRANS_CMD_AES_SSID:
                if (subchunkSize != 0 && subChunk->data[subchunkSize - 1] != 0)
                    return -1;
                memcpy(keyDst->ssid, subChunk->data, subchunkSize);
                break;

            default:
                return -1;
        }
        if (subChunk->nextOffset == 0)
            break;

        currChunk = (AOSSi_KeyTransferPacketChunk*)((u8*)baseChunk + SONtoHs(subChunk->nextOffset));
        subChunk = (AOSSi_KeyTransferPacketSubChunk*)(currChunk->subchunks);
    } while (TRUE);
    return 0;
}
int AOSSi_HandleKeyTransferUnknown(AOSSi_KeyTransferPacketChunk* buf, AOSSi_Struct0x5b8* output, int ifIdx) {
    u32 subchunkSize;

    subchunkSize = SONtoHs(buf->subchunks->dataSize);
    if ((int)subchunkSize <= 0) {
        return -1;
    }
    if (buf->subchunks->cmd != KEYTRANS_CMD_UNKPROTOCOL_DATA) {
        return -1;
    }
    memcpy(output->data[ifIdx + 3], buf->subchunks->data, subchunkSize);
    return 0;
}

static u8 AOSSi_ifIdxCmdLookup[2] = {NEGOTIATION_CMD_KEYTRANS_IF0, NEGOTIATION_CMD_KEYTRANS_IF1};

int AOSSi_HandleKeyTransferPacket(int ifIdx, AOSSi_ChunkedPacketData* packet, int size, AOSSiSparseWirelessSettings* settings,
                                  AOSSi_Struct0x5b8* param_5) {
    u32 offsetToNextChunk;
    int retVal;
    int remaining;
    AOSSi_ChunkedPacketData* currChunk;
    u32 flags;

    flags = AOSS_MODE_NONE;
    if (size <= 0)
        return -2;

    currChunk = (AOSSi_ChunkedPacketData*)packet;
    while (TRUE) {
        if (currChunk->cmd == AOSSi_ifIdxCmdLookup[ifIdx])
            break;

        offsetToNextChunk = (u16)SONtoHs(currChunk->dataSize) + sizeof(AOSSi_ChunkedPacketData);
        size -= offsetToNextChunk;
        currChunk = (AOSSi_ChunkedPacketData*)((u8*)currChunk + offsetToNextChunk);
        if (size <= 0)
            return -4;
    }
    remaining = SONtoHs((currChunk++)->dataSize);
    do {
        switch (currChunk->cmd) {
            case NEGOTIATION_CMD_WEP40_INFO:
                retVal = AOSSi_HandleKeyTransferWEP((AOSSi_KeyTransferPacketChunk*)currChunk, &settings[ifIdx].wep40);
                flags |= AOSS_MODE_WEP40;
                break;
            case NEGOTIATION_CMD_WEP104_INFO:
                retVal = AOSSi_HandleKeyTransferWEP((AOSSi_KeyTransferPacketChunk*)currChunk, &settings[ifIdx].wep104);
                flags |= AOSS_MODE_WEP104;
                break;
            case NEGOTIATION_CMD_TKIP_INFO:
                retVal = AOSSi_HandleKeyTransferWPA((AOSSi_KeyTransferPacketChunk*)currChunk, &settings[ifIdx].tkip);
                flags |= AOSS_MODE_TKIP;
                break;
            case NEGOTIATION_CMD_AES_INFO:
                retVal = AOSSi_HandleKeyTransferWPA((AOSSi_KeyTransferPacketChunk*)currChunk, &settings[ifIdx].aes);
                flags |= AOSS_MODE_AES;
                break;
            case NEGOTIATION_CMD_UNKPROTOCOL_INFO:
                retVal = AOSSi_HandleKeyTransferUnknown((AOSSi_KeyTransferPacketChunk*)currChunk, param_5, ifIdx);
                break;
            default:
                retVal = -3;
                break;
        }
        if (retVal != 0)
            return retVal;

        offsetToNextChunk = (u16)SONtoHs(currChunk->dataSize) + 4;
        remaining -= offsetToNextChunk;
        currChunk = (AOSSi_ChunkedPacketData*)((u8*)currChunk + offsetToNextChunk);
        if (remaining <= 0) {
            break;
        }
    } while (TRUE);
    AOSSi_internalState.negotiatedModes |= flags;
    return 0;
}

int AOSSi_SendNegotiationMsg(int negotiationState, void* unk, AOSSi_MessageIdent* msgIdents, int s) {
    switch (negotiationState) {
        case NEGOTIATION_STATE_UNK0:
            AOSSi_UpdateStatus(2);
            return AOSSi_SendNegotiationMsg0(unk, msgIdents, s);

        case NEGOTIATION_STATE_KEY_TRANSFER:
            AOSSi_UpdateStatus(3);
            return AOSSi_SendKeyTransferMsg(unk, msgIdents, s);

        case NEGOTIATION_STATE_REBOOTING_STACK:
            AOSSi_UpdateStatus(5);
            return AOSSi_SendFinishNegotiationMsg(unk, msgIdents, s);

        default:
            return -1;
    }
}

inline int AOSSi_SendFinishNegotiationMsg(void* _, AOSSi_MessageIdent* msgIdents, int s) {
    u8 encMsgIdent[8];
    SOSockAddrIn sendDest;
    AOSSiSOPacket* packet = &AOSSi_packetBuf->sendPacket;
    memset(packet, 0, 0x5dc);
    memcpy(encMsgIdent, &msgIdents[2], 8);
    AOSSi_NameXor((u8*)&encMsgIdent, 8, AOSSi_MELCO, strlen(AOSSi_MELCO));

    AOSSi_Send(s, packet, &sendDest, NEGOTIATION_MSG_FINISH_SEND, 0, 0, 0, encMsgIdent, FALSE);
    return 0;
}

inline u32 AOSSi_CRC32Step(u32 curr, u8 data) {
    return (curr >> 8) ^ AOSSi_CRC32Table[(u8)(curr ^ data)];
}
inline u32 AOSSi_CRC32(s32 dataSize, u8* data) {
    s32 i;
    u32 curr = 0xFFFFFFFF;
    AOSSi_InitCRC32Table(0, AOSSi_CRC32Table);
    for (i = 0; i < dataSize; i++) {
        curr = AOSSi_CRC32Step(curr, data[i]);
    }
    return curr ^ 0xFFFFFFFF;
}
