// ============================================================
//  util.cpp  通用工具函数实现
// ============================================================
#include "util.h"
#include "state.h"
#include "audio.h"
#include "data.h"
#include <set>
#include <cmath>
#include <algorithm>
#include <random>

// ==================== 字体 ====================
Font loadChineseFont() {
    std::set<int> uniq;
    auto add = [&](const char* s) {
        if (!s || !s[0]) return;
        int n = 0; int* cp = LoadCodepoints(s, &n);
        for (int i = 0; i < n; ++i) uniq.insert(cp[i]);
        UnloadCodepoints(cp);
    };
    add(MISC_POOL);
    for (int p = 0; p < 6; ++p) {
        add(HPAGES[p].tab);
        for (int i = 0; i < HPAGES[p].n; ++i) add(HPAGES[p].l[i].s);
    }
    for (int i = 0; i < 8; ++i) {
        add(TUT[i].title); add(TUT[i].goalDesc); add(TUT[i].tip);
        for (int k = 0; k < 6; ++k) add(TUT[i].teach[k]);
    }
    for (int i = 0; i < 4; ++i) add(CFG[i].name);
    for (int i = 0; i < IK_COUNT; ++i) { add(ITEMS[i].name); add(ITEMS[i].d1); add(ITEMS[i].d2); }
    for (int i = 0; i < A_COUNT; ++i) { add(ACH[i].name); add(ACH[i].desc); add(ACH[i].cond); }
    for (int i = 0; i < SKIN_COUNT; ++i) { add(SKINS[i].name); add(SKINS[i].desc); }
    for (int i = 0; i < 4; ++i) add(TIER_NAME[i]);
    for (int i = 0; i < U_COUNT; ++i) { add(UPG[i].name); add(UPG[i].desc); }
    // 界面代码硬编码文字（见 data.cpp 的 UI_LITERALS 注释）
    for (int i = 0; i < UI_LITERAL_N; ++i) add(UI_LITERALS[i]);
    // 用户许可协议文本量大、用字生僻（法律术语），单独全量扫描，
    // 避免像 MISC_POOL 那样手工维护字符池漏字导致方块乱码。
    for (int p = 0; p < LICENSE_PAGE_N; ++p) {
        add(LICENSE_PAGES[p].title);
        for (int i = 0; i < LICENSE_PAGES[p].n; ++i) add(LICENSE_PAGES[p].l[i].s);
    }
    std::vector<int> cps(uniq.begin(), uniq.end());

    // 字体候选按"质感"排序：优先等线/黑体这类字形饱满、笔画均匀的现代字体，
    // 微软雅黑与等线在小字号下的字形开口更大，锐度明显好于宋体。
    //
    // 清晰度关键在两点（图集分辨率与 mipmap），详见 util.h 中 FONT_ATLAS_PX
    // 上方的实测数据表：96px 图集 + 不生成 mipmap + 双线性过滤。
    const char* paths[] = {
#ifdef __EMSCRIPTEN__
        "/fonts/font.ttf",                  // Browser asset bundled into the virtual filesystem.
#endif
        "font.ttf",
        "C:/Windows/Fonts/Deng.ttf",       // 等线：现代、干净、字重均匀
        "C:/Windows/Fonts/msyh.ttc",       // 微软雅黑：开口大，小字号可读性好
        "C:/Windows/Fonts/simhei.ttf",     // 黑体：字重足
        "C:/Windows/Fonts/STXIHEI.TTF",
        "C:/Windows/Fonts/simsun.ttc",
    };
    for (const char* p : paths)
        if (FileExists(p)) {
            // SDF 级别的锐度不需要，关键是图集分辨率与显示字号匹配 + 不用 mipmap
            Font f = LoadFontEx(p, FONT_ATLAS_PX, cps.data(), (int)cps.size());
            if (f.texture.id != 0 && f.glyphCount > 100) {
                SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);
                return f;
            }
        }
    return GetFontDefault();
}

