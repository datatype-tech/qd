// ============================================================
//  types.h  基础枚举 / 结构 / 常量定义
//  被所有模块共享，必须最先包含
// ============================================================
#pragma once
#include "raylib.h"
#include <string>
#include <vector>
#include <deque>

using std::string;
using std::to_string;

// ==================== 虚拟分辨率与棋盘常量 ====================
// SSAA：超采样倍数。所有绘制逻辑仍使用 VW×VH 虚拟坐标，
// 内部画布放大 SSAA 倍后再缩回，为几何图形与文字提供抗锯齿。
// （raylib 无法给 RenderTexture 开 MSAA，故用超采样替代。）
// 3x：全屏时（如 1920x1080）内部画布 3840x2160 缩到屏上仍是 2 倍干净降采样，
// 文字在全屏下保持与窗口同等的锐度（原 2x 在全屏只有约 1.33 倍，文字发虚）。
const int SSAA = 3;
const int VW = 1280, VH = 720;
const int SIZE = 5, CELL = 96, GAP = 8;
const int GRID_X = 40, GRID_Y = 150;
const float PX = 600;

// ==================== 基础结构 ====================
enum CellSt { EMPTY, SEED, ENTANGLED, FLOWER };
struct Cell { CellSt st = EMPTY; int mat = 0, partner = -1, life = 0; };
struct LogMsg { string text; Color col; };
struct FloatTxt { Vector2 pos; string text; Color col; float life, size; };
struct Particle { Vector2 pos, vel; float life, maxLife, size; Color col; };
struct Shock { Vector2 pos; float r, maxR, life; Color col; };

enum Scene { MENU, HELP, PLAY, RESULT, TUT_SEL, TUT_BRIEF, ACHIEVE, SKINSEL, META, STATS, LICENSE, SLOTS };
enum Tool { T_PLANT, T_OBSERVE, T_ENTANGLE };
enum Mode { M_CLASSIC, M_ENDLESS, M_TUTORIAL, M_DAILY };

const int TL_PLANT = 1, TL_OBS = 2, TL_ENT = 4, TL_SHOP = 8, TL_MED = 16;
const int TL_ALL = 31;

enum GoalType { G_OBSERVE, G_MATOBS, G_COMBO, G_ENTSYNC, G_BUY, G_ENERGY, G_FLOWER, G_SURVIVE };

struct TutLevel {
    const char* title;
    const char* teach[6];
    const char* goalDesc;
    GoalType goal; int goalN;
    int startE, turnLimit, tools, stormPct; bool decoher, entropy;
    const char* preset;
    const char* tip;
};

struct DiffCfg { const char* name; int turns, target, startE, riftDmg, stormPct; float decoher; };

// ==================== 商品 ====================
enum ItemKind {
    IK_LENS, IK_STAB, IK_AMP,
    IK_SPRING, IK_FERT, IK_SHIELD, IK_SEEDER, IK_EYE,
    IK_BLOOM, IK_CORE, IK_LUCK, IK_CRYSTAL, IK_ENTROPY, IK_COUNT
};
struct ItemDef { const char* name; const char* d1; const char* d2; int cost; int cidx; };

// ==================== 成就与装扮 ====================
// 等级：0=铜 1=银 2=金 3=彩虹
enum AchTier { T_BRONZE, T_SILVER, T_GOLD, T_RAINBOW };

enum AchId {
    // 猫系列（铜银金彩）
    A_CAT1, A_CAT10, A_CAT30, A_CAT100,
    // 连击系列
    A_COMBO3, A_COMBO6, A_COMBO10, A_COMBO16,
    // 花系列
    A_FLOWER10, A_FLOWER40, A_FLOWER120,
    // 通关系列
    A_WINCASUAL, A_WINSTD, A_WINHARD, A_WINNIGHT,
    // 无尽系列（终极成就在最后）
    A_ENDLESS20, A_ENDLESS40, A_ENDLESS80, A_ENDLESS1000,
    // 其它
    A_TUTDONE, A_ENT20, A_SHOP15, A_RICH300, A_PERFECT, A_DAILY1, A_DAILY7,
    // 隐藏成就
    A_H_EMPTY, A_H_ALLFLOWER, A_H_NOSHOP, A_H_LASTGASP, A_H_CATCOMBO,
    A_H_PACIFIST, A_H_NIGHTOWL,
    A_COUNT
};
struct AchDef {
    const char* name; const char* desc; const char* cond;
    int tier; int unlockSkin; bool hidden;
};

// ---------- 装扮 ----------
// type: 0=界面风格  1=种子造型  2=花朵造型
// style: 当 type==0 时指向 ThemeKind；type!=0 时指向造型 ID（SeedArt / FlowerArt）
struct SkinDef { const char* name; const char* desc; int type; Color a, b; int style; };
const int SKIN_COUNT = 18;
// 终极装扮（豪华界面）由 A_ENDLESS1000 解锁
const int SKIN_COSMIC = 14;

