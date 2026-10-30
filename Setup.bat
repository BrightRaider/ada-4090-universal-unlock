@echo off
setlocal
cd /d "%~dp0"
title Ada 4090 Universal Unlock Setup
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Setup-AdaUnlock.ps1"
if errorlevel 1 pause
