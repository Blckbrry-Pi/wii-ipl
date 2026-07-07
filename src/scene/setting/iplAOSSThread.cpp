#include "scene/setting/iplAOSSThread.h"

#include "scene/setting/iplUSBAPThread.h"

#include "string.h"

#include <revolution/mem/allocator.h>
#include <revolution/mem/expHeap.h>
#include <revolution/net.h>
#include <revolution/soex.h>

#include <egg/core/eggExpHeap.h>

namespace ipl {
    namespace scene {
        static MEMAllocator m_allocator;
        s32 AOSSThread::startTimeHi;
        u32 AOSSThread::startTimeLo;
        u32 AOSSThread::sbss_0x8;

        AOSSThread::AOSSThread(EGG::Heap* heap) {
            mHeapHandle = 0;
            sbss_0x8 = 0;
            pHeapMem = heap->alloc(0x40000, 0x20);
            pThreadStack = heap->alloc(0x1000, 0x20);
        }
        void AOSSThread::destroy(int) {
            if ((sbss_0x8 != 0) && IsThreadTerminated()) {
                WaitForThreadExit();
                SOFinish();
                sbss_0x8 = 0;
            }
            if (mHeapHandle != NULL) {
                MEMDestroyExpHeap(mHeapHandle);
                mHeapHandle = NULL;
            }
        }
        AOSSThread::~AOSSThread() {
            destroy(1);
        }

        bool AOSSThread::start() {
            if (sbss_0x8) {
                return false;
            } else {
                SOLibraryConfig soLibConf;
                soLibConf.alloc = fn_SOAlloc;
                soLibConf.free = fn_SOFree;

                BOOL level = OSDisableInterrupts();
                s64 time = OSGetTime();
                startTimeHi = time >> 32;
                startTimeLo = time;
                mHeapHandle = MEMCreateExpHeapEx(pHeapMem, 0x40000, 2);
                MEMInitAllocatorForExpHeap(&m_allocator, mHeapHandle, 0x20);
                mPriority = OSGetThreadPriority(OSGetCurrentThread()) - 2;
                sbss_0x8 = 1;
                SOInit(&soLibConf);
                memset(pThreadStack, 0, 4);
                Create(pThreadStack, 0x1000, mPriority);
                OSRestoreInterrupts(level);
                return true;
            }
        }

        void* AOSSThread::fn_SOAlloc(u32, s32 size) {
            void* ptr;
            BOOL level;
            level = OSDisableInterrupts();
            ptr = MEMAllocFromAllocator(&m_allocator, size);
            OSRestoreInterrupts(level);
            return ptr;
        }
        void AOSSThread::fn_SOFree(u32, void* ptr, s32) {
            BOOL level = OSDisableInterrupts();
            MEMFreeToAllocator(&m_allocator, ptr);
            OSRestoreInterrupts(level);
        }

