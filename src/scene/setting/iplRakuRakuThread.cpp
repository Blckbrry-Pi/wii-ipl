#include "scene/setting/iplRakuRakuThread.h"

// #include "scene/setting/ATERM.h"

#include "string.h"

#include <revolution/soex.h>

// #include <egg

namespace ipl {
    namespace scene {
#define RAKU_RAKU_THREAD__ATERMI_HEAP_SIZE 0x40000
#define RAKU_RAKU_THREAD__ATERMI_STACK_SIZE 0x1000
        s32 RakuRakuThread::startTimeHi = 0;
        u32 RakuRakuThread::startTimeLo = 0;
        bool RakuRakuThread::hasBeenStartedMaybe = false;
        OSMessage RakuRakuThread::msgQueueBuf[1] = {(OSMessage)0};

        MEMAllocator rakuRakuAllocator;
        ATERM_TimestampedState RakuRakuThread::timeState;
        ATERMiApConfigResult RakuRakuThread::apConfigResult;
        OSMessageQueue RakuRakuThread::msgQueue;

        int RakuRakuThread::state = 1;

        // TimeCallback
        void RakuRakuThread::fn_ATERMStateCb(ATERM_TimestampedState* val) {
            BOOL level = OSDisableInterrupts();
            timeState.state = val->state;
            timeState.timeDiff = val->timeDiff;
            timeState.unk_0x08 = val->unk_0x08;

            s64 time = OSGetTime();
            startTimeLo = time;
            startTimeHi = time >> 32;

            OSRestoreInterrupts(level);
        }
        RakuRakuThread::RakuRakuThread(EGG::Heap* heap) {
            u32 uVar1 = ATERMi_ApConfigGetVersion();
            if ((uVar1 >> 8 & 0xf) + (uVar1 >> 0xc & 0xf) * 10 != 1) {
                pHeapMem = NULL;
            } else {
                pHeapMem = heap->alloc(RAKU_RAKU_THREAD__ATERMI_HEAP_SIZE, 0x20);
                pThreadStack = heap->alloc(0x1000, 0x20);
            }
            pHeap = NULL;
            hasBeenStartedMaybe = false;
            unk_0x330 = 0;
            unk_0x334 = 0;
            OSInitMessageQueue(&msgQueue, msgQueueBuf, ARRAY_LENGTH(msgQueueBuf));
        }
        RakuRakuThread::~RakuRakuThread() {
            if (unk_0x334) {
                OSJamMessage(&msgQueue, (OSMessage)2, 1);
                WaitForThreadExit();
            }
            destroy();
        }
        void RakuRakuThread::destroy() {
            ATERM_TimestampedState apConfigState;

            if (hasBeenStartedMaybe) {
                OSSendMessage(&msgQueue, (OSMessage)1, 0);
                OSTick start = OSGetTick();
                while (OSTicksToMilliseconds(OSGetTick() - start) < 2000) {
                    if (ATERMi_ApConfigGetState(&apConfigState) == 1 && apConfigState.state == 7)
                        break;
                }
                SOFinish();
                hasBeenStartedMaybe = false;
            }
            if (pHeap != NULL) {
                MEMDestroyExpHeap(pHeap);
                memset(&rakuRakuAllocator, 0, 0x10);
                pHeap = NULL;
            }
        }
        bool RakuRakuThread::start() {
            if (hasBeenStartedMaybe || pHeapMem == NULL)
                return 0;

            BOOL level = OSDisableInterrupts();

            u64 time = OSGetTime();
            startTimeLo = time;
            startTimeHi = time >> 32;

            pHeap = MEMCreateExpHeapEx(pHeapMem, RAKU_RAKU_THREAD__ATERMI_HEAP_SIZE, 2);
            MEMInitAllocatorForExpHeap(&rakuRakuAllocator, pHeap, 0x20);

            mPriority = OSGetThreadPriority(OSGetCurrentThread()) - 1;

            hasBeenStartedMaybe = true;
            unk_0x330 = 0;

            memset(&apConfigResult, 0, sizeof(apConfigResult));
            memset(&timeState, 0, sizeof(timeState));

            SOLibraryConfig soLibConf;
            soLibConf.alloc = &fn_SOAlloc;
            soLibConf.free = &fn_SOFree;
            SOInit(&soLibConf);
            ATERMi_ApConfigStart(mPriority, 200, &fn_ATERMStateCb, &fn_ATERMAlloc, &fnATERMFree, 0x1000);

            if (unk_0x334 == 0) {
                unk_0x334 = 1;
                memset(pThreadStack, 0, 4);
                Create(pThreadStack, RAKU_RAKU_THREAD__ATERMI_STACK_SIZE, mPriority - 1);
                OSReport("RakuRakuThread (for End function) start with prio(%d) \n", mPriority);
            }
            OSRestoreInterrupts(level);
            return 1;
        }

