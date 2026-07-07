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
            bool finish(NCDApConfig*, int*);
            void printInfo();

        private:
            static void* fn_SOAlloc(u32, s32 size);
            static void fn_SOFree(u32, void* buf, s32 size);

            static void fn_ATERMStateCb(ATERM_TimestampedState*);

            static void* fn_ATERMAlloc(u32 size);
            static void fnATERMFree(void* buf);

            u8 unk_0x32c[4];      // 0x32c
            u32 unk_0x330;        // 0x330
            u32 unk_0x334;        // 0x334
            s32 mPriority;        // 0x338
            void* pHeapMem;       // 0x33c
            void* pThreadStack;   // 0x340
            MEMHeapHandle pHeap;  // 0x344

            static s32 startTimeHi;
            static u32 startTimeLo;
            static bool hasBeenStartedMaybe;
            static OSMessage msgQueueBuf[1];

            // static MEMAllocator allocator;
            static ATERM_TimestampedState timeState;
            static ATERMiApConfigResult apConfigResult;
            static OSMessageQueue msgQueue;

            static int state;
        };
    }  // namespace scene
}  // namespace ipl

#endif  // IPL_SCENE_RAKURAKUTHREAD_H
