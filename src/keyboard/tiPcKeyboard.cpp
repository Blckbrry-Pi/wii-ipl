#include "keyboard/tiPcKeyboard.h"

#include "keyboard/tiLanguageIndependentData.h"
#include "keyboard/tiLayoutGather.h"
#include "keyboard/tiManager.h"

#include <revolution/kbd/hidtypes.h>

#include <new>

namespace textinput {
    namespace keyboard {
        namespace pctype {
#define NUM_CONTROL_KEYS 23
#define NUM_PANE_CHARS 50
#define NUM_GRID_ENTRIES 56

            struct AnimationFile {
                AnimationId id;       // 0x00
                char fileName[0x40];  // 0x04
            };

            struct PaneToAnimation {
                KeyType type;                     // 0x00
                const char* paneName;             // 0x04
                u32 count;                        // 0x08
                const char* forceAddName;         // 0x0C
                const AnimationFile* pAnims[12];  // 0x10
            private:
                const char* getForceAddName() const { return forceAddName; }
            };

            struct GridData {
                enum GridDataIdx {
                    GD_IDX_DIARESIS = 0,
                    GD_IDX_PUNCT,
                    GD_IDX_HIRAGANA,
                    GD_IDX_KATAKANA,
                };
                char szPaneName[18];  // 0x00
                wchar_t wc[4];        // 0x12
            };
            struct PaneVisibility {
                u16 visibleCount;                // 0x00
                u16 invisibleCount;              // 0x02
                const char* visibleNames[24];    // 0x04
                const char* invisibleNames[24];  // 0x64
            };
            struct CharCodeToAblaut {
                wchar_t chCode;      // 0x0
                wchar_t ablauts[7];  // 0x2
            };

            // clang-format off
            const wchar_t* csszPredictLanguage[] = {L"", L"", L"Eng", L"Fra", L"Esp", L"EN", L"DE", L"FR", L"ES", L"IT", L"NL", L"CN", L"KR"};

            extern const GridData csGridKeyboard[];
            const GridData csGridKeyboard[56] = {
                {"P_Gkey_00", {L'à',  L'!',  L'\0', L'\0'}},
                {"P_Gkey_01", {L'á',  L'?',  L'あ', L'ア'}},
                {"P_Gkey_02", {L'â',  L'&',  L'か', L'カ'}},
                {"P_Gkey_03", {L'ä',  L'″',  L'さ', L'サ'}},
                {"P_Gkey_04", {L'è',  L'\'', L'た', L'タ'}},
                {"P_Gkey_05", {L'é',  L'～',  L'な', L'ナ'}},
                {"P_Gkey_06", {L'ê',  L':',  L'は', L'ハ'}},
                {"P_Gkey_07", {L'ë',  L';',  L'ま', L'マ'}},
                {"P_Gkey_08", {L'ì',  L'@',  L'や', L'ヤ'}},
                {"P_Gkey_09", {L'í',  L'~',  L'ら', L'ラ'}},
                {"P_Gkey_10", {L'î',  L'_',  L'わ', L'ワ'}},
                {"P_Gkey_11", {L'à',  L'+',  L'\0', L'\0'}},
                {"P_Gkey_12", {L'á',  L'-',  L'い', L'イ'}},
                {"P_Gkey_13", {L'â',  L'*',  L'き', L'キ'}},
                {"P_Gkey_14", {L'ä',  L'/',  L'し', L'シ'}},
                {"P_Gkey_15", {L'è',  L'×',  L'ち', L'チ'}},
                {"P_Gkey_16", {L'é',  L'÷',  L'に', L'ニ'}},
                {"P_Gkey_17", {L'ê',  L'=',  L'ひ', L'ヒ'}},
                {"P_Gkey_18", {L'ë',  L'→',  L'み', L'ミ'}},
                {"P_Gkey_19", {L'ì',  L'←',  L'ゆ', L'ユ'}},
                {"P_Gkey_20", {L'í',  L'↑',  L'り', L'リ'}},
                {"P_Gkey_21", {L'î',  L'↓',  L'を', L'ヲ'}},
                {"P_Gkey_22", {L'ñ',  L'「',  L'\0', L'\0'}},
                {"P_Gkey_23", {L'ß',  L'」',  L'う', L'ウ'}},
                {"P_Gkey_24", {L'À',  L'“',  L'く', L'ク'}},
                {"P_Gkey_25", {L'Á',  L'”',  L'す', L'ス'}},
                {"P_Gkey_26", {L'Â',  L'(',  L'つ', L'ツ'}},
                {"P_Gkey_27", {L'Ä',  L')',  L'ぬ', L'ヌ'}},
                {"P_Gkey_28", {L'È',  L'<',  L'ふ', L'フ'}},
                {"P_Gkey_29", {L'É',  L'>',  L'む', L'ム'}},
                {"P_Gkey_30", {L'Ê',  L'{',  L'よ', L'ヨ'}},
                {"P_Gkey_31", {L'Ë',  L'}',  L'る', L'ル'}},
                {"P_Gkey_32", {L'Ì',  L'•',  L'ん', L'ン'}},
                {"P_Gkey_33", {L'Í',  L'%',  L'\0', L'\0'}},
                {"P_Gkey_34", {L'Î',  L'※',  L'え', L'エ'}},
                {"P_Gkey_35", {L'Ï',  L'〒',  L'け', L'ケ'}},
                {"P_Gkey_36", {L'Ò',  L'#',  L'せ', L'セ'}},
                {"P_Gkey_37", {L'Ó',  L'♭',  L'て', L'テ'}},
                {"P_Gkey_38", {L'Ô',  L'♪',  L'ね', L'ネ'}},
                {"P_Gkey_39", {L'Ö',  L'±',  L'へ', L'ヘ'}},
                {"P_Gkey_40", {L'Œ',  L'$',  L'め', L'メ'}},
                {"P_Gkey_41", {L'Ù',  L'¢',  L'！', L'！'}},
                {"P_Gkey_42", {L'Ú',  L'£',  L'れ', L'レ'}},
                {"P_Gkey_43", {L'Û',  L'⁜',  L'、', L'、'}},
                {"P_Gkey_44", {L'Ü',  L'^',  L'\0', L'\0'}},
                {"P_Gkey_45", {L'Ç',  L'°',  L'お', L'オ'}},
                {"P_Gkey_46", {L'Ñ',  L'｜',  L'こ', L'コ'}},
                {"P_Gkey_47", {L'¡',  L'／',  L'そ', L'ソ'}},
                {"P_Gkey_48", {L'¿',  L'＼',  L'と', L'ト'}},
                {"P_Gkey_49", {L'¬',  L'∞',  L'の', L'ノ'}},
                {"P_Gkey_50", {L'¢',  L'∴',  L'ほ', L'ホ'}},
                {"P_Gkey_51", {L'£',  L'…',  L'も', L'モ'}},
                {"P_Gkey_52", {L'§',  L'™',  L'？', L'？'}},
                {"P_Gkey_53", {L'\0', L'©',  L'ろ', L'ロ'}},
                {"P_Gkey_54", {L'\0', L'®',  L'。', L'。'}},
                {"P_Gkey_55", {L'¢',  L'∴',  L'ー', L'ー'}},
            };

            extern const PaneNameToControlKey csPaneNameToControlKey[];
            const PaneNameToControlKey csPaneNameToControlKey[] = {
                {"P_key_LF",         VCODE_RETURN},
                {"P_key_DELETE",     VCODE_DELETE},
                {"P_key_CAPS",       VCODE_CAPS},
                {"P_key_SHIFT",      VCODE_SHIFT},
                {"P_key_SPACE",      CODE_UNK_0x02},

                {"P_Gkey_LF",        VCODE_RETURN},
                {"P_Gkey_DELETE",    VCODE_DELETE},
                {"P_Gkey_SPACE",     CODE_UNK_0x02},

                {"W_USEU_prdc_lang", CODE_UNK_0x0A},
                {"W_USEU_Chng_sign", CODE_UNK_0x0D},
                {"W_JP_Chng_ABC",    CODE_UNK_0x0B},
                {"W_JP_Chng_KANA",   VCODE_QWERTY_OFF},
                {"W_JP_Chng_sign",   CODE_UNK_0x0D},
                {"P_Mode_roma_hira", VCODE_TRANSLATEMODE_1},
                {"P_Mode_roma_kata", VCODE_TRANSLATEMODE_2},
                {"P_Mode_direct",    VCODE_TRANSLATEMODE_0},
                {"P_hiragana",       CODE_UNK_0x12},
                {"P_katakana",       CODE_UNK_0x13},
                {"P_Gkey_komoji",    VCODE_CONVERT_SMALL},
                {"P_Gkey_handaku",   VCODE_CONVERT_HANDAKU},
                {"P_Gkey_dakuten",   VCODE_CONVERT_DAKUTEN},

                {"P_Mode_kr_eng",    VCODE_TRANSLATEMODE_0},
                {"P_Mode_kr_han",    VCODE_TRANSLATEMODE_1},
            };

            extern const PaneVisibility csJPNAsciiVisiblePanes;
            extern const PaneVisibility csUSAsciiVisiblePanes;
            extern const PaneVisibility csEngAsciiVisiblePanesUK;
            extern const PaneVisibility csEngAsciiVisiblePanesSP;
            extern const PaneVisibility csAsciiVisiblePanes;
            extern const PaneVisibility csAsciiVisiblePanesNL;
            extern const PaneVisibility csCNAsciiVisiblePanes;
            extern const PaneVisibility csKRAsciiVisiblePanes;
            const PaneVisibility csJPNAsciiVisiblePanes = {
                13, 24,
                {
                    "N_modeSelect_all",
                    "P_Mode_direct", "P_Mode_roma_hira", "P_Mode_roma_kata",
                    "N_KeyChange_JP",
                    "W_JP_Chng_ABC", "W_JP_Chng_KANA", "W_JP_Chng_sign",
                    "T_JP_Chng_ABC", "T_JP_Chng_KANA", "T_JP_Chng_sign",
                    "T_key_CAPS", "T_key_SHIFT",
                },
                {
                    "P_key_41", "T_key_41", "P_key_42", "T_key_42", "P_key_43", "T_key_43",
                    "N_KeyChange_USEU",
                    "P_Gkey_00", "P_Gkey_11", "P_Gkey_22", "P_Gkey_33", "P_Gkey_44",
                    "T_Gkey_00", "T_Gkey_11", "T_Gkey_22", "T_Gkey_33", "T_Gkey_44",
                    "W_USEU_prdc_lang", "W_USEU_Chng_sign", "T_USEU_prdc_lang", "T_USEU_Chng_sign",
                    "P_SHIFTMark", "P_CAPSMark",
                    "N_modeSelect_kr",
                },
            };
            const PaneVisibility csUSAsciiVisiblePanes = {
                17, 20,
                {
                    "N_KeyChange_USEU",
                    "P_Gkey_00", "P_Gkey_11", "P_Gkey_22", "P_Gkey_33", "P_Gkey_44",
                    "T_Gkey_00", "T_Gkey_11", "T_Gkey_22", "T_Gkey_33", "T_Gkey_44",
                    "W_USEU_prdc_lang", "W_USEU_Chng_sign", "T_USEU_prdc_lang", "T_USEU_Chng_sign",
                    "T_key_CAPS", "T_key_SHIFT",
                },
                {
                    "P_key_41", "T_key_41", "P_key_42", "T_key_42", "P_key_43", "T_key_43",
                    "N_modeSelect_all",
                    "P_Mode_direct", "P_Mode_roma_hira", "P_Mode_roma_kata",
                    "N_KeyChange_JP",
                    "W_JP_Chng_ABC", "W_JP_Chng_KANA", "W_JP_Chng_sign",
                    "T_JP_Chng_ABC", "T_JP_Chng_KANA", "T_JP_Chng_sign",
                    "P_SHIFTMark", "P_CAPSMark",
                    "N_modeSelect_kr",
                },
            };
            const PaneVisibility csEngAsciiVisiblePanesUK = {
                21, 16,
                {
                    "P_key_42", "T_key_42", "P_key_43", "T_key_43",
                    "N_KeyChange_USEU",
                    "P_Gkey_00", "P_Gkey_11", "P_Gkey_22", "P_Gkey_33", "P_Gkey_44",
                    "T_Gkey_00", "T_Gkey_11", "T_Gkey_22", "T_Gkey_33", "T_Gkey_44",
                    "W_USEU_prdc_lang", "W_USEU_Chng_sign", "T_USEU_prdc_lang", "T_USEU_Chng_sign",
                    "T_key_CAPS", "T_key_SHIFT",
                },
                {
                    "P_key_41", "T_key_41",
                    "N_modeSelect_all",
                    "P_Mode_direct", "P_Mode_roma_hira", "P_Mode_roma_kata",
                    "N_KeyChange_JP",
                    "W_JP_Chng_ABC", "W_JP_Chng_KANA", "W_JP_Chng_sign",
                    "T_JP_Chng_ABC", "T_JP_Chng_KANA", "T_JP_Chng_sign",
                    "P_SHIFTMark", "P_CAPSMark",
                    "N_modeSelect_kr",
                },
            };
            const PaneVisibility csEngAsciiVisiblePanesSP = {
                21, 16,
                {
                    "P_key_42", "T_key_42", "P_key_43", "T_key_43",
                    "N_KeyChange_USEU",
                    "P_Gkey_00", "P_Gkey_11", "P_Gkey_22", "P_Gkey_33", "P_Gkey_44",
                    "T_Gkey_00", "T_Gkey_11", "T_Gkey_22", "T_Gkey_33", "T_Gkey_44",
                    "W_USEU_prdc_lang", "W_USEU_Chng_sign", "T_USEU_prdc_lang", "T_USEU_Chng_sign",
                    "P_SHIFTMark", "P_CAPSMark",
                },
                {
                    "P_key_41", "T_key_41",
                    "N_modeSelect_all",
                    "P_Mode_direct", "P_Mode_roma_hira", "P_Mode_roma_kata",
                    "N_KeyChange_JP",
                    "W_JP_Chng_ABC", "W_JP_Chng_KANA", "W_JP_Chng_sign",
                    "T_JP_Chng_ABC", "T_JP_Chng_KANA", "T_JP_Chng_sign",
                    "T_key_CAPS", "T_key_SHIFT",
                    "N_modeSelect_kr",
                },
            };
            const PaneVisibility csAsciiVisiblePanes = {
                23, 14,
                {
                    "P_key_41", "T_key_41", "P_key_42", "T_key_42", "P_key_43", "T_key_43",
                    "N_KeyChange_USEU",
                    "P_Gkey_00", "P_Gkey_11", "P_Gkey_22", "P_Gkey_33", "P_Gkey_44",
                    "T_Gkey_00", "T_Gkey_11", "T_Gkey_22", "T_Gkey_33", "T_Gkey_44",
                    "W_USEU_prdc_lang", "W_USEU_Chng_sign", "T_USEU_prdc_lang", "T_USEU_Chng_sign",
                    "P_SHIFTMark", "P_CAPSMark",
                },
                {
                    "N_modeSelect_all",
                    "P_Mode_direct", "P_Mode_roma_hira", "P_Mode_roma_kata",
                    "N_KeyChange_JP",
                    "W_JP_Chng_ABC", "W_JP_Chng_KANA", "W_JP_Chng_sign",
                    "T_JP_Chng_ABC", "T_JP_Chng_KANA", "T_JP_Chng_sign",
                    "T_key_CAPS", "T_key_SHIFT",
                    "N_modeSelect_kr",
                },
            };
            const PaneVisibility csAsciiVisiblePanesNL = {
                19, 18,
                {
                    "P_key_43", "T_key_43",
                    "N_KeyChange_USEU",
                    "P_Gkey_00", "P_Gkey_11", "P_Gkey_22", "P_Gkey_33", "P_Gkey_44",
                    "T_Gkey_00", "T_Gkey_11", "T_Gkey_22", "T_Gkey_33", "T_Gkey_44",
                    "W_USEU_prdc_lang", "W_USEU_Chng_sign", "T_USEU_prdc_lang", "T_USEU_Chng_sign",
                    "T_key_CAPS", "T_key_SHIFT",
                },
                {
                    "P_key_41", "T_key_41", "P_key_42", "T_key_42",
                    "N_modeSelect_all",
                    "P_Mode_direct", "P_Mode_roma_hira", "P_Mode_roma_kata",
                    "N_KeyChange_JP",
                    "W_JP_Chng_ABC", "W_JP_Chng_KANA", "W_JP_Chng_sign",
                    "T_JP_Chng_ABC", "T_JP_Chng_KANA", "T_JP_Chng_sign",
                    "P_SHIFTMark", "P_CAPSMark",
                    "N_modeSelect_kr",
                },
            };
            const PaneVisibility csCNAsciiVisiblePanes = {
                20, 21,
                {
                    "N_KeyChange_USEU",
                    "P_Gkey_00", "P_Gkey_11", "P_Gkey_22", "P_Gkey_33", "P_Gkey_44",
                    "T_Gkey_00", "T_Gkey_11", "T_Gkey_22", "T_Gkey_33", "T_Gkey_44",
                    "W_USEU_Chng_sign", "T_USEU_Chng_sign",
                    "T_key_CAPS", "T_key_SHIFT",
                    "P_key_42", "T_key_42", "P_key_43", "T_key_43",
                    "N_modeSelect_kr",
                },
                {
                    "P_key_41", "T_key_41", 
                    "N_modeSelect_all",
                    "P_Mode_direct", "P_Mode_roma_hira", "P_Mode_roma_kata",
                    "N_KeyChange_JP",
                    "W_JP_Chng_ABC", "W_JP_Chng_KANA", "W_JP_Chng_sign",
                    "T_JP_Chng_ABC", "T_JP_Chng_KANA", "T_JP_Chng_sign",
                    "P_SHIFTMark", "P_CAPSMark",
                    "W_USEU_prdc_lang",  "T_USEU_prdc_lang",
                    "B_Mode_direct", "B_Mode_roma_hira", "B_Mode_roma_kata",
                    "T_Mode_cn_pinyin",
                },
            };
            const PaneVisibility csKRAsciiVisiblePanes = {
                16, 22,
                {
                    "N_KeyChange_USEU",
                    "P_Gkey_00", "P_Gkey_11", "P_Gkey_22", "P_Gkey_33", "P_Gkey_44",
                    "T_Gkey_00", "T_Gkey_11", "T_Gkey_22", "T_Gkey_33", "T_Gkey_44",
                    "W_USEU_Chng_sign", "T_USEU_Chng_sign",
                    "T_key_CAPS", "T_key_SHIFT",
                    "N_modeSelect_kr",
                },
                {
                    "P_key_41", "T_key_41", "P_key_42", "T_key_42", "P_key_43", "T_key_43",
                    "P_Mode_direct",
                    "N_KeyChange_JP",
                    "W_JP_Chng_ABC", "W_JP_Chng_KANA", "W_JP_Chng_sign",
                    "T_JP_Chng_ABC", "T_JP_Chng_KANA", "T_JP_Chng_sign",
                    "P_SHIFTMark", "P_CAPSMark",
                    "W_USEU_prdc_lang",  "T_USEU_prdc_lang",
                    "N_modeSelect_all",
                    "B_Mode_direct", "B_Mode_roma_hira", "B_Mode_roma_kata",
                },
            };

