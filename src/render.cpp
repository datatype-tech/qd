// ============================================================
//  render.cpp  植物 / 商品图标 / 背景 / 特效 / 奖章绘制实现
// ============================================================
#include "render.h"
#include "state.h"
#include "data.h"
#include "util.h"
#include <cmath>
#include <algorithm>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

// ============================================================
//  植物绘制
//  设计原则：成熟度（0..3）绝不用数字表达，而是让造型本身说话。
//  每款种子装扮都有专属的"成长语言"：
//    默认 幼苗   —— 破土 → 抽茎 → 展叶 → 顶端结晶花苞
//    水晶       —— 浑浊原石 → 棱面显现 → 内部光核 → 悬浮棱锥完全折射
//    黄金       —— 粗胚 → 锤打成型 → 浮雕纹路 → 镶宝石且光泽流转
//    永劫       —— 沙漏：下漏斗积沙渐满，指针环逐格点亮，满时循环回流
//    猫瞳       —— 猫眼：紧闭一线 → 微睁 → 半开 → 完全睁开且瞳孔收缩
// ============================================================

// 成熟度进度环：所有造型共用的底部刻度（弧线而非数字）
static void drawMatArc(Vector2 ct, int mat, Color on, Color off) {
    const float R = 33.0f;
    // 底盘：四段弧，已达成的段落点亮
    for (int i = 0; i < 3; ++i) {
        float a0 = 118.0f + i * 38.0f, a1 = a0 + 30.0f;
        DrawRing(ct, R, R + 3.2f, a0, a1, 14, i < mat ? on : off);
    }
    // 圆满时补一枚闭合光点，表示"再无成长空间"
    if (mat >= 3) {
        Vector2 p = { ct.x + cosf(232 * DEG2RAD) * (R + 1.6f),
                      ct.y + sinf(232 * DEG2RAD) * (R + 1.6f) };
        DrawCircleV(p, 3.0f, on);
    }
}

// ---------- 造型一：水晶种子 ----------
// 成熟度 = 棱面的完整度与内部光核的强度
static void seedCrystal(Vector2 ct, int mat, float t, const SkinDef& s) {
    float pul = 0.5f + 0.5f * sinf(t * 2.4f);
    float lift = (mat >= 3) ? 4.0f + sinf(t * 1.8f) * 2.5f : 0.0f;   // 圆满时悬浮
    Vector2 c = { ct.x, ct.y - lift };

    DrawCircleGradient((int)c.x, (int)c.y, 30 + 8 * pul * (0.4f + mat * 0.2f),
                       Fade(s.a, 0.10f + mat * 0.05f), Fade(s.a, 0));
    // 底部岩座：随成熟逐渐"脱离"母岩
    DrawEllipse((int)ct.x, (int)(ct.y + 27), 24 - mat * 3.0f, 7, Color{ 48,56,70,255 });

    if (mat == 0) {
        // 未开面的浑浊原石：不规则轮廓、无光核
        DrawPoly(c, 7, 19, 12, Color{ 96,116,134,255 });
        DrawPoly(c, 7, 13, -20, Color{ 116,138,158,255 });
        DrawPolyLines(c, 7, 19, 12, Fade(s.a, 0.35f));
        return;
    }
    // 主棱锥：面数随成熟增加（4 → 6 → 8），越成熟越"规整通透"
    int sides = mat == 1 ? 4 : mat == 2 ? 6 : 8;
    float R = 17.0f + mat * 2.2f;
    float rot = t * (8 + mat * 6);

    // 外层折射光晕
    DrawPoly(c, sides, R + 3.0f, rot, Fade(s.a, 0.16f + 0.10f * pul));
    // 主体
    DrawPoly(c, sides, R, rot, Fade(s.a, 0.55f + mat * 0.10f));
    DrawPolyLines(c, sides, R, rot, Fade(s.b, 0.85f));
    // 棱线：从中心放射，条数随成熟增加
    for (int i = 0; i < sides; ++i) {
        float a = rot * DEG2RAD + i * 2 * PI / sides;
        DrawLineEx(c, { c.x + cosf(a) * R, c.y + sinf(a) * R }, 1.1f, Fade(s.b, 0.30f));
    }
    // 内部光核：成熟度越高越亮越大
    if (mat >= 2) {
        DrawCircleGradient((int)c.x, (int)c.y, 9 + mat * 2.0f + pul * 2,
                           Fade(s.b, 0.55f), Fade(s.b, 0));
        DrawPoly(c, sides, 6.0f + mat, -rot * 1.6f, Fade(RAYWHITE, 0.75f));
    }
    // 圆满：四向星芒 + 折射亮点
    if (mat >= 3) {
        for (int i = 0; i < 4; ++i) {
            float a = t * 0.8f + i * PI / 2;
            float L = 15 + 6 * pul;
            DrawLineEx({ c.x + cosf(a) * 8, c.y + sinf(a) * 8 },
                       { c.x + cosf(a) * L, c.y + sinf(a) * L }, 1.6f, Fade(RAYWHITE, 0.55f));
        }
        DrawCircleV({ c.x - R * 0.34f, c.y - R * 0.40f }, 2.2f, Fade(RAYWHITE, 0.9f));
    }
}

// ---------- 造型二：黄金种子 ----------
// 成熟度 = 锻造完成度：粗胚 → 成型 → 浮雕 → 镶宝石
static void seedGold(Vector2 ct, int mat, float t, const SkinDef& s) {
    float pul = 0.5f + 0.5f * sinf(t * 2.0f);
    DrawCircleGradient((int)ct.x, (int)ct.y, 30 + 6 * pul, Fade(s.a, 0.14f), Fade(s.a, 0));
    DrawEllipse((int)ct.x, (int)(ct.y + 27), 25, 7, Color{ 62,50,28,255 });

    Color dark = Color{ 158,112,26,255 };
    if (mat == 0) {
        // 粗胚：坑洼的金块，边缘不平整
        DrawCircleV({ ct.x, ct.y + 2 }, 18, dark);
        DrawCircleV({ ct.x - 6, ct.y - 3 }, 8, Color{ 186,138,44,255 });
        DrawCircleV({ ct.x + 7, ct.y + 6 }, 6, Color{ 138,96,20,255 });
        DrawCircleV({ ct.x - 4, ct.y - 6 }, 2.6f, Fade(RAYWHITE, 0.35f));
        return;
    }
    // 成型的金果：随成熟更饱满、更规整
    float R = 16.0f + mat * 2.0f;
    DrawCircleV({ ct.x, ct.y + 1 }, R + 1.5f, dark);
    DrawCircleV({ ct.x, ct.y }, R, Fade(s.a, 0.95f));
    // 上方高光弧：金属光泽
    DrawRing({ ct.x, ct.y }, R * 0.52f, R * 0.80f, 195, 315, 24, Fade(s.b, 0.55f));
    DrawCircleV({ ct.x - R * 0.34f, ct.y - R * 0.36f }, R * 0.20f, Fade(RAYWHITE, 0.70f));

    // 浮雕纹路：mat>=2 起出现同心錾刻
    if (mat >= 2) {
        for (int i = 1; i <= 2; ++i)
            DrawCircleLines((int)ct.x, (int)ct.y, R * (0.40f + i * 0.22f), Fade(dark, 0.75f));
        // 四向錾点
        for (int i = 0; i < 4; ++i) {
            float a = i * PI / 2 + 0.4f;
            DrawCircleV({ ct.x + cosf(a) * R * 0.62f, ct.y + sinf(a) * R * 0.62f }, 1.8f,
                        Fade(dark, 0.9f));
        }
    }
    // 圆满：顶部镶一枚宝石，四周金光流转
    if (mat >= 3) {
        Vector2 g = { ct.x, ct.y - R * 0.20f };
        DrawPoly(g, 6, 6.4f, t * 30, Color{ 255,120,150,255 });
        DrawPolyLines(g, 6, 6.4f, t * 30, Fade(RAYWHITE, 0.85f));
        DrawCircleV({ g.x - 1.6f, g.y - 1.8f }, 1.5f, Fade(RAYWHITE, 0.9f));
        for (int i = 0; i < 3; ++i) {
            float a = t * 1.5f + i * 2.09f;
            DrawCircleV({ ct.x + cosf(a) * (R + 8), ct.y + sinf(a) * (R + 8) }, 2.0f,
                        Fade(s.b, 0.85f));
        }
    }
}

