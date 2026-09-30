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
$rawSignature = $ecdsa.SignHash($hash)
$ecdsa.Dispose()
$sha.Dispose()

function ConvertTo-DerInteger([byte[]]$value) {
  $first = 0
  while ($first -lt ($value.Length - 1) -and $value[$first] -eq 0) { $first++ }
  $part = New-Object byte[] ($value.Length - $first)
  [Array]::Copy($value, $first, $part, 0, $part.Length)
  if (($part[0] -band 0x80) -ne 0) {
    $withZero = New-Object byte[] ($part.Length + 1)
    [Array]::Copy($part, 0, $withZero, 1, $part.Length)
    $part = $withZero
  }
  return $part
}

$r = New-Object byte[] 32
$s = New-Object byte[] 32
[Array]::Copy($rawSignature, 0, $r, 0, 32)
[Array]::Copy($rawSignature, 32, $s, 0, 32)
$r = ConvertTo-DerInteger $r
$s = ConvertTo-DerInteger $s
$body = New-Object byte[] (4 + $r.Length + $s.Length)
$body[0] = 0x02; $body[1] = [byte]$r.Length
[Array]::Copy($r, 0, $body, 2, $r.Length)
$sPos = 2 + $r.Length
$body[$sPos] = 0x02; $body[$sPos + 1] = [byte]$s.Length
[Array]::Copy($s, 0, $body, $sPos + 2, $s.Length)
$signature = New-Object byte[] ($body.Length + 2)
$signature[0] = 0x30; $signature[1] = [byte]$body.Length
[Array]::Copy($body, 0, $signature, 2, $body.Length)

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
