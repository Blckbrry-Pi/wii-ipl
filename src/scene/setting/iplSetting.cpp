#include "scene/setting/iplSetting.h"

#include "scene/setting/iplNCDSetting.h"
#include "scene/setting/iplParental.h"
#include "scene/setting/iplSensitivity.h"

#include "scene/nakamuratest/iplNakamuraTest.h"

#include "scene/parentalDialog/iplParentalDialog.h"

#include <private/os.h>
#include <private/vi.h>
#include <revolution/os.h>

#include <private/wpad/WPADInternal.h>

#include <cstring>
#include <stdlib.h>

#include "iplSystem.h"
#include "sound/iplSound.h"

#include "iplwww/www_surface.h"
#include "iplwww/www_trasition.h"
#include "iplwww/www_wiisetting.h"
#include "iplwww/www_window.h"

#include "utility/iplCharacterCode.h"
#include "utility/iplESMisc.h"
#include "utility/iplWpad.h"

#include <internal/mem_funcs.h>

// #include "runtime.h"
// todo: check <> vs "" imports

namespace ipl {
    namespace scene {
        NCDAossConfig m_AOSSConfig;
        NCDApConfig m_RakuConfig;

        void* Setting::mem1Buffer_;
        void* Setting::mem2Buffer_;
        static int queuedSceneChangeIdx;

        // clang-format off
        static const char* panes_B_AP[] = {"B_AP2", "B_AP3", "B_AP4", "B_AP5"};
        static const char* panes_B_Arw[] = {"B_ArwA", "B_ArwB"};
        static const char* panes_T_Name[] = {"T_Name1", "T_Name2", "T_Name3", "T_Name4", "T_Name5", "T_Name6"};
        static const char* panes_N_AP[] = {"N_AP1", "N_AP2", "N_AP3", "N_AP4", "N_AP5", "N_AP6"};
        static const char* groupNames[Setting::GROUP_MAX] = {
            "G_ListUpDown", "G_ListInOut",
            "G_ArwA", "G_ArwB",
            "G_Denpa",
            "G_Lock",
            "G_AP0", "G_AP1", "G_AP2", "G_AP3", "G_AP4", "G_AP5", "G_AP6", "G_AP7",
            "G_Denpa1", "G_Denpa2", "G_Denpa3", "G_Denpa4", "G_Denpa5", "G_Denpa6",
            "G_Lock1", "G_Lock2", "G_Lock3", "G_Lock4", "G_Lock5", "G_Lock6",
        };
        static const char* scAnimName[19] = {
            "my_AP_a_ArwAppear.brlan",
            "my_AP_a_ArwLost.brlan",
            "my_AP_a_ArwFocusOn.brlan",
            "my_AP_a_ArwFocusOff.brlan",
            "my_AP_a_ArwSelect.brlan",

            "my_AP_a_ScrollUp.brlan",
            "my_AP_a_ScrollDown.brlan",

            "my_AP_a_BtnFocusOn.brlan",
            "my_AP_a_BtnFocusOff.brlan",

            "my_AP_a_ListAppear.brlan",
            "my_AP_a_ListLost.brlan",

            "my_AP_a_Denpa0.brlan",
            "my_AP_a_Denpa1.brlan",
            "my_AP_a_Denpa2.brlan",
            "my_AP_a_Denpa3.brlan",

            "my_AP_a_LockOff.brlan",
            "my_AP_a_LockOn.brlan",
        };
        // clang-format on

        typedef struct {
            struct {
                u16 anm;
                u16 grp;
            } entries[Setting::ANIM_MAX];
        } SCAnmTable;

        typedef struct {
            const char* entries[Setting::ARG_MAX];
        } SCAnmStartPages;

        typedef struct {
            const char* entries[SC_PRODUCT_AREA_MAX];
        } SCAnmRegionArcPaths;

        typedef struct {
            const char* entries[SC_LANG_MAX];
        } SCAnmLangAbbreviations;

        typedef struct {
            const char* entries[SC_PRODUCT_AREA_MAX];
        } SCAnmInitialSetupPaths;

        typedef struct {
            const char* entries[Setting::ARG_MAX];
        } SCAnmHeadings;

        extern const SCAnmTable scAnmTable;

        inline bool dialogHasResult() {
            return System::getDialog()->getLastResult() >= DialogWindow::RESULT_WAIT;
        }
        // inline bool dialogRButton()

        Setting::Setting(EGG::Heap* heap, int startId) : scene::FaderSceneBase(heap) {
            unk_0x05C = 0;
            pWwwlib = NULL;
            pIplSetting = NULL;
            pWwwArc = NULL;
            pFontFile = NULL;
            pLytFile = NULL;
            mBrowserCreated = 0;

            setSceneParentFlags(SCN_PARENTFLAG_DRAW | SCN_PARENTFLAG_CALC);

            mSceneState = 0;
            mStartId = startId;
            mScanAPState = 1;

            mUsbApState = 1;
            mSetUpdateState = SET_UPDATE_INIT;

            mNUPState = 0;

            apRelated_0x914 = 0;
            mApActiveAnim = ANIM_NONE;
            unk_0x91C = 0;
            unk_0x91D = 0;
            unk_0x91E = false;
            unk_0x91F = 0;

            mSetEulaState = SET_EULA_INIT;
            unk_0xB9C = 0;

            mFrameCounter = 0;
            mSupportCode = 0;

            unk_0xB5C = 1;
            mWiiSettingFlagMsgModified = 0;
            mAspectRatio = SCGetAspectRatio();
            mProgressiveMode = SCGetProgressiveMode();
            mEuRGB60Mode = SCGetEuRgb60Mode();
            unk_0xB94 = 0;
            unk_0xBAC = 0;
            unk_0x094 = 0;
        }
        bool Setting::isAnimating() {
            return pLytSceenChangeL->isPlaying() || pLytSceenChangeR->isPlaying() || mFadeFramesElapsed != 0x14;
        }
        void Setting::getFuncMsgQ() {
            OSMessage msg = NULL;
            if (mWiiSettingFlagMsgModified == 0) {
                if (OSReceiveMessage(&mQueue, &msg, 0)) {
                    mWiiSettingFlagMsgModified = 1;
                    pWiiSettingFlag->smthMsgData = (u8)(u32)msg;
                }
            }
        }
        void Setting::resetFuncMsgQ() {
            pWiiSettingFlag->smthMsgData = 0;
            mWiiSettingFlagMsgModified = 0;
        }

        Setting::~Setting() {
            OSReport("***Destruct!!\n");

            // Destroy mem1 and mem2 buffers (for browser)
            if (mem1Buffer_ != NULL) {
                System::createMem1AppHeap()->free(mem1Buffer_);
                mem1Buffer_ = NULL;
            }
            if (mem2Buffer_ != NULL) {
                System::getMem2App()->free(mem2Buffer_);
                mem2Buffer_ = NULL;
            }

            // Clean up files
            if (pWwwlib)
                delete pWwwlib;
            if (pIplSetting)
                delete pIplSetting;
            if (pWwwArc)
                delete pWwwArc;
            if (pFontFile)
                delete pFontFile;
            if (pBgTpl)
                delete pBgTpl;

            // Reset system bs2 (to load updated settings data I assume)
            System::destroyMem1AppHeap();
            System::getBS2Manager()->restart();
            OSReport(" ... bs2 manager restarted\n");
        }

        void Setting::destroy() {
            delete pAossThread;
            delete pRakuRakuThread;
        }

        // static const char FONT_REGULAR[] = "WiiNTLG-Regular.ttc";
        // static const char FONT_CHINESE[] = ;
        // static const char FONT_KOREAN[] = ;
        void Setting::prepare() {
            System::getBS2Manager()->abort();
            while (System::getBS2Manager()->getIPLState() != 8) {
                System::getBS2Manager()->update();
                VIWaitForRetrace();
            }

            System::getUsbEtherMacAddr();
            mStartTick = OSGetTick();

            pWwwlib = System::getNandManager()->readSharedAsync(System::createMem1AppHeap(), "wwwlib-rvl.lz7", 2);

            char archivePath[0x40];
            char fontName[0x20];
            switch (SCGetProductArea()) {
                case SC_PRODUCT_AREA_JPN:
                case SC_PRODUCT_AREA_USA:
                case SC_PRODUCT_AREA_EUR:
                case SC_PRODUCT_AREA_AUS:
                    snprintf(fontName, sizeof(fontName), "WiiNTLG-Regular.ttc");
                    snprintf(archivePath, sizeof(archivePath), "/html/%s/iplsetting.ash", "US2");
                    break;
                case SC_PRODUCT_AREA_KOR:
                    snprintf(fontName, sizeof(fontName), "Wii-kr_Round Gothic B.ttf");
                    snprintf(archivePath, sizeof(archivePath), "/html/%s/iplsetting.ash", "US2");
                    break;
                case SC_PRODUCT_AREA_CHN:
                    snprintf(fontName, sizeof(fontName), "Wii-cn_HeiTiW5.ttf");
                    snprintf(archivePath, sizeof(archivePath), "/html/%s/iplsetting.ash", "US2");
                    break;
                case SC_PRODUCT_AREA_TWN:
                    snprintf(fontName, sizeof(fontName), "WiiNTLG-Regular.ttc");
                    snprintf(archivePath, sizeof(archivePath), "/html/%s/iplsetting.ash", "TW2");
                    break;
                default:
                    snprintf(fontName, sizeof(fontName), "WiiNTLG-Regular.ttc");
                    snprintf(archivePath, sizeof(archivePath), "/html/%s/iplsetting.ash", "US2");
                    break;
            }

            pIplSetting = System::getNandManager()->readAsync(System::createMem1AppHeap(), archivePath, 0);

            pWwwArc = System::getNandManager()->readAsync(System::getMem2App(), "/www.arc", 0);
            pFontFile = System::getNandManager()->readSharedAsync(System::getMem2App(), fontName, 3);
            pBgTpl = System::getNandManager()->readAsync(System::getMem2App(), "/html/BG_16x9.tpl", 0);
            pLytFile = System::getNandManager()->readLayoutAsync(getSceneHeap(), "setting.ash", 0);

            if (mStartId == ARG_SETUP) {
                memset(&mOwnerNickname, 0, sizeof(mOwnerNickname));
                mAspectRatio = false;
                www::wiisetting::setInitSetupFlag(1);
            } else if (mStartId == ARG_UNK_5) {
                www::wiisetting::setInitSetupFlag(1);
            } else {
                www::wiisetting::setInitSetupFlag(0);
            }
        }
        void Setting::create() {
            nand::File* arcTempFile =
                System::getNandManager()->write(getSceneHeap(), "/tmp/www.arc", pWwwArc->getBuffer(), pWwwArc->getLength(), 0b110000);

            if (arcTempFile->isFullForTask()) {
                System::getErrorHandler()->log("NAND", 0, "iplSetting.cpp", 0x178);
                // "The system files are corrupted."
                System::getErrorHandler()->set(ErrorHandler::DEFAULT, 0x02);
            }
            delete arcTempFile;

            pLytSceenChange = new layout::Object(getSceneHeap(), pLytFile, "arc", "SceenChange_b.brlyt");
            pLytSceenChangeR = pLytSceenChange->bind("SceenChange_b_Right.brlan", true);
            pLytSceenChangeL = pLytSceenChange->bind("SceenChange_b_Left.brlan", true);
            pLytSceenChange->finishBinding();

            pLytMyAP = new layout::Object(getSceneHeap(), pLytFile, "arc", "my_AP_a.brlyt");
            for (int i = 0; i < (int)ARRAY_LENGTH(scAnmTable.entries); i++) {
                bool unused = i == 0x14 || i == 0x0a;
                pLytMyAP->bindToGroup(scAnimName[scAnmTable.entries[i].anm], groupNames[scAnmTable.entries[i].grp], false, unused);
            }
            pLytMyAP->finishBinding();

            pLytWaiting = new layout::Object(getSceneHeap(), pLytFile, "arc", "it_Waiting_a.brlyt");
            pLytWaiting->bindToGroup("it_Waiting_a_Wait.brlan", "G_Wait", false, false);
            pLytWaiting->finishBinding();
            pLytWaiting->GetRootPane()->FindPaneByName("N_Wait")->SetVisible(false);  // The SetVisible call is inlined (it shouldn't be)

            pImeData = new ext_ead::www::ImeData();

            pAPScanThread = new APScanThread();
            pUsbApThread = new USBAPThread();

            ncd::NCDSetting::init();
            parental::Parental::init();

            pAossThread = new AOSSThread(getSceneHeap());
            unk_0x088 = 0;

            pRakuRakuThread = new RakuRakuThread(getSceneHeap());
            unk_0x08C = 0;

            www::wiisetting::initWiiSetting();
            initWiiSettingData();

            initString();
            www::wiisetting::setStringBuf(pHtmlStr);

            OSInitMessageQueue(&mQueue, mQueueBuf, ARRAY_LENGTH(mQueueBuf));
            www::wiisetting::setMsgQueue(&mQueue);

            pAPScanThreadStack = getSceneHeap()->alloc(0x1000, 0x20);

            pResultUSBAP = (u16*)getSceneHeap()->alloc(0x800, 0x4);
            memset(pResultUSBAP, 0, 0x800);

            pUSBApBssDescriptorsBuf = (u8*)getSceneHeap()->alloc(0x79, 0x4);
            memset(pUSBApBssDescriptorsBuf, 0, 0x79);

            pApEvent = new APEvent(this);
            pGuiManager = new ipl::gui::PaneManager(pApEvent, pLytMyAP->getDrawInfo(), NULL, NULL, true);

            pGuiManager->createLayoutScene(*pLytMyAP->getNW4RLyt());
            pGuiManager->setAllComponentTriggerTarget(false);

            for (int i = 0; i < (int)ARRAY_LENGTH(panes_B_AP); i++) {
                pGuiManager->setTriggerTarget(pLytMyAP->FindPaneByName(panes_B_AP[i]), true);
            }
            for (int i = 0; i < (int)ARRAY_LENGTH(panes_B_Arw); i++) {
                pGuiManager->setTriggerTarget(pLytMyAP->FindPaneByName(panes_B_Arw[i]), true);
            }
            TPLBind((TPLPalette*)pBgTpl->getBuffer());

            OSReport("*** prepare costs: %dms\n", OSTicksToMilliseconds(OSGetTick() - mStartTick));
            mStartTick = OSGetTick();
        }

        typedef struct {
            const char* localPathFmt;
            const char* urlPathFmt;
        } WWWBrowserPathMapper;
        char ARC_PATH_FMT[] = "marc:%s/%s/";
        char FILE_DVD_PATH_FMT[] = "file:dvd/html/IPLSetting/%s/%s/";

        const SCAnmTable scAnmTable = {{
            {Setting::BRLAN_ARW_APPEAR, Setting::GROUP_ARW_A},       {Setting::BRLAN_ARW_APPEAR, Setting::GROUP_ARW_B},
            {Setting::BRLAN_ARW_LOST, Setting::GROUP_ARW_A},         {Setting::BRLAN_ARW_LOST, Setting::GROUP_ARW_B},
            {Setting::BRLAN_ARW_FOCUS_ON, Setting::GROUP_ARW_A},     {Setting::BRLAN_ARW_FOCUS_ON, Setting::GROUP_ARW_B},
            {Setting::BRLAN_ARW_FOCUS_OFF, Setting::GROUP_ARW_A},    {Setting::BRLAN_ARW_FOCUS_OFF, Setting::GROUP_ARW_B},
            {Setting::BRLAN_ARW_SELECT, Setting::GROUP_ARW_A},       {Setting::BRLAN_ARW_SELECT, Setting::GROUP_ARW_B},

            {Setting::BRLAN_SCROLL_UP, Setting::GROUP_LIST_UPDOWN},  {Setting::BRLAN_SCROLL_DOWN, Setting::GROUP_LIST_UPDOWN},

            {Setting::BRLAN_BTN_FOCUS_ON, Setting::GROUP_AP2},       {Setting::BRLAN_BTN_FOCUS_ON, Setting::GROUP_AP3},
            {Setting::BRLAN_BTN_FOCUS_ON, Setting::GROUP_AP4},       {Setting::BRLAN_BTN_FOCUS_ON, Setting::GROUP_AP5},
            {Setting::BRLAN_BTN_FOCUS_OFF, Setting::GROUP_AP2},      {Setting::BRLAN_BTN_FOCUS_OFF, Setting::GROUP_AP3},
            {Setting::BRLAN_BTN_FOCUS_OFF, Setting::GROUP_AP4},      {Setting::BRLAN_BTN_FOCUS_OFF, Setting::GROUP_AP5},

            {Setting::BRLAN_LIST_APPEAR, Setting::GROUP_LIST_INOUT}, {Setting::BRLAN_LIST_LOST, Setting::GROUP_LIST_INOUT},

            {Setting::BRLAN_DENPA0, Setting::GROUP_DENPA1},          {Setting::BRLAN_DENPA0, Setting::GROUP_DENPA2},
            {Setting::BRLAN_DENPA0, Setting::GROUP_DENPA3},          {Setting::BRLAN_DENPA0, Setting::GROUP_DENPA4},
            {Setting::BRLAN_DENPA0, Setting::GROUP_DENPA5},          {Setting::BRLAN_DENPA0, Setting::GROUP_DENPA6},
            {Setting::BRLAN_DENPA1, Setting::GROUP_DENPA1},          {Setting::BRLAN_DENPA1, Setting::GROUP_DENPA2},
            {Setting::BRLAN_DENPA1, Setting::GROUP_DENPA3},          {Setting::BRLAN_DENPA1, Setting::GROUP_DENPA4},
            {Setting::BRLAN_DENPA1, Setting::GROUP_DENPA5},          {Setting::BRLAN_DENPA1, Setting::GROUP_DENPA6},
            {Setting::BRLAN_DENPA2, Setting::GROUP_DENPA1},          {Setting::BRLAN_DENPA2, Setting::GROUP_DENPA2},
            {Setting::BRLAN_DENPA2, Setting::GROUP_DENPA3},          {Setting::BRLAN_DENPA2, Setting::GROUP_DENPA4},
            {Setting::BRLAN_DENPA2, Setting::GROUP_DENPA5},          {Setting::BRLAN_DENPA2, Setting::GROUP_DENPA6},
            {Setting::BRLAN_DENPA3, Setting::GROUP_DENPA1},          {Setting::BRLAN_DENPA3, Setting::GROUP_DENPA2},
            {Setting::BRLAN_DENPA3, Setting::GROUP_DENPA3},          {Setting::BRLAN_DENPA3, Setting::GROUP_DENPA4},
            {Setting::BRLAN_DENPA3, Setting::GROUP_DENPA5},          {Setting::BRLAN_DENPA3, Setting::GROUP_DENPA6},

            {Setting::BRLAN_LOCK_OFF, Setting::GROUP_LOCK1},         {Setting::BRLAN_LOCK_OFF, Setting::GROUP_LOCK2},
            {Setting::BRLAN_LOCK_OFF, Setting::GROUP_LOCK3},         {Setting::BRLAN_LOCK_OFF, Setting::GROUP_LOCK4},
            {Setting::BRLAN_LOCK_OFF, Setting::GROUP_LOCK5},         {Setting::BRLAN_LOCK_OFF, Setting::GROUP_LOCK6},
            {Setting::BRLAN_LOCK_ON, Setting::GROUP_LOCK1},          {Setting::BRLAN_LOCK_ON, Setting::GROUP_LOCK2},
            {Setting::BRLAN_LOCK_ON, Setting::GROUP_LOCK3},          {Setting::BRLAN_LOCK_ON, Setting::GROUP_LOCK4},
            {Setting::BRLAN_LOCK_ON, Setting::GROUP_LOCK5},          {Setting::BRLAN_LOCK_ON, Setting::GROUP_LOCK6},
        }};
        const SCAnmStartPages scAnmStartPages = {{
            "index01.html",
            "Internet/Internet_index.html",
            "Setup/startup_index1.html",
            "Update/Update_index.html",
            "index02.html",
            "%s",
            "%s",
        }};
        const SCAnmRegionArcPaths scAnmRegionArcPaths = {{
            "JP/JP",
            "FIX/US",
            "EU/EU",
            "",
            "",
            "TW/TW",
            "KR/KR",
            "",
            "",
            "",
            "",
            "CN/CN",
        }};
        const SCAnmLangAbbreviations scAnmLangAbbreviations = {{
            "JPN",
            "ENG",
            "GER",
            "FRA",
            "SPA",
            "ITA",
            "DUT",
            "CHN",
            "ENG",
            "KOR",
        }};
        const SCAnmInitialSetupPaths scAnmInitialSetupPaths = {{
            "Setup/ScreenSave.html",
            "Country/US_Country_flame.html",
            "Setup/ScreenSave.html",
            "Setup/ScreenSave.html",
            "Setup/ScreenSave.html",
            "Setup/ScreenSave.html",
            "Setup/ScreenSave.html",
            "Setup/ScreenSave.html",
            "Setup/ScreenSave.html",
            "Setup/ScreenSave.html",
            "Setup/ScreenSave.html",
            "Setup/ScreenSave.html",
        }};
        const SCAnmHeadings scAnmHeadings = {{
            "Calendar",
            "Display",
            "Sound",
            "Parental_Control",
            "Internet",
            "Wiiconnect24",
            "Update",

        }};
        void Setting::createBrowser() {
            const SCAnmTable& anmTable = scAnmTable;
            u32 mem1BufSize, mem2BufSize;
            int rectW, rectH;

            WWWBrowserPathMapper pathMapper;

            SCAnmStartPages startPaths;
            SCAnmRegionArcPaths regionArcPaths;
            SCAnmLangAbbreviations langAbbrevs;
            SCAnmInitialSetupPaths setupPaths;
            SCAnmHeadings headings;
            char arcPathFmt[100];
            char arcPath[100];

            mem1BufSize = System::createMem1AppHeap()->getAllocatableSize();
            mem2BufSize = System::getMem2App()->getAllocatableSize() - 0x80000 - (13 / 5.0 * 1024 * 1024);

            mem1Buffer_ = System::createMem1AppHeap()->alloc(mem1BufSize, 4);
            mem2Buffer_ = System::getMem2App()->alloc(mem2BufSize, 4);

            OSReport("Setting Scene: mem1: %d  mem2: %d\n", mem1BufSize, mem2BufSize);

            nw4r::ut::Rect proj16x9;
            nw4r::ut::Rect proj4x3;
            System::getProjectionRect16x9(&proj16x9);
            System::getProjectionRect4x3(&proj4x3);

            rectW = proj4x3.right - proj4x3.left;
            rectH = proj4x3.bottom - proj4x3.top;

            pathMapper = (WWWBrowserPathMapper){ARC_PATH_FMT, FILE_DVD_PATH_FMT};

            startPaths = scAnmStartPages;
            regionArcPaths = scAnmRegionArcPaths;
            langAbbrevs = scAnmLangAbbreviations;
            setupPaths = scAnmInitialSetupPaths;

            memset(arcPathFmt, 0, sizeof(arcPathFmt));
            memset(arcPath, 0, sizeof(arcPath));
            strcpy(arcPathFmt, pathMapper.localPathFmt);
            strcat(arcPathFmt, startPaths.entries[mStartId]);

            int productArea = SCGetProductArea();
            switch (productArea) {
                case SC_PRODUCT_AREA_JPN:
                case SC_PRODUCT_AREA_USA:
                case SC_PRODUCT_AREA_EUR:
                    break;

                case SC_PRODUCT_AREA_AUS:
                    productArea = SC_PRODUCT_AREA_EUR;
                    break;

                case SC_PRODUCT_AREA_TWN:
                case SC_PRODUCT_AREA_KOR:
                case SC_PRODUCT_AREA_CHN:
                    break;

                default:
                    productArea = SC_PRODUCT_AREA_USA;
                    break;
            }

            if (mStartId == 5) {
                snprintf(arcPath, sizeof(arcPath), arcPathFmt, regionArcPaths.entries[productArea], langAbbrevs.entries[System::getLanguage()],
                         setupPaths.entries[productArea]);
            } else if (mStartId == 6) {
                headings = scAnmHeadings;
                int i;
                for (i = 0; i < 7; i++) {
                    if (strstr(pHtmlStr->netSettingArg, headings.entries[i]) != NULL)
                        break;
                }

                const char* baseHtmlPath;
                switch (i) {
                    case 0:
                    case 1:
                    case 2:
                        baseHtmlPath = "index01.html";
                        break;

                    case 3:
                    case 4:
                    case 5:
                        baseHtmlPath = "index02.html";
                        break;

                    case 6:
                        baseHtmlPath = "index03.html";
                        break;

                    case 7:
                        baseHtmlPath = "index01.html";
                        break;
                }
                snprintf(arcPath, sizeof(arcPath), arcPathFmt, regionArcPaths.entries[productArea], langAbbrevs.entries[System::getLanguage()],
                         baseHtmlPath);
            } else {
                snprintf(arcPath, sizeof(arcPath), arcPathFmt, regionArcPaths.entries[productArea], langAbbrevs.entries[System::getLanguage()]);
            }

            OSReport("***********************************\n");
            OSReport("%s\n", arcPath);
            OSReport("***********************************\n");
            ICInvalidateRange(pWwwlib->getBuffer(), pWwwlib->getLength());
            OSReport(" RSO PLACED : %p %d\n", pWwwlib->getBuffer(), pWwwlib->getLength());

            ext_ead::www::SurfaceManager::CreateManager(rectW, rectH, rectW, rectH, mem1Buffer_, mem1BufSize, mem2Buffer_, mem2BufSize,
                                                        pWwwlib->getBuffer(), arcPath);
            ext_ead::www::SurfaceManager::RegisterArcFile(pIplSetting->getBuffer());
            ext_ead::www::SurfaceManager::RegisterIniFile(pWwwArc->getBuffer(), pWwwArc->getLength());
            ext_ead::www::SurfaceManager::RegisterFontFile(0, pFontFile->getBuffer(), pFontFile->getLength());
            ext_ead::www::SurfaceManager::StartThread();
        }

