// ============================================================
//  game.cpp  核心游戏逻辑实现
// ============================================================
#include "game.h"
#include "state.h"
#include "data.h"
#include "util.h"
#include "audio.h"
#include "save.h"
#include "achievements.h"
#include <algorithm>
#include <ctime>
#include <cstdlib>

// ==================== 逻辑辅助 ====================
bool toolOK(int bit) { return mode != M_TUTORIAL || (TUT[tutIdx].tools & bit); }
bool medReady() { return turnNo >= medReadyTurn; }
bool casualMode() { return mode == M_CLASSIC && diff == 0; }
bool medAllowed() { return toolOK(TL_MED) && !casualMode(); }
bool entropyOn() { return (mode == M_ENDLESS) || (mode == M_TUTORIAL && TUT[tutIdx].entropy); }
int  curRiftDmg() {
    int base = (mode == M_TUTORIAL) ? 12 : (mode == M_DAILY) ? 15 : CFG[diff].riftDmg;
    if (mode != M_TUTORIAL) base -= rec.upg[U_STABLE] * 2;
    return std::max(4, base);
}
int curStorm() {
    if (mode == M_TUTORIAL) return TUT[tutIdx].stormPct;
    if (mode == M_DAILY)    return 9;
    if (mode == M_ENDLESS)  return std::min(50, CFG[diff].stormPct + wave * 2);
    return CFG[diff].stormPct;
}
float curDecoher() {
    if (mode == M_TUTORIAL) return TUT[tutIdx].decoher ? 0.16f : 0.0f;
    if (mode == M_ENDLESS)  return std::min(0.30f, 0.10f + wave * 0.012f);
    if (mode == M_DAILY)    return 0.10f;
    return CFG[diff].decoher;
}
int curEntropy() { return std::max(0, 2 + wave - entropyCut); }
int upgCost(int i) { return UPG[i].base + UPG[i].step * rec.upg[i]; }

int todayCode() {
    time_t now = time(nullptr);
    tm* t = localtime(&now);
    return (t->tm_year + 1900) * 10000 + (t->tm_mon + 1) * 100 + t->tm_mday;
}

void refreshShop() {
    std::vector<int> pool;
    for (int k = IK_SPRING; k < IK_COUNT; ++k) {
        if (k == IK_ENTROPY && !entropyOn()) continue;
        if (k == IK_CRYSTAL && mode != M_CLASSIC) continue;
        pool.push_back(k);
    }
    std::shuffle(pool.begin(), pool.end(), rng);
    for (int i = 0; i < 3; ++i) shopSlot[i] = (i < (int)pool.size()) ? pool[i] : -1;
}
void applyPreset(const char* p) {
    for (int i = 0; i < SIZE * SIZE && p[i]; ++i) {
        if (p[i] >= '0' && p[i] <= '3') grid[i] = Cell{ SEED,p[i] - '0',-1,0 };
        else if (p[i] == 'F')           grid[i] = Cell{ FLOWER,0,-1,3 };
    }
}

void resetGame(Mode m, int d) {
    mode = m; if (m == M_CLASSIC) diff = d;
    if (m == M_DAILY) rng.seed((unsigned)todayCode());
    else rng.seed(std::random_device{}());
    grid.assign(SIZE * SIZE, Cell{});
    logs.clear(); floats.clear(); parts.clear(); shocks.clear();
    combo = maxCombo = flowers = cats = 0;
    lensLv = stableLv = ampLv = 0;
    statObs = statMatObs = statEntSync = statBuy = 0;
    entSel = -1; turnNo = 1; wave = 1;
    medReadyTurn = 0; shieldCharges = eyeCharges = luckCharges = entropyCut = springTurns = 0;
    shopOpen = false; copied = false;
    newRecord = false; winGame = false; tutPassed = false; riftThisRun = 0;
    catStreak = 0; obsThisRun = 0; entUsedRun = 0; shopUsedRun = 0; minEnergyRun = 9999;
    checkNightOwl();
    plantCost = 10;
    if (m != M_TUTORIAL) {
        plantCost = std::max(5, 10 - rec.upg[U_PLANT]);
        if (rec.upg[U_LENS] > 0) lensLv = 1;
    }
    if (m == M_CLASSIC) {
        energy = CFG[d].startE + rec.upg[U_ENERGY] * 6;
        maxTurn = CFG[d].turns; winEnergy = CFG[d].target;
        tool = T_OBSERVE;
        addLog("欢迎来到量子花园，园丁。目标：" + to_string(winEnergy) + " 灵能。", RAYWHITE);
    } else if (m == M_ENDLESS) {
        energy = 80 + rec.upg[U_ENERGY] * 6; maxTurn = 99999; winEnergy = 99999; tool = T_OBSERVE;
        addLog("无尽模式：熵增将不断吞噬灵能，生存到极限吧！", GOLD);
    } else if (m == M_DAILY) {
        energy = 60 + rec.upg[U_ENERGY] * 6; maxTurn = 28; winEnergy = 240; tool = T_OBSERVE;
        addLog("每日挑战 " + to_string(todayCode()) + "：全球同一牌局，祝你好运！", SKYBLUE);
    } else {
        TutLevel& L = TUT[tutIdx];
        energy = L.startE; maxTurn = L.turnLimit ? L.turnLimit : 99999; winEnergy = 99999;
        applyPreset(L.preset);
        tool = (L.tools & TL_OBS) ? T_OBSERVE : T_PLANT;
        addLog(string("提示：") + L.tip, SKYBLUE);
    }
    if (m != M_TUTORIAL) ++rec.totalGames;
    refreshShop();
    shopRefreshTurn = turnNo + 6;
    scene = PLAY;
}

