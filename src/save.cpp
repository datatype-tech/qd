// ============================================================
//  save.cpp  存档读写实现
// ============================================================
#include "save.h"
#include "state.h"
#include "data.h"
#include "util.h"
#include <fstream>
#include <sstream>
#include <cstdio>
#include <algorithm>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

static const char* SAVE_TAG = "QGSAVE";
// 6.3 未改动存档字段数量（装扮 18 款 / 成就 32 项 / 升级 4 项均不变），
// 因此仍沿用 610 版本号，老存档可直接读取，进度不丢失。
static const int   SAVE_VER = 610;
bool saveWasLegacy = false;

// 前置声明：批量删除槽位文件 + 对局存档落盘（定义在下方 runPath 之后）。
// clearAllRecords() 需要它们做"一次删完、只落盘一次"的批量删除。
static void removeRunFiles(int slot);
static void syncRunStore();

void loadRecords() {
    rec = Records{};                    // 先恢复默认，避免残留脏数据
    rec.skinUnlocked[0] = true;
    std::ifstream f("qgarden_record.txt");
    if (!f) return;

    // 版本校验：旧版存档字段数量不同，直接按位读会错位，
    // 把分数、星尘等非零值误读成“成就已解锁”，必须拒绝。
    string tag; int ver = 0;
    if (!(f >> tag) || tag != SAVE_TAG) { saveWasLegacy = true; return; }
    if (!(f >> ver) || ver != SAVE_VER) { saveWasLegacy = true; return; }

    Records r{};
    r.skinUnlocked[0] = true;
    bool okRead = true;
    for (int i = 0; i < 4 && okRead; ++i) okRead = (bool)(f >> r.bestClassic[i]);
    okRead = okRead && (bool)(f >> r.bestEndlessTurn >> r.bestEndlessScore >> r.bestCombo
                                >> r.totalCats >> r.tutProgress);
    okRead = okRead && (bool)(f >> r.totalFlowers >> r.totalEntSync >> r.totalBuys
                                >> r.totalWins >> r.totalGames);
    for (int i = 0; i < A_COUNT && okRead; ++i) {
        int v = 0; okRead = (bool)(f >> v); r.achUnlocked[i] = (v != 0);
    }
    for (int i = 0; i < SKIN_COUNT && okRead; ++i) {
        int v = 0; okRead = (bool)(f >> v); r.skinUnlocked[i] = (v != 0);
    }
    okRead = okRead && (bool)(f >> r.themeSkin >> r.seedSkin >> r.flowerSkin);
    okRead = okRead && (bool)(f >> r.stardust);
    for (int i = 0; i < U_COUNT && okRead; ++i) okRead = (bool)(f >> r.upg[i]);
    okRead = okRead && (bool)(f >> r.dailyDay >> r.dailyScore >> r.dailyStreak
                                >> r.dailyBest >> r.dailyDone);
    okRead = okRead && (bool)(f >> r.histN);
    r.histN = std::clamp(r.histN, 0, 12);
    for (int i = 0; i < r.histN && okRead; ++i) okRead = (bool)(f >> r.hist[i]);
    int lu = 0, lo = 1;
    if (okRead && (f >> lu)) { f >> lo; r.luxUnlocked = (lu != 0); r.luxOn = (lo != 0); }
    int la = 0;
    if (okRead && (f >> la)) { r.licenseAgreed = (la != 0); }   // 老存档没有这个字段，读不到就维持默认 false
    int as = 0;
    if (okRead && (f >> as)) { r.achSeen = as; }                // 同上：老存档缺此字段时默认 0
    int gm = 0;
    if (okRead && (f >> gm)) { r.guideMenuDone = (gm != 0); }   // 新手引导是否已看过

    if (!okRead) { saveWasLegacy = true; return; }   // 档案损坏：不采用

    rec = r;
    rec.skinUnlocked[0] = true;
    rec.themeSkin = std::clamp(rec.themeSkin, 0, SKIN_COUNT - 1);
    if (rec.seedSkin >= SKIN_COUNT || rec.seedSkin < -1) rec.seedSkin = -1;
    if (rec.flowerSkin >= SKIN_COUNT || rec.flowerSkin < -1) rec.flowerSkin = -1;
    for (int i = 0; i < U_COUNT; ++i) rec.upg[i] = std::clamp(rec.upg[i], 0, UPG[i].maxLv);
    // 一致性修正：终极成就与豪华界面、装扮必须同步
    if (rec.achUnlocked[A_ENDLESS1000]) {
        rec.luxUnlocked = true;
        rec.skinUnlocked[SKIN_COSMIC] = true;
    } else {
        rec.luxUnlocked = false;                     // 未拿到终极成就就不可能有豪华界面
    }
    for (int i = 0; i < A_COUNT; ++i)                // 已解锁成就补发对应装扮
        if (rec.achUnlocked[i] && ACH[i].unlockSkin >= 0)
            rec.skinUnlocked[ACH[i].unlockSkin] = true;
}

