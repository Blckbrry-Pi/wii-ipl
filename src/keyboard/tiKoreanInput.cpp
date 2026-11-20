#include "keyboard/tiKoreanInput.h"

namespace textinput {
#define JAMO_COUNT_BEGINNING 19
#define JAMO_COUNT_MIDDLE 21
#define JAMO_COUNT_ENDING 28

    struct BeginningJamo {
        wchar_t chars[JAMO_COUNT_BEGINNING];
    };
    struct MiddleJamo {
        wchar_t chars[JAMO_COUNT_MIDDLE];
    };
    struct EndingJamo {
        wchar_t chars[JAMO_COUNT_ENDING];
    };

    // KSX 1001 precomposed hangul syllables
    const HangulStream::KSXFilter HangulStream::csDefaultKSXFilter = {
        2350,
        {L'가', L'각', L'간', L'갇', L'갈', L'갉', L'갊', L'감', L'갑', L'값', L'갓', L'갔', L'강', L'갖', L'갗', L'같', L'갚', L'갛', L'개', L'객',
         L'갠', L'갤', L'갬', L'갭', L'갯', L'갰', L'갱', L'갸', L'갹', L'갼', L'걀', L'걋', L'걍', L'걔', L'걘', L'걜', L'거', L'걱', L'건', L'걷',
         L'걸', L'걺', L'검', L'겁', L'것', L'겄', L'겅', L'겆', L'겉', L'겊', L'겋', L'게', L'겐', L'겔', L'겜', L'겝', L'겟', L'겠', L'겡', L'겨',
         L'격', L'겪', L'견', L'겯', L'결', L'겸', L'겹', L'겻', L'겼', L'경', L'곁', L'계', L'곈', L'곌', L'곕', L'곗', L'고', L'곡', L'곤', L'곧',
         L'골', L'곪', L'곬', L'곯', L'곰', L'곱', L'곳', L'공', L'곶', L'과', L'곽', L'관', L'괄', L'괆', L'괌', L'괍', L'괏', L'광', L'괘', L'괜',
         L'괠', L'괩', L'괬', L'괭', L'괴', L'괵', L'괸', L'괼', L'굄', L'굅', L'굇', L'굉', L'교', L'굔', L'굘', L'굡', L'굣', L'구', L'국', L'군',
         L'굳', L'굴', L'굵', L'굶', L'굻', L'굼', L'굽', L'굿', L'궁', L'궂', L'궈', L'궉', L'권', L'궐', L'궜', L'궝', L'궤', L'궷', L'귀', L'귁',
         L'귄', L'귈', L'귐', L'귑', L'귓', L'규', L'균', L'귤', L'그', L'극', L'근', L'귿', L'글', L'긁', L'금', L'급', L'긋', L'긍', L'긔', L'기',
         L'긱', L'긴', L'긷', L'길', L'긺', L'김', L'깁', L'깃', L'깅', L'깆', L'깊', L'까', L'깍', L'깎', L'깐', L'깔', L'깖', L'깜', L'깝', L'깟',
         L'깠', L'깡', L'깥', L'깨', L'깩', L'깬', L'깰', L'깸', L'깹', L'깻', L'깼', L'깽', L'꺄', L'꺅', L'꺌', L'꺼', L'꺽', L'꺾', L'껀', L'껄',
         L'껌', L'껍', L'껏', L'껐', L'껑', L'께', L'껙', L'껜', L'껨', L'껫', L'껭', L'껴', L'껸', L'껼', L'꼇', L'꼈', L'꼍', L'꼐', L'꼬', L'꼭',
         L'꼰', L'꼲', L'꼴', L'꼼', L'꼽', L'꼿', L'꽁', L'꽂', L'꽃', L'꽈', L'꽉', L'꽐', L'꽜', L'꽝', L'꽤', L'꽥', L'꽹', L'꾀', L'꾄', L'꾈',
         L'꾐', L'꾑', L'꾕', L'꾜', L'꾸', L'꾹', L'꾼', L'꿀', L'꿇', L'꿈', L'꿉', L'꿋', L'꿍', L'꿎', L'꿔', L'꿜', L'꿨', L'꿩', L'꿰', L'꿱',
         L'꿴', L'꿸', L'뀀', L'뀁', L'뀄', L'뀌', L'뀐', L'뀔', L'뀜', L'뀝', L'뀨', L'끄', L'끅', L'끈', L'끊', L'끌', L'끎', L'끓', L'끔', L'끕',
         L'끗', L'끙', L'끝', L'끼', L'끽', L'낀', L'낄', L'낌', L'낍', L'낏', L'낑', L'나', L'낙', L'낚', L'난', L'낟', L'날', L'낡', L'낢', L'남',
         L'납', L'낫', L'났', L'낭', L'낮', L'낯', L'낱', L'낳', L'내', L'낵', L'낸', L'낼', L'냄', L'냅', L'냇', L'냈', L'냉', L'냐', L'냑', L'냔',
         L'냘', L'냠', L'냥', L'너', L'넉', L'넋', L'넌', L'널', L'넒', L'넓', L'넘', L'넙', L'넛', L'넜', L'넝', L'넣', L'네', L'넥', L'넨', L'넬',
         L'넴', L'넵', L'넷', L'넸', L'넹', L'녀', L'녁', L'년', L'녈', L'념', L'녑', L'녔', L'녕', L'녘', L'녜', L'녠', L'노', L'녹', L'논', L'놀',
         L'놂', L'놈', L'놉', L'놋', L'농', L'높', L'놓', L'놔', L'놘', L'놜', L'놨', L'뇌', L'뇐', L'뇔', L'뇜', L'뇝', L'뇟', L'뇨', L'뇩', L'뇬',
         L'뇰', L'뇹', L'뇻', L'뇽', L'누', L'눅', L'눈', L'눋', L'눌', L'눔', L'눕', L'눗', L'눙', L'눠', L'눴', L'눼', L'뉘', L'뉜', L'뉠', L'뉨',
         L'뉩', L'뉴', L'뉵', L'뉼', L'늄', L'늅', L'늉', L'느', L'늑', L'는', L'늘', L'늙', L'늚', L'늠', L'늡', L'늣', L'능', L'늦', L'늪', L'늬',
         L'늰', L'늴', L'니', L'닉', L'닌', L'닐', L'닒', L'님', L'닙', L'닛', L'닝', L'닢', L'다', L'닥', L'닦', L'단', L'닫', L'달', L'닭', L'닮',
         L'닯', L'닳', L'담', L'답', L'닷', L'닸', L'당', L'닺', L'닻', L'닿', L'대', L'댁', L'댄', L'댈', L'댐', L'댑', L'댓', L'댔', L'댕', L'댜',
         L'더', L'덕', L'덖', L'던', L'덛', L'덜', L'덞', L'덟', L'덤', L'덥', L'덧', L'덩', L'덫', L'덮', L'데', L'덱', L'덴', L'델', L'뎀', L'뎁',
         L'뎃', L'뎄', L'뎅', L'뎌', L'뎐', L'뎔', L'뎠', L'뎡', L'뎨', L'뎬', L'도', L'독', L'돈', L'돋', L'돌', L'돎', L'돐', L'돔', L'돕', L'돗',
         L'동', L'돛', L'돝', L'돠', L'돤', L'돨', L'돼', L'됐', L'되', L'된', L'될', L'됨', L'됩', L'됫', L'됴', L'두', L'둑', L'둔', L'둘', L'둠',
         L'둡', L'둣', L'둥', L'둬', L'뒀', L'뒈', L'뒝', L'뒤', L'뒨', L'뒬', L'뒵', L'뒷', L'뒹', L'듀', L'듄', L'듈', L'듐', L'듕', L'드', L'득',
         L'든', L'듣', L'들', L'듦', L'듬', L'듭', L'듯', L'등', L'듸', L'디', L'딕', L'딘', L'딛', L'딜', L'딤', L'딥', L'딧', L'딨', L'딩', L'딪',
         L'따', L'딱', L'딴', L'딸', L'땀', L'땁', L'땃', L'땄', L'땅', L'땋', L'때', L'땍', L'땐', L'땔', L'땜', L'땝', L'땟', L'땠', L'땡', L'떠',
         L'떡', L'떤', L'떨', L'떪', L'떫', L'떰', L'떱', L'떳', L'떴', L'떵', L'떻', L'떼', L'떽', L'뗀', L'뗄', L'뗌', L'뗍', L'뗏', L'뗐', L'뗑',
         L'뗘', L'뗬', L'또', L'똑', L'똔', L'똘', L'똥', L'똬', L'똴', L'뙈', L'뙤', L'뙨', L'뚜', L'뚝', L'뚠', L'뚤', L'뚫', L'뚬', L'뚱', L'뛔',
         L'뛰', L'뛴', L'뛸', L'뜀', L'뜁', L'뜅', L'뜨', L'뜩', L'뜬', L'뜯', L'뜰', L'뜸', L'뜹', L'뜻', L'띄', L'띈', L'띌', L'띔', L'띕', L'띠',
         L'띤', L'띨', L'띰', L'띱', L'띳', L'띵', L'라', L'락', L'란', L'랄', L'람', L'랍', L'랏', L'랐', L'랑', L'랒', L'랖', L'랗', L'래', L'랙',
         L'랜', L'랠', L'램', L'랩', L'랫', L'랬', L'랭', L'랴', L'략', L'랸', L'럇', L'량', L'러', L'럭', L'런', L'럴', L'럼', L'럽', L'럿', L'렀',
         L'렁', L'렇', L'레', L'렉', L'렌', L'렐', L'렘', L'렙', L'렛', L'렝', L'려', L'력', L'련', L'렬', L'렴', L'렵', L'렷', L'렸', L'령', L'례',
         L'롄', L'롑', L'롓', L'로', L'록', L'론', L'롤', L'롬', L'롭', L'롯', L'롱', L'롸', L'롼', L'뢍', L'뢨', L'뢰', L'뢴', L'뢸', L'룀', L'룁',
         L'룃', L'룅', L'료', L'룐', L'룔', L'룝', L'룟', L'룡', L'루', L'룩', L'룬', L'룰', L'룸', L'룹', L'룻', L'룽', L'뤄', L'뤘', L'뤠', L'뤼',
         L'뤽', L'륀', L'륄', L'륌', L'륏', L'륑', L'류', L'륙', L'륜', L'률', L'륨', L'륩', L'륫', L'륭', L'르', L'륵', L'른', L'를', L'름', L'릅',
         L'릇', L'릉', L'릊', L'릍', L'릎', L'리', L'릭', L'린', L'릴', L'림', L'립', L'릿', L'링', L'마', L'막', L'만', L'많', L'맏', L'말', L'맑',
         L'맒', L'맘', L'맙', L'맛', L'망', L'맞', L'맡', L'맣', L'매', L'맥', L'맨', L'맬', L'맴', L'맵', L'맷', L'맸', L'맹', L'맺', L'먀', L'먁',
         L'먈', L'먕', L'머', L'먹', L'먼', L'멀', L'멂', L'멈', L'멉', L'멋', L'멍', L'멎', L'멓', L'메', L'멕', L'멘', L'멜', L'멤', L'멥', L'멧',
         L'멨', L'멩', L'며', L'멱', L'면', L'멸', L'몃', L'몄', L'명', L'몇', L'몌', L'모', L'목', L'몫', L'몬', L'몰', L'몲', L'몸', L'몹', L'못',
         L'몽', L'뫄', L'뫈', L'뫘', L'뫙', L'뫼', L'묀', L'묄', L'묍', L'묏', L'묑', L'묘', L'묜', L'묠', L'묩', L'묫', L'무', L'묵', L'묶', L'문',
         L'묻', L'물', L'묽', L'묾', L'뭄', L'뭅', L'뭇', L'뭉', L'뭍', L'뭏', L'뭐', L'뭔', L'뭘', L'뭡', L'뭣', L'뭬', L'뮈', L'뮌', L'뮐', L'뮤',
         L'뮨', L'뮬', L'뮴', L'뮷', L'므', L'믄', L'믈', L'믐', L'믓', L'미', L'믹', L'민', L'믿', L'밀', L'밂', L'밈', L'밉', L'밋', L'밌', L'밍',
         L'및', L'밑', L'바', L'박', L'밖', L'밗', L'반', L'받', L'발', L'밝', L'밞', L'밟', L'밤', L'밥', L'밧', L'방', L'밭', L'배', L'백', L'밴',
         L'밸', L'뱀', L'뱁', L'뱃', L'뱄', L'뱅', L'뱉', L'뱌', L'뱍', L'뱐', L'뱝', L'버', L'벅', L'번', L'벋', L'벌', L'벎', L'범', L'법', L'벗',
         L'벙', L'벚', L'베', L'벡', L'벤', L'벧', L'벨', L'벰', L'벱', L'벳', L'벴', L'벵', L'벼', L'벽', L'변', L'별', L'볍', L'볏', L'볐', L'병',
         L'볕', L'볘', L'볜', L'보', L'복', L'볶', L'본', L'볼', L'봄', L'봅', L'봇', L'봉', L'봐', L'봔', L'봤', L'봬', L'뵀', L'뵈', L'뵉', L'뵌',
         L'뵐', L'뵘', L'뵙', L'뵤', L'뵨', L'부', L'북', L'분', L'붇', L'불', L'붉', L'붊', L'붐', L'붑', L'붓', L'붕', L'붙', L'붚', L'붜', L'붤',
         L'붰', L'붸', L'뷔', L'뷕', L'뷘', L'뷜', L'뷩', L'뷰', L'뷴', L'뷸', L'븀', L'븃', L'븅', L'브', L'븍', L'븐', L'블', L'븜', L'븝', L'븟',
         L'비', L'빅', L'빈', L'빌', L'빎', L'빔', L'빕', L'빗', L'빙', L'빚', L'빛', L'빠', L'빡', L'빤', L'빨', L'빪', L'빰', L'빱', L'빳', L'빴',
         L'빵', L'빻', L'빼', L'빽', L'뺀', L'뺄', L'뺌', L'뺍', L'뺏', L'뺐', L'뺑', L'뺘', L'뺙', L'뺨', L'뻐', L'뻑', L'뻔', L'뻗', L'뻘', L'뻠',
         L'뻣', L'뻤', L'뻥', L'뻬', L'뼁', L'뼈', L'뼉', L'뼘', L'뼙', L'뼛', L'뼜', L'뼝', L'뽀', L'뽁', L'뽄', L'뽈', L'뽐', L'뽑', L'뽕', L'뾔',
         L'뾰', L'뿅', L'뿌', L'뿍', L'뿐', L'뿔', L'뿜', L'뿟', L'뿡', L'쀼', L'쁑', L'쁘', L'쁜', L'쁠', L'쁨', L'쁩', L'삐', L'삑', L'삔', L'삘',
         L'삠', L'삡', L'삣', L'삥', L'사', L'삭', L'삯', L'산', L'삳', L'살', L'삵', L'삶', L'삼', L'삽', L'삿', L'샀', L'상', L'샅', L'새', L'색',
         L'샌', L'샐', L'샘', L'샙', L'샛', L'샜', L'생', L'샤', L'샥', L'샨', L'샬', L'샴', L'샵', L'샷', L'샹', L'섀', L'섄', L'섈', L'섐', L'섕',
         L'서', L'석', L'섞', L'섟', L'선', L'섣', L'설', L'섦', L'섧', L'섬', L'섭', L'섯', L'섰', L'성', L'섶', L'세', L'섹', L'센', L'셀', L'셈',
         L'셉', L'셋', L'셌', L'셍', L'셔', L'셕', L'션', L'셜', L'셤', L'셥', L'셧', L'셨', L'셩', L'셰', L'셴', L'셸', L'솅', L'소', L'속', L'솎',
         L'손', L'솔', L'솖', L'솜', L'솝', L'솟', L'송', L'솥', L'솨', L'솩', L'솬', L'솰', L'솽', L'쇄', L'쇈', L'쇌', L'쇔', L'쇗', L'쇘', L'쇠',
         L'쇤', L'쇨', L'쇰', L'쇱', L'쇳', L'쇼', L'쇽', L'숀', L'숄', L'숌', L'숍', L'숏', L'숑', L'수', L'숙', L'순', L'숟', L'술', L'숨', L'숩',
         L'숫', L'숭', L'숯', L'숱', L'숲', L'숴', L'쉈', L'쉐', L'쉑', L'쉔', L'쉘', L'쉠', L'쉥', L'쉬', L'쉭', L'쉰', L'쉴', L'쉼', L'쉽', L'쉿',
         L'슁', L'슈', L'슉', L'슐', L'슘', L'슛', L'슝', L'스', L'슥', L'슨', L'슬', L'슭', L'슴', L'습', L'슷', L'승', L'시', L'식', L'신', L'싣',
         L'실', L'싫', L'심', L'십', L'싯', L'싱', L'싶', L'싸', L'싹', L'싻', L'싼', L'쌀', L'쌈', L'쌉', L'쌌', L'쌍', L'쌓', L'쌔', L'쌕', L'쌘',
         L'쌜', L'쌤', L'쌥', L'쌨', L'쌩', L'썅', L'써', L'썩', L'썬', L'썰', L'썲', L'썸', L'썹', L'썼', L'썽', L'쎄', L'쎈', L'쎌', L'쏀', L'쏘',
         L'쏙', L'쏜', L'쏟', L'쏠', L'쏢', L'쏨', L'쏩', L'쏭', L'쏴', L'쏵', L'쏸', L'쐈', L'쐐', L'쐤', L'쐬', L'쐰', L'쐴', L'쐼', L'쐽', L'쑈',
         L'쑤', L'쑥', L'쑨', L'쑬', L'쑴', L'쑵', L'쑹', L'쒀', L'쒔', L'쒜', L'쒸', L'쒼', L'쓩', L'쓰', L'쓱', L'쓴', L'쓸', L'쓺', L'쓿', L'씀',
         L'씁', L'씌', L'씐', L'씔', L'씜', L'씨', L'씩', L'씬', L'씰', L'씸', L'씹', L'씻', L'씽', L'아', L'악', L'안', L'앉', L'않', L'알', L'앍',
         L'앎', L'앓', L'암', L'압', L'앗', L'았', L'앙', L'앝', L'앞', L'애', L'액', L'앤', L'앨', L'앰', L'앱', L'앳', L'앴', L'앵', L'야', L'약',
         L'얀', L'얄', L'얇', L'얌', L'얍', L'얏', L'양', L'얕', L'얗', L'얘', L'얜', L'얠', L'얩', L'어', L'억', L'언', L'얹', L'얻', L'얼', L'얽',
         L'얾', L'엄', L'업', L'없', L'엇', L'었', L'엉', L'엊', L'엌', L'엎', L'에', L'엑', L'엔', L'엘', L'엠', L'엡', L'엣', L'엥', L'여', L'역',
         L'엮', L'연', L'열', L'엶', L'엷', L'염', L'엽', L'엾', L'엿', L'였', L'영', L'옅', L'옆', L'옇', L'예', L'옌', L'옐', L'옘', L'옙', L'옛',
         L'옜', L'오', L'옥', L'온', L'올', L'옭', L'옮', L'옰', L'옳', L'옴', L'옵', L'옷', L'옹', L'옻', L'와', L'왁', L'완', L'왈', L'왐', L'왑',
         L'왓', L'왔', L'왕', L'왜', L'왝', L'왠', L'왬', L'왯', L'왱', L'외', L'왹', L'왼', L'욀', L'욈', L'욉', L'욋', L'욍', L'요', L'욕', L'욘',
         L'욜', L'욤', L'욥', L'욧', L'용', L'우', L'욱', L'운', L'울', L'욹', L'욺', L'움', L'웁', L'웃', L'웅', L'워', L'웍', L'원', L'월', L'웜',
         L'웝', L'웠', L'웡', L'웨', L'웩', L'웬', L'웰', L'웸', L'웹', L'웽', L'위', L'윅', L'윈', L'윌', L'윔', L'윕', L'윗', L'윙', L'유', L'육',
         L'윤', L'율', L'윰', L'윱', L'윳', L'융', L'윷', L'으', L'윽', L'은', L'을', L'읊', L'음', L'읍', L'읏', L'응', L'읒', L'읓', L'읔', L'읕',
         L'읖', L'읗', L'의', L'읜', L'읠', L'읨', L'읫', L'이', L'익', L'인', L'일', L'읽', L'읾', L'잃', L'임', L'입', L'잇', L'있', L'잉', L'잊',
         L'잎', L'자', L'작', L'잔', L'잖', L'잗', L'잘', L'잚', L'잠', L'잡', L'잣', L'잤', L'장', L'잦', L'재', L'잭', L'잰', L'잴', L'잼', L'잽',
         L'잿', L'쟀', L'쟁', L'쟈', L'쟉', L'쟌', L'쟎', L'쟐', L'쟘', L'쟝', L'쟤', L'쟨', L'쟬', L'저', L'적', L'전', L'절', L'젊', L'점', L'접',
         L'젓', L'정', L'젖', L'제', L'젝', L'젠', L'젤', L'젬', L'젭', L'젯', L'젱', L'져', L'젼', L'졀', L'졈', L'졉', L'졌', L'졍', L'졔', L'조',
         L'족', L'존', L'졸', L'졺', L'좀', L'좁', L'좃', L'종', L'좆', L'좇', L'좋', L'좌', L'좍', L'좔', L'좝', L'좟', L'좡', L'좨', L'좼', L'좽',
         L'죄', L'죈', L'죌', L'죔', L'죕', L'죗', L'죙', L'죠', L'죡', L'죤', L'죵', L'주', L'죽', L'준', L'줄', L'줅', L'줆', L'줌', L'줍', L'줏',
         L'중', L'줘', L'줬', L'줴', L'쥐', L'쥑', L'쥔', L'쥘', L'쥠', L'쥡', L'쥣', L'쥬', L'쥰', L'쥴', L'쥼', L'즈', L'즉', L'즌', L'즐', L'즘',
         L'즙', L'즛', L'증', L'지', L'직', L'진', L'짇', L'질', L'짊', L'짐', L'집', L'짓', L'징', L'짖', L'짙', L'짚', L'짜', L'짝', L'짠', L'짢',
         L'짤', L'짧', L'짬', L'짭', L'짯', L'짰', L'짱', L'째', L'짹', L'짼', L'쨀', L'쨈', L'쨉', L'쨋', L'쨌', L'쨍', L'쨔', L'쨘', L'쨩', L'쩌',
         L'쩍', L'쩐', L'쩔', L'쩜', L'쩝', L'쩟', L'쩠', L'쩡', L'쩨', L'쩽', L'쪄', L'쪘', L'쪼', L'쪽', L'쫀', L'쫄', L'쫌', L'쫍', L'쫏', L'쫑',
         L'쫓', L'쫘', L'쫙', L'쫠', L'쫬', L'쫴', L'쬈', L'쬐', L'쬔', L'쬘', L'쬠', L'쬡', L'쭁', L'쭈', L'쭉', L'쭌', L'쭐', L'쭘', L'쭙', L'쭝',
         L'쭤', L'쭸', L'쭹', L'쮜', L'쮸', L'쯔', L'쯤', L'쯧', L'쯩', L'찌', L'찍', L'찐', L'찔', L'찜', L'찝', L'찡', L'찢', L'찧', L'차', L'착',
         L'찬', L'찮', L'찰', L'참', L'찹', L'찻', L'찼', L'창', L'찾', L'채', L'책', L'챈', L'챌', L'챔', L'챕', L'챗', L'챘', L'챙', L'챠', L'챤',
         L'챦', L'챨', L'챰', L'챵', L'처', L'척', L'천', L'철', L'첨', L'첩', L'첫', L'첬', L'청', L'체', L'첵', L'첸', L'첼', L'쳄', L'쳅', L'쳇',
         L'쳉', L'쳐', L'쳔', L'쳤', L'쳬', L'쳰', L'촁', L'초', L'촉', L'촌', L'촐', L'촘', L'촙', L'촛', L'총', L'촤', L'촨', L'촬', L'촹', L'최',
         L'쵠', L'쵤', L'쵬', L'쵭', L'쵯', L'쵱', L'쵸', L'춈', L'추', L'축', L'춘', L'출', L'춤', L'춥', L'춧', L'충', L'춰', L'췄', L'췌', L'췐',
         L'취', L'췬', L'췰', L'췸', L'췹', L'췻', L'췽', L'츄', L'츈', L'츌', L'츔', L'츙', L'츠', L'측', L'츤', L'츨', L'츰', L'츱', L'츳', L'층',
         L'치', L'칙', L'친', L'칟', L'칠', L'칡', L'침', L'칩', L'칫', L'칭', L'카', L'칵', L'칸', L'칼', L'캄', L'캅', L'캇', L'캉', L'캐', L'캑',
         L'캔', L'캘', L'캠', L'캡', L'캣', L'캤', L'캥', L'캬', L'캭', L'컁', L'커', L'컥', L'컨', L'컫', L'컬', L'컴', L'컵', L'컷', L'컸', L'컹',
         L'케', L'켁', L'켄', L'켈', L'켐', L'켑', L'켓', L'켕', L'켜', L'켠', L'켤', L'켬', L'켭', L'켯', L'켰', L'켱', L'켸', L'코', L'콕', L'콘',
         L'콜', L'콤', L'콥', L'콧', L'콩', L'콰', L'콱', L'콴', L'콸', L'쾀', L'쾅', L'쾌', L'쾡', L'쾨', L'쾰', L'쿄', L'쿠', L'쿡', L'쿤', L'쿨',
         L'쿰', L'쿱', L'쿳', L'쿵', L'쿼', L'퀀', L'퀄', L'퀑', L'퀘', L'퀭', L'퀴', L'퀵', L'퀸', L'퀼', L'큄', L'큅', L'큇', L'큉', L'큐', L'큔',
         L'큘', L'큠', L'크', L'큭', L'큰', L'클', L'큼', L'큽', L'킁', L'키', L'킥', L'킨', L'킬', L'킴', L'킵', L'킷', L'킹', L'타', L'탁', L'탄',
         L'탈', L'탉', L'탐', L'탑', L'탓', L'탔', L'탕', L'태', L'택', L'탠', L'탤', L'탬', L'탭', L'탯', L'탰', L'탱', L'탸', L'턍', L'터', L'턱',
         L'턴', L'털', L'턺', L'텀', L'텁', L'텃', L'텄', L'텅', L'테', L'텍', L'텐', L'텔', L'템', L'텝', L'텟', L'텡', L'텨', L'텬', L'텼', L'톄',
         L'톈', L'토', L'톡', L'톤', L'톨', L'톰', L'톱', L'톳', L'통', L'톺', L'톼', L'퇀', L'퇘', L'퇴', L'퇸', L'툇', L'툉', L'툐', L'투', L'툭',
         L'툰', L'툴', L'툼', L'툽', L'툿', L'퉁', L'퉈', L'퉜', L'퉤', L'튀', L'튁', L'튄', L'튈', L'튐', L'튑', L'튕', L'튜', L'튠', L'튤', L'튬',
         L'튱', L'트', L'특', L'튼', L'튿', L'틀', L'틂', L'틈', L'틉', L'틋', L'틔', L'틘', L'틜', L'틤', L'틥', L'티', L'틱', L'틴', L'틸', L'팀',
         L'팁', L'팃', L'팅', L'파', L'팍', L'팎', L'판', L'팔', L'팖', L'팜', L'팝', L'팟', L'팠', L'팡', L'팥', L'패', L'팩', L'팬', L'팰', L'팸',
         L'팹', L'팻', L'팼', L'팽', L'퍄', L'퍅', L'퍼', L'퍽', L'펀', L'펄', L'펌', L'펍', L'펏', L'펐', L'펑', L'페', L'펙', L'펜', L'펠', L'펨',
         L'펩', L'펫', L'펭', L'펴', L'편', L'펼', L'폄', L'폅', L'폈', L'평', L'폐', L'폘', L'폡', L'폣', L'포', L'폭', L'폰', L'폴', L'폼', L'폽',
         L'폿', L'퐁', L'퐈', L'퐝', L'푀', L'푄', L'표', L'푠', L'푤', L'푭', L'푯', L'푸', L'푹', L'푼', L'푿', L'풀', L'풂', L'품', L'풉', L'풋',
         L'풍', L'풔', L'풩', L'퓌', L'퓐', L'퓔', L'퓜', L'퓟', L'퓨', L'퓬', L'퓰', L'퓸', L'퓻', L'퓽', L'프', L'픈', L'플', L'픔', L'픕', L'픗',
         L'피', L'픽', L'핀', L'필', L'핌', L'핍', L'핏', L'핑', L'하', L'학', L'한', L'할', L'핥', L'함', L'합', L'핫', L'항', L'해', L'핵', L'핸',
         L'핼', L'햄', L'햅', L'햇', L'했', L'행', L'햐', L'향', L'허', L'헉', L'헌', L'헐', L'헒', L'험', L'헙', L'헛', L'헝', L'헤', L'헥', L'헨',
         L'헬', L'헴', L'헵', L'헷', L'헹', L'혀', L'혁', L'현', L'혈', L'혐', L'협', L'혓', L'혔', L'형', L'혜', L'혠', L'혤', L'혭', L'호', L'혹',
         L'혼', L'홀', L'홅', L'홈', L'홉', L'홋', L'홍', L'홑', L'화', L'확', L'환', L'활', L'홧', L'황', L'홰', L'홱', L'홴', L'횃', L'횅', L'회',
         L'획', L'횐', L'횔', L'횝', L'횟', L'횡', L'효', L'횬', L'횰', L'횹', L'횻', L'후', L'훅', L'훈', L'훌', L'훑', L'훔', L'훗', L'훙', L'훠',
         L'훤', L'훨', L'훰', L'훵', L'훼', L'훽', L'휀', L'휄', L'휑', L'휘', L'휙', L'휜', L'휠', L'휨', L'휩', L'휫', L'휭', L'휴', L'휵', L'휸',
         L'휼', L'흄', L'흇', L'흉', L'흐', L'흑', L'흔', L'흖', L'흗', L'흘', L'흙', L'흠', L'흡', L'흣', L'흥', L'흩', L'희', L'흰', L'흴', L'흼',
         L'흽', L'힁', L'히', L'힉', L'힌', L'힐', L'힘', L'힙', L'힛', L'힝'},
    };