// ==================== 界面风格系统 ====================
// 每种风格不只换色，而是整套视觉语言：幕布、面板、按钮形状、标题处理
enum ThemeKind {
    TK_QUANTUM,   // 量子基调：冷静深空，初始风格
    TK_MYSTIC,    // 玄幻仙纹：云纹符箓、仙气流转
    TK_STEAM,     // 黄铜蒸汽：齿轮铆钉、煤气灯暖黄
    TK_NATURE,    // 苔原苗圃：木纹叶脉、晨光苗床
    TK_TECH,      // 赛博矩阵：电路走线、HUD 切角
    TK_CAT,       // 猫猫乐园：爪印肉垫、毛球按钮
    TK_MECHA,     // 熔岩机甲：切角装甲、熔流警条
    TK_VOID,      // 虚空极简：绝对黑与一线留白
    TK_ENTROPY,   // 熵潮血月：血月低垂、热寂涟漪
    TK_COSMIC,    // 宇宙尽头：豪华星系（终极成就）
    TK_COUNT
};

// 按钮造型：每种风格一套独立画法
enum BtnShape {
    BS_ROUND,     // 标准圆角
    BS_RUNE,      // 符箓：卷角边框 + 两侧仙纹
    BS_BEVEL,     // 黄铜：斜面高光 + 四角铆钉
    BS_LEAF,      // 叶片：左右叶尖 + 叶脉
    BS_SHARP,     // HUD：直角切角 + 转角括号
    BS_EAR,       // 猫耳：胶囊 + 顶部双耳 + 爪印
    BS_ARMOR,     // 装甲：切角板 + 斜纹警条
    BS_LINE,      // 极简：一条细线与留白
    BS_PILL,      // 涟漪胶囊
    BS_ORNATE     // 华丽：金饰四角 + 双线
};

struct ThemeStyle {
    ThemeKind kind;
    const char* label;       // 风格名（显示在衣橱预览里）
    Color bgTop, bgBot;      // 背景幕布渐变
    Color accent, accent2;   // 主 / 副强调色
    Color panel, panelEdge;  // 面板底色 / 描边
    Color title;             // 标题字色
    Color text, textDim;     // 正文 / 弱化文字
    Color cellFill, cellEdge;// 棋盘格底 / 描边
    BtnShape btn;
    float round;             // 按钮圆角系数
    float titleSpace;        // 标题字距
    bool  titleGlow;         // 标题是否带辉光
};

// ==================== 造型 ID ====================
// 种子造型（对应 SkinDef.style，type==1）
enum SeedArt { SA_SPROUT, SA_CRYSTAL, SA_GOLD, SA_ETERNAL, SA_CATEYE };
// 花朵造型（对应 SkinDef.style，type==2）
enum FlowerArt { FA_BLOSSOM, FA_STARLIGHT, FA_GALAXY, FA_PERFECT, FA_FRAGRANT, FA_CATTRIO };

// 成熟度四阶的名称：不再用数字，而用阶段名描述
extern const char* MAT_STAGE[4];

// ==================== 元进度 ====================
enum UpgId { U_ENERGY, U_PLANT, U_STABLE, U_LENS, U_COUNT };
struct UpgDef { const char* name; const char* desc; int maxLv; int base; int step; };

// ==================== 存档 ====================
struct Records {
    int bestClassic[4] = { 0,0,0,0 };
    int bestEndlessTurn = 0, bestEndlessScore = 0;
    int bestCombo = 0, totalCats = 0, tutProgress = 0;
    int totalFlowers = 0, totalEntSync = 0, totalBuys = 0, totalWins = 0, totalGames = 0;
    bool achUnlocked[A_COUNT] = { false };
    bool skinUnlocked[SKIN_COUNT] = { false };
    int themeSkin = 0, seedSkin = -1, flowerSkin = -1;
    int stardust = 0, upg[U_COUNT] = { 0,0,0,0 };
    int dailyDay = 0, dailyScore = 0, dailyStreak = 0, dailyBest = 0, dailyDone = 0;
    int histN = 0, hist[12] = { 0 };
    bool luxUnlocked = false, luxOn = true;
    bool licenseAgreed = false;   // 是否已同意版权/反编译声明
    int  achSeen = 0;             // 玩家已查看过的成就数量（用于成就墙红点提示）
    bool guideMenuDone = false;   // 首次进入游戏的新手引导是否已看过
};

// ==================== 规则手册 ====================
struct HLine { float x, y, size; int c; const char* s; };
struct HPage { const char* tab; const HLine* l; int n; };

// ==================== 用户许可协议（EULA） ====================
// 不用固定坐标，改为逐行数组 + 运行时按行距自动步进（类似教程简介页的排版），
// 这样条款文字量再大也不会因为手写坐标算错而重叠或溢出面板。
enum LicKind { LK_TITLE, LK_SECTION, LK_SUB, LK_BODY, LK_NOTE, LK_GAP };
struct LicLine { LicKind kind; const char* s; };
struct LicPage { const char* title; const LicLine* l; int n; };

// ==================== 成就 Toast ====================
struct AchToast { int id; float life; };
