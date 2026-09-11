// ============================================================
//  main.cpp  量子花园 6.3  Quantum Garden（网页版适配）
//  桌面版：与原版完全一致的初始化 + while 主循环
//  网页版：Emscripten 环境 —— 先把 IndexedDB 存档同步进虚拟文件系统，
//          同步完成后再启动游戏（避免存档被空数据覆盖），
//          用 emscripten_set_main_loop 驱动帧循环。
// ============================================================
#include "types.h"
#include "data.h"
#include "state.h"
#include "audio.h"
#include "util.h"
#include "save.h"
#include "render.h"
#include "game.h"
#include "scenes.h"
#include "guard.h"
#include <algorithm>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

static float s_dt = 0.0f;
static float debugCheckTimer = 0.0f;

// 单帧逻辑（桌面版 while 循环体原样搬到这里，网页每帧调用一次）
static void frameOnce() {
    s_dt = GetFrameTime(); gTime += s_dt;
    const float dt = s_dt;
    (void)debugCheckTimer;

    if (IsKeyPressed(KEY_F11)) toggleFull();
    if (IsKeyPressed(KEY_M)) muted = !muted;
    if (IsKeyPressed(KEY_ESCAPE) && shopOpen) { shopOpen = false; playSfx(sfxClick); }
    if (IsKeyPressed(KEY_H) && !shopOpen && !(scene == LICENSE && !rec.licenseAgreed)) {
        if (scene != HELP) { prevScene = scene; scene = HELP; } else scene = prevScene;
    }
    updateAmbient();

    gScale = std::min(GetScreenWidth() / (float)VW, GetScreenHeight() / (float)VH);
    Vector2 rm = GetMousePosition();
    float mdw = floorf(VW * gScale), mdh = floorf(VH * gScale);
    float mdx = floorf((GetScreenWidth()  - mdw) * 0.5f);
    float mdy = floorf((GetScreenHeight() - mdh) * 0.5f);
    gMouse.x = (rm.x - mdx) / gScale;
    gMouse.y = (rm.y - mdy) / gScale;
    if (shake > 0) shake = std::max(0.0f, shake - dt * 40);

    BeginTextureMode(target);
        Camera2D ssCam = { 0 };
        ssCam.zoom = (float)SSAA;
        BeginMode2D(ssCam);
        drawBG(gTime);
        if (scene == MENU)           sceneMenu(gTime);
        else if (scene == HELP)      sceneHelp();
        else if (scene == TUT_SEL)   sceneTutSel();
        else if (scene == TUT_BRIEF) sceneTutBrief();
        else if (scene == ACHIEVE)   sceneAchieve(gTime);
        else if (scene == SKINSEL)   sceneSkin(gTime);
        else if (scene == META)      sceneMeta(gTime);
        else if (scene == STATS)     sceneStats();
        else if (scene == LICENSE)   sceneLicense();
        else if (scene == PLAY) {
            uiLock = shopOpen;
            scenePlay(dt, gTime);
            uiLock = false;
            if (shopOpen) sceneShop(gTime);
        }
        else sceneResult(gTime);
        drawFX(dt);
        if (luxActive()) drawLuxFrame(gTime);
        txtS(muted ? "音效已关闭 (M)" : "音效已开启 (M)",
            VW - 210.0f, VH - 26.0f, 18, muted ? GRAY : Fade(SKYBLUE, 0.6f));
        EndMode2D();
    EndTextureMode();

    float ox = (randF() - 0.5f) * shake, oy = (randF() - 0.5f) * shake;
    BeginDrawing();
        ClearBackground(BLACK);
        Rectangle src = { 0,0,(float)target.texture.width,-(float)target.texture.height };
        float dw = floorf(VW * gScale), dh = floorf(VH * gScale);
        float dx = floorf((GetScreenWidth()  - dw) * 0.5f + ox);
        float dy = floorf((GetScreenHeight() - dh) * 0.5f + oy);
        Rectangle dst = { dx, dy, dw, dh };
        DrawTexturePro(target.texture, src, dst, { 0,0 }, 0, WHITE);
    EndDrawing();

    if (shouldQuit) {
        // 浏览器无法真正"关窗口"，保存存档后停住循环并提示
        saveRecords();
        BeginDrawing();
            ClearBackground(BLACK);
            txtC("已退出游戏，可以关闭此标签页。", GetScreenWidth() / 2.0f,
                 GetScreenHeight() / 2.0f - 20, 30, GRAY);
        EndDrawing();
        emscripten_cancel_main_loop();
    }
}

