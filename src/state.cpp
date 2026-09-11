// ============================================================
//  state.cpp  全局可变状态定义
// ============================================================
#include "state.h"

RenderTexture2D target;
Vector2 gMouse = { 0,0 };
float gScale = 1.0f;
bool uiLock = false;

Scene scene = MENU, prevScene = MENU;
Tool tool = T_OBSERVE;
Mode mode = M_CLASSIC;
int diff = 1, helpPage = 0, tutIdx = 0, licensePage = 0;
std::vector<Cell> grid(SIZE * SIZE);
std::deque<LogMsg> logs;
std::vector<FloatTxt> floats;
std::vector<Particle> parts;
std::vector<Shock> shocks;
std::mt19937 rng{ std::random_device{}() };
Font font;
Font fontTitle;
Records rec;

int energy, turnNo, combo, maxCombo, maxTurn, winEnergy, wave;
int lensLv, stableLv, ampLv, flowers, cats, plantCost, entSel;
int statObs, statMatObs, statEntSync, statBuy;
int medReadyTurn = 0;
int shieldCharges = 0, eyeCharges = 0, luckCharges = 0, entropyCut = 0, springTurns = 0;
int shopSlot[3] = { -1,-1,-1 }, shopRefreshTurn = 0;
int riftThisRun = 0, lastScore = 0, lastStardust = 0;
int catStreak = 0, obsThisRun = 0, entUsedRun = 0, shopUsedRun = 0, minEnergyRun = 9999;
int achPage = 0;
bool shopOpen = false, copied = false;
bool winGame = false, newRecord = false, tutPassed = false;
bool shouldQuit = false;
float shake = 0, gTime = 0;
const int lensCost[] = { 30,55,90 };
const int stableCost[] = { 28,50 };
const int ampCost[] = { 32,60 };

std::vector<AchToast> toasts;
