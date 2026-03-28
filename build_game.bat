@echo off
setlocal enabledelayedexpansion

echo ========================================
echo Building Game Configuration
echo ========================================
echo.

:: Define file paths
set ENGINE_SETTINGS=Engine\Assets\settings\engine_settings.json
set GAME_SETTINGS=Engine\Assets\settings\game_settings.json
set ENGINE_BACKUP=%ENGINE_SETTINGS%.backup
set GAME_BACKUP=%GAME_SETTINGS%.backup

:: Create backups
echo Creating backups of settings files...
copy /Y "%ENGINE_SETTINGS%" "%ENGINE_BACKUP%" >nul
copy /Y "%GAME_SETTINGS%" "%GAME_BACKUP%" >nul

if not exist "%ENGINE_BACKUP%" (
    echo ERROR: Failed to create backup: %ENGINE_BACKUP%
    echo Check that %ENGINE_SETTINGS% exists and is writable.
    exit /b 1
)
if not exist "%GAME_BACKUP%" (
    echo ERROR: Failed to create backup: %GAME_BACKUP%
    echo Check that %GAME_SETTINGS% exists and is writable.
    exit /b 1
)

:: Modify engine_settings.json to set load_all_assets_on_startup to false
echo Configuring engine_settings.json...
powershell -Command "(Get-Content '%ENGINE_SETTINGS%') -replace '\"load_all_assets_on_startup\": true', '\"load_all_assets_on_startup\": false' | Set-Content '%ENGINE_SETTINGS%'"

:: Modify game_settings.json to set fullscreen to true
echo Configuring game_settings.json...
powershell -Command "(Get-Content '%GAME_SETTINGS%') -replace '\"fullscreen\": false', '\"fullscreen\": true' | Set-Content '%GAME_SETTINGS%'"

echo.
echo ========================================
echo Starting Build Process (Game Config)
echo ========================================
echo.

:: Build the Game configuration
set MSBUILD_EXE=

:: Prefer msbuild on PATH if available
where msbuild >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    for /f "delims=" %%i in ('where msbuild') do (
        set MSBUILD_EXE=%%i
        goto :msbuild_found
    )
)

:: Try to find MSBuild via vswhere (standard Visual Studio install)
set VSWHERE_EXE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe
if exist "%VSWHERE_EXE%" (
    for /f "usebackq delims=" %%i in (`"%VSWHERE_EXE%" -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\\Current\\Bin\\MSBuild.exe`) do (
        set MSBUILD_EXE=%%i
        goto :msbuild_found
    )
)

echo.
echo ERROR: Could not find MSBuild.
echo - If Visual Studio is installed, try running from a "Developer PowerShell" / "Developer Command Prompt".
echo - Or ensure %VSWHERE_EXE% exists.
echo.
set BUILD_RESULT=9009
goto :restore_settings

:msbuild_found
"%MSBUILD_EXE%" GAME.sln /p:Configuration=Game /p:Platform=x64

:: Store the build result
set BUILD_RESULT=%ERRORLEVEL%

echo.
echo ========================================
echo Restoring Original Settings
echo ========================================
echo.

:: Restore original settings
:restore_settings
echo Restoring engine_settings.json...
if exist "%ENGINE_BACKUP%" (
    copy /Y "%ENGINE_BACKUP%" "%ENGINE_SETTINGS%" >nul
    del "%ENGINE_BACKUP%"
) else (
    echo WARNING: Missing backup, cannot restore: %ENGINE_BACKUP%
)

echo Restoring game_settings.json...
if exist "%GAME_BACKUP%" (
    copy /Y "%GAME_BACKUP%" "%GAME_SETTINGS%" >nul
    del "%GAME_BACKUP%"
) else (
    echo WARNING: Missing backup, cannot restore: %GAME_BACKUP%
)

echo.
echo ========================================
if %BUILD_RESULT% EQU 0 (
    echo Build Completed Successfully!
) else (
    echo Build Failed with Error Code: %BUILD_RESULT%
)
echo ========================================
echo.

exit /b %BUILD_RESULT%
