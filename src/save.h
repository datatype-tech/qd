// ============================================================
//  save.h  存档读写（qgarden_record.txt）
// ============================================================
#pragma once
#include "types.h"

extern bool saveWasLegacy;   // 读到旧版存档时置位

void loadRecords();
void saveRecords();
void resetAchievements();    // 重置成就与装扮（保留统计与星尘）
// 清空游戏纪录：删除玩家档案（qgarden_record.txt）与全部对局存档槽位，
// 把 rec 恢复为初始值，效果等同于"第一次打开游戏"（下次启动会重新弹出
// 用户协议、重新触发新手引导）。
// 说明：旧版本存档字段数量与 610 不同，loadRecords() 会判定为 legacy 而
// 整体拒读；此时用本函数清掉冲突的旧档案，即可从零重新开始。
void clearAllRecords();

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
