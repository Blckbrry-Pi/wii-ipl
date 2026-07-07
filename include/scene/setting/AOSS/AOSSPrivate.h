#ifndef AOSS_PRIVATE_H
#define AOSS_PRIVATE_H

#include <revolution/soex.h>

#include "scene/setting/AOSS/AOSS.h"

#include "string.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    NEGOTIATION_CMD_PHASE0_INFO_RECV = 0x01,
    NEGOTIATION_CMD_KEYTRANS_REQ = 0x02,
    NEGOTIATION_CMD_WEP40_INFO = 0x03,
    NEGOTIATION_CMD_WEP104_INFO = 0x04,
    NEGOTIATION_CMD_TKIP_INFO = 0x05,
    NEGOTIATION_CMD_AES_INFO = 0x06,
    NEGOTIATION_CMD_FINISH = 0x07,
    NEGOTIATION_CMD_KEYTRANS_IF1 = 0x08,
    NEGOTIATION_CMD_KEYTRANS_IF0 = 0x09,
    NEGOTIATION_CMD_UNKPROTOCOL_INFO = 0x0a,
};

typedef enum {
    NEGOTIATION_STATE_ERR = -1,
    NEGOTIATION_STATE_UNK0 = 0,
    NEGOTIATION_STATE_KEY_TRANSFER = 1,
    NEGOTIATION_STATE_REBOOTING_STACK = 2,
    NEGOTIATION_STATE_DONE = 100,
} AOSSi_NegotiationSetupState;

enum {
    NEGOTIATION_MSG_LOAD_ENCKEY = 0x1000,
    NEGOTIATION_MSG_UNK0_RECV = 0x1010,
    NEGOTIATION_MSG_KEY_TRANSFER_SEND = 0x2000,
    NEGOTIATION_MSG_KEY_TRANSFER_RECV = 0x2010,
    NEGOTIATION_MSG_FINISH_SEND = 0x3000,
    NEGOTIATION_MSG_FINISH_RECV = 0x3010,
};

enum {
    AOSS_MODE_NONE = 0b00000,
    AOSS_MODE_WEP40 = 0b00001,
    AOSS_MODE_WEP104 = 0b00010,
    AOSS_MODE_TKIP = 0b00100,
    AOSS_MODE_AES = 0b01000,
    AOSS_MODE_ALL = (AOSS_MODE_WEP40 | AOSS_MODE_WEP104 | AOSS_MODE_TKIP | AOSS_MODE_AES),
    AOSS_MODE_ENC_SUPPORTED = 0b10000,
};

typedef struct {
    u8 mac[6];
    u16 idx;
} AOSSi_MessageIdent;

typedef struct {
    u8 cmd;        // 0x00
    u16 dataSize;  // 0x02
    u8 data[];     // 0x04
} AOSSi_ChunkedPacketData;
typedef struct {
    u8 cmd;          // 0x00
    u8 hasNext;      // 0x01
    u16 size;        // 0x02
    u16 nextOffset;  // 0x04
    u8 data[];       // 0x06
} AOSSi_ExtendedChunkedPacketData;

typedef struct {
    u8 cmd;          // 0x00
    u16 dataSize;    // 0x02
    u16 nextOffset;  // 0x04
    u8 data[];       // 0x06
} AOSSi_KeyTransferPacketSubChunk;
typedef struct {
    u8 cmd;                                       // 0x00
    u16 size;                                     // 0x02
    u16 nextOffset;                               // 0x04
    AOSSi_KeyTransferPacketSubChunk subchunks[];  // 0x06
} AOSSi_KeyTransferPacketChunk;

typedef struct {
    void* deviceName;            // 0x00
    u32 prodInfoDataLen;         // 0x04
    undefined4 allowedModes;     // 0x08
    undefined4 negotiatedModes;  // 0x0c
    u32 gateway;                 // 0x10
    u32 netmask;                 // 0x14
    s8 unk_0x18;                 // 0x18
    u8 prodInfoDataType;         // 0x19
    u8 unk_0x1a;                 // 0x1a
    u8 unk_0x1b;                 // 0x1b
    u8 unk_0x1c;                 // 0x1c
    u8 unk_0x1d;                 // 0x1d
    u8 unk_0x1e;                 // 0x1e
    u8 unk_0x1f;                 // 0x1f
} AOSSiInternalState;
typedef struct {
    u32 unk_0x00;      // 0x00
    u32 keyChunkSize;  // 0x04
    char ssid[0x20];   // 0x08
    u32 unk_0x28;      // 0x28
    u32 unk_0x2c;      // 0x2c
    u8 key[4][0x40];   // 0x30
} AOSSiSparseWirelessWEP;
typedef struct {
    u32 unk_0x00;     // 0x00
    u32 keySize;      // 0x04
    char ssid[0x20];  // 0x08
    u32 unk_0x28;     // 0x28
    u32 unk_0x2c;     // 0x2c
    char key[0x40];   // 0x30
} AOSSiSparseWirelessTKIPAES;

typedef struct {
    u32 unk_0x000;                    // 0x000
    u32 unk_0x004;                    // 0x000
    AOSSiSparseWirelessWEP wep40;     // 0x008
    AOSSiSparseWirelessWEP wep104;    // 0x138
    AOSSiSparseWirelessTKIPAES tkip;  // 0x268
    AOSSiSparseWirelessTKIPAES aes;   // 0x2d8
    u32 unk_0x348;                    // 0x348
    u32 unk_0x34c;                    // 0x34c
} AOSSiSparseWirelessSettings;
typedef struct {
    u8 data[5][0x80];
} AOSSi_Struct0x5b8;