int gainEnergy(int base, bool ent) {
    int g = (int)(base * (1.0 + combo * 0.10) * (ent ? 1.0 + ampLv * 0.6 : 1.0));
    energy += g; return g;
}
void spreadVine(int i) {
    int r = i / SIZE, c = i % SIZE;
    const int dr[] = { -1,1,0,0 }, dc[] = { 0,0,-1,1 };
    for (int k = 0; k < 4; ++k) {
        int nr = r + dr[k], nc = c + dc[k];
        if (nr >= 0 && nr < SIZE && nc >= 0 && nc < SIZE) {
            Cell& n = grid[nr * SIZE + nc];
            if (n.st == EMPTY) { n = Cell{ SEED,1,-1,0 }; emitBurst(cellCenter(nr * SIZE + nc), 10, GREEN, 90, 3); }
        }
    }
}

void collapse(int i, int forced) {
    Cell& c = grid[i];
    bool wasEnt = (c.st == ENTANGLED);
    int partner = c.partner;
    int roll;
    if (forced >= 0) roll = forced;
    else {
        roll = randInt(1, 100) + c.mat * 6 + lensLv * 6;
        if (luckCharges > 0) { roll += 12; --luckCharges; }
        if (eyeCharges > 0) { if (roll < 77) roll = randInt(77, 96); --eyeCharges; }
    }
    Vector2 ct = cellCenter(i);
    c = Cell{};

    if (roll >= 97) {
        playSfx(sfxCat);
        int g = gainEnergy(55, wasEnt);
        addLog("传说！薛定谔之猫跃出叠加态！+" + to_string(g), GOLD);
        addFloat({ ct.x,ct.y - 20 }, "+" + to_string(g), GOLD, 40);
        emitBurst(ct, 60, GOLD, 260, 5); addShock(ct, 130, GOLD); addShake(9);
        ++combo; ++cats; ++rec.totalCats; ++catStreak;
        checkAchievements();
    } else if (roll >= 77) {
        playSfx(sfxFlower);
        int g = gainEnergy(18, wasEnt);
        grid[i] = Cell{ FLOWER,0,-1,3 };
        addLog("坍缩为灵能花！+" + to_string(g) + "，未来三回合持续产出。", PINK);
        addFloat({ ct.x,ct.y - 20 }, "+" + to_string(g), PINK);
        emitBurst(ct, 26, PINK, 150, 4); addShock(ct, 80, PINK);
        ++combo; ++flowers; ++rec.totalFlowers; catStreak = 0;
        checkAchievements();
    } else if (roll >= 53) {
        playSfx(sfxVine);
        addLog("坍缩为量子藤！叠加态蔓延至相邻空地。", GREEN);
        emitBurst(ct, 20, GREEN, 130, 4); addShock(ct, 70, GREEN);
        spreadVine(i); ++combo; catStreak = 0;
    } else if (roll >= 32) {
        playSfx(sfxGrass);
        addLog("坍缩为枯草，一无所获，连击中断。", GRAY);
        emitBurst(ct, 14, Color{ 150,140,110,255 }, 80, 3); combo = 0; catStreak = 0;
    } else {
        int dmg = std::max(0, curRiftDmg() + (mode == M_ENDLESS ? wave : 0) - stableLv * 8);
        catStreak = 0;
        if (shieldCharges > 0) {
            --shieldCharges; dmg = 0;
            playSfx(sfxShield);
            addLog("相位护盾吸收了裂缝伤害！剩余 " + to_string(shieldCharges) + " 次。", SKYBLUE);
            emitBurst(ct, 22, SKYBLUE, 150, 4); addShock(ct, 90, SKYBLUE);
        } else {
            playSfx(sfxRift);
            ++riftThisRun;
            energy -= dmg;
            addLog("坍缩为时空裂缝！-" + to_string(dmg) + " 灵能，连击中断。", RED);
            addFloat({ ct.x,ct.y - 20 }, "-" + to_string(dmg), RED);
            emitBurst(ct, 34, RED, 200, 4); addShock(ct, 100, RED); addShake(12);
        }
        combo = 0;
    }
    if (combo > maxCombo) { maxCombo = combo; checkAchievements(); }
    if (partner >= 0 && grid[partner].st == ENTANGLED) {
        addLog("幽灵般的超距作用！纠缠伙伴同步坍缩。", VIOLET);
        ++statEntSync; ++rec.totalEntSync;
        checkAchievements();
        collapse(partner, roll);
    }
}

