// ============================================================
//  scenes.cpp  各界面（场景）绘制实现
//  【维护说明】成就墙卡片曾经把"已得装扮"文字与"豪华界面已解锁"文字
//  画在完全相同的坐标 (x+300, y+7)，导致终极成就那一行文字互相重叠。
//  这里改为：终极成就（ult）单独用一行合并显示，不再和装扮奖励文字叠加。
// ============================================================
#include "scenes.h"
#include "state.h"
#include "data.h"
#include "util.h"
#include "audio.h"
#include "save.h"
#include "achievements.h"
#include "render.h"
#include "game.h"
#include <cmath>
#include <algorithm>

// ============================================================
//  新手引导：高亮圈出要点的位置 + 其余区域变暗 + 文字/箭头提示
// ============================================================
// 压暗除目标外的四个区域（比整屏压暗再"挖洞"更简单可靠）
static void dimExcept(Rectangle r) {
    Color dim = Fade(BLACK, 0.62f);
    if (r.y > 0) DrawRectangle(0, 0, VW, (int)r.y, dim);
    if (r.y + r.height < VH) DrawRectangle(0, (int)(r.y + r.height), VW, VH - (int)(r.y + r.height), dim);
    if (r.x > 0) DrawRectangle(0, (int)r.y, (int)r.x, (int)r.height, dim);
    if (r.x + r.width < VW) DrawRectangle((int)(r.x + r.width), (int)r.y, VW - (int)(r.x + r.width), (int)r.height, dim);
}

// 高亮目标 + 提示气泡 + 指向箭头
static void drawGuideFocus(Rectangle r, const string& text, float t) {
    dimExcept(r);
    float pulse = 0.5f + 0.5f * sinf(t * 4.0f);
    DrawRectangleLinesEx(r, 3.0f, Fade(GOLD, 0.75f + 0.25f * pulse));
    DrawRectangleRoundedLines({ r.x - 5 - pulse * 7, r.y - 5 - pulse * 7,
                                r.width + 10 + pulse * 14, r.height + 10 + pulse * 14 },
                              0.25f, 8, Fade(GOLD, 0.55f * (1.0f - pulse)));

    float fs = 21.0f;
    Vector2 m = MeasureTextEx(font, text.c_str(), TS(fs), 1);
    float pad = 16.0f, bh = 46.0f;
    float bw = m.x + pad * 2;
    float bx = std::clamp(r.x + r.width / 2 - bw / 2, 20.0f, VW - bw - 20.0f);
    float by = (r.y > 120.0f) ? r.y - bh - 18.0f : r.y + r.height + 18.0f;
    by = std::clamp(by, 60.0f, VH - bh - 16.0f);
    DrawRectangleRounded({ bx, by, bw, bh }, 0.28f, 8, Fade(Color{ 24,28,44,255 }, 0.97f));
    DrawRectangleRoundedLines({ bx, by, bw, bh }, 0.28f, 8, GOLD);
    txt(text, bx + pad, by + (bh - m.y) / 2, fs, RAYWHITE);

    // 箭头：在气泡与目标之间，指向目标
    float acx = r.x + r.width / 2;
    float ah = 9.0f + pulse * 4.0f;
    if (by < r.y) { float ay = by + bh + 3; DrawTriangle({ acx, ay + ah }, { acx - 9, ay }, { acx + 9, ay }, GOLD); }
    else          { float ay = by - 3;        DrawTriangle({ acx, ay - ah }, { acx - 9, ay }, { acx + 9, ay }, GOLD); }
}

// 棋盘区域整体矩形
static Rectangle boardRect() {
    return { (float)GRID_X, (float)GRID_Y,
             (float)(SIZE * CELL + (SIZE - 1) * GAP), (float)(SIZE * CELL + (SIZE - 1) * GAP) };
}
// 优先圈出"最该操作"的格子：圆满种子 -> 任意种子/纠缠 -> 棋盘整体
static Rectangle guideCellRect() {
    for (int i = 0; i < SIZE * SIZE; ++i)
        if (grid[i].st == SEED && grid[i].mat >= 3) return cellRect(i);
    for (int i = 0; i < SIZE * SIZE; ++i)
        if (grid[i].st == SEED || grid[i].st == ENTANGLED) return cellRect(i);
    return boardRect();
}
// 圈出"一颗可纠缠的种子"（成熟优先），可排除已选中的 except 格，避免重复点同一颗
static Rectangle guideSeedCellExcept(int except) {
    for (int i = 0; i < SIZE * SIZE; ++i)
        if (i != except && grid[i].st == SEED && grid[i].mat >= 3) return cellRect(i);
    for (int i = 0; i < SIZE * SIZE; ++i)
        if (i != except && grid[i].st == SEED) return cellRect(i);
    return boardRect();
}
// 圈出"纠缠中的种子"（观测它才会触发同步坍缩，不能圈成熟但未纠缠的普通种子）
static Rectangle guideEntangledCell() {
    for (int i = 0; i < SIZE * SIZE; ++i)
        if (grid[i].st == ENTANGLED) return cellRect(i);
    return boardRect();
}
// 场上是否还有"可操作"的种子/纠缠（灵能花是被动产出、不可点击，不计入）
static bool boardHasSeed() {
    for (auto& c : grid) if (c.st == SEED || c.st == ENTANGLED) return true;
    return false;
}
static int boardMaxMat() {
    int m = -1;
    for (auto& c : grid) if (c.st == SEED && c.mat > m) m = c.mat;
    return m;
}
static bool boardHasEntangled() {
    for (auto& c : grid) if (c.st == ENTANGLED) return true;
    return false;
}

// 教程内的分步指引：返回 true 表示当前应显示引导
// 说明：全部基于当前局面"无状态"推导，不需要额外存档字段，怎么点都不会错位。
static bool tutorialGuide(Rectangle& outR, string& outT) {
    if (mode != M_TUTORIAL) return false;
    Rectangle rPlant = { PX, 180, 200, 46 };
    Rectangle rObs   = { PX + 215, 180, 200, 46 };
    Rectangle rEnt   = { PX + 430, 180, 200, 46 };
    Rectangle rMed   = { PX, 238, 630, 42 };
    Rectangle rShop  = { PX, 290, 630, 46 };

    switch (tutIdx) {
    case 0:   // 观测
        if (tool != T_OBSERVE) { outR = rObs; outT = "第 1 步：点【观测】选择行动"; return true; }
        if (statObs < 3) {
            outR = guideCellRect();
            outT = "第 2 步：点棋盘上的种子让它坍缩（" + to_string(statObs) + "/3）";
            return true;
        }
        break;
    case 1:   // 种植与成熟度
        if (!boardHasSeed()) {
            if (tool != T_PLANT) { outR = rPlant; outT = "第 1 步：点【种植】选择种植"; }
            else                 { outR = boardRect(); outT = "第 2 步：点任意空地，花 10 灵能种下种子"; }
            return true;
        }
        if (boardMaxMat() < 3) { outR = rMed; outT = "第 3 步：点【冥想】推进回合，让种子长大（+6 灵能）"; return true; }
        if (statMatObs < 2) {
            if (tool != T_OBSERVE) { outR = rObs; outT = "第 4 步：点【观测】准备收割"; }
            else                   { outR = guideCellRect(); outT = "第 5 步：点【圆满】的种子收割（" + to_string(statMatObs) + "/2）"; }
            return true;
        }
        break;
    case 2:   // 连击与量子藤
        if (tool != T_OBSERVE) { outR = rObs; outT = "点【观测】，连续拿好结果就能叠【连击】"; return true; }
        if (maxCombo < 3) { outR = guideCellRect(); outT = "盯住成熟的种子观测，叠满 3 连击即可通关（" + to_string(maxCombo) + "/3）"; return true; }
        break;
    case 3:   // 灵能花
        if (!boardHasSeed()) {
            if (tool != T_PLANT) { outR = rPlant; outT = "先点【种植】把种子铺上场"; }
            else                 { outR = boardRect(); outT = "点空地种下种子"; }
            return true;
        }
        if (flowers < 2) {
            if (tool != T_OBSERVE) { outR = rObs; outT = "点【观测】"; }
            else                   { outR = guideCellRect(); outT = "观测成熟种子，直到开出 2 朵灵能花（" + to_string(flowers) + "/2）"; }
            return true;
        }
        break;
    case 4:   // 量子纠缠
        if (!boardHasEntangled()) {
            if (tool != T_ENTANGLE) { outR = rEnt; outT = "第 1 步：点【纠缠】"; return true; }
            if (entSel < 0)         { outR = guideSeedCellExcept(-1);    outT = "第 2 步：点这颗高亮的种子（第一颗）"; return true; }
                                    { outR = guideSeedCellExcept(entSel); outT = "第 3 步：点另一颗高亮的种子，完成纠缠"; return true; }
        }
        if (statEntSync < 2) {
            if (tool != T_OBSERVE) { outR = rObs; outT = "第 4 步：点【观测】"; }
            else                   { outR = guideEntangledCell(); outT = "观测这颗高亮的纠缠种子，触发同步坍缩（" + to_string(statEntSync) + "/2）"; }
            return true;
        }
        break;
    case 5:   // 商店
        if (statBuy < 2) { outR = rShop; outT = "点【进入量子商店】选购商品，买满 2 件即可通关（" + to_string(statBuy) + "/2）"; return true; }
        break;
    case 6:   // 第 7 关：风暴与退相干（自主应对，给基础指路）
        if (!boardHasSeed()) {
            if (tool != T_PLANT) { outR = rPlant; outT = "点【种植】铺开局面"; }
            else                 { outR = boardRect(); outT = "点空地种下种子"; }
            return true;
        }
        if (tool != T_OBSERVE) { outR = rObs; outT = "点【观测】收割成熟种子"; return true; }
        outR = guideCellRect(); outT = "成熟了要及时收割，保持场上有花持续产出"; return true;
    case 7:   // 第 8 关：毕业考，不加引导，让玩家自由发挥
        break;
    }
    return false;
}

