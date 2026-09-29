#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# ============================================================
#  界面视觉稿（改动预览，非游戏截图）
#  按 raylib 的真实坐标 / 字号 / 配色复刻 scenes.cpp 的 sceneMenu、
#  sceneSettings 与 drawClearConfirm，用于核对排版（越界 / 重叠 / 压字）。
#  覆盖：主菜单（含最下方的设置入口）、设置页（音效开 / 关两种状态）、
#        清空纪录二次确认弹窗。
#
#  为什么需要它：本机 GLFW 报 "Win32: Failed to create interface"，
#  raylib 无法创建窗口，没法实机截图，所以用 Pillow 按同一套坐标出图自查。
#
#  用法：python tools/preview_menu.py        输出到 tools/preview_*.png
#        python tools/preview_menu.py <目录>  输出到指定目录
# ============================================================
import os
import sys

from PIL import Image, ImageDraw, ImageFont

VW, VH = 1280, 720
TS = 1.20                                   # util.h 的 TEXT_SCALE
BODY = r"C:\Windows\Fonts\Deng.ttf"         # 与游戏同族的等线体
TITLE = r"C:\Windows\Fonts\Dengb.ttf"
OUT = sys.argv[1] if len(sys.argv) > 1 else os.path.dirname(os.path.abspath(__file__))

# 近似 raylib 的常用色
RAYWHITE = (245, 245, 245)
GOLD = (255, 203, 0)
GREEN = (0, 228, 48)
SKYBLUE = (102, 191, 255)
VIOLET = (135, 60, 190)
RED = (230, 41, 55)
ORANGE = (255, 161, 0)
GRAY = (130, 130, 130)
DARKGRAY = (80, 80, 80)
PINK = (255, 109, 194)


def fnt(sz, path=BODY):
    return ImageFont.truetype(path, int(round(sz * TS)))


def txt(d, s, x, y, sz, col, path=BODY, center=False):
    f = fnt(sz, path)
    if center:
        x -= d.textlength(s, font=f) / 2
    d.text((x, y), s, font=f, fill=col)


def button(d, x, y, w, h, label, base, sz=22):
    d.rounded_rectangle([x, y, x + w, y + h], radius=h * 0.18,
                        fill=tuple(int(c * 0.30) for c in base) + (255,),
                        outline=tuple(min(255, int(c * 0.75)) for c in base), width=2)
    f = fnt(sz)
    asc, desc = f.getmetrics()
    txt(d, label, x + (w - d.textlength(label, font=f)) / 2,
        y + (h - (asc + desc)) / 2, sz, (235, 238, 245))


def panel(d, x, y, w, h):
    d.rounded_rectangle([x, y, x + w, y + h], radius=12, fill=(22, 28, 44, 245),
                        outline=(60, 72, 104), width=2)


def backdrop(d):
    for y in range(VH):
        k = y / VH
        d.line([(0, y), (VW, y)], fill=(int(10 + 14 * k), int(14 + 18 * k), int(26 + 24 * k)))


def base_menu(d, muted=False):
    """主菜单（与 sceneMenu 的坐标一致）"""
    backdrop(d)
    txt(d, "量 子 花 园", VW / 2, 26, 58, (150, 205, 255), TITLE, center=True)
    txt(d, "Quantum Garden 6.3　·　量子基调", VW / 2, 96, 20, (150, 165, 190), center=True)

    bx = VW / 2 - 200
    button(d, bx, 132, 400, 48, "新 手 教 程   8 关，从零学会", GREEN, 23)
    for i, (nm, col) in enumerate([("休闲", SKYBLUE), ("标准", SKYBLUE),
                                   ("挑战", ORANGE), ("噩梦", RED)]):
        button(d, bx, 192 + i * 50, 400, 42, f"{nm}   20 回合 / 目标 200", col, 21)
    button(d, bx, 396, 400, 46, "无 尽 模 式   生存到极限", VIOLET, 22)
    button(d, bx, 452, 400, 46, "每 日 挑 战   全球同一牌局", GOLD, 22)
    button(d, bx, 512, 128, 40, "规则 (H)", DARKGRAY, 19)
    button(d, bx + 136, 512, 128, 40, "成就墙", GOLD, 19)
    button(d, bx + 272, 512, 128, 40, "星尘工坊", VIOLET, 19)
    button(d, bx, 560, 196, 40, "数据中心", SKYBLUE, 19)
    button(d, bx + 204, 560, 196, 40, "全屏 (F11)", DARKGRAY, 19)
    button(d, bx, 608, 400, 36, "关于 / 用户许可协议", DARKGRAY, 17)

    button(d, 40, 548, 250, 42, "继续游戏 / 存档管理 3/10", GREEN, 17)
    button(d, 40, 596, 250, 42, "继续新手引导 4 / 8", (60, 170, 110), 19)
    button(d, 40, 644, 250, 42, "关于 / 开源许可", SKYBLUE, 18)
    # 本次新增：主菜单最下方的设置入口
    button(d, 40, 692, 1200, 28, "设置（音效 / 清空纪录）", DARKGRAY, 16)

    panel(d, 40, 132, 250, 400)
    txt(d, "历史最佳记录", 62, 143, 22, GOLD)
    txt(d, "教程进度：3 / 8 关", 62, 175, 18, GREEN)
    for i, nm in enumerate(["休闲", "标准", "挑战", "噩梦"]):
        txt(d, f"{nm}：{1200 + i * 300}", 62, 201 + i * 24, 18, RAYWHITE)
    for i, (k, v, c) in enumerate([("无尽回合：", "37", VIOLET),
                                   ("每日最佳：", "880", SKYBLUE),
                                   ("连续天数：", "4 天", SKYBLUE),
                                   ("最高连击：", "x12", PINK),
                                   ("薛定谔之猫：", "3", GOLD),
                                   ("成就：", "9 / 32", GOLD),
                                   ("星尘：", "145", VIOLET)]):
        txt(d, k + v, 62, 301 + i * 24, 18, c)
    txt(d, "终极成就：未解锁", 62, 473, 18, GRAY)

    txt(d, "最近战绩曲线", VW - 350, 138, 21, SKYBLUE)
    d.rectangle([VW - 350, 164, VW - 40, 350], outline=(70, 84, 116), width=1)
    d.ellipse([VW - 225, 410, VW - 165, 470], outline=(240, 150, 200), width=3)
    txt(d, "音效已关闭 (M)" if muted else "音效已开启 (M)", VW - 210, VH - 26, 18,
        GRAY if muted else SKYBLUE)