bool goalMet() {
    TutLevel& L = TUT[tutIdx];
    switch (L.goal) {
        case G_OBSERVE:  return statObs >= L.goalN;
        case G_MATOBS:   return statMatObs >= L.goalN;
        case G_COMBO:    return maxCombo >= L.goalN;
        case G_ENTSYNC:  return statEntSync >= L.goalN;
        case G_BUY:      return statBuy >= L.goalN;
        case G_ENERGY:   return energy >= L.goalN;
        case G_FLOWER:   return flowers >= L.goalN;
        case G_SURVIVE:  return turnNo >= L.goalN;
    }
    return false;
}
string goalProgress() {
    TutLevel& L = TUT[tutIdx];
    int cur = 0;
    switch (L.goal) {
        case G_OBSERVE: cur = statObs; break;
        case G_MATOBS: cur = statMatObs; break;
        case G_COMBO: cur = maxCombo; break;
        case G_ENTSYNC: cur = statEntSync; break;
        case G_BUY: cur = statBuy; break;
        case G_ENERGY: cur = std::max(0, energy); break;
        case G_FLOWER: cur = flowers; break;
        case G_SURVIVE: cur = turnNo; break;
    }
    return string(L.goalDesc) + "   " + to_string(std::min(cur, L.goalN)) + " / " + to_string(L.goalN);
}

