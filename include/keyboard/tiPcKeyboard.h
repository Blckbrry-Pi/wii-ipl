#ifndef TEXTINPUT_PC_KEYBOARD_H
#define TEXTINPUT_PC_KEYBOARD_H

#include "tiKeyboard.h"
#include "tiNw4rManager.h"
#include "tiTextInputBase.h"

#include "tiString.h"

#include "tiPkData.h"

namespace textinput {
    class Manager;
    namespace keyboard {
        namespace pctype {
            class LayoutByNW4R;

            struct PaneToAnimation;
            struct GridData;
            struct PaneNameToControlKey {
                const char paneName[20];
                VKeyCode code;
            };
            struct PaneVisibility;
            struct CharCodeToAblaut;
            struct LanguageDependencyData {
                const PaneNameToCharCodeWithModeAscii* pPaneChars;  // 0x00
                const GridData* pGridData;                          // 0x04
                const PaneNameToControlKey* pPaneNameToControlKey;  // 0x08
                const PaneVisibility* pPaneVisibility;              // 0x0C
                const CharCodeToAblaut* pCharCodeToAblaut;          // 0x10
            };

            enum AnimationId {
                ANIM_NORMAL = 0,
                ANIM_FOCUS_IN,
                ANIM_FOCUS_OUT,
                ANIM_ROLL_OVER,
                ANIM_PUSHED,

                ANIM_TOGGLE_ON,
                ANIM_TOGGLE_OFF,
                ANIM_TARGET_ON,
                ANIM_TOGGLE_FOCUS_IN,
                ANIM_TOGGLE_FOCUS_OUT,
                ANIM_TOGGLE_NORMAL,
                ANIM_TOGGLE_PUSHED,

                ANIM_ACTIVE_ON,
                ANIM_ACTIVE_OFF,
                ANIM_ACTIVE_NORMAL,
                ANIM_INACTIVE_NORMAL,

                ANIM_UNK_16,

                ANIM_MAX,
            };

            enum KeyType {
                KEY_TYPE_NORMAL = 0,
                KEY_TYPE_SHIFTCAPS,
                KEY_TYPE_TOGGLE,
                KEY_TYPE_ONOFF,
            };

            // TODO
            class Base : public KeyboardBase {
            public:
                enum TranslateMode {
                    TM_00 = 0,
                    TM_01,
                    TM_02,
                };

                enum InputMode {
                    IM_00 = 0,
                    IM_01,
                    IM_02,
                    IM_03,
                    IM_04,
                    IM_05,
                    IM_HIRAGANA,
                    IM_KATAKANA,
                };

                class KeyState {
                public:
                    KeyState(Base* kb) : mABCFlags(0), unk_0x04(0), mAIUFlags(0), meLanguage(USA), mpLangDepData(NULL), mpKeyboard(kb) {}
                    enum InternalABCInputMode {
                        IM_ABC_DIRECT = 0,
                        IM_ABC_HIRAGANA,
                        IM_ABC_KATAKANA,
                    };
                    enum InternalAIUInputMode {
                        IM_AIU_HIRAGANA = 0,
                        IM_AIU_KATAKANA,
                    };

                    wchar_t getWCCode(char* paneName);
                    wchar_t getWCCode(u32 idk);

                    void andABCFlag(u32 value) { setABCFlag(mABCFlags & value); }
                    void enableABCFlag(u32 newlyEnabled) { setABCFlag(getABCKanaFlags2() | newlyEnabled); }
                    void disableABCFlag(u32 newlyDisabled) { setABCFlag(getABCKanaFlags2() & ~newlyDisabled); }
                    void setABCFlag(u32 newFlags);
                    // u32 getABCMode() {}

                    bool hasKanaBitsSet() { return (mAIUFlags & ~0xF) != 0; }
                    void resetKanaBits() { mAIUFlags = (mAIUFlags & 0xF) | (0 & ~0xF); }
                    // InputMode getAiuInputMode() { return (InputMode)nw4r::lyt::detail::GetBits(mAiuInputMode, 0, 4); }

