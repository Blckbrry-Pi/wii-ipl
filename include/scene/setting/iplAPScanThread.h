#ifndef IPL_SCENE_APSCANTHREAD_H
#define IPL_SCENE_APSCANTHREAD_H

#include "utility/iplThread.h"

#include <revolution/wd.h>

namespace ipl {
    namespace scene {
        class APScanThread : public utility::ut_thread {
        public:
            APScanThread();
            virtual ~APScanThread();

            virtual void* Run() override;

            void setParam();
            void setResultData(u16*);

        private:
            WDScanParam mScanParams;
            u16* mpBssDescriptorsResultData;
        };
    }  // namespace scene
}  // namespace ipl

#endif  // IPL_SCENE_APSCANTHREAD_H