void endTurn() {
    ++turnNo;
    for (auto& c : grid) if (c.st == SEED && c.mat < 3) ++c.mat;
    int passive = 0;
    for (int i = 0; i < SIZE * SIZE; ++i)
        if (grid[i].st == FLOWER) {
            passive += 4; emitBurst(cellCenter(i), 5, PINK, 45, 2.5f);
            if (--grid[i].life <= 0) grid[i] = Cell{};
        }
    if (passive) { energy += passive; addLog("灵能花产出 " + to_string(passive) + " 灵能。", PINK); }

    if (springTurns > 0) {
        --springTurns; energy += 10;
        addLog("灵能源泉涌出 10 灵能，剩余 " + to_string(springTurns) + " 回合。", GOLD);
        addFloat({ 300, 300 }, "+10", GOLD, 32);
    }

    float dch = curDecoher();
    for (int i = 0; i < SIZE * SIZE; ++i)
        if (grid[i].st == SEED && grid[i].mat >= 3 && randF() < dch) {
            grid[i] = Cell{};
            playSfx(sfxDecoh);
            addLog("退相干！一株过熟的种子悄然消散。", Color{ 160,160,180,255 });
            emitBurst(cellCenter(i), 16, Color{ 170,170,200,255 }, 70, 3);
        }

    int ev = randInt(1, 100), sp = curStorm();
    if (ev <= 8) {
        std::vector<int> es;
        for (int i = 0; i < SIZE * SIZE; ++i) if (grid[i].st == EMPTY) es.push_back(i);
        if (!es.empty()) {
            int p = es[randInt(0, (int)es.size() - 1)];
            grid[p] = Cell{ SEED,0,-1,0 };
            playSfx(sfxTunnel);
            addLog("量子隧穿！一颗种子凭空出现。", SKYBLUE);
            emitBurst(cellCenter(p), 18, SKYBLUE, 120, 3);
        }
    } else if (ev <= 8 + sp) {
        int t = -1, best = -1;
        for (int i = 0; i < SIZE * SIZE; ++i)
            if (grid[i].st == SEED && grid[i].mat > best) { best = grid[i].mat; t = i; }
        if (t >= 0) {
            playSfx(sfxStorm);
            emitBurst(cellCenter(t), 28, RED, 170, 4); addShake(8);
            grid[t] = Cell{};
            addLog("量子风暴来袭！最成熟的种子被抹除！", RED);
        }
    } else if (ev <= 8 + sp + 5) {
        energy += 20;
        playSfx(sfxTide);
        addLog("灵能潮汐涌过花园，+20 灵能。", GOLD);
    }

    if (entropyOn()) {
        int nw = 1 + (turnNo - 1) / 8;
        if (nw != wave) { wave = nw; addLog("熵增加剧！波次提升至 " + to_string(wave), ORANGE); addShake(6); }
        int e = curEntropy(); energy -= e;
        plantCost = std::min(250, std::max(5, 10 - rec.upg[U_PLANT]) + (wave - 1) * 2);
        addLog("熵增吞噬了 " + to_string(e) + " 灵能。", Color{ 190,150,140,255 });
    }

    if (turnNo >= shopRefreshTurn) {
        refreshShop();
        shopRefreshTurn = turnNo + 6;
        addLog("货架已刷新，新的限时商品到货了。", SKYBLUE);
    }

    checkAchievements();

    if (mode == M_TUTORIAL) {
        if (goalMet()) { tutPassed = true; gotoResult(); return; }
        if (energy <= 0 || (TUT[tutIdx].turnLimit && turnNo > TUT[tutIdx].turnLimit)) {
            tutPassed = false; gotoResult();
        }
    } else if (mode == M_ENDLESS) {
        if (energy <= 0) gotoResult();
    } else {
        if (energy <= 0 || energy >= winEnergy || turnNo > maxTurn) gotoResult();
    }
}

int calcScore() {
    if (mode == M_ENDLESS)
        return turnNo * 12 + maxCombo * 15 + flowers * 8 + cats * 40;
    return std::max(0, energy) + maxCombo * 15 + flowers * 10 + cats * 40 + (winGame ? 100 : 0);
}
string gradeOf(int s) { return s >= 420 ? "S" : s >= 320 ? "A" : s >= 210 ? "B" : "C"; }

void pushHistory(int s) {
    if (rec.histN < 12) rec.hist[rec.histN++] = s;
    else { for (int i = 0; i < 11; ++i) rec.hist[i] = rec.hist[i + 1]; rec.hist[11] = s; }
}

void gotoResult() {
    shopOpen = false; copied = false;
    winGame = ((mode == M_CLASSIC || mode == M_DAILY) && energy >= winEnergy);
    bool good = (mode == M_TUTORIAL) ? tutPassed : winGame;
    playSfx(good ? sfxWin : sfxLose);

    if (mode == M_TUTORIAL) {
        if (tutPassed && tutIdx + 1 > rec.tutProgress) { rec.tutProgress = tutIdx + 1; newRecord = true; }
        checkAchievements();
        if (newRecord) playSfx(sfxRecord);
        saveRecords(); scene = RESULT; return;
    }

    if (mode == M_CLASSIC && winGame) {
        ++rec.totalWins;
        { const int winAch[4] = { A_WINCASUAL, A_WINSTD, A_WINHARD, A_WINNIGHT };
          unlockAch(winAch[std::clamp(diff, 0, 3)]); }
        if (!shopUsedRun) unlockAch(A_H_NOSHOP);       // 隐藏：苦行僧
        if (!entUsedRun)  unlockAch(A_H_PACIFIST);     // 隐藏：和平主义者
        if (riftThisRun == 0) unlockAch(A_PERFECT);
    }
    if (mode == M_DAILY && winGame && riftThisRun == 0) unlockAch(A_PERFECT);

    lastScore = calcScore();
    lastStardust = lastScore / 8;
    rec.stardust += lastStardust;
    pushHistory(lastScore);

    if (mode == M_CLASSIC) {
        if (lastScore > rec.bestClassic[diff]) { rec.bestClassic[diff] = lastScore; newRecord = true; }
    } else if (mode == M_ENDLESS) {
        if (turnNo > rec.bestEndlessTurn) { rec.bestEndlessTurn = turnNo; newRecord = true; }
        if (lastScore > rec.bestEndlessScore) { rec.bestEndlessScore = lastScore; newRecord = true; }
    } else if (mode == M_DAILY) {
        int today = todayCode();
        if (rec.dailyDay != today) {
            if (rec.dailyDone == 1 && rec.dailyDay == today - 1) ++rec.dailyStreak;
            else rec.dailyStreak = 1;
            rec.dailyDay = today; rec.dailyScore = lastScore; rec.dailyDone = 1;
            newRecord = true;
            unlockAch(A_DAILY1);
        }
        if (lastScore > rec.dailyBest) { rec.dailyBest = lastScore; newRecord = true; }
    }
    if (maxCombo > rec.bestCombo) rec.bestCombo = maxCombo;
    checkAchievements();
    if (newRecord) playSfx(sfxRecord);
    saveRecords(); scene = RESULT;
}