// ==================== 场景：主菜单 ====================
void sceneMenu(float t) {
    const ThemeStyle& th = curTheme();
    // 首次进入游戏的新手引导：锁定交互，只允许点击高亮的"新手引导"按钮
    bool menuGuide = (!rec.guideMenuDone && rec.tutProgress < 8);
    if (menuGuide) { guideLock = true; guideRect = { 40,596,250,42 }; }
    txtTitle("量 子 花 园", VW / 2.0f, 26, 58, th.title);
    txtSC(string("Quantum Garden 6.3　·　") + th.label, VW / 2.0f, 96, 20, th.textDim);

    float bx = VW / 2.0f - 200;
    if (uiButton({ bx,132,400,48 }, "新 手 教 程   8 关，从零学会", GREEN, false, 23)) scene = TUT_SEL;
    for (int i = 0; i < 4; ++i) {
        string s = string(CFG[i].name) + "   " + to_string(CFG[i].turns) + " 回合 / 目标 " + to_string(CFG[i].target);
        Color c = i < 2 ? SKYBLUE : i == 2 ? ORANGE : RED;
        if (uiButton({ bx,192.0f + i * 50,400,42 }, s, c, false, 21)) resetGame(M_CLASSIC, i);
    }
    if (uiButton({ bx,396,400,46 }, "无 尽 模 式   生存到极限", VIOLET, false, 22)) resetGame(M_ENDLESS, 3);
    bool doneToday = (rec.dailyDay == todayCode() && rec.dailyDone == 1);
    if (uiButton({ bx,452,400,46 },
                 doneToday ? "每日挑战   今日已完成，可重玩" : "每 日 挑 战   全球同一牌局",
                 doneToday ? DARKGRAY : GOLD, false, 22)) resetGame(M_DAILY, 1);

    if (uiButton({ bx,512,128,40 }, "规则 (H)", DARKGRAY, false, 19)) { prevScene = MENU; scene = HELP; }
    if (uiButton({ bx + 136,512,128,40 }, "成就墙", GOLD, false, 19)) scene = ACHIEVE;
    // 新成就红点：已解锁数量超过"看过"的数量时，在成就墙按钮右上角提示
    {
        int achDone = 0;
        for (int i = 0; i < A_COUNT; ++i) if (rec.achUnlocked[i]) ++achDone;
        if (achDone > rec.achSeen) {
            float px = bx + 136 + 128 - 9, py = 512 + 9;
            DrawCircleV({ px,py }, 9, RED);
            DrawCircleV({ px,py }, 9, Fade(RAYWHITE, 0.35f + 0.35f * sinf(t * 5)));
            txtC(to_string(achDone - rec.achSeen), px, py - 8, 13, RAYWHITE);
        }
    }
    if (uiButton({ bx + 272,512,128,40 }, "星尘工坊", VIOLET, false, 19)) scene = META;
    if (uiButton({ bx,560,196,40 }, "数据中心", SKYBLUE, false, 19)) scene = STATS;
    if (uiButton({ bx + 204,560,196,40 }, "全屏 (F11)", DARKGRAY, false, 19)) toggleFull();
    if (uiButton({ bx,608,400,36 }, "关于 / 用户许可协议", DARKGRAY, false, 17)) { prevScene = MENU; scene = LICENSE; }

    // 左侧面板下方：存档管理入口 + 新手引导入口
    int usedSlots = usedSlotCount();
    if (uiButton({ 40,548,250,42 },
                 usedSlots > 0 ? ("继续游戏 / 存档管理 " + to_string(usedSlots) + "/10")
                               : "存档管理（暂无存档）",
                 usedSlots > 0 ? GREEN : DARKGRAY, false, 17)) {
        slotsMode = 1; prevScene = MENU; scene = SLOTS;
        playSfx(sfxClick);
    }
    Rectangle guideBtn = { 40, 596, 250, 42 };
    if (rec.tutProgress < 8) {
        string gt = rec.tutProgress == 0 ? "新 手 引 导（推荐）"
                                         : "继续新手引导 " + to_string(rec.tutProgress + 1) + " / 8";
        if (uiButton(guideBtn, gt, Color{ 60,170,110,255 }, false, 19)) {
            tutIdx = std::clamp(rec.tutProgress, 0, 7);
            rec.guideMenuDone = true; saveRecords();
            scene = TUT_BRIEF;
            playSfx(sfxClick);
        }
    }
    // 首次进入游戏的引导遮罩在函数末尾统一绘制（保证盖在所有元素之上）

    // 关于 / 开源许可（游戏版权、Apache 2.0、字体 OFL）
    if (uiButton({ 40, 644, 250, 42 }, "关于 / 开源许可", SKYBLUE, false, 18)) {
        aboutPage = 0; aboutScroll = 0; scene = ABOUT; playSfx(sfxClick);
    }

    uiPanel({ 40,132,250,400 }, 0.06f);
    txtS("历史最佳记录", 62, 143, 22, GOLD);
    txt("教程进度：" + to_string(rec.tutProgress) + " / 8 关", 62, 175, 18, GREEN);
    for (int i = 0; i < 4; ++i)
        txt(string(CFG[i].name) + "：" + (rec.bestClassic[i] ? to_string(rec.bestClassic[i]) : "暂无"),
            62, 201.0f + i * 24, 18, RAYWHITE);
    txt("无尽回合：" + (rec.bestEndlessTurn ? to_string(rec.bestEndlessTurn) : "暂无"), 62, 301, 18, VIOLET);
    txt("每日最佳：" + (rec.dailyBest ? to_string(rec.dailyBest) : "暂无"), 62, 325, 18, SKYBLUE);
    txt("连续天数：" + to_string(rec.dailyStreak) + " 天", 62, 349, 18, SKYBLUE);
    txt("最高连击：x" + to_string(rec.bestCombo), 62, 373, 18, PINK);
    txt("薛定谔之猫：" + to_string(rec.totalCats), 62, 397, 18, GOLD);
    int adone = 0; for (int i = 0; i < A_COUNT; ++i) if (rec.achUnlocked[i]) ++adone;
    txt("成就：" + to_string(adone) + " / " + to_string(A_COUNT), 62, 421, 18, GOLD);
    txt("星尘：" + to_string(rec.stardust), 62, 445, 18, VIOLET);
    if (rec.luxUnlocked)
        txt("宇宙尽头已解锁", 62, 473, 18, Fade(GOLD, 0.65f + 0.35f * sinf(t * 3)));
    else
        txt("终极成就：未解锁", 62, 473, 18, GRAY);

    txt("最近战绩曲线", VW - 350.0f, 138, 21, SKYBLUE);
    drawCurve({ VW - 350.0f, 164, 310, 186 }, false);
    drawFlower({ VW - 195.0f, 440 }, 3, t);
    drawSeed({ VW - 195.0f, 552 }, (int)(fmodf(t, 4.0f)), t);

    if (menuGuide)
        drawGuideFocus({ 40,596,250,42 }, "欢迎来到量子花园！点这里开始【新手引导】，跟着提示一步步学", t);
}

// ==================== 场景：规则 ====================
void sceneHelp() {
    uiPanel({ 50,20,VW - 100.0f,VH - 60.0f }, 0.02f);
    txtTitle("游 戏 规 则 手 册", VW / 2.0f, 32, 34, curTheme().title);
    for (int i = 0; i < 6; ++i)
        if (uiButton({ 268.0f + i * 130,72,122,36 }, HPAGES[i].tab, SKYBLUE, helpPage == i, 19))
            helpPage = i;
    const HPage& p = HPAGES[helpPage];
    for (int i = 0; i < p.n; ++i)
        txt(p.l[i].s, p.l[i].x, p.l[i].y, p.l[i].size, HCOL[p.l[i].c]);
    if (uiButton({ 90,68,150,36 }, "返回 (H)", DARKGRAY, false, 20)) scene = prevScene;
}

// ==================== 场景：最终用户许可协议（EULA） ====================
// 兼容两种入场方式：
//  1) 首次启动强制门槛（rec.licenseAgreed == false）：不显示"返回"，
//     必须翻到最后一页才会出现"同意并继续"/"不同意，退出游戏"两个按钮，
//     同意后立即写盘保存。中途翻页不允许用 H 键或其他方式跳过（main.cpp 已拦截）。
//  2) 之后从主菜单"关于/许可协议"按钮进入回看：随时显示"返回"，翻页自由，
//     不重复弹同意逻辑。
void sceneLicense() {
    licensePage = std::clamp(licensePage, 0, LICENSE_PAGE_N - 1);
    const LicPage& p = LICENSE_PAGES[licensePage];
    bool lastPage = (licensePage == LICENSE_PAGE_N - 1);

    uiPanel({ 90,36,VW - 180.0f,VH - 92.0f }, 0.02f);
    txtTitle("最终用户许可协议", VW / 2.0f, 50, 30, curTheme().title);
    txtSC("《量子花园》 Quantum Garden　·　第 " + to_string(licensePage + 1) + " / " +
          to_string(LICENSE_PAGE_N) + " 页", VW / 2.0f, 92, 18, SKYBLUE);
    // 页标题（如"重要提示"）单独做成醒目大字，避免只看小字而漏掉
    txtTitle(p.title, VW / 2.0f, 116, 24, GOLD, false);

    // 逐行数组 + 运行时按行距自动步进（不依赖手写坐标，条款再长也不会重叠）
    float y = 158;
    for (int i = 0; i < p.n; ++i) {
        const LicLine& l = p.l[i];
        switch (l.kind) {
        case LK_SECTION: txt(l.s, 130, y, 23, GOLD);              y += 34; break;
        case LK_SUB:     txt(l.s, 150, y, 21, SKYBLUE);           y += 30; break;
        case LK_NOTE:    txt(l.s, 130, y, 19, GRAY);              y += 28; break;
        case LK_GAP:     y += 12; break;
        default:         txt(l.s, 150, y, 19, RAYWHITE);          y += 26; break;
        }
    }

    // 翻页
    if (uiButton({ 130,VH - 118.0f,140,40 }, "上一页", DARKGRAY, false, 19, licensePage > 0))
        --licensePage;
    if (uiButton({ VW - 270.0f,VH - 118.0f,140,40 }, "下一页", DARKGRAY, false, 19, !lastPage))
        ++licensePage;

    if (!rec.licenseAgreed) {
        if (!lastPage) {
            txt("请翻阅至最后一页后再作出选择。", VW / 2.0f - 220, VH - 62.0f, 19, GRAY);
        } else {
            if (uiButton({ VW / 2.0f - 230,VH - 68.0f,220,44 }, "同意并继续", GREEN, false, 21)) {
                rec.licenseAgreed = true;
                saveRecords();
                licensePage = 0;
                scene = MENU;
                playSfx(sfxClick);
            }
            if (uiButton({ VW / 2.0f + 10,VH - 68.0f,220,44 }, "不同意，退出游戏", RED, false, 21)) {
                shouldQuit = true;
            }
        }
    } else {
        if (uiButton({ VW / 2.0f - 90,VH - 68.0f,180,44 }, "返回", DARKGRAY, false, 21)) {
            licensePage = 0;
            scene = prevScene;
        }
    }
}