            extern const CharCodeToAblaut csCharCodeToAblautSP[];
            extern const CharCodeToAblaut csCharCodeToAblautNL[];
            extern const CharCodeToAblaut csCharCodeToAblautDE[];
            extern const CharCodeToAblaut csCharCodeToAblautFR[];
            const CharCodeToAblaut csCharCodeToAblautSP[] = {
                {L'a', {L'à', L'á', L'â', L'ã', L'ä',    0,    0}},
                {L'i', {L'ì', L'í', L'î',    0, L'ï',    0,    0}},
                {L'u', {L'ù', L'ú', L'û',    0, L'ü',    0,    0}},
                {L'e', {L'è', L'é', L'ê',    0, L'ë',    0,    0}},
                {L'o', {L'ò', L'ó', L'ô', L'õ', L'ö',    0,    0}},
                {L'A', {L'À', L'Á', L'Â', L'Ã', L'Ä',    0,    0}},
                {L'I', {L'Ì', L'Í', L'Î',    0, L'Ï',    0,    0}},
                {L'U', {L'Ù', L'Ú', L'Û',    0, L'Ü',    0,    0}},
                {L'E', {L'È', L'É', L'Ê',    0, L'Ë',    0,    0}},
                {L'O', {L'Ò', L'Ó', L'Ô', L'Õ', L'Ö',    0,    0}},
                {L'y', {   0, L'ý',    0,    0, L'ÿ',    0,    0}},
                {L'Y', {   0, L'Ý',    0,    0, L'Ÿ',    0,    0}},
            };
            const CharCodeToAblaut csCharCodeToAblautNL[] = {
                {L'a', {L'à',    0, L'â', L'ã',    0, L'á', L'ä'}},
                {L'i', {L'ì',    0, L'î',    0,    0, L'í', L'ï'}},
                {L'u', {L'ù',    0, L'û',    0,    0, L'ú', L'ü'}},
                {L'e', {L'è',    0, L'ê',    0,    0, L'é', L'ë'}},
                {L'o', {L'ò',    0, L'ô', L'õ',    0, L'ó', L'ö'}},
                {L'A', {L'À',    0, L'Â', L'Ã',    0, L'Á', L'Ä'}},
                {L'I', {L'Ì',    0, L'Î',    0,    0, L'Í', L'Ï'}},
                {L'U', {L'Ù',    0, L'Û',    0,    0, L'Ú', L'Ü'}},
                {L'E', {L'È',    0, L'Ê',    0,    0, L'É', L'Ë'}},
                {L'O', {L'Ò',    0, L'Ô', L'Õ',    0, L'Ó', L'Ö'}},
                {L'y', {   0,    0,    0,    0,    0, L'ý', L'ÿ'}},
                {L'Y', {   0,    0,    0,    0,    0, L'Ý', L'Ÿ'}},
            };
            const CharCodeToAblaut csCharCodeToAblautDE[] = {
                {L'a', {L'à', L'á',    0,    0,    0,    0,    0}},
                {L'i', {L'ì', L'í',    0,    0,    0,    0,    0}},
                {L'u', {L'ù', L'ú',    0,    0,    0,    0,    0}},
                {L'e', {L'è', L'é',    0,    0,    0,    0,    0}},
                {L'o', {L'ò', L'ó',    0,    0,    0,    0,    0}},
                {L'A', {L'À', L'Á',    0,    0,    0,    0,    0}},
                {L'I', {L'Ì', L'Í',    0,    0,    0,    0,    0}},
                {L'U', {L'Ù', L'Ú',    0,    0,    0,    0,    0}},
                {L'E', {L'È', L'É',    0,    0,    0,    0,    0}},
                {L'O', {L'Ò', L'Ó',    0,    0,    0,    0,    0}},
                {L'y', {   0, L'ý',    0,    0,    0,    0,    0}},
                {L'Y', {   0, L'Ý',    0,    0,    0,    0,    0}},
            };
            const CharCodeToAblaut csCharCodeToAblautFR[] = {
                {L'a', {   0,    0, L'â',    0, L'ä',    0,    0}},
                {L'i', {   0,    0, L'î',    0, L'ï',    0,    0}},
                {L'u', {   0,    0, L'û',    0, L'ü',    0,    0}},
                {L'e', {   0,    0, L'ê',    0, L'ë',    0,    0}},
                {L'o', {   0,    0, L'ô',    0, L'ö',    0,    0}},
                {L'A', {   0,    0, L'Â',    0, L'Ä',    0,    0}},
                {L'I', {   0,    0, L'Î',    0, L'Ï',    0,    0}},
                {L'U', {   0,    0, L'Û',    0, L'Ü',    0,    0}},
                {L'E', {   0,    0, L'Ê',    0, L'Ë',    0,    0}},
                {L'O', {   0,    0, L'Ô',    0, L'Ö',    0,    0}},
                {L'y', {   0,    0,    0,    0, L'ÿ',    0,    0}},
                {L'Y', {   0,    0,    0,    0, L'Ÿ',    0,    0}},
            };

            extern const LanguageDependencyData csLanguageDependencyData[];
            extern const LanguageDependencyData csJapanKanaInput;
            const LanguageDependencyData csLanguageDependencyData[] = {
                {csUSKeyboard, csGridKeyboard, csPaneNameToControlKey, &csJPNAsciiVisiblePanes  , NULL                },  // JP
                {csUSKeyboard, csGridKeyboard, csPaneNameToControlKey, &csUSAsciiVisiblePanes   , NULL                },  // USA
                {csUKKeyboard, csGridKeyboard, csPaneNameToControlKey, &csEngAsciiVisiblePanesUK, NULL                },  // UK
                {csFRKeyboard, csGridKeyboard, csPaneNameToControlKey, &csAsciiVisiblePanes     , csCharCodeToAblautFR},  // FR
                {csDEKeyboard, csGridKeyboard, csPaneNameToControlKey, &csAsciiVisiblePanes     , csCharCodeToAblautDE},  // DE
                {csITKeyboard, csGridKeyboard, csPaneNameToControlKey, &csAsciiVisiblePanes     , NULL                },  // IT
                {csESKeyboard, csGridKeyboard, csPaneNameToControlKey, &csEngAsciiVisiblePanesSP, csCharCodeToAblautSP},  // SP
                {csNLKeyboard, csGridKeyboard, csPaneNameToControlKey, &csAsciiVisiblePanesNL   , csCharCodeToAblautNL},  // NL
                {csCNKeyboard, csGridKeyboard, csPaneNameToControlKey, &csCNAsciiVisiblePanes   , NULL                },  // CN
                {csUSKeyboard, csGridKeyboard, csPaneNameToControlKey, &csKRAsciiVisiblePanes   , NULL                },  // KR
            };
            const LanguageDependencyData csJapanKanaInput = {csJPKeyboard, csGridKeyboard, csPaneNameToControlKey, &csJPNAsciiVisiblePanes, NULL};

            extern const AnimationFile csAninationFile[];
            const AnimationFile csAninationFile[] = {
                {ANIM_NORMAL,           "fs_VK_ascii_keytop_a_normal.brlan"},
                {ANIM_FOCUS_IN,         "fs_VK_ascii_keytop_a_Focus-IN.brlan"},
                {ANIM_FOCUS_OUT,        "fs_VK_ascii_keytop_a_Focus-OUT.brlan"},
                {ANIM_ROLL_OVER,        "fs_VK_ascii_keytop_a_Roll_over.brlan"},
                {ANIM_PUSHED,           "fs_VK_ascii_keytop_a_Pushed.brlan"},

                {ANIM_TOGGLE_ON,        "fs_VK_ascii_keytop_a_toggle-ON.brlan"},
                {ANIM_TOGGLE_OFF,       "fs_VK_ascii_keytop_a_toggle-OFF.brlan"},
                {ANIM_TARGET_ON,        "fs_VK_ascii_keytop_a_Target_ON.brlan"},
                {ANIM_TOGGLE_FOCUS_IN,  "fs_VK_ascii_keytop_a_toggleON_Focus-IN.brlan"},
                {ANIM_TOGGLE_FOCUS_OUT, "fs_VK_ascii_keytop_a_toggleON_Focus-OUT.brlan"},
                {ANIM_TOGGLE_NORMAL,    "fs_VK_ascii_keytop_a_normal_toggle-ON.brlan"},
                {ANIM_TOGGLE_PUSHED,    "fs_VK_ascii_keytop_a_toggleON_Pushed.brlan"},

                {ANIM_ACTIVE_ON,        "fs_VK_ascii_keytop_a_active-ON.brlan"},
                {ANIM_ACTIVE_OFF,       "fs_VK_ascii_keytop_a_active-OFF.brlan"},
                {ANIM_ACTIVE_NORMAL,    "fs_VK_ascii_keytop_a_active_normal.brlan"},
                {ANIM_INACTIVE_NORMAL,  "fs_VK_ascii_keytop_a_not_active_normal.brlan"},
            };

