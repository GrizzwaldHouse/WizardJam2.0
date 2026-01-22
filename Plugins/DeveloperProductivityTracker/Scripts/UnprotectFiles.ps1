# ============================================================================
# UnprotectFiles.ps1
# Developer: Marcus Daley
# Date: January 2026
# Project: DeveloperProductivityTracker Plugin
# ============================================================================
# Purpose:
# Removes file protection attributes from plugin source files.
# Run this before editing protected files.
#
# Usage:
# .\UnprotectFiles.ps1 [-Path <directory>]
# ============================================================================

param(
    [Parameter(Mandatory=$false)]
    [string]$Path = $PSScriptRoot + "\..\Source"
)

$script:FilesUnprotected = 0
$script:FilesFailed = 0

function Write-Banner {
    Write-Host ""
    Write-Host "=============================================" -ForegroundColor Cyan
    Write-Host " Developer Productivity Tracker" -ForegroundColor Cyan
    Write-Host " File Unprotection Script" -ForegroundColor Cyan
    Write-Host "=============================================" -ForegroundColor Cyan
    Write-Host ""
}

function Unprotect-File {
    param(
        [string]$FilePath
    )

    if (-not (Test-Path $FilePath -Force)) {
        return
    }

    try {
        $file = Get-Item $FilePath -Force

        # Check if file has any protection attributes
        $hasProtection = ($file.Attributes -band [System.IO.FileAttributes]::ReadOnly) -or
                        ($file.Attributes -band [System.IO.FileAttributes]::Hidden) -or
                        ($file.Attributes -band [System.IO.FileAttributes]::System)

        if (-not $hasProtection) {
            return
        }

        # Remove all protection attributes
        $file.Attributes = $file.Attributes -band (-bnot [System.IO.FileAttributes]::ReadOnly)
        $file.Attributes = $file.Attributes -band (-bnot [System.IO.FileAttributes]::Hidden)
        $file.Attributes = $file.Attributes -band (-bnot [System.IO.FileAttributes]::System)

        $relativePath = $FilePath.Replace($Path, "").TrimStart("\", "/")
        Write-Host "  [OK] Unprotected: $relativePath" -ForegroundColor Green
        $script:FilesUnprotected++
    }
    catch {
        Write-Host "  [FAIL] Could not unprotect: $FilePath" -ForegroundColor Red
        Write-Host "         Error: $($_.Exception.Message)" -ForegroundColor Red
        $script:FilesFailed++
    }
}

function Unprotect-Directory {
    param(
        [string]$Directory,
        [string]$Pattern
    )

    if (-not (Test-Path $Directory)) {
        return
    }

    $files = Get-ChildItem -Path $Directory -Filter $Pattern -Recurse -File -Force
    foreach ($file in $files) {
        Unprotect-File -FilePath $file.FullName
    }
}

# Main execution
Write-Banner

Write-Host "Source Path: $Path" -ForegroundColor White
Write-Host ""

# Verify path exists
if (-not (Test-Path $Path)) {
    Write-Host "[ERROR] Source path not found: $Path" -ForegroundColor Red
    exit 1
}

Write-Host "Removing protection..." -ForegroundColor Cyan
Write-Host ""

# Unprotect all source files
Write-Host "Header files (.h):" -ForegroundColor Yellow
Unprotect-Directory -Directory $Path -Pattern "*.h"

Write-Host ""
Write-Host "Implementation files (.cpp):" -ForegroundColor Yellow
Unprotect-Directory -Directory $Path -Pattern "*.cpp"

Write-Host ""
Write-Host "Build files (.cs):" -ForegroundColor Yellow
Unprotect-Directory -Directory $Path -Pattern "*.cs"

# Summary
Write-Host ""
Write-Host "=============================================" -ForegroundColor Cyan
Write-Host " Unprotection Complete" -ForegroundColor Cyan
Write-Host "=============================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Files unprotected: $script:FilesUnprotected" -ForegroundColor Green

if ($script:FilesFailed -gt 0) {
    Write-Host "Files failed: $script:FilesFailed" -ForegroundColor Red
}

Write-Host ""
Write-Host "Files are now editable." -ForegroundColor Gray
Write-Host "Remember to re-protect with: .\ProtectFiles.ps1" -ForegroundColor Gray
Write-Host ""