// 重置成就与装扮（保留统计与星尘）
void resetAchievements() {
    for (int i = 0; i < A_COUNT; ++i) rec.achUnlocked[i] = false;
    for (int i = 0; i < SKIN_COUNT; ++i) rec.skinUnlocked[i] = false;
    rec.skinUnlocked[0] = true;
    rec.themeSkin = 0; rec.seedSkin = -1; rec.flowerSkin = -1;
    rec.luxUnlocked = false; rec.luxOn = true;
}

// ============================================================
//  清空游戏纪录：把玩家档案与全部对局存档一次性抹掉
//  ------------------------------------------------------------
//  【为什么需要它】旧版本存档的字段数量与 610 不同，loadRecords() 检测到
//  版本号不匹配会判定为 legacy 并整体拒读（否则按位读会串位，把分数、星尘
//  误读成"成就已解锁"）。玩家升级后如果留下旧档案，就会一直卡在不可用的
//  状态里；本函数提供出口：删掉冲突的旧档案，从零重新开始。
//
//  清空范围：
//    - qgarden_record.txt     玩家档案（分数/成就/装扮/星尘/升级/每日/曲线/
//                             教程进度/引导标记/协议同意标记）
//    - qg_run_1..10.txt       全部对局存档槽位
//    - qg_run.txt             兼容槽位 1 的旧文件名
//  清空后 rec 取 Records{} 默认值，其中 licenseAgreed=false、guideMenuDone=false
//  正是"第一次打开游戏"的状态，因此下次启动会重新弹出用户协议与新手引导。
// ============================================================
void clearAllRecords() {
    // 先删掉全部对局存档槽位，避免主菜单继续显示"继续游戏 N/10"。
    // 这里用不带同步的 removeRunFiles()，10 个槽位删完只落盘一次 ——
    // 网页版每次 syncRunStore() 都是一次异步 FS.syncfs，逐个同步会一次排进
    // 10 次（见 removeRunFiles 的注释）。
    for (int s = 1; s <= RUN_SLOTS; ++s) removeRunFiles(s);
    syncRunStore();

    rec = Records{};                 // 全部字段回到初始值（含 licenseAgreed=false）
    rec.skinUnlocked[0] = true;      // 初始装扮：第一款默认解锁
    curRunSlot = 0;
    saveWasLegacy = false;           // 旧档案已被删除，冲突解除

    // 成就弹窗（AchToast）是独立的全局队列，不属于 rec；不清掉的话清档之后
    // 还会飘出"成就解锁"的提示，而对应的成就刚刚已经被删了。
    toasts.clear();

    // 写回一份全新的初始档案：这样即使玩家清空后直接关掉游戏（主循环末尾
    // 的 saveRecords() 没来得及跑），磁盘上留下的也是干净的初始档案。
    saveRecords();

#ifdef __EMSCRIPTEN__
    // 网页版：删除文件后必须再同步一次 IDBFS，否则 IndexedDB 里还留着旧档案，
    // 刷新页面就会"复活"。saveRecords() 的同步有 800ms 防抖，这里立即落盘，
    // 保证删除动作持久化。
    EM_ASM({ FS.syncfs(false, function(err){ if (err) console.log('[QG] 清空纪录落盘失败:', err); }); });
#endif
}