static void webInitAndRun() {
    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(VW, VH, "Quantum Garden 6.3");
    SetTargetFPS(60);
    SetExitKey(0);
    // 先解密协议文本，再建字体图集（字库扫描需要用到协议内容）
    printf("INFO: [QG] secure-text %s\n", loadSecureText() ? "loaded" : "FALLBACK");
    initHCol();
    initSfx();
    font = loadChineseFont();
    fontTitle = loadTitleFont();
    target = LoadRenderTexture(VW * SSAA, VH * SSAA);
    SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);
    // 构建标记：带 "INFO:" 前缀 —— 外壳 shell.html 只把非 INFO 输出弹进错误框，
    // 因此这行不再触发红色错误提示；仍可在浏览器控制台(F12)里看到，用于确认线上版本
    printf("INFO: [QG] build C-0826 (WebGL2 + lens-text)\n");
    loadRecords();                                   // 此时 IndexedDB 已同步完成
    if (!rec.licenseAgreed) { prevScene = MENU; scene = LICENSE; }
    // 第三参数用 0：不模拟无限循环。simulate=1 会抛 "unwind" 内部异常，
    // 从 JS 回调链进入时无人捕获会显示 Uncaught；调度器本身不受影响。
    emscripten_set_main_loop(frameOnce, 0, 0);
}

// JS 侧在 FS.syncfs(true) 完成后回调这里，确保先读到云端(IndexedDB)存档再进游戏
extern "C" void EMSCRIPTEN_KEEPALIVE start_game() { webInitAndRun(); }
#endif