            const wchar_t lbl_81614A20[26] = {L'ㅁ', L'ㅠ', L'ㅊ', L'ㅇ', L'ㄷ', L'ㄹ', L'ㅎ', L'ㅗ', L'ㅑ', L'ㅓ', L'ㅏ', L'ㅣ', L'ㅡ',
                                              L'ㅜ', L'ㅐ', L'ㅔ', L'ㅂ', L'ㄱ', L'ㄴ', L'ㅅ', L'ㅕ', L'ㅍ', L'ㅈ', L'ㅌ', L'ㅛ', L'ㅋ'};
            const wchar_t lbl_81614A54[26] = {L'ㅁ', L'ㅠ', L'ㅊ', L'ㅇ', L'ㄸ', L'ㄹ', L'ㅎ', L'ㅗ', L'ㅑ', L'ㅓ', L'ㅏ', L'ㅣ', L'ㅡ',
                                              L'ㅜ', L'ㅒ', L'ㅖ', L'ㅃ', L'ㄲ', L'ㄴ', L'ㅆ', L'ㅕ', L'ㅍ', L'ㅉ', L'ㅌ', L'ㅛ', L'ㅋ'};

            // Why does base require (and incorrectly )
            #define PANE_TO_ANIM_FILES__BASE FILE(NORMAL), FILE(FOCUS_IN), FILE(FOCUS_OUT), FILE(ROLL_OVER), FILE(PUSHED)
            #define PANE_TO_ANIM_FILES__TOGGLEABLE FILE(TOGGLE_ON), FILE(TOGGLE_OFF)
            #define PANE_TO_ANIM_FILES__TARGETABLE FILE(TARGET_ON)
            #define PANE_TO_ANIM_FILES__FULLTOGGLE FILE(TOGGLE_FOCUS_IN), FILE(TOGGLE_FOCUS_OUT), FILE(TOGGLE_NORMAL), FILE(TOGGLE_PUSHED)
            #define PANE_TO_ANIM_FILES__ACTIVATABLE FILE(ACTIVE_ON), FILE(ACTIVE_OFF), FILE(ACTIVE_NORMAL), FILE(INACTIVE_NORMAL)

