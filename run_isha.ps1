# ISHA AI PowerShell Launcher
$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot
Write-Host "Starting ISHA AI Engine and Google Stitch UI..." -ForegroundColor Cyan
python run_isha.py