// ==================== 场景：教程 ====================
void sceneTutSel() {
    txtTitle("新 手 教 程", VW / 2.0f, 50, 46, GREEN);
    txtC("选择关卡（按顺序解锁）　　已通关 " + to_string(rec.tutProgress) + " / 8 关",
         VW / 2.0f, 112, 24, RAYWHITE);
    for (int i = 0; i < 8; ++i) {
        float x = 140.0f + (i % 2) * 520, y = 170.0f + (i / 2) * 100;
        bool unlocked = (i <= rec.tutProgress);
        bool done = (i < rec.tutProgress);
        DrawRectangleRounded({ x,y,460,80 }, 0.1f, 8,
            unlocked ? Color{ 34,40,58,255 } : Color{ 26,28,36,255 });
        txt(TUT[i].title, x + 20, y + 14, 22, unlocked ? RAYWHITE : Color{ 92,96,108,255 });
        txt(done ? "已通关" : (unlocked ? "点击开始" : "未解锁"), x + 20, y + 46, 19,
            done ? GOLD : (unlocked ? GREEN : GRAY));
        if (unlocked && CheckCollisionPointRec(gMouse, { x,y,460,80 }) && guideAllows({ x,y,460,80 })) {
            DrawRectangleLinesEx({ x,y,460,80 }, 2, GREEN);
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { playSfx(sfxClick); tutIdx = i; scene = TUT_BRIEF; }
        }
    }
    if (uiButton({ VW / 2.0f - 110,620,220,46 }, "返回主菜单", DARKGRAY)) scene = MENU;
}
void sceneTutBrief() {
    uiPanel({ 160,60,VW - 320.0f,VH - 140.0f }, 0.03f);
    txtTitle(TUT[tutIdx].title, VW / 2.0f, 84, 34, GREEN);
    txt("本关要教你的", 220, 148, 24, GOLD);
    float y = 188;
    for (int i = 0; i < 6; ++i)
        if (TUT[tutIdx].teach[i][0]) { txt(TUT[tutIdx].teach[i], 240, y, 21, RAYWHITE); y += 32; }
    y += 14;
    txt("本关目标", 220, y, 24, GOLD); y += 36;
    txt(TUT[tutIdx].goalDesc, 240, y, 24, SKYBLUE); y += 36;
    txt(string("提示：") + TUT[tutIdx].tip, 240, y, 20, GRAY);
    if (uiButton({ VW / 2.0f - 230,VH - 130.0f,220,48 }, "开始挑战", GREEN)) resetGame(M_TUTORIAL, 1);
    if (uiButton({ VW / 2.0f + 10,VH - 130.0f,220,48 }, "返回", DARKGRAY)) scene = TUT_SEL;
}

// ==================== 场景：成就墙 ====================
// achPage 已在全局状态区声明：0=全部 1=铜 2=银 3=金 4=彩虹 5=隐藏

void sceneAchieve(float t) {
    uiPanel({ 40,12,VW - 80.0f,VH - 34.0f }, 0.02f);
    if (luxActive())
        DrawRectangleLinesEx({ 40,12,VW - 80.0f,VH - 34.0f }, 2, Fade(GOLD, 0.45f));

    int done = 0, tierDone[4] = { 0,0,0,0 }, tierAll[4] = { 0,0,0,0 }, hidDone = 0, hidAll = 0;
    for (int i = 0; i < A_COUNT; ++i) {
        if (ACH[i].hidden) { ++hidAll; if (rec.achUnlocked[i]) ++hidDone; }
        ++tierAll[ACH[i].tier];
        if (rec.achUnlocked[i]) { ++done; ++tierDone[ACH[i].tier]; }
    }
    txtTitle("成 就 墙", VW / 2.0f, 22, 32, GOLD);
    txtC("已解锁 " + to_string(done) + " / " + to_string(A_COUNT)
         + "　　铜 " + to_string(tierDone[0]) + "/" + to_string(tierAll[0])
         + "　银 " + to_string(tierDone[1]) + "/" + to_string(tierAll[1])
         + "　金 " + to_string(tierDone[2]) + "/" + to_string(tierAll[2])
         + "　彩虹 " + to_string(tierDone[3]) + "/" + to_string(tierAll[3])
         + "　隐藏 " + to_string(hidDone) + "/" + to_string(hidAll),
         VW / 2.0f, 62, 19, RAYWHITE);
    DrawRectangleRounded({ 400,90,480,10 }, 1.0f, 8, Color{ 44,48,66,255 });
    DrawRectangleRounded({ 400,90,480 * (done / (float)A_COUNT),10 }, 1.0f, 8, GOLD);

    // 进入成就墙即视为"已查看"：清除主菜单的新成就红点
    if (rec.achSeen != done) { rec.achSeen = done; saveRecords(); }

    // 分类页签
    const char* tabs[6] = { "全部","铜级","银级","金级","彩虹级","隐藏" };
    for (int i = 0; i < 6; ++i) {
        Color tc = i == 0 ? SKYBLUE : i == 5 ? VIOLET : tierColor(i - 1, t);
        if (uiButton({ 300.0f + i * 116,110,108,32 }, tabs[i], tc, achPage == i, 18))
            achPage = i;
    }

    // 筛选
    std::vector<int> list;
    for (int i = 0; i < A_COUNT; ++i) {
        if (achPage == 5) { if (ACH[i].hidden) list.push_back(i); }
        else if (achPage == 0) { if (!ACH[i].hidden) list.push_back(i); }
        else if (!ACH[i].hidden && ACH[i].tier == achPage - 1) list.push_back(i);
    }
    // 终极成就置顶展示
    if (achPage == 0 || achPage == 4) {
        for (size_t i = 0; i < list.size(); ++i)
            if (list[i] == A_ENDLESS1000) { list.erase(list.begin() + i); list.insert(list.begin(), A_ENDLESS1000); break; }
    }

    float gx = 66, gy = 156, cw = 573, ch = 62;
    int rows = 8;
    for (size_t n = 0; n < list.size() && n < (size_t)(rows * 2); ++n) {
        int i = list[n];
        float x = gx + (n % 2) * (cw + 12), y = gy + (n / 2) * (ch + 4);
        bool ok = rec.achUnlocked[i];
        bool ult = (i == A_ENDLESS1000);
        Color col = tierColor(ACH[i].tier, t);
        DrawRectangleRounded({ x,y,cw,ch }, 0.14f, 8,
            ok ? Color{ 36,42,60,255 } : Color{ 25,27,35,255 });
        if (ok) {
            float gl = ACH[i].tier == T_RAINBOW ? (2.2f + 0.9f * sinf(t * 3)) : 1.6f;
            DrawRectangleLinesEx({ x,y,cw,ch }, gl, Fade(col, ult ? 0.95f : 0.55f));
        } else if (ult)
            DrawRectangleLinesEx({ x,y,cw,ch }, 1.5f, Fade(GOLD, 0.25f));

        drawMedal({ x + 34, y + 31 }, 19, ACH[i].tier, ok, t);
        txt(ACH[i].name, x + 64, y + 6, 20, ok ? RAYWHITE : Color{ 110,114,126,255 });
        // 等级标签
        Vector2 nw = MeasureTextEx(font, ACH[i].name, TS(20), 1);
        DrawRectangleRounded({ x + 70 + nw.x, y + 9, 40, 18 }, 0.4f, 6, Fade(col, ok ? 0.85f : 0.20f));
        txtC(TIER_NAME[ACH[i].tier], x + 90 + nw.x, y + 10, 15,
             ok ? Color{ 24,26,34,255 } : Color{ 120,124,136,255 });
        txt(ok ? ACH[i].desc : (ACH[i].hidden ? "隐藏成就：条件未知，传闻就藏在花园里" : ACH[i].desc),
            x + 64, y + 30, 16, ok ? GRAY : Color{ 82,86,98,255 });
        txt(ACH[i].hidden && !ok ? "???" : ("条件：" + string(ACH[i].cond)),
            x + 300, y + 30, 16, ok ? Fade(col, 0.9f) : Color{ 92,96,108,255 });

        // 修复：终极成就（ult）曾把"已得装扮"与"豪华界面已解锁"两行文字画在
        // 同一坐标 (x+300, y+7) 上导致重叠。现在改为二者合并为一行显示，
        // 非终极成就的卡片则照常只显示装扮奖励文字，互不冲突。
        if (ult) {
            string ultLabel = ok
                ? ("★ 豪华界面已解锁" + (ACH[i].unlockSkin >= 0 ? "　已得：" + string(SKINS[ACH[i].unlockSkin].name) : ""))
                : "★ 解锁星系豪华界面";
            txt(ultLabel, x + 300, y + 7, 15,
                Fade(ColorFromHSV(fmodf(t * 80, 360.0f), 0.7f, 1.0f), ok ? 1.0f : 0.6f));
        } else if (ACH[i].unlockSkin >= 0) {
            txt(ok ? ("已得：" + string(SKINS[ACH[i].unlockSkin].name))
                   : (ACH[i].hidden ? "奖励：???" : "奖励：" + string(SKINS[ACH[i].unlockSkin].name)),
                x + 300, y + 7, 16, ok ? GOLD : Color{ 96,100,112,255 });
        }
    }

    txt("成就分为铜 → 银 → 金 → 彩虹四级，同系列条件逐级提升；隐藏成就需自行探索。",
        66, VH - 60.0f, 18, GRAY);
    if (uiButton({ VW - 470.0f,VH - 66.0f,200,40 }, "装扮衣橱", GOLD, false, 20)) scene = SKINSEL;
    if (uiButton({ VW - 258.0f,VH - 66.0f,190,40 }, "返回主菜单", DARKGRAY, false, 20)) scene = MENU;
}

