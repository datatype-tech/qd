// ============================================================
//  save.h  存档读写（qgarden_record.txt）
// ============================================================
#pragma once
#include "types.h"

extern bool saveWasLegacy;   // 读到旧版存档时置位

void loadRecords();
void saveRecords();
void resetAchievements();    // 重置成就与装扮（保留统计与星尘）

// ---------- 对局存档（qg_run.txt）：支持中途退出后"继续游戏/导入存档" ----------
bool hasSavedRun();          // 是否存在可继续的对局
void saveRun();              // 将当前对局状态写入存档
bool loadRun();              // 读取存档并恢复对局（成功返回 true，玩家进入 PLAY）
void clearRunSave();         // 删除对局存档（对局正常结束后调用）
