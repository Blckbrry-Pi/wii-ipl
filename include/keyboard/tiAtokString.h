
#ifndef TEXTINPUT_ATOK_STRING_H
#define TEXTINPUT_ATOK_STRING_H

#include "keyboard/tiString.h"

#include <eztx.h>
#include <revolution/gx/GXStruct.h>
#include <revolution/mem/allocator.h>

namespace textinput {
    namespace tistring {
#ifndef VERSION_43J
        class WithAtok : public Decolated {
        public:
            struct DrawInfo {
                const wchar_t* unk_0x00;                       // 0x00
                s16 unk_0x04;                                  // 0x04
                s16 unk_0x06;                                  // 0x06
                u8 unk_0x08;                                   // 0x08
                GXColor unk_0x0C __attribute__((aligned(4)));  // 0x0C
                GXColor unk_0x10 __attribute__((aligned(4)));  // 0x10
            };

            WithAtok(u16 maxLen) : Decolated(maxLen) {}

            virtual void pushBack(wchar_t ch) override {}                // 0x010
            virtual void popBack() override {}                           // 0x014
            virtual void inputChar(wchar_t ch) override {}               // 0x050
            virtual void backSpace() override {}                         // 0x05C
            virtual void confirm(const wchar_t* string) override {}      // 0x060
            virtual bool moveCursorRight() override { return false; }    // 0x064
            virtual bool moveCursorLeft() override { return false; }     // 0x068
            virtual void getCursorPos(u32* start, u32* end) override {}  // 0x080

            virtual bool isFix() { return true; }                                 // 0x0D0
            virtual void setFix(bool _) {}                                        // 0x0D4
            virtual void initConverting() {}                                      // 0x0D8
            virtual bool isConverting() { return false; }                         // 0x0DC
            virtual const wchar_t* getConfirmedWCString() const { return NULL; }  // 0x0E0
            virtual s32 getCurrentNumPredicted() { return NULL; }                 // 0x0E4
            virtual void getPredicted(int, wchar_t*) {}                           // 0x0E8
            virtual void commitPredicted(int) {}                                  // 0x0EC
            virtual void setSelectedCandidate(s32) {}                             // 0x0F0
            virtual bool isCandidateSelected() { return false; }                  // 0x0F4
            virtual s32 getSelectedCandidate() { return 0; }                      // 0x0F8
            virtual void init() {}                                                // 0x0FC
            virtual void setInputting(wchar_t) {}                                 // 0x100
            virtual void getDrawString(DrawInfo&) {}                              // 0x104
            virtual void openDictionary(void* atokDict, int atokDictSize, void* apotDict, int apotDictSize, void* nintendoDict,
                                        int nintendoDictSize) {}     // 0x108
            virtual void closeDictionary() {}                        // 0x10C
            virtual bool hasConfirmedString() { return false; }      // 0x110
            virtual void enableConfirmedString(bool enabled) {}      // 0x114
            virtual void startConverting() {}                        // 0x118
            virtual bool isDictionaryOpened() { return false; }      // 0x11C
            virtual void changeKanaMode(bool) {}                     // 0x120
            virtual u16 getInputStringLength() { return 0; }         // 0x124
            virtual void resetRelation() {}                          // 0x128
            virtual void setFixMode(bool) {}                         // 0x12C
            virtual void setFixPrediction(int, const char**) {}      // 0x130
            virtual void setDefaultPrediction(int, const char**) {}  // 0x134
            virtual s16 getSelectedConverting() { return 0; }        // 0x138
            virtual u32 getFixedPredictionNum() { return 0; }        // 0x13C

            void setUnk_0x080(bool flag) {}
            bool getUnk_0x081() { return false; }
            bool getUnk_0x082() { return false; }
            void confirmAll() {}
            void unselect() {}
            void updatePrediction() {}
        };
#else
        class WithAtok : public Decolated {
        public:
            struct DrawInfo {
                const wchar_t* unk_0x00;                       // 0x00
                s16 unk_0x04;                                  // 0x04
                s16 unk_0x06;                                  // 0x06
                u8 unk_0x08;                                   // 0x08
                GXColor unk_0x0C __attribute__((aligned(4)));  // 0x0C
                GXColor unk_0x10 __attribute__((aligned(4)));  // 0x10
            };

            WithAtok(u16 maxLen)
                : Decolated(maxLen), mbIsFix(false), unk_0x04D(false), mbHasConfirmedString(false), mbIsDictionaryOpened(false), unk_0x05C(40),
                  mCurrNumPredicted(-1), mFixedPredNum(0), mFixedPreds(NULL), mDefaultPredNum(0), unk_0x078(0), mSelectedConverting(0),
                  unk_0x080(false), unk_0x081(false), unk_0x082(false), unk_0x0C8(0), unk_0x0CA(0) {
                unk_0x068 = 0;
            }

