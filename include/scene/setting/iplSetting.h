#ifndef IPL_SCENE_SETTING_H
#define IPL_SCENE_SETTING_H

#include "layout/GUIManager.h"
#include "layout/iplGuiManager.h"
#include "system/iplNand.h"
#include "system/iplNandShared.h"
#include "utility/iplThread.h"

#include "scene/setting/iplAOSSThread.h"
#include "scene/setting/iplAPScanThread.h"
#include "scene/setting/iplRakuRakuThread.h"
#include "scene/setting/iplUSBAPThread.h"

#include "iplSceneHeader.h"

#include "system/iplController.h"
#include "system/iplKeyboard.h"

#include "iplwww/www_browser.h"
#include "iplwww/www_wiisetting.h"

#include <revolution/wd.h>

namespace ipl {
    namespace scene {

        typedef struct {
            u16 data[0x400];
        } BSSDescBuf ALIGN32;

        FADER_SCENE_CLASS(Setting) {
        public:
            Setting(EGG::Heap * heap, int startId);
            virtual ~Setting();

            // u16 getProfileID();
            // void setConnectTestResult(int, int, bool, int);
            // int getUpdateTiming();

            enum {
                ARG_NORMAL_PAGE = 0,
                ARG_INTERNET_SETTING,
                ARG_SETUP,
                ARG_UPDATE,
                ARG_INTERNET_PAGE,
                ARG_UNK_5,
                ARG_UNK_6,
                ARG_MAX,
            };

            virtual BOOL isResetAcceptable() const override;
            virtual void prepare() override;
            virtual void create() override;
            virtual void destroy() override;

            virtual void draw() override;
            virtual FaderSceneCommand calcFadein() override;
            virtual FaderSceneCommand calcNormal() override;
            virtual FaderSceneCommand calcFadeout() override;

            // protected:
            void initAP();
            void resetAP();
            void redrawAP();
            void scanAP();

            void initScroll();
            void updateScroll();

            void setAPDraw();

            int get_arw_no(const char* paneName);
            int get_ap_no(const char* paneName);
            void start_trig_event(const char* paneName);
            void start_point_event(const char* paneName);
            void start_left_event(const char* paneName);
            u8 getRadioLevel(const WDBssDesc* bss);

            u16 getProfileID();
            void setConnectTestResult(int, int, bool, int);
            int getUpdateTiming();
            void setInitializeResult(bool success, int errno);

            typedef enum {
                GROUP_LIST_UPDOWN = 0,  // 0x00
                GROUP_LIST_INOUT,       // 0x01
                GROUP_ARW_A,            // 0x02
                GROUP_ARW_B,            // 0x03
                GROUP_DENPA,            // 0x04
                GROUP_LOCK,             // 0x05
                GROUP_AP0,              // 0x06
                GROUP_AP1,              // 0x07
                GROUP_AP2,              // 0x08
                GROUP_AP3,              // 0x09
                GROUP_AP4,              // 0x0a
                GROUP_AP5,              // 0x0b
                GROUP_AP6,              // 0x0c
                GROUP_AP7,              // 0x0d
                GROUP_DENPA1,           // 0x0e
                GROUP_DENPA2,           // 0x0f
                GROUP_DENPA3,           // 0x10
                GROUP_DENPA4,           // 0x11
                GROUP_DENPA5,           // 0x12
                GROUP_DENPA6,           // 0x13
                GROUP_LOCK1,            // 0x14
                GROUP_LOCK2,            // 0x15
                GROUP_LOCK3,            // 0x16
                GROUP_LOCK4,            // 0x17
                GROUP_LOCK5,            // 0x18
                GROUP_LOCK6,            // 0x19

                GROUP_MAX = 0x1a,
            } GroupIndices;

            typedef enum {
                BRLAN_ARW_APPEAR = 0,  // 0x00
                BRLAN_ARW_LOST,        // 0x01
                BRLAN_ARW_FOCUS_ON,    // 0x02
                BRLAN_ARW_FOCUS_OFF,   // 0x03
                BRLAN_ARW_SELECT,      // 0x04
                BRLAN_SCROLL_UP,       // 0x05
                BRLAN_SCROLL_DOWN,     // 0x06
                BRLAN_BTN_FOCUS_ON,    // 0x07
                BRLAN_BTN_FOCUS_OFF,   // 0x08
                BRLAN_LIST_APPEAR,     // 0x09
                BRLAN_LIST_LOST,       // 0x0a
                BRLAN_DENPA0,          // 0x0b
                BRLAN_DENPA1,          // 0x0c
                BRLAN_DENPA2,          // 0x0d
                BRLAN_DENPA3,          // 0x0e
                BRLAN_LOCK_OFF,        // 0x0f
                BRLAN_LOCK_ON,         // 0x10

                BRLAN_MAX = 0x11,
            } BrlanPaths;

            typedef enum {
                ANIM_NONE = -1,

                ANIM_ARW_A_APPEAR = 0,  // 0x00
                ANIM_ARW_B_APPEAR,      // 0x01
                ANIM_ARW_A_LOST,        // 0x02
                ANIM_ARW_B_LOST,        // 0x03
                ANIM_ARW_A_FOCUS_ON,    // 0x04
                ANIM_ARW_B_FOCUS_ON,    // 0x05
                ANIM_ARW_A_FOCUS_OFF,   // 0x06
                ANIM_ARW_B_FOCUS_OFF,   // 0x07
                ANIM_ARW_A_SELECT,      // 0x08
                ANIM_ARW_B_SELECT,      // 0x09

                ANIM_LIST_SCROLL_UP,    // 0x0a
                ANIM_LIST_SCROLL_DOWN,  // 0x0b

                ANIM_FOCUS_ON_AP2,   // 0x0c
                ANIM_FOCUS_ON_AP3,   // 0x0d
                ANIM_FOCUS_ON_AP4,   // 0x0e
                ANIM_FOCUS_ON_AP5,   // 0x0f
                ANIM_FOCUS_OFF_AP2,  // 0x10
                ANIM_FOCUS_OFF_AP3,  // 0x11
                ANIM_FOCUS_OFF_AP4,  // 0x12
                ANIM_FOCUS_OFF_AP5,  // 0x13

                ANIM_LIST_APPEAR,  // 0x14
                ANIM_LIST_LOST,    // 0x15

                ANIM_DENPA_ANM0_GRP1,  // 0x16
                ANIM_DENPA_ANM0_GRP2,  // 0x17
                ANIM_DENPA_ANM0_GRP3,  // 0x18
                ANIM_DENPA_ANM0_GRP4,  // 0x19
                ANIM_DENPA_ANM0_GRP5,  // 0x1a
                ANIM_DENPA_ANM0_GRP6,  // 0x1b
                ANIM_DENPA_ANM1_GRP1,  // 0x1c
                ANIM_DENPA_ANM1_GRP2,  // 0x1d
                ANIM_DENPA_ANM1_GRP3,  // 0x1e
                ANIM_DENPA_ANM1_GRP4,  // 0x1f
                ANIM_DENPA_ANM1_GRP5,  // 0x20
                ANIM_DENPA_ANM1_GRP6,  // 0x21
                ANIM_DENPA_ANM2_GRP1,  // 0x22
                ANIM_DENPA_ANM2_GRP2,  // 0x23
                ANIM_DENPA_ANM2_GRP3,  // 0x24
                ANIM_DENPA_ANM2_GRP4,  // 0x25
                ANIM_DENPA_ANM2_GRP5,  // 0x26
                ANIM_DENPA_ANM2_GRP6,  // 0x27
                ANIM_DENPA_ANM3_GRP1,  // 0x28
                ANIM_DENPA_ANM3_GRP2,  // 0x29
                ANIM_DENPA_ANM3_GRP3,  // 0x2a
                ANIM_DENPA_ANM3_GRP4,  // 0x2b
                ANIM_DENPA_ANM3_GRP5,  // 0x2c
                ANIM_DENPA_ANM3_GRP6,  // 0x2d

                ANIM_LOCK1_OFF,  // 0x2e
                ANIM_LOCK2_OFF,  // 0x2f
                ANIM_LOCK3_OFF,  // 0x30
                ANIM_LOCK4_OFF,  // 0x31
                ANIM_LOCK5_OFF,  // 0x32
                ANIM_LOCK6_OFF,  // 0x33
                ANIM_LOCK1_ON,   // 0x34
                ANIM_LOCK2_ON,   // 0x35
                ANIM_LOCK3_ON,   // 0x36
                ANIM_LOCK4_ON,   // 0x37
                ANIM_LOCK5_ON,   // 0x38
                ANIM_LOCK6_ON,   // 0x39

                ANIM_MAX = 0x3a,  // 0x3a
            } ApAnimIdx;

        private:
            void updateController_();
            void changeVideoMode();
            bool isInitialSequenceExit(const controller::Interface*);
            bool updateScreenMode();

            void onTextInputOK();
            void calcKeyboard();
            void calcSetting();
            bool calcSafeMode();

            void getFuncMsgQ();
            void resetFuncMsgQ();

            bool isAnimating();
            void createBrowser();
            void initWiiSettingData();

            void setDisPos();
            void setNickName();
            void setSecurityKey();
            void setSSID();
            void setIP();
            void setDNS();
            void setProxy();
            void setBasic();
            void setMTU();
            void setParePass();
            void setPareRePass();
            void setPareJudgePass();
            void setSecA();
            void setReSecA();
            void setMasterKey();

            u32 checkTextNum(const char* str);
            bool checkSpace();

            void convertIP(char* str, const u8* ip);
            void convertRevIP(u8 * ip, const char* str);
            void adjustSecA(wchar_t * scratch);
            void reAdjustSecA();

            void initString();
            void initNickName();
            void initSecurityKey();
            void initSSID();
            void initIP();
            void initDNS();
            void initProxy();
            void initBasic();
            void initMTU();
            void initSecA();
            void initVersion();
            void initDirectUrl();

            void initHTMLText();
            void initMessage();
            void initKeyboard(const char*);

            void setDefaultBackString();
            bool checkInputString(const wchar_t*);
            bool checkIPString(const wchar_t*);

            void setSE();

            enum SetEulaState {
                SET_EULA_INIT = 0,
                SET_EULA_START = 1,
                SET_EULA_CANCEL = 2,
                SET_EULA_WAIT_FOR_SOUND_STOP = 3,
                SET_EULA_WAIT_STOP_MOTOR = 4,
                SET_EULA_DONE = 5,
            };

            void setUseEULA_();
            void setUseEULA_Init_();
            void setUseEULA_Cancel_();
            void setUseEULA_Start_();
            void setUseEULA_WaitStopMotor_();
            bool validateEULA_();

            enum SetUpdateState {
                SET_UPDATE_INIT = 0,
                SET_UPDATE_WAIT_ACCEPT_DIALOG = 1,
                SET_UPDATE_CONNECT_TEST_START = 2,
                SET_UPDATE_CONNECT_TEST_CREATE_WAIT = 3,
                SET_UPDATE_CONNECT_TEST_RUN = 4,
                SET_UPDATE_CONNECT_TEST_FAILED = 5,
                SET_UPDATE_SUCCESS_DIALOG = 6,
                SET_UPDATE_NO_UPDATE_DIALOG = 7,
                SET_UPDATE_EULA_INIT = 8,
                SET_UPDATE_UNK_9 = 9,
                SET_UPDATE_REBOOT_SYS = 10,
            };

            enum SetNUPState {
                SET_NUP_ERROR = 0,
            };

            void setUpdate_();
            void setUpdate_Init_();
            void setUpdate_WaitAcceptDialog_();
            void setUpdate_ConnectTestStart_();
            void setUpdate_ConnectTestCreateWait_();
            void setUpdate_ConnectTestRun_();
            void setUpdate_ConnectTestFailed_();
            void setUpdate_SuccessDialog_();
            void setUpdate_NoUpdateDialog_();
            void setUpdate_EULAInit_();
            void setUpdate_Reboot_();

            void setNUP();
            void makeErrorMessage();
            u32 getErrorNum();
            void makeSupportCode();

            void setUSBAP();
            void cancelUSBAP();

            void AOSSProcess();
            void RakuProcess();

            void waitStart();
            void waitFinish();
            bool isWaitPlaying();

            u32 mNUPState;                               // 0x058
            u8 unk_0x05C;                                // 0x05C
            nand::SharedFile* mpWwwlib;                  // 0x060
            nand::File* mpIplSetting;                    // 0x064
            nand::File* mpWwwArc;                        // 0x068
            nand::SharedFile* mpFontFile;                // 0x06C
            nand::File* mpBgTpl;                         // 0x070
            s32 mSceneState;                             // 0x074
            u32 mScanAPState;                            // 0x078
            SetEulaState mSetEulaState;                  // 0x07C
            SetUpdateState mSetUpdateState;              // 0x080
            u32 mUsbApState;                             // 0x084
            s32 unk_0x088;                               // 0x088
            s32 unk_0x08C;                               // 0x08C
            u32 mWiiSettingFlagMsgModified;              // 0x090
            u32 unk_0x094;                               // 0x094
            ESTitleId mTitleID;                          // 0x098
            u32 mStartTick;                              // 0x0a0
            SCOwnerNickname mOwnerNickname;              // 0x0a4
            nand::LayoutFile* mpLytFile;                 // 0x0bc
            layout::Object* mpLytSceenChange;            // 0x0c0
            layout::Object* mpLytMyAP;                   // 0x0c4
            layout::Object* mpLytWaiting;                // 0x0c8
            layout::Animator* mpLytSceenChangeR;         // 0x0cc
            layout::Animator* mpLytSceenChangeL;         // 0x0d0
            ::gui::EventHandler* mpApEvent;              // 0x0d4
            ipl::gui::PaneManager* mpGuiManager;         // 0x0d8
            USBAPThread* mpUsbApThread;                  // 0x0dc
            AOSSThread* mpAossThread;                    // 0x0e0
            RakuRakuThread* mpRakuRakuThread;            // 0x0e4
            APScanThread* mpAPScanThread;                // 0x0e8
            BSSDescBuf mApBssDescriptorsBuf;             // 0x100
            WDBssDesc* mpBssDescriptors;                 // 0x900
            void* mpAPScanThreadStack;                   // 0x904
            u32 unk_0x908;                               // 0x908
            u16* mpResultUSBAP;                          // 0x90c
            u8* mpBssDescriptorsBufUSBAP;                // 0x910
            s32 apRelated_0x914;                         // 0x914
            ApAnimIdx mApActiveAnim;                     // 0x918
            u8 unk_0x91C;                                // 0x91C
            u8 unk_0x91D;                                // 0x91D
            bool unk_0x91E;                              // 0x91E
            u8 unk_0x91F;                                // 0x91F
            ext_ead::www::ImeData* mpImeData;            // 0x920
            www::wiisetting::WiiData* mpWiiSettingData;  // 0x924
            www::wiisetting::WiiFlag* mpWiiSettingFlag;  // 0x928
            u8 mFrameCounter;                            // 0x92c
            u32 mSupportCode;                            // 0x930
            www::wiisetting::SetStringBuf* mpHtmlStr;    // 0x934
            char msHtmlStrScratch[0x202];                // 0x938
            u8 padding_0xB3A;                            // 0xB3A
            u8 mBrowserCreated;                          // 0xB3B
            keyboard::Manager::State mKbdMgrState;       // 0xB3C
            s32 mStartId;                                // 0xB4C
            BOOL mbAspectRatio;                          // 0xB50
            BOOL mbProgressiveMode;                      // 0xB54
            BOOL mbEuRGB60Mode;                          // 0xB58
            u8 unk_0xB5C;                                // 0xB5C
            OSMessageQueue mQueue;                       // 0xB60
            OSMessage mQueueBuf[5];                      // 0xB80
            s32 unk_0xB94;                               // 0xB94
            s32 mFadeFramesElapsed;                      // 0xB98
            s32 unk_0xB9C;                               // 0xB9C
            OSTime mFadeInStart;                         // 0xBA0
            u32 mWpadConnectedMask;                      // 0aBA8
            u8 unk_0xBAC;                                // 0xBAC

            static void* mem1Buffer_;
            static void* mem2Buffer_;
        };

        class APEvent : public ::gui::EventHandler {
        public:
            APEvent(Setting* s) : mpSetting(s) {}
            virtual void onEvent(u32 compId, u32 event, void* data) override;

        private:
            Setting* mpSetting;
        };
    }  // namespace scene
}  // namespace ipl

#endif  // IPL_SCENE_SETTING_H
