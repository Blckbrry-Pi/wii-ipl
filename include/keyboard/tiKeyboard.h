#ifndef TEXTINPUT_KEYBOARD_H
#define TEXTINPUT_KEYBOARD_H

#include "tiTextInputBase.h"

namespace textinput {
    namespace keyboard {

        enum VKeyCode {
            VCODE_RETURN = 0x00,
            VCODE_DELETE = 0x01,
            CODE_UNK_0x02 = 0x02,  // Space
            VCODE_CAPS = 0x03,     // Caps Lock
            VCODE_SHIFT = 0x04,    // Shift
            VCODE_MORE_CHARS = 0x09,
            CODE_UNK_0x0A = 0x0A,               // Change Predict Language
            CODE_UNK_0x0B = 0x0B,               // QWERTY On
            VCODE_QWERTY_OFF = 0x0C,            // QWERTY Off
            CODE_UNK_0x0D = 0x0D,               // Change Sign
            VCODE_SET_AIU_INPUT_MODE_4 = 0x0E,  // Set AIU Input Mode 4 (TODO: Find better name)
            VCODE_TRANSLATEMODE_0 = 0x0F,       // Set Translate Mode to TM_0
            VCODE_TRANSLATEMODE_1 = 0x10,       // Set Translate Mode to TM_1
            VCODE_TRANSLATEMODE_2 = 0x11,       // Set Translate Mode to TM_2
            CODE_UNK_0x12 = 0x12,               // Set AIU Hiragana
            CODE_UNK_0x13 = 0x13,               // Set AIU Katakana
            VCODE_CONVERT_DAKUTEN = 0x14,       // Convert Dakuten (Japanese Kana)
            VCODE_CONVERT_HANDAKU = 0x15,       // Convert Handakuten (Japanese Kana)
            VCODE_CONVERT_SMALL = 0x16,         // Convert Small/lowercase (Japanese Kana)
            CODE_UNK_0x17 = 0x17,
            VCODE_SGNCLOSE = 0x18,
            VCODE_SGNNEXT = 0x19,
            VCODE_SGNPREV = 0x1A,
            CODE_UNK_0x1B = 0x1B,
        };

        class KeyboardBase : public CommandSender {
        public:
            KeyboardBase() : meLanguage(JP) {}

            virtual ~KeyboardBase() {}

            virtual void onKey(u32, void*);                                         // 0x20
            virtual int getType();                                                  // 0x24
            virtual void setLanguage(Language language) { meLanguage = language; }  // 0x28
            virtual Language getLanguage() const { return meLanguage; }             // 0x2C
            virtual void update() {}                                                // 0x30
            virtual void onActive() {}                                              // 0x34

        protected:
            Language meLanguage;  // 0x10
        };

    }  // namespace keyboard
}  // namespace textinput

#endif  // TEXTINPUT_KEYBOARD_H