    const HangulStream::JamoBuilderMap HangulStream::csJamoMapBeginning[] = {
        {L"ㄱ", L'ᄀ'}, {L"ㄲ", L'ᄁ'}, {L"ㄴ", L'ᄂ'}, {L"ㄷ", L'ᄃ'}, {L"ㄸ", L'ᄄ'}, {L"ㄹ", L'ᄅ'}, {L"ㅁ", L'ᄆ'},
        {L"ㅂ", L'ᄇ'}, {L"ㅃ", L'ᄈ'}, {L"ㅅ", L'ᄉ'}, {L"ㅆ", L'ᄊ'}, {L"ㅇ", L'ᄋ'}, {L"ㅈ", L'ᄌ'}, {L"ㅉ", L'ᄍ'},
        {L"ㅊ", L'ᄎ'}, {L"ㅋ", L'ᄏ'}, {L"ㅌ", L'ᄐ'}, {L"ㅍ", L'ᄑ'}, {L"ㅎ", L'ᄒ'},
    };
    const HangulStream::JamoBuilderMap HangulStream::csJamoMapMiddle[] = {
        {L"ㅏ", L'ᅡ'},   {L"ㅐ", L'ᅢ'},   {L"ㅑ", L'ᅣ'},   {L"ㅒ", L'ᅤ'},   {L"ㅓ", L'ᅥ'},   {L"ㅔ", L'ᅦ'},   {L"ㅕ", L'ᅧ'},
        {L"ㅖ", L'ᅨ'},   {L"ㅗ", L'ᅩ'},   {L"ㅗㅏ", L'ᅪ'}, {L"ㅗㅐ", L'ᅫ'}, {L"ㅗㅣ", L'ᅬ'}, {L"ㅛ", L'ᅭ'},   {L"ㅜ", L'ᅮ'},
        {L"ㅜㅓ", L'ᅯ'}, {L"ㅜㅔ", L'ᅰ'}, {L"ㅜㅣ", L'ᅱ'}, {L"ㅠ", L'ᅲ'},   {L"ㅡ", L'ᅳ'},   {L"ㅡㅣ", L'ᅴ'}, {L"ㅣ", L'ᅵ'},
    };
    const HangulStream::JamoBuilderMap HangulStream::csJamoMapEnding[] = {
        {L"ㄱ", L'ᆨ'},   {L"ㄲ", L'ᆩ'},   {L"ㄱㅅ", L'ᆪ'}, {L"ㄴ", L'ᆫ'},   {L"ㄴㅈ", L'ᆬ'}, {L"ㄴㅎ", L'ᆭ'}, {L"ㄷ", L'ᆮ'},
        {L"ㄹ", L'ᆯ'},   {L"ㄹㄱ", L'ᆰ'}, {L"ㄹㅁ", L'ᆱ'}, {L"ㄹㅂ", L'ᆲ'}, {L"ㄹㅅ", L'ᆳ'}, {L"ㄹㅌ", L'ᆴ'}, {L"ㄹㅍ", L'ᆵ'},
        {L"ㄹㅎ", L'ᆶ'}, {L"ㅁ", L'ᆷ'},   {L"ㅂ", L'ᆸ'},   {L"ㅂㅅ", L'ᆹ'}, {L"ㅅ", L'ᆺ'},   {L"ㅆ", L'ᆻ'},   {L"ㅇ", L'ᆼ'},
        {L"ㅈ", L'ᆽ'},   {L"ㅊ", L'ᆾ'},   {L"ㅋ", L'ᆿ'},   {L"ㅌ", L'ᇀ'},   {L"ㅍ", L'ᇁ'},   {L"ㅎ", L'ᇂ'},
    };