        void Setting::initDirectUrl() {
            if (mStartId != ARG_UNK_6)
                return;
            if (System::getNetSettingArg() == NULL)
                return;
            // @bug This should be a copy of 0x40, not of 0x80
            memcpy(pHtmlStr->netSettingArg, System::getNetSettingArg(), 0x80);
        }
        void Setting::initString() {
            pHtmlStr = (www::wiisetting::SetStringBuf*)getSceneHeap()->alloc(sizeof(www::wiisetting::SetStringBuf), 4);
            OSReport("HTML String Alloc Size:%d\n", sizeof(www::wiisetting::SetStringBuf));
            memset(pHtmlStr, 0, sizeof(www::wiisetting::SetStringBuf));
            memset(mHtmlStrScratch, 0, sizeof(mHtmlStrScratch));

            initNickName();
            initSecurityKey();
            initSSID();
            initIP();
            initDNS();
            initProxy();
            initBasic();
            initMTU();
            initSecA();
            initVersion();
            initDirectUrl();
        }
        FaderSceneCommand Setting::calcFadein() {
            FaderSceneCommand cmd;

            if (System::hasCreatedAfter() && !mBrowserCreated) {
                mBrowserCreated = 1;
                createBrowser();
                OSReport("............browser created\n");
            } else if (!mBrowserCreated) {
                OSReport("wait first init\n");
                return FADER_SCN_CONTINUE;
            }
            if ((ext_ead::www::SurfaceManager::GetInstance()->GetBrowserThread() == NULL) ||
                !ext_ead::www::SurfaceManager::GetInstance()->GetBrowserThread()->GetTextureBuffer(0, NULL) || !System::hasCreatedAfter())
                cmd = FADER_SCN_CONTINUE;
            else {
                mKbdMgrState = *System::getKeyboard()->getState();
                System::getFader()->fadeIn();
                mFadeInStart = OSGetTime();

                OSReport("*** create page costs: %dms\n", OSTicksToMilliseconds(OSGetTick() - mStartTick));
                return FADER_SCN_NEXT;
            }
            return FADER_SCN_CONTINUE;
        }

        void Setting::updateController_() {
            nw4r::ut::Rect projRect;
            ext_ead::www::BrowserThread::CmdPacket cmd;

            System::getProjectionRect4x3(&projRect);
            if (isAnimating())
                return;

            controller::Interface* controller = System::getYoungController();
            if (controller != NULL) {
                if (((int)unk_0xB9C < 10) || (pHtmlStr->netSettingArg[0] != '\0')) {
                    cmd.data.controller.irX = -1000.0;
                    cmd.data.controller.irY = -1000.0;
                    cmd.data.controller.btnHold = 0;
                    cmd.data.controller.btnTrigger = 0;
                    cmd.data.controller.btnRelease = 0;
                } else {
                    cmd.data.controller.irX = controller->getDpdProjectionPos().x - projRect.left;
                    cmd.data.controller.irY = controller->getDpdProjectionPos().y - projRect.top;

                    u32 claBtnHold = controller->getClassicHoldFlag() << 0x10;
                    u32 claBtnTrigger = controller->getClassicTrigFlag() << 0x10;
                    u32 claBtnRelease = controller->getClassicReleaseFlag() << 0x10;
                    cmd.data.controller.btnHold = claBtnHold | controller->getHoldFlag();
                    cmd.data.controller.btnTrigger = claBtnTrigger | controller->getTrigFlag();
                    cmd.data.controller.btnRelease = claBtnRelease | controller->getReleaseFlag();
                }
                cmd.type = 0;
                if ((mKbdMgrState.type == textinput::MemoManager::ST_Hidden) && (mSceneState == 0))
                    ext_ead::www::SurfaceManager::GetInstance()->GetBrowserThread()->SendUIEvent(&cmd);
            } else {
                cmd.type = 0;
                cmd.data.controller.irX = -1000.0f;
                cmd.data.controller.irY = -1000.0f;
                cmd.data.controller.btnHold = 0;
                cmd.data.controller.btnTrigger = 0;
                cmd.data.controller.btnRelease = 0;
                ext_ead::www::SurfaceManager::GetInstance()->GetBrowserThread()->SendUIEvent(&cmd);
            }
        }

        void Setting::changeVideoMode() {
            setSE();
            VISetBlack(TRUE);
            VIFlush();
            VIWaitForRetrace();
            System::resetFrameworkRenderMode();

            // Wait ~100 frames
            u32 blackoutStart = OSGetTick();
            while (OSTicksToMilliseconds(OSGetTick() - blackoutStart) < 1660) {
                VIWaitForRetrace();
            }

            mAspectRatio = SCGetAspectRatio() & 0xff;
            mProgressiveMode = SCGetProgressiveMode();
            mEuRGB60Mode = SCGetEuRgb60Mode();
            unk_0xB94 = 0;
            VISetBlack(FALSE);
            VIFlush();
            VIWaitForRetrace();
            if ((u32)System::getRegion() == SC_PRODUCT_AREA_EUR) {
                if (mEuRGB60Mode == FALSE) {
                    *(u32*)OSPhysicalToCached(OS_ADDR_TV_VIDEO_FORMAT) = VI_PAL;
                } else {
                    *(u32*)OSPhysicalToCached(OS_ADDR_TV_VIDEO_FORMAT) = VI_EURGB60;
                }
            }

            while (pLytSceenChangeL->isPlaying() || pLytSceenChangeR->isPlaying()) {
                pLytSceenChange->calc();
            }
            mFadeFramesElapsed = 0x14;
            unk_0xB9C = 10;
            return;
        }

        bool Setting::isInitialSequenceExit(const controller::Interface* interface) {
            u32 connMask = utility::wpad::getWpadConnectedMask();

            if (interface->downTrg(controller::BTN_INTERACT) ||
                (OSTicksToMilliseconds(OSGetTime() - mFadeInStart) >= 500 && connMask != mWpadConnectedMask &&
                 utility::wpad::isIncreaseConnectedWpad(mWpadConnectedMask, connMask))) {
                return true;
            } else {
                if (connMask != mWpadConnectedMask)
                    mWpadConnectedMask = connMask;
                return false;
            }
        }
        bool Setting::updateScreenMode() {
            if (unk_0x91E) {
                // W H Y
                switch (unk_0xB94) {
                    case 0:
                        if (mEuRGB60Mode != SCGetEuRgb60Mode()) {
                            unk_0xB94 = 3;
                        } else if (mProgressiveMode != SCGetProgressiveMode()) {
                            unk_0xB94 = 2;
                        }
                        if ((unk_0xB94 != 0) && (unk_0xB94 != 1)) {
                            System::getFader()->fadeOut();
                            mFadeFramesElapsed = 0;
                            return true;
                        }
                }
            } else if (unk_0xB94 == 1 && mSceneState == 8) {
                System::getFader()->fadeOut();
                mFadeFramesElapsed = 0;
                mFrameCounter = 0;
                mSceneState = 0;
                return true;
            }
            switch (unk_0xB94) {
                case 1:
                    if (System::getFader()->getStatus() == 0) {
                        System::getDialog()->terminate();
                        SCSetAspectRatio(mAspectRatio);
                        SCFlush();
                        if (dialogHasResult()) {
                            changeVideoMode();
                            System::getKeyboard()->changeAspectRatio();
                            System::getFader()->fadeIn();
                        } else {
                            mFadeFramesElapsed = 0;
                            return true;
                        }
                    } else {
                        mFadeFramesElapsed = 0;
                        return true;
                    }
                    break;
                case 2:
                case 3:
                    if (System::getFader()->getStatus() == 0) {
                        changeVideoMode();
                        System::getFader()->fadeIn();
                    } else {
                        mFadeFramesElapsed = 0;
                        return true;
                    }
                    break;
            }
            return false;
        }