        void* AOSSThread::Run() {
            unk_0x32c = 0xf;
            AOSS_SetCallback((AOSSiCallback)USBAPThread::callback);
            if (AOSSi_InitLocal(fn_SOAlloc, fn_SOFree) == -1) {
                return this;
            } else {
                memset(&mAossCfg, 0, 0x26c);
                mAossCfg.mode = 0x0f;
                mAossCfg.retryCount_Scan = 50;
                mAossCfg.retryCount_Comu = 50;
                mAossCfg.retryInterval_Scan = 100;
                mAossCfg.retryInterval_Comu = 100;
                mAossCfg.selectInterval = 20000;
                mAossCfg.productInfo.dataType = 0x50;

                memcpy(mAossCfg.productInfo.deviceName, "Wii", 3);
                mAossCfg.productInfo.dataLen = 4;

                NETGetWirelessMacAddress(&mAossCfg.macAddr);
                ((u32*)pThreadStack)[0xff0] = 0x97654321;
                unk_0x32c = AOSSi_Init(&mAossCfg);
                AOSSi_EndLocal();
                OSCheckActiveThreads();
                return this;
            }
        }
        bool AOSSThread::cancel() {
            if (sbss_0x8 == 0) {
                return false;
            } else {
                BOOL level = OSDisableInterrupts();
                AOSSi_Cancel();
                OSRestoreInterrupts(level);
                return true;
            }
        }
        bool AOSSThread::finish(NCDAossConfig* cfgOut, int* result) {
            if (sbss_0x8 == 0) {
                *result = -99;
                return true;
            } else {
                if (IsThreadTerminated()) {
                    destroy(0);
                } else {
                    if (OSTicksToMilliseconds((u32)OSGetTime() - startTimeLo) >= 90 * 1000) {
                        BOOL level = OSDisableInterrupts();
                        AOSSi_Cancel();
                        OSRestoreInterrupts(level);
                    }
                    return 0;
                }
                {
                    if (unk_0x32c == 0) {
                        BOOL level = OSDisableInterrupts();
                        AOSSiWirelessSettings* wireless = &mAossCfg.wirelessSettings;
                        if ((mAossCfg.mode & 1) == 1) {
                            memcpy(cfgOut->wep40.ssid, wireless->wep40SSID, sizeof(cfgOut->wep40.ssid));
                            memcpy(cfgOut->wep40.key[0], wireless->wep40Key[0], sizeof(cfgOut->wep40.key[0]));
                            memcpy(cfgOut->wep40.key[1], wireless->wep40Key[1], sizeof(cfgOut->wep40.key[1]));
                            memcpy(cfgOut->wep40.key[2], wireless->wep40Key[2], sizeof(cfgOut->wep40.key[2]));
                            memcpy(cfgOut->wep40.key[3], wireless->wep40Key[3], sizeof(cfgOut->wep40.key[3]));
                            cfgOut->wep40.keyId = 1;
                            cfgOut->wep40.ssidLength = strlen(wireless->wep40SSID);
                        } else {
                            memset(cfgOut->wep40.ssid, 0, sizeof(cfgOut->wep40.ssid));
                            cfgOut->wep40.ssidLength = 0;
                        }
                        if ((mAossCfg.mode & 2) == 2) {
                            memcpy(cfgOut->wep104.ssid, wireless->wep104SSID, sizeof(cfgOut->wep104.ssid));
                            memcpy(cfgOut->wep104.key[0], wireless->wep104Key[0], sizeof(cfgOut->wep104.key[0]));
                            memcpy(cfgOut->wep104.key[1], wireless->wep104Key[1], sizeof(cfgOut->wep104.key[1]));
                            memcpy(cfgOut->wep104.key[2], wireless->wep104Key[2], sizeof(cfgOut->wep104.key[2]));
                            memcpy(cfgOut->wep104.key[3], wireless->wep104Key[3], sizeof(cfgOut->wep104.key[3]));
                            cfgOut->wep104.keyId = 1;
                            cfgOut->wep104.ssidLength = strlen(wireless->wep104SSID);
                        } else {
                            memset(cfgOut->wep104.ssid, 0, sizeof(cfgOut->wep104.ssid));
                            cfgOut->wep104.ssidLength = 0;
                        }
                        if ((mAossCfg.mode & 4) == 4) {
                            memcpy(cfgOut->tkip.ssid, wireless->tkipSSID, sizeof(cfgOut->tkip.ssid));
                            memcpy(cfgOut->tkip.key, wireless->tkipKey, sizeof(cfgOut->tkip.key));
                            cfgOut->tkip.ssidLength = strlen(wireless->tkipSSID);
                            cfgOut->tkip.keyLen = strlen((char*)wireless->tkipKey);
                        } else {
                            memset(cfgOut->tkip.ssid, 0, sizeof(cfgOut->tkip.ssid));
                            cfgOut->tkip.ssidLength = 0;
                        }
                        if ((mAossCfg.mode & 8) == 8) {
                            memcpy(cfgOut->aes.ssid, wireless->aesSSID, sizeof(cfgOut->aes.ssid));
                            memcpy(cfgOut->aes.key, wireless->aesKey, sizeof(cfgOut->aes.key));
                            cfgOut->aes.ssidLength = strlen(wireless->aesSSID);
                            cfgOut->aes.keyLen = strlen((char*)wireless->aesKey);
                        } else {
                            memset(cfgOut->aes.ssid, 0, sizeof(cfgOut->aes.ssid));
                            cfgOut->aes.ssidLength = 0;
                        }
                        printInfo();
                        OSRestoreInterrupts(level);
                    } else if (unk_0x32c == -2) {
                        *result = -98;
                    }
                    *result = mAossCfg.aossResult;
                    return 1;
                }
            }
        }

#pragma dont_inline on
        void AOSSThread::printInfo() {
            // TODO: Check if there's a version where this function isn't stubbed
            return;
        }
#pragma reset
    }  // namespace scene
}  // namespace ipl