void checkStateAfterFreeAction() {
    if (mode == M_TUTORIAL) {
        if (goalMet()) { tutPassed = true; gotoResult(); return; }
        if (energy <= 0) { tutPassed = false; gotoResult(); }
        return;
    }
    if (energy <= 0) { gotoResult(); return; }
    if ((mode == M_CLASSIC || mode == M_DAILY) && energy >= winEnergy) gotoResult();
}

void clickCell(int i) {
    Cell& c = grid[i];
    if (tool == T_PLANT) {
        if (!toolOK(TL_PLANT)) { addLog("本关暂未解锁该功能。", GRAY); return; }
        if (c.st != EMPTY) { addLog("该位置已被占用。", GRAY); return; }
        if (energy < plantCost) { addLog("灵能不足！", RED); return; }
        c = Cell{ SEED,0,-1,0 }; energy -= plantCost;
        playSfx(sfxPlant);
        emitBurst(cellCenter(i), 12, Color{ 190,200,230,255 }, 70, 3);
        addLog("种子落入土壤，进入叠加态。", RAYWHITE);
        endTurn();
    } else if (tool == T_OBSERVE) {
        if (c.st != SEED && c.st != ENTANGLED) { addLog("那里没有可观测的叠加态。", GRAY); return; }
        ++statObs; if (c.st == SEED && c.mat >= 3) ++statMatObs;
        playSfx(sfxObserve);
        collapse(i, -1); endTurn();
    } else {
        if (!toolOK(TL_ENT)) { addLog("本关暂未解锁该功能。", GRAY); return; }
        if (c.st != SEED) { addLog("必须选择叠加态种子！", RED); return; }
        if (entSel < 0) { entSel = i; addLog("已选中第一株，请点击第二株。", VIOLET); }
        else if (entSel != i) {
            grid[entSel].st = grid[i].st = ENTANGLED;
            grid[entSel].partner = i; grid[i].partner = entSel;
            entUsedRun = 1;
            playSfx(sfxEnt);
            emitBurst(cellCenter(i), 16, VIOLET, 110, 3);
            emitBurst(cellCenter(entSel), 16, VIOLET, 110, 3);
            entSel = -1;
            addLog("量子纠缠建立！观测其一将同步坍缩。", VIOLET);
            endTurn();
        }
    }
}

