// ============================================================
//  data.h  静态配置数据（难度 / 商品 / 成就 / 装扶 / 升级 / 教程 / 规则手册 / 字库池）
//  只读数据表的声明，定义见 data.cpp
// ============================================================
#pragma once
#include "types.h"

extern DiffCfg CFG[4];
extern ItemDef ITEMS[IK_COUNT];

extern const char* TIER_NAME[4];
Color tierColor(int tier, float t);

extern AchDef ACH[A_COUNT];
extern SkinDef SKINS[SKIN_COUNT];
extern UpgDef UPG[U_COUNT];

// ==================== 界面风格 ====================
extern ThemeStyle THEMES[TK_COUNT];
// 当前生效的风格（随 rec.themeSkin 与豪华界面开关解析）
const ThemeStyle& curTheme();

// 规则手册每页内容与颜色表
extern Color HCOL[9];
void initHCol();

extern const HLine HP1[];
extern const HLine HP2[];
extern const HLine HP3[];
extern const HLine HP4[];
extern const HLine HP5[];
extern const HLine HP6[];
extern const HPage HPAGES[6];

// 用户许可协议（EULA）：明文在加密资源 text.bin（QGX1 格式）中，
// 启动时由 loadSecureText() 解密填充。必须在 loadChineseFont() 之前调用
// （字库扫描依赖协议文本）。修改文字请编辑 tools/eula_master.txt 后重新打包。
extern const LicPage* LICENSE_PAGES;   // 加载后指向页数组
extern int LICENSE_PAGE_N;             // 页数（加载后有效）
bool loadSecureText();                 // 幂等；返回 false 表示使用了占位页

// 教程关卡
extern TutLevel TUT[8];

// 字库池：确保中文字体加载覆盖游戏内用到的所有字符
extern const char* MISC_POOL;

// 界面绘制代码里硬编码、不属于任何数据表的字符串。
// 字体图集按需生成，只扫描各数据表 + 这份清单；
// 新增界面文字若不在此处也不在任何数据表中，会因缺字形显示为问号！
extern const char* const UI_LITERALS[];
extern const int UI_LITERAL_N;
