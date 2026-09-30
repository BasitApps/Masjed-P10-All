param(
  [ValidateSet('1x1', '2x1', '3x1', '4x1')]
  [string]$Profile = '4x1',
  [string]$AndroidProject = "$env:USERPROFILE\Desktop\MasjedClockWiFi"
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$envName = "p10_${Profile}_release"
$pio = (Get-Command pio -ErrorAction SilentlyContinue).Source
if (!$pio) { $pio = Join-Path $env:USERPROFILE '.platformio\penv\Scripts\pio.exe' }
if (!(Test-Path $pio)) { throw "PlatformIO not found: $pio" }
Push-Location $root
try {
  & $pio run -e $envName
  if ($LASTEXITCODE -ne 0) { throw "PlatformIO build failed: $envName" }

  $firmware = Join-Path $root ".pio\build\$envName\updateP104_${envName}.bin"
  if (!(Test-Path $firmware)) { throw "Build output not found: $firmware" }

  $assetDir = Join-Path $AndroidProject 'app\src\main\assets'
  if (!(Test-Path $assetDir)) { throw "Android assets directory not found: $assetDir" }
  $asset = Join-Path $assetDir 'updateP104.bin'
  # The customer app posts this raw image to /updateMain. Never append a
  # signature trailer here; that path is intentionally kept backward compatible.
  Copy-Item -LiteralPath $firmware -Destination $asset -Force

  Write-Output "release=$envName"
  Write-Output "firmware=$firmware"
  Write-Output "androidAsset=$asset"
  Write-Output "bytes=$((Get-Item -LiteralPath $asset).Length)"
}
finally { Pop-Location }