// Using names from here https://en.wikipedia.org/wiki/RC4#Description:
// i -> offsA
// j -> offsB
// S -> perm
typedef struct {
    u32 offsA;    // 0x0
    u32 offsB;    // 0x4
    u8* perm;     // 0x8
    u32 permLen;  // 0xc
} AOSSi_RC4InternalState;

typedef union {
    struct {
        u16 dataSize;    // 0x0
        u8 randSeed[2];  // 0x2
        u8 data[];       // 0x4
    } enc;               // 0x0
    u8 unenc[];          // 0x0
} AOSSi_PacketData;

typedef struct {
    u8 cmd;        // 0x0
    u8 unk_0x1;    // 0x1
    u16 dataSize;  // 0x2
    struct {
        u32 allowedModes;
    } data;  // 0x4
} AOSSi_KeyTransferReq;

typedef struct {
    u16 unk_0x00;                 // 0x00
    u16 unk_0x02;                 // 0x02
    u16 unk_0x04;                 // 0x04
    u16 negotiationPhase;         // 0x06
    u16 unk_0x08;                 // 0x08
    u16 dataSize;                 // 0x0a
    u16 encMode;                  // 0x0c
    u8 crc32;                     // 0x0e
    u8 unk_0x0f;                  // 0x0f
    AOSSi_MessageIdent msgIdent;  // 0x10
    AOSSi_PacketData packetData;  // 0x18
} AOSSiSOPacket;

typedef union {
    struct {
        u32 sockFd;     // 0x00
        u32 soRecvRes;  // 0x04
        u16 unk_0x08;   // 0x08
        u16 unk_0x0a;   // 0x0a
        union {
            AOSSiSOPacket recvPacket;  // 0x0c
        };
    };
    AOSSiSOPacket sendPacket;
} AOSS_Struct8C;

extern AOSSiInternalState AOSSi_internalState;
extern AOSSiSparseWirelessSettings AOSSi_sparseWireless[2];
extern AOSSi_Struct0x5b8 AOSSi_extraConfig;
extern u8 AOSSi_wepNegotiationKey[104];  // they made it 104 bytes instead of 104 bits lol
extern u32 AOSSi_CRC32Table[0x100];

extern AOSS_Struct8C* AOSSi_packetBuf;                // lbl_81698C8C
extern int AOSSi_soStarted;                           // lbl_81698C88
extern u32 AOSSi_error;                               // lbl_81698C84
extern int AOSSi_allowEnc;                            // lbl_81698C80
extern AOSSi_MessageIdent AOSSi_sentMessageIdent;     // lbl_81698C78
extern AOSSiWLANConnectResult* AOSSi_socketStartRes;  // lbl_81698C74
extern AOSSiBSSList* AOSSi_bssList;                   // lbl_81698C74

extern int AOSSi_sockfd;
extern char AOSSi_MELCO[6];
extern int AOSSi_status;

inline s16 AOSSi_SetupPacketHeader(int s, AOSSiSOPacket* packet, u16 negotiationPhase, s16 dataSize, u16 encMode, u8 crc32, u8* encMsgIdent) {
    s16 totalSize;

    // Set up packet header
    packet->unk_0x00 = SOHtoNs(1);
    packet->unk_0x02 = 0;
    packet->unk_0x04 = 0;
    packet->negotiationPhase = SOHtoNs(negotiationPhase);
    packet->unk_0x08 = 0;
    packet->dataSize = SOHtoNs(dataSize);
    packet->encMode = SOHtoNs(encMode);
    packet->crc32 = crc32;
    packet->unk_0x0f = 0x11;
    memcpy(&packet->msgIdent, encMsgIdent, 8);
    return dataSize + offsetof(AOSSiSOPacket, packetData);
}
inline void AOSSi_SendTo(int s, AOSSiSOPacket* packet, SOSockAddrIn* sendDest, s16 totalSize, BOOL overrideAddr) {
    memset(sendDest, 0, 8);
    sendDest->family = 2;
    sendDest->port = SOHtoNs(0x5790);
    sendDest->addr.addr = SOHtoNl(AOSSi_internalState.gateway);
    if (overrideAddr || AOSSi_internalState.unk_0x18 == 0) {
        sendDest->addr.addr = -1;
    }
    sendDest->len = 8;

    SOSendTo(s, packet, totalSize, 0, (SOSockAddr*)sendDest);
}
inline void AOSSi_Send(int s, AOSSiSOPacket* packet, SOSockAddrIn* sendDest, u16 negotiationPhase, s16 dataSize, u16 encMode, u8 crc32,
                       u8* encMsgIdent, BOOL overrideAddr) {
    // SOSockAddrIn sendDest;
    s16 totalSize;

    totalSize = AOSSi_SetupPacketHeader(s, packet, negotiationPhase, dataSize, encMode, crc32, encMsgIdent);
    AOSSi_SendTo(s, packet, sendDest, totalSize, overrideAddr);
}

#ifdef __cplusplus
}
#endif

#endif  // AOSS_PRIVATE_H
