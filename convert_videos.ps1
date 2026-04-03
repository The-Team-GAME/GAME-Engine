$videosDir = "Engine\Assets\videos"

# Convert any .mp4 -> .mpg (mpeg1video)
Get-ChildItem -Path $videosDir -Filter "*.mp4" | ForEach-Object {
    $input = $_.FullName
    $output = [System.IO.Path]::ChangeExtension($_.FullName, ".mpg")

    Write-Host "Converting (mp4->mpg): $($_.Name) -> $([System.IO.Path]::GetFileName($output))"
    ffmpeg -y -i $input -c:v mpeg1video -b:v 8000k -q:v 1 -c:a mp2 -b:a 224k -f mpeg $output
    Write-Host ""
}

# Fix any .mpg files that are mpeg2video (pl_mpeg only supports mpeg1video)
Get-ChildItem -Path $videosDir -Filter "*.mpg" | ForEach-Object {
    $mpg = $_.FullName
    $codec = & ffprobe -v error -select_streams v:0 -show_entries stream=codec_name -of csv=p=0 $mpg 2>$null
    if ($codec -and $codec.Trim() -eq "mpeg2video") {
        $tmp = $mpg + ".tmp.mpg"
        Write-Host "Re-encoding mpeg2->mpeg1: $($_.Name)"
        ffmpeg -y -i $mpg -c:v mpeg1video -b:v 8000k -q:v 1 -c:a mp2 -b:a 224k -f mpeg $tmp
        Move-Item -Force $tmp $mpg
        Write-Host ""
    }
}

Write-Host "Done."