        FaderSceneCommand Setting::calcNormal() {
            if (unk_0x05C == 0) {
                getFuncMsgQ();
                controller::Interface* controller = System::getYoungController();
                if (0x14 < ++mFadeFramesElapsed) {
                    mFadeFramesElapsed = 0x14;
                }
                if (0 < unk_0xB9C) {
                    unk_0xB9C++;
                }
                if (unk_0xB9C > 10) {
                    unk_0xB9C = 10;
                }
                if (updateScreenMode()) {
                    return FADER_SCN_CONTINUE;
                }
                if (controller != NULL) {
                    if (controller->downTrg(controller::BTN_INTERACT) && (u32)pWiiSettingData->data[www::wiisetting::WB_ID_PAGE_ID] == 0x1e &&
                        mFrameCounter >= 30 && mFrameCounter < 60 * 2) {
                        mFrameCounter = 60 * 2;
                        www::wiisetting::setFuncResult(1);
                        pWiiSettingData->data[www::wiisetting::WB_ID_SE] = 1;
                        setSE();
                        unk_0xB9C = 0;
                    }
                    if (isInitialSequenceExit(controller) && (u32)pWiiSettingData->data[www::wiisetting::WB_ID_PAGE_ID] == 0x1f) {
                        snd::getSystem()->startSE("WIPL_SE_DECIDE");
                        www::wiisetting::setFuncResult(1);
                        pWiiSettingData->data[www::wiisetting::WB_ID_PAGE_ID] = 0;
                    }
                }
                if (((unk_0xBAC == '\0') && (unk_0xB9C == 10)) && (pHtmlStr->netSettingArg[0] != '\0')) {
                    www::wiisetting::setFuncResult(1);
                    unk_0xBAC = 1;
                }
                if (System::getFader()->getStatus() == 1) {
                    updateController_();
                }
                if (ext_ead::www::SurfaceManager::GetInstance()->GetBrowserThread()->ReceiveWindowEvent(pImeData) != 0) {
                    if (pImeData->unk_0x00 == 0) {
                        OSReport("IME Created ");
                        if (pImeData->text != NULL) {
                            OSReport("initKeyboard %s\n", pImeData->text);
                            OSReport("initKeyboard %d\n", pWiiSettingData->data[www::wiisetting::WB_ID_FORM_ID]);
                            if (pWiiSettingData->data[www::wiisetting::WB_ID_FORM_ID] != 0) {
                                mSceneState = 1;
                                initKeyboard(pImeData->text);
                            } else {
                                ext_ead::www::SurfaceManager::GetInstance()->GetBrowserThread()->CommitIme(pImeData, pImeData->text);
                                ext_ead::www::SurfaceManager::GetInstance()->GetBrowserThread()->DisposeImeData(pImeData);
                            }
                        } else {
                            OSReport("NULL ptr\n");
                        }
                    } else {
                        OSReport("Other Event\n");
                    }
                }
            }
            switch (mSceneState) {
                case 0x02:
                    if (dialogHasResult()) {
                        unk_0xB9C = 1;
                        mSceneState = 0;
                        unk_0xB5C = 1;
                    }
                    break;
                case 0x03:
                    if (System::getDialog()->getLastResult() == 1) {
                        if (unk_0xB9C == 0) {
                            www::wiisetting::setFuncResult(1);
                        } else {
                            if (www::wiisetting::getFuncResult() == 0) {
                                mSceneState = 0;
                            }
                        }
                        unk_0xB9C = 1;
                    } else if (System::getDialog()->getLastResult() == 2) {
                        if (unk_0xB9C == 0) {
                            www::wiisetting::setFuncResult(2);
                        } else {
                            if (www::wiisetting::getFuncResult() == 0) {
                                mSceneState = 0;
                            }
                        }
                        unk_0xB9C = 1;
                    }
                    break;
                case 0x04:
                    mFrameCounter = 1;
                    if (System::getHomeButtonMenu()->disable() != 0 && dialogHasResult()) {
                        mSceneState = 0;
                    }
                    break;
                case 0x05: {
                    ParentalDialog* parentalDialog = (ParentalDialog*)System::getSceneManager()->getScene(SCENE_PARENTAL_DIALOG);
                    if (parentalDialog != NULL) {
                        unk_0x91F = 1;
                        switch (parentalDialog->getResult()) {
                            case ParentalDialog::RESULT_SUCCESS:
                                mSceneState = 6;
                                www::wiisetting::setFuncResult(1);
                                break;
                            case ParentalDialog::RESULT_OVER_ATTEMPTS:
                                www::wiisetting::setFuncResult(2);
                                break;
                            case ParentalDialog::RESULT_CANCELLED:
                                www::wiisetting::setFuncResult(2);
                                break;
                            default:
                                break;
                        }
                    } else {
                        if (unk_0x91F == 1) {
                            mSceneState = 0;
                            unk_0x91F = 0;
                            resetFuncMsgQ();
                            if (pWiiSettingFlag->smthMsgData != 0x4f) {
                                unk_0xB9C = 1;
                            }
                        }
                    }
                    break;
                }
                case 0x06:
                    if (System::getSceneManager()->getScene(SCENE_PARENTAL_DIALOG) == NULL) {
                        unk_0x91F = 0;
                        if (pWiiSettingFlag->smthMsgData == 0x4f) {
                            if (ncd::NCDSetting::getEnableFlag()) {
                                if (SCGetEULA()) {
                                    www::wiisetting::setFuncResult(6);
                                    mSceneState = 0;
                                    resetFuncMsgQ();
                                } else {
                                    if ((u32)System::getRegion() != SC_PRODUCT_AREA_EUR) {
                                        System::getDialog()->callBtn2(MESG_SETTINGS_PLEASE_REVIEW_EULA, MESG_CMN_OK, MESG_CMN_QUIT);
                                    } else {
                                        System::getDialog()->callBtn2(MESG_SETTINGS_PLEASE_REVIEW_EULA_EUR, MESG_CMN_OK, MESG_CMN_QUIT);
                                    }
                                    mSceneState = 0x0e;
                                }
                            } else {
                                if ((u32)System::getRegion() != SC_PRODUCT_AREA_EUR) {
                                    System::getDialog()->callBtn2(MESG_SETTINGS_WC24_AND_SHOP_NEED_CONFIRMED_EULA, MESG_NETWORK_SETTINGS_BTN,
                                                                  MESG_CMN_QUIT);
                                } else {
                                    System::getDialog()->callBtn2(MESG_SETTINGS_WC24_AND_SHOP_NEED_CONFIRMED_EULA_EUR, MESG_NETWORK_SETTINGS_BTN,
                                                                  MESG_CMN_QUIT);
                                }
                                mSceneState = 0x09;
                            }
                        } else {
                            mSceneState = 0;
                        }
                    }
                    break;
                case 0x07:
                    if (System::getDialog()->getLastResult() == 1) {
                        www::wiisetting::setFuncResult(1);
                        SCSetWCFlags(SCGetWCFlags() & ~SC_WC_FLAGS_ENABLED);

                        SCIdleModeInfo idleModeInfo;
                        idleModeInfo.standby = 0;
                        idleModeInfo.led = 0;
                        SCSetIdleMode(&idleModeInfo);

                        System::getNwc24Manager()->enableLedNotification(TRUE);

                        SCSetEULA(FALSE);
                        ncd::NCDSetting::adjustNWC24Flag();
                        parental::Parental::setCountry(pWiiSettingData->data[www::wiisetting::WB_ID_COUNTRY]);
                        parental::Parental::clear();
                        System::reloadDownloadTask();
                        mSceneState = 0;
                    } else if (System::getDialog()->getLastResult() == 2) {
                        www::wiisetting::setFuncResult(2);
                        mSceneState = 0;
                        unk_0xB9C = 1;
                    }
                    break;
                case 0x0a:
                    if (System::getDialog()->getLastResult() == 2) {
                        if ((u32)System::getRegion() != SC_PRODUCT_AREA_EUR) {
                            System::getDialog()->callBtn1(MESG_SETTINGS_PLEASE_REVIEW_EULA, MESG_BTN_WII_MENU);
                        } else {
                            System::getDialog()->callBtn1(MESG_SETTINGS_PLEASE_REVIEW_EULA_EUR, MESG_BTN_WII_MENU);
                        }
                        mSceneState = 0xb;
                        SCSetWCFlags(SCGetWCFlags() & ~SC_WC_FLAGS_ENABLED);
                        SCIdleModeInfo idleModeInfo;
                        idleModeInfo.standby = 0;
                        idleModeInfo.led = 0;
                        SCSetIdleMode(&idleModeInfo);

                        System::getNwc24Manager()->enableLedNotification(TRUE);

                        SCSetEULA(FALSE);
                        ncd::NCDSetting::adjustNWC24Flag();
                        if (mStartId == ARG_SETUP || mStartId == ARG_UNK_5) {
                            SCSetConfigDoneFlag(TRUE);
                            SCSetConfigDoneFlag2(TRUE);
                        }
                        SCFlush();
                    } else if (System::getDialog()->getLastResult() == 1) {
                        mSetUpdateState = SET_UPDATE_INIT;
                        unk_0xB9C = 1;
                        mSceneState = 0;
                    }
                    break;
                case 0x0b:
                    if (dialogHasResult()) {
                        mSetUpdateState = SET_UPDATE_REBOOT_SYS;
                        pWiiSettingFlag->smthMsgData = 0x54;
                        mSceneState = 0;
                    }
                    break;
                case 0x0c:
                    if (System::getDialog()->getLastResult() == 2) {
                        SCSetWCFlags(SCGetWCFlags() & ~SC_WC_FLAGS_ENABLED);
                        SCIdleModeInfo idleModeInfo;
                        idleModeInfo.standby = 0;
                        idleModeInfo.led = 0;
                        SCSetIdleMode(&idleModeInfo);

                        System::getNwc24Manager()->enableLedNotification(TRUE);
                        SCSetEULA(FALSE);
                        ncd::NCDSetting::adjustNWC24Flag();
                        SCFlush();
                        if ((u32)System::getRegion() != SC_PRODUCT_AREA_EUR) {
                            System::getDialog()->callBtn1(MESG_SETTINGS_WC24_AND_SHOP_NEED_CONFIRMED_EULA, MESG_CMN_OK);
                        } else {
                            System::getDialog()->callBtn1(MESG_SETTINGS_WC24_AND_SHOP_NEED_CONFIRMED_EULA_EUR, MESG_CMN_OK);
                        }
                        mSceneState = 9;
                    } else if (System::getDialog()->getLastResult() == 1) {
                        unk_0xB9C = 1;
                        mSceneState = 0;
                    }
                    break;
                case 0x0d:
                    if (System::getDialog()->getLastResult() == 1) {
                        www::wiisetting::setFuncResult(8);
                        mSceneState = 0;
                    } else if (System::getDialog()->getLastResult() == 2) {
                        www::wiisetting::setFuncResult(7);
                        mSceneState = 0;
                    }
                    break;
                case 0x0e:
                    if (System::getDialog()->getLastResult() == 1) {
                        pWiiSettingFlag->smthMsgData = 0x55;
                        mSceneState = 0;
                    } else if (System::getDialog()->getLastResult() == 2) {
                        if (unk_0xB9C == 0) {
                            www::wiisetting::setFuncResult(2);
                        } else {
                            if (www::wiisetting::getFuncResult() == 0) {
                                resetFuncMsgQ();
                                mSceneState = 0;
                            }
                        }
                        unk_0xB9C = 1;
                    }
                    break;
                case 0x01:
                    calcKeyboard();
                    break;
                case 0x08:
                    if (++mFrameCounter == 3 * 60) {
                        unk_0xB94 = 1;
                        mFrameCounter = 0;
                    }
                    break;
                case 0x09:
                    if (System::getDialog()->getLastResult() == 1) {
                        if (pWiiSettingFlag->smthMsgData == 0x4f) {
                            www::wiisetting::setFuncResult(5);
                            resetFuncMsgQ();
                        } else {
                            www::wiisetting::setFuncResult(6);
                        }
                        mSceneState = 0;
                    } else if (System::getDialog()->getLastResult() == 2) {
                        if (pWiiSettingFlag->smthMsgData == 0x4f) {
                            if (unk_0xB9C == 0) {
                                www::wiisetting::setFuncResult(2);
                            } else {
                                if (www::wiisetting::getFuncResult() == 0) {
                                    mSceneState = 0;
                                    resetFuncMsgQ();
                                }
                            }
                            unk_0xB9C = 1;
                        } else {
                            mSceneState = 0;
                            www::wiisetting::setFuncResult(2);
                        }
                    }
                    break;
                case 0x11:
                    calcSafeMode();
                    break;
            }
            if (pWiiSettingData->data[www::wiisetting::WB_ID_PAGE_ID] != 0) {
                if (pWiiSettingData->data[www::wiisetting::WB_ID_PAGE_ID] == 0x1e) {
                    u8 newFrameCount = (mFrameCounter += 1);
                    if (newFrameCount == 1) {
                        System::getDialog()->callBtn1(MESG_SETTINGS_SENSITIVITY_SETUP_PROMPT, MESG_CMN_OK);
                        mSceneState = 4;
                    } else if (newFrameCount == 3) {
                        System::getPointer()->setVisible(false);
                    } else if (newFrameCount == 85) {
                        BOOL level = OSDisableInterrupts();
                        WPADSetSensorBarPower(FALSE);
                        OSRestoreInterrupts(level);
                    } else if (newFrameCount == 90) {
                        BOOL level = OSDisableInterrupts();
                        WPADSetSensorBarPower(TRUE);
                        OSRestoreInterrupts(level);
                        mFrameCounter = 30;
                    } else if (newFrameCount == 150) {
                        BOOL level = OSDisableInterrupts();
                        WPADSetSensorBarPower(TRUE);
                        OSRestoreInterrupts(level);
                        System::getHomeButtonMenu()->enable();
                        pWiiSettingData->data[www::wiisetting::WB_ID_PAGE_ID] = 0;
                        System::getPointer()->setVisible(true);
                        mFrameCounter = 0;
                    }
                } else if (pWiiSettingData->data[www::wiisetting::WB_ID_PAGE_ID] < 0x1f) {
                    initHTMLText();
                    initMessage();
                }
            }
            if (pWiiSettingData->data[www::wiisetting::WB_ID_SET_STRING] != 0) {
                calcSetting();
            }
            switch (pWiiSettingFlag->smthMsgData) {
                case 0x01:
                    resetFuncMsgQ();
                    waitStart();
                    Base::createChildScene(SCENE_NAKAMURA_TEST, this, NULL, NULL);
                    break;
                case 0x09:
                    setNUP();
                    break;
                case 0x1e:
                    memset(pResultUSBAP, 0, 0x800);
                    memset(pUSBApBssDescriptorsBuf, 0, 0x79);
                    pWiiSettingFlag->smthMsgData = 0x23;
                case 0x23:
                    setUSBAP();
                    break;
                case 0x1f:
                    cancelUSBAP();
                    break;
                case 0x20:
                case 0x21:
                case 0x22:
                    AOSSProcess();
                    break;
                case 0x28:
                case 0x29:
                case 0x2a:
                case 0x2b:
                case 0x2c:
                    RakuProcess();
                    break;
                case 0x34: {
                    controller::Interface* controller = System::getYoungController();
                    if (!System::getResetHandler()->isResetting() && (System::getHomeButtonMenu()->disable(), unk_0x05C == 0) && controller != NULL &&
                        controller->downTrg(controller::BTN_INTERACT)) {
                        resetFuncMsgQ();
                        System::getHomeButtonMenu()->enable();
                        www::wiisetting::setFuncResult(5);
                    }
                    break;
                }
                case 0x04:
                    resetAP();
                    initAP();
                    pWiiSettingFlag->smthMsgData = 2;
                case 0x02:
                case 0x03:
                    scanAP();
                    break;
                case 0x05:
                    resetAP();
                    break;
                case 0x07:
                    redrawAP();
                    break;
                case 0x06:
                    initAP();
                    resetFuncMsgQ();
                    break;
                case 0x1c: {
                    BOOL settingDataAspectRatio = pWiiSettingData->data[www::wiisetting::WB_ID_DIS_WIDE];
                    if (mAspectRatio != settingDataAspectRatio) {
                        mAspectRatio = settingDataAspectRatio;
                        if (settingDataAspectRatio == (u32)TRUE) {
                            System::getDialog()->callBtn0(MESG_SETTINGS_SET_TV_TO_16_9, 0);
                        } else if (settingDataAspectRatio == FALSE) {
                            System::getDialog()->callBtn0(MESG_SETTINGS_SET_TV_TO_4_3, 0);
                        }
                        mSceneState = 8;
                    }
                    resetFuncMsgQ();
                    break;
                }
                case 0x1a:
                    if ((u32)System::getRegion() != SC_PRODUCT_AREA_EUR) {
                        System::getDialog()->callBtn2(MESG_SETTINGS_DONT_FORGET_PARENTAL_PASSCODE, MESG_CMN_OK, MESG_CMN_BACK_ALT);
                    } else {
                        System::getDialog()->callBtn2(MESG_SETTINGS_DONT_FORGET_PARENTAL_PASSCODE_EUR, MESG_CMN_OK, MESG_CMN_BACK_ALT);
                    }
                    mSceneState = 3;
                    resetFuncMsgQ();
                    break;
                case 0x1d:
                case 0x4d:
                case 0x4e:
                    if (pWiiSettingFlag->smthMsgData == 0x1d || calcSafeMode()) {
                        if ((u8)parental::Parental::checkFlags()) {
                            Base::createChildScene(SCENE_PARENTAL_DIALOG, this, NULL, (void*)ParentalDialog::TYPE_IPL);
                            mSceneState = 5;
                        } else {
                            www::wiisetting::setFuncResult(1);
                            if (pWiiSettingFlag->smthMsgData == 0x4e) {
                                mSceneState = 6;
                            }
                        }
                        if (pWiiSettingFlag->smthMsgData == 0x1d || pWiiSettingFlag->smthMsgData == 0x4d) {
                            resetFuncMsgQ();
                        } else {
                            pWiiSettingFlag->smthMsgData = 0x4f;
                        }
                    }
                    break;
                case 0x50:
                    SCSetLanguage(pWiiSettingData->data[www::wiisetting::WB_ID_LANGUAGE]);
                    if (!SCGetConfigDoneFlag() && !SCGetConfigDoneFlag2() && (u32)System::getRegion() == SC_PRODUCT_AREA_EUR) {
                        if (pWiiSettingData->data[www::wiisetting::WB_ID_LANGUAGE] != SC_LANG_FRENCH) {
                            pWiiSettingData->data[www::wiisetting::WB_ID_COUNTRY] = 0x40;
                            parental::Parental::setCountry(0x40);
                        } else {
                            pWiiSettingData->data[www::wiisetting::WB_ID_COUNTRY] = 0x68;
                            parental::Parental::setCountry(0x68);
                        }
                    }
                    SCFlush();
                    System::getKeyboard()->setLanguage(SCGetLanguage());
                    System::getMessageManager()->initMessage();

                    System::reloadDownloadTask();
                    resetFuncMsgQ();
                    break;

                case 0x51:
                    snd::getSystem()->muteOffBGM(0x5a);
                    resetFuncMsgQ();
                    break;
                case 0x52:
                    snd::getSystem()->muteOnBGM(0x5a);
                    resetFuncMsgQ();
                    break;

                case 0x53:
                    if (mStartId == ARG_SETUP || mStartId == ARG_UNK_5) {
                        parental::Parental::setCountry(pWiiSettingData->data[www::wiisetting::WB_ID_COUNTRY]);
                        parental::Parental::clear();
                        www::wiisetting::setFuncResult(1);
                    } else {
                        if (pWiiSettingData->data[www::wiisetting::WB_ID_COUNTRY] == (parental::Parental::getCountry() & 0xff)) {
                            www::wiisetting::setFuncResult(1);
                        } else {
                            System::getDialog()->callBtn2(MESG_SETTINGS_PARENTAL_NEEDS_RECONFIGURE, MESG_CMN_OK, MESG_CMN_BACK_ALT);
                            mSceneState = 7;
                        }
                    }
                    resetFuncMsgQ();
                    break;
                case 0x54:
                    setUpdate_();
                    break;
                case 0x55:
                    if (mSetEulaState == SET_EULA_INIT) {
                        mSetEulaState = SET_EULA_START;
                    }
                    setUseEULA_();
                    break;
                case 0x56:
                    mSetUpdateState = SET_UPDATE_UNK_9;
                    setUpdate_();
                    break;
                case 0x57:
                    setUseEULA_();
                    break;
                case 0x58:
                    if (mSetEulaState == SET_EULA_INIT) {
                        mSetEulaState = SET_EULA_CANCEL;
                    }
                    setUseEULA_();
                    break;
                case 0x59:
                    mSetUpdateState = SET_UPDATE_CONNECT_TEST_START;
                    pWiiSettingFlag->smthMsgData = 0x54;
                    break;

                case 0x17:
                    parental::Parental::clearMiss();
                case 0x5a:
                    System::getHomeButtonMenu()->enable();
                    resetFuncMsgQ();
                    break;
                case 0x5b:
                    if (System::getHomeButtonMenu()->disable()) {
                        resetFuncMsgQ();
                    }
                    break;
                case 0x5c:
                    unk_0xB5C = 1;
                    resetFuncMsgQ();
                    break;
                case 0x18:
                    ncd::NCDSetting::write();
                case 0x5d:
                    unk_0xB5C = 0;
                    resetFuncMsgQ();
                    break;
                case 0x5e:
                    System::getHomeButtonMenu()->enable();
                    unk_0xB5C = 1;
                    resetFuncMsgQ();
                    break;
                case 0x5f:
                    if (System::getHomeButtonMenu()->disable()) {
                        unk_0xB5C = 0;
                        resetFuncMsgQ();
                    }
                    break;
                case 0x64:
                    Base::createChildScene(SCENE_KITAYAMA_TEST, this, NULL, (void*)2);
                    resetFuncMsgQ();
                    waitStart();
                    break;
                case 0x65:
                    mSceneState = 0xf;
                    if (++mFrameCounter >= 100) {
                        unk_0xB5C = 1;
                        System::getResetHandler()->reset();
                        resetFuncMsgQ();
                    }
                    break;
                case 0x66:
                    mSceneState = 0x10;
                    if (++mFrameCounter >= 100) {
                        unk_0xB5C = 1;
                        System::getResetHandler()->powerOff();
                        resetFuncMsgQ();
                    }
                    break;
                case 0x67:
                    if (calcSafeMode()) {
                        www::wiisetting::setFuncResult(1);
                        resetFuncMsgQ();
                    }
            }
            if (pWiiSettingData->data[www::wiisetting::WB_ID_SE] || pWiiSettingData->data[www::wiisetting::WB_ID_EXCSE]) {
                setSE();
            }

            if (pWiiSettingData->data[www::wiisetting::WB_ID_FINISH]) {
                System::getFader()->fadeOut();
                return FADER_SCN_NEXT;
            } else {
                pLytSceenChange->calc();
                pLytMyAP->calc();
                pLytWaiting->calc();
                unk_0x91E = false;
                return FADER_SCN_CONTINUE;
            }
        }

        FaderSceneCommand Setting::calcFadeout() {
            if (System::getFader()->getStatus() == EGG::Fader::PREPARE_IN) {
                if (!unk_0x05C) {
                    unk_0x05C = 1;
                    ext_ead::www::SurfaceManager::GetInstance()->StopThreadAsync();
                    OSReport("!!!!!!!!!!!!! SCFlush !!!!!!!!!!!!!!\n");
                    SCFlush();
                } else if (unk_0x05C && mSetEulaState == 5) {
                    if ((mStartId == ARG_SETUP) || (mStartId == ARG_UNK_5)) {
                        SCSetConfigDoneFlag(1);
                        SCSetConfigDoneFlag2(1);
                        SCSetUpdateType(0);
                        SCFlush();
                    }
                    while (WPADGetStatus() != 0 || System::getBS2Manager()->getIPLState() != bs2::IPL_STATE_8) {
                        snd::getSystem()->calc();
                        System::getBS2Manager()->update();
                        VIWaitForRetrace();
                        if (WPADGetStatus() != 0) {
                            OSReport("wait for WPAD\n");
                        }
                        if (System::getBS2Manager()->getIPLState() != bs2::IPL_STATE_8) {
                            OSReport("wait for BS2\n");
                        }
                    }

                    VISetBlack(TRUE);
                    VIFlush();
                    VIWaitForRetrace();
                    OSReport("VI Black\n");

                    while (!__OSSyncSram())
                        OSReport("sync sram\n");

                    while (!System::isReceiveScheduleStopped()) {
                        OSReport("Wait ScheduleStopped\n");
                        OSSleepMilliseconds(5);
                    }

                    __OSLaunchTitlelForSystem(mTitleID, 0, NULL);

                    while (true) {
                        OSReport("hoge");
                    }
                } else if (unk_0x05C && ::ext_ead::www::SurfaceManager::GetInstance()->IsThreadStopped()) {
                    OSReport("reserve destroy\n");
                    ext_ead::www::SurfaceManager::DisposeManager();
                    OSReport("reserve destroy done\n");
                    if (mStartId == ARG_SETUP || mStartId == ARG_UNK_5) {
                        SCSetConfigDoneFlag(1);
                        SCSetConfigDoneFlag2(1);
                        SCSetUpdateType(0);
                        SCFlush();
                        Base::reserveAllSceneDestruction(SCENE_HEALTH, NULL);
                    } else {
                        System::reloadDownloadTask();
                        Base::reserveAllSceneDestruction(SCENE_SETTING_BG, NULL);
                    }
                    delete pImeData;
                    return FADER_SCN_NEXT;
                }
            }
            return FADER_SCN_CONTINUE;
        }
        void Setting::draw() {
            if (!System::getSceneManager()->onDefaultDrawLayer())
                return;

            if (!mBrowserCreated) {
                utility::Graphics::setOrtho(0);
                nw4r::ut::Rect bgRect(-1000.0, -1000.0, 1000.0, 1000.0);
                GXColor bgColor = {0x00, 0x00, 0x00, 0xFF};
                utility::Graphics::drawPolygon(bgRect, bgColor);
                return;
            }

            if (ext_ead::www::SurfaceManager::GetInstance() == NULL)
                return;

            ext_ead::www::BrowserThread* browserThread = ext_ead::www::SurfaceManager::GetInstance()->mpBrowserThread;
            if (browserThread == NULL)
                return;
            void* browserRenderTex = browserThread->GetTextureBuffer(0, NULL);
            if (browserRenderTex == NULL)
                return;

            u8 windowUnk_0x2C4;
            if (browserThread->GetWindow(0) == NULL) {
                windowUnk_0x2C4 = 0;
            } else {
                windowUnk_0x2C4 = ((ext_ead::www::BrowserWindow*)browserThread->GetWindow(0))->unk_0x2C4[3];
            }
            if (windowUnk_0x2C4) {
                unk_0x91E = true;
                mFadeFramesElapsed = 0;
                if (browserThread->GetWindow(0) != NULL) {
                    ((ext_ead::www::BrowserWindow*)browserThread->GetWindow(0))->unk_0x2C4[3] = 0;
                }
                www::trasition::ScrollState scrollState = www::trasition::GetScrollState();
                if (scrollState == www::trasition::SCROLL_LEFT) {
                    queuedSceneChangeIdx = 1;
                } else if (scrollState == www::trasition::SCROLL_RIGHT) {
                    queuedSceneChangeIdx = -1;
                } else {
                    queuedSceneChangeIdx = 0;
                }
                www::trasition::ResetScrollState();
                void* bufB = browserThread->GetTextureBuffer(1, NULL);
                void* bufA = browserThread->GetTextureBuffer(0, NULL);
                OSReport("changed %p %p\n", bufA, bufB);
                ext_ead::www::Heap::reportLeaHeap();
            }

            if (!pLytSceenChangeL->isPlaying() && !pLytSceenChangeR->isPlaying()) {
                pLytSceenChange->GetRootPane()->FindPaneByName("N_Tra0")->SetVisible(false);
            } else {
                pLytSceenChange->GetRootPane()->FindPaneByName("N_Tra0")->SetVisible(true);
            }
            WWWRect* wwwRectTexA;
            WWWRect* wwwRectTexB;
            void* texBufBrowserB = browserThread->GetTextureBuffer(1, &wwwRectTexB);
            void* texBufBrowserA = browserThread->GetTextureBuffer(0, &wwwRectTexA);

            nw4r::ut::Rect projRect4x3;
            System::getProjectionRect4x3(&projRect4x3);

            double projWD = projRect4x3.GetWidth();
            int projW = abs(projWD);
            double projHD = projRect4x3.GetHeight();
            int projH = abs(projHD);
            int projL = -projW / 2;
            int projT = projH / 2;
            int projR = projW / 2;
            int projB = -projH / 2;

            nw4r::ut::Rect rectTexB(projL, projT, projR, projB);

            if (texBufBrowserB && texBufBrowserA) {
                GXTexObj texBrowserA;
                GXTexObj texBrowserB;
                GXInitTexObj(&texBrowserB, texBufBrowserB, wwwRectTexB->w, wwwRectTexB->h, GX_TF_RGB565, GX_CLAMP, GX_CLAMP, 0);
                GXInitTexObj(&texBrowserA, texBufBrowserA, wwwRectTexA->w, wwwRectTexA->h, GX_TF_RGB565, GX_CLAMP, GX_CLAMP, 0);
                GXInitTexObjLOD(&texBrowserB, GX_LINEAR, GX_LINEAR, 0, 0, 0, GX_FALSE, GX_FALSE, GX_ANISO_1);
                GXInitTexObjLOD(&texBrowserA, GX_LINEAR, GX_LINEAR, 0, 0, 0, GX_FALSE, GX_FALSE, GX_ANISO_1);

                GXTexObj texBg;
                TPLGetGXTexObjFromPalette((TPLPalette*)pBgTpl->getBuffer(), &texBg, 0);
                TPLDescriptor* bgTplDesc = TPLGet((TPLPalette*)pBgTpl->getBuffer(), 0);

                nw4r::ut::Rect projRect16x9;
                System::getProjectionRect16x9(&projRect16x9);
                double projWD = projRect16x9.GetWidth();
                int projWI = abs(projWD);
                double projHD = projRect16x9.GetHeight();
                int projHI = abs(projHD);

                float projL = -projWI / 2;  // NOLINT(bugprone-integer-division)
                float projT1 = projHI / 2;  // NOLINT(bugprone-integer-division)
                float projR = projWI / 2;   // NOLINT(bugprone-integer-division)
                float projT0 = projHI / 2;  // NOLINT(bugprone-integer-division)

                nw4r::ut::Rect rectTexBrowser1(projL, projT1, projL + bgTplDesc->textureHeader->width, projT1 - bgTplDesc->textureHeader->height);
                nw4r::ut::Rect rectTexBrowser0(projR - bgTplDesc->textureHeader->width, projT0, projR, projT0 - bgTplDesc->textureHeader->height);
                if (queuedSceneChangeIdx) {
                    nw4r::lyt::Material* matTex0 = pLytSceenChange->GetRootPane()->FindPaneByName("N_Tra0")->FindMaterialByName("Tex0");
                    nw4r::lyt::Material* matTex1 = pLytSceenChange->GetRootPane()->FindPaneByName("N_Tra0")->FindMaterialByName("Tex1");
                    nw4r::lyt::Material* matTex2 = pLytSceenChange->GetRootPane()->FindPaneByName("N_Tra0")->FindMaterialByName("Tex2");
                    matTex0->SetTexture(0, texBrowserB);
                    matTex1->SetTexture(0, texBrowserA);
                    matTex2->SetTexture(0, texBrowserA);

                    pLytSceenChange->GetRootPane()->FindPaneByName("N_Tra0")->SetVisible(true);
                    if (queuedSceneChangeIdx == 1) {
                        pLytSceenChangeL->play();
                    } else {
                        pLytSceenChangeR->play();
                    }
                    pLytSceenChange->calc();
                    queuedSceneChangeIdx = 0;
                }

                if ((mSceneState == 8) && (mFadeFramesElapsed != 0x14)) {
                    mFadeFramesElapsed = 0;
                }
                utility::Graphics::setOrtho(0);
                GXColor baseColor = {0xff, 0xff, 0xff, 0xff};

                u8 alpha = (mFadeFramesElapsed * 0xff) / 0x14;
                GXColor transpColor = {0xff, 0xff, 0xff, alpha};
                utility::Graphics::drawTexture(rectTexB, texBrowserB, baseColor, 1);

                bool browserBIsNotAllTransparent = false;
                for (int i = 0; i < wwwRectTexB->w * wwwRectTexB->h * 2 / 4; i++) {
                    if (((u32*)texBufBrowserB)[i] != 0) {
                        browserBIsNotAllTransparent = true;
                        break;
                    }
                }
                if (browserBIsNotAllTransparent) {
                    utility::Graphics::drawTexture(rectTexBrowser0, texBg, baseColor, 1);
                    utility::Graphics::drawTexture(rectTexBrowser1, texBg, baseColor, 1);
                }

                utility::Graphics::drawTexture(rectTexB, texBrowserA, transpColor, 1);
                utility::Graphics::drawTexture(rectTexBrowser0, texBg, transpColor, 1);
                utility::Graphics::drawTexture(rectTexBrowser1, texBg, transpColor, 1);

                if (pWiiSettingData->data[www::wiisetting::WB_ID_PAGE_ID] == 0x1e && mFrameCounter > 3) {
                    SensitivityDrawing::draw(pBgTpl);
                }
                pLytSceenChange->draw();
                pLytWaiting->draw();

                if (mFadeFramesElapsed == 0xc)
                    unk_0xB9C = 1;
            }
            if (2 <= pWiiSettingFlag->smthMsgData && pWiiSettingFlag->smthMsgData <= 7) {
                GXRenderModeObj renderMode;
                renderMode = *System::getRenderModeObj();
                u32 scissorL;
                u32 scissorT;
                u32 scissorW;
                u32 scissorH;
                GXGetScissor(&scissorL, &scissorT, &scissorW, &scissorH);
                GXSetScissor(0, renderMode.efbHeight / 2 - 0xa4, renderMode.fbWidth, 306);
                pLytMyAP->draw();
                GXSetScissor(scissorL, scissorT, scissorW, scissorH);
            }
        }
        void Setting::initWiiSettingData() {
            pWiiSettingData = www::wiisetting::getWiiSettingData();
            pWiiSettingFlag = www::wiisetting::getWiiSettingFlag();
            pWiiSettingData->data[www::wiisetting::WB_ID_COUNTRY] = parental::Parental::getCountry();
            pWiiSettingData->data[www::wiisetting::WB_ID_DIS_POS] = 32 - (SCGetDisplayOffsetH() + 16);
            pWiiSettingData->data[www::wiisetting::WB_ID_LIGHT] = SCGetBtDpdSensibility();
            pWiiSettingData->data[www::wiisetting::WB_ID_DIS_WIDE] = SCGetAspectRatio();
            pWiiSettingData->data[www::wiisetting::WB_ID_LANGUAGE] = SCGetLanguage();
            pWiiSettingData->data[www::wiisetting::WB_ID_RATE] = parental::Parental::checkRating();
        }