    static bool isMiddleJamo(wchar_t jamo) {
        if (L'ㅏ' <= jamo && jamo <= L'ㅣ')
            return true;
        else
            return false;
    }

    static wchar_t reduceKSXMiddle_(wchar_t middle) {
        switch (middle) {
            case L'ᅪ':
            case L'ᅫ':
            case L'ᅬ':
                middle = L'ᅩ';
                break;
            case L'ᅯ':
            case L'ᅰ':
            case L'ᅱ':
                middle = L'ᅮ';
                break;
            case L'ᅴ':
                middle = L'ᅳ';
                break;
            default:
                middle = L'\0';
                break;
        }
        return middle;
    }
    static wchar_t reduceKSXEnding_(wchar_t ending) {
        switch (ending) {
            case L'ᆪ':
                ending = L'ᆨ';
                break;
            case L'ᆬ':
            case L'ᆭ':
                ending = L'ᆫ';
                break;
            case L'ᆰ':
            case L'ᆱ':
            case L'ᆲ':
            case L'ᆳ':
            case L'ᆴ':
            case L'ᆵ':
            case L'ᆶ':
                ending = L'ᆯ';
                break;
            case L'ᆹ':
                ending = L'ᆸ';
                break;
            default:
                ending = L'\0';
                break;
        }
        return ending;
    }

