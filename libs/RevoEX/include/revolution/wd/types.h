#ifndef REVOLUTION_WD_SCAN_H
#define REVOLUTION_WD_SCAN_H

// #error aaaaaaaa

#include <revolution/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WD_BSSID_LENGTH 6
#define WD_SSID_LENGTH 0x20

typedef struct WDBssDesc_ {
    u16 length;               // 0x00
    u16 RSSI;                 // 0x02
    u8 BSSID[6];              // 0x04
    u16 SSIDLength;           // 0x0a
    u8 SSID[WD_SSID_LENGTH];  // 0x0c
    u16 capabilities;         // 0x2c
    u16 rateSet_basic;        // 0x2e
    u16 rateSet_support;      // 0x30
    u16 beacon_period;        // 0x32
    u16 DTIM_period;          // 0x34
    u16 channel;              // 0x36
    u16 CF_period;            // 0x38
    u16 CF_max_duation;       // 0x3a
    u16 IEs_length;           // 0x3c
    u8 unk_0x3e[0x10];
} WDBssDesc;

typedef struct WDInfo {
    u8 MAC[6];                   // 0x00
    u16 EnableChannelsMask;      // 0x06
    u16 NTRallowedChannelsMask;  // 0x08
    u8 CountryCode[4];           // 0x0a
    u8 channel;                  // 0x0e
    u8 initialized;              // 0x0f
    u8 version[80];              // 0x10
    u8 unknown[48];              // 0x90
} ALIGN32 WDInfo;

typedef struct WDScanParameters {
    u16 ChannelBitmap;
    u16 MaxChannelTime;
    u8 BSSID[WD_BSSID_LENGTH];
    u16 ScanType;

    u16 SSIDLength;
    u8 SSID[WD_SSID_LENGTH];
    // NOTE: Conflicts with WiiBrew (Mar 2026), but used as 32 bytes.
    // https://wiibrew.org/wiki//dev/net/wd/command
    u8 SSIDMatchMask[WD_SSID_LENGTH];
} WDScanParameters;

#ifdef __cplusplus
}
#endif

#endif  // REVOLUTION_WD_SCAN_H