// 标题字体：偏粗字重，用于大标题与强调，和正文形成层次对比
Font loadTitleFont() {
    std::set<int> uniq;
    auto add = [&](const char* s) {
        if (!s || !s[0]) return;
        int n = 0; int* cp = LoadCodepoints(s, &n);
        for (int i = 0; i < n; ++i) uniq.insert(cp[i]);
        UnloadCodepoints(cp);
    };
    add(MISC_POOL);
    for (int p = 0; p < 6; ++p) {
        add(HPAGES[p].tab);
        for (int i = 0; i < HPAGES[p].n; ++i) add(HPAGES[p].l[i].s);
    }
    for (int i = 0; i < 8; ++i) {
        add(TUT[i].title); add(TUT[i].goalDesc); add(TUT[i].tip);
        for (int k = 0; k < 6; ++k) add(TUT[i].teach[k]);
    }
    for (int i = 0; i < 4; ++i) add(CFG[i].name);
    for (int i = 0; i < IK_COUNT; ++i) { add(ITEMS[i].name); add(ITEMS[i].d1); add(ITEMS[i].d2); }
    for (int i = 0; i < A_COUNT; ++i) { add(ACH[i].name); add(ACH[i].desc); add(ACH[i].cond); }
    for (int i = 0; i < SKIN_COUNT; ++i) { add(SKINS[i].name); add(SKINS[i].desc); }
    for (int i = 0; i < TK_COUNT; ++i) add(THEMES[i].label);
    for (int i = 0; i < 4; ++i) { add(TIER_NAME[i]); add(MAT_STAGE[i]); }
    for (int i = 0; i < U_COUNT; ++i) { add(UPG[i].name); add(UPG[i].desc); }
    for (int p = 0; p < LICENSE_PAGE_N; ++p) {
        add(LICENSE_PAGES[p].title);
        for (int i = 0; i < LICENSE_PAGES[p].n; ++i) add(LICENSE_PAGES[p].l[i].s);
    }
    for (int i = 0; i < UI_LITERAL_N; ++i) add(UI_LITERALS[i]);
    std::vector<int> cps(uniq.begin(), uniq.end());

    const char* paths[] = {
#ifdef __EMSCRIPTEN__
        "/fonts/font_title.ttf",
#endif
        "font_title.ttf",
        "C:/Windows/Fonts/Dengb.ttf",      // 等线 Bold
        "C:/Windows/Fonts/msyhbd.ttc",     // 雅黑 Bold
        "C:/Windows/Fonts/simhei.ttf",
        "C:/Windows/Fonts/Deng.ttf",
    };
    // 标题字号更大（最大 58），图集用 96px 让大字保持 1:1 附近的采样比。
    // 同样不生成 mipmap，避免小标题被低阶 mipmap 糊掉。
    for (const char* p : paths)
        if (FileExists(p)) {
            Font f = LoadFontEx(p, TITLE_ATLAS_PX, cps.data(), (int)cps.size());
            if (f.texture.id != 0 && f.glyphCount > 100) {
                SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);
                return f;
            }
        }
    return Font{ 0 };   // 取不到就退回正文字体（见 titleFontOr）
}

// ==================== 工具 ====================
int randInt(int lo, int hi) { return std::uniform_int_distribution<int>(lo, hi)(rng); }
float randF() { return std::uniform_real_distribution<float>(0.f, 1.f)(rng); }
Color icol(int i) { return HCOL[std::clamp(i, 0, 8)]; }

// 字间距随字号缩放：大字放松、小字收紧，排版更透气
static inline float autoSpacing(float sz) { return sz * 0.045f + 0.6f; }
// 标题字体取不到时退回正文字体
static inline const Font& titleFontOr() { return fontTitle.texture.id ? fontTitle : font; }

// ---------- 像素对齐 ----------
// 文字落在小数坐标上时，双线性采样会把一个像素的墨迹摊到相邻两个像素，
// 边缘因此发灰发虚。把绘制原点吸附到像素网格即可避免。
// 由于内部以 SSAA 倍超采样绘制，虚拟坐标下的"一个真实像素"等于 1/SSAA，
// 因此按 1/SSAA 的粒度吸附，才能真正落在超采样画布的整数像素上。
static inline float snap(float v) {
    return floorf(v * SSAA + 0.5f) / SSAA;
}
static inline Vector2 snapV(float x, float y) { return Vector2{ snap(x), snap(y) }; }

void txt(const string& s, float x, float y, float sz, Color c) {
    sz = TS(sz);   // 全局字号放大
    DrawTextEx(font, s.c_str(), snapV(x, y), sz, autoSpacing(sz), c);
}
void txtC(const string& s, float cx, float y, float sz, Color c) {
    sz = TS(sz);
    float sp = autoSpacing(sz);
    Vector2 m = MeasureTextEx(font, s.c_str(), sz, sp);
    DrawTextEx(font, s.c_str(), snapV(cx - m.x / 2, y), sz, sp, c);
}

// ---------- 有质感的文字 ----------
// 柔和投影：偏移整数像素的半透明黑，立体感明显且不引入灰边
void txtS(const string& s, float x, float y, float sz, Color c) {
    sz = TS(sz);
    float sp = autoSpacing(sz);
    DrawTextEx(font, s.c_str(), snapV(x + 1, y + 1), sz, sp, Fade(BLACK, 0.45f));
    DrawTextEx(font, s.c_str(), snapV(x, y), sz, sp, c);
}
void txtSC(const string& s, float cx, float y, float sz, Color c) {
    // 按放大后的字号测量居中，再交给 txtS（txtS 内部再放大一次，避免双重缩放）
    float sp = autoSpacing(TS(sz));
    Vector2 m = MeasureTextEx(font, s.c_str(), TS(sz), sp);
    txtS(s, cx - m.x / 2, y, sz, c);
}

// 描边：四向偏移绘制深色字形再叠正色，保证任何幕布上都清晰。
// 偏移量取整数像素，避免描边本身变成一圈灰雾。
void txtOutline(const string& s, float x, float y, float sz, Color c, Color oc, float w) {
    sz = TS(sz);
    float sp = autoSpacing(sz);
    float o = snap(w); if (o < 1) o = 1;
    const float dx[8] = { -1,1,0,0,-1,1,-1,1 };
    const float dy[8] = { 0,0,-1,1,-1,-1,1,1 };
    for (int i = 0; i < 8; ++i)
        DrawTextEx(font, s.c_str(), snapV(x + dx[i] * o, y + dy[i] * o), sz, sp, oc);
    DrawTextEx(font, s.c_str(), snapV(x, y), sz, sp, c);
}