    static int getMatchingRunLen(const wchar_t* a, const wchar_t* b) {
        int j = 0;
        while (true) {
            if (*a != *b || *a == L'\0')
                break;
            a++;
            b++;
            j++;
        }
        return j;
    }

    bool HangulStream::KSXFilter::Allowed(wchar_t ch) const {
        u32 min = 0;
        u32 max = mCount - 1;
        const wchar_t* entries = mEntries;
        while (min <= max) {
            u32 midpoint = (min + max) / 2;
            if (ch == entries[midpoint]) {
                return true;
            } else if (entries[midpoint] < ch) {
                min = midpoint + 1;
            } else {
                max = midpoint - 1;
            }
        }
        return false;
    }
    bool HangulStream::IsValidKSX_(wchar_t ch) const {
        if (!mbKSXFilterEnabled) {
            return true;
        }
        if (ch < L'가')
            return false;
        if (ch > L'힝')
            return false;

        return mpKSXFilter->Allowed(ch);
    }
    wchar_t HangulStream::GetBeginning_(wchar_t ch) {
        for (int i = 0; i < ARRAY_LENGTH(csJamoMapBeginning); i++) {
            if (csJamoMapBeginning[i].to == ch) {
                return csJamoMapBeginning[i].from[0];
            }
        }
        return L'\0';
    }
    wchar_t HangulStream::ToSyllable_(wchar_t beginning, wchar_t middle, wchar_t ending) {
        if (middle == L'\0') {
            return GetBeginning_(beginning);
        } else {
            if (ending == L'\0') {
                ending = L'ᆧ';
            }
            return L'가' + ((beginning - L'ᄀ') * JAMO_COUNT_MIDDLE + middle - L'ᅡ') * JAMO_COUNT_ENDING + ending - L'ᆧ';
        }
    }

