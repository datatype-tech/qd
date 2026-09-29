#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# ============================================================
#  主菜单视觉稿（改动预览，非游戏截图）
#  按 raylib 的真实坐标 / 字号 / 配色复刻 scenes.cpp 的 sceneMenu 与
#  drawClearConfirm，用于核对新增"清空设置"入口与确认弹窗的排版。
#  环境无法创建 OpenGL 设备（GLFW 报 Failed to create interface），
#  所以用 Pillow 按同一套坐标出图代替实机截图。
#  用法：python tools/preview_menu.py   （输出到 tools/preview_*.png）
# ============================================================
import os
from PIL import Image, ImageDraw, ImageFont

VW, VH = 1280, 720
TS = 1.20                                    # util.h 的 TEXT_SCALE
BODY = r"C:\Windows\Fonts\Deng.ttf"          # 与游戏同族的等线体
TITLE = r"C:\Windows\Fonts\Dengb.ttf"
OUT = os.path.dirname(os.path.abspath(__file__))


def fnt(p, sz):
    return ImageFont.truetype(p, int(round(sz * TS)))


def txt(d, s, x, y, sz, col, path=BODY, center=False):
    f = fnt(path, sz)
    if center:
        x -= d.textlength(s, font=f) / 2
    d.text((x, y), s, font=f, fill=col)


def button(d, x, y, w, h, label, base, sz=22):
    d.rounded_rectangle([x, y, x + w, y + h], radius=h * 0.18,
                        fill=tuple(int(c * 0.30) for c in base) + (255,),
                        outline=tuple(min(255, int(c * 0.75)) for c in base), width=2)
    f = fnt(BODY, sz)
    asc, desc = f.getmetrics()
    txt(d, label, x + (w - d.textlength(label, font=f)) / 2,
        y + (h - (asc + desc)) / 2, sz, (235, 238, 245))


def panel(d, x, y, w, h):
    d.rounded_rectangle([x, y, x + w, y + h], radius=12, fill=(22, 28, 44, 245),
                        outline=(60, 72, 104), width=2)


def base_menu(d):
    for y in range(VH):
        k = y / VH
        d.line([(0, y), (VW, y)], fill=(int(10 + 14 * k), int(14 + 18 * k), int(26 + 24 * k)))

    txt(d, "量 子 花 园", VW / 2, 26, 58, (150, 205, 255), TITLE, center=True)
    txt(d, "Quantum Garden 6.3　·　量子基调", VW / 2, 96, 20, (150, 165, 190), center=True)

    bx = VW / 2 - 200
    button(d, bx, 132, 400, 48, "新 手 教 程   8 关，从零学会", (60, 190, 120), 23)
    for i, (nm, col) in enumerate([("休闲", (90, 170, 230)), ("标准", (90, 170, 230)),
                                   ("挑战", (240, 160, 60)), ("噩梦", (230, 80, 80))]):
        button(d, bx, 192 + i * 50, 400, 42, f"{nm}   20 回合 / 目标 200", col, 21)
    button(d, bx, 396, 400, 46, "无 尽 模 式   生存到极限", (170, 110, 240), 22)
    button(d, bx, 452, 400, 46, "每 日 挑 战   全球同一牌局", (230, 190, 70), 22)
    button(d, bx, 512, 128, 40, "规则 (H)", (70, 74, 88), 19)
    button(d, bx + 136, 512, 128, 40, "成就墙", (230, 190, 70), 19)
    button(d, bx + 272, 512, 128, 40, "星尘工坊", (170, 110, 240), 19)
    button(d, bx, 560, 196, 40, "数据中心", (90, 170, 230), 19)
    button(d, bx + 204, 560, 196, 40, "全屏 (F11)", (70, 74, 88), 19)
    button(d, bx, 608, 400, 36, "关于 / 用户许可协议", (70, 74, 88), 17)

    button(d, 40, 548, 250, 42, "继续游戏 / 存档管理 3/10", (60, 190, 120), 17)
    button(d, 40, 596, 250, 42, "继续新手引导 4 / 8", (60, 170, 110), 19)
    button(d, 40, 644, 250, 42, "关于 / 开源许可", (90, 170, 230), 18)

    panel(d, 40, 132, 250, 400)
    txt(d, "历史最佳记录", 62, 143, 22, (230, 190, 70))
    txt(d, "教程进度：3 / 8 关", 62, 175, 18, (60, 190, 120))
    for i, nm in enumerate(["休闲", "标准", "挑战", "噩梦"]):
        txt(d, f"{nm}：{1200 + i * 300}", 62, 201 + i * 24, 18, (225, 230, 240))
    for i, (k, v, c) in enumerate([("无尽回合：", "37", (170, 110, 240)),
                                   ("每日最佳：", "880", (90, 170, 230)),
                                   ("连续天数：", "4 天", (90, 170, 230)),
                                   ("最高连击：", "x12", (240, 150, 200)),
                                   ("薛定谔之猫：", "3", (230, 190, 70)),
                                   ("成就：", "9 / 32", (230, 190, 70)),
                                   ("星尘：", "145", (170, 110, 240))]):
        txt(d, k + v, 62, 301 + i * 24, 18, c)
    txt(d, "终极成就：未解锁", 62, 473, 18, (150, 155, 170))

    txt(d, "最近战绩曲线", VW - 350, 138, 21, (90, 170, 230))
    d.rectangle([VW - 350, 164, VW - 40, 350], outline=(70, 84, 116), width=1)
    d.ellipse([VW - 225, 410, VW - 165, 470], outline=(240, 150, 200), width=3)
    txt(d, "音效已开启 (M)", VW - 210, VH - 26, 18, (110, 190, 230))

    # ↓↓↓ 本次新增：主菜单最下方的"清空设置"通栏入口
    button(d, 40, 692, 1200, 28, "清空设置（游戏纪录）", (70, 74, 88), 16)