                    // ABC flags
                    u32 getABCKanaFlags() { return mABCFlags & KANAFLAGS_MASK; }
                    u32 getABCKanaFlags2() const { return mABCFlags & KANAFLAGS_MASK; }
                    // void setABCKanaFlags(u32 newFlags) {
                    //     u32 oldFlags = getABCKanaFlags();
                    //     if (oldFlags != newFlags) {
                    //         mABCFlags = (mABCFlags & ~KANAFLAGS_MASK) | (newFlags & KANAFLAGS_MASK);
                    //         refresh_();
                    //     }
                    // }
                    bool isABCCaps() const { return mABCFlags & MODFLAG_CAPS; }
                    bool isABCShift() const { return mABCFlags & MODFLAG_SHIFT; }
                    void setABCCaps(bool flagState) { mABCFlags = (mABCFlags & ~MODFLAG_CAPS) | (flagState ? MODFLAG_CAPS : 0); }
                    void setABCShift(bool flagState) { mABCFlags = (mABCFlags & ~MODFLAG_SHIFT) | (flagState ? MODFLAG_SHIFT : 0); }
                    void toggleABCCaps() { mABCFlags = (mABCFlags & ~MODFLAG_CAPS) | ((mABCFlags ^ MODFLAG_CAPS) & MODFLAG_CAPS); }
                    void toggleABCShift() { mABCFlags = (mABCFlags & ~MODFLAG_SHIFT) | ((mABCFlags ^ MODFLAG_SHIFT) & MODFLAG_SHIFT); }

                    u32 getUpdatedABCFlags(InternalABCInputMode mode) const { return (mABCFlags & ~IM_MASK) | ((mode << IM_SHIFT) & IM_MASK); }
                    void setABCInputModeInternal(InternalABCInputMode mode) {
                        u32 oldMode = getABCInputModeInternal();
                        if (oldMode != mode) {
                            mABCFlags = getUpdatedABCFlags(mode);
                            refresh_();
                        }
                    }
                    InternalABCInputMode getABCInputModeInternal() const { return (InternalABCInputMode)((mABCFlags & IM_MASK) >> IM_SHIFT); }
                    bool abcInputModeInternalIs(InternalABCInputMode mode) {
                        u32 currMode = getABCInputModeInternal();
                        return mode == currMode;
                    }
                    InputMode getABCInputMode() const {
                        switch (getABCInputModeInternal()) {
                            case IM_ABC_DIRECT:
                                return IM_00;
                            case IM_ABC_HIRAGANA:
                                return IM_HIRAGANA;
                            case IM_ABC_KATAKANA:
                                return IM_KATAKANA;
                            default:
                                return IM_HIRAGANA;
                        }
                    }
                    void setABCInputMode(InputMode mode) {
                        switch (mode) {
                            case IM_00:
                                setABCInputModeInternal(IM_ABC_DIRECT);
                                break;
                            case IM_HIRAGANA:
                                setABCInputModeInternal(IM_ABC_HIRAGANA);
                                break;
                            case IM_KATAKANA:
                                setABCInputModeInternal(IM_ABC_KATAKANA);
                                break;
                            default:
                                break;
                        }
                    }

                    TranslateMode getTranslateMode() const {
                        switch (getABCInputModeInternal()) {
                            case IM_ABC_DIRECT:
                                return TM_00;
                            case IM_ABC_HIRAGANA:
                                return TM_01;
                            case IM_ABC_KATAKANA:
                                return TM_02;
                            default:
                                return TM_00;
                        }
                    }

                    // AIU flags
                    u32 getAIUKanaFlags() { return mAIUFlags & KANAFLAGS_MASK; }
                    void setAIUKanaFlags(u32 newFlags) {
                        u32 oldFlags = getAIUKanaFlags();
                        if (oldFlags != newFlags) {
                            mAIUFlags = (mAIUFlags & ~KANAFLAGS_MASK) | (newFlags & KANAFLAGS_MASK);
                            refresh_();
                        }
                    }
                    bool isAIUKanaDakuten() { return mAIUFlags & KANAFLAG_DAKUTEN; }
                    bool isAIUKanaHandaku() { return mAIUFlags & KANAFLAG_HANDAKU; }
                    bool isAIUKanaSmall() { return mAIUFlags & KANAFLAG_SMALL; }
                    void setAIUKanaDakuten(bool flagState) { mAIUFlags = (mAIUFlags & ~KANAFLAG_DAKUTEN) | (flagState ? KANAFLAG_DAKUTEN : 0); }
                    void setAIUKanaHandaku(bool flagState) { mAIUFlags = (mAIUFlags & ~KANAFLAG_HANDAKU) | (flagState ? KANAFLAG_HANDAKU : 0); }
                    void setAIUKanaSmall(bool flagState) { mAIUFlags = (mAIUFlags & ~KANAFLAG_SMALL) | (flagState ? KANAFLAG_SMALL : 0); }

