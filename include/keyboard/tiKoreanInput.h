#ifndef TEXTINPUT_KOREAN_INPUT_H
#define TEXTINPUT_KOREAN_INPUT_H

#include "tiTextInputBase.h"

namespace textinput {
#if defined(VERSION_43K)
    class HangulStream {
    public:
        HangulStream() : mLength(0), mbKSXFilterEnabled(false), mpKSXFilter(NULL) {}

        void Put(wchar_t ch);
        u16 Get();
        u32 GetLength() const { return mLength; }
        void Flush();
        void Clear();
        void BackSpace();
        wchar_t GetUnfixed(u32 idx) const;
        static wchar_t QwertyToHangul(wchar_t qwertyCh);

        void SetKSXFilterEnabled(bool enabled) { mbKSXFilterEnabled = enabled; }
        void SetDefaultKSXFilter();

    protected:
        wchar_t mChars[8];      // 0x00
        u32 mLength;            // 0x10
        u8 mbKSXFilterEnabled;  // 0x14
        void* mpKSXFilter;      // 0x18
    };
#else
    class HangulStream {
    public:
        HangulStream() {}

        void Put(wchar_t ch) {}
        u16 Get() { return L'\0'; }
        u32 GetLength() const { return 0; }
        void Flush() {}
        void Clear() {}
        void BackSpace() {}
        wchar_t GetUnfixed(u32 idx) const { return L'\0'; }
        static wchar_t QwertyToHangul(wchar_t qwertyCh) { return qwertyCh; }

        void SetKSXFilterEnabled(bool enabled) {}
        void SetDefaultKSXFilter() {}
    };
#endif
}  // namespace textinput

#endif  // TEXTINPUT_KOREAN_INPUT_H
