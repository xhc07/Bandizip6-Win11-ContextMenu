[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'

$PackageName = 'BandiZip11.ContextMenu'
$Clsid = '{4C1E9A76-2B3D-4F80-9E5A-7D6C1B2A3F40}'
$InstallRoot = Join-Path $env:LOCALAPPDATA 'BandiZip11'

Write-Host "BandiZip11 uninstaller" -ForegroundColor Green

$package = Get-AppxPackage -Name $PackageName -ErrorAction SilentlyContinue
if ($package) {
    Write-Host "Removing package $($package.PackageFullName)..."
    try {
        Remove-AppxPackage -Package $package.PackageFullName -ErrorAction Stop
    } catch {
        Write-Warning "Could not remove the package: $($_.Exception.Message)"
        Write-Host "Close File Explorer and run this script again."
        throw
    }
} else {
    Write-Host "Package is not installed."
}

if (Test-Path $InstallRoot) {
    Remove-Item -Path $InstallRoot -Recurse -Force -ErrorAction SilentlyContinue
}

$clsidKey = "HKCU:\Software\Classes\CLSID\$Clsid"
if (Test-Path $clsidKey) {
    Remove-Item -Path $clsidKey -Recurse -Force -ErrorAction SilentlyContinue
}

Write-Host "BandiZip11 has been removed." -ForegroundColor Green
Write-Host "Existing Bandizip settings were not changed."
