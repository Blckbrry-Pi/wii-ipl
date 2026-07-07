#ifndef IPL_SCENE_USBAPTHREAD_H
#define IPL_SCENE_USBAPTHREAD_H

#include "utility/iplThread.h"

namespace ipl {
    namespace scene {
        void USBAPRegisterCallback(int);
        class USBAPThread {
        public:
            USBAPThread();

            void Init(u16* result, u8* statePtr);
            void cancel();
            BOOL is();
            void setData(const wchar_t* text, u8* data);
            static void callback();

        private:
            wchar_t mNickname[10];
        };
    }  // namespace scene
}  // namespace ipl

#endif  // IPL_SCENE_USBAPTHREAD_H