// ==================== 场景：装扮衣橱 ====================
// 风格缩影：在一小块预览区内画出该风格最具辨识度的元素，
// 让玩家在选择前就能看出"科技风 / 玄幻风 / 猫猫风"的差别。
static void drawThemeChip(Rectangle pv, const ThemeStyle& ps, float t) {
    float cx = pv.x + pv.width * 0.5f, cy = pv.y + pv.height * 0.5f;
    // 迷你按钮：用该风格的形状语言
    Rectangle mb = { pv.x + 8, cy - 8, pv.width - 16, 16 };
    switch (ps.btn) {
    case BS_RUNE: {   // 玄幻：符箓 + 云纹
        DrawRectangleRounded(mb, 0.15f, 6, Fade(ps.accent, 0.45f));
        DrawRectangleRoundedLines(mb, 0.15f, 6, Fade(ps.accent2, 0.9f));
        DrawRing({ mb.x + 7, cy }, 3, 4.2f, t * 40, t * 40 + 230, 12, Fade(ps.accent2, 0.9f));
        DrawRing({ mb.x + mb.width - 7, cy }, 3, 4.2f, -t * 40, -t * 40 + 230, 12,
                 Fade(ps.accent2, 0.9f));
    } break;
    case BS_BEVEL: {  // 蒸汽：齿轮 + 铆钉按钮
        DrawRectangleRounded(mb, 0.2f, 6, Fade(ps.accent, 0.5f));
        DrawRectangleRounded({ mb.x + 1,mb.y + 1,mb.width - 2,mb.height * 0.45f }, 0.4f, 5,
                             Fade(RAYWHITE, 0.22f));
        DrawRectangleRoundedLines(mb, 0.2f, 6, Fade(ps.accent2, 0.95f));
        for (int i = 0; i < 2; ++i)
            DrawCircleV({ i ? mb.x + mb.width - 5 : mb.x + 5, cy }, 2.0f,
                        Fade(Color{ 90,66,32,255 }, 0.95f));
        // 角落齿轮
        Vector2 g = { pv.x + pv.width - 11, pv.y + 10 };
        DrawCircleLines((int)g.x, (int)g.y, 6, Fade(ps.accent2, 0.7f));
        for (int i = 0; i < 8; ++i) {
            float a = t * 0.5f + i * PI / 4;
            DrawLineEx({ g.x + cosf(a) * 6, g.y + sinf(a) * 6 },
                       { g.x + cosf(a) * 8.5f, g.y + sinf(a) * 8.5f }, 1.8f,
                       Fade(ps.accent2, 0.7f));
        }
    } break;
    case BS_LEAF: {   // 苗圃：叶片按钮
        DrawRectangleRounded(mb, 0.45f, 8, Fade(ps.accent, 0.5f));
        DrawTriangle({ mb.x - 5, cy }, { mb.x + 4, cy - 6 }, { mb.x + 4, cy + 6 },
                     Fade(ps.accent, 0.5f));
        DrawTriangle({ mb.x + mb.width + 5, cy }, { mb.x + mb.width - 4, cy + 6 },
                     { mb.x + mb.width - 4, cy - 6 }, Fade(ps.accent, 0.5f));
        DrawLineEx({ mb.x + 8, cy }, { mb.x + mb.width - 8, cy }, 1.0f, Fade(RAYWHITE, 0.25f));
    } break;
    case BS_SHARP: {  // 科技：切角 HUD + 网格
        for (int i = 0; i < 4; ++i)
            DrawLine((int)pv.x, (int)(pv.y + 6 + i * 10), (int)(pv.x + pv.width),
                     (int)(pv.y + 6 + i * 10), Fade(ps.accent, 0.10f));
        Vector2 p[6] = { {mb.x,mb.y + 6},{mb.x,mb.y + mb.height},
                         {mb.x + mb.width - 6,mb.y + mb.height},{mb.x + mb.width,mb.y + mb.height - 6},
                         {mb.x + mb.width,mb.y},{mb.x + 6,mb.y} };
        DrawTriangleFan(p, 6, Fade(ps.accent, 0.40f));
        for (int i = 0; i < 6; ++i)
            DrawLineEx(p[i], p[(i + 1) % 6], 1.3f, Fade(ps.accent, 0.95f));
        float f = fmodf(t * 0.8f, 1.0f);
        DrawLineEx({ mb.x + 2, mb.y + f * mb.height }, { mb.x + mb.width - 2, mb.y + f * mb.height },
                   1.2f, Fade(ps.accent2, 0.55f));
    } break;
    case BS_EAR: {    // 猫猫：猫耳按钮 + 爪印
        float er = 6.0f;
        Vector2 e1 = { mb.x + mb.width * 0.28f, mb.y + 1 }, e2 = { mb.x + mb.width * 0.72f, mb.y + 1 };
        DrawTriangle({ e1.x - er,e1.y + er }, { e1.x,e1.y - er }, { e1.x + er,e1.y + er },
                     Fade(ps.accent, 0.85f));
        DrawTriangle({ e2.x - er,e2.y + er }, { e2.x,e2.y - er }, { e2.x + er,e2.y + er },
                     Fade(ps.accent, 0.85f));
        DrawRectangleRounded(mb, 0.5f, 8, Fade(ps.accent, 0.55f));
        Vector2 pw = { mb.x + 9, cy };
        DrawEllipse((int)pw.x, (int)(pw.y + 1), 3.0f, 2.4f, Fade(RAYWHITE, 0.8f));
        for (int k = 0; k < 3; ++k) {
            float a = (-125 + k * 45) * DEG2RAD;
            DrawCircleV({ pw.x + cosf(a) * 4.4f, pw.y + sinf(a) * 4.4f }, 1.3f,
                        Fade(RAYWHITE, 0.8f));
        }
    } break;
    case BS_ARMOR: {  // 机甲：装甲 + 警条
        Vector2 p[6] = { {mb.x,mb.y + 5},{mb.x,mb.y + mb.height},
                         {mb.x + mb.width - 5,mb.y + mb.height},{mb.x + mb.width,mb.y + mb.height - 5},
                         {mb.x + mb.width,mb.y},{mb.x + 5,mb.y} };
        DrawTriangleFan(p, 6, Fade(ps.accent, 0.50f));
        for (int i = 0; i < 6; ++i)
            DrawLineEx(p[i], p[(i + 1) % 6], 1.4f, Fade(ps.accent, 0.95f));
        for (int i = 0; i < 5; ++i)
            DrawLineEx({ pv.x + 4 + i * 7, pv.y + pv.height - 3 },
                       { pv.x + 8 + i * 7, pv.y + pv.height - 9 }, 2.0f,
                       Fade(ps.accent2, 0.55f));
    } break;
    case BS_LINE: {   // 极简：一条线与留白
        DrawRectangle((int)mb.x, (int)mb.y, 3, (int)mb.height, ps.accent);
        DrawLine((int)mb.x, (int)(mb.y + mb.height), (int)(mb.x + mb.width * 0.6f),
                 (int)(mb.y + mb.height), Fade(ps.accent, 0.5f));
        DrawLine((int)pv.x, (int)(pv.y + pv.height * 0.72f), (int)(pv.x + pv.width),
                 (int)(pv.y + pv.height * 0.72f), Fade(ps.accent, 0.16f));
    } break;
    case BS_PILL: {   // 熵潮：胶囊 + 血月 + 涟漪
        float f = fmodf(t * 0.7f, 1.0f);
        DrawRectangleRoundedLines({ mb.x - f * 4,mb.y - f * 3,mb.width + f * 8,mb.height + f * 6 },
                                  0.5f, 8, Fade(ps.accent, (1 - f) * 0.4f));
        DrawRectangleRounded(mb, 0.5f, 8, Fade(ps.accent, 0.5f));
        Vector2 m2 = { mb.x + 9, cy };
        DrawCircleV(m2, 4.0f, Fade(Color{ 255,150,150,255 }, 0.95f));
        DrawCircleV({ m2.x + 1.8f, m2.y - 0.9f }, 3.2f, Fade(ps.bgTop, 0.95f));
    } break;
    case BS_ORNATE: { // 宇宙：金饰双线 + 星点
        DrawRectangleRounded(mb, 0.3f, 8, Fade(ps.accent, 0.45f));
        DrawRectangleRoundedLines(mb, 0.3f, 8, Fade(ps.accent, 0.95f));
        DrawRectangleRoundedLines({ mb.x + 2,mb.y + 2,mb.width - 4,mb.height - 4 }, 0.3f, 8,
                                  Fade(RAYWHITE, 0.25f));
        for (int i = 0; i < 2; ++i)
            DrawPoly({ i ? mb.x + mb.width - 5 : mb.x + 5, cy }, 4, 2.8f, t * 22,
                     Fade(ps.accent, 0.95f));
        for (int i = 0; i < 4; ++i) {
            float a = t * 0.6f + i * 1.57f;
            DrawCircleV({ cx + cosf(a) * 26, pv.y + 8 + sinf(a) * 4 }, 1.3f,
                        Fade(RAYWHITE, 0.6f));
        }
    } break;
    default: {        // 量子：柔和圆角 + 星点
        DrawRectangleRounded(mb, 0.5f, 8, Fade(ps.accent, 0.45f));
        DrawRectangleRounded({ mb.x + 2,mb.y + 1,mb.width - 4,mb.height * 0.42f }, 0.5f, 6,
                             Fade(RAYWHITE, 0.14f));
        DrawRectangleRoundedLines(mb, 0.5f, 8, Fade(RAYWHITE, 0.35f));
        for (int i = 0; i < 5; ++i) {
            float px = pv.x + 6 + fmodf(i * 37.0f + t * 6, pv.width - 12);
            DrawCircleV({ px, pv.y + 7 + (i % 2) * 4 }, 1.2f, Fade(ps.accent, 0.5f));
        }
    } break;
    }
}

