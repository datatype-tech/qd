// ============================================================
//  render.h  植物 / 商品图标 / 背景 / 特效 / 奖章绘制
// ============================================================
#pragma once
#include "types.h"

void drawSeed(Vector2 ct, int mat, float t);
void drawEnt(Vector2 ct, float t);
void drawFlower(Vector2 ct, int life, float t);
void drawItemIcon(int k, Vector2 c, float R, float t);

bool luxActive();                 // 豪华界面是否生效
void drawBG(float t);
void drawLuxFrame(float t);       // 【宇宙尽头的花园】豪华界面前景层
void drawFX(float dt);
void drawMedal(Vector2 c, float R, int tier, bool ok, float t);
void drawCurve(Rectangle r, bool big);

void toggleFull();
