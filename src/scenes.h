// ============================================================
//  scenes.h  各界面（场景）绘制函数
// ============================================================
#pragma once
#include "types.h"

void sceneMenu(float t);
void sceneHelp();
void sceneLicense();
void sceneTutSel();
void sceneTutBrief();
void sceneAchieve(float t);
void sceneSkin(float t);
void sceneMeta(float t);
void sceneStats();
void drawShopCard(Rectangle r, int k, bool fixedItem, float t);
void sceneShop(float t);
void scenePlay(float dt, float t);
void sceneResult(float t);
