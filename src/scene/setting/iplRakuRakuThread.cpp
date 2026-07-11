#include "scene/setting/iplRakuRakuThread.h"

// #include "scene/setting/ATERM.h"

#include "string.h"

#include <revolution/soex.h>

// #include <egg

namespace ipl {
    namespace scene {
#define RAKU_RAKU_THREAD__ATERMI_HEAP_SIZE 0x40000
#define RAKU_RAKU_THREAD__ATERMI_STACK_SIZE 0x1000

        s32 RakuRakuThread::smUpdateTimeHi = 0;
        u32 RakuRakuThread::smUpdateTimeLo = 0;
        bool RakuRakuThread::smbIsStarted = false;
        OSMessage RakuRakuThread::smMsgQueueBuf[1] = {(OSMessage)0};

        MEMAllocator RakuRakuThread::smRakuRakuAllocator;
        ATERMStateUpdate RakuRakuThread::smStateUpdate;
        ATERMiApConfigResult RakuRakuThread::smApConfigResult;
        OSMessageQueue RakuRakuThread::smMsgQueue;

        ATERMConfigState RakuRakuThread::smState = ATERM_STATE_IDLE;

        // TimeCallback
        void RakuRakuThread::ATERMStateCb(ATERMStateUpdate* val) {
            BOOL level = OSDisableInterrupts();
            smStateUpdate.state = val->state;
            smStateUpdate.timeDiff = val->timeDiff;
            smStateUpdate.isDone = val->isDone;

            s64 time = OSGetTime();
            smUpdateTimeLo = time;
            smUpdateTimeHi = time >> 32;

            OSRestoreInterrupts(level);
        }
        RakuRakuThread::RakuRakuThread(EGG::Heap* heap) {
            u32 uVar1 = ATERMi_ApConfigGetVersion();
            if ((uVar1 >> 8 & 0xf) + (uVar1 >> 0xc & 0xf) * 10 != 1) {
                mpHeapMem = NULL;
            } else {
                mpHeapMem = heap->alloc(RAKU_RAKU_THREAD__ATERMI_HEAP_SIZE, 0x20);
                mpThreadStack = heap->alloc(0x1000, 0x20);
            }
            mpHeap = NULL;
            smbIsStarted = false;
            mbThreadFinished = 0;
            mbThreadStarted = 0;
            OSInitMessageQueue(&smMsgQueue, smMsgQueueBuf, ARRAY_LENGTH(smMsgQueueBuf));
        }
        RakuRakuThread::~RakuRakuThread() {
            if (mbThreadStarted) {
                OSJamMessage(&smMsgQueue, (OSMessage)2, 1);
                WaitForThreadExit();
            }
            destroy();
        }
        void RakuRakuThread::destroy() {
            ATERMStateUpdate apConfigState;

            if (smbIsStarted) {
                OSSendMessage(&smMsgQueue, (OSMessage)1, 0);
                OSTick start = OSGetTick();
                while (OSTicksToMilliseconds(OSGetTick() - start) < 2000) {
                    if (ATERMi_ApConfigGetState(&apConfigState) == 1 && apConfigState.state == 7)
                        break;
                }
                SOFinish();
                smbIsStarted = false;
            }
            if (mpHeap != NULL) {
                MEMDestroyExpHeap(mpHeap);
                memset(&smRakuRakuAllocator, 0, 0x10);
                mpHeap = NULL;
            }
        }
        bool RakuRakuThread::start() {
            if (smbIsStarted || mpHeapMem == NULL)
                return 0;

            BOOL level = OSDisableInterrupts();

            u64 time = OSGetTime();
            smUpdateTimeLo = time;
            smUpdateTimeHi = time >> 32;

            mpHeap = MEMCreateExpHeapEx(mpHeapMem, RAKU_RAKU_THREAD__ATERMI_HEAP_SIZE, MEM_HEAP_OPT_DEBUG_FILL);
            MEMInitAllocatorForExpHeap(&smRakuRakuAllocator, mpHeap, 0x20);

            mPriority = OSGetThreadPriority(OSGetCurrentThread()) - 1;

            smbIsStarted = true;
            mbThreadFinished = 0;

            memset(&smApConfigResult, 0, sizeof(smApConfigResult));
            memset(&smStateUpdate, 0, sizeof(smStateUpdate));

            SOLibraryConfig soLibConf;
            soLibConf.alloc = &SOAlloc;
            soLibConf.free = &SOFree;
            SOInit(&soLibConf);
            ATERMi_ApConfigStart(mPriority, 200, &ATERMStateCb, &ATERMAlloc, &ATERMFree, 0x1000);

            if (mbThreadStarted == 0) {
                mbThreadStarted = 1;
                memset(mpThreadStack, 0, 4);
                Create(mpThreadStack, RAKU_RAKU_THREAD__ATERMI_STACK_SIZE, mPriority - 1);
                OSReport("RakuRakuThread (for End function) start with prio(%d) \n", mPriority);
            }
            OSRestoreInterrupts(level);
            return 1;
        }

        void* RakuRakuThread::Run() {
            int msg;
            do {
                while (OSReceiveMessage(&smMsgQueue, (OSMessage*)&msg, 0)) {
                    if (msg == 0x2) {
                        return this;
                    }
                }
                OSReceiveMessage(&smMsgQueue, (OSMessage*)&msg, 1);
                if (msg == 0x2)
                    break;
                ((u32*)mpThreadStack)[0xff0] = 0x97654321;
                ATERMi_ApConfigEnd();
                mbThreadFinished = 1;
            } while (mbThreadStarted != 0);
            return this;
        }