                    u32 getUpdatedAIUMode(InternalAIUInputMode mode) const { return (mAIUFlags & ~IM_MASK) | ((mode << IM_SHIFT) & IM_MASK); }
                    void setAIUInputModeInternal(InternalAIUInputMode mode) {
                        u32 oldMode = getAIUInputModeInternal();
                        if (oldMode != mode) {
                            mAIUFlags = getUpdatedAIUMode(mode);
                            refresh_();
                        }
                    }
                    InternalAIUInputMode getAIUInputModeInternal() const { return (InternalAIUInputMode)((mAIUFlags & IM_MASK) >> IM_SHIFT); }
                    bool aiuInputModeInternalIs(InternalAIUInputMode mode) {
                        u32 currMode = getAIUInputModeInternal();
                        return mode == currMode;
                    }
                    InputMode getAIUInputMode() const {
                        switch (getAIUInputModeInternal()) {
                            case IM_AIU_HIRAGANA:
                                return IM_HIRAGANA;
                            case IM_AIU_KATAKANA:
                                return IM_KATAKANA;
                            default:
                                return IM_HIRAGANA;
                        }
                    }
                    void setAIUInputMode(InputMode mode) {
                        switch (mode) {
                            case IM_HIRAGANA:
                                setAIUInputModeInternal(IM_AIU_HIRAGANA);
                                break;
                            case IM_KATAKANA:
                                setAIUInputModeInternal(IM_AIU_KATAKANA);
                                break;
                            default:
                                break;
                        }
                    }

                    // Some other flags idk
                    u32 isAIU() { return unk_0x04; }
                    void setIsAIU(bool newState) {
                        if (newState != isAIU()) {
                            unk_0x04 = newState;
                            refresh_();
                        }
                    }

                    void init();

                    // bool allowDakuten() { return nw4r::lyt::detail::TestBit(mAIUFlags, 5); }
                    // bool allowHandaku() { return nw4r::lyt::detail::TestBit(mAIUFlags, 6); }
                    // bool allowSmallKana() { return nw4r::lyt::detail::TestBit(mAIUFlags, 7); }

                    VKeyCode getControlKey(char* paneName);
                    void refreshText(nw4r::lyt::Pane* pane);

                    // TODO: remove and obj oriented
                    friend class Base;
                    friend class LayoutByNW4R;

                private:
                    void refresh_();

                    u32 mABCFlags;                                // 0x0
                    u32 unk_0x04;                                 // 0x4
                    u32 mAIUFlags;                                // 0x8
                    Language meLanguage;                          // 0xC
                    const LanguageDependencyData* mpLangDepData;  // 0x10
                    Base* mpKeyboard;                             // 0x14

                    static const u32 IM_SHIFT = 0;
                    static const u32 IM_MASK = 0x0000000F;

                public:
                    static const u32 KANAFLAGS_MASK = ~IM_MASK;

                    static const u32 KANAFLAG_DAKUTEN = 0x20;
                    static const u32 KANAFLAG_HANDAKU = 0x40;
                    static const u32 KANAFLAG_SMALL = 0x80;

                    static const u32 MODFLAG_SHIFT = 0x80;
                    static const u32 MODFLAG_CAPS = 0x40;
                };

                Base(Manager* manager) : mKeyState(this), mbIsOnlyQwerty(false), mpAllocator(NULL), mpManager(manager) {}
                virtual ~Base();                                                                         // 0x08
                virtual void create(MEMAllocator* allocator) override;                                   // 0x0C
                virtual void init() override;                                                            // 0x10
                virtual void updateFromReceiver(u32, void*) override;                                    // 0x1C
                virtual void onKey(u32, void*) override;                                                 // 0x20
                virtual int getType() override { return 0; }                                             // 0x24
                virtual void setLanguage(Language) override;                                             // 0x28
                virtual void onActive() override;                                                        // 0x34
                virtual void inputCharCode(wchar_t);                                                     // 0x38
                virtual u8* getState() { return unk_0x14; }                                              // 0x3C
                virtual bool isShiftOn() const { return mKeyState.isABCShift(); }                        // 0x40
                virtual bool isCapsOn() const { return mKeyState.isABCCaps(); }                          // 0x44
                virtual bool isABC() { return mKeyState.unk_0x04 == 0; }                                 // 0x48
                virtual void setABC(bool);                                                               // 0x4C
                virtual InputMode getABCInputMode() const { return IM_00; }                              // 0x50
                virtual InputMode getAIUInputMode() const { return mKeyState.getAIUInputMode(); }        // 0x54
                virtual TranslateMode getTranslateMode() const { return mKeyState.getTranslateMode(); }  // 0x58
                virtual void setTranslateMode(TranslateMode);                                            // 0x5C
                virtual void onlyQwerty(bool);                                                           // 0x60
                virtual void setLangKeyActive(bool);                                                     // 0x64
                virtual void setInputModeJP(bool isAbc, u32 abcMode, u32 aiuMode) {}                     // 0x68
                virtual void setInputModeCK(u32) {}                                                      // 0x6C
                virtual void refreshState() {}                                                           // 0x70
                virtual void changeABCInputMode(InputMode);                                              // 0x74
                virtual void changeAIUInputMode(InputMode);                                              // 0x78
                virtual void onClose();                                                                  // 0x7C
                virtual wchar_t getWCCode(char*);                                                        // 0x80
                virtual VKeyCode getControlKey(char*);                                                   // 0x84
                virtual void sendInputWChar(wchar_t, bool);                                              // 0x88
                virtual void goSignInputMode() {}                                                        // 0x8C
                virtual void changePredictLanguage() {}                                                  // 0x90
                virtual void updateFixMode();                                                            // 0x94

