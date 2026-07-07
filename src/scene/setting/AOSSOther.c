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
void AOSSi_InitCRC32Table(undefined4 _, u32* randBuf);

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
inline u32 AOSSi_CRC32_UnsignedSize(u32 dataSize, void* data) {
    s32 i;
    u32 curr = 0xFFFFFFFF;
    AOSSi_InitCRC32Table(0, AOSSi_CRC32Table);
    for (i = 0; i < dataSize; i++) {
        curr = AOSSi_CRC32Step(curr, ((u8*)data)[i]);
    }
    return curr ^ 0xFFFFFFFF;
}

#define ADDR_32(A, B, C, D) (((u8)(A) << 0x18) | ((u8)(B) << 0x10) | ((u8)(C) << 0x08) | ((u8)(D) << 0x00))

int AOSSi_NameXor(u8* buf, int bufSize, char* name, int nameBytes);
void AOSSi_KeySchedRC4(AOSSi_RC4InternalState* state, u8* seedData, u32 seedDataSize, int stateSize);

s16 AOSSi_BuildNegotiationMsg0(void* packetData);
// s16 AOSSi_BuildKeyTransferMsg(void* packetData);
u32 AOSSi_CRC32_UnsignedSize(u32 dataSize, void* data);

inline int AOSSi_SendNegotiationMsg0Inner(AOSSi_MessageIdent* msgIdent, int s, AOSSiSOPacket* _packet) {
    AOSSi_ChunkedPacketData* packetDataScratch;
    short dataSize;
    void* packetDataDst;

    SOSockAddrIn sendDest;
    u8 encMsgIdent[8];

    s16 totalSize;

    AOSSiSOPacket* packet = &AOSSi_packetBuf->sendPacket;
    memset(packet, 0, 0x5dc);

    packetDataScratch = (AOSSi_ChunkedPacketData*)AOSSi_Alloc(0x210);
    if (packetDataScratch == NULL) {
        AOSSi_error = 2;
        return -1;
    }
    memset(packetDataScratch, 0, 0x210);
    packetDataDst = &packet->packetData.enc;

    memcpy(&AOSSi_sentMessageIdent, msgIdent, 8);
    memcpy(encMsgIdent, &AOSSi_sentMessageIdent, 8);

    dataSize = AOSSi_BuildNegotiationMsg0(packetDataScratch->data);
    if (dataSize < 0) {
        AOSSi_error = 3;
        if (packetDataScratch != NULL) {
            AOSSi_Free(packetDataScratch);
        }
        return -1;
    }

    packetDataScratch->cmd = 0;
    packetDataScratch->dataSize = SOHtoNs(dataSize);
    dataSize += 4;
    memcpy(packetDataDst, packetDataScratch, dataSize);

    if (AOSSi_NameXor(encMsgIdent, 8, AOSSi_MELCO, 6) != 0) {
        AOSSi_error = 2;
        if (packetDataScratch != NULL)
            AOSSi_Free(packetDataScratch);
        return -1;
    }

    AOSSi_Send(s, packet, &sendDest, NEGOTIATION_MSG_LOAD_ENCKEY, dataSize, AOSS_MODE_ENC_SUPPORTED, 0, encMsgIdent, TRUE);

    if (packetDataScratch != NULL)
        AOSSi_Free(packetDataScratch);
    return 0;
}

int AOSSi_SendNegotiationMsg0(void* _, AOSSi_MessageIdent* msgIdents, int s) {
    return AOSSi_SendNegotiationMsg0Inner(&msgIdents[0], s, NULL);
}

