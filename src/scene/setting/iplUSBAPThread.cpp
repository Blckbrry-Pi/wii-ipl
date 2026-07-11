#include "scene/setting/iplUSBAPThread.h"

#include "scene/setting/iplUSBAP.h"
#include "utility/iplCharacterCode.h"

#include "string.h"

#include <revolution/soex.h>

namespace ipl {
    namespace scene {
        USBAPThread::USBAPThread() {
        }

        void USBAPRegisterCallback(int newState) {
            if (newState == 1) {
                *usbapState = 1;
                OSReport("Registration completed !\n");
            } else {
                *usbapState = 2;
                OSReport("Registration failed...\n");
            }
        }

        void USBAPThread::Init(u16* result, u8* statePtr) {
            int newPriority = OSGetThreadPriority(OSGetCurrentThread()) + 1;
            if (USBAPStartRegistration(0, newPriority, FALSE, msNickname, USBAPRegisterCallback, result, statePtr) == TRUE) {
                OSReport("Registration started\n");
            }
        }

        void USBAPThread::cancel() {
            USBAPCancelRegistration();
        }

        BOOL USBAPThread::is() {
            return USBAPIsThreadTerminated();
        }

        void USBAPThread::setData(const wchar_t* text, u8* data) {
            usbapState = data;
            memcpy(msNickname, text, sizeof(msNickname));
            utility::CharacterCode::changeEndian(msNickname, 10);
        }

        void USBAPThread::callback() {
        }
    }  // namespace scene
}  // namespace ipl