void sceneSkin(float t) {
    uiPanel({ 40,12,VW - 80.0f,VH - 34.0f }, 0.02f);
    txtTitle("装 扮 衣 橱", VW / 2.0f, 22, 32, VIOLET);
    int un = 0; for (int i = 0; i < SKIN_COUNT; ++i) if (rec.skinUnlocked[i]) ++un;
    txtC("已解锁 " + to_string(un) + " / " + to_string(SKIN_COUNT) + "　　完成成就可获得更多外观，点击即可更换",
         VW / 2.0f, 62, 19, RAYWHITE);

    // 三类装扮：界面风格 / 种子造型 / 花朵造型
    const char* cat[3] = { "界面风格（整套视觉：幕布・按钮・面板・标题）",
                           "种子造型（成熟度由造型本身表现）",
                           "花朵造型" };
    float ys[3] = { 96, 330, 470 };
    const float CW = 148, CH = 104, GAPX = 12;
    for (int ty = 0; ty < 3; ++ty) {
        txt(cat[ty], 66, ys[ty], 20, GOLD);
        int col = 0;
        for (int i = 0; i < SKIN_COUNT; ++i) {
            if (SKINS[i].type != ty) continue;
            int perRow = 7;
            float x = 66.0f + (col % perRow) * (CW + GAPX);
            float y = ys[ty] + 28 + (col / perRow) * (CH + 8);
            bool ok = rec.skinUnlocked[i];
            bool cur = (ty == 0 && rec.themeSkin == i) || (ty == 1 && rec.seedSkin == i)
                     || (ty == 2 && rec.flowerSkin == i);
            bool hover = ok && CheckCollisionPointRec(gMouse, { x,y,CW,CH }) && guideAllows({ x,y,CW,CH });
            DrawRectangleRounded({ x,y,CW,CH }, 0.12f, 8,
                ok ? (hover ? Color{ 52,58,84,255 } : Color{ 34,40,58,255 }) : Color{ 24,26,34,255 });
            DrawRectangleLinesEx({ x,y,CW,CH }, 2, cur ? GOLD : Fade(RAYWHITE, ok ? 0.18f : 0.06f));
            float icx = x + CW / 2, icy = y + 34;
            if (ok) {
                if (ty == 0) {
                    // 界面风格预览：迷你幕布 + 该风格的按钮缩影，一眼看出差异
                    const ThemeStyle& ps = THEMES[std::clamp(SKINS[i].style, 0, TK_COUNT - 1)];
                    Rectangle pv = { x + 10, y + 8, CW - 20, 46 };
                    DrawRectangleGradientV((int)pv.x, (int)pv.y, (int)pv.width, (int)pv.height,
                                           ps.bgTop, ps.bgBot);
                    drawThemeChip(pv, ps, t);
                    DrawRectangleLinesEx(pv, 1.4f, Fade(ps.accent, 0.75f));
                    if (i == SKIN_COSMIC)
                        DrawRectangleLinesEx({ pv.x - 2,pv.y - 2,pv.width + 4,pv.height + 4 }, 2,
                            ColorFromHSV(fmodf(t * 80, 360.0f), 0.7f, 1.0f));
                } else if (ty == 1) {
                    // 种子造型预览：直接调用真实绘制，成熟度循环演示 0→3
                    int demoMat = (int)fmodf(t * 0.9f, 4.0f);
                    int saveSeed = rec.seedSkin;
                    rec.seedSkin = i;
                    drawSeed({ icx, icy + 2 }, demoMat, t);
                    rec.seedSkin = saveSeed;
                } else {
                    // 花朵造型预览：真实绘制
                    int saveFlower = rec.flowerSkin;
                    rec.flowerSkin = i;
                    drawFlower({ icx, icy - 2 }, 3, t);
                    rec.flowerSkin = saveFlower;
                }
                txt(SKINS[i].name, x + 10, y + 62, 18, cur ? GOLD : RAYWHITE);
                txt(cur ? "使用中" : "点击使用", x + 10, y + 84, 15, cur ? GOLD : Fade(RAYWHITE, 0.45f));
                if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    playSfx(sfxClick);
                    if (ty == 0) rec.themeSkin = i;
                    else if (ty == 1) rec.seedSkin = (rec.seedSkin == i) ? -1 : i;
                    else rec.flowerSkin = (rec.flowerSkin == i) ? -1 : i;
                    saveRecords();
                }
            } else {
                txtC("未解锁", icx, y + 26, 19, Color{ 92,96,108,255 });
                txt(SKINS[i].desc, x + 8, y + 62, 13, Color{ 78,82,94,255 });
                txt("完成对应成就", x + 8, y + 84, 13, Color{ 70,74,86,255 });
            }
            ++col;
        }
    }

    // 豪华界面开关
    if (rec.luxUnlocked) {
        Rectangle lr = { VW - 430.0f, 96, 340, 46 };
        DrawRectangleRounded(lr, 0.2f, 8, Fade(Color{ 60,40,90,255 }, 0.8f));
        DrawRectangleLinesEx(lr, 2, ColorFromHSV(fmodf(t * 70, 360.0f), 0.7f, 1.0f));
        txt("★ 宇宙尽头的花园（豪华界面）", lr.x + 14, lr.y + 12, 19, GOLD);
        if (uiButton({ lr.x + lr.width - 92, lr.y + 7, 82, 32 }, rec.luxOn ? "已开启" : "已关闭",
                     rec.luxOn ? GREEN : DARKGRAY, false, 17)) {
            rec.luxOn = !rec.luxOn; saveRecords();
        }
    }

    txt("种子与花朵外观再次点击可取消，恢复默认样式。", 66, VH - 60.0f, 18, GRAY);
    if (uiButton({ VW - 470.0f,VH - 66.0f,200,40 }, "返回成就墙", DARKGRAY, false, 20)) scene = ACHIEVE;
    if (uiButton({ VW - 258.0f,VH - 66.0f,190,40 }, "返回主菜单", DARKGRAY, false, 20)) scene = MENU;
}

// ==================== 场景：星尘工坊 ====================
void sceneMeta(float t) {
    uiPanel({ 50,20,VW - 100.0f,VH - 60.0f }, 0.02f);
    txtTitle("星 尘 工 坊", VW / 2.0f, 34, 36, VIOLET);
    txtC("每局结束按评分获得星尘，即使失败也不会白玩一局", VW / 2.0f, 82, 21, RAYWHITE);
    for (int i = 0; i < 5; ++i) {
        float a = t * 0.6f + i * 1.25f;
        DrawCircleV({ VW / 2.0f + cosf(a) * 150, 128 + sinf(a) * 14 }, 2.5f, Fade(VIOLET, 0.7f));
    }
    txtC("当前星尘：" + to_string(rec.stardust), VW / 2.0f, 116, 28, GOLD);

    for (int i = 0; i < U_COUNT; ++i) {
        float x = 130.0f + (i % 2) * 520, y = 176.0f + (i / 2) * 190;
        bool maxed = rec.upg[i] >= UPG[i].maxLv;
        int cost = upgCost(i);
        bool poor = !maxed && rec.stardust < cost;
        bool can = !maxed && !poor;
        bool hover = can && CheckCollisionPointRec(gMouse, { x,y,480,168 }) && guideAllows({ x,y,480,168 });
        DrawRectangleRounded({ x,y,480,168 }, 0.08f, 8,
            hover ? Color{ 48,54,80,255 } : Color{ 32,38,56,255 });
        DrawRectangleLinesEx({ x,y,480,168 }, 2, maxed ? GOLD : Fade(VIOLET, hover ? 0.9f : 0.4f));
        txt(UPG[i].name, x + 24, y + 18, 26, maxed ? GOLD : RAYWHITE);
        txt(UPG[i].desc, x + 24, y + 56, 20, GRAY);
        for (int k = 0; k < UPG[i].maxLv; ++k)
            DrawRectangleRounded({ x + 24.0f + k * 30, y + 92, 22, 12 }, 0.4f, 4,
                                 k < rec.upg[i] ? VIOLET : Color{ 52,56,72,255 });
        txt("Lv " + to_string(rec.upg[i]) + " / " + to_string(UPG[i].maxLv), x + 200, y + 88, 20, RAYWHITE);
        if (maxed)      txt("已满级", x + 24, y + 126, 21, GOLD);
        else if (poor)  txt("需要 " + to_string(cost) + " 星尘（不足）", x + 24, y + 126, 21, RED);
        else            txt(hover ? "点击升级，消耗 " + to_string(cost) + " 星尘"
                                  : "升级需要 " + to_string(cost) + " 星尘",
                            x + 24, y + 126, 21, hover ? GREEN : SKYBLUE);
        if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            rec.stardust -= cost; ++rec.upg[i];
            playSfx(sfxUpg);
            saveRecords();
        }
    }
    txt("升级在教程模式中不生效；星尘不会因失败而扣除。", 130, VH - 108.0f, 19, GRAY);
    if (uiButton({ VW / 2.0f - 110,VH - 72.0f,220,42 }, "返回主菜单", DARKGRAY, false, 20)) scene = MENU;
}

