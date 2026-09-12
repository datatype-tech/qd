// ============================================================
//  util.h  通用工具函数：随机数、文本绘制、UI 按钮、几何辅助、字体加载
// ============================================================
#pragma once
#include "types.h"
#include "data.h"

// ==================== 字体 ====================
// 【图集分辨率：以超采样参考图为基准实测得出】
// 正文字号集中在 18~25px。以 8 倍超采样图为"理论正确"基准逐档对比：
//   图集     笔画墨量误差    边缘过渡带宽（越窄越锐）
//    20px      +0.16%            1.88 px   ← 1:1，但边缘最虚
//    48px      -0.50%            1.16 px
//    96px      -0.38%            0.62 px   ← 最锐且笔画无损失
//   128px      -2.53%            0.48 px   ← 笔画开始丢失，字变残
// 结论：96px 是拐点，边缘最锐利且不丢笔画；再往上会因欠采样掉笔画。
// 另：绝不生成 mipmap —— GenTextureMipmaps + TRILINEAR 在小字号会取到
// 模糊的低阶 mipmap，实测边缘锐度直接腰斩（梯度 213 → 102，灰边 31%→70%）。
const int FONT_ATLAS_PX  = 128;
// 标题字号最大到 58px，3x 超采样下需 174px 才不放大；用 192px 图集保证
// 大字标题在全屏下依然锐利（正文 128px 图集，正文最大约 30px×3=90px）。
const int TITLE_ATLAS_PX = 192;

// font       正文字体
// fontTitle  标题 / 强调字体（更粗字重）
Font loadChineseFont();
Font loadTitleFont();

int randInt(int lo, int hi);
float randF();
Color icol(int i);

void txt(const string& s, float x, float y, float sz, Color c);
void txtC(const string& s, float cx, float y, float sz, Color c);

// ---------- 有质感的文字绘制 ----------
// 带柔和投影的正文：让文字从背景中"浮"起来，避免糊在幕布上
void txtS(const string& s, float x, float y, float sz, Color c);
void txtSC(const string& s, float cx, float y, float sz, Color c);
// 描边文字：深色描边包住字形，在任何幕布上都清晰
void txtOutline(const string& s, float x, float y, float sz, Color c, Color oc, float w = 1.6f);
// 标题：按当前风格自动加字距、描边、辉光，可选副标题式弱化
void txtTitle(const string& s, float cx, float y, float sz, Color c, bool glow = true);
// 字距可控的文字（用于标题的呼吸感排布）
void txtTracked(const string& s, float x, float y, float sz, float spacing, Color c);
Vector2 measureTracked(const string& s, float sz, float spacing);
void addLog(const string& s, Color c);
void addFloat(Vector2 p, const string& s, Color c, float sz = 30);
void emitBurst(Vector2 p, int n, Color c, float spd, float sz);
void addShock(Vector2 p, float r, Color c);
void addShake(float a);
Rectangle cellRect(int i);
Vector2 cellCenter(int i);

bool uiButton(Rectangle r, const string& label, Color base, bool active = false,
              float fs = 22, bool enabled = true);

// 新手引导期间：只有与高亮区域重叠的控件才允许交互（其余点击/悬停一律屏蔽）
bool guideAllows(Rectangle r);

// ==================== 风格化 UI 构件 ====================
// 面板：按当前风格绘制底色、描边与角部装饰
void uiPanel(Rectangle r, float roundness = 0.04f);
// 面板（指定强调色描边）
void uiPanelAccent(Rectangle r, Color edge, float roundness = 0.04f);
