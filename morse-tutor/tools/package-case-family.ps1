$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$mechanical = Join-Path $root 'mechanical'
& node (Join-Path $PSScriptRoot 'validate-models.mjs') --family
if ($LASTEXITCODE -ne 0) { throw 'Family mesh validation failed; no archives generated' }
$catalog = Get-Content (Join-Path $mechanical 'display-profiles.json') -Raw | ConvertFrom-Json
$downloads = Join-Path $mechanical 'downloads'
New-Item -ItemType Directory -Force $downloads | Out-Null
$shared = @('bricohams-paddle_left.stl','bricohams-paddle_right.stl','bricohams-clamp.stl','bricohams-fit_coupon.stl') |
    ForEach-Object { Join-Path (Join-Path $mechanical 'stl') $_ }
$sources = @('bricohams.scad','display-profiles.json','README.md','family-validation.json') |
    ForEach-Object { Join-Path $mechanical $_ }
foreach ($profile in $catalog.profiles) {
    $files = Get-ChildItem (Join-Path (Join-Path $mechanical 'family') $profile.id) -File
    Compress-Archive -Path (@($files.FullName) + $shared + $sources) -DestinationPath (Join-Path $downloads ($profile.id + '.zip')) -Force
}
Compress-Archive -Path (@((Join-Path $mechanical 'family')) + $shared + $sources) -DestinationPath (Join-Path $downloads 'bricohams-display-family.zip') -Force
Write-Output "Packaged $($catalog.profiles.Count) individual archives and complete family archive"