def base_settings(d, muted=False, cleared=False):
    """设置页（与 sceneSettings 的坐标一致）"""
    backdrop(d)
    txt(d, "音效已关闭 (M)" if muted else "音效已开启 (M)", VW - 210, VH - 26, 18,
        GRAY if muted else SKYBLUE)

    panel(d, 140, 60, VW - 280, 540)
    txt(d, "设 置", VW / 2, 76, 36, (150, 205, 255), TITLE, center=True)
    txt(d, "设置会即时生效，不需要确认", VW / 2, 126, 19, GRAY, center=True)

    rowX, rowW = 210, VW - 420

    # ---------- 音效 ----------
    txt(d, "音效", rowX + 10, 184, 26, RAYWHITE)
    txt(d, "当前：已关闭（按 M 键也可切换）" if muted else "当前：已开启（按 M 键也可切换）",
        rowX + 10, 220, 20, GRAY if muted else SKYBLUE)
    button(d, rowX + rowW - 200, 180, 190, 54, "开 启 音 效" if muted else "关 闭 音 效",
           GREEN if muted else DARKGRAY, 22)

    d.line([rowX, 262, rowX + rowW, 262], fill=(255, 255, 255, 40), width=2)

    # ---------- 清空纪录 ----------
    txt(d, "游戏纪录", rowX + 10, 282, 26, RAYWHITE)
    txt(d, "分数、成就、装扮、星尘、升级、教程进度与全部对局存档", rowX + 10, 318, 20, GRAY)
    txt(d, "旧版本存档格式不兼容时，清空即可解决冲突", rowX + 10, 344, 20, GRAY)
    txt(d, "清空后需重启游戏", rowX + 10, 380, 22, ORANGE)
    button(d, rowX + rowW - 230, 356, 220, 54, "清 空 纪 录", RED, 22)

    if cleared:
        txt(d, "纪录已清空，请重启游戏以回到初次启动状态。", rowX + 10, 424, 20, GREEN)

    button(d, VW / 2 - 110, VH - 94, 220, 42, "返回主菜单", DARKGRAY, 20)


def render_menu(path):
    img = Image.new("RGB", (VW, VH))
    d = ImageDraw.Draw(img, "RGBA")
    base_menu(d)
    img.save(path)
    print("wrote", path)


def render_settings(path, muted=False, cleared=False):
    img = Image.new("RGB", (VW, VH))
    d = ImageDraw.Draw(img, "RGBA")
    base_settings(d, muted=muted, cleared=cleared)
    img.save(path)
    print("wrote", path)


def render_confirm(path):
    """清空纪录二次确认弹窗（叠在设置页上）"""
    img = Image.new("RGB", (VW, VH))
    d = ImageDraw.Draw(img, "RGBA")
    base_settings(d)
    d.rectangle([0, 0, VW, VH], fill=(0, 0, 0, 184))

    boxW, boxH = 940, 352
    bx, by = (VW - boxW) / 2, (VH - boxH) / 2 - 20
    panel(d, bx, by, boxW, boxH)
    d.rounded_rectangle([bx, by, bx + boxW, by + boxH], radius=12, outline=RED, width=3)

    txt(d, "清 空 游 戏 纪 录", VW / 2, by + 24, 30, (235, 90, 90), TITLE, center=True)
    txt(d, "将永久删除以下全部内容，且无法恢复：", VW / 2, by + 86, 20, RAYWHITE, center=True)
    txt(d, "分数纪录 · 成就 · 装扮 · 星尘 · 升级 · 教程与引导进度 · 全部对局存档",
        VW / 2, by + 120, 20, GOLD, center=True)
    txt(d, "旧版本存档格式不兼容时，用这里清空即可解决冲突。",
        VW / 2, by + 154, 19, GRAY, center=True)
    txt(d, "清空后需重启游戏：重新打开才会回到初次启动状态，",
        VW / 2, by + 190, 20, ORANGE, center=True)
    txt(d, "并重新弹出用户协议与新手引导。", VW / 2, by + 220, 20, ORANGE, center=True)

    button(d, bx + 60, by + boxH - 64, 230, 46, "取消", DARKGRAY, 21)
    button(d, bx + boxW - 290, by + boxH - 64, 230, 46, "确认清空", RED, 21)
    img.save(path)
    print("wrote", path)


render_menu(os.path.join(OUT, "preview_menu.png"))
render_settings(os.path.join(OUT, "preview_settings.png"), muted=False)
render_settings(os.path.join(OUT, "preview_settings_muted.png"), muted=True, cleared=True)
render_confirm(os.path.join(OUT, "preview_confirm.png"))