Vector2 measureTracked(const string& s, float sz, float spacing) {
    return MeasureTextEx(titleFontOr(), s.c_str(), TS(sz), spacing);
}
void txtTracked(const string& s, float x, float y, float sz, float spacing, Color c) {
    DrawTextEx(titleFontOr(), s.c_str(), snapV(x, y), TS(sz), spacing, c);
}

// 标题：粗字重 + 风格字距 + 描边 + 可选辉光。
// 辉光用多层低透明度同色字形堆叠模拟，比后处理便宜且不需要额外 RT。
void txtTitle(const string& s, float cx, float y, float sz, Color c, bool glow) {
    sz = TS(sz);   // 全局字号放大
    const ThemeStyle& th = curTheme();
    const Font& f = titleFontOr();
    float sp = th.titleSpace + sz * 0.03f;
    Vector2 m = MeasureTextEx(f, s.c_str(), sz, sp);
    float x = cx - m.x / 2;

    x = snap(x); y = snap(y);   // 标题也吸附到整数像素

    if (glow && th.titleGlow) {
        // 呼吸辉光：随时间轻微起伏，避免死板。
        // 偏移取整数像素，否则辉光层会在主字形边缘糊出一圈灰边。
        float pulse = 0.55f + 0.45f * sinf(gTime * 1.7f);
        for (int i = 3; i >= 1; --i) {
            float o = (float)(i * 2);
            Color gc = Fade(c, 0.085f * pulse * (4 - i));
            DrawTextEx(f, s.c_str(), Vector2{ x - o, y }, sz, sp, gc);
            DrawTextEx(f, s.c_str(), Vector2{ x + o, y }, sz, sp, gc);
            DrawTextEx(f, s.c_str(), Vector2{ x, y - o }, sz, sp, gc);
            DrawTextEx(f, s.c_str(), Vector2{ x, y + o }, sz, sp, gc);
        }
    }
    // 深色描边压住背景杂色（整数偏移，边缘干净）
    const float dx[4] = { -1,1,0,0 }, dy[4] = { 0,0,-1,1 };
    for (int i = 0; i < 4; ++i)
        DrawTextEx(f, s.c_str(), Vector2{ x + dx[i] * 2, y + dy[i] * 2 }, sz, sp,
                   Fade(BLACK, 0.55f));
    DrawTextEx(f, s.c_str(), Vector2{ x + 2, y + 2 }, sz, sp, Fade(BLACK, 0.35f));
    DrawTextEx(f, s.c_str(), Vector2{ x, y }, sz, sp, c);
}
void addLog(const string& s, Color c) {
    logs.push_back({ s,c });
    while (logs.size() > 12) logs.pop_front();
}
void addFloat(Vector2 p, const string& s, Color c, float sz) {
    floats.push_back({ p,s,c,1.4f,sz });
}
void emitBurst(Vector2 p, int n, Color c, float spd, float sz) {
    for (int i = 0; i < n; ++i) {
        float a = randF() * 2 * PI, v = spd * (0.35f + randF());
        parts.push_back({ p,{cosf(a) * v,sinf(a) * v - 20},0.7f + randF() * 0.7f,1.4f,sz * (0.5f + randF()),c });
    }
}
void addShock(Vector2 p, float r, Color c) { shocks.push_back({ p,0,r,1.0f,c }); }
void addShake(float a) { shake = std::max(shake, a); }
Rectangle cellRect(int i) {
    return Rectangle{ (float)(GRID_X + (i % SIZE) * (CELL + GAP)),
                      (float)(GRID_Y + (i / SIZE) * (CELL + GAP)),(float)CELL,(float)CELL };
}
Vector2 cellCenter(int i) { Rectangle r = cellRect(i); return Vector2{ r.x + CELL / 2,r.y + CELL / 2 }; }

// ============================================================
//  风格化 UI 构件
// ============================================================

// 切角矩形：赛博 / 机甲风格的基础形状（左上、右下各切一角）
// 注意顶点顺序：raylib 的 DrawTriangleFan 会做背面剔除，
// 顺序错了整个形状会直接不可见（已实测验证），务必保持下面的绕向。
static void cutCorners(Rectangle r, float cut, Vector2* p) {
    p[0] = { r.x,                r.y + cut };
    p[1] = { r.x,                r.y + r.height };
    p[2] = { r.x + r.width - cut, r.y + r.height };
    p[3] = { r.x + r.width,      r.y + r.height - cut };
    p[4] = { r.x + r.width,      r.y };
    p[5] = { r.x + cut,          r.y };
}
static void drawCutRect(Rectangle r, float cut, Color c) {
    Vector2 p[6]; cutCorners(r, cut, p);
    DrawTriangleFan(p, 6, c);
}
static void drawCutRectLines(Rectangle r, float cut, float th, Color c) {
    Vector2 p[6]; cutCorners(r, cut, p);
    for (int i = 0; i < 6; ++i) DrawLineEx(p[i], p[(i + 1) % 6], th, c);
}