// ---------- 造型三：永劫之种（沙漏） ----------
// 成熟度 = 下半积沙量 + 指针环点亮格数；圆满时沙流回溯，象征循环
static void seedEternal(Vector2 ct, int mat, float t, const SkinDef& s) {
    float f = mat / 3.0f;
    DrawCircleGradient((int)ct.x, (int)ct.y, 30, Fade(s.a, 0.14f), Fade(s.a, 0));

    const float HW = 15.0f, HH = 19.0f;    // 沙漏半宽 / 半高
    Vector2 tl = { ct.x - HW, ct.y - HH }, tr = { ct.x + HW, ct.y - HH };
    Vector2 bl = { ct.x - HW, ct.y + HH }, br = { ct.x + HW, ct.y + HH };
    Vector2 nk = { ct.x, ct.y };           // 颈部

    // 玻璃外壳（上下两个三角）
    DrawTriangle(tl, nk, tr, Fade(s.a, 0.13f));
    DrawTriangle(bl, br, nk, Fade(s.a, 0.13f));

    // 上半余沙：随成熟减少（0 级满仓，3 级见底）
    float up = 1.0f - f;
    if (up > 0.02f) {
        float w = HW * up, h = HH * up;
        DrawTriangle({ ct.x - w, ct.y - h }, nk, { ct.x + w, ct.y - h }, Fade(s.a, 0.92f));
        // 沙面高光线，强化"液面"的存在感
        DrawLineEx({ ct.x - w, ct.y - h }, { ct.x + w, ct.y - h }, 2.0f,
                   Fade(Color{ 255,255,255,255 }, 0.45f));
    }
    // 下半积沙：随成熟增多，堆成锥形
    if (f > 0.02f) {
        DrawTriangle({ ct.x - HW * f, ct.y + HH }, { ct.x + HW * f, ct.y + HH }, nk,
                     Fade(s.a, 0.95f));
        DrawLineEx({ ct.x - HW * f, ct.y + HH }, { ct.x + HW * f, ct.y + HH }, 2.2f,
                   Fade(Color{ 255,255,255,255 }, 0.40f));
    }
    // 流沙细线：未圆满时向下落，圆满时向上回流（时间循环）
    bool loop = (mat >= 3);
    float ph = fmodf(t * 1.6f, 1.0f);
    for (int i = 0; i < 3; ++i) {
        float p = fmodf(ph + i * 0.33f, 1.0f);
        float yy = loop ? (ct.y + HH * 0.8f - p * HH * 1.6f) : (ct.y + p * HH * 0.9f);
        DrawCircleV({ ct.x + sinf(p * 9 + i) * 1.6f, yy }, 1.5f,
                    Fade(loop ? s.b : s.a, 0.9f * (1 - p * 0.4f)));
    }
    // 玻璃框线
    DrawLineEx(tl, tr, 2.2f, Fade(s.b, 0.8f));
    DrawLineEx(bl, br, 2.2f, Fade(s.b, 0.8f));
    DrawLineEx(tl, nk, 1.6f, Fade(s.b, 0.55f));
    DrawLineEx(tr, nk, 1.6f, Fade(s.b, 0.55f));
    DrawLineEx(bl, nk, 1.6f, Fade(s.b, 0.55f));
    DrawLineEx(br, nk, 1.6f, Fade(s.b, 0.55f));

    // 指针环：三格刻度，成熟一级点亮一格
    for (int i = 0; i < 3; ++i) {
        float a0 = -80.0f + i * 26.0f;
        DrawRing(ct, 23, 25.6f, a0, a0 + 20, 12,
                 i < mat ? Fade(s.b, 0.95f) : Fade(RAYWHITE, 0.14f));
    }
    // 圆满：外圈完整回环
    if (loop) {
        float a = t * 90;
        DrawRing(ct, 26.5f, 28.5f, a, a + 300, 40, Fade(s.b, 0.55f));
        DrawCircleV({ ct.x + cosf(a * DEG2RAD) * 27.5f, ct.y + sinf(a * DEG2RAD) * 27.5f },
                    2.4f, Fade(RAYWHITE, 0.9f));
    }
}

// ---------- 造型四：猫神之瞳 ----------
// 成熟度 = 眼睛睁开程度：紧闭一线 → 微睁 → 半开 → 完全睁开（瞳孔收成竖缝）
static void seedCatEye(Vector2 ct, int mat, float t, const SkinDef& s) {
    // open: 0 = 完全闭合，1 = 完全睁开。加一点呼吸感，让眼睛"活着"
    float base[4] = { 0.06f, 0.34f, 0.64f, 1.00f };
    float open = base[mat < 0 ? 0 : mat > 3 ? 3 : mat];
    // 眨眼：仅在睁开状态偶尔快速闭合一次
    float blink = 1.0f;
    if (mat >= 1) {
        float cyc = fmodf(t * 0.42f + ct.x * 0.011f, 1.0f);
        if (cyc > 0.955f) blink = 1.0f - sinf((cyc - 0.955f) / 0.045f * PI) * 0.88f;
    }
    open *= blink;
    open += 0.012f * sinf(t * 2.3f) * (mat >= 1 ? 1.0f : 0.0f);
    if (open < 0.03f) open = 0.03f;

    const float EW = 25.0f;              // 眼睛半宽
    float EH = 17.0f * open;             // 眼睛半高
    Color iris = s.a, glowC = s.b;       // a=虹膜暖色, b=瞳孔辉光色

    // 眼周辉光：越睁开越亮
    DrawCircleGradient((int)ct.x, (int)ct.y, 30 + 8 * open,
                       Fade(glowC, 0.10f + 0.16f * open), Fade(glowC, 0));

    // 眼窝暗底（始终存在，给出"眼睛"的位置感）
    DrawEllipse((int)ct.x, (int)ct.y, EW + 2, 18, Color{ 26,20,26,235 });

    if (open <= 0.10f) {
        // ── 紧闭：一道弧线加睫毛，看得出"闭着的眼" ──
        DrawRing(ct, EW - 2, EW, 188, 352, 28, Fade(s.b, 0.55f));
        DrawLineEx({ ct.x - EW, ct.y + 1 }, { ct.x + EW, ct.y + 1 }, 2.6f,
                   Fade(Color{ 240,225,210,255 }, 0.75f));
        for (int i = 0; i < 3; ++i) {
            float px = ct.x - 12 + i * 12;
            DrawLineEx({ px, ct.y + 2 }, { px - 2, ct.y + 7 }, 1.8f, Fade(s.b, 0.45f));
        }
    } else {
        // ── 睁开：杏仁形眼白 + 虹膜 + 竖缝瞳孔 ──
        // 眼白（巩膜）
        DrawEllipse((int)ct.x, (int)ct.y, EW, EH, Color{ 248,242,232,255 });
        // 虹膜：占据大部分眼形
        float ir = EH * 0.98f;
        float irW = std::min(EW * 0.72f, 15.0f);
        DrawEllipse((int)ct.x, (int)ct.y, irW, ir, Fade(iris, 0.95f));
        // 虹膜纹理：放射细纹
        for (int i = 0; i < 10; ++i) {
            float a = i * PI / 5 + t * 0.20f;
            DrawLineEx({ ct.x + cosf(a) * irW * 0.30f, ct.y + sinf(a) * ir * 0.30f },
                       { ct.x + cosf(a) * irW * 0.92f, ct.y + sinf(a) * ir * 0.92f },
                       1.0f, Fade(Color{ 255,238,190,255 }, 0.28f));
        }
        // 瞳孔：猫科的竖椭圆。越成熟越"聚焦"（变窄变长）
        float pw = irW * (0.46f - 0.16f * open);
        float phh = ir * (0.80f + 0.16f * open);
        DrawEllipse((int)ct.x, (int)ct.y, std::max(1.6f, pw), phh, Color{ 18,16,22,255 });
        // 高光：让眼睛显得湿润
        DrawCircleV({ ct.x - irW * 0.34f, ct.y - ir * 0.34f }, 2.4f + 1.2f * open,
                    Fade(RAYWHITE, 0.85f));
        DrawCircleV({ ct.x + irW * 0.26f, ct.y + ir * 0.30f }, 1.4f, Fade(RAYWHITE, 0.40f));
        // 上眼睑投影与眼线
        DrawRing(ct, EW - 1.5f, EW + 0.5f, 182, 358, 30, Fade(Color{ 60,40,44,255 }, 0.55f));
        DrawLineEx({ ct.x - EW, ct.y - EH * 0.86f }, { ct.x + EW, ct.y - EH * 0.86f },
                   1.6f, Fade(Color{ 90,60,60,255 }, 0.35f));
    }

    // 完全睁开时：竖起的猫耳与胡须，点明"猫神"身份
    if (mat >= 3) {
        Color fur = Fade(s.a, 0.9f);
        for (int k = 0; k < 2; ++k) {
            float dir = k ? 1.0f : -1.0f;
            Vector2 e = { ct.x + dir * 15, ct.y - 20 };
            DrawTriangle(k ? Vector2{ e.x - 7, e.y + 7 } : Vector2{ e.x + dir * 7, e.y + 7 },
                         Vector2{ e.x + dir * 1.0f, e.y - 8 },
                         k ? Vector2{ e.x + 7, e.y + 7 } : Vector2{ e.x - dir * 7, e.y + 7 },
                         fur);
        }
        for (int k = 0; k < 2; ++k) {
            float dir = k ? 1.0f : -1.0f;
            float sw = sinf(t * 2.2f + k) * 1.8f;
            DrawLineEx({ ct.x + dir * (EW - 3), ct.y + 6 },
                       { ct.x + dir * (EW + 11), ct.y + 3 + sw }, 1.3f, Fade(RAYWHITE, 0.45f));
            DrawLineEx({ ct.x + dir * (EW - 3), ct.y + 9 },
                       { ct.x + dir * (EW + 10), ct.y + 12 + sw }, 1.3f, Fade(RAYWHITE, 0.35f));
        }
    }
}