        // SOAlloc
        void* RakuRakuThread::SOAlloc(u32, s32 size) {
            return MEMAllocFromAllocator(&smRakuRakuAllocator, size);
        }
        // SOFree
        void RakuRakuThread::SOFree(u32, void* buf, s32 size) {
            MEMFreeToAllocator(&smRakuRakuAllocator, buf);
        }

        // Alloc
        void* RakuRakuThread::ATERMAlloc(u32 size) {
            return MEMAllocFromAllocator(&smRakuRakuAllocator, size);
        }
        // Free
        void RakuRakuThread::ATERMFree(void* buf) {
            MEMFreeToAllocator(&smRakuRakuAllocator, buf);
        }

        int RakuRakuThread::getState() {
            if (!smbIsStarted) {
                return 0;
            }
            BOOL level = OSDisableInterrupts();
            switch (smStateUpdate.state) {
                case ATERM_STATE_IDLE:
                    smState = ATERM_STATE_IDLE;
                    break;
                case ATERM_STATE_FOUND_ROUTER:
                    smState = ATERM_STATE_FOUND_ROUTER;
                    break;
                case ATERM_STATE_CONFIRMED_ROUTER:
                    smState = ATERM_STATE_FOUND_ROUTER;
                    break;
                case ATERM_STATE_ESTABLISHING_ENCRYPTION:
                    smState = ATERM_STATE_ESTABLISHING_ENCRYPTION;
                    break;
                case ATERM_STATE_TRANSFERRING_KEYS:
                    smState = ATERM_STATE_TRANSFERRING_KEYS;
                    break;
                case ATERM_STATE_DONE_OK:
                    smState = ATERM_STATE_DONE_OK;
                    break;
                case ATERM_STATE_DONE_ERR:
                    smState = ATERM_STATE_DONE_ERR;
                    break;
                default:
                    smState = ATERM_STATE_IDLE;
                    break;
            }
            if (smState != 6 && smState != 7 && (OSTicksToMilliseconds((u32)OSGetTime() - smUpdateTimeLo) >= 90 * 1000)) {
                OSSendMessage(&smMsgQueue, (OSMessage)1, 0);
            }
            OSRestoreInterrupts(level);
            return smState;
        }

        void RakuRakuThread::cancel() {
            OSSendMessage(&smMsgQueue, (OSMessage)1, 0);
        }

        bool RakuRakuThread::finish(NCDApConfig* rakuCfg, int* result) {
            if (!smbIsStarted) {
                if (result != NULL)
                    *result = -99;
                return true;
            } else if ((u32)(smStateUpdate.state - 6) <= 1) {
                if (mbThreadFinished == 0) {
                    if (smStateUpdate.state == 6) {
                        ATERMi_ApConfigGetResult(&smApConfigResult);
                        printInfo();
                    }
                    OSSendMessage(&smMsgQueue, (OSMessage)1, 0);
                    return false;
                } else {
                    SOFinish();
                    smbIsStarted = false;
                    destroy();
                }
            } else {
                return false;
            }
            if (rakuCfg != NULL) {
                memcpy(rakuCfg->ssid, smApConfigResult.ssid, sizeof(smApConfigResult.ssid));
                rakuCfg->ssidLength = strlen(smApConfigResult.ssid);
                if (smApConfigResult.mode == 1) {
                    // wep104
                    rakuCfg->privacy.mode = 1;
                    rakuCfg->privacy.wep40.keyId = smApConfigResult.wepKeyID;
                    for (int i = 0; i < (s32)ARRAY_LENGTH((rakuCfg->privacy).wep40.key); i++) {
                        memcpy((rakuCfg->privacy).wep40.key[i], smApConfigResult.wepKeys[i], sizeof((rakuCfg->privacy).wep40.key[i]));
                    }
                } else if (smApConfigResult.mode == 2) {
                    // wep104
                    rakuCfg->privacy.mode = 2;
                    rakuCfg->privacy.wep104.keyId = smApConfigResult.wepKeyID;
                    for (int i = 0; i < (s32)ARRAY_LENGTH((rakuCfg->privacy).wep104.key); i++) {
                        memcpy((rakuCfg->privacy).wep104.key[i], smApConfigResult.wepKeys[i], sizeof((rakuCfg->privacy).wep104.key[i]));
                    }
                } else if (smApConfigResult.mode == 4) {
                    // tkip
                    (rakuCfg->privacy).mode = 4;
                    memcpy(rakuCfg->privacy.tkip.key, smApConfigResult.textKeys, sizeof(rakuCfg->privacy.tkip.key));
                    rakuCfg->privacy.tkip.keyLen = strlen(smApConfigResult.textKeys);
                } else if (smApConfigResult.mode == 5) {
                    // aes
                    (rakuCfg->privacy).mode = 6;
                    memcpy(rakuCfg->privacy.aes.key, smApConfigResult.textKeys, sizeof(rakuCfg->privacy.aes.key));
                    rakuCfg->privacy.aes.keyLen = strlen(smApConfigResult.textKeys);
                }
            }
            if (result != NULL) {
                *result = smStateUpdate.isDone;
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
