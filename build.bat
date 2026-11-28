@echo off
setlocal enabledelayedexpansion

set ROOT=%~dp0
set ROOT=%ROOT:~0,-1%
set BUILD_DIR=%ROOT%\build
set QT_PREFIX=D:\C_Project_IDE\QT\6.12.0\msvc2022_64
set CMAKE_EXE=D:\C_Project_IDE\QT\Tools\CMake_64\bin\cmake.exe
set NINJA_EXE=D:\C_Project_IDE\QT\Tools\Ninja\ninja.exe
set VCVARS="D:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
set REDIST=D:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.44.35112\x64\Microsoft.VC143.CRT
set RAR="C:\Program Files\WinRAR\Rar.exe"
set RELEASE_DIR=%ROOT%\release
set OUT=%RELEASE_DIR%\Serial_port_debug.exe

echo ============================================
echo  Serial Port Debug - One-click Build
echo ============================================

echo [1/5] Init MSVC environment...
call %VCVARS% >nul
if errorlevel 1 goto :fail

echo [2/5] CMake configure...
%CMAKE_EXE% -S "%ROOT%" -B "%BUILD_DIR%" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="%QT_PREFIX%" -DCMAKE_MAKE_PROGRAM="%NINJA_EXE%"
if errorlevel 1 goto :fail

echo [3/5] Compile...
%CMAKE_EXE% --build "%BUILD_DIR%" --config Release
if errorlevel 1 goto :fail

echo [4/5] Prepare minimal runtime...
set LITE=%BUILD_DIR%\deploy_lite
if exist "%LITE%" rd /s /q "%LITE%"
mkdir "%LITE%\platforms" 2>nul
mkdir "%LITE%\styles" 2>nul

copy /Y "%BUILD_DIR%\Serial_port_debug.exe" "%LITE%\" >nul
for %%F in (Qt6Core.dll Qt6Gui.dll Qt6Widgets.dll Qt6SerialPort.dll Qt6Network.dll) do copy /Y "%QT_PREFIX%\bin\%%F" "%LITE%\" >nul
copy /Y "%QT_PREFIX%\plugins\platforms\qwindows.dll" "%LITE%\platforms\" >nul
copy /Y "%QT_PREFIX%\plugins\styles\qmodernwindowsstyle.dll" "%LITE%\styles\" >nul
copy /Y "%REDIST%\*.dll" "%LITE%\" >nul

echo [5/5] Package to single exe...
if not exist "%RELEASE_DIR%" mkdir "%RELEASE_DIR%"
> "%BUILD_DIR%\sfx.conf" echo Setup=Serial_port_debug.exe
>> "%BUILD_DIR%\sfx.conf" echo TempMode
>> "%BUILD_DIR%\sfx.conf" echo Silent=1
>> "%BUILD_DIR%\sfx.conf" echo Overwrite=1
if exist "%OUT%" del /f /q "%OUT%" >nul 2>&1
%RAR% a -r -ep1 -sfx -z"%BUILD_DIR%\sfx.conf" "%OUT%" "%LITE%\" >nul
if errorlevel 1 goto :fail

echo.
echo ============================================
echo SUCCESS: %OUT%
for %%A in ("%OUT%") do echo Size: %%~zA bytes
echo NOTE: Single-file SFX. Extracts to temp dir at runtime, auto-cleans on exit.
echo ============================================
pause
exit /b 0

:fail
echo.
echo BUILD FAILED
pause
exit /b 1
