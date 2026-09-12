// ============================================================
//  save.h  存档读写（qgarden_record.txt）
// ============================================================
#pragma once
#include "types.h"

extern bool saveWasLegacy;   // 读到旧版存档时置位

void loadRecords();
void saveRecords();
void resetAchievements();    // 重置成就与装扮（保留统计与星尘）

// ---------- 对局存档（10 个槽位）：支持中途退出后"继续游戏/导入存档" ----------
// 槽位编号 1..10，文件为 qg_run_1.txt ... qg_run_10.txt，可反复覆盖写入。
const int RUN_SLOTS = 10;
struct RunSlotInfo { bool used = false; int mode = 0; int turn = 0; int energy = 0; };

RunSlotInfo readRunSlot(int slot);   // 读取槽位摘要（用于存档界面显示）
bool hasSavedRun(int slot);          // 指定槽位是否有存档
void saveRun(int slot);              // 把当前对局写入指定槽位（覆盖旧档）
bool loadRun(int slot);              // 读取指定槽位并恢复对局
void clearRunSave(int slot);         // 删除指定槽位
int  usedSlotCount();                // 已使用的槽位数量（用于主菜单提示）
