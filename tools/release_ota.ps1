param(
  [ValidateSet('1x1', '2x1', '3x1', '4x1', 'All')]
  [string]$Profile,
  [Parameter(Mandatory = $true)]
  [string]$AndroidProject
)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSHOME 'Modules\Microsoft.PowerShell.Utility') -ErrorAction Stop
$root = Split-Path -Parent $PSScriptRoot
if (!$Profile) {
  Write-Host 'Prepare Android firmware (no clock upload, no APK build)'
  Write-Host '1 = P101 (1x1)   2 = P102 (2x1)   3 = P103 (3x1)   4 = P104 (4x1)'
  $choice = (Read-Host 'Choose 1-4, or A for all four models').Trim()
  $Profile = switch ($choice) {
    '1' { '1x1' }
    '2' { '2x1' }
    '3' { '3x1' }
    '4' { '4x1' }
    'A' { 'All' }
    default { throw 'Invalid choice; no Android files changed.' }
  }
}
$profiles = if ($Profile -eq 'All') { @('1x1', '2x1', '3x1', '4x1') } else { @($Profile) }
$assetDir = Join-Path $AndroidProject 'app\src\main\assets'
if (!(Test-Path -LiteralPath $assetDir -PathType Container)) {
  throw "Android assets directory not found: $assetDir"
}
$assetDir = (Resolve-Path -LiteralPath $assetDir).ProviderPath
$pio = (Get-Command pio -ErrorAction SilentlyContinue).Source
if (!$pio) { $pio = Join-Path $env:USERPROFILE '.platformio\penv\Scripts\pio.exe' }
if (!(Test-Path $pio)) { throw "PlatformIO not found: $pio" }
Push-Location $root
try {
  $buildArgs = @('run')
  foreach ($size in $profiles) { $buildArgs += @('-e', "p10_${size}_release") }
  & $pio @buildArgs
  if ($LASTEXITCODE -ne 0) { throw 'PlatformIO build failed; no Android files changed.' }

  # Validate every selected output before replacing any Android asset.
  $outputs = foreach ($size in $profiles) {
    $envName = "p10_${size}_release"
    $name = "updateP10$($size.Substring(0, 1)).bin"
    $firmware = Join-Path $root ".pio\build\$envName\$name"
    if (!(Test-Path -LiteralPath $firmware -PathType Leaf)) { throw "Build output not found: $firmware" }
    [PSCustomObject]@{ Firmware = $firmware; Asset = (Join-Path $assetDir $name) }
  }

  foreach ($output in $outputs) {
    # Commercial OTA expects raw BIN bytes, with no signature trailer.
    Copy-Item -LiteralPath $output.Firmware -Destination $output.Asset -Force
    if ((Get-FileHash -LiteralPath $output.Asset).Hash -ne (Get-FileHash -LiteralPath $output.Firmware).Hash) {
      throw "Android asset hash mismatch: $($output.Asset)"
    }
    Write-Output "firmware=$($output.Firmware)"
    Write-Output "androidAsset=$($output.Asset)"
    Write-Output "bytes=$((Get-Item -LiteralPath $output.Asset).Length)"
  }
  Write-Host 'Done. Rebuild the app in Android Studio to include these firmware files.'
}
finally { Pop-Location }
