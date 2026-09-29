#!/usr/bin/env python3
# ============================================================
#  build_embedded.py - 生成 src/embedded_assets.h（桌面版内嵌资源）
#
#  用途
#  ----
#  桌面版发布（build.bat single / 红熊猫单文件）不依赖外部文件，字体与加密
#  协议文本都以字节数组的形式内嵌在 embedded_assets.h 里，由 src/util.cpp
#  通过 qgEmbeddedFont()/qgEmbeddedText()/qgEmbeddedLicense() 读取。
#
#  本脚本只重写"字体"部分，QG_TEXT_BIN（加密协议）与许可全文原样保留 ——
#  协议文本要改请用 tools/qg_pack_text.py 重新打包 text.bin，不要动这里。
#
#  输入
#  ----
#    web/fonts/font.ttf         正文字体子集
#    web/fonts/font_title.ttf   标题字体子集
#    src/embedded_assets.h      现有文件（用于取出需要保留的 QG_TEXT_BIN 段）
#
#  用法
#  ----
#    python tools/build_embedded.py            # 写回 src/embedded_assets.h
#    python tools/build_embedded.py --root=<dir>
#        从 <dir>/web/fonts/*.ttf 读字体，写回 <dir>/src/embedded_assets.h
#        （在桌面版源码目录里复用同一脚本时用，例如 --root=.）
# ============================================================
import io
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)

for _a in sys.argv[1:]:
    if _a.startswith("--root="):
        REPO = os.path.abspath(_a.split("=", 1)[1])

HEADER_PATH = os.path.join(REPO, "src", "embedded_assets.h")
FONT_PATH = os.path.join(REPO, "web", "fonts", "font.ttf")
FONT_TITLE_PATH = os.path.join(REPO, "web", "fonts", "font_title.ttf")

BANNER = ("// 自动生成：内嵌字体与加密协议文本（桌面单文件发布用，请勿手改）\n"
          "// 重新生成：python tools/build_embedded.py\n"
          "// 字体子集：python tools/gen_font_subset.py（换字体或新增界面文案后跑）")


def read_bytes(path):
    with open(path, "rb") as f:
        return f.read()


def emit_array(name, data):
    """按现有格式输出：每行 24 个 0xNN，缩进两空格，行尾逗号。"""
    out = ["static const unsigned char %s[] = {" % name]
    line = []
    for i, b in enumerate(data):
        line.append("0x%02X" % b)
        if len(line) == 24:
            out.append("  " + ",".join(line) + ",")
            line = []
    if line:
        out.append("  " + ",".join(line) + ",")
    out.append("};")
    out.append("static const unsigned int %s_SIZE = %d;" % (name, len(data)))
    return "\n".join(out)


def extract_block(text, name):
    """从现有头文件里取出某个数组的完整定义（含 SIZE 行），原样保留。"""
    pattern = re.compile(
        r"static const unsigned char %s\[\] = \{.*?\n\};\n"
        r"static const unsigned int %s_SIZE = \d+;" % (re.escape(name), re.escape(name)),
        re.S)
    m = pattern.search(text)
    return m.group(0) if m else None


def main():
    for p in (FONT_PATH, FONT_TITLE_PATH):
        if not os.path.exists(p):
            print("ERROR: 缺少字体文件 %s（先跑 tools/gen_font_subset.py）" % p)
            return 1

    body = read_bytes(FONT_PATH)
    title = read_bytes(FONT_TITLE_PATH)

    # ---- 取出需要原样保留的段 ----
    old = io.open(HEADER_PATH, encoding="utf-8").read() if os.path.exists(HEADER_PATH) else ""
    keep = extract_block(old, "QG_TEXT_BIN")
    if keep is None:
        print("ERROR: 现有 %s 里找不到 QG_TEXT_BIN 段；协议数据不能由本脚本重建，"
              "请先恢复该文件（或改用 tools/qg_pack_text.py 重新打包 text.bin）"
              % os.path.relpath(HEADER_PATH, REPO))
        return 1

    # 许可全文数组（若存在）同样原样保留
    keep_lic = extract_block(old, "QG_LICENSE_BIN")

    parts = [BANNER, "#pragma once",
             emit_array("QG_FONT_TTF", body),
             emit_array("QG_FONT_TITLE_TTF", title),
             keep]
    if keep_lic:
        parts.append(keep_lic)

    text = "\n".join(parts) + "\n"
    io.open(HEADER_PATH, "w", encoding="utf-8", newline="\n").write(text)
    print("已写出 %s" % os.path.relpath(HEADER_PATH, REPO))
    print("  正文 %d 字节 / 标题 %d 字节 / 保留段 %s"
          % (len(body), len(title), "QG_TEXT_BIN" + (" + QG_LICENSE_BIN" if keep_lic else "")))
    return 0


if __name__ == "__main__":
    sys.exit(main())
