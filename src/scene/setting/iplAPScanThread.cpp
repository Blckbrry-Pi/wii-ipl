#include "scene/setting/iplAPScanThread.h"

#include <string.h>

namespace ipl {
    namespace scene {
        APScanThread::APScanThread() {
        }
        APScanThread::~APScanThread() {
        }
        void* APScanThread::Run() {
            int scanOnceRet = WDScanOnce((u8*)mpBssDescriptorsResultData, 0x800, &mScanParams);
            if (scanOnceRet != 0) {
                OSReport("ERROR(%d): UpdateScanInfo\n", scanOnceRet);
            }
            // @bug this should return something
        }
        void APScanThread::setResultData(u16* resultData) {
            mpBssDescriptorsResultData = resultData;
            setParam();
        }
        void APScanThread::setParam() {
            mScanParams.channelBit = 0;
            mScanParams.maxChannelTime = 300;
            memset(mScanParams.bssid, 0xff, sizeof(mScanParams.bssid));
            mScanParams.type = 0;
            mScanParams.ssidLength = 0;
            memset(mScanParams.ssid, 0, sizeof(mScanParams.ssid));
            memset(mScanParams.ssidMask, 0xff, sizeof(mScanParams.ssidMask));

            int checkEnableChannelRet = WDCheckEnableChannel(&mScanParams.channelBit);
            if (checkEnableChannelRet != 0) {
                OSReport("ERROR(%d): StartupScanParam\n", checkEnableChannelRet);
            }
        }
    }  // namespace scene
}  // namespace ipl
