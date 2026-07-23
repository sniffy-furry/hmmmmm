# convert_ico.ps1 — wraps appicon.png in a minimal ICO container (single 256x256 PNG entry).
# Windows Vista+ ICO format supports embedded PNG chunks, so the OS can scale to 16/32/48px.
# Called by CMake custom command; output appicon.ico is embedded as ICON resource in the exe.

param(
    [string]$PngPath,
    [string]$IcoPath
)

if (-not (Test-Path $PngPath)) {
    Write-Error "PNG not found: $PngPath"
    exit 1
}

$pngBytes = [System.IO.File]::ReadAllBytes($PngPath)

$ms     = [System.IO.MemoryStream]::new()
$writer = [System.IO.BinaryWriter]::new($ms)

# ICONDIR (6 bytes)
$writer.Write([uint16]0)   # reserved
$writer.Write([uint16]1)   # type = 1 (icon)
$writer.Write([uint16]1)   # count = 1 entry

# ICONDIRENTRY (16 bytes)  — image offset = 6 + 16 = 22
$writer.Write([byte]0)     # width  (0 = 256)
$writer.Write([byte]0)     # height (0 = 256)
$writer.Write([byte]0)     # colorCount
$writer.Write([byte]0)     # reserved
$writer.Write([uint16]1)   # planes
$writer.Write([uint16]32)  # bitCount
$writer.Write([uint32]$pngBytes.Length)  # bytesInRes
$writer.Write([uint32]22)  # imageOffset

# Raw PNG bytes (Windows reads this directly as a PNG-in-ICO entry)
$writer.Write($pngBytes)
$writer.Flush()

[System.IO.File]::WriteAllBytes($IcoPath, $ms.ToArray())
$ms.Dispose()

Write-Host "Created $IcoPath ($($ms.Length) bytes from $($pngBytes.Length)-byte PNG)"
