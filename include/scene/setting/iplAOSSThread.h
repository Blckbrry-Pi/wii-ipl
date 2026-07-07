#ifndef IPL_SCENE_AOSSTHREAD_H
#define IPL_SCENE_AOSSTHREAD_H

#include "utility/iplThread.h"

#include "scene/setting/AOSS/AOSSLink.h"

#include "revolution/ncd/NCDTypes.h"

#include <egg/core/eggHeap.h>

namespace ipl {
    namespace scene {
        class AOSSThread : public utility::ut_thread {
        public:
            AOSSThread(EGG::Heap* heap);
            virtual ~AOSSThread() override;
            virtual void* Run() override;

            bool start();
            void destroy(int);
            bool cancel();
            bool finish(NCDAossConfig* cfgOut, int* finishResult);

            void printInfo();

        private:
            // SOAlloc
            static void* fn_SOAlloc(u32, s32 size);
            // SOFree
            static void fn_SOFree(u32, void* ptr, s32);
            inline AOSSiConfig* getAOSSiCfg() { return &mAossCfg; }

            int unk_0x32c;
            int mPriority;
            void* pThreadStack;
            void* pHeapMem;
            MEMHeapHandle mHeapHandle;
            AOSSiConfig mAossCfg;

            static s32 startTimeHi;
            static u32 startTimeLo;
            static u32 sbss_0x8;
        };
    }  // namespace scene
}  // namespace ipl

#endif  // IPL_SCENE_AOSSTHREAD_H
