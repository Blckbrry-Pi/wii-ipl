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
        s32 AOSSThread::smStartTimeHi;
        u32 AOSSThread::smStartTimeLo;
        BOOL AOSSThread::smIsStarted;

        AOSSThread::AOSSThread(EGG::Heap* heap) {
            mpHeap = 0;
            smIsStarted = 0;
            mpHeapMem = heap->alloc(0x40000, 0x20);
            mpThreadStack = heap->alloc(0x1000, 0x20);
        }
        void AOSSThread::destroy(int) {
            if ((smIsStarted != 0) && IsThreadTerminated()) {
                WaitForThreadExit();
                SOFinish();
                smIsStarted = 0;
            }
            if (mpHeap != NULL) {
                MEMDestroyExpHeap(mpHeap);
                mpHeap = NULL;
            }
        }
        AOSSThread::~AOSSThread() {
            destroy(1);
        }

        bool AOSSThread::start() {
            if (smIsStarted) {
                return false;
            } else {
                SOLibraryConfig soLibConf;
                soLibConf.alloc = SOAlloc;
                soLibConf.free = SOFree;

                BOOL level = OSDisableInterrupts();
                s64 time = OSGetTime();
                smStartTimeHi = time >> 32;
                smStartTimeLo = time;
                mpHeap = MEMCreateExpHeapEx(mpHeapMem, 0x40000, 2);
                MEMInitAllocatorForExpHeap(&m_allocator, mpHeap, 0x20);
                mPriority = OSGetThreadPriority(OSGetCurrentThread()) - 2;
                smIsStarted = 1;
                SOInit(&soLibConf);
                memset(mpThreadStack, 0, 4);
                Create(mpThreadStack, 0x1000, mPriority);
                OSRestoreInterrupts(level);
                return true;
            }
        }

        void* AOSSThread::SOAlloc(u32, s32 size) {
            void* ptr;
            BOOL level;
            level = OSDisableInterrupts();
            ptr = MEMAllocFromAllocator(&m_allocator, size);
            OSRestoreInterrupts(level);
            return ptr;
        }
        void AOSSThread::SOFree(u32, void* ptr, s32) {
            BOOL level = OSDisableInterrupts();
            MEMFreeToAllocator(&m_allocator, ptr);
            OSRestoreInterrupts(level);
        }

        void* AOSSThread::Run() {
            mAOSSErrno = 0xf;
            AOSS_SetCallback((AOSSiCallback)USBAPThread::callback);
            if (AOSSi_InitLocal(SOAlloc, SOFree) == -1) {
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
                ((u32*)mpThreadStack)[0xff0] = 0x97654321;
                mAOSSErrno = AOSSi_Init(&mAossCfg);
                AOSSi_EndLocal();
                OSCheckActiveThreads();
                return this;
            }
        }
        bool AOSSThread::cancel() {
            if (smIsStarted == 0) {
                return false;
            } else {
                BOOL level = OSDisableInterrupts();
                AOSSi_Cancel();
                OSRestoreInterrupts(level);
                return true;
            }
        }
        bool AOSSThread::finish(NCDAossConfig* cfgOut, int* result) {
            if (smIsStarted == 0) {
                *result = -99;
                return true;
            } else {
                if (IsThreadTerminated()) {
                    destroy(0);
                } else {
                    if (OSTicksToMilliseconds((u32)OSGetTime() - smStartTimeLo) >= 90 * 1000) {
                        BOOL level = OSDisableInterrupts();
                        AOSSi_Cancel();
                        OSRestoreInterrupts(level);
                    }
                    return 0;
                }
                {
                    if (mAOSSErrno == 0) {
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
                    } else if (mAOSSErrno == -2) {
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
