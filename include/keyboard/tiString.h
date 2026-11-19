#ifndef TEXTINPUT_STRING_BASE_H
#define TEXTINPUT_STRING_BASE_H

#include "tiKoreanInput.h"

#include <revolution/kpr.h>
#include <revolution/mem/allocator.h>
#include <revolution/types.h>

namespace textinput {
    namespace tistring {
        class StringBase {
        public:
            StringBase(u16 maxLength)
                : muMaxLength(maxLength), muLength(0), mpszString(NULL), mpszTmpString(NULL), mwcCandidate(0), mpAllocator(NULL) {}

            virtual ~StringBase();  // 0x08

            virtual void create(MEMAllocator* allocator);                   // 0x0C
            virtual void pushBack(wchar_t ch);                              // 0x10
            virtual void popBack();                                         // 0x14
            virtual void clear();                                           // 0x18
            virtual u16 getLength() const { return muLength; }              // 0x1C
            virtual bool append(const wchar_t* string);                     // 0x20
            virtual bool insert(u16 pos, const wchar_t* string);            // 0x24
            virtual void remove(u16 pos, u16 len);                          // 0x28
            virtual void replace(u16 pos, u16 len, const wchar_t* string);  // 0x2C

            virtual void set(const wchar_t* string);  // 0x30
            virtual void setAt(u16 pos, wchar_t ch);  // 0x34

            virtual void setLength(u16);  // 0x38

            virtual wchar_t* getWCString() const { return mpszString; }  // 0x3C

            virtual void setCandidate(wchar_t candidate) { mwcCandidate = candidate; }  // 0x40
            virtual wchar_t getCandidate() const { return mwcCandidate; }               // 0x44
            virtual bool hasCandidate() const { return mwcCandidate != L'\0'; }         // 0x48

            virtual wchar_t getLastWChar();  // 0x4C

            static u32 wcsLen(const wchar_t* wcStr);

        protected:
            u16 muMaxLength;  // 0x04
            u16 muLength;     // 0x06

            wchar_t* mpszString;     // 0x08
            wchar_t* mpszTmpString;  // 0x0C

            wchar_t mwcCandidate;  // 0x10

            MEMAllocator* mpAllocator;  // 0x14
        };

        class KanaStream {
        public:
            KPRQueue mQueue;
        };

        class Decolated : public StringBase {
        public:
            enum TranslateMode {
                TM_NONE = 0,
                TM_ROMAJI_HIRAGANA = 1,
                TM_ROMAJI_KATAKANA = 2,
                TM_ROMAJI_KANAFIX = 3,
            };

            Decolated(u16 maxLen) : StringBase(maxLen), mCursorStart(0), mCursorEnd(0), mbIsSustaining(0), mTranslateMode(TM_NONE) {
                initKanaConverter();
            }

            virtual void clear() override {
                StringBase::clear();
                mCursorStart = 0;
                mCursorEnd = 0;
                mbIsSustaining = false;
                initKanaConverter();
            }
            virtual void set(const wchar_t* string) override {
                StringBase::set(string);
                mCursorStart = getLength();
                mCursorEnd = mCursorStart;
            }

            virtual void inputChar(wchar_t ch);
            virtual void inputString(const wchar_t* string);
            void inputString(const wchar_t* string, TranslateMode translateMode);
            virtual void deleteChar() {}
            virtual void backSpace();
            virtual void confirm(const wchar_t* string);
            virtual bool moveCursorRight();
            virtual bool moveCursorLeft();
            virtual void setCursorPos(u32 newPos);
            virtual void onSustain();
            virtual void offSustain();
            virtual bool isOnSustain() { return mbIsSustaining; }
            virtual u32 getCursorPos() const { return mCursorStart; }
            virtual void getCursorPos(u32* start, u32* end);
            virtual bool canBackSpace();
            virtual bool deleteForward();
            virtual void getSelected(u32& start, u32& end);
            virtual wchar_t getWCharAtCursor();
            virtual void replaceAtCursor(wchar_t newCh);
            virtual bool isDakuten();
            virtual void converDakuten();
            virtual bool isHandaku();
            virtual void converHandaku();
            virtual void convertAll();
            virtual bool isSmall();
            virtual void converSmall();
            virtual bool atTheBeginningOfASentence();
            virtual void initKanaConverter();
            virtual wchar_t* getKanaBuffer() { return mKanaBuf; }
            virtual bool isKanaFix() const {
                if (mTranslateMode == TM_ROMAJI_KANAFIX)
                    return mHangulStream.GetLength() == 0;
                return KPRLookAhead(&mKanaStream.mQueue, NULL, 0) == 0;
            }
            virtual void confirmKana();
            virtual void clearKana();
            virtual void EnableKSXFilter(bool enabled) {
                mHangulStream.SetKSXFilterEnabled(enabled);
                mHangulStream.SetDefaultKSXFilter();
            }

            TranslateMode getTranslateMode() const { return mTranslateMode; }
            void setTranslateMode(TranslateMode translateMode);
            void setLength(u16 newLen) override;

        private:
            inline u16 translateInputChar(wchar_t ch, wchar_t* stringOut);

            u32 mCursorStart;              // 0x18
            u32 mCursorEnd;                // 0x1C
            bool mbIsSustaining;           // 0x20
            TranslateMode mTranslateMode;  // 0x24
            KanaStream mKanaStream;        // 0x28
            HangulStream mHangulStream;    // 0x40
            wchar_t mKanaBuf[5];           // 0x42 (U/E/J)
        };
    }  // namespace tistring
}  // namespace textinput

#endif  // TEXTINPUT_STRING_BASE_H
