#include "scene/setting/iplSensitivity.h"

#include "revolution/kpad.h"
#include "system/iplSystem.h"

namespace ipl {
    namespace SensitivityDrawing {
#define X_RANGE (WPAD_MAX_DPD_X + 1)
#define Y_RANGE (WPAD_MAX_DPD_Y + 1)
#define X_HALFRANGE (X_RANGE >> 1)
#define Y_HALFRANGE (Y_RANGE >> 1)
#define OFFS -45

        f32 getVertScale(f32 height) {
            return 0.5f * height / Y_RANGE;
        }
        f32 getHoriScale(f32 width) {
            return 0.5f * width / X_RANGE;
        }
        f32 calcOffsetSize(u16 size) {
            return size + 25;
        }
        f32 calcDispY(s16 irY, f32 horiScale) {
            return (irY - Y_HALFRANGE) * horiScale;
        }
        f32 calcDispX(s16 irX, f32 horiScale) {
            return (irX - X_HALFRANGE) * horiScale;
        }

        void draw(nand::File* cursorTpl) {
            GXRenderModeObj renderModeObj = *System::getRenderModeObj();

            nw4r::ut::Rect projRect;
            System::getProjectionRect(&projRect);

            GXColor grayAreaBg = {0x60, 0x60, 0x60, 0xc0};

            u32 savedScissorL, savedScissorT, savedScissorW, savedScissorH;
            GXGetScissor(&savedScissorL, &savedScissorT, &savedScissorW, &savedScissorH);

            // Set up scissor for the gray IR display area
            {
                f32 projHalfW = 0.5f * projRect.GetWidth();
                f32 projHalfH = 0.5f * projRect.GetHeight();
                f32 fbHoriScale = renderModeObj.fbWidth / projRect.GetWidth();
                f32 fbVertScale = renderModeObj.efbHeight / projRect.GetHeight();
                utility::Graphics::setDefaultOrtho(0);

                f32 projH, projW;
                projH = projRect.GetHeight();
                projW = projRect.GetWidth();
                u32 scissorT, scissorW, scissorH, scissorL;
                scissorH = fbVertScale * (0.5f * projH);
                scissorW = fbHoriScale * (0.5f * projW);
                scissorT = fbVertScale * (0.5f * projRect.top + projHalfH + OFFS);
                scissorL = fbHoriScale * (0.5f * projRect.left + projHalfW);
                GXSetScissor(scissorL, scissorT, scissorW, scissorH);
            }

            utility::Graphics::drawPolygon(projRect, grayAreaBg);
            controller::Interface* controller = System::getYoungController();
            if (controller != NULL && controller->getKPADStatus() != NULL) {
                GXTexObj texObj;
                TPLGetGXTexObjFromPalette((TPLPalette*)cursorTpl->getBuffer(), &texObj, 1);
                GXInitTexObjWrapMode(&texObj, GX_MIRROR, GX_MIRROR);
                // f32 horiScale = getHoriScale(projRect.GetWidth());
                // f32 vertScale = getVertScale(projRect.GetHeight());
                f32 horiScale = 0.5f * projRect.GetWidth() / X_RANGE;
                f32 vertScale = 0.5f * projRect.GetHeight() / Y_RANGE;

                u32 latestIdx = WPADGetLatestIndexInBuf(controller->getChannel());

                short* ringBuffer;
                DPDObject* curr;
                DPDObject* start;
                switch (controller->getType()) {
                    case WPAD_DEV_CORE: {
                        WPADStatus* status = KPADGetWPADRingBuffer(controller->getChannel());
                        curr = &status[latestIdx].obj[3];
                        start = &status[latestIdx].obj[0];
                        break;
                    }
                    case WPAD_DEV_FREESTYLE: {
                        WPADFSStatus* status = KPADGetWPADFSRingBuffer(controller->getChannel());
                        curr = &status[latestIdx].obj[3];
                        start = &status[latestIdx].obj[0];
                        break;
                    }
                    case WPAD_DEV_CLASSIC: {
                        WPADCLStatus* status = KPADGetWPADCLRingBuffer(controller->getChannel());
                        curr = &status[latestIdx].obj[3];
                        start = &status[latestIdx].obj[0];
                        break;
                    }
                }

                do {
                    if (curr->size != 0) {
                        f32 halfSize;
                        f32 dispX, dispY;
                        halfSize = 0.15f * (curr->size + 25);
                        dispX = (curr->x - X_HALFRANGE) * horiScale;
                        dispY = -((curr->y - Y_HALFRANGE) * vertScale);
                        nw4r::ut::Rect texRect(dispX - halfSize, dispY + halfSize - OFFS, dispX + halfSize, dispY - halfSize - OFFS);
                        utility::Graphics::drawTexture(texRect, texObj, (GXColor){0xff, 0xff, 0xff, 0xff}, 2);
                    }
                } while (--curr >= start);
            }
            GXSetScissor(savedScissorL, savedScissorT, savedScissorW, savedScissorH);
        }

    }  // namespace SensitivityDrawing
}  // namespace ipl
