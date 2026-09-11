// ============================================================
//  guard.h  反调试检测（仅在发布编译时通过 -DANTI_DEBUG 生效）
//  【维护说明】此模块故意不 include 任何 raylib 相关头文件，
//  因为 windows.h 里的 GDI/User32 函数名（如 Rectangle、CloseWindow）
//  会和 raylib 的同名函数/结构体冲突。保持独立编译单元即可避免此问题。
//  用红熊猫 C++ 直接编译调试（不带 -DANTI_DEBUG）时此检测不生效，
//  不会干扰你自己用 IDE 内置调试器（gdb）调试游戏逻辑。
//  只有正式发布用 build.bat 编译时才会打开。
// ============================================================
#pragma once

// 返回 true 表示检测到调试器正在附加到本进程。
// 未定义 ANTI_DEBUG 宏时，恒返回 false（不影响开发期调试）。
bool isDebuggerPresentCheck();