// ---------- 默认造型：幼苗 ----------
// 成熟度 = 破土 → 抽茎 → 展叶 → 顶端结晶花苞
static void seedSprout(Vector2 ct, int mat, float t) {
    Color mc = mat >= 3 ? Color{ 130,255,160,255 } : mat >= 2 ? Color{ 205,255,130,255 }
             : mat >= 1 ? Color{ 255,215,120,255 } : Color{ 200,212,235,255 };
    float pul = 0.5f + 0.5f * sinf(t * 3.0f + ct.x * 0.03f);
    DrawCircleGradient((int)ct.x, (int)ct.y, 34 + 5 * pul, Fade(mc, 0.16f), Fade(mc, 0.0f));
    DrawEllipse((int)ct.x, (int)(ct.y + 26), 28, 9, Color{ 58,45,38,255 });
    DrawEllipse((int)ct.x, (int)(ct.y + 24), 20, 6, Color{ 76,58,46,255 });
    if (mat >= 1)
        DrawLineEx({ ct.x,ct.y + 16 }, { ct.x,ct.y + 16 - (10.0f + mat * 7) }, 3.5f,
                   Color{ 96,190,110,255 });
    if (mat >= 2) {
        float sw = sinf(t * 2) * 3;
        DrawEllipse((int)(ct.x - 13 + sw), (int)(ct.y + 2), 11, 6, Color{ 110,215,125,255 });
        DrawEllipse((int)(ct.x + 13 - sw), (int)(ct.y - 4), 11, 6, Color{ 96,195,115,255 });
    }
    if (mat >= 3) {
        DrawCircleGradient((int)ct.x, (int)(ct.y - 16), 16, Fade(mc, 0.5f), Fade(mc, 0));
        DrawPoly({ ct.x,ct.y - 16 }, 6, 9 + pul, t * 25, mc);
        DrawPoly({ ct.x,ct.y - 16 }, 6, 5, -t * 25, RAYWHITE);
    } else {
        // 未抽茎前是一枚埋在土里的种子：用裂纹表示即将破土，而不是问号数字
        DrawEllipse((int)ct.x, (int)(ct.y + 10), 13, 16, Color{ 96,70,50,255 });
        DrawEllipse((int)(ct.x - 4), (int)(ct.y + 5), 5, 7, Color{ 142,108,76,255 });
        if (mat == 0) {
            DrawLineEx({ ct.x - 4, ct.y + 2 }, { ct.x + 2, ct.y + 9 }, 1.4f, Fade(mc, 0.8f));
            DrawLineEx({ ct.x + 2, ct.y + 9 }, { ct.x - 1, ct.y + 16 }, 1.4f, Fade(mc, 0.6f));
        }
    }
    for (int k = 0; k < 2; ++k) {
        float a = t * 2.2f + k * PI;
        DrawCircleV({ ct.x + cosf(a) * 30,ct.y - 6 + sinf(a) * 11 }, 3.0f, Fade(mc, 0.95f));
    }
    drawMatArc(ct, mat, Fade(mc, 0.95f), Fade(RAYWHITE, 0.14f));
}

void drawSeed(Vector2 ct, int mat, float t) {
    if (mat < 0) mat = 0; if (mat > 3) mat = 3;
    if (rec.seedSkin >= 0 && rec.seedSkin < SKIN_COUNT && SKINS[rec.seedSkin].type == 1) {
        const SkinDef& s = SKINS[rec.seedSkin];
        switch (s.style) {
        case SA_CRYSTAL: seedCrystal(ct, mat, t, s); break;
        case SA_GOLD:    seedGold(ct, mat, t, s);    break;
        case SA_ETERNAL: seedEternal(ct, mat, t, s); break;
        case SA_CATEYE:  seedCatEye(ct, mat, t, s);  break;
        default:         seedSprout(ct, mat, t);     return;
        }
        // 猫瞳自带"睁眼程度"作为刻度，再加进度环会喧宾夺主，故跳过
        if (s.style != SA_CATEYE)
            drawMatArc(ct, mat, Fade(s.b, 0.95f), Fade(RAYWHITE, 0.14f));
        return;
    }
    seedSprout(ct, mat, t);
}
void drawEnt(Vector2 ct, float t) {
    DrawCircleGradient((int)ct.x, (int)ct.y, 34, Fade(VIOLET, 0.28f), Fade(VIOLET, 0));
    DrawRing(ct, 17, 20, t * 70, t * 70 + 260, 36, Fade(VIOLET, 0.9f));
    DrawRing(ct, 23, 25, -t * 50, -t * 50 + 160, 36, Fade(PURPLE, 0.65f));
    DrawCircleV(ct, 11, Color{ 186,124,255,255 });
    DrawCircleV(ct, 5, RAYWHITE);
}
// ============================================================
//  花朵绘制
//  花期（life）同样不用数字：用底部花瓣状刻度表示剩余回合
// ============================================================

// 花期刻度：剩余回合数用小花瓣表示，谢掉的变成空心
static void drawLifePetals(Vector2 ct, int life, Color on) {
    for (int i = 0; i < 3; ++i) {
        Vector2 p = { ct.x - 10 + i * 10, ct.y + 38 };
        if (i < life) {
            DrawCircleV(p, 3.6f, on);
            DrawCircleV({ p.x - 0.8f, p.y - 0.8f }, 1.5f, Fade(RAYWHITE, 0.55f));
        } else {
            DrawCircleLines((int)p.x, (int)p.y, 3.4f, Fade(RAYWHITE, 0.22f));
        }
    }
}

// ---------- 星辉花：花瓣是绕轴公转的星辰 ----------
static void flowerStarlight(Vector2 ct, int life, float t, const SkinDef& s) {
    DrawCircleGradient((int)ct.x, (int)ct.y, 42, Fade(s.a, 0.22f), Fade(s.a, 0));
    // 轨道
    DrawRing(ct, 17.5f, 18.5f, 0, 360, 40, Fade(s.a, 0.16f));
    for (int p = 0; p < 5; ++p) {
        float a = (t * 42 + p * 72) * DEG2RAD;
        Vector2 pp = { ct.x + cosf(a) * 18, ct.y + sinf(a) * 18 * 0.86f };
        // 拖尾
        for (int k = 1; k <= 3; ++k) {
            float a2 = a - k * 0.16f;
            DrawCircleV({ ct.x + cosf(a2) * 18, ct.y + sinf(a2) * 18 * 0.86f },
                        2.2f - k * 0.5f, Fade(s.a, 0.30f / k));
        }
        // 四角星
        DrawPoly(pp, 4, 7.0f, t * 60 + p * 30, Fade(s.a, 0.95f));
        DrawPoly(pp, 4, 3.2f, -t * 60, Fade(RAYWHITE, 0.9f));
    }
    // 中心恒星
    DrawCircleGradient((int)ct.x, (int)ct.y, 12, Fade(s.b, 0.7f), Fade(s.b, 0));
    DrawCircleV(ct, 6.0f, Color{ 255,250,235,255 });
    for (int i = 0; i < 4; ++i) {
        float a = t * 0.9f + i * PI / 2;
        DrawLineEx({ ct.x + cosf(a) * 7, ct.y + sinf(a) * 7 },
                   { ct.x + cosf(a) * 13, ct.y + sinf(a) * 13 }, 1.5f, Fade(RAYWHITE, 0.6f));
    }
    drawLifePetals(ct, life, s.a);
}

// ---------- 宇宙之花：一个正在绽放的星系（双旋臂 + 核球） ----------
static void flowerGalaxy(Vector2 ct, int life, float t, const SkinDef& s) {
    DrawCircleGradient((int)ct.x, (int)ct.y, 46, Fade(s.a, 0.20f), Fade(s.a, 0));
    // 双旋臂：每条臂由渐变粒子构成，整体缓慢自转
    for (int arm = 0; arm < 2; ++arm) {
        for (int i = 0; i < 26; ++i) {
            float f = i / 26.0f;
            float ang = f * 3.4f + t * 0.55f + arm * PI;
            float rr = 4 + f * 21;
            Vector2 p = { ct.x + cosf(ang) * rr, ct.y + sinf(ang) * rr * 0.55f };
            Color c = arm ? s.b : s.a;
            DrawCircleV(p, 2.6f * (1 - f * 0.55f), Fade(c, 0.85f * (1 - f * 0.45f)));
        }
    }
    // 核球
    DrawCircleGradient((int)ct.x, (int)ct.y, 14, Fade(Color{ 255,240,200,255 }, 0.85f),
                       Fade(s.a, 0));
    DrawCircleV(ct, 5.5f, Color{ 255,252,240,255 });
    // 外缘尘环
    DrawRing(ct, 24, 25.5f, t * 12, t * 12 + 300, 40, Fade(s.b, 0.28f));
    drawLifePetals(ct, life, s.a);
}

// ---------- 完美之花：几何纯净的结晶花，绝对对称 ----------
static void flowerPerfect(Vector2 ct, int life, float t, const SkinDef& s) {
    DrawCircleGradient((int)ct.x, (int)ct.y, 40, Fade(s.a, 0.20f), Fade(s.a, 0));
    float breathe = 0.5f + 0.5f * sinf(t * 1.6f);
    // 六枚菱形花瓣，严格对称
    for (int p = 0; p < 6; ++p) {
        float a = (p * 60 + t * 10) * DEG2RAD;
        Vector2 pp = { ct.x + cosf(a) * 16, ct.y + sinf(a) * 16 };
        DrawPoly(pp, 4, 9.5f, p * 60 + t * 10, Fade(s.a, 0.55f));
        DrawPolyLines(pp, 4, 9.5f, p * 60 + t * 10, Fade(s.b, 0.9f));
    }
    // 内层反向六边形
    DrawPolyLines(ct, 6, 12 + breathe, -t * 14, Fade(s.b, 0.7f));
    DrawPolyLines(ct, 6, 20 + breathe * 1.5f, t * 14, Fade(s.a, 0.45f));
    // 中心：无瑕的白核
    DrawCircleV(ct, 5.5f, Color{ 250,255,253,255 });
    DrawCircleLines((int)ct.x, (int)ct.y, 8.5f, Fade(s.b, 0.8f));
    // 四向纯净星芒
    for (int i = 0; i < 4; ++i) {
        float a = i * PI / 2 + PI / 4;
        DrawLineEx({ ct.x + cosf(a) * 22, ct.y + sinf(a) * 22 },
                   { ct.x + cosf(a) * (27 + breathe * 3), ct.y + sinf(a) * (27 + breathe * 3) },
                   1.4f, Fade(RAYWHITE, 0.45f));
    }
    drawLifePetals(ct, life, s.a);
}

