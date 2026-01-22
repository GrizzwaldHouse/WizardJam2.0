# ============================================================================
# ProtectFiles.ps1
# Developer: Marcus Daley
# Date: January 2026
# Project: DeveloperProductivityTracker Plugin
# ============================================================================
# Purpose:
# Applies file protection attributes to plugin source files.
# Three protection levels for different file categories.
#
# Usage:
# .\ProtectFiles.ps1 [-Level 1|2|3] [-Path <directory>]
# ============================================================================

param(
    [Parameter(Mandatory=$false)]
    [ValidateSet(1, 2, 3)]
    [int]$Level = 2,

    [Parameter(Mandatory=$false)]
    [string]$Path = $PSScriptRoot + "\..\Source"
)

# Protection level definitions
# Level 1: +r (Read-only) - Headers, documentation
# Level 2: +h +r (Hidden + Read-only) - Standard implementations
# Level 3: +s +h +r (System + Hidden + Read-only) - Core subsystems

$script:FilesProtected = 0
$script:FilesFailed = 0

function Write-Banner {
    Write-Host ""
    Write-Host "=============================================" -ForegroundColor Cyan
    Write-Host " Developer Productivity Tracker" -ForegroundColor Cyan
    Write-Host " File Protection Script" -ForegroundColor Cyan
    Write-Host "=============================================" -ForegroundColor Cyan
    Write-Host ""
}

function Protect-File {
    param(
        [string]$FilePath,
        [int]$ProtectionLevel
    )

    if (-not (Test-Path $FilePath)) {
        Write-Host "  [SKIP] File not found: $FilePath" -ForegroundColor Yellow
        return
    }

    try {
        $file = Get-Item $FilePath -Force

        switch ($ProtectionLevel) {
            1 {
                # Level 1: Read-only only
                $file.Attributes = $file.Attributes -bor [System.IO.FileAttributes]::ReadOnly
            }
            2 {
                # Level 2: Hidden + Read-only
                $file.Attributes = $file.Attributes -bor [System.IO.FileAttributes]::ReadOnly
                $file.Attributes = $file.Attributes -bor [System.IO.FileAttributes]::Hidden
            }
            3 {
                # Level 3: System + Hidden + Read-only
                $file.Attributes = $file.Attributes -bor [System.IO.FileAttributes]::ReadOnly
                $file.Attributes = $file.Attributes -bor [System.IO.FileAttributes]::Hidden
                $file.Attributes = $file.Attributes -bor [System.IO.FileAttributes]::System
            }
        }

        $relativePath = $FilePath.Replace($Path, "").TrimStart("\", "/")
        Write-Host "  [OK] Protected (L$ProtectionLevel): $relativePath" -ForegroundColor Green
        $script:FilesProtected++
    }
    catch {
        Write-Host "  [FAIL] Could not protect: $FilePath" -ForegroundColor Red
        Write-Host "         Error: $($_.Exception.Message)" -ForegroundColor Red
        $script:FilesFailed++
    }
}

function Protect-Directory {
    param(
        [string]$Directory,
        [string]$Pattern,
        [int]$ProtectionLevel
    )

    if (-not (Test-Path $Directory)) {
        Write-Host "  [SKIP] Directory not found: $Directory" -ForegroundColor Yellow
        return
    }

    $files = Get-ChildItem -Path $Directory -Filter $Pattern -Recurse -File -Force
    foreach ($file in $files) {
        Protect-File -FilePath $file.FullName -ProtectionLevel $ProtectionLevel
    }
}

# Main execution
Write-Banner

Write-Host "Protection Level: $Level" -ForegroundColor White
Write-Host "Source Path: $Path" -ForegroundColor White
Write-Host ""

# Verify path exists
if (-not (Test-Path $Path)) {
    Write-Host "[ERROR] Source path not found: $Path" -ForegroundColor Red
    exit 1
}

Write-Host "Applying protection..." -ForegroundColor Cyan
Write-Host ""

# Level 1 files: Headers and documentation
Write-Host "Level 1 files (Read-only):" -ForegroundColor Yellow
Protect-Directory -Directory $Path -Pattern "*.h" -ProtectionLevel ([Math]::Min($Level, 1))

# Level 2 files: Standard implementations
if ($Level -ge 2) {
    Write-Host ""
    Write-Host "Level 2 files (Hidden + Read-only):" -ForegroundColor Yellow
    Protect-Directory -Directory "$Path\DeveloperProductivityTracker\Private\External" -Pattern "*.cpp" -ProtectionLevel 2
    Protect-Directory -Directory "$Path\DeveloperProductivityTracker\Private\Wellness" -Pattern "*.cpp" -ProtectionLevel 2
    Protect-Directory -Directory "$Path\DeveloperProductivityTracker\Private\Visualization" -Pattern "*.cpp" -ProtectionLevel 2
    Protect-Directory -Directory "$Path\DeveloperProductivityTracker\Private\UI" -Pattern "*.cpp" -ProtectionLevel 2
}

# Level 3 files: Core subsystems and security
if ($Level -ge 3) {
    Write-Host ""
    Write-Host "Level 3 files (System + Hidden + Read-only):" -ForegroundColor Yellow
    Protect-Directory -Directory "$Path\DeveloperProductivityTracker\Private\Core" -Pattern "*.cpp" -ProtectionLevel 3
    Protect-File -FilePath "$Path\DeveloperProductivityTracker\Private\DeveloperProductivityTrackerModule.cpp" -ProtectionLevel 3
}

# Summary
Write-Host ""
Write-Host "=============================================" -ForegroundColor Cyan
Write-Host " Protection Complete" -ForegroundColor Cyan
Write-Host "=============================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Files protected: $script:FilesProtected" -ForegroundColor Green

if ($script:FilesFailed -gt 0) {
    Write-Host "Files failed: $script:FilesFailed" -ForegroundColor Red
}

Write-Host ""
Write-Host "To remove protection, run: .\UnprotectFiles.ps1" -ForegroundColor Gray
Write-Host ""