    void HangulStream::SkipChars_(u32 length) {
        wchar_t* dst = mChars;
        const wchar_t* src = mChars + length;
        for (int i = 0; i < mLength - length; i++) {
            *dst = *src;
            dst++;
            src++;
        }
        mLength = mLength - length;
        mChars[mLength] = L'\0';
    }

    void HangulStream::Put(wchar_t ch) {
        mChars[mLength] = ch;
        mChars[++mLength] = L'\0';
    }

    wchar_t HangulStream::Get() {
        wchar_t beginningJamo, middleJamo, endingJamo;
        u32 beginningMatchLen, middleMatchLen, endingMatchLen;

        u32 offset = 0;
        beginningJamo = SearchBeginning_(&mChars[offset], &beginningMatchLen);
        offset = offset + beginningMatchLen;
        middleJamo = SearchMiddle_(&mChars[offset], &middleMatchLen);
        offset = offset + middleMatchLen;
        if (middleJamo != L'\0') {
            endingJamo = SearchEnding_(&mChars[offset], &endingMatchLen);
            offset = offset + endingMatchLen;
        } else {
            endingJamo = SearchEnding_(mChars, &endingMatchLen);
            if (endingMatchLen != 0) {
                offset = endingMatchLen;
            }
        }

        if (endingJamo != L'\0') {
            if (isMiddleJamo(mChars[offset])) {
                // Rebrackets syllables by cleaving off the last character of
                // the end
                //
                // Example 1: 자산 (jasan) is typed like ㅈ ㅏ ㅅ ㅏ ㄴ (WKTKS on Doebelsik)
                // 잣 is a valid hangul syllable, made of valid jamo, so by
                // default you're going to end up with:
                // - Beginning - ㅈ
                // - Middle    - ㅏ
                // - End       - ㅅ
                // - Offset    - 3
                // - After     - ㅏ ㄴ (NOT a valid syllable)
                // This checks if there would've been a valid syllable if one
                // fewer jamo had been consumed (checks ㅅ ㅏ ㄴ) for validity
                // If there is a valid next syllable, it reduces the length of the
                // current syllable by one by cleaving off the last character making
                // up the ending
                // We end up with 자 (ㅈ ㅏ) (ja) as our current syllable, and ㅅ ㅏ ㄴ
                // left over, End set to NULL, and the offset being 2
                //
                // Example 2: 갑사 (gabsa) typed like ㄱ ㅏ ㅂ ㅅ ㅏ (RKQTK)
                // Greedily, you get 값 (gabs):
                // - Beginning - ㄱ
                // - Middle    - ㅏ
                // - End       - ᆹ
                // - Offset    - 4
                // - After     - ㅏ (NOT a valid syllable)
                // Because ㅅ ㅏ is a valid syllable 사 (sa), end is set to
                // ㅂ, offset is now 3, and ㅅ ㅏ is left over
                if (IsValidKSX_(GetUnfixed(offset - 1))) {
                    offset--;
                    endingJamo = reduceKSXEnding_(endingJamo);
                }
            }
        }

        wchar_t chOut;
        if (beginningJamo != L'\0' && middleJamo != L'\0') {
            chOut = ToSyllable_(beginningJamo, middleJamo, endingJamo);
            if (!IsValidKSX_(chOut)) {
                if (endingJamo != L'\0') {
                    offset--;
                    endingJamo = reduceKSXEnding_(endingJamo);
                } else if (middleJamo != L'\0') {
                    offset--;
                    middleJamo = reduceKSXMiddle_(middleJamo);
                }
                chOut = ToSyllable_(beginningJamo, middleJamo, endingJamo);
            }
        } else {
            if (middleJamo != L'\0') {
                chOut = HangulJamoToCJKJamo_(middleJamo);
                if (endingJamo != L'\0') {
                    SkipChars_(mLength > middleMatchLen ? middleMatchLen : mLength);
                    return chOut;
                }
            } else if (endingJamo != L'\0') {
                chOut = HangulJamoToCJKJamo_(endingJamo);
            } else {
                chOut = HangulJamoToCJKJamo_(beginningJamo);
            }
        }

        if (mChars[offset] == 0xFFFF) {
            offset++;
        } else if (offset == mLength) {
            return L'\0';
        }
        SkipChars_(mLength > offset ? offset : mLength);
        return chOut;
    }

