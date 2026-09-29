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
void sceneSlots();
void sceneAbout();
void sceneSettings(float t);

// 是否有"清空纪录"这类模态确认弹窗正开着。
// main.cpp 用它挡掉全局快捷键（M 切音效 / H 开规则页）：弹窗是模态的，音效在
// 弹窗背后被悄悄切掉、或者按 H 跑到规则页把待确认的弹窗晾在一边，都容易让
// 玩家莫名其妙。ESC 与弹窗按钮由弹窗自己处理，不受此影响。
bool isModalOpen();
