// ============================================================
//  audio.cpp  程序化音效系统实现
// ============================================================
#include "audio.h"
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdlib>

static const int SR_ = 22050;
enum WForm { W_SIN, W_SQR, W_TRI, W_SAW, W_NOI };
struct Note { float start, dur, f0, f1, vol; WForm w; float atk, rel; };

Sound sfxClick, sfxPlant, sfxObserve, sfxCat, sfxFlower, sfxVine, sfxGrass,
      sfxRift, sfxBuy, sfxEnt, sfxStorm, sfxDecoh, sfxTide, sfxTunnel,
      sfxWin, sfxLose, sfxRecord, sfxAmbient, sfxMeditate, sfxIdle,
      sfxShopOpen, sfxItem, sfxShield, sfxAch, sfxUpg;
bool audioOK = false, muted = false;

static float oscf(WForm w, float ph) {
    switch (w) {
        case W_SIN: return sinf(ph * 2 * PI);
        case W_SQR: return ph < 0.5f ? 1.0f : -1.0f;
        case W_TRI: return 4.0f * fabsf(ph - 0.5f) - 1.0f;
        case W_SAW: return 2.0f * ph - 1.0f;
        default:    return GetRandomValue(-1000, 1000) / 1000.0f;
    }
}
static Sound buildSound(const std::vector<Note>& ns) {
    float total = 0.02f;
    for (const Note& n : ns) total = std::max(total, n.start + n.dur + 0.02f);
    int frames = (int)(total * SR_);
    std::vector<short> buf(frames, 0);
    for (const Note& n : ns) {
        int s0 = (int)(n.start * SR_), len = std::max(1, (int)(n.dur * SR_));
        float ph = 0;
        for (int i = 0; i < len; ++i) {
            int idx = s0 + i;
            if (idx < 0 || idx >= frames) continue;
            float t = i / (float)len;
            float f = n.f0 + (n.f1 - n.f0) * t;
            ph += f / SR_; if (ph >= 1.0f) ph -= (int)ph;
            float env = 1.0f;
            if (n.atk > 0 && t < n.atk)          env = t / n.atk;
            else if (n.rel > 0 && t > 1 - n.rel) env = (1 - t) / n.rel;
            int mixed = buf[idx] + (int)(oscf(n.w, ph) * env * n.vol * 11000);
            buf[idx] = (short)std::clamp(mixed, -32000, 32000);
        }
    }
    Wave w = { 0 };
    w.frameCount = frames;      // raylib 3.x 请改为 w.sampleCount = frames;
    w.sampleRate = SR_;
    w.sampleSize = 16;
    w.channels = 1;
    w.data = malloc(frames * sizeof(short));
    memcpy(w.data, buf.data(), frames * sizeof(short));
    Sound s = LoadSoundFromWave(w);
    UnloadWave(w);
    return s;
}

void playSfx(Sound s) { if (audioOK && !muted) PlaySound(s); }

