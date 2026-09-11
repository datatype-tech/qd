// ============================================================
//  game.h  核心游戏逻辑：回合推进 / 观测坍缩 / 商店 / 存档评分
// ============================================================
#pragma once
#include "types.h"

void gotoResult();
bool goalMet();
string goalProgress();
void endTurn();

bool toolOK(int bit);
bool medReady();
bool casualMode();
bool medAllowed();
bool entropyOn();
int  curRiftDmg();
int  curStorm();
float curDecoher();
int  curEntropy();
int  upgCost(int i);

int todayCode();

void refreshShop();
void applyPreset(const char* p);
void resetGame(Mode m, int d);

int gainEnergy(int base, bool ent);
void spreadVine(int i);
void collapse(int i, int forced);

void checkStateAfterFreeAction();
void clickCell(int i);
void applyItem(int k);
void tryBuyFixed(int& lv, const int* cost, int maxLv, const string& name);
void tryBuySlot(int slot);

int calcScore();
string gradeOf(int s);
void pushHistory(int s);
string shareText();