// ---------- 芬芳之花：重瓣绽放，香气以可见波纹扩散 ----------
static void flowerFragrant(Vector2 ct, int life, float t, const SkinDef& s) {
    DrawCircleGradient((int)ct.x, (int)ct.y, 42, Fade(s.a, 0.22f), Fade(s.a, 0));
    // 香气波纹：三层向外扩散的环
    for (int i = 0; i < 3; ++i) {
        float f = fmodf(t * 0.42f + i / 3.0f, 1.0f);
        DrawRing(ct, 20 + f * 16, 21.4f + f * 16, 0, 360, 36,
                 Fade(s.b, 0.26f * (1 - f)));
    }
    // 外层大花瓣（8 枚）
    for (int p = 0; p < 8; ++p) {
        float a = (t * 14 + p * 45) * DEG2RAD;
        Vector2 pp = { ct.x + cosf(a) * 17, ct.y + sinf(a) * 17 };
        DrawCircleV(pp, 9.5f, Fade(s.a, 0.95f));
        DrawCircleV(pp, 5.5f, Fade(Color{ 255,205,225,255 }, 0.9f));
    }
    // 内层小花瓣（5 枚，反向偏转形成重瓣）
    for (int p = 0; p < 5; ++p) {
        float a = (-t * 22 + p * 72 + 20) * DEG2RAD;
        Vector2 pp = { ct.x + cosf(a) * 8.5f, ct.y + sinf(a) * 8.5f };
        DrawCircleV(pp, 6.0f, Fade(Color{ 255,178,206,255 }, 0.95f));
    }
    // 花心与花蕊
    DrawCircleV(ct, 5.0f, s.b);
    for (int i = 0; i < 6; ++i) {
        float a = i * PI / 3 + t * 0.7f;
        Vector2 e = { ct.x + cosf(a) * 7.5f, ct.y + sinf(a) * 7.5f };
        DrawLineEx(ct, e, 1.1f, Fade(Color{ 255,240,180,255 }, 0.75f));
        DrawCircleV(e, 1.7f, Color{ 255,248,205,255 });
    }
    drawLifePetals(ct, life, s.a);
}

// ---------- 三连之花：三张猫脸围成一朵 ----------
static void flowerCatTrio(Vector2 ct, int life, float t, const SkinDef& s) {
    DrawCircleGradient((int)ct.x, (int)ct.y, 42, Fade(s.a, 0.22f), Fade(s.a, 0));
    for (int k = 0; k < 3; ++k) {
        float a = (t * 26 + k * 120) * DEG2RAD;
        Vector2 c = { ct.x + cosf(a) * 14, ct.y + sinf(a) * 14 };
        float R = 9.0f;
        // 猫耳
        for (int e = 0; e < 2; ++e) {
            float dir = e ? 1.0f : -1.0f;
            Vector2 ep = { c.x + dir * R * 0.62f, c.y - R * 0.66f };
            DrawTriangle({ ep.x - R * 0.34f, ep.y + R * 0.40f },
                         { ep.x + dir * 0.5f, ep.y - R * 0.52f },
                         { ep.x + R * 0.34f, ep.y + R * 0.40f }, Fade(s.a, 0.95f));
        }
        // 脸
        DrawCircleV(c, R, Fade(s.a, 0.95f));
        DrawCircleV({ c.x, c.y + R * 0.16f }, R * 0.74f, Fade(Color{ 255,246,225,255 }, 0.92f));
        // 眼睛（竖瞳）与鼻子
        DrawEllipse((int)(c.x - R * 0.30f), (int)(c.y - R * 0.02f), 1.5f, 2.6f, Color{ 40,30,34,255 });
        DrawEllipse((int)(c.x + R * 0.30f), (int)(c.y - R * 0.02f), 1.5f, 2.6f, Color{ 40,30,34,255 });
        DrawCircleV({ c.x, c.y + R * 0.30f }, 1.5f, Color{ 236,138,158,255 });
        // 胡须
        for (int w = 0; w < 2; ++w) {
            float dir = w ? 1.0f : -1.0f;
            DrawLineEx({ c.x + dir * R * 0.42f, c.y + R * 0.34f },
                       { c.x + dir * R * 1.05f, c.y + R * 0.20f }, 0.9f, Fade(BLACK, 0.35f));
        }
    }
    // 中心：三连纪念金星
    DrawCircleGradient((int)ct.x, (int)ct.y, 11, Fade(s.b, 0.65f), Fade(s.b, 0));
    DrawPoly(ct, 3, 6.4f, -t * 34, s.b);
    DrawPoly(ct, 3, 6.4f, -t * 34 + 60, Fade(s.b, 0.75f));
    DrawCircleV(ct, 2.4f, RAYWHITE);
    drawLifePetals(ct, life, s.a);
}

// ---------- 默认造型：灵能花 ----------
static void flowerBlossom(Vector2 ct, int life, float t) {
    DrawCircleGradient((int)ct.x, (int)ct.y, 38, Fade(PINK, 0.20f), Fade(PINK, 0));
    DrawLineEx({ ct.x,ct.y + 30 }, { ct.x,ct.y + 6 }, 3.5f, Color{ 96,190,110,255 });
    DrawEllipse((int)(ct.x - 11), (int)(ct.y + 20), 9, 5, Color{ 110,215,125,255 });
    for (int p = 0; p < 6; ++p) {
        float a = (t * 22 + p * 60) * DEG2RAD;
        Vector2 pp = { ct.x + cosf(a) * 16,ct.y + sinf(a) * 16 - 4 };
        DrawCircleV(pp, 11, Color{ 244,132,186,255 });
        DrawCircleV(pp, 6.5f, Color{ 255,196,222,255 });
    }
    DrawCircleV({ ct.x,ct.y - 4 }, 9, GOLD);
    DrawCircleV({ ct.x,ct.y - 4 }, 4.5f, Color{ 255,247,205,255 });
    drawLifePetals(ct, life, PINK);
}

void drawFlower(Vector2 ct, int life, float t) {
    if (life < 0) life = 0;
    if (rec.flowerSkin >= 0 && rec.flowerSkin < SKIN_COUNT && SKINS[rec.flowerSkin].type == 2) {
        const SkinDef& s = SKINS[rec.flowerSkin];
        switch (s.style) {
        case FA_STARLIGHT: flowerStarlight(ct, life, t, s); return;
        case FA_GALAXY:    flowerGalaxy(ct, life, t, s);    return;
        case FA_PERFECT:   flowerPerfect(ct, life, t, s);   return;
        case FA_FRAGRANT:  flowerFragrant(ct, life, t, s);  return;
        case FA_CATTRIO:   flowerCatTrio(ct, life, t, s);   return;
        default: break;
        }
    }
    flowerBlossom(ct, life, t);
}