        void Setting::initHTMLText() {
            OSReport("initHTMLText pageId:%d\n", pWiiSettingData->data[www::wiisetting::WB_ID_PAGE_ID]);
            memset(mHtmlStrScratch, 0, sizeof(mHtmlStrScratch));
            switch (pWiiSettingData->data[www::wiisetting::WB_ID_PAGE_ID]) {
                case 0x02:
                    initNickName();
                    break;
                case 0x03:
                    initSecurityKey();
                    break;
                case 0x04:
                    initSSID();
                    break;
                case 0x05:
                    initIP();
                    break;
                case 0x06:
                    initDNS();
                    break;
                case 0x07:
                    initProxy();
                    break;
                case 0x08:
                    initBasic();
                    break;
                case 0x09:
                    initMTU();
                    break;
                case 0x0a:
                    memset(pHtmlStr->parentalPass, 0, sizeof(pHtmlStr->parentalPass));
                    break;
                case 0x0b:
                    memset(pHtmlStr->parentalRePass, 0, sizeof(pHtmlStr->parentalRePass));
                    break;
                case 0x0c:
                    memset(pHtmlStr->parentalJudgePass, 0, sizeof(pHtmlStr->parentalJudgePass));
                    break;
                case 0x0d:
                    initSecA();
                    break;
                case 0x0e:
                    memset(pHtmlStr->parentalReSecA, 0, sizeof(pHtmlStr->parentalReSecA));
                    break;
                case 0x0f:
                    memset(pHtmlStr->masterKey, 0, sizeof(pHtmlStr->masterKey));
                    break;
                case 0x10:
                    memset(pHtmlStr->asterisks, 0, sizeof(pHtmlStr->asterisks));
            }
        }
        void Setting::initMessage() {
            OSReport("initMessage pageId:%d\n", pWiiSettingData->data[www::wiisetting::WB_ID_PAGE_ID]);
            pWiiSettingData->data[www::wiisetting::WB_ID_PAGE_ID] = 0;
        }
        void Setting::initKeyboard(const char* utf8Str) {
            OSReport("initKeyboard formId:%d\n", pWiiSettingData->data[www::wiisetting::WB_ID_FORM_ID]);
            memset(mHtmlStrScratch, 0, sizeof(mHtmlStrScratch));

            u32 stringLimit;
            u32 rowLimit;
            keyboard::Manager::KeyboardType kbdType;
            bool shouldZero;
            int prodArea;

            rowLimit = 1;
            shouldZero = false;
            prodArea = SCGetProductArea();

            switch (pWiiSettingData->data[www::wiisetting::WB_ID_FORM_ID]) {
                case www::wiisetting::FORM_ID_NICKNAME:
                    stringLimit = 10;
                    kbdType = keyboard::Manager::NORMAL_BIGTEXT_WITHOUT_LINEFEED;
                    break;
                case www::wiisetting::FORM_ID_SSID:
                    stringLimit = 0x20;
                    rowLimit = 2;
                    kbdType = keyboard::Manager::ONLY_QWERTY_WITHOUT_LINEFEED_AND_SIGN;
                    break;
                    break;
                case www::wiisetting::FORM_ID_PROXY_SERVER:
                    stringLimit = 0xff;
                    rowLimit = 0x10;
                    kbdType = keyboard::Manager::ONLY_QWERTY_WITHOUT_LINEFEED_AND_SIGN;
                    break;
                case www::wiisetting::FORM_ID_PROXY_BASIC_USERNAME:
                    stringLimit = 0x20;
                    rowLimit = 2;
                    kbdType = keyboard::Manager::ONLY_QWERTY_WITHOUT_LINEFEED_AND_SIGN;
                    break;
                case www::wiisetting::FORM_ID_PARENTAL_SEC_ANSWER:
                case www::wiisetting::FORM_ID_PARENTAL_RE_SEC_ANSWER:
                    stringLimit = 0x20;
                    rowLimit = 2;
                    if ((prodArea == SC_PRODUCT_AREA_CHN) || (prodArea == SC_PRODUCT_AREA_KOR)) {
                        kbdType = keyboard::Manager::PREDICT_WITHOUT_LINEFEED;
                    } else {
                        kbdType = keyboard::Manager::NORMAL_WITHOUT_LINEFEED_WITH_SIGN;
                    }
                    break;
                case www::wiisetting::FORM_ID_IP_ADDR:
                case www::wiisetting::FORM_ID_IP_NETMASK:
                case www::wiisetting::FORM_ID_IP_GATEWAY:
                case www::wiisetting::FORM_ID_DNS1:
                case www::wiisetting::FORM_ID_DNS2:
                    stringLimit = 0xf;
                    kbdType = keyboard::Manager::NUMERIC_BIGTEXT_WITH_DOT;
                    break;
                case www::wiisetting::FORM_ID_PROXY_PORT:
                case www::wiisetting::FORM_ID_MASTER_KEY:
                    stringLimit = 5;
                    kbdType = keyboard::Manager::NUMERIC;
                    break;
                case www::wiisetting::FORM_ID_ADJ_MTU:
                case www::wiisetting::FORM_ID_PARENTAL_PASS:
                case www::wiisetting::FORM_ID_PARENTAL_RE_PASS:
                case www::wiisetting::FORM_ID_PARENTAL_JUDGE_PASS:
                    stringLimit = 4;
                    kbdType = keyboard::Manager::NUMERIC;
                    break;
                case www::wiisetting::FORM_ID_SECURITY_KEY:
                    if ((u16)ncd::NCDSetting::getPrivacyMode() == 1) {
                        stringLimit = 0x1a;
                        rowLimit = 2;
                    } else {
                        stringLimit = 0x40;
                        rowLimit = 4;
                    }
                    kbdType = keyboard::Manager::ONLY_QWERTY_WITHOUT_LINEFEED_AND_SIGN;
                    break;
                case www::wiisetting::FORM_ID_PROXY_BASIC_PASSWORD:
                    stringLimit = 0x20;
                    rowLimit = 2;
                    kbdType = keyboard::Manager::ONLY_QWERTY_WITHOUT_LINEFEED_AND_SIGN;
                    break;
                case www::wiisetting::FORM_ID_DUMMY_SECURITY_KEY:
                    stringLimit = 0x40;
                    rowLimit = 4;
                    kbdType = keyboard::Manager::ONLY_QWERTY_WITHOUT_LINEFEED_AND_SIGN;
            }
            u32 formID = pWiiSettingData->data[www::wiisetting::WB_ID_FORM_ID];
            if (formID != 0x0d && formID != 0x02 && formID != 0x12 && formID != 0x13 && formID != 0x16) {
                utility::CharacterCode::UTF8ToUTF16((wchar_t*)mHtmlStrScratch, utf8Str, sizeof(mHtmlStrScratch) >> 1);
            }

            // TODO: This string was originally encoded in Shift-JIS
            OSReport("キーボード: %d %d %d %d\n", kbdType, stringLimit, rowLimit, wcslen((wchar_t*)mHtmlStrScratch));
            ((wchar_t*)mHtmlStrScratch)[stringLimit] = 0;

            switch (pWiiSettingData->data[www::wiisetting::WB_ID_FORM_ID]) {
                case www::wiisetting::FORM_ID_NICKNAME:
                case www::wiisetting::FORM_ID_SECURITY_KEY:
                case www::wiisetting::FORM_ID_SSID:
                case www::wiisetting::FORM_ID_PROXY_SERVER:
                case www::wiisetting::FORM_ID_PROXY_PORT:
                case www::wiisetting::FORM_ID_PROXY_BASIC_USERNAME:
                case www::wiisetting::FORM_ID_PROXY_BASIC_PASSWORD:
                case www::wiisetting::FORM_ID_ADJ_MTU:
                case www::wiisetting::FORM_ID_PARENTAL_PASS:
                case www::wiisetting::FORM_ID_PARENTAL_RE_PASS:
                case www::wiisetting::FORM_ID_PARENTAL_JUDGE_PASS:
                case www::wiisetting::FORM_ID_PARENTAL_SEC_ANSWER:
                case www::wiisetting::FORM_ID_PARENTAL_RE_SEC_ANSWER:
                case www::wiisetting::FORM_ID_MASTER_KEY:
                case www::wiisetting::FORM_ID_DUMMY_SECURITY_KEY:
                    shouldZero = checkInputString((wchar_t*)mHtmlStrScratch);

                    break;

                case www::wiisetting::FORM_ID_IP_ADDR:
                case www::wiisetting::FORM_ID_IP_NETMASK:
                case www::wiisetting::FORM_ID_IP_GATEWAY:
                case www::wiisetting::FORM_ID_DNS1:
                case www::wiisetting::FORM_ID_DNS2:
                    shouldZero = checkIPString((wchar_t*)mHtmlStrScratch);
                    break;

                default:
                    break;
            }
        COMPARE:
            if (shouldZero)
                memset(mHtmlStrScratch, 0, sizeof(mHtmlStrScratch));

            if (prodArea == SC_PRODUCT_AREA_CHN) {
                void* sysDict = System::getKeyboard()->getZiSystemDic();
                void* oemDict = System::getKeyboard()->getZiOemDic();
                textinput::MemoInputForm* form = System::getKeyboard()->memoFrm();
                form->setZiDictionary(oemDict, sysDict);
            }
            keyboard::Manager::KeyboardSetting kbdSetting;
            kbdSetting.type = kbdType;
            kbdSetting.wcString = (wchar_t*)mHtmlStrScratch;
            kbdSetting.stringLimit = stringLimit;
            kbdSetting.rowLimit = rowLimit;
            System::getKeyboard()->start(0, kbdSetting);

            if (shouldZero) {
                setDefaultBackString();
            } else {
                System::getKeyboard()->memoMgr()->setTitleText(L"");
            }
            if (pWiiSettingData->data[www::wiisetting::WB_ID_FORM_ID] == 0x11) {
                System::getKeyboard()->memoMgr()->setSecretInputMode(true);
            }
        }

