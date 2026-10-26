[CmdletBinding()]
param(
    [switch]$EnableDeveloperMode,
    [switch]$RestartExplorer
)

$ErrorActionPreference = 'Stop'

$PackageName = 'BandiZip11.ContextMenu'
$PackageDirName = 'BandiZip11'
$Clsid = '{4C1E9A76-2B3D-4F80-9E5A-7D6C1B2A3F40}'

$Root = Split-Path -Parent $PSCommandPath
$Dist = Join-Path $Root 'dist'
$InstallRoot = Join-Path $env:LOCALAPPDATA $PackageDirName
$PackageDir = Join-Path $InstallRoot 'Package'

function Write-Step([string]$Text) {
    Write-Host "==> $Text" -ForegroundColor Cyan
}

function Test-Administrator {
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($identity)
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Test-DeveloperMode {
    $key = 'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\AppModelUnlock'
    $value = (Get-ItemProperty -Path $key -Name AllowDevelopmentWithoutDevLicense -ErrorAction SilentlyContinue).AllowDevelopmentWithoutDevLicense
    return $value -eq 1
}

function Find-Bandizip {
    $keys = @(
        'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\Bandizip.exe',
        'HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\Bandizip.exe'
    )
    foreach ($key in $keys) {
        if (Test-Path $key) {
            $path = (Get-ItemProperty -Path $key).'(default)'
            if ($path -and (Test-Path $path)) { return $path }
        }
    }

    $candidates = @(Join-Path $env:ProgramFiles 'Bandizip\Bandizip.exe')
    $programFilesX86 = ${env:ProgramFiles(x86)}
    if ($programFilesX86) {
        $candidates += Join-Path $programFilesX86 'Bandizip\Bandizip.exe'
    }
    foreach ($path in $candidates) {
        if ($path -and (Test-Path $path)) { return $path }
    }
    return $null
}

Write-Host "BandiZip11 installer" -ForegroundColor Green
Write-Host "Windows 11 new context menu integration for Bandizip 6.x"
Write-Host ""

if ([Environment]::OSVersion.Version.Build -lt 22000) {
    throw "Windows 11 (build 22000 or newer) is required."
}

$bandizip = Find-Bandizip
if (-not $bandizip) {
    throw "Bandizip was not found. Install Bandizip 6.x first, then run this script again."
}
$version = (Get-Item $bandizip).VersionInfo.FileVersion
Write-Host "Bandizip: $bandizip ($version)"
if ($version -notmatch '^6\.') {
    Write-Warning "This tool targets Bandizip 6.x. The installed version is $version; command line behaviour may differ."
}

if (-not (Test-Path $Dist)) {
    throw "The dist folder is missing. Run build.ps1 first, or use a release package that includes dist."
}
foreach ($required in @('AppxManifest.xml', 'BandiZip11Menu.dll', 'BandiZip11Host.exe')) {
    if (-not (Test-Path (Join-Path $Dist $required))) {
        throw "Missing file: $(Join-Path $Dist $required)"
    }
}

if (-not (Test-DeveloperMode)) {
    Write-Warning "Windows Developer Mode is not enabled. Unsigned local packages normally require it."
    if ($EnableDeveloperMode) {
        if (-not (Test-Administrator)) {
            Write-Host "Requesting administrator rights to enable Developer Mode..."
            $arguments = "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`" -EnableDeveloperMode"
            Start-Process -FilePath 'powershell.exe' -Verb RunAs -ArgumentList $arguments | Out-Null
            return
        }
        Write-Step "Enabling Windows Developer Mode"
        $key = 'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\AppModelUnlock'
        New-Item -Path $key -Force | Out-Null
        Set-ItemProperty -Path $key -Name AllowDevelopmentWithoutDevLicense -Value 1 -Type DWord
        Write-Host "Developer Mode enabled." -ForegroundColor Green
    } else {
        Write-Host ""
        Write-Host "Enable it with one of these options, then run install.ps1 again:" -ForegroundColor Yellow
        Write-Host "  1. Settings > System > For developers > Developer Mode = On"
        Write-Host "  2. Run: powershell -ExecutionPolicy Bypass -File `"$PSCommandPath`" -EnableDeveloperMode"
        Write-Host ""
    }
}

Write-Step "Removing the previous package"
$existing = Get-AppxPackage -Name $PackageName -ErrorAction SilentlyContinue
if ($existing) {
    Remove-AppxPackage -Package $existing.PackageFullName -ErrorAction Stop
}

Write-Step "Copying files to $PackageDir"
if (Test-Path $PackageDir) {
    Remove-Item -Path $PackageDir -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $PackageDir | Out-Null
Copy-Item -Path (Join-Path $Dist 'AppxManifest.xml') -Destination $PackageDir
Copy-Item -Path (Join-Path $Dist 'BandiZip11Menu.dll') -Destination $PackageDir
Copy-Item -Path (Join-Path $Dist 'BandiZip11Host.exe') -Destination $PackageDir
Copy-Item -Path (Join-Path $Dist 'Assets') -Destination $PackageDir -Recurse

$manifest = Join-Path $PackageDir 'AppxManifest.xml'
Write-Step "Registering the package"
try {
    # -AllowUnsigned belongs to the -Path parameter set and cannot be used with -Register.
    Add-AppxPackage -Register $manifest -ErrorAction Stop
} catch {
    Write-Warning $_.Exception.Message
    if (-not (Test-DeveloperMode)) {
        Write-Host "Developer Mode is required for an unsigned local package." -ForegroundColor Yellow
        Write-Host "Enable it with: powershell -ExecutionPolicy Bypass -File `"$PSCommandPath`" -EnableDeveloperMode"
    }
    throw
}

$package = Get-AppxPackage -Name $PackageName -ErrorAction SilentlyContinue
if (-not $package) {
    throw "Package registration did not complete."
}

Write-Host ""
Write-Host "Installed: $($package.PackageFullName)" -ForegroundColor Green
Write-Host "Location : $($package.InstallLocation)"
Write-Host ""
Write-Host "Right-click a file, folder, or folder background in File Explorer."
Write-Host "If the new menu does not show Bandizip yet, restart File Explorer or sign out and back in."
Write-Host ""
Write-Host "Uninstall with: powershell -ExecutionPolicy Bypass -File `"$(Join-Path $Root 'uninstall.ps1')`""

if ($RestartExplorer) {
    Write-Step "Restarting File Explorer"
    Get-Process explorer -ErrorAction SilentlyContinue | Stop-Process -Force
    Start-Sleep -Seconds 2
    Start-Process explorer.exe
}