// ==================== 场景：数据中心 ====================
void sceneStats() {
    uiPanel({ 50,20,VW - 100.0f,VH - 60.0f }, 0.02f);
    txtTitle("数 据 中 心", VW / 2.0f, 34, 36, curTheme().title);
    txtC("最近 12 局综合评分走势", VW / 2.0f, 82, 21, RAYWHITE);
    drawCurve({ 90,116,780,330 }, true);

    float x = 910, y = 120;
    txt("总计概览", x, y, 23, GOLD); y += 34;
    txt("总场次：" + to_string(rec.totalGames), x, y, 20, RAYWHITE); y += 30;
    txt("胜利数：" + to_string(rec.totalWins), x, y, 20, RAYWHITE); y += 30;
    int rate = rec.totalGames ? rec.totalWins * 100 / rec.totalGames : 0;
    txt("胜率：" + to_string(rate) + "%", x, y, 20, GOLD); y += 30;
    txt("累计花朵：" + to_string(rec.totalFlowers), x, y, 20, PINK); y += 30;
    txt("累计猫：" + to_string(rec.totalCats), x, y, 20, GOLD); y += 30;
    txt("同步坍缩：" + to_string(rec.totalEntSync), x, y, 20, VIOLET); y += 30;
    txt("购买次数：" + to_string(rec.totalBuys), x, y, 20, SKYBLUE); y += 30;
    txt("最高连击：x" + to_string(rec.bestCombo), x, y, 20, PINK); y += 30;
    txt("星尘存量：" + to_string(rec.stardust), x, y, 20, VIOLET); y += 40;

    int avg = 0;
    if (rec.histN) { for (int i = 0; i < rec.histN; ++i) avg += rec.hist[i]; avg /= rec.histN; }
    txt("近期均值：" + (rec.histN ? to_string(avg) : "暂无"), x, y, 22, GOLD);

    txt("每日挑战：连续 " + to_string(rec.dailyStreak) + " 天　最佳 "
        + (rec.dailyBest ? to_string(rec.dailyBest) : "暂无"), 90, 470, 22, SKYBLUE);
    txt("教程进度：" + to_string(rec.tutProgress) + " / 8 关", 90, 506, 22, GREEN);
    int adone = 0; for (int i = 0; i < A_COUNT; ++i) if (rec.achUnlocked[i]) ++adone;
    txt("成就进度：" + to_string(adone) + " / " + to_string(A_COUNT), 90, 542, 22, GOLD);
    txt("经典最佳：休闲 " + to_string(rec.bestClassic[0]) + "　标准 " + to_string(rec.bestClassic[1])
        + "　挑战 " + to_string(rec.bestClassic[2]) + "　噩梦 " + to_string(rec.bestClassic[3]),
        90, 578, 21, RAYWHITE);
    txt("无尽最远：" + to_string(rec.bestEndlessTurn) + " 回合　评分 " + to_string(rec.bestEndlessScore),
        90, 610, 21, VIOLET);

    if (uiButton({ VW / 2.0f - 110,VH - 72.0f,220,42 }, "返回主菜单", DARKGRAY, false, 20)) scene = MENU;
}

// ==================== 场景：商店 ====================
void drawShopCard(Rectangle r, int k, bool fixedItem, float t) {
    int lv = 0, maxLv = 0, cost = 0;
    bool soldOut = false, maxed = false;
    if (fixedItem) {
        if (k == IK_LENS) { lv = lensLv; maxLv = 3; if (lv < 3) cost = lensCost[lv]; }
        else if (k == IK_STAB) { lv = stableLv; maxLv = 2; if (lv < 2) cost = stableCost[lv]; }
        else { lv = ampLv; maxLv = 2; if (lv < 2) cost = ampCost[lv]; }
        maxed = (lv >= maxLv);
    } else {
        if (k < 0) soldOut = true; else cost = ITEMS[k].cost;
    }
    bool poor = (!soldOut && !maxed && energy < cost);
    bool canBuy = !soldOut && !maxed && !poor && toolOK(TL_SHOP);
    bool hover = !uiLock && canBuy && CheckCollisionPointRec(gMouse, r) && guideAllows(r);

    DrawRectangleRounded(r, 0.10f, 8, soldOut ? Color{ 28,30,40,255 }
                                    : hover ? Color{ 48,56,80,255 } : Color{ 34,40,58,255 });
    DrawRectangleLinesEx(r, 2, soldOut ? Color{ 50,52,62,255 }
                             : hover ? RAYWHITE : Fade(icol(ITEMS[k].cidx), 0.55f));
    if (soldOut) { txtC("售 罄", r.x + r.width / 2, r.y + r.height / 2 - 16, 28, GRAY); return; }

    drawItemIcon(k, { r.x + 62, r.y + 74 }, 38, t);
    string nm = ITEMS[k].name;
    if (fixedItem) nm += "  Lv" + to_string(lv) + (maxed ? "" : " → " + to_string(lv + 1));
    txt(nm, r.x + 116, r.y + 26, 23, icol(ITEMS[k].cidx));
    if (maxed) txt("已满级", r.x + 116, r.y + 62, 21, GRAY);
    else       txt("价格 " + to_string(cost) + " 灵能", r.x + 116, r.y + 62, 21, poor ? RED : GOLD);
    txt(ITEMS[k].d1, r.x + 18, r.y + 118, 19, RAYWHITE);
    txt(ITEMS[k].d2, r.x + 18, r.y + 144, 19, GRAY);
    if (!toolOK(TL_SHOP))  txt("本关暂未解锁该功能", r.x + 18, r.y + 174, 19, GRAY);
    else if (maxed)        txt("已满级", r.x + 18, r.y + 174, 19, GRAY);
    else if (poor)         txt("灵能不足", r.x + 18, r.y + 174, 19, RED);
    else                   txt(hover ? "点击购买（不消耗回合）" : "购买不消耗回合",
                               r.x + 18, r.y + 174, 19, hover ? GREEN : Fade(RAYWHITE, 0.6f));

    if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (fixedItem) {
            if (k == IK_LENS) tryBuyFixed(lensLv, lensCost, 3, "量子透镜");
            else if (k == IK_STAB) tryBuyFixed(stableLv, stableCost, 2, "时空稳定器");
            else tryBuyFixed(ampLv, ampCost, 2, "纠缠增幅器");
        } else {
            for (int i = 0; i < 3; ++i) if (shopSlot[i] == k) { tryBuySlot(i); break; }
        }
    }
}

void sceneShop(float t) {
    // 教程第 6 关：只允许点击商品卡片区域
    bool shopGuide = (mode == M_TUTORIAL && tutIdx == 5 && statBuy < 2);
    Rectangle shopGuideRect = { 90, 145, 1105, 470 };
    if (shopGuide) { guideLock = true; guideRect = shopGuideRect; }

    DrawRectangle(0, 0, VW, VH, Fade(BLACK, 0.62f));
    uiPanelAccent({ 70,30,1140,660 }, curTheme().accent, 0.02f);
    txtTitle("量 子 商 店", VW / 2.0f, 44, 36, curTheme().title);
    txt("当前灵能：" + to_string(energy), 100, 92, 24, GOLD);
    txt("货架刷新倒计时：" + to_string(std::max(0, shopRefreshTurn - turnNo)) + " 回合", 340, 92, 24, VIOLET);
    txt("进入商店与购买均不消耗回合，可随时补给", 700, 94, 20, GRAY);

    txt("固定装备（可升级）", 100, 126, 22, GOLD);
    float xs[3] = { 100,470,840 };
    for (int i = 0; i < 3; ++i) drawShopCard({ xs[i],150,340,210 }, IK_LENS + i, true, t);

    txt("限时商品（每 6 回合随机刷新）", 100, 372, 22, GOLD);
    for (int i = 0; i < 3; ++i) drawShopCard({ xs[i],396,340,210 }, shopSlot[i], false, t);

    if (uiButton({ VW / 2.0f - 130,624,260,44 }, "离开商店 (ESC)", DARKGRAY)) shopOpen = false;

    // 教程第 6 关：指引购买商品（只高亮商品区，其余不可点击）
    if (shopGuide)
        drawGuideFocus(shopGuideRect,
                       "点任意商品卡片购买（不消耗回合），买满 2 件即可通关（" + to_string(statBuy) + "/2）", t);
}

// ==================== 场景：存档槽位（10 个，可覆盖） ====================
void sceneSlots() {
    uiPanel({ 60,20,VW - 120.0f,VH - 40.0f }, 0.02f);
    txtTitle("存 档 槽 位", VW / 2.0f, 30, 34, curTheme().title);
    txtC(slotsMode == 0 ? "选择一个槽位保存当前对局（已有存档会被覆盖）"
                        : "选择存档继续游戏，也可删除不需要的存档",
         VW / 2.0f, 76, 20, RAYWHITE);

    const char* modeName[4] = { "经典", "无尽", "教程", "每日" };
    for (int i = 0; i < RUN_SLOTS; ++i) {
        int slot = i + 1;
        float col = (float)(i % 2), row = (float)(i / 2);
        Rectangle card = { 90 + col * 560, 118 + row * 106, 520, 94 };
        RunSlotInfo info = readRunSlot(slot);
        uiPanel(card, 0.10f);
        DrawRectangleRounded({ card.x, card.y, 5, card.height }, 1.0f, 6,
                             info.used ? Fade(GOLD, 0.9f) : Fade(GRAY, 0.5f));
        txt("槽位 " + to_string(slot), card.x + 16, card.y + 12, 21, GOLD);
        if (info.used) {
            string mt = (info.mode >= 0 && info.mode < 4) ? modeName[info.mode] : "未知";
            txt(mt + "模式 · 第 " + to_string(info.turn) + " 回合 · 灵能 " + to_string(info.energy),
                card.x + 112, card.y + 14, 19, RAYWHITE);
        } else {
            txt("空存档", card.x + 112, card.y + 14, 19, GRAY);
        }

        float bx = card.x + card.width - 246, by = card.y + 50;
        if (slotsMode == 0) {
            if (uiButton({ bx, by, 110, 34 }, info.used ? "覆盖" : "保存", GREEN, false, 18)) {
                saveRun(slot);
                addLog("已保存到槽位 " + to_string(slot) + "。", GREEN);
                playSfx(sfxClick);
            }
        } else {
            if (uiButton({ bx, by, 110, 34 }, "读取", SKYBLUE, false, 18, info.used)) {
                if (loadRun(slot)) playSfx(sfxClick);
            }
            if (uiButton({ bx + 122, by, 110, 34 }, "删除", Color{ 150,70,70,255 }, false, 18, info.used)) {
                clearRunSave(slot);
                playSfx(sfxClick);
            }
        }
    }

    if (uiButton({ VW / 2.0f - 110, VH - 74.0f, 220, 46 },
                 slotsMode == 0 ? "返回对局" : "返回主菜单", DARKGRAY, false, 20)) {
        scene = (slotsMode == 0) ? PLAY : MENU;
    }
}