// 面板：不同风格的容器有不同的边框语言与角饰
void uiPanelAccent(Rectangle r, Color edge, float roundness) {
    const ThemeStyle& th = curTheme();
    switch (th.btn) {
    case BS_SHARP: {   // 赛博：切角面板 + 四角 HUD 括号
        drawCutRect(r, 16, th.panel);
        drawCutRectLines(r, 16, 1.6f, Fade(edge, 0.75f));
        float L = 22;
        DrawLineEx({ r.x + 16, r.y + 1 }, { r.x + 16 + L, r.y + 1 }, 3, edge);
        DrawLineEx({ r.x + r.width - 1, r.y + 16 }, { r.x + r.width - 1, r.y + 16 + L }, 3, edge);
        DrawLineEx({ r.x + 1, r.y + r.height - 16 - L }, { r.x + 1, r.y + r.height - 16 }, 3, edge);
        DrawLineEx({ r.x + r.width - 16 - L, r.y + r.height - 1 },
                   { r.x + r.width - 16, r.y + r.height - 1 }, 3, edge);
    } break;
    case BS_ARMOR: {   // 机甲：装甲板 + 顶部警示条
        drawCutRect(r, 14, th.panel);
        drawCutRectLines(r, 14, 2.0f, Fade(edge, 0.8f));
        DrawRectangle((int)(r.x + 18), (int)r.y + 4, (int)(r.width - 40), 3, Fade(edge, 0.5f));
    } break;
    case BS_LINE: {    // 极简：无圆角，仅左侧一条强调线
        DrawRectangleRec(r, th.panel);
        DrawRectangleLinesEx(r, 1.0f, Fade(edge, 0.45f));
        DrawRectangle((int)r.x, (int)r.y, 3, (int)r.height, Fade(edge, 0.85f));
    } break;
    case BS_RUNE: {    // 玄幻：双线边框 + 四角云纹
        DrawRectangleRounded(r, roundness, 8, th.panel);
        DrawRectangleRoundedLines(r, roundness, 8, Fade(edge, 0.7f));
        DrawRectangleRoundedLines({ r.x + 5,r.y + 5,r.width - 10,r.height - 10 }, roundness, 8,
                                  Fade(edge, 0.28f));
        const Vector2 cn[4] = { {r.x + 12,r.y + 12},{r.x + r.width - 12,r.y + 12},
                                {r.x + 12,r.y + r.height - 12},{r.x + r.width - 12,r.y + r.height - 12} };
        for (int i = 0; i < 4; ++i)
            DrawRing(cn[i], 5, 7, i * 90.0f, i * 90.0f + 200, 16, Fade(th.accent2, 0.55f));
    } break;
    case BS_BEVEL: {   // 蒸汽：厚边框 + 四角铆钉
        DrawRectangleRounded(r, roundness, 8, th.panel);
        DrawRectangleRoundedLines(r, roundness, 8, Fade(edge, 0.85f));
        const Vector2 cn[4] = { {r.x + 13,r.y + 13},{r.x + r.width - 13,r.y + 13},
                                {r.x + 13,r.y + r.height - 13},{r.x + r.width - 13,r.y + r.height - 13} };
        for (int i = 0; i < 4; ++i) {
            DrawCircleV(cn[i], 4.0f, Fade(th.accent2, 0.9f));
            DrawCircleV({ cn[i].x - 1,cn[i].y - 1 }, 1.6f, Fade(RAYWHITE, 0.6f));
        }
    } break;
    case BS_EAR: {     // 猫猫：大圆角 + 角落爪印
        DrawRectangleRounded(r, std::max(roundness, 0.10f), 10, th.panel);
        DrawRectangleRoundedLines(r, std::max(roundness, 0.10f), 10, Fade(edge, 0.6f));
        Vector2 pw = { r.x + r.width - 20, r.y + 16 };
        DrawCircleV(pw, 5.0f, Fade(th.accent, 0.30f));
        for (int i = 0; i < 4; ++i) {
            float a = (-150 + i * 40) * DEG2RAD;
            DrawCircleV({ pw.x + cosf(a) * 8, pw.y + sinf(a) * 8 }, 2.2f, Fade(th.accent, 0.30f));
        }
    } break;
    case BS_ORNATE: {  // 宇宙：双金线 + 四角菱形
        DrawRectangleRounded(r, roundness, 8, th.panel);
        DrawRectangleRoundedLines(r, roundness, 8, Fade(edge, 0.8f));
        DrawRectangleRoundedLines({ r.x + 6,r.y + 6,r.width - 12,r.height - 12 }, roundness, 8,
                                  Fade(th.accent, 0.30f));
        const Vector2 cn[4] = { {r.x + 14,r.y + 14},{r.x + r.width - 14,r.y + 14},
                                {r.x + 14,r.y + r.height - 14},{r.x + r.width - 14,r.y + r.height - 14} };
        for (int i = 0; i < 4; ++i) DrawPoly(cn[i], 4, 5.0f, gTime * 18 + i * 45, Fade(th.accent, 0.8f));
    } break;
    case BS_LEAF: {    // 苗圃：圆润木框 + 叶脉中线
        DrawRectangleRounded(r, std::max(roundness, 0.06f), 8, th.panel);
        DrawRectangleRoundedLines(r, std::max(roundness, 0.06f), 8, Fade(edge, 0.7f));
        DrawLineEx({ r.x + 10, r.y + r.height - 6 }, { r.x + r.width - 10, r.y + r.height - 6 },
                   1.5f, Fade(th.accent, 0.22f));
    } break;
    default: {         // 量子 / 熵潮：柔和圆角 + 顶部渐亮
        DrawRectangleRounded(r, roundness, 8, th.panel);
        DrawRectangleRoundedLines(r, roundness, 8, Fade(edge, 0.55f));
        DrawRectangleRounded({ r.x + 3, r.y + 2, r.width - 6, r.height * 0.10f }, 0.5f, 6,
                             Fade(RAYWHITE, 0.05f));
    } break;
    }
}
void uiPanel(Rectangle r, float roundness) {
    uiPanelAccent(r, curTheme().panelEdge, roundness);
}