        void Setting::calcKeyboard() {
            switch (mKbdMgrState.iplType) {
                case keyboard::Manager::STATE_VISIBLE:
                    break;
                case keyboard::Manager::STATE_DISAPPEARING:
                    char* str;
                    if (mKbdMgrState.pressOK) {
                        onTextInputOK();
                        switch (pWiiSettingData->data[www::wiisetting::WB_ID_FORM_ID]) {
                            case www::wiisetting::FORM_ID_NICKNAME:
                                str = pHtmlStr->nickname;
                                break;
                            case www::wiisetting::FORM_ID_SECURITY_KEY:
                            case www::wiisetting::FORM_ID_DUMMY_SECURITY_KEY:
                                str = pHtmlStr->securityKey;
                                break;
                            case www::wiisetting::FORM_ID_SSID:
                                str = pHtmlStr->ssid;
                                break;
                            case www::wiisetting::FORM_ID_IP_ADDR:
                                str = pHtmlStr->ip.addr;
                                break;
                            case www::wiisetting::FORM_ID_IP_NETMASK:
                                str = pHtmlStr->ip.netmask;
                                break;
                            case www::wiisetting::FORM_ID_IP_GATEWAY:
                                str = pHtmlStr->ip.gateway;
                                break;
                            case www::wiisetting::FORM_ID_DNS1:
                                str = pHtmlStr->dns1;
                                break;
                            case www::wiisetting::FORM_ID_DNS2:
                                str = pHtmlStr->dns2;
                                break;
                            case www::wiisetting::FORM_ID_PROXY_SERVER:
                                str = pHtmlStr->proxy.server;
                                break;
                            case www::wiisetting::FORM_ID_PROXY_PORT:
                                str = pHtmlStr->proxy.port;
                                break;
                            case www::wiisetting::FORM_ID_PROXY_BASIC_USERNAME:
                                str = pHtmlStr->proxyBasic.uname;
                                break;
                            case www::wiisetting::FORM_ID_PROXY_BASIC_PASSWORD:
                                str = pHtmlStr->proxyBasic.pass;
                                break;
                            case www::wiisetting::FORM_ID_PARENTAL_PASS:
                                str = pHtmlStr->parentalPass;
                                break;
                            case www::wiisetting::FORM_ID_PARENTAL_RE_PASS:
                                str = pHtmlStr->parentalRePass;
                                break;
                            case www::wiisetting::FORM_ID_PARENTAL_JUDGE_PASS:
                                str = pHtmlStr->parentalJudgePass;
                                break;
                            case www::wiisetting::FORM_ID_PARENTAL_SEC_ANSWER:
                                str = pHtmlStr->parentalSecA;
                                break;
                            case www::wiisetting::FORM_ID_PARENTAL_RE_SEC_ANSWER:
                                str = pHtmlStr->parentalReSecA;
                                break;
                            case www::wiisetting::FORM_ID_MASTER_KEY:
                                str = pHtmlStr->masterKey;
                                break;
                            case www::wiisetting::FORM_ID_ADJ_MTU:
                                str = pHtmlStr->adjMtu;
                                break;
                        }
                        OSReport("formID:%d %s\n", pWiiSettingData->data[www::wiisetting::WB_ID_FORM_ID], str);
                        if (strlen(str) == 0) {
                            str[0] = '\0';
                            ::ext_ead::www::SurfaceManager::GetInstance()->GetBrowserThread()->CommitIme(pImeData, str);
                            memset(pHtmlStr->asterisks, 0, 0x42);
                        } else {
                            if ((pWiiSettingData->data[www::wiisetting::WB_ID_FORM_ID] == www::wiisetting::FORM_ID_SECURITY_KEY) ||
                                (pWiiSettingData->data[www::wiisetting::WB_ID_FORM_ID] == www::wiisetting::FORM_ID_DUMMY_SECURITY_KEY)) {
                                int i = 0;
                                memcpy(pHtmlStr->asterisks, pHtmlStr->securityKey, 0x41);
                                pHtmlStr->asterisks[0x41] = '\0';

                                for (; pHtmlStr->asterisks[i]; i++) {
                                    pHtmlStr->asterisks[i] = '*';
                                }
                                if (i > 0x20) {
                                    pHtmlStr->asterisks[0x20] = '\n';
                                    pHtmlStr->asterisks[i] = '*';
                                }
                                ::ext_ead::www::SurfaceManager::GetInstance()->GetBrowserThread()->CommitIme(pImeData, pHtmlStr->asterisks);
                            } else {
                                ::ext_ead::www::SurfaceManager::GetInstance()->GetBrowserThread()->CommitIme(pImeData, str);
                            }
                        }
                    } else {
                        switch (pWiiSettingData->data[www::wiisetting::WB_ID_FORM_ID]) {
                            case www::wiisetting::FORM_ID_NICKNAME:
                                str = pHtmlStr->nickname;
                                break;
                            case www::wiisetting::FORM_ID_SECURITY_KEY:
                            case www::wiisetting::FORM_ID_DUMMY_SECURITY_KEY:
                                str = pHtmlStr->asterisks;
                                break;
                            case www::wiisetting::FORM_ID_SSID:
                                str = pHtmlStr->ssid;
                                break;
                            case www::wiisetting::FORM_ID_IP_ADDR:
                                str = pHtmlStr->ip.addr;
                                break;
                            case www::wiisetting::FORM_ID_IP_NETMASK:
                                str = pHtmlStr->ip.netmask;
                                break;
                            case www::wiisetting::FORM_ID_IP_GATEWAY:
                                str = pHtmlStr->ip.gateway;
                                break;
                            case www::wiisetting::FORM_ID_DNS1:
                                str = pHtmlStr->dns1;
                                break;
                            case www::wiisetting::FORM_ID_DNS2:
                                str = pHtmlStr->dns2;
                                break;
                            case www::wiisetting::FORM_ID_PROXY_SERVER:
                                str = pHtmlStr->proxy.server;
                                break;
                            case www::wiisetting::FORM_ID_PROXY_PORT:
                                str = pHtmlStr->proxy.port;
                                break;
                            case www::wiisetting::FORM_ID_PROXY_BASIC_USERNAME:
                                str = pHtmlStr->proxyBasic.uname;
                                break;
                            case www::wiisetting::FORM_ID_PROXY_BASIC_PASSWORD:
                                str = pHtmlStr->proxyBasic.pass;
                                break;
                            case www::wiisetting::FORM_ID_PARENTAL_PASS:
                                str = pHtmlStr->parentalPass;
                                break;
                            case www::wiisetting::FORM_ID_PARENTAL_RE_PASS:
                                str = pHtmlStr->parentalRePass;
                                break;
                            case www::wiisetting::FORM_ID_PARENTAL_JUDGE_PASS:
                                str = pHtmlStr->parentalJudgePass;
                                break;
                            case www::wiisetting::FORM_ID_PARENTAL_SEC_ANSWER:
                                str = pHtmlStr->parentalSecA;
                                break;
                            case www::wiisetting::FORM_ID_PARENTAL_RE_SEC_ANSWER:
                                str = pHtmlStr->parentalReSecA;
                                break;
                            case www::wiisetting::FORM_ID_MASTER_KEY:
                                str = pHtmlStr->masterKey;
                                break;
                            case www::wiisetting::FORM_ID_ADJ_MTU:
                                str = pHtmlStr->adjMtu;
                                break;
                        }
                        ext_ead::www::SurfaceManager::GetInstance()->GetBrowserThread()->CommitIme(pImeData, str);
                    }
                    pWiiSettingData->data[www::wiisetting::WB_ID_FORM_ID] = 0;
                    ext_ead::www::SurfaceManager::GetInstance()->GetBrowserThread()->DisposeImeData(pImeData);
                    break;
                case keyboard::Manager::STATE_HIDDEN_AFTER_DISAPPEAR:
                    System::getKeyboard()->baseMgr()->setTitleText(L"");
                    mSceneState = 0;
                    break;
                case keyboard::Manager::STATE_HIDDEN:
                case keyboard::Manager::STATE_APPEARING:
                    switch (pWiiSettingData->data[www::wiisetting::WB_ID_FORM_ID]) {
                        case www::wiisetting::FORM_ID_MAC_ADDR:
                        case www::wiisetting::LAN_MAC_ADDR:
                        default:
                            break;

                        case www::wiisetting::FORM_ID_NICKNAME:
                        case www::wiisetting::FORM_ID_SECURITY_KEY:
                        case www::wiisetting::FORM_ID_DUMMY_SECURITY_KEY:
                        case www::wiisetting::FORM_ID_SSID:
                        case www::wiisetting::FORM_ID_IP_ADDR:
                        case www::wiisetting::FORM_ID_IP_NETMASK:
                        case www::wiisetting::FORM_ID_IP_GATEWAY:
                        case www::wiisetting::FORM_ID_DNS1:
                        case www::wiisetting::FORM_ID_DNS2:
                        case www::wiisetting::FORM_ID_PROXY_SERVER:
                        case www::wiisetting::FORM_ID_PROXY_PORT:
                        case www::wiisetting::FORM_ID_PROXY_BASIC_USERNAME:
                        case www::wiisetting::FORM_ID_PROXY_BASIC_PASSWORD:
                        case www::wiisetting::FORM_ID_PARENTAL_PASS:
                        case www::wiisetting::FORM_ID_PARENTAL_RE_PASS:
                        case www::wiisetting::FORM_ID_PARENTAL_JUDGE_PASS:
                        case www::wiisetting::FORM_ID_PARENTAL_SEC_ANSWER:
                        case www::wiisetting::FORM_ID_PARENTAL_RE_SEC_ANSWER:
                        case www::wiisetting::FORM_ID_MASTER_KEY:
                        case www::wiisetting::FORM_ID_ADJ_MTU:
                            if (System::getKeyboard()->baseMgr()->isVacancy()) {
                                setDefaultBackString();
                            } else {
                                System::getKeyboard()->baseMgr()->setTitleText(L"");
                            }
                            break;
                    }
                    break;
            }
            mKbdMgrState = *System::getKeyboard()->getState();
        }
        void Setting::calcSetting() {
            OSReport("setstring:%d\n", pWiiSettingData->data[www::wiisetting::WB_ID_SET_STRING]);
            switch (pWiiSettingData->data[www::wiisetting::WB_ID_SET_STRING]) {
                case 1:
                    setDisPos();
                    break;
                case 2:
                    setNickName();
                    break;
                case 3:
                    setSecurityKey();
                    break;
                case 4:
                    setSSID();
                    break;
                case 5:
                    setIP();
                    break;
                case 6:
                    setDNS();
                    break;
                case 7:
                    setProxy();
                    break;
                case 8:
                    setBasic();
                    break;
                case 9:
                    setMTU();
                    break;
                case 10:
                    setParePass();
                    break;
                case 0xb:
                    setPareRePass();
                    break;
                case 0xc:
                    setPareJudgePass();
                    break;
                case 0xd:
                    setSecA();
                    break;
                case 0xe:
                    setReSecA();
                    break;
                case 0xf:
                    setMasterKey();
            }
            pWiiSettingData->data[www::wiisetting::WB_ID_SET_STRING] = 0;
            return;
        }
        void Setting::onTextInputOK() {
            char buf[0x302];
            OSReport("Keyboard Confirm:%d\n", pWiiSettingData->data[www::wiisetting::WB_ID_FORM_ID]);
            // Why???
            wcslen(mKbdMgrState.wcString);
            memset(mHtmlStrScratch, 0, sizeof(mHtmlStrScratch));
            memset(buf, 0, sizeof(buf));
            memcpy(mHtmlStrScratch, mKbdMgrState.wcString, sizeof(mHtmlStrScratch));

            u8 formID = pWiiSettingData->data[www::wiisetting::WB_ID_FORM_ID];
            if (formID == www::wiisetting::FORM_ID_SECURITY_KEY || formID == www::wiisetting::FORM_ID_DUMMY_SECURITY_KEY) {
                utility::CharacterCode::UTF16ToANSI((u8*)buf, (wchar_t*)mHtmlStrScratch, 0x100);
                memset(buf + wcslen((wchar_t*)mHtmlStrScratch), 0, 0x100 - wcslen((wchar_t*)mHtmlStrScratch));
                memcpy(pHtmlStr->securityKey, buf, sizeof(pHtmlStr->securityKey));
            } else {
                if (formID == www::wiisetting::FORM_ID_PARENTAL_SEC_ANSWER || formID == www::wiisetting::FORM_ID_PARENTAL_RE_SEC_ANSWER) {
                    adjustSecA((wchar_t*)mHtmlStrScratch);
                }
                utility::CharacterCode::UTF16ToUTF8(buf, (wchar_t*)mHtmlStrScratch, 0x301);
                switch (pWiiSettingData->data[www::wiisetting::WB_ID_FORM_ID]) {
                    case www::wiisetting::FORM_ID_NICKNAME:
                        memcpy(pHtmlStr->nickname, buf, sizeof(pHtmlStr->nickname));
                        break;
                    case www::wiisetting::FORM_ID_SSID:
                        memcpy(pHtmlStr->ssid, buf, sizeof(pHtmlStr->ssid));
                        break;
                    case www::wiisetting::FORM_ID_IP_ADDR:
                        memcpy(pHtmlStr->ip.addr, buf, sizeof(pHtmlStr->ip.addr));
                        break;
                    case www::wiisetting::FORM_ID_IP_NETMASK:
                        memcpy(pHtmlStr->ip.netmask, buf, sizeof(pHtmlStr->ip.netmask));
                        break;
                    case www::wiisetting::FORM_ID_IP_GATEWAY:
                        memcpy(pHtmlStr->ip.gateway, buf, sizeof(pHtmlStr->ip.gateway));
                        break;
                    case www::wiisetting::FORM_ID_DNS1:
                        memcpy(pHtmlStr->dns1, buf, sizeof(pHtmlStr->dns1));
                        break;
                    case www::wiisetting::FORM_ID_DNS2:
                        memcpy(pHtmlStr->dns2, buf, sizeof(pHtmlStr->dns2));
                        break;
                    case www::wiisetting::FORM_ID_PROXY_SERVER:
                        memcpy(pHtmlStr->proxy.server, buf, sizeof(pHtmlStr->proxy.server));
                        break;
                    case www::wiisetting::FORM_ID_PROXY_PORT:
                        memcpy(pHtmlStr->proxy.port, buf, sizeof(pHtmlStr->proxy.port));
                        break;
                    case www::wiisetting::FORM_ID_PROXY_BASIC_USERNAME:
                        memcpy(pHtmlStr->proxyBasic.uname, buf, sizeof(pHtmlStr->proxyBasic.uname));
                        break;
                    case www::wiisetting::FORM_ID_PROXY_BASIC_PASSWORD:
                        memcpy(pHtmlStr->proxyBasic.pass, buf, sizeof(pHtmlStr->proxyBasic.pass));
                        break;
                    case www::wiisetting::FORM_ID_PARENTAL_PASS:
                        memcpy(pHtmlStr->parentalPass, buf, sizeof(pHtmlStr->parentalPass));
                        break;
                    case www::wiisetting::FORM_ID_PARENTAL_RE_PASS:
                        memcpy(pHtmlStr->parentalRePass, buf, sizeof(pHtmlStr->parentalRePass));
                        break;
                    case www::wiisetting::FORM_ID_PARENTAL_JUDGE_PASS:
                        memcpy(pHtmlStr->parentalJudgePass, buf, sizeof(pHtmlStr->parentalJudgePass));
                        break;
                    case www::wiisetting::FORM_ID_PARENTAL_SEC_ANSWER:
                        memcpy(pHtmlStr->parentalSecA, buf, sizeof(pHtmlStr->parentalSecA));
                        break;
                    case www::wiisetting::FORM_ID_PARENTAL_RE_SEC_ANSWER:
                        memcpy(pHtmlStr->parentalReSecA, buf, sizeof(pHtmlStr->parentalReSecA));
                        break;
                    case www::wiisetting::FORM_ID_MASTER_KEY:
                        memcpy(pHtmlStr->masterKey, buf, sizeof(pHtmlStr->masterKey));
                        break;
                    case www::wiisetting::FORM_ID_ADJ_MTU:
                        memcpy(pHtmlStr->adjMtu, buf, sizeof(pHtmlStr->adjMtu));
                        break;
                }
            }
        }
        void Setting::initNickName() {
            bool nicknameRes = SCGetOwnerNickName(&mOwnerNickname);
            OSReport("SCGetOwnerNickName:%d\n", nicknameRes);
            if (nicknameRes != 0) {
                memcpy(mHtmlStrScratch, mOwnerNickname.name, (u32)mOwnerNickname.length << 1);
            }
            memset(pHtmlStr->nickname, 0, sizeof(pHtmlStr->nickname));
            utility::CharacterCode::UTF16ToUTF8(pHtmlStr->nickname, (wchar_t*)mHtmlStrScratch, sizeof(pHtmlStr->nickname));
        }
        void Setting::initSecurityKey() {
            memset(pHtmlStr->securityKey, 0, sizeof(pHtmlStr->securityKey));

            u32 keyLen;
            switch (ncd::NCDSetting::getNCDPrivacyMode()) {
                case 0:
                    keyLen = 0x00;
                    break;
                case 1:
                    keyLen = 0x05;
                    break;
                case 2:
                    keyLen = 0x0d;
                    break;
                case 4:
                case 5:
                case 6:
                    keyLen = 0x40;
                    break;
                default:
                    keyLen = 0;
                    break;
            }
            if (keyLen != 0) {
                memcpy(pHtmlStr->securityKey, ncd::NCDSetting::getPrivacy(), keyLen);
            }
            OSReport("privacy : %s\n", ncd::NCDSetting::getPrivacy());
        }
        void Setting::initSSID() {
            memset(pHtmlStr->ssid, 0, sizeof(pHtmlStr->ssid));
            utility::CharacterCode::ANSIToUTF8(pHtmlStr->ssid, ncd::NCDSetting::getSSID()->ssid, ncd::NCDSetting::getSSID()->ssidLength);
            OSReport("initHTMLText initString:%s length:%d\n", ncd::NCDSetting::getSSID()->ssid, ncd::NCDSetting::getSSID()->ssidLength);
        }
        void Setting::initIP() {
            memset(pHtmlStr->ip.addr, 0, sizeof(pHtmlStr->ip.addr));
            memset(pHtmlStr->ip.netmask, 0, sizeof(pHtmlStr->ip.netmask));
            memset(pHtmlStr->ip.gateway, 0, sizeof(pHtmlStr->ip.gateway));

            convertIP(pHtmlStr->ip.addr, ncd::NCDSetting::getIP()->addr);
            convertIP(pHtmlStr->ip.netmask, ncd::NCDSetting::getIP()->netmask);
            convertIP(pHtmlStr->ip.gateway, ncd::NCDSetting::getIP()->gateway);
        }
        void Setting::initDNS() {
            memset(pHtmlStr->dns1, 0, sizeof(pHtmlStr->dns1));
            memset(pHtmlStr->dns2, 0, sizeof(pHtmlStr->dns2));

            convertIP(pHtmlStr->dns1, ncd::NCDSetting::getIP()->dns1);
            convertIP(pHtmlStr->dns2, ncd::NCDSetting::getIP()->dns2);
        }
        void Setting::initProxy() {
            memset(pHtmlStr->proxy.server, 0, sizeof(pHtmlStr->proxy.server));
            memset(pHtmlStr->proxy.port, 0, sizeof(pHtmlStr->proxy.port));

            memcpy(pHtmlStr->proxy.server, ncd::NCDSetting::getProxy()->http.server, 0x100);
            sprintf(pHtmlStr->proxy.port, "%d", ncd::NCDSetting::getProxy()->http.port);
        }
        void Setting::initBasic() {
            memset(pHtmlStr->proxyBasic.uname, 0, sizeof(pHtmlStr->proxyBasic.uname));
            memset(pHtmlStr->proxyBasic.pass, 0, sizeof(pHtmlStr->proxyBasic.pass));

            memcpy(pHtmlStr->proxyBasic.uname, ncd::NCDSetting::getProxy()->http.username, 0x21);
            memcpy(pHtmlStr->proxyBasic.pass, ncd::NCDSetting::getProxy()->http.password, 0x21);
        }
        void Setting::initMTU() {
            u8 scratch[4];
            memset(pHtmlStr->adjMtu, 0, sizeof(pHtmlStr->adjMtu));

            sprintf((char*)scratch, "%d", ncd::NCDSetting::getMTU());
            utility::CharacterCode::ANSIToUTF8(pHtmlStr->adjMtu, scratch);
        }

        void Setting::initSecA() {
            wchar_t scratch[0x22];
            memset(pHtmlStr->parentalSecA, 0, sizeof(pHtmlStr->parentalSecA));
            memset(scratch, 0, sizeof(scratch));

            wcsncpy(scratch, parental::Parental::getSecA(), 0x20);
            memset(mHtmlStrScratch, 0, sizeof(mHtmlStrScratch));
            wcsncpy((wchar_t*)mHtmlStrScratch, scratch, 0x20);

            adjustSecA(scratch);
            utility::CharacterCode::UTF16ToUTF8(pHtmlStr->parentalSecA, scratch, 100);
        }

#define TITLE_ID(HI, LO_0, LO_1, LO_2, LO_3)                                                                                                         \
    (((u64)HI << 0x20) | ((u64)LO_0 << 0x18) | ((u64)LO_1 << 0x10) | ((u64)LO_2 << 0x08) | ((u64)LO_3 << 0x00))
        void Setting::initVersion() {
            const char* localRegionLetterBuf[] = {"J", "U", "E", "", "", "J", "K", "", "", "", "", "C"};
            ESTitleId localRegionTitleIDBuf[] = {
                TITLE_ID(0x00010008, 'H', 'A', 'K', 'J'),
                TITLE_ID(0x00010008, 'H', 'A', 'K', 'E'),
                TITLE_ID(0x00010008, 'H', 'A', 'K', 'P'),
                0,
                0,
                TITLE_ID(0x00010008, 'H', 'A', 'K', 'J'),
                TITLE_ID(0x00010008, 'H', 'A', 'K', 'K'),
                0,
                0,
                0,
                0,
                TITLE_ID(0x00010008, 'H', 'A', 'K', 'C'),
            };
            int region = System::getRegion();
            sprintf(pHtmlStr->version, "Ver. %d.%d%s", 4, 3, localRegionLetterBuf[region]);
            mTitleID = localRegionTitleIDBuf[region];
        }

        void Setting::setDisPos() {
            __VISetAdjustingValues((s8)(0x10 - pWiiSettingData->data[www::wiisetting::WB_ID_DIS_POS]), 0);
        }
        void Setting::setNickName() {
            mOwnerNickname.length = wcslen((wchar_t*)mHtmlStrScratch);
            if ((checkTextNum(pHtmlStr->nickname) & 0xff) == 3) {
                memset(mOwnerNickname.name, 0, sizeof(mOwnerNickname.name));
                memcpy(mOwnerNickname.name, mHtmlStrScratch, mOwnerNickname.length * sizeof(wchar_t));
                OSReport("nicknameFlag:1 %d %s\n", (bool)SCSetOwnerNickName(&mOwnerNickname), mOwnerNickname.name);
            }
        }
        void Setting::setSecurityKey() {
            int len;

            len = ncd::NCDSetting::checkWEPKey(pHtmlStr->securityKey);
            if (len >= 0) {
                www::wiisetting::setFuncResult(3);
                ncd::NCDSetting::setPrivacy((u8*)pHtmlStr->securityKey, len);
                OSReport("securityFlag:1 %s\n", pHtmlStr->securityKey);
            } else {
                System::getDialog()->callBtn0(MESG_SETTINGS_INCORRECT_INFORMATION, 60 * 3);
                mSceneState = 2;
                www::wiisetting::setFuncResult(4);
            }
        }
        void Setting::setSSID() {
            u8 ssidBuf[sizeof(pHtmlStr->ssid)];

            memset(ssidBuf, 0, sizeof(pHtmlStr->ssid));
            utility::CharacterCode::UTF8ToANSI(ssidBuf, pHtmlStr->ssid);
            memset(ssidBuf + 0x20, 0, sizeof(pHtmlStr->ssid) - 0x20);
            ncd::NCDSetting::setSSID((u8*)ssidBuf);
        }
        void Setting::setIP() {
            NCDIpProfile profile;

            memset(&profile, 0, sizeof(profile));
            convertRevIP(profile.addr, pHtmlStr->ip.addr);
            convertRevIP(profile.netmask, pHtmlStr->ip.netmask);
            convertRevIP(profile.gateway, pHtmlStr->ip.gateway);
            ncd::NCDSetting::setIP(&profile);
        }
        void Setting::setDNS() {
            NCDIpProfile profile;

            memset(&profile, 0, sizeof(profile));
            convertRevIP(profile.dns1, pHtmlStr->dns1);
            convertRevIP(profile.dns2, pHtmlStr->dns2);
            ncd::NCDSetting::setDNS(&profile);
        }
        void Setting::setProxy() {
            u32 portInt;
            wchar_t portBuf[6];
            NCDProxyServerProfile profile;

            char* portRef = pHtmlStr->proxy.port;

            memset(portBuf, 0, sizeof(portBuf));
            utility::CharacterCode::UTF8ToUTF16(portBuf, portRef, sizeof(portBuf) >> 1);
            utility::CharacterCode::UTF16ToU32(&portInt, portBuf);

            profile.port = portInt;
            if ((u16)portInt == 0) {
                www::wiisetting::setFuncResult(4);
                System::getDialog()->callBtn0(MESG_SETTINGS_INCORRECT_INFORMATION, 60 * 3);
                mSceneState = 2;
            } else if (ncd::NCDSetting::checkProxy(pHtmlStr->proxy.server)) {
                www::wiisetting::setFuncResult(3);
                memset(&profile.server, 0, sizeof(profile.server));
                utility::CharacterCode::UTF8ToANSI((u8*)profile.server, pHtmlStr->proxy.server);
                ncd::NCDSetting::setProxy(&profile);
            } else {
                www::wiisetting::setFuncResult(4);
                System::getDialog()->callBtn0(MESG_SETTINGS_INCORRECT_INFORMATION, 60 * 3);
                mSceneState = 2;
            }
        }
        void Setting::setBasic() {
            NCDProxyServerProfile profile;
            if (ncd::NCDSetting::checkProxyBasic(pHtmlStr->proxyBasic.uname) && ncd::NCDSetting::checkProxyBasic(pHtmlStr->proxyBasic.pass)) {
                www::wiisetting::setFuncResult(3);
                memset(&profile, 0, sizeof(profile));
                utility::CharacterCode::UTF8ToANSI((u8*)profile.username, pHtmlStr->proxyBasic.uname);
                utility::CharacterCode::UTF8ToANSI((u8*)profile.password, pHtmlStr->proxyBasic.pass);
                ncd::NCDSetting::setBasic(&profile);
            } else {
                www::wiisetting::setFuncResult(4);
                System::getDialog()->callBtn0(MESG_SETTINGS_INCORRECT_INFORMATION, 60 * 3);
                mSceneState = 2;
            }
        }
        void Setting::setMTU() {
            u32 mtuInt;
            wchar_t mutBuf[6];
            char* mtuRef;

            mtuRef = pHtmlStr->adjMtu;
            memset(mutBuf, 0, 0xc);
            utility::CharacterCode::UTF8ToUTF16(mutBuf, mtuRef, 6);
            utility::CharacterCode::UTF16ToU32(&mtuInt, mutBuf);

            int mtu = mtuInt & 0xffff;
            if (mtu < 0x240 || mtu > 1500)
                mtu = 0;
            ncd::NCDSetting::setMTU(mtu);

            pWiiSettingData->data[www::wiisetting::WB_ID_SET_STRING] = 0;
        }
        void Setting::setParePass() {
            char passBuf[sizeof(pHtmlStr->parentalPass)];

            memset(passBuf, 0, sizeof(passBuf));
            utility::CharacterCode::UTF8ToANSI((u8*)passBuf, pHtmlStr->parentalPass);
            if ((checkTextNum(passBuf) & 0xff) == 3) {
                parental::Parental::setPass(passBuf);
            }
            memset(pHtmlStr->parentalPass, 0, sizeof(pHtmlStr->parentalPass));
        }
        void Setting::setPareRePass() {
            char rePassBuf[sizeof(pHtmlStr->parentalRePass)];
            u8 funcResult;

            memset(rePassBuf, 0, sizeof(rePassBuf));
            funcResult = 2;
            utility::CharacterCode::UTF8ToANSI((u8*)rePassBuf, pHtmlStr->parentalRePass);
            if ((checkTextNum(rePassBuf) & 0xff) == 3) {
                if (parental::Parental::checkPass(rePassBuf)) {
                    funcResult = 1;
                }
                www::wiisetting::setFuncResult(funcResult);
            }
            memset(pHtmlStr->parentalRePass, 0, sizeof(pHtmlStr->parentalRePass));
        }
        void Setting::setPareJudgePass() {
            char judgePassBuf[sizeof(pHtmlStr->parentalJudgePass)];
            u8 funcResult;

            memset(judgePassBuf, 0, sizeof(judgePassBuf));
            funcResult = 2;
            utility::CharacterCode::UTF8ToANSI((u8*)judgePassBuf, pHtmlStr->parentalJudgePass);
            if ((checkTextNum(judgePassBuf) & 0xff) == 3) {
                if (parental::Parental::judgePass(judgePassBuf)) {
                    funcResult = 1;
                }
                www::wiisetting::setFuncResult(funcResult);
            }
            memset(pHtmlStr->parentalJudgePass, 0, sizeof(pHtmlStr->parentalJudgePass));
        }
        void Setting::setSecA() {
            memset(mHtmlStrScratch, 0, sizeof(mHtmlStrScratch));
            utility::CharacterCode::UTF8ToUTF16((wchar_t*)mHtmlStrScratch, pHtmlStr->parentalSecA, 0x44);
            reAdjustSecA();
            if ((checkTextNum(NULL) & 0xff) == 3) {
                parental::Parental::setSecA((wchar_t*)mHtmlStrScratch);
            }
            memset(mHtmlStrScratch, 0, sizeof(mHtmlStrScratch));
        }
        void Setting::setReSecA() {
            u8 funcResult;

            funcResult = 2;
            memset(mHtmlStrScratch, 0, sizeof(mHtmlStrScratch));
            utility::CharacterCode::UTF8ToUTF16((wchar_t*)mHtmlStrScratch, pHtmlStr->parentalReSecA, 0x44);
            reAdjustSecA();
            if ((checkTextNum(NULL) & 0xff) == 3) {
                if (parental::Parental::judgeSecA((wchar_t*)mHtmlStrScratch)) {
                    funcResult = 1;
                }
                www::wiisetting::setFuncResult(funcResult);
            }
            memset(mHtmlStrScratch, 0, sizeof(mHtmlStrScratch));
        }
        void Setting::setMasterKey() {
            char masterKeyBuf[sizeof(pHtmlStr->masterKey)];
            u8 funcResult;
            memset(masterKeyBuf, 0, sizeof(masterKeyBuf));
            funcResult = 2;
            if ((checkTextNum(pHtmlStr->masterKey) & 0xff) == '\x03') {
                utility::CharacterCode::UTF8ToANSI((u8*)masterKeyBuf, pHtmlStr->masterKey);
                if (parental::Parental::judgeMaster(masterKeyBuf)) {
                    funcResult = 1;
                }
                www::wiisetting::setFuncResult(funcResult);
            }
            memset(pHtmlStr->masterKey, 0, sizeof(pHtmlStr->masterKey));
            return;
        }

