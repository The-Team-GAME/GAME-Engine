param(
    [ValidateSet("Debug", "Game", "Release")]
    [string]$Configuration = "Game",

    [string]$Platform = "x64"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Get-MSBuildPath {
    $msbuildCommand = Get-Command MSBuild.exe -ErrorAction SilentlyContinue
    if ($msbuildCommand) {
        return $msbuildCommand.Source
    }

    $vswherePath = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswherePath) {
        $installationPath = & $vswherePath -latest -requires Microsoft.Component.MSBuild -property installationPath
        if ($LASTEXITCODE -eq 0 -and -not [string]::IsNullOrWhiteSpace($installationPath)) {
            $candidatePath = Join-Path $installationPath "MSBuild\Current\Bin\MSBuild.exe"
            if (Test-Path $candidatePath) {
                return $candidatePath
            }
        }
    }

    throw "MSBuild.exe was not found. Run this from a Developer PowerShell or install Visual Studio Build Tools."
}

function Write-JsonFile {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,

        [Parameter(Mandatory = $true)]
        [object]$Data
    )

    $json = $Data | ConvertTo-Json -Depth 10
    $encoding = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($Path, $json + [Environment]::NewLine, $encoding)
}

$repoRoot = $PSScriptRoot
$solutionPath = Join-Path $repoRoot "GAME.sln"
$engineSettingsPath = Join-Path $repoRoot "Engine\Assets\settings\engine_settings.json"
$gameSettingsPath = Join-Path $repoRoot "Engine\Assets\settings\game_settings.json"

foreach ($path in @($solutionPath, $engineSettingsPath, $gameSettingsPath)) {
    if (-not (Test-Path $path)) {
        throw "Required file not found: $path"
    }
}

$msbuildPath = Get-MSBuildPath

Write-Host "Building $solutionPath ($Configuration|$Platform)..."
& $msbuildPath $solutionPath "/m" "/nologo" "/t:Build" "/p:Configuration=$Configuration" "/p:Platform=$Platform"
if ($LASTEXITCODE -ne 0) {
    throw "Build failed with exit code $LASTEXITCODE."
}

$engineSettings = Get-Content -Raw $engineSettingsPath | ConvertFrom-Json
$engineSettings.load_all_assets_on_startup = $false
Write-JsonFile -Path $engineSettingsPath -Data $engineSettings

$gameSettings = Get-Content -Raw $gameSettingsPath | ConvertFrom-Json
$gameSettings.fullscreen = $true
$gameSettings.bgm_volume = [double]1.0
$gameSettings.sfx_volume = [double]1.0
$gameSettings.vsync = $true
Write-JsonFile -Path $gameSettingsPath -Data $gameSettings

Write-Host "Build completed successfully."
Write-Host "Updated Engine/Assets/settings/engine_settings.json and Engine/Assets/settings/game_settings.json."