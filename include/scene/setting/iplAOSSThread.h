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
            static void* SOAlloc(u32, s32 size);
            // SOFree
            static void SOFree(u32, void* ptr, s32);
            inline AOSSiConfig* getAOSSiCfg() { return &mAossCfg; }

            int mAOSSErrno;
            int mPriority;
            void* mpThreadStack;
            void* mpHeapMem;
            MEMHeapHandle mpHeap;
            AOSSiConfig mAossCfg;

            static s32 smStartTimeHi;
            static u32 smStartTimeLo;
            static BOOL smIsStarted;
        };
    }  // namespace scene
}  // namespace ipl

#endif  // IPL_SCENE_AOSSTHREAD_H