        void* RakuRakuThread::Run() {
            int msg;
            do {
                while (OSReceiveMessage(&msgQueue, (OSMessage*)&msg, 0)) {
                    if (msg == 0x2) {
                        return this;
                    }
                }
                OSReceiveMessage(&msgQueue, (OSMessage*)&msg, 1);
                if (msg == 0x2)
                    break;
                ((u32*)pThreadStack)[0xff0] = 0x97654321;
                ATERMi_ApConfigEnd();
                unk_0x330 = 1;
            } while (unk_0x334 != 0);
            return this;
        }

        // SOAlloc
        void* RakuRakuThread::fn_SOAlloc(u32, s32 size) {
            return MEMAllocFromAllocator(&rakuRakuAllocator, size);
        }
        // SOFree
        void RakuRakuThread::fn_SOFree(u32, void* buf, s32 size) {
            MEMFreeToAllocator(&rakuRakuAllocator, buf);
        }

        // Alloc
        void* RakuRakuThread::fn_ATERMAlloc(u32 size) {
            return MEMAllocFromAllocator(&rakuRakuAllocator, size);
        }
        // Free
        void RakuRakuThread::fnATERMFree(void* buf) {
            MEMFreeToAllocator(&rakuRakuAllocator, buf);
        }

        int RakuRakuThread::getState() {
            if (hasBeenStartedMaybe == '\0') {
                return 0;
            }
            BOOL level = OSDisableInterrupts();
            switch (timeState.state) {
                case 1:
                    state = 1;
                    break;
                case 2:
                    state = 2;
                    break;
                case 3:
                    state = 2;
                    break;
                case 4:
                    state = 4;
                    break;
                case 5:
                    state = 5;
                    break;
                case 6:
                    state = 6;
                    break;
                case 7:
                    state = 7;
                    break;
                default:
                    state = 1;
                    break;
            }
            if (((state != 6) && (state != 7)) && (OSTicksToMilliseconds((u32)OSGetTime() - startTimeLo) >= 90 * 1000)) {
                OSSendMessage(&msgQueue, (OSMessage)1, 0);
            }
            OSRestoreInterrupts(level);
            return state;
        }

        void RakuRakuThread::cancel() {
            OSSendMessage(&msgQueue, (OSMessage)1, 0);
        }

        bool RakuRakuThread::finish(NCDApConfig* rakuCfg, int* resultMaybe) {
            if (!hasBeenStartedMaybe) {
                if (resultMaybe != NULL)
                    *resultMaybe = -99;
                return true;
            } else if ((u32)(timeState.state - 6) <= 1) {
                if (unk_0x330 == 0) {
                    if (timeState.state == 6) {
                        ATERMi_ApConfigGetResult(&apConfigResult);
                        printInfo();
                    }
                    OSSendMessage(&msgQueue, (OSMessage)1, 0);
                    return false;
                } else {
                    SOFinish();
                    hasBeenStartedMaybe = false;
                    destroy();
                }
            } else {
                return false;
            }
            if (rakuCfg != NULL) {
                memcpy(rakuCfg->ssid, apConfigResult.ssid, sizeof(apConfigResult.ssid));
                rakuCfg->ssidLength = strlen(apConfigResult.ssid);
                if (apConfigResult.mode == 1) {
                    // wep104
                    rakuCfg->privacy.mode = 1;
                    rakuCfg->privacy.wep40.keyId = apConfigResult.wepKeyID;
                    for (int i = 0; i < (s32)ARRAY_LENGTH((rakuCfg->privacy).wep40.key); i++) {
                        memcpy((rakuCfg->privacy).wep40.key[i], apConfigResult.wepKeys[i], sizeof((rakuCfg->privacy).wep40.key[i]));
                    }
                } else if (apConfigResult.mode == 2) {
                    // wep104
                    rakuCfg->privacy.mode = 2;
                    rakuCfg->privacy.wep104.keyId = apConfigResult.wepKeyID;
                    for (int i = 0; i < (s32)ARRAY_LENGTH((rakuCfg->privacy).wep104.key); i++) {
                        memcpy((rakuCfg->privacy).wep104.key[i], apConfigResult.wepKeys[i], sizeof((rakuCfg->privacy).wep104.key[i]));
                    }
                } else if (apConfigResult.mode == 4) {
                    // tkip
                    (rakuCfg->privacy).mode = 4;
                    memcpy(rakuCfg->privacy.tkip.key, apConfigResult.textKeys, sizeof(rakuCfg->privacy.tkip.key));
                    rakuCfg->privacy.tkip.keyLen = strlen(apConfigResult.textKeys);
                } else if (apConfigResult.mode == 5) {
                    // aes
                    (rakuCfg->privacy).mode = 6;
                    memcpy(rakuCfg->privacy.aes.key, apConfigResult.textKeys, sizeof(rakuCfg->privacy.aes.key));
                    rakuCfg->privacy.aes.keyLen = strlen(apConfigResult.textKeys);
                }
            }
            if (resultMaybe != NULL) {
                *resultMaybe = timeState.unk_0x08;
            }
            return 1;
        }

#pragma dont_inline on
        void RakuRakuThread::printInfo() {
            // TODO: Check if there's a version where this function isn't stubbed
            return;
        }
#pragma reset
    }  // namespace scene
}  // namespace ipl
