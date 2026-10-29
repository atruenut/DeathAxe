@echo off
setlocal EnableDelayedExpansion
REM ============================================================================
REM  DeathAxe - Windows build script (MinGW-w64 g++  ->  DeathAxe.vst3, a plain x64 DLL)
REM
REM    build.bat            compile to dist\DeathAxe.vst3
REM    build.bat install    compile, then copy into %CommonProgramFiles%\VST3  (run as Administrator)
REM    build.bat clean      delete build\ and dist\
REM
REM  Needs:  g++ (MinGW-w64, e.g. MSYS2 ucrt64 / winlibs) on PATH, and the VST3 SDK (MIT licence).
REM  The SDK is looked up in  %VST3_SDK_ROOT%  or  .\vst3sdk ; if missing and git is installed it is cloned.
REM ============================================================================
cd /d "%~dp0"

if /i "%1"=="clean" (
  rmdir /s /q build 2>nul & rmdir /s /q dist 2>nul
  echo cleaned. & goto :eof
)

where g++ >nul 2>nul
if errorlevel 1 (
  echo [error] g++ not found on PATH. Install MSYS2 or winlibs ^(MinGW-w64^) and reopen this window.
  echo         Alternative: use CMake + Visual Studio, see README.md.
  exit /b 1
)

set "SDK=%VST3_SDK_ROOT%"
if "%SDK%"=="" set "SDK=%~dp0vst3sdk"
if not exist "%SDK%\pluginterfaces" (
  where git >nul 2>nul
  if errorlevel 1 (
    echo [error] VST3 SDK not found at "%SDK%" and git is not installed.
    echo         Download https://github.com/steinbergmedia/vst3sdk ^(with submodules^) and set VST3_SDK_ROOT.
    exit /b 1
  )
  echo Cloning the VST3 SDK ^(one time, ~100 MB^)...
  git clone --depth 1 --recurse-submodules --shallow-submodules https://github.com/steinbergmedia/vst3sdk.git "%SDK%"
  if errorlevel 1 ( echo [error] SDK clone failed. & exit /b 1 )
)

if not exist build\obj mkdir build\obj
if not exist dist mkdir dist

set "SRCS="
for %%F in (
  base\source\baseiids base\source\fbuffer base\source\fdebug base\source\fobject base\source\fstreamer
  base\source\fstring base\source\updatehandler base\thread\source\fcondition base\thread\source\flock
  pluginterfaces\base\conststringtable pluginterfaces\base\coreiids pluginterfaces\base\funknown pluginterfaces\base\ustring
  public.sdk\source\common\commoniids public.sdk\source\common\pluginview
  public.sdk\source\main\pluginfactory public.sdk\source\main\dllmain
  public.sdk\source\vst\vstaudioeffect public.sdk\source\vst\vstbus public.sdk\source\vst\vstcomponent
  public.sdk\source\vst\vstcomponentbase public.sdk\source\vst\vsteditcontroller public.sdk\source\vst\vstinitiids
  public.sdk\source\vst\vstparameters
) do set "SRCS=!SRCS! %SDK%\%%F.cpp"
set "SRCS=%SRCS% src\vst3\deathaxe_vst3.cpp"

set CXXFLAGS=-std=c++17 -O2 -DRELEASE=1 -DNDEBUG -I%SDK% -fno-strict-aliasing -w

echo Compiling...
set OBJS=
for %%S in (%SRCS%) do (
  set "OBJ=build\obj\%%~nS.o"
  set "OBJS=!OBJS! !OBJ!"
  if not exist "!OBJ!" (
    g++ %CXXFLAGS% -c "%%S" -o "!OBJ!" || ( echo [error] failed: %%S & exit /b 1 )
  ) else if "%%~nS"=="deathaxe_vst3" (
    g++ %CXXFLAGS% -c "%%S" -o "!OBJ!" || ( echo [error] failed: %%S & exit /b 1 )
  )
)

echo Linking...
g++ -shared -static -static-libgcc -static-libstdc++ -o dist\DeathAxe.vst3 !OBJS! -lole32 -luuid -lshell32 -ladvapi32 -lgdi32 -lcomdlg32 -luser32
if errorlevel 1 ( echo [error] link failed. & exit /b 1 )

echo.
echo Built dist\DeathAxe.vst3

if /i "%1"=="install" (
  set "DEST=%CommonProgramFiles%\VST3"
  if not exist "!DEST!" mkdir "!DEST!"
  copy /y dist\DeathAxe.vst3 "!DEST!\DeathAxe.vst3" >nul
  if errorlevel 1 ( echo [error] copy failed - run this window as Administrator. & exit /b 1 )
  echo Installed to !DEST!\DeathAxe.vst3
) else (
  echo To install: copy dist\DeathAxe.vst3 to "%CommonProgramFiles%\VST3" ^(or run: build.bat install as Administrator^)
)
endlocal
