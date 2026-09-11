============================================================
量子花园 6.3 网页版（WebAssembly）
============================================================

【怎么玩】
  双击 run_web.bat，浏览器会自动打开 http://localhost:8080
  首次加载需读取约 30MB 资源（字体+程序），等待数秒。
  存档自动保存在浏览器的 IndexedDB 里，同一浏览器再次打开会续档。

【文件结构】
  src\            游戏源码（从桌面版 AI 文件夹复制并做了网页适配）
  web\index.html  构建产物入口
  web\index.js / .wasm / .data   构建产物其余部分
  web\shell.html  网页外壳模板（页面标题、画布缩放逻辑）
  web\fonts\      打包进网页的字体（等线/等线Bold，来自本机系统字体，
                  仅限本机个人使用；如要公开发布请替换为可自由分发的字体，
                  例如思源黑体 Noto Sans SC，替换后重新构建即可）
  build_web.bat   一键重新构建
  run_web.bat     一键启动本地服务器试玩

【源码相对桌面版的改动】
  1. main.cpp：新增 __EMSCRIPTEN__ 分支 —— 先挂载 IDBFS 并从 IndexedDB
     同步存档，完成后回调 start_game() 初始化并进入 emscripten 主循环；
     桌面版逻辑原样保留在 #else 分支。
  2. save.cpp：saveRecords() 写完本地虚拟文件后，调用 FS.syncfs(false)
     把改动同步回浏览器持久存储（否则刷新页面就丢档）。
  3. util.cpp：字体搜索路径最前面加了 /fonts/font.ttf 和
     /fonts/font_title.ttf（即 --preload-file 打包进网页的字体）。
  4. guard.cpp（Windows 反调试）不参与网页版编译。

【重新构建前置条件】（换电脑后需要重做一次）
  1. Emscripten SDK 安装在 D:\emsdk（当前版本 6.0.8）
     注意 emsdk 要求 Python>=3.10，绿色版 Python 放在 D:\emsdk_dl\py312
  2. Web 版 raylib 库：D:\emsdk_dl\raylib_x\raylib-5.5\src\libraylib.a
     编译方法：
       cd D:\emsdk_dl\raylib_x\raylib-5.5\src
       （用 D:\emsdk\upstream\emscripten\em++.py 逐个执行）
       em++ -c -O2 -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2 ^
         rcore.c rshapes.c rtextures.c rtext.c utils.c raudio.c
       llvm-ar rcs libraylib.a rcore.o rshapes.o rtextures.o rtext.o utils.o raudio.o

【已知限制】
  - F11 全屏由浏览器接管，游戏内"全屏 (F11)"按钮在网页版可能无效，
    直接按浏览器自己的 F11 即可。
  - 浏览器禁止自动播放声音，第一次点击页面后音效才会正常响起。
  - "不同意，退出游戏"在网页上无法真正关闭标签页，只会停在提示画面。
  - file:// 双击直接打开 index.html 无法运行（浏览器安全策略），
    必须通过 run_web.bat 的 http 方式访问。