    void HangulStream::Flush() {
        if (mLength != 0) {
            mChars[mLength] = 0xFFFF;
            mChars[++mLength] = L'\0';
        }
    }

    void HangulStream::Clear() {
        mLength = 0;
        for (int i = 0; i < ARRAY_LENGTH(mChars); i++) {
            mChars[i] = L'\0';
        }
    }

    void HangulStream::BackSpace() {
        if (mLength != 0) {
            mChars[--mLength] = L'\0';
        }
    }

    wchar_t HangulStream::GetUnfixed(u32 idx) const {
        wchar_t beginningJamo, middleJamo, endingJamo;
        u32 matchLen;

        u32 offset = idx;
        beginningJamo = SearchBeginning_(&mChars[offset], &matchLen);
        offset = offset + matchLen;
        middleJamo = SearchMiddle_(&mChars[offset], &matchLen);
        offset = offset + matchLen;
        if (middleJamo != L'\0') {
            endingJamo = SearchEnding_(&mChars[offset], &matchLen);
            offset = offset + matchLen;
        } else {
            endingJamo = SearchEnding_(mChars, &matchLen);
            if (matchLen != 0) {
                offset = matchLen;
            }
        }

        if (endingJamo != L'\0') {
            if (isMiddleJamo(mChars[offset])) {
                // See note in HangulStream::Get for more information on why
                // this is doing this
                if (IsValidKSX_(GetUnfixed(offset - 1))) {
                    endingJamo = reduceKSXEnding_(endingJamo);
                }
            }
        }
        wchar_t chOut;
        if (beginningJamo != L'\0' && middleJamo != L'\0') {
            chOut = ToSyllable_(beginningJamo, middleJamo, endingJamo);
            if (!IsValidKSX_(chOut)) {
                if (endingJamo != L'\0') {
                    endingJamo = reduceKSXEnding_(endingJamo);
                }
                chOut = ToSyllable_(beginningJamo, middleJamo, endingJamo);
            }
        } else {
            if (middleJamo != L'\0')
                chOut = HangulJamoToCJKJamo_(middleJamo);
            else if (endingJamo != L'\0')
                chOut = HangulJamoToCJKJamo_(endingJamo);
            else
                chOut = HangulJamoToCJKJamo_(beginningJamo);
        }
        return chOut;
    }

