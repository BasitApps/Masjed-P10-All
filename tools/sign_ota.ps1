param(
  [Parameter(Mandatory = $true)][string]$Firmware,
  [Parameter(Mandatory = $true)][string]$Profile,
  [Parameter(Mandatory = $true)][int]$Version,
  [string]$KeyFile = "$env:USERPROFILE\Desktop\Basit-ota-signing-key.pkcs8.b64",
  [string]$Output = "",
  [string]$SignedOutput = "",
  [string]$AssetOutput = ""
)

$bytes = [IO.File]::ReadAllBytes((Resolve-Path $Firmware))
$sha = [Security.Cryptography.SHA256]::Create()
$hash = $sha.ComputeHash($bytes)
$hashHex = ([BitConverter]::ToString($hash)).Replace('-', '').ToLowerInvariant()
$keyDer = [Convert]::FromBase64String((Get-Content -Raw $KeyFile).Trim())
$cng = [Security.Cryptography.CngKey]::Import(
  $keyDer, [Security.Cryptography.CngKeyBlobFormat]::Pkcs8PrivateBlob)
$ecdsa = New-Object Security.Cryptography.ECDsaCng($cng)
$signature = $ecdsa.SignHash($hash)
$ecdsa.Dispose()
$sha.Dispose()
# BearSSL's ESP8266 EC verifier expects the fixed-width raw P-256 form
# r||s (64 bytes), not ASN.1 DER.

$signedPath = if ($SignedOutput) { $SignedOutput } else { "$Firmware.signed.bin" }
$stream = [IO.File]::Open($signedPath, [IO.FileMode]::Create)
try {
  $stream.Write($bytes, 0, $bytes.Length)
  $stream.Write($signature, 0, $signature.Length)
  $length = [BitConverter]::GetBytes([uint32]$signature.Length)
  $stream.Write($length, 0, $length.Length)
} finally { $stream.Dispose() }

$signedSize = (Get-Item $signedPath).Length
$text = @(
  "profile=$Profile",
  "version=$Version",
  "size=$signedSize",
  "sha256=$hashHex"
) -join "`n"

if ($Output) { Set-Content -LiteralPath $Output -Value $text -Encoding ascii }
if ($AssetOutput) { Copy-Item -LiteralPath $signedPath -Destination $AssetOutput -Force }
if (!$Output) { $text; "signed=$signedPath" }