            virtual ~WithAtok() override;                              // 0x008
            virtual void pushBack(wchar_t ch) override;                // 0x010
            virtual void popBack() override;                           // 0x014
            virtual void inputChar(wchar_t ch) override;               // 0x050
            virtual void backSpace() override;                         // 0x05C
            virtual void confirm(const wchar_t* string) override;      // 0x060
            virtual bool moveCursorRight() override;                   // 0x064
            virtual bool moveCursorLeft() override;                    // 0x068
            virtual void getCursorPos(u32* start, u32* end) override;  // 0x080

            virtual void openDictionary(void* atokDict, int atokDictSize, void* apotDict, int apotDictSize, void* nintendoDict,
                                        int nintendoDictSize);                                     // 0x0D0
            virtual void closeDictionary() { mbIsDictionaryOpened = false; }                       // 0x0D4
            virtual void init();                                                                   // 0x0D8
            virtual void setInputting(wchar_t);                                                    // 0x0DC
            virtual bool isFix() { return mbIsFix && Decolated::isKanaFix(); }                     // 0x0E0
            virtual void setFix(bool newIsFix) { mbIsFix = newIsFix; }                             // 0x0E4
            virtual void initConverting();                                                         // 0x0E8
            virtual bool isConverting() { return unk_0x068 == 1; }                                 // 0x0EC
            virtual const wchar_t* getConfirmedWCString() const { return mpConfirmedWcString; }    // 0x0F0
            virtual const wchar_t* getConvertingWCString() const { return mpConvertingWcString; }  // 0x0F4
            virtual s32 getCurrentNumPredicted() { return mCurrNumPredicted; }                     // 0x0F8
            virtual void getPredicted(int, wchar_t*);                                              // 0x0FC
            virtual void commitPredicted(int);                                                     // 0x100
            virtual void resetRelation();                                                          // 0x104
            virtual void setFixMode(bool);                                                         // 0x108
            virtual void changeKanaMode(bool);                                                     // 0x10C
            virtual void setSelectedCandidate(s32);                                                // 0x110
            virtual s32 getSelectedCandidate() { return mSelectedCandidate; }                      // 0x114
            virtual bool isCandidateSelected() { return mSelectedCandidate != -1; }                // 0x118
            virtual void getDrawString(DrawInfo&);                                                 // 0x11C
            virtual void startConverting() {                                                       // 0x120
                mSelectedConverting = 0;
                mSelectedCandidate = -1;
                setCandidate(L'\0');
                translateConverting();
            }
            virtual bool hasConfirmedString() { return mbHasConfirmedString; }    // 0x124
            virtual void enableConfirmedString(bool enabled);                     // 0x128
            virtual bool isDictionaryOpened() { return mbIsDictionaryOpened; }    // 0x12C
            virtual u16 getInputStringLength();                                   // 0x130
            virtual void setFixPrediction(int count, const char** predictions) {  // 0x134
                mFixedPredNum = count;
                mFixedPreds = predictions;
            }
            virtual void setDefaultPrediction(int, const char**);                // 0x138
            virtual s32 getSelectedConverting() { return mSelectedConverting; }  // 0x13C
            virtual u32 getFixedPredictionNum() { return mFixedPredNum; }        // 0x140
            virtual u32 getDefaultPredictionNum() { return mDefaultPredNum; }    // 0x144

            void translateConverting();

            void setUnk_0x080(bool flag) { unk_0x080 = flag; }
            bool getUnk_0x081() { return unk_0x081; }
            bool getUnk_0x082() { return unk_0x082; }
            void confirmAll();
            void unselect();
            void updatePrediction();

        private:
            bool mbIsFix;                                     // 0x04C
            u8 unk_0x04D;                                     // 0x04D
            bool mbHasConfirmedString;                        // 0x04E
            bool mbIsDictionaryOpened;                        // 0x04F
            u8 unk_0x050[0x4];                                // 0x050
            const wchar_t* mpConfirmedWcString;               // 0x054
            const wchar_t* mpConvertingWcString;              // 0x058
            u32 unk_0x05C;                                    // 0x05C
            s32 mCurrNumPredicted;                            // 0x060
            s32 mSelectedCandidate;                           // 0x064
            u32 unk_0x068;                                    // 0x068
            u32 mFixedPredNum;                                // 0x06C
            const char** mFixedPreds;                         // 0x070
            u32 mDefaultPredNum;                              // 0x074
            u32 unk_0x078;                                    // 0x078
            u32 mSelectedConverting;                          // 0x07C
            bool unk_0x080;                                   // 0x080
            bool unk_0x081;                                   // 0x081
            bool unk_0x082;                                   // 0x082
            u8 unk_0x084[0x44] __attribute__((aligned(4)));   // 0x084
            u16 unk_0x0C8;                                    // 0x0C8
            u16 unk_0x0CA;                                    // 0x0CA
            u8 unk_0x0CC[0x11C] __attribute__((aligned(4)));  // 0x0CC
        };
#endif
    }  // namespace tistring
}  // namespace textinput

#endif  // TEXTINPUT_ATOK_STRING_H