// ==================== 场景：游戏中 ====================
void scenePlay(float dt, float t) {
    const ThemeStyle& th = curTheme();
    // 教程内的分步引导：先算好目标并加锁（商店打开时由 sceneShop 负责）
    Rectangle guideR; string guideT;
    bool hasGuide = (!shopOpen) && tutorialGuide(guideR, guideT);
    if (hasGuide) { guideLock = true; guideRect = guideR; }

    txtS("量子花园", 40, 14, 24, th.title);
    string mtag = mode == M_ENDLESS ? "无尽模式"
                : mode == M_DAILY ? string("每日挑战 ") + to_string(todayCode())
                : mode == M_TUTORIAL ? string("教程 · 第 ") + to_string(tutIdx + 1) + " 关"
                : string("经典 · ") + CFG[diff].name;
    txt(mtag, 170, 17, 22, GOLD);
    txt("H 规则   F11 全屏   M 音效", 640, 20, 18, GRAY);

    // 对局内快捷操作：存档（进入 10 个槽位界面）与退出（放弃本局回到主菜单）
    if (uiButton({ VW - 268, 8, 120, 32 }, "存档", SKYBLUE, false, 18)) {
        slotsMode = 0; prevScene = PLAY; scene = SLOTS;
        playSfx(sfxClick);
    }
    if (uiButton({ VW - 140, 8, 120, 32 }, "退出", Color{ 150,70,70,255 }, false, 18)) {
        scene = MENU;
        playSfx(sfxClick);
    }

    txt("回合 " + to_string(turnNo) + (maxTurn > 9999 ? "" : "/" + to_string(maxTurn)), 40, 54, 25, RAYWHITE);
    txt("灵能 " + to_string(energy) + ((mode == M_CLASSIC || mode == M_DAILY) ? "/" + to_string(winEnergy) : ""),
        230, 54, 25, GOLD);
    txt("连击 x" + to_string(combo) + " (+" + to_string(combo * 10) + "%)", 450, 54, 25, PINK);
    if (entropyOn()) {
        txt("波次 " + to_string(wave), 740, 54, 25, VIOLET);
        txt("熵增 -" + to_string(curEntropy()) + "/回合", 860, 54, 25, ORANGE);
    } else if (mode == M_CLASSIC || mode == M_DAILY)
        txt("剩余 " + to_string(std::max(0, maxTurn - turnNo + 1)) + " 回合", 740, 54, 25, SKYBLUE);

    DrawRectangleRounded({ 40,90,512,14 }, 1.0f, 8, Color{ 44,48,66,255 });
    float ratio = std::clamp(energy / (float)((mode == M_CLASSIC || mode == M_DAILY) ? winEnergy : 200), 0.0f, 1.0f);
    DrawRectangleRounded({ 40,90,512 * ratio,14 }, 1.0f, 8,
                         ratio > 0.6f ? GOLD : ratio > 0.25f ? ORANGE : RED);
    if (mode == M_TUTORIAL) txt("本关目标：" + goalProgress(), 40, 114, 21, GREEN);

    // 棋盘格：底色与描边跟随当前界面风格
    for (int i = 0; i < SIZE * SIZE; ++i) {
        Rectangle r = cellRect(i);
        bool hv = !uiLock && CheckCollisionPointRec(gMouse, r);
        Color fill = hv ? ColorBrightness(th.cellFill, 0.30f) : th.cellFill;
        // 极简与科技风用直角格，其余用圆角，呼应各自的形状语言
        bool sharpCell = (th.btn == BS_SHARP || th.btn == BS_LINE || th.btn == BS_ARMOR);
        if (sharpCell) {
            DrawRectangleRec(r, fill);
            DrawRectangleLinesEx(r, 1.0f, Fade(th.cellEdge, 0.55f));
            // 四角定位标记
            float L = 9;
            Color mk = Fade(th.accent, hv ? 0.75f : 0.30f);
            DrawLineEx({ r.x,r.y }, { r.x + L,r.y }, 2, mk);
            DrawLineEx({ r.x,r.y }, { r.x,r.y + L }, 2, mk);
            DrawLineEx({ r.x + r.width,r.y + r.height }, { r.x + r.width - L,r.y + r.height }, 2, mk);
            DrawLineEx({ r.x + r.width,r.y + r.height }, { r.x + r.width,r.y + r.height - L }, 2, mk);
        } else {
            DrawRectangleRounded(r, 0.12f, 8, fill);
            DrawRectangleRounded({ r.x + 4,r.y + 4,r.width - 8,r.height - 8 }, 0.12f, 8,
                                 Fade(th.bgBot, 0.55f));
            DrawRectangleRoundedLines(r, 0.12f, 8, Fade(th.cellEdge, 0.45f));
        }
        if (i == entSel) DrawRectangleLinesEx(r, 3, VIOLET);
        else if (hv)     DrawRectangleLinesEx(r, 2, Fade(th.accent, 0.55f));
    }
    float pul = 0.4f + 0.3f * sinf(t * 4.0f);
    for (int i = 0; i < SIZE * SIZE; ++i)
        if (grid[i].st == ENTANGLED && grid[i].partner > i) {
            Vector2 a = cellCenter(i), b = cellCenter(grid[i].partner);
            DrawLineEx(a, b, 7, Fade(VIOLET, pul * 0.45f));
            DrawLineEx(a, b, 3, Fade(Color{ 200,150,255,255 }, pul + 0.35f));
            float f = fmodf(t * 0.55f, 1.0f);
            DrawCircleV({ a.x + (b.x - a.x) * f,a.y + (b.y - a.y) * f }, 5.5f, Fade(RAYWHITE, 0.9f));
        }
    for (int i = 0; i < SIZE * SIZE; ++i) {
        Vector2 ct = cellCenter(i);
        if (grid[i].st == SEED)           drawSeed(ct, grid[i].mat, t);
        else if (grid[i].st == ENTANGLED) drawEnt(ct, t);
        else if (grid[i].st == FLOWER)    drawFlower(ct, grid[i].life, t);
    }
    if (!uiLock && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        for (int i = 0; i < SIZE * SIZE; ++i)
            if (CheckCollisionPointRec(gMouse, cellRect(i)) && guideAllows(cellRect(i))) { clickCell(i); break; }

    txt("行动（先选行动，再点击格子）", PX, 150, 22, SKYBLUE);
    if (medAllowed() && !medReady()) txt("冥想冷却中", PX + 508, 151, 18, ORANGE);
    if (uiButton({ PX,180,200,46 }, "种植 -" + to_string(plantCost), GREEN, tool == T_PLANT, 22, toolOK(TL_PLANT)))
        { tool = T_PLANT; entSel = -1; }
    if (uiButton({ PX + 215,180,200,46 }, "观测", SKYBLUE, tool == T_OBSERVE, 22, toolOK(TL_OBS)))
        { tool = T_OBSERVE; entSel = -1; }
    if (uiButton({ PX + 430,180,200,46 }, "纠缠", VIOLET, tool == T_ENTANGLE, 22, toolOK(TL_ENT)))
        { tool = T_ENTANGLE; entSel = -1; }

    bool medOn = medAllowed();
    bool medNow = medOn && medReady();
    string medLabel = !medOn ? "静观（跳过一回合，休闲模式不可冥想）"
                    : medNow ? "冥想 +6 灵能（消耗一回合）"
                             : "静观（跳过一回合，本回合无收益）";
    if (uiButton({ PX,238,630,42 }, medLabel,
                 medNow ? DARKGRAY : Color{ 60,62,72,255 }, false, 21, toolOK(TL_MED))) {
        if (medNow) {
            energy += 6;
            medReadyTurn = turnNo + 2;
            playSfx(sfxMeditate);
            addLog("冥想汲取了 6 点灵能。心神需要平复，下一回合无法再次冥想。", RAYWHITE);
        } else {
            playSfx(sfxIdle);
            addLog(medOn ? "静观花园，心神尚未平复，本回合没有收益。"
                         : "静观花园，休闲模式无法冥想，本回合没有收益。", GRAY);
        }
        endTurn();
    }
    if (uiButton({ PX,290,630,46 }, "进 入 量 子 商 店（不消耗回合）", GOLD, false, 23, toolOK(TL_SHOP))) {
        shopOpen = true; playSfx(sfxShopOpen);
    }

    txt("装备：透镜 Lv" + to_string(lensLv) + "   稳定器 Lv" + to_string(stableLv)
        + "   增幅器 Lv" + to_string(ampLv), PX, 348, 20, SKYBLUE);
    string buff = "增益：";
    if (springTurns)   buff += "源泉 " + to_string(springTurns) + " 回合   ";
    if (shieldCharges) buff += "护盾 x" + to_string(shieldCharges) + "   ";
    if (eyeCharges)    buff += "之眼 x" + to_string(eyeCharges) + "   ";
    if (luckCharges)   buff += "幸运 x" + to_string(luckCharges) + "   ";
    if (entropyCut)    buff += "熵抑 -" + to_string(entropyCut);
    if (buff == "增益：") buff += "暂无";
    txt(buff, PX, 376, 20, GOLD);

    txt("量子日志", PX, 406, 21, SKYBLUE);
    uiPanel({ PX,432,630,270 }, 0.04f);
    float ly = 442;
    for (auto& m : logs) { txt(m.text, PX + 12, ly, 18, m.col); ly += 21.0f; }

    // 教程内分步指引（商店打开时由 sceneShop 负责显示）
    if (hasGuide) drawGuideFocus(guideR, guideT, t);
}

// ==================== 场景：结算 ====================
void sceneResult(float t) {
    DrawRectangle(0, 0, VW, VH, Fade(BLACK, 0.55f));
    uiPanel({ VW / 2.0f - 290,60,580,600 }, 0.04f);

    if (mode == M_TUTORIAL) {
        if (tutIdx == 7 && tutPassed) {
            // 最后一关通关：毕业结束语
            txtTitle("毕 业 典 礼", VW / 2.0f, 96, 42, GOLD);
            float ey = 160;
            txtC("恭喜你，园丁！八堂课，你已全部通关。", VW / 2.0f, ey, 22, GOLD); ey += 34;
            txtC("从第一次让波函数坍缩时的小心翼翼，", VW / 2.0f, ey, 21, RAYWHITE); ey += 30;
            txtC("到如今在熵增里也能从容收获——", VW / 2.0f, ey, 21, RAYWHITE); ey += 30;
            txtC("你学会了种植、观测与纠缠，", VW / 2.0f, ey, 21, RAYWHITE); ey += 30;
            txtC("也学会了在风暴与等待中保持耐心。", VW / 2.0f, ey, 21, RAYWHITE); ey += 36;
            txtC("量子花园真正的旅程，此刻才刚刚开始。", VW / 2.0f, ey, 22, GOLD); ey += 36;
            txtC("去经典模式追逐更高的分数，", VW / 2.0f, ey, 21, RAYWHITE); ey += 30;
            txtC("去无尽模式挑战生存的极限，", VW / 2.0f, ey, 21, RAYWHITE); ey += 30;
            txtC("去每日挑战与全世界的园丁一决高下。", VW / 2.0f, ey, 21, RAYWHITE); ey += 36;
            txtC("花园永远为你留着一块空地，", VW / 2.0f, ey, 21, RAYWHITE); ey += 30;
            txtC("愿每一次观测，都开出你想要的花。", VW / 2.0f, ey, 22, PINK); ey += 16;
            if (uiButton({ VW / 2.0f - 230,560,220,48 }, "开始经典模式", GREEN, false, 21)) resetGame(M_CLASSIC, 1);
            if (uiButton({ VW / 2.0f + 10,560,220,48 }, "返回主菜单", DARKGRAY, false, 21)) scene = MENU;
            return;
        }
        txtTitle(tutPassed ? "通 关" : "本关未完成", VW / 2.0f, 100, 46, tutPassed ? GOLD : GRAY);
        txtC(TUT[tutIdx].title, VW / 2.0f, 170, 24, RAYWHITE);
        txtC(tutPassed ? "本关目标已达成！" : "再试一次，慢慢来。", VW / 2.0f, 220, 24, SKYBLUE);
        txtC(goalProgress(), VW / 2.0f, 280, 24, GREEN);
        txtC("存活回合：" + to_string(turnNo), VW / 2.0f, 340, 24, RAYWHITE);
        txtC("最终灵能：" + to_string(std::max(0, energy)), VW / 2.0f, 380, 24, GOLD);
        txtC("最高连击：x" + to_string(maxCombo), VW / 2.0f, 420, 24, PINK);
        if (tutPassed && tutIdx < 7) {
            if (uiButton({ VW / 2.0f - 230,560,220,48 }, "下一关", GREEN)) { ++tutIdx; scene = TUT_BRIEF; }
        } else if (uiButton({ VW / 2.0f - 230,560,220,48 }, "重试本关", SKYBLUE)) resetGame(M_TUTORIAL, 1);
        if (uiButton({ VW / 2.0f + 10,560,220,48 }, "返回主菜单", DARKGRAY)) scene = MENU;
        return;
    }

    if (winGame)          txtTitle("胜 利", VW / 2.0f, 84, 44, GOLD);
    else if (energy <= 0) txtTitle("灵能耗尽", VW / 2.0f, 84, 44, RED);
    else                  txtTitle("时间到", VW / 2.0f, 84, 44, GRAY);
    txtC(winGame ? "你成为了量子园艺大师！"
        : (energy <= 0 ? "花园坍缩进了虚空……" : "未达成目标，再接再厉。"),
        VW / 2.0f, 138, 22, RAYWHITE);

    string grade = gradeOf(lastScore);
    float y = 178;
    txtC("存活回合：" + to_string(turnNo), VW / 2.0f, y, 22, RAYWHITE); y += 31;
    txtC("最终灵能：" + to_string(std::max(0, energy)), VW / 2.0f, y, 22, GOLD); y += 31;
    txtC("最高连击：x" + to_string(maxCombo), VW / 2.0f, y, 22, PINK); y += 31;
    txtC("收获灵能花：" + to_string(flowers) + "　薛定谔之猫：" + to_string(cats),
         VW / 2.0f, y, 22, PINK); y += 31;
    txtC("时空裂缝：" + to_string(riftThisRun) + " 次", VW / 2.0f, y, 22,
         riftThisRun ? RED : GREEN); y += 36;
    txtC("综合评分：" + to_string(lastScore), VW / 2.0f, y, 26, RAYWHITE); y += 36;
    txtC("评级  " + grade, VW / 2.0f, y, 38, grade == "S" ? GOLD : grade == "A" ? GREEN : RAYWHITE);
    y += 48;
    txtC("获得星尘 +" + to_string(lastStardust) + "　　当前 " + to_string(rec.stardust),
         VW / 2.0f, y, 22, VIOLET);
    y += 30;
    if (mode == M_DAILY) {
        txtC("每日连续天数：" + to_string(rec.dailyStreak) + " 天", VW / 2.0f, y, 20, SKYBLUE);
        y += 27;
    }
    if (newRecord)
        txtC("★ 新 纪 录 ★", VW / 2.0f, y, 26, Fade(GOLD, 0.6f + 0.4f * sinf(t * 6)));

    if (uiButton({ VW / 2.0f - 270,596,170,44 }, "再来一局", SKYBLUE, false, 21))
        resetGame(mode, diff);
    if (uiButton({ VW / 2.0f - 85,596,170,44 }, copied ? "已复制到剪贴板" : "复制战绩",
                 GOLD, false, copied ? 17.0f : 21.0f)) {
        SetClipboardText(shareText().c_str());
        copied = true;
    }
    if (uiButton({ VW / 2.0f + 100,596,170,44 }, "返回主菜单", DARKGRAY, false, 21)) scene = MENU;
}

// ==================== 场景：关于 / 开源许可 ====================
// 把内嵌的许可全文按行拆分（只读，缓存一次）
static const std::vector<std::string>& aboutLicenseLines(int which) {
    static std::vector<std::string> cache[2];
    static bool loaded[2] = { false, false };
    if (!loaded[which]) {
        loaded[which] = true;
        int n = 0;
        const unsigned char* d = qgEmbeddedLicense(which, &n);
        if (d && n > 0) {
            std::string s((const char*)d, (size_t)n);
            size_t pos = 0;
            while (pos <= s.size()) {
                size_t e = s.find('\n', pos);
                if (e == std::string::npos) e = s.size();
                std::string line = s.substr(pos, e - pos);
                if (!line.empty() && line.back() == '\r') line.pop_back();
                cache[which].push_back(line);
                if (e >= s.size()) break;
                pos = e + 1;
            }
        }
    }
    return cache[which];
}

void sceneAbout() {
    uiPanel({ 90,36,VW - 180.0f,VH - 86.0f }, 0.02f);

    if (aboutPage == 0) {
        txtTitle("关 于", VW / 2.0f, 56, 36, curTheme().title);
        float y = 124;
        txtC("《量子花园》 Quantum Garden", VW / 2.0f, y, 24, GOLD); y += 40;
        txtC("版权所有 © 2026 bd_sakura，保留所有权利", VW / 2.0f, y, 20, RAYWHITE); y += 34;
        txtC("本游戏以 Apache License 2.0 授权发布", VW / 2.0f, y, 20, SKYBLUE); y += 28;
        txtC("许可全文：https://www.apache.org/licenses/LICENSE-2.0", VW / 2.0f, y, 17, GRAY); y += 46;
        txtC("内嵌字体：思源黑体 Noto Sans SC", VW / 2.0f, y, 20, RAYWHITE); y += 28;
        txtC("以 SIL Open Font License 1.1 授权", VW / 2.0f, y, 19, SKYBLUE); y += 26;
        txtC("字体版权 (c) 2014-2021 Adobe，保留字体名 'Source'", VW / 2.0f, y, 17, GRAY); y += 28;
        txtC("OFL 全文：https://scripts.sil.org/OFL", VW / 2.0f, y, 17, GRAY); y += 46;
        txtC("联系方式：tuboshujingqi@163.com", VW / 2.0f, y, 19, PINK); y += 30;
        txtC("本页为游戏内展示；完整许可亦随游戏附带文本文件。", VW / 2.0f, y, 16, GRAY);

        if (uiButton({ VW / 2.0f - 300, VH - 108.0f, 280, 46 }, "Apache 2.0 许可全文", SKYBLUE, false, 19)) { aboutPage = 1; aboutScroll = 0; }
        if (uiButton({ VW / 2.0f + 20, VH - 108.0f, 280, 46 }, "SIL OFL 1.1 字体许可全文", VIOLET, false, 19)) { aboutPage = 2; aboutScroll = 0; }
        if (uiButton({ VW / 2.0f - 90, VH - 54.0f, 180, 40 }, "返回主菜单", DARKGRAY, false, 20)) { aboutPage = 0; scene = MENU; }
        return;
    }

    // ---- 许可全文（滚轮滚动）----
    const std::vector<std::string>& lines = aboutLicenseLines(aboutPage - 1);
    txtTitle(aboutPage == 1 ? "Apache License 2.0" : "SIL Open Font License 1.1",
             VW / 2.0f, 50, 26, curTheme().title);

    int visible = 21;
    int maxScroll = std::max(0, (int)lines.size() - visible);
    aboutScroll -= GetMouseWheelMove() * 3.0f;
    if (aboutScroll < 0) aboutScroll = 0;
    if (aboutScroll > maxScroll) aboutScroll = (float)maxScroll;

    float y = 92;
    for (int i = 0; i < visible; ++i) {
        int idx = (int)aboutScroll + i;
        if (idx >= (int)lines.size()) break;
        txt(lines[idx], 120, y, 16, RAYWHITE);
        y += 24;
    }
    txt("滚轮滚动  " + to_string((int)aboutScroll + 1) + " / " + to_string((int)lines.size()),
        120, VH - 62.0f, 16, GRAY);

    if (uiButton({ VW / 2.0f - 220, VH - 56.0f, 200, 40 }, "返回关于", DARKGRAY, false, 19)) aboutPage = 0;
    if (uiButton({ VW / 2.0f + 20, VH - 56.0f, 200, 40 }, "返回主菜单", DARKGRAY, false, 19)) { aboutPage = 0; scene = MENU; }
}