    BeginningJamo sBeginningJamo = {
        L'ㄱ', L'ㄲ', L'ㄴ', L'ㄷ', L'ㄸ', L'ㄹ', L'ㅁ', L'ㅂ', L'ㅃ', L'ㅅ', L'ㅆ', L'ㅇ', L'ㅈ', L'ㅉ', L'ㅊ', L'ㅋ', L'ㅌ', L'ㅍ', L'ㅎ',
    };
    MiddleJamo sMiddleJamo = {
        L'ㅏ', L'ㅐ', L'ㅑ', L'ㅒ', L'ㅓ', L'ㅔ', L'ㅕ', L'ㅖ', L'ㅗ', L'ㅘ', L'ㅙ',
        L'ㅚ', L'ㅛ', L'ㅜ', L'ㅝ', L'ㅞ', L'ㅟ', L'ㅠ', L'ㅡ', L'ㅢ', L'ㅣ',
    };
    EndingJamo sEndingJamo = {
        L'\0', L'ㄱ', L'ㄲ', L'ㄳ', L'ㄴ', L'ㄵ', L'ㄶ', L'ㄷ', L'ㄹ', L'ㄺ', L'ㄻ', L'ㄼ', L'ㄽ', L'ㄾ',
        L'ㄿ', L'ㅀ', L'ㅁ', L'ㅂ', L'ㅄ', L'ㅅ', L'ㅆ', L'ㅇ', L'ㅈ', L'ㅊ', L'ㅋ', L'ㅌ', L'ㅍ', L'ㅎ',
    };
    wchar_t HangulStream::HangulJamoToCJKJamo_(wchar_t hangulCh) {
        BeginningJamo beginning = sBeginningJamo;
        MiddleJamo middle = sMiddleJamo;
        EndingJamo end = sEndingJamo;
        if (hangulCh < L'ᄀ') {
            return L'\0';
        } else if (hangulCh < L'ᅡ') {
            return beginning.chars[hangulCh - L'ᄀ'];
        } else if (hangulCh < L'ᆧ') {
            return middle.chars[hangulCh - L'ᅡ'];
        } else {
            return end.chars[hangulCh - L'ᆧ'];
        }
    }