void initSfx() {
    InitAudioDevice();
    if (!IsAudioDeviceReady()) return;
    audioOK = true;
    sfxClick = buildSound({ {0,0.11f,470,380,0.18f,W_SIN,0.32f,0.60f},
                            {0,0.11f,940,760,0.05f,W_SIN,0.32f,0.60f} });
    sfxMeditate = buildSound({ {0.00f,1.00f,396,396,0.15f,W_SIN,0.38f,0.50f},
                               {0.12f,0.92f,528,528,0.12f,W_SIN,0.38f,0.50f},
                               {0.24f,0.84f,660,660,0.09f,W_SIN,0.40f,0.50f},
                               {0.36f,0.70f,792,792,0.05f,W_SIN,0.45f,0.55f} });
    sfxIdle = buildSound({ {0,0.55f,330,300,0.10f,W_SIN,0.45f,0.5f} });
    sfxPlant = buildSound({ {0,0.16f,320,520,0.34f,W_TRI,0.05f,0.6f},
                            {0,0.10f,120, 60,0.22f,W_NOI,0.03f,0.9f} });
    sfxObserve = buildSound({ {0,0.22f,240,1000,0.26f,W_SIN,0.08f,0.5f},
                              {0.05f,0.18f,300,1200,0.13f,W_SIN,0.12f,0.6f} });
    sfxCat = buildSound({ {0.00f,0.13f, 523, 523,0.42f,W_TRI,0.03f,0.4f},
                          {0.10f,0.13f, 659, 659,0.42f,W_TRI,0.03f,0.4f},
                          {0.20f,0.13f, 784, 784,0.42f,W_TRI,0.03f,0.4f},
                          {0.30f,0.30f,1047,1047,0.42f,W_TRI,0.03f,0.6f},
                          {0.30f,0.50f,1568,2093,0.16f,W_SIN,0.08f,0.7f} });
    sfxFlower = buildSound({ {0.00f,0.12f,660, 880,0.34f,W_SIN,0.05f,0.5f},
                             {0.10f,0.22f,880,1320,0.30f,W_SIN,0.05f,0.6f} });
    sfxVine = buildSound({ {0.00f,0.25f,180,520,0.26f,W_TRI,0.06f,0.6f},
                           {0.10f,0.20f,240,700,0.15f,W_SIN,0.10f,0.7f} });
    sfxGrass = buildSound({ {0,0.18f,400,200,0.16f,W_NOI,0.05f,0.8f},
                            {0,0.16f,180,120,0.16f,W_TRI,0.10f,0.7f} });
    sfxRift = buildSound({ {0,0.45f,260, 50,0.44f,W_SQR,0.02f,0.5f},
                           {0,0.35f,900,100,0.28f,W_NOI,0.04f,0.7f} });
    sfxShield = buildSound({ {0,0.30f,900,500,0.28f,W_SIN,0.06f,0.6f},
                             {0.05f,0.25f,450,700,0.18f,W_TRI,0.10f,0.6f} });
    sfxBuy = buildSound({ {0.00f,0.09f, 660, 660,0.26f,W_TRI,0.06f,0.5f},
                          {0.08f,0.09f, 880, 880,0.26f,W_TRI,0.06f,0.5f},
                          {0.16f,0.18f,1175,1175,0.26f,W_TRI,0.06f,0.6f} });
    sfxItem = buildSound({ {0.00f,0.10f, 880, 880,0.24f,W_SIN,0.08f,0.5f},
                           {0.09f,0.16f,1320,1560,0.20f,W_SIN,0.08f,0.6f} });
    sfxShopOpen = buildSound({ {0.00f,0.16f,300,620,0.22f,W_TRI,0.12f,0.5f},
                               {0.10f,0.20f,620,930,0.14f,W_SIN,0.12f,0.6f} });
    sfxEnt = buildSound({ {0.00f,0.45f, 500, 500,0.26f,W_SIN,0.10f,0.5f},
                          {0.00f,0.45f, 507, 507,0.26f,W_SIN,0.10f,0.5f},
                          {0.15f,0.30f,1000,1400,0.13f,W_SIN,0.14f,0.6f} });
    sfxStorm = buildSound({ {0,0.60f,600,80,0.38f,W_NOI,0.08f,0.6f},
                            {0,0.50f,140,45,0.28f,W_SQR,0.08f,0.6f} });
    sfxDecoh = buildSound({ {0,0.40f, 900,300,0.18f,W_SIN,0.10f,0.8f},
                            {0,0.35f,1200,400,0.10f,W_NOI,0.15f,0.8f} });
    sfxTide = buildSound({ {0.00f,0.30f,520, 780,0.28f,W_TRI,0.08f,0.6f},
                           {0.12f,0.32f,780,1040,0.24f,W_TRI,0.08f,0.7f},
                           {0.26f,0.34f,1040,1300,0.16f,W_SIN,0.10f,0.7f} });
    sfxTunnel = buildSound({ {0.00f,0.28f,1200,300,0.26f,W_SIN,0.05f,0.7f},
                             {0.05f,0.20f, 300,900,0.16f,W_TRI,0.08f,0.6f} });
    sfxWin = buildSound({ {0.00f,0.18f, 523, 523,0.42f,W_TRI,0.03f,0.4f},
                          {0.15f,0.18f, 659, 659,0.42f,W_TRI,0.03f,0.4f},
                          {0.30f,0.18f, 784, 784,0.42f,W_TRI,0.03f,0.4f},
                          {0.45f,0.45f,1047,1047,0.42f,W_TRI,0.03f,0.5f},
                          {0.60f,0.50f,1568,1568,0.18f,W_SIN,0.12f,0.7f} });
    sfxLose = buildSound({ {0.00f,0.25f,440,440,0.36f,W_TRI,0.05f,0.4f},
                           {0.20f,0.25f,392,392,0.36f,W_TRI,0.05f,0.4f},
                           {0.40f,0.25f,330,330,0.36f,W_TRI,0.05f,0.4f},
                           {0.60f,0.55f,247,220,0.36f,W_TRI,0.05f,0.6f} });
    sfxRecord = buildSound({ {0.00f,0.10f,1047,1047,0.30f,W_SIN,0.05f,0.5f},
                             {0.08f,0.10f,1319,1319,0.30f,W_SIN,0.05f,0.5f},
                             {0.16f,0.10f,1568,1568,0.30f,W_SIN,0.05f,0.5f},
                             {0.24f,0.30f,2093,2093,0.30f,W_SIN,0.05f,0.6f} });
    sfxAch = buildSound({ {0.00f,0.12f, 784, 784,0.32f,W_TRI,0.05f,0.4f},
                          {0.10f,0.12f,1047,1047,0.32f,W_TRI,0.05f,0.4f},
                          {0.20f,0.34f,1319,1568,0.30f,W_SIN,0.06f,0.6f} });
    sfxUpg = buildSound({ {0.00f,0.14f,392,588,0.30f,W_TRI,0.06f,0.5f},
                          {0.12f,0.26f,588,880,0.26f,W_SIN,0.08f,0.6f} });
    sfxAmbient = buildSound({ {0,4.0f, 55.0f, 55.0f,0.22f,W_SIN,0.02f,0.02f},
                              {0,4.0f, 82.5f, 82.5f,0.14f,W_SIN,0.02f,0.02f},
                              {0,4.0f,110.0f,110.0f,0.09f,W_TRI,0.02f,0.02f},
                              {1.0f,2.0f,660,662,0.05f,W_SIN,0.5f,0.5f} });
    SetSoundVolume(sfxAmbient, 0.32f);
}
void updateAmbient() {
    if (!audioOK) return;
    if (muted) { if (IsSoundPlaying(sfxAmbient)) StopSound(sfxAmbient); return; }
    if (!IsSoundPlaying(sfxAmbient)) PlaySound(sfxAmbient);
}
void unloadSfx() {
    if (!audioOK) return;
    Sound all[] = { sfxClick,sfxPlant,sfxObserve,sfxCat,sfxFlower,sfxVine,sfxGrass,
                    sfxRift,sfxBuy,sfxEnt,sfxStorm,sfxDecoh,sfxTide,sfxTunnel,
                    sfxWin,sfxLose,sfxRecord,sfxAmbient,sfxMeditate,sfxIdle,
                    sfxShopOpen,sfxItem,sfxShield,sfxAch,sfxUpg };
    for (Sound s : all) UnloadSound(s);
    CloseAudioDevice();
}
