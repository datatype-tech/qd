// ============================================================
//  state.h  全局可变状态（游戏运行期状态、随机数引擎、存档缓存等）
//  定义见 state.cpp
// ============================================================
#pragma once
#include "types.h"
#include <random>

extern Scene scene, prevScene;
extern Tool tool;
extern Mode mode;
extern int diff, helpPage, tutIdx, licensePage;
extern int slotsMode;    // 0 = 保存模式（对局中进入）1 = 读取/管理模式（主菜单进入）
extern int curRunSlot;   // 当前对局关联的存档槽位（1..10，0 表示未关联）
extern std::vector<Cell> grid;
extern std::deque<LogMsg> logs;
extern std::vector<FloatTxt> floats;
extern std::vector<Particle> parts;
extern std::vector<Shock> shocks;
extern std::mt19937 rng;
extern Font font;        // 正文字体
extern Font fontTitle;   // 标题 / 强调字体（更粗字重）
extern Records rec;

extern int energy, turnNo, combo, maxCombo, maxTurn, winEnergy, wave;
extern int lensLv, stableLv, ampLv, flowers, cats, plantCost, entSel;
extern int statObs, statMatObs, statEntSync, statBuy;
extern int medReadyTurn;
extern int shieldCharges, eyeCharges, luckCharges, entropyCut, springTurns;
extern int shopSlot[3], shopRefreshTurn;
extern int riftThisRun, lastScore, lastStardust;
extern int catStreak, obsThisRun, entUsedRun, shopUsedRun, minEnergyRun;
extern int achPage;
extern bool shopOpen, copied;
extern bool winGame, newRecord, tutPassed;
extern bool shouldQuit;   // 置为 true 时主循环下一帧结束（用于"不同意条款并退出"）
extern float shake, gTime;
extern const int lensCost[3];
extern const int stableCost[2];
extern const int ampCost[2];

extern std::vector<AchToast> toasts;

// 渲染与输入相关的全局状态（原本定义在主文件顶部）
extern RenderTexture2D target;
extern Vector2 gMouse;
extern float gScale;
extern bool uiLock;
