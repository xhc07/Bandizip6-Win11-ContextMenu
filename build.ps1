[CmdletBinding()]
param(
    [switch]$Clean
)

$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSCommandPath
$Source = Join-Path $Root 'src'
$Dist = Join-Path $Root 'dist'
$Build = Join-Path $Root 'build'
$Package = Join-Path $Root 'package'

function Write-Step([string]$Text) {
    Write-Host "==> $Text" -ForegroundColor Cyan
}

function Find-Zig {
    $command = Get-Command zig -ErrorAction SilentlyContinue
    if ($command) { return $command.Source }

    $bundled = Join-Path $Root '..\tools\zig\ziglang\zig.exe'
    if (Test-Path $bundled) { return (Resolve-Path $bundled).Path }

    $local = Join-Path $Root 'tools\zig\ziglang\zig.exe'
    if (Test-Path $local) { return (Resolve-Path $local).Path }

    throw "Zig 0.13 or newer was not found. Install it from https://ziglang.org/download/ and add zig.exe to PATH, or place it at tools\zig\ziglang\zig.exe."
}

$Zig = Find-Zig
$ZigDir = Split-Path -Parent $Zig
$ZigLib = Join-Path $ZigDir 'lib'
$MingwInclude = Join-Path $ZigLib 'libc\include\any-windows-any'
if (-not (Test-Path $MingwInclude)) {
    throw "The Zig installation does not contain the MinGW headers expected at $MingwInclude."
}

if ($Clean) {
    Remove-Item $Build, $Dist -Recurse -Force -ErrorAction SilentlyContinue
}
New-Item -ItemType Directory -Force $Build, $Dist, (Join-Path $Dist 'Assets') | Out-Null

$env:ZIG_GLOBAL_CACHE_DIR = Join-Path $Build 'zig-cache-global'
$env:ZIG_LOCAL_CACHE_DIR = Join-Path $Build 'zig-cache'
$env:TEMP = Join-Path $Build 'tmp'
$env:TMP = $env:TEMP
New-Item -ItemType Directory -Force $env:TEMP | Out-Null

Write-Step "Building BandiZip11Menu.dll"
$dll = Join-Path $Build 'BandiZip11Menu.dll'
& $Zig c++ -target x86_64-windows-gnu -std=c++20 -O2 -shared -o $dll `
    (Join-Path $Source 'common.cpp') `
    (Join-Path $Source 'bandizip.cpp') `
    (Join-Path $Source 'command.cpp') `
    -lole32 -lshlwapi -luuid -ladvapi32 -lshell32 -luser32
if ($LASTEXITCODE -ne 0) { throw "Failed to build BandiZip11Menu.dll (exit $LASTEXITCODE)." }

Write-Step "Building BandiZip11Host.exe"
$hostExe = Join-Path $Build 'BandiZip11Host.exe'
& $Zig cc -target x86_64-windows-gnu -nostdlib "-isystem" $MingwInclude `
    -D_WIN32_WINNT=0x0A00 -DUNICODE -D_UNICODE `
    "-Wl,--subsystem,windows" "-Wl,-e,HostEntry" `
    -o $hostExe (Join-Path $Source 'host.c') -lkernel32 -luser32
if ($LASTEXITCODE -ne 0) { throw "Failed to build BandiZip11Host.exe (exit $LASTEXITCODE)." }

Write-Step "Assembling dist folder"
Copy-Item $dll $Dist -Force
Copy-Item $hostExe $Dist -Force
Copy-Item (Join-Path $Package 'AppxManifest.xml') $Dist -Force
Copy-Item (Join-Path $Package 'Assets\*') (Join-Path $Dist 'Assets') -Force

Write-Host ""
Write-Host "Build complete: $Dist" -ForegroundColor Green
Get-ChildItem $Dist -Recurse -File | Select-Object FullName, Length
