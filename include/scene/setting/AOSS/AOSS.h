#ifndef AOSS_H
#define AOSS_H

#include <revolution/soex.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    u8 wep40Key[4][6];      // 0x000
    char wep40SSID[0x21];   // 0x018
    u8 wep104Key[4][14];    // 0x039
    char wep104SSID[0x21];  // 0x071
    u8 tkipKey[0x40];       // 0x092
    char tkipSSID[0x21];    // 0x0d2
    u8 aesKey[0x40];        // 0x0f3
    char aesSSID[0x21];     // 0x133
} AOSSiWirelessSettings;

typedef struct {
    u8 dataType;           // 0x000
    u16 dataLen;           // 0x002
    u8 deviceName[0x100];  // 0x004
} AOSSiProductInfo;

typedef struct {
    u16 count;     // 0x0
    u16 interval;  // 0x2
} AOSSiRetryCfg;
typedef struct {
    int count;                   // 0x0
    AOSSiRetryCfg retryCfgComu;  // 0x4
    AOSSiRetryCfg retryCfgScan;  // 0x8
} ALIGN32 AOSSiTimeouts;

// Some names taken from a decompilation of the android AOSS app
typedef struct AOSSiConfig {
    u16 mode;                      // 0x000
    AOSSiProductInfo productInfo;  // 0x002
    s16 retryCount_Scan;           // 0x106
    s16 retryInterval_Scan;        // 0x108
    s16 retryCount_Comu;           // 0x10a
    s16 retryInterval_Comu;        // 0x10c
    s16 selectInterval;            // 0x10e
    u8 macAddr[0x6];               // 0x110
    u8 aossResult;                 // 0x116
    AOSSiWirelessSettings wirelessSettings;
} AOSSiConfig;

typedef struct {
    u32 SSIDLength;     // 0x00
    u8 SSID[0x20];      // 0x04
    u32 channel;        // 0x24
    u32 unk_0x28;       // 0x28
    u32 unk_0x2c;       // 0x2c
    u8 BSSID[6];        // 0x30
    u32 rateCount;      // 0x38
    u8 ratesBuf[0x10];  // 0x3c
    u32 beaconPeriod;   // 0x4c
    u32 capabilities;   // 0x50
} AOSSiBSSListEntry;

typedef struct {
    u32 count;                // 0x00
    AOSSiBSSListEntry bss[];  // 0x04
} AOSSiBSSList;

typedef struct {
    u32 ssidLen;      // 0x00
    char ssid[0x20];  // 0x04
    u32 unk_0x24;     // 0x24
    u32 keyLen;       // 0x28
    char key[0x10];   // 0x2c
} AOSSInfoSSID;

typedef struct {
    u32 connected;    // 0x00
    u32 ssidLen;      // 0x04
    char ssid[0x20];  // 0x08
    u32 channel;      // 0x28
} AOSSiWLANConnectResult;

typedef void (*AOSSiCallback)(int status);

int AOSSi_Init(AOSSiConfig*);
int AOSSi_Init_old(AOSSiConfig*);

int AOSS_CheckAP(AOSSiBSSList* bssList);

#ifdef __cplusplus
}
#endif

#endif  // AOSS_H