            #define PANE_TO_ANIM_FILESET_NORMAL {PANE_TO_ANIM_FILES__BASE, FILE(TOGGLE_ON)}
            #define PANE_TO_ANIM_FILESET_ONOFF {PANE_TO_ANIM_FILES__BASE, PANE_TO_ANIM_FILES__TOGGLEABLE, PANE_TO_ANIM_FILES__ACTIVATABLE}
            #define PANE_TO_ANIM_FILESET_TOGGLE {PANE_TO_ANIM_FILES__BASE, PANE_TO_ANIM_FILES__TOGGLEABLE, PANE_TO_ANIM_FILES__TARGETABLE}
            #define PANE_TO_ANIM_FILESET_SHIFTCAPS {PANE_TO_ANIM_FILES__BASE, PANE_TO_ANIM_FILES__TOGGLEABLE, PANE_TO_ANIM_FILES__TARGETABLE, PANE_TO_ANIM_FILES__FULLTOGGLE}
            #define FILE(TAG) &csAninationFile[ANIM_ ## TAG]
            static const char *csKeyBaseName = "P_key_00";
            extern PaneToAnimation csPaneToAnimation[];
            PaneToAnimation csPaneToAnimation[] = {
                {KEY_TYPE_NORMAL,     "P_key_00",          5,            NULL, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_01",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_02",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_03",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_04",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_05",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_06",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_07",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_08",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_09",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_10",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_11",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_12",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_13",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_15",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_16",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_17",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_18",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_14",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL}, // Why is key 14 out of order???
                {KEY_TYPE_NORMAL,     "P_key_19",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_20",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_21",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_22",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_23",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_24",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_25",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_26",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_27",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_28",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_29",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_30",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_31",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_32",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_33",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_34",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_35",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_36",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_37",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_38",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_39",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_40",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_41",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_42",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_43",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_44",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_45",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_46",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_47",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_48",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_49",          5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_DELETE",      5,            NULL, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_LF",          5,            NULL, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_key_SPACE",       5,            NULL, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_SHIFTCAPS,  "P_key_SHIFT",      12,            NULL, PANE_TO_ANIM_FILESET_SHIFTCAPS},
                {KEY_TYPE_SHIFTCAPS,  "P_key_CAPS",       12,            NULL, PANE_TO_ANIM_FILESET_SHIFTCAPS},
                {KEY_TYPE_NORMAL,     "P_Gkey_00",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_01",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_02",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_03",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_04",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_05",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_06",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_07",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_08",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_09",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_10",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_11",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_12",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_13",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_14",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_15",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_16",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_17",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_18",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_19",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_20",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_21",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_22",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_23",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_24",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_25",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_26",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_27",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_28",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_29",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_30",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_31",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_32",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_33",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_34",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_35",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_36",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_37",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_38",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_39",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_40",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_41",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_42",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_43",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_44",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_45",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_46",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_47",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_48",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_49",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_50",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_51",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_52",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_53",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_54",         5,   csKeyBaseName, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_55",         5,            NULL, PANE_TO_ANIM_FILESET_NORMAL}, // Why doesn't this have the force name bound?
                {KEY_TYPE_NORMAL,     "P_Gkey_DELETE",     5,            NULL, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_LF",         5,            NULL, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "P_Gkey_SPACE",      5,            NULL, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "W_USEU_prdc_lang",  5,            NULL, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_NORMAL,     "W_USEU_Chng_sign",  5,            NULL, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_TOGGLE,     "W_JP_Chng_ABC",     7,            NULL, PANE_TO_ANIM_FILESET_TOGGLE},
                {KEY_TYPE_TOGGLE,     "W_JP_Chng_KANA",    7,            NULL, PANE_TO_ANIM_FILESET_TOGGLE},
                {KEY_TYPE_NORMAL,     "W_JP_Chng_sign",    5,            NULL, PANE_TO_ANIM_FILESET_NORMAL},
                {KEY_TYPE_TOGGLE,     "P_hiragana",        7,            NULL, PANE_TO_ANIM_FILESET_TOGGLE},
                {KEY_TYPE_TOGGLE,     "P_katakana",        7,            NULL, PANE_TO_ANIM_FILESET_TOGGLE},
                {KEY_TYPE_ONOFF,      "P_Gkey_dakuten",   11,            NULL, PANE_TO_ANIM_FILESET_ONOFF},
                {KEY_TYPE_ONOFF,      "P_Gkey_handaku",   11,            NULL, PANE_TO_ANIM_FILESET_ONOFF},
                {KEY_TYPE_ONOFF,      "P_Gkey_komoji",    11,            NULL, PANE_TO_ANIM_FILESET_ONOFF},
                {KEY_TYPE_TOGGLE,     "P_Mode_roma_hira",  7, "P_Mode_direct", PANE_TO_ANIM_FILESET_TOGGLE},
                {KEY_TYPE_TOGGLE,     "P_Mode_roma_kata",  7, "P_Mode_direct", PANE_TO_ANIM_FILESET_TOGGLE},
                {KEY_TYPE_TOGGLE,     "P_Mode_direct",     7,            NULL, PANE_TO_ANIM_FILESET_TOGGLE},
                {KEY_TYPE_TOGGLE,     "P_Mode_kr_eng",     7, "P_Mode_direct", PANE_TO_ANIM_FILESET_TOGGLE},
                {KEY_TYPE_TOGGLE,     "P_Mode_kr_han",     7, "P_Mode_direct", PANE_TO_ANIM_FILESET_TOGGLE},
            };
            #undef FILE
            // clang-format on

            u16 convertHangul(u16 ch, bool isCaps) {
                if (isCaps) {
                    ch = util::reverseLetterCaseW(ch);
                }
                if (L'a' <= ch && ch <= L'z') {
                    return lbl_81614A20[ch - L'a'];
                } else if (L'A' <= ch && ch <= L'Z') {
                    return lbl_81614A54[ch - L'A'];
                }
                return ch;
            }
            wchar_t Base::transformHardwareWChar(wchar_t ch) const {
                ch = inputform::DeadKeyStream::ToIndependentClass(ch);
                ch = mgr()->getHWKeyboard()->convertWCCode(ch);
                if (mgr()->getToolBar()->isQwerty() && mgr()->getPCKeyboard()->getTranslateMode() != TM_00 && mgr()->getLanguage() == KR) {
                    ch = convertHangul(ch, isCapsOn());
                }
                return ch;
            }

            void Base::create(MEMAllocator* allocator) {
                mpAllocator = allocator;
            }
            void Base::KeyState::init() {
                unk_0x04 = 0;
                if (meLanguage == JP && !mpKeyboard->mbIsOnlyQwerty) {
                    mABCFlags = 1;
                } else if ((meLanguage == CN || meLanguage == KR) && mpKeyboard->mbIsLangKeyActive) {
                    mABCFlags = 1;
                } else {
                    mABCFlags = 0;
                }
                mAIUFlags = 0;
                refresh_();
            }
            void Base::init() {
                unk_0x14[0] = 0;
                mpLangDepData = &csLanguageDependencyData[getLanguage()];
                mKeyState.init();

                u32 data = mKeyState.getTranslateMode();
                sendCommand(CommandReceiver::INPUT_COMMAND_18, &data);
            }

            void Base::inputCharCode(wchar_t ch) {
                sendInputWChar(ch, false);
            }

            void Base::onKey(u32 phase, void* data) {
                wchar_t ch = getWCCode((char*)data);
                if (phase != 4)
                    return;

                if (ch == L'\0') {
                    switch (getControlKey((char*)data)) {
                        case VCODE_DELETE:
                            sendCommand(CommandReceiver::INPUT_COMMAND_1, NULL);
                            break;
                        case VCODE_RETURN:
                            sendCommand(CommandReceiver::INPUT_COMMAND_7, NULL);
                            sendCommand(CommandReceiver::INPUT_COMMAND_39, NULL);
                            break;
                        case CODE_UNK_0x02:
                            if (mpManager->getInputForm()->canConvert()) {
                                sendCommand(CommandReceiver::INPUT_COMMAND_40, NULL);
                            } else {
                                sendInputWChar(L' ', false);
                            }
                            break;
                        case CODE_UNK_0x0B:
                            setABC(true);
                            break;
                        case VCODE_QWERTY_OFF:
                            setABC(false);
                            break;
                        case CODE_UNK_0x0D:
                            goSignInputMode();
                            break;
                        case CODE_UNK_0x0A:
                            changePredictLanguage();
                            break;
                        case VCODE_SET_AIU_INPUT_MODE_4:
                            changeAIUInputMode(IM_04);
                            break;
                        case VCODE_TRANSLATEMODE_0: {
                            u32 cmdData[1] = {tistring::Decolated::TM_NONE};
                            sendCommand(CommandReceiver::INPUT_COMMAND_18, &cmdData);
                            setTranslateMode(TM_00);
                            break;
                        }
                        case VCODE_TRANSLATEMODE_1: {
                            u32 cmdData[1] = {tistring::Decolated::TM_ROMAJI_HIRAGANA};
                            sendCommand(CommandReceiver::INPUT_COMMAND_18, &cmdData);
                            setTranslateMode(TM_01);
                            break;
                        }
                        case VCODE_TRANSLATEMODE_2: {
                            u32 cmdData[1] = {tistring::Decolated::TM_ROMAJI_KATAKANA};
                            sendCommand(CommandReceiver::INPUT_COMMAND_18, &cmdData);
                            setTranslateMode(TM_02);
                            break;
                        }
                        case CODE_UNK_0x12:
                            mKeyState.setAIUInputModeInternal(KeyState::IM_AIU_HIRAGANA);
                            break;
                        case CODE_UNK_0x13:
                            mKeyState.setAIUInputModeInternal(KeyState::IM_AIU_KATAKANA);
                            break;
                        case VCODE_CONVERT_DAKUTEN:
                            if (mKeyState.isAIUKanaDakuten()) {
                                sendCommand(CommandReceiver::INPUT_COMMAND_25, NULL);
                            }
                            break;
                        case VCODE_CONVERT_HANDAKU:
                            if (mKeyState.isAIUKanaHandaku()) {
                                sendCommand(CommandReceiver::INPUT_COMMAND_26, NULL);
                            }
                            break;
                        case VCODE_CONVERT_SMALL:
                            if (mKeyState.isAIUKanaSmall()) {
                                sendCommand(CommandReceiver::INPUT_COMMAND_28, NULL);
                            }
                            break;
                        default:
                            break;
                    }
                } else {
                    inputCharCode(ch);
                }
            }

            void Base::updateFromReceiver(u32 cmd, void* data) {
                if (cmd == CommandReceiver::INPUT_COMMAND_GET_PREDICT_MODE)
                    return;

                CommandReceiver::ChangePredictMode cmdData = {};
                sendCommand(CommandReceiver::INPUT_COMMAND_GET_PREDICT_MODE, &cmdData);

                if ((cmd != CommandReceiver::INPUT_COMMAND_25 && cmd != CommandReceiver::INPUT_COMMAND_26 &&
                     cmd != CommandReceiver::INPUT_COMMAND_28 && cmd != CommandReceiver::INPUT_COMMAND_24 &&
                     cmd != CommandReceiver::INPUT_COMMAND_20 && cmd != CommandReceiver::INPUT_COMMAND_19 &&
                     cmd != CommandReceiver::INPUT_COMMAND_35) ||
                    cmd == CommandReceiver::INPUT_COMMAND_36) {
                    if (mKeyState.hasKanaBitsSet()) {
                        mKeyState.resetKanaBits();
                        mKeyState.refresh_();
                    }
                }
                if (cmd == CommandReceiver::INPUT_COMMAND_36) {
                    unk_0x14[0] = 1;
                }
            }

            void Base::onActive() {
                u32 translateMode = mKeyState.getTranslateMode();
                sendCommand(CommandReceiver::INPUT_COMMAND_18, &translateMode);
                sendCommand(CommandReceiver::INPUT_COMMAND_39, NULL);

                if (!mKeyState.isAIU()) {
                    CommandReceiver::ChangeKanaModeData kanaMode;
                    switch (mKeyState.getTranslateMode()) {
                        case TM_01:
                            kanaMode.bKanaOn = true;
                            break;
                        case TM_02:
                            kanaMode.bKanaOn = false;
                            break;
                        default:
                            goto LAB_81410a1c;
                    }
                    sendCommand(CommandReceiver::INPUT_COMMAND_19, &kanaMode);
                } else {
                    CommandReceiver::ChangeKanaModeData kanaMode;
                    switch (mKeyState.getAIUInputMode()) {
                        case IM_HIRAGANA:
                            kanaMode.bKanaOn = true;
                            break;
                        case IM_KATAKANA:
                            kanaMode.bKanaOn = false;
                            break;
                        default:
                            goto LAB_81410a1c;
                    }
                    sendCommand(CommandReceiver::INPUT_COMMAND_19, &kanaMode);
                }
            LAB_81410a1c:
                updateFixMode();
            }
            void Base::onClose() {
            }
            wchar_t Base::getWCCode(char* paneName) {
                return mKeyState.getWCCode(paneName);
            }

            VKeyCode Base::KeyState::getControlKey(char* paneName) {
                for (u16 i = 0; i < NUM_CONTROL_KEYS; i++) {
                    if (util::strcmp(mpLangDepData->pPaneNameToControlKey[i].paneName, paneName)) {
                        return mpLangDepData->pPaneNameToControlKey[i].code;
                    }
                }
                return CODE_UNK_0x1B;
            }
            VKeyCode Base::getControlKey(char* paneName) {
                return mKeyState.getControlKey(paneName);
            }

            void Base::changeABCInputMode(InputMode mode) {
                mKeyState.setABCInputMode(mode);
            }
            void Base::changeAIUInputMode(InputMode mode) {
                mKeyState.setAIUInputMode(mode);
            }

            void Base::sendInputWChar(wchar_t ch, bool flag) {
                unk_0x14[0] = 0;

                u32 inputFlags = 0;
                if (mKeyState.isABCCaps()) {
                    inputFlags |= 2;
                }
                if (mKeyState.isABCShift()) {
                    inputFlags |= 1;
                }

                CommandReceiver::Command0Data cmdData = {ch, inputFlags, false, true, 0};
                sendCommand(0, &cmdData);

                if (mKeyState.isABCShift()) {
                    LayoutGather& lg = LayoutGather::Singleton::getInstance();
                    if (!lg.isHoldingShift()) {
                        if (mKeyState.getABCKanaFlags() != 0) {
                            mKeyState.setABCShift(false);
                            mKeyState.refresh_();
                        }
                    }
                }

                if (unk_0x14[0] == 0) {
                    u32 newKanaFlags = 0;
                    if (ch != util::KBD_ConvertDakuten(ch)) {
                        newKanaFlags |= KeyState::KANAFLAG_DAKUTEN;
                    }
                    if (ch != util::KBD_ConvertHandaku(ch)) {
                        newKanaFlags |= KeyState::KANAFLAG_HANDAKU;
                    }
                    if (ch != util::KBD_ConvertSmall(ch)) {
                        newKanaFlags |= KeyState::KANAFLAG_SMALL;
                    }
                    mKeyState.setAIUKanaFlags(newKanaFlags);
                }
            }

            void Base::setLanguage(Language lang) {
                meLanguage = lang;
                mKeyState.meLanguage = lang;
                mKeyState.refresh_();
            }

            void Base::updateFixMode() {
                if (mpManager->getToolBar() != NULL) {
                    if (!mpManager->getToolBar()->isQwerty()) {
                        return;
                    }
                }

                switch (getLanguage()) {
                    case JP: {
                        CommandReceiver::SetFixModeData setFixModeData = {};

                        if (isABC() && mKeyState.getABCInputModeInternal() == KeyState::IM_ABC_DIRECT) {
                            setFixModeData.bFixOn = true;
                        } else {
                            setFixModeData.bFixOn = false;
                        }
                        sendCommand(CommandReceiver::INPUT_COMMAND_20, &setFixModeData);
                        break;
                    }

                    default:
                        break;
                }
                return;
            }
            void Base::setABC(bool isABC) {
                mKeyState.setIsAIU(!isABC);
                sendCommand(CommandReceiver::INPUT_COMMAND_41, NULL);
            }
            void Base::setTranslateMode(TranslateMode mode) {
                KeyState::InternalABCInputMode abcInputMode;
                switch (mode) {
                    case TM_00:
                        abcInputMode = KeyState::IM_ABC_DIRECT;
                        break;
                    case TM_01:
                        abcInputMode = KeyState::IM_ABC_HIRAGANA;
                        break;
                    case TM_02:
                        abcInputMode = KeyState::IM_ABC_KATAKANA;
                        break;
                }

                if (abcInputMode != (u32)mKeyState.getABCInputModeInternal()) {
                    if (getLanguage() == KR || getLanguage() == CN) {
                        mpManager->getInputForm()->onCommand(CommandReceiver::INPUT_COMMAND_6, NULL);
                    }

                    mKeyState.setABCInputModeInternal(abcInputMode);

                    if (getLanguage() == CN && mpManager->getCandidateBox() != NULL) {
                        mpManager->getCandidateBox()->checkValidation();
                    }

                    u32 newTranslateMode = mode;
                    sendCommand(CommandReceiver::INPUT_COMMAND_18, &newTranslateMode);
                    mpLangDepData = &csJapanKanaInput;
                    sendCommand(CommandReceiver::INPUT_COMMAND_41, NULL);
                }
            }

            void Base::KeyState::refresh_() {
                if ((meLanguage != JP && meLanguage != KR && meLanguage != CN) || mpKeyboard->mbIsOnlyQwerty) {
                    setABCInputModeInternal(IM_ABC_DIRECT);
                }
                if ((meLanguage == CN || meLanguage == KR) && abcInputModeInternalIs(IM_ABC_KATAKANA)) {
                    setABCInputModeInternal(IM_ABC_HIRAGANA);
                }
                if (abcInputModeInternalIs(IM_ABC_DIRECT)) {
                    mpLangDepData = csLanguageDependencyData + meLanguage;
                } else if (meLanguage == JP) {
                    mpLangDepData = &csJapanKanaInput;
                } else {
                    mpLangDepData = csLanguageDependencyData + meLanguage;
                }

                CommandReceiver::SetFixModeData setFixModeCmdData;
                CommandReceiver::ChangeKanaModeData setKanaModeCmdData;
                if (!isAIU()) {
                    switch (getABCInputModeInternal()) {
                        case IM_ABC_DIRECT: {
                            setFixModeCmdData.bFixOn = true;
                            mpKeyboard->sendCommand(CommandReceiver::INPUT_COMMAND_20, &setFixModeCmdData);
                            break;
                        }
                        case IM_ABC_HIRAGANA: {
                            setFixModeCmdData.bFixOn = false;
                            setKanaModeCmdData.bKanaOn = true;
                            mpKeyboard->sendCommand(CommandReceiver::INPUT_COMMAND_20, &setFixModeCmdData);
                            mpKeyboard->sendCommand(CommandReceiver::INPUT_COMMAND_19, &setKanaModeCmdData);
                            break;
                        }
                        case IM_ABC_KATAKANA: {
                            setFixModeCmdData.bFixOn = false;
                            setKanaModeCmdData.bKanaOn = false;
                            mpKeyboard->sendCommand(CommandReceiver::INPUT_COMMAND_20, &setFixModeCmdData);
                            mpKeyboard->sendCommand(CommandReceiver::INPUT_COMMAND_19, &setKanaModeCmdData);
                            break;
                        }
                    }
                } else {
                    switch (getAIUInputModeInternal()) {
                        case IM_AIU_HIRAGANA: {
                            setFixModeCmdData.bFixOn = false;
                            setKanaModeCmdData.bKanaOn = true;
                            mpKeyboard->sendCommand(CommandReceiver::INPUT_COMMAND_20, &setFixModeCmdData);
                            mpKeyboard->sendCommand(CommandReceiver::INPUT_COMMAND_19, &setKanaModeCmdData);
                            break;
                        }
                        case IM_AIU_KATAKANA: {
                            setFixModeCmdData.bFixOn = false;
                            setKanaModeCmdData.bKanaOn = false;
                            mpKeyboard->sendCommand(CommandReceiver::INPUT_COMMAND_20, &setFixModeCmdData);
                            mpKeyboard->sendCommand(CommandReceiver::INPUT_COMMAND_19, &setKanaModeCmdData);
                            break;
                        }
                    }
                }
                if (mpKeyboard != NULL) {
                    mpKeyboard->refreshState();
                }
            }
            void Base::KeyState::refreshText(nw4r::lyt::Pane* pane) {
                for (u16 i = 0; i < NUM_PANE_CHARS; i++) {
                    char tbPaneName[0x11];
                    util::replaceChar(tbPaneName, 0x11, mpLangDepData->pPaneChars[i].szPaneName, 0, 'T');
                    nw4r::lyt::TextBox* tb = nw4r::ut::DynamicCast<nw4r::lyt::TextBox*>(pane->FindPaneByName(tbPaneName));
                    if (tb != NULL) {
                        wchar_t tbText[2];
                        tbText[0] = getWCCode(i);
                        tbText[1] = L'\0';

                        if (meLanguage == KR && abcInputModeInternalIs(IM_ABC_HIRAGANA)) {
                            tbText[0] = convertHangul(tbText[0], mpKeyboard->isCapsOn());
                        }

                        tb->SetString(tbText);
                    }
                }
                for (u16 i = 0; i < 0x38; i++) {
                    char tbPaneName[0x11];
                    util::replaceChar(tbPaneName, 0x11, mpLangDepData->pGridData[i].szPaneName, 0, 'T');
                    nw4r::lyt::TextBox* tb = nw4r::ut::DynamicCast<nw4r::lyt::TextBox*>(pane->FindPaneByName(tbPaneName));

                    if (tb != NULL) {
                        wchar_t tbText[2];

                        u32 idx = getAIUInputModeInternal() == IM_AIU_HIRAGANA ? 2 : 3;

                        // Equivalent, but doesn't match because mwcc hates enums
                        // u32 idx = getAIUInputModeInternal() == IM_AIU_HIRAGANA ? GridData::GD_IDX_HIRAGANA : GridData::GD_IDX_KATAKANA;

                        tbText[0] = mpLangDepData->pGridData[i].wc[idx];
                        tbText[1] = L'\0';

                        tb->SetString(tbText);
                    }
                }
            }

            void Base::KeyState::setABCFlag(u32 newFlags) {
                u32 oldFlags = getABCKanaFlags();
                if (oldFlags != newFlags) {
                    mABCFlags = (mABCFlags & ~KANAFLAGS_MASK) | (newFlags & KANAFLAGS_MASK);
                    if (mpKeyboard)
                        mpKeyboard->refreshState();
                }
            }
            wchar_t Base::KeyState::getWCCode(u32 paneCharI) {
                u32 idx = 0;
                if (isABCCaps())
                    idx |= 1;
                if (isABCShift())
                    idx |= 2;

                wchar_t ch = this->mpLangDepData->pPaneChars[paneCharI].wc[idx];
                if (!isAIU() && abcInputModeInternalIs(IM_ABC_DIRECT)) {
                    if (ch == L'、') {
                        ch = L',';
                    } else if (ch == L'。') {
                        ch = L'.';
                    } else if (ch == L'ー') {
                        ch = L'-';
                    } else if (ch == L'「') {
                        ch = L'[';
                    } else if (ch == L'」') {
                        ch = L']';
                    }
                } else if (mpKeyboard->getLanguage() == CN && abcInputModeInternalIs(IM_ABC_HIRAGANA)) {
                    if (ch == L'[') {
                        ch = L'《';
                    } else if (ch == L']') {
                        ch = L'》';
                    } else if (ch == L'.') {
                        ch = L'。';
                    } else if (ch == L'\'') {
                        ch = L'、';
                    } else if (ch == L',') {
                        ch = L'，';
                    } else if (ch == L'-') {
                        ch = L'－';
                    } else if (ch == L'_') {
                        ch = L'＿';
                    } else if (ch == L':') {
                        ch = L'：';
                    } else if (ch == L';') {
                        ch = L'；';
                    } else if (ch == L'!') {
                        ch = L'！';
                    } else if (ch == L'?') {
                        ch = L'？';
                    } else if (ch == L'(') {
                        ch = L'（';
                    } else if (ch == L')') {
                        ch = L'）';
                    }
                }
                return ch;
            }
            wchar_t Base::KeyState::getWCCode(char* paneName) {
                wchar_t ch = L'\0';
                for (u16 i = 0; i < NUM_PANE_CHARS; i++) {
                    if (!util::strcmp(mpLangDepData->pPaneChars[i].szPaneName, paneName))
                        continue;

                    ch = getWCCode(i);
                }
                for (u16 i = 0; i < NUM_GRID_ENTRIES; i++) {
                    if (!util::strcmp(mpLangDepData->pGridData[i].szPaneName, paneName))
                        continue;

                    u32 idx = getAIUInputModeInternal() == IM_AIU_HIRAGANA ? 2 : 3;
                    ch = mpLangDepData->pGridData[i].wc[idx];
                }
                return ch;
            }

            void Base::onlyQwerty(bool isOnlyQwerty) {
                mbIsOnlyQwerty = isOnlyQwerty;
                if (getLanguage() == JP) {
                    if (isOnlyQwerty) {
                        setABC(true);
                        mKeyState.setABCFlag(0);
                        changeABCInputMode(IM_00);
                    } else {
                        setABC(false);
                        changeAIUInputMode(IM_HIRAGANA);
                    }
                }
            }

            // TODO: This MIGHT be header-defined
            void LayoutByNW4R::setABC(bool newIsAbc) {
                changeAnimationAllToNormal();
                if (newIsAbc != isABC()) {
                    if (newIsAbc) {
                        searchAnmPane("W_JP_Chng_ABC")->changeAnimation(ANIM_PUSHED);
                        searchAnmPane("W_JP_Chng_KANA")->changeAnimation(ANIM_TOGGLE_OFF);
                    } else {
                        searchAnmPane("W_JP_Chng_ABC")->changeAnimation(ANIM_TOGGLE_OFF);
                        searchAnmPane("W_JP_Chng_KANA")->changeAnimation(ANIM_PUSHED);
                    }
                }
                mKeyState.setIsAIU(!newIsAbc);
                sendCommand(CommandReceiver::INPUT_COMMAND_41, NULL);
            }

            void Base::setLangKeyActive(bool isLangKeyActive) {
                mbIsLangKeyActive = isLangKeyActive;
                if (getLanguage() == KR || getLanguage() == CN) {
                    if (!isLangKeyActive) {
                        mKeyState.setABCFlag(0);
                        changeABCInputMode(IM_00);
                    }
                }
            }
            LayoutByNW4R::~LayoutByNW4R() {
                mpEventHandler->~EventHandler();
                MEMFreeToAllocator(mpAllocator, mpEventHandler);

                for (AnmPane* curr = (AnmPane*)nw4r::ut::List_GetFirst(&mAnmPanes); curr != NULL;
                     curr = (AnmPane*)nw4r::ut::List_GetFirst(&mAnmPanes)) {
                    nw4r::ut::List_Remove(&mAnmPanes, curr);
                    curr->destroy(mpAllocator);
                }
            }

            AnmPane::~AnmPane() {
            }

            Base::~Base() {
            }

            void LayoutByNW4R::create(MEMAllocator* allocator) {
                mpAllocator = allocator;
                mpEventHandler = new (MEMAllocFromAllocator(allocator, sizeof(EventHandler))) EventHandler(this);
                Layout::createWithEventHandler(allocator, mpEventHandler);

                mpPaneManager->setAllComponentTriggerTarget(false);
                mpPaneManager->setAllBoundingBoxComponentTriggerTarget(true);

                createAnmPane_(allocator);
                mShiftButton.Create(this, "P_key_SHIFT", "B_key_SHIFT");
                mCapsButton.Create(this, "P_key_CAPS", "B_key_CAPS");
                mModePanel.Create(this);

                mShiftButton.setBoundingTriggerTarget(true);
                mCapsButton.setBoundingTriggerTarget(true);
                mModePanel.setBoundingTriggerTargets(true);

                init();

                mKeyState.setABCFlag(0);
                changeABCInputMode(IM_00);
            }
            void LayoutByNW4R::createAnmPane_(MEMAllocator* allocator) {
                for (u16 i = 0; i < ARRAY_LENGTH(csPaneToAnimation); i++) {
                    const PaneToAnimation* currP2A = &csPaneToAnimation[i];
                    AnmPane* anmPane = NULL;
                    switch (currP2A->type) {
                        case KEY_TYPE_NORMAL: {
                            anmPane = new (MEMAllocFromAllocator(allocator, sizeof(NormalButtonAnmPane)))
                                NormalButtonAnmPane(getPane(currP2A->paneName), NULL);
                            break;
                        }
                        case KEY_TYPE_SHIFTCAPS: {
                            anmPane =
                                new (MEMAllocFromAllocator(allocator, sizeof(ShiftCapsAnmPane))) ShiftCapsAnmPane(getPane(currP2A->paneName), NULL);
                            break;
                        }
                        case KEY_TYPE_TOGGLE: {
                            anmPane = new (MEMAllocFromAllocator(allocator, sizeof(ToggleButtonAnmPane)))
                                ToggleButtonAnmPane(getPane(currP2A->paneName), NULL);
                            break;
                        }
                        case KEY_TYPE_ONOFF: {
                            anmPane = new (MEMAllocFromAllocator(allocator, sizeof(OnOffButtonAnmPane)))
                                OnOffButtonAnmPane(getPane(currP2A->paneName), NULL);
                            break;
                        }
                    }

                    nw4r::ut::List_Append(&mAnmPanes, anmPane);

                    const char* forceAddName = currP2A->forceAddName;
                    u32 count = currP2A->count;
                    for (u16 j = 0; j < count; j++) {
                        void* res = mpMultiArcResourceAccessor->GetResource(0, currP2A->pAnims[j]->fileName);
                        AnimTransformPane* anmTransformPane = (AnimTransformPane*)getLayout()->CreateAnimTransform(res, mpMultiArcResourceAccessor);
                        if (forceAddName == NULL) {
                            anmPane->addAnimation(allocator, currP2A->pAnims[j]->id, anmTransformPane, 0, 1);
                        } else {
                            anmPane->forceAddAnimation(allocator, currP2A->pAnims[j]->id, anmTransformPane, forceAddName, 0, 1);
                        }
                    }
                }
            }

            extern nw4r::math::_VEC3 modeButtonTranslateCn;
            extern nw4r::math::_VEC3 modeButtonTranslateKr;
            inline void UIModePanel::setupForLanguage(Language lang, nw4r::math::VEC3& cnTranslate_, nw4r::math::VEC3& krTranslate_) {
                nw4r::math::VEC3 cnTranslate = cnTranslate_;
                nw4r::math::VEC3 krTranslate = krTranslate_;
                if (lang == CN) {
                    getTextboxModeA()->GetMaterial()->SetTexture(0, getTextureCnDirect());
                    getTextboxModeB()->GetMaterial()->SetTexture(0, getTextureCnPinyin());
                    nw4r::math::VEC3 chosen;
                    chosen = cnTranslate;
                    getSelectButton()->SetTranslate(chosen);
                } else if (lang == KR) {
                    getTextboxModeA()->GetMaterial()->SetTexture(0, getTextureKrEnglish());
                    getTextboxModeB()->GetMaterial()->SetTexture(0, getTextureKrHangul());
                    nw4r::math::VEC3 chosen;
                    chosen = krTranslate;
                    getSelectButton()->SetTranslate(chosen);
                }
            }
            void LayoutByNW4R::init() {
                Base::init();
                setLineFeedButton(true);
                setPredictLanguageButton(true);
                setSignWindowButton(true);

                mModePanel.setupForLanguage(meLanguage, (nw4r::math::VEC3&)modeButtonTranslateCn, (nw4r::math::VEC3&)modeButtonTranslateKr);

                searchAnmPane("P_key_SHIFT")->changeAnimation(ANIM_NORMAL);
                searchAnmPane("P_key_CAPS")->changeAnimation(ANIM_NORMAL);

                const PaneVisibility* paneVisibility = mpLangDepData->pPaneVisibility;
                for (u16 i = 0; i < paneVisibility->visibleCount; i++) {
                    setVisible(paneVisibility->visibleNames[i], true);
                }
                for (u16 i = 0; i < paneVisibility->invisibleCount; i++) {
                    setVisible(paneVisibility->invisibleNames[i], false);
                }

                if (getLanguage() == JP) {
                    setString("T_JP_Chng_sign", langindependent::cLanguageIndependentString[langindependent::LANG_STRID_SIGN_WINDOW][getLanguage()]);
                } else {
                    setString("T_USEU_Chng_sign",
                              langindependent::cLanguageIndependentString[langindependent::LANG_STRID_SIGN_WINDOW][getLanguage()]);
                }

                setString("T_key_SPACE", langindependent::cLanguageIndependentString[langindependent::LANG_STRID_SPACE][getLanguage()]);
                onlyQwerty(false);
                setLangKeyActive(true);

                CommandReceiver::ChangePredictMode predictMode = {};
                sendCommand(CommandReceiver::INPUT_COMMAND_GET_PREDICT_MODE, &predictMode);
                updatePredictLanguage(&predictMode);
                mpLayout->Animate();
                mpLayout->CalculateMtx(mDrawInfo);

                setVisible("T_key_SPACE", true);
                setVisible("T_Gkey_SPACE", true);
                setVisible("P_key_HENKAN", false);
                setVisible("P_Gkey_HENKAN", false);
            }
            void LayoutByNW4R::initLayout() {
                if (getLanguage() == JP) {
                    if (isABC()) {
                        searchAnmPane("W_JP_Chng_ABC")->changeAnimation(ANIM_TOGGLE_ON);
                        searchAnmPane("W_JP_Chng_KANA")->changeAnimation(ANIM_NORMAL);
                    } else {
                        searchAnmPane("W_JP_Chng_ABC")->changeAnimation(ANIM_NORMAL);
                        searchAnmPane("W_JP_Chng_KANA")->changeAnimation(ANIM_TOGGLE_ON);
                    }

                    if (mKeyState.getAIUInputModeInternal() == Base::KeyState::IM_AIU_HIRAGANA) {
                        searchAnmPane("P_hiragana")->changeAnimation(ANIM_TOGGLE_ON);
                        searchAnmPane("P_katakana")->changeAnimation(ANIM_NORMAL);
                    } else {
                        searchAnmPane("P_hiragana")->changeAnimation(ANIM_NORMAL);
                        searchAnmPane("P_katakana")->changeAnimation(ANIM_TOGGLE_ON);
                    }

                    if (mKeyState.isAIUKanaDakuten()) {
                        searchAnmPane("P_Gkey_dakuten")->changeAnimation(ANIM_ACTIVE_NORMAL);
                    } else {
                        searchAnmPane("P_Gkey_dakuten")->changeAnimation(ANIM_INACTIVE_NORMAL);
                    }
                    if (mKeyState.isAIUKanaHandaku()) {
                        searchAnmPane("P_Gkey_handaku")->changeAnimation(ANIM_ACTIVE_NORMAL);
                    } else {
                        searchAnmPane("P_Gkey_handaku")->changeAnimation(ANIM_INACTIVE_NORMAL);
                    }
                    if (mKeyState.isAIUKanaSmall()) {
                        searchAnmPane("P_Gkey_komoji")->changeAnimation(ANIM_ACTIVE_NORMAL);
                    } else {
                        searchAnmPane("P_Gkey_komoji")->changeAnimation(ANIM_INACTIVE_NORMAL);
                    }

                    switch (mKeyState.getABCInputModeInternal()) {
                        case Base::KeyState::IM_ABC_DIRECT:
                            searchAnmPane("P_Mode_roma_hira")->changeAnimation(ANIM_NORMAL);
                            searchAnmPane("P_Mode_roma_kata")->changeAnimation(ANIM_NORMAL);
                            searchAnmPane("P_Mode_direct")->changeAnimation(ANIM_TOGGLE_ON);
                            break;
                        case Base::KeyState::IM_ABC_HIRAGANA:
                            searchAnmPane("P_Mode_roma_hira")->changeAnimation(ANIM_TOGGLE_ON);
                            searchAnmPane("P_Mode_roma_kata")->changeAnimation(ANIM_NORMAL);
                            searchAnmPane("P_Mode_direct")->changeAnimation(ANIM_NORMAL);
                            break;
                        case Base::KeyState::IM_ABC_KATAKANA:
                            searchAnmPane("P_Mode_roma_hira")->changeAnimation(ANIM_NORMAL);
                            searchAnmPane("P_Mode_roma_kata")->changeAnimation(ANIM_TOGGLE_ON);
                            searchAnmPane("P_Mode_direct")->changeAnimation(ANIM_NORMAL);
                            break;
                    }
                }
                if (getLanguage() == KR || getLanguage() == CN) {
                    switch (mKeyState.getABCInputModeInternal()) {
                        case Base::KeyState::IM_ABC_DIRECT:
                            searchAnmPane("P_Mode_kr_eng")->changeAnimation(ANIM_TOGGLE_ON);
                            searchAnmPane("P_Mode_kr_han")->changeAnimation(ANIM_NORMAL);
                            break;
                        default:
                            searchAnmPane("P_Mode_kr_eng")->changeAnimation(ANIM_NORMAL);
                            searchAnmPane("P_Mode_kr_han")->changeAnimation(ANIM_TOGGLE_ON);
                            break;
                    }
                }
                mpLayout->Animate();
                mpLayout->CalculateMtx(mDrawInfo);
            }
            void LayoutByNW4R::calc() {
                Layout::calc();
                if (mpManager->getInputForm()->canConvert()) {
                    setVisible("T_key_SPACE", false);
                    setVisible("T_Gkey_SPACE", false);
                    setVisible("P_key_HENKAN", true);
                    setVisible("P_Gkey_HENKAN", true);
                } else {
                    setVisible("T_key_SPACE", true);
                    setVisible("T_Gkey_SPACE", true);
                    setVisible("P_key_HENKAN", false);
                    setVisible("P_Gkey_HENKAN", false);
                }
            }
            void LayoutByNW4R::draw() {
                Layout::draw();
            }
            void LayoutByNW4R::inputCharCode(wchar_t ch) {
                sendInputWChar(ch, false);
            }

            inline void LayoutByNW4R::onPressedCaps() {
                LayoutGather& lg = LayoutGather::Singleton::getInstance();
                if (!lg.isHoldingShift()) {
                    if (mKeyState.getABCKanaFlags() != 0) {
                        mKeyState.setABCShift(false);
                        mKeyState.refresh_();
                    }
                    if (mShiftButton.isActive()) {
                        mShiftButton.setActive(false);
                        mShiftButton.getAnmPane()->onAnmEvent(nw4rmanager::AnmPane::PE_5);
                    }
                }
                lg.changeCapsLock(!lg.isCapsLock());

                mKeyState.toggleABCCaps();
                mKeyState.refresh_();

                bool isHandaku = mKeyState.isABCCaps();
                mCapsButton.setActive(isHandaku);
                if (isHandaku) {
                    mCapsButton.getAnmPane()->onAnmEvent(nw4rmanager::AnmPane::PE_10);
                } else {
                    mCapsButton.getAnmPane()->onAnmEvent(nw4rmanager::AnmPane::PE_11);
                }
                mpEventObserver->onSE(sound::SE_KETAI_MODE_SWITCHING);
            }
            void LayoutByNW4R::onPressedShift(bool playSound) {
                mKeyState.enableABCFlag(KeyState::KANAFLAG_SMALL);

                mShiftButton.setActive(true);
                mShiftButton.getAnmPane()->onAnmEvent(nw4rmanager::AnmPane::PE_10);

                // TODO: Why is there a bctrl here?

                if (playSound) {
                    mpEventObserver->onSE(sound::SE_KETAI_MODE_SWITCHING);
                }
            }
            void LayoutByNW4R::onReleasedShift() {
                // Figure out how to get mwcc to NOT optimize the 2 bitwise ops
                mKeyState.disableABCFlag(KeyState::KANAFLAG_SMALL);

                if (mShiftButton.isActive()) {
                    mShiftButton.setActive(false);
                    mShiftButton.getAnmPane()->onAnmEvent(nw4rmanager::AnmPane::PE_5);
                }
            }
            void LayoutByNW4R::onKey(u32 key, void* data) {
                getWCCode((char*)data);
                if (key == 4 && getWCCode((char*)data) == L'\0') {
                    switch (getControlKey((char*)data)) {
                        case VCODE_CAPS:
                            onPressedCaps();
                            break;

                        case VCODE_SHIFT: {
                            LayoutGather& lg = LayoutGather::Singleton::getInstance();
                            if (mKeyState.getABCKanaFlags() != 0) {
                                mKeyState.setABCCaps(false);
                                mKeyState.refresh_();
                            }
                            if (mCapsButton.isActive()) {
                                mCapsButton.setActive(false);
                                mCapsButton.getAnmPane()->onAnmEvent(nw4rmanager::AnmPane::PE_5);
                            }
                            if (!lg.isHoldingShift()) {
                                mKeyState.toggleABCShift();
                                mKeyState.refresh_();
                            }

                            bool isHandaku = mKeyState.isABCShift();
                            mShiftButton.setActive(isHandaku);
                            if (isHandaku) {
                                mShiftButton.getAnmPane()->onAnmEvent(nw4rmanager::AnmPane::PE_10);
                            } else {
                                mShiftButton.getAnmPane()->onAnmEvent(nw4rmanager::AnmPane::PE_11);
                            }
                            mpEventObserver->onSE(sound::SE_KETAI_MODE_SWITCHING);
                            break;
                        }
                        case VCODE_RETURN:
                            mpEventObserver->onSE(sound::SE_CHAR_DECIDE);
                            break;
                        case CODE_UNK_0x0B:
                            if (!isABC()) {
                                mpEventObserver->onSE(sound::SE_KETAI_MODE_SWITCHING);
                                searchAnmPane("W_JP_Chng_KANA")->changeAnimation(ANIM_TOGGLE_OFF);
                            }
                            break;
                        case VCODE_QWERTY_OFF:
                            if (isABC()) {
                                mpEventObserver->onSE(sound::SE_KETAI_MODE_SWITCHING);
                                searchAnmPane("W_JP_Chng_ABC")->changeAnimation(ANIM_TOGGLE_OFF);
                            }
                            break;
                        case CODE_UNK_0x12:
                            if (mKeyState.aiuInputModeInternalIs(Base::KeyState::IM_AIU_KATAKANA)) {
                                mpEventObserver->onSE(sound::SE_KETAI_MODE_SWITCHING);
                                searchAnmPane("P_katakana")->changeAnimation(ANIM_TOGGLE_OFF);
                            }
                            break;
                        case CODE_UNK_0x13:
                            if (mKeyState.aiuInputModeInternalIs(Base::KeyState::IM_AIU_HIRAGANA)) {
                                mpEventObserver->onSE(sound::SE_KETAI_MODE_SWITCHING);
                                searchAnmPane("P_hiragana")->changeAnimation(ANIM_TOGGLE_OFF);
                            }
                            break;
                        case VCODE_TRANSLATEMODE_0:
                            // if (mKeyState.abcInputModeInternalIs(Base::KeyState::IM_ABC_DIRECT)) {
                            //     searchAnmPane("P_Mode_direct")->changeAnimation(ANIM_TOGGLE_OFF);
                            //     searchAnmPane("P_Mode_kr_eng")->changeAnimation(ANIM_TOGGLE_OFF);
                            // }
                            if (mKeyState.abcInputModeInternalIs(Base::KeyState::IM_ABC_KATAKANA)) {
                                searchAnmPane("P_Mode_roma_kata")->changeAnimation(ANIM_TOGGLE_OFF);
                            }
                            if (mKeyState.abcInputModeInternalIs(Base::KeyState::IM_ABC_HIRAGANA)) {
                                searchAnmPane("P_Mode_roma_hira")->changeAnimation(ANIM_TOGGLE_OFF);
                                searchAnmPane("P_Mode_kr_han")->changeAnimation(ANIM_TOGGLE_OFF);
                            }
                            break;
                        case VCODE_TRANSLATEMODE_1:
                            if (mKeyState.abcInputModeInternalIs(Base::KeyState::IM_ABC_DIRECT)) {
                                searchAnmPane("P_Mode_direct")->changeAnimation(ANIM_TOGGLE_OFF);
                                searchAnmPane("P_Mode_kr_eng")->changeAnimation(ANIM_TOGGLE_OFF);
                            }
                            if (mKeyState.abcInputModeInternalIs(Base::KeyState::IM_ABC_KATAKANA)) {
                                searchAnmPane("P_Mode_roma_kata")->changeAnimation(ANIM_TOGGLE_OFF);
                            }
                            // if (mKeyState.abcInputModeInternalIs(Base::KeyState::IM_ABC_HIRAGANA)) {
                            //     searchAnmPane("P_Mode_roma_hira")->changeAnimation(ANIM_TOGGLE_OFF);
                            //     searchAnmPane("P_Mode_kr_han")->changeAnimation(ANIM_TOGGLE_OFF);
                            // }
                            break;
                        case VCODE_TRANSLATEMODE_2:
                            if (mKeyState.abcInputModeInternalIs(Base::KeyState::IM_ABC_DIRECT)) {
                                searchAnmPane("P_Mode_direct")->changeAnimation(ANIM_TOGGLE_OFF);
                                // searchAnmPane("P_Mode_kr_eng")->changeAnimation(ANIM_TOGGLE_OFF);
                            }
                            // if (mKeyState.abcInputModeInternalIs(Base::KeyState::IM_ABC_KATAKANA)) {
                            //     searchAnmPane("P_Mode_roma_kata")->changeAnimation(ANIM_TOGGLE_OFF);
                            // }
                            if (mKeyState.abcInputModeInternalIs(Base::KeyState::IM_ABC_HIRAGANA)) {
                                searchAnmPane("P_Mode_roma_hira")->changeAnimation(ANIM_TOGGLE_OFF);
                                // searchAnmPane("P_Mode_kr_han")->changeAnimation(ANIM_TOGGLE_OFF);
                            }
                            break;
                        default:
                            break;
                    }
                }
                Base::onKey(key, data);
            }
            void LayoutByNW4R::setLanguage(Language language) {
                meLanguage = language;
                mKeyState.meLanguage = language;
                mKeyState.refresh_();
                init();
            }
            void LayoutByNW4R::updateFromReceiver(u32 cmd, void* data) {
                Base::updateFromReceiver(cmd, data);
                updateDakuten();
                switch (cmd) {
                    case CommandReceiver::INPUT_COMMAND_SET_PREDICT_MODE:
                        updatePredictLanguage((CommandReceiver::ChangePredictMode*)data);
                        break;
                }
            }
            void LayoutByNW4R::onActive() {
                Base::onActive();
                Layout::init();
                if (mKeyState.isAIUKanaDakuten()) {
                    searchAnmPane("P_Gkey_dakuten")->changeAnimation(ANIM_ACTIVE_NORMAL);
                } else {
                    searchAnmPane("P_Gkey_dakuten")->changeAnimation(ANIM_INACTIVE_NORMAL);
                }
                if (mKeyState.isAIUKanaHandaku()) {
                    searchAnmPane("P_Gkey_handaku")->changeAnimation(ANIM_ACTIVE_NORMAL);
                } else {
                    searchAnmPane("P_Gkey_handaku")->changeAnimation(ANIM_INACTIVE_NORMAL);
                }
                if (mKeyState.isAIUKanaSmall()) {
                    searchAnmPane("P_Gkey_komoji")->changeAnimation(ANIM_ACTIVE_NORMAL);
                } else {
                    searchAnmPane("P_Gkey_komoji")->changeAnimation(ANIM_INACTIVE_NORMAL);
                }
            }
            void LayoutByNW4R::onClose() {
                initPaneLastDrawReceived();
                cancelStateFocusIn();
            }
            void LayoutByNW4R::throwReleaseForAll() {
                for (AnmPane* curr = (AnmPane*)nw4r::ut::List_GetFirst(&mAnmPanes); curr != NULL;
                     curr = (AnmPane*)nw4r::ut::List_GetNext(&mAnmPanes, curr)) {
                    if (curr->getKeyType() == KEY_TYPE_NORMAL) {
                        curr->onAnmEvent(nw4rmanager::AnmPane::PE_2);
                    }
                }
            }
            void LayoutByNW4R::cancelStateFocusIn() {
                for (AnmPane* curr = (AnmPane*)nw4r::ut::List_GetFirst(&mAnmPanes); curr != NULL;
                     curr = (AnmPane*)nw4r::ut::List_GetNext(&mAnmPanes, curr)) {
                    switch (curr->getKeyType()) {
                        case KEY_TYPE_NORMAL:
                            switch (curr->getState()) {
                                case ANIM_FOCUS_IN:
                                case ANIM_ROLL_OVER:
                                case ANIM_PUSHED:
                                    curr->changeAnimation(ANIM_FOCUS_OUT);
                                    break;
                                case ANIM_UNK_16:
                                    curr->changeAnimation(ANIM_NORMAL);
                                    break;
                                default:
                                    break;
                            }
                            break;
                        case KEY_TYPE_SHIFTCAPS:
                            switch (curr->getState()) {
                                case ANIM_FOCUS_IN:
                                case ANIM_ROLL_OVER:
                                case ANIM_TOGGLE_PUSHED:
                                    curr->changeAnimation(ANIM_FOCUS_OUT);
                                    break;

                                case ANIM_PUSHED:
                                case ANIM_TOGGLE_ON:
                                case ANIM_TOGGLE_FOCUS_IN:
                                    curr->changeAnimation(ANIM_TOGGLE_FOCUS_OUT);
                                    break;
                                default:
                                    break;
                            }
                            break;
                        case KEY_TYPE_TOGGLE:
                            switch (curr->getState()) {
                                case ANIM_FOCUS_IN:
                                case ANIM_ROLL_OVER:
                                    curr->changeAnimation(ANIM_FOCUS_OUT);
                                    break;
                                case ANIM_PUSHED:
                                    curr->changeAnimation(ANIM_TOGGLE_ON);
                                    break;
                                default:
                                    break;
                            }
                            break;
                        case KEY_TYPE_ONOFF:
                            switch (curr->getState()) {
                                case ANIM_FOCUS_IN:
                                case ANIM_ROLL_OVER:
                                case ANIM_PUSHED:
                                    curr->changeAnimation(ANIM_FOCUS_OUT);
                                    break;
                                default:
                                    break;
                            }
                            break;
                    }
                }
            }
            void LayoutByNW4R::goSignInputMode() {
                mpSignWindow->open(this, false);
                throwReleaseForAll();
            }
            void LayoutByNW4R::changePredictLanguage() {
                throwReleaseForAll();
                CommandReceiver::ChangePredictMode predictMode = {};
                sendCommand(CommandReceiver::INPUT_COMMAND_GET_PREDICT_MODE, &predictMode);
                mpPredictLanguageDialog->open((inputform::Base::PredictMode)predictMode.ePredictMode, this);
            }
            void LayoutByNW4R::updatePredictLanguage(CommandReceiver::ChangePredictMode* cmdData) {
                if (getLanguage() != JP) {
                    if (!cmdData->bPredictEnabled) {
                        setVisible("P_prdc_ON", false);
                        setVisible("P_prdc_OFF", true);
                    } else {
                        setVisible("P_prdc_ON", true);
                        setVisible("P_prdc_OFF", false);
                    }
                    ((nw4r::lyt::TextBox*)getPane("T_USEU_prdc_lang"))->SetString(csszPredictLanguage[cmdData->ePredictMode]);
                }
            }
            void LayoutByNW4R::sendInputWChar(wchar_t ch, bool flag) {
                bool isSmall = false;
                nw4r::lyt::Pane* pane = NULL;
                if (mKeyState.isABCShift()) {
                    pane = mAnmPaneFifo.getLast();
                    isSmall = true;
                }
                Base::sendInputWChar(ch, flag);
                if (isSmall && pane != NULL) {
                    setPaneLastDrawReceived(pane);
                }
                updateDakuten();
            }
            void LayoutByNW4R::updateDakuten() {
                if (mKeyState.isAIUKanaDakuten()) {
                    searchAnmPane("P_Gkey_dakuten")->onAnmEvent(nw4rmanager::AnmPane::PE_6);
                } else {
                    searchAnmPane("P_Gkey_dakuten")->onAnmEvent(nw4rmanager::AnmPane::PE_7);
                }
                if (mKeyState.isAIUKanaHandaku()) {
                    searchAnmPane("P_Gkey_handaku")->onAnmEvent(nw4rmanager::AnmPane::PE_6);
                } else {
                    searchAnmPane("P_Gkey_handaku")->onAnmEvent(nw4rmanager::AnmPane::PE_7);
                }
                if (mKeyState.isAIUKanaSmall()) {
                    searchAnmPane("P_Gkey_komoji")->onAnmEvent(nw4rmanager::AnmPane::PE_6);
                } else {
                    searchAnmPane("P_Gkey_komoji")->onAnmEvent(nw4rmanager::AnmPane::PE_7);
                }
            }
            void LayoutByNW4R::setLineFeedButton(bool lineFeedButtonOn) {
                mbLineFeedButtonEnabled = lineFeedButtonOn;
                setVisible("P_key_LF", mbLineFeedButtonEnabled);
                setVisible("P_Gkey_LF", mbLineFeedButtonEnabled);
            }
            void LayoutByNW4R::setPredictLanguageButton(bool prdcLangButtonOn) {
                setVisible("W_USEU_prdc_lang", prdcLangButtonOn);
            }
            void LayoutByNW4R::setSignWindowButton(bool signWindowButtonOn) {
                if (!signWindowButtonOn) {
                    setVisible("W_USEU_Chng_sign", signWindowButtonOn);
                    setVisible("W_JP_Chng_sign", signWindowButtonOn);
                } else {
                    setVisible("W_USEU_Chng_sign", false);
                    setVisible("W_JP_Chng_sign", false);
                    if (getLanguage() == JP) {
                        setVisible("W_JP_Chng_sign", true);
                    } else {
                        setVisible("W_USEU_Chng_sign", true);
                    }
                }
            }
            void LayoutByNW4R::onlyQwerty(bool isOnlyQwerty) {
                mbIsOnlyQwerty = isOnlyQwerty;
                if (getLanguage() == JP) {
                    if (isOnlyQwerty) {
                        setABC(true);
                        mKeyState.setABCFlag(0);
                        changeABCInputMode(IM_00);
                    } else {
                        setABC(false);
                        changeAIUInputMode(IM_HIRAGANA);
                    }
                }
                if (getLanguage() == JP) {
                    initLayout();
                    setVisible("W_JP_Chng_ABC", !isOnlyQwerty);
                    setVisible("W_JP_Chng_KANA", !isOnlyQwerty);
                    setVisible("P_Mode_roma_hira", !isOnlyQwerty);
                    setVisible("P_Mode_roma_kata", !isOnlyQwerty);
                    setVisible("P_Mode_direct", !isOnlyQwerty);
                    setVisible("P_romajiBox", !isOnlyQwerty);
                } else if (getLanguage() == KR || getLanguage() == CN) {
                    if (mpManager->getCandidateBox()->isActive()) {
                        setLangKeyActive(!isOnlyQwerty);
                    } else {
                        setLangKeyActive(false);
                    }
                }
            }
            void LayoutByNW4R::setLangKeyActive(bool isLangKeyActive) {
                mbIsLangKeyActive = isLangKeyActive;
                if (getLanguage() == KR || getLanguage() == CN) {
                    if (!isLangKeyActive) {
                        mKeyState.setABCFlag(0);
                        changeABCInputMode(IM_00);
                    }
                }
                if (getLanguage() == KR || getLanguage() == CN) {
                    initLayout();
                    setVisible("P_Mode_kr_eng", isLangKeyActive);
                    setVisible("P_Mode_kr_han", isLangKeyActive);
                    setVisible("P_hangulBox", isLangKeyActive);
                    if (getLanguage() == CN) {
                        setVisible("P_key_42", isLangKeyActive);
                        setVisible("P_key_43", isLangKeyActive);
                    }
                }
            }
            void LayoutByNW4R::setInputModeJP(bool isAbc, u32 abcMode, u32 aiuMode) {
                setABC(isAbc);
                mKeyState.setABCInputModeInternal((KeyState::InternalABCInputMode)abcMode);
                mKeyState.setAIUInputModeInternal((KeyState::InternalAIUInputMode)aiuMode);

                switch (abcMode) {
                    case KeyState::IM_ABC_DIRECT:
                        mpLangDepData = &csLanguageDependencyData[getLanguage()];
                        break;
                    case Base::KeyState::IM_ABC_HIRAGANA:
                        mpLangDepData = &csJapanKanaInput;
                        break;
                    case Base::KeyState::IM_ABC_KATAKANA:
                        mpLangDepData = &csJapanKanaInput;
                        break;
                }
                initLayout();
                sendCommand(CommandReceiver::INPUT_COMMAND_41, NULL);
            }
            void LayoutByNW4R::setTranslateMode(TranslateMode mode) {
                KeyState::InternalABCInputMode oldInputMode = mKeyState.getABCInputModeInternal();
                Base::setTranslateMode(mode);
                if (!mKeyState.abcInputModeInternalIs(oldInputMode)) {
                    mpEventObserver->onSE(sound::SE_KETAI_MODE_SWITCHING);
                    switch (oldInputMode) {
                        case KeyState::IM_ABC_DIRECT:
                            searchAnmPane("P_Mode_direct")->changeAnimation(ANIM_TOGGLE_OFF);
                            searchAnmPane("P_Mode_kr_eng")->changeAnimation(ANIM_TOGGLE_OFF);
                            break;
                        case KeyState::IM_ABC_HIRAGANA:
                            searchAnmPane("P_Mode_roma_hira")->changeAnimation(ANIM_TOGGLE_OFF);
                            searchAnmPane("P_Mode_kr_han")->changeAnimation(ANIM_TOGGLE_OFF);
                            break;
                        case KeyState::IM_ABC_KATAKANA:
                            searchAnmPane("P_Mode_roma_kata")->changeAnimation(ANIM_TOGGLE_OFF);
                            break;
                    }
                    switch (mode) {
                        case Base::TM_00:
                            searchAnmPane("P_Mode_direct")->onAnmEvent(nw4rmanager::AnmPane::PE_0);
                            searchAnmPane("P_Mode_kr_eng")->onAnmEvent(nw4rmanager::AnmPane::PE_0);
                            break;
                        case Base::TM_01:
                            searchAnmPane("P_Mode_roma_hira")->onAnmEvent(nw4rmanager::AnmPane::PE_0);
                            searchAnmPane("P_Mode_kr_han")->onAnmEvent(nw4rmanager::AnmPane::PE_0);
                            break;
                        case Base::TM_02:
                            searchAnmPane("P_Mode_roma_kata")->onAnmEvent(nw4rmanager::AnmPane::PE_0);
                            break;
                    }
                }
            }

            bool LayoutByNW4R::updateInput(int chan, f32 x, f32 y, u32 trig, u32 hold, u32 release, void* data) {
                bool ret = Layout::updateInput(chan, x, y, trig, hold, release, data);

                LayoutGather& lg = LayoutGather::Singleton::getInstance();
                bool isHoldingShift = lg.isHoldingShift();

                if (hold & 0x400) {
                    lg.setPressedShiftB(true);
                    input::HKBManager::getInstance().SetForceModifierState(KBD_HID_MOD_SHIFT, KBD_HID_MOD_SHIFT);
                    if (!isHoldingShift) {
                        onPressedShift(isABC());
                    }
                } else {
                    lg.setPressedShiftB(false);
                    input::HKBManager::getInstance().SetForceModifierState(0, 0);
                    if (isHoldingShift) {
                        if (!lg.isHoldingShift()) {
                            onReleasedShift();
                        }
                    }
                }
                return ret;
            }
            bool LayoutByNW4R::updateInput(input::HKBManager& hkbManager) {
                input::HKBManager::KeySet curr = hkbManager.GetTriggeredKeySet();
                for (; curr.IsValid(); curr = curr.GetNext()) {
                    nw4rmanager::AnmPane* pane = NULL;
                    u8 keyCode = curr.GetKey();
                    wchar_t ch = curr.GetWChar();
                    switch (keyCode) {
                        case KBD_KEY_KEYBOARD_SPACEBAR:
                            break;

                        case KBD_KEY_KEYBOARD_ENTER:
                        case KBD_KEY_KEYPAD_ENTER:
                            if (hkbManager.GetModifierState() & KBD_HID_MOD_ALT)
                                break;
                            pane = searchAnmPane("P_key_LF");
                            break;

                        default:
                            pane = searchAnmPane(transformHardwareWChar(ch));
                            break;
                    }
                    if (pane != NULL) {
                        pane->onAnmEvent(nw4rmanager::AnmPane::PE_0);
                    }
                }
                curr = hkbManager.GetRepeatedKeySet();
                for (; curr.IsValid(); curr = curr.GetNext()) {
                    nw4rmanager::AnmPane* pane = NULL;
                    u8 keyCode = curr.GetKey();
                    wchar_t ch = curr.GetWChar();
                    switch (keyCode) {
                        case KBD_KEY_KEYBOARD_BACKSPACE:
                            if (hkbManager.GetModifierState() & KBD_HID_MOD_ALT) {
                                break;
                            }
                        case KBD_KEY_KEYBOARD_DELETE:
                            pane = searchAnmPane("P_key_DELETE");
                            break;

                        case KBD_KEY_KEYBOARD_SPACEBAR:
                            if (hkbManager.GetModifierState() & KBD_HID_MOD_GUI)
                                break;
                            pane = searchAnmPane("P_key_SPACE");
                            break;

                        default:
                            break;
                    }
                    if (pane != NULL) {
                        pane->onAnmEvent(nw4rmanager::AnmPane::PE_0);
                    }
                }
                return false;
            }

            void LayoutByNW4R::changeAnimationAllToNormal() {
                for (AnmPane* curr = (AnmPane*)nw4r::ut::List_GetFirst(&mAnmPanes); curr != NULL;
                     curr = (AnmPane*)nw4r::ut::List_GetNext(&mAnmPanes, curr)) {
                    if (curr->getKeyType() == KEY_TYPE_NORMAL) {
                        curr->changeAnimation(ANIM_NORMAL);
                    }
                }
                if (mKeyState.aiuInputModeInternalIs(Base::KeyState::IM_AIU_HIRAGANA)) {
                    searchAnmPane("P_hiragana")->changeAnimation(ANIM_TOGGLE_ON);
                    searchAnmPane("P_katakana")->changeAnimation(ANIM_NORMAL);
                } else {
                    searchAnmPane("P_hiragana")->changeAnimation(ANIM_NORMAL);
                    searchAnmPane("P_katakana")->changeAnimation(ANIM_TOGGLE_ON);
                }
            }

            void LayoutByNW4R::refreshState() {
                bool gridChanged;
                if (isABC()) {
                    if (isVisible("N_VK_grid", &gridChanged)) {
                        gridChanged = gridChanged != false;
                    } else {
                        gridChanged = false;
                    }
                    setVisible("N_VK_grid", false);
                    setVisible("N_VK_grd_Bnd_ALL", false);
                    setVisible("N_VK_ascii", true);
                    setVisible("N_VK_asc_Bnd_ALL", true);
                } else {
                    if (isVisible("N_VK_grid", &gridChanged)) {
                        gridChanged = gridChanged != true;
                    } else {
                        gridChanged = false;
                    }
                    setVisible("N_VK_grid", true);
                    setVisible("N_VK_grd_Bnd_ALL", true);
                    setVisible("N_VK_ascii", false);
                    setVisible("N_VK_asc_Bnd_ALL", false);
                }
                mKeyState.refreshText(mpLayout->GetRootPane());

                if (getLanguage() == JP) {
                    if (mKeyState.aiuInputModeInternalIs(Base::KeyState::IM_AIU_HIRAGANA)) {
                        ((nw4r::lyt::TextBox*)getPane("T_JP_Chng_KANA"))->SetString(L"あいう");
                    } else if (mKeyState.aiuInputModeInternalIs(Base::KeyState::IM_AIU_KATAKANA)) {
                        ((nw4r::lyt::TextBox*)getPane("T_JP_Chng_KANA"))->SetString(L"アイウ");
                    }
                }
                if (!isShiftOn()) {
                    if (mShiftButton.isActive()) {
                        mShiftButton.setActive(false);
                        mShiftButton.getAnmPane()->onAnmEvent(nw4rmanager::AnmPane::PE_5);
                    }
                }
                if (!isCapsOn()) {
                    if (mCapsButton.isActive()) {
                        mCapsButton.setActive(false);
                        mCapsButton.getAnmPane()->onAnmEvent(nw4rmanager::AnmPane::PE_5);
                    }

                    LayoutGather& lg = LayoutGather::Singleton::getInstance();
                    lg.changeCapsLock(0);
                }
                if (gridChanged) {
                    initPaneLastDrawReceived();
                }
            }
            void LayoutByNW4R::onEvent(UIObj* obj, u32 id, void* sound) {
                if (id == 0) {
                    mpEventObserver->onSE((sound::SE)(u32)sound);
                }
            }
            void EventHandler::onTiEvent(gui::PaneComponent* paneComponent, u32 event, Input* input) {
                const char* __s2 = paneComponent->getPane()->GetName();
                if (util::strcmp("B_key_SHIFT", __s2) || util::strcmp("B_key_CAPS", __s2))
                    return;
                if (__s2[0] != 'B')
                    return;

                char paneName[0x11];
                util::replaceChar(paneName, 0x11, __s2, 0, 'P');
                if (strncmp(__s2 + 5, "Chng", 4) == 0 || strncmp(__s2 + 7, "Chng", 4) == 0 || strncmp(__s2 + 7, "prdc", 4) == 0) {
                    util::replaceChar(paneName, 0x11, __s2, 0, 'W');
                }

                nw4rmanager::AnmPane* anmPane = mpLayout->searchAnmPane(paneName);
                switch (event) {
                    case 4:
                        if (!(input->field_0x0C & 0x800))
                            break;
                        if (anmPane != NULL) {
                            anmPane->onAnmEvent(nw4rmanager::AnmPane::PE_0);
                        }
                        mpLayout->onKey(4, paneName);
                        break;
                    case 1:
                        if (anmPane != NULL) {
                            anmPane->onAnmEvent(nw4rmanager::AnmPane::PE_2);
                        }
                        mpLayout->onKey(1, paneName);
                        break;
                    case 0:
                        if (anmPane != NULL) {
                            mpEventObserver->onSE(sound::SE_SELECT);
                            mpLayout->setPaneLastDrawReceived(anmPane->getPane());
                            anmPane->onAnmEvent(nw4rmanager::AnmPane::PE_1);
                        }
                        break;
                }

                if (event == 2 && (input->field_0x10 & 0x800) && !(input->field_0x0C & 0x800)) {
                    if (util::strcmp("P_key_DELETE", paneName) || util::strcmp("P_Gkey_DELETE", paneName) || util::strcmp("P_key_SPACE", paneName) ||
                        util::strcmp("P_Gkey_SPACE", paneName)) {
                        if (paneComponent->isDragging(input->field_0x00)) {
                            u32 flightDuration = mpLayout->getFlightDuration(input->field_0x00, __s2);
                            if (flightDuration >= 30 && flightDuration % 9 == 0) {
                                anmPane->onAnmEvent(nw4rmanager::AnmPane::PE_0);
                                mpLayout->onKey(4, paneName);
                            }
                        }
                    }
                }
            }

            void NormalButtonAnmPane::onAnmEvent(AnmPaneEvent paneEvent) {
                switch (meCurrAnimId) {
                    case ANIM_NORMAL:
                        if (paneEvent == PE_1)
                            changeAnimation(ANIM_FOCUS_IN);
                        if (paneEvent == PE_0)
                            changeAnimation(ANIM_UNK_16);
                        break;
                    case ANIM_FOCUS_IN:
                        if (paneEvent == PE_4)
                            changeAnimation(ANIM_ROLL_OVER);
                        if (paneEvent == PE_2)
                            changeAnimation(ANIM_FOCUS_OUT);
                        if (paneEvent == PE_0)
                            changeAnimation(ANIM_PUSHED);
                        break;
                    case ANIM_ROLL_OVER:
                        if (paneEvent == PE_2)
                            changeAnimation(ANIM_FOCUS_OUT);
                        if (paneEvent == PE_0)
                            changeAnimation(ANIM_PUSHED);
                        break;
                    case ANIM_FOCUS_OUT:
                        if (paneEvent == PE_4)
                            changeAnimation(ANIM_NORMAL);
                        if (paneEvent == PE_1)
                            changeAnimation(ANIM_FOCUS_IN);
                        if (paneEvent == PE_0)
                            changeAnimation(ANIM_UNK_16);
                        break;
                    case ANIM_PUSHED:
                        if (paneEvent == PE_4)
                            changeAnimation(ANIM_ROLL_OVER);
                        if (paneEvent == PE_2)
                            changeAnimation(ANIM_FOCUS_OUT);
                        if (paneEvent == PE_0)
                            changeAnimation(ANIM_PUSHED);
                        break;
                    case ANIM_UNK_16:
                        if (paneEvent == PE_4)
                            changeAnimation(ANIM_NORMAL);
                        if (paneEvent == PE_1)
                            changeAnimation(ANIM_FOCUS_IN);
                        if (paneEvent == PE_0)
                            changeAnimation(ANIM_UNK_16);
                        break;
                    default:
                        break;
                }
            }

            bool ShiftCapsAnmPane::isFocused() const {
                switch (meCurrAnimId) {
                    case ANIM_NORMAL:
                    case ANIM_FOCUS_OUT:
                    case ANIM_TOGGLE_OFF:
                    case ANIM_TOGGLE_FOCUS_OUT:
                    case ANIM_TOGGLE_NORMAL:
                        return false;

                    default:
                        return true;
                }
            }
            void ShiftCapsAnmPane::onAnmEvent(AnmPaneEvent paneEvent) {
                if (paneEvent == PE_10) {
                    mbIsOn = true;
                    if (!isFocused()) {
                        changeAnimation(ANIM_TOGGLE_FOCUS_OUT);
                    } else {
                        changeAnimation(ANIM_PUSHED);
                    }
                } else if (paneEvent == PE_11) {
                    mbIsOn = false;
                    if (!isFocused()) {
                        changeAnimation(ANIM_FOCUS_OUT);
                    } else {
                        changeAnimation(ANIM_TOGGLE_PUSHED);
                    }
                } else if (paneEvent == PE_5) {
                    mbIsOn = false;
                    if (meCurrAnimId != ANIM_TOGGLE_FOCUS_OUT) {
                        if (meCurrAnimId == ANIM_TOGGLE_NORMAL) {
                            changeAnimation(ANIM_TOGGLE_OFF);
                        } else if (meCurrAnimId == ANIM_TOGGLE_FOCUS_IN || meCurrAnimId == ANIM_PUSHED || meCurrAnimId == ANIM_TOGGLE_ON) {
                            changeAnimation(ANIM_ROLL_OVER);
                        }
                    }
                } else if (!mbIsOn) {
                    switch (meCurrAnimId) {
                        case ANIM_TOGGLE_FOCUS_OUT:
                            if (paneEvent == PE_4)
                                changeAnimation(ANIM_NORMAL);
                            break;
                        case ANIM_TOGGLE_OFF:
                            if (paneEvent == PE_4)
                                changeAnimation(ANIM_NORMAL);
                        case ANIM_NORMAL:
                            if (paneEvent == PE_1)
                                changeAnimation(ANIM_FOCUS_IN);
                            break;
                        case ANIM_FOCUS_IN:
                            if (paneEvent == PE_4)
                                changeAnimation(ANIM_ROLL_OVER);
                            if (paneEvent == PE_2)
                                changeAnimation(ANIM_FOCUS_OUT);
                            break;
                        case ANIM_ROLL_OVER:
                            if (paneEvent == PE_2)
                                changeAnimation(ANIM_FOCUS_OUT);
                            break;
                        case ANIM_FOCUS_OUT:
                            if (paneEvent == PE_4)
                                changeAnimation(ANIM_NORMAL);
                            if (paneEvent == PE_1)
                                changeAnimation(ANIM_FOCUS_IN);
                            break;
                        case ANIM_TOGGLE_PUSHED:
                            if (paneEvent == PE_4)
                                changeAnimation(ANIM_ROLL_OVER);
                            if (paneEvent == PE_2)
                                changeAnimation(ANIM_FOCUS_OUT);
                            break;
                        default:
                            break;
                    }
                } else {
                    switch (meCurrAnimId) {
                        case ANIM_TOGGLE_OFF:
                            if (paneEvent == PE_4)
                                changeAnimation(ANIM_NORMAL);
                        case ANIM_NORMAL:
                            if (paneEvent == PE_1)
                                changeAnimation(ANIM_FOCUS_IN);
                            if (paneEvent == PE_0)
                                changeAnimation(ANIM_TOGGLE_FOCUS_OUT);
                            break;
                        case ANIM_TOGGLE_NORMAL:
                            if (paneEvent == PE_1)
                                changeAnimation(ANIM_TOGGLE_FOCUS_IN);
                            break;
                        case ANIM_TOGGLE_FOCUS_IN:
                            if (paneEvent == PE_4)
                                changeAnimation(ANIM_TOGGLE_ON);
                            if (paneEvent == PE_2)
                                changeAnimation(ANIM_TOGGLE_FOCUS_OUT);
                            break;
                        case ANIM_TOGGLE_ON:
                            if (paneEvent == PE_2)
                                changeAnimation(ANIM_TOGGLE_FOCUS_OUT);
                            break;
                        case ANIM_TOGGLE_FOCUS_OUT:
                            if (paneEvent == PE_4)
                                changeAnimation(ANIM_TOGGLE_NORMAL);
                            if (paneEvent == PE_1)
                                changeAnimation(ANIM_TOGGLE_FOCUS_IN);
                            break;
                        case ANIM_PUSHED:
                            if (paneEvent == PE_4)
                                changeAnimation(ANIM_TOGGLE_ON);
                            if (paneEvent == PE_2)
                                changeAnimation(ANIM_TOGGLE_FOCUS_OUT);
                            break;
                        case ANIM_TOGGLE_PUSHED:
                        default:
                            break;
                    }
                }
            }

            void ToggleButtonAnmPane::onAnmEvent(AnmPaneEvent paneEvent) {
                if (paneEvent == PE_0 && meCurrAnimId != ANIM_TOGGLE_ON && meCurrAnimId != ANIM_PUSHED) {
                    changeAnimation(ANIM_PUSHED);
                }
                switch (meCurrAnimId) {
                    case ANIM_NORMAL:
                        if (paneEvent == PE_1)
                            changeAnimation(ANIM_FOCUS_IN);
                        break;
                    case ANIM_FOCUS_IN:
                        if (paneEvent == PE_4)
                            changeAnimation(ANIM_ROLL_OVER);
                        if (paneEvent == PE_2)
                            changeAnimation(ANIM_FOCUS_OUT);
                        break;
                    case ANIM_ROLL_OVER:
                        if (paneEvent == PE_2)
                            changeAnimation(ANIM_FOCUS_OUT);
                        break;
                    case ANIM_FOCUS_OUT:
                        if (paneEvent == PE_4)
                            changeAnimation(ANIM_NORMAL);
                        if (paneEvent == PE_1)
                            changeAnimation(ANIM_FOCUS_IN);
                        break;
                    case ANIM_PUSHED:
                        if (paneEvent == PE_4)
                            changeAnimation(ANIM_TOGGLE_ON);
                        break;
                    case ANIM_TOGGLE_OFF:
                        if (paneEvent == PE_4)
                            changeAnimation(ANIM_NORMAL);
                        break;
                    default:
                        break;
                }
            }
            void OnOffButtonAnmPane::onAnmEvent(AnmPaneEvent paneEvent) {
                if (paneEvent == PE_0 && meCurrAnimId != ANIM_INACTIVE_NORMAL && meCurrAnimId != ANIM_FOCUS_OUT && meCurrAnimId != ANIM_ACTIVE_OFF) {
                    changeAnimation(ANIM_PUSHED);
                } else if (paneEvent == PE_6 && meCurrAnimId != ANIM_ACTIVE_NORMAL && meCurrAnimId != ANIM_PUSHED && meCurrAnimId != ANIM_ROLL_OVER &&
                           meCurrAnimId != ANIM_ACTIVE_ON && meCurrAnimId != ANIM_FOCUS_IN && meCurrAnimId != ANIM_FOCUS_OUT) {
                    changeAnimation(ANIM_ACTIVE_ON);
                } else if (paneEvent == PE_7 && meCurrAnimId != ANIM_INACTIVE_NORMAL) {
                    changeAnimation(ANIM_ACTIVE_OFF);
                } else {
                    switch (meCurrAnimId) {
                        case ANIM_ACTIVE_ON:
                            if (paneEvent == PE_4)
                                changeAnimation(ANIM_ACTIVE_NORMAL);
                            if (paneEvent == PE_1)
                                changeAnimation(ANIM_FOCUS_IN);
                            break;
                        case ANIM_ACTIVE_OFF:
                            if (paneEvent == PE_4)
                                changeAnimation(ANIM_INACTIVE_NORMAL);
                            break;
                        case ANIM_ACTIVE_NORMAL:
                            if (paneEvent == PE_1)
                                changeAnimation(ANIM_FOCUS_IN);
                            break;
                        case ANIM_FOCUS_IN:
                            if (paneEvent == PE_4)
                                changeAnimation(ANIM_ROLL_OVER);
                            if (paneEvent == PE_2)
                                changeAnimation(ANIM_FOCUS_OUT);
                            break;
                        case ANIM_ROLL_OVER:
                            if (paneEvent == PE_2)
                                changeAnimation(ANIM_FOCUS_OUT);
                            break;
                        case ANIM_FOCUS_OUT:
                            if (paneEvent == PE_4)
                                changeAnimation(ANIM_ACTIVE_NORMAL);
                            if (paneEvent == PE_1)
                                changeAnimation(ANIM_FOCUS_IN);
                            break;
                        case ANIM_PUSHED:
                            if (paneEvent == PE_4)
                                changeAnimation(ANIM_ROLL_OVER);
                            if (paneEvent == PE_2)
                                changeAnimation(ANIM_FOCUS_OUT);
                            break;
                        default:
                            break;
                    }
                }
            }

            UIModifierButton::UIModifierButton(u32 keyType, LayoutByNW4R* layout, UIObj::Listener* listener)
                : UIObj(keyType, layout, listener), mpPanePaneComponent(NULL), mpBoundingPaneComponent(NULL), mpAnmPane(NULL), mbIsActive(false) {
            }
            void UIModifierButton::Create(nw4rmanager::Layout* lyt, const char* paneNameP, const char* paneNameB) {
                gui::PaneManager* paneManager = lyt->getPaneManager();
                mpPanePaneComponent = paneManager->searchPaneComponent(paneNameP);
                mpBoundingPaneComponent = paneManager->searchPaneComponent(paneNameB);
                mpAnmPane = lyt->searchAnmPane(paneNameP);
                mpBoundingPaneComponent->setListener(this);
            }
            void UIModifierButton::setBoundingTriggerTarget(bool bEnable) {
                mpBoundingPaneComponent->setTriggerTarget(bEnable);
            }
            void UIModifierButton::onGUIEvent(gui::PaneComponent& comp, u32 event, nw4rmanager::TiEventHandler::Input* data) {
                const char* targetPaneName = keyType == 1 ? "P_key_SHIFT" : "P_key_CAPS";

                switch (event) {
                    case 4:
                        if (data->field_0x0C & 0x800) {
                            mpLayout->onKey(4, (void*)targetPaneName);
                        }
                        break;
                    case 1:
                        mpAnmPane->onAnmEvent(nw4rmanager::AnmPane::PE_2);
                        mpLayout->onKey(1, (void*)targetPaneName);
                        break;
                    case 0:
                        if (mpListener != NULL) {
                            mpListener->onEvent(this, 0, (void*)4);
                        }
                        mpLayout->setPaneLastDrawReceived(mpAnmPane->getPane());
                        mpAnmPane->onAnmEvent(nw4rmanager::AnmPane::PE_1);
                        break;
                }
            }

            UIModePanel::UIModePanel(u32 keyType, LayoutByNW4R* layout, UIObj::Listener* listener)
                : UIObj(keyType, layout, listener), unk_0x14(0), mpTextboxModeA(NULL), mpTextboxModeB(NULL), mpButtonModeSelect(NULL) {
                for (int i = 0; i < ARRAY_LENGTH(mpPanePCs); i++) {
                    mpPanePCs[i] = NULL;
                    mpBoundingPCs[i] = NULL;
                    mpAnmPanes[i] = NULL;
                }
            }
            void UIModePanel::setBoundingTriggerTargets(bool bEnable) {
                for (int i = 0; i < ARRAY_LENGTH(mpBoundingPCs); i++) {
                    mpBoundingPCs[i]->setTriggerTarget(bEnable);
                }
            }
            void UIModePanel::Create(nw4rmanager::Layout* lyt) {
                const char* paneNames[5] = {"P_Mode_direct", "P_Mode_roma_hira", "P_Mode_roma_kata", "P_Mode_kr_eng", "P_Mode_kr_han"};
                const char* boundingNames[5] = {"B_Mode_direct", "B_Mode_roma_hira", "B_Mode_roma_kata", "B_Mode_kr_eng", "B_Mode_kr_han"};

                gui::PaneManager* paneManager = lyt->getPaneManager();
                for (int i = 0; i < ARRAY_LENGTH(paneNames); i++) {
                    mpPanePCs[i] = paneManager->searchPaneComponent(paneNames[i]);
                    mpBoundingPCs[i] = paneManager->searchPaneComponent(boundingNames[i]);
                    mpAnmPanes[i] = lyt->searchAnmPane(paneNames[i]);
                    mpBoundingPCs[i]->setListener(this);
                }

                nw4r::lyt::Pane* rootPane = lyt->getLayout()->GetRootPane();
                mpTextboxModeA = rootPane->FindPaneByName("T_Mode_kr_eng");
                mpTextboxModeB = rootPane->FindPaneByName("T_Mode_kr_han");
                mpButtonModeSelect = rootPane->FindPaneByName("N_modeSelect_kr");

                rootPane->FindPaneByName("T_Mode_kr_eng")->GetMaterial()->GetTexture(&mTexKrEng, 0);
                rootPane->FindPaneByName("T_Mode_kr_han")->GetMaterial()->GetTexture(&mTexKrHan, 0);
                rootPane->FindPaneByName("T_Mode_direct")->GetMaterial()->GetTexture(&mTexDirect, 0);
                rootPane->FindPaneByName("T_Mode_cn_pinyin")->GetMaterial()->GetTexture(&mTexCnPinyin, 0);
            }
            void UIModePanel::onGUIEvent(gui::PaneComponent& comp, u32 event, nw4rmanager::TiEventHandler::Input* data) {
            }

            nw4r::math::_VEC3 modeButtonTranslateCn = {-219.0f, -130.0f, 0.0f};
            nw4r::math::_VEC3 modeButtonTranslateKr = {-209.0f, -90.0f, 0.0f};
        }  // namespace pctype
    }  // namespace keyboard
}  // namespace textinput