                bool isOnlyQwerty() { return mbIsOnlyQwerty; }
                bool isLangKeyActive() { return mbIsLangKeyActive; }

                KeyState* getKeyState() { return &mKeyState; }

            protected:
                wchar_t transformHardwareWChar(wchar_t ch) const;

                Manager* mgr() { return mpManager; }
                const Manager* mgr() const { return mpManager; }

                u8 unk_0x14[0x8];                             // 0x14
                const LanguageDependencyData* mpLangDepData;  // 0x1C
                KeyState mKeyState;                           // 0x20
                MEMAllocator* mpAllocator;                    // 0x38
                bool mbIsOnlyQwerty;                          // 0x3C
                bool mbIsLangKeyActive;                       // 0x3D
                Manager* mpManager;                           // 0x40
            };

            class LayoutByNW4R;

            class EventHandler : public nw4rmanager::TiEventHandler {
            public:
                EventHandler(LayoutByNW4R* lyt) : mpLayout(lyt) {}

                virtual void onTiEvent(gui::PaneComponent* paneComponent, u32 event, Input* input) override;

            private:
                LayoutByNW4R* mpLayout;  // 0xC
            };
            class UIObj : public gui::EventHandler {
            public:
                class Listener {
                public:
                    virtual void onEvent(UIObj*, u32, void*);
                };

                UIObj(u32 keyType, LayoutByNW4R* layout, UIObj::Listener* listener) : keyType(keyType), mpLayout(layout), mpListener(listener) {}
                virtual void onEvent(gui::GUIComponent& comp, u32 event, void* data) override {
                    onGUIEvent((gui::PaneComponent&)comp, event, (nw4rmanager::TiEventHandler::Input*)data);
                }  // 0x0C
                virtual void onGUIEvent(gui::PaneComponent& comp, u32 event, nw4rmanager::TiEventHandler::Input* data) {}  // 0x18

            protected:
                u32 keyType;             // 0x08
                LayoutByNW4R* mpLayout;  // 0x0C
                Listener* mpListener;    // 0x10
            };

            class UIModifierButton : public UIObj {
            public:
                UIModifierButton(u32 keyType, LayoutByNW4R* layout, UIObj::Listener* listener);
                void Create(nw4rmanager::Layout* lyt, const char* paneNameP, const char* paneNameB);
                virtual void onGUIEvent(gui::PaneComponent& comp, u32 event, nw4rmanager::TiEventHandler::Input* data) override;  // 0x18

                void setBoundingTriggerTarget(bool bEnable);

                bool isActive() const { return mbIsActive; }
                void setActive(bool newIsActive) { mbIsActive = newIsActive; }

                nw4rmanager::AnmPane* getAnmPane() { return mpAnmPane; }

            private:
                gui::PaneComponent* mpPanePaneComponent;      // 0x14
                gui::PaneComponent* mpBoundingPaneComponent;  // 0x18
                nw4rmanager::AnmPane* mpAnmPane;              // 0x1C
                bool mbIsActive;                              // 0x20
            };
            class UIModePanel : public UIObj {
            public:
                UIModePanel(u32 keyType, LayoutByNW4R* layout, UIObj::Listener* listener);
                void Create(nw4rmanager::Layout* lyt);
                virtual void onGUIEvent(gui::PaneComponent& comp, u32 event, nw4rmanager::TiEventHandler::Input* data) override;  // 0x18