        u32 Setting::checkTextNum(const char* str) {
            u8 funcResult;
            u32 msgId;

            funcResult = 4;
            msgId = 0;
            switch (pWiiSettingData->data[www::wiisetting::WB_ID_SET_STRING]) {
                case 0x02:
                    if (wcslen((wchar_t*)mHtmlStrScratch)) {
                        if (checkSpace()) {
                            funcResult = 3;
                        } else {
                            msgId = MESG_SETTINGS_CANT_BE_ALL_SPACES;
                        }
                    } else {
                        msgId = MESG_SETTINGS_ENTER_CONSOLE_NICK;
                    }
                    break;

                case 0x0a:
                case 0x0b:
                case 0x0c:
                    if (strlen(str) == 4)
                        funcResult = 3;
                    msgId = MESG_SETTINGS_ENTER_LEN4_PIN;
                    break;

                case 0x0d:
                case 0x0e: {
                    u32 minLen;
                    switch (System::getRegion()) {
                        case SC_PRODUCT_AREA_JPN:
                        case SC_PRODUCT_AREA_CHN:
                            minLen = 3;
                            break;
                        case SC_PRODUCT_AREA_KOR:
                            minLen = 2;
                            break;
                        default:
                            minLen = 6;
                            break;
                    }
                    if (wcslen((wchar_t*)mHtmlStrScratch) >= minLen) {
                        if (checkSpace()) {
                            funcResult = 3;
                        } else {
                            msgId = MESG_SETTINGS_CANT_BE_ALL_SPACES;
                        }
                    } else {
                        msgId = MESG_SETTINGS_SECRET_ANSWER_TOO_SHORT;
                    }
                    break;
                }
                case 0x0f:
                    if (strlen(str) == 5)
                        funcResult = 3;
                    msgId = MESG_SETTINGS_INPUT_LEN5_MASTER_KEY;
                    break;
            }

            www::wiisetting::setFuncResult(funcResult);
            if (funcResult == 4) {
                System::getDialog()->callBtn0(msgId, 60 * 3);
                mSceneState = 2;
            }
            return funcResult;
        }

        bool Setting::checkSpace() {
            wchar_t c;
            for (int i = 0; (c = ((wchar_t*)mHtmlStrScratch)[i]); i++) {
                if ((c != L' ') && (c != L'　'))
                    return true;
            }
            return false;
        }
        void Setting::convertIP(char* str, const u8* ip) {
            u8 scratch[20];
            sprintf((char*)scratch, "%03d.%03d.%03d.%03d", ip[0], ip[1], ip[2], ip[3]);
            utility::CharacterCode::ANSIToUTF8(str, scratch);
        }
        void Setting::convertRevIP(u8* ip, const char* ansiStr) {
            int numStartOffset;
            int strI;
            int byteI;
            char str[0x14];

            byteI = 0;
            memset(str, 0, sizeof(str));
            utility::CharacterCode::UTF8ToANSI((u8*)str, ansiStr);

            for (strI = 0, numStartOffset = 0; strI < 0x10; strI++) {
                u8 c = str[strI];
                if (c != '.' && c != '\0')
                    continue;

                if (c == '\0')
                    strI = 0x10;

                str[strI] = '\0';
                u32 n = atoi(str + numStartOffset);
                if (n > 0xff) {
                    n = 0xff;
                }
                if (byteI == 0) {
                    ip[byteI] = n;
                } else {
                    ip[1] = ip[2];
                    ip[2] = ip[3];
                    ip[3] = n;
                }

                byteI++;
                numStartOffset = strI + 1;
                if (byteI == 3 && strI != 0x10) {
                    u32 n = atoi(str + numStartOffset);
                    if (n > 0xff) {
                        n = 0xff;
                    }
                    ip[1] = ip[2];
                    ip[2] = ip[3];
                    ip[3] = n;
                    return;
                }
            }
        }
        void Setting::adjustSecA(wchar_t* buf) {
            wchar_t scratch[0x18];
            int i;
            bool sawNonAscii;

            for (i = 0, sawNonAscii = false; buf[i] != 0; i++) {
                if (buf[i] > 0x7f) {
                    sawNonAscii = true;
                    break;
                }
            }
            if (sawNonAscii && wcslen((wchar_t*)mHtmlStrScratch) > 0x10) {
                memcpy(scratch, buf + 0x10, 0x22);
                memcpy(buf + 0x11, scratch, 0x22);
                buf[0x10] = L'\n';
            }
        }
        void Setting::reAdjustSecA() {
            char scratchSpace[0x24];
            int i = 0;
            bool hasMultiCharSymbol = false;
            while (((wchar_t*)mHtmlStrScratch)[i] != 0) {
                if (((wchar_t*)mHtmlStrScratch)[i] > 0x7f) {
                    hasMultiCharSymbol = true;
                    break;
                }
                i++;
            };
            if (hasMultiCharSymbol && wcslen((wchar_t*)this->mHtmlStrScratch) > 0x10) {
                memcpy(scratchSpace, this->mHtmlStrScratch + 0x22, 0x22);
                memcpy(this->mHtmlStrScratch + 0x20, scratchSpace, 0x22);
            }
        }

        void Setting::setDefaultBackString() {
            switch (pWiiSettingData->data[www::wiisetting::WB_ID_FORM_ID]) {
                case www::wiisetting::FORM_ID_NICKNAME:
                    System::getKeyboard()->baseMgr()->setTitleText(System::getMessage(0x15c));
                    break;
                case www::wiisetting::FORM_ID_SECURITY_KEY:
                    System::getKeyboard()->baseMgr()->setTitleText(System::getMessage(0x153));
                    break;
                case www::wiisetting::FORM_ID_SSID:
                    System::getKeyboard()->baseMgr()->setTitleText(System::getMessage(0x152));
                    break;
                case www::wiisetting::FORM_ID_IP_ADDR:
                    System::getKeyboard()->baseMgr()->setTitleText(System::getMessage(0x14d));
                    break;
                case www::wiisetting::FORM_ID_IP_NETMASK:
                    System::getKeyboard()->baseMgr()->setTitleText(System::getMessage(0x150));
                    break;
                case www::wiisetting::FORM_ID_IP_GATEWAY:
                    System::getKeyboard()->baseMgr()->setTitleText(System::getMessage(0x151));
                    break;
                case www::wiisetting::FORM_ID_DNS1:
                    System::getKeyboard()->baseMgr()->setTitleText(System::getMessage(0x14e));
                    break;
                case www::wiisetting::FORM_ID_DNS2:
                    System::getKeyboard()->baseMgr()->setTitleText(System::getMessage(0x14f));
                    break;
                case www::wiisetting::FORM_ID_PROXY_SERVER:
                    System::getKeyboard()->baseMgr()->setTitleText(System::getMessage(0x154));
                    break;
                case www::wiisetting::FORM_ID_PROXY_PORT:
                    System::getKeyboard()->baseMgr()->setTitleText(System::getMessage(0x155));
                    break;
                case www::wiisetting::FORM_ID_PROXY_BASIC_USERNAME:
                    System::getKeyboard()->baseMgr()->setTitleText(System::getMessage(0x156));
                    break;
                case www::wiisetting::FORM_ID_PROXY_BASIC_PASSWORD:
                    System::getKeyboard()->baseMgr()->setTitleText(System::getMessage(0x157));
                    break;
                case www::wiisetting::FORM_ID_ADJ_MTU:
                    System::getKeyboard()->baseMgr()->setTitleText(System::getMessage(0x158));
                    break;
                case www::wiisetting::FORM_ID_PARENTAL_PASS:
                    System::getKeyboard()->baseMgr()->setTitleText(System::getMessage(0x159));
                    break;
                case www::wiisetting::FORM_ID_PARENTAL_RE_PASS:
                    System::getKeyboard()->baseMgr()->setTitleText(System::getMessage(0x159));
                    break;
                case www::wiisetting::FORM_ID_PARENTAL_JUDGE_PASS:
                    System::getKeyboard()->baseMgr()->setTitleText(System::getMessage(0x159));
                    break;
                case www::wiisetting::FORM_ID_PARENTAL_SEC_ANSWER:
                    System::getKeyboard()->baseMgr()->setTitleText(System::getMessage(0x15a));
                    break;
                case www::wiisetting::FORM_ID_PARENTAL_RE_SEC_ANSWER:
                    System::getKeyboard()->baseMgr()->setTitleText(System::getMessage(0x15a));
                    break;
                case www::wiisetting::FORM_ID_MASTER_KEY:
                    System::getKeyboard()->baseMgr()->setTitleText(System::getMessage(0x15b));
                    break;
                case www::wiisetting::FORM_ID_DUMMY_SECURITY_KEY:
                    System::getKeyboard()->baseMgr()->setTitleText(System::getMessage(0x153));
                    break;
            }
        }

#pragma dont_inline on
        bool Setting::checkInputString(const wchar_t* s) {
            return !*s;
        }
#pragma dont_inline off

        bool Setting::checkIPString(const wchar_t* s) {
            wchar_t stackBlankIP[] = L"000.000.000.000";
            if (memcmp(s, stackBlankIP, 0x20) == 0 || checkInputString(s)) {
                return true;
            } else {
                return false;
            }
        }
        bool Setting::calcSafeMode() {
            nwc24::Manager* nwc24 = System::getNwc24Manager();
            if (nwc24)
                return true;

            switch (unk_0x094) {
                case 0:
                    resetFuncMsgQ();
                    System::getDialog()->callBtn1(MESG_SETTINGS_FEATURE_UNAVAILABLE_SAFE_MODE, MESG_CMN_OK);
                    www::wiisetting::setFuncResult(2);
                    mSceneState = 0x11;
                    unk_0x094 = 1;
                    break;
                case 1:
                    if (dialogHasResult()) {
                        unk_0xB9C = 1;
                        mSceneState = 0;
                        unk_0x094 = 0;
                    }
                    break;
            }
            return false;
        }

        void Setting::initAP() {
            mScanAPState = 1;
            apRelated_0x914 = 0;
            mApActiveAnim = ANIM_NONE;
        }
        void Setting::resetAP() {
            pLytMyAP->getAnim(ANIM_LIST_LOST)->play();
            if (apRelated_0x914 != 0) {
                pLytMyAP->getAnim(ANIM_ARW_A_LOST)->play();
            }
            if (((WDBssDesc*)mApBssDescriptorsBuf.data)->length != apRelated_0x914 + 4) {
                pLytMyAP->getAnim(ANIM_ARW_B_LOST)->play();
            }

            for (int i = ANIM_FOCUS_OFF_AP2; i <= ANIM_FOCUS_OFF_AP5; i++) {
                pLytMyAP->getAnim(i)->play();
            }

            initAP();
            mApActiveAnim = ANIM_LIST_LOST;
            pWiiSettingFlag->smthMsgData = 3;
            mScanAPState = 9;
        }
        void Setting::redrawAP() {
            mScanAPState = 4;
            apRelated_0x914 = 0;
            mApActiveAnim = ANIM_NONE;
            pWiiSettingFlag->smthMsgData = 3;

            pLytMyAP->getAnim(ANIM_LIST_APPEAR)->initFrame();
            for (int i = ANIM_FOCUS_OFF_AP2; i <= ANIM_FOCUS_OFF_AP5; i++) {
                pLytMyAP->getAnim(i)->play();
            }
            pLytMyAP->calc();
            unk_0x91E = false;
        }
        void Setting::scanAP() {
            BOOL animIsPlaying = FALSE;
            switch (mScanAPState) {
                case 1:
                    memset(mApBssDescriptorsBuf.data, 0, sizeof(mApBssDescriptorsBuf));
                    pAPScanThread->setResultData(mApBssDescriptorsBuf.data);
                    memset(pAPScanThreadStack, 0, 0x1000);
                    pAPScanThread->Create(pAPScanThreadStack, 0x1000, 0x12);
                    mScanAPState = 2;
                    unk_0x91E = false;
                    break;
                case 2:
                    if (unk_0x91E == true)
                        waitStart();
                    if (pAPScanThread->IsThreadTerminated()) {
                        pAPScanThread->WaitForThreadExit();
                        if (*mApBssDescriptorsBuf.data != 0) {
                            www::wiisetting::setFuncResult(1);
                            setAPDraw();
                            mScanAPState = 3;
                        } else {
                            www::wiisetting::setFuncResult(2);
                            initAP();
                            pLytMyAP->getAnim(ANIM_LIST_APPEAR)->initAnmFrame();
                            pLytMyAP->getAnim(ANIM_ARW_A_APPEAR)->initAnmFrame();
                            pLytMyAP->getAnim(ANIM_ARW_B_APPEAR)->initAnmFrame();
                        }
                        resetFuncMsgQ();
                        unk_0xB9C = 0;
                        waitFinish();
                    }
                    break;
                case 3:
                    if (pWiiSettingFlag->smthMsgData == 3) {
                        mScanAPState = 4;
                        unk_0x91E = false;
                    }
                    break;
                case 4:
                    initScroll();
                    setAPDraw();
                    pGuiManager->init();
                    break;
                case 5:
                    pGuiManager->update();
                    break;
                case 6:
                    unk_0xB9C = 1;
                    animIsPlaying |= pLytMyAP->getAnim(mApActiveAnim)->isPlaying();
                    if (!animIsPlaying) {
                        mScanAPState = 5;
                        setAPDraw();
                        if (unk_0x91C != 0) {
                            pLytMyAP->getAnim(ANIM_LIST_SCROLL_UP)->initAnmFrame();
                        } else {
                            pLytMyAP->getAnim(ANIM_LIST_SCROLL_DOWN)->initAnmFrame();
                        }
                        if (apRelated_0x914 == 0) {
                            pLytMyAP->GetRootPane()->FindPaneByName("N_AP1")->SetVisible(false);
                        } else if (apRelated_0x914 == 1) {
                            pLytMyAP->GetRootPane()->FindPaneByName("N_AP1")->SetVisible(true);
                        }
                        if (*mApBssDescriptorsBuf.data == apRelated_0x914 + 4) {
                            pLytMyAP->GetRootPane()->FindPaneByName("N_AP6")->SetVisible(false);
                        } else if (*mApBssDescriptorsBuf.data == apRelated_0x914 + 5) {
                            pLytMyAP->GetRootPane()->FindPaneByName("N_AP6")->SetVisible(true);
                        }
                        pLytMyAP->FindPaneByName("N_AP0")->SetVisible(true);
                        pLytMyAP->FindPaneByName("N_AP7")->SetVisible(true);
                    }
                    break;
                case 7:
                    unk_0xB9C = 1;
                    animIsPlaying |= pLytMyAP->getAnim(mApActiveAnim)->isPlaying();
                    if (!animIsPlaying) {
                        updateScroll();
                        mScanAPState = 6;
                    }
                    break;
                case 8:
                    resetAP();
                    unk_0x91E = false;
                    break;
                case 9:
                    animIsPlaying |= pLytMyAP->getAnim(mApActiveAnim)->isPlaying();
                    if (!animIsPlaying) {
                        resetFuncMsgQ();
                        mScanAPState = 1;
                        mApActiveAnim = ANIM_NONE;
                        pGuiManager->init();
                        pLytMyAP->getAnim(ANIM_LIST_APPEAR)->initFrame();
                        pLytMyAP->calc();
                    }
                    break;
            }
        }
        void Setting::initScroll() {
            if (unk_0x91E == false)
                return;

            pLytMyAP->FindPaneByName("N_AP2")->SetVisible(true);
            pLytMyAP->FindPaneByName("N_AP3")->SetVisible(true);
            pLytMyAP->FindPaneByName("N_AP4")->SetVisible(true);
            pLytMyAP->FindPaneByName("N_AP5")->SetVisible(true);
            pLytMyAP->FindPaneByName("N_AP6")->SetVisible(true);
            pLytMyAP->FindPaneByName("N_AP7")->SetVisible(true);

            switch (*mApBssDescriptorsBuf.data) {
                case 0:
                    pLytMyAP->FindPaneByName("N_AP2")->SetVisible(false);
                case 1:
                    pLytMyAP->FindPaneByName("N_AP3")->SetVisible(false);
                case 2:
                    pLytMyAP->FindPaneByName("N_AP4")->SetVisible(false);
                case 3:
                    pLytMyAP->FindPaneByName("N_AP5")->SetVisible(false);
                    pLytMyAP->FindPaneByName("N_AP6")->SetVisible(false);
                    pLytMyAP->FindPaneByName("N_AP7")->SetVisible(false);
                    break;
            }

            pLytMyAP->getAnim(ANIM_LIST_APPEAR)->play();
            pLytMyAP->getAnim(ANIM_ARW_A_APPEAR)->initAnmFrame();

            pLytMyAP->FindPaneByName(panes_B_Arw[0])->SetVisible(false);
            pLytMyAP->FindPaneByName(panes_N_AP[0])->SetVisible(false);

            if (*mApBssDescriptorsBuf.data <= apRelated_0x914 + 4) {
                pLytMyAP->FindPaneByName("N_ArwB")->SetVisible(false);
                pLytMyAP->FindPaneByName(panes_B_Arw[1])->SetVisible(false);

                for (int i = *mApBssDescriptorsBuf.data + 1; i < 6; i++) {
                    pLytMyAP->FindPaneByName(panes_N_AP[i])->SetVisible(false);
                }
            } else {
                pLytMyAP->FindPaneByName("N_ArwB")->SetVisible(true);
                pLytMyAP->FindPaneByName(panes_B_Arw[1])->SetVisible(true);
                pLytMyAP->getAnim(ANIM_ARW_B_APPEAR)->play();

                for (int i = 1; i < 6; i++) {
                    pLytMyAP->FindPaneByName(panes_N_AP[i])->SetVisible(true);
                }
            }
            mScanAPState = 6;
            mApActiveAnim = ANIM_LIST_APPEAR;
            unk_0x91E = false;
        }

        void Setting::updateScroll() {
            if (unk_0x91C == 0) {
                if (--apRelated_0x914 == 0) {
                    pLytMyAP->getAnim(ANIM_ARW_A_LOST)->play();

                    pLytMyAP->FindPaneByName(panes_B_Arw[0])->SetVisible(false);

                    for (int i = 0; i < 8; i++) {
                        pGuiManager->getPaneComponentByPane(pLytMyAP->FindPaneByName(panes_B_Arw[0]))->setPointed(i, false);
                    }
                }
                if (*mApBssDescriptorsBuf.data == apRelated_0x914 + 5) {
                    pLytMyAP->getAnim(ANIM_ARW_B_APPEAR)->play();
                    pLytMyAP->FindPaneByName(panes_B_Arw[1])->SetVisible(true);
                }
            } else {
                apRelated_0x914++;
                if (*mApBssDescriptorsBuf.data == apRelated_0x914 + 4) {
                    pLytMyAP->getAnim(ANIM_ARW_B_LOST)->play();

                    pLytMyAP->FindPaneByName(panes_B_Arw[1])->SetVisible(false);

                    for (int i = 0; i < 8; i++) {
                        pGuiManager->getPaneComponentByPane(pLytMyAP->FindPaneByName(panes_B_Arw[1]))->setPointed(i, false);
                    }
                }
                if (apRelated_0x914 == 1) {
                    pLytMyAP->getAnim(ANIM_ARW_A_APPEAR)->play();
                    pLytMyAP->FindPaneByName(panes_B_Arw[0])->SetVisible(true);
                }
            }

            pLytMyAP->getAnim(ANIM_LIST_SCROLL_UP)->stop();
            pLytMyAP->getAnim(ANIM_LIST_SCROLL_DOWN)->stop();

            mApActiveAnim = (ApAnimIdx)(ANIM_LIST_SCROLL_UP + unk_0x91C);
            pLytMyAP->getAnim(mApActiveAnim)->play();
        }
        void Setting::setAPDraw() {
            int resultOffset = 2;
            int i = 0;
            pBssDescriptors = (WDBssDesc*)(mApBssDescriptorsBuf.data + 1);
            for (int i = 0; i <= *mApBssDescriptorsBuf.data; i++) {
                resultOffset += pBssDescriptors->length * 2;
                if (resultOffset > 0x800) {
                    return;
                }
                pLytMyAP->getAnim(ANIM_LIST_SCROLL_UP)->stop();
                pLytMyAP->getAnim(ANIM_LIST_SCROLL_DOWN)->stop();

                u8 wdPrivacyMode = WDGetPrivacyMode(pBssDescriptors);

                char stackSSID[0x21];
                memcpy(stackSSID, pBssDescriptors->ssid, sizeof(pBssDescriptors->ssid));
                stackSSID[ARRAY_LENGTH(stackSSID) - 1] = '\0';

                wchar_t wcharSSID[0x21];
                memset(wcharSSID, 0, sizeof(wcharSSID));
                int baseAdjAPIdx = (i + 1) - apRelated_0x914;
                if (0 <= baseAdjAPIdx && baseAdjAPIdx <= 5) {
                    nw4r::lyt::TextBox* textBox = nw4r::ut::DynamicCast<nw4r::lyt::TextBox*>(pLytMyAP->FindPaneByName(panes_T_Name[baseAdjAPIdx]));
                    utility::CharacterCode::UTF8ToUTF16(wcharSSID, stackSSID, ARRAY_LENGTH(wcharSSID));
                    textBox->SetString(wcharSSID);
                    if (wdPrivacyMode == 0) {
                        pLytMyAP->getAnim(baseAdjAPIdx + ANIM_LOCK1_OFF)->play();
                        pLytMyAP->getAnim(baseAdjAPIdx + ANIM_LOCK1_ON)->stop();
                    } else {
                        pLytMyAP->getAnim(baseAdjAPIdx + ANIM_LOCK1_ON)->play();
                        pLytMyAP->getAnim(baseAdjAPIdx + ANIM_LOCK1_OFF)->stop();
                    }
                    pLytMyAP->getAnim(baseAdjAPIdx + ANIM_DENPA_ANM0_GRP1)->stop();
                    pLytMyAP->getAnim(baseAdjAPIdx + ANIM_DENPA_ANM1_GRP1)->stop();
                    pLytMyAP->getAnim(baseAdjAPIdx + ANIM_DENPA_ANM2_GRP1)->stop();
                    pLytMyAP->getAnim(baseAdjAPIdx + ANIM_DENPA_ANM3_GRP1)->stop();

                    pLytMyAP->getAnim(baseAdjAPIdx + ANIM_DENPA_ANM0_GRP1 + getRadioLevel(pBssDescriptors) * 6)->play();
                }
                pBssDescriptors = (WDBssDesc*)((u8*)mApBssDescriptorsBuf.data + resultOffset);
            }
        }