int AOSSi_SendKeyTransferMsg(void* _, AOSSi_MessageIdent* msgIdents, int s) {
    AOSSi_PacketData* packetData;
    u32 dataSize;

    u8 crc32;
    undefined2 encMode;
    AOSSiSOPacket* packet;

    u8 encMsgIdent[8];

    AOSSi_KeyTransferReq keyTransferReq;
    SOSockAddrIn sendDest;
    u16 randSeed;

    AOSSi_RC4InternalState rc4State;

    u32 crcCurr;

    u8 newOffsA, newOffsB;
    u8 valA, valB;
    u8* unencBuf;
    u8* buf;
    int i;

    packet = &AOSSi_packetBuf->sendPacket;
    crc32 = 0;
    encMode = 0;
    memset(&keyTransferReq, 0, 8);
    memset(packet, 0, 0x5dc);

    packetData = &packet->packetData;

    keyTransferReq.cmd = NEGOTIATION_CMD_KEYTRANS_REQ;
    keyTransferReq.unk_0x1 = 0;
    keyTransferReq.dataSize = SOHtoNs(sizeof(keyTransferReq.data));
    keyTransferReq.data.allowedModes = AOSSi_internalState.allowedModes;
    keyTransferReq.data.allowedModes = SOHtoNl(AOSSi_internalState.allowedModes);
    dataSize = sizeof(AOSSi_KeyTransferReq);

    if (AOSSi_allowEnc == TRUE) {
        encMode = 1;

        crc32 = AOSSi_CRC32_UnsignedSize(dataSize, &keyTransferReq);

        rc4State.perm = AOSSi_Alloc(sizeof(AOSSi_KeyTransferReq));
        if (rc4State.perm != NULL) {
            randSeed = rand();
            memcpy(packetData->enc.randSeed, &randSeed, sizeof(randSeed));

            memcpy(AOSSi_wepNegotiationKey, packetData->enc.randSeed, 2);
            memcpy(AOSSi_wepNegotiationKey + 2, &AOSSi_sentMessageIdent, 8);

            AOSSi_KeySchedRC4(&rc4State, AOSSi_wepNegotiationKey, 10, dataSize);

            unencBuf = (u8*)&keyTransferReq;
            for (i = 0; i < dataSize; i++) {
                buf = rc4State.perm;
                newOffsA = ((1 + rc4State.offsA) % rc4State.permLen);
                valA = buf[newOffsA];

                newOffsB = ((valA + rc4State.offsB) % rc4State.permLen);
                valB = buf[newOffsB];

                rc4State.offsA = newOffsA;
                rc4State.offsB = newOffsB;

                buf[newOffsB] = valA;
                buf[newOffsA] = valB;

                packetData->enc.data[i] = buf[(valA + valB) % rc4State.permLen] ^ unencBuf[i];
            }

            AOSSi_Free(rc4State.perm);
        }
        packetData->enc.dataSize = SOHtoNs(sizeof(AOSSi_KeyTransferReq));
        dataSize = offsetof(AOSSi_PacketData, enc.data) + sizeof(AOSSi_KeyTransferReq);
    } else {
        memcpy(packetData->unenc, &keyTransferReq, sizeof(AOSSi_KeyTransferReq));
    }
    memcpy(encMsgIdent, &msgIdents[1], 8);
    if (AOSSi_NameXor(encMsgIdent, 8, AOSSi_MELCO, 6) != 0) {
        AOSSi_error = 2;
        return -1;
    }

    AOSSi_Send(s, packet, &sendDest, NEGOTIATION_MSG_KEY_TRANSFER_SEND, dataSize, encMode, crc32, encMsgIdent, FALSE);
    return 0;
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

s16 AOSSi_BuildNegotiationMsg0(void* packetData) {
    s16 currSize;
    u32 secondData;

    AOSSi_ExtendedChunkedPacketData* currChunk = (AOSSi_ExtendedChunkedPacketData*)packetData;

    currChunk->cmd = AOSSi_internalState.prodInfoDataType;
    currChunk->hasNext = TRUE;
    currSize = AOSSi_internalState.prodInfoDataLen;
    memcpy(currChunk->data, AOSSi_internalState.deviceName, currSize);
    currChunk->size = SOHtoNs(currSize);

    currSize += offsetof(AOSSi_ExtendedChunkedPacketData, data);
    currSize = (currSize + 1) / 2 * 2;
    currChunk->nextOffset = SOHtoNs(currSize & 0xfffe);

    currChunk = (AOSSi_ExtendedChunkedPacketData*)((u8*)currChunk + currSize);
    currChunk->cmd = 0x60;
    currChunk->hasNext = FALSE;
    currChunk->nextOffset = SOHtoNs(0);

    secondData = SOHtoNl(14);
    memcpy(currChunk->data, &secondData, 4);
    currChunk->size = SOHtoNs(4);
    currSize += 10;

    return currSize;
}
s16 AOSSi_BuildKeyTransferMsg(void* packetData) {
    AOSSi_KeyTransferReq* keyTransferReq = (AOSSi_KeyTransferReq*)packetData;

    keyTransferReq->cmd = NEGOTIATION_CMD_KEYTRANS_REQ;
    keyTransferReq->unk_0x1 = 0;
    keyTransferReq->dataSize = SOHtoNs(sizeof(keyTransferReq->data));
    keyTransferReq->data.allowedModes = AOSSi_internalState.allowedModes;
    keyTransferReq->data.allowedModes = SOHtoNl(AOSSi_internalState.allowedModes);
    return sizeof(AOSSi_KeyTransferReq);
}

// See https://en.wikipedia.org/wiki/RC4#Key-scheduling_algorithm_(KSA)
// This is slightly different than the one described, it uses a custom
// permutation size and index modulus, rather than always being 256
void AOSSi_KeySchedRC4(AOSSi_RC4InternalState* rc4State, u8* key, u32 keyLen, int dataSize) {
    u32 keyI;
    u32 j;
    u32 i;
    u8 temp, offsetBufVal;
    u8* permBuf;

    rc4State->offsB = 0;
    rc4State->offsA = 0;
    rc4State->permLen = dataSize;

    permBuf = rc4State->perm;
    for (i = 0; i < dataSize; i++) {
        permBuf[i] = i;
    }

    j = 0;
    keyI = 0;
    for (i = 0; i < dataSize; i++) {
        offsetBufVal = permBuf[i];
        j = (offsetBufVal + j + key[keyI++]) % rc4State->permLen;

        // Swap i and j
        temp = permBuf[j];
        permBuf[j] = offsetBufVal;
        permBuf[i] = temp;

        if (keyI >= keyLen)
            keyI = 0;
    }
}

void AOSSi_InitCRC32Table(undefined4 _, u32* cur32Table) {
    BOOL shouldXor;
    u32 tableVal;

    int tableI;

#define AOSS_CRC_TABLEGEN_STEP(VALUE, SHOULD_XOR_BOOL)                                                                                               \
    SHOULD_XOR_BOOL = VALUE & 1;                                                                                                                     \
    VALUE >>= 1;                                                                                                                                     \
    if (SHOULD_XOR_BOOL)                                                                                                                             \
        VALUE ^= 0xedb88320;

    for (tableI = 0; tableI < 0x100; tableI++) {
        tableVal = tableI;

        AOSS_CRC_TABLEGEN_STEP(tableVal, shouldXor);
        AOSS_CRC_TABLEGEN_STEP(tableVal, shouldXor);
        AOSS_CRC_TABLEGEN_STEP(tableVal, shouldXor);
        AOSS_CRC_TABLEGEN_STEP(tableVal, shouldXor);
        AOSS_CRC_TABLEGEN_STEP(tableVal, shouldXor);
        AOSS_CRC_TABLEGEN_STEP(tableVal, shouldXor);
        AOSS_CRC_TABLEGEN_STEP(tableVal, shouldXor);
        AOSS_CRC_TABLEGEN_STEP(tableVal, shouldXor);

        *(cur32Table++) = tableVal;
    }
}
// inline u32 AOSSi_CRC32Step(u32 curr, u8 data) {
//     return (curr >> 8) ^ AOSSi_CRC32Table[(u8)(curr ^ data)];
// }
// inline u32 AOSSi_CRC32(s32 dataSize, u8* data) {
//     s32 i;
//     u32 curr = 0xFFFFFFFF;
//     AOSSi_InitCRC32Table(0, AOSSi_CRC32Table);
//     for (i = 0; i < dataSize; i++) {
//         curr = AOSSi_CRC32Step(curr, data[i]);
//     }
//     return curr ^ 0xFFFFFFFF;
// }
// inline u32 AOSSi_CRC32_UnsignedSize(u32 dataSize, void* data) {
//     s32 i;
//     u32 curr = 0xFFFFFFFF;
//     AOSSi_InitCRC32Table(0, AOSSi_CRC32Table);
//     for (i = 0; i < dataSize; i++) {
//         curr = AOSSi_CRC32Step(curr, ((u8*)data)[i]);
//     }
//     return curr ^ 0xFFFFFFFF;
// }

void AOSSi_XorBufs(u8* to, u8* from, int size) {
    int j;
    for (j = 0; j < size; j++) {
        to[j] ^= from[j];
    }
}

int AOSSi_NameXor(u8* buf, int bufSize, char* name, int nameBytes) {
    int halfBufSize;
    u8* bufL;
    u8* bufR;
    int i;
    u8* xorBuf;
    u8* tempBuf;
    int nameOffset;
    int j;

    halfBufSize = bufSize / 2;
    xorBuf = AOSSi_Alloc(halfBufSize);
    if (xorBuf == NULL)
        return -1;

    tempBuf = AOSSi_Alloc(bufSize);
    if (tempBuf == NULL) {
        AOSSi_Free(xorBuf);
        return -1;
    }

    bufL = buf;
    bufR = bufL + halfBufSize;
    for (i = 0; i < 2; i++) {
        nameOffset = i % nameBytes;
        for (j = 0; j < halfBufSize; j++) {
            xorBuf[j] = j;
            xorBuf[j] ^= name[nameOffset];

            if (++nameOffset >= nameBytes)
                nameOffset = 0;
        }

        AOSSi_XorBufs(bufR, xorBuf, halfBufSize);
        memcpy(tempBuf, bufR, halfBufSize);
        memcpy(tempBuf + halfBufSize, bufL, halfBufSize);
        memcpy(bufL, tempBuf, bufSize);
    }
    AOSSi_Free(xorBuf);
    AOSSi_Free(tempBuf);
    return 0;
}

// WLAN Connect
int AOSSi_ConnectToSSID(AOSSInfoSSID* ssid, AOSSiWLANConnectResult* res) {
    int i;

    i = 0;
    if (AOSSi_WLANConnect(ssid, res) != 0) {
        return -1;
    }
    if (SOStartup() == 0) {
        AOSSi_soStarted = 1;
        while (SOGetHostID() == 0) {
            if (AOSSi_cancel_flag == 1) {
                return -1;
            }
            i++;
            if (i > 30) {
                if (AOSSi_soStarted == 1) {
                    AOSSi_soStarted = 0;
                    SOCleanup();
                }
                AOSSi_socketStartRes->connected = FALSE;
                break;
            }

            OSSleepTicks(OSMillisecondsToTicks(100));
        }
    } else {
        return -1;
    }
    return 0;
}