                void setBoundingTriggerTargets(bool bEnable);

                nw4r::lyt::Pane* getTextboxModeA() { return mpTextboxModeA; }
                nw4r::lyt::Pane* getTextboxModeB() { return mpTextboxModeB; }
                nw4r::lyt::Pane* getSelectButton() { return mpButtonModeSelect; }
                void setSelectButtonTranslate(nw4r::math::VEC3 translate) { mpButtonModeSelect->SetTranslate(translate); }

                void setupForLanguage(Language lang, nw4r::math::VEC3& cnTranslate, nw4r::math::VEC3& krTranslate);

                GXTexObj& getTextureCnDirect() { return mTexDirect; }
                GXTexObj& getTextureCnPinyin() { return mTexCnPinyin; }
                GXTexObj& getTextureKrEnglish() { return mTexKrEng; }
                GXTexObj& getTextureKrHangul() { return mTexKrHan; }

            private:
                u32 unk_0x14;                          // 0x14
                gui::PaneComponent* mpPanePCs[5];      // 0x18
                gui::PaneComponent* mpBoundingPCs[5];  // 0x2C
                nw4rmanager::AnmPane* mpAnmPanes[5];   // 0x40
                nw4r::lyt::Pane* mpTextboxModeA;       // 0x54
                nw4r::lyt::Pane* mpTextboxModeB;       // 0x58
                nw4r::lyt::Pane* mpButtonModeSelect;   // 0x5C
                GXTexObj mTexDirect;                   // 0x60
                GXTexObj mTexCnPinyin;                 // 0x80
                GXTexObj mTexKrEng;                    // 0xA0
                GXTexObj mTexKrHan;                    // 0xC0
            };

            // TODO
            class LayoutByNW4R : public Base, public nw4rmanager::Layout, public UIObj::Listener {
            public:
                LayoutByNW4R(Manager* manager, nw4r::lyt::MultiArcResourceAccessor* resAccessor, const char* layoutBinName, EventObserver* event)
                    : Base(manager), nw4rmanager::Layout(resAccessor, layoutBinName, event), mpEventHandler(NULL), mShiftButton(1, this, this),
                      mCapsButton(0, this, this), mModePanel(2, this, this) {}

                virtual ~LayoutByNW4R() override;                                                                               // 0x008
                virtual void create(MEMAllocator* allocator) override;                                                          // 0x00C
                virtual void init() override;                                                                                   // 0x010
                virtual void updateFromReceiver(u32 cmd, void* data) override;                                                  // 0x01C
                virtual void onKey(u32 key, void* data) override;                                                               // 0x020
                virtual void setLanguage(Language language) override;                                                           // 0x028
                virtual void onActive() override;                                                                               // 0x034
                virtual void inputCharCode(wchar_t ch) override;                                                                // 0x038
                virtual void setABC(bool isAbc) override;                                                                       // 0x04C
                virtual void setTranslateMode(TranslateMode mode) override;                                                     // 0x05C
                virtual void onlyQwerty(bool) override;                                                                         // 0x060
                virtual void setLangKeyActive(bool) override;                                                                   // 0x064
                virtual void setInputModeJP(bool isAbc, u32 abcMode, u32 aiuMode) override;                                     // 0x068
                virtual void setInputModeCK(u32 abcMode) override { setInputModeJP(true, abcMode, 0); }                         // 0x06C
                virtual void refreshState() override;                                                                           // 0x070
                virtual void onClose() override;                                                                                // 0x07C
                virtual void sendInputWChar(wchar_t, bool) override;                                                            // 0x088
                virtual void goSignInputMode() override;                                                                        // 0x08C
                virtual void changePredictLanguage() override;                                                                  // 0x090
                virtual void draw() override;                                                                                   // 0x11C
                virtual void calc() override;                                                                                   // 0x120
                virtual void setPredictLanguageDialog(predictlang::LayoutByNW4R* dialog) { mpPredictLanguageDialog = dialog; }  // 0x124
                virtual void setSignWindow(signwindow::LayoutByNW4R* signWindow) { mpSignWindow = signWindow; }                 // 0x128
                virtual void setLineFeedButton(bool);                                                                           // 0x12C
                virtual void setPredictLanguageButton(bool);                                                                    // 0x130
                virtual void setSignWindowButton(bool);                                                                         // 0x134
                virtual bool updateInput(int chan, f32 x, f32 y, u32 trig, u32 hold, u32 release, void* data) override;         // 0x138
                virtual bool updateInput(input::HKBManager& hkbManager) override;                                               // 0x13C
                virtual void changeAnimationAllToNormal();                                                                      // 0x140
                virtual void throwReleaseForAll();                                                                              // 0x144
                virtual void cancelStateFocusIn();                                                                              // 0x148
                virtual void updatePredictLanguage(CommandReceiver::ChangePredictMode* cmdData);                                // 0x14C
                virtual void updateDakuten();                                                                                   // 0x150
                virtual void initLayout();                                                                                      // 0x154
                virtual void onEvent(UIObj*, u32, void*) override;                                                              // 0x158

