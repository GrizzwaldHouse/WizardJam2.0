# ============================================================================
# SetupPermissions.ps1
# Developer: Marcus Daley
# Date: January 2026
# Project: DeveloperProductivityTracker Plugin
# ============================================================================
# Purpose:
# Sets up NTFS permissions for plugin directories.
# Optional advanced protection using ACLs.
# Requires administrator privileges.
#
# Usage:
# Run as Administrator:
# .\SetupPermissions.ps1 [-Path <directory>] [-Restrictive]
# ============================================================================

param(
    [Parameter(Mandatory=$false)]
    [string]$Path = $PSScriptRoot + "\..",

    [Parameter(Mandatory=$false)]
    [switch]$Restrictive = $false
)

# Check for administrator privileges
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

function Write-Banner {
    Write-Host ""
    Write-Host "=============================================" -ForegroundColor Cyan
    Write-Host " Developer Productivity Tracker" -ForegroundColor Cyan
    Write-Host " Permission Setup Script" -ForegroundColor Cyan
    Write-Host "=============================================" -ForegroundColor Cyan
    Write-Host ""
}

function Set-DirectoryPermissions {
    param(
        [string]$Directory,
        [bool]$IsRestrictive
    )

    if (-not (Test-Path $Directory)) {
        Write-Host "  [SKIP] Directory not found: $Directory" -ForegroundColor Yellow
        return
    }

    try {
        $acl = Get-Acl $Directory

        if ($IsRestrictive) {
            # Restrictive mode: Only current user has full control
            # Remove inheritance
            $acl.SetAccessRuleProtection($true, $false)

            # Clear existing rules
            $acl.Access | ForEach-Object { $acl.RemoveAccessRule($_) } | Out-Null

            # Add current user with full control
            $currentUser = [System.Security.Principal.WindowsIdentity]::GetCurrent().Name
            $rule = New-Object System.Security.AccessControl.FileSystemAccessRule(
                $currentUser,
                "FullControl",
                "ContainerInherit,ObjectInherit",
                "None",
                "Allow"
            )
            $acl.AddAccessRule($rule)

            # Add SYSTEM with read access (for backups, etc.)
            $systemRule = New-Object System.Security.AccessControl.FileSystemAccessRule(
                "SYSTEM",
                "ReadAndExecute",
                "ContainerInherit,ObjectInherit",
                "None",
                "Allow"
            )
            $acl.AddAccessRule($systemRule)
        }
        else {
            # Standard mode: Ensure current user has full control
            $currentUser = [System.Security.Principal.WindowsIdentity]::GetCurrent().Name
            $rule = New-Object System.Security.AccessControl.FileSystemAccessRule(
                $currentUser,
                "FullControl",
                "ContainerInherit,ObjectInherit",
                "None",
                "Allow"
            )
            $acl.AddAccessRule($rule)
        }

        Set-Acl $Directory $acl

        $relativePath = $Directory.Replace($Path, "").TrimStart("\", "/")
        if ([string]::IsNullOrEmpty($relativePath)) { $relativePath = "[Root]" }
        Write-Host "  [OK] Permissions set: $relativePath" -ForegroundColor Green
    }
    catch {
        Write-Host "  [FAIL] Could not set permissions: $Directory" -ForegroundColor Red
        Write-Host "         Error: $($_.Exception.Message)" -ForegroundColor Red
    }
}

# Main execution
Write-Banner

if (-not $isAdmin) {
    Write-Host "[WARNING] Not running as Administrator" -ForegroundColor Yellow
    Write-Host "          Some permission changes may fail." -ForegroundColor Yellow
    Write-Host "          For full functionality, run as Administrator." -ForegroundColor Yellow
    Write-Host ""
}

Write-Host "Plugin Path: $Path" -ForegroundColor White
Write-Host "Mode: $(if ($Restrictive) { 'Restrictive' } else { 'Standard' })" -ForegroundColor White
Write-Host ""

# Verify path exists
if (-not (Test-Path $Path)) {
    Write-Host "[ERROR] Plugin path not found: $Path" -ForegroundColor Red
    exit 1
}

Write-Host "Setting up permissions..." -ForegroundColor Cyan
Write-Host ""

# Set permissions on key directories
$directories = @(
    $Path,
    "$Path\Source",
    "$Path\Source\DeveloperProductivityTracker",
    "$Path\Source\DeveloperProductivityTracker\Public",
    "$Path\Source\DeveloperProductivityTracker\Private",
    "$Path\Content",
    "$Path\Scripts"
)

foreach ($dir in $directories) {
    Set-DirectoryPermissions -Directory $dir -IsRestrictive $Restrictive
}

# Summary
Write-Host ""
Write-Host "=============================================" -ForegroundColor Cyan
Write-Host " Permission Setup Complete" -ForegroundColor Cyan
Write-Host "=============================================" -ForegroundColor Cyan
Write-Host ""

if ($Restrictive) {
    Write-Host "Restrictive permissions applied." -ForegroundColor Yellow
    Write-Host "Only your user account has full access." -ForegroundColor Yellow
}
else {
    Write-Host "Standard permissions applied." -ForegroundColor Green
}

Write-Host ""
Write-Host "Additional protection available:" -ForegroundColor Gray
Write-Host "  - Run .\ProtectFiles.ps1 to set file attributes" -ForegroundColor Gray
Write-Host "  - Use -Restrictive flag for tighter ACL control" -ForegroundColor Gray
Write-Host ""
