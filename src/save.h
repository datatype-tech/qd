// ============================================================
//  save.h  存档读写（qgarden_record.txt）
// ============================================================
#pragma once
#include "types.h"

extern bool saveWasLegacy;   // 读到旧版存档时置位

void loadRecords();
void saveRecords();
void resetAchievements();    // 重置成就与装扮（保留统计与星尘）