int main() {
#ifdef __EMSCRIPTEN__
    // 挂载 IDBFS 到 /working 并切工作目录，qgarden_record.txt 的读写都落在浏览器持久存储；
    // --preload-file 打包的字体放在 /fonts，不与存档目录冲突。
    // 存档同步是异步的：等 syncfs(true) 回调后再调 start_game()。
    EM_ASM(
        try {
            FS.mkdir('/working');
        } catch(e) {}
        FS.mount(IDBFS, {}, '/working');
        FS.chdir('/working');
        FS.syncfs(true, function(err) {
            if (err) console.log('[QG] 存档加载失败(首次运行属正常):', err);
            // {async:true}：start_game 内部含 ASYNCIFY 暂停点（如 emscripten_sleep），
            // 普通 ccall 接不住 unwind 会静默中断导致黑屏；async 模式返回 Promise 并正确接管。
            Module.ccall('start_game', null, null, null, { async: true });
        });
    );
    return 0;
#else
    // FLAG_MSAA_4X_HINT 只对默认帧缓冲有效。本游戏所有内容都画在
    // LoadRenderTexture 创建的 FBO 里，而 raylib 无法为其开启多重采样。
    // 因此改用超采样（SSAA）：以 SSAA 倍分辨率绘制，再双线性缩回。
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(VW, VH, "Quantum Garden 6.3");
    SetTargetFPS(60);
    SetExitKey(0);
    // 先解密协议文本，再建字体图集（字库扫描需要用到协议内容）
    printf("INFO: [QG] secure-text %s\n", loadSecureText() ? "loaded" : "FALLBACK");
    initHCol();
    initSfx();
    font = loadChineseFont();
    fontTitle = loadTitleFont();      // 标题字体：更粗字重，与正文形成层次
    target = LoadRenderTexture(VW * SSAA, VH * SSAA);
    SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);
    loadRecords();
    if (!rec.licenseAgreed) { prevScene = MENU; scene = LICENSE; }   // 首次启动强制展示版权声明

    // 反调试：每隔约 2 秒轮询一次。发布编译（-DANTI_DEBUG）时才真正生效。
    float debugCheckTimer = 0.0f;

    while (!WindowShouldClose() && !shouldQuit) {
        float dt = GetFrameTime(); gTime += dt;

        debugCheckTimer += dt;
        if (debugCheckTimer >= 2.0f) {
            debugCheckTimer = 0.0f;
            if (isDebuggerPresentCheck()) { shouldQuit = true; break; }
        }

        if (IsKeyPressed(KEY_F11)) toggleFull();
        if (IsKeyPressed(KEY_M)) muted = !muted;
        if (IsKeyPressed(KEY_ESCAPE) && shopOpen) { shopOpen = false; playSfx(sfxClick); }
        if (IsKeyPressed(KEY_H) && !shopOpen && !(scene == LICENSE && !rec.licenseAgreed)) {
            if (scene != HELP) { prevScene = scene; scene = HELP; } else scene = prevScene;
        }
        updateAmbient();

        gScale = std::min(GetScreenWidth() / (float)VW, GetScreenHeight() / (float)VH);
        // 鼠标映射必须与下方贴图的"整数像素吸附"完全一致，
        // 否则光标位置会与画面产生最多 1px 的偏移，影响点击判定。
        Vector2 rm = GetMousePosition();
        float mdw = floorf(VW * gScale), mdh = floorf(VH * gScale);
        float mdx = floorf((GetScreenWidth()  - mdw) * 0.5f);
        float mdy = floorf((GetScreenHeight() - mdh) * 0.5f);
        gMouse.x = (rm.x - mdx) / gScale;
        gMouse.y = (rm.y - mdy) / gScale;
        if (shake > 0) shake = std::max(0.0f, shake - dt * 40);

        BeginTextureMode(target);
            Camera2D ssCam = { 0 };
            ssCam.zoom = (float)SSAA;
            BeginMode2D(ssCam);
            drawBG(gTime);
            if (scene == MENU)           sceneMenu(gTime);
            else if (scene == HELP)      sceneHelp();
            else if (scene == TUT_SEL)   sceneTutSel();
            else if (scene == TUT_BRIEF) sceneTutBrief();
            else if (scene == ACHIEVE)   sceneAchieve(gTime);
            else if (scene == SKINSEL)   sceneSkin(gTime);
            else if (scene == META)      sceneMeta(gTime);
            else if (scene == STATS)     sceneStats();
            else if (scene == LICENSE)   sceneLicense();
            else if (scene == PLAY) {
                uiLock = shopOpen;
                scenePlay(dt, gTime);
                uiLock = false;
                if (shopOpen) sceneShop(gTime);
            }
            else sceneResult(gTime);
            drawFX(dt);
            if (luxActive()) drawLuxFrame(gTime);        // 豪华界面：覆盖在面板之上
            txtS(muted ? "音效已关闭 (M)" : "音效已开启 (M)",
                VW - 210.0f, VH - 26.0f, 18, muted ? GRAY : Fade(SKYBLUE, 0.6f));
            EndMode2D();
        EndTextureMode();

        float ox = (randF() - 0.5f) * shake, oy = (randF() - 0.5f) * shake;
        BeginDrawing();
            ClearBackground(BLACK);
            Rectangle src = { 0,0,(float)target.texture.width,-(float)target.texture.height };
            float dw = floorf(VW * gScale), dh = floorf(VH * gScale);
            float dx = floorf((GetScreenWidth()  - dw) * 0.5f + ox);
            float dy = floorf((GetScreenHeight() - dh) * 0.5f + oy);
            Rectangle dst = { dx, dy, dw, dh };
            DrawTexturePro(target.texture, src, dst, { 0,0 }, 0, WHITE);
        EndDrawing();
    }
    saveRecords();
    UnloadRenderTexture(target); UnloadFont(font);
    if (fontTitle.texture.id) UnloadFont(fontTitle);
    unloadSfx();
    CloseWindow();
    return 0;
#endif
}