        int Setting::get_arw_no(const char* pane) {
            int ret = -1;
            for (int i = 0; i < (int)ARRAY_LENGTH(panes_B_Arw); i++) {
                if (strcmp(panes_B_Arw[i], pane) == 0) {
                    ret = i;
                    break;
                }
            }
            return ret;
        }
        int Setting::get_ap_no(const char* pane) {
            int ret = -1;
            for (int i = 0; i < (int)ARRAY_LENGTH(panes_B_AP); i++) {
                if (strcmp(panes_B_AP[i], pane) == 0) {
                    ret = i;
                    break;
                }
            }
            return ret;
        }
        void Setting::start_point_event(const char* pane) {
            int apNo = get_ap_no(pane);
            pLytMyAP->getAnim(ANIM_LIST_SCROLL_UP)->stop();
            pLytMyAP->getAnim(ANIM_LIST_SCROLL_DOWN)->stop();
            if (apNo != -1) {
                pLytMyAP->getAnim(apNo + ANIM_FOCUS_ON_AP2)->play();
                pWiiSettingData->data[www::wiisetting::WB_ID_SE] = 2;
                setSE();
                return;
            }

            int arwNo = get_arw_no(pane);
            if (arwNo != -1) {
                pLytMyAP->getAnim(arwNo + ANIM_ARW_A_FOCUS_ON)->play();
                pWiiSettingData->data[www::wiisetting::WB_ID_SE] = 2;
                setSE();
                return;
            }
        }
        void Setting::start_left_event(const char* pane) {
            int apNo = get_ap_no(pane);
            if (apNo != -1) {
                pLytMyAP->getAnim(apNo + ANIM_FOCUS_OFF_AP2)->play();
                return;
            }

            int arwNo = get_arw_no(pane);
            if (arwNo != -1) {
                pLytMyAP->getAnim(arwNo + ANIM_ARW_A_FOCUS_OFF)->play();
                return;
            }
        }
        void Setting::start_trig_event(const char* pane) {
            u8 ssidBuf[sizeof(pBssDescriptors->ssid) + 1];
            int apNo = get_ap_no(pane);
            int bssBufOffset = 2;
            if (apNo != -1) {
                for (int i = 0; i <= *mApBssDescriptorsBuf.data; i++) {
                    bssBufOffset = bssBufOffset + pBssDescriptors->length * 2;
                    if (bssBufOffset > (int)sizeof(mApBssDescriptorsBuf))
                        break;
                    if (i == apNo + apRelated_0x914 + 1) {
                        u8 privacyMode = WDGetPrivacyMode(pBssDescriptors);
                        memcpy(ssidBuf, pBssDescriptors->ssid, sizeof(pBssDescriptors->ssid));
                        ssidBuf[sizeof(ssidBuf) - 1] = '\0';
                        memset(pHtmlStr->securityKey, 0, sizeof(pHtmlStr->securityKey));
                        memset(pHtmlStr->ssid, 0, sizeof(pHtmlStr->ssid));
                        utility::CharacterCode::ANSIToUTF8(pHtmlStr->ssid, ssidBuf, sizeof(pBssDescriptors->ssid));
                        ncd::NCDSetting::setSSID(ssidBuf);
                        ncd::NCDSetting::setWDPrivacyMode(privacyMode);
                        if (privacyMode == 0) {
                            www::wiisetting::setFuncResult(2);
                        } else {
                            www::wiisetting::setFuncResult(1);
                        }
                        pWiiSettingData->data[www::wiisetting::WB_ID_SE] = 3;
                        setSE();
                        OSReport("SET DATA : %d %s %d\n", i, ssidBuf, privacyMode);
                        break;
                    }
                    pBssDescriptors = (WDBssDesc*)((u8*)mApBssDescriptorsBuf.data + bssBufOffset);
                }
                unk_0x91E = false;
                mScanAPState = 8;
                return;
            }
            int arwNo = get_arw_no(pane);
            if (arwNo != -1) {
                mApActiveAnim = (ApAnimIdx)(ANIM_ARW_A_SELECT + arwNo);
                pLytMyAP->getAnim(mApActiveAnim)->play();
                if (arwNo == 0) {
                    unk_0x91C = 0;
                    if (apRelated_0x914 == 1) {
                        pLytMyAP->GetRootPane()->FindPaneByName("N_AP0")->SetVisible(false);
                    }
                } else if (arwNo == 1) {
                    unk_0x91C = 1;
                    if (*mApBssDescriptorsBuf.data == apRelated_0x914 + 5) {
                        pLytMyAP->GetRootPane()->FindPaneByName("N_AP7")->SetVisible(false);
                    }
                }
                mScanAPState = 7;
                snd::getSystem()->startSE("WIPL_SE_BT_PUSH");
            }
        }

        u8 Setting::getRadioLevel(const WDBssDesc* bss) {
            u8 rssi = bss->rssi & 0xff;
            if (rssi >= 196)
                return 3;
            else if (rssi >= 181)
                return 2;
            else if (rssi >= 171)
                return 1;
            else
                return 0;
        }

        void Setting::setUseEULA_() {
            switch (mSetEulaState) {
                case SET_EULA_INIT:
                    setUseEULA_Init_();
                    break;
                case SET_EULA_START:
                    setUseEULA_Start_();
                    break;
                case SET_EULA_CANCEL:
                    setUseEULA_Cancel_();
                    break;
                case SET_EULA_WAIT_FOR_SOUND_STOP:
                    if (!snd::getSystem()->isSEActive("WIPL_SE_DECIDE")) {
                        mSetEulaState = SET_EULA_WAIT_STOP_MOTOR;
                    }
                    break;
                case SET_EULA_WAIT_STOP_MOTOR:
                    setUseEULA_WaitStopMotor_();
                    break;
                default:
                    break;
            }
        }
        void Setting::setUseEULA_Init_() {
            if (ncd::NCDSetting::getEnableFlag()) {
                mSetEulaState = SET_EULA_START;
            } else {
                if ((u32)System::getRegion() != SC_PRODUCT_AREA_EUR) {
                    System::getDialog()->callBtn2(MESG_SETTINGS_WC24_AND_SHOP_NEED_CONFIRMED_EULA, MESG_NETWORK_SETTINGS_BTN, MESG_CMN_QUIT);
                } else {
                    System::getDialog()->callBtn2(MESG_SETTINGS_WC24_AND_SHOP_NEED_CONFIRMED_EULA_EUR, MESG_NETWORK_SETTINGS_BTN, MESG_CMN_QUIT);
                }
                mSceneState = 9;
                mSetEulaState = SET_EULA_INIT;
                resetFuncMsgQ();
            }
            return;
        }
        void Setting::setUseEULA_Cancel_() {
            System::getDialog()->callBtn2(MESG_SETTINGS_ASK_ARE_YOU_SURE, MESG_CMN_NO, MESG_CMN_YES, true);
            mSceneState = 0xc;
            mSetEulaState = SET_EULA_INIT;
            resetFuncMsgQ();
        }
        void Setting::setUseEULA_Start_() {
            if (validateEULA_()) {
                for (int i = 0; i < 4; i++) {
                    controller::Interface* controller = System::getController(i);
                    if (controller != NULL)
                        controller->cancelRumbling();
                }
                System::getBS2Manager()->abort();
                System::stopReceiveSchedule();
                mSetEulaState = SET_EULA_WAIT_FOR_SOUND_STOP;
            }
        }
        void Setting::setUseEULA_WaitStopMotor_() {
            __WPADReconnect(TRUE);
            pWiiSettingData->data[www::wiisetting::WB_ID_FINISH] = 1;
            snd::getSystem()->stopAllSound(0x14);
            mSetEulaState = SET_EULA_DONE;
            resetFuncMsgQ();
            return;
        }
        bool Setting::validateEULA_() {
            bool eulaValid;
            ESTmdView* tmdView;
            ESError result;

            eulaValid = false;
            tmdView = NULL;
            result = utility::ESMisc::GetTmdView(System::getTreasureHeap(), this->mTitleID, &tmdView);
            if (result == -1025 || result == -106) {
                mSetEulaState = SET_EULA_INIT;
                mSceneState = 0xd;
                resetFuncMsgQ();
                System::getDialog()->callBtn2(MESG_SETTINGS_FUNCTIONALITY_REQUIRES_SYSTEM_UPDATE, MESG_CMN_OK, MESG_CMN_QUIT);
            } else if (result != 0) {
                System::getErrorHandler()->log("ES", result, "iplSetting.cpp", 0x10f1);
                System::getErrorHandler()->set(ErrorHandler::DEFAULT, MESG_ERR_FILE);
            } else {
                result = 0;
                if (!utility::ESMisc::ContentExist(tmdView, 1, &result) && result != 0) {
                    System::getErrorHandler()->log("ES", result, "iplSetting.cpp", 0x10fc);
                    System::getErrorHandler()->set(ErrorHandler::DEFAULT, MESG_ERR_FILE);
                }
                eulaValid = true;
            }
            if (tmdView != NULL) {
                System::getTreasureHeap()->free(tmdView);
            }
            return eulaValid;
        }

        void Setting::setUpdate_() {
            switch (mSetUpdateState) {
                case SET_UPDATE_INIT:
                    setUpdate_Init_();
                    break;
                case SET_UPDATE_WAIT_ACCEPT_DIALOG:
                    setUpdate_WaitAcceptDialog_();
                    break;
                case SET_UPDATE_CONNECT_TEST_START:
                    setUpdate_ConnectTestStart_();
                    break;
                case SET_UPDATE_CONNECT_TEST_CREATE_WAIT:
                    setUpdate_ConnectTestCreateWait_();
                    break;
                case SET_UPDATE_CONNECT_TEST_RUN:
                    setUpdate_ConnectTestRun_();
                    break;
                case SET_UPDATE_CONNECT_TEST_FAILED:
                    setUpdate_ConnectTestFailed_();
                    break;
                case SET_UPDATE_SUCCESS_DIALOG:
                    setUpdate_SuccessDialog_();
                    break;
                case SET_UPDATE_NO_UPDATE_DIALOG:
                    setUpdate_NoUpdateDialog_();
                    break;
                case SET_UPDATE_EULA_INIT:
                    setUpdate_EULAInit_();
                    break;
                case SET_UPDATE_UNK_9:
                    System::getDialog()->callBtn2(MESG_SETTINGS_ASK_ARE_YOU_SURE, MESG_CMN_NO, MESG_CMN_YES, true);
                    mSceneState = 10;
                    resetFuncMsgQ();
                    break;
                case SET_UPDATE_REBOOT_SYS:
                    setUpdate_Reboot_();
                    break;
            }
        }
        void Setting::setUpdate_Init_() {
            if (ncd::NCDSetting::getEnableFlag()) {
                www::wiisetting::setFuncResult(1);
                if ((u32)System::getRegion() == SC_PRODUCT_AREA_EUR) {
                    System::getDialog()->callBtn1Sml(MESG_SETTINGS_MODIFICATIONS_DISCLAIMER_EUR, MESG_SETTINGS_I_ACCEPT_EUR);
                } else {
                    System::getDialog()->callBtn1Sml(MESG_SETTINGS_MODIFICATIONS_DISCLAIMER, MESG_SETTINGS_I_ACCEPT);
                }
                mSetUpdateState = SET_UPDATE_WAIT_ACCEPT_DIALOG;
            } else {
                System::getDialog()->callBtn2(MESG_SETTINGS_CONFIGURE_INTERNET_TO_UPDATE, MESG_NETWORK_SETTINGS_BTN, MESG_CMN_QUIT);
                mSceneState = 9;
                resetFuncMsgQ();
            }
        }
        void Setting::setUpdate_WaitAcceptDialog_() {
            if (dialogHasResult())
                mSetUpdateState = SET_UPDATE_CONNECT_TEST_START;
        }
        void Setting::setUpdate_ConnectTestStart_() {
            if (System::getSceneManager()->getScene(SCENE_NAKAMURA_TEST) == NULL && www::wiisetting::getFuncResult() == 0) {
                waitStart();
                Base::createChildScene(SCENE_NAKAMURA_TEST, this, NULL, NULL);
                mSetUpdateState = SET_UPDATE_CONNECT_TEST_CREATE_WAIT;
            }
        }
        void Setting::setUpdate_ConnectTestCreateWait_() {
            if (System::getSceneManager()->getScene(SCENE_NAKAMURA_TEST) != NULL)
                mSetUpdateState = SET_UPDATE_CONNECT_TEST_RUN;
        }
        void Setting::setUpdate_ConnectTestRun_() {
            if (!isWaitPlaying()) {
                if (pWiiSettingFlag->err == 0) {
                    pWiiSettingFlag->smthMsgData = 9;
                    mNUPState = 3;
                } else if (System::getDialog()->getLastResult() == 1) {
                    mSetUpdateState = SET_UPDATE_CONNECT_TEST_START;
                } else if (System::getDialog()->getLastResult() == 2) {
                    www::wiisetting::setFuncResult(1);
                    mSetUpdateState = SET_UPDATE_CONNECT_TEST_FAILED;
                    mFrameCounter = 0;
                }
            }
        }
        void Setting::setUpdate_ConnectTestFailed_() {
            if (mFrameCounter == 0) {
                System::getDialog()->callBtn1(MESG_SETTINGS_UPDATE_NEEDS_INTERNET, MESG_BTN_WII_MENU);
                mFrameCounter++;
            } else if (dialogHasResult()) {
                mSetUpdateState = SET_UPDATE_REBOOT_SYS;
            }
        }
        void Setting::setUpdate_SuccessDialog_() {
            if (dialogHasResult()) {
                System::getDialog()->callBtn1(MESG_SETTINGS_UPDATES_COMPLETE, MESG_CMN_OK);
                mSetUpdateState = SET_UPDATE_EULA_INIT;
            }
        }
        void Setting::setUpdate_NoUpdateDialog_() {
            if (dialogHasResult()) {
                if (mFrameCounter == 0) {
                    System::getDialog()->callBtn1(MESG_SETTINGS_NO_UPDATES_AVAILABLE, MESG_BTN_WII_MENU);
                    mFrameCounter++;
                } else if (dialogHasResult()) {  // Yknow, some if this code is pretty questionable.
                    mSetUpdateState = SET_UPDATE_REBOOT_SYS;
                }
            }
        }
        void Setting::setUpdate_EULAInit_() {
            if (dialogHasResult()) {
                if (SCGetEULA()) {
                    mSetUpdateState = SET_UPDATE_REBOOT_SYS;
                } else {
                    mSetUpdateState = SET_UPDATE_INIT;
                    www::wiisetting::setFuncResult(1);
                    resetFuncMsgQ();
                }
            }
        }
        void Setting::setUpdate_Reboot_() {
            if (mFrameCounter == 60 * 3) {
                if (mStartId == ARG_SETUP || mStartId == ARG_UNK_5) {
                    SCSetConfigDoneFlag(1);
                    SCSetConfigDoneFlag2(1);
                    SCFlush();
                }
                unk_0xB5C = 1;
                System::getResetHandler()->reset();
            }
            mFrameCounter++;
        }

        u16 Setting::getProfileID() {
            u16 id;
            if (mSetUpdateState >= SET_UPDATE_CONNECT_TEST_CREATE_WAIT) {
                id = ncd::NCDSetting::getUseProfileID();
                ncd::NCDSetting::initSetID((u8)id);
                if ((u8)id == 3)
                    id = 0;
            } else {
                id = ncd::NCDSetting::getID();
            }
            return id;
        }

        int Setting::getUpdateTiming() {
            return mNUPState;
        }

        void Setting::setConnectTestResult(int param_2, int param_3, bool connectTestFlag, int supportCode) {
            mSupportCode = supportCode;
            if (connectTestFlag == 0 && param_2 == 3) {
                resetFuncMsgQ();
                mNUPState = 0;
                unk_0xB5C = 1;
            } else if (pWiiSettingFlag->smthMsgData == 9) {
                switch (param_2) {
                    case 1:
                        System::getDialog()->setProgBarLength(100);
                        mSetUpdateState = SET_UPDATE_SUCCESS_DIALOG;
                        mNUPState = 0;
                        pWiiSettingFlag->smthMsgData = 0x54;
                        SCSetUpdateType(2);
                        SCFlush();
                        break;
                    case 2:
                        System::getDialog()->terminate();
                        mNUPState = 6;
                        pWiiSettingFlag->err = param_3;
                        break;
                    case 3:
                        System::getDialog()->terminate();
                        if (!SCGetConfigDoneFlag2()) {
                            this->mSetUpdateState = SET_UPDATE_SUCCESS_DIALOG;
                        } else {
                            this->mSetUpdateState = SET_UPDATE_NO_UPDATE_DIALOG;
                            mFrameCounter = 0;
                        }
                        mNUPState = 0;
                        pWiiSettingFlag->smthMsgData = 0x54;
                        break;
                }
            } else {
                if (param_2 == 1) {
                    pWiiSettingFlag->err = 0;
                    ncd::NCDSetting::setConnectTestFlag(true);
                    System::reloadDownloadTask();
                } else {
                    if (this->mSetUpdateState == SET_UPDATE_INIT) {
                        mSceneState = 2;
                    }
                    pWiiSettingFlag->err = param_3;
                    makeErrorMessage();
                    ncd::NCDSetting::setConnectTestFlag(false);
                }
                waitFinish();
                www::wiisetting::setFuncResult(param_2 & 0xff);
            }
        }

        void Setting::setNUP() {
            switch (mNUPState) {
                case 0:
                    makeSupportCode();
                    mNUPState = 1;
                    break;
                case 1:
                    if (System::getDialog()->getLastResult() == DialogWindow::RESULT_LEFT_BUTTON) {
                        if ((u32)System::getRegion() == SC_PRODUCT_AREA_EUR) {
                            System::getDialog()->callBtn1Sml(MESG_SETTINGS_MODIFICATIONS_DISCLAIMER_EUR, MESG_SETTINGS_I_ACCEPT_EUR);
                        } else {
                            System::getDialog()->callBtn1Sml(MESG_SETTINGS_MODIFICATIONS_DISCLAIMER, MESG_SETTINGS_I_ACCEPT);
                        }
                        mNUPState = 2;
                    } else if (System::getDialog()->getLastResult() == DialogWindow::RESULT_RIGHT_BUTTON) {
                        www::wiisetting::setFuncResult(2);
                        mNUPState = 10;
                    }
                    break;
                case 2:
                    if (dialogHasResult())
                        mNUPState = 3;
                    break;
                case 3:
                    System::getDialog()->callBtnPrg(MESG_SETTINGS_UPDATING);
                    mNUPState = 4;
                    break;
                case 4: {
                    NakamuraTest* nakamuraTest = (NakamuraTest*)System::getSceneManager()->getScene(SCENE_NAKAMURA_TEST);
                    u64 completed = nakamuraTest->amtCompleted();
                    u64 total = nakamuraTest->amtTotal();
                    System::getDialog()->setProgBarLength(completed * 100 / total);
                    break;
                }
                case 5:
                    if (dialogHasResult()) {
                        System::getDialog()->callBtn0(MESG_SETTINGS_UPDATE_COMPLETE, 60 * 3);
                        mNUPState = 8;
                    }
                    break;
                case 6:
                    if (dialogHasResult()) {
                        makeErrorMessage();
                        mNUPState = 11;
                    }
                    break;
                case 7:
                    if (dialogHasResult()) {
                        System::getDialog()->callBtn1(MESG_SETTINGS_NO_UPDATES_AVAILABLE, MESG_BTN_WII_MENU);
                        mNUPState = 8;
                    }
                    break;
                case 8:
                    if (dialogHasResult()) {
                        mNUPState = 9;
                        mFrameCounter = 0;
                        if (mStartId == ARG_SETUP || mStartId == ARG_UNK_5) {
                            SCSetConfigDoneFlag(TRUE);
                            SCSetConfigDoneFlag2(TRUE);
                            SCFlush();
                        }
                    }
                    break;
                case 9:
                    if (mFrameCounter == 60 * 3) {
                        unk_0xB5C = 1;
                        System::getResetHandler()->reset();
                    }
                    mFrameCounter++;
                    break;
                case 11:
                    if (dialogHasResult()) {
                        mSetUpdateState = SET_UPDATE_CONNECT_TEST_FAILED;
                        pWiiSettingFlag->smthMsgData = 0x54;
                        mNUPState = 0;
                        mFrameCounter = 0;
                    }
                    break;
            }
            return;
        }
        void Setting::makeErrorMessage() {
            wchar_t errnoStr[8];
            wchar_t errMsg[0x140];
            const wchar_t* msgA = System::getMessageManager()->getMessage(MESG_ERROR_CODE);
            const wchar_t* msgB = System::getMessageManager()->getMessage(getErrorNum());

            u32 errno = pWiiSettingFlag->err;
            OSReport("error:%d\n", errno);
            swprintf(errnoStr, 8, L"%d\n", errno);

            memset(errMsg, 0, 0x280);

            u32 offset = 0;
            u32 componentLen;

            componentLen = wcslen(msgA);
            wcsncat(errMsg + offset, msgA, componentLen);
            offset += componentLen;

            componentLen = wcslen(errnoStr);
            wcsncat(errMsg + offset, errnoStr, componentLen);
            offset += componentLen;

            componentLen = wcslen(msgB);
            wcsncat(errMsg + offset, msgB, componentLen);
            offset += componentLen;

            errMsg[offset] = L'\0';
            if (pWiiSettingFlag->smthMsgData == 9) {
                if ((u32)System::getRegion() == SC_PRODUCT_AREA_EUR && (u32)System::getLanguage() == SC_LANG_GERMAN &&
                    getErrorNum() == MESG_ERROR_IP_COLLISION_EUR) {
                    System::getDialog()->callBtn1(errMsg, MESG_CMN_OK, 78.0f);
                } else {
                    System::getDialog()->callBtn1(errMsg, MESG_CMN_OK);
                }
            } else if (mSetUpdateState == SET_UPDATE_INIT) {
                if ((u32)System::getRegion() == SC_PRODUCT_AREA_EUR && (u32)System::getLanguage() == SC_LANG_GERMAN &&
                    getErrorNum() == MESG_ERROR_IP_COLLISION_EUR) {
                    System::getDialog()->callBtn1(errMsg, MESG_CMN_OK, 78.0f);
                } else {
                    System::getDialog()->callBtn1(errMsg, MESG_CMN_OK);
                }
            } else {
                System::getDialog()->callBtn2(errMsg, MESG_SETTINGS_TRY_AGAIN, MESG_CMN_QUIT);
            }
            return;
        }

