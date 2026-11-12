@echo off
setlocal
cd /d "%~dp0"

if exist "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" (
    for /f "usebackq tokens=*" %%i in (`"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
        set "VS_DIR=%%i"
    )
)

if defined VS_DIR (
    call "%VS_DIR%\VC\Auxiliary\Build\vcvars64.bat" >nul
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
) else (
    echo [ERROR] Visual Studio C++ Build Tools bulunamadi.
    exit /b 1
)

if not exist build mkdir build
rc.exe /nologo /fo "build\installer.res" "installer.rc"
if errorlevel 1 exit /b 1

cl.exe /nologo /O2 /GL /MT /EHsc /std:c++17 /utf-8 /W3 /guard:cf /Gy /Gw ^
    "main_gui.cpp" "build\installer.res" ^
    /Fe"build\Umineko_TR_Kurulum.exe" /Fo"build\\" ^
    /link /LTCG /SUBSYSTEM:WINDOWS /DYNAMICBASE /HIGHENTROPYVA /NXCOMPAT /CETCOMPAT ^
    /GUARD:CF /OPT:REF /OPT:ICF /INCREMENTAL:NO
if errorlevel 1 exit /b 1

echo [OK] Derleme tamamlandi: build\Umineko_TR_Kurulum.exe