                void onPressedShift(bool playSound);
                void onReleasedShift();

                void onPressedCaps();

                bool isLineFeedEnabled() { return mbLineFeedButtonEnabled; }

            protected:
                void createAnmPane_(MEMAllocator* allocator);

                EventHandler* mpEventHandler;                        // 0x0F0
                predictlang::LayoutByNW4R* mpPredictLanguageDialog;  // 0x0F4
                signwindow::LayoutByNW4R* mpSignWindow;              // 0x0F8
                bool mbLineFeedButtonEnabled;                        // 0x0FC
                UIModifierButton mShiftButton;                       // 0x100
                UIModifierButton mCapsButton;                        // 0x124
                UIModePanel mModePanel;                              // 0x148
            };

            class AnmPane : public nw4rmanager::AnmPane {
            public:
                AnmPane(nw4r::lyt::Pane* pane, nw4rmanager::AnmObserver* observer)
                    : nw4rmanager::AnmPane(pane, observer), meCurrAnimId(ANIM_NORMAL) {}

                virtual void init() override { meCurrAnimId = ANIM_NORMAL; }  // 0x08
                virtual void changeAnimation(u32 id) override {
                    meCurrAnimId = (AnimationId)id;
                    nw4rmanager::AnmPane::changeAnimation(id == ANIM_UNK_16 ? ANIM_FOCUS_OUT : id);
                }  // 0x14
                virtual ~AnmPane();  // 0x20

                virtual AnimationId getState() const { return meCurrAnimId; }
                virtual KeyType getKeyType() const { return meKeyType; }

            protected:
                AnimationId meCurrAnimId;  // 0x2C
                KeyType meKeyType;         // 0x30
            };
            class NormalButtonAnmPane : public AnmPane {
            public:
                NormalButtonAnmPane(nw4r::lyt::Pane* pane, nw4rmanager::AnmObserver* observer) : AnmPane(pane, observer) {
                    meKeyType = KEY_TYPE_NORMAL;
                }
                virtual void onAnmEvent(AnmPaneEvent paneEvent) override;  // 0x10
            };
            class ShiftCapsAnmPane : public AnmPane {
            public:
                ShiftCapsAnmPane(nw4r::lyt::Pane* pane, nw4rmanager::AnmObserver* observer) : AnmPane(pane, observer), mbIsOn(false) {
                    meKeyType = KEY_TYPE_SHIFTCAPS;
                }
                virtual void onAnmEvent(AnmPaneEvent paneEvent) override;  // 0x10

                bool isFocused() const;

            private:
                bool mbIsOn;  // 0x34 (Might be a u8, I doubt it)
            };
            class ToggleButtonAnmPane : public AnmPane {
            public:
                ToggleButtonAnmPane(nw4r::lyt::Pane* pane, nw4rmanager::AnmObserver* observer) : AnmPane(pane, observer) {
                    meKeyType = KEY_TYPE_TOGGLE;
                }
                virtual void onAnmEvent(AnmPaneEvent paneEvent) override;  // 0x10
            };
            class OnOffButtonAnmPane : public AnmPane {
            public:
                OnOffButtonAnmPane(nw4r::lyt::Pane* pane, nw4rmanager::AnmObserver* observer) : AnmPane(pane, observer) {
                    meKeyType = KEY_TYPE_ONOFF;
                }
                virtual void onAnmEvent(AnmPaneEvent paneEvent) override;  // 0x10
            };

            class Sample : public LayoutByNW4R {
            public:
                Sample(Manager* manager, nw4r::lyt::MultiArcResourceAccessor* resAccessor, const char* layoutBinName, EventObserver* event)
                    : LayoutByNW4R(manager, resAccessor, layoutBinName, event) {}
            };
        }  // namespace pctype
    }  // namespace keyboard
}  // namespace textinput

#endif  // TEXTINPUT_HW_KEYBOARD_H
