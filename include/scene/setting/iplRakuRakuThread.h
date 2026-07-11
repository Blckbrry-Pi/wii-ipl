#ifndef IPL_SCENE_RAKURAKUTHREAD_H
#define IPL_SCENE_RAKURAKUTHREAD_H

#include <revolution/mem/allocator.h>
#include <revolution/mem/expHeap.h>
#include <revolution/ncd/NCDTypes.h>

#include "egg/core/eggHeap.h"
#include "utility/iplThread.h"

#include "scene/setting/ATERM.h"

namespace ipl {
    namespace scene {
        // extern "C" {

        // static void* fn_SOAlloc(u32, s32 size);
        // static void fn_SOFree(u32, void* buf, s32 size);

        // static void* fn_ATERMAlloc(u32 size);
        // static void fnATERMFree(void* buf);
        // }

        typedef struct {
            MEMAllocator _;
        } RakuRakuThread_AllocatorWrapper;
        class RakuRakuThread : public utility::ut_thread {
        public:
            RakuRakuThread(EGG::Heap* heap);
            virtual ~RakuRakuThread();

            void destroy();
            bool start();
            virtual void* Run() override;

            int getState();
            void cancel();
            bool finish(NCDApConfig* apCfg, int* result);
            void printInfo();

        private:
            static void* SOAlloc(u32, s32 size);
            static void SOFree(u32, void* buf, s32 size);

            static void ATERMStateCb(ATERMStateUpdate*);

            static void* ATERMAlloc(u32 size);
            static void ATERMFree(void* buf);

            u8 unk_0x32C[4];        // 0x32C
            BOOL mbThreadFinished;  // 0x330
            BOOL mbThreadStarted;   // 0x334
            s32 mPriority;          // 0x338
            void* mpHeapMem;        // 0x33c
            void* mpThreadStack;    // 0x340
            MEMHeapHandle mpHeap;   // 0x344

            static s32 smUpdateTimeHi;
            static u32 smUpdateTimeLo;
            static bool smbIsStarted;
            static OSMessage smMsgQueueBuf[1];

            static MEMAllocator smRakuRakuAllocator;
            static ATERMStateUpdate smStateUpdate;
            static ATERMiApConfigResult smApConfigResult;
            static OSMessageQueue smMsgQueue;

            static ATERMConfigState smState;
        };
    }  // namespace scene
}  // namespace ipl

#endif  // IPL_SCENE_RAKURAKUTHREAD_H