void saveRecords() {
    std::ofstream f("qgarden_record.txt");
    f << SAVE_TAG << " " << SAVE_VER << "\n";
    for (int i = 0; i < 4; ++i) f << rec.bestClassic[i] << " ";
    f << rec.bestEndlessTurn << " " << rec.bestEndlessScore << " "
      << rec.bestCombo << " " << rec.totalCats << " " << rec.tutProgress << "\n";
    f << rec.totalFlowers << " " << rec.totalEntSync << " " << rec.totalBuys << " "
      << rec.totalWins << " " << rec.totalGames << "\n";
    for (int i = 0; i < A_COUNT; ++i) f << (rec.achUnlocked[i] ? 1 : 0) << " ";
    f << "\n";
    for (int i = 0; i < SKIN_COUNT; ++i) f << (rec.skinUnlocked[i] ? 1 : 0) << " ";
    f << "\n" << rec.themeSkin << " " << rec.seedSkin << " " << rec.flowerSkin << "\n";
    f << rec.stardust << " ";
    for (int i = 0; i < U_COUNT; ++i) f << rec.upg[i] << " ";
    f << "\n" << rec.dailyDay << " " << rec.dailyScore << " " << rec.dailyStreak << " "
      << rec.dailyBest << " " << rec.dailyDone << "\n";
    f << rec.histN << " ";
    for (int i = 0; i < rec.histN; ++i) f << rec.hist[i] << " ";
    f << "\n" << (rec.luxUnlocked ? 1 : 0) << " " << (rec.luxOn ? 1 : 0) << "\n";
    f << (rec.licenseAgreed ? 1 : 0) << "\n";
    f << rec.achSeen << "\n";
    f << (rec.guideMenuDone ? 1 : 0) << "\n";

#ifdef __EMSCRIPTEN__
    // 网页版：MEMFS 是内存文件系统，必须把改动同步回 IndexedDB 才能持久保存。
    // 连续多次存档时只保留最后一次同步（防抖 800ms），避免
    // "FS.syncfs operations in flight" 警告和无谓的 IO。
    EM_ASM({
        if (Module._qgSyncTimer) clearTimeout(Module._qgSyncTimer);
        Module._qgSyncTimer = setTimeout(function() {
            Module._qgSyncTimer = null;
            FS.syncfs(false, function(err){ if(err) console.log('[QG] 存档保存失败:', err); });
        }, 800);
    });
#endif
}

// ============================================================
//  对局存档：把"当前这一局"的状态写进槽位文件 qg_run_<n>.txt
//  与玩家档案（成就/星尘等）分开，互不影响。
//  网页版走 IDBFS，因此写完同样需要 syncfs 落盘。
// ============================================================
static const char* RUN_TAG = "QGRUN1";

// 槽位文件路径：1..10 -> qg_run_1.txt ... qg_run_10.txt
// 为兼容旧版本，槽位 1 还会回退读取最初的 qg_run.txt。
static void runPath(int slot, int cand, char* buf, int n) {
    if (cand == 0) std::snprintf(buf, n, "qg_run_%d.txt", slot);
    else           std::snprintf(buf, n, "qg_run.txt");
}

static void syncRunStore() {
#ifdef __EMSCRIPTEN__
    EM_ASM({ FS.syncfs(false, function(err){ if(err) console.log('[QG] 对局存档保存失败:', err); }); });
#endif
}

// 只删文件、不落盘：给"一次性删很多槽位"的场合用。
// 网页版每个 syncRunStore() 都是一次异步 FS.syncfs；清空纪录要连删 10 个槽位
// （文件还可能本来就不存在），逐个同步会一口气排进 10 次 syncfs，正是
// saveRecords() 注释里想避免的 "FS.syncfs operations in flight" 情形。
// 所以批量删除走这个函数，最后统一同步一次。
static void removeRunFiles(int slot) {
    for (int cand = 0; cand < 2; ++cand) {
        if (cand == 1 && slot != 1) break;             // 仅槽位 1 兼容旧文件名
        char path[64]; runPath(slot, cand, path, sizeof(path));
        std::remove(path);
    }
}

bool hasSavedRun(int slot) {
    for (int cand = 0; cand < 2; ++cand) {
        if (cand == 1 && slot != 1) break;             // 仅槽位 1 兼容旧文件名
        char path[64]; runPath(slot, cand, path, sizeof(path));
        std::ifstream f(path);
        if (!f) continue;
        string tag;
        if ((f >> tag) && tag == RUN_TAG) return true;
    }
    return false;
}

RunSlotInfo readRunSlot(int slot) {
    RunSlotInfo info;
    for (int cand = 0; cand < 2; ++cand) {
        if (cand == 1 && slot != 1) break;
        char path[64]; runPath(slot, cand, path, sizeof(path));
        std::ifstream f(path);
        if (!f) continue;
        string tag; int m = 0, d = 0, ti = 0;
        if (!(f >> tag) || tag != RUN_TAG) continue;
        if (!(f >> m >> d >> ti)) continue;
        int turn = 0, energy = 0;
        if (!(f >> turn >> energy)) continue;
        info.used = true; info.mode = m; info.turn = turn; info.energy = energy;
        return info;
    }
    return info;
}

