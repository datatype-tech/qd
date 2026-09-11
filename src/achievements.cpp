// ============================================================
//  achievements.cpp  成就解锁与判定实现
// ============================================================
#include "achievements.h"
#include "state.h"
#include "data.h"
#include "save.h"
#include "audio.h"
#include "util.h"
#include <ctime>

void unlockAch(int id) {
    if (id < 0 || id >= A_COUNT || rec.achUnlocked[id]) return;
    rec.achUnlocked[id] = true;
    toasts.push_back({ id, 4.5f });
    playSfx(ACH[id].tier == T_RAINBOW ? sfxRecord : sfxAch);
    int sk = ACH[id].unlockSkin;
    if (sk >= 0 && sk < SKIN_COUNT && !rec.skinUnlocked[sk]) {
        rec.skinUnlocked[sk] = true;
        addLog(string("解锁新装扮：") + SKINS[sk].name, GOLD);
    }
    if (id == A_ENDLESS1000) {
        rec.luxUnlocked = true; rec.luxOn = true;
        addLog("传说解锁：宇宙尽头的花园豪华界面已永久启用！", GOLD);
    }
    saveRecords();
}
void checkAchievements() {
    if (rec.totalCats >= 1)     unlockAch(A_CAT1);
    if (rec.totalCats >= 10)    unlockAch(A_CAT10);
    if (rec.totalCats >= 30)    unlockAch(A_CAT30);
    if (rec.totalCats >= 100)   unlockAch(A_CAT100);
    if (maxCombo >= 3)          unlockAch(A_COMBO3);
    if (maxCombo >= 6)          unlockAch(A_COMBO6);
    if (maxCombo >= 10)         unlockAch(A_COMBO10);
    if (maxCombo >= 16)         unlockAch(A_COMBO16);
    if (rec.totalFlowers >= 10)  unlockAch(A_FLOWER10);
    if (rec.totalFlowers >= 40)  unlockAch(A_FLOWER40);
    if (rec.totalFlowers >= 120) unlockAch(A_FLOWER120);
    if (rec.totalEntSync >= 20) unlockAch(A_ENT20);
    if (rec.tutProgress >= 8)   unlockAch(A_TUTDONE);
    if (rec.totalBuys >= 15)    unlockAch(A_SHOP15);
    if (energy >= 300)          unlockAch(A_RICH300);
    if (mode == M_ENDLESS && turnNo >= 20)   unlockAch(A_ENDLESS20);
    if (mode == M_ENDLESS && turnNo >= 40)   unlockAch(A_ENDLESS40);
    if (mode == M_ENDLESS && turnNo >= 80)   unlockAch(A_ENDLESS80);
    // 终极成就：必须是无尽模式、存活至少 1000 回合、且本局仍在进行（灵能 > 0）
    if (mode == M_ENDLESS && turnNo >= 1000 && energy > 0) unlockAch(A_ENDLESS1000);
    if (rec.dailyStreak >= 7)   unlockAch(A_DAILY7);
    if (catStreak >= 3)         unlockAch(A_H_CATCOMBO);
    if (mode != M_TUTORIAL && energy > 0 && energy <= 1) unlockAch(A_H_LASTGASP);
    if (mode != M_TUTORIAL && turnNo >= 8) {          // 隐藏：空园之主
        bool empty = true;
        for (auto& c : grid) if (c.st != EMPTY) { empty = false; break; }
        if (empty) unlockAch(A_H_EMPTY);
    }
    {                                                  // 隐藏：满园芬芳
        int fl = 0;
        for (auto& c : grid) if (c.st == FLOWER) ++fl;
        if (fl >= SIZE * SIZE) unlockAch(A_H_ALLFLOWER);
    }
}

// 隐藏：夜猫子（本地时间 0 点到 4 点游玩）
void checkNightOwl() {
    time_t now = time(nullptr); tm* lt = localtime(&now);
    if (lt && lt->tm_hour >= 0 && lt->tm_hour < 5) unlockAch(A_H_NIGHTOWL);
}