        u32 Setting::getErrorNum() {
            if ((u32)System::getRegion() != SC_PRODUCT_AREA_EUR) {
                if (pWiiSettingFlag->err == 32001) {
                    return MESG_ERROR_UPD_SERVER;
                } else if (pWiiSettingFlag->err == 32002) {
                    return MESG_ERROR_UPD_INTERNET;
                } else if (pWiiSettingFlag->err == 32003) {
                    return MESG_ERROR_UPD_NAND_FULL;
                } else if (pWiiSettingFlag->err < 33000) {
                    return MESG_ERROR_UPD_UNKNOWN;
                } else if (pWiiSettingFlag->err < 50200) {
                    return MESG_ERROR_NCD_INTERNET;
                } else if (pWiiSettingFlag->err < 50300) {
                    return MESG_ERROR_NCD_NOSETUP;
                } else if (pWiiSettingFlag->err < 50400) {
                    return MESG_ERROR_NCD_INVALID;
                } else if (pWiiSettingFlag->err < 50500) {
                    return MESG_ERROR_NCD_LAN_INVALID;
                } else if (pWiiSettingFlag->err < 51040) {
                    return MESG_ERROR_NCD_WL_INVALID;
                } else if (pWiiSettingFlag->err < 51050) {
                    return MESG_ERROR_WIFI_USB_NOT_FOUND;
                } else if (pWiiSettingFlag->err < 51100) {
                    return MESG_ERROR_NCD_WL_INVALID;
                } else if (pWiiSettingFlag->err < 51400) {
                    return MESG_ERROR_NWC24_NETWORK;
                } else if (pWiiSettingFlag->err < 51500) {
                    return MESG_ERROR_INTERNET_ERROR_1;
                } else if (pWiiSettingFlag->err < 52100) {
                    return MESG_ERROR_INTERNET_ERROR_2;
                } else if (pWiiSettingFlag->err < 52200) {
                    return MESG_ERROR_INTERNET_ERROR_3;
                } else if (pWiiSettingFlag->err < 52300) {
                    return MESG_ERROR_SERVER_UNREACHABLE_1;
                } else if (pWiiSettingFlag->err < 52500) {
                    return MESG_ERROR_PROXY_UNREACHABLE;
                } else if (pWiiSettingFlag->err < 52600) {
                    return MESG_ERROR_UNAME_PASSWORD;
                } else if (pWiiSettingFlag->err < 52700) {
                    return MESG_ERROR_SERVER_UNREACHABLE_2;
                } else if (pWiiSettingFlag->err < 52800) {
                    return MESG_ERROR_IP_COLLISION;
                } else if (pWiiSettingFlag->err < 55000) {
                    return MESG_ERROR_NETWORK_DISCONNECT;
                } else if (pWiiSettingFlag->err > 100000) {
                    return MESG_ERROR_NWC24_SERVER;
                }
            } else {
                if (pWiiSettingFlag->err == 32001) {
                    return MESG_ERROR_UPD_SERVER_EUR;
                } else if (pWiiSettingFlag->err == 32002) {
                    return MESG_ERROR_UPD_INTERNET_EUR;
                } else if (pWiiSettingFlag->err == 32003) {
                    return MESG_ERROR_UPD_NAND_FULL_EUR;
                } else if (pWiiSettingFlag->err < 33000) {
                    return MESG_ERROR_UPD_UNKNOWN_EUR;
                } else if (pWiiSettingFlag->err < 50200) {
                    return MESG_ERROR_NCD_INTERNET_EUR;
                } else if (pWiiSettingFlag->err < 50300) {
                    return MESG_ERROR_NCD_NOSETUP_EUR;
                } else if (pWiiSettingFlag->err < 50400) {
                    return MESG_ERROR_NCD_INVALID_EUR;
                } else if (pWiiSettingFlag->err < 50500) {
                    return MESG_ERROR_NCD_LAN_INVALID_EUR;
                } else if (pWiiSettingFlag->err < 51040) {
                    return MESG_ERROR_NCD_WL_INVALID_EUR;
                } else if (pWiiSettingFlag->err < 51050) {
                    return 0x1b8;
                } else if (pWiiSettingFlag->err < 51100) {
                    return MESG_ERROR_NCD_WL_INVALID_EUR;
                } else if (pWiiSettingFlag->err < 51400) {
                    return MESG_ERROR_NWC24_NETWORK_EUR;
                } else if (pWiiSettingFlag->err < 51500) {
                    return MESG_ERROR_INTERNET_ERROR_1_EUR;
                } else if (pWiiSettingFlag->err < 52100) {
                    return MESG_ERROR_INTERNET_ERROR_2_EUR;
                } else if (pWiiSettingFlag->err < 52200) {
                    return MESG_ERROR_INTERNET_ERROR_3_EUR;
                } else if (pWiiSettingFlag->err < 52300) {
                    return MESG_ERROR_SERVER_UNREACHABLE_1_EUR;
                } else if (pWiiSettingFlag->err < 52500) {
                    return MESG_ERROR_PROXY_UNREACHABLE_EUR;
                } else if (pWiiSettingFlag->err < 0xcd78) {
                    return MESG_ERROR_UNAME_PASSWORD_EUR;
                } else if (pWiiSettingFlag->err < 0xcddc) {
                    return MESG_ERROR_SERVER_UNREACHABLE_2_EUR;
                } else if (pWiiSettingFlag->err < 0xce40) {
                    return MESG_ERROR_IP_COLLISION_EUR;
                } else if (pWiiSettingFlag->err < 55000) {
                    return MESG_ERROR_NETWORK_DISCONNECT_EUR;
                } else if (pWiiSettingFlag->err > 100000) {
                    return MESG_ERROR_NWC24_SERVER;
                } else {
                    // Unreachable
                }
            }
        }
        void Setting::makeSupportCode() {
            wchar_t codeStr[12];
            wchar_t supportCodeMsg[0x100];

            const wchar_t* msgA = System::getMessageManager()->getMessage(0x16d);
            const wchar_t* msgB = System::getMessageManager()->getMessage(0x1b9);
            swprintf(codeStr, ARRAY_LENGTH(codeStr), L"%d\n", mSupportCode);
            memset(supportCodeMsg, 0, sizeof(supportCodeMsg));

            u32 offset = 0;
            u32 componentLen;
            componentLen = wcslen(msgA);
            wcsncat(supportCodeMsg, msgA, componentLen);
            offset += componentLen + 1;
            supportCodeMsg[componentLen] = L'\n';
            supportCodeMsg[offset] = L'\0';

            componentLen = wcslen(msgB);
            wcsncat(supportCodeMsg + offset, msgB, componentLen);
            offset += componentLen;

            componentLen = wcslen(codeStr);
            wcsncat(supportCodeMsg + offset, codeStr, componentLen);
            offset += componentLen;

            supportCodeMsg[offset] = L'\0';
            System::getDialog()->callBtn2(supportCodeMsg, MESG_CMN_NO, MESG_CMN_YES, true);
        }

        void Setting::setInitializeResult(bool success, int errno) {
            waitFinish();
            if (success) {
                www::wiisetting::setFuncResult(1);
            } else {
                switch (errno) {
                    case -5:
                        System::getErrorHandler()->set(ErrorHandler::DEFAULT, MESG_ERR_NAND);
                        break;
                    case -2:
                        System::getErrorHandler()->log("NandSDWorker", -2, "iplSetting.cpp", 0x13a7);
                        System::getErrorHandler()->set(ErrorHandler::DEFAULT, MESG_ERR_FILE);
                        break;
                }
            }
        }

        void Setting::setUSBAP() {
            switch (mUsbApState) {
                case 1:
                    if (pUsbApThread->is()) {
                        bool getOwnerNickSuccess = SCGetOwnerNickName(&mOwnerNickname);
                        if (getOwnerNickSuccess) {
                            OSReport("USB SCGetOwnerNickName:%d\n", getOwnerNickSuccess);
                            pUsbApThread->setData((wchar_t*)mOwnerNickname.name, &unk_0x91D);
                            pUsbApThread->Init(pResultUSBAP, pUSBApBssDescriptorsBuf);
                            mUsbApState = 2;
                        } else {
                            www::wiisetting::setFuncResult(2);
                            resetFuncMsgQ();
                        }
                    }
                    break;
                case 2:
                    if (unk_0x91D) {
                        // TODO: Figure out why this is reaccessed.
                        www::wiisetting::setFuncResult(*(volatile u8*)&unk_0x91D);
                        mUsbApState = 1;
                        unk_0x91D = 0;
                        resetFuncMsgQ();
                    }
                    break;
            }
        }
        void Setting::cancelUSBAP() {
            switch (mUsbApState) {
                case 1:
                case 2:
                    pUsbApThread->cancel();
                    mUsbApState = 3;
                    break;
                case 3:
                    if (unk_0x91D || pUsbApThread->is()) {
                        www::wiisetting::setFuncResult(5);
                        mUsbApState = 1;
                        unk_0x91D = 0;
                        resetFuncMsgQ();
                    }
                    break;
            }
        }

        void Setting::AOSSProcess() {
            if (pWiiSettingFlag->smthMsgData == 0x22) {
                if (unk_0x088 != 3) {
                    System::getDialog()->callBtn1(MESG_SETTINGS_AOSS_SETUP_FAILED, MESG_CMN_OK);
                    unk_0x088 = 3;
                    mSceneState = 2;
                } else {
                    if (dialogHasResult()) {
                        unk_0x088 = 0;
                        www::wiisetting::setFuncResult(10);
                        resetFuncMsgQ();
                    }
                }
                return;
            } else {
                int finishResult;
                u32 stack_unk_0x88 = unk_0x088;
                bool isEq0x21 = pWiiSettingFlag->smthMsgData == 0x21;
                switch (stack_unk_0x88) {
                    case 0:
                        if (!snd::getSystem()->isSEActive("WIPL_SE_DECIDE")) {
                            if ((isEq0x21 == 0) && pAossThread->start()) {
                                unk_0x088 = 1;
                            } else {
                                www::wiisetting::setFuncResult(isEq0x21 ? 5 : 2);
                                resetFuncMsgQ();
                            }
                        }
                        break;
                    case 1:
                    case 2:
                        if (pAossThread->finish(&m_AOSSConfig, &finishResult)) {
                            unk_0x088 = 0;
                            if (isEq0x21) {
                                www::wiisetting::setFuncResult(5);
                            } else if (finishResult != 0) {
                                OSReport("m_AOSSThread : Terminated with Error(%d)\n", finishResult);
                                www::wiisetting::setFuncResult(2);
                            } else {
                                ncd::NCDSetting::getData();
                                ncd::NCDSetting::getID();
                                ncd::NCDSetting::setAOSSParams(m_AOSSConfig);
                                www::wiisetting::setFuncResult(1);
                            }
                            unk_0x088 = 0;
                            resetFuncMsgQ();
                        } else if (isEq0x21 && unk_0x088 != 2) {
                            pAossThread->cancel();
                            unk_0x088 = 2;
                        }
                        break;
                }
            }
        }
        void Setting::RakuProcess() {
            if (pWiiSettingFlag->smthMsgData == 0x2c) {
                if (unk_0x08C != 1) {
                    System::getDialog()->callBtn1(MESG_SETTINGS_AOSS_BLANK, MESG_CMN_OK);
                    unk_0x08C = 1;
                    mSceneState = 2;
                    return;
                }
                if (!dialogHasResult())
                    return;

                unk_0x08C = 0;
                www::wiisetting::setFuncResult(10);
                resetFuncMsgQ();
                return;
            }
            bool isEq0x2b = pWiiSettingFlag->smthMsgData == 0x2b;
            int state = pRakuRakuThread->getState();
            int finishState;
            if (isEq0x2b) {
                switch (state) {
                    case 0:
                    case 6:
                    case 7:
                        if (pRakuRakuThread->finish(NULL, NULL) == 0)
                            break;
                        www::wiisetting::setFuncResult(5);
                        resetFuncMsgQ();
                        break;
                    default:

                        pRakuRakuThread->cancel();
                        break;
                }
            } else {
                switch (state) {
                    case 0:
                        if (snd::getSystem()->isSEActive("WIPL_SE_DECIDE"))
                            break;

                        if (pRakuRakuThread->start() != 0)
                            break;
                        www::wiisetting::setFuncResult(isEq0x2b ? 5 : 2);
                        resetFuncMsgQ();
                        break;

                    case 4:
                        if (pWiiSettingFlag->smthMsgData != 0x28)
                            break;
                        www::wiisetting::setFuncResult(1);
                        resetFuncMsgQ();
                        break;
                    case 5:
                        if (pWiiSettingFlag->smthMsgData != 0x29)
                            break;
                        www::wiisetting::setFuncResult(1);
                        resetFuncMsgQ();
                        break;

                    case 6:
                        if (pRakuRakuThread->finish(&m_RakuConfig, &finishState) == 0)
                            break;
                        if (finishState != 1) {
                            www::wiisetting::setFuncResult(2);
                        } else {
                            ncd::NCDSetting::getData();
                            ncd::NCDSetting::getID();
                            ncd::NCDSetting::setRakuParams(m_RakuConfig);
                            www::wiisetting::setFuncResult(1);
                        }
                        resetFuncMsgQ();
                        break;
                    case 7:
                        if (pRakuRakuThread->finish(NULL, &finishState) == 0)
                            break;
                        www::wiisetting::setFuncResult(2);
                        resetFuncMsgQ();
                        break;
                }
            }
        }

        void Setting::waitStart() {
            pLytWaiting->getAnim(ANIM_ARW_A_APPEAR)->play();
            pLytWaiting->GetRootPane()->FindPaneByName("N_Wait")->SetVisible(true);
            snd::getSystem()->startSE("WIPL_SE_COPYING");
        }
        void Setting::waitFinish() {
            pLytWaiting->getAnim(ANIM_ARW_A_APPEAR)->stop();
            pLytWaiting->GetRootPane()->FindPaneByName("N_Wait")->SetVisible(false);
            snd::getSystem()->startSE("WIPL_SE_COPY_FINISH");
        }
        bool Setting::isWaitPlaying() {
            return pLytWaiting->getAnim(ANIM_ARW_A_APPEAR)->isPlaying();
        }

        void Setting::setSE() {
            if (unk_0xB9C < 10 && (u32)pWiiSettingData->data[www::wiisetting::WB_ID_SE] == '\x02') {
                pWiiSettingData->data[www::wiisetting::WB_ID_SE] = 0;
            }

            if ((pWiiSettingData->data[www::wiisetting::WB_ID_SE] == 0 || pWiiSettingData->data[www::wiisetting::WB_ID_SE] == 2) &&
                pWiiSettingData->data[www::wiisetting::WB_ID_EXCSE] != 0) {
                pWiiSettingData->data[www::wiisetting::WB_ID_SE] = pWiiSettingData->data[www::wiisetting::WB_ID_EXCSE];
            }

            switch (pWiiSettingData->data[www::wiisetting::WB_ID_SE]) {
                case 0x01:
                    snd::getSystem()->startSE("WIPL_SE_BT_PUSH");
                    break;
                case 0x02:
                    snd::getSystem()->startSE("WIPL_SE_BT_TARGETTING");
                    break;
                case 0x03:
                    snd::getSystem()->startSE("WIPL_SE_DECIDE");
                    break;
                case 0x04:
                    snd::getSystem()->startSE("WIPL_SE_CANCEL");
                    break;
                case 0x05:
                    snd::getSystem()->startSE("WIPL_SE_CHOICE_CHG");
                    break;
                case 0x06:
                    snd::getSystem()->startSE("WIPL_SE_CHAR_DELETE_ERROR");
                    break;
                case 0x0a:
                    snd::getSystem()->setOutputMode(snd::EAUDIO_OUTPUT_MODE_MONO);
                    snd::getSystem()->startSE("WIPL_SE_OUTPUT_MODE_SELECT");
                    break;
                case 0x0b:
                    snd::getSystem()->setOutputMode(snd::EAUDIO_OUTPUT_MODE_STEREO);
                    snd::getSystem()->startSE("WIPL_SE_OUTPUT_MODE_SELECT");
                    break;
                case 0x0c:
                    snd::getSystem()->setOutputMode(snd::EAUDIO_OUTPUT_MODE_SURROUND);
                    snd::getSystem()->startSE("WIPL_SE_OUTPUT_MODE_SELECT");
                    break;
                case 0x1e:
                    snd::getSystem()->setOutputMode(snd::EAUDIO_OUTPUT_MODE_MONO);
                    snd::getSystem()->startSE("WIPL_SE_CANCEL");
                    break;
                case 0x1f:
                    snd::getSystem()->setOutputMode(snd::EAUDIO_OUTPUT_MODE_STEREO);
                    snd::getSystem()->startSE("WIPL_SE_CANCEL");
                    break;
                case 0x20:
                    snd::getSystem()->setOutputMode(snd::EAUDIO_OUTPUT_MODE_SURROUND);
                    snd::getSystem()->startSE("WIPL_SE_CANCEL");
                    break;
            }

            u32 se = pWiiSettingData->data[www::wiisetting::WB_ID_SE];
            if (se != '\x02' && se != '\0' && (u32)pWiiSettingData->data[www::wiisetting::WB_ID_PAGE_ID] != '\x1e' &&
                pWiiSettingData->data[www::wiisetting::WB_ID_EXCSE] == 0) {
                unk_0xB9C = 0;
            } else if (se == '\x02') {
                controller::Interface* controller = System::getYoungController();
                if (controller) {
                    controller->rumble(0);
                }
            }
            pWiiSettingData->data[www::wiisetting::WB_ID_SE] = 0;
            pWiiSettingData->data[www::wiisetting::WB_ID_EXCSE] = 0;
        }
        void APEvent::onEvent(u32 compId, u32 event, void* data) {
            gui::PaneComponent* comp = (gui::PaneComponent*)mpManager->getComponent(compId);
            const char* paneName = comp->getPane()->GetName();
            switch (event) {
                case 1:
                    setting->start_point_event(paneName);
                    break;
                case 2:
                    setting->start_left_event(paneName);
                    break;
                case 0:
                    if (((controller::Interface*)data)->downTrg(controller::BTN_INTERACT))
                        setting->start_trig_event(paneName);
                    break;
            }
        }
        BOOL Setting::isResetAcceptable() const {
            return unk_0xB5C;
        }

        typedef struct {
            wchar_t digits[10];
        } SCNumber;
        static const SCNumber scNumber = {L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7', L'8', L'9'};
        static const SCNumber scNumber2 = {L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7', L'8', L'9'};

        int SCNUMBER_UNUSED_numToWTextA(u32 number) {
            SCNumber scNumberData = scNumber;
            wchar_t textDigits[10] = L"";
            wchar_t text[10] = L"";
            textDigits[0] = scNumberData.digits[number / 1000000000];
            textDigits[1] = scNumberData.digits[number / 100000000 % 10];
            textDigits[2] = scNumberData.digits[number / 10000000 % 10];
            textDigits[3] = scNumberData.digits[number / 1000000 % 10];
            textDigits[4] = scNumberData.digits[number / 100000 % 10];
            textDigits[5] = scNumberData.digits[number / 10000 % 10];
            textDigits[6] = scNumberData.digits[number / 1000 % 10];
            textDigits[7] = scNumberData.digits[number / 100 % 10];
            textDigits[8] = scNumberData.digits[number / 10 % 10];
            textDigits[9] = scNumberData.digits[number % 10];
            int zeroOffset;
            for (zeroOffset = 0; zeroOffset < 3; zeroOffset++)
                if (textDigits[zeroOffset] != L'0')
                    break;
            wcscpy(text, textDigits + zeroOffset);
        }
        int SCNUMBER_UNUSED_numToWTextB(u32 number) {
            SCNumber scNumberData = scNumber2;
            wchar_t textDigits[10] = L"";
            wchar_t text[10] = L"";
            textDigits[0] = scNumberData.digits[number / 1000000000];
            textDigits[1] = scNumberData.digits[number / 100000000 % 10];
            textDigits[2] = scNumberData.digits[number / 10000000 % 10];
            textDigits[3] = scNumberData.digits[number / 1000000 % 10];
            textDigits[4] = scNumberData.digits[number / 100000 % 10];
            textDigits[5] = scNumberData.digits[number / 10000 % 10];
            textDigits[6] = scNumberData.digits[number / 1000 % 10];
            textDigits[7] = scNumberData.digits[number / 100 % 10];
            textDigits[8] = scNumberData.digits[number / 10 % 10];
            textDigits[9] = scNumberData.digits[number % 10];
            int zeroOffset;
            for (zeroOffset = 0; zeroOffset < 3; zeroOffset++)
                if (textDigits[zeroOffset] != L'0')
                    break;
            wcscpy(text, textDigits + zeroOffset);
        }

        // const u16 scNumber[10] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
        // const u16 scNumber2[10] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

    }  // namespace scene
}  // namespace ipl