// ==================== 商品动态图标 ====================
void drawItemIcon(int k, Vector2 c, float R, float t) {
    Color base = icol(ITEMS[k].cidx);
    DrawCircleGradient((int)c.x, (int)c.y, R * 1.25f, Fade(base, 0.18f), Fade(base, 0.0f));
    switch (k) {
    case IK_LENS: {
        DrawRing(c, R * 0.66f, R * 0.76f, t * 40, t * 40 + 300, 36, GOLD);
        DrawEllipse((int)c.x, (int)c.y, R * 0.52f, R * 0.34f, Fade(SKYBLUE, 0.55f));
        DrawEllipse((int)c.x, (int)c.y, R * 0.34f, R * 0.22f, Fade(RAYWHITE, 0.75f));
        float g = fmodf(t * 0.7f, 1.0f);
        DrawCircleV({ c.x - R * 0.3f + g * R * 0.6f, c.y - R * 0.12f }, 3, RAYWHITE);
    } break;
    case IK_STAB: {
        float p = 0.5f + 0.5f * sinf(t * 3);
        DrawPoly(c, 6, R * 0.78f, t * 12, Fade(SKYBLUE, 0.22f + 0.18f * p));
        DrawPolyLines(c, 6, R * 0.78f, t * 12, SKYBLUE);
        DrawPolyLines(c, 6, R * 0.50f, -t * 18, Fade(RAYWHITE, 0.6f));
        DrawCircleV(c, R * 0.16f, SKYBLUE);
    } break;
    case IK_AMP: {
        float a = t * 2.0f;
        Vector2 p1 = { c.x + cosf(a) * R * 0.62f, c.y + sinf(a) * R * 0.36f };
        Vector2 p2 = { c.x - cosf(a) * R * 0.62f, c.y - sinf(a) * R * 0.36f };
        DrawLineEx(p1, p2, 3, Fade(VIOLET, 0.75f));
        DrawCircleV(p1, R * 0.18f, Color{ 200,150,255,255 });
        DrawCircleV(p2, R * 0.18f, Color{ 200,150,255,255 });
        DrawRing(c, R * 0.80f, R * 0.86f, -t * 30, -t * 30 + 200, 32, Fade(VIOLET, 0.5f));
    } break;
    case IK_SPRING: {
        DrawEllipse((int)c.x, (int)(c.y + R * 0.62f), (int)(R * 0.72f), (int)(R * 0.22f),
                    Color{ 52,58,78,255 });
        DrawEllipse((int)c.x, (int)(c.y + R * 0.60f), (int)(R * 0.52f), (int)(R * 0.15f),
                    Fade(GOLD, 0.35f));
        for (int i = 0; i < 3; ++i) {
            float ph = fmodf(t * 0.9f + i * 0.33f, 1.0f);
            float w = R * (0.34f - 0.22f * ph);
            DrawEllipse((int)c.x, (int)(c.y + R * 0.55f - ph * R * 1.15f),
                        (int)w, (int)(w * 0.55f), Fade(GOLD, 0.75f * (1 - ph * 0.6f)));
        }
        for (int i = 0; i < 5; ++i) {
            float ph = fmodf(t * 1.15f + i * 0.2f, 1.0f);
            float a = (i * 72 + 20) * DEG2RAD;
            float rr = R * 0.18f + ph * R * 0.62f;
            DrawCircleV({ c.x + cosf(a) * rr, c.y + R * 0.30f - sinf(ph * PI) * R * 0.75f },
                        2.6f * (1 - ph * 0.5f), Fade(Color{ 255,236,170,255 }, 1 - ph));
        }
        DrawRing(c, R * 0.86f, R * 0.92f, t * 34, t * 34 + 150, 28, Fade(GOLD, 0.45f));
    } break;
    case IK_FERT: {
        DrawCircleV({ c.x, c.y + R * 0.18f }, R * 0.42f, Color{ 96,200,120,255 });
        DrawPoly({ c.x, c.y - R * 0.30f }, 3, R * 0.36f, 180, Color{ 96,200,120,255 });
        DrawCircleV({ c.x - R * 0.12f, c.y + R * 0.10f }, R * 0.12f, Fade(RAYWHITE, 0.6f));
        for (int i = 0; i < 4; ++i) {
            float yy = fmodf(t * 40 + i * 18, 70.0f) / 70.0f;
            DrawCircleV({ c.x - R * 0.5f + i * R * 0.34f, c.y + R * 0.7f - yy * R * 1.4f },
                        2.6f, Fade(GREEN, 1 - yy));
        }
    } break;
    case IK_SHIELD: {
        DrawPoly(c, 6, R * 0.72f, 90, Fade(SKYBLUE, 0.28f));
        DrawPolyLines(c, 6, R * 0.72f, 90, SKYBLUE);
        DrawRing(c, R * 0.82f, R * 0.90f, t * 80, t * 80 + 120, 32, Fade(RAYWHITE, 0.85f));
        DrawRing(c, R * 0.82f, R * 0.90f, t * 80 + 180, t * 80 + 300, 32, Fade(RAYWHITE, 0.55f));
        DrawCircleV(c, R * 0.18f, Fade(SKYBLUE, 0.8f));
    } break;
    case IK_SEEDER: {
        DrawRectangleRounded({ c.x - R * 0.52f, c.y - R * 0.72f, R * 1.04f, R * 0.52f }, 0.28f, 6,
                             Color{ 120,110,92,255 });
        DrawRectangleRounded({ c.x - R * 0.18f, c.y - R * 0.24f, R * 0.36f, R * 0.22f }, 0.3f, 4,
                             Color{ 90,82,68,255 });
        for (int i = 0; i < 3; ++i) {
            float yy = fmodf(t * 45 + i * 20, 60.0f) / 60.0f;
            DrawCircleV({ c.x + (i - 1) * R * 0.26f, c.y - R * 0.02f + yy * R * 0.8f },
                        3.4f, Fade(Color{ 150,230,150,255 }, 1 - yy * 0.7f));
        }
        DrawEllipse((int)c.x, (int)(c.y + R * 0.78f), (int)(R * 0.6f), (int)(R * 0.16f), Color{ 62,48,40,255 });
    } break;
    case IK_EYE: {
        float bl = 0.55f + 0.45f * fabsf(sinf(t * 0.9f));
        DrawEllipse((int)c.x, (int)c.y, R * 0.86f, R * 0.52f * bl, Fade(RAYWHITE, 0.9f));
        float ox = sinf(t * 1.6f) * R * 0.22f;
        DrawCircleV({ c.x + ox, c.y }, R * 0.26f * bl + 1, Color{ 70,150,235,255 });
        DrawCircleV({ c.x + ox, c.y }, R * 0.12f * bl + 1, Color{ 20,24,36,255 });
        DrawCircleV({ c.x + ox + R * 0.08f, c.y - R * 0.08f }, R * 0.05f, RAYWHITE);
        DrawRing(c, R * 0.92f, R * 0.98f, -t * 40, -t * 40 + 90, 24, Fade(PINK, 0.6f));
    } break;
    case IK_BLOOM: {
        for (int p = 0; p < 6; ++p) {
            float a = (t * 26 + p * 60) * DEG2RAD;
            DrawCircleV({ c.x + cosf(a) * R * 0.42f, c.y + sinf(a) * R * 0.42f }, R * 0.26f,
                        Color{ 244,132,186,255 });
        }
        DrawCircleV(c, R * 0.24f, GOLD);
        for (int i = 0; i < 3; ++i) {
            float a = t * 1.4f + i * 2.1f;
            DrawCircleV({ c.x + cosf(a) * R * 0.85f, c.y + sinf(a) * R * 0.85f }, 2.6f,
                        Fade(RAYWHITE, 0.8f));
        }
    } break;
    case IK_CORE: {
        DrawRing(c, R * 0.5f, R * 0.58f, t * 60, t * 60 + 250, 32, Fade(VIOLET, 0.9f));
        DrawRing(c, R * 0.72f, R * 0.80f, -t * 45, -t * 45 + 200, 32, Fade(PURPLE, 0.7f));
        DrawCircleV(c, R * 0.24f, Color{ 200,150,255,255 });
        DrawCircleV(c, R * 0.11f, RAYWHITE);
    } break;
    case IK_LUCK: {
        float w = fabsf(cosf(t * 2.4f));
        DrawEllipse((int)c.x, (int)c.y, R * 0.56f * w + 2, R * 0.56f, GOLD);
        DrawEllipse((int)c.x, (int)c.y, (R * 0.56f * w + 2) * 0.62f, R * 0.34f, Color{ 255,232,150,255 });
        for (int i = 0; i < 3; ++i) {
            float a = t * 2.0f + i * 2.1f;
            DrawCircleV({ c.x + cosf(a) * R * 0.86f, c.y + sinf(a) * R * 0.86f }, 2.2f, Fade(GOLD, 0.85f));
        }
    } break;
    case IK_CRYSTAL: {
        DrawPoly(c, 4, R * 0.62f, t * 36, Fade(SKYBLUE, 0.55f));
        DrawPolyLines(c, 4, R * 0.62f, t * 36, RAYWHITE);
        DrawPoly(c, 4, R * 0.30f, -t * 56, Fade(RAYWHITE, 0.8f));
        float s = 0.5f + 0.5f * sinf(t * 3);
        DrawCircleV({ c.x + R * 0.5f, c.y - R * 0.5f }, 2 + 2 * s, Fade(RAYWHITE, 0.8f));
    } break;
    case IK_ENTROPY: {
        for (int i = 0; i < 3; ++i)
            DrawRing(c, R * (0.28f + i * 0.20f), R * (0.34f + i * 0.20f),
                     -t * (50 + i * 24), -t * (50 + i * 24) + 190, 28,
                     Fade(ORANGE, 0.75f - i * 0.18f));
        DrawCircleV(c, R * 0.14f, Fade(ORANGE, 0.9f));
    } break;
    }
}

// ==================== 背景与特效 ====================
// 豪华界面是否生效
bool luxActive() { return rec.luxUnlocked && rec.luxOn; }

// 垂直渐变幕布：所有风格的共同底层
static void bgGradient(Color top, Color bot) {
    DrawRectangleGradientV(0, 0, VW, VH, top, bot);
}

