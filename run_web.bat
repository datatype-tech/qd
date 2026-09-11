@echo off
setlocal enabledelayedexpansion
REM ============================================================
REM  run_web.bat - Start Quantum Garden web version
REM  Picks the first free port from 8080-8095 (stale servers
REM  from previous attempts may occupy old ports), then opens
REM  your default browser.
REM  To stop: just close this console window.
REM ============================================================
cd /d "%~dp0web"
if not exist index.html (
  echo [ERROR] web\index.html not found. Run build_web.bat first.
  pause
  exit /b 1
)

set PORT=
for /L %%p in (8080,1,8095) do (
  if not defined PORT (
    netstat -an | findstr /C:":%%p " | findstr /I "LISTENING" >nul 2>&1
    if errorlevel 1 set PORT=%%p
  )
)
if not defined PORT (
  echo [ERROR] All ports 8080-8095 are busy.
  echo Close old local-server windows first, then run this again.
  pause
  exit /b 1
)

echo Starting Quantum Garden at http://localhost:!PORT!
echo First load downloads ~30MB of assets, please wait a few seconds.
echo Close THIS window to stop the server.
start "" "http://localhost:!PORT!"
REM serve.py = http.server + "Cache-Control: no-store", so after each
REM rebuild the browser always fetches fresh index.js/.wasm/.data
REM (plain http.server lets stale cached copies linger and show old builds).
python "%~dp0serve.py" !PORT!
endlocal
