$videosDir = "Engine\Assets\videos"

Get-ChildItem -Path $videosDir -Filter "*.mp4" | ForEach-Object {
    $input = $_.FullName
    $output = [System.IO.Path]::ChangeExtension($_.FullName, ".mpg")

    Write-Host "Converting: $($_.Name) -> $([System.IO.Path]::GetFileName($output))"
    ffmpeg -y -i $input -c:v mpeg1video -b:v 8000k -q:v 1 -c:a mp2 -b:a 224k -f mpeg $output
    Write-Host ""
}

Write-Host "Done."