int usedSlotCount() {
    int n = 0;
    for (int s = 1; s <= RUN_SLOTS; ++s) if (hasSavedRun(s)) ++n;
    return n;
}

void clearRunSave(int slot) {
    removeRunFiles(slot);
    syncRunStore();
}

void saveRun(int slot) {
    char path[64]; runPath(slot, 0, path, sizeof(path));
    std::ofstream f(path);
    if (!f) return;
    f << RUN_TAG << "\n";
    f << mode << " " << diff << " " << tutIdx << "\n";
    f << turnNo << " " << energy << " " << combo << " " << maxCombo << " " << wave
      << " " << (int)tool << " " << entSel << "\n";
    f << lensLv << " " << stableLv << " " << ampLv << " " << plantCost
      << " " << maxTurn << " " << winEnergy << "\n";
    f << medReadyTurn << " " << shieldCharges << " " << eyeCharges << " " << luckCharges
      << " " << entropyCut << " " << springTurns << "\n";
    f << shopSlot[0] << " " << shopSlot[1] << " " << shopSlot[2] << " " << shopRefreshTurn << "\n";
    f << riftThisRun << " " << catStreak << " " << obsThisRun << " " << entUsedRun
      << " " << shopUsedRun << " " << minEnergyRun << "\n";
    f << statObs << " " << statMatObs << " " << statEntSync << " " << statBuy
      << " " << flowers << " " << cats << "\n";
    for (int i = 0; i < SIZE * SIZE; ++i)
        f << (int)grid[i].st << " " << grid[i].mat << " " << grid[i].partner << " " << grid[i].life << " ";
    f << "\n";
    std::ostringstream rs;
    rs << rng;
    f << rs.str() << "\n";
    f.close();
    curRunSlot = slot;
    syncRunStore();
}

bool loadRun(int slot) {
    for (int cand = 0; cand < 2; ++cand) {
        if (cand == 1 && slot != 1) break;
        char path[64]; runPath(slot, cand, path, sizeof(path));
        std::ifstream f(path);
        if (!f) continue;
        string tag;
        if (!(f >> tag) || tag != RUN_TAG) continue;

        int m = 0, d = 0, ti = 0;
        if (!(f >> m >> d >> ti)) continue;
        mode = (Mode)m; diff = d; tutIdx = ti;
        int tl = 0;
        if (!(f >> turnNo >> energy >> combo >> maxCombo >> wave >> tl >> entSel)) continue;
        tool = (Tool)tl;
        if (!(f >> lensLv >> stableLv >> ampLv >> plantCost >> maxTurn >> winEnergy)) continue;
        if (!(f >> medReadyTurn >> shieldCharges >> eyeCharges >> luckCharges >> entropyCut >> springTurns)) continue;
        if (!(f >> shopSlot[0] >> shopSlot[1] >> shopSlot[2] >> shopRefreshTurn)) continue;
        if (!(f >> riftThisRun >> catStreak >> obsThisRun >> entUsedRun >> shopUsedRun >> minEnergyRun)) continue;
        if (!(f >> statObs >> statMatObs >> statEntSync >> statBuy >> flowers >> cats)) continue;

        grid.assign(SIZE * SIZE, Cell{});
        bool ok = true;
        for (int i = 0; i < SIZE * SIZE && ok; ++i) {
            int st = 0, mat = 0, partner = -1, life = 0;
            if (!(f >> st >> mat >> partner >> life)) { ok = false; break; }
            grid[i] = Cell{ (CellSt)st, mat, partner, life };
        }
        if (!ok) continue;
        string rngLine;
        if (!(f >> rngLine)) continue;
        std::istringstream rs(rngLine);
        rs >> rng;

        turnNo = std::max(1, turnNo);
        energy = std::max(0, energy);
        entSel = -1;
        shopOpen = false; copied = false;
        winGame = false; newRecord = false;
        logs.clear(); floats.clear(); parts.clear(); shocks.clear();
        curRunSlot = slot;
        addLog("已载入存档槽位 " + to_string(slot) + "，欢迎回来继续耕耘。", GREEN);
        scene = PLAY;
        return true;
    }
    return false;
}
