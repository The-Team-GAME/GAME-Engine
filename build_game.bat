@echo off
setlocal enabledelayedexpansion

echo ========================================
echo Building Game Configuration
echo ========================================
echo.

:: Define file paths
set ENGINE_SETTINGS=MonkeyEngine\Assets\settings\engine_settings.json
set GAME_SETTINGS=MonkeyEngine\Assets\settings\game_settings.json
set ENGINE_BACKUP=%ENGINE_SETTINGS%.backup
set GAME_BACKUP=%GAME_SETTINGS%.backup

:: Create backups
echo Creating backups of settings files...
copy /Y "%ENGINE_SETTINGS%" "%ENGINE_BACKUP%" >nul
copy /Y "%GAME_SETTINGS%" "%GAME_BACKUP%" >nul

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
msbuild Games\MonkeyStoleMyGPA\MonkeyStoleMyGPA.sln /p:Configuration=Game /p:Platform=x64

:: Store the build result
set BUILD_RESULT=%ERRORLEVEL%

echo.
echo ========================================
echo Restoring Original Settings
echo ========================================
echo.

:: Restore original settings
echo Restoring engine_settings.json...
copy /Y "%ENGINE_BACKUP%" "%ENGINE_SETTINGS%" >nul
del "%ENGINE_BACKUP%"

echo Restoring game_settings.json...
copy /Y "%GAME_BACKUP%" "%GAME_SETTINGS%" >nul
del "%GAME_BACKUP%"

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
