#!/usr/bin/env python3
# ============================================================
#  gen_font_subset.py - 重新生成游戏字体子集与内嵌资源
#
#  为什么需要它
#  ------------
#  游戏用 pyftsubset 裁剪过的 Noto Sans SC（1381 字形）打包，并在启动时按
#  "源码里出现过的所有字符" 建字体图集（LoadFontEx + 码点数组，见 src/util.cpp）。
#  因此新增界面文案时，如果新出现的汉字不在子集里，就会显示成问号/方块。
#  本脚本把整棵源码树里的字符扫一遍，重建 charset、字体子集与内嵌资源，
#  从根上杜绝漏字。
#
#  用法
#  ----
#    python tools/gen_font_subset.py            # 完整流程（需 fontTools）
#    python tools/gen_font_subset.py --check    # 只报告缺字，不改任何文件
#
#  缺 fontTools 时先装： pip install fonttools brotli
#
#  产物（请一并提交）
#  -----------------
#    tools/charset.txt          字符清单（单一文本行，供审阅/复用）
#    web/fonts/font.ttf         网页版正文字体（--preload-file 打包进 .data）
#    web/fonts/font_title.ttf   网页版标题字体
#    src/embedded_assets.h      桌面版内嵌字体/协议数据（由 build_embedded.py 生成）
#
#  源字体优先级：tools/font_src/*.ttf > web/fonts/*.ttf（必须是全量字体、
#  不能是上一轮的裁剪产物，否则裁剪会一轮比一轮少字）。
# ============================================================
import io
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)

# 需要覆盖的字符范围（务必包含 charset.txt 里已经在用的全部符号，
# 否则重生成会把 ← → ─ ○ ● ★ ☆ 这类界面符号裁掉，变成豆腐块）：
#   U+0020-007E  ASCII
#   U+00A0-00FF  拉丁补充（© × ÷ 等）
#   U+2000-206F  常用标点（— … 等）
#   U+2190-21FF  箭头（← →）
#   U+2500-257F  制表符（─）
#   U+25A0-25FF  几何图形（○ ●）
#   U+2600-26FF  杂项符号（★ ☆）
#   U+2E80-9FFF  CJK 部首/标点/假名/注音/汉字
#   U+FE30-FE4F  CJK 兼容形式
#   U+FF00-FFEF  全角符号
RANGES = [
    (0x0020, 0x007E),
    (0x00A0, 0x00FF),
    (0x2000, 0x206F),
    (0x2190, 0x21FF),
    (0x2500, 0x257F),
    (0x25A0, 0x25FF),
    (0x2600, 0x26FF),
    (0x2E80, 0x9FFF),
    (0xFE30, 0xFE4F),
    (0xFF00, 0xFFEF),
]

# 扫描范围：源码 + 数据清单（不扫二进制与构建产物）
SRC_DIRS = ["src", "tools"]
SRC_EXTS = (".h", ".cpp", ".txt")
SKIP_FILES = {"charset.txt", "eula_master.txt", "embedded_assets.h",
              "embedded_licenses.h"}


def wanted(ch):
    o = ord(ch)
    return any(lo <= o <= hi for lo, hi in RANGES)


# ============================================================
#  只扫描"会被画到屏幕上"的文字
#  ------------------------------------------------------------
#  代码注释里的汉字永远不会渲染，把它们算进字符集只会带来两个坏处：
#    1) 子集里混进一批用不到的字形，白占体积；
#    2) 换用裁剪过的字体当源字体时，注释里的生僻字会让本脚本报"缺字"，
#       把真正的缺字警告淹没掉。
#  所以这里先把注释剥掉（字符串字面量原样保留）再收集字符。
# ============================================================
def strip_comments(text):
    out = []
    i, n = 0, len(text)
    state = 0          # 0=普通代码 1=字符串 2=字符常量 3=行注释 4=块注释
    while i < n:
        c = text[i]
        nxt = text[i + 1] if i + 1 < n else ""
        if state == 0:
            if c == "/" and nxt == "/":
                state = 3
                i += 2
                continue
            if c == "/" and nxt == "*":
                state = 4
                i += 2
                continue
            if c == '"':
                state = 1
            elif c == "'":
                state = 2
            out.append(c)
        elif state == 1:
            out.append(c)
            if c == "\\":
                if i + 1 < n:
                    out.append(text[i + 1])
                    i += 2
                    continue
            elif c == '"':
                state = 0
        elif state == 2:
            out.append(c)
            if c == "\\":
                if i + 1 < n:
                    out.append(text[i + 1])
                    i += 2
                    continue
            elif c == "'":
                state = 0
        elif state == 3:
            if c == "\n":
                state = 0
                out.append(c)
        elif state == 4:
            if c == "*" and nxt == "/":
                state = 0
                i += 2
                continue
            if c == "\n":
                out.append(c)   # 保留换行，行号不错位
        i += 1
    return "".join(out)


def collect_from_sources():
    """扫描源码树里出现的所有目标范围字符（去掉注释，保持首次出现顺序）。"""
    seen = set()
    ordered = []
    for d in SRC_DIRS:
        root = os.path.join(REPO, d)
        for dirpath, _dirnames, filenames in os.walk(root):
            for fn in sorted(filenames):
                if not fn.endswith(SRC_EXTS) or fn in SKIP_FILES:
                    continue
                path = os.path.join(dirpath, fn)
                try:
                    text = io.open(path, encoding="utf-8-sig").read()
                except (UnicodeDecodeError, OSError):
                    continue
                if fn.endswith((".h", ".cpp")):
                    text = strip_comments(text)
                for ch in text:
                    if ch not in seen and wanted(ch):
                        seen.add(ch)
                        ordered.append(ch)
    return ordered