void applyItem(int k) {
    switch (k) {
    case IK_SPRING:
        springTurns += 5;
        addLog("灵能源泉已开启，接下来 5 个回合每回合产出 10 灵能。", GOLD);
        break;
    case IK_FERT: {
        int n = 0;
        for (int i = 0; i < SIZE * SIZE; ++i)
            if (grid[i].st == SEED && grid[i].mat < 3) { ++grid[i].mat; ++n; emitBurst(cellCenter(i), 8, GREEN, 70, 3); }
        addLog(n ? "量子肥料让所有种子成熟度 +1。" : "场上没有合适目标，效果未生效。", n ? GREEN : GRAY);
    } break;
    case IK_SHIELD:
        shieldCharges += 2;
        addLog("相位护盾已激活，可吸收 2 次裂缝伤害。", SKYBLUE);
        break;
    case IK_SEEDER: {
        std::vector<int> es;
        for (int i = 0; i < SIZE * SIZE; ++i) if (grid[i].st == EMPTY) es.push_back(i);
        std::shuffle(es.begin(), es.end(), rng);
        int n = std::min(3, (int)es.size());
        for (int i = 0; i < n; ++i) { grid[es[i]] = Cell{ SEED,1,-1,0 }; emitBurst(cellCenter(es[i]), 10, GREEN, 80, 3); }
        addLog(n ? "自动播种机种下了 " + to_string(n) + " 颗种子。" : "场上没有合适目标，效果未生效。",
               n ? GREEN : GRAY);
    } break;
    case IK_EYE:
        eyeCharges += 1;
        addLog("观测者之眼已凝聚，下次观测保底灵能花。", PINK);
        break;
    case IK_BLOOM: {
        int n = 0;
        for (int i = 0; i < SIZE * SIZE; ++i)
            if (grid[i].st == FLOWER) { grid[i].life += 3; ++n; emitBurst(cellCenter(i), 10, PINK, 70, 3); }
        addLog(n ? "花之祝福让所有灵能花花期 +3。" : "场上没有合适目标，效果未生效。", n ? PINK : GRAY);
    } break;
    case IK_CORE: {
        std::vector<int> ss;
        for (int i = 0; i < SIZE * SIZE; ++i) if (grid[i].st == SEED) ss.push_back(i);
        if (ss.size() >= 2) {
            std::shuffle(ss.begin(), ss.end(), rng);
            int a = ss[0], b = ss[1];
            grid[a].st = grid[b].st = ENTANGLED;
            grid[a].partner = b; grid[b].partner = a;
            emitBurst(cellCenter(a), 16, VIOLET, 110, 3);
            emitBurst(cellCenter(b), 16, VIOLET, 110, 3);
            addLog("纠缠核心自动建立了一对纠缠。", VIOLET);
        } else addLog("场上没有合适目标，效果未生效。", GRAY);
    } break;
    case IK_LUCK:
        luckCharges += 3;
        addLog("幸运硬币生效，接下来 3 次观测 +12 加值。", GOLD);
        break;
    case IK_CRYSTAL:
        maxTurn += 3;
        addLog("时间结晶延长了 3 个回合。", SKYBLUE);
        break;
    case IK_ENTROPY:
        entropyCut += 2;
        addLog("熵抑制器让每回合熵增减少 2 点。", ORANGE);
        break;
    }
}

void tryBuyFixed(int& lv, const int* cost, int maxLv, const string& name) {
    if (!toolOK(TL_SHOP)) { addLog("本关暂未解锁该功能。", GRAY); return; }
    if (lv >= maxLv) { addLog(name + " 已满级。", GRAY); return; }
    if (energy < cost[lv]) { addLog("灵能不足！", RED); return; }
    energy -= cost[lv]; ++lv; ++statBuy; ++rec.totalBuys; shopUsedRun = 1;
    playSfx(sfxBuy);
    addLog("购买成功！" + name + " 升级至 Lv" + to_string(lv) + "（不消耗回合）", GOLD);
    checkAchievements();
    checkStateAfterFreeAction();
}
void tryBuySlot(int slot) {
    if (!toolOK(TL_SHOP)) { addLog("本关暂未解锁该功能。", GRAY); return; }
    int k = shopSlot[slot];
    if (k < 0) return;
    if (energy < ITEMS[k].cost) { addLog("灵能不足！", RED); return; }
    energy -= ITEMS[k].cost; ++statBuy; ++rec.totalBuys; shopUsedRun = 1;
    shopSlot[slot] = -1;
    playSfx(sfxItem);
    addLog("购买成功！" + string(ITEMS[k].name) + "（不消耗回合）", GOLD);
    applyItem(k);
    checkAchievements();
    checkStateAfterFreeAction();
}

// ==================== 结算分享文本 ====================
string shareText() {
    string modeName = mode == M_ENDLESS ? "无尽" : mode == M_DAILY ? "每日挑战"
                    : string("经典") + CFG[diff].name;
    string s = "量子花园 6.0 | " + modeName;
    if (mode == M_DAILY) s += " #" + to_string(todayCode());
    s += "\n评分 " + to_string(lastScore) + "  评级 " + gradeOf(lastScore);
    s += "\n回合 " + to_string(turnNo) + "  灵能 " + to_string(std::max(0, energy));
    s += "\n连击 x" + to_string(maxCombo) + "  花 " + to_string(flowers) + "  猫 " + to_string(cats);
    s += "\n裂缝 " + to_string(riftThisRun) + " 次  星尘 +" + to_string(lastStardust);
    return s;
}