void drawBG(float t) {
    const ThemeStyle& th = curTheme();
    if (luxActive()) {
        // 【宇宙尽头的花园】星系演化背景
        ClearBackground(Color{ 6,6,14,255 });
        for (int i = 0; i < 3; ++i) {                       // 星云辉光
            float a = t * 0.05f + i * 2.1f;
            Vector2 c = { VW * 0.5f + cosf(a) * 320, VH * 0.5f + sinf(a * 0.8f) * 190 };
            Color nb = i == 0 ? Color{ 90,50,190,255 } : i == 1 ? Color{ 190,60,140,255 }
                                                       : Color{ 40,110,200,255 };
            DrawCircleGradient((int)c.x, (int)c.y, 300, Fade(nb, 0.16f), Fade(nb, 0.0f));
        }
        for (int i = 0; i < 150; ++i) {                     // 星场
            float sx = fmodf(i * 137.5f + t * (3 + i % 7), (float)VW);
            float sy = fmodf(i * 79.3f + t * (1 + i % 3), (float)VH);
            float tw = 0.35f + 0.65f * fabsf(sinf(t * (1.0f + (i % 5) * 0.4f) + i));
            DrawCircle((int)sx, (int)sy, 0.8f + (i % 4) * 0.5f,
                       Fade(i % 9 == 0 ? GOLD : RAYWHITE, tw * 0.75f));
        }
        for (int k = 0; k < 2; ++k) {                       // 旋臂尘带
            for (int i = 0; i < 90; ++i) {
                float f = i / 90.0f;
                float ang = f * 5.2f + t * 0.14f + k * PI;
                float rr = 60 + f * 470;
                DrawCircleV({ VW * 0.5f + cosf(ang) * rr, VH * 0.5f + sinf(ang) * rr * 0.56f },
                            1.6f * (1 - f * 0.55f),
                            Fade(k ? Color{ 255,190,120,255 } : Color{ 160,200,255,255 }, 0.26f * (1 - f)));
            }
        }
        DrawRectangleLinesEx({ 6,6,VW - 12.0f,VH - 12.0f }, 2,
                             Fade(GOLD, 0.30f + 0.16f * sinf(t * 1.6f)));
        return;
    }
    // ---------- 各风格专属幕布 ----------
    ClearBackground(th.bgBot);
    bgGradient(th.bgTop, th.bgBot);
    Color acc = th.accent;

    switch (th.kind) {
    // ===== 玄幻仙纹：云纹翻卷、符箓浮空、灵光upward =====
    case TK_MYSTIC: {
        // 底部云海：多层缓慢横移的弧带
        for (int L = 0; L < 3; ++L) {
            float yb = VH - 40 - L * 52.0f;
            float spd = 10 + L * 7;
            for (int i = 0; i < 9; ++i) {
                float cx = fmodf(i * 190.0f + t * spd, VW + 240.0f) - 120.0f;
                DrawRing({ cx, yb }, 42, 50, 195, 345, 26,
                         Fade(th.accent, 0.055f + L * 0.018f));
                DrawRing({ cx + 60, yb + 12 }, 28, 34, 190, 350, 20,
                         Fade(th.accent2, 0.040f));
            }
        }
        // 悬浮符箓：缓慢升腾并旋转
        for (int i = 0; i < 7; ++i) {
            float ph = fmodf(t * 0.055f + i * 0.143f, 1.0f);
            float px = 110 + fmodf(i * 271.0f, VW - 220.0f);
            float py = VH - ph * (VH + 80) + 40;
            float al = 0.16f * sinf(ph * PI);
            Rectangle rr = { px - 9, py - 14, 18, 28 };
            DrawRectangleRounded(rr, 0.25f, 6, Fade(th.accent2, al));
            DrawRectangleRoundedLines(rr, 0.25f, 6, Fade(th.accent, al * 1.6f));
            DrawLineEx({ px, py - 8 }, { px, py + 8 }, 1.2f, Fade(th.accent, al * 1.4f));
            DrawLineEx({ px - 5, py - 2 }, { px + 5, py - 2 }, 1.2f, Fade(th.accent, al * 1.4f));
        }
        // 星点灵光
        for (int i = 0; i < 40; ++i) {
            float sx = fmodf(i * 137.5f + t * (4 + i % 4), (float)VW);
            float sy = fmodf(i * 91.7f + t * 3, (float)VH);
            float tw = 0.4f + 0.6f * fabsf(sinf(t * 1.4f + i));
            DrawCircle((int)sx, (int)sy, 1.4f, Fade(th.accent2, 0.13f * tw));
        }
    } break;

    // ===== 黄铜蒸汽：转动齿轮、铆钉网格、蒸汽升腾 =====
    case TK_STEAM: {
        // 背景铆钉网格
        for (int y = 40; y < VH; y += 80)
            for (int x = 40; x < VW; x += 80)
                DrawCircle(x, y, 1.8f, Fade(th.accent2, 0.10f));
        // 大齿轮：两枚反向转动
        struct G { float x, y, R; int teeth; float spd; };
        const G gs[3] = { {150, 600, 96, 14, 0.20f}, {VW - 120.0f, 130, 78, 12, -0.26f},
                          {VW - 260.0f, 620, 58, 10, 0.32f} };
        for (int k = 0; k < 3; ++k) {
            const G& g = gs[k];
            Vector2 c = { g.x, g.y };
            float rot = t * g.spd;
            DrawCircleLines((int)c.x, (int)c.y, g.R, Fade(th.accent2, 0.14f));
            DrawCircleLines((int)c.x, (int)c.y, g.R * 0.62f, Fade(th.accent2, 0.10f));
            for (int i = 0; i < g.teeth; ++i) {
                float a = rot + i * 2 * PI / g.teeth;
                Vector2 p1 = { c.x + cosf(a) * g.R, c.y + sinf(a) * g.R };
                Vector2 p2 = { c.x + cosf(a) * (g.R + 12), c.y + sinf(a) * (g.R + 12) };
                DrawLineEx(p1, p2, 6.0f, Fade(th.accent2, 0.13f));
            }
            // 辐条
            for (int i = 0; i < 6; ++i) {
                float a = rot + i * PI / 3;
                DrawLineEx({ c.x + cosf(a) * g.R * 0.20f, c.y + sinf(a) * g.R * 0.20f },
                           { c.x + cosf(a) * g.R * 0.60f, c.y + sinf(a) * g.R * 0.60f },
                           3.0f, Fade(th.accent2, 0.11f));
            }
        }
        // 蒸汽：底部升起的膨胀团
        for (int i = 0; i < 10; ++i) {
            float ph = fmodf(t * 0.10f + i * 0.1f, 1.0f);
            float px = 80 + fmodf(i * 233.0f, VW - 160.0f) + sinf(ph * 3 + i) * 22;
            float py = VH - ph * (VH * 0.85f);
            float rr = 16 + ph * 46;
            DrawCircleGradient((int)px, (int)py, rr,
                               Fade(Color{ 255,232,196,255 }, 0.055f * (1 - ph)),
                               Fade(Color{ 255,232,196,255 }, 0));
        }
        // 煤气灯暖光晕（四角）
        DrawCircleGradient(0, 0, 260, Fade(th.accent, 0.06f), Fade(th.accent, 0));
        DrawCircleGradient(VW, VH, 300, Fade(th.accent, 0.05f), Fade(th.accent, 0));
    } break;

    // ===== 苔原苗圃：木纹地板、藤蔓垂挂、晨光与花粉 =====
    case TK_NATURE: {
        // 顶部晨光斜射
        for (int i = 0; i < 5; ++i) {
            float x0 = 120 + i * 240.0f;
            DrawTriangle({ x0, 0 }, { x0 - 70, (float)VH }, { x0 + 130, (float)VH },
                         Fade(Color{ 255,248,200,255 }, 0.022f));
        }
        // 木纹横条
        for (int y = 0; y < VH; y += 46) {
            DrawLine(0, y, VW, y, Fade(Color{ 70,52,34,255 }, 0.16f));
            for (int i = 0; i < 3; ++i) {
                float yy = y + 12.0f + i * 11;
                DrawLine(0, (int)yy, VW, (int)yy, Fade(Color{ 60,44,28,255 }, 0.05f));
            }
        }
        // 顶部垂挂藤蔓
        for (int i = 0; i < 8; ++i) {
            float px = 60 + i * 165.0f;
            float len = 90 + fmodf(i * 71.0f, 130.0f);
            float sw = sinf(t * 0.7f + i) * 9;
            Vector2 prev = { px, 0 };
            for (int k = 1; k <= 8; ++k) {
                float f = k / 8.0f;
                Vector2 p = { px + sw * f * f, len * f };
                DrawLineEx(prev, p, 2.2f, Fade(Color{ 90,150,100,255 }, 0.22f));
                // 叶片
                if (k % 2 == 0) {
                    float dir = (k % 4 == 0) ? 1.0f : -1.0f;
                    DrawEllipse((int)(p.x + dir * 7), (int)p.y, 7, 3.6f,
                                Fade(Color{ 110,200,130,255 }, 0.20f));
                }
                prev = p;
            }
        }
        // 花粉浮尘
        for (int i = 0; i < 34; ++i) {
            float px = fmodf(i * 197.0f + t * (6 + i % 5), (float)VW);
            float py = fmodf(i * 113.0f + sinf(t * 0.5f + i) * 30 + t * 4, (float)VH);
            DrawCircleV({ px, py }, 1.6f + (i % 3) * 0.5f,
                        Fade(Color{ 255,246,190,255 }, 0.14f));
        }
    } break;

    // ===== 赛博矩阵：电路走线、数据雨、扫描线、HUD 网格 =====
    case TK_TECH: {
        // 密网格
        for (int x = 0; x < VW; x += 40) DrawLine(x, 0, x, VH, Fade(acc, 0.035f));
        for (int y = 0; y < VH; y += 40) DrawLine(0, y, VW, y, Fade(acc, 0.035f));
        // 电路走线：直角折线 + 节点
        for (int i = 0; i < 12; ++i) {
            float y0 = 30 + i * 56.0f;
            float x0 = fmodf(i * 173.0f, VW * 0.6f);
            float w1 = 90 + fmodf(i * 61.0f, 160.0f);
            float h1 = (i % 2 ? 1 : -1) * (34 + fmodf(i * 37.0f, 40.0f));
            DrawLineEx({ x0, y0 }, { x0 + w1, y0 }, 1.4f, Fade(acc, 0.13f));
            DrawLineEx({ x0 + w1, y0 }, { x0 + w1, y0 + h1 }, 1.4f, Fade(acc, 0.13f));
            DrawLineEx({ x0 + w1, y0 + h1 }, { x0 + w1 + 70, y0 + h1 }, 1.4f, Fade(acc, 0.13f));
            DrawCircleV({ x0, y0 }, 2.6f, Fade(acc, 0.28f));
            DrawCircleV({ x0 + w1 + 70, y0 + h1 }, 2.6f, Fade(th.accent2, 0.26f));
            // 数据脉冲沿线奔跑
            float f = fmodf(t * 0.5f + i * 0.13f, 1.0f);
            DrawCircleV({ x0 + f * w1, y0 }, 2.4f, Fade(RAYWHITE, 0.45f * (1 - f * 0.4f)));
        }
        // 数据雨
        for (int i = 0; i < 22; ++i) {
            float px = 24 + fmodf(i * 227.0f, (float)VW);
            float ph = fmodf(t * (0.22f + (i % 5) * 0.05f) + i * 0.17f, 1.0f);
            float py = ph * (VH + 120) - 60;
            for (int k = 0; k < 7; ++k)
                DrawRectangle((int)px, (int)(py - k * 15), 2, 9,
                              Fade(acc, 0.20f * (1 - k / 7.0f)));
        }
        // 全屏扫描线
        float sy = fmodf(t * 60, (float)VH);
        DrawRectangle(0, (int)sy, VW, 2, Fade(acc, 0.10f));
        DrawRectangleGradientV(0, (int)sy - 40, VW, 40, Fade(acc, 0.0f), Fade(acc, 0.045f));
    } break;

    // ===== 猫猫乐园：爪印小径、毛线球、飘落猫爪 =====
    case TK_CAT: {
        // 斜向柔和条纹（毛毯质感）
        for (int i = -VH; i < VW; i += 76) {
            DrawLineEx({ (float)i, 0 }, { i + (float)VH, (float)VH }, 26,
                       Fade(Color{ 255,255,255,255 }, 0.014f));
        }
        // 爪印小径：沿正弦曲线一路走过去
        for (int i = 0; i < 16; ++i) {
            float px = 50 + i * 82.0f;
            float py = VH * 0.5f + sinf(i * 0.55f + t * 0.25f) * 210;
            float al = 0.075f + 0.03f * sinf(t * 1.1f + i);
            float dir = (i % 2) ? 1.0f : -1.0f;
            Vector2 c = { px, py + dir * 13 };
            DrawEllipse((int)c.x, (int)(c.y + 3), 7.0f, 5.6f, Fade(th.accent, al));
            for (int k = 0; k < 4; ++k) {
                float a = (-140 + k * 40) * DEG2RAD;
                DrawCircleV({ c.x + cosf(a) * 9.5f, c.y + sinf(a) * 9.5f }, 2.9f,
                            Fade(th.accent, al));
            }
        }
        // 毛线球：缓慢滚动并带缠绕线
        for (int i = 0; i < 3; ++i) {
            float px = fmodf(t * (11 + i * 6) + i * 430.0f, VW + 160.0f) - 80.0f;
            float py = 140 + i * 210.0f;
            float R = 30 + i * 7.0f;
            DrawCircleV({ px, py }, R, Fade(th.accent, 0.055f));
            for (int k = 0; k < 5; ++k)
                DrawRing({ px, py }, R * 0.30f + k * R * 0.13f, R * 0.32f + k * R * 0.13f,
                         t * 26 + k * 55, t * 26 + k * 55 + 210, 18,
                         Fade(th.accent2, 0.055f));
        }
        // 飘落的小爪
        for (int i = 0; i < 9; ++i) {
            float ph = fmodf(t * 0.075f + i * 0.111f, 1.0f);
            float px = 90 + fmodf(i * 311.0f, VW - 180.0f) + sinf(ph * 4 + i) * 26;
            float py = ph * (VH + 60) - 30;
            float al = 0.10f * sinf(ph * PI);
            DrawEllipse((int)px, (int)(py + 3), 5.2f, 4.2f, Fade(RAYWHITE, al));
            for (int k = 0; k < 4; ++k) {
                float a = (-140 + k * 40) * DEG2RAD;
                DrawCircleV({ px + cosf(a) * 7, py + sinf(a) * 7 }, 2.1f, Fade(RAYWHITE, al));
            }
        }
    } break;

    // ===== 熔岩机甲：装甲板缝、警示斜纹、熔流裂隙、飞溅火星 =====
    case TK_MECHA: {
        // 装甲板分割缝
        for (int y = 0; y < VH; y += 96) {
            DrawLine(0, y, VW, y, Fade(BLACK, 0.30f));
            DrawLine(0, y + 2, VW, y + 2, Fade(th.accent, 0.045f));
        }
        for (int x = 0; x < VW; x += 160) {
            DrawLine(x, 0, x, VH, Fade(BLACK, 0.24f));
            // 板角铆钉
            for (int y = 24; y < VH; y += 96)
                DrawCircle(x + 12, y, 2.2f, Fade(th.accent2, 0.14f));
        }
        // 底部与顶部警示斜纹条
        for (int i = 0; i < 40; ++i) {
            float sx = fmodf(i * 44.0f + t * 12, VW + 60.0f) - 30.0f;
            DrawLineEx({ sx, (float)VH }, { sx + 26, VH - 26.0f }, 9,
                       Fade(i % 2 ? th.accent2 : Color{ 30,20,16,255 }, 0.10f));
            DrawLineEx({ sx, 0 }, { sx + 26, 26 }, 9,
                       Fade(i % 2 ? th.accent2 : Color{ 30,20,16,255 }, 0.07f));
        }
        // 熔流裂隙：两道搏动的岩缝
        for (int k = 0; k < 2; ++k) {
            float baseY = k ? VH * 0.72f : VH * 0.28f;
            float pulse = 0.55f + 0.45f * sinf(t * 1.5f + k * 2);
            Vector2 prev = { 0, baseY };
            for (int i = 1; i <= 26; ++i) {
                float f = i / 26.0f;
                Vector2 p = { f * VW, baseY + sinf(f * 7 + k * 2 + t * 0.4f) * 34 };
                DrawLineEx(prev, p, 3.4f, Fade(Color{ 255,140,60,255 }, 0.13f * pulse));
                DrawLineEx(prev, p, 1.3f, Fade(Color{ 255,225,170,255 }, 0.16f * pulse));
                prev = p;
            }
        }
        // 飞溅火星
        for (int i = 0; i < 22; ++i) {
            float ph = fmodf(t * 0.30f + i * 0.045f, 1.0f);
            float px = fmodf(i * 173.0f, (float)VW);
            float py = VH - ph * VH * 0.9f;
            DrawCircleV({ px + sinf(ph * 6 + i) * 16, py }, 1.9f * (1 - ph),
                        Fade(Color{ 255,190,110,255 }, 0.35f * (1 - ph)));
        }
    } break;

    // ===== 虚空极简：近乎全黑，一条水平线与极少留白刻度 =====
    case TK_VOID: {
        // 唯一的水平地平线
        float hy = VH * 0.62f;
        DrawLine(0, (int)hy, VW, (int)hy, Fade(th.accent, 0.14f));
        DrawRectangleGradientV(0, (int)hy - 60, VW, 60, Fade(th.accent, 0.0f),
                               Fade(th.accent, 0.022f));
        // 左右两侧极简刻度
        for (int i = 0; i < 9; ++i) {
            float yy = 90 + i * 60.0f;
            float w = (i % 3 == 0) ? 22.0f : 11.0f;
            DrawLine(0, (int)yy, (int)w, (int)yy, Fade(th.accent, 0.10f));
            DrawLine(VW - (int)w, (int)yy, VW, (int)yy, Fade(th.accent, 0.10f));
        }
        // 极少数缓慢漂移的尘点
        for (int i = 0; i < 14; ++i) {
            float px = fmodf(i * 311.0f + t * 3.0f, (float)VW);
            float py = fmodf(i * 173.0f + t * 1.4f, (float)VH);
            DrawCircleV({ px, py }, 1.1f, Fade(RAYWHITE, 0.07f));
        }
        // 极细的呼吸边框
        DrawRectangleLinesEx({ 18,18,VW - 36.0f,VH - 36.0f }, 1,
                             Fade(th.accent, 0.05f + 0.03f * sinf(t * 0.8f)));
    } break;

    // ===== 熵潮血月：低垂血月、热寂涟漪、灰烬飘散 =====
    case TK_ENTROPY: {
        // 血月
        Vector2 mc = { VW * 0.78f, VH * 0.26f };
        float mR = 96;
        DrawCircleGradient((int)mc.x, (int)mc.y, mR * 2.1f,
                           Fade(Color{ 255,90,90,255 }, 0.10f), Fade(Color{ 255,60,60,255 }, 0));
        DrawCircleV(mc, mR, Fade(Color{ 190,54,62,255 }, 0.30f));
        DrawCircleV(mc, mR * 0.97f, Fade(Color{ 220,80,84,255 }, 0.22f));
        // 月面暗斑
        for (int i = 0; i < 6; ++i) {
            float a = i * 1.05f;
            DrawCircleV({ mc.x + cosf(a) * mR * 0.45f, mc.y + sinf(a) * mR * 0.42f },
                        mR * (0.10f + (i % 3) * 0.045f), Fade(Color{ 140,40,48,255 }, 0.20f));
        }
        // 热寂涟漪：从月心向外扩散的巨环
        for (int i = 0; i < 4; ++i) {
            float f = fmodf(t * 0.10f + i / 4.0f, 1.0f);
            DrawRing(mc, mR + f * 420, mR + f * 420 + 2.2f, 0, 360, 60,
                     Fade(th.accent, 0.055f * (1 - f)));
        }
        // 灰烬：向上飘散并左右摆动
        for (int i = 0; i < 40; ++i) {
            float ph = fmodf(t * 0.085f + i * 0.025f, 1.0f);
            float px = fmodf(i * 197.0f, (float)VW) + sinf(ph * 5 + i) * 26;
            float py = VH - ph * (VH + 40);
            DrawCircleV({ px, py }, 1.5f + (i % 3) * 0.6f,
                        Fade(Color{ 255,170,150,255 }, 0.16f * sinf(ph * PI)));
        }
        // 地面热雾
        DrawRectangleGradientV(0, VH - 130, VW, 130, Fade(th.accent, 0.0f),
                               Fade(th.accent, 0.05f));
    } break;

    // ===== 量子基调（默认）：星点 + 细网格，冷静克制 =====
    default: {
        for (int i = 0; i < 46; ++i) {
            float sx = fmodf(i * 137.5f + t * (8 + i % 5) * 2, (float)VW);
            float sy = fmodf(i * 91.7f + t * 6, (float)VH);
            DrawCircle((int)sx, (int)sy, 1.5f + (i % 3) * 0.6f,
                       Fade(acc, 0.10f + (i % 4) * 0.03f));
        }
        for (int x = 0; x < VW; x += 64) DrawLine(x, 0, x, VH, Fade(acc, 0.025f));
        for (int y = 0; y < VH; y += 64) DrawLine(0, y, VW, y, Fade(acc, 0.025f));
        // 轻微的量子波纹，给静态画面一点呼吸
        for (int i = 0; i < 3; ++i) {
            float f = fmodf(t * 0.07f + i / 3.0f, 1.0f);
            DrawRing({ VW * 0.5f, VH * 0.5f }, 120 + f * 380, 121.5f + f * 380, 0, 360, 60,
                     Fade(acc, 0.030f * (1 - f)));
        }
    } break;
    }
}