// ============================================================
//  按钮：每种风格一套完全不同的造型
// ============================================================
// 引导锁：引导激活时，只有高亮目标区域内的控件可交互
bool guideAllows(Rectangle r) {
    if (!guideLock) return true;
    return CheckCollisionRecs(r, guideRect);
}

bool uiButton(Rectangle r, const string& label, Color base, bool active,
              float fs, bool enabled) {
    const ThemeStyle& th = curTheme();

    // ---------- 禁用态：统一压暗，保留形状语言 ----------
    if (!enabled) {
        Color off = Color{ 38,40,52,255 }, offEdge = Color{ 58,60,72,255 };
        switch (th.btn) {
        case BS_SHARP: case BS_ARMOR:
            drawCutRect(r, 10, off); drawCutRectLines(r, 10, 1.0f, offEdge); break;
        case BS_LINE:
            DrawRectangleRec(r, off);
            DrawRectangle((int)r.x, (int)r.y, 2, (int)r.height, offEdge); break;
        default:
            DrawRectangleRounded(r, th.round, 10, off);
            DrawRectangleRoundedLines(r, th.round, 10, offEdge); break;
        }
        Vector2 m = MeasureTextEx(font, label.c_str(), TS(fs), autoSpacing(TS(fs)));
        txt(label, r.x + (r.width - m.x) / 2, r.y + (r.height - m.y) / 2, fs,
            Color{ 96,100,112,255 });
        return false;
    }

    bool hover = !uiLock && CheckCollisionPointRec(gMouse, r) && guideAllows(r);
    // 悬停微放大：所有风格共用的反馈
    float ex = hover ? 3.0f : 0.0f;
    Rectangle rr = { r.x - ex, r.y - ex * 0.6f, r.width + ex * 2, r.height + ex * 1.2f };
    float fillA = active ? 0.88f : (hover ? 0.56f : 0.24f);
    Color fill = Fade(base, fillA);
    Color txtCol = hover || active ? RAYWHITE : th.text;

    switch (th.btn) {
    // ---------- 玄幻仙纹：卷角符箓，两侧仙纹与流转灵光 ----------
    case BS_RUNE: {
        if (hover || active)
            DrawRectangleRounded({ rr.x - 3,rr.y - 3,rr.width + 6,rr.height + 6 }, 0.10f, 8,
                                 Fade(th.accent, active ? 0.30f : 0.16f));
        DrawRectangleRounded(rr, 0.10f, 8, fill);
        DrawRectangleRoundedLines(rr, 0.10f, 8, Fade(RAYWHITE, active ? 0.8f : hover ? 0.5f : 0.22f));
        DrawRectangleRoundedLines({ rr.x + 4,rr.y + 4,rr.width - 8,rr.height - 8 }, 0.10f, 8,
                                  Fade(th.accent2, 0.30f));
        // 两侧云纹
        for (int s = 0; s < 2; ++s) {
            float cx = s ? rr.x + rr.width - 14 : rr.x + 14;
            DrawRing({ cx, rr.y + rr.height / 2 }, 4.5f, 6.2f,
                     gTime * 40 + s * 180, gTime * 40 + s * 180 + 230, 16,
                     Fade(th.accent2, active ? 0.9f : 0.5f));
        }
        // 顶部流转灵光
        if (hover || active) {
            float f = fmodf(gTime * 0.5f, 1.0f);
            DrawCircleV({ rr.x + 10 + f * (rr.width - 20), rr.y + 3 }, 2.4f,
                        Fade(th.accent2, 0.85f));
        }
    } break;

    // ---------- 黄铜蒸汽：斜面高光 + 四角铆钉 ----------
    case BS_BEVEL: {
        DrawRectangleRounded({ rr.x + 1,rr.y + 3,rr.width,rr.height }, 0.16f, 8,
                             Fade(BLACK, 0.35f));                    // 底部投影
        DrawRectangleRounded(rr, 0.16f, 8, fill);
        // 上亮下暗的金属斜面
        DrawRectangleRounded({ rr.x + 2, rr.y + 2, rr.width - 4, rr.height * 0.46f }, 0.34f, 8,
                             Fade(RAYWHITE, hover ? 0.20f : 0.12f));
        DrawRectangleRounded({ rr.x + 2, rr.y + rr.height * 0.62f, rr.width - 4, rr.height * 0.32f },
                             0.34f, 8, Fade(BLACK, 0.16f));
        DrawRectangleRoundedLines(rr, 0.16f, 8, Fade(th.accent2, active ? 1.0f : 0.7f));
        // 四角铆钉
        const float rx[4] = { 9, -9, 9, -9 }, ry[4] = { 8, 8, -8, -8 };
        for (int i = 0; i < 4; ++i) {
            Vector2 p = { rx[i] > 0 ? rr.x + rx[i] : rr.x + rr.width + rx[i],
                          ry[i] > 0 ? rr.y + ry[i] : rr.y + rr.height + ry[i] };
            DrawCircleV(p, 3.4f, Fade(Color{ 90,66,32,255 }, 0.95f));
            DrawCircleV({ p.x - 0.8f,p.y - 0.8f }, 1.5f, Fade(RAYWHITE, 0.7f));
        }
    } break;

    // ---------- 苔原苗圃：左右叶尖 + 叶脉 ----------
    case BS_LEAF: {
        if (hover || active)
            DrawRectangleRounded({ rr.x - 2,rr.y - 2,rr.width + 4,rr.height + 4 }, 0.34f, 10,
                                 Fade(th.accent, active ? 0.28f : 0.15f));
        DrawRectangleRounded(rr, 0.34f, 10, fill);
        // 左右叶尖
        float mid = rr.y + rr.height / 2;
        DrawTriangle({ rr.x - 7, mid }, { rr.x + 5, mid - 9 }, { rr.x + 5, mid + 9 },
                     Fade(base, fillA));
        DrawTriangle({ rr.x + rr.width + 7, mid }, { rr.x + rr.width - 5, mid + 9 },
                     { rr.x + rr.width - 5, mid - 9 }, Fade(base, fillA));
        DrawRectangleRoundedLines(rr, 0.34f, 10,
                                  Fade(RAYWHITE, active ? 0.75f : hover ? 0.45f : 0.16f));
        // 叶脉：中线 + 斜向支脉
        DrawLineEx({ rr.x + 12, mid }, { rr.x + rr.width - 12, mid },
                   1.2f, Fade(RAYWHITE, hover ? 0.20f : 0.10f));
        for (int i = 1; i < 5; ++i) {
            float px = rr.x + rr.width * i / 5.0f;
            DrawLineEx({ px, mid }, { px + 7, mid - 6 }, 1.0f, Fade(RAYWHITE, 0.10f));
            DrawLineEx({ px, mid }, { px + 7, mid + 6 }, 1.0f, Fade(RAYWHITE, 0.10f));
        }
    } break;

    // ---------- 赛博矩阵：切角 HUD + 转角括号 + 扫描线 ----------
    case BS_SHARP: {
        float cut = std::min(14.0f, rr.height * 0.42f);
        if (hover || active) {
            Rectangle g = { rr.x - 3,rr.y - 3,rr.width + 6,rr.height + 6 };
            drawCutRect(g, cut + 2, Fade(th.accent, active ? 0.26f : 0.14f));
        }
        drawCutRect(rr, cut, fill);
        drawCutRectLines(rr, cut, active ? 2.2f : 1.5f,
                         active ? th.accent : Fade(th.accent, hover ? 0.85f : 0.45f));
        // 转角括号
        float L = 14;
        Color br = Fade(th.accent, active ? 1.0f : 0.7f);
        DrawLineEx({ rr.x + cut, rr.y + 2 }, { rr.x + cut + L, rr.y + 2 }, 2.4f, br);
        DrawLineEx({ rr.x + rr.width - 2, rr.y + cut }, { rr.x + rr.width - 2, rr.y + cut - L + 6 },
                   2.4f, br);
        DrawLineEx({ rr.x + 2, rr.y + rr.height - cut }, { rr.x + 2, rr.y + rr.height - cut + L - 6 },
                   2.4f, br);
        // 内部电路走线
        DrawLineEx({ rr.x + 6, rr.y + rr.height - 5 }, { rr.x + rr.width * 0.32f, rr.y + rr.height - 5 },
                   1.4f, Fade(th.accent, 0.30f));
        // 扫描线：仅在悬停/选中时扫过
        if (hover || active) {
            float f = fmodf(gTime * 0.85f, 1.0f);
            float sy = rr.y + 3 + f * (rr.height - 6);
            DrawLineEx({ rr.x + 4, sy }, { rr.x + rr.width - 4, sy }, 1.6f,
                       Fade(th.accent2, 0.45f * (1 - fabsf(f - 0.5f) * 1.2f)));
        }
    } break;

    // ---------- 猫猫乐园：胶囊 + 顶部猫耳 + 爪印 ----------
    case BS_EAR: {
        float earR = std::min(12.0f, rr.height * 0.34f);
        // 猫耳（先画，让身体盖住耳根）
        Vector2 e1 = { rr.x + rr.width * 0.24f, rr.y + 2 };
        Vector2 e2 = { rr.x + rr.width * 0.76f, rr.y + 2 };
        Color earC = Fade(base, fillA + 0.12f);
        DrawTriangle({ e1.x - earR, e1.y + earR }, { e1.x, e1.y - earR * 0.95f },
                     { e1.x + earR, e1.y + earR }, earC);
        DrawTriangle({ e2.x - earR, e2.y + earR }, { e2.x, e2.y - earR * 0.95f },
                     { e2.x + earR, e2.y + earR }, earC);
        // 耳窝（粉色内耳）
        Color inner = Fade(Color{ 255,170,200,255 }, hover || active ? 0.9f : 0.5f);
        DrawTriangle({ e1.x - earR * 0.5f, e1.y + earR * 0.7f }, { e1.x, e1.y - earR * 0.35f },
                     { e1.x + earR * 0.5f, e1.y + earR * 0.7f }, inner);
        DrawTriangle({ e2.x - earR * 0.5f, e2.y + earR * 0.7f }, { e2.x, e2.y - earR * 0.35f },
                     { e2.x + earR * 0.5f, e2.y + earR * 0.7f }, inner);
        if (hover || active)
            DrawRectangleRounded({ rr.x - 3,rr.y - 2,rr.width + 6,rr.height + 5 }, 0.48f, 12,
                                 Fade(th.accent, active ? 0.28f : 0.15f));
        DrawRectangleRounded(rr, 0.48f, 12, fill);
        DrawRectangleRounded({ rr.x + 4, rr.y + 3, rr.width - 8, rr.height * 0.40f }, 0.5f, 10,
                             Fade(RAYWHITE, hover ? 0.16f : 0.08f));
        DrawRectangleRoundedLines(rr, 0.48f, 12,
                                  Fade(RAYWHITE, active ? 0.8f : hover ? 0.5f : 0.18f));
        // 左侧肉垫爪印
        Vector2 pw = { rr.x + 15, rr.y + rr.height / 2 };
        DrawEllipse((int)pw.x, (int)(pw.y + 2), 4.6f, 3.8f,
                    Fade(RAYWHITE, active ? 0.85f : 0.45f));
        for (int i = 0; i < 3; ++i) {
            float a = (-125 + i * 45) * DEG2RAD;
            DrawCircleV({ pw.x + cosf(a) * 6.4f, pw.y + sinf(a) * 6.4f - 1 }, 1.9f,
                        Fade(RAYWHITE, active ? 0.85f : 0.45f));
        }
        // 选中时尾巴轻摆
        if (active) {
            float sw = sinf(gTime * 3.2f) * 5;
            Vector2 t0 = { rr.x + rr.width - 4, rr.y + rr.height - 6 };
            DrawLineBezier(t0, { t0.x + 13, t0.y - 10 + sw }, 2.6f, Fade(base, 0.95f));
        }
    } break;

    // ---------- 熔岩机甲：切角装甲板 + 斜纹警条 ----------
    case BS_ARMOR: {
        float cut = std::min(12.0f, rr.height * 0.38f);
        drawCutRect({ rr.x + 1,rr.y + 3,rr.width,rr.height }, cut, Fade(BLACK, 0.40f));
        drawCutRect(rr, cut, fill);
        // 上部装甲高光
        DrawRectangle((int)(rr.x + 6), (int)(rr.y + 3), (int)(rr.width - 12),
                      (int)(rr.height * 0.30f), Fade(RAYWHITE, hover ? 0.13f : 0.07f));
        drawCutRectLines(rr, cut, active ? 2.4f : 1.6f,
                         active ? th.accent : Fade(th.accent, hover ? 0.85f : 0.45f));
        // 左侧斜纹警条
        for (int i = 0; i < 4; ++i) {
            float sx = rr.x + 8 + i * 6;
            DrawLineEx({ sx, rr.y + rr.height - 5 }, { sx + 5, rr.y + rr.height - 12 },
                       2.0f, Fade(th.accent2, active ? 0.75f : 0.35f));
        }
        // 选中时右侧熔流指示灯
        if (active) {
            float p = 0.5f + 0.5f * sinf(gTime * 5);
            DrawCircleV({ rr.x + rr.width - 12, rr.y + rr.height / 2 }, 3.2f + p,
                        Fade(Color{ 255,220,120,255 }, 0.85f));
        }
    } break;

    // ---------- 虚空极简：无填充，仅一线与大量留白 ----------
    case BS_LINE: {
        if (active) DrawRectangleRec(rr, Fade(base, 0.14f));
        else if (hover) DrawRectangleRec(rr, Fade(RAYWHITE, 0.045f));
        // 左侧强调线：选中时变粗变亮
        DrawRectangle((int)rr.x, (int)rr.y, active ? 4 : 2, (int)rr.height,
                      active ? th.accent : Fade(th.accent, hover ? 0.75f : 0.30f));
        // 底部细线，只在悬停时从左侧生长
        float w = active ? rr.width : (hover ? rr.width * 0.55f : 0);
        if (w > 0)
            DrawRectangle((int)rr.x, (int)(rr.y + rr.height - 1), (int)w, 1,
                          Fade(th.accent, 0.55f));
        txtCol = active ? RAYWHITE : hover ? th.text : th.textDim;
    } break;

    // ---------- 熵潮血月：胶囊 + 扩散涟漪 ----------
    case BS_PILL: {
        if (hover || active) {
            // 由中心向外的热寂涟漪
            float f = fmodf(gTime * 0.7f, 1.0f);
            DrawRectangleRoundedLines({ rr.x - f * 6, rr.y - f * 4,
                                        rr.width + f * 12, rr.height + f * 8 },
                                      0.5f, 10, Fade(th.accent, (1 - f) * 0.35f));
        }
        DrawRectangleRounded(rr, 0.40f, 10, fill);
        DrawRectangleRounded({ rr.x + 3, rr.y + 2, rr.width - 6, rr.height * 0.42f }, 0.5f, 8,
                             Fade(RAYWHITE, hover ? 0.12f : 0.06f));
        DrawRectangleRoundedLines(rr, 0.40f, 10,
                                  Fade(RAYWHITE, active ? 0.8f : hover ? 0.5f : 0.18f));
        if (active) {
            // 血月：左侧一枚缺月
            Vector2 mc = { rr.x + 15, rr.y + rr.height / 2 };
            DrawCircleV(mc, 5.5f, Fade(Color{ 255,150,150,255 }, 0.95f));
            DrawCircleV({ mc.x + 2.6f, mc.y - 1.2f }, 4.4f, Fade(base, 0.95f));
        }
    } break;

    // ---------- 宇宙尽头：金饰双线 + 环绕星点 ----------
    case BS_ORNATE: {
        if (hover || active)
            DrawRectangleRounded({ rr.x - 4,rr.y - 3,rr.width + 8,rr.height + 6 }, 0.20f, 10,
                                 Fade(th.accent, active ? 0.28f : 0.15f));
        DrawRectangleRounded(rr, 0.20f, 10, fill);
        DrawRectangleRounded({ rr.x + 3, rr.y + 2, rr.width - 6, rr.height * 0.44f }, 0.4f, 8,
                             Fade(RAYWHITE, hover ? 0.14f : 0.07f));
        DrawRectangleRoundedLines(rr, 0.20f, 10, Fade(th.accent, active ? 1.0f : 0.65f));
        DrawRectangleRoundedLines({ rr.x + 4,rr.y + 4,rr.width - 8,rr.height - 8 }, 0.20f, 10,
                                  Fade(RAYWHITE, 0.16f));
        // 四角金菱
        const Vector2 cn[4] = { {rr.x + 10,rr.y + 9},{rr.x + rr.width - 10,rr.y + 9},
                                {rr.x + 10,rr.y + rr.height - 9},{rr.x + rr.width - 10,rr.y + rr.height - 9} };
        for (int i = 0; i < 4; ++i)
            DrawPoly(cn[i], 4, 3.6f, gTime * 22 + i * 45, Fade(th.accent, active ? 0.95f : 0.55f));
        // 环绕星点
        if (hover || active)
            for (int i = 0; i < 3; ++i) {
                float f = fmodf(gTime * 0.32f + i / 3.0f, 1.0f);
                float px = rr.x + 8 + f * (rr.width - 16);
                DrawCircleV({ px, rr.y + 2.5f }, 1.9f, Fade(RAYWHITE, 0.75f));
                DrawCircleV({ rr.x + rr.width - 8 - f * (rr.width - 16), rr.y + rr.height - 2.5f },
                            1.9f, Fade(th.accent2, 0.7f));
            }
    } break;

    // ---------- 量子基调：柔和圆角 + 顶部高光（原始手感） ----------
    default: {
        if (hover || active)
            DrawRectangleRounded({ rr.x - 2,rr.y - 2,rr.width + 4,rr.height + 4 }, th.round, 10,
                                 Fade(base, active ? 0.42f : 0.26f));
        DrawRectangleRounded(rr, th.round, 10, fill);
        DrawRectangleRounded({ rr.x + 3, rr.y + 2, rr.width - 6, rr.height * 0.42f }, 0.5f, 8,
                             Fade(RAYWHITE, hover ? 0.10f : 0.05f));
        DrawRectangleRoundedLines(rr, th.round, 10,
                                  active ? Fade(RAYWHITE, 0.85f) : Fade(RAYWHITE, hover ? 0.55f : 0.16f));
        if (active)
            DrawRectangleRounded({ rr.x + 5, rr.y + rr.height * 0.28f, 4, rr.height * 0.44f },
                                 1.0f, 6, RAYWHITE);
    } break;
    }

    // ---------- 文字：统一带投影，保证在任何底色上可读 ----------
    float sp = autoSpacing(TS(fs));
    Vector2 m = MeasureTextEx(font, label.c_str(), TS(fs), sp);
    float tx = rr.x + (rr.width - m.x) / 2, ty = rr.y + (rr.height - m.y) / 2;
    // 极简风格文字左对齐，呼应其留白语言
    if (th.btn == BS_LINE) tx = rr.x + 16;
    // txt() 内部已做像素吸附，这里投影偏移取整数
    txt(label, tx + 1, ty + 1, fs, Fade(BLACK, 0.50f));
    txt(label, tx, ty, fs, txtCol);

    bool clicked = hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    if (clicked) playSfx(sfxClick);
    return clicked;
}
