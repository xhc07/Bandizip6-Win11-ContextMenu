@echo off
setlocal EnableExtensions
cd /d "%~dp0"
title BandiZip11 Setup

set "RESTART_MODE=0"
set "ACTION="
set "CLI_MODE=0"
set "RC=0"
if not "%~1"=="" (
    for %%A in (%*) do (
        if /i "%%~A"=="--restart" set "RESTART_MODE=1"
        if /i "%%~A"=="--no-restart" set "RESTART_MODE=2"
        if /i "%%~A"=="install" set "ACTION=install"
        if /i "%%~A"=="uninstall" set "ACTION=uninstall"
    )
)
if defined ACTION set "CLI_MODE=1"

set "ISADMIN=0"
fltmc >nul 2>&1
if not errorlevel 1 set "ISADMIN=1"

if defined ACTION goto %ACTION%

:menu
cls
echo ========================================
echo  BandiZip11 Setup
echo ========================================
echo.
echo  [1] Install Bandizip 11 context menu
echo  [2] Uninstall Bandizip 11 context menu
echo  [0] Exit
echo.
choice /c 120 /n /m "Select 1, 2 or 0: "
if errorlevel 3 goto exit
if errorlevel 2 goto uninstall
if errorlevel 1 goto install
goto menu

:install
if not exist "%~dp0install.ps1" (
    echo [ERROR] install.ps1 was not found next to this script.
    set "RC=1"
    goto exit
)
if not exist "%~dp0dist\BandiZip11Menu.dll" (
    echo [ERROR] dist\BandiZip11Menu.dll was not found.
    set "RC=1"
    goto exit
)

call :ensure_dev_mode
if errorlevel 1 (
    echo [ERROR] Could not enable Developer Mode. UAC may have been cancelled.
    set "RC=1"
    goto exit
)

echo.
echo Installing...
echo.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0install.ps1"
if errorlevel 1 (
    echo.
    echo [ERROR] Installation failed. See the messages above.
    set "RC=1"
    goto exit
)

echo.
echo Installation completed.
call :ask_restart
set "RC=0"
goto exit

:uninstall
if not exist "%~dp0uninstall.ps1" (
    echo [ERROR] uninstall.ps1 was not found next to this script.
    set "RC=1"
    goto exit
)

echo.
echo Uninstalling...
echo.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0uninstall.ps1"
if errorlevel 1 (
    echo.
    echo [ERROR] Uninstall failed. The package may still be in use.
    if "%ISADMIN%"=="0" (
        choice /c 12 /n /m "Retry with administrator rights? [1=Yes 2=No] "
        if errorlevel 2 goto uninstall_failed
        call :elevated_uninstall
        if errorlevel 1 goto uninstall_failed
    ) else (
        goto uninstall_failed
    )
)

echo.
echo Uninstall completed.
call :ask_restart
set "RC=0"
goto exit

:uninstall_failed
echo.
echo Close File Explorer, or restart Windows, then try again.
set "RC=1"
goto exit

:ensure_dev_mode
set "DEVMODE="
for /f "tokens=3" %%A in ('reg query "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\AppModelUnlock" /v AllowDevelopmentWithoutDevLicense 2^>nul ^| findstr /i "AllowDevelopmentWithoutDevLicense"') do set "DEVMODE=%%A"

if /i "%DEVMODE%"=="0x1" (
    echo Developer Mode: enabled.
    exit /b 0
)

echo Developer Mode: not enabled.
echo Requesting administrator privileges to enable it...
powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "$ErrorActionPreference='Stop'; try { $p = Start-Process -FilePath 'reg.exe' -Verb RunAs -Wait -PassThru -ArgumentList 'add','HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\AppModelUnlock','/v','AllowDevelopmentWithoutDevLicense','/t','REG_DWORD','/d','1','/f'; exit $p.ExitCode } catch { exit 1 }"
if errorlevel 1 exit /b 1
echo Developer Mode enabled.
exit /b 0

:elevated_uninstall
powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "$ErrorActionPreference='Stop'; try { $p = Start-Process -FilePath 'powershell.exe' -Verb RunAs -Wait -PassThru -ArgumentList '-NoProfile','-ExecutionPolicy','Bypass','-File','%~dp0uninstall.ps1'; exit $p.ExitCode } catch { exit 1 }"
exit /b %ERRORLEVEL%

:ask_restart
if "%RESTART_MODE%"=="1" goto ask_restart_do
if "%RESTART_MODE%"=="2" goto ask_restart_no

if "%ISADMIN%"=="1" (
    echo.
    echo This window is elevated, so automatic Explorer restart is skipped.
    echo Close this window and start Explorer from a normal command prompt.
    exit /b 0
)

echo.
echo Restart File Explorer now?
echo   [1] Yes
echo   [2] No
choice /c 12 /n /m "Select 1 or 2: "
if errorlevel 2 goto ask_restart_no
if errorlevel 1 goto ask_restart_do
goto ask_restart_no

:ask_restart_do
echo.
echo Restarting File Explorer...
taskkill /f /im explorer.exe >nul 2>&1
timeout /t 2 /nobreak >nul
start "" explorer.exe
echo File Explorer restarted.
exit /b 0

:ask_restart_no
echo.
echo Skipped. Restart File Explorer or sign out later to load the changes.
exit /b 0

:exit
if "%CLI_MODE%"=="1" exit /b %RC%
echo.
echo Press any key to close this window...
pause >nul
exit /b %RC%