def load_existing_charset():
    path = os.path.join(HERE, "charset.txt")
    if not os.path.exists(path):
        return ""
    # utf-8-sig 去掉文件头 BOM；再显式剔除残留的 U+FEFF —— 这个零宽字符
    # 混进来会顺着"缺字"提示一路带到 print()，在 GBK 控制台上直接抛
    # UnicodeEncodeError 把脚本打断。
    text = io.open(path, encoding="utf-8-sig").read()
    return "".join(c for c in text.replace("\r", "").replace("\n", "")
                   if c != "\ufeff")


def font_cmap(path):
    from fontTools.ttLib import TTFont
    f = TTFont(path, fontNumber=0, lazy=True)
    cmap = set(f.getBestCmap().keys())
    f.close()
    return cmap


def pick_source(preferred, fallback):
    """优先用 tools/font_src 里的全量源字体，否则退回现有子集。"""
    p = os.path.join(HERE, "font_src", preferred)
    if os.path.exists(p):
        return p, "full"
    p = os.path.join(REPO, fallback)
    if os.path.exists(p):
        return p, "existing-subset"
    return None, None


def main():
    check_only = "--check" in sys.argv

    src_chars = collect_from_sources()
    old = load_existing_charset()
    src_wanted = set(src_chars)
    old_set = set(c for c in old if wanted(c))

    body_src, body_kind = pick_source("font_regular.ttf", "web/fonts/font.ttf")
    title_src, title_kind = pick_source("font_bold.ttf", "web/fonts/font_title.ttf")
    if not body_src or not title_src:
        print("ERROR: 找不到源字体（tools/font_src/font_regular.ttf 或 web/fonts/font.ttf）")
        return 1

    # ---- 决定这次要裁哪些字 ----
    # 关键：源字体决定上限。用现有子集当源字体时，它能提供的就只有已提交的那些
    # 字形，任何新增字都必须换成全量源字体才可能裁出来 —— 硬把它写进 charset
    # 反而会做出豆腐块。所以最终清单 = 源字体实际覆盖到的部分；覆盖不到的在
    # 下面单独列出来提醒。
    src_cmap = font_cmap(body_src) & font_cmap(title_src)
    merged = []
    seen = set()
    for ch in list(old) + src_chars:                 # 旧清单优先，保证"只增不减"
        if ch in seen or not wanted(ch):
            continue
        seen.add(ch)
        if ord(ch) in src_cmap:
            merged.append(ch)
    charset = "".join(merged)

    print("源码需要 %d 字（其中 %d 字是旧清单里没有的新增），源字体可提供 %d 字"
          % (len(src_wanted | old_set), len(src_wanted - old_set), len(src_cmap)))
    print("charset: 旧 %d 字 -> 新 %d 字%s"
          % (len(old_set), len(merged),
             "（本次无变化）" if set(merged) == old_set else ""))

    # ---- 缺字体检：源字体裁不出来的字 ----
    short = sorted((src_wanted | old_set) - set(merged))
    if short:
        print("  !! 源字体缺 %d 字，无法裁出（这些字会显示成豆腐块）：%s"
              % (len(short), "".join(short[:80])))
        print("     修法：把全量 Noto Sans SC 放到 tools/font_src/"
              "font_regular.ttf 与 font_bold.ttf，再重跑本脚本。")
        print("     注：若这些字只出现在代码注释里则无影响 —— 本脚本已剥离注释，"
              "能走到这里说明它们出现在字符串字面量中。")
    else:
        print("  源字体覆盖全部所需字符，无缺字")

    problems = 0
    for label, path, kind in (("正文", body_src, body_kind), ("标题", title_src, title_kind)):
        cmap = font_cmap(path)
        miss = [c for c in merged if ord(c) not in cmap]
        print("  %s源(%s): %s  覆盖 %d/%d"
              % (label, kind, os.path.basename(path), len(merged) - len(miss), len(merged)))
        if miss:
            problems += 1
            print("    !! 缺 %d 字: %s" % (len(miss), "".join(miss[:60])))

    if check_only:
        print("check-only: 未修改任何文件")
        return 0
    if problems:
        print("ERROR: 源字体缺字，先换全量源字体再跑（避免裁出豆腐块）")
        return 1

    from fontTools import subset

    # ---- 1) charset.txt ----
    io.open(os.path.join(HERE, "charset.txt"), "w", encoding="utf-8", newline="\n").write(charset)
    print("已写出 tools/charset.txt")

    # ---- 2) 字体子集 ----
    targets = [
        (body_src, os.path.join(REPO, "web", "fonts", "font.ttf")),
        (title_src, os.path.join(REPO, "web", "fonts", "font_title.ttf")),
    ]
    for src, dst in targets:
        args = [
            src,
            "--text=%s" % charset,
            "--output-file=%s" % dst,
            "--layout-features=*",
            "--glyph-names",
            "--no-hinting",
            "--desubroutinize",
            "--drop-tables+=DSIG",
            "--name-IDs=*",
            "--recalc-bounds",
        ]
        subset.main(args)
        print("  %s -> %s (%d 字节)" % (os.path.basename(src), os.path.relpath(dst, REPO),
                                        os.path.getsize(dst)))

    # ---- 3) 内嵌资源 ----
    gen = os.path.join(HERE, "build_embedded.py")
    if os.path.exists(gen):
        import subprocess
        r = subprocess.call([sys.executable, gen], cwd=HERE)
        if r != 0:
            print("ERROR: build_embedded.py 失败 (%d)" % r)
            return r
    else:
        print("提示: 未找到 tools/build_embedded.py，跳过 embedded_assets.h 生成")

    print("完成。请把 tools/charset.txt、web/fonts/*.ttf、src/embedded_assets.h 一并提交。")
    return 0


if __name__ == "__main__":
    sys.exit(main())