// 【宇宙尽头的花园】豪华界面前景层
void drawLuxFrame(float t) {
    float br = 0.34f + 0.20f * sinf(t * 1.5f);
    DrawRectangleLinesEx({ 4,4,VW - 8.0f,VH - 8.0f }, 3, Fade(GOLD, br));
    DrawRectangleLinesEx({ 11,11,VW - 22.0f,VH - 22.0f }, 1,
                         Fade(Color{ 255,235,170,255 }, br * 0.45f));
    const Vector2 cn[4] = { {28,28},{VW - 28.0f,28},{28,VH - 28.0f},{VW - 28.0f,VH - 28.0f} };
    for (int i = 0; i < 4; ++i) {
        DrawCircleGradient((int)cn[i].x, (int)cn[i].y, 30, Fade(GOLD, 0.20f), Fade(GOLD, 0));
        DrawPoly(cn[i], 4, 10 + 2.5f * sinf(t * 2 + i), t * 24 + i * 45, Fade(GOLD, 0.85f));
        DrawPoly(cn[i], 4, 4.5f, -t * 32, Fade(RAYWHITE, 0.9f));
    }
    for (int i = 0; i < 3; ++i) {
        float f = fmodf(t * 0.12f + i / 3.0f, 1.0f);
        DrawCircleV({ 12 + f * (VW - 24), 7 }, 2.6f, Fade(GOLD, 0.8f));
        DrawCircleV({ VW - 12 - f * (VW - 24), VH - 7.0f }, 2.6f, Fade(GOLD, 0.8f));
    }
}

