// ============================================================
//  audio.h  程序化音效系统（无需外部音频文件，运行期合成波形）
// ============================================================
#pragma once
#include "raylib.h"

extern Sound sfxClick, sfxPlant, sfxObserve, sfxCat, sfxFlower, sfxVine, sfxGrass,
      sfxRift, sfxBuy, sfxEnt, sfxStorm, sfxDecoh, sfxTide, sfxTunnel,
      sfxWin, sfxLose, sfxRecord, sfxAmbient, sfxMeditate, sfxIdle,
      sfxShopOpen, sfxItem, sfxShield, sfxAch, sfxUpg;
extern bool audioOK, muted;

void playSfx(Sound s);
void initSfx();
void updateAmbient();
void unloadSfx();