    void HangulStream::SetDefaultKSXFilter() {
        mpKSXFilter = &csDefaultKSXFilter;
    }

    wchar_t HangulStream::SearchBeginning_(const wchar_t* text, u32* lenMatched) {
        u32 maxLenFound = 0;
        int idxFound = -1;
        for (int i = 0; i < ARRAY_LENGTH(csJamoMapBeginning); i++) {
            int lenMatched = getMatchingRunLen(text, csJamoMapBeginning[i].from);
            if (lenMatched > maxLenFound) {
                maxLenFound = lenMatched;
                idxFound = i;
            }
        }
        if (lenMatched != NULL) {
            *lenMatched = maxLenFound;
        }
        if (idxFound >= 0) {
            return csJamoMapBeginning[idxFound].to;
        } else {
            return L'\0';
        }
    }
    wchar_t HangulStream::SearchMiddle_(const wchar_t* text, u32* lenMatched) {
        u32 maxLenFound = 0;
        int idxFound = -1;
        for (int i = 0; i < ARRAY_LENGTH(csJamoMapMiddle); i++) {
            int lenMatched = getMatchingRunLen(text, csJamoMapMiddle[i].from);
            if (lenMatched > maxLenFound) {
                maxLenFound = lenMatched;
                idxFound = i;
            }
        }
        if (lenMatched != NULL) {
            *lenMatched = maxLenFound;
        }
        if (idxFound >= 0) {
            return csJamoMapMiddle[idxFound].to;
        } else {
            return L'\0';
        }
    }
    wchar_t HangulStream::SearchEnding_(const wchar_t* text, u32* lenMatched) {
        u32 maxLenFound = 0;
        int idxFound = -1;
        for (int i = 0; i < ARRAY_LENGTH(csJamoMapEnding); i++) {
            int lenMatched = getMatchingRunLen(text, csJamoMapEnding[i].from);
            if (lenMatched > maxLenFound) {
                maxLenFound = lenMatched;
                idxFound = i;
            }
        }
        if (lenMatched != NULL) {
            *lenMatched = maxLenFound;
        }
        if (idxFound >= 0) {
            return csJamoMapEnding[idxFound].to;
        } else {
            return L'\0';
        }
    }

    // See https://en.wikipedia.org/wiki/Korean_language_and_computers#Modern_text_input
    wchar_t HangulStream::QwertyToHangul(wchar_t qwertyCh) {
        wchar_t dubeolsikLookup[] = {L'ㅁ', L'ㅠ', L'ㅊ', L'ㅇ', L'ㄷ', L'ㄹ', L'ㅎ', L'ㅗ', L'ㅑ', L'ㅓ', L'ㅏ', L'ㅣ', L'ㅡ',
                                     L'ㅜ', L'ㅐ', L'ㅔ', L'ㅂ', L'ㄱ', L'ㄴ', L'ㅅ', L'ㅕ', L'ㅍ', L'ㅈ', L'ㅌ', L'ㅛ', L'ㅋ'};

        if (L'a' <= qwertyCh && qwertyCh <= L'z') {
            return dubeolsikLookup[qwertyCh - L'a'];
        } else if (L'A' <= qwertyCh && qwertyCh <= L'Z') {
            wchar_t baseJamo = dubeolsikLookup[qwertyCh - L'A'];
            switch (qwertyCh) {
                case L'Q':  // ㅂ -> ㅃ
                case L'W':  // ㅈ -> ㅉ
                case L'E':  // ㄷ -> ㄸ
                case L'R':  // ㄱ -> ㄲ
                case L'T':  // ㅅ -> ㅆ
                    return baseJamo + 1;

                case L'O':  // ㅐ -> ㅒ
                case L'P':  // ㅔ -> ㅖ
                    return baseJamo + 2;

                default:
                    return baseJamo;
            }
        } else {
            return qwertyCh;
        }
    }

}  // namespace textinput