void drawFX(float dt) {
    for (auto& p : parts) {
        p.life -= dt; p.vel.y += 130 * dt;
        p.pos.x += p.vel.x * dt; p.pos.y += p.vel.y * dt;
        DrawCircleV(p.pos, p.size * (p.life / p.maxLife + 0.25f), Fade(p.col, p.life / p.maxLife));
    }
    parts.erase(std::remove_if(parts.begin(), parts.end(),
        [](const Particle& p) { return p.life <= 0; }), parts.end());
    for (auto& s : shocks) {
        s.life -= dt * 2.0f; s.r = s.maxR * (1.0f - s.life);
        if (s.r > 2) DrawRing(s.pos, s.r - 2, s.r + 2, 0, 360, 40, Fade(s.col, s.life * 0.75f));
    }
    shocks.erase(std::remove_if(shocks.begin(), shocks.end(),
        [](const Shock& s) { return s.life <= 0; }), shocks.end());
    for (auto& f : floats) {
        f.life -= dt; f.pos.y -= 42 * dt;
        txtC(f.text, f.pos.x, f.pos.y, f.size, Fade(f.col, std::min(1.0f, f.life)));
    }
    floats.erase(std::remove_if(floats.begin(), floats.end(),
        [](const FloatTxt& f) { return f.life <= 0; }), floats.end());

    float ty = 90;
    for (auto& a : toasts) {
        a.life -= dt;
        float alpha = std::min(1.0f, a.life);
        float slide = (a.life > 3.6f) ? (a.life - 3.6f) * 250 : 0;
        Rectangle r = { VW - 380.0f + slide, ty, 350, 66 };
        Color tc = tierColor(ACH[a.id].tier, gTime);
        DrawRectangleRounded(r, 0.16f, 8, Fade(curTheme().panel, alpha * 0.98f));
        DrawRectangleLinesEx(r, 2, Fade(tc, alpha));
        drawMedal({ r.x + 36, r.y + 33 }, 19, ACH[a.id].tier, true, gTime);
        txt(string("成就解锁・") + TIER_NAME[ACH[a.id].tier] + "级",
            r.x + 68, r.y + 9, 16, Fade(tc, alpha));
        txt(ACH[a.id].name, r.x + 68, r.y + 31, 21, Fade(RAYWHITE, alpha));
        ty += 74;
    }
    toasts.erase(std::remove_if(toasts.begin(), toasts.end(),
        [](const AchToast& a) { return a.life <= 0; }), toasts.end());
}

void toggleFull() {
#ifdef __EMSCRIPTEN__
    // 网页版：raylib 的 GetCurrentMonitor()/SetWindowSize() 在 web 平台未实现
    //（前者直接告警，后者经 GLFW 模拟会调用新版 Emscripten 已移除的
    // Module.requestFullscreen 而抛 TypeError）。改用浏览器标准 Fullscreen API：
    // 对整个页面进出全屏，canvas 尺寸由 shell.html 的 fit() 监听 resize 自动适配。
    EM_ASM({
        var el = document.documentElement;
        var fsEl = document.fullscreenElement || document.webkitFullscreenElement;
        if (!fsEl) {
            var req = el.requestFullscreen || el.webkitRequestFullscreen;
            if (req) req.call(el);
        } else {
            var exit = document.exitFullscreen || document.webkitExitFullscreen;
            if (exit) exit.call(document);
        }
    });
#else
    if (!IsWindowFullscreen()) {
        int m = GetCurrentMonitor();
        SetWindowSize(GetMonitorWidth(m), GetMonitorHeight(m));
        ToggleFullscreen();
    } else { ToggleFullscreen(); SetWindowSize(VW, VH); }
#endif
}

// ==================== 成长曲线 ====================
void drawCurve(Rectangle r, bool big) {
    DrawRectangleRounded(r, 0.06f, 8, Color{ 24,28,42,220 });
    if (rec.histN < 2) {
        txtC("曲线暂无数据，需要至少两局才会显示", r.x + r.width / 2, r.y + r.height / 2 - 12,
             big ? 22.0f : 18.0f, GRAY);
        return;
    }
    int mx = 1;
    for (int i = 0; i < rec.histN; ++i) mx = std::max(mx, rec.hist[i]);
    float pad = big ? 40.0f : 24.0f;
    float w = r.width - pad * 2, h = r.height - pad * 2;
    for (int g = 0; g <= 4; ++g) {
        float yy = r.y + pad + h * g / 4.0f;
        DrawLine((int)(r.x + pad), (int)yy, (int)(r.x + pad + w), (int)yy, Fade(RAYWHITE, 0.08f));
        if (big) txt(to_string(mx - mx * g / 4), r.x + 6, yy - 9, 16, Fade(RAYWHITE, 0.4f));
    }
    Vector2 prev = { 0,0 };
    for (int i = 0; i < rec.histN; ++i) {
        float x = r.x + pad + (rec.histN == 1 ? 0 : w * i / (rec.histN - 1.0f));
        float y = r.y + pad + h * (1.0f - rec.hist[i] / (float)mx);
        if (i > 0) DrawLineEx(prev, { x,y }, big ? 3.0f : 2.0f, SKYBLUE);
        prev = { x,y };
    }
    for (int i = 0; i < rec.histN; ++i) {
        float x = r.x + pad + (rec.histN == 1 ? 0 : w * i / (rec.histN - 1.0f));
        float y = r.y + pad + h * (1.0f - rec.hist[i] / (float)mx);
        DrawCircleV({ x,y }, big ? 5.0f : 3.5f, GOLD);
        if (big) txtC(to_string(rec.hist[i]), x, y - 26, 16, Fade(RAYWHITE, 0.7f));
    }
}

// 等级奖章（小图标）
void drawMedal(Vector2 c, float R, int tier, bool ok, float t) {
    Color col = tierColor(tier, t);
    if (!ok) {
        DrawCircleV(c, R, Color{ 42,44,54,255 });
        DrawCircleLines((int)c.x, (int)c.y, R, Color{ 70,74,88,255 });
        txtC("?", c.x, c.y - R * 0.68f, R * 1.25f, Color{ 96,100,112,255 });
        return;
    }
    if (tier == T_RAINBOW) {
        for (int i = 0; i < 6; ++i)
            DrawCircleV(c, R + 5 - i * 0.8f,
                        Fade(ColorFromHSV(fmodf(t * 90 + i * 60, 360.0f), 0.8f, 1.0f), 0.13f));
    }
    DrawCircleGradient((int)c.x, (int)c.y, R * 1.5f, Fade(col, 0.28f), Fade(col, 0));
    DrawCircleV(c, R, Fade(col, 0.9f));
    DrawCircleV(c, R * 0.72f, Fade(BLACK, 0.30f));
    DrawPoly(c, tier == T_RAINBOW ? 6 : 5, R * 0.56f, t * (20 + tier * 12), RAYWHITE);
    DrawCircleLines((int)c.x, (int)c.y, R, Fade(RAYWHITE, 0.65f));
}