def render_menu(path, toast):
    img = Image.new("RGB", (VW, VH))
    d = ImageDraw.Draw(img, "RGBA")
    base_menu(d)
    if toast:
        # 清空完成后的短暂提示，位于按钮标签与右下角音效文字之间的空档
        txt(d, "已清空游戏纪录，回到初次启动状态。", 760, 697, 14, (90, 230, 140))
    img.save(path)
    print("wrote", path)


def render_modal(path):
    img = Image.new("RGB", (VW, VH))
    d = ImageDraw.Draw(img, "RGBA")
    base_menu(d)
    d.rectangle([0, 0, VW, VH], fill=(0, 0, 0, 184))

    boxW, boxH = 940, 320
    bx, by = (VW - boxW) / 2, (VH - boxH) / 2 - 20
    panel(d, bx, by, boxW, boxH)
    d.rounded_rectangle([bx, by, bx + boxW, by + boxH], radius=12, outline=(230, 80, 80), width=3)

    txt(d, "清 空 游 戏 纪 录", VW / 2, by + 24, 30, (235, 90, 90), TITLE, center=True)
    txt(d, "将永久删除以下全部内容，且无法恢复：", VW / 2, by + 86, 20, (235, 238, 245), center=True)
    txt(d, "分数纪录 · 成就 · 装扮 · 星尘 · 升级 · 教程与引导进度 · 全部对局存档",
        VW / 2, by + 120, 20, (230, 190, 70), center=True)
    txt(d, "清空后回到第一次打开游戏的状态：重新弹出用户协议与新手引导。",
        VW / 2, by + 154, 19, (90, 170, 230), center=True)
    txt(d, "旧版本存档格式不兼容时，用这里清空即可解决冲突。",
        VW / 2, by + 184, 19, (150, 155, 170), center=True)

    button(d, bx + 60, by + boxH - 64, 230, 46, "取消", (70, 74, 88), 21)
    button(d, bx + boxW - 290, by + boxH - 64, 230, 46, "确认清空", (230, 80, 80), 21)
    img.save(path)
    print("wrote", path)


render_menu(os.path.join(OUT, "preview_menu.png"), toast=False)
render_menu(os.path.join(OUT, "preview_menu_toast.png"), toast=True)
render_modal(os.path.join(OUT, "preview_confirm.png"))
