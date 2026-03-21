$videosDir = "Engine\Assets\videos"

function Resolve-FfmpegPath {
    $cmd = Get-Command ffmpeg -ErrorAction SilentlyContinue
    if ($cmd) {
        return $cmd.Source
    }

    $pkgRoot = Join-Path $env:LOCALAPPDATA "Microsoft\WinGet\Packages"
    if (Test-Path $pkgRoot) {
        $found = Get-ChildItem -LiteralPath $pkgRoot -Recurse -Filter "ffmpeg.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($found) {
            return $found.FullName
        }
    }

    return $null
}

$ffmpegPath = Resolve-FfmpegPath
if (-not $ffmpegPath) {
    Write-Host "ffmpeg not found. Install it with:" -ForegroundColor Yellow
    Write-Host "  winget install --id Gyan.FFmpeg -e" -ForegroundColor Yellow
    Write-Host "Then restart your terminal/VS Code so PATH updates." -ForegroundColor Yellow
    exit 1
}

Get-ChildItem -Path $videosDir -Filter "*.mp4" | ForEach-Object {
    $input = $_.FullName
    $output = [System.IO.Path]::ChangeExtension($_.FullName, ".mpg")

    Write-Host "Converting: $($_.Name) -> $([System.IO.Path]::GetFileName($output))"
    & $ffmpegPath -y -i $input -c:v mpeg1video -b:v 8000k -q:v 1 -pix_fmt yuv420p -c:a mp2 -b:a 224k -ar 44100 -ac 2 -f mpeg $output
    Write-Host ""
}

Write-Host "Done."
