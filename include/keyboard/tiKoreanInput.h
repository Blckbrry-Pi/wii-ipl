#ifndef TEXTINPUT_KOREAN_INPUT_H
#define TEXTINPUT_KOREAN_INPUT_H

#include "tiTextInputBase.h"

namespace textinput {
#if defined(VERSION_43K)
    class HangulStream {
    public:
        HangulStream() : mLength(0), mbKSXFilterEnabled(false), mpKSXFilter(NULL) {}

        void Put(wchar_t ch);
        wchar_t Get();
        u32 GetLength() const { return mLength; }
        void Flush();
        void Clear();
        void BackSpace();
        wchar_t GetUnfixed(u32 idx) const;
        static wchar_t QwertyToHangul(wchar_t qwertyCh);

        void SetKSXFilterEnabled(bool enabled) { mbKSXFilterEnabled = enabled; }
        void SetDefaultKSXFilter();

    protected:
        struct KSXFilter {
        public:
            bool Allowed(wchar_t ch) const;

            u16 mCount;
            wchar_t mEntries[];
        };
        struct JamoBuilderMap {
            wchar_t from[4];
            wchar_t to;
        };

        static wchar_t SearchBeginning_(const wchar_t* text, u32* lenMatched);
        static wchar_t SearchMiddle_(const wchar_t* text, u32* lenMatched);
        static wchar_t SearchEnding_(const wchar_t* text, u32* lenMatched);

        bool IsValidKSX_(wchar_t ch) const;
        static wchar_t GetBeginning_(wchar_t ch);
        static wchar_t ToSyllable_(wchar_t beginning, wchar_t middle, wchar_t ending);
        static wchar_t HangulJamoToCJKJamo_(wchar_t cjkCh);
        void SkipChars_(u32 length);

        wchar_t mChars[8];             // 0x00
        u32 mLength;                   // 0x10
        u8 mbKSXFilterEnabled;         // 0x14
        const KSXFilter* mpKSXFilter;  // 0x18

        static const KSXFilter csDefaultKSXFilter;

        static const JamoBuilderMap csJamoMapBeginning[];
        static const JamoBuilderMap csJamoMapMiddle[];
        static const JamoBuilderMap csJamoMapEnding[];
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
